/*! \file lightmon.c \brief light level related functions.*/
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
//   This is a high level module to convert filtered ADC count into light data
// base on the calibration table. It does not know how the sensor got the ADC data.
// It just know how to compute the light level. It formats output light data string to
// a caller supplied string buffer but it does not know how to output the data.
//
//            Application          Abstract Object        Hardware driver
//         +---------------+      +---------------+     +----------------+
//         |     light     |      |               |     |                |
//    -----|     MODULE    |<-----| SENSOR MODULE |<----| CPU ADC value  |
//         |  ADC to C, F  |      |      ADC      |     |                |
//         |               |      |               |     |                |
//         +---------------+      +---------------+     +----------------+
//
//
//
//
// Threshold light values will be saved in the same unit as the cal unit.
// If the user enter the value in unit different than cal unit, it will do
// convered entered value to cal unit and then saved it; these values have suffix of NV.
// light value is saved in display unit; these values have the suffix of FNV.
//
// NOTE:
// NV suffix stands for nonvolatile memory. It is in RAM and recalled from nonvolatile memory during powerup.
// FNV suffix stands for ferri nonvolatile memory. It is in RAM and frequently write to ferri memory everytime the value has changed.
// ****************************************************************************

#include  "config.h"
#include  "cmdparser.h"		// for CMD_ITEM_DELIMITER
#include  "calibrate.h"
#include  "sensor.h"
#include  "lightmon.h"
#include  "loadcell.h"
#include  "lc_tare.h"
#include  "lc_total.h"
#include  "lc_zero.h"
#include  "led_display.h"
#include  "nvmem.h"
#include  "commonlib.h"
#include  "panelmain.h"
#include  <math.h>
#include  <stdio.h>		// for sprintf_P()

#include	"scalecore_sys.h"		// 2015-01-16 -WFC-
#include	"self_test.h"			// 2015-01-16 -WFC-


LIGHT_SENSOR_T	gaLightSensor[ MAX_NUM_LIGHT_SENSOR ];

BYTE light_sensor_format_annunciator( LIGHT_SENSOR_T *pTS, char *pOutStr );


/**
 * It computes light level based on ADC counts of voltage from light sensor chip.
 *
 * @param  sn  -- Sensor channel number.
 *
 * @post updated contents in gaLSensorDescriptor[ sn ] and gaLightSensor[ sn ].
 *
 * @return none.
 *
 *   Vref == 1.1 V.     10 bits resultion.
 *   1 volt = 1/1.1 * 2^10 = 930.90909 = 931 ADC count.
 *   0.00107421875 volt = 1 ADC count.
What follows is not valid for this sensor change when we get reference meter
 *   0.0195 V = 1 C degree = 0.0195 V * 930.90909 count/ V = 18.152727255 == 18 count / degree.
 *   0 C = 0.4 Volt = 0.4V * 930.90909 count/V = 372.363636 = 372 count at 0 C;
 *	 Vo = 0.0195V * T + Vzero = 0.0195 * T + 0.4 V;
 *   Temp out ADC_count = 18 count * T + 372 count;
 *   Temp = (ADC_count - 372) / 18.152727255 = ( ADC_count - 372) * 0.055088 in C;
 *
 * History:  Created on 2009/01/08  by Wai Fai Chin
 * 2015-01-15 -WFC- ensured LED intensity value not over limit.
 * 2015-01-16 -WFC- Set intensity to lowest if in power saving state.
 */


void  light_sensor_compute_light( BYTE sn )
{
	BYTE					sensorStatus;
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	LIGHT_SENSOR_T			*pLS;			// points to a Light Sensor.
	UINT16					w;				// 2015-01-15 -WFC-
	
	if ( SENSOR_NUM_LIGHT_SENSOR == sn ) { // ensured sn is a light sensor index.
		pSnDesc = &gaLSensorDescriptor[ sn ];
		pLS = (LIGHT_SENSOR_T *) pSnDesc-> pDev;
		sensorStatus = pSnDesc-> status & ~SENSOR_STATUS_GOT_VALUE;		// Assume it has no valid value.
		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {						// if this sensor is enabled.
			if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {		// if got a new ADC count
				pLS-> status &= ~(LIGHT_STATUS_GOT_VALID_VALUE | LIGHT_STATUS_GOT_CAL_VALUE);	// assumed it has no valid weight
				pLS-> value = pSnDesc-> value = (float) pSnDesc-> curADCcount; // 2011-04-19 -WFC-
				// if LED configured as auto intensity AND not sleep mode AND not in panel setup menu mode, then...
				if ( SYS_POWER_SAVE_STATE_ACTIVE == gbSysPowerSaveState )			// 2015-01-16 -WFC-
					led_display_set_intensity( 0 );									// 2015-01-16 -WFC-
#if !CONFIG_4260IS
				else if (!(gtSystemFeatureFNV.ledIntensity) && !led_display_is_led_dimmed_sleep() &&
					(gbPanelMainRunMode != PANEL_RUN_MODE_PANEL_SETUP_MENU) )
				{
					// 2011-04-19 -WFC- led_display_send_hw_cmd( LED_HW_INTENSITY_CMD, (BYTE)(pSnDesc-> curADCcount / 64) );		// set LEDs to dim
					// led_display_set_intensity(((BYTE)(pSnDesc-> curADCcount >> 6)) );	// 2011-04-19 -WFC- dimming LED based on light sensor ADC count.
					led_display_set_intensity(((BYTE)(((pSnDesc-> curADCcount) + 2048) >> 8)) ); // 2013-10-31 -DLM- dimming LED based on Sc3.
				}
#endif
				pLS-> status	|= LIGHT_STATUS_GOT_VALID_VALUE;			// flag this loadcell has a valid weight
				sensorStatus	|= SENSOR_STATUS_GOT_VALUE;					// flag this sensor has a valid value.
			} // end if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc->status) ) {}
		} // end if ( SENSOR_STATUS_ENABLED...) if this sensor is enabled.
		pSnDesc-> status |= sensorStatus;					// update sensor status.
	} // end if valid sensor number.
} // end light_sensor_compute_light()


/**
 * It formats light level data in a dispaly format output based on countby.
 *
 * @param  pLs		-- pointer to light data structure; it is an input parameter.
 * @param  pOutStr  -- points to an allocated output string buffer.
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 */

BYTE light_sensor_format_output( LIGHT_SENSOR_T *pLS, char *pOutStr )
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	//char *ptr_P; this is also work
	PGM_P	ptr_P;
	char	unitName[7];
	BYTE	n;
	BYTE 	formatBuf[6];
	
	len = 0;
	if ( LIGHT_RUN_MODE_ENABLED & (pLS-> runModes)) {			// if this voltage monitor is enabled.
		if ( LIGHT_STATUS_GOT_VALID_VALUE & (pLS-> status)) {		// if this voltage monitor has valid value.
			memcpy_P ( &ptr_P, &gcUnitNameTbl[ pLS-> viewCB.unit ], sizeof(PGM_P));
			strcpy_P(unitName, ptr_P );
			fRound = float_round( pLS-> value, pLS-> viewCB.fValue);
			if ( pLS-> viewCB.decPt > 0 )
				precision = pLS-> viewCB.decPt;
			else
				precision = 0;
			float_format( formatBuf, 8, precision);
			len = (BYTE) sprintf( pOutStr, formatBuf, fRound );
			n = sprintf_P( pOutStr + len, gcStrFmt_pct_4sNewLine, unitName );
			len += n;
		}
		else
			len = (BYTE) sprintf_P( pOutStr, gcStrFmtDash10LfCr_P);		// print ---------- line.
	}
//	serial0_send_string( pOutStr );
	return len;	// number of char in the output string buffer.
} // end light_sensor_format_output()

/* 2011-04-11 -WFC- removed Pete's codes
BYTE light_sensor_format_output( LIGHT_SENSOR_T *pLS, char *pOutStr )
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	//char *ptr_P; this is also work
	PGM_P	ptr_P;
	char	unitName[5];
	BYTE	n;
	BYTE 	formatBuf[6];

	len = 0;
	if ( LIGHT_RUN_MODE_ENABLED & (pLS-> runModes)) {			// if this voltage monitor is enabled.
		if ( LIGHT_STATUS_GOT_VALID_VALUE & (pLS-> status)) {		// if this voltage monitor has valid value.
/ *			memcpy_P ( &ptr_P, &gcUnitNameTbl[ pLS-> viewCB.unit ], sizeof(PGM_P));
			strcpy_P( unitName, ptr_P );
			fRound = float_round( pLS-> value, pLS-> viewCB.fValue);
			if ( pLS-> viewCB.decPt > 0 )
				precision = pLS-> viewCB.decPt;
			else
				precision = 0;
			float_format( formatBuf, 8, precision);
			len = (BYTE) sprintf( pOutStr, formatBuf, fRound );
			n = sprintf_P( pOutStr + len, PSTR(" %4s\r\n"), unitName );
			len += n; * /
			len = (BYTE) sprintf( pOutStr, "%8.1f\n\r", pLS-> value );
		}
		else
			len = (BYTE) sprintf_P( pOutStr, gcStrFmtDash10LfCr_P);		// print ---------- line.
	}
//	serial0_send_string( pOutStr );
	return len;	// number of char in the output string buffer.
} // end light_sensor_format_output()
*/


/**
 * It formats light data in packet format:
 *  data; unit; annunciator
 *
 * @param  pLs		-- pointer to light data structure; it is an input parameter.
 * @param  pOutStr	 -- points to an allocated output string buffer.
 * @param  valueType -- value type such as gross, net, tare, total, etc...
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/01/16 by Wai Fai Chin
 * 2012-02-23 -WFC- Changed input parameter from LIGHT_SENSOR_T to LSENSOR_DESCRIPTOR_T, support new sensor_format_sys_annunciator() with extra parameters.
 * 				Added codes to support ADC count value type.
 */

//2012-02-23 -WFC- BYTE light_sensor_format_packet_output( LIGHT_SENSOR_T *pLS, char *pOutStr, BYTE valueType )
BYTE light_sensor_format_packet_output( LSENSOR_DESCRIPTOR_T *pSnDesc, char *pOutStr, BYTE valueType ) //2012-02-23 -WFC-
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	BYTE 	formatBuf[6];
	LIGHT_SENSOR_T *pLS;
	
	len = 0;
	pLS = (LIGHT_SENSOR_T *) pSnDesc-> pDev;
	if ( LIGHT_RUN_MODE_ENABLED & (pLS-> runModes)) {			// if this loadcell is enabled.
		if ( LIGHT_STATUS_GOT_VALID_VALUE & (pLS-> status)) {	// if this loadcell has valid value
			if ( SENSOR_VALUE_TYPE_ADC_COUNT == valueType )
				fRound = (float)(pSnDesc-> curADCcount);
			else
				fRound = float_round( pLS-> value, pLS-> viewCB.fValue);

			precision = 0;
			if ( SENSOR_VALUE_TYPE_ADC_COUNT != valueType ) {
				if ( pLS-> viewCB.decPt > 0 )
					precision = pLS-> viewCB.decPt;
				else
					precision = 0;
			}
			float_format( formatBuf, 8, precision);
			len += (BYTE) sprintf( pOutStr + len, formatBuf, fRound );
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
			len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLS-> viewCB.unit );
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
		}
	}
	
	if ( 0 == len ) {
		len = (BYTE) sprintf_P( pOutStr, gcStrDefaultSensorOutFormat );		// formated default 0;0;0; as weight;unit;numLitSeg;
	}
	else {
		if ( valueType & SENSOR_VALUE_TYPE_CAL_BARGRAPH_bm ) {				// if this sensor is selected for output bargraph, then format it for output.
			len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) gbBargraphNumLitSeg );
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
		}
		else {
			pOutStr[len]= '0';						// number of lit segment is 0
			len++;
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
		}
	}

	len += light_sensor_format_annunciator( pLS, pOutStr + len );
	// 2012-02-23 -WFC- len += sensor_format_sys_annunciator( pOutStr + len );
	len += sensor_format_sys_annunciator( pOutStr + len, 0 );		// 2012-02-23 -WFC-
	return len;	// number of char in the output string buffer.
} // end light_sensor_format_packet_output(,,)


/**
 * It formats annunciator of this light sensor.
 *  
 *
 * @param  pLs		-- pointer to light level data structure; it is an input parameter.
 * @param  pOutStr	 -- points to an allocated output string buffer.
 *
 * @post   format annuciators are stored in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/01/20 by Wai Fai Chin
 */
BYTE light_sensor_format_annunciator( LIGHT_SENSOR_T *pLS, char *pOutStr )
{
	BYTE len;
	len  = sprintf_P( pOutStr, gcStrFmt_pct_d, (int) pLS-> runModes );
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLS-> status );
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	pOutStr[len]= '0';		// dummy annunciator.
	len++;
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	return len;
} // end light_sensor_format_annunciator(,)


/**
 * It initialized light sensor when the application software runs the very 1st time of this device.
 * It initializes gaLightSensor[] data structure of sn based on
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaLightSensor[].
 *
 * @post   updated gaLightSensor[] data structure of this light sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 *  2011-04-21 -WFC- default light sensor viewing unit to LUX.
 */

void light_sensor_1st_init( BYTE sn )
{
	LIGHT_SENSOR_T	*pLS;	// points to a voltage monitor
	
	if ( SENSOR_NUM_LIGHT_SENSOR == sn ) { // ensured sn is a light sensor index.
		pLS = &gaLightSensor[ 0 ];
		pLS-> viewCB.fValue	= gafSensorShowCBFNV[ sn ];
		pLS-> viewCB.iValue	= gawSensorShowCBFNV[ sn ];
		pLS-> viewCB.decPt	= gabSensorShowCBdecPtFNV[ sn ];
		pLS-> viewCB.unit	= gabSensorShowCBunitsFNV[ sn ] = gabSensorViewUnitsFNV[ sn ] = SENSOR_UNIT_LIGHT; // 2011-04-21 -WFC-
	}
} // end light_sensor_1st_init()


/**
 * It initializes gaLightSensor[] data structure of sn based on
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaLightSensor[].
 *
 * @post   updated gaLightSensor[] data structure of this light sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 */

void light_sensor_init( BYTE sn )
{
	LIGHT_SENSOR_T	*pLS;	// points to a light sensor
	
	if ( SENSOR_NUM_LIGHT_SENSOR == sn ) { // ensured sn is a light sensor index.
		pLS = &gaLightSensor[ 0 ];
		pLS-> status   = 0;										// clear status
		pLS-> runModes = 0;										// disabled this loadcell.
		light_sensor_update_param( sn );
	}
} // end light_sensor_init()


/**
 * It updates parameter of gaLightSensor[] data structure of sn based on changed value of
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaLightSensor[].
 *
 * @post   updated gaLightSensor[] data structure of this light sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 */

void  light_sensor_update_param( BYTE sn )
{
	LIGHT_SENSOR_T	*pLS;	// points to a light sensor
	
	if ( SENSOR_NUM_LIGHT_SENSOR == sn ) { // ensured sn is a lighte sensor index.
		pLS = &gaLightSensor[ 0 ];
		//pLS-> viewCapacity	= gafSensorShowCapacityFNV[ sn ];		// NOTE that viewCapacity and viewCB could had already updated by cmd_current_unit_post_update()
		pLS-> viewCB.fValue	= gafSensorShowCBFNV[ sn ];
		pLS-> viewCB.iValue	= gawSensorShowCBFNV[ sn ];
		pLS-> viewCB.decPt	= gabSensorShowCBdecPtFNV[ sn ];
		pLS-> viewCB.unit	= gabSensorShowCBunitsFNV[ sn ];
		pLS-> runModes |= (gabSensorFeaturesFNV[  sn  ] & SENSOR_FEATURE_SENSOR_ENABLE);	// note that SENSOR_FEATURE_SENSOR_ENABLE must = TMPS_RUN_MODE_ENABLED,
	}
} // end light_sensor_update_param()


/**
 * It performs ADC to light level...
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaLightSensor[].
 *
 * @post   updated gaLightSensor[] data structure of this light sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 */

void light_sensor_tasks( BYTE sn )
{
	LIGHT_SENSOR_T	*pLS;	// points to a light sensor

	if ( SENSOR_NUM_LIGHT_SENSOR == sn ) { // ensured sn is a pcb temperature sensor index.
		pLS = &gaLightSensor[ 0 ];
		if ( LIGHT_RUN_MODE_ENABLED & (pLS-> runModes)) {		// if this voltage monitor is enabled.
			light_sensor_compute_light( sn );
		}
	}
} // end light_sensor_tasks()

