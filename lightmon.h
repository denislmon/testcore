/*! \file lightmon.h \brief light sensor related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2010 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: independent
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Copyed by Pete Jensen on 2010/21/10 from temperature Created on 2008/12/08 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup Light montion converts raw ADC to light value and manages high level logics(lightmon.c)
/// \code #include "lightmon.h" \endcode
/// \par Overview
///   This is a high level module to convert filtered ADC count into light data
/// base on the calibration table. It does not know how the sensor got the ADC data.
/// It just know how to compute the light. It formats output light data string to
/// a caller supplied string buffer but it does not know how to output the data.
/// \code
///
///            Application          Abstract Object        Hardware driver
///         +---------------+      +---------------+     +----------------+
///         |     light     |      |               |     |                |
///    -----|     MODULE    |<-----| SENSOR MODULE |<----| CPU ADC value  |
///         |  ADC to C, F  |      |      ADC      |     |                |
///         |               |      |               |     |                |
///         +---------------+      +---------------+     +----------------+
///
///	\endcode
/// \note
///
/// Threshold light values will be saved in the same unit as the cal unit.
/// If the user enter the value in unit different than cal unit, it will be
/// convered entered value to cal unit and then saved it; these values have suffix of NV.
/// light value is saved in display unit; these values have the suffix of FNV.
//
//	
// ****************************************************************************
//@{
 

#ifndef MSI_LIGHTMON_H
#define MSI_LIGHTMON_H

#include  "config.h"

#include  "timer.h"

#include  "sensor.h"
#include  "calibrate.h"

#define  MAX_NUM_LIGHT_SENSOR		1
#define  MAX_NUM_LIGHT_UNITS		1

#define  LIGHT_RUN_MODE_ENABLED			SENSOR_FEATURE_SENSOR_ENABLE
#define  LIGHT_RUN_MODE_IN_CAL				0x40
#define  LIGHT_RUN_MODE_NORMAL_ACTIVE		0x20

#define  LIGHT_STATUS_GOT_VALID_VALUE 0x80
#define  LIGHT_STATUS_GOT_CAL_VALUE	0x40

/*!
  \brief light monitor data structure.
*/

typedef struct  LIGHT_SENSOR_TAG {
									/// light value
	float	value;
									/// voltage scale factor of ADC count.
	// float	scaleFactor;
									/// sampling interval timer
	TIMER_T sampleTimer;
									/// voltage monitor running mode: enabled, in calibration
	BYTE	runModes;
									/// status
	BYTE	status;
									/// pointer to calibration table structure.
	SENSOR_CAL_T *pCal;
									/// countby for display or show weight data
	MSI_CB		viewCB;
}LIGHT_SENSOR_T;


extern LIGHT_SENSOR_T	gaLightSensor[ MAX_NUM_LIGHT_SENSOR ];

void	light_sensor_1st_init( BYTE sn );
void	light_sensor_init( BYTE sn );
void	light_sensor_compute_light( BYTE sn );
BYTE	light_sensor_format_output( LIGHT_SENSOR_T *pTS, char *pOutStr );
//2012-02-23 -WFC-BYTE	light_sensor_format_packet_output( LIGHT_SENSOR_T *pTS, char *pOutStr, BYTE valueType );
BYTE	light_sensor_format_packet_output( LSENSOR_DESCRIPTOR_T *pSnDesc, char *pOutStr, BYTE valueType ); //2012-02-23 -WFC-
void	light_sensor_tasks( BYTE sn );
void	light_sensor_update_param( BYTE sn );


#endif		// end MSI_LIGHTMON_H
//@}

