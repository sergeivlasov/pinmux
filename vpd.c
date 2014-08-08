/*
---------------------------------------------------------------------------------
coreIPM/vpd.c

Author: Gokhan Sozmen
---------------------------------------------------------------------------------
Copyright (C) 2007-2008 Gokhan Sozmen
---------------------------------------------------------------------------------

---------------------------------------------------------------------------------
See http://www.coreipm.com for documentation, latest information, licensing, 
support and contact details.
---------------------------------------------------------------------------------
*/

#include <stdlib.h>
#include "common.h"
#include "vpd.h"
#include "eeprom.h"
#include "pca9546.h"
#include "tmp432.h"
#include "ucd9081.h"

/* 
    07.08 changes 
    branch=slave->master
                    */

/*     
        This file contain VPD initialization methos,
        SDR initialization methods, and
        conversion methods between VPD and SDR structures' data.
*/

const UINT8 katana_sensorID_VPD_translation_table[] = {0, // invalid ID
        KAT_FULCRUM_PCB_SENSOR_ID,          KAT_FULCRUM_CHIP_SENSOR_ID,         MCPA_TILERA_0_SENSOR_ID, 
        MCPA_TILERA_1_SENSOR_ID,            MCPA_PCB_SENSOR_ID,                 MCPB_TILERA_0_SENSOR_ID,           
        MCPB_TILERA_1_SENSOR_ID,            MCPB_PCB_SENSOR_ID,                 PSA_XILINX_A_SENSOR_ID,           
        PSA_XILINX_B_SENSOR_ID,             PSA_PCB_SENSOR_ID,                  UCD9081_KAT_12VA_SENSOR_ID,       
        UCD9081_KAT_12VB_SENSOR_ID,         UCD9081_KAT_1V0_SW_VDDX_SENSOR_ID,  UCD9081_KAT_1V2_SW_VTT_SENSOR_ID, 
        UCD9081_KAT_1V25_SW_VDD_SENSOR_ID,  UCD9081_KAT_3V3_PD1_SENSOR_ID,      UCD9081_KAT_2V5_OTHER_SENSOR_ID,  
        UCD9081_KAT_3V3_SENSE_SENSOR_ID,    UCD9081_KAT_1V2_VSDD_A_SENSOR_ID,   UCD9081_KAT_1V2_A0_VCDD_SENSOR_ID,
        UCD9081_KAT_1V2_A1_VCDD_SENSOR_ID,  UCD9081_KAT_1V8_DDR_A_SENSOR_ID,    UCD9081_KAT_2V5_OTHER_A_SENSOR_ID,
        UCD9081_KAT_1V2_VSDD_B_SENSOR_ID,   UCD9081_KAT_1V2_B0_VCDD_SENSOR_ID,  UCD9081_KAT_1V2_B1_VCDD_SENSOR_ID,
        UCD9081_KAT_1V8_DDR_B_SENSOR_ID,    UCD9081_KAT_2V5_OTHER_B_SENSOR_ID,  UCD9081_KAT_1V0_PSA_INT_SENSOR_ID,
        UCD9081_KAT_1V0_PSA_MGTAVCC_SENSOR_ID,UCD9081_KAT_1V2_PSA_MGTAVTT_SENSOR_ID,UCD9081_KAT_1V8_PSA_VCCO_SENSOR_ID,
        UCD9081_KAT_2V5_PSA_OTHER_SENSOR_ID   
};
const UINT8 lmc_sensorID_VPD_translation_table[] = {0, // invalid ID
        LMC_PCB_SENSOR_ID,              LMC_XILINX_A_SENSOR_ID,         LMC_XILINX_B_SENSOR_ID,    
        UCD9081_LMC1_1V0_SENSOR_ID,     UCD9081_LMC2_1V0_MGT_SENSOR_ID, UCD9081_LMC3_1V2_SENSOR_ID, 
        UCD9081_LMC4_1V5_DDR_SENSOR_ID, UCD9081_LMC5_1V8_SENSOR_ID,     UCD9081_LMC6_2V5_SENSOR_ID, 
        UCD9081_LMC7_3V3_SENSOR_ID
};

const UINT8 hmc_sensorID_VPD_translation_table[] = {0, // invalid ID
        HMC_EXHAUST_SENSOR_ID,          UCD9081_HMC_1V8_SENSOR_ID,      UCD9081_HMC_3V3_SENSOR_ID               
};


//------------------------------------------------------------------------------
// 
UINT16
vpd_sn_to_sdr_id(UINT8 VPD_sensor_number, UINT8* ref_table, UINT8 maxnum)
{
        if (VPD_sensor_number >= maxnum) return 0;
        return (UINT16)ref_table[VPD_sensor_number];
} // vpd_sn_to_sdr_id

//------------------------------------------------------------------------------
// invalid record "mark" is used to determine end-of-VPD-records.
bool
vpd_record_valid(VPD_GENERIC_RECORD* vr)
{
        if (vr->sensor_type > ST_TYPELIST_MAX) return false; 
        if (vr->sensor_number > 250) return false; // sensible max value
        
        // TODO.

        return true;
} // vpd_record_valid

//------------------------------------------------------------------------------
// convert VPD UINT16 value to the SDR UINT8 value per SDR descriptors.
UINT8
convert_vpd_to_sdr(UINT16 VPD_value, UINT8 scale )
{
        UINT16 temp;
        if (0 == scale) return 0;               // invalid scale value
        temp = (VPD_value + scale - 1) / scale; // round up
        if (temp > 255 ) temp = 255;            // stick to safe value
        return temp;
} // convert_vpd_to_sdr

void
merge_vpd_to_sdr(VPD_GENERIC_RECORD* vr, UINT16 sensor_ID)
{
        UINT8 found;
        FULL_SENSOR_RECORD* sr;
        
        found = find_sensor(sensor_ID, 0);
        
        if( 1 == vr->sensor_type )     {  // ST_TEMPERATURE
                if (( DUMP_VERBOSE & global_debug_setting ) &&
                    ( DBG_LVL1     & global_debug_setting ))
                { 
                        putstr( " Tsensor#" ); puthex( ((TEMPERATURE_RECORD*)vr)->sensor_number );
                        putstr( "/" );         puthex( sensor_ID );
                        putstr( " sdn:" ); putdecimal( ((TEMPERATURE_RECORD*)vr)->t_shutdown );
                        putstr( " tht:" ); putdecimal( ((TEMPERATURE_RECORD*)vr)->t_throttle );
                        putstr( " wrn:" ); putdecimal( ((TEMPERATURE_RECORD*)vr)->t_warning  );
                        putstr( " abs:" ); putdecimal( ((TEMPERATURE_RECORD*)vr)->t_absolute );  
                        putstr( " hy:" );  puthex( ((TEMPERATURE_RECORD*)vr)->pos_hysteresis );  
                }        
                if ( 255 == found) return;
                sr = (FULL_SENSOR_RECORD*)sdr_entry_table[found].record_ptr;
                // temperature sensors are 8-bit, no scaling needed.
                sr->upper_non_recoverable_threshold =
                        ((TEMPERATURE_RECORD*)vr)->t_shutdown;
                sr->upper_critical_threshold =
                        ((TEMPERATURE_RECORD*)vr)->t_throttle;
                sr->upper_non_critical_threshold =
                        ((TEMPERATURE_RECORD*)vr)->t_warning;
                sr->positive_going_threshold_hysteresis_value =
                        ((TEMPERATURE_RECORD*)vr)->pos_hysteresis;
                sr->negative_going_threshold_hysteresis_value =
                        ((TEMPERATURE_RECORD*)vr)->neg_hysteresis;
        }else if( 2 == vr->sensor_type ){  // ST_VOLTAGE
                if (( DUMP_VERBOSE & global_debug_setting ) &&
                    ( DBG_LVL1     & global_debug_setting ))
                { 
                        putstr( " Vsensor#" ); puthex( ((VOLTAGE_RECORD*)vr)->sensor_number  );
                        putstr( "/" );         puthex( sensor_ID );
                        putstr( " sdn:" ); putdecimal16( swap(((VOLTAGE_RECORD*)vr)->v_upper_shutdown),4 );
                        putstr( "/" );     putdecimal16( swap(((VOLTAGE_RECORD*)vr)->v_lower_shutdown),4 );
                        putstr( " wrn:" ); putdecimal16( swap(((VOLTAGE_RECORD*)vr)->v_upper_warning), 4 );
                        putstr( "/" );     putdecimal16( swap(((VOLTAGE_RECORD*)vr)->v_lower_warning), 4 );
                        putstr( " hy:" );  putdecimal16( swap(((VOLTAGE_RECORD*)vr)->pos_hysteresis),  2 ); 
                }     
                if (255==found) return;
                sr = (FULL_SENSOR_RECORD*)sdr_entry_table[found].record_ptr;
                // volt sensors are 16-bit, scaling needed.
                sr->upper_non_recoverable_threshold =
                        convert_vpd_to_sdr( swap(((VOLTAGE_RECORD*)vr)->v_upper_shutdown),
                                            sdr_entry_table[found].sensor_scale_value);
                sr->upper_critical_threshold =
                        convert_vpd_to_sdr( swap(((VOLTAGE_RECORD*)vr)->v_upper_warning), 
                                            sdr_entry_table[found].sensor_scale_value);
                sr->upper_non_critical_threshold =
                        convert_vpd_to_sdr( swap(((VOLTAGE_RECORD*)vr)->v_upper_warning), 
                                            sdr_entry_table[found].sensor_scale_value);

                sr->lower_non_recoverable_threshold =
                        convert_vpd_to_sdr( swap(((VOLTAGE_RECORD*)vr)->v_lower_shutdown),
                                            sdr_entry_table[found].sensor_scale_value);
                sr->lower_critical_threshold =
                        convert_vpd_to_sdr( swap(((VOLTAGE_RECORD*)vr)->v_lower_warning), 
                                            sdr_entry_table[found].sensor_scale_value);
                sr->lower_non_critical_threshold =
                        convert_vpd_to_sdr( swap(((VOLTAGE_RECORD*)vr)->v_lower_warning), 
                                            sdr_entry_table[found].sensor_scale_value);

                sr->positive_going_threshold_hysteresis_value =
                        convert_vpd_to_sdr( swap(((VOLTAGE_RECORD*)vr)->pos_hysteresis), 
                                            sdr_entry_table[found].sensor_scale_value);
                sr->negative_going_threshold_hysteresis_value =
                        convert_vpd_to_sdr( swap(((VOLTAGE_RECORD*)vr)->neg_hysteresis), 
                                            sdr_entry_table[found].sensor_scale_value);
        }
} // merge_vpd_to_sdr

#define VPD_MAX_POWER_OFFSET_SDD
void 
module_vpd_init()
{
        UINT16 blk[sizeof(VPD_GENERIC_RECORD)/2];
        VPD_KATANA_RECORD  *kr; // fixed block VPD record (!too big!)
        VPD_GENERIC_RECORD *vr; // block 4 record (sensor's characteristics)
        UINT16 rec_num;
        UINT16 eeoffset;
        UINT16 eesz;
        UINT16 sID;
        UINT32 a;
        
        power_rating.max_katana = DEFAULT_POWER_WATTS_KATANA;

        vr = (VPD_GENERIC_RECORD*)&blk[0];
        kr = (VPD_KATANA_RECORD*)&blk[0];
        
//
// There are two max power values in the VPD. Same values, different offsets
// we keep the code for them both here. 
// If the VPD_MAX_POWER_OFFSET_SDD then the value is taken as per SDD from 
// VPD block 0 PhysicalCharacteristics, otherwise from block 0 max_power.
//        
#ifdef VPD_MAX_POWER_OFFSET_SDD
        a = ((UINT8*)&(kr->PhysicalCharacteristics[2])) - ((UINT8*)kr);

        if ( eeprom_read(0, KAT_MUX, MUX_CH_1, KAT_EEPROM_ADDR, a, (UINT8*)kr, 1 )) return;
        power_rating.max_katana = (UINT16)(5 * *((UINT8*)kr));  // 5W increments
#else        
        a = ((UINT8*)&(kr->max_power)) - ((UINT8*)kr);

        if ( eeprom_read(0, KAT_MUX, MUX_CH_1, KAT_EEPROM_ADDR, a, (UINT8*)kr, 2 )) return;
        power_rating.max_katana = swap(*((UINT16*)kr));         // VPD is stored in big endian format
#endif
        power_rating.min_katana = power_rating.max_katana/2;    // temp4debug, until clear where min comes from
        if (( DUMP_VERBOSE & global_debug_setting ) &&
            ( DBG_LVL1     & global_debug_setting ))            // [dbg 88]
        { 
                putstr( "Katana max power:" ); 
                putdecimal16( power_rating.max_katana, 2);
                putstr( " Watts.\n" ); 
        }
        
        // find block 4 (sensor data)
        a = ((UINT8*)&(kr->block_4_offset)) - ((UINT8*)kr);
        if ( eeprom_read(0, KAT_MUX, MUX_CH_1, KAT_EEPROM_ADDR, a, (UINT8*)kr, 2 )) return;
        eeoffset = swap(*((UINT16*)kr));                        // VPD is stored in big endian format
        
        if ( eeprom_read(0, KAT_MUX, MUX_CH_1,  // read the header, take the block length
                         KAT_EEPROM_ADDR, eeoffset, 
                         (UINT8*)vr, sizeof(VPD_TEMPERATURE_VOLTAGE_SETTINGS_HDR) )) return;
        eesz = swap(((VPD_TEMPERATURE_VOLTAGE_SETTINGS_HDR*)vr)->length); // big endian

        rec_num = 0;
        putstr( "\n" );
        while(rec_num * sizeof(VPD_GENERIC_RECORD) < eesz){     // read KATANA CCA vpd
                if (eeprom_read( 0, KAT_MUX, MUX_CH_1, EEPROM_I2C_ADDRESS, 
                             rec_num * sizeof(VPD_GENERIC_RECORD) + 
                             eeoffset + sizeof(VPD_TEMPERATURE_VOLTAGE_SETTINGS_HDR), 
                             (UINT8*)vr, sizeof(VPD_GENERIC_RECORD) )) break;
                if ( 0 == vpd_record_valid(vr) ){ 
//                        if ( DUMP_VERBOSE & global_debug_setting ) {
//                                putstr( "Katana invalid record #" ); 
//                                puthex( rec_num+1 ); putstr( "\n" );
//                        }
                        break;
                }
                // store VPD values read.
                if( 1 == vr->sensor_type ){  // ST_TEMPERATURE
                        if (( DUMP_VERBOSE & global_debug_setting ) &&
                            ( DBG_LVL1     & global_debug_setting ))    // [dbg 88]
                        { 
                                putstr( "Kat rec #" ); puthex( rec_num+1 );
                        }
                
                        // Sensor numbers are from the Katana SDD and 
                        //                    VPD_writer.xml documents.
                        
                        sID = vpd_sn_to_sdr_id(vr->sensor_number, 
                                        (UINT8*)katana_sensorID_VPD_translation_table,
                                        sizeof(katana_sensorID_VPD_translation_table));

			tmp432_set_therm_limit( sID, ((TEMPERATURE_RECORD *)vr)->t_absolute, 
					((TEMPERATURE_RECORD *)vr)->pos_hysteresis );

			merge_vpd_to_sdr( vr, sID);
                        if (( DUMP_VERBOSE & global_debug_setting ) &&
                            ( DBG_LVL1     & global_debug_setting )) {    // [dbg 88] 
                                putstr( " x:" );  puthex( find_sensor(sID, 0) ); 
                                putstr( "\n" ); 
                        }
                }else  if( 2 == vr->sensor_type ){   // ST_VOLTAGE
                        if (( DUMP_VERBOSE & global_debug_setting ) &&
                            ( DBG_LVL1     & global_debug_setting )) { 
                                putstr( "Kat rec #" ); puthex( rec_num+1 );
                        }

                        // Voltage SDRs hold 8-bit packed data, different from
                        //         16-bit raw sensor and VPD data (millivolts).
                        // Conversion needed per SDR static descriptors.
                        sID = vpd_sn_to_sdr_id(vr->sensor_number, 
                                        (UINT8*)katana_sensorID_VPD_translation_table,
                                         sizeof(katana_sensorID_VPD_translation_table));

                        merge_vpd_to_sdr( vr, sID);
                        if (( DUMP_VERBOSE & global_debug_setting ) &&
                            ( DBG_LVL1     & global_debug_setting )) { 
                                putstr( " x:" );  puthex( find_sensor(sID, 0) ); 
                                putstr( "\n" ); 
                        }
                }else{  
                        if (( DUMP_VERBOSE & global_debug_setting ) &&
                        ( DBG_LVL1     & global_debug_setting ))
                        if (0 != ((VPD_GENERIC_RECORD*)vr)->sensor_number){ // we ignore records with sensor# = 0
                                putstr( "Katana invalid sensor type:" ); puthex( vr->sensor_type ); 
                                putstr( " #" );                          puthex( vr->sensor_number ); 
                                putstr( " @rec#" );                      puthex( rec_num+1 );           
                                putstr( "\n" );
                        }
                }
                rec_num++;
        }
        if (( DUMP_VERBOSE & global_debug_setting ) &&
            ( DBG_LVL1     & global_debug_setting ))
        { 
                putstr( "Katana VPD records found:" ); 
                puthex( rec_num ); putstr( "\n" );
        }

        if( module_get_bmc_state( BMC_STATE_LMC_PRESENT ) ){
                power_rating.max_lmc = DEFAULT_POWER_WATTS_LMC;
                
#ifdef VPD_MAX_POWER_OFFSET_SDD
                a = ((UINT8*)&(kr->PhysicalCharacteristics[2])) - ((UINT8*)kr);
                if ( eeprom_read(0, LMC_MUX, MUX_CH_1, LMC_EEPROM_ADDR, a, (UINT8*)kr, 1 )) return;
                power_rating.max_lmc = (UINT16)(5 * *((UINT8*)kr));     // 5W increments
#else        
                a = ((UINT8*)&(kr->max_power)) - ((UINT8*)kr);
                if ( eeprom_read(0, LMC_MUX, MUX_CH_1, LMC_EEPROM_ADDR, a, (UINT8*)kr, 2 )) return;
                power_rating.max_lmc = swap(*((UINT16*)kr));            // VPD is stored in big endian format
#endif
                power_rating.min_lmc = power_rating.max_lmc/2; // temp for debug, until clear where min comes from
                if (( DUMP_VERBOSE & global_debug_setting ) &&
                    ( DBG_LVL1     & global_debug_setting )) {     // [dbg 88]
                        putstr( "LMC max power:" ); 
                        putdecimal16( power_rating.max_lmc,2 );
                        putstr( " Watts.\n" ); 
                }
                
                // find VPD block 4
                a = ((UINT8*)&(kr->block_4_offset)) - ((UINT8*)kr);
                if ( eeprom_read(0, LMC_MUX, MUX_CH_1, LMC_EEPROM_ADDR, a, (UINT8*)kr, 2 )) return;
                eeoffset = swap(*((UINT16*)kr));                        // VPD is stored in big endian format

                if ( eeprom_read(0, LMC_MUX, MUX_CH_1, // read the header, take the block length
                         LMC_EEPROM_ADDR, eeoffset,    // VPD_TEMPERATURE_VOLTAGE_SETTINGS_OFFSET, 
                         (UINT8*)vr, sizeof(VPD_TEMPERATURE_VOLTAGE_SETTINGS_HDR) )) return;
                eesz = swap( ((VPD_TEMPERATURE_VOLTAGE_SETTINGS_HDR*)vr)->length);
                rec_num = 0;
                if (eesz < 65535) // safety
                while(rec_num * sizeof(VPD_GENERIC_RECORD) < eesz){     // read LMC vpd
                        if (eeprom_read( 0, LMC_MUX, MUX_CH_1, EEPROM_I2C_ADDRESS, 
                                rec_num * sizeof(VPD_GENERIC_RECORD) + 
                                eeoffset + sizeof(VPD_TEMPERATURE_VOLTAGE_SETTINGS_HDR), 
                                (UINT8*)vr, sizeof(VPD_GENERIC_RECORD) )) break;
                        if ( 0 == vpd_record_valid(vr) ) break;
                        
                        // store VPD values read.
                        if( 1 == vr->sensor_type ){  // ST_TEMPERATURE
                                if (( DUMP_VERBOSE & global_debug_setting ) &&
                                    ( DBG_LVL1     & global_debug_setting )) { 
                                        putstr( "LMC rec #" ); puthex( rec_num+1 );
                                }
                                // sensor numbers are from Katana SDD and 
                                //                    VPD_writer.xml documents.
                                sID = vpd_sn_to_sdr_id(vr->sensor_number, 
                                        (UINT8*)lmc_sensorID_VPD_translation_table,
                                         sizeof(lmc_sensorID_VPD_translation_table));

				tmp432_set_therm_limit( sID, ((TEMPERATURE_RECORD *)vr)->t_absolute, 
					((TEMPERATURE_RECORD *)vr)->pos_hysteresis );

                                merge_vpd_to_sdr( vr, sID);
                                if (( DUMP_VERBOSE & global_debug_setting ) &&
                                    ( DBG_LVL1     & global_debug_setting )) { 
                                        putstr( " x:" );  puthex( find_sensor(sID, 0) ); 
                                        putstr( "\n" ); 
                                }
                        }else  if( 2 == vr->sensor_type ){   // ST_VOLTAGE
                                if (( DUMP_VERBOSE & global_debug_setting ) &&
                                    ( DBG_LVL1     & global_debug_setting )) { 
                                        putstr( "LMC rec #" ); puthex( rec_num+1 );
                                }
                                sID = vpd_sn_to_sdr_id(vr->sensor_number, 
                                        (UINT8*)lmc_sensorID_VPD_translation_table,
                                         sizeof(lmc_sensorID_VPD_translation_table));
                        
                                merge_vpd_to_sdr( vr, sID);
                                if (( DUMP_VERBOSE & global_debug_setting ) &&
                                    ( DBG_LVL1     & global_debug_setting )) { 
                                        putstr( " x:" );  puthex( find_sensor(sID, 0) ); 
                                        putstr( "\n" ); 
                                }
                        } // else - invalid record. 
                          //        This is possible and does not invalidate the VPD
                        rec_num++;
                }
                if (( DUMP_VERBOSE & global_debug_setting ) &&
                    ( DBG_LVL1     & global_debug_setting )) {    // [dbg 88] 
                        putstr( "LMC VPD records found:" ); 
                        puthex( rec_num ); putstr( "\n" );
                }
        }else   if (( DUMP_VERBOSE & global_debug_setting ) &&
                    ( DBG_LVL1     & global_debug_setting )) { 
                 putstr( "LMC not present.\n" );
        }

        if(module_get_bmc_state( BMC_STATE_HMC_PRESENT )){
        	power_rating.max_hmc = DEFAULT_POWER_WATTS_HMC;

#ifdef VPD_MAX_POWER_OFFSET_SDD
                a = ((UINT8*)&(kr->PhysicalCharacteristics[2])) - ((UINT8*)kr);
                if ( eeprom_read(0, HMC_MUX, MUX_CH_1, HMC_EEPROM_ADDR, a, (UINT8*)kr, 1 )) return;
                power_rating.max_hmc = (UINT16)(5 * *((UINT8*)kr));     // 5W increments
#else        
                a = ((UINT8*)&(kr->max_power)) - ((UINT8*)kr);
                if ( eeprom_read(0, HMC_MUX, MUX_CH_1, HMC_EEPROM_ADDR, a, (UINT8*)kr, 2 )) return;
                power_rating.max_hmc = swap(*((UINT16*)kr));            // VPD is stored in big endian format
#endif
                
                power_rating.min_hmc = power_rating.max_hmc/2; // temp for debug, until clear where min comes from
                
                if (( DUMP_VERBOSE & global_debug_setting ) &&
                    ( DBG_LVL1     & global_debug_setting )) { 
                        putstr( "HMC max power:" ); 
                        putdecimal16( power_rating.max_hmc,2 );
                        putstr( " Watts.\n" ); 
                }

                // find block 4 (sensor data)
                a = ((UINT8*)&(kr->block_4_offset)) - ((UINT8*)kr);
                if ( eeprom_read(0, HMC_MUX, MUX_CH_1, HMC_EEPROM_ADDR, a, (UINT8*)kr, 2 )) return;
                eeoffset = swap(*((UINT16*)kr));                        // VPD is stored in big endian format

                if ( eeprom_read(0, HMC_MUX, MUX_CH_1, // read the header, take the block length
                         HMC_EEPROM_ADDR, eeoffset,    // VPD_TEMPERATURE_VOLTAGE_SETTINGS_OFFSET, 
                         (UINT8*)vr, sizeof(VPD_TEMPERATURE_VOLTAGE_SETTINGS_HDR) )) return;
                eesz = swap( ((VPD_TEMPERATURE_VOLTAGE_SETTINGS_HDR*)vr)->length);
                rec_num = 0;
                if (eesz < 65535) // safety
                while(rec_num * sizeof(VPD_GENERIC_RECORD) < eesz){     // read HMC vpd
                        if (eeprom_read( 0, HMC_MUX, MUX_CH_1, EEPROM_I2C_ADDRESS, 
                                rec_num * sizeof(VPD_GENERIC_RECORD) + 
                                eeoffset + sizeof(VPD_TEMPERATURE_VOLTAGE_SETTINGS_HDR), 
                                (UINT8*)vr, sizeof(VPD_GENERIC_RECORD) )) break;
                        if ( 0 == vpd_record_valid(vr) ) break;
                        
                        // store VPD values read.
                        if( 1 == vr->sensor_type ){  // ST_TEMPERATURE
                                if (( DUMP_VERBOSE & global_debug_setting ) &&
                                    ( DBG_LVL1     & global_debug_setting )) { 
                                        putstr( "HMC rec #" ); puthex( rec_num+1 );
                                }
                                // sensor numbers are from Katana SDD and 
                                //                    VPD_writer.xml documents.
                                sID = vpd_sn_to_sdr_id(vr->sensor_number, 
                                        (UINT8*)hmc_sensorID_VPD_translation_table,
                                         sizeof(hmc_sensorID_VPD_translation_table));

                                tmp432_set_therm_limit( sID, 
                                        ((TEMPERATURE_RECORD *)vr)->t_absolute, 
					((TEMPERATURE_RECORD *)vr)->pos_hysteresis );

                                merge_vpd_to_sdr( vr, sID);
                                if (( DUMP_VERBOSE & global_debug_setting ) &&
                                    ( DBG_LVL1     & global_debug_setting )) { 
                                        putstr( " x:" );  puthex( find_sensor(sID, 0) ); 
                                        putstr( "\n" ); 
                                }
                        }else  if( 2 == vr->sensor_type ){   // ST_VOLTAGE
                                if (( DUMP_VERBOSE & global_debug_setting ) &&
                                    ( DBG_LVL1     & global_debug_setting )) { 
                                        putstr( "HMC rec #" ); puthex( rec_num+1 );
                                }
                                sID = vpd_sn_to_sdr_id(vr->sensor_number, 
                                        (UINT8*)hmc_sensorID_VPD_translation_table,
                                         sizeof(hmc_sensorID_VPD_translation_table));
                        
                                merge_vpd_to_sdr( vr, sID);
                                if (( DUMP_VERBOSE & global_debug_setting ) &&
                                    ( DBG_LVL1     & global_debug_setting )) { 
                                        putstr( " x:" );  puthex( find_sensor(sID, 0) ); 
                                        putstr( "\n" ); 
                                }
                        } // else - invalid record. 
                          //        This is possible and does not invalidate the VPD
                        
                        if( 1000 < rec_num++ ) return; // safety check.
                }
                if (( DUMP_VERBOSE & global_debug_setting ) &&
                    ( DBG_LVL1     & global_debug_setting )) {    // [dbg 88] 
                        putstr( "HMC VPD records found:" ); 
                        puthex( rec_num ); putstr( "\n" );
                }
        }else  if (( DUMP_VERBOSE & global_debug_setting ) &&
                   ( DBG_LVL1     & global_debug_setting )) { 
                putstr( "HMC not present.\n" );
        }
        module_set_bmc_state(BMC_STATE_VPD_OK); 
        if (( DUMP_VERBOSE & global_debug_setting ) &&
                   ( DBG_LVL1     & global_debug_setting ))  
                putstr( "The VPD data structure is found correct.\n" );
} // module_vpd_init
//------------------------------------------------------------------------------


/* end of document vpd.c ---                                                   */
/*-----------------------------------------------------------------------------*/

