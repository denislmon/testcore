/*! \file cmdaction.c \brief functions for process commands.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: independent
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2007/07/24 by Wai Fai Chin
//  History:  Modified on 2011/12/16 by Denis Monteiro
// 
//     It processes a valid command from cmdparser module. Its execution based
//  on the input command and the command descriptor.
//
// ****************************************************************************

/**
 * \code
 * Host Command Structure:
 *    {src dest cmdxP1;P2;;;Pn}
 *    spaces are for clarity. No space between src dest and cmd.
 *    where:
 *           {  is the command start character marke the start of the command.
 *         src  is the source id in two digit hex char range from 00 to FF. FF is a free ID use by host before first connection with this device.
 *         dest is the command id in two digit hex char range from 00 to FE. FF is for every device, broadcast mode.
 *         cmd  is the command id in two digit hex char range from 00 to FE. FF is invalid command.
 *           x  is the command attribute field has the following char:
 *                 r -- answers the command query with contents of items that associated
 *                      with the command. It is for output to the host computer.
 *                 i -- answers the command query of 1D array index type.
 *                      The number followed by i is the index of all the following array outputs.
 *
 *                 ? -- command query items that associated with the command.
 *                 = -- assigns input contents to items that associated with the command.
 *                 z -- reset items that associated with the command to its default contents.
 *              g, = -- denoted for action type command. Action type command normally has no input parameter.
 *                      In some case, it needs an array index number such as sensor ID, after the 'g' or '='.
 *                      The resulted action might output data to a registered stream device or other I/O devices.
 *                 0 to F are reserved for future use.
 *          Pn  is the input parameters or contents of items that associated with the command.
 *           }  is the end character marked the end of the command.
 * Command Version:  00-00-01
 * \endcode
 * \note every input command will always has an corresponding respond output.
 *       The output could be a queried value, status, or some other output.
 *
 * History:  Created on 2006/11/07 by Wai Fai Chin
 *			 2009/03/31 -WFC- modified it to include source and destination ID.
 */


#include  "cmdparser.h"
#include  "cmdaction.h"
#include  "commonlib.h"
#include  "sensor.h"
#include  "calibrate.h"
#include  "dataoutputs.h"
#include  "nvmem.h"
#include  "adc_lt.h"
#include  "lc_zero.h"
#include  "lc_tare.h"
#include  "lc_total.h"
#include  "fer_nvram.h"

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3  )	// 2010-10-07 -WFC-
#include  "hw3460_led.h"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
#include  "hw4260B_led.h"
#endif

#include  "stream_router.h"
#include  "msi_packet_router.h"
#include  "vs_math.h"

#include  "scalecore_sys.h"
#include  "setpoint.h"
#include  "bios.h"		// for DISABLE_GLOBAL_INTERRUPT
#include  "dac_cpu.h"
#include  "self_test.h"
#include  <stdio.h>

#include  "vs_sensor.h"

// 2011-12-05 -WFC- #include  "led_display.h"			// it is included in panelmain.h file.
#include  "led_lcd_msg_def.h"	// 2011-03-03	-WFC-
#include  "v_key.h"				// 2011-07-25	-WFC-

// 2011-07-26 -WFC- v
#if ( CONFIG_INCLUDED_PRINT_STRING_MODULE == TRUE )
#include "print_string.h"
#endif
// 2011-07-26 -WFC- ^

// 2011-08-10 -WFC- v
#if (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )
#include  "panelmain.h"
extern struct	pt		gPanelMainSelfTest_thread_pt;		// 2011-08-22 -WFC-
#endif
// 2011-07-26 -WFC- ^

extern void main_system_master_default_system_configuration( void );

// private functions:
void  cmd_act_answer_query( CMD_PRE_PARSER_STATE_T *pCmdS );
int   cmd_act_format_display_item  ( CMD_NEXT_ITEM_T  *pItem, char *pStr, CMD_PRE_PARSER_STATE_T *pCmdS );
int   cmd_act_format_display_item_P( CMD_NEXT_ITEM_T  *pItem, char *pStr, CMD_PRE_PARSER_STATE_T *pCmdS );
void  cmd_act_next_item( CMD_NEXT_ITEM_T  *pItem );
//void  cmd_act_set_defaults( BYTE cmd );
BYTE  cmd_act_set_item( CMD_NEXT_ITEM_T  *pItem, char *pStr, CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_act_set_items( CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_current_unit_post_update( 	CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_current_unit_pre_update(		CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_goto_loadcell( 				CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_goto_sensor( 					CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_cal_cb_query_action_mapper(	CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_is_cal_countby_ok_mapper(		CMD_PRE_PARSER_STATE_T *pCmdS );
// 2010-09-10 BYTE  cmd_is_current_countby_ok( 		CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_is_new_cal_point_ok( 			CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_is_ok_start_new_cal( 			CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_is_zero_cal_point_ok( 		CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_lc_auto_tare( 				CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_lc_clear_tare( 				CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_lc_toggle_net_gross_mode( 	CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_lc_total_thresh_post_update(	CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_lc_zero(						CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_is_ok_to_zero( 				CMD_PRE_PARSER_STATE_T *pCmdS );  // 2015-05-12 -WFC-
BYTE  cmd_loadcell_update_or_init(		CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_no_save_cal_exit(				CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_save_cal_exit(				CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_send_sensor_data_packet(		CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_sensor_features_init(			CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_stream_out_modes_action(		CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_system_run_mode( 				CMD_PRE_PARSER_STATE_T *pCmdS );


void cmd_show_cmd_status(				CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_pre_password_action( 			CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_execute_password_cmd( 		CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_cnfg_cal_point_post_action_mapper(CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_cnfg_cal_point_pre_action_mapper(	CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_cnfg_cal_hdr_post_action_mapper( 	CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_cnfg_cal_hdr_pre_action_mapper( 	CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_sensor_cal_point_query_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_sensor_value_query_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_loadcell_rcal_query_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE cmd_query_cal_status_pre_action( CMD_PRE_PARSER_STATE_T *pCmdS );	// 2015-05-11 -WFC-

BYTE  cmd_setpoint_value_validation(	CMD_PRE_PARSER_STATE_T *pCmdS );

void  cmd_update_or_init_sensor_params( BYTE ch, BYTE sensorN );
//void  cmd_update_loadcell_zero_band_params( BYTE ch, BYTE sensorN );

BYTE  cmd_goto_bootloader_cmd( CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_rcal_set_post_action( CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_execute_sc_hw_test_cmd( CMD_PRE_PARSER_STATE_T *pCmdS );		// TESTONLY
BYTE  cmd_execute_HW3460_cmd( CMD_PRE_PARSER_STATE_T *pCmdS );	// TESTONLY

BYTE  cmd_execute_cal_dac_cmd_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_execute_cnfg_sensor_dac_cmd( CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_execute_manual_out_dac_cmd_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_execute_cnfg_math_channel_cmd( CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_execute_lc_manual_total( CMD_PRE_PARSER_STATE_T *pCmdS );
BYTE  cmd_execute_lc_zero_total( CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_execute_cnfg_bargraph_cmd( CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_master_default_system_configuration_cmd( CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_memory_check_cmd( CMD_PRE_PARSER_STATE_T *pCmdS );
void  cmd_clear_global_scratch_memory( void );

BYTE  cmd_lc_undo_zero( CMD_PRE_PARSER_STATE_T *pCmdS );

// 2011-09-27 -WFC- #if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD )
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-09-27 -WFC-
char cmd_reliable_send_command_main_thread( BYTE managerID );
char cmd_reliable_send_command_thread( CMD_RELIABLE_SENDING_CLASS_T *pCmdRSobj );
void cmd_reliable_send_manager_check_ack( BYTE srcID, BYTE receivedCmd );
#endif

BYTE cmd_clear_service_counters_cmd( CMD_PRE_PARSER_STATE_T *pCmdS );

BYTE  cmd_viewing_countby_query_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );		// 2010-09-10 -WFC-
BYTE  cmd_is_viewing_countby_ok_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );			// 2010-09-10 -WFC-

BYTE  cmd_lc_total_wt_read_post_action( CMD_PRE_PARSER_STATE_T *pCmdS );			// 2010-11-10 -WFC-

BYTE  cmd_cnfg_peakhold_post_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );		// 2011-03-29 -WFC-
BYTE  cmd_cnfg_peakhold_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );			// 2011-03-29 -WFC-

BYTE  cmd_print_string_cnfg_post_action( CMD_PRE_PARSER_STATE_T *pCmdS );			// 2011-07-26 -WFC-
BYTE  cmd_formatter_post_action( CMD_PRE_PARSER_STATE_T *pCmdS );					// 2011-07-26 -WFC-
BYTE  cmd_print_string_post_action( CMD_PRE_PARSER_STATE_T *pCmdS );				// 2011-07-26 -WFC-

BYTE  cmd_send_loadcell_gnt_packet( CMD_PRE_PARSER_STATE_T *pCmdS );				// 2011-08-10 -WFC-
BYTE  cmd_send_loadcell_vcb_packet( CMD_PRE_PARSER_STATE_T *pCmdS );				// 2012-02-13 -WFC-
BYTE  cmd_send_loadcell_simple_packet( CMD_PRE_PARSER_STATE_T *pCmdS );				// 2012-02-13 -WFC-
BYTE  cmd_toggle_hires_post_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );			// 2012-02-23 -WFC-
BYTE  cmd_remove_last_total_post_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS );	// 2012-03-09 -WFC-
BYTE  cmd_rf_config_post_action( CMD_PRE_PARSER_STATE_T *pCmdS );					// 2012-04-24 -WFC-
BYTE  cmd_ethernet_config_post_action( CMD_PRE_PARSER_STATE_T *pCmdS );				// 2012-07-09 -DLM-

// 2011-04-22 -WFC- BYTE cmd_rcal_enabled_pre_update( CMD_PRE_PARSER_STATE_T *pCmdS );					// 2011-04-12 -WFC-

/// a remote meter is not able to connect to ScaleCore. Note that future version can handle connection status of many device. Right now, any device communication timeout is no connection.
BYTE	gbConnectionStatus;

BYTE	 gbSCHWTestCode;	
BYTE	 gbSCHWTestState;

/// scratch memory for interface between host command and complex data structure.
float	gfAcmdTmp1, gfAcmdTmp2;
INT32	gi32AcmdTmp1;
UINT16	gu16AcmdTmp1;
INT16	gi16AcmdTmp1;
BYTE	gbAcmdTmp1, gbAcmdTmp2, gbAcmdTmp3;
INT8	gi8AcmdTmp1;
BYTE	gStrAcmdTmp[ CMD_MAX_TMP_STRING_LENGTH + 6];


// private variables for this module.
// for handle array index type of a command.
BYTE  gbCMD_Index;

// It is for passing results from pre action to post action of a command.
BYTE  gbCMDTmp[MAX_NUM_SENSORS];

#if ( CONFIG_EMULATE_ADC == FALSE )
BYTE  cmd_adc_config_init( CMD_PRE_PARSER_STATE_T *pCmdS );
#endif

BYTE	gbCmdError;		// error status of a command of gbCmdID. For e.g. If a command is password protect, it will respond {00rCmd,error}
BYTE	gbCmdID;

/// System Running Mode use by command {07=sysRunMode} private for cmdAction.c.
BYTE	gbCmdSysRunMode;
UINT16  gwSysStatus;			// system status;

/// Acknowledgment of remote device or this device Rcal value in ASCII string.
BYTE	gabAckRcalStr[ MAX_NUM_RCAL_LOADCELL ][ CMD_MAX_TMP_STRING_LENGTH ];

#if ( CONFIG_PRODUCT_AS != CONFIG_AS_HLI  )

// Acknowledgment.
BYTE	gbAckCmdError;		// error status of a query result of gbCmdID. For e.g. If a command is password protect, it will respond {00rCmd,error}
//BYTE	gbAckCmdID;

/// Acknowledgment of remote device System Running Mode use by command {07=sysRunMode},
BYTE	gbAckCmdSysRunMode;

/// Acknowledgment of remote device system error code for user.
// 2012-02-28 -WFC- BYTE	gbAckSysErrorCodeForUser;			// 2011-01-03 -WFC-

///	Acknowledgment from a remote device ID, model code, model name, serial numbers.
PRODUCT_INFO_OF_REMOTE_DEVICE_T		gtAckProductInfoRemoteDev;

/// Acknowledgment from a remote device SysStatus.
UINT16  gwAckSysStatus;

/// Acknowledgment of remote device Rcal value in ASCII string.
BYTE	gAckRcalStr[ CMD_MAX_TMP_STRING_LENGTH ];

// Acknowledgment of remote device or this device Rcal value in ASCII string.
// BYTE	gabAckRcalStr[ MAX_NUM_RCAL_LOADCELL ][ CMD_MAX_TMP_STRING_LENGTH ];

/// Acknowledgment from a remote device of its bargraph sensor id.
BYTE  gbAckBargraphSensorID;

/// Acknowledgment from a remote device of its number of lit segment of bargraph of its sensor value.
BYTE  gbAckBargraphNumLitSeg;
#endif

// The following two variables will init by NV during power up.
// It uses as scratch variable during parsing the command.
// If the post action validated the input, then it copied into NV.
//
BYTE	gbPassWordAct; 
BYTE	gbCmdLock;
BYTE	gStrPassword[ MAX_PASSWORD_LENGTH + 1 ];

//BYTE	gStrPasswordNV[ MAX_PASSWORD_LENGTH + 1];

// 2011-09-27 -WFC- #if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD )
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-09-27 -WFC-
CMD_RELIABLE_SENDING_CLASS_T gaCmdReliableSendObj[ CMD_RELIABLE_SEND_MAX_NUM_OBJ ];
CMD_RELIABLE_SENDING_MANAGER_CLASS_T gaCmdReliableSendManagerObj[ CMD_RELIABLE_SEND_MANAGER_MAX_NUM_OBJ ];
//CMD_RELIABLE_SENDING_MANAGER_CLASS_T gpCmdRSManagerObj[ CMD_RELIABLE_SEND_MANAGER_MAX_NUM_OBJ ];
#endif

/////////////////////////////////////////////////////////////////////////////// 


/////////////////////////////////////////////////////////////////////////////// 

// software version number, negative means test version. > 0 mean production released.
// need	call cmd_act_set_defaults( 0x01 );	to set the software version and configuration version.
//INT16	gwsSoftVersion;
//BYTE	gbCNFG_VERSION;	// configuration setting format version for nonvolatile memory.

// INT16	gwsSoftVersion = -1;
// BYTE	gbCNFG_VERSION = 1;	// configuration setting format version for nonvolatile memory.

// command 00 action items: is for system wide configuration .
// command 00 items

/*
 * Doxygen doesn't append attribute syntax of
 * GCC, and confuses the typedefs with function decls, so
 * supply a doxygen-friendly view.
 */

// FAA will not allow to have auto time stamp because it creates different checksum on the same source code every time a new one is built.
//const char gcStrSOft_BuiltDate[]	PROGMEM = __DATE__;
//const char gcStrSOft_BuiltTime[]	PROGMEM = __TIME__;

const char gcStrSOft_BuiltDate[]	PROGMEM = "2017-09-12";
const char gcStrSOft_BuiltTime[]	PROGMEM = "16:50";

const char gcStrMasterPassword[] PROGMEM = "0199";
const char gcStrPasswordMask[] PROGMEM = "***";
const char gcStrDefUserModelName[] PROGMEM = USER_DEF_MODEL_NAME;
const char gcStr3QuestionsMarks[] PROGMEM = "???";
const char gcStr5QuestionsMarks[] PROGMEM = "?????";
const char gcStr0p0[] PROGMEM = "0.0";

// default device ID range 0 to 8 is for ScaleCore with loadcells. 9 is for a smart remote meter.
/// Default device ID for a remote meter is 9. 0 is for the ScaleCore with loadcell.
#define CMD_DEFAULT_DSC_DEVICE_ID		0
#define CMD_DEFAULT_REMOTE_DEVICE_ID	9
#define CMD_DEFAULT_HANDHELD_METER_ID	250

// {00?} will returns {00rcmd; error}	command; error of command (0==none). It will show this whenever there is an command error.
CMD_BYTE_ITEM_T     gcCmd_Cmd_Error	PROGMEM = { 0,  255,  0, &gbCmdError, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_Error_Cmd	PROGMEM = { 0,  255,  0, &gbCmdID, TYPE_BYTE, &gcCmd_Cmd_Error };

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
// {00rcmd; error}	acknowledgment command; error of command (0==none). It will show this whenever there is an command error.
CMD_BYTE_ITEM_T     gcAckCmd_Cmd_Error	PROGMEM = { 0,  255,  0, &gbAckCmdError, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcAckCmd_Error_Cmd	PROGMEM = { 0,  255,  0, &gbCmdID, TYPE_BYTE, &gcAckCmd_Cmd_Error };
#endif

// {01?} returns {productID; product version ID; software version; built date and time}
CMD_STRING_ITEM_T	gcCmd_SoftBuiltTime PROGMEM = { 0, sizeof(gcStrSOft_BuiltTime), &gcStrSOft_BuiltTime, &gcStrSOft_BuiltTime, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_STRING_ITEM_T	gcCmd_SoftBuiltDate PROGMEM = { 0, sizeof(gcStrSOft_BuiltDate), &gcStrSOft_BuiltDate, &gcStrSOft_BuiltDate, TYPE_STRING | TYPE_READ_ONLY_PGM, &gcCmd_SoftBuiltTime };
CMD_STRING_ITEM_T	gcCmd_SoftVersion	PROGMEM = { 0, MAX_SOFTWARE_VERSION_LENGTH, &gcStr5QuestionsMarks, &gAppBootShareNV.softVersionStr, TYPE_STRING | TYPE_READ_ONLY_PGM, &gcCmd_SoftBuiltDate };
CMD_BYTE_ITEM_T		gcCmd_Prod_Ver		PROGMEM = { 0,  255,	0, &gAppBootShareNV.productVersionID,	TYPE_STRING,	&gcCmd_SoftVersion };
CMD_BYTE_ITEM_T		gcCmd_Prod_Id		PROGMEM = { 0,  255,	0, &gAppBootShareNV.productID,			TYPE_BYTE,		&gcCmd_Prod_Ver };

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
// Acknowledgment of {01?}  {productID; product version ID; software version; built date and time}
// Use gStrAcmdTmp to save RAM space because we don't care about the built time stamped.
CMD_STRING_ITEM_T	gcAckCmd_SoftBuiltTime	PROGMEM = { 0, sizeof(gcStrSOft_BuiltTime), &gcStrSOft_BuiltTime, &gStrAcmdTmp, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_STRING_ITEM_T	gcAckCmd_SoftBuiltDate	PROGMEM = { 0, sizeof(gcStrSOft_BuiltDate), &gcStrSOft_BuiltDate, &gStrAcmdTmp, TYPE_STRING, &gcAckCmd_SoftBuiltTime };
CMD_STRING_ITEM_T	gcAckCmd_SoftVersion	PROGMEM = { 0, MAX_SOFTWARE_VERSION_LENGTH, &gcStr5QuestionsMarks, &gtAckProductInfoRemoteDev.softVersionStr, TYPE_STRING, &gcAckCmd_SoftBuiltDate };
CMD_BYTE_ITEM_T		gcAckCmd_Prod_Ver		PROGMEM = { 0,  255,	0, &gtAckProductInfoRemoteDev.productVersionID,	TYPE_STRING,	&gcAckCmd_SoftVersion };
CMD_BYTE_ITEM_T		gcAckCmd_Prod_Id		PROGMEM = { 0,  255,	0, &gtAckProductInfoRemoteDev.productID,	TYPE_BYTE,	&gcAckCmd_Prod_Ver };
#endif

// {02?}
// System Status: 
CMD_UINT16_ITEM_T   gcCmd_SysStatus PROGMEM = { 0, 0xFFFF, 0, &gwSysStatus, TYPE_END, CMD_NO_NEXT_ITEM };

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
// Acknowledgment of {02?}
// System Status:
CMD_UINT16_ITEM_T   gcAckCmd_SysStatus PROGMEM = { 0, 0xFFFF, 0, &gwAckSysStatus, TYPE_END, CMD_NO_NEXT_ITEM };
#endif

// {03=deviceID; userModelCode; userModelName; serialNum; serialNumProd}		device ID; user defined ModelCode; user Model Name; serial number for ScaleCore board; product serial number.
// NOTE: you need productID, productVersionID and userDefModelCode to identify an unique product.
// TODO: protect serial numbers.
CMD_UINT32_ITEM_T	gcCmd_Serial_Num_Prd	PROGMEM = { 0,  999999999,	0,	&gtProductInfoFNV.serialNumProduct,	TYPE_END,		CMD_NO_NEXT_ITEM };
CMD_UINT32_ITEM_T	gcCmd_Serial_Num		PROGMEM = { 0,  999999999,	0,	&gtProductInfoFNV.serialNum,		TYPE_UINT32,	&gcCmd_Serial_Num_Prd  };
CMD_STRING_ITEM_T	gcCmd_UserModel_Name	PROGMEM = { 0, MAX_MODEL_NAME_LENGTH, &gcStrDefUserModelName, &gtProductInfoFNV.modelName, TYPE_UINT32,	&gcCmd_Serial_Num };
CMD_BYTE_ITEM_T		gcCmd_UserModel_Code	PROGMEM = { 0,  0xFF,		0,	&gtProductInfoFNV.userDefModelCode,	TYPE_STRING,	&gcCmd_UserModel_Name };
CMD_BYTE_ITEM_T		gcCmd_Device_ID			PROGMEM = { 0,  0xFF, CMD_DEFAULT_DSC_DEVICE_ID, &gtProductInfoFNV.devID,	TYPE_BYTE,		&gcCmd_UserModel_Code };

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
// Acknowledgment {03ideviceID; userModelCode; userModelName; serialNum; serialNumProd}		device ID; user defined ModelCode; user Model Name; serial number for ScaleCore board; product serial number.
// NOTE: you need productID, productVersionID and userDefModelCode to identify an unique product.
CMD_UINT32_ITEM_T	gcAckCmd_Serial_Num_Prd	PROGMEM = { 0,  999999999,	0,	&gtAckProductInfoRemoteDev.serialNumProduct,	TYPE_END,		CMD_NO_NEXT_ITEM };
CMD_UINT32_ITEM_T	gcAckCmd_Serial_Num		PROGMEM = { 0,  999999999,	0,	&gtAckProductInfoRemoteDev.serialNum,			TYPE_UINT32,	&gcAckCmd_Serial_Num_Prd  };
CMD_STRING_ITEM_T	gcAckCmd_UserModel_Name	PROGMEM = { 0, MAX_MODEL_NAME_LENGTH, &gcStrDefUserModelName, &gtAckProductInfoRemoteDev.modelName, TYPE_UINT32,	&gcAckCmd_Serial_Num };
CMD_BYTE_ITEM_T		gcAckCmd_UserModel_Code	PROGMEM = { 0,  0xFF,		0,	&gtAckProductInfoRemoteDev.userDefModelCode,	TYPE_STRING,	&gcAckCmd_UserModel_Name };
CMD_BYTE_ITEM_T		gcAckCmd_Device_ID		PROGMEM = { 0,  0xFF, CMD_DEFAULT_REMOTE_DEVICE_ID, &gtAckProductInfoRemoteDev.devID,	TYPE_BYTE,	&gcAckCmd_UserModel_Code };
#endif

// {04=actionType; cmdlock; password}	Action type, cmdLock; password. Note that action type 0==normal, 1 == set password;
CMD_STRING_ITEM_T	gcCmd_Password_Str	PROGMEM = { 0, MAX_PASSWORD_LENGTH, &gcStrMasterPassword, &gStrPassword, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T   	gcCmd_Cmd_Lock		PROGMEM	= { 0, 255, CMD_UNLOCKED, &gbCmdLock,		TYPE_STRING,	&gcCmd_Password_Str };
CMD_BYTE_ITEM_T   	gcCmd_Password_Act	PROGMEM	= { 0, 255, PASSWORD_NORMAL_MODE,	&gbPassWordAct,	TYPE_BYTE,		&gcCmd_Cmd_Lock };
CMD_ACTION_ITEM_T   gcCmd_Password_Post_Action	PROGMEM = { cmd_execute_password_cmd, CMD_NO_NEXT_ITEM };

// {05=}
// Action command to output value and annunciator in packet format according to MSI_CB.  TBI
// {05=channel; valueType} return {05iChannel;valueType;data;unit;numLitSeg; anc1;anc2;anc3;anc4;SysErrorCode} or  {00r5;errorCode}
CMD_BYTE_ITEM_T   	gcCmd_Sensor_Data_Packe_Value_type		PROGMEM	= { 0, 255, SENSOR_VALUE_TYPE_GROSS, &gbCMDTmp[0],	TYPE_END,	CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T	gcCmd_Sensor_Data_Packet_Channel		PROGMEM = { 0,  MAX_NUM_SENSORS - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Sensor_Data_Packe_Value_type};
CMD_ACTION_ITEM_T   gcCmd_Sensor_Data_Packet_Post_Action	PROGMEM = { cmd_send_sensor_data_packet, CMD_NO_NEXT_ITEM };

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
// Acknowledgment {05iChannel;valueType;data;unit;numLitSeg; anc1;anc2;anc3;anc4;SysErrorCode} or  {00r5;errorCode}
CMD_BYTE_ITEM_T		gcAckCmd_Sensor_Data_Packet_sysErrorCode	PROGMEM = { 0,	255,	0, 					&gVsSensorInfo.sysErrorCodeForUser,	TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T		gcAckCmd_Sensor_Data_Packet_tripPoints		PROGMEM = { 0,	255,	0, 					&gVsSensorInfo.tripPoints,		TYPE_BYTE, &gcAckCmd_Sensor_Data_Packet_sysErrorCode };
CMD_BYTE_ITEM_T		gcAckCmd_Sensor_Data_Packet_status2			PROGMEM = { 0,	255,	0, 					&gVsSensorInfo.sensorStatus2,	TYPE_BYTE, &gcAckCmd_Sensor_Data_Packet_tripPoints};
CMD_BYTE_ITEM_T		gcAckCmd_Sensor_Data_Packet_status			PROGMEM = { 0,	255,	0, 					&gVsSensorInfo.sensorStatus, 	TYPE_BYTE, &gcAckCmd_Sensor_Data_Packet_status2};
CMD_BYTE_ITEM_T		gcAckCmd_Sensor_Data_Packet_runMode			PROGMEM = { 0,	255,	0, 					&gVsSensorInfo.runModes, 		TYPE_BYTE, &gcAckCmd_Sensor_Data_Packet_status};
CMD_BYTE_ITEM_T		gcAckCmd_Sensor_Data_Packet_numLitSeg		PROGMEM = { 0,	255,	0, 					&gVsSensorInfo.numLitSeg,		TYPE_BYTE, &gcAckCmd_Sensor_Data_Packet_runMode};
CMD_BYTE_ITEM_T		gcAckCmd_Sensor_Data_Packet_unit			PROGMEM = { 0,	SENSOR_UNIT_MAX,  0,		&gVsSensorInfo.viewCB.unit,		TYPE_BYTE, &gcAckCmd_Sensor_Data_Packet_numLitSeg};
CMD_STRING_ITEM_T  	gcAckCmd_Sensor_Data_Packe_Value_str		PROGMEM	= { 0,	VS_SENSOR_DATA_STR_LEN + 1, &gcStrFmtDash10LfCr_P, &gVsSensorInfo.dataStr, TYPE_BYTE, &gcAckCmd_Sensor_Data_Packet_unit };
CMD_BYTE_ITEM_T   	gcAckCmd_Sensor_Data_Packe_Value_type		PROGMEM	= { 0,	255, SENSOR_VALUE_TYPE_GROSS, &gVsSensorInfo.valueType,	TYPE_STRING,	&gcAckCmd_Sensor_Data_Packe_Value_str };
CMD_BYTE_ITEM_T		gcAckCmd_Sensor_Data_Packet_Channel			PROGMEM = { 0,	MAX_NUM_SENSORS - 1,	0,	&gVsSensorInfo.sensorID, TYPE_BYTE, &gcAckCmd_Sensor_Data_Packe_Value_type};
#endif

// 2011-07-22 -WFC- v
// {06=Fkey1; Fkey2; sysPowerOffMode; LedSleepMode; LedIntensity}
#if  ( CONFIG_4260IS == TRUE) // IS limits the power consumption
CMD_BYTE_ITEM_T		gcCmd_SysFeature_Led_Intensity	PROGMEM = { 0,  SYS_FEATURE_LED_INTENSITY_MAX_VALUE - 1, OFF,	&gtSystemFeatureFNV.ledIntensity,	TYPE_END,		CMD_NO_NEXT_ITEM };
#else
CMD_BYTE_ITEM_T		gcCmd_SysFeature_Led_Intensity	PROGMEM = { 0,  SYS_FEATURE_LED_INTENSITY_MAX_VALUE, OFF,	&gtSystemFeatureFNV.ledIntensity,	TYPE_END,		CMD_NO_NEXT_ITEM };
#endif
CMD_BYTE_ITEM_T		gcCmd_SysFeature_Led_Sleep		PROGMEM = { 0,  SYS_FEATURE_LED_SLEEP_MAX_VALUE,	 OFF,	&gtSystemFeatureFNV.ledSleep,	 TYPE_BYTE,	&gcCmd_SysFeature_Led_Intensity  };
CMD_BYTE_ITEM_T		gcCmd_SysFeature_Auto_Off_Mode	PROGMEM = { 0,	SYS_FEATURE_AUTO_OFF_MODE_MAX_VALUE, OFF,	&gtSystemFeatureFNV.autoOffMode, TYPE_BYTE,	&gcCmd_SysFeature_Led_Sleep };
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
CMD_BYTE_ITEM_T		gcCmd_SysFeature_FKey2			PROGMEM = { 0,  FUNC_KEY_MAX_NUM, FUNC_KEY_TEST, &gtSystemFeatureFNV.userKeyFunc[1],	TYPE_BYTE,	&gcCmd_SysFeature_Auto_Off_Mode };
CMD_BYTE_ITEM_T		gcCmd_SysFeature_FKey1			PROGMEM = { 0,  FUNC_KEY_MAX_NUM, FUNC_KEY_PHOLD, &gtSystemFeatureFNV.userKeyFunc[0],	TYPE_BYTE,	&gcCmd_SysFeature_FKey2 };
#else
CMD_BYTE_ITEM_T		gcCmd_SysFeature_FKey2			PROGMEM = { 0,  FUNC_KEY_MAX_NUM, FUNC_KEY_DISABLED, &gtSystemFeatureFNV.userKeyFunc[1],	TYPE_BYTE,	&gcCmd_SysFeature_Auto_Off_Mode };
CMD_BYTE_ITEM_T		gcCmd_SysFeature_FKey1			PROGMEM = { 0,  FUNC_KEY_MAX_NUM, FUNC_KEY_DISABLED, &gtSystemFeatureFNV.userKeyFunc[0],	TYPE_BYTE,	&gcCmd_SysFeature_FKey2 };
#endif
// 2011-07-22 -WFC- ^

// {07=systemRunningMode} Save configuration settings to non-volatile memory.
CMD_BYTE_ITEM_T   	gcCmd_Sys_Run_Mode				PROGMEM	= { 0, MAX_NUM_SYS_RUN_MODE, SYS_RUN_MODE_NORMAL,	&gbCmdSysRunMode,	TYPE_END,	CMD_NO_NEXT_ITEM };
CMD_ACTION_ITEM_T   gcCmd_Sys_Run_Mode_Post_Action	PROGMEM = { cmd_system_run_mode, CMD_NO_NEXT_ITEM };

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
// Acknowledgment of {07=systemRunningMode} Save configuration settings to non-volatile memory.
CMD_BYTE_ITEM_T   	gcAckCmd_Sys_Run_Mode			PROGMEM	= { 0, MAX_NUM_SYS_RUN_MODE, SYS_RUN_MODE_NORMAL,	&gbAckCmdSysRunMode,	TYPE_END,	CMD_NO_NEXT_ITEM };
#endif

// {08=state;code}	state 1=off for LEDs, CODE: 0== red LED, 1==blue LED, 2==green LED; 3== + Excitation; 4 == - Excitation; 5== turn off excitation; 6==RCAL H,L base on state; 7==EXTIO H, L based on state. 9 == night/day LED mode, L state = night, H == day.
// {08=param;code}  use as general purpose instruction command, more than just test hardware IOs.
CMD_BYTE_ITEM_T   	gcCmd_SC_HW_Test_Code	PROGMEM	= { 0, 11,   0,	&gbSCHWTestCode,	TYPE_END,	CMD_NO_NEXT_ITEM };
// 2014-10-20 -WFC- CMD_BYTE_ITEM_T   	gcCmd_SC_HW_Test_State	PROGMEM	= { 0, ON, OFF,	&gbSCHWTestState,	TYPE_BYTE,	&gcCmd_SC_HW_Test_Code };
CMD_BYTE_ITEM_T   	gcCmd_SC_HW_Test_State	PROGMEM	= { 0, 255, 0,	&gbSCHWTestState,	TYPE_BYTE,	&gcCmd_SC_HW_Test_Code };
CMD_ACTION_ITEM_T   gcCmd_SC_HW_Test_Action	PROGMEM = { cmd_execute_sc_hw_test_cmd, CMD_NO_NEXT_ITEM };

// 2011-08-18 -WFC- v
// {09=password, postAction}	password is always required for goto bootloader; post action code, 85 == master reset.
CMD_BYTE_ITEM_T		gcCmd_Goto_Bootloader_Post_Boot_Status	PROGMEM = { 0,  255,  0, &gbAcmdTmp1, TYPE_END, CMD_NO_NEXT_ITEM};
CMD_STRING_ITEM_T	gcCmd_Goto_Bootloader_Password_Str		PROGMEM = { 0, MAX_PASSWORD_LENGTH, &gcStrMasterPassword, &gStrPassword, TYPE_BYTE, &gcCmd_Goto_Bootloader_Post_Boot_Status};
// 2011-08-18 -WFC- ^
CMD_ACTION_ITEM_T   gcCmd_Goto_Bootloader_Post_Action	PROGMEM = { cmd_goto_bootloader_cmd, CMD_NO_NEXT_ITEM };


// {0A?SensorID;calPoint;adcCnt;value} this command only valid when the sensor is in calibration.
CMD_FLOAT_ITEM_T    gcCmd_Sensor_Cal_Point_Query_Value		PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gfAcmdTmp1, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_INT32_ITEM_T	gcCmd_Sensor_Cal_Point_Query_AdcCnt		PROGMEM = { -2147483648,  2147483647,  0, &gi32AcmdTmp1, TYPE_FLOAT, &gcCmd_Sensor_Cal_Point_Query_Value};
CMD_BYTE_ITEM_T		gcCmd_Sensor_Cal_Point_Query_CalPoint	PROGMEM = { 0,  MAX_CAL_POINTS - 1,  0, &gbAcmdTmp1, TYPE_INT32, &gcCmd_Sensor_Cal_Point_Query_AdcCnt};
CMD_BYTE_ITEM_T		gcCmd_Sensor_Cal_Point_Query_SensorID	PROGMEM = { 0,  MAX_NUM_CAL_SENSORS - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Sensor_Cal_Point_Query_CalPoint};

// {0B?channel} will return {0BiChannel, calOpStep, errorStatus }
// if calOpStep < CAL_STATUS_COMPLETED 254, then it is in cal mode.

CMD_BYTE_ITEM_T     gcCmd_Cal_error_Status		PROGMEM = { 0,  255, 0, &gabCalErrorStatus, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_CalOp_Step			PROGMEM = { 0,  255, 0, &gabCalOpStep, TYPE_BYTE, &gcCmd_Cal_error_Status };
CMD_1D_INDEX_ITEM_T gcCmd_CalOp_Status_Channel	PROGMEM = { 0,MAX_NUM_CAL_SENSORS - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_CalOp_Step };

// {0C=channel;unit;capacity;tmpzone} start calibration setup, tmpzone range 0 to 2;
// Calibration setup: 

CMD_BYTE_ITEM_T     gcCmd_Cal_tmp_zone PROGMEM = { 0,  CAL_MAX_TEMPERATURE_ZONE - 1,  0, &gabCalTmpZone, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_FLOAT_ITEM_T    gcCmd_Cal_Capacity PROGMEM = { 0.99,  3.402823E+38, 10000.0, &gafCal_capacity, TYPE_BYTE, &gcCmd_Cal_tmp_zone };
CMD_BYTE_ITEM_T     gcCmd_Cal_Unit     PROGMEM = { 0,  MAX_NUM_LOADCELL_UNITS,  0, &gabCalCB_unit, TYPE_FLOAT, &gcCmd_Cal_Capacity };
//CMD_BYTE_ITEM_T     gcCmd_Cal_ChnlType PROGMEM = { 0,  1,  0, &gabCalChannelType, TYPE_BYTE, &gcCmd_Cal_Unit };
CMD_1D_INDEX_ITEM_T	gcCmd_Cal_Channel  PROGMEM = { 0,  MAX_NUM_CAL_SENSORS - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Cal_Unit };

// {0D=channel; wantCbIndex; max_possible_num; iCountby; decPt}
// User should query this cmd, it responded with max possible number of cb,
// wantCbIndex, iCountby and decPt are the standard countby based on a given capacity.
// On the setting action, max_possible_num, iCountby and decPt will be ignor.
CMD_INT8_ITEM_T		gcCmd_Cal_CB_DecPt			PROGMEM = { -38, 38,  0, &gi8AcmdTmp1, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_UINT16_ITEM_T	gcCmd_Cal_CB_iV				PROGMEM = { 0,  0xFFFF, 1, &gu16AcmdTmp1, TYPE_INT8, &gcCmd_Cal_CB_DecPt };
CMD_BYTE_ITEM_T		gcCmd_Cal_CB_Max_Combin		PROGMEM = { 0,  255,  0, &gbAcmdTmp2, TYPE_UINT16, &gcCmd_Cal_CB_iV };
CMD_BYTE_ITEM_T		gcCmd_Cal_CB_WantCB_index	PROGMEM = { 0,  255,  0, &gbAcmdTmp1, TYPE_BYTE, &gcCmd_Cal_CB_Max_Combin };
CMD_BYTE_ITEM_T		gcCmd_Cal_CB_Channel		PROGMEM = { 0,  MAX_NUM_CAL_SENSORS - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Cal_CB_WantCB_index };
// NOTE even though it is an array index type, BUT you cannot set it as index type when you use non array type temporary global variables such as gbCMD_Index,,gu16AcmTmp1 etc.. to map to complex data structure.
// CMD_1D_INDEX_ITEM_T	gcCmd_Cal_CB_Channel	PROGMEM = { 0,  MAX_NUM_CAL_SENSORS - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Cal_CB_WantCB_index };

CMD_ACTION_ITEM_T   gcCmd_Cal_CB_Post_Action	PROGMEM = { cmd_is_cal_countby_ok_mapper, CMD_NO_NEXT_ITEM };

// {0E=channel} set specified channel to a zero cal point.
CMD_1D_INDEX_ITEM_T	gcCmd_Cal_Zero_Channel		PROGMEM = { 0,  MAX_NUM_CAL_SENSORS - 1,  0, &gbCMD_Index, TYPE_END, CMD_NO_NEXT_ITEM};
CMD_ACTION_ITEM_T   gcCmd_Cal_zero_Post_Action	PROGMEM = { cmd_is_zero_cal_point_ok, CMD_NO_NEXT_ITEM };


// {0F=channel; fvalue} set value to a specified channel that corresponds to the current ADC count.
CMD_FLOAT_ITEM_T    gcCmd_Cal_Value				PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gafCalValue, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T	gcCmd_Cal_Value_Channel		PROGMEM = { 0,  MAX_NUM_CAL_SENSORS - 1,  0, &gbCMD_Index, TYPE_FLOAT, &gcCmd_Cal_Value};
CMD_ACTION_ITEM_T   gcCmd_Cal_Value_Post_Action	PROGMEM = { cmd_is_new_cal_point_ok, CMD_NO_NEXT_ITEM };

// {10=channel} save calibration of specified channel and exit.
CMD_1D_INDEX_ITEM_T	gcCmd_Cal_Save_Channel		PROGMEM = { 0,  MAX_NUM_CAL_SENSORS - 1,  0, &gbCMD_Index, TYPE_END, CMD_NO_NEXT_ITEM};
CMD_ACTION_ITEM_T   gcCmd_Cal_Save_Post_Action	PROGMEM = { cmd_save_cal_exit, CMD_NO_NEXT_ITEM };

// {11=channel} abort calibration of specified channel.
CMD_1D_INDEX_ITEM_T	gcCmd_Cal_No_Save_Channel		PROGMEM = { 0,  MAX_NUM_CAL_SENSORS - 1,  0, &gbCMD_Index, TYPE_END, CMD_NO_NEXT_ITEM};
CMD_ACTION_ITEM_T   gcCmd_Cal_No_Save_Post_Action	PROGMEM = { cmd_no_save_cal_exit, CMD_NO_NEXT_ITEM };

//  {12=ScaleStandard_mode} scale standard mode and AZM, zero on powerup, motion detect enable flags.
// 2011-07-21 -WFC- v
#if (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
CMD_BYTE_ITEM_T   	gcCmd_Scale_Standard_Mode		PROGMEM	= { 0, 255, SCALE_STD_MODE_MOTION_DETECT | SCALE_STD_MODE_AZM ,	&gbScaleStandardModeNV,	TYPE_END,	CMD_NO_NEXT_ITEM };
#else
CMD_BYTE_ITEM_T   	gcCmd_Scale_Standard_Mode		PROGMEM	= { 0, 255, SCALE_STD_MODE_MOTION_DETECT,	&gbScaleStandardModeNV,	TYPE_END,	CMD_NO_NEXT_ITEM };
#endif
// 2011-07-21 -WFC- ^

// {13=loadcellID; Rcal_on_enabled}
CMD_BYTE_ITEM_T     gcCmd_Rcal_Enabled				PROGMEM = { 0, ON, OFF, &gabLoadcellRcalEnabled, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T gcCmd_Rcal_Enabled_LC_ID		PROGMEM = { 0, MAX_NUM_RCAL_LOADCELL - 1, 0, &gbCMD_Index, TYPE_BYTE,  &gcCmd_Rcal_Enabled };
CMD_ACTION_ITEM_T   gcCmd_Rcal_Enabled_Post_Action	PROGMEM = { cmd_rcal_set_post_action, CMD_NO_NEXT_ITEM };

// {14?loadcellID;adcCnt;value} 
// CMD_FLOAT_ITEM_T    gcCmd_Sensor_Query_Value			PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gfAcmdTmp1, TYPE_END, CMD_NO_NEXT_ITEM };
// CMD_INT32_ITEM_T	gcCmd_Sensor_Query_Value_AdcCnt		PROGMEM = { -2147483648,  2147483647,  0, &gi32AcmdTmp1, TYPE_FLOAT, &gcCmd_Sensor_Query_Value };
CMD_STRING_ITEM_T	gcCmd_Sensor_Query_Value_Str		PROGMEM = { 0, CMD_MAX_TMP_STRING_LENGTH, gcStr0p0, gStrAcmdTmp, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_INT32_ITEM_T	gcCmd_Sensor_Query_Value_AdcCnt		PROGMEM = { -2147483648,  2147483647,  0, &gi32AcmdTmp1, TYPE_STRING, &gcCmd_Sensor_Query_Value_Str };
CMD_BYTE_ITEM_T		gcCmd_Sensor_Query_Value_SensorID	PROGMEM = { 0,  MAX_NUM_LOADCELL - 1,  0, &gbCMD_Index, TYPE_INT32, &gcCmd_Sensor_Query_Value_AdcCnt };


// {15=CalTalbeNum;sensorID;status;capacity;iCountBy;decPoint;unit;temp} This is for both query and configuration. A set action only allow in configuration mode.
// The float type of countby will generated by the CPU after received configuration command.
CMD_FLOAT_ITEM_T    gcCmd_Cnfg_Cal_HDR_Temp			PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gfAcmdTmp2, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T		gcCmd_Cnfg_Cal_HDR_Unit			PROGMEM = { 0,  SENSOR_UNIT_MAX - 1,  0, &gbAcmdTmp2, TYPE_FLOAT, &gcCmd_Cnfg_Cal_HDR_Temp};
CMD_INT8_ITEM_T		gcCmd_Cnfg_Cal_HDR_DecPt		PROGMEM = { -128, 127,  0, &gi8AcmdTmp1, TYPE_BYTE, &gcCmd_Cnfg_Cal_HDR_Unit};
CMD_UINT16_ITEM_T	gcCmd_Cnfg_Cal_HDR_CBi			PROGMEM = { 0,  0xFFFF, 1, &gu16AcmdTmp1, TYPE_INT8, &gcCmd_Cnfg_Cal_HDR_DecPt };
CMD_FLOAT_ITEM_T	gcCmd_Cnfg_Cal_HDR_Cap			PROGMEM = { -3.402823E+38,  3.402823E+38, 10000.0, &gfAcmdTmp1, TYPE_UINT16, &gcCmd_Cnfg_Cal_HDR_CBi};
CMD_BYTE_ITEM_T		gcCmd_Cnfg_Cal_HDR_Status		PROGMEM = { 0,  CAL_STATUS_UNCAL,  CAL_STATUS_UNCAL, &gbAcmdTmp1, TYPE_FLOAT, &gcCmd_Cnfg_Cal_HDR_Cap};
CMD_BYTE_ITEM_T		gcCmd_Cnfg_Cal_HDR_SensorID		PROGMEM = { 0,  MAX_NUM_CAL_SENSORS - 1,  0, &gbAcmdTmp3, TYPE_BYTE, &gcCmd_Cnfg_Cal_HDR_Status};
CMD_BYTE_ITEM_T		gcCmd_Cnfg_Cal_HDR_TableNum		PROGMEM = { 0,  CAL_MAX_NUM_CAL_TABLE - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Cnfg_Cal_HDR_SensorID};

CMD_ACTION_ITEM_T   gcCmd_Cnfg_Cal_HDR_Post_Action	PROGMEM = { cmd_cnfg_cal_hdr_post_action_mapper, CMD_NO_NEXT_ITEM };

// {16=CalTalbeNum;calPoint;adcCnt;value} This is for both query and configuration. A set action only allow in configuration mode.
CMD_FLOAT_ITEM_T    gcCmd_Cnfg_CalPoint_Value		PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gfAcmdTmp1, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_INT32_ITEM_T	gcCmd_Cnfg_CalPoint_AdcCnt		PROGMEM = { -2147483648,  2147483647,  0, &gi32AcmdTmp1, TYPE_FLOAT, &gcCmd_Cnfg_CalPoint_Value};
CMD_BYTE_ITEM_T		gcCmd_Cnfg_CalPoint_CalPoint	PROGMEM = { 0,  MAX_CAL_POINTS - 1,  0, &gbAcmdTmp1, TYPE_INT32, &gcCmd_Cnfg_CalPoint_AdcCnt};
CMD_BYTE_ITEM_T		gcCmd_Cnfg_CalPoint_TableNum	PROGMEM = { 0,  CAL_MAX_NUM_CAL_TABLE - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Cnfg_CalPoint_CalPoint};

CMD_ACTION_ITEM_T   gcCmd_Cnfg_CalPoint_Post_Action	PROGMEM = { cmd_cnfg_cal_point_post_action_mapper, CMD_NO_NEXT_ITEM };

// 2010-09-10 -WFC- v Added validation logics and prevent user enter bogus countby.
// {17=sensorID;iCountby;decpt;unit}
CMD_BYTE_ITEM_T     gcCmd_ViewingCountby_Unit	  		PROGMEM = { 0, SENSOR_UNIT_MAX - 1, 0, &gbAcmdTmp1, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_INT8_ITEM_T		gcCmd_ViewingCountby_DecPt			PROGMEM = { -38, 38,  0, &gi8AcmdTmp1, TYPE_BYTE, &gcCmd_ViewingCountby_Unit };
CMD_UINT16_ITEM_T	gcCmd_ViewingCountby_iV				PROGMEM = { 0,  0xFFFF, 1, &gu16AcmdTmp1, TYPE_INT8, &gcCmd_ViewingCountby_DecPt };
CMD_BYTE_ITEM_T		gcCmd_ViewingCountby_Sensor_Num		PROGMEM = { 0, MAX_NUM_SENSORS - 1,  0, &gbCMD_Index, TYPE_UINT16,  &gcCmd_ViewingCountby_iV };
// NOTE even though it is an array index type, BUT you cannot set it as index type when you use non array type temporary global variables such as gbCMD_Index,,gu16AcmTmp1 etc.. to map to complex data structure.
// CMD_1D_INDEX_ITEM_T gcCmd_ViewingCountby_Sensor_Num		PROGMEM = { 0, MAX_NUM_SENSORS - 1,  0, &gbCMD_Index, TYPE_UINT16,  &gcCmd_ViewingCountby_iV };

CMD_ACTION_ITEM_T   gcCmd_ViewingCountby_Post_Action	PROGMEM = { cmd_is_viewing_countby_ok_mapper, CMD_NO_NEXT_ITEM };

// 2010-08-30 -WFC- v
// {17=sensorID;iCountby;decpt;unit}
//CMD_BYTE_ITEM_T     gcCmd_CurCountby_Unit	  		PROGMEM = { 0, SENSOR_UNIT_MAX - 1, 0, &gabSensorShowCBunitsFNV, TYPE_END, CMD_NO_NEXT_ITEM };
//CMD_INT8_ITEM_T		gcCmd_CurCountby_DecPt			PROGMEM = { -38, 38,  0, &gabSensorShowCBdecPtFNV, TYPE_BYTE, &gcCmd_CurCountby_Unit };
//CMD_UINT16_ITEM_T	gcCmd_CurCountby_iV				PROGMEM = { 0,  0xFFFF, 1, &gawSensorShowCBFNV, TYPE_INT8, &gcCmd_CurCountby_DecPt };
//CMD_1D_INDEX_ITEM_T gcCmd_CurCountby_Sensor_Num		PROGMEM = { 0, MAX_NUM_SENSORS - 1,  0, &gbCMD_Index, TYPE_UINT16,  &gcCmd_CurCountby_iV };

// {17=sensorID;iCountby;decpt}
//CMD_INT8_ITEM_T		gcCmd_CurCountby_DecPt		PROGMEM = { -38, 38,  0, &gabSensorShowCBdecPtFNV, TYPE_END, CMD_NO_NEXT_ITEM };
//CMD_UINT16_ITEM_T	gcCmd_CurCountby_iV				PROGMEM = { 0,  0xFFFF, 1, &gawSensorShowCBFNV, TYPE_INT8, &gcCmd_CurCountby_DecPt };
//CMD_1D_INDEX_ITEM_T gcCmd_CurCountby_Sensor_Num	PROGMEM = { 0, MAX_NUM_SENSORS - 1,  0, &gbCMD_Index, TYPE_UINT16,  &gcCmd_CurCountby_iV };

//CMD_ACTION_ITEM_T   gcCmd_CurCountby_Post_Action	PROGMEM = { cmd_is_current_countby_ok, CMD_NO_NEXT_ITEM };

// {18=sensorID;unit}
// -WFC- 2010-08-30 CMD_BYTE_ITEM_T     gcCmd_CurUnit			  PROGMEM = { 0, SENSOR_UNIT_MAX - 1, 0, &gabSensorShowCBunitsFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_CurUnit			  PROGMEM = { 0, SENSOR_UNIT_MAX - 1, 0, &gabSensorViewUnitsFNV, TYPE_END, CMD_NO_NEXT_ITEM };	// -WFC- 2010-08-30
CMD_1D_INDEX_ITEM_T gcCmd_CurUnit_Sensor_Num  PROGMEM = { 0, MAX_NUM_SENSORS - 1, 0, &gbCMD_Index, TYPE_BYTE,  &gcCmd_CurUnit };
CMD_ACTION_ITEM_T   gcCmd_CurUnit_Post_Action PROGMEM = { cmd_current_unit_post_update, CMD_NO_NEXT_ITEM };
// 2010-08-30 -WFC- ^

// {19=sensorID;capacity; percentCapUnderLoad}
CMD_INT8_ITEM_T     gcCmd_CurCapacity_UnderLoad  PROGMEM = { -100, 90, -10, &gabPcentCapUnderloadFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_FLOAT_ITEM_T	gcCmd_CurCapacity			 PROGMEM = { -3.402823E+38,  3.402823E+38, 1.0, &gafSensorShowCapacityFNV, TYPE_INT8, &gcCmd_CurCapacity_UnderLoad };
CMD_1D_INDEX_ITEM_T gcCmd_CurCapacity_Sensor_Num PROGMEM = { 0, MAX_NUM_SENSORS - 1,  0, &gbCMD_Index, TYPE_FLOAT,  &gcCmd_CurCapacity };


// {1A=listenerNum;destID;streamType;sensorID;outputMode;interval} control output behavior of an IO_STREAM object specified by listenerNum.
CMD_BYTE_ITEM_T     gcCmd_Listener_Interval		PROGMEM = { 0, 255, TT_1SEC, &gabListenerIntervalFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_ListenerModes			PROGMEM = { 0, MAX_LISTENER_MODE, LISTENER_MODE_COMMAND, &gabListenerModesFNV, TYPE_BYTE, &gcCmd_Listener_Interval };
CMD_BYTE_ITEM_T     gcCmd_ListenerWant_SensorID	PROGMEM = { 0, MAX_NUM_SENSORS - 1, 0, &gabListenerWantSensorFNV, TYPE_BYTE, &gcCmd_ListenerModes };
CMD_BYTE_ITEM_T     gcCmd_Listener_StreamType	PROGMEM = { 0, MAX_STREAM_TYPE, IO_STREAM_TYPE_UART, &gabListenerStreamTypeFNV, TYPE_BYTE, &gcCmd_ListenerWant_SensorID };
CMD_BYTE_ITEM_T     gcCmd_Listener_DevID 		PROGMEM = { 0, 255, IO_STREAM_BROADCAST_ID, &gabListenerDevIdFNV, TYPE_BYTE, &gcCmd_Listener_StreamType };
CMD_1D_INDEX_ITEM_T gcCmd_Listener_ListenerNum	PROGMEM = { 0, MAX_NUM_STREAM_LISTENER - 1, 0, &gbCMD_Index, TYPE_BYTE,  &gcCmd_Listener_DevID };

CMD_ACTION_ITEM_T   gcCmd_ListenerModes_Post_Action	PROGMEM = { cmd_stream_out_modes_action, CMD_NO_NEXT_ITEM };

// {1B?stream registry number; sourceID; stream type}
CMD_BYTE_ITEM_T     gcCmd_Stream_Registry_Type	PROGMEM = { 0,  MAX_STREAM_TYPE, IO_STREAM_TYPE_UART,	&gaStreamReg_type,	TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_Stream_Registry_SrcID	PROGMEM = { 0,  255,   IO_STREAM_INVALID_SRCID, &gaStreamReg_sourceID, TYPE_BYTE, &gcCmd_Stream_Registry_Type };
CMD_1D_INDEX_ITEM_T gcCmd_Stream_Registry_Num	PROGMEM = { 0,MAX_NUM_STREAM_REGISTRY - 1,  0, &gbCMD_Index,	TYPE_BYTE, &gcCmd_Stream_Registry_SrcID };

// {1C=status; channel; networkID; devType; networkMode } configure RF device settings. 2012-04-24 -WFC-
CMD_UINT16_ITEM_T   gcCmd_Rf_Config_PowerLevel	PROGMEM = { 0,  4, 1, &gRfDeviceSettingsFNV.powerlevel, TYPE_END, CMD_NO_NEXT_ITEM }; // 2013-04-02 -DLM-
CMD_BYTE_ITEM_T     gcCmd_Rf_Config_NetworkMode	PROGMEM = { 0,  255,   RF_NETWORK_MODE_PEER_TO_PEER, &gRfDeviceSettingsFNV.networkMode, TYPE_UINT16, &gcCmd_Rf_Config_PowerLevel };
CMD_BYTE_ITEM_T     gcCmd_Rf_Config_DevType		PROGMEM = { 0,  255,   RF_DEVICE_TYPE_XBEE, &gRfDeviceSettingsFNV.deviceType, TYPE_BYTE,  &gcCmd_Rf_Config_NetworkMode };
CMD_UINT16_ITEM_T   gcCmd_Rf_Config_NetworkID	PROGMEM = { 0,  0xFFFF, 0x1A5D, &gRfDeviceSettingsFNV.networkID, TYPE_BYTE, &gcCmd_Rf_Config_DevType };
CMD_UINT16_ITEM_T   gcCmd_Rf_Config_Channel		PROGMEM = { 0,  0xFFFF, 15, &gRfDeviceSettingsFNV.channel, TYPE_UINT16, &gcCmd_Rf_Config_NetworkID };
CMD_BYTE_ITEM_T     gcCmd_Rf_Config_Status		PROGMEM = { 0,  255,    0, &gRfDeviceSettingsFNV.status, TYPE_UINT16, &gcCmd_Rf_Config_Channel	 };


CMD_ACTION_ITEM_T   gcCmd_Rf_Config_Post_Action		PROGMEM = { cmd_rf_config_post_action, CMD_NO_NEXT_ITEM };

// {1D=status} configure Ethernet device settings. 2012-07-09 -DLM-
CMD_BYTE_ITEM_T     gcCmd_Ethernet_Config_Status	PROGMEM = { 0,  255,    0, &gEthernetDeviceSettingsFNV.status, TYPE_END, CMD_NO_NEXT_ITEM };

//CMD_ACTION_ITEM_T   gcCmd_Ethernet_Config_Post_Action		PROGMEM = { cmd_ethernet_config_post_action, CMD_NO_NEXT_ITEM };


// {1D} Reserved Future Command.

// {1E=sensorID; features Enabled; cnvSpeed; sensorType}
CMD_BYTE_ITEM_T     gcCmd_Sensor_Cnfg_Type		PROGMEM = { 0,  255,  0, &gabSensorTypeFNV, TYPE_END, CMD_NO_NEXT_ITEM };
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
CMD_BYTE_ITEM_T     gcCmd_Sensor_Cnfg_Speed		PROGMEM = { 0,  255,  8, &gabSensorSpeedFNV, TYPE_BYTE,  &gcCmd_Sensor_Cnfg_Type };
CMD_BYTE_ITEM_T     gcCmd_Sensor_Cnfg_Features	PROGMEM = { 0,  255,  SENSOR_CNFG_FILTER_LEVEL_LOW, &gabSensorFeaturesFNV, TYPE_BYTE, &gcCmd_Sensor_Cnfg_Speed };
#else
CMD_BYTE_ITEM_T     gcCmd_Sensor_Cnfg_Speed		PROGMEM = { 0,  255,  8, &gabSensorSpeedFNV, TYPE_BYTE,  &gcCmd_Sensor_Cnfg_Type };
CMD_BYTE_ITEM_T     gcCmd_Sensor_Cnfg_Features	PROGMEM = { 0,  255,  1, &gabSensorFeaturesFNV, TYPE_BYTE, &gcCmd_Sensor_Cnfg_Speed };
#endif
CMD_1D_INDEX_ITEM_T gcCmd_Sensor_Cnfg_SensorID	PROGMEM = { 0, MAX_NUM_SENSORS - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Sensor_Cnfg_Features };

CMD_ACTION_ITEM_T   gcCmd_Sensor_Cnfg_Post_Action PROGMEM = { cmd_sensor_features_init, CMD_NO_NEXT_ITEM };

// {1F=loadcellID; opModes; mtnPeriod, mtnBandD, pendingTime } changed to 
CMD_BYTE_ITEM_T     gcCmd_LC_opModes_pending_time	PROGMEM = { 0,  255,   40, &gab_Stable_Pending_TimeNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_LC_opModes_MtnChk_Band	PROGMEM = { 0,  255,    3, &gabMotionDetectBand_dNV, TYPE_BYTE, &gcCmd_LC_opModes_pending_time };
CMD_BYTE_ITEM_T     gcCmd_LC_opModes_MtnChk_Period	PROGMEM = { 0,  255,   20, &gabMotionDetectPeriodTimeNV, TYPE_BYTE,  &gcCmd_LC_opModes_MtnChk_Band };
CMD_1D_INDEX_ITEM_T gcCmd_LC_opModes_LcID			PROGMEM = { 0,MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_LC_opModes_MtnChk_Period };

CMD_ACTION_ITEM_T   gcCmd_Lc_Update_Post_Action		PROGMEM = { cmd_loadcell_update_or_init, CMD_NO_NEXT_ITEM };

// 2011-11-21 -WFC- In command {20}, {21}, {22}, Changed powerup zero range from (+25,-20)% to (+20, -20)% for all Scale Modes.

// {20=loadcellID; %capZeroBandLo, %capZeroBandHi, azmCbRang, azmPeriod, %capPwupZeroBandLo, %capPwupZeroBandHi}
// 2011-08-16 -WFC- updated default to new legal for trade requirement.
CMD_BYTE_ITEM_T     gcCmd_LC_STD_PwupZeroBand_Hi	PROGMEM = { 1,   25,  20,		&gab_STD_pwupZeroBandHiNV, 		TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_LC_STD_PwupZeroBand_Lo	PROGMEM = { 1,   20,  20, 		&gab_STD_pwupZeroBandLoNV, 		TYPE_BYTE, &gcCmd_LC_STD_PwupZeroBand_Hi };
CMD_BYTE_ITEM_T     gcCmd_LC_STD_AZM_Period			PROGMEM = { TT_50mSEC, 255, TT_2SEC,	&gab_STD_AZM_IntervalTimeNV,	TYPE_BYTE, &gcCmd_LC_STD_PwupZeroBand_Lo };
CMD_BYTE_ITEM_T     gcCmd_LC_STD_AZM_CB_Range		PROGMEM = { 0,  255,   1,		&gab_STD_AZM_CBRangeNV,			TYPE_BYTE, &gcCmd_LC_STD_AZM_Period };
CMD_BYTE_ITEM_T     gcCmd_LC_STD_ZeroBand_Hi		PROGMEM = { 1,  100, 100,		&gab_STD_pcentCapZeroBandHiNV, TYPE_BYTE, &gcCmd_LC_STD_AZM_CB_Range };
CMD_BYTE_ITEM_T     gcCmd_LC_STD_ZeroBand_Lo		PROGMEM = { 1,   20,  20,		&gab_STD_pcentCapZeroBandLoNV, TYPE_BYTE, &gcCmd_LC_STD_ZeroBand_Hi };
CMD_1D_INDEX_ITEM_T gcCmd_LC_STD_LcID				PROGMEM = { 0,MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,			TYPE_BYTE, &gcCmd_LC_STD_ZeroBand_Lo };

// {21=loadcellID; %capZeroBandLo, %capZeroBandHi, azmCbRang, azmPeriod, %capPwupZeroBandLo, %capPwupZeroBandHi}
// 2011-08-16 -WFC- updated default to new legal for trade requirement.
// 2011-11-21 -WFC- Changed zero range from (3, -1)% to (+100, -20)%, for NTEP mode.
CMD_BYTE_ITEM_T     gcCmd_LC_NTEP_PwupZeroBand_Hi	PROGMEM = { 1,   25,  20,		&gab_NTEP_pwupZeroBandHiNV, 		TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_LC_NTEP_PwupZeroBand_Lo	PROGMEM = { 1,   20,  20, 		&gab_NTEP_pwupZeroBandLoNV, 		TYPE_BYTE, &gcCmd_LC_NTEP_PwupZeroBand_Hi };
CMD_BYTE_ITEM_T     gcCmd_LC_NTEP_AZM_Period		PROGMEM = { TT_50mSEC, 255, TT_1SEC,	&gab_NTEP_AZM_IntervalTimeNV,	TYPE_BYTE, &gcCmd_LC_NTEP_PwupZeroBand_Lo };
CMD_BYTE_ITEM_T     gcCmd_LC_NTEP_AZM_CB_Range		PROGMEM = { 0,  255,   0,		&gab_NTEP_AZM_CBRangeNV,			TYPE_BYTE, &gcCmd_LC_NTEP_AZM_Period };
CMD_BYTE_ITEM_T     gcCmd_LC_NTEP_ZeroBand_Hi		PROGMEM = { 1,  100, 100,		&gab_NTEP_pcentCapZeroBandHiNV,		TYPE_BYTE, &gcCmd_LC_NTEP_AZM_CB_Range };
CMD_BYTE_ITEM_T     gcCmd_LC_NTEP_ZeroBand_Lo		PROGMEM = { 1,   20,  20,		&gab_NTEP_pcentCapZeroBandLoNV, 	TYPE_BYTE, &gcCmd_LC_NTEP_ZeroBand_Hi };
CMD_1D_INDEX_ITEM_T gcCmd_LC_NTEP_LcID				PROGMEM = { 0,MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,				TYPE_BYTE, &gcCmd_LC_NTEP_ZeroBand_Lo };

// {22=loadcellID; %capZeroBandLo, %capZeroBandHi, azmCbRang, azmPeriod, %capPwupZeroBandLo, %capPwupZeroBandHi}
// 2011-08-16 -WFC- updated default to new legal for trade requirement.
CMD_BYTE_ITEM_T     gcCmd_LC_OIML_PwupZeroBand_Hi	PROGMEM = { 1,   25,  20,		&gab_OIML_pwupZeroBandHiNV, 		TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_LC_OIML_PwupZeroBand_Lo	PROGMEM = { 1,   20,  20, 		&gab_OIML_pwupZeroBandLoNV, 		TYPE_BYTE, &gcCmd_LC_OIML_PwupZeroBand_Hi };
CMD_BYTE_ITEM_T     gcCmd_LC_OIML_AZM_Period		PROGMEM = { TT_50mSEC, 255, TT_1SEC,	&gab_OIML_AZM_IntervalTimeNV,	TYPE_BYTE, &gcCmd_LC_OIML_PwupZeroBand_Lo };
CMD_BYTE_ITEM_T     gcCmd_LC_OIML_AZM_CB_Range		PROGMEM = { 0,  255,   0,		&gab_OIML_AZM_CBRangeNV,			TYPE_BYTE, &gcCmd_LC_OIML_AZM_Period };
CMD_BYTE_ITEM_T     gcCmd_LC_OIML_ZeroBand_Hi		PROGMEM = { 1,  100,   3,		&gab_OIML_pcentCapZeroBandHiNV,		TYPE_BYTE, &gcCmd_LC_OIML_AZM_CB_Range };
CMD_BYTE_ITEM_T     gcCmd_LC_OIML_ZeroBand_Lo		PROGMEM = { 1,   20,   1,		&gab_OIML_pcentCapZeroBandLoNV, 	TYPE_BYTE, &gcCmd_LC_OIML_ZeroBand_Hi };
CMD_1D_INDEX_ITEM_T gcCmd_LC_OIML_LcID				PROGMEM = { 0,MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,				TYPE_BYTE, &gcCmd_LC_OIML_ZeroBand_Lo };


// 2012-10-04 #if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
// {23=loadcellID; LCopModes; totalMode; dropThreshold%cap; riseThreshold%cap; minStableTime}
CMD_BYTE_ITEM_T     gcCmd_LC_Total_MinStableTime	PROGMEM = { 0,  255, 0, 		&gabTotalMinStableTimeNV,		TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_LC_Total_RiseThrsh		PROGMEM = { 0,  99, 1, 		&gabTotalRiseThresholdPctCapNV, TYPE_BYTE, &gcCmd_LC_Total_MinStableTime };
CMD_BYTE_ITEM_T     gcCmd_LC_Total_DropThrsh		PROGMEM = { 0,  98, 0,		&gabTotalDropThresholdPctCapNV, TYPE_BYTE, &gcCmd_LC_Total_RiseThrsh };
CMD_BYTE_ITEM_T     gcCmd_LC_Total_Mode				PROGMEM = { 0,  LC_TOTAL_MODE_ON_COMMAND, LC_TOTAL_MODE_DISABLED, &gabTotalModeFNV, TYPE_BYTE, &gcCmd_LC_Total_DropThrsh };
CMD_BYTE_ITEM_T     gcCmd_LC_LC_opModes				PROGMEM = { 0,  255, 0x0, &gabLcOpModesFNV, TYPE_BYTE, &gcCmd_LC_Total_Mode };
CMD_1D_INDEX_ITEM_T gcCmd_LC_Total_LcID				PROGMEM = { 0,MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_BYTE, &gcCmd_LC_LC_opModes };
//#else
//// {23=loadcellID; LCopModes; totalMode; dropThreshold%cap; riseThreshold%cap; minStableTime}
//CMD_BYTE_ITEM_T     gcCmd_LC_Total_MinStableTime	PROGMEM = { 0,  255, 0, 		&gabTotalMinStableTimeNV,		TYPE_END,  CMD_NO_NEXT_ITEM };
//CMD_BYTE_ITEM_T     gcCmd_LC_Total_RiseThrsh		PROGMEM = { 0,  99, 11, 		&gabTotalRiseThresholdPctCapNV, TYPE_BYTE, &gcCmd_LC_Total_MinStableTime };
//CMD_BYTE_ITEM_T     gcCmd_LC_Total_DropThrsh		PROGMEM = { 0,  98, 10,		&gabTotalDropThresholdPctCapNV, TYPE_BYTE, &gcCmd_LC_Total_RiseThrsh };
//CMD_BYTE_ITEM_T     gcCmd_LC_Total_Mode				PROGMEM = { 0,  LC_TOTAL_MODE_ON_COMMAND, LC_TOTAL_MODE_DISABLED, &gabTotalModeFNV, TYPE_BYTE, &gcCmd_LC_Total_DropThrsh };
//CMD_BYTE_ITEM_T     gcCmd_LC_LC_opModes				PROGMEM = { 0,  255, 0x0, &gabLcOpModesFNV, TYPE_BYTE, &gcCmd_LC_Total_Mode };
//CMD_1D_INDEX_ITEM_T gcCmd_LC_Total_LcID				PROGMEM = { 0,MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_BYTE, &gcCmd_LC_LC_opModes };
//#endif

// {24=loadcellID; onAcceptLowerWt; onAcceptUpperWt}
CMD_FLOAT_ITEM_T	gcCmd_LC_Total_UpperLimitWt		PROGMEM = { -3.402823E+38,  3.402823E+38, 2000.0, &gafTotalOnAcceptUpperWtNV, TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_FLOAT_ITEM_T	gcCmd_LC_Total_LowerLimitWt		PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gafTotalOnAcceptLowerWtNV, TYPE_FLOAT, &gcCmd_LC_Total_UpperLimitWt };
CMD_1D_INDEX_ITEM_T gcCmd_Lc_Total_Thresh_LcID		PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1,  0, 		&gbCMD_Index,	TYPE_FLOAT, &gcCmd_LC_Total_LowerLimitWt  };

CMD_ACTION_ITEM_T   gcCmd_Lc_Total_Thresh_Post_Action PROGMEM = { cmd_lc_total_thresh_post_update, CMD_NO_NEXT_ITEM };

// {25?loadcellID} outputs total weight, number of total count, read only.
CMD_UINT16_ITEM_T   gcCmd_LC_Total_Wt_Num_Total	PROGMEM = { 0,  0xFFFF, 0, &gawNumTotalFNV, TYPE_END, CMD_NO_NEXT_ITEM };				// 2011-07-27 -WFC-
// CMD_FLOAT_ITEM_T    gcCmd_LC_Total_Wt			PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gafTotalWtFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_FLOAT_ITEM_T    gcCmd_LC_Total_Wt				PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gafTotalWtFNV, TYPE_UINT16, &gcCmd_LC_Total_Wt_Num_Total };
CMD_1D_INDEX_ITEM_T gcCmd_LC_Total_Wt_LcID			PROGMEM = { 0,MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_FLOAT, &gcCmd_LC_Total_Wt };

CMD_ACTION_ITEM_T   gcCmd_LC_Total_Wt_Post_Action	PROGMEM = { cmd_lc_total_wt_read_post_action, CMD_NO_NEXT_ITEM };	// 2010-11-10 -WFC-

// {26?loadcellID} outputs total statistics, read only.

CMD_FLOAT_ITEM_T    gcCmd_LC_Total_Stat_MaxWt		PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gafMaxTotalWtFNV, TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_FLOAT_ITEM_T    gcCmd_LC_Total_Stat_MinWt		PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gafMinTotalWtFNV, TYPE_FLOAT, &gcCmd_LC_Total_Stat_MaxWt };
CMD_UINT16_ITEM_T   gcCmd_LC_Total_Stat_Num_Total	PROGMEM = { 0,  0xFFFF, 0, &gawNumTotalFNV, TYPE_FLOAT, &gcCmd_LC_Total_Stat_MinWt };
CMD_FLOAT_ITEM_T    gcCmd_LC_Total_Stat_SqWt		PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gafSumSqTotalWtFNV, TYPE_UINT16, &gcCmd_LC_Total_Stat_Num_Total };
CMD_1D_INDEX_ITEM_T gcCmd_LC_Total_Stat_LcID		PROGMEM = { 0,MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_FLOAT, &gcCmd_LC_Total_Stat_SqWt };

// {27=loadcellID} toggle net/gross.
CMD_1D_INDEX_ITEM_T gcCmd_NetGross_LcID					PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_ACTION_ITEM_T   gcCmd_Lc_Toggle_Net_Gross_Action	PROGMEM = { cmd_lc_toggle_net_gross_mode, CMD_NO_NEXT_ITEM };
   
// {28=loadcellID} zero command.
CMD_1D_INDEX_ITEM_T gcCmd_Zero_LcID			PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_ACTION_ITEM_T   gcCmd_Lc_Zero_Action	PROGMEM = { cmd_lc_zero, CMD_NO_NEXT_ITEM };

// {29=loadcellID} tare command.
CMD_1D_INDEX_ITEM_T gcCmd_Auto_Tare_LcID		PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_ACTION_ITEM_T   gcCmd_Lc_Auto_Tare_Action	PROGMEM = { cmd_lc_auto_tare, CMD_NO_NEXT_ITEM };

// {2A=loadcellID} clear tare command.
CMD_1D_INDEX_ITEM_T gcCmd_Clear_Tare_LcID		PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_ACTION_ITEM_T   gcCmd_Lc_Clear_Tare_Action	PROGMEM = { cmd_lc_clear_tare, CMD_NO_NEXT_ITEM };

// {2B=sensor number; name} name of a specified sensor number.
CMD_STRING_ITEM_T	gcCmd_Sensor_Name_Str PROGMEM = { 0, MAX_SENSOR_NAME_SIZE + 1, &gcStrNoName, &gabSensorNameFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T gcCmd_Sensor_Name_Num PROGMEM = { 0,MAX_NUM_SENSORS - 1,  0, &gbCMD_Index,	TYPE_STRING, &gcCmd_Sensor_Name_Str };

// {2C=setpoint_num;sensorID;cmplogic_valueMode;hystCB;fcmpValue} 
CMD_FLOAT_ITEM_T    gcCmd_Setpoint_Cmp_Value		PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gaSP_fCmpValueFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T     gcCmd_Setpoint_HystCB			PROGMEM = { 0,  99, 3, 		&gaSP_hysteresisCB_FNV, TYPE_FLOAT, &gcCmd_Setpoint_Cmp_Value };
CMD_BYTE_ITEM_T     gcCmd_Setpoint_Cmp_VMode		PROGMEM = { 0,  255, SP_CMP_LOGIC_GREATER_THAN | SP_CMP_VALUE_MODE_NET_GROSS,	&gaSP_cmp_logic_value_modeFNV, TYPE_BYTE, &gcCmd_Setpoint_HystCB };
CMD_BYTE_ITEM_T     gcCmd_Setpoint_Sensor_Num		PROGMEM = { 0,  MAX_NUM_SENSORS - 1, 0, &gaSP_sensorID_FNV, TYPE_BYTE, &gcCmd_Setpoint_Cmp_VMode };
CMD_1D_INDEX_ITEM_T gcCmd_Setpoint_Num				PROGMEM = { 0,SETPOINT_MAX_NUM - 1,  0, &gbCMD_Index,		TYPE_BYTE, &gcCmd_Setpoint_Sensor_Num };

CMD_ACTION_ITEM_T   gcCmd_Setpoint_Post_Action		PROGMEM = { cmd_setpoint_value_validation, CMD_NO_NEXT_ITEM };

// 2010-08-30 -WFC- V
// {2D?loadcellID} outputs {2Dix;liftCounter; overloadCounter}
// {2D?loadcellID} outputs {2Dix;liftCounter; overloadCounter; userLiftCounter}  2014-10-20 -WFC- v
CMD_UINT32_ITEM_T   gcCmd_Service_Cnt_UserLifCnt	PROGMEM = { 0, 0xFFFFFFFF, 0, &gaulUserLiftCntFNV,	 TYPE_END,	CMD_NO_NEXT_ITEM };
CMD_UINT32_ITEM_T   gcCmd_Service_Cnt_OverloadCnt	PROGMEM = { 0, 0xFFFFFFFF, 0, &gaulOverloadedCntFNV, TYPE_UINT32, &gcCmd_Service_Cnt_UserLifCnt };
CMD_UINT32_ITEM_T   gcCmd_Service_Cnt_liftCnt		PROGMEM = { 0, 0xFFFFFFFF, 0, &gaulLiftCntFNV,		 TYPE_UINT32, &gcCmd_Service_Cnt_OverloadCnt };
CMD_1D_INDEX_ITEM_T gcCmd_Service_Cnt_LcID			PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1, 0, &gbCMD_Index, TYPE_UINT32, &gcCmd_Service_Cnt_liftCnt };
// 2014-10-20 -WFC- ^

// {2D?loadcellID} outputs {2Dix;Over25%capCounter; overloadCounter}
//CMD_UINT16_ITEM_T   gcCmd_Service_Cnt_OverloadCnt	PROGMEM = { 0, 0xFFFF, 0, &gawOverloadedCntFNV,	TYPE_END,	CMD_NO_NEXT_ITEM };
//CMD_UINT16_ITEM_T   gcCmd_Service_Cnt_25perCapCnt	PROGMEM = { 0, 0xFFFF, 0, &gaw25perCapCntFNV,		TYPE_UINT16, &gcCmd_Service_Cnt_OverloadCnt };
//CMD_1D_INDEX_ITEM_T gcCmd_Service_Cnt_LcID		PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1, 0, &gbCMD_Index, TYPE_UINT16, &gcCmd_Service_Cnt_25perCapCnt };

// {2E=PV_loadcellID; liftThreshold; dropThreshold; status}
CMD_BYTE_ITEM_T   gcCmd_Service_Status				PROGMEM = { 0, 255, 0, &gabServiceStatusFNV,	TYPE_END,	CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T   gcCmd_Drop_Threshold_PctCap		PROGMEM = { 0, 100, 1, &gabDropThresholdPctCapFNV,	TYPE_BYTE,	&gcCmd_Service_Status };
CMD_BYTE_ITEM_T   gcCmd_Lift_Threshold_PctCap		PROGMEM = { 0, 100, 5, &gabLiftThresholdPctCapFNV,	TYPE_BYTE,	&gcCmd_Drop_Threshold_PctCap };
CMD_1D_INDEX_ITEM_T gcCmd_Lift_Drop_Threshold_LcID	PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1, 0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Lift_Threshold_PctCap };

// {2E=loadcellID} outputs {2Eix;liftCounter}
//CMD_UINT16_ITEM_T   gcCmd_Lift_Cnt_counter		PROGMEM = { 0, 0xFFFF, 0, &gawLiftCntFNV,	TYPE_END,	CMD_NO_NEXT_ITEM };
//CMD_1D_INDEX_ITEM_T gcCmd_Lift_Cnt_LcID			PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1, 0, &gbCMD_Index, TYPE_UINT16, &gcCmd_Lift_Cnt_counter };
// 2010-08-30 -WFC- ^

// {2F=state,code} test LED display board. for TESTONLY
// 2011-08-10 -WFC- v documentation does not match the command item order, I fixed it by re-order it. Now cmd {08} and {2F} have same items order.
//CMD_BYTE_ITEM_T   	gcCmd_LED_Map		PROGMEM	= { 0,  255,  0,	&gbSCHWTestState,	TYPE_END,	CMD_NO_NEXT_ITEM };
//CMD_BYTE_ITEM_T   	gcCmd_HW3460_Digit	PROGMEM	= { 0, 255, 0,	&gbSCHWTestCode,	TYPE_BYTE,	&gcCmd_LED_Map };

CMD_BYTE_ITEM_T   	gcCmd_HW3460_Digit	PROGMEM	= { 0, 255, 0,	&gbSCHWTestCode,	TYPE_END,	CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T   	gcCmd_LED_Map		PROGMEM	= { 0, 255, 0,	&gbSCHWTestState,	TYPE_BYTE,	&gcCmd_HW3460_Digit };
// 2011-08-10 -WFC- ^
CMD_ACTION_ITEM_T   gcCmd_HW3460_Action	PROGMEM = { cmd_execute_HW3460_cmd, CMD_NO_NEXT_ITEM };


// {30=dacChnl; status; sensorID; sensorUnit; sensorValueType; sensorVmin; sensorVmax}
// Where: status bit7 1 =calibrated, bit6 1 = enable; bit5 1 = in cal mode; bit4 1 = manual mode, 0=normal mode;
// sensorID==source sensor ID;
// sensorValueType == bit7: 1=no filtered; bit6:0 Value type, 0==gross; 1==net,,,6=ADC count etc..
// sensorVmin==source sensor value for min span point of DAC output.
// This command automatically save settings to non volatile memory.
//{{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Sensor_DAC_Chnl },		{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }},
CMD_FLOAT_ITEM_T    gcCmd_Sensor_DAC_snVmax		PROGMEM = { -3.402823E+38,  3.402823E+38, 1000.0, &gafDacSensorValueAtMaxSpanFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_FLOAT_ITEM_T    gcCmd_Sensor_DAC_snVmin		PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gafDacSensorValueAtMinSpanFNV, TYPE_FLOAT, &gcCmd_Sensor_DAC_snVmax };
CMD_BYTE_ITEM_T   	gcCmd_Sensor_DAC_valueType	PROGMEM = { 0,  255,	0, &gabDacSensorValueTypeFNV, TYPE_FLOAT, &gcCmd_Sensor_DAC_snVmin };
CMD_BYTE_ITEM_T   	gcCmd_Sensor_DAC_sensorUnit	PROGMEM = { 0,  MAX_NUM_LOADCELL_UNITS, 0, &gabDacSensorUnitFNV, TYPE_BYTE, &gcCmd_Sensor_DAC_valueType };
CMD_BYTE_ITEM_T   	gcCmd_Sensor_DAC_sensorID	PROGMEM = { 0,  MAX_NUM_SENSORS - 1, 0, &gabDacSensorIdFNV, TYPE_BYTE, &gcCmd_Sensor_DAC_sensorUnit };
CMD_BYTE_ITEM_T   	gcCmd_Sensor_DAC_status		PROGMEM = { 0,  255, DAC_STATUS_ENABLED, &gabDacCnfgStatusFNV, TYPE_BYTE, &gcCmd_Sensor_DAC_sensorID };
CMD_1D_INDEX_ITEM_T gcCmd_Sensor_DAC_Chnl		PROGMEM = { 0, MAX_DAC_CHANNEL - 1, 0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Sensor_DAC_status };

CMD_ACTION_ITEM_T   gcCmd_Sensor_DAC_Post_Action	PROGMEM = { cmd_execute_cnfg_sensor_dac_cmd, CMD_NO_NEXT_ITEM };

// {31=dacChnl; calType; opCode} where calType: 0 == Aborted; 1 == save cal and exit; 2==offset, 3==gain; 4==min span point; 5== max span point;
CMD_INT16_ITEM_T   	gcCmd_Cal_DAC_OpCode		PROGMEM = { -32768,  32767, 0, &gi16AcmdTmp1, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T   	gcCmd_Cal_DAC_CalType		PROGMEM = { 0,  DAC_CAL_TYPE_MAX_TYPE, DAC_CAL_TYPE_OFFSET, &gbAcmdTmp1, TYPE_INT16, &gcCmd_Cal_DAC_OpCode };
CMD_BYTE_ITEM_T		gcCmd_Cal_DAC_Chnl			PROGMEM = { 0, MAX_DAC_CHANNEL - 1, 0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Cal_DAC_CalType };

CMD_ACTION_ITEM_T   gcCmd_Cal_DAC_Post_Action	PROGMEM = { cmd_execute_cal_dac_cmd_mapper, CMD_NO_NEXT_ITEM };

// {32=dacChnl; offset; gain; CntMinSpan; CntMaxSpan} where CntMinSpan is the DAC count for Min DAC output of min value span point.
// This command automatically save settings to non volatile memory.
CMD_UINT16_ITEM_T  	gcCmd_Intern_Setting_DAC_MaxSpan	PROGMEM = { 0, MAX_DAC_VALUE, MAX_DAC_VALUE, &gawDacCountMaxSpanFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_UINT16_ITEM_T  	gcCmd_Intern_Setting_DAC_MinSpan	PROGMEM = { 0, MAX_DAC_VALUE, 0, &gawDacCountMinSpanFNV, TYPE_UINT16, &gcCmd_Intern_Setting_DAC_MaxSpan };
CMD_INT8_ITEM_T   	gcCmd_Intern_Setting_DAC_Gain		PROGMEM = { MIN_DAC_GAIN, MAX_DAC_GAIN, 0, &gabDacGainFNV, TYPE_UINT16, &gcCmd_Intern_Setting_DAC_MinSpan };
CMD_INT8_ITEM_T   	gcCmd_Intern_Setting_DAC_Offset		PROGMEM = { MIN_DAC_OFFSET, MAX_DAC_OFFSET, 0, &gabDacOffsetFNV, TYPE_INT8, &gcCmd_Intern_Setting_DAC_Gain };
CMD_1D_INDEX_ITEM_T gcCmd_Intern_Setting_DAC_Chnl		PROGMEM = { 0, MAX_DAC_CHANNEL - 1, 0, &gbCMD_Index, TYPE_INT8, &gcCmd_Intern_Setting_DAC_Offset };

CMD_ACTION_ITEM_T   gcCmd_Intern_Setting_DAC_Post_Action PROGMEM = { cmd_execute_cnfg_sensor_dac_cmd, CMD_NO_NEXT_ITEM };

// inserted cmd {33} for manual force output DAC.
// {33=dacChnl; DacCnt} manual force output DAC. Note that in order to make this work, you have to set the DAC status to manual mode with cmd {30}.
CMD_UINT16_ITEM_T  	gcCmd_Manual_Out_DacCnt			PROGMEM = { 0, MAX_DAC_VALUE, 0, &gu16AcmdTmp1, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T		gcCmd_Manual_Out_DAC_Chnl		PROGMEM = { 0, MAX_DAC_CHANNEL - 1, 0, &gbCMD_Index, TYPE_UINT16, &gcCmd_Manual_Out_DacCnt };

CMD_ACTION_ITEM_T   gcCmd_Manual_Out_Post_Action	PROGMEM = { cmd_execute_manual_out_dac_cmd_mapper, CMD_NO_NEXT_ITEM };

// {34=mathChnl; SensorID; status; mathExprs} mathChnl; SensorID =  Sensor ID assigned to this math channel; status; mathExpr = math expression,
// 2010-08-30	-WFC- Obsoleted {34=mathChnl; SensorID; unit; status; mathExprs} mathChnl; SensorID =  Sensor ID assigned to this math channel; unit; status; mathExpr = math expression,
// 0+1+2 means value of sensor0 + sensor1 + sensor2. Now is only support + only.
CMD_STRING_ITEM_T	gcCmd_Cnfg_Math_Exprs 		PROGMEM = { 0, MAX_VS_RAW_EXPRS_SIZE + 1, &gcMathExprsDefault, &gabVSMathRawExprsFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T   	gcCmd_Cnfg_Math_Status		PROGMEM = { 0, 255, 0, &gaVsMathStatus, TYPE_STRING, &gcCmd_Cnfg_Math_Exprs };
// 2010-08-30	-WFC- CMD_BYTE_ITEM_T   	gcCmd_Cnfg_Math_Unit		PROGMEM = { 0, SENSOR_UNIT_MAX - 1, SENSOR_UNIT_LB, &gabVSMathUnitFNV, TYPE_BYTE, &gcCmd_Cnfg_Math_Status };
CMD_BYTE_ITEM_T   	gcCmd_Cnfg_Math_SensorID	PROGMEM = { SENSOR_NUM_MATH_LOADCELL,  SENSOR_NUM_MATH_LOADCELL, SENSOR_NUM_MATH_LOADCELL, &gabVSMathSensorIdFNV, TYPE_BYTE, &gcCmd_Cnfg_Math_Status };	// 2010-08-30	-WFC-
// 2010-08-30	-WFC- CMD_BYTE_ITEM_T   	gcCmd_Cnfg_Math_SensorID	PROGMEM = { SENSOR_NUM_MATH_LOADCELL,  SENSOR_NUM_MATH_LOADCELL, SENSOR_NUM_MATH_LOADCELL, &gabVSMathSensorIdFNV, TYPE_BYTE, &gcCmd_Cnfg_Math_Unit };
CMD_1D_INDEX_ITEM_T gcCmd_Cnfg_Math_Chnl 		PROGMEM = { 0,MAX_NUM_VS_MATH - 1,  0, &gbCMD_Index,	TYPE_BYTE, &gcCmd_Cnfg_Math_SensorID };

CMD_ACTION_ITEM_T   gcCmd_Cnfg_Math_Chnl_Post_Action	PROGMEM = { cmd_execute_cnfg_math_channel_cmd, CMD_NO_NEXT_ITEM };

// {35?loadcellID; RcalValue}	loadcellID range 0-1 for ScaleCore2. RcalValue in ASCII, precision based on countby. Return {00rcmd;errorCode} if there is an error.
CMD_STRING_ITEM_T	gcCmd_Query_Rcal_Value_Str	PROGMEM = { 0, CMD_MAX_TMP_STRING_LENGTH, gcStr0p0, gStrAcmdTmp, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T gcCmd_Query_Rcal_Value_LcID	PROGMEM = { 0, MAX_NUM_RCAL_LOADCELL - 1, 0, &gbCMD_Index, TYPE_STRING, &gcCmd_Query_Rcal_Value_Str };

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
// Acknowledgment {35iloadcellID; RcalValue}	loadcellID range 0-1 for ScaleCore2. RcalValue in ASCII, precision based on countby. Return {00rcmd;errorCode} if there is an error.
CMD_STRING_ITEM_T		gcAckCmd_Query_Rcal_Value_Str	PROGMEM = { 0, CMD_MAX_TMP_STRING_LENGTH, gcStr0p0, gabAckRcalStr, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T		gcAckCmd_Query_Rcal_Value_LcID	PROGMEM = { 0, MAX_NUM_RCAL_LOADCELL - 1, 0, &gbCMD_Index, TYPE_STRING, &gcAckCmd_Query_Rcal_Value_Str };
#endif

// {36=loadcellID} Manual total.
CMD_1D_INDEX_ITEM_T gcCmd_Lc_Manual_Total_LcID		PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_ACTION_ITEM_T   gcCmd_Lc_Manual_Total_Action	PROGMEM = { cmd_execute_lc_manual_total, CMD_NO_NEXT_ITEM };

// {37=loadcellID} Manual Zero total.
CMD_1D_INDEX_ITEM_T	gcCmd_Lc_Zero_Total_LcID		PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_ACTION_ITEM_T   gcCmd_Lc_Zero_Total_Action		PROGMEM = { cmd_execute_lc_zero_total, CMD_NO_NEXT_ITEM };

// {38?loadcellID; inputStatus}	loadcellID range 0-1 for ScaleCore2. status code of loadcell bridge inputs. 0==NO error, otherwise a bad connection had occured.
CMD_BYTE_ITEM_T			gcCmd_LC_Input_Status		PROGMEM = { 0,  255, 0, 		&gabLoadcellBridgeInputsStatus,		TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T		gcCmd_LC_Input_Status_LcID	PROGMEM = { 0, MAX_NUM_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_BYTE, &gcCmd_LC_Input_Status };

// {39=sensorID, valueType; unit; maxNumSeg; 1stSegValue; lastSegValue}	sensorID: input sensor ID; valueType: input value type; unit: unit of sensor value; maxNumSeg = 0 means disabled; 1stSegValue: min value to lit up the 1st Segment; lastSegValue: value to lit up the last segment.
CMD_FLOAT_ITEM_T    gcCmd_Bargraph_Last_Seg_Value	PROGMEM = { -3.402823E+38,  3.402823E+38, 1.0, &gtSystemFeatureFNV.maxValueLastSegment, TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_FLOAT_ITEM_T    gcCmd_Bargraph_1st_Seg_Value	PROGMEM = { -3.402823E+38,  3.402823E+38, 0.0, &gtSystemFeatureFNV.minValue1stSegment, TYPE_FLOAT, &gcCmd_Bargraph_Last_Seg_Value };
CMD_BYTE_ITEM_T		gcCmd_Bargraph_MaxNumSeg		PROGMEM = { 0, BARGRAPH_MAX_NUM_SEGMENT, 21, &gtSystemFeatureFNV.maxNumSegment, TYPE_FLOAT, &gcCmd_Bargraph_1st_Seg_Value };
CMD_BYTE_ITEM_T		gcCmd_Bargraph_ValueUnit		PROGMEM = { 0, SENSOR_UNIT_MAX - 1, 0, &gtSystemFeatureFNV.bargraphValueUnit, TYPE_BYTE, &gcCmd_Bargraph_MaxNumSeg };
CMD_BYTE_ITEM_T		gcCmd_Bargraph_ValueType		PROGMEM = { 0, 255, SENSOR_VALUE_TYPE_GROSS | SENSOR_VALUE_TYPE_NON_FILTERED_bm, &gtSystemFeatureFNV.bargraphValueType, TYPE_BYTE, &gcCmd_Bargraph_ValueUnit };
CMD_BYTE_ITEM_T		gcCmd_Bargraph_SensorID			PROGMEM = { 0, MAX_NUM_SENSORS - 1, 0, &gtSystemFeatureFNV.bargraphSensorID, TYPE_BYTE, &gcCmd_Bargraph_ValueType };

CMD_ACTION_ITEM_T	gcCmd_Bargraph_Post_Action		PROGMEM = { cmd_execute_cnfg_bargraph_cmd, CMD_NO_NEXT_ITEM };

// {3A?sensorID,BargraphSegNum} READ ONLY. return sensorID, number of lit segment of the system bar graph based on sensor value
CMD_BYTE_ITEM_T		gcCmd_Bargraph_Query_NumSeg				PROGMEM = { 0, 255, 0, &gbBargraphNumLitSeg, TYPE_END,  CMD_NO_NEXT_ITEM  };
CMD_BYTE_ITEM_T		gcCmd_Bargraph_Query_NumSeg_SensorID	PROGMEM = { 0, MAX_NUM_SENSORS - 1, 0, &gtSystemFeatureFNV.bargraphSensorID, TYPE_BYTE, &gcCmd_Bargraph_Query_NumSeg };

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
// Acknowledgment {3ArsensorID,BargraphSegNum} READ ONLY. return sensorID, number of lit segment of the system bar graph based on sensor value
CMD_BYTE_ITEM_T		gcAckCmd_Bargraph_Query_NumSeg			PROGMEM = { 0, 255, 0, &gbAckBargraphNumLitSeg, TYPE_END,  CMD_NO_NEXT_ITEM  };
CMD_BYTE_ITEM_T		gcAckCmd_Bargraph_Query_NumSeg_SensorID	PROGMEM = { 0, 255, 0, &gbAckBargraphSensorID, TYPE_BYTE,  &gcAckCmd_Bargraph_Query_NumSeg };
#endif

// {3B=password}	password is always required for software master reset to clear all configurations. If successful, no acknowledgment else it respond with {00rcmd;errorCode}
CMD_ACTION_ITEM_T   gcCmd_Master_Default_Sys_Cnfg_Post_Action	PROGMEM = { cmd_master_default_system_configuration_cmd, CMD_NO_NEXT_ITEM };

// {3C=password}	non volatile memories checks.
CMD_ACTION_ITEM_T   gcCmd_Memory_Check_Post_Action	PROGMEM = { cmd_memory_check_cmd, CMD_NO_NEXT_ITEM };

// {3D=loadcellID} undo zero command.
CMD_1D_INDEX_ITEM_T gcCmd_Undo_Zero_LcID		PROGMEM = { 0, MAX_NUM_PV_LOADCELL - 1,  0, &gbCMD_Index,	TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_ACTION_ITEM_T   gcCmd_Lc_Undo_Zero_Action	PROGMEM = { cmd_lc_undo_zero, CMD_NO_NEXT_ITEM };

// {3E=password}	password is always required for clear all service counters, status flags of all loadcell.
CMD_ACTION_ITEM_T   gcCmd_Clear_Service_Counters_Post_Action	PROGMEM = { cmd_clear_service_counters_cmd, CMD_NO_NEXT_ITEM };

// {3F=loadcellNum; opMode; sampleSpeed} This is for both query and configuration. opMode: 0==disable peakhold, 1==enabled peakhold, 2== zero peak hold.; sample speed 0 to 15.
CMD_BYTE_ITEM_T		gcCmd_Cnfg_Peakhold_SampleSpeed	PROGMEM = { 0,  15, 3, &gbAcmdTmp2, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_BYTE_ITEM_T		gcCmd_Cnfg_Peakhold_opMode		PROGMEM = { 0,  2,  0, &gbAcmdTmp1, TYPE_BYTE, &gcCmd_Cnfg_Peakhold_SampleSpeed};
CMD_BYTE_ITEM_T		gcCmd_Cnfg_Peakhold_LoadcellNum	PROGMEM = { 0, MAX_NUM_LOADCELL - 1,  0, &gbCMD_Index, TYPE_BYTE, &gcCmd_Cnfg_Peakhold_opMode};

CMD_ACTION_ITEM_T   gcCmd_Cnfg_Peakhold_Post_Action	PROGMEM = { cmd_cnfg_peakhold_post_action_mapper, CMD_NO_NEXT_ITEM };

// 2011-07-25 -WFC- v
// {40=listenerNum;CtrlMode;outputInterval;composite} configuration of print string of a listener.
CMD_UINT32_ITEM_T	gcCmd_Print_String_Cnfg_Composite	PROGMEM = { 1, 99999, 12345, &gaulPrintStringCompositeFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_UINT16_ITEM_T	gcCmd_Print_String_Cnfg_Interval	PROGMEM = { 0, 65535, 2, &gawPrintStringIntervalFNV, TYPE_UINT32, &gcCmd_Print_String_Cnfg_Composite };
CMD_BYTE_ITEM_T		gcCmd_Print_String_Cnfg_Ctrl_Mode 	PROGMEM = { 0, 3, OFF, &gabPrintStringCtrlModeFNV, TYPE_UINT16, &gcCmd_Print_String_Cnfg_Interval };
CMD_1D_INDEX_ITEM_T gcCmd_Print_String_Cnfg_ListenerNum	PROGMEM = { 0, MAX_NUM_STREAM_LISTENER - 1, 0, &gbCMD_Index, TYPE_BYTE,  &gcCmd_Print_String_Cnfg_Ctrl_Mode };

CMD_ACTION_ITEM_T   gcCmd_Print_String_Cnfg_Action	PROGMEM = { cmd_print_string_cnfg_post_action, CMD_NO_NEXT_ITEM };

// {41=FormatterNumber; Formatter} Configure formatter of a specified formatter number.
CMD_STRING_ITEM_T	gcCmd_Formatter_Str PROGMEM = { 0, PRINT_STRING_MAX_FORMATER_LENGTH + 1, &gcStrDefPrintFormatter0, &gabPrintStringUserFormatterFNV, TYPE_END, CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T gcCmd_Formatter_Num PROGMEM = { 0, PRINT_STRING_MAX_NUM_FORMATER - 1,  0, &gbCMD_Index,	TYPE_STRING, &gcCmd_Formatter_Str };

CMD_ACTION_ITEM_T   gcCmd_Formatter_Action	PROGMEM = { cmd_formatter_post_action, CMD_NO_NEXT_ITEM };

// {42=listenerNum} output print string to specified listener.
CMD_1D_INDEX_ITEM_T gcCmd_Print_String			PROGMEM = { 0, MAX_NUM_STREAM_LISTENER - 1,  0, &gbCMD_Index,	TYPE_END,  CMD_NO_NEXT_ITEM };
CMD_ACTION_ITEM_T   gcCmd_Print_String_Action	PROGMEM = { cmd_print_string_post_action, CMD_NO_NEXT_ITEM };
// 2011-07-25 -WFC- ^

// Action command to output gross, net, tare weights, liftCnt|TtlCnt, cb, and annunciator in packet format.
// {43=lc} return {43iLC;grossWt; netWt; tareWt; LiftCnt|TtlCnt; iCb; decPoint; unit; numSeg; annunciator1; annunciator2; annunciator3; annunciator4; SysErrorCode}
CMD_1D_INDEX_ITEM_T	gcCmd_Loadcell_Gnt_Packet_Lc		PROGMEM = { 0,  MAX_NUM_PV_LOADCELL - 1 ,  0, &gbCMD_Index, TYPE_END,	CMD_NO_NEXT_ITEM};
CMD_ACTION_ITEM_T   gcCmd_Loadcell_Gnt_Packet_Post_Action	PROGMEM = { cmd_send_loadcell_gnt_packet, CMD_NO_NEXT_ITEM };

// 2012-02-13 -WFC- v
// Action command to output valueType, data, liftCnt|TtlCnt, cb, and annunciator in packet format.
// {44=lc; valueType} return {44iLC; valueType; data; LiftCnt|TtlCnt; iCb; decPoint; unit; annunciator1; annunciator2; annunciator3; annunciator4}
CMD_BYTE_ITEM_T   	gcCmd_Loadcell_Vcb_Packet_Value_type	PROGMEM	= { 0, 255, SENSOR_VALUE_TYPE_GROSS, &gbCMDTmp[0],	TYPE_END,	CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T	gcCmd_Loadcell_Vcb_Packet_Lc			PROGMEM = { 0,  MAX_NUM_PV_LOADCELL - 1 ,  0, &gbCMD_Index, TYPE_BYTE,	&gcCmd_Loadcell_Vcb_Packet_Value_type};
CMD_ACTION_ITEM_T   gcCmd_Loadcell_Vcb_Packet_Post_Action	PROGMEM = { cmd_send_loadcell_vcb_packet, CMD_NO_NEXT_ITEM };

// Action command to output valueType, data, liftCnt|TtlCnt, cb, and annunciator in packet format.
// {45=lc; valueType} return {45iLC; data}
CMD_BYTE_ITEM_T   	gcCmd_Loadcell_Simple_Packet_Value_type		PROGMEM	= { 0, 255, SENSOR_VALUE_TYPE_GROSS, &gbCMDTmp[0],	TYPE_END,	CMD_NO_NEXT_ITEM };
CMD_1D_INDEX_ITEM_T	gcCmd_Loadcell_Simple_Packet_Lc				PROGMEM = { 0,  MAX_NUM_PV_LOADCELL - 1 ,  0, &gbCMD_Index, TYPE_BYTE,	&gcCmd_Loadcell_Simple_Packet_Value_type};
CMD_ACTION_ITEM_T   gcCmd_Loadcell_Simple_Packet_Post_Action	PROGMEM = { cmd_send_loadcell_simple_packet, CMD_NO_NEXT_ITEM };
// 2012-02-13 -WFC- ^

// Action command to toggle high resolution mode of a specified loadcell number.
// {46=lc} return {00r70;errorCode}
CMD_BYTE_ITEM_T		gcCmd_Toggle_HIRES_LoadcellNum	PROGMEM = { 0, MAX_NUM_LOADCELL - 1,  0, &gbCMD_Index, TYPE_END,	CMD_NO_NEXT_ITEM};
CMD_ACTION_ITEM_T   gcCmd_Toggle_HIRES_Post_Action	PROGMEM = { cmd_toggle_hires_post_action_mapper, CMD_NO_NEXT_ITEM };

// Action command to remove last total of a specified loadcell number.
// {47=lc} return {00r71;errorCode}
CMD_BYTE_ITEM_T		gcCmd_Remove_Last_Total_LoadcellNum	PROGMEM = { 0, MAX_NUM_LOADCELL - 1,  0, &gbCMD_Index, TYPE_END,	CMD_NO_NEXT_ITEM};
CMD_ACTION_ITEM_T   gcCmd_Remove_Last_Total_Post_Action	PROGMEM = { cmd_remove_last_total_post_action_mapper, CMD_NO_NEXT_ITEM };

// TYPE_ACTION only allow at the beginning of the command talbe in the
// CMD_DESCRIPTOR_T structure. It has no parameter CMD_NEXT_ITEM_T, however,
// it must have at least one CMD_ACTION_ITEM_T. The command format is {cmdg}, {cmdG} or {cmd=}.

// Command Descriptors, each line is a root of a command.
CMD_DESCRIPTOR_T    gcCommandTable[] PROGMEM = {
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_BYTE,		&gcCmd_Error_Cmd  },			{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},				// {00?} will return {00rcmd; error}	command; error of command (0==none). It will show this whenever a command has no default respond or there is an command error.
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_BYTE,		&gcCmd_Prod_Id},				{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},   			// {01?} returns {product ID; product version; software version; built date; time}
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_UINT16,		&gcCmd_SysStatus},				{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},				// {02?} System status

   {{CMD_TYPE_NORMAL | CMD_TYPE_PASSWORD_PROTECTED },	{ TYPE_BYTE,	&gcCmd_Device_ID },	{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},				// {03=deviceID; userModelCode, serialNum, serialNumProd}		device ID; user defined ModelCode; serial number for ScaleCore board; product serial number.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT },	{ TYPE_BYTE, &gcCmd_Password_Act },{ cmd_pre_password_action,	&gcCmd_Password_Post_Action }},			// {04=actionType; cmdlock; password}	Action type, cmdLock; password. Note that action type 0==normal, 1 == set password;

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX, &gcCmd_Sensor_Data_Packet_Channel},	{ CMD_NO_ACTION_ITEM, &gcCmd_Sensor_Data_Packet_Post_Action}}, 					// {05=channel; valueType} // Action command return {05iChannel;valueType;data;unit;numLitSeg; anc1;anc2;sysAnnunciator} or {00r5;errorCode}
   //{{CMD_TYPE_NORMAL}, { TYPE_BYTE, &gcCmd_Sensor_Data_Packet_Channel},	{ CMD_NO_ACTION_ITEM, &gcCmd_Sensor_Data_Packet_Post_Action}}, 					// {05=channel; valueType} // Action command return {05iChannel;valueType;data;unit;numLitSeg; anc1;anc2;sysAnnunciator} or {00r5;errorCode}
   // 2011-07-22 -WFC- {{CMD_TYPE_NORMAL}, { TYPE_ACTION, CMD_NO_NEXT_ITEM },	{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},							// {06g} Action command to output value and annunciator in none packet format.
   {{CMD_TYPE_NORMAL},	{ TYPE_BYTE,	&gcCmd_SysFeature_FKey1 },	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }},											// {06=Fkey1; Fkey2; sysPowerOffMode; LedSleepMode; LedIntensity} 2011-07-22 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,		&gcCmd_Sys_Run_Mode },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Sys_Run_Mode_Post_Action }},								// {07=sysRunMode} system running mode.
   {{CMD_TYPE_NORMAL | CMD_TYPE_PASSWORD_PROTECTED }, { TYPE_BYTE,	&gcCmd_SC_HW_Test_State },	{ CMD_NO_ACTION_ITEM,	&gcCmd_SC_HW_Test_Action }},		// {08=state;code}	state 1==off for LEDs, CODE: 0== red LED, 1==blue LED, 2==green LED; 3== + Excitation; 4 == - Excitation; 5== turn off excitation; 6==RCAL H,L base on state; 7==EXTIO H, L based on state.

   // 2011-08-18 -WFC- {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY}, { TYPE_STRING, &gcCmd_Password_Str },		{ cmd_pre_password_action,	&gcCmd_Goto_Bootloader_Post_Action }},	// {09=password}	password is always required for goto bootloader.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY}, { TYPE_STRING, &gcCmd_Goto_Bootloader_Password_Str },	{ cmd_pre_password_action,	&gcCmd_Goto_Bootloader_Post_Action }},	// {09=password, postAction}	password is always required for goto bootloader; post action status, bit0 ==1 master reset. // 2011-08-18 -WFC-

   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX}, { TYPE_BYTE,&gcCmd_Sensor_Cal_Point_Query_SensorID},{ cmd_sensor_cal_point_query_pre_action_mapper, CMD_NO_ACTION_ITEM }},
																																							// {0A?SensorID;calPoint;adcCnt;value} this command only valid when the sensor is in calibration.

//   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_CalOp_Status_Channel},	{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},   			// {0B?channel} returns {0BiChnl, calOpStep, errorStatus} where if calOpStep < CAL_STATUS_COMPLETED 254, then it is in cal mode.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY }, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_CalOp_Status_Channel},	{ cmd_query_cal_status_pre_action,		CMD_NO_ACTION_ITEM }}, 	// {0B?channel} returns {0BiChnl, calOpStep, errorStatus} where if calOpStep < CAL_STATUS_COMPLETED 254, then it is in cal mode. 2015-05-11 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_Channel},			{ cmd_is_ok_start_new_cal,	CMD_NO_ACTION_ITEM }},					// {0C=channel;unit;capacity;tmpzone}
   // Even though it is an array type, it involves non array global variables map to array of complex data structure, thus you CANNOT set it as TYPE_1D_INDEX.
   //   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY }, { TYPE_1D_INDEX, &gcCmd_Cal_CB_Channel},	{ cmd_cal_cb_query_action_mapper,	&gcCmd_Cal_CB_Post_Action }},	// {0D=channel; wantCbIndex; max_possible_num; iCountby; decPt}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE, &gcCmd_Cal_CB_Channel},	{ cmd_cal_cb_query_action_mapper,	&gcCmd_Cal_CB_Post_Action }}, 		// {0D=channel; wantCbIndex; max_possible_num; iCountby; decPt}
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_Zero_Channel},		{ CMD_NO_ACTION_ITEM,		&gcCmd_Cal_zero_Post_Action}},		// {0E=channel} 		set specified channel to a zero cal point.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_Value_Channel},		{ CMD_NO_ACTION_ITEM,		&gcCmd_Cal_Value_Post_Action }},	// {0F=channel; fvalue} set value to a specified channel that corresponds to the current ADC count. This value could be weight, temperature, etc...
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_Save_Channel },		{ CMD_NO_ACTION_ITEM,		&gcCmd_Cal_Save_Post_Action }},		// {10=channel} save calibration of specified channel and exit.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_No_Save_Channel },	{ CMD_NO_ACTION_ITEM,		&gcCmd_Cal_No_Save_Post_Action }},	// {11=channel} abort calibration of specified channel.

   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,						&gcCmd_Scale_Standard_Mode },	{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM  }},				//  {12=ScaleStandard_mode} scale standard mode and AZM, zero on powerup, motion detect enable flags.
   
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Rcal_Enabled_LC_ID },	{ CMD_NO_ACTION_ITEM,		&gcCmd_Rcal_Enabled_Post_Action }},	// {13=loadcellID; Rcal_On_Enabled}  1 == On, 0==off
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX },{TYPE_BYTE,	&gcCmd_Sensor_Query_Value_SensorID}, { cmd_sensor_value_query_pre_action_mapper, CMD_NO_ACTION_ITEM }},
																																							// {14?sensorID;adcCnt;value}	
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_UPDATE_ONLY_IN_CNFG_MODE | CMD_TYPE_PASSWORD_PROTECTED | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE,	&gcCmd_Cnfg_Cal_HDR_TableNum },	{ cmd_cnfg_cal_hdr_pre_action_mapper,	&gcCmd_Cnfg_Cal_HDR_Post_Action }},
																																							// {15=CalTalbeNum;sensorID;status;capacity;iCountBy;decPoint;unit;temp} This is for both query and configuration. A set action only allow in configuration mode.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_UPDATE_ONLY_IN_CNFG_MODE | CMD_TYPE_PASSWORD_PROTECTED | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE,	&gcCmd_Cnfg_CalPoint_TableNum },	{ cmd_cnfg_cal_point_pre_action_mapper, &gcCmd_Cnfg_CalPoint_Post_Action }},
																																							// {16=CalTalbe;calPoint;adcCnt;value} 
   // configuration in normal run mode.
   // the following commands will recompute countbys and save its value to FNV ram memory, not the actual non volatile memory.
   // 2010-09-10 -WFC- {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_CurCountby_Sensor_Num},	{ CMD_NO_ACTION_ITEM,		&gcCmd_CurCountby_Post_Action }},	// {17=sensorID;iCountby;decpt} This command will re-compute countby and save its value to nonvolatile memory.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE,	&gcCmd_ViewingCountby_Sensor_Num},	{ cmd_viewing_countby_query_action_mapper,	&gcCmd_ViewingCountby_Post_Action }},	// {17=sensorID;iCountby;decpt;unit} 2010-09-10 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_CurUnit_Sensor_Num},		{ cmd_current_unit_pre_update, &gcCmd_CurUnit_Post_Action }},	// {18=sensorID;unit} This command will re-compute countby and save its value to nonvolatile memory.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX,	&gcCmd_CurCapacity_Sensor_Num},	{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Update_Post_Action }},
																																							// {19=sensorID;capacity; percentCapUnderLoad}

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,	&gcCmd_Listener_ListenerNum},					{ CMD_NO_ACTION_ITEM,	&gcCmd_ListenerModes_Post_Action }},	// {1A=listenerNum;destID;streamType;sensorID;outputMode;interval} control output behavior of an IO_STREAM object specified by listenerNum.
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_Stream_Registry_Num },	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }},					// {1B?stream registry number; sourceID; stream type}

   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,		&gcCmd_Rf_Config_Status },		{ CMD_NO_ACTION_ITEM,		&gcCmd_Rf_Config_Post_Action }},						// {1C=status; channel; networkID; devType; networkMode } configure RF device settings. 2012-04-24 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,		&gcCmd_Ethernet_Config_Status },	{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},								// {1D=status} configure Ethernet device settings. 2012-07-09 -DLM-

   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_Sensor_Cnfg_SensorID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Sensor_Cnfg_Post_Action }},
																																							// {1E=sensorID; features Enabled; cnvSpeed; sensorType}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_opModes_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},		// {1F=loadcellID; mtnPeriod, mtnBandD, pendingTime}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_STD_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},			// {20=loadcellID; %capZeroBandLo, %capZeroBandHi, azmCbRang, azmPeriod, %capPwupZeroBandLo, %capPwupZeroBandHi} STD mode
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_NTEP_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},			// {21=loadcellID; %capZeroBandLo, %capZeroBandHi, azmCbRang, azmPeriod, %capPwupZeroBandLo, %capPwupZeroBandHi} NTEP mode
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_OIML_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},			// {22=loadcellID; %capZeroBandLo, %capZeroBandHi, azmCbRang, azmPeriod, %capPwupZeroBandLo, %capPwupZeroBandHi} OIML mode
   
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_Total_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},			// {23=loadcellID; LCopModes; totalMode; dropThreshold%cap; riseThreshold%cap; minStableTime}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_Lc_Total_Thresh_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Total_Thresh_Post_Action }},
																																							// {24=loadcellID; onAcceptLowerWt; onAcceptUpperWt}
   // 2010-11-10 -WFC-  {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_LC_Total_Wt_LcID},		{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }}, 				// {25?loadcellID} outputs total weight.
   {{CMD_TYPE_NORMAL | CMD_TYPE_HANDLE_RETURN_QUERY }, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_LC_Total_Wt_LcID},	{ CMD_NO_ACTION_ITEM,	&gcCmd_LC_Total_Wt_Post_Action }}, 	// {25?loadcellID} outputs total weight.
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_LC_Total_Stat_LcID},		{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }}, 				// {26?loadcellID} outputs total statistics.

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_NetGross_LcID },			{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Toggle_Net_Gross_Action }},		// {27=loadcellID} toggle net/gross.
   {{CMD_TYPE_NORMAL | CMD_TYPE_PASSWORD_PROTECTED },	{ TYPE_1D_INDEX,	&gcCmd_Zero_LcID },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Zero_Action }},		// {28=loadcellID} zero command.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Auto_Tare_LcID },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Auto_Tare_Action }},				// {29=loadcellID} tare command.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Clear_Tare_LcID },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Clear_Tare_Action }},				// {2A=loadcellID} clear tare command.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Sensor_Name_Num },		{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }},						// {2B=sensor number; name} name of a specified sensor number.

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Setpoint_Num },			{ CMD_NO_ACTION_ITEM,	&gcCmd_Setpoint_Post_Action }},				// {2C=setpoint_num;sensorID;cmplogic_valueMode;hystCB;fcmpValue}
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_Service_Cnt_LcID},	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }}, 						// {2D?loadcellID} outputs {2Dix;Over25%capCounter; overloadCounter}
   //2010-08-30 -WFC-   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Lift_Cnt_LcID},			{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }}, 	// {2E=loadcellID} outputs {2Eix;liftCounter}
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Lift_Drop_Threshold_LcID},	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }}, 					// {2E=PV_loadcellID; liftThreshold; dropThreshold; status}  2010-08-30 -WFC-
   //2011-08-10 -WFC-    {{CMD_TYPE_NORMAL}, { TYPE_BYTE,						&gcCmd_HW3460_Digit },			{ CMD_NO_ACTION_ITEM,	&gcCmd_HW3460_Action }},					// {2F=digitNum; imgMap} test LED display board. TESTONLY
   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,						&gcCmd_LED_Map },			{ CMD_NO_ACTION_ITEM,	&gcCmd_HW3460_Action }},						// {2F=digitNum; imgMap} test LED display board.

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Sensor_DAC_Chnl },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Sensor_DAC_Post_Action }},			// {30=dacChnl; status; sensorID; sensorUnit; sensorValueType; sensorVmin; sensorVmax} status bit7 1== calibrated, bit6 1==enable, bit4 1 = manual mode, 0=normal mode; sensorID==source sensor ID; sensorVmin==source sensor value for min span point of DAC output.
   {{CMD_TYPE_NORMAL | CMD_TYPE_DYNAMIC_INDEX  }, { TYPE_BYTE,	&gcCmd_Cal_DAC_Chnl },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Cal_DAC_Post_Action }},				// {31=dacChnl; calType; opCode} where calType: calType: 0 == Aborted; 1 == save cal and exit; 2==offset, 3==gain; 4==min span point; 5== max span point;
																																							//                        opCode: 0==init; else it is adjustment value range -32768 to 32767.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Intern_Setting_DAC_Chnl },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Intern_Setting_DAC_Post_Action }}, // {32=dacChnl; offset; gain; CntMinSpan; CntMaxSpan} where CntMinSpan is the DAC count for Min DAC output of min value span point.
   {{CMD_TYPE_NORMAL | CMD_TYPE_DYNAMIC_INDEX  }, { TYPE_BYTE,	&gcCmd_Manual_Out_DAC_Chnl },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Manual_Out_Post_Action }},	// {33=dacChnl; DacCnt} manual force output DAC.

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cnfg_Math_Chnl },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Cnfg_Math_Chnl_Post_Action }},		// {34=mathChnl; SensorID; overloadThrshUnit; overloadThreshold; mathExprs} mathChnl; SensorID =  Sensor ID assigned to this math channel; overload threshold unit; overload threshold; mathExpr = math expression,
   //{{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX },	{TYPE_READ_ONLY |TYPE_BYTE,	&gcCmd_Query_Rcal_Value_LcID}, { cmd_loadcell_rcal_query_pre_action_mapper, CMD_NO_ACTION_ITEM }},	// {35?loadcellID; RcalValue}	loadcellID range 0-1 for ScaleCore2. RcalValue in float, precision based on countby.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX },	{TYPE_BYTE,	&gcCmd_Query_Rcal_Value_LcID}, { cmd_loadcell_rcal_query_pre_action_mapper, CMD_NO_ACTION_ITEM }},	// {35?loadcellID; RcalValue}	loadcellID range 0-1 for ScaleCore2. RcalValue in float, precision based on countby.
   {{CMD_TYPE_NORMAL },	{TYPE_1D_INDEX,	&gcCmd_Lc_Manual_Total_LcID}, 					{  CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Manual_Total_Action }},			// {36=loadcellID} Manual total.
   {{CMD_TYPE_NORMAL },	{TYPE_1D_INDEX,	&gcCmd_Lc_Zero_Total_LcID}, 					{  CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Zero_Total_Action }},				// {37=loadcellID} Manual Zero total.
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_LC_Input_Status_LcID},	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }}, 					// {38?loadcellID; inputStatus}	loadcellID range 0-1 for ScaleCore2. status code of loadcell bridge inputs. 0==NO error, otherwise a bad connection had occurred.
   {{CMD_TYPE_NORMAL | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE,	&gcCmd_Bargraph_SensorID },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Bargraph_Post_Action  }},	// {39=sensorID, valueType; unit; maxNumSeg; 1stSegValue; lastSegValue}	sensorID: input sensor ID; valueType: input value type; unit: unit of sensor value; maxNumSeg = 0 means disabled; 1stSegValue: min value to lit up the 1st Segment; lastSegValue: value to lit up the last segment.
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_BYTE,	&gcCmd_Bargraph_Query_NumSeg_SensorID },	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM  }},			// {3A?sensorID,BargraphSegNum} READ ONLY. return sensorID and number of lit segment of the system bar graph based on sensor value.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY}, { TYPE_STRING, &gcCmd_Password_Str },	{ cmd_pre_password_action,	&gcCmd_Master_Default_Sys_Cnfg_Post_Action }},	// {3B=password}	password is always required for software master reset to clear all configurations. If successful, no acknowledgment else it responds with {00rcmd;errorCode}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY}, { TYPE_STRING, &gcCmd_Password_Str },	{ cmd_pre_password_action,	&gcCmd_Memory_Check_Post_Action }},		// {3C=password}	non volatile memories checks.
   {{CMD_TYPE_NORMAL },	{ TYPE_1D_INDEX,	&gcCmd_Undo_Zero_LcID },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Undo_Zero_Action }},		// {3D=loadcellID} undo zero command.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY}, { TYPE_STRING, &gcCmd_Password_Str },		{ cmd_pre_password_action,	&gcCmd_Clear_Service_Counters_Post_Action }},	// {3E=password}	password is always required for clear service counter and status flags.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE,	&gcCmd_Cnfg_Peakhold_LoadcellNum },	{ cmd_cnfg_peakhold_pre_action_mapper, &gcCmd_Cnfg_Peakhold_Post_Action }},
																																							// {3F=loadcellID;opCode;sensorSpeed}
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,	&gcCmd_Print_String_Cnfg_ListenerNum},	{ CMD_NO_ACTION_ITEM,	&gcCmd_Print_String_Cnfg_Action }},					// {40=listenerNum;CtrlMode;outputInterval;composite} configuration of print string of a listener.	2011-07-26 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,	&gcCmd_Formatter_Num },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Formatter_Action }},											// {41=FormatterNumber; Formatter} Configure formatter of a specified formatter number.				2011-07-26 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,	&gcCmd_Print_String },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Print_String_Action }},										// {42=listenerNum} output print string to specified listener.										2011-07-26 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX, &gcCmd_Loadcell_Gnt_Packet_Lc},	{ CMD_NO_ACTION_ITEM, &gcCmd_Loadcell_Gnt_Packet_Post_Action}},						// {43=loadcellNum} // Action command return {43iLC;grossWt; netWt; tareWt; LiftCnt|TtlCnt; iCb; decPoint; unit; numSeg; annunciator1; annunciator2; annunciator3; annunciator4; SysErrorCode},
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX, &gcCmd_Loadcell_Vcb_Packet_Lc},	{ CMD_NO_ACTION_ITEM, &gcCmd_Loadcell_Vcb_Packet_Post_Action}},						// {44=sensorID;valueType} // Action command return {44iLC;valueType; data; LiftCnt|TtlCnt; iCb; decPoint; unit; annunciator1; annunciator2; annunciator3; annunciator4} 2012-02-13 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX, &gcCmd_Loadcell_Simple_Packet_Lc},	{ CMD_NO_ACTION_ITEM, &gcCmd_Loadcell_Simple_Packet_Post_Action}},				// {45=sensorID;valueType} // Action command return {45iLC;data}, 	2012-02-13 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,	&gcCmd_Toggle_HIRES_LoadcellNum },	{ CMD_NO_ACTION_ITEM, &gcCmd_Toggle_HIRES_Post_Action }},							// {46=loadcellNum} 2012-02-23 -WFC- toggle high resolution display mode
   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,	&gcCmd_Remove_Last_Total_LoadcellNum },	{ CMD_NO_ACTION_ITEM, &gcCmd_Remove_Last_Total_Post_Action }}							// {47=loadcellNum} 2012-02-23 -WFC- remove last total.
};

#define MAX_CMD_ID  0x47

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-

////////////////////////// ******************************
// Command Descriptors for acknowledge command from a device, each line is a root of a command.
CMD_DESCRIPTOR_T    gcAckCommandTable[] PROGMEM = {
   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,		&gcAckCmd_Error_Cmd  },			{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},				// {00?} will return {00rcmd; error}	command; error of command (0==none). It will show this whenever a command has no default respond or there is an command error.
   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,		&gcAckCmd_Prod_Id},				{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},   			// {01?} returns {product ID; product version; software version; built date; time}
   {{CMD_TYPE_NORMAL}, { TYPE_UINT16,	&gcAckCmd_SysStatus},			{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},				// {02?} System status

   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,	&gcAckCmd_Device_ID },	{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},												// {03=deviceID; userModelCode, serialNum, serialNumProd}		device ID; user defined ModelCode; serial number for ScaleCore board; product serial number.
   {{CMD_TYPE_NORMAL}, { TYPE_END,						CMD_NO_NEXT_ITEM },				{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},					// {04=actionType; cmdlock; password}	Action type, cmdLock; password. Note that action type 0==normal, 1 == set password;

   {{CMD_TYPE_NORMAL}, { TYPE_BYTE, &gcAckCmd_Sensor_Data_Packet_Channel},					{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }},					// {05=channel; valueType} // Action command return {05iChannel;valueType;data;unit;numLitSeg; anc1;anc2;sysAnnunciator} or {00r5;errorCode}
   {{CMD_TYPE_NORMAL}, { TYPE_END,						CMD_NO_NEXT_ITEM },				{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},					// {06g} Action command to output value and annunciator in none packet format.
   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,						&gcAckCmd_Sys_Run_Mode },			{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }},					// {07=sysRunMode} system running mode.
   {{CMD_TYPE_NORMAL}, { TYPE_END,						CMD_NO_NEXT_ITEM },				{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},					// {08=state;code}	state 1==off for LEDs, CODE: 0== red LED, 1==blue LED, 2==green LED; 3== + Excitation; 4 == - Excitation; 5== turn off excitation; 6==RCAL H,L base on state; 7==EXTIO H, L based on state.

   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY}, { TYPE_STRING, &gcCmd_Password_Str },		{ cmd_pre_password_action,	&gcCmd_Goto_Bootloader_Post_Action }},	// {09=password}	password is always required for goto bootloader.

   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX}, { TYPE_BYTE,&gcCmd_Sensor_Cal_Point_Query_SensorID},{ cmd_sensor_cal_point_query_pre_action_mapper, CMD_NO_ACTION_ITEM }},
																																							// {0A?SensorID;calPoint;adcCnt;value} this command only valid when the sensor is in calibration.

   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_CalOp_Status_Channel},	{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},   			// {0B?channel} returns {0BiChnl, calOpStep, errorStatus} where if calOpStep < CAL_STATUS_COMPLETED 254, then it is in cal mode.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_Channel},			{ cmd_is_ok_start_new_cal,	CMD_NO_ACTION_ITEM }},					// {0C=channel;unit;capacity;tmpzone}
   // Even though it is an array type, it involves non array global variables map to array of complex data structure, thus you CANNOT set it as TYPE_1D_INDEX.
   //   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY }, { TYPE_1D_INDEX, &gcCmd_Cal_CB_Channel},	{ cmd_cal_cb_query_action_mapper,	&gcCmd_Cal_CB_Post_Action }},	// {0D=channel; wantCbIndex; max_possible_num; iCountby; decPt}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE, &gcCmd_Cal_CB_Channel},	{ cmd_cal_cb_query_action_mapper,	&gcCmd_Cal_CB_Post_Action }}, 		// {0D=channel; wantCbIndex; max_possible_num; iCountby; decPt}
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_Zero_Channel},		{ CMD_NO_ACTION_ITEM,		&gcCmd_Cal_zero_Post_Action}},		// {0E=channel} 		set specified channel to a zero cal point.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_Value_Channel},		{ CMD_NO_ACTION_ITEM,		&gcCmd_Cal_Value_Post_Action }},	// {0F=channel; fvalue} set value to a specified channel that corresponds to the current ADC count. This value could be weight, temperature, etc...
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_Save_Channel },		{ CMD_NO_ACTION_ITEM,		&gcCmd_Cal_Save_Post_Action }},		// {10=channel} save calibration of specified channel and exit.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cal_No_Save_Channel },	{ CMD_NO_ACTION_ITEM,		&gcCmd_Cal_No_Save_Post_Action }},	// {11=channel} abort calibration of specified channel.

   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,						&gcCmd_Scale_Standard_Mode },	{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM  }},				//  {12=ScaleStandard_mode} scale standard mode and AZM, zero on powerup, motion detect enable flags.

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Rcal_Enabled_LC_ID },	{ CMD_NO_ACTION_ITEM,		&gcCmd_Rcal_Enabled_Post_Action }},	// {13=loadcellID; Rcal_On_Enabled}  1 == On, 0==off
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX },{TYPE_BYTE,	&gcCmd_Sensor_Query_Value_SensorID}, { cmd_sensor_value_query_pre_action_mapper, CMD_NO_ACTION_ITEM }},
																																							// {14?sensorID;adcCnt;value}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_UPDATE_ONLY_IN_CNFG_MODE | CMD_TYPE_PASSWORD_PROTECTED | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE,	&gcCmd_Cnfg_Cal_HDR_TableNum },	{ cmd_cnfg_cal_hdr_pre_action_mapper,	&gcCmd_Cnfg_Cal_HDR_Post_Action }},
																																							// {15=CalTalbeNum;sensorID;status;capacity;iCountBy;decPoint;unit;temp} This is for both query and configuration. A set action only allow in configuration mode.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_UPDATE_ONLY_IN_CNFG_MODE | CMD_TYPE_PASSWORD_PROTECTED | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE,	&gcCmd_Cnfg_CalPoint_TableNum },	{ cmd_cnfg_cal_point_pre_action_mapper, &gcCmd_Cnfg_CalPoint_Post_Action }},
																																							// {16=CalTalbe;calPoint;adcCnt;value}
   // configuration in normal run mode.
   // the following commands will recompute countbys and save its value to FNV ram memory, not the actual non volatile memory.
   // 2010-09-24 -WFC- {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_CurCountby_Sensor_Num},	{ CMD_NO_ACTION_ITEM,		&gcCmd_CurCountby_Post_Action }},	// {17=sensorID;iCountby;decpt} This command will re-compute countby and save its value to nonvolatile memory.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE,	&gcCmd_ViewingCountby_Sensor_Num},	{ cmd_viewing_countby_query_action_mapper,	&gcCmd_ViewingCountby_Post_Action }},	// {17=sensorID;iCountby;decpt;unit} 2010-09-10 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_CurUnit_Sensor_Num},		{ cmd_current_unit_pre_update, &gcCmd_CurUnit_Post_Action }},	// {18=sensorID;unit} This command will re-compute countby and save its value to nonvolatile memory.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX,	&gcCmd_CurCapacity_Sensor_Num},	{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Update_Post_Action }},
																																							// {19=sensorID;capacity; percentCapUnderLoad}

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,	&gcCmd_Listener_ListenerNum},					{ CMD_NO_ACTION_ITEM,	&gcCmd_ListenerModes_Post_Action }},	// {1A=listenerNum;destID;streamType;sensorID;outputMode;interval} control output behavior of an IO_STREAM object specified by listenerNum.
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_Stream_Registry_Num },	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }},					// {1B?stream registry number; sourceID; stream type}

   {{CMD_TYPE_NORMAL}, { TYPE_END,							CMD_NO_NEXT_ITEM },				{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},				// {1C} Reserved Future Command, may be for RF or IP stuff.
   {{CMD_TYPE_NORMAL}, { TYPE_END,							CMD_NO_NEXT_ITEM },				{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }},				// {1D} Reserved Future Command

   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_Sensor_Cnfg_SensorID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Sensor_Cnfg_Post_Action }},
																																							// {1E=sensorID; features Enabled; cnvSpeed; sensorType}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_opModes_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},		// {1F=loadcellID; mtnPeriod, mtnBandD, pendingTime}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_STD_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},			// {20=loadcellID; %capZeroBandLo, %capZeroBandHi, azmCbRang, azmPeriod, %capPwupZeroBandLo, %capPwupZeroBandHi} STD mode
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_NTEP_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},			// {21=loadcellID; %capZeroBandLo, %capZeroBandHi, azmCbRang, azmPeriod, %capPwupZeroBandLo, %capPwupZeroBandHi} NTEP mode
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_OIML_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},			// {22=loadcellID; %capZeroBandLo, %capZeroBandHi, azmCbRang, azmPeriod, %capPwupZeroBandLo, %capPwupZeroBandHi} OIML mode

   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_LC_Total_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Update_Post_Action }},			// {23=loadcellID; LCopModes; totalMode; dropThreshold%cap; riseThreshold%cap; minStableTime}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_POST_DEFAULT }, { TYPE_1D_INDEX, &gcCmd_Lc_Total_Thresh_LcID},	{ CMD_NO_ACTION_ITEM, &gcCmd_Lc_Total_Thresh_Post_Action }},
																																							// {24=loadcellID; onAcceptLowerWt; onAcceptUpperWt}
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_LC_Total_Wt_LcID},		{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }}, 				// {25?loadcellID} outputs total weight.
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_LC_Total_Stat_LcID},		{ CMD_NO_ACTION_ITEM,		CMD_NO_ACTION_ITEM }}, 				// {26?loadcellID} outputs total statistics.

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_NetGross_LcID },			{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Toggle_Net_Gross_Action }},		// {27=loadcellID} toggle net/gross.
   {{CMD_TYPE_NORMAL | CMD_TYPE_PASSWORD_PROTECTED },	{ TYPE_1D_INDEX,	&gcCmd_Zero_LcID },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Zero_Action }},		// {28=loadcellID} zero command.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Auto_Tare_LcID },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Auto_Tare_Action }},				// {29=loadcellID} tare command.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Clear_Tare_LcID },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Clear_Tare_Action }},				// {2A=loadcellID} clear tare command.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Sensor_Name_Num },		{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }},						// {2B=sensor number; name} name of a specified sensor number.

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Setpoint_Num },			{ CMD_NO_ACTION_ITEM,	&gcCmd_Setpoint_Post_Action }},				// {2C=setpoint_num;sensorID;cmplogic_valueMode;hystCB;fcmpValue}
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_Service_Cnt_LcID},	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }}, 						// {2D?loadcellID} outputs {2Dix;Over25%capCounter; overloadCounter}
   // 2010-09-24 -WFC {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Lift_Cnt_LcID},			{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }}, 						// {2E=loadcellID} outputs {2Eix;liftCounter}
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Lift_Drop_Threshold_LcID},	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }}, 					// {2E=PV_loadcellID; liftThreshold; dropThreshold; status}  2010-08-30 -WFC-
   {{CMD_TYPE_NORMAL}, { TYPE_BYTE,						&gcCmd_HW3460_Digit },			{ CMD_NO_ACTION_ITEM,	&gcCmd_HW3460_Action }},					// {2F=digitNum; imgMap} test LED display board. TESTONLY

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Sensor_DAC_Chnl },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Sensor_DAC_Post_Action }},			// {30=dacChnl; status; sensorID; sensorUnit; sensorValueType; sensorVmin; sensorVmax} status bit7 1== calibrated, bit6 1==enable, bit4 1 = manual mode, 0=normal mode; sensorID==source sensor ID; sensorVmin==source sensor value for min span point of DAC output.
   {{CMD_TYPE_NORMAL | CMD_TYPE_DYNAMIC_INDEX  }, { TYPE_BYTE,	&gcCmd_Cal_DAC_Chnl },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Cal_DAC_Post_Action }},				// {31=dacChnl; calType; opCode} where calType: calType: 0 == Aborted; 1 == save cal and exit; 2==offset, 3==gain; 4==min span point; 5== max span point;
																																							//                        opCode: 0==init; else it is adjustment value range -32768 to 32767.
   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Intern_Setting_DAC_Chnl },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Intern_Setting_DAC_Post_Action }}, // {32=dacChnl; offset; gain; CntMinSpan; CntMaxSpan} where CntMinSpan is the DAC count for Min DAC output of min value span point.
   {{CMD_TYPE_NORMAL | CMD_TYPE_DYNAMIC_INDEX  }, { TYPE_BYTE,	&gcCmd_Manual_Out_DAC_Chnl },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Manual_Out_Post_Action }},	// {33=dacChnl; DacCnt} manual force output DAC.

   {{CMD_TYPE_NORMAL}, { TYPE_1D_INDEX,					&gcCmd_Cnfg_Math_Chnl },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Cnfg_Math_Chnl_Post_Action }},		// {34=mathChnl; SensorID; overloadThrshUnit; overloadThreshold; mathExprs} mathChnl; SensorID =  Sensor ID assigned to this math channel; overload threshold unit; overload threshold; mathExpr = math expression,
   //{{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY | CMD_TYPE_DYNAMIC_INDEX },	{TYPE_READ_ONLY |TYPE_BYTE,	&gcCmd_Query_Rcal_Value_LcID}, { cmd_loadcell_rcal_query_pre_action_mapper, CMD_NO_ACTION_ITEM }},	// {35?loadcellID; RcalValue}	loadcellID range 0-1 for ScaleCore2. RcalValue in float, precision based on countby.
   {{CMD_TYPE_NORMAL },	{TYPE_1D_INDEX,	&gcAckCmd_Query_Rcal_Value_LcID}, { CMD_NO_ACTION_ITEM, CMD_NO_ACTION_ITEM }},											// {35iloadcellID; RcalValue}	loadcellID range 0-1 for ScaleCore2. RcalValue in float, precision based on countby.
   {{CMD_TYPE_NORMAL },	{TYPE_1D_INDEX,	&gcCmd_Lc_Manual_Total_LcID}, 					{  CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Manual_Total_Action }},			// {36=loadcellID} Manual total.
   {{CMD_TYPE_NORMAL },	{TYPE_1D_INDEX,	&gcCmd_Lc_Zero_Total_LcID}, 					{  CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Zero_Total_Action }},				// {37=loadcellID} Manual Zero total.
   {{CMD_TYPE_NORMAL}, { TYPE_READ_ONLY | TYPE_1D_INDEX,	&gcCmd_LC_Input_Status_LcID},	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM }}, 					// {38?loadcellID; inputStatus}	loadcellID range 0-1 for ScaleCore2. status code of loadcell bridge inputs. 0==NO error, otherwise a bad connection had occurred.
   {{CMD_TYPE_NORMAL | CMD_TYPE_DYNAMIC_INDEX }, { TYPE_BYTE,	&gcCmd_Bargraph_SensorID },		{ CMD_NO_ACTION_ITEM,	&gcCmd_Bargraph_Post_Action  }},	// {39=sensorID, valueType; unit; maxNumSeg; 1stSegValue; lastSegValue}	sensorID: input sensor ID; valueType: input value type; unit: unit of sensor value; maxNumSeg = 0 means disabled; 1stSegValue: min value to lit up the 1st Segment; lastSegValue: value to lit up the last segment.
   {{CMD_TYPE_NORMAL}, {TYPE_BYTE,	&gcAckCmd_Bargraph_Query_NumSeg_SensorID },	{ CMD_NO_ACTION_ITEM,	CMD_NO_ACTION_ITEM  }},										// {3ArsensorID,BargraphSegNum} READ ONLY. return sensorID and number of lit segment of the system bar graph based on sensor value.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY}, { TYPE_STRING, &gcCmd_Password_Str },	{ cmd_pre_password_action,	&gcCmd_Master_Default_Sys_Cnfg_Post_Action }},	// {3B=password}	password is always required for software master reset to clear all configurations. If successful, no acknowledgment else it responds with {00rcmd;errorCode}
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY}, { TYPE_STRING, &gcCmd_Password_Str },	{ cmd_pre_password_action,	&gcCmd_Memory_Check_Post_Action }},		// {3C=password}	non volatile memories checks.
   {{CMD_TYPE_NORMAL | CMD_TYPE_PASSWORD_PROTECTED },	{ TYPE_1D_INDEX,	&gcCmd_Undo_Zero_LcID },	{ CMD_NO_ACTION_ITEM,	&gcCmd_Lc_Undo_Zero_Action }},		// {3D=loadcellID} undo zero command.
   {{CMD_TYPE_NORMAL | CMD_TYPE_ACT_PRE_QUERY}, { TYPE_STRING, &gcCmd_Password_Str },		{ cmd_pre_password_action,	&gcCmd_Clear_Service_Counters_Post_Action }}	// {3E=password}	password is always required for clear service counter and status flags.
};
#endif

// TODO: added commands to manual total and zero total.

/////////////////////////////////////////////////////////////////////////////// 


/**
 * It executes a command supplied by the caller and also acknowledge it with
 * either normal respond or error status.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0xFF if it has a invalid command else other values are valid.
 *         pCmdS points at valid command contents.
 *
 * @note
 *       0xFF command ID value is not allow.
 *       every input command will always has an corresponding respond output.
 *       The output could be a queried value, status, or some other output.
 *
 * History:  Created on 2007-07-25 by Wai Fai Chin
 * 2010-11-10 -WFC- implement codes to handle CMD_TYPE_HANDLE_RETURN_QUERY command.
 * 2015-05-15 -WFC- Allow excution of pre_action for read only cmd.
 */

BYTE  cmd_act_execute( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE status;
	CMD_ACTION_ITEM_T	act;
	BYTE cmd_unlocked;
	BYTE cmdType;
	BYTE cmd;
	BYTE ch;

	cmd_unlocked = FALSE;								// assumed all commands are locked.
    cmd = pCmdS-> cmd;
	if ( CMD_STATUS_NO_ERROR == pCmdS-> status ) {		// if no pre parser error
		cmd_clear_global_scratch_memory();
		if ( cmd != 0 )								// Note that command 0 is a read only status command and will not save it to gbCmdID.
			gbCmdID = cmd;								// save it for acknowledge the input command and it's status.
		if ( cmd <= MAX_CMD_ID ) {						// ensured that there is a valid command defined in the command descriptor table.
			ch  = pCmdS-> strBuf[0];
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
			if ( 'i' == ch || 'r' == ch )
				// cmdType  = gcCommandTable[ cmd ].cmdType;	// for none Avr-Gcc compiler that handle both program and data memory address mode.
				memcpy_P ( &cmdType,  &gcAckCommandTable[ cmd ].cmdType,  sizeof(BYTE));
			else
#endif
				// cmdType  = gcCommandTable[ cmd ].cmdType;	// for none Avr-Gcc compiler that handle both program and data memory address mode.
				memcpy_P ( &cmdType,  &gcCommandTable[ cmd ].cmdType,  sizeof(BYTE));
			// if this cmd is not password protected or protection is unlocked, flaged this cmd is unlocked.
			if ( !(cmdType & CMD_TYPE_PASSWORD_PROTECTED )	|| (CMD_UNLOCKED == gtProductInfoFNV.cmdLock ) ) {
				cmd_unlocked = TRUE;
			}
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
			if ( 'i' == ch || 'r' == ch )
				// act = gcAckCommandTable[ cmd ].action;		// for none Avr-Gcc compiler that handle both program and data memory address mode.
				memcpy_P ( &act,  &gcAckCommandTable[ cmd ].action,  sizeof(CMD_ACTION_ITEM_T));    // Avr-Gcc compiler needs to explicitly get flash data from program memory space.
			else
				// act = gcCommandTable[ cmd ].action;		// for none Avr-Gcc compiler that handle both program and data memory address mode.
#endif
				memcpy_P ( &act,  &gcCommandTable[ cmd ].action,  sizeof(CMD_ACTION_ITEM_T));    // Avr-Gcc compiler needs to explicitly get flash data from program memory space.
			switch ( ch ) {
				case 'g' :
				case 'G' :
				case '=' :
					if ( cmd_unlocked ) {
						status = TRUE;												// assumed allow to perform g and = actions.
						if ( cmdType & CMD_TYPE_UPDATE_ONLY_IN_CNFG_MODE ) {
							if ( SYS_RUN_MODE_IN_CNFG != gbSysRunMode ) {
								status = FALSE;
								pCmdS-> status = CMD_STATUS_NOT_ALLOW_TO_CONFIG;
							}
						}
						
						if ( act.pMethod != 0 ) {
//							if ( !(cmdType & CMD_TYPE_ACT_PRE_QUERY) && status )	// command that needed act on pre query does not required to perform pre action here.
//								pCmdS-> status = (*act.pMethod)( pCmdS );			// pre action

							if ( status )	{
								// command that needed act on pre query does not required to perform pre action here, unless it is dynamic index.
								if ( !(cmdType & CMD_TYPE_ACT_PRE_QUERY) || (cmdType & CMD_TYPE_DYNAMIC_INDEX) ) {
									if ( (cmdType & CMD_TYPE_DYNAMIC_INDEX) )
										cmd_act_set_items( pCmdS );						// 1st, get the user specified input parameters.
									pCmdS-> status = (*act.pMethod)( pCmdS );			// pre action
								}
							}
						}
						
						if ( CMD_STATUS_NO_ERROR == pCmdS-> status ) {
							if ( ('=' == ch) && status )			// if allow to set
								cmd_act_set_items( pCmdS );
								
							if ( (CMD_STATUS_NO_ERROR == pCmdS-> status) && status) { // if no cmd error AND allow to perform set post action, then
									if ( act.pNext != 0 ) {
									// walk to next action item
									// act = *act.pNext;
									memcpy_P ( &act, act.pNext, sizeof(CMD_ACTION_ITEM_T));    
									if ( act.pMethod != 0 )
										pCmdS-> status = (*act.pMethod)( pCmdS );		// post action method
								}
							}
						}
					}
					else {
						pCmdS-> status = CMD_STATUS_CMD_LOCKED;
					}
					break;
				case 'i' :
				case 'r' :
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-

					/* For host device to parse the ScaleCore, uncomment these two statement and ignor cmd_unlocked flag.
					   The host device may need to supply its own action method depends on it application.
					   cmd_unlocked = TRUE;		// force to access the command because it is command respond from device.
					*/
					if ( cmd_unlocked ) {
						status = TRUE;												// assumed allow to perform g and = actions.
						if ( act.pMethod != 0 ) {
							if ( !(cmdType & CMD_TYPE_ACT_PRE_QUERY) && status )	// command that needed act on pre query does not required to perform pre action here.
								pCmdS-> status = (*act.pMethod)( pCmdS );			// pre action
						}

						if ( CMD_STATUS_NO_ERROR == pCmdS-> status ) {
								cmd_act_set_items( pCmdS );

							if ( (CMD_STATUS_NO_ERROR == pCmdS-> status) && status) { // if no cmd error AND allow to perform set post action, then
									if ( act.pNext != 0 ) {
									// walk to next action item
									// act = *act.pNext;
									memcpy_P ( &act, act.pNext, sizeof(CMD_ACTION_ITEM_T));
									if ( act.pMethod != 0 )
										pCmdS-> status = (*act.pMethod)( pCmdS );		// post action method
								}
							}
						}
					}
					else {
						pCmdS-> status = CMD_STATUS_CMD_LOCKED;
					}
					pCmdS-> state = CMD_STATE_ACKED;		// tell the command engine not to ack because the received command is a responsive command.
					// note that poStream was built by the msi_packet_router where poStream->destID was piStream->sourceID.
					cmd_reliable_send_manager_check_ack( pCmdS-> poStream-> destID, gbCmdID );
#endif
					break;

				case '?' :					// query command action
					if ( 0 == cmd ) {
						// get the previous cmd status of a host that has this destID.
						if ( !msi_packet_router_get_parser_status( pCmdS-> poStream-> destID, &gbCmdID, &gbCmdError ) ) {
							// we don't have the previous cmd status record of the specified destID.
							// use the current info.
							gbCmdID = cmd;
							gbCmdError = pCmdS-> status;
						}
					}
					gbCmdSysRunMode = gbSysRunMode;			// update the internal sys run mode to the host command variable so it will be shown to user.
					status = CMD_STATUS_NO_ERROR;
					if ( cmdType & CMD_TYPE_ACT_PRE_QUERY ) {
						cmd_act_set_items( pCmdS );									// 1st, get the user specified input parameters.
						// 2015-05-15 -WFC- if ( CMD_STATUS_NO_ERROR == pCmdS-> status) { // if no cmd error such as index error, out of range items etc..., then
						if ( CMD_STATUS_NO_ERROR == pCmdS-> status ||
							 CMD_STATUS_READ_ONLY == pCmdS-> status	) { // if no cmd error such as index error, out of range items etc..., then 2015-05-15 -WFC-
							if ( act.pMethod != 0 )
								status = pCmdS-> status = (*act.pMethod)( pCmdS );	// 2nd, perform prescribed pre action before call cmd_act_answer_query().
						}
						else
							status = TRUE;	// flagged it had an error.
					}
					
					// 2010-11-10 -WFC-	if ( !status )						// if no error, then perform answer query command.
					// 2010-11-10 -WFC-		cmd_act_answer_query( pCmdS );
					if ( !status ) {				// if no error, then perform answer query command.
						cmd_act_answer_query( pCmdS );
						if ( cmdType & CMD_TYPE_HANDLE_RETURN_QUERY ) {	// 2010-11-10 -WFC- if cmd required to perform post query action
							if ( act.pNext != 0 ) {
								// walk to next action item
								// act = *act.pNext;
								memcpy_P ( &act, act.pNext, sizeof(CMD_ACTION_ITEM_T));
								if ( act.pMethod != 0 )
									pCmdS-> status = (*act.pMethod)( pCmdS );		// post action method
							}
						}
					}
					break;
				/*
				case 'o' :
				case 'O' :
					if ( act.pMethod != 0 ) {
						if ( (*act.pMethod)( pCmdS ) )	// pre action
							return TRUE;					// return if there is any error.
					}
					break;
				*/
				case 'z' :
				case 'Z' :
					if ( cmd_unlocked ) {
						cmd_act_set_defaults( cmd );
						if ( cmdType & CMD_TYPE_ACT_POST_DEFAULT ) {
							if ( act.pNext != 0 ) {
								// walk to next action item
								// act = *act.pNext;
								memcpy_P ( &act, act.pNext, sizeof(CMD_ACTION_ITEM_T));    
								if ( act.pMethod != 0 )
									(*act.pMethod)( pCmdS );		// execute post action method
							}
						}
					}
					else {
						pCmdS-> status = CMD_STATUS_CMD_LOCKED;
					}
					break;
				default :
					pCmdS-> status = CMD_STATUS_NO_SUCH_SUB_CMD;
					break;
			}// end switch()
		} // end if ( cmd <= MAX_CMD_ID ) {}
		else {
			pCmdS-> status = CMD_STATUS_NO_SUCH_CMD;
		}
	}// end if ( CMD_STATUS_NO_ERROR == pCmds-> status ) {}
	
	if ( CMD_STATE_ACKED != pCmdS-> state ) {	// if it had not acknowledge the command, ack it.
		cmd_show_cmd_status( pCmdS );
	}
	
	if ( cmd != 0 ) {
		// update current cmd status of a host that has this destID.
		msi_packet_router_update_parser_status( pCmdS-> poStream-> destID, cmd, pCmdS-> status );
	}
	
	return TRUE;
} // end cmd_act_execute()

/**
 * It executes a valid query command supplied by the caller.
 * It prints out in the format as {cmdrp1;p2;p3...Pn}
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none
 *
 * @note
 *       0xFF command ID value is not allow.
 *
 * History:  Created on 2007/07/25 by Wai Fai Chin
 * 2007/08/31 -WFC- Added codes to read value from program memory space.
 *
 */
/*
void  cmd_act_answer_query( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	CMD_NEXT_ITEM_T  nItem;
	BYTE n;
	BYTE nPara;
	char str[CMD_MAX_STRING_LEN_PER_OUTPUT];

	if ( 'i' == ch || 'r' == ch )
		// nItem  = gcAckCommandTable[ pCmdS->cmd ].nextItem;		// for none Avr-Gcc compiler that handle both program and data memory address mode.
		memcpy_P ( &nItem,  &gcAckCommandTable[ pCmdS->cmd ].nextItem,  sizeof(CMD_NEXT_ITEM_T));	// Avr-Gcc compiler needs to explicitly get flash data from program memory space.
	else
		memcpy_P ( &nItem,  &gcCommandTable[ pCmdS->cmd ].nextItem,  sizeof(CMD_NEXT_ITEM_T));	// Avr-Gcc compiler needs to explicitly get flash data from program memory space.

	str[0] = CMD_START_CHAR;
	nPara = 0;
	n = 1;
    n += sprintf_P( str + n, PSTR("%02X"), pCmdS->cmd);
	if ( TYPE_1D_INDEX == ( nItem.type & 0x0F) )
		str[n] = 'i';					// i flags it as 1D index array type answer.
	else
		str[n] = 'r';
    n++;
	(*pCmdS-> pStream-> pSend_bytes)( str, n);					//send "{cmdr"
	// walk through all of items in the link list.
    while ( nItem.type != TYPE_END ) {
		if ( nPara )	// if it is not the very first parameter
			(*pCmdS-> pStream-> pSend_bytes_P)( PSTR(";"), 1);
		if ( TYPE_READ_ONLY_PGM_CHECK & nItem.type ) 							// if the value is in program memory space
			n = (BYTE) cmd_act_format_display_item_P( &nItem, str, pCmdS );	// display value from program memory space
		else	
			n = (BYTE) cmd_act_format_display_item( &nItem, str, pCmdS );		// display value from data ram space.
		(*pCmdS-> pStream-> pSend_bytes)( str, n);
		cmd_act_next_item( &nItem );					// next item
		nPara++;
	}// end while
	(*pCmdS-> pStream-> pSend_bytes_P)( PSTR("}"), 1);
} // end cmd_act_answer_query()
*/

void  cmd_act_answer_query( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	CMD_NEXT_ITEM_T  nItem;
	BYTE n;
	BYTE cmdType;
	char str[CMD_MAX_STRING_LEN_PER_OUTPUT];

	memcpy_P ( &cmdType,  &gcCommandTable[ pCmdS->cmd  ].cmdType,  sizeof(BYTE));

	// nItem  = gcCommandTable[ pCmdS->cmd ].nextItem;		// for none Avr-Gcc compiler that handle both program and data memory address mode.
	memcpy_P ( &nItem,  &gcCommandTable[ pCmdS->cmd ].nextItem,  sizeof(CMD_NEXT_ITEM_T));	// Avr-Gcc compiler needs to explicitly get flash data from program memory space.    
    n = sprintf_P( str, gcStrFmt_PacketHdr_Cmd, CMD_START_CHAR, pCmdS-> poStream-> sourceID, pCmdS-> poStream-> destID, pCmdS->cmd);
	if ( (TYPE_1D_INDEX == ( nItem.type & 0x0F)) ||
		(CMD_TYPE_DYNAMIC_INDEX & cmdType))
		str[n] = 'i';					// i flags it as 1D index array type answer.
	else
		str[n] = 'r';
    n++;

	if ( TYPE_READ_ONLY_PGM_CHECK & nItem.type ) 							// if the value is in program memory space
		n += (BYTE) cmd_act_format_display_item_P( &nItem, &str[n], pCmdS );	// display value from program memory space
	else	
		n += (BYTE) cmd_act_format_display_item( &nItem, &str[n], pCmdS );	// display value from data ram space.
	
	if ( CMD_STATUS_NO_ERROR == pCmdS-> status ) {				// if no error from the possible index type parameter.
		pCmdS-> state = CMD_STATE_ACKED;						// flag that we acknowledge this command.
		// (*pCmdS-> pStream-> pSend_bytes)( str, n);			// send "{cmdrnnn"
		io_stream_write_bytes( pCmdS-> poStream, str, n );
		// stream_router_routes_a_ostream( pCmdS-> poStream );
		cmd_act_next_item( &nItem );							// next item
		// walk through all of items in the link list.
		while ( nItem.type != TYPE_END ) {
			// (*pCmdS-> pStream-> pSend_bytes_P)( PSTR(";"), 1);
			io_stream_write_bytes_P( pCmdS-> poStream, PSTR(";"), 1 );
			if ( TYPE_READ_ONLY_PGM_CHECK & nItem.type ) 							// if the value is in program memory space
				n = (BYTE) cmd_act_format_display_item_P( &nItem, str, pCmdS );	// display value from program memory space
			else	
				n = (BYTE) cmd_act_format_display_item( &nItem, str, pCmdS );		// display value from data ram space.
			// (*pCmdS-> pStream-> pSend_bytes)( str, n);
			io_stream_write_bytes( pCmdS-> poStream, str, n );
			stream_router_routes_a_ostream( pCmdS-> poStream );
			cmd_act_next_item( &nItem );						// next item
		}// end while
		// (*pCmdS-> pStream-> pSend_bytes_P)( PSTR("}"), 1);
		io_stream_write_bytes_P( pCmdS-> poStream, PSTR("}"), 1 );
		stream_router_routes_a_ostream( pCmdS-> poStream );
	} // end if ( CMD_STATUS_NO_ERROR == pCmds-> status ) {}
	// else it defer the acknowledge to the caller to call cmd_show_cmd_status( pCmds );
} // end cmd_act_answer_query()


/**
 * It formats display based on the data type.
 *
 * @param  pItem  -- point to a next item data structure in program memory space.
 * @param   pStr  -- string buffer pointer.
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return length of the string excludes the null char.
 *
 * @note
 *       0xFF command ID value is not allow.
 *
 * History:  Created on 2007-07-26 by Wai Fai Chin
 * 2010-11-10 -WFC- handle user entered a number out of range of byte type.
 *            If user enter a number > 16bit, it will not detected it!!!
 *            I hope user understands the limitation of data type.
 *
 */

int  cmd_act_format_display_item( CMD_NEXT_ITEM_T  *pItem, char *pStr, CMD_PRE_PARSER_STATE_T *pCmdS )
{
	CMD_ITEM_TYPE_UNION uItem;
	int n;
	UINT16 addr;
	BYTE i;			// index
	BYTE bM;		// tmp byte varible
	BYTE type;

	n = 0;
	i = pCmdS -> index;
	if ( i <= pCmdS -> maxIndexRange )  {	// ensured that index will not out of bound
		type = (pItem -> type) & 0x0F;	// strip out read only, post action bits; only get data type
		// type = pItem -> type;
		// type &= 0x0F;
		switch ( type )  {
			case TYPE_BYTE :
			case TYPE_1D_INDEX :
				//bM = ((CMD_BYTE_ITEM_T*) pItem-> pNext )-> pV[i];
				memcpy_P ( &uItem.cmd_byte_item,  (CMD_BYTE_ITEM_T*) pItem-> pNext,  sizeof(CMD_BYTE_ITEM_T));
				bM = uItem.cmd_byte_item.pV[i];
				if ( TYPE_1D_INDEX == type ) {		// Note that 1D index type only allow in the beginning of command table. It treats the entire input paremters as 1D array types.
					if ( sscanf_P( &(pCmdS -> strBuf[1]), gcStrFmt_pct_d, &n )!= EOF ) {	// if it has a valid number after the '?'.
						bM = (BYTE) n;
					}
					if ( (n >= 0 ) && ( n < 256 ) ) { 	// 2010-11-10 -WFC- handle user entered a number out of range of byte type. If user enter a number > 16bit, it will not detected it!!! I hope user understand the limitation of data type.
						// pCmdS -> maxIndexRange = ((CMD_BYTE_ITEM_T*) pItem-> pNext )-> max;	// max range of index number for this command array variables.
						pCmdS -> maxIndexRange = uItem.cmd_byte_item.max;	// max range of index number for this command array variables.
						if ( bM > pCmdS -> maxIndexRange ) { // prevent user input index out of range.
							bM = 0;
							pItem -> type = TYPE_END;   // forced to end this invalid index array command.
							pCmdS-> status = CMD_STATUS_ERROR_INDEX;
						}
						pCmdS -> index = bM;
					}
					else { // user entered a number out of range of unsigned BYTE type item. Not in range 0 to 255. 2010-11-10 -WFC-
						bM = 0;
						pItem -> type = TYPE_END;   // forced to end this invalid index array command.
						pCmdS-> status = CMD_STATUS_ERROR_INDEX;
						pCmdS -> index = bM;
					}
				}
				if ( CMD_STATUS_ERROR_INDEX != pCmdS-> status )		// if no index error, then format it, otherwise the caller will format it with error code.
					n = sprintf_P( pStr, gcStrFmt_pct_u, (UINT16) bM);
				break;
			case TYPE_INT8 :
				// n = sprintf( pStr, "%d", (int) ((CMD_INT8_ITEM_T*) pItem-> pNext )-> pV[i] );
				memcpy_P ( &uItem.cmd_i8_item,  (CMD_INT8_ITEM_T*) pItem-> pNext,  sizeof(CMD_INT8_ITEM_T));
				n = sprintf_P( pStr, gcStrFmt_pct_d, (int) uItem.cmd_i8_item.pV[i] );
				break;
			case TYPE_UINT16 :
				// n = sprintf( pStr, "%u", (UINT16)((CMD_UINT16_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_ui16_item,  (CMD_UINT16_ITEM_T*) pItem-> pNext,  sizeof(CMD_UINT16_ITEM_T));
				n = sprintf_P( pStr, gcStrFmt_pct_u, uItem.cmd_ui16_item.pV[i]);
				break;
			case TYPE_INT16 :
				// n = sprintf( pStr, "%d", (INT16) ((CMD_INT16_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_i16_item,  (CMD_INT16_ITEM_T*) pItem-> pNext,  sizeof(CMD_INT16_ITEM_T));
				n = sprintf_P( pStr, gcStrFmt_pct_d, uItem.cmd_i16_item.pV[i]);
				break;
			case TYPE_UINT32 :
				// n = sprintf( pStr, "%lu", (UINT32)((CMD_UINT32_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_ui32_item,  (CMD_UINT32_ITEM_T*) pItem-> pNext,  sizeof(CMD_UINT32_ITEM_T));
				n = sprintf_P( pStr, gcStrFmt_pct_lu, uItem.cmd_ui32_item.pV[i]);
				break;
			case TYPE_INT32 :
				// n = sprintf( pStr, "%ld", (INT32) ((CMD_INT32_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_i32_item,  (CMD_INT32_ITEM_T*) pItem-> pNext,  sizeof(CMD_INT32_ITEM_T));
				n = sprintf_P( pStr, gcStrFmt_pct_ld, uItem.cmd_i32_item.pV[i]);
				break;
			case TYPE_FLOAT :
				// n = sprintf( pStr, "%f", (float) ((CMD_FLOAT_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_float_item,  (CMD_FLOAT_ITEM_T*) pItem-> pNext,  sizeof(CMD_FLOAT_ITEM_T));
				n = sprintf_P( pStr, gcStrFmt_pct_f, uItem.cmd_float_item.pV[i]);
				break;
			case TYPE_STRING :
				// n = sprintf( pStr, "%s", ((CMD_STRING_ITEM_T*) pItem-> pNext )-> pStr);
				// It supports array type for string type that has same string size because of memory limitation.
				// n = (int) copy_until_match_char( pStr,  ((CMD_STRING_ITEM_T*) pItem-> pNext )-> pStr,
			 	//					CMD_ITEM_DELIMITER, 19 );
				memcpy_P ( &uItem.cmd_string_item,  (CMD_STRING_ITEM_T*) pItem-> pNext,  sizeof(CMD_STRING_ITEM_T));
				addr = uItem.cmd_string_item.max * i;
				n = (int) copy_until_match_char_xnull( pStr, uItem.cmd_string_item.pStr + addr, CMD_ITEM_DELIMITER, CMD_MAX_STRING_LEN_PER_OUTPUT - 1 );
				break;
		}// end switch
	} // end if ( i <= pCmdS -> maxIndexRange )  {	// ensured that index will not out of bound
	return n;
} // end cmd_act_format_display_item(,)



/**
 * It formats display of a value in program memory space based on the data type.
 *
 * @param  pItem  -- point to a next item data structure in program memory space.
 * @param   pStr  -- string buffer pointer.
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return length of the string excludes the null char.
 *
 * @note
 *       0xFF command ID value is not allow.
 *	Caller must verified that the value is a program memory space before call this function.
 *
 * History:  Created on 2007/08/31 by Wai Fai Chin
 */

int  cmd_act_format_display_item_P( CMD_NEXT_ITEM_T  *pItem, char *pStr, CMD_PRE_PARSER_STATE_T *pCmdS )
{
	CMD_ITEM_TYPE_UNION uItem;
	GENERIC_UNION u;
	int n;
	BYTE i;			// index
	BYTE type;

	n = 0;
	i = pCmdS -> index;
	if ( i <= pCmdS -> maxIndexRange )  {	// ensured that index will not out of bound
		type = (pItem -> type) & 0x0F;	// strip out read only, post action bits; only get data type
		// type = pItem -> type;
		// type &= 0x0F;
		switch ( type )  {
			case TYPE_BYTE :
			// case TYPE_1D_INDEX : // index value is not allow in program memory space because it is always in ram as temporary variable.
				//bM = ((CMD_BYTE_ITEM_T*) pItem-> pNext )-> pV[i];
				memcpy_P ( &uItem.cmd_byte_item,  (CMD_BYTE_ITEM_T*) pItem-> pNext,  sizeof(CMD_BYTE_ITEM_T));
				memcpy_P ( &u.bv,  (BYTE*) &uItem.cmd_byte_item.pV[i],  sizeof(BYTE));
				n = sprintf_P( pStr, gcStrFmt_pct_u, (UINT16) u.bv);
				break;
			case TYPE_INT8 :
				// n = sprintf( pStr, "%d", (int) ((CMD_INT8_ITEM_T*) pItem-> pNext )-> pV[i] );
				memcpy_P ( &uItem.cmd_i8_item,  (CMD_INT8_ITEM_T*) pItem-> pNext,  sizeof(CMD_INT8_ITEM_T));
				memcpy_P ( &u.i8v,  (INT8*) &uItem.cmd_i8_item.pV[i],  sizeof(INT8));
				n = sprintf_P( pStr, gcStrFmt_pct_d, (int) u.i8v );
				break;
			case TYPE_UINT16 :
				// n = sprintf( pStr, "%u", (UINT16)((CMD_UINT16_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_ui16_item,  (CMD_UINT16_ITEM_T*) pItem-> pNext,  sizeof(CMD_UINT16_ITEM_T));
				memcpy_P ( &u.ui16v,  (UINT16*) &uItem.cmd_ui16_item.pV[i],  sizeof(UINT16));
				n = sprintf_P( pStr, gcStrFmt_pct_u, u.ui16v);
				break;
			case TYPE_INT16 :
				// n = sprintf( pStr, "%d", (INT16) ((CMD_INT16_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_i16_item,  (CMD_INT16_ITEM_T*) pItem-> pNext,  sizeof(CMD_INT16_ITEM_T));
				memcpy_P ( &u.i16v,  (INT16*) &uItem.cmd_i16_item.pV[i],  sizeof(INT16));
				n = sprintf_P( pStr, gcStrFmt_pct_d, u.i16v);
				break;
			case TYPE_UINT32 :
				// n = sprintf( pStr, "%lu", (UINT32)((CMD_UINT32_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_ui32_item,  (CMD_UINT32_ITEM_T*) pItem-> pNext,  sizeof(CMD_UINT32_ITEM_T));
				memcpy_P ( &u.ui32v,  (UINT32*) &uItem.cmd_ui32_item.pV[i],  sizeof(UINT32));
				n = sprintf_P( pStr, gcStrFmt_pct_lu, u.ui32v);
				break;
			case TYPE_INT32 :
				// n = sprintf( pStr, "%ld", (INT32) ((CMD_INT32_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_i32_item,  (CMD_INT32_ITEM_T*) pItem-> pNext,  sizeof(CMD_INT32_ITEM_T));
				memcpy_P ( &u.i32v,  (INT32*) &uItem.cmd_i32_item.pV[i],  sizeof(INT32));
				n = sprintf_P( pStr, gcStrFmt_pct_ld, u.i32v);
				break;
			case TYPE_FLOAT :
				// n = sprintf( pStr, "%f", (float) ((CMD_FLOAT_ITEM_T*) pItem-> pNext )-> pV[i]);
				memcpy_P ( &uItem.cmd_float_item,  (CMD_FLOAT_ITEM_T*) pItem-> pNext,  sizeof(CMD_FLOAT_ITEM_T));
				memcpy_P ( &u.aFloat,  (float*) &uItem.cmd_float_item.pV[i],  sizeof(float));
				n = sprintf_P( pStr, gcStrFmt_pct_f, u.aFloat);
				break;
			case TYPE_STRING :
				// n = sprintf( pStr, "%s", ((CMD_STRING_ITEM_T*) pItem-> pNext )-> pStr);
				// no array type for string type because of memory limitation.
				// n = (int) copy_until_match_char( pStr,  ((CMD_STRING_ITEM_T*) pItem-> pNext )-> pStr,
			 	//					CMD_ITEM_DELIMITER, CMD_MAX_STRING_LEN_PER_OUTPUT - 1 );
				memcpy_P ( &uItem.cmd_string_item,  (CMD_STRING_ITEM_T*) pItem-> pNext,  sizeof(CMD_STRING_ITEM_T));
				n = (int) copy_until_match_char_xnull_P( pStr, uItem.cmd_string_item.pStr, CMD_ITEM_DELIMITER, CMD_MAX_STRING_LEN_PER_OUTPUT - 1 );
				break;
		}// end switch
	} // end if ( i <= pCmdS -> maxIndexRange )  {	// ensured that index will not out of bound
	return n;
} // end cmd_act_format_output_item_P(,)


/**
 * It executes a valid command supplied by the caller.
 * It sets value to each item in the command descriptor from
 * input parameters of the command.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return TRUE always
 *
 * @note
 *       0xFF command ID value is not allow.
 *
 * History:  Created on 2007/07/27 by Wai Fai Chin
 */

BYTE  cmd_act_set_items( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	char ch;
	char cmdSetType;
	CMD_NEXT_ITEM_T  nItem;
	char *pStr;

	cmdSetType = pCmdS-> strBuf[0];

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
	if ( 'i' == cmdSetType || 'r' == cmdSetType )
		// nItem  = gcAckCommandTable[ pCmdS->cmd ].nextItem;
		memcpy_P ( &nItem,  &gcAckCommandTable[ pCmdS->cmd ].nextItem,  sizeof(CMD_NEXT_ITEM_T));
	else
#endif
		memcpy_P ( &nItem,  &gcCommandTable[ pCmdS->cmd ].nextItem,  sizeof(CMD_NEXT_ITEM_T));

	pStr = &(pCmdS -> strBuf[1]);
	ch = *pStr;								// fetched a possible delimiter and points next char

	//if ( nItem.type & TYPE_READ_ONLY ) {
	if ( (nItem.type & TYPE_READ_ONLY) && !( cmdSetType == 'r' || cmdSetType == 'i') ) {		//allow r and i to set the read only commands because of responsive command for a meter.
		pCmdS-> status = CMD_STATUS_READ_ONLY;	// Cannot assign value to a read only command.
		return TRUE;							// prevent it to set read only items.
	}
	else {	 
		// walk through all of items in the link list.
    	while ( nItem.type != TYPE_END ) {
			if ( 0 == ch ) break;				// end of command input string, done.
			// the following skip empty parameters like ;;;}
			if ( CMD_ITEM_DELIMITER == ch ) {	// if the next char is a delimiter, walk to next item.
				cmd_act_next_item( &nItem );	// walk to next item.
		    	++pStr;							// pointing at next char
			}
			else {
				if ( CMD_END_CHAR == ch )
					++pStr;
				else {
					cmd_act_set_item( &nItem, pStr, pCmdS );
	    	    	pStr = find_a_char_in_string( pStr, CMD_ITEM_DELIMITER );
				}
			}
			ch = *pStr;						// fetched a possible delimiter and pointing at next char
		}// end while
	}// end else{}
	return TRUE;
} // end cmd_act_set_items()


/**
 * It converts Ascii string into a value of a data type in the item.
 *
 * @param  pItem  -- point to an item data structure.
 * @param   pStr  -- input string buffer pointer.
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return true if successed.
 *
 *
 * History:  Created on 2007-07-27 by Wai Fai Chin
 * 2010-11-10 -WFC- handle user entered a number out of range of byte type.
 *            If user enter a number > 16bit, it will not detected it!!!
 *            I hope user understand the limitation of data type.
 * 2011-11-03 -WFC- handle unsigned byte data within 0 to 255, signed byte within -128 to 127. This resolved problem report #931.
 */

BYTE  cmd_act_set_item( CMD_NEXT_ITEM_T  *pItem, char *pStr, CMD_PRE_PARSER_STATE_T *pCmdS ) 
{
	CMD_ITEM_TYPE_UNION uItem;
	GENERIC_UNION newV; // bV, min, max;
	UINT16 addr;
	BYTE i;			// index
	BYTE type;

	i = pCmdS -> index;
	type = (pItem -> type) & 0x0F;		// strip out read only, post action bits; only get data type
		switch ( type )  {
			case TYPE_BYTE :
			case TYPE_INT8 :
			case TYPE_1D_INDEX :
				if ( sscanf_P( pStr, gcStrFmt_pct_d, &newV.i16.l )!= EOF ) {			// if it has a valid value.
					// 2011-11-03 -WFC- v handle unsigned byte data within 0 to 255, signed byte within -128 to 127. This resolved problem report #931.
					addr = FALSE;			// save RAM memory space by use addr as boolean variable. assume input is out of range.
					if ( TYPE_BYTE == type || TYPE_1D_INDEX == type ) {
						if ( (newV.i16.l >= 0 ) && ( newV.i16.l < 256 ) )
							addr = TRUE;
					}
					else {
						if ( (newV.i16.l >= -128 ) && ( newV.i16.l < 128 ) )
							addr = TRUE;
					}
					// 2011-11-03 -WFC- if ( (newV.i16.l >= -128 ) && ( newV.i16.l < 256 ) ) { 	// 2010-11-10 -WFC- handle user entered a number out of range of byte type. If user enter a number > 16bit, it will not detected it!!! I hope user understand the limitation of data type.
					if ( addr ) {	// if input is within range, then
					// 2011-11-03 -WFC- ^
						memcpy_P ( &uItem.cmd_byte_item,  (CMD_BYTE_ITEM_T*) pItem-> pNext,  sizeof(CMD_BYTE_ITEM_T));
						//min.b.b0 = ((CMD_BYTE_ITEM_T*) pItem-> pNext )-> min;
						//max.b.b0 = ((CMD_BYTE_ITEM_T*) pItem-> pNext )-> max;
						// bV.ba = uItem.cmd_byte_item.min; bug!!!
						// bV.bb = uItem.cmd_byte_item.max; bug!!! It cannot uses UNION type to share, even though it has memory space to do so. It should use bV.b.b0 and bV.b.b1 instead.
						if ( TYPE_BYTE == type || TYPE_1D_INDEX == type ) {
							// validate it before accept the value.
							// if ( newV.b.b0 >= min.b.b0 && newV.b.b0 <= max.b.b0 ) {
							if ( newV.bv >= uItem.cmd_byte_item.min && newV.bv <= uItem.cmd_byte_item.max ) {
								if ( TYPE_1D_INDEX == type ) {
									pCmdS -> index = newV.bv;
									pCmdS -> maxIndexRange = uItem.cmd_byte_item.max;		// may not need this.
								}
								else
									// ((CMD_BYTE_ITEM_T*) pItem-> pNext )-> pV[i] = newV.b.b0;
									uItem.cmd_byte_item.pV[i] = newV.bv;
							}
							else { // validation failed
								pItem -> type = TYPE_END;   // forced to end this invalid index array command.
								if ( TYPE_1D_INDEX == type ) {
									// pItem -> type = TYPE_END;   // forced to end this invalid index array command.
									pCmdS-> status = CMD_STATUS_ERROR_INDEX;
								}
								else
									pCmdS-> status = CMD_STATUS_OUT_RANGE_INPUT;
							}
						}
						else {
							// validate it before accept the value.
							// if ( newV.i8.b0 >= min.i8.b0 &&	 newV.i8.b0 <= max.i8.b0)
							if ( newV.i8v >= uItem.cmd_i8_item.min && newV.i8v <= uItem.cmd_i8_item.max)
								// ((CMD_INT8_ITEM_T*) pItem-> pNext )-> pV[i] = newV.i8.b0;
								uItem.cmd_i8_item.pV[i] = newV.i8v;
							else {
								pItem -> type = TYPE_END;   // forced to end this invalid input cmd.
								pCmdS-> status = CMD_STATUS_OUT_RANGE_INPUT;
							}
						}
					}
					else { // user entered a number out of range of BYTE type item. -128 to 255. 2010-11-10 -WFC-
						pItem -> type = TYPE_END;   // forced to end this invalid input cmd.
						pCmdS-> status = CMD_STATUS_OUT_RANGE_INPUT;
					}
				}
				break;
			case TYPE_UINT16 :
				if ( sscanf_P( pStr, gcStrFmt_pct_ud, &newV.ui16.l )!= EOF ) {			// if it has a valid value.
					//if ( newV.ui16.l >= ((CMD_UINT16_ITEM_T*) pItem-> pNext )-> min &&
					//	 newV.ui16.l <= ((CMD_UINT16_ITEM_T*) pItem-> pNext )-> max)
					//	((CMD_UINT16_ITEM_T*) pItem-> pNext )-> pV[i] = newV.ui16.l;
					memcpy_P ( &uItem.cmd_ui16_item,  (CMD_UINT16_ITEM_T*) pItem-> pNext,  sizeof(CMD_UINT16_ITEM_T));
					if ( newV.ui16.l >= uItem.cmd_ui16_item.min &&
						 newV.ui16.l <= uItem.cmd_ui16_item.max )
						uItem.cmd_ui16_item.pV[i] = newV.ui16.l;
				}
				break;
			case TYPE_INT16 :
				if ( sscanf_P( pStr, gcStrFmt_pct_d, &newV.i16.l )!= EOF ) {			// if it has a valid value.
					//if ( newV.i16.l >= ((CMD_INT16_ITEM_T*) pItem-> pNext )-> min &&
					//	 newV.i16.l <= ((CMD_INT16_ITEM_T*) pItem-> pNext )-> max)
					//	((CMD_INT16_ITEM_T*) pItem-> pNext )-> pV[i] = newV.i16.l;
					memcpy_P ( &uItem.cmd_i16_item,  (CMD_INT16_ITEM_T*) pItem-> pNext,  sizeof(CMD_INT16_ITEM_T));
					if ( newV.i16.l >= uItem.cmd_i16_item.min &&
						 newV.i16.l <= uItem.cmd_i16_item.max )
						uItem.cmd_i16_item.pV[i] = newV.i16.l;
				}
				break;
			case TYPE_UINT32 :
				if ( sscanf_P( pStr, gcStrFmt_pct_lu, &newV.ui32v )!= EOF ) {			// if it has a valid value.
					//if ( newV.ui32 >= ((CMD_UINT32_ITEM_T*) pItem-> pNext )-> min &&
					//	 newV.ui32 <= ((CMD_UINT32_ITEM_T*) pItem-> pNext )-> max)
					//	((CMD_UINT32_ITEM_T*) pItem-> pNext )-> pV[i] = newV.ui32;
					memcpy_P ( &uItem.cmd_ui32_item,  (CMD_UINT32_ITEM_T*) pItem-> pNext,  sizeof(CMD_UINT32_ITEM_T));
					if ( newV.ui32v >= uItem.cmd_ui32_item.min &&
						 newV.ui32v <= uItem.cmd_ui32_item.max )
						uItem.cmd_ui32_item.pV[i] = newV.ui32v;
				}
				break;
			case TYPE_INT32 :
				if ( sscanf_P( pStr, gcStrFmt_pct_ld, &newV.i32v )!= EOF ) {			// if it has a valid value.
					//if ( newV.i32 >= ((CMD_INT32_ITEM_T*) pItem-> pNext )-> min &&
					//	 newV.i32 <= ((CMD_INT32_ITEM_T*) pItem-> pNext )-> max)
					//	((CMD_INT32_ITEM_T*) pItem-> pNext )-> pV[i] = newV.i32;
					memcpy_P ( &uItem.cmd_i32_item,  (CMD_INT32_ITEM_T*) pItem-> pNext,  sizeof(CMD_INT32_ITEM_T));
					if ( newV.i32v >= uItem.cmd_i32_item.min &&
						 newV.i32v <= uItem.cmd_i32_item.max )
						uItem.cmd_i32_item.pV[i] = newV.i32v;
				}
				break;
			case TYPE_FLOAT :
				if ( sscanf_P( pStr, gcStrFmt_pct_f, &newV.aFloat )!= EOF ) {			// if it has a valid value.
					//if ( newV.aFloat >= ((CMD_FLOAT_ITEM_T*) pItem-> pNext )-> min &&
					//	 newV.aFloat <= ((CMD_FLOAT_ITEM_T*) pItem-> pNext )-> max)
					//	((CMD_FLOAT_ITEM_T*) pItem-> pNext )-> pV[i] = newV.aFloat;
					memcpy_P ( &uItem.cmd_float_item,  (CMD_FLOAT_ITEM_T*) pItem-> pNext,  sizeof(CMD_FLOAT_ITEM_T));
					if ( newV.aFloat >= uItem.cmd_float_item.min &&
						 newV.aFloat <= uItem.cmd_float_item.max )
						uItem.cmd_float_item.pV[i] = newV.aFloat; 
				}
				break;
			case TYPE_STRING :
				// It supports 1D array of string type with the same string size because of memory limitation.
				//copy_until_match_char( ((CMD_STRING_ITEM_T*) pItem-> pNext )-> pStr, pStr,
			 	//					CMD_ITEM_DELIMITER, ((CMD_STRING_ITEM_T*) pItem-> pNext )-> max );
				memcpy_P ( &uItem.cmd_string_item,  (CMD_STRING_ITEM_T*) pItem-> pNext,  sizeof(CMD_STRING_ITEM_T));
				addr = uItem.cmd_string_item.max * i;
				copy_until_match_char( uItem.cmd_string_item.pStr + addr, pStr, CMD_ITEM_DELIMITER, uItem.cmd_string_item.max);
				break;
		}// end switch
		return TRUE;
} // end cmd_act_set_item(,)


/**
 * It walks to next item.
 *
 * @param  pItem  -- point to an item data structure.
 *
 * @return none.
 * @post   *pItem has new content of next item.
 *
 * History:  Created on 2007/07/27 by Wai Fai Chin
 */

void  cmd_act_next_item( CMD_NEXT_ITEM_T  *pItem )
{
	CMD_ITEM_TYPE_UNION uItem;

		switch ( (pItem -> type) &0x0F )  {
			case TYPE_BYTE :
			case TYPE_INT8 :
			case TYPE_1D_INDEX :
				// *pItem = ((CMD_INT8_ITEM_T*) pItem-> pNext )-> nextItem;
				memcpy_P ( &uItem.cmd_i8_item, (CMD_INT8_ITEM_T*) pItem-> pNext, sizeof(CMD_INT8_ITEM_T));
				*pItem = uItem.cmd_i8_item.nextItem;
				break;
			case TYPE_UINT16 :
			case TYPE_INT16  :
				// *pItem = ((CMD_INT16_ITEM_T*) pItem-> pNext )-> nextItem;
				memcpy_P ( &uItem.cmd_i16_item,  (CMD_INT16_ITEM_T*) pItem-> pNext,  sizeof(CMD_INT16_ITEM_T));
				*pItem = uItem.cmd_i16_item.nextItem;
				break;
			case TYPE_UINT32 :
			case TYPE_INT32  :
			case TYPE_FLOAT  :
				// *pItem = ((CMD_UINT32_ITEM_T*) pItem-> pNext )-> nextItem;
				memcpy_P ( &uItem.cmd_ui32_item,  (CMD_UINT32_ITEM_T*) pItem-> pNext,  sizeof(CMD_UINT32_ITEM_T));
				*pItem = uItem.cmd_ui32_item.nextItem;
				break;
			case TYPE_STRING  :
				// *pItem = ((CMD_STRING_ITEM_T*) pItem-> pNext )-> nextItem;
				// *pItem = ((CMD_STRING_ITEM_T*) pItem-> pNext )-> nextItem;
				memcpy_P ( &uItem.cmd_string_item,  (CMD_STRING_ITEM_T*) pItem-> pNext,  sizeof(CMD_STRING_ITEM_T));
				*pItem = uItem.cmd_string_item.nextItem;
				break;
		}// end switch
} // end cmd_act_next_item()


/**
 * It resets items that associated with the command to its default contents.
 * It prints out in the format as {cmdrp1;p2;p3...Pn}
 *
 * @param  cmd  --  command ID.
 *
 * @return none
 *
 * @note
 *       0xFF command ID value is not allow.
 *
 * History:  Created on 2006/11/14 by Wai Fai Chin
 */

void  cmd_act_set_defaults( BYTE cmd )
{
	CMD_NEXT_ITEM_T		nItem;
	CMD_ITEM_TYPE_UNION	uItem;
	BYTE i;				// index
	BYTE bIndexLimit;	// index limit;
	BYTE type;

	// nItem  = gcCommandTable[ cmd ].nextItem;
	memcpy_P ( &nItem,  &gcCommandTable[ cmd ].nextItem,  sizeof(CMD_NEXT_ITEM_T));    
	if ( nItem.type & TYPE_READ_ONLY )
		return;		// prevent it to set read only items.

	bIndexLimit = 0;		// assumed this command is not a 1D array type parameters.
	// walk through all of items in the link list.
    while ( TYPE_END != nItem.type ) {
		type = nItem.type & 0x0F;		// strip out read only, post action bits; only get data type
		switch ( type )  {  // based on next item type.
			case TYPE_BYTE :
			case TYPE_INT8 :
			case TYPE_1D_INDEX :
				if ( TYPE_1D_INDEX == type ) {
					// bIndexLimit = ((CMD_BYTE_ITEM_T*) nItem.pNext )-> max;	// max range of index number for this command array variables.
					memcpy_P ( &uItem.cmd_byte_item,  (CMD_BYTE_ITEM_T*) nItem.pNext,  sizeof(CMD_BYTE_ITEM_T));
					bIndexLimit = uItem.cmd_byte_item.max;
				}
				else {
					memcpy_P ( &uItem.cmd_byte_item,  (CMD_BYTE_ITEM_T*) nItem.pNext,  sizeof(CMD_BYTE_ITEM_T));
					for ( i=0; i <= bIndexLimit; i++ )
						// ((CMD_BYTE_ITEM_T*) nItem.pNext )-> pV[i] = ((CMD_BYTE_ITEM_T*) nItem.pNext )-> defaultV;
						uItem.cmd_byte_item.pV[i] = uItem.cmd_byte_item.defaultV;
				}
				break;
			case TYPE_UINT16 :
			case TYPE_INT16  :
				memcpy_P ( &uItem.cmd_ui16_item,  (CMD_UINT16_ITEM_T*) nItem.pNext,  sizeof(CMD_UINT16_ITEM_T));
				for ( i=0; i <= bIndexLimit; i++ )
					// ((CMD_UINT16_ITEM_T*) nItem.pNext )-> pV[i] = ((CMD_UINT16_ITEM_T*) nItem.pNext )-> defaultV;
					uItem.cmd_ui16_item.pV[i] = uItem.cmd_ui16_item.defaultV;
				break;
			case TYPE_UINT32 :
			case TYPE_INT32  :
			case TYPE_FLOAT  :
				memcpy_P ( &uItem.cmd_ui32_item,  (CMD_UINT32_ITEM_T*) nItem.pNext,  sizeof(CMD_UINT32_ITEM_T));
				for ( i=0; i <= bIndexLimit; i++ )
					// ((CMD_UINT32_ITEM_T*) nItem.pNext )-> pV[i] = ((CMD_UINT32_ITEM_T*) nItem.pNext )-> defaultV;
					uItem.cmd_ui32_item.pV[i] = uItem.cmd_ui32_item.defaultV;
				break;
			case TYPE_STRING :
				// 1D array of string type data ( multi strings ) is not supported because of memory limitation.
				// copy_until_match_char(  ((CMD_STRING_ITEM_T*) nItem.pNext )-> pStr,
			 	//						((CMD_STRING_ITEM_T*) nItem.pNext )-> pDefStr,
			 	//					 0, ((CMD_STRING_ITEM_T*) nItem.pNext )-> max );
				memcpy_P ( &uItem.cmd_string_item,  (CMD_STRING_ITEM_T*)nItem.pNext,  sizeof(CMD_STRING_ITEM_T));
				copy_until_match_char_P(  uItem.cmd_string_item.pStr,
			 							uItem.cmd_string_item.pDefStr,
			 						 0, uItem.cmd_string_item.max );
				break;
		}// end switch

		cmd_act_next_item( &nItem );					// next item
	}// end while() walk through all in a link list.
} // end cmd_act_set_defaults()



/**
 * See if it is OK to start a new calibration. 
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if not OK to start a new calibration.
 *         0 error if it is ok to start a new calibration.
 *
 *
 * History:  Created on 2007/08/30 by Wai Fai Chin
 * 2011-04-12 -WFC- Setup loadcell runmode, disabled peak hold before calls adc_lt_construct_op_desc().
 * 2011-08-05 -WFC- Ensured it is a slow CPU system clock speed in cal mode for DynaLink2 because it might be in Peak Hold high speed mode.
 * 2015-05-08 -WFC- Not allow to cal if it is in legal for trade mode and no cal switch tripped.
 */

BYTE cmd_is_ok_start_new_cal( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	INT16				i;
	BYTE				n;
	BYTE				errorStatus;
	SENSOR_CAL_T		*pSensorCal;
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to sensor local descriptor
	
	errorStatus = CAL_ERROR_WRONG_SENSOR_ID;					// assumed wrong sensor ID
	// 2015-05-08 -WFC- v
	if ( (SCALE_STD_MODE_OIML == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK)) ||
		 (SCALE_STD_MODE_NTEP == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK))) {
		if ( !(gbSysStatus & SYS_STATUS_CAL_SWITCH_TRIPPED) ) {
			gabCalErrorStatus[ 0 ]= CAL_ERROR_NOT_ALLOW;
			return CMD_STATUS_CAL_ERROR_NOT_ALLOW;
		}
	}
	// 2015-05-08 -WFC- ^

	//n = pCmdS -> index;										// n is the sensor ID or channel number.
	if ( sscanf_P( &(pCmdS-> strBuf[1]), gcStrFmt_pct_d, &i )!= EOF ) {	// if it has a valid value.
		n = (BYTE) i;
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {		// if n is a valid channel number.
			errorStatus = cal_allows_new_cal( pSensorCal );
			gabCalOpStep[ n ] = pSensorCal-> status;			// set cal status to cal op status for user command interface.
			if ( !errorStatus )	{							// if it allows to start a new calibration.
				if ( '=' == pCmdS-> strBuf[0] ) {
					cmd_act_set_defaults( pCmdS-> cmd );
					cmd_act_set_items( pCmdS );					// grab all input values into its variables.
					pSnDesc = &gaLSensorDescriptor[ n ];
					pSnDesc-> conversion_cnfg |= SENSOR_CNFG_ENABLED; // force to enable sensor because user wants to calibrate this sensor.
					if ( gabCalTmpZone[n] ) {					// if specified a none zero temperature zone, then
																// force to enable temperature compensation because user specified temperature zone.
						pSnDesc-> conversion_cnfg |= SENSOR_FEATURE_TEMPERATURE_CMP;
					}
					pSnDesc-> cnfg |= SENSOR_CNFG_ENABLED;				// force to enable sensor because user wants to calibrate this sensor.
					if ( sensor_descriptor_init( n ) ) {				// if it has valid contents,
						// NOTE that pSnDesc-> type has been init based on gabSensorTypeFNV[] by sensor_descriptor_init();
						// TODO: if sensor type is loadcell,then else adc_cpu_construct_op_desc(). 
						// set specific sensor type run mode.
						switch ( pSnDesc-> type )	{
							case SENSOR_TYPE_LOADCELL :
									#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
									//	if ( ((LOADCELL_T *) (pSnDesc-> pDev))-> runModes &  LC_RUN_MODE_PEAK_HOLD_ENABLED )
											bios_clock_slow();		// 2011-08-05 -WFC-
									#endif
									((LOADCELL_T *) (pSnDesc-> pDev))-> runModes |= LC_RUN_MODE_IN_CAL;			// flag this loadcell is in calibration mode
									((LOADCELL_T *) (pSnDesc-> pDev))-> runModes &= ~( LC_RUN_MODE_NORMAL_ACTIVE | LC_RUN_MODE_PEAK_HOLD_ENABLED);	// flag it is NOT in normal active mode that it may not has a cal table and it is in cal mode. Disabled Peak hold mode.
									break;
						}
						adc_lt_construct_op_desc( pSnDesc, n );		// then contruct ADC operation descriptor.
					}
					pSensorCal-> countby.unit	= gabCalCB_unit[ n ];
					pSensorCal-> capacity 		= gafCal_capacity[ n ];
						
//					// set specific sensor type run mode.
//					switch ( pSnDesc-> type )	{
//						case SENSOR_TYPE_LOADCELL :
//								((LOADCELL_T *) (pSnDesc-> pDev))-> runModes |= LC_RUN_MODE_IN_CAL;			// flag this loadcell is in calibration mode
//								((LOADCELL_T *) (pSnDesc-> pDev))-> runModes &= ~LC_RUN_MODE_NORMAL_ACTIVE;	// flag it is NOT in normal active mode that it may not has a cal table and it is in cal mode.
//					}
				}
			}	// end if ( !errorStatus )	{}
			gabCalErrorStatus[ n ] = errorStatus;				// set cal status to cal op status for user command interface.
		}// end if ( sensor_get_cal_base(,)) {}
	}

	return errorStatus;
} // end cmd_is_ok_start_new_cal()

/**
 * Not allow to cal if it is in legal for trade mode and no cal switch tripped.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if not OK to start a new calibration.
 *         0 error if it is ok to start a new calibration.
 *
 *
 * History:  Created on 2015-05-11 by Wai Fai Chin
 *
 */

BYTE cmd_query_cal_status_pre_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	INT16	i;
	BYTE 	n;
	BYTE	errorStatus;

	errorStatus = CMD_STATUS_NO_ERROR;							// assumed passed.
	if ( (SCALE_STD_MODE_OIML == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK)) ||
		 (SCALE_STD_MODE_NTEP == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK))) {
		if ( !(gbSysStatus & SYS_STATUS_CAL_SWITCH_TRIPPED) ) {
			n = 0;
			if ( CMD_END_CHAR != pCmdS-> strBuf[1] ) {
				if ( sscanf_P( &(pCmdS-> strBuf[1]), gcStrFmt_pct_d, &i )!= EOF ) {	// if it has a valid value.
					n = (BYTE) i;
					if ( n >= MAX_NUM_CAL_SENSORS )
						n = 3;
				}
			}
			gabCalErrorStatus[ n ]= CAL_ERROR_NOT_ALLOW;
		}
	}

	return errorStatus;
} // end cmd_query_cal_status_pre_action()



/**
 * It maps complex cal data structure to host command variables so
 * the command engine will answer the query result back to host.
 * This method maps header portion of the cal data structure.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 * 
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2009/05/05 by Wai Fai Chin
 */

BYTE cmd_cal_cb_query_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// User should query this cmd, it responded with max possible number of cb,
	// wantCbIndex, iCountby and decPt are the standard countby based on a given capacity.
	// On the setting action, max_possible_num, iCountby and decPt will be ignor.
	// global scratch memory for this module.
	
	// gbCMD_Index	== sensorID;
	// gbAcmdTmp1 	== wantCBindex;
	// gbAcmdTmp2 	== max_possible_num_cb;  // output item, read only.
	// gu16AcmdTmp1	== countby;
	// gi8AcmdTmp1	== decimalPoint;

	BYTE			errorStatus;
	BYTE			bStdCBIndex;
	MSI_CB			wantedCB;
	SENSOR_CAL_T	*pSensorCal;

	errorStatus = CMD_STATUS_NO_ERROR;							// assumed passed.
	if ( sensor_get_cal_base( gbCMD_Index, &pSensorCal ) ) {				// if n is a valid channel number.
		// gbAcmdTmp2 = MAX number of possible countby in this table.
		gbAcmdTmp2 = cal_gen_countby_table( pSensorCal-> capacity, &bStdCBIndex, &gbAcmdTmp1, &wantedCB );
		gu16AcmdTmp1 = wantedCB.iValue;
		gi8AcmdTmp1  = wantedCB.decPt;
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_cal_cb_query_action_mapper()

/**
 * See if the cal countby is OK. 
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if not OK to start a new calibration.
 *         0 error if it is ok to start a new calibration.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2008/06/26 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 */

BYTE cmd_is_cal_countby_ok_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// User should query this cmd, it responded with max possible number of cb,
	// wantCbIndex, iCountby and decPt are the standard countby based on a given capacity.
	// On the setting action, max_possible_num, iCountby and decPt will be ignor.
	// global scratch memory for this module.
	
	// gbCMD_Index	== sensorID;
	// gbAcmdTmp1 	== wantCBindex;
	// gbAcmdTmp2 	== max_possible_num_cb; // output item, read only.
	// gu16AcmdTmp1	== countby;
	// gi8AcmdTmp1	== decimalPoint;

	BYTE			errorStatus;
	BYTE			bStdCBIndex;
	UINT32			u32Tmp;
	MSI_CB			wantedCB;
	SENSOR_CAL_T	*pSensorCal;
	
	if ( sensor_get_cal_base( gbCMD_Index, &pSensorCal ) ) {	// if n is a valid channel number.
		if ( gbAcmdTmp1 < CAL_MAX_NUM_POSSIBLE_CB ) {			// if wantCBindex within boundary
			if (  pSensorCal-> status >= CAL_STATUS_GOT_UNIT_CAP &&  pSensorCal-> status < CAL_STATUS_COMPLETED ) {
				u32Tmp = (UINT32) pSensorCal-> capacity;
				if ( u32Tmp > 1 )	{			// ensured capacity > 1
					gbAcmdTmp2 = cal_gen_countby_table( pSensorCal-> capacity, &bStdCBIndex, &gbAcmdTmp1, &wantedCB );
					//pSensorCal->countby = wantedCB;
					pSensorCal->countby.fValue = wantedCB.fValue;
					pSensorCal->countby.iValue = wantedCB.iValue;
					pSensorCal->countby.decPt  = wantedCB.decPt;
					// -WFC- 2010-08-30 gabSensorShowCBunitsFNV[ gbCMD_Index ]	= pSensorCal->countby.unit;
					gabSensorShowCBunitsFNV[ gbCMD_Index ]	= gabSensorViewUnitsFNV[ gbCMD_Index ] = pSensorCal->countby.unit;	// -WFC- 2010-08-30
					gabSensorShowCBdecPtFNV[ gbCMD_Index ]	= pSensorCal->countby.decPt;
					gafSensorShowCBFNV[ gbCMD_Index ]		= pSensorCal->countby.fValue;
					gabCalOpStep[ gbCMD_Index ] =
					pSensorCal-> status = CAL_STATUS_GOT_COUNTBY;
					errorStatus = CMD_STATUS_NO_ERROR;
				} // end if ( u32Tmp > 1 )	{}
				else
					errorStatus = CMD_STATUS_ERROR_CANNOT_UPDATE;	// cannot update because capacity <= 1;
			} // end if (  pSensorCal-> status >= CAL_STATUS_GOT_UNIT_CAP &&  pSensorCal->status < CAL_STATUS_COMPLETED ) {}
			else {
				errorStatus = CAL_ERROR_CANNOT_CHANGE_COUNTBY;
			}
		}
		else {
			errorStatus = CMD_STATUS_ERROR_2ND_INDEX;
		}
	}// end if ( sensor_get_cal_base(,)) {}
	else {
		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
	}

	return errorStatus;
} // end cmd_is_cal_countby_ok_mapper()


/**
 * It validate viewing countby number from host command before it maps to gaSensorShowXXXXFNV[].
 * This method helps prevent user entered bogus countby info without destroy gaSensorShowXXXXFNV[].
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return error codes. 0 if no error.
 *
 * @post   updated valid countby in gafSensorShowCBFNV[], gawSensorShowCBFNV[] and gabSensorShowCBdecPtFNV[].
 *	
 * @note
 * 		Meaning of countby decimal point, positive value moves to left place while negative moves to the right.
 *
 * History:  Created on 2010-09-10 by Wai Fai Chin
 * 2010-11-09 -WFC- handle 1 UNIT and OIML mode on unit change.
 * 2010-12-17 -WFC- Fixed not update unit bug.
 *
 */

BYTE cmd_is_viewing_countby_ok_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== sensorID;
	// gbAcmdTmp1 	== countby unit;
	// gu16AcmdTmp1	== countby;
	// gi8AcmdTmp1	== decimalPoint;

	BYTE	errorStatus;
	BYTE	i;

	i = gbCMD_Index;

	errorStatus = CMD_STATUS_NO_ERROR;				// assumed passed.
	if ( i < MAX_NUM_SENSORS ) {
		if ( 1 == gu16AcmdTmp1 || 10 == gu16AcmdTmp1 || 1000 == gu16AcmdTmp1 || 10000 == gu16AcmdTmp1 ||
			 2 == gu16AcmdTmp1 || 20 == gu16AcmdTmp1 || 2000 == gu16AcmdTmp1 || 20000 == gu16AcmdTmp1 ||
			 5 == gu16AcmdTmp1 || 50 == gu16AcmdTmp1 || 5000 == gu16AcmdTmp1 || 50000 == gu16AcmdTmp1 )  {

			if ( i < MAX_NUM_PV_LOADCELL ) {
				// OIML and 1 UNIT mode are not allow to change unit.
				if ( ((gbScaleStandardModeNV & SCALE_STD_MODE_MASK) < SCALE_STD_MODE_OIML) ||
					  ( SYS_RUN_MODE_IN_CNFG == gbCmdSysRunMode) ) {				// config mode trump other restriction.
					gawSensorShowCBFNV[i] = gu16AcmdTmp1;
					gabSensorShowCBdecPtFNV[i] = gi8AcmdTmp1;
					gabSensorShowCBunitsFNV[i] = gbAcmdTmp1;		// 2010-12-17 -WFC-
					gafSensorShowCBFNV[i] = cal_scale_float_countby( gu16AcmdTmp1, gi8AcmdTmp1);
					cmd_update_or_init_sensor_params( pCmdS-> strBuf[0], i );	// update or init sensor related variables. It calls Xsensor_update_param() calls Xsensor_recompute_weights_unit(), in turn, it calls Xsensor_zero_init().
				}
				else {
					errorStatus = CMD_STATUS_NOT_ALLOW_TO_CONFIG;
					// 2010-12-17 -WFC- gabSensorViewUnitsFNV[i] = gbCMDTmp[i];		// restored old unit as before this command.
				}
			}
			else { // other sensor is not bound on SCALE_STD_MODE.
				gawSensorShowCBFNV[i] = gu16AcmdTmp1;
				gabSensorShowCBdecPtFNV[i] = gi8AcmdTmp1;
				gabSensorShowCBunitsFNV[i] = gbAcmdTmp1;		// 2010-12-17 -WFC-
				gafSensorShowCBFNV[i] = cal_scale_float_countby( gu16AcmdTmp1, gi8AcmdTmp1);
				cmd_update_or_init_sensor_params( pCmdS-> strBuf[0], i );	// update or init sensor related variables. It calls Xsensor_update_param() calls Xsensor_recompute_weights_unit(), in turn, it calls Xsensor_zero_init().
			}
		}
		else {
			errorStatus = CMD_STATUS_ERROR_INVALID_COUNTBY;
		}
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}
	return errorStatus;
} // end cmd_is_viewing_countby_ok_mapper()

/* 2010-11-09 -WFC- removed
BYTE cmd_is_viewing_countby_ok_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== sensorID;
	// gbAcmdTmp1 	== countby unit;
	// gu16AcmdTmp1	== countby;
	// gi8AcmdTmp1	== decimalPoint;

	BYTE	errorStatus;
	BYTE	i;

	i = gbCMD_Index;

	errorStatus = CMD_STATUS_NO_ERROR;				// assumed passed.
	if ( i < MAX_NUM_SENSORS ) {
		if ( 1 == gu16AcmdTmp1 || 10 == gu16AcmdTmp1 || 1000 == gu16AcmdTmp1 || 10000 == gu16AcmdTmp1 ||
			 2 == gu16AcmdTmp1 || 20 == gu16AcmdTmp1 || 2000 == gu16AcmdTmp1 || 20000 == gu16AcmdTmp1 ||
			 5 == gu16AcmdTmp1 || 50 == gu16AcmdTmp1 || 5000 == gu16AcmdTmp1 || 50000 == gu16AcmdTmp1 )  {
		gawSensorShowCBFNV[i] = gu16AcmdTmp1;
		gabSensorShowCBdecPtFNV[i] = gi8AcmdTmp1;
		gafSensorShowCBFNV[i] = cal_scale_float_countby( gu16AcmdTmp1, gi8AcmdTmp1);
		cmd_update_or_init_sensor_params( pCmdS-> strBuf[0], i );	// update or init sensor related variables. It calls Xsensor_update_param() calls Xsensor_recompute_weights_unit(), in turn, it calls Xsensor_zero_init().
		}
		else {
			errorStatus = CMD_STATUS_ERROR_INVALID_COUNTBY;
		}
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}
	return errorStatus;
} // end cmd_is_viewing_countby_ok_mapper()
*/



/**
 * It maps complex view countby variables to host command variables so
 * the command engine will answer the query result back to host.
 * This method helps prevent user entered bogus countby info without destroy gaSensorShowXXXXFNV[].
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2010-09-10 by Wai Fai Chin
 */

BYTE cmd_viewing_countby_query_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== sensorID;
	// gbAcmdTmp1 	== countby unit;
	// gu16AcmdTmp1	== countby;
	// gi8AcmdTmp1	== decimalPoint;

	BYTE			errorStatus;

	errorStatus = CMD_STATUS_NO_ERROR;				// assumed passed.
	if ( gbCMD_Index < MAX_NUM_SENSORS ) {			// if n is a valid sensor id.
		gu16AcmdTmp1 = gawSensorShowCBFNV[ gbCMD_Index ];
		gi8AcmdTmp1	 = gabSensorShowCBdecPtFNV[ gbCMD_Index ];
		gbAcmdTmp1	 = gabSensorShowCBunitsFNV[ gbCMD_Index ];
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_viewing_countby_query_action_mapper()


/**
 * It normalize new current countby based on newly changed unit.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error always.
 * @post	gbCMDTmp[] holds the old unit. 
 *
 * History:  Created on 2007/01/25 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 */

BYTE cmd_current_unit_pre_update( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE i;
	// save sensor display units to gbTmpStr0[] for post action processing. 
	for ( i=0; i < MAX_NUM_SENSORS; i++)
		// -WFC- 2010-08-30	gbCMDTmp[i] = gabSensorShowCBunitsFNV[i];
		gbCMDTmp[i] = gabSensorViewUnitsFNV[i];		// -WFC- 2010-08-30
	return CMD_STATUS_NO_ERROR;

} // end cmd_current_unit_pre_update()


/**
 * It normalize new current countby based on newly changed unit.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 * 				gbCMDTmp[] has old units.
 *
 *  gabSensorShowCBunitsFNV[i] has the new unit mode.
 *
 * @return 0 error always. 
 *
 *
 * History:  Created on 2007-01-25 by Wai Fai Chin
 * 2010-11-09 -WFC- handle 1 UNIT and OIML mode on unit change.
 */

BYTE cmd_current_unit_post_update( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	i;
	BYTE	errorStatus;
	
	i = pCmdS -> index;					// i is the sensor ID or channel number.
	errorStatus = CMD_STATUS_NO_ERROR;
	// TODO: tell remote sensor to update unit, recompute math channel unit. The new cmd protocol may automaticaly take care of this.
	if ( i < MAX_NUM_SENSORS ) {
		/* removed because the loadcell_update_param() will handle it.
		memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ gbCMDTmp[i] ][gabSensorShowCBunitsFNV[i] ],  sizeof(float));
		gafSensorShowCapacityFNV[i]	=	gafSensorShowCapacityFNV[i] * unitCnv;
		
		cb.fValue = gafSensorShowCBFNV[i] * unitCnv;
		cal_normalize_verify_input_countby( &cb);
		gafSensorShowCBFNV[i] = cb.fValue;
		gawSensorShowCBFNV[i] = cb.iValue;
		gabSensorShowCBdecPtFNV[i] = cb.decPt;
		*/
		// 2010-11-09 -WFC- V
		if ( i < MAX_NUM_PV_LOADCELL ) {
			// OIML and 1 UNIT mode are not allow to change unit.
			if ( ((gbScaleStandardModeNV & SCALE_STD_MODE_MASK) < SCALE_STD_MODE_OIML) ||
				  ( SYS_RUN_MODE_IN_CNFG == gbCmdSysRunMode) ) {				// config mode trump other restriction.
				cmd_update_or_init_sensor_params( pCmdS-> strBuf[0], i );		// update or init loadcell related variables. It calls loadcell_update_param() calls loadcell_recompute_weights_unit(), in turn, it calls ro_init().
			}
			else {
				errorStatus = CMD_STATUS_NOT_ALLOW_TO_CONFIG;
				gabSensorViewUnitsFNV[i] = gbCMDTmp[i];		// restored old unit as before this command.
			}
		}
		else // other sensor is not bound on SCALE_STD_MODE.
			cmd_update_or_init_sensor_params( pCmdS-> strBuf[0], i );		// update or init loadcell related variables. It calls loadcell_update_param() calls loadcell_recompute_weights_unit(), in turn, it calls lc_zero_init().
		// 2010-11-09 -WFC- ^
 	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}
	
	return errorStatus;
} // end cmd_current_unit_post_update()


/**
 * It handles user entering on accept threshold window values.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error always. 
 * @note The threshold window values will always saved in calibration unit.
 *
 * History:  Created on 2008/01/23 by Wai Fai Chin
 * 2010-08-30 -WFC- replaced gabVSMathUnitFNV[] with gabSensorShowCBunitsFNV[].
 * 2010-11-10 -WFC- converted capacity according to metric system convention.
 */

BYTE cmd_lc_total_thresh_post_update( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		lc;
	LOADCELL_T	*pLc;
	float		unitCnv;
	BYTE		errorStatus;
	BYTE		calUnit;
	
	lc = pCmdS -> index;					// i is the sensor ID or channel number.
	errorStatus = CMD_STATUS_NO_ERROR;
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[lc] ||
				SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[lc])	{
			pLc = &gaLoadcell[ lc ];
			if ( SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[lc] )
				// 2010-08-30	-WFC-	calUnit = gabVSMathUnitFNV[0];
				calUnit = gabSensorShowCBunitsFNV[0];		// 2010-08-30	-WFC-
			else
				calUnit = pLc-> pCal-> countby.unit;

			if ( pLc-> viewCB.unit != calUnit ) {		// if current unit is not the same as the calibration unit,
				// convert threshold weight from current unit to cal unit value.
				// 2010-11-10 -WFC- memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pLc-> viewCB.unit ][ calUnit ], sizeof(float));
				memcpy_P ( &unitCnv,  &gafLoadcellCapacityUnitsTbl[ pLc-> viewCB.unit ][ calUnit ], sizeof(float)); // 2010-11-10 -WFC- converted capacity according to metric system convention.
				gafTotalOnAcceptUpperWtNV[lc] *= unitCnv;
				gafTotalOnAcceptLowerWtNV[lc] *= unitCnv;
			}
			//need to save it to EEPROM here???
		}
 	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}
	
	return errorStatus;
} // end cmd_lc_total_thresh_post_update()


/**
 * See if zero cal point is OK. 
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if it failed to zero cal point.
 *         0 error if it is ok.
 *
 *
 * History:  Created on 2006/12/27 by Wai Fai Chin
 */

BYTE cmd_is_zero_cal_point_ok( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE			n;
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;
	
	n = pCmdS -> index;									// n is the sensor ID or channel number.
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {					// if n is a valid channel number.
		// handle different type of sensor here:
		// the following logic is to handle loadcell type.
		errorStatus = cal_zero_point( n, pSensorCal );
		gabCalOpStep[ n ] = pSensorCal-> status;			// set cal status to cal op status for user command interface.
		gabCalErrorStatus[ n ] = errorStatus;				// set cal status to cal op status for user command interface.
	}// end if ( sensor_get_cal_base(,)) {}
	else {
		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
	}

	return errorStatus;

} // end cmd_is_zero_cal_point_ok()

/**
 * See if this cal point is OK after received new cal value. 
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if the new cal point value failed.
 *         0 error if it is ok.
 *
 *
 * History:  Created on 2006/12/27 by Wai Fai Chin
 * 2010/04/02	Added logics to use Rcal value to calibrate if Rcal is enabled.
 * 2012-09-18 -WFC- Modified to handle C-Cal for non DSC and HD product.
 */

// 2011-05-19 -WFC- #if ( CONFIG_PCB_AS == CONFIG_PCB_AS_SCALECORE2)
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-05-19 -WFC-
BYTE cmd_is_new_cal_point_ok( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE			n;
	BYTE			errorStatus;
	INT32			adcCnt;
	SENSOR_CAL_T	*pSensorCal;
	
	errorStatus = CAL_ERROR_NONE;								// assumed passed.
	n = pCmdS -> index;											// n is the sensor ID or channel number.
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {				// if n is a valid channel number.
		if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] )	{
			if ( gabLoadcellRcalEnabled[n] ) 					// if Rcal Enabled, it means this calibration is use Rcal value to calibrate the loadcell by enter the Rcal number instead of weight value.
				adcCnt = gaLSensorDescriptor[ SENSOR_NUM_RCAL ].curADCcount;	// sensor 3 is the Rcal resistor for ScaleCore2.
			else
				adcCnt = gaLSensorDescriptor[n].curADCcount;
			errorStatus = cal_build_table( adcCnt, gafCalValue[n], pSensorCal);
		} // end if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {}
		gabCalErrorStatus[ n ]	= errorStatus;						// set cal status to cal op status for user command interface.
		gabCalOpStep[ n ] 		= pSensorCal-> status;				// set cal status to cal op status for user command interface.
	} // end if ( sensor_get_cal_base( channel, &pSensorCal ) )  {}
	else {
		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
	}

	return errorStatus;
} // end cmd_is_new_cal_point_ok()
// 2012-09-18 -WFC- v handle C-Cal calibration.
#else
BYTE cmd_is_new_cal_point_ok( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE			n;
	BYTE			errorStatus;
	INT32			adcCnt;
	SENSOR_CAL_T	*pSensorCal;
	float fV;

	errorStatus = CAL_ERROR_NONE;									// assumed passed.
	n = pCmdS -> index;												// n is the sensor ID or channel number.
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {					// if n is a valid channel number.
		if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] )	{
			if ( gabLoadcellRcalEnabled[n] )			{		// if Rcal Enabled, it means this calibration is use Rcal value to calibrate the loadcell by enter the Rcal number instead of weight value.
				adcCnt = pSensorCal-> adcCnt[0] + (INT32) (gafCalValue[n]);		// assumed gafCalValue[] is a C-Cal
				fV = pSensorCal->capacity * 0.1f;					// C-Cal is ADC count at 10% of capacity weight.
			}
			else {
				adcCnt = gaLSensorDescriptor[n].curADCcount;
				fV = gafCalValue[n];
			}
			errorStatus = cal_build_table( adcCnt, fV, pSensorCal);
		} // end if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {}
		gabCalErrorStatus[ n ]	= errorStatus;						// set cal status to cal op status for user command interface.
		gabCalOpStep[ n ] 		= pSensorCal-> status;				// set cal status to cal op status for user command interface.
	} // end if ( sensor_get_cal_base( channel, &pSensorCal ) )  {}
	else {
		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
	}
	return errorStatus;
} // end cmd_is_new_cal_point_ok()
#endif
//#else
//BYTE cmd_is_new_cal_point_ok( CMD_PRE_PARSER_STATE_T *pCmdS )
//{
//	BYTE			n;
//	BYTE			errorStatus;
//	SENSOR_CAL_T	*pSensorCal;
//
//	errorStatus = CAL_ERROR_NONE;									// assumed passed.
//	n = pCmdS -> index;												// n is the sensor ID or channel number.
//	if ( sensor_get_cal_base( n, &pSensorCal ) ) {					// if n is a valid channel number.
//		if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] )	{
//			errorStatus = cal_build_table( gaLSensorDescriptor[n].curADCcount, gafCalValue[n], pSensorCal);
//		} // end if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {}
//		gabCalErrorStatus[ n ]	= errorStatus;						// set cal status to cal op status for user command interface.
//		gabCalOpStep[ n ] 		= pSensorCal-> status;				// set cal status to cal op status for user command interface.
//	} // end if ( sensor_get_cal_base( channel, &pSensorCal ) )  {}
//	else {
//		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
//	}
//	return errorStatus;
//} // end cmd_is_new_cal_point_ok()
//#endif
// 2012-09-18 -WFC- ^

/**
 * It maps complex cal data structure to host command variables so
 * the command engine will answer the query result back to host.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 * 
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2009/05/05 by Wai Fai Chin
 */

BYTE cmd_sensor_cal_point_query_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== sensorID;
	// gbAcmdTmp1	== calpoint;
	// gi32AcmdTmp1	== ADC count;
	// gfAcmdTmp1	== value;

	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;

	errorStatus = CMD_STATUS_NO_ERROR;								// assumed passed.
	if ( sensor_get_cal_base( gbCMD_Index, &pSensorCal ) ) {		// if gbCMD_Index is a valid sensorID.
		if ( gbAcmdTmp1 < MAX_CAL_POINTS ) 	{					// if cal point is within range.
			gfAcmdTmp1	= pSensorCal-> value[  gbAcmdTmp1 ];
			gi32AcmdTmp1= pSensorCal-> adcCnt[ gbAcmdTmp1 ];
		}
		else
			errorStatus = CMD_STATUS_ERROR_2ND_INDEX;
	} 
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}
	
	return errorStatus;
} // end cmd_sensor_cal_point_query_pre_action_mapper()

/**
 * It maps complex cal data structure to host command variables so
 * the command engine will answer the query result back to host.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 * 
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2009/09/18 by Wai Fai Chin
 */

BYTE cmd_sensor_value_query_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== sensorID;
	// gi32AcmdTmp1	== ADC count;
	// gStrAcmdTmp	== value in ASCII string;

	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;
	MSI_CB                cb;

	errorStatus = CMD_STATUS_NO_ERROR;								// assumed passed.
	if ( gbCMD_Index < MAX_NUM_LOADCELL )	{
		if ( sensor_get_cal_base( gbCMD_Index, &pSensorCal ) ) {		// if gbCMD_Index is a valid sensorID.
			gi32AcmdTmp1= gaLSensorDescriptor[ gbCMD_Index ].curADCcount;
			cb = pSensorCal-> countby;
			cal_next_lower_countby( &cb );
			float_round_to_string( gaLSensorDescriptor[ gbCMD_Index ].value, &cb, 8, gStrAcmdTmp );
		}
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_sensor_value_query_pre_action_mapper()

/**
 * It process required action before Rcal takes effect.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 * 
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 * History:  Created on 2009/09/18 by Wai Fai Chin
 */

// 2011-05-19 -WFC- #if ( CONFIG_PCB_AS == CONFIG_PCB_AS_SCALECORE2)
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-05-19 -WFC-
BYTE cmd_rcal_set_post_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== loadcellID;

	BYTE			n;
	BYTE	errorStatus;

	n = pCmdS -> index;												// n is the sensor ID or channel number.
	errorStatus = CMD_STATUS_NO_ERROR;								// assumed passed.
	if ( n < MAX_NUM_LOADCELL )	{
		if ( gabLoadcellRcalEnabled[n] ) {
			gabSensorFeaturesFNV[ SENSOR_NUM_RCAL ] |= SENSOR_FEATURE_SENSOR_ENABLE;				// enabled Rcal input sensor
		}
		else
			gabSensorFeaturesFNV[ SENSOR_NUM_RCAL ] &= ~SENSOR_FEATURE_SENSOR_ENABLE;				// disabled Rcal input sensor
		if ( sensor_descriptor_init( SENSOR_NUM_RCAL ) ) {											// if it has valid contents,
			adc_lt_construct_op_desc( &gaLSensorDescriptor[ SENSOR_NUM_RCAL ], SENSOR_NUM_RCAL );	// then construct ADC operation descriptor.
		}
	} 
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}
	
	return errorStatus;
} // end cmd_rcal_set_post_action()

#else		// for ScaleCore1:
BYTE cmd_rcal_set_post_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== loadcellID;

	BYTE			n;
	BYTE	errorStatus;

	n = pCmdS -> index;												// n is the sensor ID or channel number.
	errorStatus = CMD_STATUS_NO_ERROR;								// assumed passed.
	if ( n < MAX_NUM_LOADCELL )	{
		// TODO: reset software filter.....
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_rcal_set_post_action()
#endif

/**
 * It maps complex cal data structure to host command variables so
 * the command engine will answer the query result back to host.
 * This method maps header portion of the cal data structure.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 * 
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2009/05/05 by Wai Fai Chin
 * 2010/05/21 Added sensorID item based on requested by Tim Alberts.
 */

BYTE cmd_cnfg_cal_hdr_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== calTableNum;
	// gbAcmdTmp3	== sensorID.
	// gbAcmdTmp1	== status;
	// gfAcmdTmp1	== capacity;
	// gu16AcmdTmp1	== countby;
	// gi8AcmdTmp1	== decimalPoint;
	// gbAcmdTmp2	== unit;
	// gfAcmdTmp2	== temperature;

	BYTE			errorStatus;

	errorStatus = CMD_STATUS_NO_ERROR;								// assumed passed.
	if ( gbCMD_Index < CAL_MAX_NUM_CAL_TABLE ) {					// if the cal table index is valid, map cal table header to host command variables.
		gbAcmdTmp1	= gaSensorCalNV[ gbCMD_Index ].status;
		gfAcmdTmp1	= gaSensorCalNV[ gbCMD_Index ].capacity;
		gu16AcmdTmp1= gaSensorCalNV[ gbCMD_Index ].countby.iValue;
		gi8AcmdTmp1	= gaSensorCalNV[ gbCMD_Index ].countby.decPt;
		gbAcmdTmp2	= gaSensorCalNV[ gbCMD_Index ].countby.unit;
		gfAcmdTmp2	= gaSensorCalNV[ gbCMD_Index ].temperature;
		gbAcmdTmp3	= cal_get_sensor_id_from_table_num( gbCMD_Index );
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_cnfg_cal_hdr_pre_action_mapper()

/**
 * It maps host command inputs to the complex cal data structure.
 * This method maps header portion of the cal data structure.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 * 
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2009/05/05 by Wai Fai Chin
 */

BYTE cmd_cnfg_cal_hdr_post_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== calTableNum;
	// gbAcmdTmp1	== status;
	// gfAcmdTmp1	== capacity;
	// gu16AcmdTmp1	== countby;
	// gi8AcmdTmp1	== decimalPoint;
	// gbAcmdTmp2	== unit;
	// gfAcmdTmp2	== temperature;

	BYTE			errorStatus;

	errorStatus = CMD_STATUS_NO_ERROR;						// assumed passed.
	if ( gbCMD_Index < CAL_MAX_NUM_CAL_TABLE ) {			// if the cal table index is valid, map the command inputs to cal table header.
		gaSensorCalNV[ gbCMD_Index ].status 		= gbAcmdTmp1;
		gaSensorCalNV[ gbCMD_Index ].capacity		= gfAcmdTmp1;
		gaSensorCalNV[ gbCMD_Index ].countby.fValue= cal_scale_float_countby( gu16AcmdTmp1, gi8AcmdTmp1 );
		gaSensorCalNV[ gbCMD_Index ].countby.iValue= gu16AcmdTmp1;
		gaSensorCalNV[ gbCMD_Index ].countby.decPt	= gi8AcmdTmp1;
		gaSensorCalNV[ gbCMD_Index ].countby.unit	= gbAcmdTmp2;
		gaSensorCalNV[ gbCMD_Index ].temperature	= gfAcmdTmp2;
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_cnfg_cal_hdr_post_action_mapper()


/**
 * It maps complex cal data structure to host command variables so
 * the command engine will answer the query result back to host.
 * This method maps calpoint from the cal data structure to host command variables.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 * 
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2009/05/05 by Wai Fai Chin
 */

BYTE cmd_cnfg_cal_point_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== calTableNum;
	// gbAcmdTmp1	== calpoint;
	// gi32AcmdTmp1 == ADC_cnt;
	// gfAcmdTmp1	== value;

	BYTE	errorStatus;

	errorStatus = CMD_STATUS_NO_ERROR;						// assumed passed.
	if ( gbCMD_Index < CAL_MAX_NUM_CAL_TABLE ) {							// if the cal table index is valid, map cal points to host command variables.
		if ( gbAcmdTmp1 < MAX_CAL_POINTS ) 	{								// if cal point is within range.
			gfAcmdTmp1	= gaSensorCalNV[ gbCMD_Index ].value[  gbAcmdTmp1 ];
			gi32AcmdTmp1= gaSensorCalNV[ gbCMD_Index ].adcCnt[ gbAcmdTmp1 ];
		}
		else
			errorStatus = CMD_STATUS_ERROR_2ND_INDEX;
	} 
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_cnfg_cal_point_pre_action_mapper()

/**
 * It maps host command inputs to the complex cal data structure.
 * This method maps calpoint to the cal data structure from host command variables.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 * 
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2009/05/05 by Wai Fai Chin
 */

BYTE cmd_cnfg_cal_point_post_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== calTableNum;
	// gbAcmdTmp1	== calpoint;
	// gi32AcmdTmp1 == ADC_cnt;
	// gfAcmdTmp1	== value;

	BYTE	errorStatus;

	errorStatus = CMD_STATUS_NO_ERROR;										// assumed passed.
	if ( gbCMD_Index < CAL_MAX_NUM_CAL_TABLE ) {							// if the cal table index is valid, map cal points to host command variables.
		if ( gbAcmdTmp1 < MAX_CAL_POINTS ) 	{								// if cal point is within range.
			gaSensorCalNV[ gbCMD_Index ].value[  gbAcmdTmp1 ] = gfAcmdTmp1;
			gaSensorCalNV[ gbCMD_Index ].adcCnt[ gbAcmdTmp1 ] = gi32AcmdTmp1;
		}
		else
			errorStatus = CMD_STATUS_ERROR_2ND_INDEX;
	} 
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_cnfg_cal_point_post_action_mapper()

/**
 * It saved calibrated data to nonvolatile memory and exit calibration mode.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if calibration failed.
 *         0 error if it is ok.
 *
 *
 * History:  Created on 2006/12/29 by Wai Fai Chin
 */

BYTE cmd_save_cal_exit( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE			n;
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;

	n = pCmdS -> index;								// n is the sensor ID or channel number.
	errorStatus = CAL_ERROR_INVALID_CAL_INFO;		// assumed NO VALID CAL INFO
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {	// if n is a valid channel number,then it has a valid pointer to cal data structure.
		// Ensured that we have successful calibration
		if ( (pSensorCal-> status > 0) && (pSensorCal-> status <= MAX_CAL_POINTS) ) {
			errorStatus = cal_save_exit( n, pSensorCal );	
			gabCalErrorStatus[ n ]	= errorStatus;				// set cal status to cal op status for user command interface.
			gabCalOpStep[ n ] 		= pSensorCal-> status;		// set cal status to cal op status for user command interface.
			if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {
				// loadcell_update_param( n );					cal_save_exit() already done it.
				gaLoadcell[n].runModes &= ~LC_RUN_MODE_IN_CAL;
			}
		}
	}
	
	return errorStatus;
} // end cmd_save_cal_exit()


/**
 * It saved calibrated data to nonvolatile memory and exit calibration mode.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error always.
 *
 * History:  Created on 2006/12/29 by Wai Fai Chin
 */

BYTE cmd_no_save_cal_exit( CMD_PRE_PARSER_STATE_T *pCmdS )
{

	BYTE			n;
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;

	n = pCmdS -> index;								// n is the sensor ID or channel number.
	errorStatus = CAL_ERROR_INVALID_CAL_INFO;			// assumed NO VALID CAL INFO
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {		// if n is a valid channel number,then it has a valid pointer to cal data structure.
		if ( pSensorCal-> status < CAL_STATUS_COMPLETED ) {	// if it is in calibration mode,
			// gabCalOpStep[ n ] 		= pSensorCal-> status;	// set cal status to cal op status for user command interface.
			errorStatus = cal_recall_a_sensor_cnfg( n );
			gabCalOpStep[ n ] 		= pSensorCal-> status;		// set cal status to cal op status for user command interface. Place here instead of before is not to confuse end user.
			gabCalErrorStatus[ n ]	= errorStatus;				// set cal status to cal op status for user command interface.
			if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {
				gaLoadcell[n].runModes &= ~LC_RUN_MODE_IN_CAL;
			}
		}
	}
	
	return errorStatus;
} // end cmd_no_save_cal_exit()


/**
 * It shows commad status. command code for status command is 00.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *                   assumed updated gbCalOpStatus.
 *
 * @return none
 *
 * History:  Created on 2008/08/06 by Wai Fai Chin
 */

void cmd_show_cmd_status( CMD_PRE_PARSER_STATE_T *pCmdS )
{

	BYTE oldCmd;
	BYTE oldIndex;
	
    gbCmdError = pCmdS-> status;			// assigned cmd status for show up in status cmd 00.
	oldCmd = pCmdS-> cmd;
	oldIndex = pCmdS-> index;
	pCmdS-> status = CMD_STATUS_NO_ERROR;	// cmd_act_answer_query() will output only if no error.
	pCmdS-> cmd = 0x00;					// set cmd = query command status.
	pCmdS-> index = 0;						// cmd 00 is a none index type, thus, it needs to set to 0.
	cmd_act_answer_query( pCmdS );
    pCmdS-> status = gbCmdError;
	pCmdS-> cmd = oldCmd;					// restore
	pCmdS-> index = oldIndex;

} // end cmd_show_cmd_status()


/**
 * update new system running mode depends on the present system running mode.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if failed.
 *         0 error if passed.
 *
 * @note self test mode can only change from normal system running mode.
 *
 * History:  Created on 2009-04-28 by Wai Fai Chin
 * 2011-08-10 -WFC- Added code to handle test external display board mode for Challenger3 and DynaLink2.
 * 2011-08-22 -WFC- Added code to treat one shot and auto secondary test the same and handle it.
 * 2011-08-23 -WFC- Treat one shot and normal self test mode the same as it was power up.
 *                  Treat auto secondary self test as pressed TEST key.
 * 2011-08-23 -WFC- Added code to handle new feature, pending power off in 1 second.
 * 2011-11-14 -WFC- In case we are in one shot self test mode, if next run mode is NOT one shot self test mode, then
 *   				tell panel_main_one_shot_selftest_thread() to end itself without rely on user key interaction.
 *   				Handle undefined run mode to a new run mode.  This resolved problem report#922.
 * 2016-03-21 -WFC-	On before exit config mode, called setpoint_init_all() and lc_zero_init_all_lc_zero_config();
 * 2016-04-29 -WFC- call adc_lt_init();
 *
 */

extern TIMER_T		gSelfTestScratchTimer;

BYTE cmd_system_run_mode( CMD_PRE_PARSER_STATE_T *pCmdS )
{

	BYTE	errorStatus;
	BYTE	noNeedInitSys;
	
	errorStatus = CMD_STATUS_NO_ERROR;
	
	switch ( gbSysRunMode ) {
		case SYS_RUN_MODE_NORMAL :
				if ( gbCmdSysRunMode < SYS_RUN_MODE_EXIT_CNFG_SAVE_FRAM ) {
					// 2011-08-22 -WFC- v
					#if (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
					if ( SYS_RUN_MODE_SELF_TEST == gbCmdSysRunMode || SYS_RUN_MODE_ONE_SHOT_SELF_TEST == gbCmdSysRunMode ){
						gbCmdSysRunMode = SYS_RUN_MODE_SELF_TEST;	// override one shot mode because it is in auto secondary test mode.
						self_test_init_and_start_threads();
					}
					else if ( SYS_RUN_MODE_AUTO_SECONDARY_TEST == gbCmdSysRunMode ){
						gbPanelMainSysRunMode = gbPanelMainRunMode =
						gbSysRunMode = PANEL_RUN_MODE_ONE_SHOT_SELF_TEST;	// set it in auto secondary self test run mode.
						PT_INIT( &gPanelMainSelfTest_thread_pt );			// init auto secondary self test thread.
					}
					// 2011-08-10 -WFC- v
					else if ( SYS_RUN_MODE_TEST_EXTERNAL_LED_BOARD == gbCmdSysRunMode ) {
						led_display_hardware_init();						// end test and init led display board.
						led_display_init();
						gbPanelMainSysRunMode = gbPanelMainRunMode = gbSysRunMode = gbCmdSysRunMode;
					}
					// 2011-08-10 -WFC- ^
					#else
						if ( SYS_RUN_MODE_SELF_TEST == gbCmdSysRunMode ||
							SYS_RUN_MODE_ONE_SHOT_SELF_TEST == gbCmdSysRunMode ){
							self_test_init_and_start_threads();
						}
					#endif
					// 2011-08-22 -WFC- ^
					gbSysRunMode = gbCmdSysRunMode;
				}
			break;
		case SYS_RUN_MODE_IN_CNFG :
				if ( SYS_RUN_MODE_SELF_TEST == gbCmdSysRunMode ||
					SYS_RUN_MODE_ONE_SHOT_SELF_TEST == gbCmdSysRunMode	)
					errorStatus = CMD_STATUS_NOT_ALLOW_TO_SELFTEST;
				else {
					gbSysRunMode = SYS_RUN_MODE_NORMAL;
					//2010-11-29 -WFC- noNeedInitSys = TRUE;
					if ( SYS_RUN_MODE_EXIT_CNFG_SAVE_FRAM == gbCmdSysRunMode )	{
						noNeedInitSys = errorStatus = nvmem_save_all_essential_config_fram();
					}
					else if ( SYS_RUN_MODE_EXIT_CNFG_SAVE_ALL == gbCmdSysRunMode ) {
						noNeedInitSys = errorStatus = nvmem_save_all_config();
					}
					else if ( SYS_RUN_MODE_EXIT_CNFG_NO_SAVE == gbCmdSysRunMode ||
							  SYS_RUN_MODE_NORMAL == gbCmdSysRunMode ) {					// change from cnfg mode to normal mode.
						noNeedInitSys = errorStatus = nvmem_recall_all_config();
					}

					// 2016-04-29 -WFC- adc_cpu_1st_init();			//2010-11-29 -WFC- In case user changed user defined model code. User defined model code dictates how the system behave.
					//2010-11-29 -WFC- if ( !noNeedInitSys ) {
					sensor_init_all();
					//2010-11-29 -WFC- }

					adc_lt_init();							// 2016-04-29 -WFC-
					adc_cpu_1st_init();						// 2016-04-29 -WFC-

					setpoint_init_all();					// 2016-03-21 -WFC-
					lc_zero_init_all_lc_zero_config();		// 2016-03-21 -WFC-
				}
			break;
		case SYS_RUN_MODE_SELF_TEST :
				if ( gbCmdSysRunMode < SYS_RUN_MODE_EXIT_CNFG_SAVE_FRAM )
					gbSysRunMode = gbCmdSysRunMode;							//
				//TODO: may be need to do some house keeping here before switch to other run mode.
				if ( gbCmdSysRunMode != SYS_RUN_MODE_SELF_TEST )
					self_test_stop_all_threads();
			break;
		case SYS_RUN_MODE_ONE_SHOT_SELF_TEST :
				// 2011-11-14 -WFC- v we are in one shot self test mode,
				if ( gbCmdSysRunMode != SYS_RUN_MODE_ONE_SHOT_SELF_TEST )	//if next run mode is NOT one shot self test mode, then
					gwSysStatus = SYS_STATUS_SELF_TEST_STEP_END_TEST;		// tell panel_main_one_shot_selftest_thread() to end itself without rely on user key interaction.
				// 2011-11-14 -WFC- ^ the above two statement resolved problem report#922.
		case SYS_RUN_MODE_AUTO_SECONDARY_TEST :
		case SYS_RUN_MODE_UI_SECONDARY_TEST :
				if ( gbCmdSysRunMode < SYS_RUN_MODE_EXIT_CNFG_SAVE_FRAM )
					gbSysRunMode = gbCmdSysRunMode;							//
				if ( gbCmdSysRunMode != SYS_RUN_MODE_ONE_SHOT_SELF_TEST )
					self_test_stop_all_threads();
			break;
		case SYS_RUN_MODE_TEST_EXTERNAL_LED_BOARD :							// 2010-09-13 -WFC-
				if ( SYS_RUN_MODE_NORMAL == gbCmdSysRunMode ) {
					// for product that has attached LED dipslay board.
					// 2011-08-10 -WFC- v
					// gbSysRunMode = gbCmdSysRunMode;
					// 2011-09-12 -WFC- gbPanelMainSysRunMode = gbPanelMainRunMode = gbSysRunMode = gbCmdSysRunMode;
					#if (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_HD  || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-	// 2011-10-14 -WFC-
					gbPanelMainSysRunMode = gbPanelMainRunMode = gbSysRunMode = gbCmdSysRunMode;		// 2011-09-12 -WFC-
					led_display_hardware_init();							// end test and init led display board.
					led_display_init();
					#else
					gbSysRunMode = gbCmdSysRunMode;							// 2011-09-12 -WFC-
					#endif
					// 2011-08-10 -WFC- ^
				}
			break;
			// 2011-11-14 -WFC- v Handle undefined run mode to a new run mode. This related to problem report#922.
		default:
				if ( gbCmdSysRunMode <= SYS_RUN_MODE_TEST_EXTERNAL_LED_BOARD )
					gbSysRunMode = gbCmdSysRunMode;
			break;
			// 2011-11-14 -WFC- ^
	} // end switch()

	// 2011-08-23 -WFC- v
	if ( SYS_RUN_MODE_PEND_POWER_OFF == gbCmdSysRunMode ) {
		gbBiosRunStatus |= BIOS_RUN_STATUS_PEND_POWER_OFF;
		timer_mSec_set( &gSelfTestScratchTimer, TT_1SEC);
	}
	// 2011-08-23 -WFC- ^

	return errorStatus;
} // end cmd_system_run_mode( CMD_PRE_PARSER_STATE_T *pCmdS )


/*
 * After configured the ADC settings, init the ADC and its average variables.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if saving failed.
 *         0 error if saving pass.
 *
 *
 * History:  Created on 2007/02/19 by Wai Fai Chin
 * /
 
#if ( (CONFIG_EMULATE_ADC == FALSE )&& (CONFIG_DEBUG_ADC == TRUE ))

BYTE cmd_adc_config_init( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	gbAdcLTcmd0 &= ADC_LT_DIF_CHANNEL_MASK;
 	gbAdcLTcmd0 |= (ADC_LT_CMD_ENABLED |gbCMDTmp[0]);	// channel
 	gbAdcLTcmd1 = (gbCMDTmp[1]<<4);					// conversion speed.		
	
	return 0;
}// end cmd_adc_config_init()

#endif
*/


/**
 * After configured Sensor features, re configure sensor descriptors and ADC module operation descriptors.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if saving failed.
 *         0 error if saving pass.
 *
 *
 * History:  Created on 2007/02/19 by Wai Fai Chin
 */

BYTE cmd_sensor_features_init( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE n;
	
	n = pCmdS -> index;
	if ( sensor_descriptor_init( n ) ) {							// if it has valid contents, 
		adc_lt_construct_op_desc( &gaLSensorDescriptor[n], n );	// then construct ADC operation descriptor.
		adc_cpu_construct_op_desc(&gaLSensorDescriptor[n], n );
		// Do we need to trash all previous ADC channels reading and start from channel0 from scratch??? NO!!! because only one channel was affected at a time.
		cmd_update_or_init_sensor_params(  pCmdS-> strBuf[0], n);	// update or init loadcell related variables. It calls loadcell_update_param() calls loadcell_recompute_weights_unit(), in turn, it calls lc_zero_init().
	}
	return 0;
}// end cmd_sensor_feature_init()


/**
 * After configured loadcell related configuration, update or init its variables.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if saving failed.
 *         0 error if saving pass.
 *
 *
 * History:  Created on 2007/02/19 by Wai Fai Chin
 */

BYTE cmd_loadcell_update_or_init( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	cmd_update_or_init_sensor_params(  pCmdS-> strBuf[0], pCmdS -> index); // update or init loadcell related variables. It calls loadcell_update_param() calls loadcell_recompute_weights_unit(), in turn, it calls lc_zero_init().
	return 0;
} // end cmd_loadcell_update_or_init()


/**
 * Updated related parameters or init parameters if the command is a default or reset.
 *
 * @param	ch		-- command type.
 * @param	sensorN -- sensor ID.
 * @post	update or init related variables.
 *
 * History:  Created on 2007/11/02 by Wai Fai Chin
 */

void cmd_update_or_init_sensor_params( BYTE ch, BYTE sensorN )
{
	LSENSOR_DESCRIPTOR_T		*pSnDesc;	// points to local sensor descriptor
	
	if ( sensorN < MAX_NUM_SENSORS ) {
		pSnDesc = &gaLSensorDescriptor[ sensorN ];
		switch ( pSnDesc-> type ) {
			case SENSOR_TYPE_LOADCELL :
					if ( sensorN < SENSOR_NUM_LAST_PV_LOADCELL ) {
						if ( 'z' == ch || 'Z' == ch ) 				// if it is a reset command
							loadcell_init( sensorN );
						else										// else it is update parameters command.
							loadcell_update_param( sensorN );		// loadcell_update_param() calls loadcell_recompute_weights_unit(), in turn, it calls lc_zero_init(). 
					}
				break;
			case SENSOR_TYPE_MATH_LOADCELL :
					if (SENSOR_NUM_MATH_LOADCELL == sensorN ) {
						if ( 'z' == ch || 'Z' == ch ) {				// if it is a reset command
							vs_math_init( sensorN );
							loadcell_init( sensorN );
						}
						else	{									// else it is update parameters command.
							vs_math_update_param( sensorN );		// loadcell_update_param() calls loadcell_recompute_weights_unit(), in turn, it calls lc_zero_init().
							loadcell_update_param( sensorN );		// loadcell_update_param() calls loadcell_recompute_weights_unit(), in turn, it calls lc_zero_init().
						}
					}
				break;
			case SENSOR_TYPE_VOLTMON :
					if ( (sensorN <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sensorN >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
						if ( 'z' == ch || 'Z' == ch ) 				// if it is a reset command
							voltmon_init( sensorN );
						else										// else it is update parameters command.
							voltmon_update_param( sensorN );
					}
				break;
			case SENSOR_TYPE_TEMP :
					if ( SENSOR_NUM_PCB_TEMPERATURE == sensorN )	{
						if ( 'z' == ch || 'Z' == ch ) 				// if it is a reset command
							temp_sensor_init( sensorN );
						else										// else it is update parameters command.
							temp_sensor_update_param( sensorN );
					}
				break;
		} // end switch();
	} // end if ( sensorN < MAX_NUM_SENSORS ) {}
} // end cmd_update_or_init_sensor_params(,)


/* *
 * Updated related parameters or init parameters if the command is a default or reset.
 *
 * @param	ch		-- command type.
 * @param	sensorN -- sensor ID.
 * @post	update or init related variables.
 *
 * History:  Created on 2007/11/30 by Wai Fai Chin
 * /

void cmd_update_loadcell_zero_band_params( BYTE ch, BYTE sensorN )
{
	SENSOR_DESCRIPTOR_T		*pSnDesc;	// points to sensor descriptor
	
	if ( sensorN < MAX_NUM_SENSORS ) {
		pSnDesc = &gaLSensorDescriptor[ sensorN ];
		if ( SENSOR_TYPE_LOADCELL == pSnDesc-> type ) {	// if the sensor is loadcell type.
			if ( sensorN < (MAX_NUM_LOADCELL - 1) ) {
				loadcell_update_param( sensorN );
				// lc_zero_init(sensorN);
			}
		}
	} // end if ( sensorN < MAX_NUM_SENSORS ) {}
} // end cmd_update_loadcell_zero_band_params(,)
*/


/**
 * It toggles net/gross mode of the specified loadcell.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2007/12/04 by Wai Fai Chin
 */

BYTE cmd_lc_toggle_net_gross_mode( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	BYTE		lc;
	LOADCELL_T	*pLc;
	
	status = CMD_STATUS_ERROR_INDEX;		// assumed bad.
	lc = pCmdS -> index;					// lc is the loadcell number or sensor ID
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		lc_tare_toggle_net_gross( pLc, lc );
		status = CMD_STATUS_NO_ERROR;
	}

	return status;
} // end cmd_lc_toggle_net_gross_mode()


/**
 * It zeros the specified loadcell.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2007/12/05 by Wai Fai Chin
 * 2015-05-12 check for ok to zero before zeroing a loadcell.
 */

BYTE cmd_lc_zero( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	BYTE		lc;
	
	status = cmd_is_ok_to_zero( pCmdS );		// 2015-05-12 -WFC-
	if ( CMD_STATUS_NO_ERROR == status ) {		// 2015-05-12 -WFC-
		status = CMD_STATUS_ERROR_INDEX;		// assumed bad.
		lc = pCmdS -> index;					// lc is the loadcell number or sensor ID
		if ( lc < MAX_NUM_PV_LOADCELL ) {
			lc_zero_by_command( lc );
			status = CMD_STATUS_NO_ERROR;
		}
	}
	return status;
} // end cmd_lc_zero()

/**
 * see if it is ok to zero.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if allow to zero, else return CMD_STATUS_ERROR_NOT_ALLOW
 *
 * History:  Created on 2015-05-12 by Wai Fai Chin
 */

BYTE cmd_is_ok_to_zero( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	BYTE		lc;

	status = CMD_STATUS_NO_ERROR;
	if ( (SCALE_STD_MODE_OIML == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK)) ) {
		lc = pCmdS -> index;					// lc is the loadcell number or sensor ID
		if ( lc < MAX_NUM_PV_LOADCELL ) {
			if ( LC_OP_MODE_NET_GROSS & *(gaLoadcell[ lc ].pOpModes) ) {	// if in NET mode,
				status = CMD_STATUS_ERROR_NOT_ALLOW;
			}
		}
	}

	return status;
} // end cmd_is_ok_to_zero()


/**
 * It undoes zeroing of the specified loadcell.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2007/12/05 by Wai Fai Chin
 * 2015-05-12 check for ok to zero before undo zero a loadcell.
 */

BYTE cmd_lc_undo_zero( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	BYTE		lc;

	status = cmd_is_ok_to_zero( pCmdS );		// 2015-05-12 -WFC-
	if ( CMD_STATUS_NO_ERROR == status ) {		// 2015-05-12 -WFC-
		status = CMD_STATUS_ERROR_INDEX;		// assumed bad.
		lc = pCmdS -> index;					// lc is the loadcell number or sensor ID
		if ( lc < MAX_NUM_PV_LOADCELL ) {
			lc_zero_undo_by_command( lc );
			status = CMD_STATUS_NO_ERROR;
		}
	}
	return status;
} // end cmd_lc_undo_zero()


/**
 * It auto tares the specified loadcell.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2007/12/05 by Wai Fai Chin
 */

BYTE cmd_lc_auto_tare( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	BYTE		lc;
	LOADCELL_T	*pLc;
	
	status = CMD_STATUS_ERROR_INDEX;		// assumed bad.
	lc = pCmdS -> index;					// lc is the loadcell number or sensor ID
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		lc_tare_gross( pLc, lc );
		status = CMD_STATUS_NO_ERROR;
	}

	return status;
} // end cmd_lc_auto_tare()


/**
 * It clears tare weight the specified loadcell.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2007/12/05 by Wai Fai Chin
 */

BYTE cmd_lc_clear_tare( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	BYTE		lc;
	LOADCELL_T	*pLc;
	
	status = CMD_STATUS_ERROR_INDEX;		// assumed bad.
	lc = pCmdS -> index;					// lc is the loadcell number or sensor ID
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		lc_tare_set( pLc, 0.0f, lc );
		status = CMD_STATUS_NO_ERROR;
	}

	return status;
} // end cmd_lc_clear_tare()

/**
 * This is a pre process of a password command.
 * {04=actionType; cmdlock; password}	Action type, cmdLock; password. Note that action type 0==normal, 1 == set password;
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2009/01/21 by Wai Fai Chin
 */

BYTE cmd_pre_password_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	cmd_mask_password();
	return CMD_STATUS_NO_ERROR;
} // end cmd_pre_password_action()


void cmd_mask_password( void )
{
	BYTE  	*bPtr;
	
	bPtr = & gStrPassword;
	*bPtr++ = '*';
	*bPtr++ = '*';
	*bPtr++ = '*';
	*bPtr = 0;
} // end cmd_mask_password()


/**
 * This is a post process of a password command.
 * {04=actionType; cmdlock; password}	Action type, cmdLock; password. Note that action type 0==normal, 1 == set password;
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2008/08/01 by Wai Fai Chin
 */

BYTE cmd_execute_password_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	len;
	BYTE	errorStatus;
	
	errorStatus = CMD_STATUS_NO_ERROR;
	switch ( gbPassWordAct ) {
		case	PASSWORD_NORMAL_MODE:
				// if the entered password matched the user password OR the default master password then,
				if ( strcmp( gStrPassword, gtProductInfoFNV.password ) == 0 ||
					 strcmp_P( gStrPassword, gcStrMasterPassword ) == 0 ) {
					gtProductInfoFNV.cmdLock = gbCmdLock;
				}
				else {
					errorStatus = CMD_STATUS_WRONG_PASSWORD;
				}
			break;
		case	PASSWORD_SET_MODE:
				if ( CMD_UNLOCKED == gtProductInfoFNV.cmdLock )	{
					len = is_psw_string_has_qualify_chars( gStrPassword );
					len++; // we want to copy the null char too.
					if ( len <= (MAX_PASSWORD_LENGTH + 1)) {
						// copy scratch memory to the actual password variable.
						copy_until_match_char( gtProductInfoFNV.password, gStrPassword, 0, len);
					}
					gbCmdLock = gtProductInfoFNV.cmdLock;		// overide user lockbit input, in case user also set the lock bit.
				}
				else {
					errorStatus = CMD_STATUS_CMD_LOCKED;
				}
			break;
		default:	errorStatus = CMD_STATUS_NO_SUCH_SUB_CMD;
	} // end switch()

	if ( errorStatus ) {			// cmd failed, need to restore the correct value.
		gbCmdLock = gtProductInfoFNV.cmdLock;
	}
	
	cmd_mask_password();		// put *** mask into gStrPassword;
	return errorStatus;
} // end cmd_execute_password_cmd()


/**
 * This is a post process of a goto bootloader command.
 * {09=password}	password is always required to goto bootloader.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2009/09/15 by Wai Fai Chin
 * 2011-03-03 -WFC- Added display bLoad message.
 * 2011-08-02 -WFC- It needs to go back to normal CPU system clock speed before enter boot loader because boot loader runs at normal clock speed.
 * 2011-08-18 -WFC- Added postAction item, {09=password;postAction}, postAction, bit0 =1 app code will performs master reset after bootloaded.
 */

BYTE cmd_goto_bootloader_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	errorStatus;

	errorStatus = CMD_STATUS_WRONG_PASSWORD;

	// if the entered password matched the user password OR the default master password then,
	if ( strcmp( gStrPassword, gtProductInfoFNV.password ) == 0 ||
		 strcmp_P( gStrPassword, gcStrMasterPassword ) == 0 ) {
			gAppBootShareNV.post_boot_status = gbAcmdTmp1;			// 2011-08-18 -WFC-
			gAppBootShareNV.app_boot_status |= APPBOOT_STATUS_NEED_UPDATE_APP;		// tell boot loader to stay in boot loader and wait for update app code.
			nv_cnfg_fram_save_with_8bitCRC( APPLICATION_BOOTLOADER_SHARE_FRAM_BASE_ADDR, &gAppBootShareNV, sizeof(APP_BOOT_SHARE_T));

			//led_display_format_bload(boot_load_String);		// PHJ
			//led_display_string( boot_load_String );		//	PHJ

			#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-		// 2011-03-03 -WFC-
				led_display_string_P( gcStr_bLoad );				// 2011-03-03 -WFC-
			#endif

			DISABLE_GLOBAL_INTERRUPT			// Disabled global interrupt before jump to the reset entry point of bootloader.
			#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )		// 2011-08-02 -WFC-
				bios_clock_normal();								// 2011-08-02 -WFC- It needs to go back to normal CPU system clock speed before enter boot loader because boot loader runs at normal clock speed.
			#endif
			#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )
				void (*pfCodeEntry)( void );							// function pointer to application entry
				pfCodeEntry = BOOTLOADER_START_ADDR;	// setup function pointer to bootloader entry point.
				MCUCR = (1<<IVCE);
				MCUCR = (1<<IVSEL);				// move interrupt vectors table to the boot loader sector.
				bios_system_reset_sys_regs();		// It is VERY IMPORTANT to reset system hardware control registers to its default state before jump to bootloader.
													// Otherwise, bootloader will get stuck in the start up routine without reset the control registers.
				pfCodeEntry();						// execute bootloader code from the beginning. It will never return here again from the bootloader.
			#else
				bios_system_reset_sys_regs();		// It is VERY IMPORTANT to reset system hardware control registers to its default state before jump to bootloader.
													// Otherwise, bootloader will get stuck in the start up routine without reset the control registers.
				BYTE b;
				/*
				b = PMIC.CTRL | PMIC_IVSEL_bm;		// prepare move interrupt vector table to bootloader sector
				CCP = CCP_IOREG_gc;					// Change Configuration Protection of IO registers for up to 4 CPU cycles.
				PMIC.CTRL = b; 						// set interrupt vector table start at bootloader sector.
				*/

				//asm("jmp 0x10000");
				b = 1;
				CCP = CCP_IOREG_gc;					// Change Configuration Protection of IO registers for up to 4 CPU cycles.
				RST.CTRL = b;						// software issue reset command to the CPU. Since the CPU is programmed reset to boot loader section, it will jump to bootloader as it was power up.
			#endif
	}

	return errorStatus;
} // end cmd_goto_bootloader_cmd

/*
BYTE cmd_goto_bootloader_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	errorStatus;
    void (*pfCodeEntry)( void );							// function pointer to application entry

	errorStatus = CMD_STATUS_WRONG_PASSWORD;

	// if the entered password matched the user password OR the default master password then,
	if ( strcmp( gStrPassword, gtProductInfoFNV.password ) == 0 ||
		 strcmp_P( gStrPassword, gcStrMasterPassword ) == 0 ) {
			gAppBootShareNV.app_boot_status |= APPBOOT_STATUS_NEED_UPDATE_APP;		// tell boot loader to update app code.
			nv_cnfg_fram_save_8bitCRC( APPLICATION_BOOTLOADER_SHARE_FRAM_BASE_ADDR, &gAppBootShareNV, sizeof(APP_BOOT_SHARE_T));

			DISABLE_GLOBAL_INTERRUPT			// Disabled global interrupt before jump to the reset entry point of bootloader.
			pfCodeEntry = BOOTLOADER_START_ADDR;	// setup function pointer to bootloader entry point.
			#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )
				MCUCR = (1<<IVCE);
				MCUCR = (1<<IVSEL);				// move interrupt vectors table to the boot loader sector.
			#endif
			bios_system_reset_sys_regs();		// It is VERY IMPORTANT to reset system hardware control registers to its default state before jump to bootloader.
												// Otherwise, bootloader will get stuck in the start up routine without reset the control registers.
			pfCodeEntry();						// execute bootloader code from the beginning. It will never return here again from the bootloader.
	}

	return errorStatus;
} // end cmd_goto_bootloader_cmd
*/

/**
 *  //This controls on board LEDs on/off state of a given LED number.
 * This is a general purpose instruction commands.
 * TESTONLY
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error. 
 *
 * History:  Created on 2008/08/25 by Wai Fai Chin
 * 2014-10-20 -WFC- {08=param;OPcode}  use as general purpose instruction command, more than just test hardware IOs.
 *
 */

BYTE cmd_execute_sc_hw_test_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE lc;

	// 2014-10-20 -WFC- v
	// 2014-10-20 -WFC- bios_on_board_led( gbSCHWTestState, gbSCHWTestCode );
	if ( gbSCHWTestCode < 9 )
		bios_on_board_led( gbSCHWTestState, gbSCHWTestCode );
	else {
		switch ( gbSCHWTestCode ) {
		case CMD_OP_CODE_CLEAR_USER_LIFT_COUNTER :
			for ( lc = 0; lc < MAX_NUM_PV_LOADCELL; lc++ ) {
				gaulUserLiftCntFNV[ lc ] = 0;
				nv_cnfg_fram_save_service_counters( lc );
			}
			break;
		case CMD_OP_CODE_SET_TMP_POWER_SAVING :
				gbTmpDisabledPowerSaving = TRUE;
				if ( 0 == gbSCHWTestState )
					gbTmpDisabledPowerSaving = FALSE;
			break;
		default:;
		}
	}
	// 2014-10-20 -WFC- ^
	return CMD_STATUS_NO_ERROR;
}
/**
 * This send command directly to LED display board if run mode is in Test External LED Board.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error. 
 *
 * History:  Created on 2009-02-18 by Wai Fai Chin
 */

BYTE cmd_execute_HW3460_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// hw3460_led_send_cmd( gbSCHWTestCode, gbSCHWTestState );
	#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
	if ( SYS_RUN_MODE_TEST_EXTERNAL_LED_BOARD == gbSysRunMode )
		led_display_send_hw_cmd( gbSCHWTestCode, gbSCHWTestState );
	#endif
	return CMD_STATUS_NO_ERROR;
} // end cmd_execute_HW3460_cmd()


/**
 * Send sensor data in packet format.
 *  {cmdinn;valueType; data; unit; numSeg; annunciator1; annunciator2; annunciator3; annunciator4; SysErrorCode}
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return CMD_STATUS_.
 *
 *
 * History:  Created on 2009-01-15 by Wai Fai Chin
 * 2011-01-04 -WFC- Added sys error code for user item.
 * 2011-05-18 -WFC- handle unsupporte sensor type.
 * 2015-09-09 -WFC- added setpoint annuciator after SysErrorCode.
 */

BYTE cmd_send_sensor_data_packet( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	n;
	BYTE	sn;
	BYTE	tmp;
	BYTE	errorStatus;
	char	str[ (CMD_MAX_STRING_LEN_PER_OUTPUT + CMD_MAX_STRING_LEN_PER_OUTPUT) ];
	
	sn = pCmdS -> index;					// sn is the sensor ID or channel number.
	errorStatus = CMD_STATUS_ERROR_INDEX;	// assumed error.
	if ( sn < MAX_NUM_SENSORS ) {
	// if ( sn < MAX_NUM_LSENSORS ) {
		n = sprintf_P( str, gcStrFmt_PacketHdr_Cmd, CMD_START_CHAR, pCmdS-> poStream-> sourceID, pCmdS-> poStream-> destID, pCmdS->cmd);
		str[n] = 'i';						// i flags it as 1D index array type answer.
		n++;
		n += sprintf_P( str + n, gcStrFmt_pct_d_c, (int) sn, CMD_ITEM_DELIMITER );	// sensor ID
		tmp = gbCMDTmp[ sn ];									// value Type
		// if ( tmp <= SENSOR_VALUE_TYPE_MAX )	{					// ensure value type is valid.
			n += sprintf_P( str + n, gcStrFmt_pct_d_c, (int) tmp, CMD_ITEM_DELIMITER );
			// 2011-05-18 -WFC- v
			// 2011-05-18 -WFC- n += sensor_format_data_packet( sn, tmp, str + n );
			tmp = sensor_format_data_packet( sn, tmp, str + n );
			if ( tmp ) {											// if senor type was supported and had formated data string
				n += tmp;
				str[n]= CMD_ITEM_DELIMITER;
				n++;
				n += sprintf_P( str + n, gcStrFmt_pct_d, (int) gbSysErrorCodeForUser );
				n += sensor_format_setpoint_annunciator( str + n );		// 2015-09-09 -WFC-
				str[n] = CMD_END_CHAR;
				n++;
				pCmdS-> state = CMD_STATE_ACKED;						// flag that we acknowledge this command, so cmd module skips default respond.
				//(*pCmdS-> pStream-> pSend_bytes)( str, n);			// send "{cmdinnn;valutype;data;unit;anc1;anc2;anc3;anc4;SysErrorCode}"
				stream_router_routes_a_ostream_now( pCmdS-> poStream, str, n );
				errorStatus = CMD_STATUS_NO_ERROR;
			}
			else
				errorStatus = CMD_STATUS_ERROR_SENSOR_NOT_SUPPORTED;
			// 2011-05-18 -WFC- ^
		// }
 	}
	
	return errorStatus;
} // end cmd_send_sensor_data_packet()


/**
 * Send sensor data in packet format.
 *  {cmdinn;grossWt; netWt; tareWt; LiftCnt|TtlCnt; iCb; decPoint; unit; numSeg; annunciator1; annunciator2; annunciator3; annunciator4; SysErrorCode}
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return CMD_STATUS_
 *
 *
 * History:  Created on 2011-08-10 by Wai Fai Chin
 * 2015-09-09 -WFC- added setpoint annuciator after SysErrorCode.
 */

BYTE cmd_send_loadcell_gnt_packet( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	n;
	BYTE	sn;
	BYTE	tmp;
	BYTE	errorStatus;
	char	str[ (CMD_MAX_STRING_LEN_PER_OUTPUT + CMD_MAX_STRING_LEN_PER_OUTPUT) ];

	sn = pCmdS -> index;					// sn is the sensor ID or channel number.
	errorStatus = CMD_STATUS_ERROR_INDEX;	// assumed error.
	if ( sn <= SENSOR_NUM_LAST_PV_LOADCELL ) {
		if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ sn ] ||
					 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[ sn ] ) {
		n = sprintf_P( str, gcStrFmt_PacketHdr_Cmd, CMD_START_CHAR, pCmdS-> poStream-> sourceID, pCmdS-> poStream-> destID, pCmdS->cmd);
		str[n] = 'i';						// i flags it as 1D index array type answer.
		n++;
		n += sprintf_P( str + n, gcStrFmt_pct_d_c, (int) sn, CMD_ITEM_DELIMITER );	// sensor ID
		tmp = loadcell_format_gnt_packet_output( str + n , sn );
		if ( tmp ) {											// if senor type was supported and had formated data string
			n += tmp;
			str[n]= CMD_ITEM_DELIMITER;
			n++;
			n += sprintf_P( str + n, gcStrFmt_pct_d, (int) gbSysErrorCodeForUser );
			n += sensor_format_setpoint_annunciator( str + n );		// 2015-09-09 -WFC-
			str[n] = CMD_END_CHAR;
			n++;
			pCmdS-> state = CMD_STATE_ACKED;						// flag that we acknowledge this command, so cmd module skips default respond.
			stream_router_routes_a_ostream_now( pCmdS-> poStream, str, n );
			errorStatus = CMD_STATUS_NO_ERROR;
		}
		else
			errorStatus = CMD_STATUS_ERROR_SENSOR_NOT_SUPPORTED;
		}
 	}

	return errorStatus;
} // end cmd_send_loadcell_gnt_packet()


/**
 * Send sensor data in packet format.
 *  {cmdinn valueType; data; LiftCnt|TtlCnt; iCb; decPoint; unit; annunciator1; annunciator2; annunciator3; annunciator4; SysErrorCode}
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return CMD_STATUS_.
 *
 *
 * History:  Created on 2012-02-13 by Wai Fai Chin
 * 2014-10-31 -WFC- Output status of auto total mode when it querys value type is total value.
 * 2015-09-09 -WFC- added setpoint annuciator after SysErrorCode.
 */

BYTE cmd_send_loadcell_vcb_packet( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	n;
	BYTE	sn;
	BYTE	tmp;
	BYTE	errorStatus;
	LOADCELL_T 		*pLc;		// points to a loadcell 2014-11-03 -WFC-
	char	str[ (CMD_MAX_STRING_LEN_PER_OUTPUT + CMD_MAX_STRING_LEN_PER_OUTPUT) ];

	sn = pCmdS -> index;					// sn is the sensor ID or channel number.
	errorStatus = CMD_STATUS_ERROR_INDEX;	// assumed error.
	if ( sn <= SENSOR_NUM_LAST_PV_LOADCELL ) {
		if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ sn ] ||
					 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[ sn ] ) {
		n = sprintf_P( str, gcStrFmt_PacketHdr_Cmd, CMD_START_CHAR, pCmdS-> poStream-> sourceID, pCmdS-> poStream-> destID, pCmdS->cmd);
		str[n] = 'i';						// i flags it as 1D index array type answer.
		n++;
		n += sprintf_P( str + n, gcStrFmt_pct_d_c, (int) sn, CMD_ITEM_DELIMITER );	// sensor ID
		tmp = gbCMDTmp[ sn ];									// value Type
		tmp = loadcell_format_vcb_packet_output( str + n , sn, tmp );
		if ( tmp ) {			// if senor type was supported and had formated data string
			pLc = &gaLoadcell[ sn ];
			n += tmp;
			str[n]= CMD_ITEM_DELIMITER;
			n++;
			// 2014-10-31 -WFC- v
			tmp = gbSysErrorCodeForUser;
			if ( SENSOR_VALUE_TYPE_TOTAL == gbCMDTmp[ sn ] ) {
				// only send this out if it is in auto total mode
				if ( pLc-> totalMode > LC_TOTAL_MODE_DISABLED &&
					 pLc-> totalMode < LC_TOTAL_MODE_LOAD_DROP )	{		// if in auto total modes,
					if ( LC_TOTAL_STATUS_DISABLED_AUTO_MODES & pLc-> totalT.status ) 		// if auto total modes disabled,
						tmp = SYS_USER_ERROR_CODE_AUTO_TOTAL_OFF;
					else
						tmp = SYS_USER_ERROR_CODE_AUTO_TOTAL_ON;
				}
			}

			// n += sprintf_P( str + n, gcStrFmt_pct_d, (int) gbSysErrorCodeForUser );
			n += sprintf_P( str + n, gcStrFmt_pct_d, (int) tmp );
			// 2014-10-31 -WFC- ^
			n += sensor_format_setpoint_annunciator( str + n );		// 2015-09-09 -WFC-
			str[n] = CMD_END_CHAR;
			n++;
			pCmdS-> state = CMD_STATE_ACKED;						// flag that we acknowledge this command, so cmd module skips default respond.
			stream_router_routes_a_ostream_now( pCmdS-> poStream, str, n );
			errorStatus = CMD_STATUS_NO_ERROR;
		}
		else
			errorStatus = CMD_STATUS_ERROR_SENSOR_NOT_SUPPORTED;
		}
 	}

	return errorStatus;
} // end cmd_send_loadcell_vcb_packet()

/**
 * Send sensor data in packet format:
 *  {cmdinn; data}
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return CMD_STATUS_.
 * \note If gbSysErrorCodeForUser >0 and < SYS_USER_ERROR_CODE_UNDER_VOLTAGE, then report the error code + 60 offset via {cmd00}.
 * History:  Created on 2012-02-13 by Wai Fai Chin
 * 2012-02-22 -WFC- If gbSysErrorCodeForUser >0 and < SYS_USER_ERROR_CODE_UNDER_VOLTAGE, then report the error code + 60 offset via {cmd00}.
 */

BYTE cmd_send_loadcell_simple_packet( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	n;
	BYTE	sn;
	BYTE	tmp;
	BYTE	errorStatus;
	char	str[ (CMD_MAX_STRING_LEN_PER_OUTPUT + CMD_MAX_STRING_LEN_PER_OUTPUT) ];

	sn = pCmdS -> index;					// sn is the sensor ID or channel number.
	errorStatus = CMD_STATUS_ERROR_INDEX;	// assumed error.
	if ( sn <= SENSOR_NUM_LAST_PV_LOADCELL ) {
		if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ sn ] ||
					 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[ sn ] ) {
		n = sprintf_P( str, gcStrFmt_PacketHdr_Cmd, CMD_START_CHAR, pCmdS-> poStream-> sourceID, pCmdS-> poStream-> destID, pCmdS->cmd);
		str[n] = 'i';						// i flags it as 1D index array type answer.
		n++;
		n += sprintf_P( str + n, gcStrFmt_pct_d_c, (int) sn, CMD_ITEM_DELIMITER );	// sensor ID
		tmp = gbCMDTmp[ sn ];									// value Type
		tmp = loadcell_format_weight_no_space( str + n , sn, tmp);
		if ( tmp ) {											// if sensor type was supported and had formated data string
			n += tmp;
			str[n] = CMD_END_CHAR;
			n++;
			if ( gbSysErrorCodeForUser > SYS_USER_ERROR_CODE_NO_ERROR && gbSysErrorCodeForUser < SYS_USER_ERROR_CODE_UNDER_VOLTAGE ) {
				errorStatus = CMD_STATUS_SYS_USER_ERROR_CODE_OFFSET + gbSysErrorCodeForUser;
			}
			else {
				pCmdS-> state = CMD_STATE_ACKED;						// flag that we acknowledge this command, so cmd module skips default respond.
				stream_router_routes_a_ostream_now( pCmdS-> poStream, str, n );
				errorStatus = CMD_STATUS_NO_ERROR;
			}
		}
		else
			errorStatus = CMD_STATUS_ERROR_SENSOR_NOT_SUPPORTED;
		}
 	}

	return errorStatus;
} // end cmd_send_loadcell_simple_packet()

/**
 * Send sensor test data in packet format.
 *  {cmdinn;valueType; data; unit; annunciator}
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return CMD_STATUS_.
 *
 *
 * History:  Created on 2009/02/25 by Wai Fai Chin
 */

BYTE cmd_stream_out_modes_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	char	str[ (CMD_MAX_STRING_LEN_PER_OUTPUT * 3) ];
	BYTE	n;
	BYTE	i;
	BYTE	errorStatus;
	IO_STREAM_CLASS			*pOstream;	// pointer to a dynamic out stream object.

	
	errorStatus = CMD_STATUS_ERROR_INDEX;	// assumed error.
	i  = pCmdS -> index;					// i is the stream listener number.
	if ( i < MAX_NUM_STREAM_LISTENER ) {
		if ( SYS_RUN_MODE_NORMAL == gbSysRunMode) {
			if ( (gabListenerModesFNV[i] > LISTENER_MODE_ADC_TEST) &&
					(gabListenerModesFNV[i] < LISTENER_MODE_CUSTOMER_SPECIAL)) { // if it is in ADC test packet mode.
				if (stream_router_get_a_new_stream( &pOstream ) ) {		// get an output stream to this listener.
					pOstream-> type		= gabListenerStreamTypeFNV[i];		// stream type for the listener.
					pOstream-> status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
					pOstream-> sourceID	= gtProductInfoFNV.devID;
					pOstream-> destID	= gabListenerDevIdFNV[ i ];
					n = data_out_manage_format_test_data_packet( i, str );
					if ( n > 0 ) {										// if it has valid format data.
						stream_router_routes_a_ostream_now( pOstream, str, n );
						timer_mSec_set( &gatListenerTimer[i], gabListenerIntervalFNV[i]);
						pCmdS-> state = CMD_STATE_ACKED;				// flag that we acknowledge this command, so cmd module skips default respond.
					} // end if ( n >0 ) {}.
					stream_router_free_stream( pOstream );	// put resource back to stream pool.
				} // end if (stream_router_get_a_new_stream( &pOstream ) ) {}
			} // end if ( mode > LISTENER_MODE_ADC_TEST ) {}
		} // end if ( SYS_RUN_MODE_NORMAL == gbSysRunMode) {}
		errorStatus = CMD_STATUS_NO_ERROR;
	} // end if ( i < MAX_NUM_STREAM_LISTENER ) 

	return errorStatus;
} // end cmd_stream_out_modes_action()


/**
 * This is a post process of a password command.
 * {2C=setpoint_num;sensorID;cmplogic_valueMode;hystCB;fcmpValue} 
 * cmplogic_valueMode is format as bit 7== 0=disabled, 1== enabled, bit 6 to 4 compare logic, 1== '<', 2== '>';
 * bit 3 to 0; value mode; 0 == net_gross, 1== gross, 2==total.
 *
 * This function ensure that fcmpValue is less than the capacity if the sensor is loadcell type.
 * 
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2009/06/23 by Wai Fai Chin
 */
BYTE cmd_setpoint_value_validation( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	// pCmdS-> index is the setpoint number.
	status = setpoint_value_validation( pCmdS -> index );
	return status;
}// end cmd_setpoint_value_validation()


/**
 * This method handles DAC calibration of offset, gain, minimum span point and maximum span point.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * History:  Created on 2010/01/25 by Wai Fai Chin
 */

BYTE cmd_execute_cal_dac_cmd_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== DAC channel;
	// gbAcmdTmp1	== CalType; 0 == Aborted; 1 == save cal and exit; 2==offset, 3==gain; 4==min span point; 5== max span point;
	// gi16AcmdTmp1	== OpCode;  0 == init; else it is adjustment value range -32768 to 32767.

	BYTE	errorStatus;
	INT8	a;
	BYTE	b;
	BYTE	dacChannel;
	UINT16  wDacData;
	INT16	i16A;

	//dacChannel = pCmdS->index;									// DAC channel
	dacChannel = gbCMD_Index;
	errorStatus = CMD_STATUS_NO_ERROR;							// assumed passed.
	if ( dacChannel < MAX_DAC_CHANNEL )	{
		gabDacCnfgStatusFNV[ dacChannel ] |= DAC_STATUS_IN_CAL_MODE;
		switch ( gbAcmdTmp1 ) {
		case DAC_CAL_TYPE_OFFSET:
			wDacData = 0x800;									// midpoint of the DAC output.
			a = gabDacOffsetFNV[ dacChannel ];
			if ( gi16AcmdTmp1 == 0 ) {							// if opCode is for set initial condition
				a = 0;
				dac_cpu_set_gain( dacChannel, a );
			}
			else {
				i16A = (INT16) a;
				i16A += gi16AcmdTmp1;
				if ( i16A < MIN_DAC_OFFSET)
					i16A = MIN_DAC_OFFSET;
				else if ( i16A > MAX_DAC_OFFSET) {
					i16A = MAX_DAC_OFFSET;
				}
				a = (BYTE) i16A;
			}
			gabDacOffsetFNV[ dacChannel ] = a;
			dac_cpu_set_offset( dacChannel, a );
			dac_cpu_output( dacChannel, wDacData  );
			break;
		case DAC_CAL_TYPE_GAIN:
			wDacData = 0xFFF;									// max value of the DAC output.
			a = gabDacGainFNV[ dacChannel ];
			if ( gi16AcmdTmp1 == 0 ) {							// if opCode is for set initial condition
				a = 0;
				dac_cpu_set_gain( dacChannel, a );
			}
			else {
				i16A = (INT16) a;
				i16A += gi16AcmdTmp1;
				if ( i16A < MIN_DAC_GAIN)
					i16A =  MIN_DAC_GAIN;
				else if ( i16A > MAX_DAC_GAIN) {
					i16A = MAX_DAC_GAIN;
				}
				a = (BYTE) i16A;
			}
			gabDacGainFNV[ dacChannel ] = a;
			dac_cpu_set_gain( dacChannel, a );
			dac_cpu_output( dacChannel, wDacData  );
			break;
		case DAC_CAL_TYPE_MIN_SPAN:
			wDacData = gawDacCountMinSpanFNV[ dacChannel ];	// min value of the DAC output for the min value of a sensor.
			if ( gi16AcmdTmp1 == 0 ) {							// if opCode is for set initial condition
				wDacData = 0;									// initial DAC count for min output
			}
			else {
				i16A = (INT16) wDacData;
				i16A += gi16AcmdTmp1;
				if ( i16A < 0)
					i16A = 0;
				else if ( i16A > MAX_DAC_VALUE) {
					i16A = MAX_DAC_VALUE;
				}
				wDacData = (UINT16) i16A;
			}
			gawDacCountMinSpanFNV[ dacChannel ] = wDacData;
			dac_cpu_output( dacChannel, wDacData );
			break;
		case DAC_CAL_TYPE_MAX_SPAN:
			wDacData = gawDacCountMaxSpanFNV[ dacChannel ];	// min value of the DAC output for the min value of a sensor.
			if ( gi16AcmdTmp1 == 0 ) {							// if opCode is for set initial condition
				wDacData = 0xFFF;								// initial DAC count for min output
			}
			else {
				i16A = (INT16) wDacData;
				i16A += gi16AcmdTmp1;
				if ( i16A < 0)
					i16A = 0;
				else if ( i16A > MAX_DAC_VALUE) {
					i16A = MAX_DAC_VALUE;
				}
				wDacData = (UINT16) i16A;
			}
			gawDacCountMaxSpanFNV[ dacChannel ] = wDacData;
			dac_cpu_output( dacChannel, wDacData );
			break;
		case DAC_CAL_TYPE_SAVE_EXIT:
			b = gabDacCnfgStatusFNV[ dacChannel ];
			b &= ~DAC_STATUS_IN_CAL_MODE;
			b |= DAC_STATU_CALIBRATED;
			gabDacCnfgStatusFNV[ dacChannel ] = b;
			nv_cnfg_fram_save_dac_config( dacChannel );
			dac_cpu_init_settings( dacChannel );
			break;
		case DAC_CAL_TYPE_ABORT:
			nv_cnfg_fram_recall_dac_config( dacChannel );
			dac_cpu_init_settings( dacChannel );
			break;
		}
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_execute_cal_dac_cmd_mapper()

/**
 * This method force DAC output with the user specified value.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 * @note: In order for the DAC holds its output, Ensured that DAC high level status bit5=1 for manual mode.
 * Command {30} let you set it to manual mode. Manual mode bit will over ride the enable bit.
 * That is, it will force DAC to output even it is disabled.
 *
 * History:  Created on 2010/05/07 by Wai Fai Chin
 */

BYTE cmd_execute_manual_out_dac_cmd_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== DAC channel;
	// gu16AcmdTmp1	== DAC count for output.

	BYTE	errorStatus;

	errorStatus = CMD_STATUS_NO_ERROR;							// assumed passed.
	if ( gbCMD_Index < MAX_DAC_CHANNEL )	{
		dac_cpu_output( gbCMD_Index, gu16AcmdTmp1 );
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_execute_manual_out_dac_cmd_mapper()


/**
 * This method handles source sensor value settings for DAC configuration.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * History:  Created on 2010/01/27 by Wai Fai Chin
 */

BYTE cmd_execute_cnfg_sensor_dac_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// pCmds-> index == DacChannel number
	nv_cnfg_fram_save_dac_config( pCmdS->index );
	dac_cpu_init_settings( pCmdS->index );
	return 0;
} // end cmd_execute_cnfg_sensor_dac_cmd


/**
 * This method handles source sensor value settings for DAC configuration.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * History:  Created on 2010/03/10 by Wai Fai Chin
 */

BYTE cmd_execute_cnfg_math_channel_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	vs_math_init_mc( pCmdS->index, gabVSMathSensorIdFNV[ pCmdS->index ]);
	return 0;
} // end cmd_execute_cnfg_math_channel_cmd

/**
 * It maps Rcal value into string in the precision based on countby
 * to host command variables so the command engine will answer the
 * query result back to host.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: must enabled Rcal on first with the {13} command before use {35} cmd.
 *
 * command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2010/04/02	by Wai Fai Chin
 */

BYTE cmd_loadcell_rcal_query_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== loadcellID;
	// gStrAcmdTmp	== value in ASCII string;
	return cmd_read_loadcell_rcal_value( gbCMD_Index, gStrAcmdTmp );
} // end cmd_loadcell_rcal_query_pre_action_mapper()

/**
 * It reads Rcal value into string in the precision based on countby
 * to host command variables so the command engine will answer the
 * query result back to host.
 *
 * @param  lc	-- local real loadcell number.
 * @param  pStr	-- pointer to a string buffer to be store Rcal value in ASCII.
 *
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 * @note: must enabled Rcal on first with the {13} command before use {35} cmd.
 *
 * History:  Created on 2010-11-17	by Wai Fai Chin
 * 2011-04-22 -WFC- simplified logic by calling loadcell_format_rcal_string().
 */

BYTE cmd_read_loadcell_rcal_value( BYTE lc, BYTE *pStr )
{
	// global scratch memory for this module.
	// gbCMD_Index	== loadcellID;
	// gStrAcmdTmp	== value in ASCII string;

	BYTE					errorStatus;

// 2011-05-19 -WFC- #if ( CONFIG_PCB_AS == CONFIG_PCB_AS_SCALECORE2)
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-05-19 -WFC-
	SENSOR_CAL_T			*pSensorCal;
	LSENSOR_DESCRIPTOR_T	*pRcalSnDesc;	// points to Rcal input sensor descriptor
	MSI_CB					cb;

	errorStatus = CMD_STATUS_NO_ERROR;								// assumed passed.
	if ( lc < MAX_NUM_LOADCELL )	{
		if ( sensor_get_cal_base( lc, &pSensorCal ) ) {		// if gbCMD_Index is a valid sensorID.
			if (gabLoadcellRcalEnabled[ lc ]) {
				if ( ((pSensorCal-> status > 0) && (pSensorCal-> status <= MAX_CAL_POINTS)) ||	// if it has at least 2 valid cal points OR
						(CAL_STATUS_COMPLETED == pSensorCal-> status)) { 						// have a completed cal table.
					pRcalSnDesc = &gaLSensorDescriptor[ SENSOR_NUM_RCAL ];
					pRcalSnDesc-> value = adc_to_value( pRcalSnDesc-> curADCcount, &(pSensorCal-> adcCnt[0]), &(pSensorCal-> value[0]));
					cb = pSensorCal-> countby;
					cal_next_lower_countby( &cb );
					float_round_to_string( pRcalSnDesc-> value, &cb, 8, pStr );
				}
				else {
					errorStatus = CMD_STATUS_ERROR_UNCAL_SENSOR;
				}
			}
			else {
				errorStatus = CMD_STATUS_ERROR_RCAL_DISABLED;
			}
		}
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}
#else
	errorStatus = CMD_STATUS_NO_ERROR;								// assumed passed.
	if ( lc < MAX_NUM_LOADCELL )	{
		loadcell_format_rcal_string( lc, gStrAcmdTmp );
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}
#endif
	return errorStatus;
} // end cmd_read_loadcell_rcal_value()



/**
 * It handles manual totaling of the specified loadcell.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 * @note:
 *   The caller or listener needs to check the loadcell total status before query the total weight.
 *
 * History:  Created on 2010/04/12 by Wai Fai Chin
 * 2014-10-29 -WFC- modified it to total any total mode.
 */

BYTE cmd_execute_lc_manual_total( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	BYTE		lc;
	LOADCELL_T	*pLc;

	status = CMD_STATUS_ERROR_INDEX;		// assumed bad.
	lc = pCmdS -> index;					// lc is the loadcell number or sensor ID
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		if ( LC_TOTAL_MODE_ON_COMMAND == pLc-> totalMode ) {
			lc_total_evaluate( pLc, lc );
			// 2014-10-29 -WFC- lc_total_handle_command( lc );
		}
		lc_total_handle_command( lc );		// 2014-10-29 -WFC-
		status = CMD_STATUS_NO_ERROR;
	}
	return status;
} // end cmd_execute_lc_manual_total()


/**
 * It handles manual zeroing total and total statistics of the specified loadcell.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 * @note:
 *   The caller or listener needs to check the loadcell total status before query the total weight.
 *
 * History:  Created on 2010/04/13 by Wai Fai Chin
 */

BYTE cmd_execute_lc_zero_total( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	BYTE		lc;
	LOADCELL_T	*pLc;

	status = CMD_STATUS_ERROR_INDEX;		// assumed bad.
	lc = pCmdS -> index;					// lc is the loadcell number or sensor ID
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		lc_total_clear_total( pLc, lc );
		status = CMD_STATUS_NO_ERROR;
	}
	return status;
} // end cmd_execute_lc_zero_total()


/**
 * It handles manual totaling of the specified loadcell.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 * @note:
 *   The caller or listener needs to check the loadcell total status before query the total weight.
 *
 * History:  Created on 2010/04/13 by Wai Fai Chin
 * 2011-11-04 -WFC- v Detected and prevent min value > max value and report error. Its values are auto corrected by swapped them. This resolved problem report #932.
 */

BYTE cmd_execute_cnfg_bargraph_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	status;
	float 	fV;

	status = CMD_STATUS_NO_ERROR;
	// 2011-11-04 -WFC- v Detected and prevent min value > max value and report error. Its values are auto corrected by swapped them. This resolved problem report #932.
	if ( gtSystemFeatureFNV.minValue1stSegment > gtSystemFeatureFNV.maxValueLastSegment ) {
		fV = gtSystemFeatureFNV.maxValueLastSegment;
		gtSystemFeatureFNV.maxValueLastSegment = gtSystemFeatureFNV.minValue1stSegment;
		gtSystemFeatureFNV.minValue1stSegment = fV;
		status = CMD_STATUS_ERROR_MIN_GT_MAX_SWAPPED;		// error had occurred and auto corrected by swapped them.
	}
	// 2011-11-04 -WFC- ^
	data_out_compute_bargraph_resolution();
	return status;
} // end cmd_execute_cnfg_bargraph_cmd()


/**
 * This is a post process of a master system configuration reset command.
 * {3B=password}	password is always required.
 * After it default all configuration, it reset the system.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2010/05/20 by Wai Fai Chin
 * 2011-08-18 -WFC- For DynaLink2, called bios_clock_normal() before goes to boot loader. because boot loader runs at normal CPU clock speed.
 */

BYTE cmd_master_default_system_configuration_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	errorStatus;

	errorStatus = CMD_STATUS_WRONG_PASSWORD;

	// if the entered password matched the user password OR the default master password then,
	if ( strcmp( gStrPassword, gtProductInfoFNV.password ) == 0 ||
		 strcmp_P( gStrPassword, gcStrMasterPassword ) == 0 ) {
		 main_system_master_default_system_configuration();

			DISABLE_GLOBAL_INTERRUPT			// Disabled global interrupt before jump to the reset entry point of bootloader.
			#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )
				#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
					bios_clock_normal();		// 2011-08-18 -WFC- because boot loader runs at normal CPU clock speed.
				#endif

				void (*pfCodeEntry)( void );							// function pointer to application entry
				pfCodeEntry = BOOTLOADER_START_ADDR;	// setup function pointer to bootloader entry point.
				MCUCR = (1<<IVCE);
				MCUCR = (1<<IVSEL);				// move interrupt vectors table to the boot loader sector.
				bios_system_reset_sys_regs();		// It is VERY IMPORTANT to reset system hardware control registers to its default state before jump to bootloader.
													// Otherwise, bootloader will get stuck in the start up routine without reset the control registers.
				pfCodeEntry();						// execute bootloader code from the beginning. It will never return here again from the bootloader.
			#else
				bios_system_reset_sys_regs();		// It is VERY IMPORTANT to reset system hardware control registers to its default state before jump to bootloader.
													// Otherwise, bootloader will get stuck in the start up routine without reset the control registers.
				BYTE b;
				/*
				b = PMIC.CTRL | PMIC_IVSEL_bm;		// prepare move interrupt vector table to bootloader sector
				CCP = CCP_IOREG_gc;					// Change Configuration Protection of IO registers for up to 4 CPU cycles.
				PMIC.CTRL = b; 						// set interrupt vector table start at bootloader sector.
				*/

				//asm("jmp 0x10000");
				b = 1;
				CCP = CCP_IOREG_gc;					// Change Configuration Protection of IO registers for up to 4 CPU cycles.
				RST.CTRL = b;						// software issue reset command to the CPU. Since the CPU is programmed reset to boot loader section, it will jump to bootloader as it was power up.
			#endif
	}

	return errorStatus;
} // end cmd_master_default_system_configuration_cmd()



/**
 * This is a post process of a memory check command.
 * {3C=password}	password is always required.
 * It verify all non volatile memory chips.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2010/05/26 by Wai Fai Chin
 */

BYTE cmd_memory_check_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	errorStatus;

	errorStatus = CMD_STATUS_WRONG_PASSWORD;

	// if the entered password matched the user password OR the default master password then,
	if ( strcmp( gStrPassword, gtProductInfoFNV.password ) == 0 ||
		 strcmp_P( gStrPassword, gcStrMasterPassword ) == 0 ) {
		 errorStatus = nvmem_check_memory();
	}

	return errorStatus;
} // end cmd_memory_check_cmd()


/**
 * It clears global scratch memory of this command module.
 *
 * History:  Created on 2010/06/24 by Wai Fai Chin
 */

void cmd_clear_global_scratch_memory( void )
{
	BYTE i;

	gfAcmdTmp1 = gfAcmdTmp2 = 0.0f;
	gi32AcmdTmp1 = 0;
	gu16AcmdTmp1 = 0;
	gi16AcmdTmp1 = 0;
	gbAcmdTmp1 = gbAcmdTmp2 = gbAcmdTmp3 = 0;
	gi8AcmdTmp1 = 0;

	for ( i =0; i < CMD_MAX_TMP_STRING_LENGTH; i++ )
		gStrAcmdTmp[i] = 0;

	for ( i=0; i < MAX_NUM_SENSORS; i++)
	 gbCMDTmp[i] = 0;

} // end cmd_clear_global_scratch_memory()

// **********************************************************************************
//  				Reliable Send Command functions:
// **********************************************************************************

/**
 * It formats and outputs command to a destID via a given streamType.
 *
 * @param  streamType	-- stream type. IO_STREAM_TYPE_UART =0, IO_STREAM_TYPE_UART_1 = 1, IO_STREAM_TYPE_SPI = 2, IO_STREAM_TYPE_RFMODEM = 3,
 * @param  destID		-- destination ID of a device.
 * @param  cmd			-- command.
 * @param  pStr			-- points to a command parameter string buffer.
 *
 * @return none
 *
 * History:  Created on 2010/07/14 by Wai Fai Chin
 */

void	cmd_send_command( BYTE streamType, BYTE destID, BYTE cmd, BYTE *pStr)
{
	char	str[ (CMD_MAX_STRING_LEN_PER_OUTPUT * 2) ];
	BYTE	n;
	BYTE	i;
	IO_STREAM_CLASS			*pOstream;	// pointer to a dynamic out stream object.

	if (stream_router_get_a_new_stream( &pOstream ) ) {			// get an output stream to this listener.
		pOstream-> type		= streamType;						// stream type for the listener.
		pOstream-> status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
		pOstream-> sourceID	= gtProductInfoFNV.devID;
		pOstream-> destID	= destID;
		n = sprintf_P( str, gcStrFmt_PacketHdr_Cmd, CMD_START_CHAR,
			gtProductInfoFNV.devID,	destID, cmd);
		n += (BYTE) sprintf_P( str + n, PSTR("%s"), pStr);
		str[n] = CMD_END_CHAR;
		n++;
		if ( n > 0 ) {										// if it has valid format data.
			stream_router_routes_a_ostream_now( pOstream, str, n );
		} // end if ( n >0 ) {}.
		stream_router_free_stream( pOstream );	// put resource back to stream pool.
	} // end if (stream_router_get_a_new_stream( &pOstream ) ) {}

} // end cmd_send_command()

// 2011-09-27 -WFC- #if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD )
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-09-27 -WFC-

/**
 * It is a reliable command communication main task loop.
 * It walks through all cloned gaCmdReliableSendManagerObjs to send commands to a specified devices.
 *
 * History:  Created on 2010/07/19 by Wai Fai Chin
 */

void cmd_reliable_send_commands_main_tasks( void )
{
	BYTE i;
	// walk through all cmdRSManager objects
	for ( i = 0; i< CMD_RELIABLE_SEND_MANAGER_MAX_NUM_OBJ; i++) {
		cmd_reliable_send_command_main_thread(i);
	}
} // end cmd_reliable_send_command_main_tasks()

/**
 * It is a reliable command communication main thread. It is clone-able by gaCmdReliableSendManagerObj[]
 * which is index by managerID.
 *
 * @param  managerID	-- index of gaCmdReliableSendManagerObj[]. It became a cloned object working in conjunction with this main thread.
 *
 * History:  Created on 2010/07/19 by Wai Fai Chin
 */

//PT_THREAD( cmd_reliable_send_command_main_thread( BYTE managerID )) // Doxygen cannot handle this macro
char cmd_reliable_send_command_main_thread( BYTE managerID )
{
  PT_BEGIN( &(gaCmdReliableSendManagerObj[ managerID].m_pt) );
  for ( ;;) {
	  if ( CMD_SEND_MANAGER_STATUS_UNUSED == gaCmdReliableSendManagerObj[ managerID].status ) {
		  if ( cmd_get_a_request_send_cmdRSobj( &(gaCmdReliableSendManagerObj[ managerID].pCmdRSobj) )) {
			  gaCmdReliableSendManagerObj[ managerID].status = CMD_SEND_MANAGER_STATUS_HAS_CMDRSOBJ;
			  PT_SPAWN( &(gaCmdReliableSendManagerObj[ managerID].m_pt),
					  &(gaCmdReliableSendManagerObj[ managerID].pCmdRSobj->m_pt),
					  cmd_reliable_send_command_thread( gaCmdReliableSendManagerObj[ managerID].pCmdRSobj) );
			  gaCmdReliableSendManagerObj[ managerID].status = CMD_SEND_MANAGER_STATUS_UNUSED;
		  }
	  }
	  PT_YIELD( &(gaCmdReliableSendManagerObj[ managerID].m_pt) );
  }// end endless for loop.

  PT_END(&(gaCmdReliableSendManagerObj[ managerID].m_pt));

} // end cmd_reliable_send_command_main_thread()

/**
 * It is a reliable command communication thread. It guarantees the remote device will respond.
 * It will retry send command to the remote device a few time before it declared no communication.
 *
 * @param  pCmdRSobj	-- pointer to reliable send command object.
 *
 * History:  Created on 2010/07/16 by Wai Fai Chin
 */

//PT_THREAD( cmd_reliable_send_command_thread( struct pt *pt )) // Doxygen cannot handle this macro
char cmd_reliable_send_command_thread( CMD_RELIABLE_SENDING_CLASS_T *pCmdRSobj )
{
  static BYTE i;
  BYTE	ch;

  PT_BEGIN( &(pCmdRSobj-> m_pt) );
	pCmdRSobj-> status = CMD_SEND_OBJ_STATUS_BUSY;
	cmd_send_command( pCmdRSobj-> streamType, pCmdRSobj-> destID, pCmdRSobj-> cmd, pCmdRSobj-> parameterStr);
	//wait until remote device acknowledged or timeout.
    // timer_mSec_set( &(pCmdRSobj-> timer), TT_1SEC);    // Note this statement belongs to PT_BEGIN() which after the expanded statement case:0.
    timer_mSec_set( &(pCmdRSobj-> timer), TT_1p5SEC);    // Note this statement belongs to PT_BEGIN() which after the expanded statement case:0.
	PT_WAIT_UNTIL( &(pCmdRSobj-> m_pt),
					(pCmdRSobj-> status & CMD_SEND_OBJ_STATUS_SUCCESS ) ||
					timer_mSec_expired( &(pCmdRSobj-> timer)));				// wait for ack or timeout

	if ( CMD_SEND_OBJ_STATUS_BUSY == pCmdRSobj-> status) {					// if this object still busy wait for ack, then it must be timeout
		pCmdRSobj-> status = CMD_SEND_OBJ_STATUS_TIMEOUT;
		gbConnectionStatus |= CMD_ACT_CONNECT_STATUS_NO_ACK_FROM_REMOTE;	// had no ack from remote
	}
	else
		gbConnectionStatus &= ~CMD_ACT_CONNECT_STATUS_NO_ACK_FROM_REMOTE;	// got an ack from remote

  PT_END( &(pCmdRSobj-> m_pt) );
} // end cmd_reliable_send_command_thread()

/**
 * It initialized both gaCmdReliableSendManagerObj[] and gaCmdReliableSendObj[].
 *
 * History:  Created on 2010/07/19 by Wai Fai Chin
 */

void cmd_reliable_send_commands_main_init( void )
{
	BYTE	i;

	// walk through all cmdRSManager objects
	for ( i = 0; i < CMD_RELIABLE_SEND_MANAGER_MAX_NUM_OBJ; i++) {
		PT_INIT( &(gaCmdReliableSendManagerObj[i].m_pt));
		gaCmdReliableSendManagerObj[i].status = CMD_SEND_MANAGER_STATUS_UNUSED;
	}

	// walk through all cmdRSobj objects
	for ( i = 0; i < CMD_RELIABLE_SEND_MAX_NUM_OBJ; i++) {
		PT_INIT( &(gaCmdReliableSendObj[i].m_pt));
		gaCmdReliableSendObj[i].status = CMD_SEND_OBJ_STATUS_UNUSED;
	}

} // end cmd_reliable_send_commands_main_init()

/**
 * It trys to find a cmdRSobj of a given type and saved its pointer to ppCmdRSobj.
 *
 * @param	ppCmdRSobj	-- pointer of pointer to a cmdRSobj. It is for saved the pointer of a cmdRSobj.
 * @param	type		-- type of cmdRSobj, such as UNUSED, REQ_SEND etc..
 * @return  PASSED if it found a cmdRSobj of given type, else FAILED.
 *
 * History:  Created on 2010/07/20 by Wai Fai Chin
 */

BYTE cmd_get_cmdRSobj_of_given_type( CMD_RELIABLE_SENDING_CLASS_T **ppCmdRSobj, BYTE type )
{
	BYTE	i;
	BYTE status;

	status = FAILED;
	for ( i = 0; i < CMD_RELIABLE_SEND_MAX_NUM_OBJ; i++) {
		if ( type == gaCmdReliableSendObj[i].status) {
			*ppCmdRSobj = &gaCmdReliableSendObj[i];
			status = PASSED;
			break;
		}
	}
	return status;
} // end cmd_get_cmdRSobj_of_given_type()

/**
 * It trys to reclaim success or timeout cmdRSobj by set them to unused status.
 *
 * History:  Created on 2010/07/20 by Wai Fai Chin
 */

void cmd_cmdRSobj_garbage_collector( void )
{
	BYTE	i;
	BYTE	*pStatus;

	// walk through all cmdRSobj objects
	for ( i = 0; i < CMD_RELIABLE_SEND_MAX_NUM_OBJ; i++) {
		pStatus = &gaCmdReliableSendObj[i].status;
		if ( ((CMD_SEND_OBJ_STATUS_SUCCESS | CMD_SEND_OBJ_STATUS_TIMEOUT) & *pStatus) )
			*pStatus = CMD_SEND_OBJ_STATUS_UNUSED;
	}

} // end cmd_cmdRSobj_garbage_collector()


/**
 * 1st it trys to find an unused cmdRSobj. Once it found an unused cmdRSobj,
 * it fills in the cmdRSobj members based on the caller supplied parameters.
 *
 * @param  streamType	-- stream type. IO_STREAM_TYPE_UART =0, IO_STREAM_TYPE_UART_1 = 1, IO_STREAM_TYPE_SPI = 2, IO_STREAM_TYPE_RFMODEM = 3,
 * @param  destID		-- destination ID of a device.
 * @param  cmd			-- command.
 * @param  pStr			-- points to a command parameter string buffer.
 *
 * @return PASSED if it success, else FAILED.
 *
 * History:  Created on 2010/07/20 by Wai Fai Chin
 */

BYTE	cmd_setup_command_for_cmdRSobj( BYTE streamType, BYTE destID, BYTE cmd, BYTE *pStr)
{
	BYTE n;
	BYTE status;
	CMD_RELIABLE_SENDING_CLASS_T		*pCmdRSobj;

	status = FAILED;
	if ( cmd_get_an_unused_cmdRSobj( &pCmdRSobj ) ) {
		pCmdRSobj-> streamType	= streamType;
		pCmdRSobj-> destID		= destID;
		pCmdRSobj-> cmd			= cmd;
		pCmdRSobj-> status		= CMD_SEND_OBJ_STATUS_REQ_SEND;
		n = strlen(	pStr );
		if ( n < CMD_SEND_MAX_PARAMETER_STR_LEN ) {
			strcpy(	pCmdRSobj->parameterStr, pStr);
			status = PASSED;
		}
	}
	return status;
} // end cmd_setup_command_for_cmdRSobj()

/**
 * It checks for acknowledgment of a sent command by comparing source ID and received command.
 *
 * @param srcID			-- sourceID of a device.
 * @param receivedCmd	-- received command from the sourceID device.
 *
 * History:  Created on 2010-07-21 by Wai Fai Chin
 */

void cmd_reliable_send_manager_check_ack( BYTE srcID, BYTE receivedCmd )
{
	BYTE	i;
	CMD_RELIABLE_SENDING_CLASS_T		*pCmdRSobj;

	// walk through all cmdRSManager objects
	for ( i = 0; i < CMD_RELIABLE_SEND_MANAGER_MAX_NUM_OBJ; i++) {
		if ( CMD_SEND_MANAGER_STATUS_HAS_CMDRSOBJ == gaCmdReliableSendManagerObj[i].status) {
			pCmdRSobj = gaCmdReliableSendManagerObj[i].pCmdRSobj;
			if ( IO_STREAM_TYPE_UART_1 == pCmdRSobj->streamType )					// HD connects DSC via IO_STREAM_TYPE_UART_1
				gbConnectionStatus |= CMD_ACT_CONNECT_STATUS_RECEIVED_FROM_REMOTE;	// HD received info from DSC.
			if ( (srcID == pCmdRSobj-> destID) && ( receivedCmd == pCmdRSobj-> cmd )) {
				pCmdRSobj-> status = CMD_SEND_OBJ_STATUS_SUCCESS;
			}
		}
	}
} // end cmd_reliable_send_commands_main_init()

#endif

/**
 * This function is use by this application software itself for self generated
 *  query command and send result to a host or remote device.
 *
 * @param  streamType	-- stream type. IO_STREAM_TYPE_UART =0, IO_STREAM_TYPE_UART_1 = 1, IO_STREAM_TYPE_SPI = 2, IO_STREAM_TYPE_RFMODEM = 3,
 * @param  destID		-- destination ID of a device.
 * @param  cmd			-- command.
 * @param  pStr			-- points to a command parameter string buffer.
 *
 * @return none
 *
 * History:  Created on 2010-07-30 by Wai Fai Chin
 */

CMD_PRE_PARSER_STATE_T	gSelfCmdPreParserState;

void	cmd_send_self_query_result( BYTE streamType, BYTE destID, BYTE cmd, BYTE *pStr )
{
	BYTE	n;

	if ( stream_router_get_a_new_stream( &gSelfCmdPreParserState.poStream )) {		// if got a new output stream
		// construct the parser state object.
		gSelfCmdPreParserState.poStream-> type	= streamType;
		gSelfCmdPreParserState.poStream-> status = IO_STREAM_STATUS_ACTIVE;		// output dir, active
		gSelfCmdPreParserState.poStream-> sourceID	= gtProductInfoFNV.devID;
		gSelfCmdPreParserState.poStream-> destID	= destID;
		gSelfCmdPreParserState.poStream-> packetLen = 0;
		gSelfCmdPreParserState.streamObjNum = 0;		//TODO: search listener number based on device ID of registered listener.
		gSelfCmdPreParserState.state	= CMD_STATE_LOOK_FOR_PARAMETERS;
		gSelfCmdPreParserState.status	= CMD_STATUS_NO_ERROR ;
		gSelfCmdPreParserState.index	= 0;

		gSelfCmdPreParserState.cmd = cmd;

		n = strlen( pStr );
		if ( n < MAX_STREAM_CIR_BUF_SIZE ) {
			strcpy( gSelfCmdPreParserState.strBuf, pStr);
		}

		cmd_act_execute( &gSelfCmdPreParserState );				// handover the newly constructed parser object to the cmdaction module, a command server..
		stream_router_free_stream( gSelfCmdPreParserState.poStream );	// put resource back to stream pool.
	}
} // end cmd_send_self_query_result()

/**
 * It clears all service counters and service status flags of all loadcells.
 * {3E=password}	password is always required to goto bootloader.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it have valid index else CMD_STATUS_ERROR_INDEX.
 *
 * History:  Created on 2010-09-01 by Wai Fai Chin
 * 2011-05-07 -WFC- Set no error when it has a correct password.
 *
 */

BYTE cmd_clear_service_counters_cmd( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	errorStatus;
	BYTE	lc;
	errorStatus = CMD_STATUS_WRONG_PASSWORD;

	// if the entered password matched the user password OR the default master password then,
	if ( strcmp( gStrPassword, gtProductInfoFNV.password ) == 0 ||
		 strcmp_P( gStrPassword, gcStrMasterPassword ) == 0 ) {
		for ( lc = 0; lc < MAX_NUM_PV_LOADCELL; lc++ ) {
			gaulOverloadedCntFNV[ lc ]	=
			gaulUserLiftCntFNV[ lc ]	=
			gaulLiftCntFNV[ lc ] 		= 0;
			gabServiceStatusFNV[ lc ]	= 0;
			nv_cnfg_fram_save_service_counters( lc );
		}
		errorStatus = CMD_STATUS_NO_ERROR;		// 2011-05-07 -WFC-
	}

	return errorStatus;
} // end cmd_clear_service_counters_cmd

/**
 * It cleared LC_STATUS_HAS_NEW_TOTAL status of a valid loadcell index.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return error status
 * @post	gbCMDTmp[] holds the old unit.
 *
 * History:  Created on 2010-11-10 by Wai Fai Chin
 *
 */

BYTE cmd_lc_total_wt_read_post_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE		status;
	BYTE		lc;

	status = CMD_STATUS_ERROR_INDEX;		// assumed bad.
	lc = pCmdS -> index;					// lc is the loadcell number or sensor ID
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		gaLoadcell[ lc ].status &= ~LC_STATUS_HAS_NEW_TOTAL;			// flagged no more new total weight because it just send out to a remote.
		status = CMD_STATUS_NO_ERROR;
	}
	return status;

} // end cmd_lc_total_wt_read_post_action()


// 2011-03-29 -WFC- v
/**
 * It maps peak hold settings of a loadcell to host command variables so
 * the command engine will answer the query result back to host.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2011-03-29 by Wai Fai Chin
 */

BYTE cmd_cnfg_peakhold_pre_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== loadcell number;
	// gbAcmdTmp1	== opMode;
	// gbAcmdTmp2	== sample speed;

	BYTE	errorStatus;

	errorStatus = CMD_STATUS_NO_ERROR;						// assumed passed.
	if ( gbCMD_Index < MAX_NUM_LOADCELL ) {					// if loadcell number is valid, map peak hold settings to host command variables.
		if ( gaLoadcell[ gbCMD_Index ].runModes & LC_RUN_MODE_PEAK_HOLD_ENABLED )
			gbAcmdTmp1	= 1;
		else
			gbAcmdTmp1	= 0;

		gbAcmdTmp2	= gtSystemFeatureFNV.peakholdSampleSpeed;
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_cnfg_peakhold_pre_action_mapper()

/**
 * It maps host command inputs to peak hold settings.
 * This method maps peak hold settings from host command variables.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it passed otherwise a none zero erro code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2011-03-29 by Wai Fai Chin
 * 2011-07-27 -WFC- Added codes to handle PEAK annunciator for Challenger3 and speed up blink time for DynaLink2.
 * 2011-08-05 -WFC- Changed CPU system clock speed based on Peak Hold mode for DynaLink2.
 */

BYTE cmd_cnfg_peakhold_post_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== loadcell number;
	// gbAcmdTmp1	== opMode;
	// gbAcmdTmp2	== sample speed;

	BYTE					errorStatus;
	LOADCELL_T				*pLc;
	LSENSOR_DESCRIPTOR_T	*pSD;

	errorStatus = CMD_STATUS_NO_ERROR;										// assumed passed.
	if ( gbCMD_Index < MAX_NUM_LOADCELL ) {					// if loadcell number is valid, map peak hold settings to host command variables.
		pLc = &gaLoadcell[ gbCMD_Index ];
		pSD = &gaLSensorDescriptor[gbCMD_Index];
		if ( 2 == gbAcmdTmp1 ) {							// if opMode = clear peak hold, then,
			pLc-> peakHoldWt = 0.0;
			pLc-> status2 &= ~LC_STATUS2_GOT_NEW_PEAK_VALUE;
			pSD-> maxRawADCcount = -2147483648;
			pSD-> status &= ~SENSOR_STATUS_GOT_NEW_ADC_PEAK;		// clear got a new adc peak flag.
		}
		else {
			if ( 1 == gbAcmdTmp1 )  {
				pLc-> runModes |= LC_RUN_MODE_PEAK_HOLD_ENABLED;
				pSD-> maxRawADCcount = -2147483648;						// This will force loadcell to compute a new peak hold value.
				pSD-> status &= ~SENSOR_STATUS_GOT_NEW_ADC_PEAK;		// clear got a new adc peak flag.
				// 2011-07-27 -WFC- v
				#if (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
					#ifdef  LED_DISPLAY_ANC_PEAK
						led_display_set_annunciator( LED_DISPLAY_ANC_PEAK, LED_SEG_STATE_STEADY);
					#endif
				#endif
				#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
					led_display_update_annunciator_blink_timer();						// 2011-07-27 -WFC-
					led_display_set_annunciator_blink_timer_interval( TT_50mSEC );		// 2011-07-27 -WFC-
					if ( SYS_RUN_MODE_NORMAL == gbCmdSysRunMode )
						bios_clock_normal();	// 2011-08-05 -WFC-
				#endif
			}
			else {
				pLc-> runModes &= ~LC_RUN_MODE_PEAK_HOLD_ENABLED;
				#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
					led_display_set_annunciator_blink_timer_interval( TT_150mSEC );		// 2011-07-27 -WFC-
					if ( SYS_RUN_MODE_NORMAL == gbCmdSysRunMode )
						bios_clock_slow();		// 2011-08-05 -WFC-
				#endif
			}
			// 2011-07-27 -WFC- ^
		}
		gtSystemFeatureFNV.peakholdSampleSpeed = gbAcmdTmp2;
		adc_lt_construct_op_desc( pSD, gbCMD_Index );		//construct ADC operation descriptor.
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_cnfg_peakhold_post_action_mapper()

// 2011-03-29 -WFC- ^


/**
 * It validates composite of formatter from the host command.
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if there is error.
 *
 * History:  Created on 2011-07-26 by Wai Fai Chin
 */

BYTE cmd_print_string_cnfg_post_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	i;
	BYTE	errorStatus;
	BYTE	strBuf[12];

	errorStatus = CMD_STATUS_ERROR_INDEX;	// assumed error.
	i  = pCmdS -> index;					// i is the listener number
	if ( i < PRINT_STRING_MAX_NUM_FORMATER ) {
		#if ( CONFIG_INCLUDED_PRINT_STRING_MODULE == TRUE )
			errorStatus = print_string_build_validate_composite(strBuf, gaulPrintStringCompositeFNV[i]);
			if ( PRINT_STRING_BUILD_SEND_STATUS_BAD_COMPOSITE == errorStatus )
				errorStatus = CMD_STATUS_ERROR_ZERO_IN_PRINT_STRING_COMPOSITE;
			else
				errorStatus = CMD_STATUS_NO_ERROR;
		#else
			// errorStatus = CMD_STATUS_ERROR_FEATURE_NOT_SUPPORT;
			errorStatus = CMD_STATUS_NO_ERROR;		// this would not stop host computer to configure profile of this device.
		#endif
	} // end if ( i < MAX_NUM_STREAM_LISTENER )

	return errorStatus;
} // end cmd_print_string_cnfg_post_action()


/**
 * It validates formatter string from the host command.
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if there is error.
 *
 * History:  Created on 2011-07-26 by Wai Fai Chin
 * 2011-10-20 -WFC-, force to marked end of string, fixed a bug that host command {41} reported CMD_STATUS_PRE_TO_MANY_CHAR for a valid formatter.
 */

BYTE cmd_formatter_post_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	i;
	BYTE	errorStatus;

	errorStatus = CMD_STATUS_ERROR_INDEX;	// assumed error.
	i  = pCmdS -> index;					// i is the listener number
	if ( i < PRINT_STRING_MAX_NUM_FORMATER ) {
		#if ( CONFIG_INCLUDED_PRINT_STRING_MODULE == TRUE )
		gabPrintStringUserFormatterFNV[ i ][PRINT_STRING_MAX_FORMATER_LENGTH] = 0;	// 2011-10-20 -WFC-, force to marked end of string.
		errorStatus = print_string_validate_formatter(&gabPrintStringUserFormatterFNV[ i ][0]) ;
		#else
			// errorStatus = CMD_STATUS_ERROR_FEATURE_NOT_SUPPORT;
			errorStatus = CMD_STATUS_NO_ERROR;		// this would not stop host computer to configure profile of this device.
		#endif
	} // end if ( i < MAX_NUM_STREAM_LISTENER )

	return errorStatus;
} // end cmd_formatter_post_action()


/**
 * send print string out to listener.
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return none zero value error code if there is error.
 *
 * History:  Created on 2011-07-26 by Wai Fai Chin
 */

BYTE cmd_print_string_post_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	BYTE	i;
	BYTE	errorStatus;

	errorStatus = CMD_STATUS_ERROR_INDEX;	// assumed error.
	i  = pCmdS -> index;					// i is the listener number
	if ( i < MAX_NUM_STREAM_LISTENER ) {
		#if ( CONFIG_INCLUDED_PRINT_STRING_MODULE == TRUE )
		if ( SYS_RUN_MODE_NORMAL == gbSysRunMode) {
			errorStatus = print_string_setup_for_BnSobj( i );
		} // end if ( SYS_RUN_MODE_NORMAL == gbSysRunMode) {}
		#else
			errorStatus = CMD_STATUS_ERROR_FEATURE_NOT_SUPPORT;
		#endif
	} // end if ( i < MAX_NUM_STREAM_LISTENER )

	return errorStatus;
} // end cmd_print_string_post_action()


/**
 * It toggles HIRES display of a loadcell.
 * This feature only available in DynaLink2 as 2012-02-23.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it passed otherwise a none zero error code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2012-02-23 by Wai Fai Chin
 */

BYTE cmd_toggle_hires_post_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== loadcell number;
	// gbAcmdTmp1	== opMode;
	// gbAcmdTmp2	== sample speed;

	BYTE					errorStatus;
	LOADCELL_T				*pLc;

	#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
	errorStatus = CMD_STATUS_NO_ERROR;						// assumed passed.
	if ( gbCMD_Index < MAX_NUM_LOADCELL ) {					// if loadcell number is valid, map peak hold settings to host command variables.
		pLc = &gaLoadcell[ gbCMD_Index ];
		panel_main_toggle_hires_display( pLc, gbCMD_Index, LED_DISPLAY_ANC_F1 );
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}
	#else
	errorStatus = CMD_STATUS_ERROR_FEATURE_NOT_SUPPORT;
	#endif

	return errorStatus;
} // end cmd_toggle_hires_post_action_mapper()


/**
 * It removes last total of a loadcell.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error if it passed otherwise a none zero error code.
 *
 *  caller should call cmd_act_set_items() before call this.
 *
 * @note: command mapper use gbCMD_Index as an index of 1D array instead of pCmdS -> index.
 *
 * History:  Created on 2012-03-09 by Wai Fai Chin
 */

BYTE cmd_remove_last_total_post_action_mapper( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	// global scratch memory for this module.
	// gbCMD_Index	== loadcell number;
	// gbAcmdTmp1	== opMode;
	// gbAcmdTmp2	== sample speed;

	BYTE					errorStatus;
	LOADCELL_T				*pLc;

	errorStatus = CMD_STATUS_NO_ERROR;						// assumed passed.
	if ( gbCMD_Index < MAX_NUM_LOADCELL ) {					// if loadcell number is valid, map peak hold settings to host command variables.
		pLc = &gaLoadcell[ gbCMD_Index ];
		lc_total_remove_last_total( pLc, gbCMD_Index );
	}
	else {
		errorStatus = CMD_STATUS_ERROR_INDEX;
	}

	return errorStatus;
} // end cmd_remove_last_total_post_action_mapper()

/**
 * It save user settings to none volatile memory.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error always.
 *
 * History:  Created on 2012-04-24 by Wai Fai Chin
 */

BYTE cmd_rf_config_post_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	if ( '=' == pCmdS->strBuf[0]) {
		nv_cnfg_fram_save_rf_device_cnfg();
	}
	return CMD_STATUS_NO_ERROR;
} // end cmd_rf_config_post_action()

/**
 * It save user settings to none volatile memory.
 *
 * @param  pCmdS  -- point to command pre parser state structure.
 *
 * @return 0 error always.
 *
 * History:  Created on 2012-07-09 by Denis Monteiro
 */

BYTE cmd_ethernet_config_post_action( CMD_PRE_PARSER_STATE_T *pCmdS )
{
	if ( '=' == pCmdS->strBuf[0]) {
		nv_cnfg_fram_save_ethernet_device_cnfg();
	}
	return CMD_STATUS_NO_ERROR;
} // end cmd_ethernet_config_post_action()
