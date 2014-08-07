/*
-------------------------------------------------------------------------------
coreIPM/timer.c

Author: Gokhan Sozmen
-------------------------------------------------------------------------------
Copyright (C) 2007-2012 Gokhan Sozmen
-------------------------------------------------------------------------------
coreIPM is free software; you can redistribute it and/or modify it under the 
terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later 
version.

coreIPM is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with 
coreIPM; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA 02110-1301, USA.
-------------------------------------------------------------------------------
See http://www.coreipm.com for documentation, latest information, licensing, 
support and contact details.
-------------------------------------------------------------------------------
*/
#include "common.h"

/*------------------------------------------------------------------------------
 * exported globals.
 */
UINT32 lbolt, ActivityLEDtime;
CQE	cq_array[CQ_ARRAY_SIZE];

/*------------------------------------------------------------------------------*/
/* Local Function Prototypes                                                    */
/*------------------------------------------------------------------------------*/
void cq_init( void );
CQE *cq_alloc( void* handle, UINT8* reuse );
void cq_free( CQE *cqe );
CQE *cq_get_expired_elem( bool );
void cq_set_cqe_state( CQE *cqe, UINT8 state );

/*------------------------------------------------------------------------------
 * timer_initialize()
 */
void
timer_initialize( void ) 
{
	timer_initialize_platform();
	cq_init();
} // timer_initialize

/*------------------------------------------------------------------------------
 * timer_add_callout_queue( ... )
 */
UINT8
timer_add_callout_queue(
    void *handle,
    UINT32 ticks,
    void (*func) (UINT8*),                      // callback fn and its context (arg)
    UINT8 *arg )                                // arg for timer procedure
{
        CQE *cqe;

        cqe = cq_alloc(handle, arg );           // check twins there
        if( cqe ) {
                cqe->tick = ticks    + lbolt;
                if ((arg) || (cqe->func != func)) {
                        cqe->arg = arg;         // if not reuse then spoil / set new.
                        cqe->func = func;
                        cqe->handle = handle;   // this is a pointer: UINT16 at least.
                }
                return( ESUCCESS );
        }
        TheBug();    
        global_last_error = THE_BUG_CQ_ARRAY_FULL;
        return( ENOMEM );
} // timer_add_callout_queue

/*------------------------------------------------------------------------------
 * timer_remove_callout_queue()
 *      Search the callout queue using the handle and remove the
 *      entry if found.
 */
void
timer_remove_callout_queue( void *handle )
{
    CQE *cqe;
    UINT8 i;
    for ( i = 0; i < CQ_ARRAY_SIZE; i++ )
    {
        cqe = &cq_array[i];
        if( ( cqe->cqe_state == CQE_ACTIVE ) && ( cqe->handle == handle ) ) {
            cq_free(cqe);
            break;
//            if ( ((void *)&(current_i2c_context->state_transition_timer)) ==
//                 cqe->handle ) break;
        }
    }
} // timer_remove_callout_queue

/*------------------------------------------------------------------------------
 * timer_reset_callout_queue() 
 * 	Search the callout queue using the handle and reset the
 * 	timeout value with the new value passed in ticks.
 *------------------------------------------------------------------------------*/
void
timer_reset_callout_queue(
	void *handle,
	UINT32 ticks )
{
	CQE *cqe;
	UINT8 i;
	// UINT32 interrupt_mask = CURRENT_INTERRUPT_MASK;

	// DISABLE_INTERRUPTS;	
	for ( i = 0; i < CQ_ARRAY_SIZE; i++ )
	{
		cqe = &cq_array[i];
		if( ( cqe->cqe_state == CQE_ACTIVE ) && ( cqe->handle == handle ) ) {
			cqe->tick = ticks + lbolt;
			break;
		}
	}
	// ENABLE_INTERRUPTS( interrupt_mask );
} // timer_reset_callout_queue


/*------------------------------------------------------------------------------
 * timer_get_expiration_time() 
 * 	Get ticks to expiration.
 * 
 *------------------------------------------------------------------------------*/
UINT32
timer_get_expiration_time(
	void *handle ) 
{
	CQE *cqe;
	UINT8 i;

	for ( i = 0; i < CQ_ARRAY_SIZE; i++ )
	{
		cqe = &cq_array[i];
		if( ( cqe->cqe_state == CQE_ACTIVE ) && ( cqe->handle == handle ) ) {
			if( cqe->tick > lbolt )
				return( cqe->tick - lbolt );
			break;
		}
	}
	return( 0 );
} // timer_get_expiration_time
		
/*------------------------------------------------------------------------------
 * timer_process_callout_queue()
 *      Get first expired entry, invoke it's callback and remove
 *      from queue.
 *------------------------------------------------------------------------------*/
void
timer_process_callout_queue( void )
{
    CQE *cqe;

    cqe = cq_get_expired_elem(false);  if( NULL != cqe ) {
        cq_set_cqe_state( cqe, CQE_PENDING ); // mark it pending before last use, so..
        if (cqe->func){ // some callbacks have no argument. (ex: ping callback)
            (*cqe->func)( cqe->arg ); // <-- this callback could use/reallocate it safely.
        }else{
            TheBug();                         // timer with no callback function.
            global_last_error = THE_BUG_CQE_NO_CALLBACK;
        }
        if ( CQE_PENDING == cqe->cqe_state )  // if not reused (still pending ..
                            cq_free(cqe);     // .. down), kill timer
    }
} // timer_process_callout_queue

/*------------------------------------------------------------------------------*
 * CALLOUT QUEUE MANAGEMENT
 */

/*------------------------------------------------------------------------------
 * cq_init()
 *
 * initialize cq structures
 */
void
cq_init( void ) {
    UINT8 i;
    for ( i = 0; i < CQ_ARRAY_SIZE; i++ )
    {
        cq_array[i].cqe_state  = CQE_FREE;
        cq_array[i].handle = 0;
        cq_array[i].func   = 0;
        cq_array[i].arg    = 0;
    }
} // cq_init

//------------------------------------------------------------------------------
// cq_alloc( handle, reuse )
//
// get a free callout queue entry (CQE)
// reuse: if handles are same and arguments equal or default (NULL), then 
// reuse old (do not allocate new) CQE.
CQE *
cq_alloc( void* handle, UINT8* reuse )
{
    CQE *cqe = 0;
    CQE *ptr = cq_array;
    UINT8 i;

    for ( i = 0; i < CQ_ARRAY_SIZE; i++ )
    {
        ptr = &cq_array[i];
        if ( ptr->handle == handle ){       // reuse?
                if((NULL == reuse)||( ptr->arg == reuse)) { 
                        cqe = ptr;          //  reuse CQE
                        break;
                }
        }
        if( ptr->cqe_state == CQE_FREE ) {  // free entry?
            cqe = ptr;                      // remember it for "no twins" case,
        }                                   // but not yet report.    
    }
    if (NULL != cqe){
        cqe->tick = 0;
        cqe->cqe_state = CQE_ACTIVE;
    } else global_error_flags |= CQ_FULL_FLAG;
    return cqe;
} // cq_alloc

//------------------------------------------------------------------------------
// cq_free() --  free callout queue entry
//
void
cq_free( CQE *cqe )
{
    cqe->cqe_state  = CQE_FREE;
    cqe->handle    = 0;
    // cqe->arg    = 0;
} // cq_free


//------------------------------------------------------------------------------
// cq_get_expired_elem()
//
CQE *
cq_get_expired_elem( bool needDisInt )
{
    CQE *cqe = 0;
    CQE *ptr = cq_array;
    UINT8 i;

    if (needDisInt){
        DISABLE_INTERRUPTS    
    }
    for ( i = 0; i < CQ_ARRAY_SIZE; i++ )
    {
        ptr = &cq_array[i];
        if( (ptr->cqe_state == CQE_ACTIVE) && (ptr->tick) && (lbolt >= ptr->tick) ){
            cqe = ptr;
            break;
        }
    }
    if (needDisInt){
        ENABLE_INTERRUPTS
    }
    return cqe;
}// cq_get_expired_elem

//------------------------------------------------------------------------------
// cq_set_cqe_state()
//
void
cq_set_cqe_state( CQE *cqe, UINT8 state )
{
	cqe->cqe_state = state;
} // cq_set_cqe_state

//------------------------------------------------------------------------------
// cq_get_locked_total()
//
UINT8
cq_get_locked_total( void )
{
    UINT8 i, j;
    for ( i=0, j=0; j < CQ_ARRAY_SIZE; j++ ) {
        if ( CQE_FREE != cq_array[j].cqe_state )    i++;
    }
    return (i);
} // cq_get_locked_total
//------------------------------------------------------------------------------
// cq_get_locked_total()
//
UINT32 
cq_get_time_left( void* handle )
{
        int i;
        for ( i=0; i < CQ_ARRAY_SIZE; i++ ) {
                if ( CQE_FREE != cq_array[i].cqe_state )
                        if(handle == cq_array[i].handle)
                                return(cq_array[i].tick - lbolt);
        }
        return 0;
} // cq_get_time_left


/* end of document: timer.c                                                     */
/*------------------------------------------------------------------------------*/

