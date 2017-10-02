/*! \file main.c \brief main entry point for the application code. */
//****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: main.c
// Hardware: independent
// OS:       independent
// Compiler: any C compiler
// Software layer:  Application
//
// History:  Created on 2007/06/27 by Wai Fai Chin
// History:  Modified on 2011/12/16 by Denis Monteiro
//
///  \par Overview
///     It is the first entry point of application code. It initialize
///  bios and application configuration then run the application code.
//	
//****************************************************************************

/*
  to do:
  added enable software temperature compensation enable command. Done.
  {0c} will auto enable sensore channals. 						 Done.
  {0c} add temperature zone when calibrate.
  software temperature compensation calibaration module,
  software temperature compensation in Loadcell module,
  
*/

#include  "bios.h"
#include  "parser.h"
#include  "cmdaction.h"
#include  "pt.h"
#include  "sensor.h"
//#include  "loadcell.h"
#include  "calibrate.h"
#include  "dataoutputs.h"
#include  "nvmem.h"
#include  "spi.h"
#include  "adc_lt.h"
#include  "adc_cpu.h"
#include  "fer_nvram.h"
// -WFC- 2011-03-11 CANNOT put in here: #include  "panelmain.h"			//	PHJ for selftest startup
// removed because we don't need it in the LED display board. #include  "spi_nvmem.h"
#include  "stream_router.h"
#include  "scalecore_sys.h"
#include  "setpoint.h"
#include  "timer.h"

// #include	<math.h>
#include	<stdio.h>

#include  "hw_inputs_handlers.h"
#include  "hw_outputs_handlers.h"

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )
#include  "panelmain.h"																										 // 2011-12-15 -DLM-
#endif

#include "self_test.h"

#if ( CONFIG_INCLUDED_PRINT_STRING_MODULE == TRUE )		// 2011-10-28 -WFC-
#include "print_string.h"
#endif

#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-17 -WFC-
#include "rf_config.h"
#endif


const char PROGMEM gcMainTestStr[]	= "\n\rScaleCore App code is running.";

/// System Running Mode use by main() to run system tasks.
BYTE	gbSysRunMode;
/// System Power Saving Running State	2014-09-09 -WFC-
BYTE	gbSysPowerSaveRunState;

/// System status 2015-05-07 -WFC-
BYTE	gbSysStatus;

/// power up timer for keep track the scale is in power up state, use this prevent auto total etc... 2015-05-07 -WFC-
TIMER_T 	powerUpTimer;

void	main_system_init( void );
void	main_system_1st_init( void );
void	main_system_normal_init( void );
void	main_system_master_default_system_configuration( void );

//void  main_test_external_key( void); // TESTONLY

// char gStrBuf[100];

/**
 * Application code entry point.
 *
 * @return none
 *
 * History:  Created on 2007/06/27 by Wai Fai Chin
 */

#if ( CONFIG_TEST_MODULE_NEED == FALSE )

int main(void)
{
	
	bios_system_init();
	main_system_init();
//	gbScaleStandardModeNV |= SCALE_STD_MODE_MOTION_DETECT | SCALE_STD_MODE_AZM;
//	cmd_act_set_defaults( 0x12 );	// set default of scale standard and motion detection, AZM and Zero on powerup enable flags.
									//	PHJ	added here to fix motion not working on some units
	ENABLE_GLOBAL_INTERRUPT
	
	// serial0_send_string_P( gcMainTestStr );
	
	/* 2011-03-31 -WFC- v commented it out because it is not a correct way to do it.
	gbTestStartFlag = 1;	
	gbPanelMainSysRunMode = gbPanelMainRunMode =	//	PHJ	v
	gbSysRunMode = SYS_RUN_MODE_SELF_TEST;			// set Challenger3 in self test run mode.
	PT_INIT( &gPanelMainSelfTest_thread_pt );		// PHJ	^	init self test thread.
	2011-03-31 -WFC- ^ */

	gbCmdSysRunMode = gbSysRunMode = SYS_RUN_MODE_SELF_TEST;	// 2011-03-31 -WFC- force it to perform a one shot system wide self test.

	bios_scan_inputs_init();		// -WFC- 2011-03-14
	hw_outputs_handlers_init();		// -WFC- 2011-03-14

	#if ( CONFIG_ETHERNET_MODULE_AS == CONFIG_ETHERNET_MODULE_AS_DIGI ) // 2012-07-05 -DLM-
		if ( ETHERNET_DEVICE_STATUS_ENABLED_bm & gEthernetDeviceSettingsFNV.status  ) {
			BIOS_TURN_ON_ETHERNET;
			//TURN_OFF_RS232_CHIP;
			// BIOS_DISABLED_RS232_RECEIVE;
		}
		else {
			BIOS_TURN_OFF_ETHERNET;
			//TURN_ON_RS232_CHIP;
			// BIOS_ENABLED_RS232_RECEIVE;
		}
	#endif

	#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-17 -WFC-
		rf_config_thread_runner();
		// 2012-06-28 -WFC- v
		if ( gRfDeviceSettingsFNV.status & ( RF_DEVICE_STATUS_QUERY_bm | RF_DEVICE_STATUS_SET_CONFIG_bm )) {
			gRfDeviceSettingsFNV.status &= ~( RF_DEVICE_STATUS_QUERY_bm | RF_DEVICE_STATUS_SET_CONFIG_bm );
			nv_cnfg_fram_save_rf_device_cnfg();
		}
		// 2012-06-28 -WFC- ^
	#endif

	for(;;) {
		if ( !(BIOS_RUN_STATUS_POWER_OFF & gbBiosRunStatus) ) {
			stream_router_all_stream_drivers_receive();
			msi_packet_router_parse_all_stream();					// process all io streams.
			if ( SYS_RUN_MODE_IN_CNFG != gbSysRunMode ) {
				adc_lt_update();				// update Linear Tech ADC configuration based on sensor descriptor and read ADC count of the sensor channel.
				adc_cpu_update( ADC_CPU_INTERVAL_READ );
				sensor_compute_all_values();
				setpoint_process_all();
				data_out_compute_number_bargraph_segment();		// 2011-03-30	-WFC-
				data_out_to_all_listeners();	// send data out to all IO_STREAM objects. Its output behavior depends on IO_STREAM object output configuration.
				//sensor_clear_all_status();
			} // end if ( SYS_RUN_MODE_IN_CNFG != gbSysRunMode ) 

			// -WFC- 2011-03-14 v
			hw_inputs_handlers_clear_inputs_state();
			hw_outputs_handlers();
			// scan switches or binary level inputs.
			hw_inputs_handlers_scan_inputs_tasks();
			// -WFC- 2011-03-14 ^
		}
		self_test_time_critical_sys_behavior_update();		// 2011-05-12 -WFC-
		
		#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_DYNA_LINK_II \
				|| CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI || CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
			panel_main_main_tasks();
		#endif

		#if ( CONFIG_INCLUDED_PRINT_STRING_MODULE == TRUE )		// 2011-06-23 -WFC-
			print_string_main_tasks();
		#endif

		sensor_clear_all_status();
	} // end main for(ever) loop.
	return 0;
} // end main() 

#else	// need to test modules
int main(void)
{
	
	bios_system_init();
	main_system_init();

	#if ( CONFIG_TEST_TIMER_MODULE == TRUE)
		PT_INIT( &gSlowTimeOutThread_pt );
		PT_INIT( &gFastTimeOutThread_pt );
	#endif

	#if ( CONFIG_TEST_SERIAL0_MODULE == TRUE ) 
		PT_INIT( &gSerial0TestThread_pt );
	#endif

	ENABLE_GLOBAL_INTERRUPT
	
	#if (  CONFIG_TEST_PARSER_MODULE == TRUE )
		test_parser_module();
	#endif
	
	#if  ( CONFIG_TEST_STREAM_ROUTER_MODULE == TRUE )
		stream_router_module_test_bench();
	#endif

	#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )  // 2011-12-15 -DLM-
		#if  ( CONFIG_TEST_LED_DISPLAY_MODULE == TRUE)
			PT_INIT( &gLedTest_pt );
		#endif
	#endif


	for(;;) {

    	#if ( CONFIG_TEST_TIMER_MODULE == TRUE ) 
			test_slow_timeout_thread( &gSlowTimeOutThread_pt );
			test_fast_timeout_thread( &gFastTimeOutThread_pt );
		#endif

		#if (CONFIG_TEST_SERIAL0_MODULE == TRUE)
			test_serial0_thread( &gSerial0TestThread_pt );
        #endif

		stream_router_all_stream_drivers_receive();
		msi_packet_router_parse_all_stream();					// process all io streams.

		if ( SYS_RUN_MODE_IN_CNFG != gbSysRunMode ) {
			adc_lt_update();				// update Linear Tech ADC configuration based on sensor descriptor and read ADC count of the sensor channel.
			adc_cpu_update( ADC_CPU_INTERVAL_READ );
			sensor_compute_all_values();
			setpoint_process_all();
			data_out_to_all_listeners();	// send data out to all IO_STREAM objects. Its output behavior depends on IO_STREAM object output configuration.
			sensor_clear_all_status();
		} // end if ( SYS_RUN_MODE_IN_CNFG != gbSysRunMode ) 	
		
		#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
			#if  ( CONFIG_TEST_LED_DISPLAY_MODULE == TRUE)
				test_led_display_manager_thread( &gLedTest_pt );
			#endif
			panel_main_main_tasks();
		#endif
	} // end main for(ever) loop.

	return 0;
} // end main() 

#endif // if ( CONFIG_TEST_MODULE_NEED == FALSE )


/**
 * Initialize the entire system an modules.
 * It recalls settings from nvmem. If fram failed, it will not effect the EEMEM settings.
 * Calibration and other important settings are stored in EEMEM.
 *
 * @return none
 * \sa bios.h
 *
 * History:  Created on 2009/08/06 by Wai Fai Chin
 * 2011-08-15 -WFC- Called loadcell_init_all_loadcells_zero_powerup();
 * 2011-08-18 -WFC- If it requires master reset after downloaded an app code, then performs a master reset.
 *                If previous version system settings in non volatile memory does not match the new version,
 *                then it may cause device to shutdown or not communicate with host computer.
 *                This feature prevent this bad situation from happening.
 * 2011-09-01 -WFC- Treat gAppBootShareNV.post_boot_status as an integer instead of bit flag to guarantee no noise can generated master reset.
 * 2012-04-23 -WFC- default RF device configuration if it none volatile memory failure.
 * 2015-05-07 -WFC- init first power state and its timer.
 */

void  main_system_init( void)
{

	BYTE	status;
	BYTE	bTest;
	BYTE	needDefault;
	
	fram_init();

	// sensor_1st_init();	//to be remove after completed save cnfg codes.
	nvmem_default_all_dac_config_fram();

	// if ( NVRAM_READ_PASS != nv_cnfg_eemem_recall_with_init0_8bitCRC( &gAppBootShareNV, gabEEMEMAppBootShareCRC8, sizeof( APP_BOOT_SHARE_T ) ) ) {

	if (NVRAM_READ_PASS != nv_cnfg_fram_recall_with_8bitCRC( &gAppBootShareNV, APPLICATION_BOOTLOADER_SHARE_FRAM_BASE_ADDR,  sizeof( APP_BOOT_SHARE_T ) ) ) {
		// got here only if both EEPROM and FRAM failed. This could happened in emulation mode.
		gAppBootShareNV.productID = CONFIG_PRODUCT_AS;
		gAppBootShareNV.productVersionID = 1;
		gAppBootShareNV.softVersionStr[0] ='E';
		gAppBootShareNV.softVersionStr[1] ='M';
		gAppBootShareNV.softVersionStr[2] ='U';
		gAppBootShareNV.softVersionStr[3] ='L';
		gAppBootShareNV.softVersionStr[4] = 0;
	}

	// 2011-08-18 -WFC- v
	// 2011-09-01 -WFC- if ( APPBOOT_POST_BOOT_STATUS_NEED_MASTER_RESET_APP & gAppBootShareNV.post_boot_status ) {
	if ( APPBOOT_POST_BOOT_STATUS_NEED_MASTER_RESET_APP == gAppBootShareNV.post_boot_status ) {		// 2011-09-01 -WFC-
		gAppBootShareNV.post_boot_status = 0;				// cleared all status.
		nv_cnfg_fram_save_with_8bitCRC( APPLICATION_BOOTLOADER_SHARE_FRAM_BASE_ADDR, &gAppBootShareNV, sizeof(APP_BOOT_SHARE_T));
		#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-23 -WFC-
			nv_cnfg_fram_default_rf_device_cnfg();
		#endif
		main_system_master_default_system_configuration();
	}
	// 2011-08-18 -WFC- ^

	needDefault = TRUE;

	status = nvmem_recall_all_loadcell_statistics_fram();

	if ( status ) {	// if nvmem recalled failure.
		nvmem_default_all_loadcell_statistics_fram();		// default it to zero.
		nvmem_save_all_loadcell_statistics_fram();			// save defaults to fram in a defined state.
		#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-23 -WFC-
			nv_cnfg_fram_default_rf_device_cnfg();
			nv_cnfg_fram_save_rf_device_cnfg();
		#endif
	}

	status = nvmem_recall_all_essential_config_fram();
	if ( status ) {	// if nvmem recalled failure.
		nvmem_default_all_loadcell_statistics_fram();		// default it to zero.
		nvmem_default_all_dac_config_fram();
		main_system_1st_init();
		#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-23 -WFC-
			nv_cnfg_fram_default_rf_device_cnfg();
		#endif
		nvmem_save_all_essential_config_fram();
	}

	status = nvmem_read_byte_timeout( 0, &bTest);

	if ( status ) { // nvmem is working
		needDefault = nvmem_recall_all_essential_config_eemem();
	}

	if ( needDefault )	{					// need to default the system settings because it fail to recall from nvmem
		main_system_master_default_system_configuration();
	}

	main_system_normal_init();
	self_test_set_event_timers_interval();	// 2011-06-15 -WFC-
	self_test_timer_reset_event_timers();	// reset event timers so it would not trigger an expiration event that would execute LED sleep or power off etc..
	loadcell_init_all_loadcells_zero_powerup();				// for all local physical loadcells, setup zero on power up if enabled. 2011-08-15 -WFC-

	// 2015-05-07 -WFC- v
	gbSysStatus = SYS_STATUS_DURING_POWER_UP;	//system is in power up state.
	timer_mSec_set( &powerUpTimer, TT_2SEC);	// time the first power up,
	// 2015-05-07 -WFC- ^

} // end main_system_init()


/*
void  main_system_init( void)
{

	BYTE	status;
	BYTE	bTest;
	BYTE	needDefault;

	fram_init();

	sensor_1st_init();	//TODO: to be remove after compeleted save cnfg codes.

	if ( NVRAM_READ_PASS != nv_cnfg_eemem_recall_with_8bitCRC( &gAppBootShareNV, gabEEMEMAppBootShareCRC8, sizeof( APP_BOOT_SHARE_T ) ) ) {

		if (NVRAM_READ_PASS != nv_cnfg_fram_recall_with_8bitCRC( &gAppBootShareNV, APPLICATION_BOOTLOADER_SHARE_FRAM_BASE_ADDR,  sizeof( APP_BOOT_SHARE_T ) ) ) {
			// got here only if both EEPROM and FRAM failed. This could happend in emulation mode.
			gAppBootShareNV.productID = CONFIG_PRODUCT_AS;
			gAppBootShareNV.productVersionID = 1;
			gAppBootShareNV.softVersionStr[0] ='E';
			gAppBootShareNV.softVersionStr[1] ='M';
			gAppBootShareNV.softVersionStr[2] ='U';
			gAppBootShareNV.softVersionStr[3] ='L';
			gAppBootShareNV.softVersionStr[4] = 0;
		} 
	}

	needDefault = TRUE;

	status = nvmem_recall_all_loadcell_statistics_fram();
	
	if ( status ) {	// if nvmem recalled failure.
		nvmem_default_all_loadcell_statistics_fram();		// default it to zero.
		nvmem_save_all_loadcell_statistics_fram();			// save defaults to fram in a defined state.
	}
	
	status = nvmem_recall_all_essential_config_fram();
	if ( status ) {	// if nvmem recalled failure.
		nvmem_default_all_loadcell_statistics_fram();		// default it to zero.
		main_system_1st_init();				
		nvmem_save_all_essential_config_fram();
	}
	
	status = nvmem_read_byte_timeout( 0, &bTest);
	
	if ( status ) { // nvmem is working
		needDefault = nvmem_recall_all_essential_config_eemem();
	}

	if ( needDefault )	{					// need to default the system settings because it fail to recall from nvmem
		nvmem_default_all_loadcell_statistics_fram();		// default it to zero.
		main_system_1st_init();				
		nvmem_save_all_config();
	}
		
	main_system_normal_init();
	timer_reset_power_off_timer();					// Set power off timer	PHJ
	
} // end main_system_init()
*/

/**
 * Initialization of the very first time powerup.
 *
 * @return none
 * \sa bios.h
 *
 * History:  Created on 2008/08/01 by Wai Fai Chin
 * 2011-04-21 -WFC- Only default devID, userDefModelCode and modelNam. No defaults for Serial numbers.
 * 2011-06-23 -WFC- call print_string_1st_init_formatters().
 * 2011-07-25 -WFC- Default system features, such as function keys, auto power off, LED intensity, etc...
 */

void  main_system_1st_init( void)
{

	// very first time power up default settings
	// 2011-04-21 -WFC- v Only default devID, userDefModelCode and modelNam. No defaults for Serial numbers.
	// 2011-04-21 -WFC- cmd_act_set_defaults( 0x03 );			// default settings for product and device IDs
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2012-05-02 -DLM-
	gtProductInfoFNV.devID = 0;
	#if CONFIG_4260IS
	gtProductInfoFNV.userDefModelCode = USER_DEF_MODEL_BATTERY_12V_IS;
	#else
	gtProductInfoFNV.userDefModelCode = USER_DEF_MODEL_BATTERY_12V;
	#endif
#else
	gtProductInfoFNV.devID = gtProductInfoFNV.userDefModelCode = 0;
#endif
	gtProductInfoFNV.modelName[0] = 'A';
	gtProductInfoFNV.modelName[1] = 'T';
	gtProductInfoFNV.modelName[2] = 'P';
	gtProductInfoFNV.modelName[3] = 0;
	// 2011-04-21 -WFC- ^
	cmd_act_set_defaults( 0x04 );			// default settings for password settings.
	cmd_act_set_defaults( 0x06 );			// default settings for system feature, such as function keys, auto power off, LED intensity, etc... 2011-07-25 -WFC-

	sensor_1st_init();

	data_out_1st_init();
	adc_lt_1st_init();

	cal_table_all_init();
	setpoint_1st_init();

	#if ( CONFIG_INCLUDED_PRINT_STRING_MODULE == TRUE )		// 2011-06-23 -WFC-
		print_string_1st_init_formatters();
	#endif

} // end main_system_1st_init()


/**
 * Initialization of the normal powerup.
 *
 * @return none
 * \sa bios.h
 *
 * History:  Created on 2008/08/01 by Wai Fai Chin
 * 2011-06-23 -WFC- call print_string_init_formatters().
 * 2012-04-17 -WFC- call rf_device_settings_init();
 * 2012-05-10 -WFC- default serial port0 to 9600 baud for Challenger3.
 * 2015-06-15 -WFC- Called cal_cmd_variables_init() before sensor_init_all() to fix no normal loadcell filter after cycle power bug.
 * 2014-09-12 -WFC- self_test_power_saving_init().
 * 2015-06-15 -WFC- Called cal_cmd_variables_init() before sensor_init_all() to fix no loadcell filter bug.
 */

void  main_system_normal_init( void)
{
	cmd_act_init();
	#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )
		serial0_port_init( SR_BAUD_38400_V );
		serial1_port_init( SR_BAUD_38400_V );
	#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
		serial0_port_init( SR_BAUD_9600_V );
	#else
		serial0_port_init( SR_BAUD_38400_V );
	#endif

	//spi0_master_init();						// TESTONLY, This will call based on the productID and version ID.
	// normal power up.
	cmd_mask_password();
	cal_cmd_variables_init();	// 2015-06-15 -WFC-
	sensor_init_all();
	adc_lt_init();
	adc_cpu_1st_init();
	// 2015-06-15 -WFC- cal_cmd_variables_init();
	cmd_act_set_defaults( 0x08 );			// default hardware test command settings.
	setpoint_init_all();

	adc_cpu_init();				// -WFC- 2013-06-18

	stream_router_init_build_all_iostreams();
	stream_driver_uart_constructor();
	stream_driver_uart1_constructor();
	msi_packet_router_init();
	data_out_init();

	self_test_power_saving_init(); 				//2014-09-12 -WFC-

	#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )
		soft_delay( 100 );		// wait for LED power supply stable before init LED driver chip.
		panel_main_init();
	#endif

	#if ( CONFIG_INCLUDED_PRINT_STRING_MODULE == TRUE )		// 2011-06-23 -WFC-
		print_string_main_init();
	#endif

	#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-17 -WFC-
		rf_device_settings_init();
	#endif

} // end main_system_normal_init()


/**
 * Master default system configuration.
 *
 * @return none
 *
 * History:  Created on 2010/05/20 by Wai Fai Chin
 */

void main_system_master_default_system_configuration( void )
{
	main_system_1st_init();
	nvmem_default_all_loadcell_statistics_fram();		// default it to zero.
	// main_system_1st_init();
	nvmem_save_all_config();
	nvmem_recall_all_essential_config_fram();	// for TESTONLY, to be remove
}

/* *
 * test external keys
 * TESTONLY
 * @return none
 *
 * History:  Created on 2009/02/20 by Wai Fai Chin
 * /

void  main_test_external_key( void)
{
	BYTE p;
	p = PINB;
	p &= 0xF0;	// stripped out none key signal to get the real key.
	if ( p != 0xF0 )					// if a key pressed
		hw3460_led_send_cmd( 8, p );	//
} // end main_test_external_key()

*/
