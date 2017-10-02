/*! \file voltmon_challenger3.c \brief voltmon related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//              Copyright (c) 2008 to 2013 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: independent
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2008/12/08 by Wai Fai Chin
// 
//   This is a high level module to convert filtered ADC count into a voltage data
// base on the calibration table. It does not know how the sensor got the ADC data.
// It just know how to compute the voltage. It formats output voltage data string to
// a caller supplied string buffer but it does not know how to output the data.
// 
//
//            Application          Abstract Object        Hardware driver
//         +---------------+      +---------------+     +----------------+
//         |VOLTAGE MONITOR|      |               |     |                |
//    -----|     MODULE    |<-----| SENSOR MODULE |<----| CPU ADC MODULE |
//         | ADC to VOLTAGE|      |      ADC      |     |                |
//         |               |      |               |     |                |
//         +---------------+      +---------------+     +----------------+
//
//	
//
//
// NOTE:
// NV suffix stands for nonvolatile memory. It is in RAM and recalled from nonvolatile memory during powerup.
// FNV suffix stands for ferri nonvolatile memory. It is in RAM and frequently write to ferri memory everytime the value has changed.
// ****************************************************************************
// 2013-06-27 -WFC- split from voltmon.c for Challenger3.


//2011-12-06 -WFC- #include  "config.h"
#include  "cmdparser.h"		// for CMD_ITEM_DELIMITER
#include  "calibrate.h"
#include  "sensor.h"
#include  "voltmon.h"
#include  "loadcell.h"
#include  "lc_tare.h"
#include  "lc_total.h"
#include  "lc_zero.h"
#include  "nvmem.h"
#include  "commonlib.h"
#include  <math.h>
#include  <stdio.h>		// for sprintf_P()
#include  "bios.h"		// 2011-04-18 -WFC-

VOLTAGEMON_T	gaVoltageMon[ MAX_NUM_VOLTAGE_MONITOR ];
BYTE gbShutOffCnt;		// 2013-10-02 -WFC-

/*
*  ScaleCore3 VBAT1, VBAT2
*  Vs= Vb( 24.9/(330 + 24.9)). where Vs == sample voltage, Vb == battery voltage.
*  Vref == 1.2 V.     11 bits resolution.
*  1 volt = 1/1.2v * 2^11 = 1706.666 ADC count.
*  ScaleFactor is (354.9/24.9)/1706.666 = 0.0083513775;
*
*  Vs = VF( 200/(330 + 200)). where Vs == sample voltage, VF == input voltage
*  Vs = VF( 200/(330 + 200)). where Vs == sample voltage, VF == input voltage
*  Vref == 1.2 V.     11 bits resolution.
*  1 volt = 1/1.2v * 2^11 = 1706.666 ADC count.
*  ScaleFactor is 530/200/1706.666 = 0.0015527349;
*
*/

/// 1.2 Vref ScaleFactor
const float gcafVoltmonScaleFactor[ MAX_NUM_VOLTAGE_MONITOR ] PROGMEM = { 0.0083513775, 0.0083513775, 0.0015527349 };
//const float gcafVoltmonScaleFactor[ MAX_NUM_VOLTAGE_MONITOR ] PROGMEM = { 0.002087844375, 0.002087844375, 0.0015527349 };

/// offset
const float gcafVoltmonOffset[ MAX_NUM_VOLTAGE_MONITOR ] PROGMEM = { 17.104, 17.104, 3.18 };
// const float gcafVoltmonOffset[ MAX_NUM_VOLTAGE_MONITOR ] PROGMEM = { 4.275, 4.275, 3.18 };

// private methods
BYTE voltmon_format_annunciator( VOLTAGEMON_T *pVm, char *pOutStr );

BYTE voltmon_is_under_voltage_checking( BYTE sn, INT32 shutOffAdcCnt, INT32 underVBlinkAdcCnt, INT32 underVAdcCnt );	// 2012-09-25 -WFC-


/**
 * It computes voltage based on ADC counts of voltage drop across resistor network from sensor module.
 *
 * @param  sn  -- Sensor channel number.
 *
 * @post updated contents in gaLSensorDescriptor[ sn ] and gaVoltageMon[ sn ].
 *
 * @return none.
 *
 *   The sample point of the battery voltage is form by a voltage divider.
 *   Vs = Vb( 10/(120 + 10)). where Vs == sample voltage, Vb == battery voltage.
 *   Vref == 2.56 V.     10 bits resultion.
 *   1 volt = 1/2.56v * 2^10 = 409 ADC count.
 *   Vb = Vs(130/10) = Vs*13 = adcCnt * 13 / 409 = adcCnt * 0.031784841; for VBAT1 and VBAT2.
 *   Vb = Vs(144.9/24.9) = Vs*5.8192771 = adcCnt * 5.8192771 / 409 = adcCnt * 0.014228; for ScaleCore power source.
 *
 *   Vref == 1.1 V.     10 bits resultion.
 *   1 volt = 1/1.1 * 2^10 = 930.90909 = 931 ADC count.
 *   Vb = Vs(130/10) = Vs*13 = adcCnt * 13 / 930.909090909090909090909090909091 = adcCnt * 0.01396484375; for VBAT1 and VBAT2.
 *   Vb = Vs(144.9/24.9) = Vs*5.8192771 = adcCnt * 5.8192771 / 930.909 = adcCnt * 0.006251176581; for ScaleCore power source.
 *
 * default voltage scalefactor for VBAT1 monitor, VBAT2 and ScaleCore input power monitor are:
 * const float gcafVoltmonScaleFactor[ MAX_NUM_VOLTAGE_MONITOR ] PROGMEM = { 0.01396484375, 0.01396484375, 0.006251176581 };
 *
 * History:  Created on 2008/12/09  by Wai Fai Chin
 */

void  voltmon_compute_voltage( BYTE sn )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	VOLTAGEMON_T			*pVm;			// points to a voltage monitor
	BYTE					sensorStatus;
	
	if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
		//pVm = &gaVoltageMon[ sn - SENSOR_NUM_LAST_VOLTAGEMON ];
		pSnDesc = &gaLSensorDescriptor[ sn ];
		pVm = (VOLTAGEMON_T *) pSnDesc-> pDev;
		sensorStatus = pSnDesc-> status & ~SENSOR_STATUS_GOT_VALUE;		// Assume it has no valid value.
		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {						// if this sensor is enabled.
			if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {		// if got a new ADC count
				pVm-> status &= ~(VM_STATUS_GOT_VALID_VALUE | VM_STATUS_GOT_CAL_VALUE);	// assumed it has no valid value
																			// convert ADC count to voltage
				pVm-> value =
				pSnDesc-> value = pSnDesc-> curADCcount * pVm-> scaleFactor + pVm->offset;
				pVm-> status	|= VM_STATUS_GOT_VALID_VALUE;			// flag this loadcell has a valid weight
				sensorStatus	|= SENSOR_STATUS_GOT_VALUE;				// flag this sensor has a valid value.
			} // end if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc->status) ) {}
		} // end if ( SENSOR_STATUS_ENABLED...) if this sensor is enabled.
		pSnDesc-> status |= sensorStatus;					// update sensor status.
	} // end if valid sensor number.
} // end voltmon_compute_voltage()

/**
 * It initialized voltage monitor when the application software runs the very 1st time of this device.
 * It initializes gaVoltageMon[] data structure of sn based on
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaVoltageMon[].
 *
 * @post   updated gaVoltageMon[] data structure of this voltage monitor.
 *
 * History:  Created on 2008/12/10 by Wai Fai Chin
 * 2011-04-19 -WFC- default voltage countby to 0.01.
 * 2011-04-21 -WFC- default viewing unit to V.
 * 2013-06-28 -WFC- Added offset lookup value.
 */

void voltmon_1st_init( BYTE sn )
{
	VOLTAGEMON_T	*pVm;	// points to a voltage monitor
	BYTE n;

	if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
		n = sn - SENSOR_NUM_1ST_VOLTAGEMON;
		pVm = &gaVoltageMon[ n ];
		//pVm-> viewCapacity	= gafSensorShowCapacityFNV[ sn ];
		pVm-> viewCB.fValue	= gafSensorShowCBFNV[ sn ] = 0.01;			// 2011-04-19 -WFC-
		pVm-> viewCB.iValue	= gawSensorShowCBFNV[ sn ] = 1;
		pVm-> viewCB.decPt	= gabSensorShowCBdecPtFNV[ sn ] = 2;
		pVm-> viewCB.unit	= gabSensorShowCBunitsFNV[ sn ] = gabSensorViewUnitsFNV[ sn ] = SENSOR_UNIT_V;	// 2011-04-21 -WFC-
		memcpy_P ( &( pVm-> scaleFactor),  &gcafVoltmonScaleFactor[ n ],  sizeof(float));
		memcpy_P ( &( pVm-> offset),  &gcafVoltmonOffset[ n ],  sizeof(float));					// 2013-06-28 -WFC-

	}
} // end voltmon_1st_init()


/**
 * It initializes gaVoltageMon[] data structure of sn based on
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaVoltageMon[].
 *
 * @post   updated gaVoltageMon[] data structure of this voltage monitor.
 *
 * History:  Created on 2008/12/10 by Wai Fai Chin
 * 2011-04-12 -WFC- Added copy scaleFactor from flash memory lookup table.
 * 2013-06-28 -WFC- Added offset lookup value.
 * 2013-10-02 -WFC- Init gbShutOffCnt;
 */

void voltmon_init( BYTE sn )
{
	VOLTAGEMON_T	*pVm;	// points to a voltage monitor
	SENSOR_CAL_T	*pCal;	// points to a cal table
	BYTE			n;

	gbShutOffCnt = 0;
	if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
		n = sn - SENSOR_NUM_1ST_VOLTAGEMON;						// 2011-04-12 -WFC-
		pVm = &gaVoltageMon[ n ];
		if (sensor_get_cal_base( sn, &pCal ) )
			pVm-> pCal = pCal;
		else
			pVm-> pCal = 0;
		pVm-> status   = 0;										// clear status
		pVm-> runModes = 0;										// disabled this loadcell.
		memcpy_P ( &( pVm-> scaleFactor),  &gcafVoltmonScaleFactor[ n ],  sizeof(float));	// 2011-04-12 -WFC-
		memcpy_P ( &( pVm-> offset),  &gcafVoltmonOffset[ n ],  sizeof(float));				// 2013-06-28 -WFC-
		voltmon_update_param( sn );
	}
} // end voltmon_init()


/**
 * It selected the input channel of ADC chip that connect to a voltage monitor.
 * It is based on product id and version number.
 *
 * @param	vmIndex		-- index of voltage monitor array.
 * @param	pbChipID	-- pointer to chipID as output for caller
 *
 * @return ADC channel of ADC chip that connects to voltage monitor.
 * @post	assigned a chip ID to pbChipID.
 * @note caller must ensured that vmIndex range 0 to 2;
 *
 * History:  Created on 2009/10/26 by Wai Fai Chin
 * 2013-06-24 -WFC- Modified for ScaleCore3.
 */

BYTE voltmon_config_hardware( BYTE vmIndex, BYTE *pbChipID )
{
	BYTE channel;
	channel = vmIndex;							// Channel 0 <== VBAT1, Channel 1 <== VBAT2, channel 3 <== VF.
	*pbChipID = SENSOR_CHIP_CPU_ADC_A;
	return channel;
} // end voltmon_config_hardware()



/**
 * See if the specified voltage monitor is under voltage.
 * Under voltage is dependent on product ID.
 *
 * @param	sn		-- sensor number.
 *
 * @return true if under voltage.
 *
 * History:  Created on 2010/08/02 by Wai Fai Chin
 * 2011-04-18 -WFC- set voltage level flags in gbBiosSysStatus and power off device if under shut off level.
 * 2012-09-25 -WFC- rewrote to handle different threshold based on product and user defined model.
 */

BYTE voltmon_is_under_voltage( BYTE sn )
{
	INT32 shutOffAdcCnt, underVBlinkAdcCnt, underVAdcCnt;
	shutOffAdcCnt = VOLTMON_UNDER_VOLTAGE_SHUT_OFF_ADCCNT;
	underVBlinkAdcCnt = VOLTMON_UNDER_VOLTAGE_BLINK_ADCCNT;
	underVAdcCnt = VOLTMON_UNDER_VOLTAGE_ADCCNT;
	return voltmon_is_under_voltage_checking( sn, shutOffAdcCnt, underVBlinkAdcCnt, underVAdcCnt );
} // end voltmon_is_under_voltage()

/**
 * See if the specified voltage monitor is under voltage.
 * Under voltage is dependent on product ID.
 *
 * @param	sn					-- sensor number.
 * @param	shutOffAdcCnt		-- Voltage ADC count threshold to shut off the system.
 * @param	underVBlinkAdcCnt	-- Voltage ADC count threshold to blink low batter annunciator.
 * @param	underVAdcCnt		-- Voltage ADC count threshold to turn on low battery annunciator.
 *
 * @return true if under voltage.
 *
 * History:  Created on 2012-09-25 by Wai Fai Chin
 * 2013-10-02 -WFC- Added delay shut down count down logic.
 */

BYTE voltmon_is_under_voltage_checking( BYTE sn, INT32 shutOffAdcCnt, INT32 underVBlinkAdcCnt, INT32 underVAdcCnt )
{
	BYTE					status;
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	VOLTAGEMON_T			*pVm;			// points to a voltage monitor

	status = FALSE;
	if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
		pSnDesc = &gaLSensorDescriptor[ sn ];
		pVm = (VOLTAGEMON_T *) pSnDesc-> pDev;
		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {					// if this sensor is enabled.
			if ( VOLTMON_UNDER_VOLTAGE_SN == sn ) {
				if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {		// if got a new ADC count
					status = TRUE;
					if ( pSnDesc-> curADCcount < shutOffAdcCnt ) {
						gbBiosSysStatus |= ( BIOS_SYS_STATUS_VOLTAGE_BLINK_WARNING | BIOS_SYS_STATUS_UNDER_VOLTAGE ); // set both blink warning and under voltage flag.
						// 2013-10-02 -WFC- v
						gbShutOffCnt++;
						if ( gbShutOffCnt > 10 ) {
							 bios_power_off_by_shutdown_event();
							 gbShutOffCnt = 0;
						}
						// 2013-10-02 -WFC- ^
					}
					else if ( pSnDesc-> curADCcount < underVBlinkAdcCnt ) {
						gbBiosSysStatus |= ( BIOS_SYS_STATUS_VOLTAGE_BLINK_WARNING | BIOS_SYS_STATUS_UNDER_VOLTAGE ); // set both blink warning and under voltage flag.
					}
					else if ( pSnDesc-> curADCcount < underVAdcCnt ) {
						gbBiosSysStatus |= ( BIOS_SYS_STATUS_UNDER_VOLTAGE );				// set under voltage flag.
						gbBiosSysStatus &= ~( BIOS_SYS_STATUS_VOLTAGE_BLINK_WARNING );		// clear blink warning.
						status = TRUE;
					}
					else { // battery voltage are not under voltage.
						gbBiosSysStatus &= ~( BIOS_SYS_STATUS_VOLTAGE_BLINK_WARNING | BIOS_SYS_STATUS_UNDER_VOLTAGE ); // clear both blink warning and under voltage flag.
						status = FALSE;
						gbShutOffCnt = 0;	// 2013-10-02 -WFC-
					}
				}
			}
		} // end if ( SENSOR_STATUS_ENABLED...) if this sensor is enabled.
	} // end if valid sensor number.

	return status;
} // end voltmon_is_under_voltage_checking()

/**
 * It formats voltage monitor data in a dispaly format output based on countby.
 *
 * @param  pVm		-- pointer to voltage monitor structure; it is an input parameter.
 * @param  pOutStr  -- points to an allocated output string buffer.
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2008/12/12 by Wai Fai Chin
 */

BYTE voltmon_format_output( VOLTAGEMON_T *pVm, char *pOutStr )
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
	if ( VM_RUN_MODE_ENABLED & (pVm-> runModes)) {			// if this voltage monitor is enabled.
		if ( VM_STATUS_GOT_VALID_VALUE & (pVm-> status)) {		// if this voltage monitor has valid value.
			memcpy_P ( &ptr_P, &gcUnitNameTbl[ pVm-> viewCB.unit ], sizeof(PGM_P));
			strcpy_P(unitName,ptr_P);
			fRound = float_round( pVm-> value, pVm-> viewCB.fValue);
			if ( pVm-> viewCB.decPt > 0 )
				precision = pVm-> viewCB.decPt;
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
	return len;	// number of char in the output string buffer.
} // end voltmon_format_output()

/**
 * It formats voltage monitor data in packet format:
 *  data; unit; annunciator
 *
 * @param  pSnDesc	 -- pointer to sensor descriptor structure; it is an input parameter.
 * @param  pOutStr	 -- points to an allocated output string buffer.
 * @param  valueType -- value type such as gross, net, tare, total, etc...
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/01/16 by Wai Fai Chin
 * 2012-02-23 -WFC- Changed input parameter from VOLTAGEMON_T to LSENSOR_DESCRIPTOR_T, support new sensor_format_sys_annunciator() with extra parameters.
 * 				Added code to support ADC count value type.
 */

// 2012-02-23 -WFC- BYTE voltmon_format_packet_output( VOLTAGEMON_T *pVm, char *pOutStr, BYTE valueType )
BYTE voltmon_format_packet_output( LSENSOR_DESCRIPTOR_T *pSnDesc, char *pOutStr, BYTE valueType )
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	BYTE 	formatBuf[6];
	VOLTAGEMON_T *pVm;

	len = 0;
	pVm = (VOLTAGEMON_T *) pSnDesc-> pDev;
	if ( VM_RUN_MODE_ENABLED & (pVm-> runModes)) {			// if this loadcell is enabled.
		if ( VM_STATUS_GOT_VALID_VALUE & (pVm-> status)) {	// if this loadcell has valid value
			if ( SENSOR_VALUE_TYPE_ADC_COUNT == valueType )
				fRound = (float)(pSnDesc-> curADCcount);
			else
				fRound = float_round( pVm-> value, pVm-> viewCB.fValue);

			precision = 0;
			if ( SENSOR_VALUE_TYPE_ADC_COUNT != valueType ) {
				if ( pVm-> viewCB.decPt > 0 )
					precision = pVm-> viewCB.decPt;
				else
					precision = 0;
			}

			float_format( formatBuf, 8, precision);
			len += (BYTE) sprintf( pOutStr + len, formatBuf, fRound );
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
			len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pVm-> viewCB.unit );
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

	len += voltmon_format_annunciator( pVm, pOutStr + len );
	// 2012-02-23 -WFC- len += sensor_format_sys_annunciator( pOutStr + len );
	len += sensor_format_sys_annunciator( pOutStr + len, 0 );		// 2012-02-23 -WFC-
	return len;	// number of char in the output string buffer.
} // end voltmon_format_packet_output(,,)


/**
 * It formats annunciator of this voltage monitor.
 *
 *
 * @param  pVm		 -- pointer to voltage monitor structure; it is an input parameter.
 * @param  pOutStr	 -- points to an allocated output string buffer.
 *
 * @post   format annuciators are stored in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/01/20 by Wai Fai Chin
 */
BYTE voltmon_format_annunciator( VOLTAGEMON_T *pVm, char *pOutStr )
{
	BYTE len;

	len  = sprintf_P( pOutStr, gcStrFmt_pct_d, (int) pVm-> runModes );
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pVm-> status );
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	pOutStr[len]= '0';		// dummy annunciator.
	len++;
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	return len;
} // end voltmon_format_annunciator(,)


/**
 * It updates parameter of gaVoltageMon[] data structure of sn based on changed value of
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaVoltageMon[].
 *
 * @post   updated gaVoltageMon[] data structure of this voltage monitor.
 *
 * History:  Created on 2008/12/10 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 *
 */

void  voltmon_update_param( BYTE sn )
{
	VOLTAGEMON_T	*pVm;	// points to a voltage monitor

	if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
		pVm = &gaVoltageMon[ sn - SENSOR_NUM_1ST_VOLTAGEMON ];
		//pVm-> viewCapacity	= gafSensorShowCapacityFNV[ sn ];		// NOTE that viewCapacity and viewCB could had already updated by cmd_current_unit_post_update()
		pVm-> viewCB.fValue	= gafSensorShowCBFNV[ sn ];
		pVm-> viewCB.iValue	= gawSensorShowCBFNV[ sn ];
		pVm-> viewCB.decPt	= gabSensorShowCBdecPtFNV[ sn ];
		// 2010-08-30	-WFC-	pVm-> viewCB.unit	= gabSensorShowCBunitsFNV[ sn ];
		pVm-> viewCB.unit	= gabSensorViewUnitsFNV[sn];		// 2010-08-30	-WFC-
		if (gabSensorFeaturesFNV[ sn ] & SENSOR_FEATURE_SENSOR_ENABLE)
			pVm-> runModes	   |= SENSOR_FEATURE_SENSOR_ENABLE;
		else
			pVm-> runModes	   &= ~SENSOR_FEATURE_SENSOR_ENABLE;
	}
} // end voltmon_update_param()


/**
 * It performs ADC to voltage, power level check...
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaVoltageMon[].
 *
 * @post   updated gaVoltageMon[] data structure of this voltage monitor.
 *
 * History:  Created on 2008/12/10 by Wai Fai Chin
 * 2011-04-18 -WFC- check input voltage level.
 */

void voltmon_tasks( BYTE sn )
{
	VOLTAGEMON_T	*pVm;	// points to a voltage monitor

	if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
		pVm = &gaVoltageMon[ sn - SENSOR_NUM_1ST_VOLTAGEMON ];
		if ( VM_RUN_MODE_ENABLED & (pVm-> runModes)) {		// if this voltage monitor is enabled.
			voltmon_compute_voltage( sn );
			voltmon_is_under_voltage( sn );					// need to check for battery voltage level. 2011-04-18 -WFC-
		}
	}
} // end voltmon_tasks()


/**
 * It formats voltage monitor data in a dispaly format output based on countby.
 *
 * @param  pVm		-- pointer to voltage monitor structure; it is an input parameter.
 * @param  pOutStr  -- points to an allocated output string buffer.
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2008/12/12 by Wai Fai Chin
 */

/**
 * It fetches the value of specified type of a loadcell.
 *
 * @param	sn		-- sensore ID;
 * @param	pfV		-- point to floating variable that save the sensor value.
 *                     The sensor value is rounded based on countby of this sensor.
 * @return	state of this sensor.
 *
 * History:  Created on 2011-06-23 by Wai Fai Chin
 */

BYTE voltmon_get_value_of_type( BYTE sn, BYTE valueType, float *pfV, MSI_CB *pCB )
{
	BYTE state;
	VOLTAGEMON_T	*pVm;	// points to a voltage monitor

	state = SENSOR_STATE_DISABLED;
	if ( (sn <= SENSOR_NUM_LAST_VOLTAGEMON) && ( sn >= SENSOR_NUM_1ST_VOLTAGEMON) )	{
		pVm = &gaVoltageMon[ sn - SENSOR_NUM_1ST_VOLTAGEMON ];
		if ( VM_RUN_MODE_ENABLED & (pVm-> runModes)) {			// if this voltage monitor is enabled.
			if ( VM_STATUS_GOT_VALID_VALUE & (pVm-> status)) {		// if this voltage monitor has valid value.
				state = SENSOR_STATE_GOT_VALID_VALUE;
				if ( SENSOR_VALUE_TYPE_ADC_COUNT == valueType ) {
					*pfV = (float)(gaLSensorDescriptor[sn].curADCcount);
					pCB->decPt = 0;
					pCB->fValue = 1.0f;
					pCB->iValue = 1;
				}
				else {
					*pfV = pVm-> value;
					*pCB = pVm-> viewCB;
				}
			}
		}
	}
	return state;
} // end voltmon_get_value_of_type()

