/*! \file nv_cnfg_mem.c \brief nonevolatile configuration settings memory. */
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
//  History:  Created on 2009/02/02 by Wai Fai Chin
// 
//   It allocated physical global nonevolatile memory for configuration settings.
// It is for save system configuration like calibartion, output mode, etc...
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
t	=	tag
u	=	Unsigned
w	=	Word

Meaning of the suffix letter
NV  == nonvolatile memory
NVc == nonvolatile memory customer settings.

NVf == nonvolatile memory factory settings.

FNV == ferroelectric nonvolatile ram.

EEM == electrical errase read only memory.
EEM8c == EEM content appended with an 8 bits CRC
EEM16c == EEM content appended with a 16 bits CRC

*/

#include	"bios.h"
#include	"nvmem.h"
#include	"lc_total.h"
#include	"lc_tare.h"
#include	"lc_zero.h"
#include	"dac_cpu.h"
#include	<util/crc16.h>
#include	"scalecore_sys.h"
#include	<string.h>
#include	"rf_config.h"			// 2012-04-20 -WFC-

/// app boot share rec read from Non Volatile Memory.
APP_BOOT_SHARE_T 	gAppBootShareNV;		// app boot share rec read from Non Volatile Memory, it could be FRAM or EEMEM.

///	Product ID and version ID are use for configure hardware settings such as which loadcell attached to which ADC inputs etc...
PRODUCT_INFO_T		gtProductInfoFNV;

/// systems feature which contains user key function, auto off, etc...
SYSTEM_FEATURE_T	gtSystemFeatureFNV;

// 2011-07-25 -WFC- v moved from print_string.c file:
/// print string control mode
BYTE	gabPrintStringCtrlModeFNV[ MAX_NUM_STREAM_LISTENER ];

/// print string composite for each listener as defined in command {1A}.
UINT32	gaulPrintStringCompositeFNV[ MAX_NUM_STREAM_LISTENER ];

/// print string interval in seconds for each listener as defined in command {1A}.
UINT16	gawPrintStringIntervalFNV[ MAX_NUM_STREAM_LISTENER ];

/// User defined print string formatter
BYTE	gabPrintStringUserFormatterFNV[ PRINT_STRING_MAX_NUM_FORMATER ][ PRINT_STRING_MAX_FORMATER_LENGTH + 1 ];

RF_DEVICE_SETTINGS_T 	gRfDeviceSettingsFNV;		// 2012-04-24 -WFC-

ETHERNET_DEVICE_SETTINGS_T 	gEthernetDeviceSettingsFNV;		// 2012-07-06 -DLM-

const char gcStrDefPrintFormatter0[]		PROGMEM = "R7S0T7V_U_Mrn";		// Right justify 7 chars, Sensor0 current mode weight, space, unit, space, Weight Mode string, carriage return, line feed.

// 2011-07-25 -WFC- ^


// app boot share in EEMEM appended with CRC8.
//BYTE	EEMEM	gabEEMEMAppBootShareCRC8[ sizeof(APP_BOOT_SHARE_T) + 1];
//APP_BOOT_SHARE_T	EEMEM	gEEMEMAppBootShare;		// app boot share in EEMEM.

#if ( CONFIG_USE_INTERNAL_EEPROM == TRUE )
// ****************************************************************************
// SENSOR_CAL_T EEMEM	gaSensorCalEEM[ CAL_MAX_NUM_CAL_TABLE ];
/// from calibrate.c file:	SENSOR_CAL_T gaSensorCalNV[ CAL_MAX_NUM_CAL_TABLE ]; each caltable appends a 16bit CRC.
BYTE EEMEM	gaSensorCalEEM16c[ CAL_MAX_NUM_CAL_TABLE * EEMEM_SENSOR_CALTABLE_WITH_CRC_SIZE ];

#define MAX_SCALE_STANDARD_EEMEM_SIZE		1
BYTE EEMEM	gabScaleStandardModeEEMEM[ MAX_SCALE_STANDARD_EEMEM_SIZE + 1 ];		// include 8bit CRC.
#endif

/*
// ****************************************************************************
///  from loadcell.c  user specified zero offset weight, float	gafLcZeroOffsetWtNV[ MAX_NUM_LOADCELL];
float	EEMEM	gafLcZeroOffsetWtEEM[ MAX_NUM_PV_LOADCELL];


// ****************************************************************************
// from lc_tare.c has no EEMEM value settings.


// ****************************************************************************
/// from lc_total.c file:
															/// threshold in percentage of capacity to drop below before total allowed 
BYTE	EEMEM	gabTotalDropThresholdPctCapEEM[ MAX_NUM_PV_LOADCELL ];
															/// threshold in percentage of capacity to rise above before total allowed 
BYTE	EEMEM	gabTotalRiseThresholdPctCapEEM[ MAX_NUM_PV_LOADCELL ];

														/// upper bound weight of on accept total mode.
float	EEMEM	gafTotalOnAcceptUpperWtEEM[ MAX_NUM_PV_LOADCELL];
														/// lower bound weight of on accept total mode.
float	EEMEM	gafTotalOnAcceptLowerWtEEM[ MAX_NUM_PV_LOADCELL];

														/// minimum stable time before it can be total.
BYTE	EEMEM	gabTotalMinStableTimeEEM[ MAX_NUM_PV_LOADCELL];

// ****************************************************************************
/// from lc_zero.c file:
/// azm interval time
BYTE	EEMEM	gab_STD_AZM_IntervalTimeEEM[ MAX_NUM_PV_LOADCELL];
BYTE	EEMEM	gab_NTEP_AZM_IntervalTimeEEM[ MAX_NUM_PV_LOADCELL];
BYTE	EEMEM	gab_OIML_AZM_IntervalTimeEEM[ MAX_NUM_PV_LOADCELL];

/// Auto zero maintenance countby range band. It will use to compute azmThresholdWt.
BYTE	EEMEM	gab_STD_AZM_CBRangeEEM[ MAX_NUM_PV_LOADCELL];		// Auto zero maintenance countby range band.
BYTE	EEMEM	gab_NTEP_AZM_CBRangeEEM[ MAX_NUM_PV_LOADCELL];		// Auto zero maintenance countby range band.
BYTE	EEMEM	gab_OIML_AZM_CBRangeEEM[ MAX_NUM_PV_LOADCELL];		// Auto zero maintenance countby range band.

/// Percent of capacity above cal zero that can be zeroed off if below this limit, 1 = 1%, for compute zeroBandHiWt zero band high limit weight;
BYTE	EEMEM	gab_STD_pcentCapZeroBandHiEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off, 1 = 1%
BYTE	EEMEM	gab_NTEP_pcentCapZeroBandHiEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off, 1 = 1%
BYTE	EEMEM	gab_OIML_pcentCapZeroBandHiEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off, 1 = 1%

/// Percent of capacity below cal zero that can be zeroed off if above this limit, 1 = 1%, for compute zeroBandLoWt zero band low limit weight
BYTE	EEMEM	gab_STD_pcentCapZeroBandLoEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off, 1 = 1%
BYTE	EEMEM	gab_NTEP_pcentCapZeroBandLoEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off, 1 = 1%
BYTE	EEMEM	gab_OIML_pcentCapZeroBandLoEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off, 1 = 1%

/// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.
BYTE	EEMEM	gab_STD_pwupZeroBandHiEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.
BYTE	EEMEM	gab_NTEP_pwupZeroBandHiEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.
BYTE	EEMEM	gab_OIML_pwupZeroBandHiEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.

/// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.
BYTE	EEMEM	gab_STD_pwupZeroBandLoEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.
BYTE	EEMEM	gab_NTEP_pwupZeroBandLoEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.
BYTE	EEMEM	gab_OIML_pwupZeroBandLoEEM[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.

*/

#if ( CONFIG_USE_INTERNAL_EEPROM == TRUE )

/**
 * It saves data into EEMEM with 8bit CRC appended to it.
 * The CRC is MAXIM iButton convention init CRC with 0. It is flaw because 0 crc on array of 0 is still 0.
 * This is use for backward compatible with old bootloader, otherwise I will init crc to 0xFF.
 *
 * @param  pDest	-- destination address of EEMEM ( internal memory address of EEMEM).
 * @param  pSrc		-- pointer to RAM source buffer.
 * @param  nSize	-- number of bytes to be written. nSize should be < 32 bytes because crc8 can only guaranty 255 bits of data integrity.
 * @note			   Caller must supply a buffer size nSize + 1 byte for appended crc8.
 *
 * @return 0 == passed, none zero == failed.
 *
 * @note  CRC Polynomial: x^8 + x^5 + x^4 + 1; (0x8C)
 *		  Initial value: 0x0
 *        Reflected version algorithm
 *
 * History  Created on 2009-05-13 by Wai Fai Chin
 * 2010-12-15 -WFC- rename to nv_cnfg_eemem_save_with_init0_8bitCRC().
 */

BYTE nv_cnfg_eemem_save_with_init0_8bitCRC( void *pDest, const void *pSrc, BYTE nSize )
{
	BYTE i;
	BYTE crc;
	BYTE *pbSrc;

	pbSrc = pSrc;
	crc = 0;
	for (i = 0; i <  nSize; i++)
		crc = _crc_ibutton_update(crc, pbSrc[i]);

	DISABLE_GLOBAL_INTERRUPT		
	nvmem_write_block(pDest, pSrc, nSize);
	nvmem_write_byte( pDest + nSize, crc);
	ENABLE_GLOBAL_INTERRUPT
	
	return NVRAM_WRITE_PASS;
			
} // end nv_cnfg_eemem_save_with_init0_8bitCRC(,,)

/**
 * It recalls data from EEMEM with 8bit CRC integrity check.
 * The CRC is MAXIM iButton convention init CRC with 0. It is flaw because 0 crc on array of 0 is still 0.
 * This is use for backward compatible with old bootloader, otherwise I will init crc to 0xFF.
 *
 * @param  pDest	-- destination address of RAM
 * @param  pSrc		-- pointer to EEMEM source buffer.
 * @param  nSize	-- number of bytes to be written. nSize should be < 32 bytes because crc8 can only guaranty 255 bits of data integrity.
 *
 * @return 0 == passed, NVRAM_EEMEM_FAIL == failed.
 * @post 
 * History  Created on 2009-05-13 by Wai Fai Chin
 * 2010-12-15 -WFC- rename to nv_cnfg_eemem_recall_with_init0_8bitCRC().
 */

BYTE nv_cnfg_eemem_recall_with_init0_8bitCRC( void *pDest, const void *pSrc, BYTE nSize )
{
	BYTE i;
	BYTE crc, crcRead;
	BYTE *pbDest;

	nvmem_read_block( pDest, pSrc, nSize);

	crcRead = nvmem_read_byte( pSrc + nSize );

	pbDest = pDest;
	crc = 0;
	for (i = 0; i <  nSize; i++)
		crc = _crc_ibutton_update(crc, pbDest[i]);

	crcRead -=crc;
	if ( crcRead )
		crcRead = NVRAM_EEMEM_FAIL;

	//gwSysStatus = (UINT16) crcRead;
	return crcRead;
} // end nv_cnfg_eemem_recall_with_init0_8bitCRC(,,)



/**
 * It saves data into EEMEM with 8bit CRC appended to it.
 * The CRC is MAXIM iButton convention init CRC with 0. It is flaw because 0 crc on array of 0 is still 0.
 * Thus, I have to init CRC with 0xFF instead of 0 even though I use the iButton library function.
 *
 * @param  pDest	-- destination address of EEMEM ( internal memory address of EEMEM).
 * @param  pSrc		-- pointer to RAM source buffer.
 * @param  nSize	-- number of bytes to be written. nSize should be < 32 bytes because crc8 can only guaranty 255 bits of data integrity.
 * @note			   Caller must supply a buffer size nSize + 1 byte for appended crc8.
 *
 * @return 0 == passed, none zero == failed.
 *
 * @note  CRC Polynomial: x^8 + x^5 + x^4 + 1; (0x8C)
 *		  Initial value: 0x0
 *        Reflected version algorithm
 *
 * History  Created on 2009-05-13 by Wai Fai Chin
 * 2010-12-15 -WFC- init crc with 0xFF instead of 0, because MAXIM iButton convention is flaw, 0 crc on array of 0 is still 0.
 */

BYTE nv_cnfg_eemem_save_with_8bitCRC( void *pDest, const void *pSrc, BYTE nSize )
{
	BYTE i;
	BYTE crc;
	BYTE *pbSrc;

	pbSrc = pSrc;
	crc = 0xFF;
	for (i = 0; i <  nSize; i++)
		crc = _crc_ibutton_update(crc, pbSrc[i]);

	DISABLE_GLOBAL_INTERRUPT
	nvmem_write_block(pDest, pSrc, nSize);
	nvmem_write_byte( pDest + nSize, crc);
	ENABLE_GLOBAL_INTERRUPT

	return NVRAM_WRITE_PASS;

} // end nv_cnfg_eemem_save_with_8bitCRC(,,)

/**
 * It recalls data from EEMEM with 8bit CRC integrity check.
 * The CRC is MAXIM iButton convention init CRC with 0. It is flaw because 0 crc on array of 0 is still 0.
 * Thus, I have to init CRC with 0xFF instead of 0 even though I use the iButton library function.
 *
 * @param  pDest	-- destination address of RAM
 * @param  pSrc		-- pointer to EEMEM source buffer.
 * @param  nSize	-- number of bytes to be written. nSize should be < 32 bytes because crc8 can only guaranty 255 bits of data integrity.
 *
 * @return 0 == passed, NVRAM_EEMEM_FAIL == failed.
 * @post
 * History  Created on 2009-05-13 by Wai Fai Chin
 * 2010-12-15 -WFC- init crc with 0xFF instead of 0, because MAXIM iButton convention is flaw, 0 crc on array of 0 is still 0.
 */

BYTE nv_cnfg_eemem_recall_with_8bitCRC( void *pDest, const void *pSrc, BYTE nSize )
{
	BYTE i;
	BYTE crc, crcRead;
	BYTE *pbDest;

	nvmem_read_block( pDest, pSrc, nSize);

	crcRead = nvmem_read_byte( pSrc + nSize );

	pbDest = pDest;
	crc = 0xFF;
	for (i = 0; i <  nSize; i++)
		crc = _crc_ibutton_update(crc, pbDest[i]);

	crcRead -=crc;
	if ( crcRead )
		crcRead = NVRAM_EEMEM_FAIL;

	//gwSysStatus = (UINT16) crcRead;
	return crcRead;
} // end nv_cnfg_eemem_recall_with_8bitCRC(,,)


/**
 * It saves data into EEMEM with 16bit CRC appended to it.
 *
 * @param  pDest	-- destination address of EEMEM ( internal memory address of EEMEM).
 * @param  pSrc		-- pointer to RAM source buffer.
 * @param  nSize	-- number of bytes to be written. 
 *
 * @return 0 == passed, none zero == failed.
 *
 * @note  Polynomial: x^16 + x^12 + x^5 + 1 (0x1021)
 *		  Initial value: 0x0
 *        None Reflected version algorithm
 *
 * History  Created on 2009/05/14 by Wai Fai Chin
 */

BYTE nv_cnfg_eemem_save_with_16bitCRC( void *pDest, const void *pSrc, UINT16 nSize )
{
	UINT16	i;
	BYTE	*pbSrc;
	union {
		UINT16	crc;
		BYTE	bA[2];
	}u16;
	

	pbSrc = pSrc;
	u16.crc = 0xFFFF;
	for (i = 0; i <  nSize; i++)
		u16.crc = _crc_xmodem_update( u16.crc, pbSrc[i]);

	DISABLE_GLOBAL_INTERRUPT		
	nvmem_write_block(pDest, pSrc, nSize);
	nvmem_write_block( pDest + nSize, u16.bA, 2);
	ENABLE_GLOBAL_INTERRUPT
	
	return NVRAM_WRITE_PASS;
			
} // end nv_cnfg_eemem_save_with_16bitCRC(,,)


/**
 * It recalls data from EEMEM with 16bit CRC integrity check.
 *
 * @param  pDest	-- destination address of RAM
 * @param  pSrc		-- pointer to EEMEM source buffer.
 * @param  nSize	-- number of bytes to be written.
 *
 * @return 0 == passed, NVRAM_EEMEM_FAIL == failed.
 *
 * History  Created on 2009/05/13 by Wai Fai Chin
 */

BYTE nv_cnfg_eemem_recall_with_16bitCRC( void *pDest, const void *pSrc, UINT16 nSize )
{
	UINT16	i;
	UINT16	crc;
	BYTE	*pbDest;
	union {
		UINT16	crc;
		BYTE	bA[2];
	}u16;

	nvmem_read_block( pDest, pSrc, nSize);

	nvmem_read_block( u16.bA, pSrc + nSize, 2 );

	pbDest = pDest;
	crc = 0xFFFF;
	for (i = 0; i <  nSize; i++)
		crc = _crc_xmodem_update(crc, pbDest[i]);

	crc -= u16.crc;
	if ( crc )
		crc = NVRAM_EEMEM_FAIL;
	//gwSysStatus = crc;
	return crc;
} // end nv_cnfg_eemem_recall_with_16bitCRC(,,)

#endif


/**
 * Save a specified cal table.
 *
 * @param  calTableNum	-- cal table number.
 * @return 0 == passed, none zero == failed.
 *
 *
 * History:  Created on 2009/05/15 by Wai Fai Chin
 */

#if ( CONFIG_USE_INTERNAL_EEPROM == TRUE )
BYTE  nv_cnfg_eemem_save_a_cal_table( BYTE calTableNum )
{
	BYTE status;
	status = NVRAM_EEMEM_FAIL;
	if ( calTableNum < CAL_MAX_NUM_CAL_TABLE ) {
		status = nv_cnfg_eemem_save_with_16bitCRC( gaSensorCalEEM16c + calTableNum * EEMEM_SENSOR_CALTABLE_WITH_CRC_SIZE , &gaSensorCalNV[ calTableNum], sizeof(SENSOR_CAL_T) );
	}
	return status;
}
#else
BYTE  nv_cnfg_eemem_save_a_cal_table( BYTE calTableNum )
{
	BYTE status;
	status = NVRAM_EEMEM_FAIL;
	if ( calTableNum < CAL_MAX_NUM_CAL_TABLE ) {
		status = nv_cnfg_fram_save_with_16bitCRC( SENSOR_CAL_TABLES_FRAM_BASE_ADDR + calTableNum * FRAM_SENSOR_CALTABLE_WITH_CRC_SIZE, &gaSensorCalNV[ calTableNum], sizeof(SENSOR_CAL_T) );
	}
	return status;
}
#endif

/**
 * Recall a specified cal table.
 *
 * @param  calTableNum	-- cal table number.
 * @return 0 == passed, none zero == failed.
 *
 *
 * History:  Created on 2009/05/15 by Wai Fai Chin
 */

#if ( CONFIG_USE_INTERNAL_EEPROM == TRUE )
BYTE  nv_cnfg_eemem_recall_a_cal_table( BYTE calTableNum )
{
	BYTE status;
	status = NVRAM_EEMEM_FAIL;
	if ( calTableNum < CAL_MAX_NUM_CAL_TABLE ) {
		status = nv_cnfg_eemem_recall_with_16bitCRC( &gaSensorCalNV[ calTableNum ], gaSensorCalEEM16c + calTableNum * EEMEM_SENSOR_CALTABLE_WITH_CRC_SIZE, sizeof(SENSOR_CAL_T) );
		if ( gaSensorCalNV[ calTableNum ].status < CAL_STATUS_COMPLETED )		// if cal status is NOT completed or uncal,
			gaSensorCalNV[ calTableNum ].status = CAL_STATUS_UNCAL;			// then we must set it to UNCAL state so you can calibrate it again.
	}
	return status;
}
#else
BYTE  nv_cnfg_eemem_recall_a_cal_table( BYTE calTableNum )
{
	BYTE status;
	status = NVRAM_EEMEM_FAIL;
	if ( calTableNum < CAL_MAX_NUM_CAL_TABLE ) {
		status = nv_cnfg_fram_recall_with_16bitCRC( &gaSensorCalNV[ calTableNum ], SENSOR_CAL_TABLES_FRAM_BASE_ADDR + calTableNum * FRAM_SENSOR_CALTABLE_WITH_CRC_SIZE, sizeof(SENSOR_CAL_T) );
		if ( gaSensorCalNV[ calTableNum ].status < CAL_STATUS_COMPLETED )		// if cal status is NOT completed or uncal, 
			gaSensorCalNV[ calTableNum ].status = CAL_STATUS_UNCAL;			// then we must set it to UNCAL state so you can calibrate it again.
	}
	return status;
}
#endif

/**
 * Save the loadcell mode such as industry standard, AZM etc...
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2009/10/08 by Wai Fai Chin
 */
#if ( CONFIG_USE_INTERNAL_EEPROM == TRUE )
BYTE  nv_cnfg_eemem_save_scale_standard( void )
{
	BYTE status;
	status = NVRAM_EEMEM_FAIL;
	status = nv_cnfg_eemem_save_with_8bitCRC( gabScaleStandardModeEEMEM, &gbScaleStandardModeNV, 1 );
	return status;
} // end nv_cnfg_eemem_save_scale_standard()
#else
BYTE  nv_cnfg_eemem_save_scale_standard( void )
{
	BYTE status;
	status = NVRAM_EEMEM_FAIL;
	status = nv_cnfg_fram_save_with_8bitCRC( SCALE_STANDARD_MODE_FRAM_BASE_ADDR, &gbScaleStandardModeNV, 1 );
	return status;
} // end nv_cnfg_eemem_save_scale_standard()
#endif

/**
 * Save the loadcell mode such as industry standard, AZM etc...
 *
 * @return 0 == passed, none zero == failed.
 *
 *
 * History:  Created on 2009/10/08 by Wai Fai Chin
 */

#if ( CONFIG_USE_INTERNAL_EEPROM == TRUE )
BYTE  nv_cnfg_eemem_recall_scale_standard( void )
{
	BYTE status;
	status = NVRAM_EEMEM_FAIL;
	status = nv_cnfg_eemem_recall_with_8bitCRC( &gbScaleStandardModeNV, gabScaleStandardModeEEMEM, 1 );
	return status;
} // end nv_cnfg_eemem_recall_scale_standard
#else
BYTE  nv_cnfg_eemem_recall_scale_standard( void )
{
	BYTE status;
	status = NVRAM_EEMEM_FAIL;
	status = nv_cnfg_fram_recall_with_8bitCRC( &gbScaleStandardModeNV, SCALE_STANDARD_MODE_FRAM_BASE_ADDR, 1 );
	return status;
} // end nv_cnfg_eemem_recall_scale_standard
#endif


/**
 * It saves data into ferroelectric non volatile ram with 8bit CRC appended to it.
 * The CRC is MAXIM iButton convention init CRC with 0. It is flaw because 0 crc on array of 0 is still 0.
 * Thus, I have to init CRC with 0xFF instead of 0 even though I use the iButton library function.
 *
 * @param  destAddr	-- destination address of fram ( internal memory address of fram).
 * @param  pSrc		-- pointer to RAM source buffer.
 * @param  nSize	-- number of bytes to be written. nSize should be < 32 bytes because crc8 can only guaranty 255 bits of data integrity.
 *
 * @return 0 == passed, none zero == failed.
 *
 * @note  CRC Polynomial: x^8 + x^5 + x^4 + 1; (0x8C)
 *		  Initial value: 0x0
 *        Reflected version algorithm
 *
 * History  Created on 2009-05-15 by Wai Fai Chin
 * 2010-12-15 -WFC- init crc with 0xFF instead of 0, because MAXIM iButton convention is flaw, 0 crc on array of 0 is still 0.
 */

BYTE nv_cnfg_fram_save_with_8bitCRC( UINT16 destAddr, const void *pSrc, BYTE nSize )
{
	BYTE i;
	BYTE crc;
	BYTE *pbSrc;

	pbSrc = pSrc;
	crc = 0xFF;
	for (i = 0; i <  nSize; i++)
		crc = _crc_ibutton_update(crc, pbSrc[i]);

	fram_write_bytes( destAddr, pSrc, nSize);
	fram_write_bytes( destAddr + nSize, &crc, 1);
	
	return NVRAM_WRITE_PASS;
			
} // end nv_cnfg_fram_save_with_8bitCRC(,,)


/**
 * It recalls data from FRAM with 8bit CRC integrity check.
 * The CRC is MAXIM iButton convention init CRC with 0. It is flaw because 0 crc on array of 0 is still 0.
 * Thus, I have to init CRC with 0xFF instead of 0 even though I use the iButton library function.
 *
 * @param  pDest		-- destination address of RAM
 * @param  sourceAddr	-- pointer to EEMEM source buffer.
 * @param  nSize		-- number of bytes to be written. nSize should be < 32 bytes because crc8 can only guaranty 255 bits of data integrity.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2009-05-15 by Wai Fai Chin
 * 2010-12-15 -WFC- init crc with 0xFF instead of 0, because MAXIM iButton convention is flaw, 0 crc on array of 0 is still 0.
 */

BYTE nv_cnfg_fram_recall_with_8bitCRC( void *pDest, UINT16 sourceAddr, BYTE nSize )
{
	BYTE i;
	BYTE crc, crcRead;
	BYTE *pbDest;

	fram_read_bytes( pDest, sourceAddr, (UINT16) nSize );

	fram_read_bytes( &crcRead, sourceAddr + nSize, 1 );

	pbDest = pDest;
	crc = 0xFF;
	for (i = 0; i <  nSize; i++)
		crc = _crc_ibutton_update(crc, pbDest[i]);

	crcRead -=crc;
	if ( crcRead )
		crcRead = NVRAM_FRAM_FAIL;
	// gwSysStatus = (UINT16) crcRead;
	return crcRead;
} // end nv_cnfg_fram_recall_with_8bitCRC(,,)


/**
 * It saves data into ferroelectric non volatile ram with 8bit CRC appended to it.
 * The CRC is MAXIM iButton convention init CRC with 0. It is flaw because 0 crc on array of 0 is still 0.
 * This is use for backward compatible with old bootloader, otherwise I will init crc to 0xFF.
 *
 * @param  destAddr	-- destination address of fram ( internal memory address of fram).
 * @param  pSrc		-- pointer to RAM source buffer.
 * @param  nSize	-- number of bytes to be written. nSize should be < 32 bytes because crc8 can only guaranty 255 bits of data integrity.
 *
 * @return 0 == passed, none zero == failed.
 *
 * @note  CRC Polynomial: x^8 + x^5 + x^4 + 1; (0x8C)
 *		  Initial value: 0x0
 *        Reflected version algorithm
 *
 * History  Created on 2010-12-15 by Wai Fai Chin
 * /

BYTE nv_cnfg_fram_save_with_init0_8bitCRC( UINT16 destAddr, const void *pSrc, BYTE nSize )
{
	BYTE i;
	BYTE crc;
	BYTE *pbSrc;

	pbSrc = pSrc;
	crc = 0;
	for (i = 0; i <  nSize; i++)
		crc = _crc_ibutton_update(crc, pbSrc[i]);

	fram_write_bytes( destAddr, pSrc, nSize);
	fram_write_bytes( destAddr + nSize, &crc, 1);

	return NVRAM_WRITE_PASS;

} // end nv_cnfg_fram_save_with_init0_8bitCRC(,,)


/**
 * It recalls data from FRAM with 8bit CRC integrity check.
 * The CRC is MAXIM iButton convention init CRC with 0. It is flaw because 0 crc on array of 0 is still 0.
 * This is use for backward compatible with old bootloader, otherwise I will init crc to 0xFF.
 *
 * @param  pDest		-- destination address of RAM
 * @param  sourceAddr	-- pointer to EEMEM source buffer.
 * @param  nSize		-- number of bytes to be written. nSize should be < 32 bytes because crc8 can only guaranty 255 bits of data integrity.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History  Created on 2010-12-15 by Wai Fai Chin
 * /

BYTE nv_cnfg_fram_recall_with_init0_8bitCRC( void *pDest, UINT16 sourceAddr, BYTE nSize )
{
	BYTE i;
	BYTE crc, crcRead;
	BYTE *pbDest;

	fram_read_bytes( pDest, sourceAddr, (UINT16) nSize );

	fram_read_bytes( &crcRead, sourceAddr + nSize, 1 );

	pbDest = pDest;
	crc = 0;
	for (i = 0; i <  nSize; i++)
		crc = _crc_ibutton_update(crc, pbDest[i]);

	crcRead -=crc;
	if ( crcRead )
		crcRead = NVRAM_FRAM_FAIL;
	// gwSysStatus = (UINT16) crcRead;
	return crcRead;
} // end nv_cnfg_fram_recall_with_init0_8bitCRC(,,)
*/

/**
 * It saves data into FRAM with 16bit CRC appended to it.
 *
 * @param  pDest	-- destination address of fram ( internal memory address of fram).
 * @param  pSrc		-- pointer to RAM source buffer.
 * @param  nSize	-- number of bytes to be written.
 *
 * @return 0 == passed, none zero == failed.
 *
 * @note  Polynomial: x^16 + x^12 + x^5 + 1 (0x1021)
 *		  Initial value: 0x0
 *        None Reflected version algorithm
 *
 * History  Created on 2010-06-27 by Wai Fai Chin
 * 2010-12-15 -WFC- init crc with 0xFFFF instead of 0, because XMODEM convention is flaw, 0 crc on array of 0 is still 0.
 */

BYTE nv_cnfg_fram_save_with_16bitCRC( void *pDest, const void *pSrc, UINT16 nSize )
{
	UINT16	i;
	BYTE	*pbSrc;
	union {
		UINT16	crc;
		BYTE	bA[2];
	}u16;


	pbSrc = pSrc;
	u16.crc = 0xFFFF;
	for (i = 0; i <  nSize; i++)
		u16.crc = _crc_xmodem_update( u16.crc, pbSrc[i]);

	fram_write_bytes(pDest, pSrc, nSize);
	fram_write_bytes( pDest + nSize, u16.bA, 2);
	return NVRAM_WRITE_PASS;

} // end nv_cnfg_fram_save_with_16bitCRC(,,)


/**
 * It recalls data from fram with 16bit CRC integrity check.
 *
 * @param  pDest	-- destination address of RAM
 * @param  pSrc		-- pointer to FRAM source buffer.
 * @param  nSize	-- number of bytes to be written.
 *
 * @return 0 == passed, NVRAM_FRAM_FAIL == failed.
 *
 * History  Created on 2010-06-27 by Wai Fai Chin
 * 2010-12-15 -WFC- init crc with 0xFFFF instead of 0, because XMODEM convention is flaw, 0 crc on array of 0 is still 0.
 */

BYTE nv_cnfg_fram_recall_with_16bitCRC( void *pDest, const void *pSrc, UINT16 nSize )
{
	UINT16	i;
	UINT16	crc;
	BYTE	*pbDest;
	union {
		UINT16	crc;
		BYTE	bA[2];
	}u16;

	fram_read_bytes( pDest, pSrc, nSize);
	fram_read_bytes( u16.bA, pSrc + nSize, 2 );

	pbDest = pDest;
	crc = 0xFFFF;
	for (i = 0; i <  nSize; i++)
		crc = _crc_xmodem_update(crc, pbDest[i]);

	crc -= u16.crc;
	if ( crc )
		crc = NVRAM_FRAM_FAIL;
	//gwSysStatus = crc;
	return crc;
} // end nv_cnfg_fram_recall_with_16bitCRC(,,)


/**
 * Save sensor features settings of a given sensorID.
 *
 * @param  sensorID		-- sensor ID or channel number.
 * @return 0 == passed, none zero == failed.
 *
 *
 * History:  Created on 2009-05-15 by Wai Fai Chin
 * 2010-08-30 -WFC- added m.viewingUnit and gabPcentCapUnderloadFNV[]
 *
 */

BYTE  nv_cnfg_fram_save_a_sensor_feature( BYTE sensorID )
{
	BYTE status;
	SENSOR_FEATURE_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( sensorID < MAX_NUM_SENSORS ) {
		m.capacity	= gafSensorShowCapacityFNV[ sensorID ];
		m.fcountby	= gafSensorShowCBFNV[ sensorID ];
		m.icountby	= gawSensorShowCBFNV[ sensorID ];
		m.decPt		= gabSensorShowCBdecPtFNV[ sensorID ];
		m.unit		= gabSensorShowCBunitsFNV[ sensorID ];
		m.type		= gabSensorTypeFNV[ sensorID ];
		m.convSpeed	= gabSensorSpeedFNV[ sensorID ];
		m.feature	= gabSensorFeaturesFNV[ sensorID ];
		m.pcentCapUnderload = gabPcentCapUnderloadFNV[ sensorID ];
		m.viewingUnit		= gabSensorViewUnitsFNV[ sensorID ];		// 2010-08-30 -WFC-
		status = nv_cnfg_fram_save_with_8bitCRC( SENSOR_FEATURE_FRAM_BASE_ADDR + (sizeof( SENSOR_FEATURE_MAPPER_T ) + 1) * sensorID,
										&m, sizeof( SENSOR_FEATURE_MAPPER_T ));
	}
	return status;
} // end nv_cnfg_fram_save_a_sensor_feature()


/**
 * Save sensor features settings of a given sensorID.
 *
 * @param  sensorID		-- sensor ID or channel number.
 * @return 0 == passed, none zero == failed.
 *
 *
 * History:  Created on 2009/05/15 by Wai Fai Chin
 * 2010-08-30 -WFC- added m.viewingUnit and gabPcentCapUnderloadFNV[]
 */

BYTE  nv_cnfg_fram_recall_a_sensor_feature( BYTE sensorID )
{
	BYTE status;
	SENSOR_FEATURE_MAPPER_T m;
	
	status = NVRAM_FRAM_FAIL;
	if ( sensorID < MAX_NUM_SENSORS ) {
		status = nv_cnfg_fram_recall_with_8bitCRC( &m, SENSOR_FEATURE_FRAM_BASE_ADDR + (sizeof( SENSOR_FEATURE_MAPPER_T ) + 1) * sensorID,
										  sizeof( SENSOR_FEATURE_MAPPER_T ));
		if ( NVRAM_READ_PASS == status )	{
			gafSensorShowCapacityFNV[ sensorID]	= m.capacity;
			gafSensorShowCBFNV[ sensorID ]		= m.fcountby;
			gawSensorShowCBFNV[ sensorID ]		= m.icountby;
			gabSensorShowCBdecPtFNV[ sensorID ]	= m.decPt;
			gabSensorShowCBunitsFNV[ sensorID ]	= m.unit;
			gabSensorTypeFNV[ sensorID ]		= m.type;
			gabSensorSpeedFNV[ sensorID ]		= m.convSpeed;
			gabSensorFeaturesFNV[ sensorID ]	= m.feature;
			gabPcentCapUnderloadFNV[ sensorID ] = m.pcentCapUnderload;
			gabSensorViewUnitsFNV[ sensorID ]	= m.viewingUnit;			// 2010-08-30 -WFC-
		}
	}
	return status;
} // end nv_cnfg_fram_recall_a_sensor_feature();

/**
 * Default sensor viewing countby.
 *
 * @param  sensorID		-- sensor ID or channel number.
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2010-09-10 by Wai Fai Chin
 */

void  nv_cnfg_fram_default_a_sensor_viewing_cb( BYTE sensorID )
{
	if ( sensorID < MAX_NUM_SENSORS ) {
		gafSensorShowCBFNV[ sensorID ]		= 1;
		gawSensorShowCBFNV[ sensorID ]		= 1;
		gabSensorShowCBdecPtFNV[ sensorID ]	= 0;
		gabSensorShowCBunitsFNV[ sensorID ]	= 0;
	}
} // end nv_cnfg_fram_default_a_sensor_viewing_cb();


/**
 * Save total weight, events statistics data of specified loadcell number.
 *
 * @param  lc	-- loadcell number
 * @return 0 == passed, none zero == failed.
 *
 *
 * History:  Created on 2009/08/05 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_totaling_statistics( BYTE lc )
{
	BYTE status;
	TOTAL_STAT_DATA_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		m.totalWt		= gafTotalWtFNV[ lc ];
		m.sumSqTotalWt	= gafSumSqTotalWtFNV[ lc ];
		m.maxTotalWt	= gafMaxTotalWtFNV[ lc ];
		m.minTotalWt	= gafMinTotalWtFNV[ lc ];
		m.numTotal		= gawNumTotalFNV[ lc ];
		m.totalMode		= gabTotalModeFNV[ lc ];
		status = nv_cnfg_fram_save_with_8bitCRC( TOTAL_STAT_DATA_FRAM_BASE_ADDR + (sizeof( TOTAL_STAT_DATA_MAPPER_T ) + 1) * lc,
										&m, sizeof( TOTAL_STAT_DATA_MAPPER_T ));
	}
	return status;
} // end nv_cnfg_fram_save_totaling_statistics()


/**
 * Recall total weight, events statistics data of specified loadcell number.
 *
 * @param  lc	-- loadcell number
 * @return 0 == passed, none zero == failed.
 * @note if recall failed, you can default by call default of cmd {25} and {26} or defaul manually.
 *
 * History:  Created on 2009/08/05 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_totaling_statistics( BYTE lc )
{
	BYTE status;
	TOTAL_STAT_DATA_MAPPER_T m;
	
	status = NVRAM_FRAM_FAIL;
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		status = nv_cnfg_fram_recall_with_8bitCRC( &m, TOTAL_STAT_DATA_FRAM_BASE_ADDR + (sizeof( TOTAL_STAT_DATA_MAPPER_T ) + 1) * lc,
										  sizeof( TOTAL_STAT_DATA_MAPPER_T ));
		if ( NVRAM_READ_PASS == status )	{
			gafTotalWtFNV[ lc ]			= m.totalWt;
			gafSumSqTotalWtFNV[ lc ]	= m.sumSqTotalWt;
			gafMaxTotalWtFNV[ lc ]		= m.maxTotalWt;
			gafMinTotalWtFNV[ lc ]		= m.minTotalWt;
			gawNumTotalFNV[ lc ]		= m.numTotal;
			gabTotalModeFNV[ lc ]		= m.totalMode;
		}
	}
	return status;
} // end nv_cnfg_fram_recall_totaling_statistics();


/**
 * default total weight, events statistics data of specified loadcell number.
 *
 * @param  lc	-- loadcell number
 * @return none.
 *
 * History:  Created on 2009-08-06 by Wai Fai Chin
 * 2011-04-28 -WFC- remove set total mode.
 */

void  nv_cnfg_fram_default_totaling_statistics( BYTE lc )
{
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		gafTotalWtFNV[ lc ]			= 0.0f;
		gafSumSqTotalWtFNV[ lc ]	= 0.0f;
		gafMaxTotalWtFNV[ lc ]		= 0.0f;
		gafMinTotalWtFNV[ lc ]		= 0.0f;
		gawNumTotalFNV[ lc ]		= 0;
		// 2011-04-28 -WFC-		gabTotalModeFNV[ lc ]		= LC_TOTAL_MODE_AUTO_NORMAL;
	}
} // end nv_cnfg_fram_default_totaling_statistics();


/**
 * Save loadcell dynamic data of specified loadcell number.
 *
 * @param  lc	-- loadcell number
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2009/08/05 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_loadcell_dynamic_data( BYTE lc )
{
	BYTE status;
	LOADCELL_DYNAMIC_DATA_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		m.tareWt	= gafTareWtFNV[ lc ];
		m.zeroWt	= gafZeroWtFNV[ lc ];
		m.opMode	= gabLcOpModesFNV[ lc ];
		status = nv_cnfg_fram_save_with_8bitCRC( LOADCELL_DYNAMIC_DATA_FRAM_BASE_ADDR + (sizeof( LOADCELL_DYNAMIC_DATA_MAPPER_T ) + 1) * lc,
										&m, sizeof( LOADCELL_DYNAMIC_DATA_MAPPER_T ));
	}
	return status;
} // end nv_cnfg_fram_save_loadcell_dynamic_data()


/**
 * Recall loadcell dynamic data of specified loadcell number.
 *
 * @param  lc	-- loadcell number
 * @return 0 == passed, none zero == failed.
 * @note if recall failed, you need to manually default it to 0.
 *
 *
 * History:  Created on 2009/08/05 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_loadcell_dynamic_data( BYTE lc )
{
	BYTE status;
	LOADCELL_DYNAMIC_DATA_MAPPER_T m;
	
	status = NVRAM_FRAM_FAIL;
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		status = nv_cnfg_fram_recall_with_8bitCRC( &m, LOADCELL_DYNAMIC_DATA_FRAM_BASE_ADDR + (sizeof( LOADCELL_DYNAMIC_DATA_MAPPER_T ) + 1) * lc,
										  sizeof( LOADCELL_DYNAMIC_DATA_MAPPER_T ));
		if ( NVRAM_READ_PASS == status )	{
			gafTareWtFNV[ lc ]		= m.tareWt;
			gaLoadcell[lc].prvZeroWt = gafZeroWtFNV[ lc ]	= m.zeroWt;
			gabLcOpModesFNV[ lc ]	= m.opMode;
		}
	}
	return status;
} // end nv_cnfg_fram_recall_loadcell_dynamic_data();


/**
 * Default loadcell dynamic data of specified loadcell number.
 *
 * @param  lc	-- loadcell number
 * @return 0 == passed, none zero == failed.
 * @note if recall failed, you need to manually default it to 0.
 *
 * History:  Created on 2009-08-06 by Wai Fai Chin
 * 2012-10-29 -WFC- Preserved LC_OP_MODE_TRUE_CAP_UNIT_CNV flag.
 */

void  nv_cnfg_fram_default_loadcell_dynamic_data( BYTE lc )
{
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		gafTareWtFNV[ lc ]		= 0.0f;
		gafZeroWtFNV[ lc ]		= 0.0f;
		// 2012-10-29 -WFC- gabLcOpModesFNV[ lc ]	= 0;
		gabLcOpModesFNV[ lc ]	&= LC_OP_MODE_TRUE_CAP_UNIT_CNV;		// 2012-10-29 -WFC-
	}
} // end nv_cnfg_fram_default_loadcell_dynamic_data();

/**
 * Save service counters of specified loadcell number.
 *
 * @param  lc	-- loadcell number
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2009-08-05 by Wai Fai Chin
 * 2010-08-30 -WFC- removed w25pctLiftCnt, added serviceStatus, liftThreshold and dropThreshold. changed counter from 16bits to 32bits;
 * 2014-10-20 -WFC- saved gaulUserLiftCntFNV[]
 *
 */

BYTE  nv_cnfg_fram_save_service_counters( BYTE lc )
{
	BYTE status;
	SERVICE_COUNTER_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		m.userLiftCnt			= gaulUserLiftCntFNV[ lc ];			// 2014-10-20 -WFC-
		m.liftCnt				= gaulLiftCntFNV[ lc ];
		m.overloadCnt			= gaulOverloadedCntFNV[ lc ];
		m.liftThresholdPctCap	= gabLiftThresholdPctCapFNV[ lc ];
		m.dropThresholdPctCap	= gabDropThresholdPctCapFNV[lc];
		m.serviceStatus			= gabServiceStatusFNV[ lc ];
		status = nv_cnfg_fram_save_with_8bitCRC( SERVICE_COUNTER_FRAM_BASE_ADDR + (sizeof( SERVICE_COUNTER_MAPPER_T ) + 1) * lc,
										&m, sizeof( SERVICE_COUNTER_MAPPER_T ));
	}
	return status;
} // end nv_cnfg_fram_save_service_counters()

/* 2010-08-30 -WFC-
BYTE  nv_cnfg_fram_save_service_counters( BYTE lc )
{
	BYTE status;
	SERVICE_COUNTER_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		m.w5pctLiftCnt	= gawLiftCntFNV[ lc ];
		m.w25pctLiftCnt	= gaw25perCapCntFNV[ lc ];
		m.overloadCnt	= gawOverloadedCntFNV[ lc ];
		status = nv_cnfg_fram_save_with_8bitCRC( SERVICE_COUNTER_FRAM_BASE_ADDR + (sizeof( SERVICE_COUNTER_MAPPER_T ) + 1) * lc,
										&m, sizeof( SERVICE_COUNTER_MAPPER_T ));
	}
	return status;
} // end nv_cnfg_fram_save_service_counters()
 */

/**
 * Recall service counters of specified loadcell number.
 *
 * @param  lc	-- loadcell number
 * @return 0 == passed, none zero == failed.
 * @note if recall failed, you need to manually default it to 0.
 *
 * History:  Created on 2009/08/05 by Wai Fai Chin
 * 2010-08-30 -WFC- removed gaw25perCapCntFNV, added gabServiceStatusFNV, gabLiftThresholdPctCapFNV and gabDropThresholdPctCapFNV;
 * 2014-10-20 -WFC- recalled gaulUserLiftCntFNV[]
 */

BYTE  nv_cnfg_fram_recall_service_counters( BYTE lc )
{
	BYTE status;
	SERVICE_COUNTER_MAPPER_T m;
	
	status = NVRAM_FRAM_FAIL;
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		status = nv_cnfg_fram_recall_with_8bitCRC( &m, SERVICE_COUNTER_FRAM_BASE_ADDR + (sizeof( SERVICE_COUNTER_MAPPER_T ) + 1) * lc,
										  sizeof( SERVICE_COUNTER_MAPPER_T ));
		if ( NVRAM_READ_PASS == status )	{
			gaulUserLiftCntFNV[ lc ]		= m.userLiftCnt;			// 2014-10-20 -WFC-
			gaulLiftCntFNV[ lc ]			= m.liftCnt;
			gaulOverloadedCntFNV[ lc ]		= m.overloadCnt;
			gabLiftThresholdPctCapFNV[ lc ]	= m.liftThresholdPctCap;
			gabDropThresholdPctCapFNV[ lc ]	= m.dropThresholdPctCap;
			gabServiceStatusFNV[ lc ]		= m.serviceStatus;
		}
	}
	return status;
} // end nv_cnfg_fram_recall_service_counters();

/* 2010-08-30 -WFC-
BYTE  nv_cnfg_fram_recall_service_counters( BYTE lc )
{
	BYTE status;
	SERVICE_COUNTER_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		status = nv_cnfg_fram_recall_with_8bitCRC( &m, SERVICE_COUNTER_FRAM_BASE_ADDR + (sizeof( SERVICE_COUNTER_MAPPER_T ) + 1) * lc,
										  sizeof( SERVICE_COUNTER_MAPPER_T ));
		if ( NVRAM_READ_PASS == status )	{
			gawLiftCntFNV[ lc ] = m.w5pctLiftCnt;
			gaw25perCapCntFNV[ lc ] = m.w25pctLiftCnt;
			gawOverloadedCntFNV[ lc ] = m.overloadCnt;
		}
	}
	return status;
} // end nv_cnfg_fram_recall_service_counters();
*/

/**
 * default service counters of specified loadcell number.
 *
 * @param  lc	-- loadcell number
 * @return none
 *
 *
 * History:  Created on 2009-08-06 by Wai Fai Chin
 * 2010-08-30 -WFC- removed w25pctLiftCnt, added serviceStatus, liftThreshold and dropThreshold. changed counter from 16bits to 32bits;
 * 2014-10-20 -WFC- clear gaulUserLiftCntFNV[]
 */

void  nv_cnfg_fram_default_service_counters( BYTE lc )
{
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		gaulUserLiftCntFNV[ lc ]	=
		gaulLiftCntFNV[ lc ] =
		gaulOverloadedCntFNV[ lc ] = 0;
		gabServiceStatusFNV[ lc ] = 0;
		gabLiftThresholdPctCapFNV[ lc ] = 5;		// 5% of capacity.
		gabDropThresholdPctCapFNV[ lc ] = 1;		// 1% of capacity.
	}
} // end nv_cnfg_fram_default_service_counters();

/* 2010-08-30 -WFC-
void  nv_cnfg_fram_default_service_counters( BYTE lc )
{
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		gawLiftCntFNV[ lc ] =
		gaw25perCapCntFNV[ lc ] =
		gawOverloadedCntFNV[ lc ] = 0;
	}
} // end nv_cnfg_fram_default_service_counters();
*/

/**
 * It saves listener settings to none volatile FRAM.
 *
 * @param  none
 * @return 0 == passed, NVRAM_FRAM_FAIL == failed.
 *
 *
 * History:  Created on 2009/05/21 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_all_listeners_settings(void)
{
	BYTE status;
	BYTE abTmp[ MAX_NUM_STREAM_LISTENER * 5 ];
	
	memcpy(  abTmp, 							gabListenerWantSensorFNV,	MAX_NUM_STREAM_LISTENER);
	memcpy( &abTmp[MAX_NUM_STREAM_LISTENER],	gabListenerModesFNV,		MAX_NUM_STREAM_LISTENER);
	memcpy( &abTmp[MAX_NUM_STREAM_LISTENER*2],	gabListenerIntervalFNV,		MAX_NUM_STREAM_LISTENER);
	memcpy( &abTmp[MAX_NUM_STREAM_LISTENER*3],	gabListenerDevIdFNV, 		MAX_NUM_STREAM_LISTENER);
	memcpy( &abTmp[MAX_NUM_STREAM_LISTENER*4],	gabListenerStreamTypeFNV,	MAX_NUM_STREAM_LISTENER);
	status = nv_cnfg_fram_save_with_8bitCRC( LISTENER_SETTING_FRAM_BASE_ADDR, abTmp, MAX_NUM_STREAM_LISTENER * 5 );
	return status;
} // end nv_cnfg_fram_save_all_listeners_settings()


/**
 * It recalls listener settings from nonevolatile FRAM.
 *
 * @param  none
 * @return 0 == passed, NVRAM_FRAM_FAIL == failed.
 *
 *
 * History:  Created on 2009/05/21 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_all_listeners_settings(void)
{
	BYTE status;
	BYTE abTmp[ MAX_NUM_STREAM_LISTENER * 5 ];

	status = nv_cnfg_fram_recall_with_8bitCRC( abTmp, LISTENER_SETTING_FRAM_BASE_ADDR,  MAX_NUM_STREAM_LISTENER *5 );
	if ( NVRAM_READ_PASS == status )	{
		memcpy( gabListenerWantSensorFNV,	abTmp,								MAX_NUM_STREAM_LISTENER);
		memcpy( gabListenerModesFNV,		&abTmp[MAX_NUM_STREAM_LISTENER],	MAX_NUM_STREAM_LISTENER);
		memcpy( gabListenerIntervalFNV,		&abTmp[MAX_NUM_STREAM_LISTENER*2],	MAX_NUM_STREAM_LISTENER);
		memcpy( gabListenerDevIdFNV,		&abTmp[MAX_NUM_STREAM_LISTENER*3],	MAX_NUM_STREAM_LISTENER);
		memcpy( gabListenerStreamTypeFNV,	&abTmp[MAX_NUM_STREAM_LISTENER*4],	MAX_NUM_STREAM_LISTENER);
	}
	return status;
} // end nv_cnfg_fram_recall_all_listeners_settings()


/**
 * It saves set point configuration to none volatile FRAM.
 *
 * @param  none
 * @return 0 == passed, NVRAM_FRAM_FAIL == failed.
 *
 *
 * History:  Created on 2009/06/25 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_all_setpoints(void)
{
	BYTE status;
	BYTE abTmp[ FRAM_SETPOINT_SIZE ];
	
	memcpy(  abTmp, 						gaSP_sensorID_FNV,				SETPOINT_MAX_NUM);
	memcpy( &abTmp[SETPOINT_MAX_NUM],		gaSP_cmp_logic_value_modeFNV,	SETPOINT_MAX_NUM);
	memcpy( &abTmp[SETPOINT_MAX_NUM*2],	gaSP_hysteresisCB_FNV,			SETPOINT_MAX_NUM);
	memcpy( &abTmp[SETPOINT_MAX_NUM*3],	gaSP_fCmpValueFNV, 				SETPOINT_MAX_NUM * sizeof(float) );
	status = nv_cnfg_fram_save_with_8bitCRC( SETPOINT_FRAM_BASE_ADDR, abTmp, FRAM_SETPOINT_SIZE );
	return status;
} // end nv_cnfg_fram_save_all_setpoints()


/**
 * It recalls set point configuration to none volatile FRAM.
 *
 * @param  none
 * @return 0 == passed, NVRAM_FRAM_FAIL == failed.
 *
 *
 * History:  Created on 2009/06/25 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_all_setpoints(void)
{
	BYTE status;
	BYTE abTmp[ FRAM_SETPOINT_SIZE ];

	status = nv_cnfg_fram_recall_with_8bitCRC( abTmp, SETPOINT_FRAM_BASE_ADDR,  FRAM_SETPOINT_SIZE );
	if ( NVRAM_READ_PASS == status )	{
		memcpy( gaSP_sensorID_FNV,				 abTmp,						SETPOINT_MAX_NUM);
		memcpy( gaSP_cmp_logic_value_modeFNV,	&abTmp[SETPOINT_MAX_NUM],	SETPOINT_MAX_NUM);
		memcpy( gaSP_hysteresisCB_FNV,			&abTmp[SETPOINT_MAX_NUM*2],SETPOINT_MAX_NUM);
		memcpy( gaSP_fCmpValueFNV,				&abTmp[SETPOINT_MAX_NUM*3],SETPOINT_MAX_NUM * sizeof(float) );
	}
	return status;

} // end nv_cnfg_fram_recall_all_setpoints()


/**
 * Default configuration of DAC of a specified channel.
 *
 * @param  channel	-- channel number of a DAC
 * @return 0 == passed, none zero == failed.
 * @note if recall failed, you need to manually default it to 0.
 *
 *
 * History:  Created on 2010/01/26 by Wai Fai Chin
 */

void  nv_cnfg_fram_default_dac_config( BYTE channel )
{
	if ( channel < MAX_DAC_CHANNEL ) {
		gabDacOffsetFNV[ channel ]					= 0;
		gabDacGainFNV[ channel ]					= 0;
		gawDacCountMinSpanFNV[ channel ]			= 0;
		gawDacCountMaxSpanFNV[ channel ]			= MAX_DAC_VALUE;
		gafDacSensorValueAtMinSpanFNV[ channel ]	= 0;
		gafDacSensorValueAtMaxSpanFNV[ channel ]	= 1000;
		gabDacSensorUnitFNV[ channel ]				= SENSOR_UNIT_LB;
		gabDacSensorIdFNV[ channel ]				= 0;
		gabDacSensorValueTypeFNV[ channel ]			= SENSOR_VALUE_TYPE_GROSS;
		gabDacCnfgStatusFNV[ channel ]				= 0;
	}
} // end nv_cnfg_fram_default_dac_confg();


/**
 * Save configuration of DAC of a specified channel.
 *
 * @param  channel	-- channel number of a DAC
 * @return 0 == passed, none zero == failed.
 *
 *
 * History:  Created on 2010/01/26 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_dac_config( BYTE channel )
{
	BYTE status;
	DAC_CONFIG_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( channel < MAX_DAC_CHANNEL ) {
		m.offset				= gabDacOffsetFNV[ channel ];
		m.gain					= gabDacGainFNV[ channel ];
		m.wDacCountAtMinSpan	= gawDacCountMinSpanFNV[ channel ];
		m.wDacCountAtMaxSpan	= gawDacCountMaxSpanFNV[ channel ];
		m.fSenorValueAtMinSpan	= gafDacSensorValueAtMinSpanFNV[ channel ];
		m.fSensorValueAtMaxSpan	= gafDacSensorValueAtMaxSpanFNV[ channel ];
		m.sensorUnit			= gabDacSensorUnitFNV[ channel ];
		m.sensorID				= gabDacSensorIdFNV[ channel ];
		m.sensorValueType		= gabDacSensorValueTypeFNV[ channel ];
		m.DacStatus				= gabDacCnfgStatusFNV[ channel ];
		status = nv_cnfg_fram_save_with_8bitCRC( DAC_CONFIG_FRAM_BASE_ADDR + (sizeof( DAC_CONFIG_MAPPER_T ) + 1) * channel,
										&m, sizeof( DAC_CONFIG_MAPPER_T ));
	}
	return status;
} // end nv_cnfg_fram_save_dac_config()

/**
 * Recall configuration of DAC of a specified channel.
 *
 * @param  channel	-- channel number of a DAC
 * @return 0 == passed, none zero == failed.
 * @note if recall failed, you need to manually default it to 0.
 *
 *
 * History:  Created on 2010/01/26 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_dac_config( BYTE channel )
{
	BYTE status;
	DAC_CONFIG_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( channel < MAX_DAC_CHANNEL ) {
		status = nv_cnfg_fram_recall_with_8bitCRC( &m, DAC_CONFIG_FRAM_BASE_ADDR + (sizeof( DAC_CONFIG_MAPPER_T ) + 1) * channel,
										  sizeof( DAC_CONFIG_MAPPER_T ));
		if ( NVRAM_READ_PASS == status )	{
			gabDacOffsetFNV[ channel ]				= m.offset;
			gabDacGainFNV[ channel ]				= m.gain;
			gawDacCountMinSpanFNV[ channel ]		= m.wDacCountAtMinSpan;
			gawDacCountMaxSpanFNV[ channel ]		= m.wDacCountAtMaxSpan;
			gafDacSensorValueAtMinSpanFNV[ channel ]= m.fSenorValueAtMinSpan;
			gafDacSensorValueAtMaxSpanFNV[ channel ]= m.fSensorValueAtMaxSpan;
			gabDacSensorUnitFNV[ channel ]			= m.sensorUnit;
			gabDacSensorIdFNV[ channel ]			= m.sensorID;
			gabDacSensorValueTypeFNV[ channel ]		= m.sensorValueType;
			gabDacCnfgStatusFNV[ channel ]			= m.DacStatus;
		}
	}
	return status;
} // end nv_cnfg_fram_recall_dac_config();


/**
 * Save configuration of virtual sensor math of a specified channel.
 *
 * @param  channel	-- channel number of a math channel
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2010-03-11 by Wai Fai Chin
 * 2010-08-30 -WFC- removed gabVSMathUnitFNV[] because loadcell uses gabSensorShowCBunitsFNV[] as a reference unit.
 *
 */

BYTE  nv_cnfg_fram_save_vs_math_config( BYTE channel )
{
	BYTE status;
	VS_MATH_CONFIG_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( channel < MAX_NUM_VS_MATH ) {
		m.vSMathSensorId = gabVSMathSensorIdFNV[ channel ];
		// 2010-08-30 -WFC- m.unit = gabVSMathUnitFNV[ channel ];
		copy_until_match_char(  m.mathExprs, &gabVSMathRawExprsFNV[ channel][0], 0, MAX_VS_RAW_EXPRS_SIZE + 1);
		status = nv_cnfg_fram_save_with_8bitCRC( VS_MATH_CONFIG_FRAM_BASE_ADDR + (sizeof( VS_MATH_CONFIG_MAPPER_T ) + 1) * channel,
										&m, sizeof( VS_MATH_CONFIG_MAPPER_T ));
	}
	return status;
} // end nv_cnfg_fram_save_vs_math_config()

/**
 * Recall configuration of virtual sensor math of a specified channel.
 *
 * @param  channel	-- channel number of a math channel
 * @return 0 == passed, none zero == failed.
 * @note if recall failed, you need to manually default it to 0.
 *
 * History:  Created on 2010-03-11 by Wai Fai Chin
 * 2010-08-30 -WFC- removed gabVSMathUnitFNV[] because loadcell uses gabSensorShowCBunitsFNV[] as a reference unit.
 *
 */

BYTE  nv_cnfg_fram_recall_vs_math_config( BYTE channel )
{
	BYTE status;
	VS_MATH_CONFIG_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if ( channel < MAX_DAC_CHANNEL ) {
		status = nv_cnfg_fram_recall_with_8bitCRC( &m, VS_MATH_CONFIG_FRAM_BASE_ADDR + (sizeof( VS_MATH_CONFIG_MAPPER_T ) + 1) * channel,
										  sizeof( VS_MATH_CONFIG_MAPPER_T ));
		if ( NVRAM_READ_PASS == status )	{
			gabVSMathSensorIdFNV[ channel ]	= m.vSMathSensorId;
			// 2010-08-30 -WFC- gabVSMathUnitFNV[ channel ]		= m.unit;
			copy_until_match_char(  &gabVSMathRawExprsFNV[ channel][0], m.mathExprs, 0, MAX_VS_RAW_EXPRS_SIZE + 1);
		}
	}
	return status;
} // end nv_cnfg_fram_recall_vs_math_config();


/**
 * Save loadcell total mode, motion detection and operation mode settings of a specified loadcell.
 *
 * @param  lc	-- loadcell number.
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2010/06/25 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_lc_total_motion_opmode_config( BYTE lc )
{
	BYTE status;
	LC_TOTAL_MOTION_OPMODE_CONFIG_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if (lc < MAX_NUM_PV_LOADCELL ) {
		m.bLcOpModes = gabLcOpModesFNV[ lc ];
		m.bMotionDetectBand = gabMotionDetectBand_dNV[ lc ];
		m.bMotionDetectPeriodTime = gabMotionDetectPeriodTimeNV[ lc ];
		m.bStable_Pending_Time = gab_Stable_Pending_TimeNV[ lc ];
		m.bTotalDropThresholdPctCap = gabTotalDropThresholdPctCapNV[ lc ];
		m.bTotalMinStableTime = gabTotalMinStableTimeNV[ lc ];
		m.bTotalMode = gabTotalModeFNV[ lc ];
		m.bTotalRiseThresholdPctCap = gabTotalRiseThresholdPctCapNV[ lc ];
		m.fTotalOnAcceptLowerWt = gafTotalOnAcceptLowerWtNV[ lc ];
		m.fTotalOnAcceptUpperWt = gafTotalOnAcceptUpperWtNV[ lc ];
		status = nv_cnfg_fram_save_with_8bitCRC( LC_TOTAL_MOTION_OPMODE_FRAM_BASE_ADDR + (sizeof( LC_TOTAL_MOTION_OPMODE_CONFIG_MAPPER_T ) + 1) * lc,
										&m, sizeof( LC_TOTAL_MOTION_OPMODE_CONFIG_MAPPER_T ));
	}
	return status;
} // end nv_cnfg_fram_save_lc_total_motion_opmode_config()

/**
 * Recall loadcell total mode, motion detection and operation mode settings of specified loadcell.
 *
 * @param  lc	-- loadcell number.
 * @return 0 == passed, none zero == failed.
 * @note if recall failed, you need to manually default it to 0.
 *
 * History:  Created on 2010/06/25 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_lc_total_motion_opmode_config( BYTE lc )
{
	BYTE status;
	LC_TOTAL_MOTION_OPMODE_CONFIG_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if (lc < MAX_NUM_PV_LOADCELL ) {
		status = nv_cnfg_fram_recall_with_8bitCRC( &m, LC_TOTAL_MOTION_OPMODE_FRAM_BASE_ADDR + (sizeof( LC_TOTAL_MOTION_OPMODE_CONFIG_MAPPER_T ) + 1) * lc,
										  sizeof( LC_TOTAL_MOTION_OPMODE_CONFIG_MAPPER_T ));
		if ( NVRAM_READ_PASS == status )	{
			gabLcOpModesFNV[ lc ] = m.bLcOpModes;
			gabMotionDetectBand_dNV[ lc ] = m.bMotionDetectBand;
			gabMotionDetectPeriodTimeNV[ lc ] = m.bMotionDetectPeriodTime;
			gab_Stable_Pending_TimeNV[ lc ] = m.bStable_Pending_Time;
			gabTotalDropThresholdPctCapNV[ lc ] = m.bTotalDropThresholdPctCap;
			gabTotalMinStableTimeNV[ lc ] = m.bTotalMinStableTime;
			gabTotalModeFNV[ lc ] = m.bTotalMode;
			gabTotalRiseThresholdPctCapNV[ lc ] = m.bTotalRiseThresholdPctCap;
			gafTotalOnAcceptLowerWtNV[ lc ] = m.fTotalOnAcceptLowerWt;
			gafTotalOnAcceptUpperWtNV[ lc ] = m.fTotalOnAcceptUpperWt;
		}
	}
	return status;
} // end nv_cnfg_fram_recall_lc_total_motion_opmode_config();


/**
 * Save loadcell standard mode azm, power up zeroing settings of specified loadcell.
 *
 * @param  lc	-- loadcell number.
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2010/06/25 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_lc_standard_mode_azm_config( BYTE lc )
{
	BYTE status;
	LC_STANDARD_MODE_AZM_CONFIG_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if (lc < MAX_NUM_PV_LOADCELL ) {
		m.NTEP_AZM_CBRange 			= gab_NTEP_AZM_CBRangeNV[lc];
		m.NTEP_AZM_IntervalTime 	= gab_NTEP_AZM_IntervalTimeNV[lc];
		m.NTEP_pcentCapZeroBandHi	= gab_NTEP_pcentCapZeroBandHiNV[lc];
		m.NTEP_pcentCapZeroBandLo	= gab_NTEP_pcentCapZeroBandLoNV[lc];
		m.NTEP_pwupZeroBandHi		= gab_NTEP_pwupZeroBandHiNV[lc];
		m.NTEP_pwupZeroBandLo		= gab_NTEP_pwupZeroBandLoNV[lc];

		m.OIML_AZM_CBRange 			= gab_OIML_AZM_CBRangeNV[lc];
		m.OIML_AZM_IntervalTime 	= gab_OIML_AZM_IntervalTimeNV[lc];
		m.OIML_pcentCapZeroBandHi	= gab_OIML_pcentCapZeroBandHiNV[lc];
		m.OIML_pcentCapZeroBandLo	= gab_OIML_pcentCapZeroBandLoNV[lc];
		m.OIML_pwupZeroBandHi		= gab_OIML_pwupZeroBandHiNV[lc];
		m.OIML_pwupZeroBandLo		= gab_OIML_pwupZeroBandLoNV[lc];

		m.STD_AZM_CBRange 			= gab_STD_AZM_CBRangeNV[lc];
		m.STD_AZM_IntervalTime 		= gab_STD_AZM_IntervalTimeNV[lc];
		m.STD_pcentCapZeroBandHi	= gab_STD_pcentCapZeroBandHiNV[lc];
		m.STD_pcentCapZeroBandLo	= gab_STD_pcentCapZeroBandLoNV[lc];
		m.STD_pwupZeroBandHi		= gab_STD_pwupZeroBandHiNV[lc];
		m.STD_pwupZeroBandLo		= gab_STD_pwupZeroBandLoNV[lc];

		status = nv_cnfg_fram_save_with_8bitCRC( LC_STANDARD_MODE_AZM_FRAM_BASE_ADDR + (sizeof( LC_STANDARD_MODE_AZM_CONFIG_MAPPER_T ) + 1) * lc,
										&m, sizeof( LC_STANDARD_MODE_AZM_CONFIG_MAPPER_T ));
	}
	return status;
} // end nv_cnfg_fram_save_lc_standard_mode_azm_config()

/**
 * Recall loadcell standard mode azm, power up zeroing settings of specified loadcell.
 *
 * @param  lc	-- loadcell number.
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2010/06/25 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_lc_standard_mode_azm_config( BYTE lc )
{
	BYTE status;
	LC_STANDARD_MODE_AZM_CONFIG_MAPPER_T m;

	status = NVRAM_FRAM_FAIL;
	if (lc < MAX_NUM_PV_LOADCELL ) {
		status = nv_cnfg_fram_recall_with_8bitCRC( &m, LC_STANDARD_MODE_AZM_FRAM_BASE_ADDR + (sizeof( LC_STANDARD_MODE_AZM_CONFIG_MAPPER_T ) + 1) * lc,
										  sizeof( LC_STANDARD_MODE_AZM_CONFIG_MAPPER_T ));
		if ( NVRAM_READ_PASS == status )	{
			gab_NTEP_AZM_CBRangeNV[lc]			= m.NTEP_AZM_CBRange;
			gab_NTEP_AZM_IntervalTimeNV[lc]		= m.NTEP_AZM_IntervalTime;
			gab_NTEP_pcentCapZeroBandHiNV[lc]	= m.NTEP_pcentCapZeroBandHi;
			gab_NTEP_pcentCapZeroBandLoNV[lc]	= m.NTEP_pcentCapZeroBandLo;
			gab_NTEP_pwupZeroBandHiNV[lc]		= m.NTEP_pwupZeroBandHi;
			gab_NTEP_pwupZeroBandLoNV[lc]		= m.NTEP_pwupZeroBandLo;

			gab_OIML_AZM_CBRangeNV[lc]			= m.OIML_AZM_CBRange;
			gab_OIML_AZM_IntervalTimeNV[lc]		= m.OIML_AZM_IntervalTime;
			gab_OIML_pcentCapZeroBandHiNV[lc]	= m.OIML_pcentCapZeroBandHi;
			gab_OIML_pcentCapZeroBandLoNV[lc]	= m.OIML_pcentCapZeroBandLo;
			gab_OIML_pwupZeroBandHiNV[lc]		= m.OIML_pwupZeroBandHi;
			gab_OIML_pwupZeroBandLoNV[lc]		= m.OIML_pwupZeroBandLo;

			gab_STD_AZM_CBRangeNV[lc]			= m.STD_AZM_CBRange;
			gab_STD_AZM_IntervalTimeNV[lc]		= m.STD_AZM_IntervalTime;
			gab_STD_pcentCapZeroBandHiNV[lc]	= m.STD_pcentCapZeroBandHi;
			gab_STD_pcentCapZeroBandLoNV[lc]	= m.STD_pcentCapZeroBandLo;
			gab_STD_pwupZeroBandHiNV[lc]		= m.STD_pwupZeroBandHi;
			gab_STD_pwupZeroBandLoNV[lc]		= m.STD_pwupZeroBandLo;
		}
	}
	return status;
} // end nv_cnfg_fram_recall_lc_standard_mode_azm_config();


/**
 * Save all sensor names.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2010/06/27 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_all_sensor_names( void )
{
	BYTE status;
	status = nv_cnfg_fram_save_with_16bitCRC( SENSOR_NAME_FRAM_BASE_ADDR, &gabSensorNameFNV[0][0], MAX_NUM_SENSORS *( MAX_SENSOR_NAME_SIZE + 1 ) );
	return status;
}


/**
 * Recall all sensor names.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2010/06/27 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_all_sensor_names( void )
{
	BYTE status;
	status = nv_cnfg_fram_recall_with_16bitCRC( &gabSensorNameFNV[0][0], SENSOR_NAME_FRAM_BASE_ADDR, MAX_NUM_SENSORS *( MAX_SENSOR_NAME_SIZE + 1 ) );
	return status;
}

/**
 * Save print string configuration.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2011/07/08 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_print_string_config( void )
{
	BYTE status;
	BYTE i;
	PRINT_STRING_CONFIG_MAPPER_T  m;

	status = NVRAM_FRAM_FAIL;
	for ( i=0; i < MAX_NUM_STREAM_LISTENER; i++) {
		m.gabPrintStringCtrlModeFNV[i]	= gabPrintStringCtrlModeFNV[i];
		m.gaulPrintStringCompositeFNV[i]= gaulPrintStringCompositeFNV[i];
		m.gawPrintStringIntervalFNV[i]	= gawPrintStringIntervalFNV[i];
	}

	memcpy( &m.gabPrintStringUserFormatterFNV[0][0], &gabPrintStringUserFormatterFNV[0][0], PRINT_STRING_MAX_NUM_FORMATER * ( PRINT_STRING_MAX_FORMATER_LENGTH + 1 ));

	status = nv_cnfg_fram_save_with_16bitCRC( PRINT_STRING_CONFIG_FRAM_BASE_ADDR, &m, PRINT_STRING_CONFIG_MAPPER_SIZE );
	return status;
} // end nv_cnfg_fram_save_print_string_config()

/**
 * Recall loadcell standard mode azm, power up zeroing settings of specified loadcell.
 *
 * @param  lc	-- loadcell number.
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2011/07/08 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_print_string_config( void )
{
	BYTE status;
	BYTE i;
	PRINT_STRING_CONFIG_MAPPER_T  m;

	status = NVRAM_FRAM_FAIL;
	status = nv_cnfg_fram_recall_with_16bitCRC( &m, PRINT_STRING_CONFIG_FRAM_BASE_ADDR,  PRINT_STRING_CONFIG_MAPPER_SIZE);
	if ( NVRAM_READ_PASS == status )	{
		for ( i=0; i < MAX_NUM_STREAM_LISTENER; i++) {
			gabPrintStringCtrlModeFNV[i]	= m.gabPrintStringCtrlModeFNV[i];
			gaulPrintStringCompositeFNV[i]	= m.gaulPrintStringCompositeFNV[i];
			gawPrintStringIntervalFNV[i]	= m.gawPrintStringIntervalFNV[i];
		}

		memcpy( &gabPrintStringUserFormatterFNV[0][0],	&m.gabPrintStringUserFormatterFNV[0][0], PRINT_STRING_MAX_NUM_FORMATER * ( PRINT_STRING_MAX_FORMATER_LENGTH + 1 ));
	}
	return status;
} // end nv_cnfg_fram_recall_print_string_config();


/**
 * Save RF device configuration settings.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2012-04-20 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_save_rf_device_cnfg( void )
{
	return ( nv_cnfg_fram_save_with_8bitCRC( RF_DEVICE_CONFIG_FRAM_BASE_ADDR, &gRfDeviceSettingsFNV, sizeof( RF_DEVICE_SETTINGS_T )));
} // end nv_cnfg_fram_save_rf_device_cnfg()


/**
 * Recall RF device configuration settings.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2012-04-20 by Wai Fai Chin
 */

BYTE  nv_cnfg_fram_recall_rf_device_cnfg( void )
{
	return ( nv_cnfg_fram_recall_with_8bitCRC( &gRfDeviceSettingsFNV, RF_DEVICE_CONFIG_FRAM_BASE_ADDR, sizeof( RF_DEVICE_SETTINGS_T )));
} // end nv_cnfg_fram_recall_rf_device_cnfg();


/**
 * Default RF device configuration settings.
 *
 * History:  Created on 2012-04-20 by Wai Fai Chin
 *
 */

void  nv_cnfg_fram_default_rf_device_cnfg( void )
{
	gRfDeviceSettingsFNV.deviceType = RF_DEVICE_TYPE_XBEE; // 2016-01-20 -DLM-
	gRfDeviceSettingsFNV.status	 = 0;
	gRfDeviceSettingsFNV.channel = 15;
	gRfDeviceSettingsFNV.networkID = 0x1A5D;
	gRfDeviceSettingsFNV.networkMode = RF_NETWORK_MODE_PEER_TO_PEER;
	gRfDeviceSettingsFNV.powerlevel	= 4;


} // end nv_cnfg_fram_default_rf_device_cnfg();



/**
 * Save Ethernet device configuration settings.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2012-07-06 by Denis Monteiro
 */

BYTE  nv_cnfg_fram_save_ethernet_device_cnfg( void )
{
	return ( nv_cnfg_fram_save_with_8bitCRC( ETHERNET_DEVICE_CONFIG_FRAM_BASE_ADDR, &gEthernetDeviceSettingsFNV, sizeof( ETHERNET_DEVICE_SETTINGS_T )));
} // end nv_cnfg_fram_save_rf_device_cnfg()


/**
 * Recall Ethernet device configuration settings.
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2012-07-06 by Denis Monteiro
 */

BYTE  nv_cnfg_fram_recall_ethernet_device_cnfg( void )
{
	return ( nv_cnfg_fram_recall_with_8bitCRC( &gEthernetDeviceSettingsFNV, ETHERNET_DEVICE_CONFIG_FRAM_BASE_ADDR, sizeof( ETHERNET_DEVICE_SETTINGS_T )));
} // end nv_cnfg_fram_recall_rf_device_cnfg();


