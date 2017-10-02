/*! \file calibrate.h \brief functions for calibrate sensors.*/
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
// Compiler: any C
// Software layer:  Application
//
//  History:  Created on 2007/08/01 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup calibrate Calibrate sensor related functions (calibrate.c)
/// \code #include "calibrate.h" \endcode
/// \see cmdaction
/// \par Overview
///     It can calibrates multiple sensors and different type of sensors at a time.
///  This module only knows about the SENSOR_CAL_T data structure. It does not
///  care about specific sensor type such as loadcell, temperature or current sensors.
///  The final sensor type computation and error checking are delegate to specific type of sensor module.
///
/// \note
///  The Scalecore can have up to 4 loadcells. Only the first two sensors (sensor0 and sensor1) are for
///  loadcells with software temperature compensation. Sensor4  is a temperature sensor.
///  The first sensor loadcell will get temperature data from sensor 4 by default.
///  Each software temperature compensated loadcell will have 3 calibration tables corsponding
///  to 3 different temperature level.
//
// ****************************************************************************
//@{

#ifndef MSI_CALIBRATE_H
#define MSI_CALIBRATE_H


#include  "config.h"
// #include  "sensor.h"
#include "stream_router.h"
// cannot put it here because it cause infinite recurse for the compiler #include  "nvmem.h"			// for NVRAM_EEMEM_FAIL


//#define MAX_CAL_POINTS			10
//#define MAX_CAL_POINTS			6
//2011-04-20 -WFC- #define MAX_CAL_POINTS			4
#define MAX_CAL_POINTS			5			//2011-04-20 -WFC- one zero point and 4 span points.
#define MAX_LOADCELL_COUNTS		10000

#define CAL_ERROR_NOT_ALLOW					0xFF
#define CAL_ERROR_CANNOT_CHANGE_COUNTBY		0xEC
#define CAL_ERROR_WRONG_CAL_SEQUENCE		0xEB
#define CAL_ERROR_INVALID_CAL_INFO			0xEA
#define CAL_ERROR_WRONG_SENSOR_ID			0xE9
#define CAL_ERROR_LESS_THAN_4CNT_PER_D		0xE3			// 2011-02-01 -WFC-
#define CAL_ERROR_DIFFERENT_VALUE_ON_SAME_LOAD	0xE2		// 2011-01-12 -WFC-
#define CAL_ERROR_CANNOT_CHANGE_UNIT		0xE1
#define CAL_ERROR_NEED_UNIT					0xE0
#define CAL_ERROR_NEED_UNIT_CAP				0xDF
#define CAL_ERROR_NEED_COUNTBY				0xDD
#define CAL_ERROR_FAILED_CAL				0xDC
#define CAL_ERROR_TEST_LOAD_GT_CAPACITY		0xD3			// 2011-11-03 -WFC- implement for problem report #928.
#define CAL_ERROR_TEST_LOAD_TOO_SMALL		0xD2
#define CAL_ERROR_INVALID_CAPACITY			0xD1
#define CAL_ERROR_CANNOT_CHANGE_CAPACITY	0xD0
#define CAL_ERROR_NONE						0

#define	CAL_ERROR_NV_MEMORY_FAIL			0xED	//ALL Non volatile memory failed included EEMEM and FRAM.
#define CAL_ERROR_EEMEM_FAIL				0xEE	//NVRAM_EEMEM_FAIL
#define CAL_ERROR_FRAM_FAIL					0xEF	//NVRAM_FRAM_FAIL


/// if calOpStep < CAL_STATUS_COMPLETED, it is in cal mode. 
/// if calOpStep < CAL_STATUS_GOT_COUNTBY, then it is ready for calibrate test point.
#define CAL_STATUS_UNCAL			255
#define CAL_STATUS_COMPLETED		254
#define CAL_STATUS_GOT_UNIT			0xE0
#define CAL_STATUS_GOT_UNIT_CAP		0xDF
#define CAL_STATUS_GET_COUNTBY		0xDE
#define CAL_STATUS_GOT_COUNTBY		0xDD
#define CAL_STATUS_GOT_ZERO			0x0


#define CAL_MAX_TEMPERATURE_ZONE	3

/*! \def CAL_MAX_NUM_CAL_TABLE
  Maxium number of SENSOR_CAL_T tables.
  gaSensorCalNV[0] to gaSensorCalNV[2] are for loadcell of sensor0.
  gaSensorCalNV[3] to gaSensorCalNV[5] are for loadcell of sensor1;  each of these tables was calibrate
  in a different temperature zone. Sensor0 and Sensor1, each has 3 temperature zone per calibration table.
  The temperature zone difference must be at least 10 Kelvin.
  gaSensorCalNV[6] is for sensor2. gaSensorCalNV[7] is for sensor3.
  // NOT SUPPORT YET FOR gaSensorCalNV[8] is shared by voltage monitor 1, 2 and 3.
*/
#define CAL_MAX_NUM_CAL_TABLE		8

#define CAL_MAX_NUM_POSSIBLE_CB		10

/*!
  \brief sensor calibration data table of a temperature zone.
*/
typedef struct  SENSOR_CAL_TAG {
							/// capacity of this sensor
	float   capacity;
							/// MSI style countby
	MSI_CB  countby;
							/// 255=not calibrated, 254 == cal completed, 0xDF = got unit, capacity,  0xDE == got countby, 0==cal point 0,  cal point 1, 2, 3... MAX_CAL_POINTS - 1
	BYTE	status;			
							/// The temperature during calibration. always in Kelvin
	float	temperature;
										/// array of filtered ADC counts in calibration points. adcCnt[] and value[] form a cal point pare in a calibration table.
	INT32	adcCnt[ MAX_CAL_POINTS ];
										/// array of sensor value in calibration points. It could be weight, temperature, etc..
	float	value[ MAX_CAL_POINTS ];
}SENSOR_CAL_T;


extern	SENSOR_CAL_T gSensorCal;

/*
extern  float   gfCalTmpValue;
extern  BYTE    gbCalOpStatus;			// Calibrating operation status for all cal related command.
extern  BYTE    gbCalSensorChannel;		// ADC channel number is in cal mode. 
extern  BYTE    gbCalChannelType;       // 0 = loadcell, 1 = temperature
extern	BYTE	gbCalTmpZone;			// temperature zone of a calibration table. It ranges from 0 to 2.
*/

// extern	BYTE    gabCalChannelType[];       
extern	BYTE    gabCalCB_unit[];
extern	float	gafCal_capacity[];
extern	float	gafCalValue[];
extern	BYTE    gabCalErrorStatus[];
extern	BYTE    gabCalOpStep[];
extern	BYTE    gabCalSensorChannel[];
extern	BYTE	gabCalTmpZone[];


// the following will recall from nonevolatile memory during powerup.
extern	SENSOR_CAL_T gaSensorCalNV[ CAL_MAX_NUM_CAL_TABLE ];

BYTE  cal_allows_new_cal( SENSOR_CAL_T *pSensorCal );
//BYTE  cal_build_table( INT32 adcCnt, float fValue );
BYTE  cal_build_table( INT32 adcCnt, float fValue, SENSOR_CAL_T *pSensorCal );
void  cal_cmd_variables_init( void );
void  cal_table_all_init( void );
void  cal_table_init( SENSOR_CAL_T *pSensorCal );

BYTE  cal_gen_countby_table( float fCapacity, BYTE *pbStdCBIndex, BYTE *pbWantedCBIndex, MSI_CB *pWantedCB );
void  cal_next_lower_countby( MSI_CB *pCB);
BYTE  cal_normalize_input_cal_countby( SENSOR_CAL_T *pSensorCal );
BYTE  cal_normalize_verify_input_countby( MSI_CB *pCB);
float cal_scale_float_countby( UINT16 iValue, INT8 dPt);

BYTE  cal_save_exit( BYTE channel, SENSOR_CAL_T	*pSensorCal );
BYTE  cal_zero_point( BYTE channel, SENSOR_CAL_T *pSensorCal );

BYTE  cal_get_cal_table_num( BYTE n, BYTE tmpZone );
BYTE  cal_recall_a_sensor_cnfg( BYTE channel );

BYTE  cal_get_sensor_id_from_table_num( BYTE tableNUm );
void  cal_cmd_variables_of_a_sensor_init( BYTE i );


#endif // MSI_CALIBRATE_H
//@}
