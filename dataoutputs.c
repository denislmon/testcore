/*! \file dataOutputs.c \brief data output manager related functions.*/
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
//     This module manages output behavior of each listener. A listener associated 
//  with a device ID of a registered listener device, and output stream type of the assigned sensor.
//  The stream router will manage to route it to one of following stream drivers: Uart, SPI, RF modem, USB, or ethernet etc...
//
// ****************************************************************************


#include  "dataoutputs.h"
#include  "sensor.h"
#include  <stdio.h>
#include  "adc_lt.h"
#include  "cmdparser.h"
#include  "stream_router.h"
#include  "nvmem.h"
#include  "cmdaction.h"

#include "loadcellfilter.h"
/// Array of pointers to IO_STREAM_CLASS objects.
// IO_STREAM_CLASS		*gpOstreamListener[ MAX_NUM_STREAM_LISTENER ]; 

extern void  cmd_act_set_defaults( BYTE cmd );

/// array of interval timers for IO_STREAM objects.
TIMER_T gatListenerTimer[ MAX_NUM_STREAM_LISTENER ];		// interval timers for listener IO_STREAM objects.

// the following will recall from nonevolatile memory during powerup.
															/// sensor channel number of the assigned IO_STREAM object
BYTE	gabListenerWantSensorFNV[ MAX_NUM_STREAM_LISTENER ];	//  sensor channel number of the assigned IO_STREAM object
															/// 0 == disabled,	1==command mode, 2==interval
BYTE	gabListenerModesFNV[ MAX_NUM_STREAM_LISTENER ];		//  0 == disabled,	1==command mode, 2==interval
															/// 0 == continuous, 1 tick = 50 mSec or depends on task timer tick.
BYTE	gabListenerIntervalFNV[ MAX_NUM_STREAM_LISTENER ];	//  0 == continuous, 1 tick = 50 mSec or depends on task timer tick.
																/// Device ID of registered listener
BYTE	gabListenerDevIdFNV[ MAX_NUM_STREAM_LISTENER ];	//  Device ID of registered listener
															///  0 == uart, 1 == uart1, 2 == uart2,  3 == rf modem stream type, 4 == spi.
BYTE	gabListenerStreamTypeFNV[ MAX_NUM_STREAM_LISTENER ];	//  0 == uart, 1 == uart1, 2 == uart2,  3 == rf modem stream type, 4 == spi.

/// number of lit segment of the system bar graph.
BYTE	gbBargraphNumLitSeg;

/// bar graph resolution defined as inverse of value per segment, so to speed up computation by use multiplication instead of division.
float	gfBargraphResolution;

/// time critical output timer.
TIMER_T gtTimeCriticalDataTImer;
/// Time critical data period, automatically output 6.6 times per second as specified by the software requirement 6 times per second.
#define	TIME_CRITICAL_DATA_PERIOD	(TT_100mSEC + TT_50mSEC)

/// toggle timer.
TIMER_T gtCyclerTimer;
/// toggle value alternate between 0 and 1;
BYTE gbCycler;

/**
 * It initialized IO_STREAM object output modes to control the behavior of each IO_STREAM object.
 *
 * @param  none
 * 
 * @return none
 *
 * History:  Created on 2007/01/16 by Wai Fai Chin
 */


void	data_out_init( void)
{
	BYTE i;

	gbBargraphNumLitSeg = gbCycler = 0;
	data_out_compute_bargraph_resolution();
	// initialize all output timers of IO_STREAM_CLASS object.
	for ( i = 0; i < MAX_NUM_STREAM_LISTENER;	i++) {
		timer_mSec_set( &gatListenerTimer[ i ], gabListenerIntervalFNV[ i ]);
	}
	timer_mSec_set( &gtTimeCriticalDataTImer, TIME_CRITICAL_DATA_PERIOD);
	timer_mSec_set( &gtCyclerTimer, TT_2SEC);

} // end data_out_init()


/**
 * The very first time powerup initialization.
 * It initialized IO_STREAM object output modes to control the behavior of each IO_STREAM object.
 *
 * @param  none
 * 
 * @return none
 *
 * History:  Created on 2007/02/06 by Wai Fai Chin
 */

void	data_out_1st_init( void)
{
	cmd_act_set_defaults( 0x1A );					// default IO_STREAM object output configuration
	cmd_act_set_defaults( 0x39 );					// default bargraph configuration.
	//gabStreamOutModesFNV[0] = DATA_OUT_MODE_INTERVAL;
	//gabListenerModesFNV[0] = LISTENER_MODE_COMMAND;
	// gabStreamOutModesFNV[1] = DATA_OUT_MODE_ADC_TEST;
} // end dataOut_init()


/**
 * It outputs sensor data based on each IO_STREAM object configuration settings.
 * An IO_STREAM ojbect could be a UART serail, SPI, RF modem, ethernet, usb etc...
 *
 * @param  none
 * 
 * @return none
 *
 * History:  Created on 2007/10/08 by Wai Fai Chin
 * 2011-01-06 -WFC- update sys error code for user every 0.5 seconds.
 * 2011-05-09 -WFC- output ADC_TEST_FILTERED and ADC_TEST_UNFILTERED only if it sample very slow speed to prevent out of stock space.
 * 2011-08-08 -WFC- Output number of sample if it was compiled with CONFIG_DEBUG_ADC == TRUE in ADC TEST MODE.
 * 2012-02-08 -WFC- added zero weight at the end for LISTENER_MODE_ADC_TEST mode. This help debug or test AZM or COZ.
 *
 */

extern UINT8  gTTimer_mSec;     // task timer update value in 50 millisecond per count. It uses by Timer1

extern LC_FILTER_MANAGER_T gFilterManager[];

void	data_out_to_all_listeners( void)
{
	BYTE i;					// loop variable
	BYTE n;					// string length
	BYTE sN;				// sensor number
	BYTE interval;
	BYTE bTrueFalse;
	// test only v
	// BYTE a, b;
	// test only ^
	char str[120];
	static BYTE gotAvalue;
	IO_STREAM_CLASS			*pOstream;	// pointer to a dynamic out stream object.
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to local sensor descriptor
	
	// 2011-01-06 -WFC- v update sys error code for user every 0.5 seconds.
	if ( timer_mSec_expired( &gtCyclerTimer ) ) {
		gbCycler++;
		gbCycler &= 7;		// cycling range 0 to 7.
		self_test_update_sys_error_code_for_user( gtSystemFeatureFNV.bargraphSensorID, gbCycler);
		timer_mSec_set( &gtCyclerTimer, TT_500mSEC);
	}
	// 2011-01-06 -WFC- ^

	for (i=0; i < MAX_NUM_STREAM_LISTENER;	i++) {			// walk through all IO_STREAM object.
		sN = gabListenerWantSensorFNV[i];
		if ( sN  < MAX_NUM_SENSORS ) {
			pSnDesc = &gaLSensorDescriptor[ sN ];
			interval = gabListenerIntervalFNV[i];
			
			if (stream_router_get_a_new_stream( &pOstream ) ) {		// get an output stream to this listener.
				pOstream-> type		= gabListenerStreamTypeFNV[i];		// stream type for the listener.
				pOstream-> status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
				pOstream-> sourceID	= gtProductInfoFNV.devID;
				pOstream-> destID	= gabListenerDevIdFNV[ i ];
				
				switch ( gabListenerModesFNV[i]  ) {
					case LISTENER_MODE_CUSTOMER_SPECIAL:
						if ( timer_mSec_expired( &gatListenerTimer[i] ) ) {
							n = self_test_customer_special_format_value( sN, str );
							stream_router_routes_a_ostream_now( pOstream, str, n );
							timer_mSec_set( &gatListenerTimer[i], interval);
						}
						break;
					case LISTENER_MODE_INTERVAL:
						if ( timer_mSec_expired( &gatListenerTimer[i] ) ) {
							n = sensor_format_data( sN, str );
							stream_router_routes_a_ostream_now( pOstream, str, n );
							timer_mSec_set( &gatListenerTimer[i], interval);
						}
						break;
					case LISTENER_MODE_ADC_TEST :
						// 2011-08-08 -WFC- v
						#if ( CONFIG_DEBUG_ADC == TRUE )
						if ( timer_mSec_expired( &gatListenerTimer[i] ) ) {
							n = (BYTE) sprintf_P( str, PSTR("\r\n%d; %d; %ld"), gTTimer_mSec,
													pSnDesc->numSamples, pSnDesc-> curRawADCcount );
							pSnDesc->numSamples = 0;
							stream_router_routes_a_ostream_now( pOstream, str, n );
							timer_mSec_set( &gatListenerTimer[i], interval);
						}
						#else
						// 2011-08-08 -WFC- ^
						if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {
							n = (BYTE) sprintf_P( str, PSTR("\r\n%lx; %ld; %ld"), pSnDesc-> rawAdcData,
												pSnDesc-> curRawADCcount, pSnDesc-> curADCcount);
							stream_router_routes_a_ostream_now( pOstream, str, n );
							if ( SENSOR_STATUS_GOT_VALUE & (pSnDesc-> status) ) {
								// n = (BYTE) sprintf_P( str, PSTR("; %.2f"), pSnDesc-> value);
								n = (BYTE) sprintf_P( str, PSTR("; %.4f"), pSnDesc-> value);
								stream_router_routes_a_ostream_now( pOstream, str, n );
								n = (BYTE) sprintf_P( str, PSTR("; %.4f"), *(gaLoadcell[0].pZeroWt));	// 2012-02-08 -WFC-
								stream_router_routes_a_ostream_now( pOstream, str, n );					// 2012-02-08 -WFC-
							}
						}
						#endif
						break;
					case LISTENER_MODE_ADC_TEST_INTERVAL_PACKET:
						if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {
							gotAvalue = TRUE;
						}
						if ( timer_mSec_expired( &gatListenerTimer[i] ) ) {
							if ( gotAvalue ) {
								n = data_out_manage_format_test_data_packet( i, str );
								stream_router_routes_a_ostream_now( pOstream, str, n );
								gotAvalue = FALSE;
							}
							timer_mSec_set( &gatListenerTimer[i], interval);
						}
						break;
					case LISTENER_MODE_ADC_TEST_FILTERED_INPUT:
					case LISTENER_MODE_ADC_TEST_UNFILTERED_INPUT:
/*
//						if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {
//							// formated as rawADC, (loadcell 0 inputs ADC counts) [loadcell 1 inputs ADC counts]
//							n = (BYTE) sprintf_P( str, PSTR("\r\n%lx; (%ld; %ld) [%ld; %ld]"), pSnDesc-> rawAdcData,
//									pSnDesc->prvADCcount, pSnDesc->prvRawADCcount, pSnDesc-> curADCcount, pSnDesc-> curRawADCcount);
//							stream_router_routes_a_ostream_now( pOstream, str, n );
//							if ( SENSOR_STATUS_GOT_VALUE & (pSnDesc-> status) ) {
//								n = (BYTE) sprintf_P( str, PSTR("; %.2f"), pSnDesc-> value);
//								stream_router_routes_a_ostream_now( pOstream, str, n );
//							}
//						}
*/
						if ( gabSensorSpeedFNV[ sN ] >= 15 ) { // if it sample very slow speed, then print out to avoid out of stack space. 2011-05-09 -WFC-, for High speed sample rate output, set baudrate to 11K.
							if ( LISTENER_MODE_ADC_TEST_UNFILTERED_INPUT == gabListenerModesFNV[i] )
								bTrueFalse = (SENSOR_STATUS_GOT_ADC_CNT | SENSOR_STATUS_GOT_UNFILTER_ADC_CNT )& (pSnDesc-> status);
							else
								bTrueFalse = (SENSOR_STATUS_GOT_ADC_CNT )& (pSnDesc-> status);

							if ( bTrueFalse ) {
								// formated as Timer TickCount; rawADCcnt; filtered ADC count; value
								n = (BYTE) sprintf_P( str, PSTR("\r\n%d; %ld; %ld"), gTTimer_mSec,
														pSnDesc-> curRawADCcount, pSnDesc-> curADCcount );
								stream_router_routes_a_ostream_now( pOstream, str, n );
								//	if ( SENSOR_STATUS_GOT_VALUE & (pSnDesc-> status) ) {
										n = (BYTE) sprintf_P( str, PSTR("; %.2f"), pSnDesc-> value);
										stream_router_routes_a_ostream_now( pOstream, str, n );
								//	}
							}
						}
						break;
				}// end switch()
				stream_router_free_stream( pOstream );	// put resource back to stream pool.
			} // end if (stream_router_get_a_new_stream( &pOstream ) ) {}
		} // end if ( sN  < MAX_NUM_SENSORS ) {}
	} // end for (i=0; i < MAX_NUM_STREAM_LISTENER;	i++) {}
} // end data_out_to_all_listeners()


/**
 * It formate the entired packet of sensor data.
 *
 * @param  listenerNum	-- Listener number.
 * @param  pOutStr		-- points to an allocated output string buffer.
 *
 * @return string length of the packet.
 *         0 length means failure.
 *
 *
 * History:  Created on 2009/02/25 by Wai Fai Chin
 */

BYTE data_out_manage_format_test_data_packet( BYTE listenerNum, char *pOutStr )
{
	BYTE	n;
	BYTE	sn;
	
	n = 0;
	if ( listenerNum < MAX_NUM_STREAM_LISTENER ) {
		// update the following incase user change cmd 0x15 and 3.
		sn = gabListenerWantSensorFNV[ listenerNum ];					// sn is the sensor ID or channel number.
		// if ( sn < MAX_NUM_SENSORS ) {  put this back once done with the project.
		if ( sn < MAX_NUM_LSENSORS ) {
			n = sprintf_P( pOutStr, gcStrFmt_PacketHdr_Cmd, CMD_START_CHAR,
				gtProductInfoFNV.devID,	gabListenerDevIdFNV[ listenerNum ], 0x15);
			pOutStr[n] = 'i';						// i flags it as 1D index array type answer.
			n++;
			n += sprintf_P( pOutStr + n, gcStrFmt_pct_d_c, (int) listenerNum, CMD_ITEM_DELIMITER );			// listener number
			n += sprintf_P( pOutStr + n, gcStrFmt_pct_d_c, (int) gabListenerDevIdFNV[listenerNum], CMD_ITEM_DELIMITER  ); // listener Device ID
			n += sprintf_P( pOutStr + n, gcStrFmt_pct_d_c, (int) gabListenerStreamTypeFNV[listenerNum], CMD_ITEM_DELIMITER  ); // listener stream type.
			n += sprintf_P( pOutStr + n, gcStrFmt_pct_d_c, (int) sn, CMD_ITEM_DELIMITER  );	// listener wanted sensor ID
			n += sprintf_P( pOutStr + n, gcStrFmt_pct_d_c, (int) gabListenerModesFNV[ listenerNum ], CMD_ITEM_DELIMITER  );	// output mode
			n += sprintf_P( pOutStr + n, gcStrFmt_pct_d_c, (int) gabListenerIntervalFNV[ listenerNum ], CMD_ITEM_DELIMITER  ); // interval time
			n += sensor_format_test_data_packet( sn, pOutStr + n );
			pOutStr[n] = CMD_END_CHAR;						
			n++;
		} // end if ( sn < MAX_NUM_LSENSORS ) {}.
	} // end if ( portID < MAX_NUM_STREAM_LISTENER ) 
	
	return n;
} // end data_out_manage_format_test_data_packet()


/**
 * It computes number of segment to be display for bargraph based on the specified input sensor value and its resolutions.
 * @post it updated gbBargraphNumLitSeg based on sensor value.
 *
 * History:  Created on 2010/05/11 by Wai Fai Chin
 */

void data_out_compute_number_bargraph_segment( void )
{
	BYTE	sensorState;
	BYTE	state;
	float	fV;

	if ( gtSystemFeatureFNV.bargraphSensorID < MAX_NUM_SENSORS ) {					// ensured sensor number is valid.
		switch ( gabSensorTypeFNV[ gtSystemFeatureFNV.bargraphSensorID ] )	{
			case SENSOR_TYPE_LOADCELL:
					sensorState = loadcell_get_value_of_type( gtSystemFeatureFNV.bargraphSensorID, gtSystemFeatureFNV.bargraphValueType, gtSystemFeatureFNV.bargraphValueUnit, &fV );
				break;
			#if ( CONFIG_INCLUDED_VS_MATH_MODULE ==	TRUE )
			case SENSOR_TYPE_MATH_LOADCELL:
					sensorState = vs_math_get_this_value( gtSystemFeatureFNV.bargraphSensorID, gtSystemFeatureFNV.bargraphValueType, gtSystemFeatureFNV.bargraphValueUnit, &fV );
				break;
			#endif
			// TODO: SENSOR_TYPE_REMOTE
		} // end switch() {}

		state = sensorState & ~SENSOR_STATE_IN_CAL_bm;
		if ( sensorState & SENSOR_STATE_IN_CAL_bm )		// if the sensor is in calibration mode
			gbBargraphNumLitSeg = 0;
		else if ( (state <= SENSOR_STATE_MAX_FOR_DISPLAYABLE) && (state > SENSOR_STATE_GOT_VALUE_IN_CAL)) {
			gbBargraphNumLitSeg = 0;					// assume it has no lit segment.
			fV = fV - gtSystemFeatureFNV.minValue1stSegment;
			if ( float_gte_zero(fV)) {
				gbBargraphNumLitSeg =  (BYTE) (fV * gfBargraphResolution);
				gbBargraphNumLitSeg++;
				if (gbBargraphNumLitSeg > gtSystemFeatureFNV.maxNumSegment )
					gbBargraphNumLitSeg = gtSystemFeatureFNV.maxNumSegment;
			}
		}
	}
} // end data_out_compute_number_bargraph_segment()

/**
 * It computes number of segment to be display for bar graph based on the specified input sensor value and its resolutions.
 *
 * History:  Created on 2010/05/11 by Wai Fai Chin
 */

void data_out_compute_bargraph_resolution( void )
{
	gfBargraphResolution = gtSystemFeatureFNV.maxValueLastSegment - gtSystemFeatureFNV.minValue1stSegment;
	if ( float_gte_zero( gfBargraphResolution ) )
		gfBargraphResolution = (float) (gtSystemFeatureFNV.maxNumSegment) / gfBargraphResolution;
	else
		gfBargraphResolution = 0.0001f;

	/*
	gfBargraphResolution = gtSystemFeatureFNV.maxValueLastSegment - gtSystemFeatureFNV.minValue1stSegment;
	if ( gtSystemFeatureFNV.maxNumSegment > 0 )
		gfBargraphResolution = gfBargraphResolution / (float) (gtSystemFeatureFNV.maxNumSegment);
	else
		gfBargraphResolution = 1.0f;
	*/

} // end data_out_compute_bargraph_resolution()


/**
 * It outputs time critical data to a host or remote meter.
 * @post it sends gbBargraphNumLitSeg in packet format to UART1.
 *
 * History:  Created on 2010/07/26 by Wai Fai Chin
 */

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_DSC )

void data_out_time_critical_data( void )
{
	BYTE n;
	char str[14];

	if ( timer_mSec_expired( &gtTimeCriticalDataTImer ) ) {
		str[0] = 'r';
		n = 1;
		n += sprintf_P( str + n, gcStrFmt_pct_d_c, (int) gtSystemFeatureFNV.bargraphSensorID, CMD_ITEM_DELIMITER );			// listener number
		sprintf_P( str + n, gcStrFmt_pct_d, (int) gbBargraphNumLitSeg );
		cmd_send_command( IO_STREAM_TYPE_UART_1, IO_STREAM_BROADCAST_ID, CMD_GET_NUMBER_LIT_SEGMENT, str);
		timer_mSec_set( &gtTimeCriticalDataTImer, TIME_CRITICAL_DATA_PERIOD);
	}
} // data_out_time_critical_data()

#endif
