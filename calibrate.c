/*! \file calibrate.c \brief functions for calibrate sensors.*/
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
//     It calibrates one sensor at a time.
//
// ****************************************************************************


// To do:
// show tempary cal data if in cal mode else show flash cal data if available.

#include  "calibrate.h"
#include  "sensor.h"
#include  "commonlib.h"
#include  "bios.h"
#include  <stdio.h>

/*

/// 0 = loadcell, 1 = temperature ,, Sensor type
BYTE    gbCalChannelType;       

/// Calibrating operation status for all cal related command.
BYTE    gbCalOpStatus;			

/// Sensor channel in cal mode. Init it to MAX_NUM_SENSORS means no sensor is in calibration mode.
BYTE    gbCalSensorChannel;

/// Temperature zone of a calibration table. 0==normal. 1 and 2 can be use for below or above normal operating temperature by 10 degree Kelvin.
BYTE	gbCalTmpZone;

/// cal value could be weight, temperature, distance etc..
float   gfCalTmpValue;			
*/

// The following are for command user interface between commands and calibration data structures.
// 0 = loadcell, 1 = temperature ,, Sensor type
//BYTE    gabCalChannelType[ MAX_NUM_CAL_SENSORS ];       

/// Calibration unit
BYTE    gabCalCB_unit[ MAX_NUM_CAL_SENSORS ];

/// Calibration unit
float    gafCal_capacity[ MAX_NUM_CAL_SENSORS ];

/// cal value could be weight, temperature, distance etc..
float   gafCalValue[ MAX_NUM_CAL_SENSORS ];


/// Temperature zone of a calibration table. 0==normal. 1 and 2 can be use for below or above normal operating temperature by 10 degree Kelvin.
BYTE	gabCalTmpZone[ MAX_NUM_CAL_SENSORS ];


/// Calibrating operation error status for all cal related command.
//BYTE    gabCalOpStatus[ MAX_NUM_SENSORS ];
BYTE    gabCalErrorStatus[ MAX_NUM_CAL_SENSORS ];


/// Calibrating operation step for all cal related command. This is the same as gSensorCal.status.
BYTE    gabCalOpStep[ MAX_NUM_CAL_SENSORS ];


// use as a temporary calibration table during calibrate a sensor.
//SENSOR_CAL_T gSensorCal;

// The following will save in nonvolatile memory ( flash data memory ) after completed calibration.
// its address sequence will matched the flash data address.

// the following will recall from nonevolatile memory during powerup.

/*! 
  gaSensorCalNV[0] to gaSensorCalNV[2] are for loadcell of sensor0.
  gaSensorCalNV[3] to gaSensorCalNV[5] are for loadcell of sensor1;  each of these tables was calibrate
  in a different temperature zone. Sensor0 and Sensor1, each has 3 temperature zone per calibration table.
  The temperature zone difference must be at least 10 Kelvin.
  gaSensorCalNV[6] is for sensor2. gaSensorCalNV[7] is for sensor3.
  gaSensorCalNV[8] is shared by voltage monitor 1, 2 and 3.
  These data will recall from nonevolatile memory during powerup.
  
  NOTE: gaSensorCalNV[0], [3], [6] and [7] are also use as scratch memory for calibration of loadcell 0,1,2,and 3.
        It will restore from nonvolatile memory once it exit calibartion.
*/
SENSOR_CAL_T gaSensorCalNV[ CAL_MAX_NUM_CAL_TABLE ];

void  cal_normalize_countby( MSI_CB *pCB);
void  cal_normalize_input_countby( MSI_CB *pCB);
BYTE  cal_save_sensor_cal_info( BYTE n, SENSOR_CAL_T *pSensorCal ); // save sensor calibration info to nonevolatile data memory.
void  cal_scale_float_type_countby( MSI_CB *pCB);


/**
 * It initialized calibration related command variables.
 *
 * @return none
 *
 * History:  Created on 2006/12/20 by Wai Fai Chin
 */

void  cal_cmd_variables_init( void )
{
  BYTE i;

  for ( i=0; i < MAX_NUM_CAL_SENSORS; i++ ) {
	  cal_cmd_variables_of_a_sensor_init( i );
  }
} // end cal_cmd_variables_init()


/**
 * It initialized calibration related command variables of a specified sensor.
 *
 * @return none
 *
 * History:  Created on 2010/06/07 by Wai Fai Chin
 */

void  cal_cmd_variables_of_a_sensor_init( BYTE i )
{
  BYTE calTableNum;

  if ( i < MAX_NUM_CAL_SENSORS ) {
	// gabCalChannelType[i] 	= SENSOR_TYPE_LOADCELL;       
	gabCalCB_unit[i]		= SENSOR_UNIT_LB;
	gafCal_capacity[i]		= 1.0;
	gafCalValue[i]			= 0.0;
	gabCalTmpZone[i] 		= 0;
	gabCalErrorStatus[i]	= CAL_ERROR_NONE;
	//gabCalOpStep[i]		= CAL_STATUS_UNCAL;
	calTableNum = cal_get_cal_table_num( i, 0 );
	gabCalOpStep[i]			= gaSensorCalNV[calTableNum].status;
  }
} // end cal_cmd_variables_of_a_sensor_init()



/**
 * It initialized calibration table.
 *
 * @param  pSensorCal	-- pointer to calibration data structure of this sensor
 * @return none
 *
 * History:  Created on 2006/12/20 by Wai Fai Chin
 */
/*
void  cal_table_init( void )
{
  BYTE i;

  gbCalTmpZone = 0;
  gSensorCal.status  = CAL_STATUS_UNCAL;
  gbCalSensorChannel = MAX_NUM_SENSORS;				// means no sensor is in calibration mode.
  gSensorCal.countby.decPt = 0;
  for ( i=0; i < MAX_CAL_POINTS; i++ ) {
	gSensorCal.adcCnt[i] = 0;
	gSensorCal.value [i] = 0.0;
  }
} // end cal_table_init()
*/

void  cal_table_init( SENSOR_CAL_T *pSensorCal )
{
  BYTE i;
  //pSensorCal-> status  = CAL_STATUS_UNCAL;
  //pSensorCal-> countby.decPt = 0;
  for ( i=0; i < MAX_CAL_POINTS; i++ ) {
	pSensorCal-> adcCnt[i] = 0;
	pSensorCal-> value [i] = 0.0;
  }
} // end cal_table_init()


/**
 * It initialized all calibration table.
 *
 * @return none
 *
 * History:  Created on 2008/07/17 by Wai Fai Chin
 *  2010/05/21 clear everything in the data structure instead just the status item for the sake of host command user interface. -WFC-
 */

void  cal_table_all_init( void )
{
  BYTE i, j;
	for ( i=0; i<  CAL_MAX_NUM_CAL_TABLE; i++) {
		gaSensorCalNV[i].status = CAL_STATUS_UNCAL;
   		gaSensorCalNV[i].capacity = 10000.0f;
		gaSensorCalNV[i].temperature = 0.0f;
		gaSensorCalNV[i].countby.decPt = 0;
		gaSensorCalNV[i].countby.unit = 0;
		gaSensorCalNV[i].countby.iValue = 1;
		gaSensorCalNV[i].countby.fValue = 1.0f;
		for ( j=0; j < MAX_CAL_POINTS; j++ ) {
			gaSensorCalNV[i].adcCnt[j] = 0;
			gaSensorCalNV[i].value [j] = 0.0;
		}
	}
} // end cal_table_all_init()


#define CAL_VALUE_ALREADY_EXIST            0
#define CAL_TABLE_NOT_FULL_LARGEST_SO_FAR  1
#define CAL_TABLE_NOT_FULL_NOT_LARGEST     2
#define CAL_TABLE_FULL_LARGEST_SO_FAR      3
#define CAL_TABLE_FULL_NOT_LARGEST         4

/**
 * It build a calibrated point table.
 *
 * @param  adcCnt		-- filtered ADC counts. 
 * @param  fValue		-- value of the cal point. It could be weight, temperature, distance etc..
 * @param  pSensorCal	-- pointer to calibration data structure of this sensor
 *
 * @return 0 if successed else return error codes.
 *
 * History:  Created on 2006/12/20 by Wai Fai Chin
 * 2011-01-12 -WFC- prevent user enter a new value but never change load.
 * 2011-02-01 -WFC- v added logic to detect cal span is less than 4 count per d.
 * 2011-11-03 -WFC- Ensured test load value within capacity. This resolved problem report # 928.
 * 2013-09-16 -WFC- Allowed test load value greater than capacity as requested by Jonny Hendrix and David Taylor.
 */


BYTE  cal_build_table( INT32 adcCnt, float fValue, SENSOR_CAL_T *pSensorCal )
{
	#define CAL_ADC_DELTA	500
	BYTE i;
	BYTE calCase;
	BYTE locate;
	INT32 i32A;
	float fdV;

	if ( pSensorCal-> status >= CAL_STATUS_GOT_COUNTBY ) {
		// if it was not completed setup for calibration, return wrong cal sequence error.
		return CAL_ERROR_WRONG_CAL_SEQUENCE;
	}

	// 2011-11-03 -WFC- v Ensured test load value within capacity. This resolved problem report # 928.
	// 2013-09-16 -WFC- if ( fValue > pSensorCal->capacity )
	if ( fValue > (pSensorCal->capacity * 1.1f))	// 2013-09-16 -WFC-
		return CAL_ERROR_TEST_LOAD_GT_CAPACITY;
	// 2011-11-03 -WFC- ^

	calCase = 0xFF;		// init calCase
	for ( i = 0; i < MAX_CAL_POINTS; i++ ) {
		//if ( (gSensorCal.value[i] >= fValueLow) && (gSensorCal.value[i] <= fValueHi))   {
		if ( (pSensorCal-> value[i] >= (fValue - pSensorCal-> countby.fValue)) && (pSensorCal-> value[i] <= (fValue + pSensorCal-> countby.fValue)))   {
			// if entry has the same value as the input value, use new values to replace old one
			// flagged it as calibration case CAL_VALUE_ALREADY_EXIST in the table.
			pSensorCal-> value[i]  = fValue;
			pSensorCal-> adcCnt[i] = adcCnt;
			calCase = CAL_VALUE_ALREADY_EXIST;		// calCase
		}
		// 2011-01-12 -WFC- v prevent user enter a new value but never change load.
		else {
			if ( (pSensorCal-> adcCnt[i] >= ( adcCnt - CAL_ADC_DELTA ) ) && (pSensorCal-> adcCnt[i] <= ( adcCnt + CAL_ADC_DELTA ) ) ) {
				return CAL_ERROR_DIFFERENT_VALUE_ON_SAME_LOAD;
			}
			else if ( i > 0 ) {		// 2011-02-01 -WFC- v added logic to detect cal span is less than 4 count per d.
				 i32A = pSensorCal-> adcCnt[i] - pSensorCal-> adcCnt[i-1];
				 if ( i32A > 0 ) {
					 fdV = pSensorCal-> value[i] - pSensorCal-> value[i-1];
					 if ( fdV > 0.00001 || fdV < -0.00001 ) {
						 fdV = (float) i32A / fdV;				// ADC count per value.
						 fdV *= pSensorCal-> countby.fValue;
						 if ( fdV < 3.9999 ) {
							 return CAL_ERROR_LESS_THAN_4CNT_PER_D;
						 }
					 }
				 }
			} // 2011-02-01 -WFC- ^
		}
		// 2011-01-12 -WFC- ^
	} // end for ( i = 0; i < MAX_CAL_POINTS; i++ ) {}

	if ( calCase != CAL_VALUE_ALREADY_EXIST )    {		// calCase
		if ( pSensorCal-> value[ MAX_CAL_POINTS - 1] == pSensorCal-> value[ MAX_CAL_POINTS - 2]) { // if the last two cal points are the same weight, it means the table in not full.
			if ( fValue >= pSensorCal-> value[ MAX_CAL_POINTS - 1])    // if input value > last point value. It is the largest value we have encountered so far.
				calCase = CAL_TABLE_NOT_FULL_LARGEST_SO_FAR;	// calCase
			else  // input value < las point value. It is not the largest value so far.
				calCase = CAL_TABLE_NOT_FULL_NOT_LARGEST;		// calCase

			for ( i = 0; i < (MAX_CAL_POINTS - 1); i++)    {
				if ( pSensorCal-> value[i] == pSensorCal-> value[ i + 1 ])
					break;
			} 
			// locate points at the first empty entry.
			// guN.b.b1 points at the first empty entry.
			locate = i + 1;      // point to next of the cal point that has the first occurance of the same weight
		}
		else  { // the last two cal points are not the same weight, means the table is full.
			// gb_i = last, by now.
			if ( fValue >= pSensorCal-> value[i] )
				calCase = CAL_TABLE_FULL_LARGEST_SO_FAR;      // input value is the largest so far. // calCase
			else
				calCase = CAL_TABLE_FULL_NOT_LARGEST;         // input value is not the largest value.	// calCase
		}
	} // end if ( calCase != CAL_VALUE_ALREADY_EXIST )  {}

	switch( calCase )   {
		case CAL_TABLE_NOT_FULL_LARGEST_SO_FAR:  // table is not full and the input value is the largest value encountered so far.
			// fill the uncal points to the input value and counts. locate is pointing a the first empty entry.
			for ( i = locate; i < MAX_CAL_POINTS; i++)  { 
				pSensorCal-> value[  i] = fValue;
				pSensorCal-> adcCnt[ i] = adcCnt;
			} 
			break;
		case CAL_TABLE_NOT_FULL_NOT_LARGEST: // table is not full and the input value is not the largest.
			locate--;     // points to the cal point that has the first occurrence of the same value when the table is not full.
			while( locate > 0)  {
				if ( fValue > pSensorCal-> value[ locate - 1] )  { // if input value > previous cal point value.
					// set the input value to the new cal point and break the while loop.
					pSensorCal-> value[  locate ] = fValue;
					pSensorCal-> adcCnt[ locate ] = adcCnt;
					break;							// done, break the while loop
				}
				else  {// input value < prev cal point value
					// copy the prev cal point to current point and continue to search a cal point that has value less than the input value.
					// This has an effect of shifted cal points that has value > input value one position to the higher entry.
					pSensorCal-> value[  locate ] = pSensorCal-> value[  locate - 1 ];
					pSensorCal-> adcCnt[ locate ] = pSensorCal-> adcCnt[ locate - 1 ];
					locate--;
				}
			}
			break;
		case CAL_TABLE_FULL_LARGEST_SO_FAR: // table is full and the input value is the largest so far.
			// put the largest value at the end of the table.
			pSensorCal-> value[  MAX_CAL_POINTS - 1 ] = fValue;
			pSensorCal-> adcCnt[ MAX_CAL_POINTS - 1 ] = adcCnt;
			break;
		case CAL_TABLE_FULL_NOT_LARGEST:  // table is full and the input value is not the largest.
			for ( i = 0; i < (MAX_CAL_POINTS - 1); i++)   { // try to find a cal point for the input weight; that it is > prev cal weight and < then next cal weight
				if ( (fValue > pSensorCal-> value[i]  ) && (fValue < pSensorCal-> value[ i + 1 ]) )  {
					if ( i == 0)  {  // cal point 0 weight < input weight
						pSensorCal-> value[ 1 ] = fValue;
						pSensorCal-> adcCnt[ 1] = adcCnt;
						break;				// done, break the for loop.
					}
					else if ( i ==  (MAX_CAL_POINTS - 2)) {  // cal point #last_two weight < input weight
						pSensorCal-> value[  MAX_CAL_POINTS - 2 ] = fValue;
						pSensorCal-> adcCnt[ MAX_CAL_POINTS - 2 ] = adcCnt;
						break;				// done, break the for loop.
					}
					else {  // cal point number > 0 and < last_two.
						if ( (fValue - pSensorCal-> value[i] ) < (pSensorCal-> value[i+1] - fValue))   {  // if input value is closer to prv cal point value
							pSensorCal-> value[  i ] = fValue;
							pSensorCal-> adcCnt[ i ] = adcCnt;
						}
						else   { // input value is closer to next cal value.
							pSensorCal-> value[  i + 1 ] = fValue;
							pSensorCal-> adcCnt[ i + 1 ] = adcCnt;
						}
						break;				// done, break the for loop.
					} // end if (i == 0)  {} else if {} else {}
				} // end if ( (fValue > gSensorCal.value[i]  ) && (fValue < gSensorCal.value[ i + 1 ]) )  {}
			} // end for (i = 0; i < (MAX_CAL_POINTS - 1); i++) {}
			break;
		default:
			break;
	} // end switch( calCase )    {}

	pSensorCal-> status=0;
	for ( i = 0; i < MAX_CAL_POINTS - 1; i++)  { 
		if ( pSensorCal-> adcCnt[ i + 1 ] -  pSensorCal-> adcCnt[ i ] ) 
			pSensorCal-> status++;										// count the unique entries as the number of cal points.
	} 
	return CAL_ERROR_NONE;
} // end cal_build_table()



/**
 * If the specified sensor channel is in cal mode, then it will not allow to start a new calibration.
 *
 * @param  pSensorCal -- pointer to calibration data structure of this sensor
 *
 * @return 0 if it allows to cal else return error code.
 *
 *
 * History:  Created on 2006/12/22 by Wai Fai Chin
 */


BYTE  cal_allows_new_cal( SENSOR_CAL_T *pSensorCal )
{
	BYTE errorStatus;
	
	errorStatus = CAL_ERROR_NOT_ALLOW;			// assumed failed.
	if  (  pSensorCal-> status >= CAL_STATUS_COMPLETED )	{	// if this sensor channel is NOT in cal mode, 
		cal_table_init( pSensorCal );
		pSensorCal-> countby.decPt = 0;
   		errorStatus = CAL_ERROR_NONE;
		pSensorCal-> status = CAL_STATUS_GOT_UNIT_CAP;
	}

  	return errorStatus;
} // end cal_allows_new_cal()


/**
 * It normalizes user input countby for cal and see if ADC has enought resolution 
 * to handle the normalized countby.
 *
 * Once it has one cal point, it will not allow user to change countby again
 * until completed or reset calibrate operation.
 *
 * @param  pSensorCal -- pointer to calibration data structure of this sensor
 * @return 0, CAL_STATUS_GOT_COUNTBY if countby is ok else return error code.
 * @post pSensorCal->countby has new normalized value.
 *
 *
 * History:  Created on 2008/06/27 by Wai Fai Chin
 */

BYTE  cal_normalize_input_cal_countby( SENSOR_CAL_T *pSensorCal )
{
	UINT32	u32Tmp;
	BYTE	errorStatus;

	errorStatus = CAL_ERROR_NONE;							// assumed passed.
	// once it has one cal point, it will not allow user to change countby again until completed or reset calibrate operation.
	// if it has channel#, unit, and capacity and never had calibrated any value, then it can changing countby value.
	if (  pSensorCal-> status >= CAL_STATUS_GOT_UNIT_CAP &&  pSensorCal-> status < CAL_STATUS_COMPLETED ) {
		u32Tmp = (UINT32) pSensorCal-> capacity;
		if ( u32Tmp > 1 )	{			// ensured capacity > 1
			cal_normalize_input_countby( &( pSensorCal-> countby) );
			pSensorCal-> status = CAL_STATUS_GOT_COUNTBY;
		} // end if ( u32Tmp > 1 )	{}
	} // end if (  pSensorCal-> status >= CAL_STATUS_GOT_UNIT_CAP &&  pSensorCal->status < CAL_STATUS_COMPLETED ) {}
	else {
		errorStatus = CAL_ERROR_CANNOT_CHANGE_COUNTBY;
	}

	return errorStatus;
} // end cal_normalize_input_cal_countby()


/**
 * It scales float type of countby to its original value based on its decimal point.
 *
 * @param  pCB  -- pointer to a countby.
 *
 * History:  Created on 2007/01/03 by Wai Fai Chin
 */

void  cal_scale_float_type_countby( MSI_CB *pCB)
{
	INT8	dPt;
	dPt = pCB-> decPt;
	// scale float type of countby to its original value and normalize capacity
	while( dPt > 0 )   {
		pCB-> fValue /= 10.0;	// scale float type of countby to its original value.
		dPt--;
	}

	while( dPt < 0 )   {
		pCB-> fValue *= 10.0;	// scale float type of countby to its original value.
		dPt++;
	}
}  // end cal_scale_float_type_countby()


/**
 * It scales float type of countby to its original value based on its decimal point.
 *
 * @param	iValue	--	countby in integer
 * @param	dPt		--	decimal point, positive value move to left place while negative move to the right.
 *
 * @return	scaled float type countby based on a give integer countby and decimal point.
 *
 * History:  Created on 2009/05/07 by Wai Fai Chin
 */

float  cal_scale_float_countby( UINT16 iValue, INT8 dPt)
{
	float fValue;
	
	fValue = (float) iValue;
	// scale float type of countby to its original value and normalize capacity
	while( dPt > 0 )   {
		fValue /= 10.0;		// scale float type of countby to its original value.
		dPt--;
	}

	while( dPt < 0 )   {
		fValue *= 10.0;		// scale float type of countby to its original value.
		dPt++;
	}
	
	return fValue;
}  // end cal_scale_float_countby(,)



/**
 * It normalizes countby to 1, 2, 5,,, 10, 20, 50 or 0.1, 0.2, 0.5,,, 0.001,0.002, 0.005 etc...
 *
 * @param  pCB  -- pointer to a countby.
 * @post   *pCB has new countby value.
 *
 *
 * History:  Created on 2007/01/02 by Wai Fai Chin
 */

void  cal_normalize_countby( MSI_CB *pCB)
{
		// Normalize countby of maxcounts of a given capacity:
		while( pCB-> iValue == 0)	{
			pCB-> fValue *= 10.0;
			pCB-> iValue = (UINT16)(pCB-> fValue);
			pCB-> decPt++;
			if ( pCB-> decPt > 20 )		// prevent get stuck in this endless loop.
				break;
		}

		while( pCB-> iValue >= 10 )	{
			pCB-> fValue	/= 10.0;
			pCB-> iValue	/= 10;
			pCB-> decPt--;
		}
   
		if ( pCB-> fValue > 5.0 )	{
			pCB-> iValue  = 1;
			pCB-> decPt--;
		}
		else if ( pCB-> fValue > 2.0 )
			pCB-> iValue = 5;
		else if ( pCB-> fValue > 1.0 )
	 		pCB-> iValue = 2;
		else
			pCB-> iValue = 1;

} // end cal_normalize_countby()

/**
 * It normalizes user input countby. 
 *
 * @param  pCB  -- pointer to a countby.
 * @post   *pCB has new countby value.
 *
 * History:  Created on 2007/08/01 by Wai Fai Chin
 */

void  cal_normalize_input_countby( MSI_CB *pCB)
{

	pCB-> iValue = (UINT16) (pCB-> fValue);
	pCB-> decPt = 0;
    cal_normalize_countby( pCB );						// normalize user entered countby.
	pCB-> fValue = (float) pCB-> iValue;
	cal_scale_float_type_countby( pCB );				// scale float type of countby to its original value.

} // end cal_normalize_input_countby()


/**
 * It normalizes user input countby. 
 *
 * @param  pCB  -- pointer to a countby.
 * @post   *pCB has new countby value.
 *
 * History:  Created on 2007/08/01 by Wai Fai Chin
 */

BYTE  cal_normalize_verify_input_countby( MSI_CB *pCB)
{
	BYTE	errorStatus;
	errorStatus = CAL_ERROR_NONE;			// assumed passed.
	cal_normalize_input_countby( pCB );			
	return errorStatus;
} // end cal_normalize_verify_input_countby()


/**
 * Calibrating zero value.
 *
 * @param  channel	-- sensor channel number or id.
 * @param  pSensorCal -- pointer to calibration data structure of this sensor
 * @return 0 if it allows to cal else return error code.
 *
 * @note It allows to zero cal if it already has a completed calibration before.
 *   This allow to subsequent cal or Rcal.
 *
 * History:  Created on 2008/07/18 by Wai Fai Chin
 */

BYTE  cal_zero_point( BYTE channel, SENSOR_CAL_T *pSensorCal )
{
	BYTE	errorStatus;
	LOADCELL_T *pLc;

	errorStatus = CAL_ERROR_NONE;			// assumed passed.
	if ( channel < MAX_NUM_CAL_SENSORS ) {			// if n is a valid channel number.
		if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ channel ] )	{
			if ( CAL_STATUS_COMPLETED == pSensorCal-> status ) {	// if this sensor has a completed calibration table.
				cal_table_init( pSensorCal );						// just init the adc and value paire table. countby, unit, cap are remain the same.
				pSensorCal-> status = CAL_STATUS_GOT_COUNTBY;		// make this act as if calibrate from scratch.
				pLc = &gaLoadcell[ channel ];
				pLc-> runModes |= LC_RUN_MODE_IN_CAL;				// flag this loadcell is in calibration mode
				pLc-> runModes &= ~LC_RUN_MODE_NORMAL_ACTIVE;		// flag it is NOT in normal active mode that it may not has a cal table and it is in cal mode.
			}
			
			// Ensured that we have countby before start cal for zero.
			if ( CAL_STATUS_GOT_COUNTBY == pSensorCal-> status ) {
				pSensorCal-> temperature = gaLSensorDescriptor[4].value;		// TESTONLY.
				pSensorCal-> status = 0;										// start with cal point number 0
				errorStatus = cal_build_table( gaLSensorDescriptor[ channel ].curADCcount, 0.0, pSensorCal);
			}
		} // end if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ channel ] ) {}
	} // end if ( n < MAX_NUM_SENSORS )  {}
	else {
		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
	}
	
	return errorStatus;
} // end cal_zero_point()


/**
 * It saved calibrated data to nonvolatile memory and exit calibration mode.
 *
 * @param  channel		-- valid sensor channel number or id.
 * @param  pSensorCal	-- pointer to calibration data structure of this sensor
 * @return 0 if it had successfully calibrated the channel and saved the data.
 *
 *
 * History:  Created on 2008/07/10 by Wai Fai Chin
 * 2009/10/19 reset tare, zero weight, default to gross mode and cleared all totaling statistics. -WFC-
 */

BYTE  cal_save_exit( BYTE channel, SENSOR_CAL_T	*pSensorCal )
{
	BYTE	errorStatus;
	errorStatus = cal_save_sensor_cal_info( channel, pSensorCal );
	
	nv_cnfg_fram_default_loadcell_dynamic_data( channel );
	nv_cnfg_fram_default_totaling_statistics( channel );

	// MERGE_TASK: nvmem_save_a_loadcell_statistics_fram( channel );
	// MERGE_TASK: nvmem_recall_a_loadcell_statistics_fram( channel );
	errorStatus = cal_recall_a_sensor_cnfg( channel );
	return errorStatus;
} // end cal_save_exit()


/**
 * It recalls configuration settings of a specified sensor channel.
 *
 * @return 0== no error.
 *
 * History:  Created on 2009/05/18 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 */

BYTE  cal_recall_a_sensor_cnfg( BYTE channel )
{
	BYTE	errorStatus;
	LOADCELL_T *pLc;

	errorStatus = sensor_recall_config( channel );
	errorStatus |= nv_cnfg_fram_recall_a_sensor_feature( channel );
	if ( !errorStatus ) { // if no error
		sensor_assign_normal_filter( channel );
		
		if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ channel ] )	{
			pLc = &gaLoadcell[ channel ];
			// -WFC- 2010-08-30 pLc-> viewCB.unit = gabSensorShowCBunitsFNV[ channel ];
			// -WFC- 2010-08-30 pLc-> oldUnit = pLc-> viewCB.unit;					// this will prevent unit changed recompute weights and threshold by the loadcell_update_param().
			pLc-> oldUnit = pLc-> viewCB.unit = gabSensorViewUnitsFNV[ channel];		// -WFC- 2010-08-30
		}
		sensor_update_param( channel );
	}
	return errorStatus;
} // end cal_recall_a_sensor_cnfg()


/**
 * It saves sensor calibartion info to nonevolatile memory.
 *
 * For now, it just save with 1 temperature zone.
 * 
 * @param  n  -- Sensor number.
 * @param  pSensorCal -- pointer to calibration data structure of this sensor
 *
 * @return 0 == passed, none zero == failed.
 *
 * History:  Created on 2007/08/29 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 */


BYTE  cal_save_sensor_cal_info( BYTE n, SENSOR_CAL_T *pSensorCal )
{
	LOADCELL_T *pLc;
	BYTE i;
	BYTE cti;			// calibrate table index
	BYTE status;

	status = CAL_ERROR_NONE;
	if ( n < MAX_NUM_CAL_SENSORS ) {	// excluded spi temperature sensor.
		pSensorCal-> status = CAL_STATUS_COMPLETED;

		cti = cal_get_cal_table_num( n, gabCalTmpZone[n] );
		
		gaSensorCalNV[cti].capacity = pSensorCal-> capacity;
		gaSensorCalNV[cti].countby  = pSensorCal-> countby;
		gaSensorCalNV[cti].status   = pSensorCal-> status;
		gaSensorCalNV[cti].temperature = pSensorCal-> temperature;		// The temperature during calibration. always in Kelvin

		for ( i=0; i < MAX_CAL_POINTS; i++) {
			gaSensorCalNV[cti].adcCnt[ i ] = pSensorCal-> adcCnt[ i ];
			gaSensorCalNV[cti].value[  i ] = pSensorCal-> value[ i ];
		}

		gafSensorShowCapacityFNV[n ] = pSensorCal-> capacity;
		gafSensorShowCBFNV[ n ]      = pSensorCal-> countby.fValue;
		gawSensorShowCBFNV[ n ]      = pSensorCal-> countby.iValue;
		// -WFC- 2010-08-30 gabSensorShowCBunitsFNV[ n ] = pSensorCal-> countby.unit;
		gabSensorViewUnitsFNV[n]	 = gabSensorShowCBunitsFNV[ n ] = pSensorCal-> countby.unit; // -WFC- 2010-08-30
		gabSensorShowCBdecPtFNV[ n ] = pSensorCal-> countby.decPt;
		
		pLc = &gaLoadcell[ n ];
		// -WFC- 2010-08-30 pLc-> viewCB.unit = gabSensorShowCBunitsFNV[ n ];
		// -WFC- 2010-08-30 pLc-> oldUnit = pLc-> viewCB.unit;					// this will prevent unit changed recompute weights and threshold.
		pLc-> oldUnit = pLc-> viewCB.unit = gabSensorViewUnitsFNV[ n ];			// this will prevent unit changed recompute weights and threshold. // -WFC- 2010-08-30
		
		status = nv_cnfg_eemem_save_a_cal_table( cti );
		status = nv_cnfg_fram_save_a_sensor_feature( n );
		loadcell_update_param(n);
	}
	else {
		status = CAL_ERROR_WRONG_SENSOR_ID;
	}
	
	return status;
} // end cal_save_sensor_cal_info()


/**
 * It provides the cal table number of a given sensorID and temperature zone.
 * 
 * @param  		 n	-- Sensor number.
 * @param  tmpZone	-- temperature zone.
 *
 * @return cal table number.
 *
 * History:  Created on 2009/05/18 by Wai Fai Chin
 */


BYTE  cal_get_cal_table_num( BYTE n, BYTE tmpZone )
{
	BYTE cti;			// calibrate table index

	/*
	gaSensorCalNV[0] to gaSensorCalNV[2] are for loadcell of sensor0.
	gaSensorCalNV[3] to gaSensorCalNV[5] are for loadcell of sensor1;  each of these tables was calibrate
	in a different temperature zone. Sensor0 and Sensor1, each has 3 temperature zone per calibration table.
	The temperature zone difference must be at least 10 Kelvin.
	gaSensorCalNV[6] is for sensor2. gaSensorCalNV[7] is for sensor3.
	*/

	// get the entry index of a cal table.
	cti = sensor_get_cal_table_array_index( n );
	
	if (tmpZone > 2 ) // ensure that temperature zone is not out of bound.
		tmpZone = 0;
		
	switch (n) {
		case 0:
			if ( SENSOR_FEATURE_TEMPERATURE_CMP & gaLSensorDescriptor[n].conversion_cnfg ) {
				cti += tmpZone;
			}
			break;
		case 1:
			if ( SENSOR_FEATURE_TEMPERATURE_CMP & gaLSensorDescriptor[n].conversion_cnfg ) 
				cti += tmpZone;
			break;
	}
	return cti;
} // end cal_get_cal_table_num(,)

///
/**
 * It returns sensor ID based on the given cal table number.
 *
 * @param  		 tableNum	-- table number.
 *
 * @return sensor ID.
 *
 * History:  Created on 2010/05/21 by Wai Fai Chin
 */

const BYTE gcbSensorIDfromCalTableNum[] PROGMEM = { 0,0,0,1,1,1,2,3,6 };

BYTE  cal_get_sensor_id_from_table_num( BYTE tableNum )
{

	/*
	gaSensorCalNV[0] to gaSensorCalNV[2] are for loadcell of sensor0.
	gaSensorCalNV[3] to gaSensorCalNV[5] are for loadcell of sensor1;  each of these tables was calibrate
	in a different temperature zone. Sensor0 and Sensor1, each has 3 temperature zone per calibration table.
	The temperature zone difference must be at least 10 Kelvin.
	gaSensorCalNV[6] is for sensor2. gaSensorCalNV[7] is for sensor3.
	*/

	BYTE sensorID;
	memcpy_P ( &sensorID,  &gcbSensorIDfromCalTableNum[ tableNum ], sizeof(BYTE));
	return sensorID;
} // end cal_get_sensor_id_from_table_num(,)


/**
 * It generated a countby value based on capacity, maxcount and mini count of a loadcell type.
 *
 * @param  fCapacity	 	-- input capacity of a loadcell.
 * @param  pbStdCBIndex	 	-- output of index of standard countby of the countby table.
 * @param  pbWantedCBIndex	-- points to wanted countby index. 
 * @param  *pWantedCB		-- wanted countby index by the pbWantedCBIndex.
 *
 * @return	max number of possible countby table elements.
 *
 * @post   *pCB has new countby value.
 *
 *
 * History:  Created on 2009/06/17 by Wai Fai Chin
 * 2015-07-08 -WFC- extend maxcount from 10000 to 100000 for OILM
 */


BYTE cal_gen_countby_table( float fCapacity, BYTE *pbStdCBIndex, BYTE *pbWantedCBIndex, MSI_CB *pWantedCB )
{
	MSI_CB                cbTBL[ CAL_MAX_NUM_POSSIBLE_CB ];       // table of all possible countbys
	MSI_CB                tmpCB;            
	char                  dp;               
	BYTE                  i;                
	UINT32                u32V;
	UINT32                cntChkCap;        
	UINT32                cntChkVal;        
	UINT32                maxCounts;        
	UINT32                minCounts;        
	UINT32                stdCounts;        
		  
	// 2015-07-08 -WFC- for OILM maxCounts = 10000;
	maxCounts = 100000;		// 2015-07-08 -WFC- for OILM
	minCounts = 100;
	stdCounts = 3000;
	tmpCB.fValue = fCapacity / (float)(maxCounts);	// float portion of countby.
	tmpCB.iValue = (UINT16)(tmpCB.fValue);			// integer portion of countby.
	tmpCB.decPt = 0;

	cal_normalize_countby( &tmpCB);

	// by now, the tmpCB.value is the integer portion of normalized countby of maxcounts.
	*pbStdCBIndex = 0xFF;								// assumed it had not found the standard countby index.
	for(i = 0; i < CAL_MAX_NUM_POSSIBLE_CB; i++)   {
		cntChkCap = (UINT32)( fCapacity );
		cntChkVal = tmpCB.iValue;

		dp = tmpCB.decPt;						// use for scale back float value and capacity
		tmpCB.fValue = (float)(tmpCB.iValue);	// the normalized tmpCB.value was computed base on the fCapacity / (FLOAT32)(maxCounts)

		// scale float type of countby to its original value and normalize capacity
		while(dp > 0)   {
			cntChkCap   *= 10;					// normalize capacity
			tmpCB.fValue /= 10;				// scale float type of countby to its original value.
			dp--;
		}

		while(dp < 0)     {
			cntChkCap    /= 10;
			tmpCB.fValue  *= 10;				// scale float type of countby to its original value.
			dp++;
		}
		// end scale back float value to its original value and normalize capacity

		// normalized counts = normalized capacity  / normalized countby;
		u32V = cntChkCap / cntChkVal;
		if ( (stdCounts >= u32V ) && ( *pbStdCBIndex  == 0xFF))
			*pbStdCBIndex = i;				// found the standard countby and set the index to point to it.
		if ( minCounts  >  u32V)			// if normalized counts < minCounts, then
			break;                 			// we are completed generated the countby value table.

		cbTBL[i].fValue = tmpCB.fValue;
		cbTBL[i].iValue = tmpCB.iValue;
		cbTBL[i].decPt = tmpCB.decPt;

		// increament to the next countby value as 1, 2, 5, 1, 2, 5 etc..
		if ( tmpCB.iValue == 1)
			tmpCB.iValue = 2;
		else if ( tmpCB.iValue == 2)
			tmpCB.iValue = 5;
		else if ( tmpCB.iValue == 5)     {
			tmpCB.iValue = 1;
			tmpCB.decPt--;
		}
	} // end for(i = 0; i < CAL_MAX_NUM_POSSIBLE_CB; i++)   {}
	
	if ( *pbWantedCBIndex >= i )
		*pbWantedCBIndex = 0;
	
	/*
	pWantedCB-> fValue	= cbTBL[ wantedCBIndex ].fValue;
	pWantedCB-> iValue	= cbTBL[ wantedCBIndex ].iValue;
	pWantedCB-> decPt	= cbTBL[ wantedCBIndex ].decPt;
	*/
	
	*pWantedCB = cbTBL[ *pbWantedCBIndex ];
	
	return i;
} // end cal_gen_countby_table()



/**
 * It reduces a countby to the next low level countby. E.g. 5.0 will goes 2.0, 2.0 goes to 1.0,  1.0 goes to 0.5 etc..
 *
 * @param  pCB  -- pointer to a countby.
 * @post   *pCB has new countby value.
 *
 *
 * History:  Created on 2009/09/23 by Wai Fai Chin
 */

void  cal_next_lower_countby( MSI_CB *pCB)
{
	char	dp;               

	// reduce to the next countby value as 5, 2, 1, 0.5, 0.2, 0.1 etc..
	if ( pCB-> iValue == 5)
		pCB-> iValue = 2;
	else if ( pCB-> iValue == 2)
		pCB-> iValue = 1;
	else if ( pCB-> iValue == 1)     {
		pCB-> iValue = 5;
		pCB-> decPt++;
	}

	pCB-> fValue = (float) pCB-> iValue;
	dp = pCB-> decPt;

	// scale float type of countby to its original value and normalize capacity
	while(dp > 0)   {
		pCB-> fValue /= 10;			// scale float type of countby to its original value.
		dp--;
	}

	while(dp < 0)     {
		pCB-> fValue *= 10;			// scale float type of countby to its original value.
		dp++;
	}

} // end cal_normalize_countby()

