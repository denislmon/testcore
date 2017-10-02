/*! \file sensor.c \brief sensors related functions.*/
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
// Description: 
//   It is a high level module manages all aspect of different type of sensors.
//   It knows the high level work flow and delegates the detail tasks to a specific sensor type module.
//   It has an abstract sensor descriptors prescribed what type of ADC chip, input selection,
//   conversion speed and sensor filter algorithm.  Each specific sensor ADC value is filled by sensor filter algorithm
//   which will be call by its ADC module. The sensor value is also supplied by the specified sensor module.
//   The descriptor contains an abstract device pointer pDev to a specific sensor type data structure, e.g. LOADCELL_T,
//   VOLTAGEMON_T etc... This makes easy for sensor module to compute the value and format output data.
//   It has method to initialize all sensor types based on gabSensorTypeFNV[], gabSensorFeaturesFNV[] and
//   gabSensorSpeedFNV[].
//
// Note
//  The Scalecore can have up to 4 loadcells. Only the first two sensors
//  (sensor0 and sensor1) are for loadcells with software temperature compensation.
//  Sensors 0 to 3 are loadcells. Sensors 4 to 6 are battery monitor.
//  Sensor 7 is a temperature sensor. Sensor 8 is a light sensor.
//  Note that sensor 7 and 8 do not require calibration.
//
//  Sensor 9 to 15 are virtual sensors such as remote sensors, math sensors, etc.. 
//  For remote sensor, it will not allow to calibrate with remote sensor ID;
//  instead, user must uses device ID and local sensor ID with cal commands and
//  talk to remote device directly ( may be indirectly by router).
//  
//  The first sensor loadcell will get temperature data from sensor 7 by default.
//  Each software temperature compensated loadcell will have 3 calibration tables corresponding
//  to 3 different temperature level.
//
// Threshold weight values will be saved in the same unit as the cal unit.
// If the user enter the value in unit different than cal unit, it will do
// converted entered value to cal unit and then saved it; these values have suffix of NV.
// The tare, zero, total, Sum square total etc... weight values are saved in display unit;
// these values have the suffix of FNV.
//
// ****************************************************************************
 
#include  "config.h"
#include  <stdio.h>
#include  "cmdparser.h"		// for CMD_ITEM_DELIMITER
#include  "commonlib.h"
#include  "sensor.h"
#include  "temperature.h"
#include  "voltmon.h"
#include  "loadcell.h"
#include  "lc_zero.h"
#include  "lc_tare.h"
#include  "loadcellfilter.h"
#include  "calibrate.h"
#include  "nvmem.h"
#include  "vs_math.h"
#include  "scalecore_sys.h"

#if ( CONFIG_INCLUDED_LIGHT_SENSOR_MODULE == TRUE )
#include  "lightmon.h"			// 2011-04-11 -WFC-
#endif

extern void  cmd_act_set_defaults( BYTE cmd );

LSENSOR_DESCRIPTOR_T	gaLSensorDescriptor[ MAX_NUM_LSENSORS ];

// the following will recall from nonevolatile memory during powerup.

/// Percentage of view capacity of underload threshold. A value less than this threshold is underload.
INT8	gabPcentCapUnderloadFNV[ MAX_NUM_SENSORS ];

/// gafSensorShowNNNNN variables will not changed when SensorViewUnit changed. They are reference parameters.
float	gafSensorShowCapacityFNV[ MAX_NUM_SENSORS ];
float	gafSensorShowCBFNV[ MAX_NUM_SENSORS ];		// float type of countby
UINT16	gawSensorShowCBFNV[ MAX_NUM_SENSORS ];		// integer type of countby
INT8    gabSensorShowCBdecPtFNV[ MAX_NUM_SENSORS ];
													/// countby unit of a sensor. It is reference unit.  0==LBS, 1==KG, 2==TON, 3==METRIC_TON, 4==OZ, 5==GRAMS, 6==C,  7== F.
UINT8	gabSensorShowCBunitsFNV[ MAX_NUM_SENSORS ];	 // countby unit of a sensor. 0==LBS, 1==KG, 2==TON, 3==METRIC_TON, 4==OZ, 5==GRAMS, 6==C,  7== F.

		/// current viewing unit of a sensor. 0==LBS, 1==KG, 2==TON, 3==METRIC_TON, 4==OZ, 5==GRAMS, 6==C,  7== F. -WFC- 2010-08-30
UINT8	gabSensorViewUnitsFNV[ MAX_NUM_SENSORS ];	// current viewing unit of a sensor. 0==LBS, 1==KG, 2==TON, 3==METRIC_TON, 4==OZ, 5==GRAMS, 6==C,  7== F.  -WFC- 2010-08-30

		/// Sensor Type 0==loadcell, 1==voltage, 2==current, 3==Temperature, 4==light, 5== remote, 6 == math or virtual, 0xFF=undefined.
BYTE	gabSensorTypeFNV[ MAX_NUM_SENSORS ];
												/// Sensor conversion speed range 1 to 15. Smaller value faster the speed and more noise.
BYTE	gabSensorSpeedFNV[ MAX_NUM_SENSORS ];	//  Sensor conversion speed range 1 to 15. Smaller value faster the speed and more noise.

/// Sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4-3 voltage reference. Bit2 reserved, Bit1-0 filter level, 0==disabled.
BYTE	gabSensorFeaturesFNV[ MAX_NUM_SENSORS ];	//  sensor status, Bit7 enabled; Bit6 AC excitation, Bit 5 temperature compensation, (Bit4 Global reference,1=enabled) Bit4-3 voltage reference. Bit2 reserved, Bit1-0 filter level, 0==disabled.

/// Sensor names
BYTE	gabSensorNameFNV[ MAX_NUM_SENSORS ][ MAX_SENSOR_NAME_SIZE + 1 ];

/// value of a sensor converted from ADC count to a standard value such as LB, Celsius, Kelvin etc..
float	gafSensorValue[ MAX_NUM_SENSORS ];


const char gcStrLB[]    PROGMEM = "LB  ";
const char gcStrKG[]    PROGMEM = "KG  ";
const char gcStrTON[]   PROGMEM = "TON ";
const char gcStrMTON[]  PROGMEM = "MTON";
const char gcStrOZ[]    PROGMEM = "OZ  ";
const char gcStrGRAM[]  PROGMEM = "GRAM";
const char gcStrKNWT[]  PROGMEM = "KNWT";
const char gcStrV[]     PROGMEM = "V   ";
const char gcStrAMP[]   PROGMEM = "Amp ";
const char gcStrTMPC[]  PROGMEM = "C   ";
const char gcStrTMPF[]  PROGMEM = "F   ";
const char gcStrTMPK[]  PROGMEM = "Kelv";
const char gcStrLux[]	PROGMEM = "Lux ";	// 2011-04-19 -WFC-
//const char  *gcUnitNameTbl[] PROGMEM = {"LB  ", "KG  ", "TON ", "MTON", "OZ  ", "GRAM", "C   ", "F   "};
// const char *gcUnitNameTbl[] PROGMEM = {gcStrLB, gcStrKG, gcStrTON, gcStrMTON, gcStrOZ, gcStrGRAM, gcStrC, gcStrF}; this is also work.
// PGM_P gcUnitNameTbl[] PROGMEM = {gcStrLB, gcStrKG, gcStrTON, gcStrMTON, gcStrOZ, gcStrGRAM, gcStrV, gcStrAMP, gcStrTMPC, gcStrTMPF, gcStrTMPK};
// PGM_P gcUnitNameTbl[] PROGMEM = {gcStrLB, gcStrKG, gcStrTON, gcStrMTON, gcStrOZ, gcStrGRAM, gcStrKNWT, gcStrV, gcStrAMP, gcStrTMPC, gcStrTMPF, gcStrTMPK};
PGM_P gcUnitNameTbl[] PROGMEM = {gcStrLB, gcStrKG, gcStrTON, gcStrMTON, gcStrOZ, gcStrGRAM, gcStrKNWT, gcStrV, gcStrAMP, gcStrTMPC, gcStrTMPF, gcStrTMPK, gcStrLux};		// 2011-04-19 -WFC-


#if ( CONFIG_PCB_AS	== CONFIG_PCB_AS_SCALECORE1 )
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
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


// ********************** SENSORS LAYOUT ***************************
/// sensor default type defines layout.

const BYTE	gcabDefaultSensorType[ MAX_NUM_SENSORS ]	PROGMEM = {
	SENSOR_TYPE_LOADCELL, SENSOR_TYPE_LOADCELL, SENSOR_TYPE_LOADCELL, SENSOR_TYPE_LOADCELL,		// Sensors 0 to 3 are local physical loadcells
	SENSOR_TYPE_MATH_LOADCELL,																	// Sensor 4 is virtual math type loadcell.
	SENSOR_TYPE_VOLTMON,  	SENSOR_TYPE_VOLTMON,	SENSOR_TYPE_VOLTMON,						// Sensor 5 to 7 are local physical voltage monitors.
	SENSOR_TYPE_TEMP,		SENSOR_TYPE_LIGHT,													// Sensor 8 to 9 are local physical temperature and light
	SENSOR_TYPE_UNDEFINED,	SENSOR_TYPE_UNDEFINED,	SENSOR_TYPE_UNDEFINED,						// Sensor 10 to 15 are virtual sensors such as remote,
	SENSOR_TYPE_UNDEFINED,	SENSOR_TYPE_UNDEFINED,	SENSOR_TYPE_UNDEFINED
};

const BYTE	gcabDefaultSensorFeatures[ MAX_NUM_SENSORS ]	PROGMEM = {
	// 2011-05-07 -WFC- SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT_DISABLED, SENSOR_FEATURE_DEFAULT_DISABLED, SENSOR_FEATURE_DEFAULT_DISABLED,
	SENSOR_FEATURE_LOADCELL_DEFAULT, SENSOR_FEATURE_DEFAULT_DISABLED, SENSOR_FEATURE_DEFAULT_DISABLED, SENSOR_FEATURE_DEFAULT_DISABLED,	// 2011-05-07 -WFC-
	SENSOR_FEATURE_MATH_LOADCELL_DEFAULT,
	SENSOR_FEATURE_VOLTMON_DEFAULT, SENSOR_FEATURE_VOLTMON_DEFAULT, SENSOR_FEATURE_VOLTMON_DEFAULT,
	SENSOR_FEATURE_TEMP_DEFAULT, SENSOR_FEATURE_DEFAULT,
	SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT,
	SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT
};

const char gcStrDefLoadcellName[]		PROGMEM = "Loadcell";
const char gcStrDefVoltMonName[]		PROGMEM = "Voltage Mon";
const char gcStrDefTemperatureName[]	PROGMEM = "Scale Temperature";
const char gcStrDefLightSensorName[]	PROGMEM = "Light Sensor";
const char gcStrNoName[]				PROGMEM = "NO NAME";
const char gcStrMathName[]				PROGMEM	= "Sum";
const char gcStrRcalName[]				PROGMEM	= "Rcal";

PGM_P gcStrDefSensoreNameTbl[] PROGMEM = {
		gcStrDefLoadcellName, gcStrDefLoadcellName, gcStrDefLoadcellName, gcStrDefLoadcellName,
		gcStrMathName,
		gcStrDefVoltMonName,  gcStrDefVoltMonName,  gcStrDefVoltMonName,
		gcStrDefTemperatureName, gcStrDefLightSensorName,
		gcStrNoName, gcStrNoName, gcStrNoName,
		gcStrNoName, gcStrNoName, gcStrNoName
};


#elif (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2)
// ****************************************************************************
// ****************************************************************************
// ****************************************************************************
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


// ********************** SENSORS LAYOUT ***************************
/// sensor default type defines layout.

const BYTE	gcabDefaultSensorType[ MAX_NUM_SENSORS ]	PROGMEM = {
	SENSOR_TYPE_LOADCELL, SENSOR_TYPE_LOADCELL, SENSOR_TYPE_LOADCELL, SENSOR_TYPE_LOADCELL,		// Sensors 0 to 3 are local physical loadcells
	SENSOR_TYPE_MATH_LOADCELL,																	// Sensor 4 is virtual math type loadcell.
	SENSOR_TYPE_VOLTMON,  	SENSOR_TYPE_VOLTMON,	SENSOR_TYPE_VOLTMON,						// Sensor 5 to 7 are local physical voltage monitors.
	SENSOR_TYPE_TEMP,		SENSOR_TYPE_LIGHT,													// Sensor 8 to 9 are local physical temperature
	SENSOR_TYPE_UNDEFINED,	SENSOR_TYPE_UNDEFINED,	SENSOR_TYPE_UNDEFINED,						// Sensor 10 to 15 are virtual sensors such as remote,
	SENSOR_TYPE_UNDEFINED,	SENSOR_TYPE_UNDEFINED,	SENSOR_TYPE_UNDEFINED
};

const BYTE	gcabDefaultSensorFeatures[ MAX_NUM_SENSORS ]	PROGMEM = {
	SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT,
	SENSOR_FEATURE_MATH_LOADCELL_DEFAULT,
	SENSOR_FEATURE_VOLTMON_DEFAULT, SENSOR_FEATURE_VOLTMON_DEFAULT, SENSOR_FEATURE_VOLTMON_DEFAULT,
	SENSOR_FEATURE_TEMP_DEFAULT, SENSOR_FEATURE_DEFAULT,
	SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT,
	SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT
};



/* for HD display board
const BYTE	gcabDefaultSensorType[ MAX_NUM_SENSORS ]	PROGMEM = {
	SENSOR_TYPE_LOADCELL,SENSOR_TYPE_LOADCELL, SENSOR_TYPE_LOADCELL, SENSOR_TYPE_RCAL,
	SENSOR_TYPE_VOLTMON, SENSOR_TYPE_VOLTMON,  SENSOR_TYPE_VOLTMON,
	SENSOR_TYPE_TEMP,	 SENSOR_TYPE_LIGHT,
	SENSOR_TYPE_MATH_LOADCELL, SENSOR_TYPE_UNDEFINED,SENSOR_TYPE_UNDEFINED,SENSOR_TYPE_UNDEFINED,
	SENSOR_TYPE_UNDEFINED,SENSOR_TYPE_UNDEFINED,SENSOR_TYPE_UNDEFINED,SENSOR_TYPE_UNDEFINED
};

const BYTE	gcabDefaultSensorFeatures[ MAX_NUM_SENSORS ]	PROGMEM = {
	SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT,
	SENSOR_FEATURE_VOLTMON_DEFAULT, SENSOR_FEATURE_VOLTMON_DEFAULT, SENSOR_FEATURE_VOLTMON_DEFAULT,
	SENSOR_FEATURE_TEMP_DEFAULT, SENSOR_FEATURE_DEFAULT,
	SENSOR_FEATURE_MATH_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT,
	SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT, SENSOR_FEATURE_DEFAULT
};
*/

const char gcStrDefLoadcellName[]		PROGMEM = "Loadcell";
const char gcStrDefVoltMonName[]		PROGMEM = "Voltage Mon";
const char gcStrDefTemperatureName[]	PROGMEM = "Scale Temperature";
const char gcStrDefLightSensorName[]	PROGMEM = "Light Sensor";
const char gcStrNoName[]				PROGMEM = "NO NAME";
const char gcStrMathName[]				PROGMEM	= "Sum";
const char gcStrRcalName[]				PROGMEM	= "Ccal";

PGM_P gcStrDefSensoreNameTbl[] PROGMEM = {
		// gcStrDefLoadcellName, gcStrDefLoadcellName, gcStrDefLoadcellName, gcStrRcalName,    2016-03-21 -WFC- changed RcalName to DefLoadcellName.
		gcStrDefLoadcellName, gcStrDefLoadcellName, gcStrDefLoadcellName, gcStrDefLoadcellName,
		gcStrMathName,
		gcStrDefVoltMonName,  gcStrDefVoltMonName,  gcStrDefVoltMonName,
		gcStrDefTemperatureName, gcStrDefLightSensorName,
		gcStrNoName, gcStrNoName, gcStrNoName,
		gcStrNoName, gcStrNoName, gcStrNoName
};

#endif

// private methods:
void  sensor_1st_init_normalize_countby( void );


/**
 * It initialized sensors when the application software runs the very 1st time for the device.
 *
 * @return none
 *
 * History:  Created on 2007/01/08 by Wai Fai Chin
 * 2011-03-29 -WFC- default peak hold sample speed.
 * 2011-07-12 -WFC- default 220 Hz sample speed for DynaLink2 peak hold mode because of higher impedance loadcell. Higher speed will cause lower ADC reading because not enough time to charged up sample capacitor.
 */
void  sensor_1st_init( void )
{
  BYTE i;
	cmd_act_set_defaults( 0x12 );	// set default of scale standard and motion detection, AZM and Zero on powerup enable flags.
	// 2010-09-10 -WFC- cmd_act_set_defaults( 0x17 );	// set default of display count-by of each sensor
	nvmem_default_all_sensor_viewing_cb_fram();			// 2010-09-10 -WFC-
	cmd_act_set_defaults( 0x18 );	// set default of display unit of each sensor
	cmd_act_set_defaults( 0x19 );	// set default of display capacity of each sensor
	cmd_act_set_defaults( 0x1E );	// set default of sensor features selection, filter level, conversion speed, sensor type
	cmd_act_set_defaults( 0x1F );	// set default of loadcell operation mode, motionDetectPeriod, motionBand in d...
	cmd_act_set_defaults( 0x20 );	// set default of STD mode's %capZeroBandLo, %capZeroBandHi, azmPeriod, azmCbRang, %capPwupZeroBandLo, %capPwupZeroBandHi 
	cmd_act_set_defaults( 0x21 );	// set default of NTEP mode's %capZeroBandLo, %capZeroBandHi, azmPeriod, azmCbRang, %capPwupZeroBandLo, %capPwupZeroBandHi 
	cmd_act_set_defaults( 0x22 );	// set default of OIML mode's %capZeroBandLo, %capZeroBandHi, azmPeriod, azmCbRang, %capPwupZeroBandLo, %capPwupZeroBandHi 
	cmd_act_set_defaults( 0x23 );	// set default of totalMode; dropThreshold%cap; riseThreshold%cap; minStableTime.
	cmd_act_set_defaults( 0x24 );	// set default of onAcceptLowerWt; onAcceptUpperWt.
	cmd_act_set_defaults( 0x34 );	// set default of math channels.

	sensor_1st_init_normalize_countby();
	
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )	// 2011-07-12 -WFC-
	gtSystemFeatureFNV.peakholdSampleSpeed = 5;		// 2011-07-12 -WFC-  default 220 Hz sample speed for DynaLink2 peak hold mode because of higher impedance loadcell. Higher speed will cause lower ADC reading because not enough time to charged up sample capacitor.
#else
	gtSystemFeatureFNV.peakholdSampleSpeed = 3;		// 2011-03-29 -WFC- for Challenger3 and other product.
#endif

	// default sensor type of each sensor.
	memcpy_P ( &gabSensorTypeFNV,     gcabDefaultSensorType, MAX_NUM_SENSORS);    
	memcpy_P ( &gabSensorFeaturesFNV, gcabDefaultSensorFeatures, MAX_NUM_SENSORS);    
	sensor_1st_init_names();
	
	for ( i=0; i < MAX_NUM_SENSORS; i++) {
		loadcell_1st_init( i );
		voltmon_1st_init( i );
		temp_sensor_1st_init( i );
		#if ( CONFIG_INCLUDED_LIGHT_SENSOR_MODULE == TRUE )
		light_sensor_1st_init( i );			// 2011-04-19 -WFC-
		#endif
		vs_math_init( i );
	}

	// The following setup will based on configuration from nonvolatile memory.
	// sensor features, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4 Global reference,1=enabled. Bit1-0 filter level, 0==disabled.
	// gabSensorFeaturesFNV[0]= SENSOR_FEATURE_SENSOR_ENABLE |SENSOR_FEATURE_GLOBAL_REF_V;	
} // end sensor_1st_init()


/**
 * It initialized all sensors.
 *
 * @return none
 * @note Other chip module initialization must be call after executed sensor_init()
 *       because other chip module initialization is based on sensor descriptor.
 *
 * History:  Created on 2007/01/03 by Wai Fai Chin
 */

void  sensor_init_all( void )
{
	BYTE i;
	for ( i=0; i < MAX_NUM_SENSORS; i++) {
		sensor_descriptor_init( i );
		loadcell_init( i );
		voltmon_init( i );
		temp_sensor_init( i );
		#if ( CONFIG_INCLUDED_LIGHT_SENSOR_MODULE == TRUE )
		light_sensor_init( i );			// 2011-04-19 -WFC-
		#endif
		vs_math_init( i );
	}
} // end sensor_init()


/**
 * It initialized sensors countby when the application software runs the very 1st time for the device.
 *
 * @return none
 *
 * History:  Created on 2009/01/21 by Wai Fai Chin
 */
void  sensor_1st_init_normalize_countby( void )
{
	MSI_CB	cb;
	BYTE i;

	for ( i=0; i < MAX_NUM_SENSORS; i++) {
		cb.fValue = gafSensorShowCBFNV[i];	
		cal_normalize_verify_input_countby( &cb );
		gafSensorShowCBFNV[i] = cb.fValue;
		gawSensorShowCBFNV[i] = cb.iValue;
		gabSensorShowCBdecPtFNV[i] = cb.decPt;
	}
  
} // end sensor_1st_init_normalize_countby()


/**
 * It initialized a specified sensor descriptor based on the variables of
 * gabSensorTypeFNV[], gabSensorFeaturesFNV[] and gabSensorSpeedFNV[].
 *
 * @param  sn -- sensor number or ID.
 * @return true if it successfully initialized.
 * @note If gabSensorTypeFNV[], gabSensorFeaturesFNV[] or gabSensorSpeedFNV[] had been updated,
 *       sensor_descriptor_init() must be call and then call the related chip module
 *       initializer.
 *
 * History:  Created on 2007/10/05 by Wai Fai Chin
 */

#if ( CONFIG_PCB_AS	== CONFIG_PCB_AS_SCALECORE1 )

BYTE  sensor_descriptor_init( BYTE sn )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to local sensor descriptor
	BYTE devN;				// device index of local sensor, e.g. sensor#4 is voltage monitor device index 0, gaVoltageMon[0].
	BYTE status;

	status = FALSE;
	if ( sn < MAX_NUM_LSENSORS ) {
		pSnDesc = &gaLSensorDescriptor[ sn];
		pSnDesc-> type = gabSensorTypeFNV[ sn];
		switch ( pSnDesc-> type )	{
			case SENSOR_TYPE_LOADCELL :
					if ( sn <= SENSOR_NUM_LAST_LOADCELL ) { // The ScaleCore only allow channel 0 to 3 for loadcell type sensor
						pSnDesc-> chipID = SENSOR_CHIP_ID_ADC_LTC2447;
						// hookup configuration is product specific and is hard code for hook up configuration
						// base on the sensor type. Load Cell type for ADC input is Differential, channel 0 to 3, and assigned to sensor ID 0 to 3.
						// assigned ADC chip channel to this sensor.
						pSnDesc-> hookup_cnfg = loadcell_config_hardware( sn );
						// sensor features, ac/dc excitation, temperature comp, and conversion speed
						pSnDesc-> conversion_cnfg =
							(gabSensorFeaturesFNV[sn] & SENSOR_CNV_CNFG_FEATURE_MASK ) |
							(gabSensorSpeedFNV[sn] & SENSOR_CNV_CNFG_SPEED_MASK );
						// sensor enable, reference voltage, and filter setting
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;
						// if ( sensorN == gbCalSensorChannel ) 	// if this channel is calibrating,
						if ( gabCalOpStep[ sn ] < CAL_STATUS_COMPLETED ) 	// if this channel is in calibration mode,
							pSnDesc-> gapfSensorFilterMethod = lc_stepwise_triangle_filter; // assigned filter for loadcell type sensor during calibration.
						else
							pSnDesc-> gapfSensorFilterMethod = lc_filter;	// assigned loadcell filter for loadcell type sensor in normal operation mode.
						lc_filter_init( sn, pSnDesc-> cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK );
						pSnDesc-> pDev = &gaLoadcell[ sn ];
						pSnDesc-> pName = &gabSensorNameFNV[sn][0];
						pSnDesc-> maxRawADCcount = -2147483648;		// -WFC- 2011-03-28
						status = TRUE;
					}
				break;
			case SENSOR_TYPE_MATH_LOADCELL :
						pSnDesc-> chipID = SENSOR_CHIP_ID_UNDEFINED;
						pSnDesc-> hookup_cnfg =
						pSnDesc-> conversion_cnfg =
						pSnDesc-> status = 0;					// disabled sensor, no sensor adc count and value.
						pSnDesc-> gapfSensorFilterMethod = 0;	// NO algorithm yet.

						pSnDesc-> cnfg	= gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> pDev	= &gaLoadcell[ sn ];
						pSnDesc-> pName	= &gabSensorNameFNV[sn][0];
						status = TRUE;
				break;

			case SENSOR_TYPE_VOLTMON :
					if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
						devN = sn - SENSOR_NUM_1ST_VOLTAGEMON;
						// hookup configuration is product specific and is hard code for hook up configuration
						// base on the sensor type. Voltage monitors is assigned to sensor ID 5 to 7.
						// assigned ADC chip channel to this sensor.
						pSnDesc-> hookup_cnfg = voltmon_config_hardware( devN, &(pSnDesc-> chipID));
						// sensor features, ac/dc excitation, temperature comp, and conversion speed
						pSnDesc-> conversion_cnfg =
							(gabSensorFeaturesFNV[sn] & SENSOR_CNV_CNFG_FEATURE_MASK ) |
							(gabSensorSpeedFNV[sn] & SENSOR_CNV_CNFG_SPEED_MASK );
						// sensor enable, reference voltage, and filter setting
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;
						//voltmon_init( sn );
						pSnDesc-> pDev = &gaVoltageMon[ devN ];
						pSnDesc-> pName = &gabSensorNameFNV[sn][0];
						status = TRUE;
					}
				break;
			case SENSOR_TYPE_TEMP:
					if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
						pSnDesc-> chipID = SENSOR_CHIP_CPU_ADC;
						// hookup configration is product specific and is hard code for hook up configuration
						// base on the sensor type. Temperature sensor is assigned to sensor ID 7.
						// assigned CPU ADC channel to this sensor.
						pSnDesc-> hookup_cnfg = 0;
						// sensor features, ac/dc excitation, temperature comp, and conversion speed
						pSnDesc-> conversion_cnfg =
							(gabSensorFeaturesFNV[sn] & SENSOR_CNV_CNFG_FEATURE_MASK ) |
							(gabSensorSpeedFNV[sn] & SENSOR_CNV_CNFG_SPEED_MASK );
						// sensor enable, reference voltage, and filter setting
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;
						// temp_sensor_init( sn );
						pSnDesc-> pDev = &gaTempSensor[0];
						pSnDesc-> pName = &gabSensorNameFNV[sn][0];
						status = TRUE;
					} // end if sn is a pcb temperature sensor index.
				break;
			case SENSOR_TYPE_LIGHT:
					if ( SENSOR_NUM_LIGHT_SENSOR == sn ) { // ensured sn is a light sensor index.
						pSnDesc-> chipID = SENSOR_CHIP_CPU_ADC;
						// hookup configration is product specific and is hard code for hook up configuration
						// base on the sensor type. Temperature sensor is assigned to sensor ID 7.
						// assigned CPU ADC channel to this sensor.
						pSnDesc-> hookup_cnfg = 4;
						// sensor features, ac/dc excitation, temperature comp, and conversion speed
						pSnDesc-> conversion_cnfg =
							(gabSensorFeaturesFNV[sn] & SENSOR_CNV_CNFG_FEATURE_MASK ) |
							(gabSensorSpeedFNV[sn] & SENSOR_CNV_CNFG_SPEED_MASK );
						// sensor enable, reference voltage, and filter setting
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;
						// temp_sensor_init( sn );
						#if ( CONFIG_INCLUDED_LIGHT_SENSOR_MODULE == TRUE )
						pSnDesc-> pDev = &gaLightSensor[0];
						#else
						pSnDesc-> pDev = 0;
						#endif
						pSnDesc-> pName = &gabSensorNameFNV[sn][0];
						status = TRUE;
					} // end if sn is a light sensor index.
				break;
			// define more sensor type here in the future version
			default: // disabled this sensor
					pSnDesc-> chipID = SENSOR_CHIP_ID_UNDEFINED;
					pSnDesc-> hookup_cnfg =
					pSnDesc-> conversion_cnfg =
					pSnDesc-> cnfg =
					pSnDesc-> status = 0;					// disabled sensor, no sensor adc count and value.
					pSnDesc-> gapfSensorFilterMethod = 0;	// NO algorithm yet.
					status = TRUE;
				break;
		} // end switch
	} // end if ( sn < MAX_NUM_LSENSORS )
	return status;
} //end sensor_descriptor_init()

#else
// 2013-06-24 -WFC- modified for ScaleCore3
BYTE  sensor_descriptor_init( BYTE sn )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to local sensor descriptor
	BYTE devN;				// device index of local sensor, e.g. sensor#4 is voltage monitor device index 0, gaVoltageMon[0].
	BYTE status;

	status = FALSE;
	if ( sn < MAX_NUM_LSENSORS ) {
		pSnDesc = &gaLSensorDescriptor[ sn];
		pSnDesc-> type = gabSensorTypeFNV[ sn];
		switch ( pSnDesc-> type )	{
			case SENSOR_TYPE_LOADCELL :
					if ( sn <= SENSOR_NUM_LAST_LOADCELL ) { // The ScaleCore only allow channel 0 to 3 for loadcell type sensor
						pSnDesc-> chipID = SENSOR_CHIP_ID_ADC_LTC2447;
						// hookup configuration is product specific and is hard code for hook up configuration
						// base on the sensor type. Load Cell type for ADC input is Differential, channel 0 to 3, and assigned to sensor ID 0 to 3.
						// assigned ADC chip channel to this sensor.
						pSnDesc-> hookup_cnfg = loadcell_config_hardware( sn );
						// sensor features, ac/dc excitation, temperature comp, and conversion speed
						pSnDesc-> conversion_cnfg =
							(gabSensorFeaturesFNV[sn] & SENSOR_CNV_CNFG_FEATURE_MASK ) |
							(gabSensorSpeedFNV[sn] & SENSOR_CNV_CNFG_SPEED_MASK );
						// sensor enable, reference voltage, and filter setting
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;
						// if ( sensorN == gbCalSensorChannel ) 	// if this channel is calibrating,
						if ( gabCalOpStep[ sn ] < CAL_STATUS_COMPLETED ) 	// if this channel is in calibration mode,
							pSnDesc-> gapfSensorFilterMethod = lc_stepwise_triangle_filter; // assigned filter for loadcell type sensor during calibration.
						else
							pSnDesc-> gapfSensorFilterMethod = lc_filter;	// assigned loadcell filter for loadcell type sensor in normal operation mode.
						lc_filter_init( sn, pSnDesc-> cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK );
						pSnDesc-> pDev = &gaLoadcell[ sn ];
						pSnDesc-> pName = &gabSensorNameFNV[sn][0];
						status = TRUE;
					}
				break;
			case SENSOR_TYPE_RCAL :		// for ScaleCore2 ONLY.
					if ( sn <= SENSOR_NUM_LAST_LOADCELL ) { // The ScaleCore only allow channel 0 to 3 for loadcell type sensor
						pSnDesc-> chipID = SENSOR_CHIP_ID_ADC_LTC2447;
						// hookup configuration is product specific and is hard code for hook up configuration
						// base on the sensor type. Load Cell type for ADC input is Differential, channel 0 to 3, and assigned to sensor ID 0 to 3.
						// assigned ADC chip channel to this sensor.
						pSnDesc-> hookup_cnfg = loadcell_config_hardware( sn );
						// sensor features, ac/dc excitation, temperature comp, and conversion speed
						pSnDesc-> conversion_cnfg =
							(gabSensorFeaturesFNV[sn] & SENSOR_CNV_CNFG_FEATURE_MASK ) |
							(gabSensorSpeedFNV[sn] & SENSOR_CNV_CNFG_SPEED_MASK );
						// sensor enable, reference voltage, and filter setting
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;
						pSnDesc-> gapfSensorFilterMethod = lc_stepwise_triangle_filter; 		// assigned filter for Rcal sensor
						lc_filter_init( sn, pSnDesc-> cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK );
						pSnDesc-> pDev = &gaLoadcell[ sn ];
						pSnDesc-> pName = &gabSensorNameFNV[sn][0];
						status = TRUE;
					}
				break;
			case SENSOR_TYPE_MATH_LOADCELL :
						pSnDesc-> chipID = SENSOR_CHIP_ID_UNDEFINED;
						pSnDesc-> hookup_cnfg =
						pSnDesc-> conversion_cnfg =
						pSnDesc-> status = 0;					// disabled sensor, no sensor adc count and value.
						pSnDesc-> gapfSensorFilterMethod = 0;	// NO algorithm yet.

						pSnDesc-> cnfg	= gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> pDev	= &gaLoadcell[ sn ];
						pSnDesc-> pName	= &gabSensorNameFNV[sn][0];
						status = TRUE;
				break;

			case SENSOR_TYPE_VOLTMON :
					if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
						devN = sn - SENSOR_NUM_1ST_VOLTAGEMON;
						// hookup configuration is product specific and is hard code for hook up configuration
						// base on the sensor type. Voltage monitors is assigned to sensor ID 5 to 7.
						// assigned ADC chip channel to this sensor.
						pSnDesc-> hookup_cnfg = voltmon_config_hardware( devN, &(pSnDesc-> chipID));
						// sensor features, ac/dc excitation, temperature comp, and conversion speed
						pSnDesc-> conversion_cnfg =
							(gabSensorFeaturesFNV[sn] & SENSOR_CNV_CNFG_FEATURE_MASK ) |
							(gabSensorSpeedFNV[sn] & SENSOR_CNV_CNFG_SPEED_MASK );
						// sensor enable, reference voltage, and filter setting
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;
						//voltmon_init( sn );
						pSnDesc-> pDev = &gaVoltageMon[ devN ];
						pSnDesc-> pName = &gabSensorNameFNV[sn][0];
						status = TRUE;
					}
				break;
			case SENSOR_TYPE_LIGHT:
					if ( SENSOR_NUM_LIGHT_SENSOR == sn ) { // ensured sn is a light sensor index.
						pSnDesc-> chipID = SENSOR_CHIP_CPU_ADC_A;
						// hookup configration is product specific and is hard code for hook up configuration
						// base on the sensor type. Temperature sensor is assigned to sensor ID 7.
						// assigned CPU ADC channel to this sensor.
						pSnDesc-> hookup_cnfg = 3;
						// sensor features, ac/dc excitation, temperature comp, and conversion speed
						pSnDesc-> conversion_cnfg =
							(gabSensorFeaturesFNV[sn] & SENSOR_CNV_CNFG_FEATURE_MASK ) |
							(gabSensorSpeedFNV[sn] & SENSOR_CNV_CNFG_SPEED_MASK );
						// sensor enable, reference voltage, and filter setting
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;
						// temp_sensor_init( sn );
						#if ( CONFIG_INCLUDED_LIGHT_SENSOR_MODULE == TRUE )
						pSnDesc-> pDev = &gaLightSensor[0];
						#else
						pSnDesc-> pDev = 0;
						#endif
						pSnDesc-> pName = &gabSensorNameFNV[sn][0];
						status = TRUE;
					} // end if sn is a light sensor index.
				break;

			case SENSOR_TYPE_TEMP:
					if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
						pSnDesc-> chipID = SENSOR_CHIP_CPU_ADC_A;
						// hookup configuration is product specific and is hard code for hook up configuration
						// base on the sensor type. Temperature sensor is assigned to sensor ID 8.
						// assigned CPU ADC channel to this sensor.
						pSnDesc-> hookup_cnfg = 4;
						// sensor features, ac/dc excitation, temperature comp, and conversion speed
						pSnDesc-> conversion_cnfg =
							(gabSensorFeaturesFNV[sn] & SENSOR_CNV_CNFG_FEATURE_MASK ) |
							(gabSensorSpeedFNV[sn] & SENSOR_CNV_CNFG_SPEED_MASK );
						// sensor enable, reference voltage, and filter setting
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;
						// temp_sensor_init( sn );
						pSnDesc-> pDev = &gaTempSensor[0];
						pSnDesc-> pName = &gabSensorNameFNV[sn][0];
						status = TRUE;
					} // end if sn is a pcb temperature sensor index.
				break;
			// define more sensor type here in the future version
			default: // disabled this sensor
					pSnDesc-> chipID = SENSOR_CHIP_ID_UNDEFINED;
					pSnDesc-> hookup_cnfg =
					pSnDesc-> conversion_cnfg =
					pSnDesc-> cnfg =
					pSnDesc-> status = 0;					// disabled sensor, no sensor adc count and value.
					pSnDesc-> gapfSensorFilterMethod = 0;	// NO algorithm yet.
					status = TRUE;
				break;
		} // end switch
	} // end if ( sn < MAX_NUM_LSENSORS )
	return status;
} //end sensor_descriptor_init()

#endif


/**
 * It assigned a filter for use during calibration of a sensor.
 *
 * @param  sn -- sensor number or ID.
 * @return true if it successfully initialized.
 *
 * History:  Created on 2007/10/15 by Wai Fai Chin
 */

BYTE  sensor_assign_cal_filter( BYTE sn )
{
	LSENSOR_DESCRIPTOR_T		*pSnDesc;	// points to local sensor descriptor
	BYTE status;

	status = FALSE;
	if ( sn < MAX_NUM_LSENSORS ) {
		pSnDesc = &gaLSensorDescriptor[ sn ];
		switch ( pSnDesc-> type )	{
			case SENSOR_TYPE_LOADCELL :
					if ( sn < MAX_NUM_LOADCELL ) { 											// The ScaleCore only allow channel 0 to 3 for loadcell type sensor
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;														// no new values for this sensor.
						pSnDesc-> gapfSensorFilterMethod = lc_stepwise_triangle_filter;			// assigned filter for loadcell type sensor during calibration.
						lc_filter_init( sn, pSnDesc-> cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK );
						status = TRUE;
					}
				break;
			case SENSOR_TYPE_VOLTMON :
					if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sn] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;														// no new values for this sensor.
						//pSnDesc-> gapfSensorFilterMethod = 0; no filter for voltage monitor.
						voltmon_init( sn );
						status = TRUE;
					}
				break;
		} // end switch
	} // end if ( sensorN < MAX_NUM_LSENSORS )
	return status;
} //end sensor_assign_cal_filter()


/**
 * It assigned a filter for use in normal operation mode.
 *
 * @param  sensorN -- sensor number or ID.
 * @return true if it successfully initialized.
 *
 * History:  Created on 2007/10/15 by Wai Fai Chin
 */

BYTE  sensor_assign_normal_filter( BYTE sensorN )
{
	LSENSOR_DESCRIPTOR_T		*pSnDesc;	// points to local sensor descriptor
	BYTE status;

	status = FALSE;
	if ( sensorN < MAX_NUM_LSENSORS ) {
		pSnDesc = &gaLSensorDescriptor[ sensorN ];
		switch ( pSnDesc-> type )	{
			case SENSOR_TYPE_LOADCELL :
					if ( sensorN < MAX_NUM_LOADCELL ) { 											// The ScaleCore only allow channel 0 to 3 for loadcell type sensor
						pSnDesc-> cnfg = gabSensorFeaturesFNV[sensorN] & SENSOR_CNFG_SETTING_MASK;
						pSnDesc-> status = 0;														// no new values for this sensor.
						pSnDesc-> gapfSensorFilterMethod = lc_filter;								// assigned filter for loadcell type sensor use in normal operation mode.
						lc_filter_init( sensorN, pSnDesc-> cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK );
						status = TRUE;
					}
				break;
		} // end switch
	} // end if ( n < MAX_NUM_LSENSORS )
	return status;
} //end sensor_assign_normal_filter()



/**
 * It converts all sensors' ADC counts to sensor values.
 *
 * If sensor type is loadcell, then it also performs motion check,
 * AZM, COZ, zeroing, taring, totaling, overload and under-range checking.
 *
 * @return none
 *
 * History:  Created on 2007/01/03 by Wai Fai Chin
 * 2014-09-09 -WFC- Modified to handle power saving mode
 *
 */

//  checking to each sensor's compute function.
void  sensor_compute_all_values( void )
{
	BYTE		i;

	for ( i=0; i < MAX_NUM_SENSORS; i++ )	{
		switch ( gabSensorTypeFNV[i] )	{
			case SENSOR_TYPE_LOADCELL:
					// 2014-09-09 -WFC v
					// loadcell_tasks( i );
					if ( SYS_POWER_SAVE_STATE_INACTIVE == gbSysPowerSaveState )
						loadcell_tasks( i );
					else
						self_test_detect_loadcell_motion( i );
					// 2014-09-09 -WFC ^
				break;
			case SENSOR_TYPE_VOLTMON:
					voltmon_tasks( i );
				break;
			case SENSOR_TYPE_TEMP:
					temp_sensor_tasks( i );
				break;
			case SENSOR_TYPE_MATH_LOADCELL:
					if ( SYS_POWER_SAVE_STATE_INACTIVE == gbSysPowerSaveState )  // 2014-09-09 -WFC
						vs_math_tasks( i );
				break;
			#if ( CONFIG_INCLUDED_LIGHT_SENSOR_MODULE == TRUE )
			case SENSOR_TYPE_LIGHT:
					light_sensor_tasks( i );
				break;
			#endif
		} // end switch() {}
	} // end for ( i=0; i < MAX_NUM_LSENSORS; i++ )	{}
} // end sensor_compute_all_values()

/**
 * It formats sensor data to be output by comport objects.
 *
 * @param  sensorID -- sensor ID or channel number.
 * @param  pOutStr  -- points to an allocated output string buffer.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2007-08-03 by Wai Fai Chin
 * 2011-04-19 -WFC- added light sensor type.
 */

BYTE sensor_format_data( BYTE sensorID, char *pOutStr )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to local sensor descriptor
	BYTE n;
	
	n = 0;
	if ( sensorID < MAX_NUM_SENSORS ) {					// ensured sensor number is valid.
		pSnDesc = &gaLSensorDescriptor[ sensorID ];
		switch ( gabSensorTypeFNV[ sensorID ] )	{
			case SENSOR_TYPE_LOADCELL:
			case SENSOR_TYPE_MATH_LOADCELL:
					n = loadcell_format_output( (LOADCELL_T *) pSnDesc-> pDev, pOutStr );
				break;
			case SENSOR_TYPE_VOLTMON:
					n = voltmon_format_output( (VOLTAGEMON_T *) pSnDesc-> pDev, pOutStr );
				break;
			case SENSOR_TYPE_TEMP:
					n = temp_sensor_format_output( (TEMP_SENSOR_T *) pSnDesc-> pDev, pOutStr );
				break;
			#if ( CONFIG_INCLUDED_LIGHT_SENSOR_MODULE == TRUE )
			case SENSOR_TYPE_LIGHT:		// 2011-04-19 -WFC-
					n = light_sensor_format_output( (LIGHT_SENSOR_T *) pSnDesc-> pDev, pOutStr );
				break;
			#endif
//			#if ( CONFIG_INCLUDED_VS_MATH_MODULE ==	TRUE )
//			case SENSOR_TYPE_MATH_LOADCELL:
//				n = vs_math_format_output( sensorID, pOutStr );
//				break;
//			#endif
			// TODO: SENSOR_TYPE_REMOTE
		} // end switch() {}
	} // end if ( sensorID < MAX_NUM_SENSORS ) {}
	return n;	// number of char in the output string buffer.
} // end sensor_format_data()


/**
 * It formats sensor data in packet format to be output by comport objects.
 *
 * @param  sensorID  -- sensor ID or channel number.
 * @param  valueTYpe -- valueType;
 * @param  pOutStr   -- points to an allocated output string buffer.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009-01-16 by Wai Fai Chin
 * 2011-04-19 -WFC- added light sensor type.
 * 2012-02-23 -WFC- passed sensor ID for loadcell_format_packet_output().
 */

BYTE sensor_format_data_packet( BYTE sensorID, BYTE valueType, char *pOutStr )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to local sensor descriptor
	BYTE n;
	
	n = 0;
	if ( sensorID < MAX_NUM_SENSORS ) {					// ensured sensor number is valid.
		pSnDesc = &gaLSensorDescriptor[ sensorID ];
		if ( sensorID == gtSystemFeatureFNV.bargraphSensorID )
			valueType |= SENSOR_VALUE_TYPE_CAL_BARGRAPH_bm;
		switch ( gabSensorTypeFNV[ sensorID ] )	{
			case SENSOR_TYPE_LOADCELL:
			case SENSOR_TYPE_MATH_LOADCELL:
					// 2012-02-23 -WFC- n = loadcell_format_packet_output( (LOADCELL_T *) pSnDesc-> pDev, pOutStr, valueType );
					n = loadcell_format_packet_output( (LOADCELL_T *) pSnDesc-> pDev, pOutStr, valueType, sensorID ); // 2012-02-23 -WFC-
				break;
			case SENSOR_TYPE_VOLTMON:
					//2012-02-23 -WFC- 	n = voltmon_format_packet_output( (VOLTAGEMON_T *) pSnDesc-> pDev, pOutStr, valueType );
					n = voltmon_format_packet_output( pSnDesc, pOutStr, valueType );  //2012-02-23 -WFC-
				break;
			case SENSOR_TYPE_TEMP:
					//2012-02-23 -WFC-  n = temp_sensor_format_packet_output( (TEMP_SENSOR_T *) pSnDesc-> pDev, pOutStr, valueType );
					n = temp_sensor_format_packet_output( pSnDesc, pOutStr, valueType ); //2012-02-23 -WFC-
				break;
			#if ( CONFIG_INCLUDED_LIGHT_SENSOR_MODULE == TRUE )
			case SENSOR_TYPE_LIGHT:		// 2011-04-19 -WFC-
					//2012-02-23 -WFC-  n = light_sensor_format_packet_output( (LIGHT_SENSOR_T *) pSnDesc-> pDev, pOutStr, valueType ); // 2011-04-19 -WFC-
					n = light_sensor_format_packet_output( pSnDesc, pOutStr, valueType ); //2012-02-23 -WFC-
				break;
			#endif
//			#if ( CONFIG_INCLUDED_VS_MATH_MODULE ==	TRUE )
//			case SENSOR_TYPE_MATH_LOADCELL:
//					n = vs_math_format_packet_output( sensorID, pOutStr, valueType);
//					break;
//			#endif
			// TODO: SENSOR_TYPE_REMOTE
		} // end switch() {}
	} // end if ( sensorID < MAX_NUM_SENSORS ) {}
	return n;	// number of char in the output string buffer.
} // end sensor_format_data_packet(,,)


/**
 * It formats sensor data in packet format to be output by comport objects.
 *
 * @param  sensorID  -- sensor ID or channel number.
 * @param  valueTYpe -- valueType;
 * @param  pOutStr   -- points to an allocated output string buffer.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/02/25 by Wai Fai Chin
 * 2012-02-23 -WFC- passed sensor ID for loadcell_format_packet_output().
 */

BYTE sensor_format_test_data_packet( BYTE sensorID, char *pOutStr )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to local sensor descriptor
	char *pbOutStr;
	BYTE n;
	BYTE valueType;
	
	valueType = SENSOR_VALUE_TYPE_GROSS;
	n = 0;
	if ( sensorID < MAX_NUM_LSENSORS ) {					// if the sensor is local physical sensor and math_loadcell.
		pSnDesc = &gaLSensorDescriptor[ sensorID ];
		n = (BYTE) sprintf_P( pOutStr, gcStrFmt_pct_lx_ld_ld, pSnDesc-> rawAdcData, pSnDesc-> curRawADCcount, pSnDesc-> curADCcount);
		pbOutStr = pOutStr + n;
		switch ( pSnDesc->type )	{
			case SENSOR_TYPE_LOADCELL:
			case SENSOR_TYPE_MATH_LOADCELL:
					// 2012-02-23 -WFC- n = loadcell_format_packet_output( (LOADCELL_T *) pSnDesc-> pDev, pOutStr, valueType );
					n = loadcell_format_packet_output( (LOADCELL_T *) pSnDesc-> pDev, pOutStr, valueType, sensorID ); // 2012-02-23 -WFC-

				break;
			case SENSOR_TYPE_VOLTMON:
					//2012-02-23 -WFC- 	n = voltmon_format_packet_output( (VOLTAGEMON_T *) pSnDesc-> pDev, pOutStr, valueType );
					n = voltmon_format_packet_output( pSnDesc, pOutStr, valueType );  //2012-02-23 -WFC-
				break;
			case SENSOR_TYPE_TEMP:
					//2012-02-23 -WFC-  n = temp_sensor_format_packet_output( (TEMP_SENSOR_T *) pSnDesc-> pDev, pOutStr, valueType );
					n = temp_sensor_format_packet_output( pSnDesc, pOutStr, valueType ); //2012-02-23 -WFC-
				break;
		} // end switch() {}
	} // end if ( sensorID < MAX_NUM_LSENSORS ) {}
//	else if ( sensorID < MAX_NUM_SENSORS ) {	//else it is a virtual or rmeote sensor
//		switch ( gabSensorTypeFNV[ sensorID ] )	{
//			case SENSOR_TYPE_MATH_LOADCELL:
//				break;
//		}
//	}
	return n;	// number of char in the output string buffer.
} // end sensor_format_test_data_packet(,)

/**
 * It formats annunciator of the ScaleCore system, info such as setpoints,.
 *
 * @param  pOutStr	  -- points to an allocated output string buffer.
 * @param  extraFlags -- extra flags, bit4, 0x10, means high resolution display mode. //2012-02-23 -WFC-
 *
 * @post   format annunciators are stored in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 *  bit7  SYS_HW_STATUS_ANNC_UNDER_VOLTAGE        0x80, 1 = under voltage,
 *  bit6  SYS_HW_STATUS_ANNC_OVER_TEMPERATURE     0x40, 1 = over temperature
 *  bit5  SYS_HW_STATUS_ANNC_UNDER_TEMPERATURE    0x20, 1 = under temperature
 *  bit4  SYS_STATUS_ANNC_HIRES_MODE			  0x10, 1 = high resolution display mode.
 *  // bit3 reserved
 *  bit3 OIML mode					 2015-05-14 -WFC-
 *  bit2 setpoint #2, 1== tripped.
 *  bit1 setpoint #1, 1== tripped.
 *  bit0 setpoint #0, 1== tripped.
 *
 * History:  Created on 2009-01-20 by Wai Fai Chin
 * 2012-02-23 -WFC- Modified to support high resolution display flag.
 * 2015-05-14 -WFC- use bit3 to indicate OIML mode.
 * 2015-10-30 -WFC- use bit3 to indicate either OIML or NTEP mode.
 * 2016-03-29 -WFC- only keep bit0 to bit2 of setpoint flags.
 */
// 2012-02-23 -WFC- BYTE sensor_format_sys_annunciator( char *pOutStr )
BYTE sensor_format_sys_annunciator( char *pOutStr, BYTE extraFlags )
{
	BYTE len;
	BYTE annc;

	annc = (BYTE) 	gSP_Registry;
	annc = annc & 7;	// 2016-03-29 -WFC- only keep bit0 to bit2.
	if ( voltmon_is_under_voltage( VOLTMON_UNDER_VOLTAGE_SN ))
		annc |= SYS_HW_STATUS_ANNC_UNDER_VOLTAGE;

	annc |= temp_sensor_is_temp_outside_spec( SENSOR_NUM_PCB_TEMPERATURE );
	annc |= extraFlags;		// 2012-02-23 -WFC-
	// 2015-05-14 -WFC-  v
	if ( (SCALE_STD_MODE_OIML == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK)) ||
			(SCALE_STD_MODE_NTEP == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK)) 	)	{ //if in OIML or NTEP mode
		if ( !(gbSysStatus & SYS_STATUS_CAL_SWITCH_TRIPPED) )
			annc |= 0x08;		// flag as oiml mode
	}
	// 2015-05-14 -WFC-  ^

	len  = sprintf_P( pOutStr, gcStrFmt_pct_d, annc );
	return len;
}

/**
 * It formats setpoint annunciator.
 *
 * @param  pOutStr	  -- points to an allocated output string buffer.
 *
 * @post   format annunciators are stored in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 *  bit7 setpoint #7, 1== tripped.
 *  bit6 setpoint #6, 1== tripped.
 *  bit5 setpoint #5, 1== tripped.
 *  bit4 setpoint #4, 1== tripped.
 *  bit3 setpoint #3, 1== tripped.
 *  bit2 setpoint #2, 1== tripped. May use as different flag in the future.
 *  bit1 setpoint #1, 1== tripped. May use as different flag in the future.
 *  bit0 setpoint #0, 1== tripped. May use as different flag in the future.
 *
 * History:  Created on 2015-09-09 by Wai Fai Chin
 *
 */
BYTE sensor_format_setpoint_annunciator( char *pOutStr )
{
	BYTE len;
	BYTE annc;
	annc = (BYTE) 	gSP_Registry;
	len = 0;
	pOutStr[ len ]= CMD_ITEM_DELIMITER;
	len++;
	len  += sprintf_P( pOutStr + len, gcStrFmt_pct_d, annc );
	return len;
}


/**
 * It gets the base of calibration data structure of the specified sensor.
 *
 * @param  sensorID	  -- sensor ID or channel number.
 * @param  ppSensorCal -- pointer to pointer to calibration data structure of this sensor
 *
 * @return TRUE if successful else FALSE.
 * @post	pSensorCal points to calibration data structure of this sensor if successed.
 *
 * History:  Created on 2008/05/22 by Wai Fai Chin
 */


BYTE sensor_get_cal_base( BYTE sensorID, SENSOR_CAL_T **ppSensorCal )
{
	BYTE i;
	BYTE status;
	
	if ( sensorID < MAX_NUM_LSENSORS ) {
		i = sensor_get_cal_table_array_index( sensorID );
		*ppSensorCal = &gaSensorCalNV[ i ];
		status = TRUE;
	}
	else
		status = FALSE;
	
	return status;
} // end sensor_get_cal_base(,)


/**
 * compute entry index of calibration table of a given loadcell number.
 *
 * @param  lc	-- loadcell number
 *
 * @return entry index of loadcell calibration table.
 * @note   it return 0 index if lc > MAX_NUM_LOADCELL
 *
 * gaSensorCalNV[0] to gaSensorCalNV[2] are for loadcell of sensor0.
 * gaSensorCalNV[3] to gaSensorCalNV[5] are for loadcell of sensor1;  each of these tables was calibrate
 * in a different temperature zone. Sensor0 and Sensor1, each has 3 temperature zone per calibration table.
 * The temperature zone difference must be at least 10 Kelvin.
 * gaSensorCalNV[6] is for sensor2. gaSensorCalNV[7] is for sensor3.
 * gaSensorCalNV[8] is shared by voltage monitor 1, 2 and 3.
 * These data will recall from nonevolatile memory during powerup.
 * 
 * NOTE: gaSensorCalNV[0], [3], [6] and [7] are also use as scratch memory for calibration of loadcell 0,1,2,and 3.
 *       It will restore from nonvolatile memory once it exit calibartion.
 *
 * History:  Created on 2008/05/22 by Wai Fai Chin
 * 2010-11-02 -WFC- Calibration table is only for loadcell. No more gaSensorCalNV[8] is shared by voltage monitor 1, 2 and 3.
 */

BYTE sensor_get_cal_table_array_index( BYTE sensorID )
{
	BYTE n;
	
	// note that the first two loadcells have 3 temperature zones for each cal table; that is why I have to add offset 3 to point to cal base table.
	n = 0;
	if ( sensorID < SENSOR_NUM_MATH_LOADCELL ) {
		if ( 1 == sensorID )
			n = 3;
		else if ( sensorID > 1 ) 
			n = sensorID + 4;
	}
	#if ( CAL_MAX_NUM_CAL_TABLE == 9 )
	else if ( sensorID < SENSOR_NUM_PCB_TEMPERATURE ) {
		n = 8;		// gaSensorCalNV[8] is shared by voltage monitor 1, 2 and 3.
	}
	#endif
	
	return n;
} // end sensor_get_cal_table_array_index()

/**
 * It initialized sensors name the application software runs the very 1st time for the device.
 *
 * @return none
 *
 * History:  Created on 2008/12/28 by Wai Fai Chin
 */

void  sensor_1st_init_names( void )
{
	//char *ptr_P; this is also work
	PGM_P	ptr_P;
	BYTE *destPtr;
	BYTE numStr[3];
	BYTE i;
	BYTE n;
	
	// default sensor name of each sensor.
	numStr[0] = ' ';
	n = numStr[2] = 0;
	// default name for loadcells.
	for ( i=0; i <= SENSOR_NUM_LAST_PV_LOADCELL; i++) {
		numStr[1] = '1'+ i;
		memcpy_P ( &ptr_P, &gcStrDefSensoreNameTbl[ i ], sizeof(PGM_P));
		destPtr = &gabSensorNameFNV[i][0];
		strcpy_P( destPtr, ptr_P);
		//2011-04-11 -WFC- the following statements will be removed once new rcal system is use in later version for both ScaleCore1 and 2.
		// 2011-05-19 -WFC- #if ( CONFIG_PCB_AS == CONFIG_PCB_AS_SCALECORE2)
		#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-05-19 -WFC-
			if ( i < SENSOR_NUM_RCAL )
				strcat( destPtr, numStr);
		#endif
	}

	// default name for voltage monitor.
	for ( i=SENSOR_NUM_1ST_VOLTAGEMON; i <= SENSOR_NUM_LAST_VOLTAGEMON; i++) {
		numStr[1] = '1'+ n;
		memcpy_P ( &ptr_P, &gcStrDefSensoreNameTbl[ i ], sizeof(PGM_P));
		destPtr = &gabSensorNameFNV[i][0];
		strcpy_P( destPtr, ptr_P);
		strcat( destPtr, numStr);
		n++;
	}

	// default name for the rest of sensors.
	for ( i=SENSOR_NUM_PCB_TEMPERATURE; i < MAX_NUM_SENSORS; i++) {
		memcpy_P ( &ptr_P, &gcStrDefSensoreNameTbl[ i ], sizeof(PGM_P));
		strcpy_P( &gabSensorNameFNV[i][0], ptr_P);
	}

} // end sensor_1st_init_names()


/**
 * It clears status of all sensors. This procedure will be on every pass of the main task loop. 
 *
 * @return none
 *
 * History:  Created on 2009/01/12 by Wai Fai Chin
 */

void  sensor_clear_all_status( void )
{
	BYTE sn;

	// clear status of all local sensor.
	for ( sn=0; sn < MAX_NUM_LSENSORS; sn++ ) {
		gaLSensorDescriptor[ sn ].status &= ~SENSOR_STATUS_GOT_DATA_MASK; // clear got ADC count and value flag bits.
	} // end for ();
} // end sensor_clear_all_status()


/**
 * It saves sensor cal table to eemem.
 *
 * @param  sensorID		-- sensor ID or channel number.
 *
 * @return 0 == successes.
 *
 * History:  Created on 2009/05/19 by Wai Fai Chin
 */

BYTE  sensor_save_cal_table( BYTE sensorID )
{
	BYTE status;
	BYTE startTableNum;
	BYTE endTablelNum;
	BYTE i;
	/*
	gaSensorCalNV[0] to gaSensorCalNV[2] are for loadcell of sensor0.
	gaSensorCalNV[3] to gaSensorCalNV[5] are for loadcell of sensor1;  each of these tables was calibrate
	in a different temperature zone. Sensor0 and Sensor1, each has 3 temperature zone per calibration table.
	The temperature zone difference must be at least 10 Kelvin.
	gaSensorCalNV[6] is for sensor2. gaSensorCalNV[7] is for sensor3.
	*/
	
	status = NVRAM_WRITE_PASS;		// assumed passed.
	
	//get the maxium cal table number of a given sensorID.
	startTableNum = endTablelNum = sensor_get_cal_table_array_index( sensorID );
	
	if ( 0 == sensorID ) {
		endTablelNum = 2;
	}
	else if ( 1 == sensorID ) {
		endTablelNum = 5;
	}
	
	// ensure this sensor has cal table.
	if ( sensorID < SENSOR_NUM_PCB_TEMPERATURE ) {
		// some sensor has more than one cal table.
		for ( i=startTableNum; i <= endTablelNum; i++ ) {
			status = nv_cnfg_eemem_save_a_cal_table( i );
			if ( status ) break;	// break out the loop if there is an error.
		}
	}
	return status;
} // end sensor_save_cal_table()



/**
 * It recalls sensor configuration settings include cal tables from nonevolatile memory.
 *
 * @param  sensorID		-- sensor ID or channel number.
 *
 * @return 0 == successes.
 *
 * History:  Created on 2009/05/18 by Wai Fai Chin
 */

BYTE  sensor_recall_config( BYTE sensorID )
{
	BYTE status;
	BYTE startTableNum;
	BYTE endTablelNum;
	BYTE i;
	
	/*
	gaSensorCalNV[0] to gaSensorCalNV[2] are for loadcell of sensor0.
	gaSensorCalNV[3] to gaSensorCalNV[5] are for loadcell of sensor1;  each of these tables was calibrate
	in a different temperature zone. Sensor0 and Sensor1, each has 3 temperature zone per calibration table.
	The temperature zone difference must be at least 10 Kelvin.
	gaSensorCalNV[6] is for sensor2. gaSensorCalNV[7] is for sensor3.
	*/
	
	status = NVRAM_READ_PASS;		// assumed passed.
	
	//get the maxium cal table number of a given sensorID.
	startTableNum = endTablelNum = sensor_get_cal_table_array_index( sensorID );
	
	if ( 0 == sensorID ) {
		endTablelNum = 2;
	}
	else if ( 1 == sensorID ) {
		endTablelNum = 5;
	}
	
	// ensure this sensor has cal table.
	if ( sensorID < SENSOR_NUM_PCB_TEMPERATURE ) {
		// some sensor has more than one cal table.
		for ( i=startTableNum; i <= endTablelNum; i++ ) {
			status = nv_cnfg_eemem_recall_a_cal_table( i );
			if ( status ) break;	// break out the loop if there is an error.
		}
	}

	//if ( !status )	// if no previous error, then
	//	status = nv_cnfg_fram_recall_a_sensor_feature( sensorID );
	return status;
} // end sensor_recall_config()


/**
 * It updates parameter of sensor and its concrete data structure based on changed value of
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sensorID	-- sensor channel or ID.
 *
 * @post   updated sensor data structure of this sensor.
 *
 * History:  Created on 2009/05/18 by Wai Fai Chin
 */

void  sensor_update_param( BYTE sensorID )
{
	switch ( gabSensorTypeFNV[ sensorID ] ) {
		case SENSOR_TYPE_LOADCELL:
		case SENSOR_TYPE_MATH_LOADCELL:
				loadcell_update_param( sensorID );
			break;
		case SENSOR_TYPE_TEMP:
				temp_sensor_update_param( sensorID );
			break;
		case SENSOR_TYPE_VOLTMON :
				if ( (sensorID <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sensorID >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
					voltmon_update_param( sensorID );
				}
			break;
		//TODO: add update param for virtual sensor??
	}
	
} // end sensor_update_param()


/**
 * Set filter timer based on software filter level.
 *
 * @param     pSD -- pointer to sensor descriptor.
 *
 * @return none
 *
 * @note  pSD must points to a valid sensor descriptor. This sensor descriptor was initialize by
 *        sensor_init() or host commands.
 *
 * History:  Created on 2011-05-04 by Wai Fai Chin
 */

void  sensor_set_filter_timer( LSENSOR_DESCRIPTOR_T *pSD )
{
	BYTE filterLevel;
	BYTE interval;

	filterLevel = pSD->cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK;		// filter level, 0 == disabled, 1 to 3 enabled to higher level.
	if ( LC_FILTER_LO == filterLevel )
		#if ( CONFIG_SUBPRODUCT_AS == CONFIG_AS_PORTAWEIGH2_PC )
			interval =TT_50mSEC;
		#else
			interval =TT_1SEC;
		#endif
	else if ( LC_FILTER_HI == filterLevel )
		interval =TT_2SEC;
	else if ( LC_FILTER_VERY_HI == filterLevel )
		interval =TT_3SEC;

	timer_mSec_set( &pSD->filterTimer, interval );
}

