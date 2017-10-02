/*! \file nvmem.c \brief none volatile memory related functions.*/
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
//  History:  Created on 2007/08/06 by Wai Fai Chin
// 
//   It allocated global none volatile memory variables.
// It saves system configuration like calibration, output mode, etc...
// Sensors cal tables are saved in EEMEM only. The rest of settings are save in FRAM
// because it allow user keeps change and save as often as they want without
// degrade the EEMEM.
//
// ****************************************************************************


/*

C variable name convention:
Each variable should have lower case prefix modifier to describes the data type
of the variable. The first letter of a variable name should be upper case.

Meaning of the prefix letter.

a	=	array
b	=	byte
c	=	Constant
d	=	Double
g	=	Global
f	=	float
i   =   int
l	=	Long
p	=	Pointer
u	=	Unsigned
w	=	Word

Meaning of the suffix letter
NV  == none volatile memory
NVc == none volatile memory customer settings.

NVf == none volatile memory factory settings.

*/

#include  "nvmem.h"

#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )

#include  "fer_nvram.h"
#include <util/crc16.h>

#endif //  ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )


#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )

#if ( CONFIG_EMULATE_FLASHDATA == TRUE )

/**
 * It saves system configuration settings to none volatile memory.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2007/08/22 by Wai Fai Chin
 */
BYTE nvmem_save_all_config( void )
{
	// fram_test_write();		// TESTONLY
	//spi_nvmem_test_write();		// TESTONLY
	return NVRAM_WRITE_PASS;
} // end nvmem_save_all_config()

/**
 * It recalls system configuration settings from none volatile memory.
 *
 * @return none.
 *
 * History:  Created on 2007/08/22 by Wai Fai Chin
 */
BYTE  nvmem_recall_all_config( void )
{

} // end nvmem_recall_all_settings()

#else  // save info to hardware flash data memory.

/**
 * It saves system configuration settings to both eemem and fram none volatile memory.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2009/05/20 by Wai Fai Chin
 */

BYTE nvmem_save_all_config( void )
{
	BYTE status;
	
	status = nvmem_save_all_essential_config_eemem();
	
	status |= nvmem_save_all_essential_config_fram();
	return status;
} // end nvmem_save_all_config()


/**
 * It saves system configuration settings to fram none volatile memory.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2009/05/21 by Wai Fai Chin
 * 2011-07-08 -WFC- added codes to save print_string configuration.
 * 2012-04-20 -WFC- added codes to save RF device configuration settings.
 * 2012-10-25 -WFC- added codes to save loadcell statistics related settings.
 */

BYTE nvmem_save_all_essential_config_fram( void )
{
	BYTE n;
	BYTE status;
	
	status = NVRAM_WRITE_PASS;						// assumed no error.
  
	nv_cnfg_fram_save_with_8bitCRC( PRODUCT_INFO_FRAM_BASE_ADDR, &gtProductInfoFNV, sizeof(PRODUCT_INFO_T) );
	nv_cnfg_fram_save_all_listeners_settings();
	nv_cnfg_fram_save_all_setpoints();
	nv_cnfg_fram_save_with_8bitCRC( SYSTEM_FEATURE_FRAM_BASE_ADDR, &gtSystemFeatureFNV, sizeof(SYSTEM_FEATURE_T) );
	nvmem_save_all_dac_config_fram();
	nvmem_save_all_vs_math_config_fram();
	nvmem_save_all_lc_total_motion_opmode_config_fram();
	nvmem_save_all_lc_standard_mode_azm_config_fram();
	nv_cnfg_fram_save_all_sensor_names();
	nv_cnfg_fram_save_print_string_config();				// 2011-07-08 -WFC-
	nv_cnfg_fram_save_rf_device_cnfg();						// 2012-04-20 -WFC-
	nv_cnfg_fram_save_ethernet_device_cnfg();				// 2012-07-06 -DLM-
	nvmem_save_all_loadcell_statistics_fram();				// 2012-10-25 -WFC-

	for ( n=0; n < MAX_NUM_SENSORS; n++) { 
		nv_cnfg_fram_save_a_sensor_feature( n );
	}
	return status;
} // end nvmem_save_all_essential_config_fram()


/**
 * It recalls essential system configuration settings from fram memory.
 *
 * @return 0 == successes.
 *
 * History:  Created on 2009/08/06 by Wai Fai Chin
 * 2011-07-08 -WFC- added codes to recall print_string configuration.
 * 2012-04-20 -WFC- added codes to recall RF device configuration settings.
 * 2012-10-25 -WFC- added codes to recall loadcell statistics related settings.
 */

BYTE  nvmem_recall_all_essential_config_fram( void )
{
	BYTE n;
	BYTE status;

	status = nv_cnfg_fram_recall_with_8bitCRC( &gtProductInfoFNV, PRODUCT_INFO_FRAM_BASE_ADDR, sizeof(PRODUCT_INFO_T) );
	status |= nv_cnfg_fram_recall_all_listeners_settings();
	status |= nv_cnfg_fram_recall_all_setpoints();
	status |= nv_cnfg_fram_recall_with_8bitCRC( &gtSystemFeatureFNV, SYSTEM_FEATURE_FRAM_BASE_ADDR, sizeof(SYSTEM_FEATURE_T) );
	status |= nvmem_recall_all_dac_config_fram();
	status |= nvmem_recall_all_vs_math_config_fram();
	status |= nvmem_recall_all_lc_total_motion_opmode_config_fram();
	status |= nvmem_recall_all_lc_standard_mode_azm_config_fram();
	status |= nv_cnfg_fram_recall_all_sensor_names();
	status |= nv_cnfg_fram_recall_print_string_config();	// 2011-07-08 -WFC-
	status |= nv_cnfg_fram_recall_rf_device_cnfg();			// 2012-04-20 -WFC-
	status |= nv_cnfg_fram_recall_ethernet_device_cnfg();	// 2012-07-06 -DLM-
	status |= nvmem_recall_all_loadcell_statistics_fram();	// 2012-10-25 -WFC-

	for ( n=0; n < MAX_NUM_SENSORS; n++) { 
		status |= nv_cnfg_fram_recall_a_sensor_feature( n );
	}

	return status;
} // end nvmem_recall_all_essential_config_fram()



/**
 * It saves essential system configuration settings from eemem memory.
 *
 * @return always 0 == successes.
 * @note the only way to see if successful is to recall it.
 *
 * History:  Created on 2009/08/06 by Wai Fai Chin
 */

BYTE  nvmem_save_all_essential_config_eemem( void )
{
	BYTE n;
	
	for ( n=0; n < MAX_NUM_CAL_SENSORS; n++) { 
		sensor_save_cal_table( n );
	}
	
	nv_cnfg_eemem_save_scale_standard();	
	return 0;
} // end nvmem_save_all_essential_config_eemem()


/**
 * It recalls essential system configuration settings from eemem memory.
 *
 * @return 0 == successes.
 *
 * History:  Created on 2009/08/06 by Wai Fai Chin
 */

BYTE  nvmem_recall_all_essential_config_eemem( void )
{
	BYTE n;
	BYTE status;

	for ( n=0; n < MAX_NUM_SENSORS; n++) { 
	//for ( n=0; n < 1; n++) { 					// TESTONLY
		status = sensor_recall_config( n );
		if ( status ) break;					// if error break out the loop.
	}
	status |= nv_cnfg_eemem_recall_scale_standard();
	return status;
} // end nvmem_recall_essential_config_eemem()

/**
 * It recalls system configuration settings from none volatile memory.
 *
 * @return 0 == successes.
 *
 * History:  Created on 2007/02/06 by Wai Fai Chin
 */

BYTE  nvmem_recall_all_config( void )
{
	BYTE status;
	BYTE statusFram;
	BYTE statusEEMem;

	statusFram = nvmem_recall_all_essential_config_fram();
	statusEEMem = nvmem_recall_all_essential_config_eemem();
	if ( statusFram && statusEEMem ) {
		status = NVRAM_NV_MEMORY_FAIL;
	}
	else if ( statusFram )
		status = statusFram;
	else if ( statusEEMem )
		status = statusEEMem;
	else
		status = NVRAM_READ_PASS;

	return status;
} // end nvmem_recall_all_settings()


/**
 * It performs a bit by bit memory check of both 0 and 1 states.
 * The contents of memory are not destroy in the checking process
 * because it put back the original content back after checking.
 *
 * @return 0 if no error else CMD_STATUS_FRAM_FAIL;
 *
 * History:  Created on 2010-05-26 by Wai Fai Chin
 * 2010-12-07 -WFC- True bit by bit memory test.
 */

#define	NVMEM_MAX_NUM_BYTE_CHECK_AT_A_TIME		8
BYTE  nvmem_check_memory( void )
{
	BYTE status;
	UINT16	i;
	UINT16	framAddr;
	BYTE bOriginalBuf[ NVMEM_MAX_NUM_BYTE_CHECK_AT_A_TIME + 2];

	for ( framAddr = 0; framAddr < FRAM_MAX_ADDR_RANGE; framAddr += NVMEM_MAX_NUM_BYTE_CHECK_AT_A_TIME ) {
		fram_read_bytes( bOriginalBuf, framAddr, NVMEM_MAX_NUM_BYTE_CHECK_AT_A_TIME );			// fetched original content

		fram_fill_memory( 0xAA, framAddr, NVMEM_MAX_NUM_BYTE_CHECK_AT_A_TIME );
		status =  fram_compare_memory( 0xAA, framAddr, NVMEM_MAX_NUM_BYTE_CHECK_AT_A_TIME );
		fram_fill_memory( 0x55, framAddr, NVMEM_MAX_NUM_BYTE_CHECK_AT_A_TIME );
		status |=  fram_compare_memory( 0x55, framAddr, NVMEM_MAX_NUM_BYTE_CHECK_AT_A_TIME );

		fram_write_bytes(framAddr, bOriginalBuf, NVMEM_MAX_NUM_BYTE_CHECK_AT_A_TIME );			// put back original content at the same location.
	}
	return status;
} // end nvmem_check_memory()

/*
BYTE  nvmem_check_memory( void )
{
	nvmem_save_all_config();
	return (nvmem_recall_all_config());
} // end nvmem_check_memory()
*/



/**
 * It saves loadcell statistics to fram none volatile memory.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2009/08/06 by Wai Fai Chin
 */

BYTE nvmem_save_all_loadcell_statistics_fram( void )
{
	BYTE n;
	BYTE status;

	status = NVRAM_WRITE_PASS;						// assumed no error.
	for ( n=0; n < MAX_NUM_PV_LOADCELL; n++) {
		nv_cnfg_fram_save_totaling_statistics( n );
		nv_cnfg_fram_save_loadcell_dynamic_data( n );
		nv_cnfg_fram_save_service_counters( n );
	}
	return status;
} // end nvmem_save_all_loadcell_statistics_fram()


/**
 * It recalls loadcell statistics to fram none volatile memory.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2009/08/06 by Wai Fai Chin
 */

BYTE nvmem_recall_all_loadcell_statistics_fram( void )
{
	BYTE n;
	BYTE status;
	
	status = NVRAM_READ_PASS;										// assumed no error.
	for ( n=0; n < MAX_NUM_PV_LOADCELL; n++) {
		status |= nv_cnfg_fram_recall_totaling_statistics( n );
		status |= nv_cnfg_fram_recall_loadcell_dynamic_data( n );
		status |= nv_cnfg_fram_recall_service_counters( n );
		if ( status )	// if error
			break;
	}
	return status;
} // end nvmem_recall_all_loadcell_statistics_fram()


/**
 * It defaults loadcell statistics to fram none volatile memory.
 *
 * @return none
 *
 * History  Created on 2009/08/06 by Wai Fai Chin
 * 2012-10-25 -WFC- default lift and drop threshold of loadcells.
 * 2012-10-29 -WFC- default all flags in a loadcell operation mode, no true capacity unit conversion.
 */

void nvmem_default_all_loadcell_statistics_fram( void )
{
	BYTE n;
	for ( n=0; n < MAX_NUM_PV_LOADCELL; n++) {
		nv_cnfg_fram_default_totaling_statistics( n );
		nv_cnfg_fram_default_loadcell_dynamic_data( n );
		// 2011-04-20 -WFC- nv_cnfg_fram_default_service_counters( n );
		gabLiftThresholdPctCapFNV[ n ] = 5;		// 5% of capacity.  2012-10-25 -WFC-
		gabDropThresholdPctCapFNV[ n ] = 1;		// 1% of capacity.  2012-10-25 -WFC-
		gabLcOpModesFNV[ n ] = 0;				// 2012-10-29 -WFC-

	}
} // end nvmem_default_all_loadcell_statistics_fram()


/**
 * It save a specified loadcell statistics to fram none volatile memory.
 *
 * @param	lc	-- loadcell number.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2010/06/27 by Wai Fai Chin
 */

BYTE nvmem_save_a_loadcell_statistics_fram( BYTE lc )
{
	BYTE status;

	status = NVRAM_WRITE_PASS;						// assumed no error.
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		nv_cnfg_fram_save_totaling_statistics( lc );
		nv_cnfg_fram_save_loadcell_dynamic_data( lc );
		// nv_cnfg_fram_save_service_counters( lc );
	}
	return status;
} // end nvmem_save_all_loadcell_statistics_fram()


/**
 * It recalls a specified loadcell statistics to fram none volatile memory.
 *
 * @param	lc	-- loadcell number.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2010/06/27 by Wai Fai Chin
 */

BYTE nvmem_recall_a_loadcell_statistics_fram( BYTE lc )
{
	BYTE status;

	status = NVRAM_READ_PASS;									// assumed no error.
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		status |= nv_cnfg_fram_recall_totaling_statistics( lc );
		status |= nv_cnfg_fram_recall_loadcell_dynamic_data( lc );
		// status |= nv_cnfg_fram_recall_service_counters( lc );
	}
	return status;
} // end nvmem_recall_a_loadcell_statistics_fram()


/**
 * It saves all channels of DAC configuration to fram none volatile memory.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2010/01/27 by Wai Fai Chin
 */

BYTE nvmem_save_all_dac_config_fram( void )
{
	BYTE n;
	BYTE status;

	status = NVRAM_WRITE_PASS;						// assumed no error.
	for ( n=0; n < MAX_DAC_CHANNEL; n++) {
		nv_cnfg_fram_save_dac_config( n );
	}
	return status;
} // end nvmem_save_all_dac_config_fram()



/**
 * It recalls all channels of DAC configuration from fram none volatile memory.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2010/01/27 by Wai Fai Chin
 */

BYTE nvmem_recall_all_dac_config_fram( void )
{
	BYTE n;
	BYTE status;

	status = NVRAM_READ_PASS;										// assumed no error.
	for ( n=0; n < MAX_DAC_CHANNEL; n++) {
		status |= nv_cnfg_fram_recall_dac_config( n );
		if ( status )	// if error
			break;
	}
	return status;
} // end nvmem_recall_all_dac_config_fram()


/**
 * It defaults loadcell statistics to fram none volatile memory.
 *
 * @return none
 *
 * History  Created on 2010/01/27 by Wai Fai Chin
 */

void nvmem_default_all_dac_config_fram( void )
{
	BYTE n;
	for ( n=0; n < MAX_DAC_CHANNEL; n++) {
		nv_cnfg_fram_default_dac_config( n );
	}
} // end nvmem_default_all_dac_config_fram()


/**
 * It saves all math channels configuration to fram none volatile memory.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2010/03/12 by Wai Fai Chin
 */

BYTE nvmem_save_all_vs_math_config_fram( void )
{
	BYTE n;
	BYTE status;

	status = NVRAM_WRITE_PASS;						// assumed no error.
	for ( n=0; n < MAX_NUM_VS_MATH; n++) {
		nv_cnfg_fram_save_vs_math_config( n );
	}
	return status;
} // end nvmem_save_all_vs_math_config_fram()


/**
 * It recalls all math channels configuration from fram none volatile memory.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2010/03/12 by Wai Fai Chin
 */

BYTE nvmem_recall_all_vs_math_config_fram( void )
{
	BYTE n;
	BYTE status;

	status = NVRAM_READ_PASS;										// assumed no error.
	for ( n=0; n < MAX_NUM_VS_MATH; n++) {
		status |= nv_cnfg_fram_recall_vs_math_config( n );
		if ( status )	// if error
			break;
	}
	return status;
} // end nvmem_recall_all_vs_math_config_fram()

/**
 * Recall loadcell total mode, motion detection and operation mode settings of all loadcells.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2010/06/25 by Wai Fai Chin
 */

BYTE nvmem_recall_all_lc_total_motion_opmode_config_fram( void )
{
	BYTE lc;
	BYTE status;

	status = NVRAM_READ_PASS;										// assumed no error.
	for ( lc=0; lc < MAX_NUM_PV_LOADCELL; lc++) {
		status |= nv_cnfg_fram_recall_lc_total_motion_opmode_config( lc );
		if ( status )	// if error
			break;
	}
	return status;

} // end nvmem_recall_all_lc_total_motion_opmode_config_fram();

/**
 * Save loadcell total mode, motion detection and operation mode settings of all loadcells.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2010/06/25 by Wai Fai Chin
 */

BYTE nvmem_save_all_lc_total_motion_opmode_config_fram( void )
{
	BYTE lc;
	BYTE status;

	status = NVRAM_WRITE_PASS;						// assumed no error.
	for ( lc=0; lc < MAX_NUM_PV_LOADCELL; lc++) {
		nv_cnfg_fram_save_lc_total_motion_opmode_config( lc );
	}
	return status;
} // end nvmem_save_all_lc_total_motion_opmode_config_fram(()

/**
 * Recall loadcell standard mode azm, power up zeroing settings of specified loadcell.
 *
 * @param  lc	-- loadcell number.
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2010/06/25 by Wai Fai Chin
 */

BYTE nvmem_recall_all_lc_standard_mode_azm_config_fram( void )
{
	BYTE lc;
	BYTE status;

	status = NVRAM_READ_PASS;										// assumed no error.
	for ( lc=0; lc < MAX_NUM_PV_LOADCELL; lc++) {
		status |= nv_cnfg_fram_recall_lc_standard_mode_azm_config( lc );
		if ( status )	// if error
			break;
	}
	return status;

} // end nvmem_recall_all_lc_standard_mode_azm_config_fram();

/**
 * Save loadcell total mode, motion detection and operation mode settings of all loadcells.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2010/06/25 by Wai Fai Chin
 */

BYTE nvmem_save_all_lc_standard_mode_azm_config_fram( void )
{
	BYTE lc;
	BYTE status;

	status = NVRAM_WRITE_PASS;						// assumed no error.
	for ( lc=0; lc < MAX_NUM_PV_LOADCELL; lc++) {
		nv_cnfg_fram_save_lc_standard_mode_azm_config( lc );
	}
	return status;
} // end nvmem_save_all_lc_standard_mode_azm_config_fram(()

/**
 * It defaults all sensors viewing countby.
 *
 * @return none
 *
 * History  Created on 2010-09-10 by Wai Fai Chin
 */

void nvmem_default_all_sensor_viewing_cb_fram( void )
{
	BYTE n;
	for ( n=0; n < MAX_NUM_SENSORS; n++) {
		nv_cnfg_fram_default_a_sensor_viewing_cb( n );
	}
} // end nvmem_default_all_sensor_viewing_cb_fram(()


#endif  // #if ( CONFIG_EMULATE_FLASHDATA == TRUE )


#endif // #if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )


///////////////////////////////////////////////////////////////////////////////
//                      Test functions for this module.                      //
///////////////////////////////////////////////////////////////////////////////

#if  ( CONFIG_TEST_NV_MEM_MODULE == TRUE)
#include <stdio.h>                /* prototype declarations for I/O functions */
#include  "serial.h"

#define TEST_NVMEM_BUF_SIZE	70

BYTE gbTestNVMbuf[ TEST_NVMEM_BUF_SIZE + 1 ];

void nvmem_test_eeprom( void )
{
	BYTE i;
	for ( i=0; i< TEST_NVMEM_BUF_SIZE; i++) {
		gbTestNVMbuf[i] = '0' + i;
	}

	nvmem_write_block( 0, gbTestNVMbuf, TEST_NVMEM_BUF_SIZE );

	for ( i=0; i<= TEST_NVMEM_BUF_SIZE; i++) {
		gbTestNVMbuf[i] = 0;
	}

	nvmem_read_block( gbTestNVMbuf, 0, TEST_NVMEM_BUF_SIZE );
	serial0_send_bytes( gbTestNVMbuf, TEST_NVMEM_BUF_SIZE );
}

#endif // ( CONFIG_TEST_NV_MEM_MODULE == TRUE)


