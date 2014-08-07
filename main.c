/*
-------------------------------------------------------------------------------
coreIPM/main.c

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
#include <common.h>
#include <sw_download.h>
#include <wd.h>

/*-----------------------------------------------------------------------------
 * main()
 *-----------------------------------------------------------------------------*/
int main()
{
        TRY_UPDATE_CODE

        /* main_start: initialize system */
        ws_init();
        iopin_initialize();
        timer_initialize();
        i2c_initialize(false);
        uart_initialize();
        ipmi_initialize();
        module_initialize();

        module_led_blink ( ACTIVITY_LED,
                           ACTIVITY_LED_BLINK_PERIOD,
                           ACTIVITY_LED_BLINK_PERIOD/6 );

        while( 1 )       /* Do forever */
        {
                ws_process_work_list();
                terminal_process_work_list();
                timer_process_callout_queue();
#ifdef USE_ETHERNET_CONNECTION
                LAN_poll_and_process();
#endif
                wd_feed();
        }
#if defined (__GNUC__)
        return 0; // Unreachable code. (eliminate gcc warning)
#endif
} // main()

/* end of document: main.c                                                      */
/*------------------------------------------------------------------------------*/
