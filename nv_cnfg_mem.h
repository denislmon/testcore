/*! \file nv_cnfg_mem.h \brief nonevolatile configuration memory related functions.*/
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
/// \ingroup Application
/// \defgroup nv_cnfg_mem Nvmem manages nonevolatile memory such as EEPROM, FLASH etc... (nvram.c)
/// \code #include "nv_cnfg_mem.h" \endcode
/// \par Overview
///   It allocated global nonevolatile memory variables.
/// The nonevolatile memory could be EEPROM, flash or other nonevolatile memory device.
/// It is for save system configuration like calibration, output mode, etc...
//
// ****************************************************************************
//@{


#ifndef MSI_NV_CNFG_MEM_H
#define MSI_NV_CNFG_MEM_H

#include  "calibrate.h"
#include  "sensor.h"
#include  "dataoutputs.h"
#include  "eeprm.h"
#include  "setpoint.h"
#include  "vs_math.h"

#define  MAX_PASSWORD_LENGTH	8

#define USER_DEF_MODEL_ATP			0
#define USER_DEF_MODEL_CHI107		1
#define USER_DEF_MODEL_CHI234		2
#define USER_DEF_MODEL_GENERIC_AC	3
#define USER_DEF_MODEL_GENERIC_DC	4
#define USER_DEF_MODEL_NAME	"ATP"
#define	MAX_MODEL_NAME_LENGTH		6
#define BARGRAPH_MAX_NUM_SEGMENT	100
// 2012-09-25 -WFC v
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_DSC  || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI)
	#define USER_DEF_MODEL_MAX			5
#else
	#define USER_DEF_MODEL_BATTERY_3C		203				// 3 C cells batteries
    #define USER_DEF_MODEL_BATTERY_12V_IS	222				// 2017-09-12 -DLM- IS battery selection
    #define USER_DEF_MODEL_BATTERY_12V		212
    #define USER_DEF_MODEL_BATTERY_6V		206
    #define USER_DEF_MODEL_BATTERY_6D		204
    #define USER_DEF_MODEL_AC				110

	#define USER_DEF_MODEL_MAX			254
#endif
// 2012-09-25 -WFC ^


// 2011-07-25 -WFC- v moved from print_string.h file:
/// Maximum number of formatter. Note: must not > 9.
#define PRINT_STRING_MAX_NUM_FORMATER			7
#define PRINT_STRING_MAX_FORMATER_LENGTH		18
#define PRINT_STRING_MAX_FIELD_LENGTH			18

#define PRINT_STRING_MAX_NUM_COMPOSITE			5
#define PRINT_STRING_MAX_COMPOSITE_LENGTH		5

/// print string control mode
extern BYTE	gabPrintStringCtrlModeFNV[ MAX_NUM_STREAM_LISTENER ];

/// print string composite for each listener as defined in command {1A}.
extern UINT32	gaulPrintStringCompositeFNV[ MAX_NUM_STREAM_LISTENER ];

/// print string interval in seconds for each listener as defined in command {1A}.
extern UINT16	gawPrintStringIntervalFNV[ MAX_NUM_STREAM_LISTENER ];

/// User defined print string formatter
extern BYTE	gabPrintStringUserFormatterFNV[ PRINT_STRING_MAX_NUM_FORMATER ][ PRINT_STRING_MAX_FORMATER_LENGTH + 1 ];

extern const char gcStrDefPrintFormatter0[]		PROGMEM;

// 2011-07-25 -WFC- ^



/*!
  \brief Essential sensor feature mapper for map between ram variables to and from Ferri RAM.
*/
typedef struct  SENSOR_FEATURE_MAPPER_TAG {
							/// capacity of this sensor
	float   capacity;
							/// float type countby
	float   fcountby;
							/// integer type countby
	UINT16  icountby;
							/// decimal point
	INT8	decPt;
							/// unit of this countby
	UINT8	unit;
							/// sensor type
	BYTE	type;
							/// sensor conversion speed
	BYTE	convSpeed;
							/// sensor feature:  sensor status, Bit7 enabled; Bit6 AC excitation, Bit 5 temperature compensation, (Bit4 Global reference,1=enabled) Bit4-3 voltage reference. Bit2 reserved, Bit1-0 filter level, 0==disalbed.
	BYTE	feature;
							/// percentage of view capacity of underload threshold. A value less than this threshold is underload.
	BYTE	pcentCapUnderload;
							/// viewing unit dynamically selected by user. This unit changed will not changed the gafSensorShowXXXXXFNV[] variables. It changed the gaLoadcell[]'s unit and other variables.  2010-08-30 -WFC-
	BYTE	viewingUnit;	// 2010-08-30 -WFC-
}SENSOR_FEATURE_MAPPER_T;


/*!
  \brief Product info record
*/
typedef struct  PRODUCT_INFO_TAG {
							/// Device ID, it is use in host commands. ScaleCore host commands are network centerict.
	BYTE	devID;
							/// command lock
	BYTE	cmdLock;
							/// This PCB serial number
	UINT32  serialNum;
							/// product serial number
	UINT32  serialNumProduct;
	///xxxx user defined model code, for example, for CHI customer, 0 = CHI 234, 1 = helicopter Kawasaki 107,
	/// user defined model code, for example, for CHI customer, 0 = ATP mode, 1 = helicopter Kawasaki 107, 2 = CHI 234
	BYTE	userDefModelCode;
							/// user defined model name.
	BYTE	modelName[ MAX_MODEL_NAME_LENGTH + 1];
							/// password
	BYTE	password[ MAX_PASSWORD_LENGTH + 1];
}PRODUCT_INFO_T;

/*!
  \brief Product info record of a remote device
*/
typedef struct  PRODUCT_INFO_OF_REMOTE_DEVICE_TAG {
							/// Device ID, it is use in host commands. ScaleCore host commands are network centerict.
	BYTE	devID;
							///xxxx user defined model code, for example, for CHI customer, 0 = CHI 234, 1 = helicopter Kawasaki 107,
							/// user defined model code, for example, for CHI customer, 0 = ATP mode, 1 = helicopter Kawasaki 107, 2 = CHI 234
	BYTE	userDefModelCode;
							/// product ID
	BYTE	productID;
								/// product version ID
	BYTE	productVersionID;

	BYTE	softVersionStr[ MAX_SOFTWARE_VERSION_LENGTH + 1];		// software version supplied by .prg file.
							/// user defined model name.
	BYTE	modelName[ MAX_MODEL_NAME_LENGTH + 1];
			/// This PCB serial number
	UINT32  serialNum;
			/// product serial number
	UINT32  serialNumProduct;

}PRODUCT_INFO_OF_REMOTE_DEVICE_T;

/*!
  \brief System feature.
*/
typedef struct  SYSTEM_FEATURE_TAG {
							/// Programmable User key function
	BYTE	userKeyFunc[2];
							/// Auto Power off mode, 0== disabled, 1 == off in 15 minutes, 2== off in 30 minutes, 3== off in 45, 4== off in 60 minutes.
	BYTE	autoOffMode;
							/// LED intensity. 0== auto, 1 == LO_1, 2== LO_2, 3== High_1, 4== High_2
	BYTE	ledIntensity;
							/// bargraph input sensor ID.
	BYTE	bargraphSensorID;
							/// bargrpah input value type, bit7 = unfiltered type,
	BYTE	bargraphValueType;
							/// unit of sensor value for bargraph. This unit is the reference unit. Change this unit will not auto change the minValue1stSegment and maxValueLastSegment.
	BYTE	bargraphValueUnit;
							/// Maximum number of segment of a bargrpah
	BYTE	maxNumSegment;
							/// LED Auto Sleep. 0== disabled, 1 == off in 5 minutes, 2== off in 15 minutes, 3== off in 30 minutes.
	BYTE	ledSleep;		//	PHJ
							/// peak hold sample speed for all sensors.
	BYTE	peakholdSampleSpeed;	// 2011-03-29	-WFC-
							/// minimum value to turn on the 1st segment.
	float	minValue1stSegment;
							/// maximum value to turn on the last segment.
	float	maxValueLastSegment;

}SYSTEM_FEATURE_T;

/// MAX value of auto off mode. each increment is 15 minutes. 2011-07-22 -WFC-
#define	SYS_FEATURE_AUTO_OFF_MODE_MAX_VALUE		4

/// MAX value of LED sleep. each increment is 15 minutes. 2011-07-22 -WFC-
#define	SYS_FEATURE_LED_SLEEP_MAX_VALUE		3

/// MAX value of LED Intensity. each increment is power of 2. 2011-07-22 -WFC-
// 2013-09-05 -WFC- #define	SYS_FEATURE_LED_INTENSITY_MAX_VALUE		3
#define	SYS_FEATURE_LED_INTENSITY_MAX_VALUE		4			// 2013-09-05 -WFC- matched both Challenger3 and PortaWeigh plus LED intensity setting.


/*!
  \brief  total weight, event statistics
*/
typedef struct  TOTAL_STAT_DATA_MAPPER_TAG {
							/// total weight
	float   totalWt;
							/// sum squart of total weight
	float   sumSqTotalWt;
							/// maxium total weight
	float   maxTotalWt;
							/// minium total weight
	float   minTotalWt;
							/// number of total event counter
	UINT16	numTotal;
							/// total mode.
	BYTE	totalMode;
}TOTAL_STAT_DATA_MAPPER_T;


/*!
  \brief  loadcell dynamic persistent data mapper
*/
typedef struct  LOADCELL_DYNAMIC_DATA_MAPPER_TAG {
							/// tared weight
	float   tareWt;
							/// zero weight
	float   zeroWt;
							/// operation mode such as net, gross.
	BYTE	opMode;
}LOADCELL_DYNAMIC_DATA_MAPPER_T;


/*!
  \brief  loadcell dynamic persistent data mapper
 * 2010-08-30 -WFC- removed w25pctLiftCnt, added serviceStatus, liftThreshold and dropThreshold. changed counter from 16bits to 32bits;
*/
typedef struct  SERVICE_COUNTER_MAPPER_TAG {
							/// user programmable capacity weight user lift counter
	UINT32	userLiftCnt;
							/// user programmable capacity weight service lift counter
	UINT32	liftCnt;
							/// over capacity or overload counter.
	UINT32	overloadCnt;
							/// lift threshold based on percentage of capacity.
	BYTE	liftThresholdPctCap;
							/// drop threshold based on percentage of capacity.
	BYTE	dropThresholdPctCap;
							/// service status flags.
	BYTE	serviceStatus;
}SERVICE_COUNTER_MAPPER_T;

/* 2010-08-30 -WFC-
typedef struct  SERVICE_COUNTER_MAPPER_TAG {
							/// 5% capacity weight lift counter
	UINT16	w5pctLiftCnt;
							/// 25% capacity weight lift counter
	UINT16	w25pctLiftCnt;
							/// over capacity or overload counter.
	UINT16	overloadCnt;
}SERVICE_COUNTER_MAPPER_T;
*/


/*!
  \brief  DAC configuration mapper
*/
typedef struct  DAC_CONFIG_MAPPER_TAG {
							/// calibrated offset
	INT8	offset;
							/// calibrated gain
	INT8	gain;
							/// DAC count at min span point; e.g. loadcell sensor at 0 LB = 0.5V,
	UINT16	wDacCountAtMinSpan;
							/// DAC count at max span point; e.g. loadcell sensor at Capacity = 5V,
	UINT16	wDacCountAtMaxSpan;
							/// sensor value at in span point of DAC output.
	float	fSenorValueAtMinSpan;
							/// sensor value at max span point of DAC output.
	float	fSensorValueAtMaxSpan;
							/// source sensor UNIT
	BYTE	sensorUnit;
							/// source sensor ID
	BYTE	sensorID;
							/// source sensor value type. bit7 1==no filter, bit6:0 value type such as normal, total, peak.
	BYTE	sensorValueType;
							/// DAC config status
	BYTE	DacStatus;
}DAC_CONFIG_MAPPER_T;


/*!
  \brief  virtual sensor math channel configuration mapper.
*/
typedef struct  VS_MATH_CONFIG_MAPPER_TAG {
							/// sensor ID that assigned to this math channel sensor.
	BYTE	vSMathSensorId;
							/// Unit of this math channel acting like a cal unit of physical loadcell. It should not be change once set; it is a reference unit.
	// 2010-08-30 -WFC- BYTE	unit;
							/// math expression for this channel sensor.
	BYTE	mathExprs[ MAX_VS_RAW_EXPRS_SIZE + 1 ];
}VS_MATH_CONFIG_MAPPER_T;

/*!
  \brief  Loadcell operation mode includes Motion detection and total mode configuration mapper. This covered command {1F}, {23} and {24}.
*/
typedef struct LC_TOTAL_MOTION_OPMODE_CONFIG_MAPPER_TAG {
							/// window of time for wait for stable to check for pending total, zero etc....
	BYTE	bStable_Pending_Time;
							/// motion detection band in countby d. 0=0.5 countby. 1=1 d, 2=2d etc...
	BYTE	bMotionDetectBand;
							/// motion detection period. Once it had expired, it will clear motion flag even the loadcell still in motion.
	BYTE	bMotionDetectPeriodTime;

	BYTE	bTotalMinStableTime;
	BYTE	bTotalRiseThresholdPctCap;
	BYTE	bTotalDropThresholdPctCap;
	BYTE	bTotalMode;
	BYTE	bLcOpModes;

	float	fTotalOnAcceptUpperWt;
	float	fTotalOnAcceptLowerWt;
}LC_TOTAL_MOTION_OPMODE_CONFIG_MAPPER_T;

/*!
  \brief  Loadcell operation mode includes Motion detection and total mode configuration mapper. This covered command {1F}, {23} and {24}.
*/
typedef struct LC_STANDARD_MODE_AZM_CONFIG_MAPPER_TAG {
	BYTE	NTEP_pwupZeroBandHi;
	BYTE	NTEP_pwupZeroBandLo;
	BYTE	NTEP_AZM_IntervalTime;
	BYTE	NTEP_AZM_CBRange;
	BYTE	NTEP_pcentCapZeroBandHi;
	BYTE	NTEP_pcentCapZeroBandLo;

	BYTE	OIML_pwupZeroBandHi;
	BYTE	OIML_pwupZeroBandLo;
	BYTE	OIML_AZM_IntervalTime;
	BYTE	OIML_AZM_CBRange;
	BYTE	OIML_pcentCapZeroBandHi;
	BYTE	OIML_pcentCapZeroBandLo;

	BYTE	STD_pwupZeroBandHi;
	BYTE	STD_pwupZeroBandLo;
	BYTE	STD_AZM_IntervalTime;
	BYTE	STD_AZM_CBRange;
	BYTE	STD_pcentCapZeroBandHi;
	BYTE	STD_pcentCapZeroBandLo;
}LC_STANDARD_MODE_AZM_CONFIG_MAPPER_T;

// 2011-07-08 -WFC- v
/*!
  \brief  Print String setting mapper.
*/
typedef struct PRINT_STRING_CONFIG_MAPPER_TAG {
																/// print string control mode
BYTE	gabPrintStringCtrlModeFNV[ MAX_NUM_STREAM_LISTENER ];
																/// print string composite for each listener as defined in command {1A}.
UINT32	gaulPrintStringCompositeFNV[ MAX_NUM_STREAM_LISTENER ];
																/// print string interval in seconds for each listener as defined in command {1A}.
UINT16	gawPrintStringIntervalFNV[ MAX_NUM_STREAM_LISTENER ];
																/// User defined print string formatter
BYTE	gabPrintStringUserFormatterFNV[ PRINT_STRING_MAX_NUM_FORMATER ][ PRINT_STRING_MAX_FORMATER_LENGTH + 1 ];
}PRINT_STRING_CONFIG_MAPPER_T;

#define PRINT_STRING_CONFIG_MAPPER_SIZE		(MAX_NUM_STREAM_LISTENER * ((sizeof(BYTE) +  sizeof(UINT32) + sizeof(UINT16))) + (PRINT_STRING_MAX_NUM_FORMATER * (PRINT_STRING_MAX_FORMATER_LENGTH + 1)))
// 2011-07-08 -WFC- ^


// 2012-04-24 -WFC- v
#define RF_DEVICE_TYPE_XBEE				0
#define RF_DEVICE_TYPE_UNDEFINED		0XFF
#define RF_NETWORK_MODE_PEER_TO_PEER	0
#define RF_NETWORK_MODE_MASTER_SLAVE	1

#define RF_DEVICE_STATUS_ALWAYS_ON_bm		0x04		// Required RF device on even ScaleCore is power off. 2016-03-30 -WFC-
#define RF_DEVICE_STATUS_VALUES_FROM_DEV_bm	0x08		// RF settings are from RF Device, not from user defined.
#define RF_DEVICE_STATUS_QUERY_bm			0x10
#define RF_DEVICE_STATUS_SET_CONFIG_bm		0x20
#define RF_DEVICE_STATUS_INSTALLED_bm		0x40
#define RF_DEVICE_STATUS_ENABLED_bm			0x80

/*!
  \brief RF device configuration settings structure.
  \par overview
   This is an RF device configuration settings structure.
  \note
  This
*/

typedef struct RF_DEVICE_SETTINGS_TAG {
							/// device type, 0==XBee; 0xFF=undefined.
  BYTE		deviceType;
							/// network mode such as peer to peer. Master/Slave etc...
  BYTE		networkMode;
							/// status bits, enabled, installed, need_config,  etc...
  BYTE		status;
							/// network channel
  WORD16	channel;
							/// network ID
  WORD16	networkID;
							/// power level						// 2013-04-02 -DLM-
  WORD16	powerlevel;

}RF_DEVICE_SETTINGS_T;

extern	RF_DEVICE_SETTINGS_T 	gRfDeviceSettingsFNV;

// 2012-04-24 -WFC- ^


// 2012-07-06 -DLM- v
#define ETHERNET_DEVICE_TYPE_DIGI				0
#define ETHERNET_DEVICE_TYPE_UNDEFINED		0XFF

#define ETHERNET_DEVICE_STATUS_VALUES_FROM_DEV_bm	0x08		// RF settings are from RF Device, not from user defined.
#define ETHERNET_DEVICE_STATUS_QUERY_bm			0x10
#define ETHERNET_DEVICE_STATUS_SET_CONFIG_bm		0x20
#define ETHERNET_DEVICE_STATUS_INSTALLED_bm		0x40
#define ETHERNET_DEVICE_STATUS_ENABLED_bm			0x80

/*!
  \brief Ethernet device configuration settings structure.
  \par overview
   This is an Ethernet device configuration settings structure.
  \note
  This
*/

typedef struct ETHERNET_DEVICE_SETTINGS_TAG {
							/// device type, 0==DIGI; 0xFF=undefined.
  BYTE		deviceType;
							/// status bits, enabled, installed, need_config,  etc...
  BYTE		status;
							/// network ID
  //WORD16	networkID;
}ETHERNET_DEVICE_SETTINGS_T;

extern	ETHERNET_DEVICE_SETTINGS_T 	gEthernetDeviceSettingsFNV;

// 2012-07-06 -DLM- ^


// ***********************************************
// ***********************************************
//
//			FRAM memory layout:
//
// ***********************************************
// ***********************************************

#define FRAM_SETPOINT_SIZE		(SETPOINT_MAX_NUM * 3 + SETPOINT_MAX_NUM * sizeof( float))

// Sensor calibration table size included a 16bit CRC.
#define	FRAM_SENSOR_CALTABLE_WITH_CRC_SIZE		(sizeof(SENSOR_CAL_T) + 2)

#define MAX_SCALE_STANDARD_FRAM_SIZE		1

#define	APPLICATION_BOOTLOADER_SHARE_FRAM_BASE_ADDR		0
// the + 1 space is for crc8 value data integrity check.
#define	PRODUCT_INFO_FRAM_BASE_ADDR				(sizeof(APP_BOOT_SHARE_T) + 1)	
#define	SENSOR_FEATURE_FRAM_BASE_ADDR			((sizeof(PRODUCT_INFO_T) + 1) + PRODUCT_INFO_FRAM_BASE_ADDR)	
#define	LISTENER_SETTING_FRAM_BASE_ADDR			(( MAX_NUM_SENSORS * (sizeof( SENSOR_FEATURE_MAPPER_T ) + 1) ) + SENSOR_FEATURE_FRAM_BASE_ADDR)
#define	SETPOINT_FRAM_BASE_ADDR					(( (MAX_NUM_STREAM_LISTENER * 5) + 1 + LISTENER_SETTING_FRAM_BASE_ADDR))
#define	SYSTEM_FEATURE_FRAM_BASE_ADDR			(FRAM_SETPOINT_SIZE + 1 + SETPOINT_FRAM_BASE_ADDR)
#define	TOTAL_STAT_DATA_FRAM_BASE_ADDR			((sizeof(SYSTEM_FEATURE_T) + 1) + SYSTEM_FEATURE_FRAM_BASE_ADDR)
#define	LOADCELL_DYNAMIC_DATA_FRAM_BASE_ADDR	(( MAX_NUM_PV_LOADCELL * (sizeof( TOTAL_STAT_DATA_MAPPER_T ) + 1) ) + TOTAL_STAT_DATA_FRAM_BASE_ADDR)
#define SERVICE_COUNTER_FRAM_BASE_ADDR			(( MAX_NUM_PV_LOADCELL * (sizeof( LOADCELL_DYNAMIC_DATA_MAPPER_T ) + 1) ) + LOADCELL_DYNAMIC_DATA_FRAM_BASE_ADDR)
#define DAC_CONFIG_FRAM_BASE_ADDR				(( MAX_NUM_PV_LOADCELL * (sizeof( SERVICE_COUNTER_MAPPER_T ) + 1) ) + SERVICE_COUNTER_FRAM_BASE_ADDR)
#define VS_MATH_CONFIG_FRAM_BASE_ADDR			(( MAX_DAC_CHANNEL * (sizeof( DAC_CONFIG_MAPPER_T ) + 1) ) + DAC_CONFIG_FRAM_BASE_ADDR)
#define LC_TOTAL_MOTION_OPMODE_FRAM_BASE_ADDR	(( MAX_NUM_VS_MATH * (sizeof( VS_MATH_CONFIG_MAPPER_T ) + 1) ) + VS_MATH_CONFIG_FRAM_BASE_ADDR)
#define LC_STANDARD_MODE_AZM_FRAM_BASE_ADDR		(( MAX_NUM_PV_LOADCELL * (sizeof( LC_TOTAL_MOTION_OPMODE_CONFIG_MAPPER_T ) + 1) ) + LC_TOTAL_MOTION_OPMODE_FRAM_BASE_ADDR)
//#define NEXT..MAPPER_FRAM_BASE_ADDR			(( MAX_NUM_PV_LOADCELL * (sizeof( LC_STANDARD_MODE_AZM_CONFIG_MAPPER_T ) + 1) ) + LC_STANDARD_MODE_AZM_FRAM_BASE_ADDR)
// Sensor name appended a crc16, 2bytes at the end.
#define SENSOR_NAME_FRAM_BASE_ADDR				(( MAX_NUM_PV_LOADCELL * (sizeof( LC_STANDARD_MODE_AZM_CONFIG_MAPPER_T ) + 1) ) + LC_STANDARD_MODE_AZM_FRAM_BASE_ADDR)

#if ( CONFIG_USE_INTERNAL_EEPROM == TRUE )
// when use internal EEPROM, the cal table is saved in EEPROM, else it save in FRAM.
#define SCALE_STANDARD_MODE_FRAM_BASE_ADDR		(( MAX_NUM_SENSORS *( MAX_SENSOR_NAME_SIZE + 1 ) + 2)  + SENSOR_NAME_FRAM_BASE_ADDR)
#else
#define SENSOR_CAL_TABLES_FRAM_BASE_ADDR		(( MAX_NUM_SENSORS *( MAX_SENSOR_NAME_SIZE + 1 ) + 2)  + SENSOR_NAME_FRAM_BASE_ADDR)
#define SCALE_STANDARD_MODE_FRAM_BASE_ADDR		(  CAL_MAX_NUM_CAL_TABLE * (FRAM_SENSOR_CALTABLE_WITH_CRC_SIZE) + SENSOR_CAL_TABLES_FRAM_BASE_ADDR)
#define PRINT_STRING_CONFIG_FRAM_BASE_ADDR		( MAX_SCALE_STANDARD_FRAM_SIZE + 1 + SCALE_STANDARD_MODE_FRAM_BASE_ADDR )	// note it use CRC16
#define RF_DEVICE_CONFIG_FRAM_BASE_ADDR			( PRINT_STRING_CONFIG_MAPPER_SIZE + 2 + PRINT_STRING_CONFIG_FRAM_BASE_ADDR )	// 2012-04-20 -WFC-
#define ETHERNET_DEVICE_CONFIG_FRAM_BASE_ADDR	( sizeof(RF_DEVICE_SETTINGS_T) + 2 + RF_DEVICE_CONFIG_FRAM_BASE_ADDR )	// 2012-07-06 -DLM-
//#define NEXT...FRAM_BASE_ADDR					( sizeof(ETHERNET_DEVICE_SETTINGS_T) + 2 + ETHERNET_DEVICE_CONFIG_FRAM_BASE_ADDR )	// 2012-07-06 -DLM-
#endif
//#define NEXT..MAPPER_FRAM_BASE_ADDR			( PRINT_STRING_CONFIG_MAPPER_SIZE + 2 + PRINT_STRING_CONFIG_FRAM_BASE_ADDR)

/// configuration format version
#define CNFG_FORMAT_VERSION				1

#if ( CONFIG_USE_INTERNAL_EEPROM == TRUE )
#define	EEMEM_SENSOR_CALTABLE_WITH_CRC_SIZE		(sizeof(SENSOR_CAL_T) + 2)
#endif

/// app boot share rec read from FRAM.
extern	APP_BOOT_SHARE_T 	gAppBootShareNV;		// app boot share rec read from FRAM.

///	Product ID and version ID are use for configure hardware settings such as which loadcell attached to which ADC inputs etc...
extern	PRODUCT_INFO_T		gtProductInfoFNV;

/// systems feature which contains user key function, auto off, etc...
extern	SYSTEM_FEATURE_T	gtSystemFeatureFNV;

// app and bootloader share memory include an 8bit CRC.
// extern	BYTE	EEMEM	gabEEMEMAppBootShareCRC8[ sizeof(APP_BOOT_SHARE_T) + 1];
// extern	APP_BOOT_SHARE_T	EEMEM	gEEMEMAppBootShare;		// app boot share in EEMEM.

/// product info include an 8bit CRC.
// extern	BYTE	EEMEM	gaSensorCalEEM16c[ CAL_MAX_NUM_CAL_TABLE * EEMEM_SENSOR_CALTABLE_WITH_CRC_SIZE ];

#if ( CONFIG_USE_INTERNAL_EEPROM == TRUE )
// init crc 0 for backward compatible with old boot loader. Maxim iButton convention init crc to 0 is flaw, because 0 crc on array of 0 is still 0.
BYTE	nv_cnfg_eemem_save_with_init0_8bitCRC( void *pDest, const void *pSrc, BYTE nSize );
BYTE	nv_cnfg_eemem_recall_with_init0_8bitCRC( void *pDest, const void *pSrc, BYTE nSize );

BYTE	nv_cnfg_eemem_save_with_8bitCRC( void *pDest, const void *pSrc, BYTE nSize );
BYTE	nv_cnfg_eemem_recall_with_8bitCRC( void *pDest, const void *pSrc, BYTE nSize );
BYTE	nv_cnfg_eemem_save_with_16bitCRC( void *pDest, const void *pSrc, UINT16 nSize );
BYTE	nv_cnfg_eemem_recall_with_16bitCRC( void *pDest, const void *pSrc, UINT16 nSize );
#endif

BYTE 	nv_cnfg_fram_save_with_8bitCRC( UINT16 destAddr, const void *pSrc, BYTE nSize );
BYTE	nv_cnfg_fram_recall_with_8bitCRC( void *pDest, UINT16 sourceAddr, BYTE nSize );

// init crc 0 for backward compatible with old boot loader. Maxim iButton convention init crc to 0 is flaw, because 0 crc on array of 0 is still 0.
// BYTE	nv_cnfg_fram_save_with_init0_8bitCRC( UINT16 destAddr, const void *pSrc, BYTE nSize );
// BYTE	nv_cnfg_fram_recall_with_init0_8bitCRC( void *pDest, UINT16 sourceAddr, BYTE nSize );


BYTE	nv_cnfg_eemem_save_a_cal_table( BYTE calTableNum );
BYTE	nv_cnfg_eemem_save_a_sensor_cal_info( BYTE sensorID, BYTE calTableNum );

BYTE	nv_cnfg_eemem_save_scale_standard( void );
BYTE	nv_cnfg_eemem_recall_scale_standard( void );

BYTE	nv_cnfg_fram_save_all_listeners_settings(void);
BYTE	nv_cnfg_fram_recall_all_listeners_settings(void);

BYTE	nv_cnfg_fram_save_all_setpoints(void);
BYTE	nv_cnfg_fram_recall_all_setpoints(void);

BYTE	nv_cnfg_fram_save_totaling_statistics( BYTE lc );
BYTE	nv_cnfg_fram_recall_totaling_statistics( BYTE lc );
void	nv_cnfg_fram_default_totaling_statistics( BYTE lc );

BYTE	nv_cnfg_fram_save_loadcell_dynamic_data( BYTE lc );
BYTE	nv_cnfg_fram_recall_loadcell_dynamic_data( BYTE lc );
void	nv_cnfg_fram_default_loadcell_dynamic_data( BYTE lc );

BYTE	nv_cnfg_fram_save_service_counters( BYTE lc );
BYTE	nv_cnfg_fram_recall_service_counters( BYTE lc );
void	nv_cnfg_fram_default_service_counters( BYTE lc );

BYTE	nv_cnfg_fram_save_a_sensor_feature( BYTE sensorID );

BYTE	nv_cnfg_fram_recall_dac_config( BYTE channel );
BYTE	nv_cnfg_fram_save_dac_config( BYTE channel );
void	nv_cnfg_fram_default_dac_config( BYTE channel );

BYTE	nv_cnfg_fram_recall_vs_math_config( BYTE channel );
BYTE	nv_cnfg_fram_save_vs_math_config( BYTE channel );
// void	nv_cnfg_fram_default_vs_math_config( BYTE channel );

BYTE	nv_cnfg_fram_recall_lc_total_motion_opmode_config( BYTE lc );
BYTE	nv_cnfg_fram_save_lc_total_motion_opmode_config( BYTE lc );

BYTE	nv_cnfg_fram_recall_lc_standard_mode_azm_config( BYTE lc );
BYTE	nv_cnfg_fram_save_lc_standard_mode_azm_config( BYTE lc );

BYTE	nv_cnfg_fram_recall_with_16bitCRC( void *pDest, const void *pSrc, UINT16 nSize );
BYTE	nv_cnfg_fram_save_with_16bitCRC( void *pDest, const void *pSrc, UINT16 nSize );

BYTE	nv_cnfg_fram_recall_all_sensor_names( void );
BYTE	nv_cnfg_fram_save_all_sensor_names( void );

void  nv_cnfg_fram_default_a_sensor_viewing_cb( BYTE sensorID );  // 2010-09-10 -WFC-

BYTE  nv_cnfg_fram_save_print_string_config( void );		// 2011-07-08 -WFC-
BYTE  nv_cnfg_fram_recall_print_string_config( void );		// 2011-07-08 -WFC-

BYTE  nv_cnfg_fram_save_rf_device_cnfg( void );				// 2012-04-20 -WFC-
BYTE  nv_cnfg_fram_recall_rf_device_cnfg( void );			// 2012-04-20 -WFC-
void  nv_cnfg_fram_default_rf_device_cnfg( void );			// 2012-04-20 -WFC-

#endif	// end MSI_NV_CNFG_MEM_H

//@}
