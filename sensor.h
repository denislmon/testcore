/*! \file sensor.h \brief sensors related functions.*/
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
//  History:  Created on 2007/08/01 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup sensor Sensor related functions (sensor.c)
/// \code #include "sensor.h" \endcode
/// \par Overview
///   It is a high level module manages all aspect of different type of sensors.
///   It knows the high level work flow and delegates the detail tasks to a specific sensor type module.
///   It has an abstract sensor descriptors prescribed what type of ADC chip, input selection,
///   conversion speed and sensor filter algorithm.  Each specific sensor ADC value is filled by sensor filter algorithm
///   which will be call by its ADC module. The sensor value is also supplied by the specified sensor module.
///   The descriptor contains an abstract device pointer pDev to a specific sensor type data structure, e.g. LOADCELL_T,
///   VOLTAGEMON_T etc... This makes easy for sensor module to compute the value and format output data.
///   It has method to initialize all sensor types based on gabSensorTypeFNV[], gabSensorFeaturesFNV[] and
///   gabSensorSpeedFNV[].
///
///  \note
///  The Scalecore can have up to 4 local physical loadcells and 1 virtual math type
///  loadcell. Only the first two sensors (sensor0 and sensor1) are for loadcells with
///  software temperature compensation.
///  Sensors 0 to 3 are loadcells.
///  Sensors 4 is dedicate for virtual math type loadcell.
///  Sensors 5 to 7 are voltage monitors.
///  Sensor 8 is a temperature sensor. Sensor 9 is a light sensor.
///  Note that sensor 8 and 9 do not require calibration.
///
///  Sensor 10 to 15 are virtual sensors such as remote sensors, math sensors, etc..
///  For remote sensor, it will not allow to calibrate with remote sensor ID;
///  instead, user must uses device ID and local sensor ID with cal commands and
///  talk to remote device directly ( may be indirectly by router).
///  
///  The first sensor loadcell will get temperature data from sensor 7 by default.
///  Each software temperature compensated loadcell will have 3 calibration tables corresponding
///  to 3 different temperature level.
///
/// Overload Threshold weight values will be saved in the same unit as the cal unit.
/// If the user enter the value in unit different than cal unit, it will
/// convert entered value to cal unit and then saved it; these values have suffix of NV.
///
/// The tare, zero, total, Sum square total etc... weight values are saved in display unit;
/// these values have the suffix of FNV.
//
// ****************************************************************************
//@{

/* Obsolete OLD configuration.
  The Scalecore can have up to 4 loadcells. Only the first two sensors
  (sensor0 and sensor1) are for loadcells with software temperature compensation.
  Sensors 0 to 3 are loadcells. Sensors 4 to 6 are battery monitor.
  Sensor 7 is a temperature sensor. Sensor 8 is a light sensor.
  Note that sensor 7 and 8 do not require calibration.

  Sensor 9 to 15 are virtual sensors such as remote sensors, math sensors, etc..
  For remote sensor, it will not allow to calibrate with remote sensor ID;
  instead, user must uses device ID and local sensor ID with cal commands and
  talk to remote device directly ( may be indirectly by router).

  The first sensor loadcell will get temperature data from sensor 7 by default.
  Each software temperature compensated loadcell will have 3 calibration tables corresponding
  to 3 different temperature level.
*/


#ifndef MSI_SENSOR_H
#define MSI_SENSOR_H


#include  "config.h"
#include  "loadcell.h"
#include  "calibrate.h"
#include  "vs_math.h"

///  The Scalecore can have up to 4 local physical loadcells and 1 virtual math type
//   loadcell. Only the first two sensors (sensor0 and sensor1) are for loadcells with
///  software temperature compensation.
///  Sensors 0 to 3 are loadcells.
///  Sensors 4 is dedicate for virtual math type loadcell.
///  Sensors 5 to 7 are voltage monitors.
///  Sensor 8 is a temperature sensor. Sensor 9 is a light sensor.
///  Note that sensor 8 and 9 do not require calibration.
///
///  Sensor 10 to 15 are virtual sensors such as remote sensors, math sensors, etc..

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2012-02-17 -WFC- v

/*!
  \brief lay out of the sensors of ScaleCore.
  The assignment of physical local sensors and virtual math type loadcell are fixed.
  Remote Meter does not have local physical loadcell.
*/

/// MAX NUMBER OF LOCAL SENSORS, first 4 are virtual loadcells, 5th is math type loadcell, next 3 are voltages sensors, follow by a on board temperature and a light sensor.
// #define MAX_NUM_LSENSORS  		3
// But we have to stick with the convention as first 4 are local physical loadcells, 5th is math type loadcell, next 3 are voltages sensors, follow by a on board temperature and a light sensor.
// for backward compatibility of source codes and host commands. However, other than local voltage, light and temperature sensors, we can
// assigned local sensor into virtual or remote sensors by modified gcabDefaultSensorType[] and calling sensor_descriptor_init();
#define MAX_NUM_LSENSORS  		10

/// MAX NUMBER OF VIRTUAL SENSORS such as math, remote etc...
#define MAX_NUM_VSENSORS		6

/// MAX NUMBER OF Remote loadcell
#define MAX_NUM_REMOTE_LOADCELL		4

/// Total number of sensors include both local physical and virtual sensors.
// #define MAX_NUM_SENSORS  		(MAX_NUM_LSENSORS + MAX_NUM_VSENSORS)
#define MAX_NUM_SENSORS  		16

/// MAX NUMBER OF CAL-ABLE SENSORS, first 4 are loadcellS, last 3 are voltages sensor.
// 2010-11-02 -WFC- #define MAX_NUM_CAL_SENSORS		7

/// MAX NUMBER OF CAL-ABLE SENSORS should be 0 for Remote Meter, but for software system to work, it must be 1. Other peripheral sensor will deal with different data structures in future version.  2012-02-17 -WFC-
#define MAX_NUM_CAL_SENSORS		1

/// SENSOR NUMBER OF THE FIRST LOADCELL
#define SENSOR_NUM_1ST_LOADCELL			0

/// SENSOR NUMBER OF THE LAST LOADCELL
#define SENSOR_NUM_LAST_LOADCELL		0

// 2011-05-19 -WFC- #if ( CONFIG_PCB_AS == CONFIG_PCB_AS_SCALECORE2)
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-05-19 -WFC-
/// SENSOR NUMBER OF THE Rcal input for ScaleCore2 only.
#define SENSOR_NUM_RCAL					3
#endif

/// SENSOR NUMBER OF THE MATH LOADCELL
#define SENSOR_NUM_MATH_LOADCELL		4

/// SENSOR NUMBER OF THE LAST PV_LOADCELL
#define SENSOR_NUM_LAST_PV_LOADCELL		4

/// SENSOR NUMBER OF THE FIRST VOLTAGE MONITOR
#define SENSOR_NUM_1ST_VOLTAGEMON		5

/// SENSOR NUMBER OF THE LAST VOLTAGE MONITOR
#define SENSOR_NUM_LAST_VOLTAGEMON		7

/// SENSOR NUMBER OF THE ON BOARD TEMPERATURE SENSOR
#define SENSOR_NUM_PCB_TEMPERATURE		8

/// SENSOR NUMBER OF THE LIGHT SENSOR
#define SENSOR_NUM_LIGHT_SENSOR			9

#define MAX_SENSOR_NAME_SIZE	20

/// NO SUCH SENSOR ID or NUMBER
#define	SENSOR_NO_SUCH_ID		0xFF

/// SENSOR NUMBER OF THE INPUT VOLTAGE MONITOR
#define SENSOR_NUM_INPUT_VOLTAGEMON		7			// 2011-04-07 -WFC-

BYTE sensor_get_remote_loadcell_array_index( BYTE sensorID );	// 2012-02-20 -WFC-

#else
/*!
  \brief lay out of the sensors of ScaleCore.
  The assignment of physical local sensors and virtual math type loadcell are fixed.
*/

/// MAX NUMBER OF LOCAL SENSORS, first 4 are local physical loadcells, 5th is math type loadcell, next 3 are voltages sensors, follow by a on board temperature and a light sensor.
#define MAX_NUM_LSENSORS  		10

/// MAX NUMBER OF VIRTUAL SENSORS such as math, remote etc...
#define MAX_NUM_VSENSORS		6

/// Total number of sensors include both local physical and virtual sensors.
#define MAX_NUM_SENSORS  		(MAX_NUM_LSENSORS + MAX_NUM_VSENSORS)

/// MAX NUMBER OF CAL-ABLE SENSORS, first 4 are loadcellS, last 3 are voltages sensor.
// 2010-11-02 -WFC- #define MAX_NUM_CAL_SENSORS		7

// 2010-10-02 -WFC- v
/// MAX NUMBER OF CAL-ABLE SENSORS, first 4 are loadcells ONLY. Other peripheral sensor will deal with different data structures in future version.  2010-11-02 -WFC-
#if ( CAL_MAX_NUM_CAL_TABLE == 8)
#define MAX_NUM_CAL_SENSORS		4
#elif ( CAL_MAX_NUM_CAL_TABLE == 9)
#define MAX_NUM_CAL_SENSORS		7
#else
#define MAX_NUM_CAL_SENSORS		4
#endif
// 2010-10-02 -WFC- ^


/// SENSOR NUMBER OF THE FIRST LOADCELL
#define SENSOR_NUM_1ST_LOADCELL			0

/// SENSOR NUMBER OF THE LAST LOADCELL
#define SENSOR_NUM_LAST_LOADCELL		3

// 2011-05-19 -WFC- #if ( CONFIG_PCB_AS == CONFIG_PCB_AS_SCALECORE2)
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-05-19 -WFC-
/// SENSOR NUMBER OF THE Rcal input for ScaleCore2 only.
#define SENSOR_NUM_RCAL					3
#endif

/// SENSOR NUMBER OF THE MATH LOADCELL
#define SENSOR_NUM_MATH_LOADCELL		4

/// SENSOR NUMBER OF THE LAST PV_LOADCELL
#define SENSOR_NUM_LAST_PV_LOADCELL		4

/// SENSOR NUMBER OF THE FIRST VOLTAGE MONITOR
#define SENSOR_NUM_1ST_VOLTAGEMON		5

/// SENSOR NUMBER OF THE LAST VOLTAGE MONITOR
#define SENSOR_NUM_LAST_VOLTAGEMON		7

/// SENSOR NUMBER OF THE ON BOARD TEMPERATURE SENSOR
#define SENSOR_NUM_PCB_TEMPERATURE		8

/// SENSOR NUMBER OF THE LIGHT SENSOR
#define SENSOR_NUM_LIGHT_SENSOR			9

#define MAX_SENSOR_NAME_SIZE	20

/// NO SUCH SENSOR ID or NUMBER
#define	SENSOR_NO_SUCH_ID		0xFF

/// SENSOR NUMBER OF THE INPUT VOLTAGE MONITOR
// 2012-03-13 -WFC- #define SENSOR_NUM_INPUT_VOLTAGEMON		7			// 2011-04-07 -WFC-
#define SENSOR_NUM_INPUT_VOLTAGEMON		5			// 2012-03-13 -WFC- it directly connected to a battery.

#define SENSOR_NUM_INPUT_VOLTAGEMON_BATT1	5		// 2012-05-03 -DLM-
#define SENSOR_NUM_INPUT_VOLTAGEMON_BATT2	6		// 2012-03-03 -DLM-

#endif // ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2012-02-17 -WFC- ^

/// sensor value type bit5 to bit0; bit7=non filtered; bit6= update bargraph.
#define SENSOR_VALUE_TYPE_GROSS			0
#define SENSOR_VALUE_TYPE_NET			1
#define SENSOR_VALUE_TYPE_TOTAL			2
#define SENSOR_VALUE_TYPE_TARE			3
#define SENSOR_VALUE_TYPE_ZERO			4
#define SENSOR_VALUE_TYPE_PEAK			5
#define SENSOR_VALUE_TYPE_ADC_COUNT		6
#define SENSOR_VALUE_TYPE_CUR_MODE		7
#define SENSOR_VALUE_TYPE_TOTAL_COUNT   8				// 2011-06-23 -WFC-
#define SENSOR_VALUE_TYPE_MAX			8
#define SENSOR_VALUE_TYPE_FILTERED		0
#define SENSOR_VALUE_TYPE_CAL_BARGRAPH_bm	0x40
#define SENSOR_VALUE_TYPE_NON_FILTERED_bm	0x80
#define SENSOR_VALUE_TYPE_MASK_bm		0x3F
#define SENSOR_VALUE_UNIT_CUR_MODE		0x3F


#define SENSOR_TYPE_LOADCELL			0
#define	SENSOR_TYPE_VOLTMON				1
#define	SENSOR_TYPE_CURRENT				2
#define	SENSOR_TYPE_TEMP				3
#define	SENSOR_TYPE_LIGHT				4
// 2012-02-17 -WFC- #define	SENSOR_TYPE_REMOTE				5
#define	SENSOR_TYPE_REMOTE_LOADCELL		5			// 2012-02-17 -WFC-
#define	SENSOR_TYPE_MATH_LOADCELL		6
#define	SENSOR_TYPE_RCAL				7
#define	SENSOR_TYPE_REMOTE_VOLT			8			// 2012-02-17 -WFC-
#define	SENSOR_TYPE_REMOTE_TEMP			9			// 2012-02-17 -WFC-
#define	SENSOR_TYPE_UNDEFINED		 0xFF

// UINT8	gabSensorShowCBunitsFNV[ MAX_NUM_SENSORS ];	// current unit of a channel. 0==LBS, 1==KG, 2==TON, 3==METRIC_TON, 4==OZ, 5==GRAMS, 6==C,  7== F.

#define	SENSOR_UNIT_LB		0
#define	SENSOR_UNIT_KG		1
#define	SENSOR_UNIT_TON		2
#define	SENSOR_UNIT_MTON	3
#define	SENSOR_UNIT_OZ		4
#define	SENSOR_UNIT_GRAM	5
#define	SENSOR_UNIT_KNWT	6
#define	SENSOR_UNIT_MAX_W	7
#define	SENSOR_UNIT_V		7
#define	SENSOR_UNIT_AMP		8
#define	SENSOR_UNIT_MAX_E	9
#define	SENSOR_UNIT_TMPC	9
#define	SENSOR_UNIT_TMPF	10
#define	SENSOR_UNIT_TMPK	11
#define	SENSOR_UNIT_LIGHT	12
#define	SENSOR_UNIT_MAX		13


/// for Sensor Features Enable flags gabSensorFeaturesFNV and Sensor descriptor conversion_cnfg's upper 4bits.
#define		SENSOR_FEATURE_SENSOR_ENABLE	0X80
#define		SENSOR_FEATURE_AC_EXCITATION	0X40
#define		SENSOR_FEATURE_TEMPERATURE_CMP	0X20
#define		SENSOR_FEATURE_GLOBAL_REF_V		0X10
#define		SENSOR_FEATURE_REF_V_MASK		0X18
#define		SENSOR_FEATURE_ADC_CPU_VREF_1P1V	0X10
#define		SENSOR_FEATURE_ADC_CPU_VREF_2P56V	0X18

/// Sensor configuration cnfg
#define		SENSOR_CNFG_ENABLED				0X80
#define		SENSOR_CNFG_FILTER_LEVEL_MASK	0X03
#define		SENSOR_CNFG_FILTER_DISABLED		0
#define		SENSOR_CNFG_FILTER_LEVEL_LOW	1
#define		SENSOR_CNFG_FILTER_LEVEL_MEDIUM	2
#define		SENSOR_CNFG_FILTER_LEVEL_HIGH	3
#define		SENSOR_CNFG_REF_V_MASK			0X18
#define		SENSOR_CNFG_SETTING_MASK		0X9F

// Bit7 enabled, Bit6 got ADC count, Bit5 got value in current unit, Bit4-3 reserved, Bit1-0 filter level setting, 0=disabled. Status has some redundant info, it is designed to speed up excution and smaller code size.
/// Sensor descriptor status

// 2011-03-28 -WFC- v
#define		SENSOR_STATUS_GOT_ADC_CNT			0X80
#define		SENSOR_STATUS_GOT_VALUE				0X40
#define		SENSOR_STATUS_GOT_UNFILTER_ADC_CNT	0X20
#define		SENSOR_STATUS_GOT_UNFILTER_VALUE	0X10
#define		SENSOR_STATUS_GOT_NEW_ADC_PEAK		0X08		// -WFC- 2011-03-28
#define		SENSOR_STATUS_GOT_DATA_MASK			0XF8
// 2011-03-28 -WFC- ^


// bit7: 1==in cal, bit 6 to 4 reserved, bit3 to 0, state code.
#define  SENSOR_STATE_DISABLED				0
#define  SENSOR_STATE_WRONG_INDEX			1
#define  SENSOR_STATE_UNCAL					2
#define  SENSOR_STATE_ADC_BUSY				3
#define	 SENSOR_STATE_WRONG_MATH_EXPRESSION	4
// displayable state
#define  SENSOR_STATE_GOT_VALUE_IN_CAL		5
#define  SENSOR_STATE_UNDER_RANGE			6
#define  SENSOR_STATE_OVER_RANGE			7
#define  SENSOR_STATE_UNDERLOAD				8
#define  SENSOR_STATE_OVERLOAD				9
#define  SENSOR_STATE_GOT_VALID_VALUE		10
#define  SENSOR_STATE_MAX_FOR_DISPLAYABLE	SENSOR_STATE_GOT_VALID_VALUE

#define  SENSOR_STATE_IN_CAL_bm				0x80

// displayable state OBSOLETE OLD DEFINED
//#define  SENSOR_STATE_GOT_VALUE_IN_CAL		4
//#define  SENSOR_STATE_UNDER_RANGE			5
//#define  SENSOR_STATE_OVERLOAD				6
//#define  SENSOR_STATE_GOT_VALID_VALUE		7
//#define  SENSOR_STATE_UNDERLOAD				8
//#define  SENSOR_STATE_MAX_FOR_DISPLAYABLE	SENSOR_STATE_UNDERLOAD
//
//#define  SENSOR_STATE_IN_CAL_bm				0x80
//#define	 SENSOR_STATE_WRONG_MATH_EXPRESSION	10

/// Sensor descriptor chip ID
#define SENSOR_CHIP_ID_UNDEFINED				0
#define SENSOR_CHIP_CPU_ADC						1
#define SENSOR_CHIP_CPU_ADC_A					SENSOR_CHIP_CPU_ADC
#define SENSOR_CHIP_ID_ADC_LTC2447				2
#define SENSOR_CHIP_CPU_ADC_B					3

/// Sensor descriptor hookup_cnfg bit2..0 is channel number
#define	SENSOR_HOOKUP_SINGLE_END		0X80
#define	SENSOR_HOOKUP_CHANNEL_MASK		0X07

/// Sensor descriptor conversion_cnfg
/// Sensro descriptor conversion speed mask
#define SENSOR_CNV_CNFG_SPEED_MASK		0X0F
#define SENSOR_CNV_CNFG_FEATURE_MASK	0XE0


/// sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4 Global reference,1=enabled. Bit1-0 filter level, 0==disabled.
#define SENSOR_FEATURE_DEFAULT	(SENSOR_FEATURE_SENSOR_ENABLE |SENSOR_FEATURE_GLOBAL_REF_V)

/// sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4 Global reference,1=enabled. Bit1-0 filter level, 0==disabled.
#define SENSOR_FEATURE_LOADCELL_DEFAULT	(SENSOR_FEATURE_SENSOR_ENABLE |SENSOR_FEATURE_GLOBAL_REF_V | SENSOR_CNFG_FILTER_LEVEL_LOW )		// 2011-05-07 -WFC-

/// sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4 Global reference,1=enabled. Bit1-0 filter level, 0==disalbed.
#define SENSOR_FEATURE_DEFAULT_DISABLED	(SENSOR_FEATURE_GLOBAL_REF_V)		// -WFC- 2011-03-18

/// sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4,3 reference voltage, Bit1-0 filter level, 0==disabled.
#define SENSOR_FEATURE_VOLTMON_DEFAULT	(SENSOR_FEATURE_SENSOR_ENABLE |SENSOR_FEATURE_ADC_CPU_VREF_1P1V )

/// sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4,3 reference voltage, Bit1-0 filter level, 0==disabled.
#define SENSOR_FEATURE_TEMP_DEFAULT	(SENSOR_FEATURE_SENSOR_ENABLE |SENSOR_FEATURE_ADC_CPU_VREF_1P1V )

/// sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4,3 reference voltage, Bit1-0 filter level, 0==disabled.
#define SENSOR_FEATURE_MATH_LOADCELL_DEFAULT	(SENSOR_FEATURE_SENSOR_ENABLE)

/// sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4,3 reference voltage, Bit1-0 filter level, 0==disabled.
#define SENSOR_FEATURE_MATH_DEFAULT	(SENSOR_FEATURE_SENSOR_ENABLE)


/// Generic ADC status use by adc_lt, adc_cpu and other adc modules.
#define ADC_STATUS_ENABLED			0X80
#define ADC_STATUS_AC_EXCITE		0X40
#define ADC_STATUS_AC_EXCITE_STATE	0X20

/// This is for Load cell inputs configuration of an ADC chip.
#if ( CONFIG_PRODUCT_AS	>= CONFIG_AS_DSC )
#define	LOADCELL_NUM_OF_SIG1	2
#define	LOADCELL_NUM_OF_SIG2	1
#define	LOADCELL_NUM_OF_SIG3	0
#define	LOADCELL_NUM_OF_SIG4	3

/// channel number corresponding to load cell number
#define	CHANNEL_NUM_OF_LOADCELL_0	2
#define	CHANNEL_NUM_OF_LOADCELL_1	1
#define	CHANNEL_NUM_OF_LOADCELL_2	0
#define	CHANNEL_NUM_OF_LOADCELL_3	3

/// For iDSC: // 2012-07-25 -DLM-
/*
#define	CHANNEL_NUM_OF_LOADCELL_0	1
#define	CHANNEL_NUM_OF_LOADCELL_1	2
#define	CHANNEL_NUM_OF_LOADCELL_2	3
#define	CHANNEL_NUM_OF_LOADCELL_3	0
*/
#else

#define	LOADCELL_NUM_OF_SIG1	2
#define	LOADCELL_NUM_OF_SIG2	1
#define	LOADCELL_NUM_OF_SIG3	0
#define	LOADCELL_NUM_OF_SIG4	3

/// channel number corresponding to load cell number
#define	CHANNEL_NUM_OF_LOADCELL_0	2
#define	CHANNEL_NUM_OF_LOADCELL_1	1
#define	CHANNEL_NUM_OF_LOADCELL_2	0
#define	CHANNEL_NUM_OF_LOADCELL_3	3

#endif


/*!
  \brief generic sensor data structure.
*/
typedef struct  SENSOR_TAG {
							/// sensor type of this channel, local and remote physical, local and remote virtual as math etc..
	BYTE	type;
							/// generic pointer to a specific type sensor data structure or object such as loadcell, remote, math channel, etc...
	void	*pSensor;
	
}SENSOR_T;



/*!
  \brief Sensor descriptor specified ADC chip and its connection configuration of a sensor.
   It also specified the filter algorithm of the sensor. An ADC module just use this
   information to run its ADC and call the supplied filter. Since the ADC module
   called the filter method after every reading, this enabled asynchronously high speed reading.
   
   \note This must initialize by the sensor module before it can be use.
    It should be re-initialize every time the related parameter had changed.
    The ADC module does not care how the sensor hookup,
    it just acts on the SENSOR DESCRIPTOR supplied by the sensor module.
	The sensor module determine how and which ADC chip to connect.
	Sensor module must initialized first before other ADC modules perform initialization.
*/
typedef struct  LSENSOR_DESCRIPTOR_TAG {
							/// points to name string buffer.
	BYTE    *pName;
							/// sensor type such as loadcell, temperature sensor, etc..
	BYTE    type;
							/// specified which chip (ADC, temp sensor, etc) is use by this sensor. 0 == undefined.
	BYTE    chipID;
							/// hookup configuration, Bit7 is Single-end/Differential, 1=single-end.  Bit6~4 reserved, Bit3..0 channel number.
	BYTE	hookup_cnfg;
							/// conversion configuration, Bit7 enabled, Bit6 1==AC excitation, Bit 5 temperature compensation, Bit4 reserved, bit3-0; conversion speed. 1==fast to 15==very slow.
	BYTE	conversion_cnfg;
							/// Bit7 enabled, Bit6-5 reserved, Bit4-3 voltage reference mode, Bit2-0 filter level setting, 0=disabled. Status has some redundant info, it is designed to speed up excution and smaller code size. Bit7 is mainly for hi level op, while Bit7 in conversion_cnfg is for hardware module use.
	BYTE	cnfg;
							/// Bit7 got ADC count, Bit6 got value in current unit, Bit5-0 reserved.
	BYTE	status;
							/// previous sleep cycle ADC count, it could be a filtered ADC count depends on filter setting.	2014-09-16 -WFC-
	INT32	prvSleepCycleADCcount;
							/// previous ADC count, it could be a filtered ADC count depends on filter setting.
	INT32	prvADCcount;	
							/// current ADC count, it could be a filtered ADC count depends on filter setting.
	INT32	curADCcount;	
							/// value of a sensor converted from ADC count to a standard value such as LB, Celsius, Kelvin etc..
	float	value;	
							/// points to the actual device structure such as loadcell, temperature etc..
	void	*pDev;	
							/// filter algorithm supplied by other type sensor module ( loadcell, temperature, etc..), which will call by specified ADC module after every reading.
	BYTE    ( *gapfSensorFilterMethod) ( BYTE sensorN, BYTE filterCnfg, INT32 sample, INT32 *pFilteredData );
							/// raw ADC data included status of the chip. It is for debugging ADC chip.
	INT32	rawAdcData;
							/// previous raw ADC count. Un-filtered
	INT32	prvRawADCcount;	
							/// current raw ADC count. Un-filtered
	INT32	curRawADCcount;	
							/// maximum raw ADC count. Un-filtered -WFC- 2011-03-28
	INT32	maxRawADCcount;
							/// time require to sample before say it is good.	2011-05-04 -WFC-
	TIMER_T	filterTimer;	// 	time require to sample before say it is good. 2011-05-04 -WFC-
	#if ( CONFIG_DEBUG_ADC == TRUE )
	UINT16	numSamples;		//  test only 2011-08-05
	#endif
}LSENSOR_DESCRIPTOR_T;

extern LSENSOR_DESCRIPTOR_T	gaLSensorDescriptor[ MAX_NUM_LSENSORS ];

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2012-03-21 -WFC-
extern SENSOR_T	gaSensor[ MAX_NUM_SENSORS ];				// 2012-03-21 -WFC-
#endif

/// the following will recall from nonevolatile memory during powerup. the following are countby for display or showing value to the outside world.
extern	INT8	gabPcentCapUnderloadFNV[ MAX_NUM_SENSORS ];
extern	float	gafSensorShowCapacityFNV[ MAX_NUM_SENSORS ];
extern	float	gafSensorShowCBFNV[ MAX_NUM_SENSORS ];		// float type of countby
extern	UINT16	gawSensorShowCBFNV[ MAX_NUM_SENSORS ];		// integer type of countby
extern	INT8    gabSensorShowCBdecPtFNV[ MAX_NUM_SENSORS ];
extern	UINT8	gabSensorShowCBunitsFNV[ MAX_NUM_SENSORS ];	// countby unit of a sensor. 0==LBS, 1==KG, 2==TON, 3==METRIC_TON, 4==OZ, 5==GRAMS, 6==C,  7== F.
extern	UINT8	gabSensorViewUnitsFNV[ MAX_NUM_SENSORS ];	// current viewing unit of a sensor. 0==LBS, 1==KG, 2==TON, 3==METRIC_TON, 4==OZ, 5==GRAMS, 6==C,  7== F.  -WFC- 2010-08-30

extern	BYTE	gabSensorTypeFNV[ MAX_NUM_SENSORS ];
extern	BYTE	gabSensorSpeedFNV[ MAX_NUM_SENSORS ];	//  Sensor conversion speed range 1 to 15. Smaller value faster the speed and more noise.

/// Sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4-3 voltage reference. Bit2 reserved, Bit1-0 filter level, 0==disabled.
extern	BYTE	gabSensorFeaturesFNV[ MAX_NUM_SENSORS ];	//  sensor status, Bit7 enabled; Bit6 AC excitation, Bit 5 temperature compensation, (Bit4 Global reference,1=enabled) Bit4-3 voltage reference. Bit2 reserved, Bit1-0 filter level, 0==disabled.

												/// Sensor names
extern	BYTE	gabSensorNameFNV[ MAX_NUM_SENSORS ][ MAX_SENSOR_NAME_SIZE + 1 ];

/// value of a sensor converted from ADC count to a standard value such as LB, Celsius, Kelvin etc..
extern	float	gafSensorValue[ MAX_NUM_SENSORS ];

extern	const char gcStrLB[]    PROGMEM;
extern	const char gcStrKG[]    PROGMEM;
extern	const char gcStrTON[]   PROGMEM;
extern	const char gcStrMTON[]  PROGMEM;
extern	const char gcStrOZ[]    PROGMEM;
extern	const char gcStrGRAM[]  PROGMEM;
extern	const char gcStrV[]     PROGMEM;
extern	const char gcStrAMP[]   PROGMEM;
extern	const char gcStrTMPC[]  PROGMEM;
extern	const char gcStrTMPF[]  PROGMEM;
extern	const char gcStrTMPK[]  PROGMEM;
extern	const char	*gcUnitNameTbl[] PROGMEM;	//={"LB  ", "KG  ", "TON ", "MTON", "OZ  ", "GRAM", "C   ", "F   "};
extern	const char gcStrNoName[]   PROGMEM;		// = "NO NAME";

BYTE  sensor_assign_cal_filter( BYTE n );
BYTE  sensor_assign_normal_filter( BYTE n );
void  sensor_1st_init( void );
void  sensor_1st_init_names( void );
void  sensor_init_all( void );
BYTE  sensor_descriptor_init( BYTE sensorN );
void  sensor_clear_all_status( void );
void  sensor_compute_all_values( void );
BYTE  sensor_format_data( BYTE sensorID, char *pOutStr );
BYTE  sensor_format_data_packet( BYTE sensorID, BYTE valueType, char *pOutStr );
BYTE  sensor_format_test_data_packet( BYTE sensorID, char *pOutStr );
// 2012-02-23 -WFC- BYTE  sensor_format_sys_annunciator( char *pOutStr );
BYTE  sensor_format_sys_annunciator( char *pOutStr, BYTE extraFlags );		// 2012-02-23 -WFC-
BYTE  sensor_format_setpoint_annunciator( char *pOutStr );					// 2015-09-09 -WFC-
BYTE  sensor_get_cal_base( BYTE sensorID, SENSOR_CAL_T **ppSensorCal );
BYTE  sensor_get_cal_table_array_index( BYTE sensorID );
BYTE  sensor_recall_config( BYTE sensorID );
BYTE  sensor_save_cal_table( BYTE sensorID );
void  sensor_update_param( BYTE sensorID );
void  sensor_set_filter_timer( LSENSOR_DESCRIPTOR_T *pSD );		// 2011-05-04 -WFC-

#endif		// end MSI_SENSOR_H
//@}

