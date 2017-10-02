/*! \file loadcell.c \brief loadcell related functions.*/
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
//   This is a high level module to convert filtered ADC count into a weight data base on the calibration table.
// It does not know how the sensor got the ADC data. It just know how to compute the weight,
// zero, AZM, tare, net or gross and other loadcell error checking. It formats output weight data string
// to a caller supplied string buffer but it does not know how to output the data.
//
//          Application           Abstract Object        Hardware driver
//         +--------------+      +---------------+     +----------------+
//         |   LOADCELL   |      |               |     | Linear Tech    |
//    -----|    MODULE    |<-----|SENSOR MODULE  |<----| ADC chip       |
//         | ADC to Weight|      |      ADC      |     | ADC value      |
//         |              |      |               |     |                |
//         +--------------+      +---------------+     +----------------+
//
//
// NOTE:
// NV suffix stands for nonvolatile memory. It is in RAM and recalled from nonvolatile memory during powerup.
// FNV suffix stands for ferri nonvolatile memory. It is in RAM and frequently write to ferri memory everytime the value has changed.
// ****************************************************************************

#include  "config.h"
#include  "cmdparser.h"		// for CMD_ITEM_DELIMITER
#include  "calibrate.h"
#include  "sensor.h"
#include  "loadcell.h"
#include  "lc_tare.h"
#include  "lc_total.h"
#include  "lc_zero.h"
#include  "nvmem.h"
#include  "commonlib.h"
#include  <math.h>
#include  <stdio.h>		// for sprintf_P()
#include  "scalecore_sys.h"		// 2012-02-23 -WFC-

LOADCELL_T	gaLoadcell[ MAX_NUM_PV_LOADCELL ];

/*! scale standard mode such as industry==0, NTEP==1, OIML==2, 3== One unit only.
	bit7 ==> motion detection 1==enabled;
	bit6 ==> AZM, 1== enabled;
	bit5 ==> zero on powerup, 1== enabled.

	
	bit1\__ industry==0, NTEP==1, OIML==2. 3==1unit.
	bit0/

*/

BYTE	gbScaleStandardModeNV;

/// motion detection period time. If no motion had occured after the period expired, then declared no motion.
BYTE	gabMotionDetectPeriodTimeNV[ MAX_NUM_PV_LOADCELL ];

/// motion detection band in d for each loadcell, range 0 to 255, 0==1/2 d, 1==1d.
BYTE	gabMotionDetectBand_dNV[ MAX_NUM_PV_LOADCELL ];

/*! array of loadcell operation modes.
	Each loadcell modes contains 

 \code
	bit7 to bit5 reserved

	bit4 ==> Net/Gross mode,  1== Net, 0== Gross.
	
	bit1\__ 00 == NO tare auto clear, 01 == tare auto clear, 10 == tare auto clear on total.
	bit0/
  \endcode
*/

BYTE	gabLcOpModesFNV[ MAX_NUM_PV_LOADCELL ];


/// pending time for stable weight so it could be zero, tare or total etc.. This pending time applies for zero, tare and total.
BYTE	gab_Stable_Pending_TimeNV[ MAX_NUM_PV_LOADCELL ];

/// user specified zero offset weight
float	gafLcZeroOffsetWtNV[ MAX_NUM_PV_LOADCELL ];

/*! Service counters
   I will not put this into the LOADCELL_T because I want to save RAM space and
   it does not belong to virtual sensors. It is a physical local loadcell service counters.
   A scale can be fully funtional without this feature.
*/

// 2010-08-30 -WFC- v
/// user programmable capacity threshold lift counters.
UINT32	gaulLiftCntFNV[ MAX_NUM_PV_LOADCELL ];		// 2010-08-30 -WFC-
/// user lift counter is the same as gaulLiftCntFNV except it let user reset and not use to for service notification.
UINT32	gaulUserLiftCntFNV[ MAX_NUM_PV_LOADCELL ];		// 2014-10-17 -WFC-
// 5% of Capacity counters use a lift counters
// 2010-08-30 -WFC- UINT16	gawLiftCntFNV[ MAX_NUM_PV_LOADCELL ];

/// 25% of Capacity counters
// 2010-08-30 -WFC- UINT16	gaw25perCapCntFNV[ MAX_NUM_PV_LOADCELL ];

/// Over Capacity counters or Overloaded counter.
UINT32	gaulOverloadedCntFNV[ MAX_NUM_PV_LOADCELL ];		// 2010-08-30 -WFC-

// Over Capacity counters or Overloaded counter.
// 2010-08-30 -WFC- UINT16	gawOverloadedCntFNV[ MAX_NUM_PV_LOADCELL ];

///  percentage of capcity to meet lift count requirement. 0 == 0.5% of capacity; 1 == 1%. 2==2% etc..
BYTE	gabLiftThresholdPctCapFNV[ MAX_NUM_PV_LOADCELL ];			// 2010-08-30 -WFC-

///  percentage of capcity as drop threshold. 0 == 0.5% of capacity; 1 == 1%. 2==2% etc..
BYTE	gabDropThresholdPctCapFNV[ MAX_NUM_PV_LOADCELL ];			// 2010-08-30 -WFC-

/// service status, bit0: 1== excess service count. bit1: 1==require user acknowledge service request. bit2: 1== excess overload count; bit3: 1== require user acknowledge too overload events.
BYTE	gabServiceStatusFNV[ MAX_NUM_PV_LOADCELL ];		// 2010-08-30 -WFC-
// 2010-08-30 -WFC- ^

/// Rcal on enabled flag of a loadcell, 0==off, 1==on.
BYTE	gabLoadcellRcalEnabled[ MAX_NUM_RCAL_LOADCELL ];

// float fDebugV;

const float	gafLoadcellUnitsTbl[ MAX_NUM_LOADCELL_UNITS ][ MAX_NUM_LOADCELL_UNITS ] PROGMEM = {
									// TO

/*to--->         LBS                 KGS                TONS                 MTON                OZ                   G                KILO-NEWTON     */
/*         ------------------- ------------------- ------------------- ------------------- ------------------- ------------------- ------------------- */
/*from */
/*LBS  */  {     1.0          ,      0.45359      ,      0.0005       ,      0.00045359   ,     16.0          ,    453.59        ,	     0.00444822246	},
/*KGS  */  {     2.204623     ,      1.0          ,      0.0011023    ,      0.001        ,     35.273968     ,   1000.0         ,      0.009806703	},
/*TONS */  {  2000.0          ,    907.1847       ,      1.0          ,      0.9071847    ,  32000.0          , 907184.7         ,      8.89644493  	},
/*MTON */  {  2204.623        ,   1000.0          ,      1.102311     ,      1.0          ,  35273.968        ,1000000.0         ,      9.806703		},
/*OZ   */  {     0.0625       ,      0.0283494    ,      0.00003125   ,      0.00002835   ,      1.0          ,     28.349375    ,      2.780139e-4    },
/*GRAM */  {     0.0022046    ,      0.001        ,      0.0000011023 ,      0.000001     ,      0.035273968  ,      1.0         ,      9.806703e-6	},
/*KNWT */  {   224.8089		  ,    101.971068951  ,      0.11240445   ,      0.101971     ,   3596.9424       , 101971.0         ,      1.0			}
};

const float	gafLoadcellUnitsSquareTbl[ MAX_NUM_LOADCELL_UNITS ][ MAX_NUM_LOADCELL_UNITS ] PROGMEM = {
									// TO

/*to--->            LBS                  KGS                TONS                 MTON                    OZ                     G               KILO_NEWTON     */
/*         --------------------- ---------------------- ----------------- ---------------------- ---------------------- ------------------- ------------------- */
/*from */
/*LBS  */  {     1.0           	, 0.2057438881       	,      0.00000025  	,  2.057438881e-7	,     256.0				, 205743.8881		, 1.9786683125352-5	},
/*KGS  */  { 4.860362572129    	, 1.0                	,  1.21506529e-6   	,     1.e-6			,  1.244252818465024e+3	, 1.e+6				, 9.6171426e-5		},
/*TONS */  { 4.e+6             	, 8.2298407991409e+5 	,      1.0		, 8.2298407991409e-1	,  1.024e+9				, 8.2298407991409e+11	, 79.1467325	},
/*MTON */  { 4.860362572129e+6 	, 1.e+6              	,  1.215089540721	,      1.0			,  1.244252818465024e+9	, 1.e+12			, 96171426			},
/*OZ   */  {     3.90625e-3    	, 8.0368848036e-4    	,  9.765625e-10		, 8.037225e-10		,      1.0				, 8.03687062890625e+2	, 7.729173e-8	},
/*GRAM */  { 4.86026116e-6     	, 1.e-6              	,  1.21506529e-12	, 1.e-12			,  1.244252818465024e-3	,      1.0			, 9.6171426e-11		},
/*KNWT */  {  50539.04151921	 , 10398.0989 			,  0.01263476		, 0.013980989		,  12937994.6			, 10398098903		,      1.0			}
};

// 2010-11-10 -WFC- unit conversion for capacity and countby.
const float	gafLoadcellCapacityUnitsTbl[ MAX_NUM_LOADCELL_UNITS ][ MAX_NUM_LOADCELL_UNITS ] PROGMEM = {
									// TO
// PHJ v
/*to--->         LBS                 KGS                TONS                 MTON                OZ                   G                KILO-NEWTON     */
/*         ------------------- ------------------- ------------------- ------------------- ------------------- ------------------- ------------------- */
/*from */
/*LBS  */  {     1.0          ,      0.5          ,      0.0005       ,      0.0005       ,     16.0          ,    500.0         ,	     0.00490336		},
/*KGS  */  {     2.0          ,      1.0          ,      0.001        ,      0.001        ,     32.0          ,   1000.0         ,      0.009806703	},
/*TONS */  {  2000.0          ,   1000.0          ,      1.0          ,      1.0          ,  32000.0          ,1000000.0         ,      9.806703   	},
/*MTON */  {  2000.0          ,   1000.0          ,      1.0          ,      1.0          ,  32000.000        ,1000000.0         ,      9.806703		},
/*OZ   */  {     0.0625       ,      0.03124987   ,      0.00003125   ,      0.00003124987,      1.0          ,     31.24987     ,      3.064579e-4    },
/*GRAM */  {     0.002        ,      0.001        ,      0.000001     ,      0.000001     ,      0.032        ,      1.0         ,      9.806703e-6	},
/*KNWT */  {   203.942137902  ,    101.971068951  ,      0.101971     ,      0.101971     ,   3263.0742       , 101971.0         ,      1.0			}
};
// PHJ ^



// private methods
BYTE loadcell_format_annunciator( LOADCELL_T *pLc, char *pOutStr );
// 2010-08-30 -WFC- void loadcell_change_countby( LOADCELL_T *pLc, BYTE lc, BYTE oldUnit, BYTE newUnit );
void loadcell_change_countby_capacity( LOADCELL_T *pLc, BYTE lc, BYTE newUnit );		// 2010-08-30 -WFC-
void loadcell_update_overload_threshold_real_loadcell( LOADCELL_T *pLc );
void loadcell_update_overload_threshold_virtual_loadcell( LOADCELL_T *pLc );

// 2012-02-23 -WFC- BYTE loadcell_format_weight( LOADCELL_T *pLc, char *pOutStr, BYTE valueType );			// 2011-08-10 -WFC-
BYTE loadcell_format_weight( LOADCELL_T *pLc, char *pOutStr, BYTE valueType, BYTE lc ); // 2012-02-23 -WFC-


/**
 * It computes loadcell weight based on ADC counts of loadcell sensor from sensor module.
 * It tracks net, gross, tare and also check overload and update service counter.
 *
 * @param  n  -- Sensor channel number.
 *
 * @post updated contents in gaLSensorDescriptor[ n ] and gLoadcell[n].
 *
 * @return none.
 *
 * History:  Created on 2007-08-15 by Wai Fai Chin
 * 2007-08-29 -WFC- Added codes to handle temperature zone calibration table.
 * 2010-08-30 -WFC- Added programmable lift weight threshold for lift counter.
 *  expand service counters from 16bits to 32bits. Removed 25perCapCnt related
 *  variables and logics.
 * 2011-02-02 -WFC-  Re-armed LC_CNT_STATUS_OK_CNT_OVERLOAD status bit if a load is equal or less than 75% of capacity.
 * 2011-03-28 -WFC- compute peak hold.
 * 2014-10-20 -WFC- added user lift counter logic.
 */

void  loadcell_compute_weight( BYTE n )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	SENSOR_CAL_T			*pCal;			// points to a cal table
	LOADCELL_T 				*pLc;			// points to a loadcell
    float					unitCnv;
	BYTE					cti;			// calibrate table index
	BYTE					sensorStatus;
	
	if ( n < MAX_NUM_LOADCELL )	{
		pSnDesc = &gaLSensorDescriptor[ n ];
		pLc = (LOADCELL_T *) pSnDesc-> pDev;
		cti = 0;
		// compute temperature factor. this should be done after the second temperature calibration.
		// if ( SENSOR_FEATURE_TEMPERATURE_CMP & pSnDesc->conversion_cnfg ) {
		//	cti = gbCalTmpZone;
		//}
		pCal = &(pLc-> pCal[cti]);
		sensorStatus = pSnDesc-> status & ~SENSOR_STATUS_GOT_VALUE;		// Assume it has no valid value.

		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {						// if this sensor is enabled.
			pLc-> status3 &= ~LC_STATUS3_GOT_UNFILTER_ADC_CNT;		// assumed no new unfiltered ADC count.
			if ( (SENSOR_STATUS_GOT_ADC_CNT | SENSOR_STATUS_GOT_UNFILTER_ADC_CNT) & (pSnDesc-> status) ) {	// if got a new ADC count or unfiltered ADC count
				pLc-> status3 |= LC_STATUS3_GOT_UNFILTER_ADC_CNT;
				pLc-> status3 &= ~LC_STATUS3_GOT_UNFILTER_VALUE;		// assumed it has no valid unfiltered value.
				pLc-> status &= ~(LC_STATUS_GOT_VALID_WEIGHT | LC_STATUS_GOT_CAL_WEIGHT);	// assumed it has no valid weight
				if ( (pLc-> runModes) & LC_RUN_MODE_IN_CAL ) {							// if this channel is in calibration mode,
					if ( SENSOR_STATUS_GOT_ADC_CNT  & (pSnDesc-> status) ) {			// if it has new ADC count
						sensor_get_cal_base( n, &pCal );
						if ( (pCal-> status > 0) && (pCal-> status <= MAX_CAL_POINTS) ||		// if it has at least 2 valid cal points OR
							(CAL_STATUS_COMPLETED == pCal-> status) ) { 						// it has a completed cal table.
																				// convert ADC count to raw weight
							pSnDesc-> value = adc_to_value( pSnDesc-> curADCcount,
															&(pCal-> adcCnt[0]),
															&(pCal-> value[0]));
							pLc-> rawWt		 = pSnDesc-> value;
							pLc-> status	|= LC_STATUS_GOT_CAL_WEIGHT;		// flag this loadcell has a calibrating weight
							sensorStatus	|= SENSOR_STATUS_GOT_VALUE;			// flag this sensor has a valid value.
						}// end if it has at least 2 valid cal points
						pLc-> runModes	&= ~LC_RUN_MODE_NORMAL_ACTIVE;			// flag it is NOT in normal active mode that it may not has a cal table and in cal mode.
					}
				} // end if this channel is calibrating,
				else if ( CAL_STATUS_COMPLETED == pCal-> status ){			// if this loadcell has a calibration table.
																			// 	convert ADC count to raw weight
					// compute weight data with raw ADC count, un-filtered ADC count.
					pLc-> grossWtUnFiltered  = adc_to_value( pSnDesc-> curRawADCcount,
													&(pCal-> adcCnt[0]),
													&(pCal-> value[0]));
					memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pCal-> countby.unit ][pLc-> viewCB.unit], sizeof(float));
					pLc-> grossWtUnFiltered  = pLc-> grossWtUnFiltered * unitCnv;
					pLc-> grossWtUnFiltered  = pLc-> grossWtUnFiltered + gafLcZeroOffsetWtNV[n];

					pLc-> grossWtUnFiltered  = pLc-> grossWtUnFiltered - *(pLc-> pZeroWt);
					pLc-> grossWtUnFiltered  = float_round( pLc-> grossWtUnFiltered, pLc-> viewCB.fValue);
					pSnDesc-> status |= SENSOR_STATUS_GOT_UNFILTER_VALUE;			// flagged it had unfiltered value.
					pLc-> status3 |= LC_STATUS3_GOT_UNFILTER_VALUE;

					if ( SENSOR_STATUS_GOT_ADC_CNT  & (pSnDesc-> status) ) {		// if it has new ADC count
						// compute weight data with filtered ADC count.
						pSnDesc-> value = adc_to_value( pSnDesc-> curADCcount,
														&(pCal-> adcCnt[0]),
														&(pCal-> value[0]));

						// TODO: may be future feature, normalize loadcell value with temperature factor if software temperature compensation is enabled.
						// convert raw weight from cal unit to display unit weight
						memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pCal-> countby.unit ][pLc-> viewCB.unit], sizeof(float));
						pSnDesc-> value = pSnDesc-> value * unitCnv;
						pLc-> rawWt		= pSnDesc-> value + gafLcZeroOffsetWtNV[n];

						pLc-> grossWt	= pLc-> rawWt - *(pLc-> pZeroWt);
						pLc-> grossWt	= float_round( pLc-> grossWt, pLc-> viewCB.fValue);

						loadcell_tracks_net_gross_tare(	pLc, n );					// it also update pLc-> netWt.

						// -WFC- 2011-03-28 v handle Peak hold computation.
						// if ( ( LC_RUN_MODE_PEAK_HOLD_ENABLED & (pLc-> runModes) ) &&		// if high speed peak hold enabled.
						if	(( SENSOR_STATUS_GOT_NEW_ADC_PEAK & (pSnDesc-> status))) {		// it will compute a peak hold regardless of peak hold enabled or not, as long as it got a new peak hold ADC count.
							// compute weight data with filtered ADC count.
							pLc-> peakHoldWt = adc_to_value( pSnDesc-> maxRawADCcount,
															&(pCal-> adcCnt[0]),
															&(pCal-> value[0]));
							// convert raw weight from cal unit to display unit weight
							memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pCal-> countby.unit ][pLc-> viewCB.unit], sizeof(float));
							pLc-> peakHoldWt	= pLc-> peakHoldWt * unitCnv;
							pLc-> peakHoldWt	= pLc-> peakHoldWt + gafLcZeroOffsetWtNV[n];
							pLc-> peakHoldWt	= pLc-> peakHoldWt - *(pLc-> pZeroWt);
							pLc-> peakHoldWt	= float_round( pLc-> peakHoldWt, pLc-> viewCB.fValue);
							if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 				// if loadcell is in NET mode
								pLc-> peakHoldWt = pLc-> peakHoldWt - *(pLc-> pTareWt);
							pLc-> status2 |= LC_STATUS2_GOT_NEW_PEAK_VALUE;
						}
						// -WFC- 2011-03-28 ^

						// update service counters
						if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 				// if loadcell is in NET mode
							unitCnv = pLc-> netWt;
						else
							unitCnv = pLc-> grossWt;

						cti = NO;	// NO need to save service counter to FRAM.
						if ( LC_CNT_STATUS_OK_CNT_LIFT & pLc-> serviceCnt_status ) {	// if OK to count the lift event counter
							if ( float_a_gte_b( unitCnv, pLc-> liftWtThreshold ) ) {	// if current weight >= liftWtThreshold
								if ( gaulLiftCntFNV[n] < 0xFFFFFFFF ) {
									gaulLiftCntFNV[n]++;
									if ( !(LC_SERVICE_LIFT_CNT_INTERVAL & gaulLiftCntFNV[ n ]) )  {// if mod 0x3fff, then
										gabServiceStatusFNV[n] |= ( LC_SERVICE_STATUS_LIFT_CNT_NEED_USER_ACK | LC_SERVICE_STATUS_LIFT_CNT_MET_SERVICE_CNT );
									}
								}
								if ( gaulUserLiftCntFNV[n] < 0xFFFFFFFF ) {	// 2014-10-20 -WFC-
									gaulUserLiftCntFNV[n]++;				// 2014-10-20 -WFC-
								}
								pLc-> serviceCnt_status &= ~LC_CNT_STATUS_OK_CNT_LIFT;	// flag not OK to count lift event until it unload the weight again.
								cti = YES;	// Yes, need to save service counter to FRAM.
							}
						}
						else if ( float_a_lte_b( unitCnv, pLc-> dropWtThreshold ) ) {	// if the load has unloaded,
							pLc-> serviceCnt_status |= LC_CNT_STATUS_OK_CNT_LIFT;	// flag OK to count lift event
						}

						if ( float_a_gte_b( pLc-> grossWt, pLc-> overloadThresholdWt ) ) {		// if current weight >= overload threshold
							pLc-> status	|= LC_STATUS_OVERLOAD;
							if ( LC_CNT_STATUS_OK_CNT_OVERLOAD & pLc-> serviceCnt_status ) {	// if OK to count the overload event counter
								if ( gaulOverloadedCntFNV[n] < 0xFFFFFFFF ) {
									gaulOverloadedCntFNV[n]++;
									if ( !(LC_SERVICE_OVERLOAD_CNT_INTERVAL & gaulOverloadedCntFNV[ n ]) )  {// if mod 0x3ff, then
										gabServiceStatusFNV[n] |= ( LC_SERVICE_STATUS_OVERLOAD_CNT_NEED_USER_ACK | LC_SERVICE_STATUS_OVERLOAD_CNT_MET_SERVICE_CNT );
									}
								}
								pLc-> serviceCnt_status &= ~LC_CNT_STATUS_OK_CNT_OVERLOAD;		// flag not OK to count overload event until it unload the weight again.
								cti = YES;	// Yes, need to save service counter to FRAM.
							}
						}
						else {
							pLc-> status	&= ~LC_STATUS_OVERLOAD;							// clear overload flag.
						}

						// 2011-02-02 -WFC- if ( float_a_lte_b( pLc-> rawWt, pLc-> dropWtThreshold ) ) 		// if the load has unloaded,
						if ( float_a_lte_b( pLc-> rawWt, pLc-> viewCapacity * 0.75f) ) 		// 2011-02-02 -WFC-  if the load is equal or less than 75% of capacity, then
							pLc-> serviceCnt_status |=  LC_CNT_STATUS_OK_CNT_OVERLOAD;	// flag OK to count overload event and 25% cap weight event.

						if ( cti )	// if need to save service counter to FRAM.
							nv_cnfg_fram_save_service_counters( n );

						unitCnv = pLc-> viewCapacity * ((float) gabPcentCapUnderloadFNV[n]) * 0.01f;		// under load threshold.
						// 2010-10-26 -WFC- if ( float_a_lt_b( pLc-> grossWt, unitCnv ) ) {		// if current weight < under load threshold
						if ( float_a_lt_b( pLc-> rawWt, unitCnv ) ) {							// if current raw weight < under load threshold.  2010-10-26 -WFC-
							pLc-> status	|= LC_STATUS_UNDERLOAD;
						}
						else {
							pLc-> status	&= ~LC_STATUS_UNDERLOAD;							// clear overload flag.
						}

						if ( pSnDesc->curRawADCcount <= LC_ADC_CNT_UNDER_RANGE_THRESHOLD ) {
							pLc-> status2	|= LC_STATUS2_UNER_RANGE;
						}
						else {
							pLc-> status2	&= ~LC_STATUS2_UNER_RANGE;							// clear under range flag.
						}

						if ( pSnDesc->curRawADCcount > LC_ADC_CNT_OVER_RANGE_THRESHOLD ) {
							pLc-> status2	|= LC_STATUS2_OVER_RANGE;
						}
						else {
							pLc-> status2	&= ~LC_STATUS2_OVER_RANGE;							// clear over range flag.
						}

						pLc-> status	|= LC_STATUS_GOT_VALID_WEIGHT;			// flag this loadcell has a valid weight
						pLc-> status2   &= ~LC_STATUS2_UN_CAL;					// flag it is not un cal.
						sensorStatus	|= SENSOR_STATUS_GOT_VALUE;				// flag this sensor has a valid value.
						pLc-> runModes &= ~LC_RUN_MODE_IN_CAL;					// flag this loadcell is not in calibration mode.
						pLc-> runModes	|= LC_RUN_MODE_NORMAL_ACTIVE;			// flag it is in normal active mode that it has a cal table and not in cal mode.
						pLc-> status3	|= LC_STATUS3_GOT_PREV_VALID_VALUE;		// flag it had a previous valid weight, this help to zero and tare functions, 2011-05-09 -WFC-
					}
				} // end else if ( CAL_STATUS_COMPLETED == pCal->status ) {}
				else {
					pLc-> status2 |= LC_STATUS2_UN_CAL;
				}
			} // end if ( (SENSOR_STATUS_GOT_ADC_CNT | SENSOR_STATUS_GOT_UNFILTER_ADC_CNT) & (pSnDesc->status) ) {} else {}
		} // end if ( SENSOR_STATUS_ENABLED...) if this sensor is enabled.
		pSnDesc-> status |= sensorStatus;					// update sensor status.
	} // end if ( n < MAX_NUM_LOADCELL )	{}
} // end loadcell_compute_weight()


/// OK to count the lift event
#define  LC_CNT_STATUS_OK_CNT_LIFT		1
/// OK to count the 25% of capacity
#define  LC_CNT_STATUS_OK_CNT_25PCT_CAP	2
/// OK to count the overload.
#define  LC_CNT_STATUS_OK_CNT_OVERLOAD	4


/**
 * It detects motion of a specified loadcell.
 *
 * @param  pLc	-- pointer to loadcell data structure.
 *
 * @post flagged motion bit of loadcell status if motion had occured.
 *
 * @note caller must ensured that this loadcell is enabled.
 *
 * History:  Created on 2007/10/26 by Wai Fai Chin
 * 2011-04-18 -WFC- check for small motion for power save feature.
 * 2011-08-30 -WFC- Only non-legal for trade mode can have motion detection period timeout.
 * 2011-10-25 -WFC- Legal For Trade mode has no motion timeout; it can only clear motion flag if dela weight < motionThresholdWeight.
 * 2011-11-21 -WFC- clear motion only if it has 3 consecutive stable counts in half motion period each count. This fixed a bug that user can zero a weight while it was in motion.
 * 2012-02-06 -WFC- Rewrote the entire function. Motion detection does not use motionDectedPeriodTimer if filter is enabled because filter is already has sampling time window.
 *                  Added lc input parameter for loadcell_detect_motion().
 * 2016-07-25 -DLM- The function now behaves just like any other scale and uses the motion window and timer to detect stable.
 */

// 2012-02-06 -WFC- v
// void loadcell_detect_motion( LOADCELL_T *pLc )
void loadcell_detect_motion( LOADCELL_T *pLc, BYTE lc )
{
	float	deltaWt;	//Delta weight

	// 2011-04-18 -WFC- check for small motion for power save feature. v
	if ( (LC_STATUS_GOT_VALID_WEIGHT | LC_STATUS_GOT_CAL_WEIGHT) & (pLc-> status)) {	// if this loadcell has valid value
//		if ( timer_mSec_expired( &(pLc-> motionDetectPeriodTimer) ) || (gaLSensorDescriptor[lc].cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK ) ) {	// if motion detection period had expired, OR filter is enabled, 2012-02-06 -WFC-
		deltaWt = fabs( pLc-> rawWt - pLc-> prvMotionWt); // get new delta 2016-04-25 -DLM-
		if ( deltaWt > pLc-> viewCB.fValue * 4.5 )	{									// if deltaWt > 4.5d, small motion
			if ( !(pLc-> runModes &  LC_RUN_MODE_PEAK_HOLD_ENABLED ))  {				// if not in peak hold mode.
				pLc->status3 |= LC_STATUS3_SMALL_MOTION;	// set small motion flag so system monitor module use this info to save power
			}
			else
				pLc->status3 &= ~LC_STATUS3_SMALL_MOTION;	// clear small motion flag.
		}
		else
			pLc->status3 &= ~LC_STATUS3_SMALL_MOTION;		// clear small motion flag.
		// 2011-04-18 -WFC- ^

		if ( SCALE_STD_MODE_MOTION_DETECT & gbScaleStandardModeNV ) {	// if motion detection enabled,
			if ( deltaWt > pLc-> motionThresholdWeight )	{	// if deltaWt > motion threshold weight, it is in motion.
				pLc-> status	|= LC_STATUS_MOTION;			// flag it is in motion
				timer_mSec_set( &(pLc-> motionDetectPeriodTimer), pLc-> motionDetectPeriodTime); // start the timer 2016-04-25 -DLM-
				pLc-> prvMotionWt	= pLc-> rawWt;
			} // end if ( deltaWt > pLc-> motionThresholdWeight ) {}  if in motion.
			else {	// it is STABLE.
				if ( timer_mSec_expired( &(pLc-> motionDetectPeriodTimer) ) )  { // 2016-07-25 -DLM-
					pLc-> status &= ~LC_STATUS_MOTION;				// clear motion flag
					pLc-> prvMotionWt	= pLc-> rawWt;
				}
			}
		} // end if ( SCALE_STD_MODE_MOTION_DETECT & gbScaleStandardModeNV )	{}
		else { // motion detection disabled
			pLc-> status &= ~LC_STATUS_MOTION;						// clear motion flag
			pLc-> prvMotionWt	= pLc-> rawWt;
		}
		pLc-> prvRawWt	= pLc-> rawWt;

		// timer_mSec_set( &(pLc-> motionDetectPeriodTimer), pLc-> motionDetectPeriodTime); // 2016-07-25 -DLM-
//		}
	}
} // end loadcell_detect_motion()
// 2012-02-06 -WFC- ^


/*  2012-02-06 -WFC- v
void loadcell_detect_motion( LOADCELL_T *pLc, BYTE lc )
{
	BYTE	bStdMode;	// 2011-08-30 -WFC-
	float	deltaWt;	//Delta weight

	// 2011-04-18 -WFC- check for small motion for power save feature. v
	if ( (LC_STATUS_GOT_VALID_WEIGHT | LC_STATUS_GOT_CAL_WEIGHT) & (pLc-> status)) {	// if this loadcell has valid value
		deltaWt = fabs( pLc-> rawWt - pLc-> prvRawWt);
		if ( deltaWt > pLc-> viewCB.fValue * 4.5 )	{									// if deltaWt > 4.5d, small motion
			if ( !(pLc-> runModes &  LC_RUN_MODE_PEAK_HOLD_ENABLED ))  {				// if not in peak hold mode.
				pLc->status3 |= LC_STATUS3_SMALL_MOTION;	// set small motion flag so system monitor module use this info to save power
			}
			else
				pLc->status3 &= ~LC_STATUS3_SMALL_MOTION;	// clear small motion flag.
		}
		else
			pLc->status3 &= ~LC_STATUS3_SMALL_MOTION;		// clear small motion flag.
	}
	// 2011-04-18 -WFC- ^

	if ( SCALE_STD_MODE_MOTION_DETECT & gbScaleStandardModeNV ) {	// if motion detection enabled,
		if ( (LC_STATUS_GOT_VALID_WEIGHT | LC_STATUS_GOT_CAL_WEIGHT) & (pLc-> status)) {	// if this loadcell has valid value
			// deltaWt = fabs( pLc-> rawWt - pLc-> prvRawWt);
			if ( deltaWt > pLc-> motionThresholdWeight )	{	// if deltaWt > motion threshold weight, it is in motion.
				// 2012-02-06 -WFC- v
//				pLc-> status	|= LC_STATUS_MOTION;			// flag it is in motion
//				pLc-> stableCnt = 0;							// clear stable counter 2011-11-18 -WFC-
//				pLc-> prvRawWt	= pLc-> rawWt;
//				timer_mSec_set( &(pLc-> motionDetectPeriodTimer), pLc-> motionDetectPeriodTime);

				if ( pLc-> stableCnt < 128 )		// > 128 is for motion count
					pLc-> stableCnt = 128;			// 0 to 127 is for stable count.
				pLc-> stableCnt++;
				if ( pLc-> stableCnt > 129 ) {
					pLc-> status	|= LC_STATUS_MOTION;			// flag it is in motion
					pLc-> prvRawWt	= pLc-> rawWt;
					pLc-> stableCnt = 128;							// clear consecutive motion counter
				}
				timer_mSec_set( &(pLc-> motionDetectPeriodTimer), pLc-> motionDetectPeriodTime);
				// 2012-02-06 -WFC- ^
			} // end if ( deltaWt > pLc-> motionThresholdWeight ) {}  if in motion.
			if ( LC_STATUS_MOTION & (pLc-> status)  ) {				// if there is motion,
				// 2012-02-06 -WFC- if ( timer_mSec_expired( &(pLc-> motionDetectPeriodTimer) ) ) {	// if motion detection period had expired, clear motion flag.
				if ( timer_mSec_expired( &(pLc-> motionDetectPeriodTimer) ) || (gaLSensorDescriptor[lc].cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK ) ) {	// if motion detection period had expired, OR filter is enabled,
//					// 2011-08-30 -WFC- v Only non-legal for trade mode can have motion detection period timeout.
//					bStdMode = gbScaleStandardModeNV & SCALE_STD_MODE_MASK;
//					if ( SCALE_STD_MODE_INDUSTRY == bStdMode || SCALE_STD_MODE_1UNIT == bStdMode ) {
//					// 2011-08-30 -WFC- ^
//						pLc-> status &= ~LC_STATUS_MOTION;					// clear motion flag
//					// 2011-10-25 -WFC- v
//					}
//					else {	// legal for trade mode has no motion timeout; it can only clear motion flag if dela weight < motionThresholdWeight.
//						deltaWt = fabs( pLc-> rawWt - pLc-> prvRawWt);
//						if ( deltaWt < pLc-> motionThresholdWeight )	{	// if deltaWt > motion threshold weight, it is in motion.
//							pLc-> prvRawWt	 = pLc-> rawWt;
//							// 2011-11-17 -WFC- v
//							// pLc-> status &= ~LC_STATUS_MOTION;				// clear motion flag
//							pLc-> stableCnt++;
//							if ( pLc-> stableCnt > 10 )
//								pLc-> status &= ~LC_STATUS_MOTION;				// clear motion flag
//							// 2011-11-17 -WFC- ^
//						}
//					}
//					// 2011-10-25 -WFC- ^

					// 2011-11-21 -WFC- v
					deltaWt = fabs( pLc-> rawWt - pLc-> prvRawWt);
					if ( deltaWt < pLc-> motionThresholdWeight )	{	// if deltaWt > motion threshold weight, it is in motion.
						if ( pLc-> stableCnt > 128 )		// > 128 is for motion count
							pLc-> stableCnt = 0;			// 0 to 127 is for stable count.
						pLc-> stableCnt++;
						timer_mSec_set( &(pLc-> motionDetectPeriodTimer), pLc-> motionDetectPeriodTime >> 1);
						if ( pLc-> stableCnt > 1 ) {
							pLc-> status &= ~LC_STATUS_MOTION;				// clear motion flag
							pLc-> prvRawWt	= pLc-> rawWt;
							pLc-> stableCnt = 0;							// clear consecutive stable counter
						}
					}
					// 2011-11-21 -WFC- ^

					//pLc-> prvRawWt	 = pLc-> rawWt;				// these two statements caused miss-report or detect motion in some situation.
					//timer_mSec_set( &(pLc-> motionDetectPeriodTimer), pLc-> motionDetectPeriodTime);
				}
			}
		} // end if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status)) {}
	} // end if ( SCALE_STD_MODE_MOTION_DETECT & gbScaleStandardModeNV )	{}
	else { // motion detection disabled
		pLc-> status &= ~LC_STATUS_MOTION;						// clear motion flag
	}
} // end loadcell_detect_motion()
 2012-02-06 -WFC- ^ */

/**
 * It formats loadcell data in a dispaly format output based on countby, scale mode etc..
 *
 * @param  pLc		-- pointer to loadcell structure; it is an input parameter.
 * @param  pOutStr  -- points to an allocated output string buffer.
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2007/08/03 by Wai Fai Chin
 */

BYTE loadcell_format_output( LOADCELL_T *pLc, char *pOutStr )
{
	BYTE	len;
	BYTE	n;
	BYTE    precision;
	float	fRound;
	//char *ptr_P; this is also work
	PGM_P	ptr_P;
	char	unitName[5];
	BYTE 	formatBuf[6];
	
	len = n = 0;
	if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
		if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status)) {	// if this loadcell has valid value
			// memcpy_P ( unitName, gcUnitNameTbl[ pLc-> viewCB.unit ], 5); bug because gcUnitNameTbl[] is a two level pointers in program memory space. Fix this bug by get the pointer to string first, then copy the string from program memory space.
			memcpy_P ( &ptr_P, &gcUnitNameTbl[ pLc-> viewCB.unit ], sizeof(PGM_P));
			strcpy_P(unitName,ptr_P);
			// n = (BYTE) sprintf_P( pOutStr, PSTR("%f %4s\r\n"), pLc-> rawWt, unitName );
			// TODO the weight output depends on the pLc->opModes such as net, gross, total etc...
			//n = (BYTE) sprintf_P( pOutStr, PSTR("%f %4s %x\r\n"), pLc-> rawWt, unitName, pLc-> status );
			fRound = float_round( pLc-> rawWt, pLc-> viewCB.fValue);
			// n = (BYTE) sprintf_P( pOutStr, PSTR("%f %f %x\r\n"), pLc-> rawWt, fRound, pLc-> status );
			if ( pLc-> viewCB.decPt > 0 )
				precision = pLc-> viewCB.decPt;
			else
				precision = 0;
			float_format( formatBuf, 8, precision);
			// len = (BYTE) sprintf_P( pOutStr, PSTR("%f "), pLc-> rawWt);
			len = (BYTE) sprintf( pOutStr + len, formatBuf, fRound );
			pOutStr[len]= ' ';	len++;
			
			if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 				// if loadcell is in NET mode
				fRound = pLc-> netWt;
			else
				fRound = pLc-> grossWt;
			
			n = (BYTE) sprintf( pOutStr + len, formatBuf, fRound );
			len += n;
			pOutStr[len]= ' ';	len++;
			pOutStr[len]= 'Z';	len++;
			pOutStr[len]= ' ';	len++;
			
			n = (BYTE) sprintf( pOutStr + len, formatBuf, *(pLc-> pZeroWt) );
			len += n;
			pOutStr[len]= ' ';	len++;
			pOutStr[len]= 'T';	len++;
			pOutStr[len]= ' ';	len++;
			
			n = (BYTE) sprintf( pOutStr + len, formatBuf, *(pLc-> pTareWt) );

			len += n;
			pOutStr[len]= ' ';	len++;
			pOutStr[len]= 'T';	len++;
			pOutStr[len]= 'T';	len++;
			pOutStr[len]= 'L';	len++;
			pOutStr[len]= ' ';	len++;
			
			n = (BYTE) sprintf( pOutStr + len, formatBuf, *(pLc-> pTotalWt) );

			len += n;
			pOutStr[len]= ' ';	len++;
			
			n = (BYTE) sprintf_P( pOutStr + len, PSTR(" %x %x\r\n"), pLc-> status, *(pLc-> pOpModes) );
			len += n;
		}
		else
			len = (BYTE) sprintf_P( pOutStr, gcStrFmtDash10LfCr_P);		// print ---------- line.
	}
	return len;	// number of char in the output string buffer.
} // end loadcell_format_output()

/**
 * It formats loadcell data in packet format:
 *  data; unit; numSeg; annunciator1; annunciator2; annunciator3; annunciator4; SysErrorCode
 *
 * @param  pLc		 -- pointer to loadcell structure; it is an input parameter.
 * @param  pOutStr	 -- points to an allocated output string buffer.
 * @param  valueType -- value type such as gross, net, tare, total, etc...
 * @param  lc		 -- loadcell number.		// 2012-02-23 -WFC-
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009-01-16 by Wai Fai Chin
 * 2010-11-10 -WFC- reset LC_STATUS_HAS_NEW_TOTAL flag if value type is total type.
 *  Note that future version may use registered listener to keep track new total status
 *  when we have more memory or powerful MCU.
 * 2011-03-28 -WFC- Added Peak hold value type.
 * 2012-02-23 -WFC- Added code to set High Resolution flag on packet output format.
 * 2012-02-23 -WFC- Added lc input parameter, passed lc to loadcell_format_weight().
 */

// 2012-02-23 -WFC- BYTE loadcell_format_packet_output( LOADCELL_T *pLc, char *pOutStr, BYTE valueType )
BYTE loadcell_format_packet_output( LOADCELL_T *pLc, char *pOutStr, BYTE valueType, BYTE lc )		// 2012-02-23 -WFC-
{
	BYTE	len;
	BYTE	extraFlag;				// 2012-02-23 -WFC-

	len = 0;
	if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
		// 2012-02-23 -WFC- len = loadcell_format_weight( pLc, pOutStr, valueType);
		len = loadcell_format_weight( pLc, pOutStr, valueType, lc );		// 2012-02-23 -WFC-
		if ( len ) {
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
			len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLc-> viewCB.unit );
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

	len += loadcell_format_annunciator( pLc, pOutStr + len );
	// 2012-02-23 -WFC- v
	// len += sensor_format_sys_annunciator( pOutStr + len );
	if ( LC_STATUS3_IN_HI_RESOLUTION & pLc-> status3 )
		extraFlag = SYS_STATUS_ANNC_HIRES_MODE;
	else
		extraFlag = 0;
	len += sensor_format_sys_annunciator( pOutStr + len, extraFlag );
	// 2012-02-23 -WFC- ^
	// 2010-11-10 -WFC- reset LC_STATUS_HAS_NEW_TOTAL flag. Note that future version may use registered listener to keep track new total status when we have more memory or powerful MCU.
	if ( SENSOR_VALUE_TYPE_TOTAL == (valueType & SENSOR_VALUE_TYPE_MASK_bm) ) {
		pLc-> status &= ~LC_STATUS_HAS_NEW_TOTAL;			// flagged no more new total weight because it just send out to a remote.
	}
	return len;	// number of char in the output string buffer.
} // end loadcell_format_packet_output()

/*
BYTE loadcell_format_packet_output( LOADCELL_T *pLc, char *pOutStr, BYTE valueType )
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	BYTE 	formatBuf[6];

	len = 0;
	if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
		if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) ||			// if this loadcell has valid value OR
			( SENSOR_VALUE_TYPE_NON_FILTERED_bm & valueType)) {		// non filter type
			if ( valueType & SENSOR_VALUE_TYPE_NON_FILTERED_bm ) {
				fRound = pLc-> grossWtUnFiltered;
			}
			else {
				switch ( (valueType & SENSOR_VALUE_TYPE_MASK_bm)) {
					case SENSOR_VALUE_TYPE_GROSS : fRound = pLc-> grossWt;		break;
					case SENSOR_VALUE_TYPE_NET	 : fRound = pLc-> netWt;		break;
					case SENSOR_VALUE_TYPE_TOTAL : fRound = *(pLc-> pTotalWt);	break;
					case SENSOR_VALUE_TYPE_TARE	 : fRound = *(pLc-> pTareWt);	break;
					case SENSOR_VALUE_TYPE_ZERO	 : fRound = *(pLc-> pZeroWt);	break;
					case SENSOR_VALUE_TYPE_PEAK	 : fRound = pLc-> peakHoldWt;	break;		// 2011-03-28 -WFC-
					case SENSOR_VALUE_TYPE_CUR_MODE :
							if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) // if loadcell is in NET mode
								fRound = pLc-> netWt;
							else
								fRound = pLc-> grossWt;
							break;
				}
			}

			if ( pLc-> viewCB.decPt > 0 )
				precision = pLc-> viewCB.decPt;
			else
				precision = 0;
			float_format( formatBuf, 8, precision);

			fRound = float_round( fRound, pLc-> viewCB.fValue);
			len += (BYTE) sprintf( pOutStr + len, formatBuf, fRound );
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
			len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLc-> viewCB.unit );
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

	len += loadcell_format_annunciator( pLc, pOutStr + len );
	len += sensor_format_sys_annunciator( pOutStr + len );
	// 2010-11-10 -WFC- reset LC_STATUS_HAS_NEW_TOTAL flag. Note that future version may use registered listener to keep track new total status when we have more memory or powerful MCU.
	if ( SENSOR_VALUE_TYPE_TOTAL == (valueType & SENSOR_VALUE_TYPE_MASK_bm) ) {
		pLc-> status &= ~LC_STATUS_HAS_NEW_TOTAL;			// flagged no more new total weight because it just send out to a remote.
	}
	return len;	// number of char in the output string buffer.
} // end loadcell_format_packet_output()
*/


/**
 * It formats loadcell data in Ascii string.
 *
 * @param  pLc		 -- pointer to loadcell structure; it is an input parameter.
 * @param  pOutStr	 -- points to an allocated output string buffer.
 * @param  valueType -- value type such as gross, net, tare, total, total count, adc count, etc...
 * @param  lc		 -- loadcell number.		// 2012-02-23 -WFC-
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2011-08-10 by Wai Fai Chin
 * 2012-02-23 -WFC- Added lc input parameter and codes to handle total count and ADC count value types.
 * 2012-02-29 -WFC- Make sure ADC count and total count are integer display with no decimal point.
 * 2012-06-28 -WFC- Fixed rounding error bug for ADC and total count value.
 */

// 2012-02-23 -WFC- BYTE loadcell_format_weight( LOADCELL_T *pLc, char *pOutStr, BYTE valueType )
BYTE loadcell_format_weight( LOADCELL_T *pLc, char *pOutStr, BYTE valueType, BYTE lc )
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	BYTE 	formatBuf[6];

	len = 0;
	if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
		if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) ||			// if this loadcell has valid value OR
			( SENSOR_VALUE_TYPE_NON_FILTERED_bm & valueType)) {		// non filter type
			if ( valueType & SENSOR_VALUE_TYPE_NON_FILTERED_bm ) {
				switch ( (valueType & SENSOR_VALUE_TYPE_MASK_bm)) {
					case SENSOR_VALUE_TYPE_GROSS :
						fRound = pLc-> grossWtUnFiltered;
						break;
					case SENSOR_VALUE_TYPE_ADC_COUNT: // 2015-03-10 -DLM-
						fRound = (float)(gaLSensorDescriptor[lc].curADCcount);
						break;
				}
			}
			else {
				switch ( (valueType & SENSOR_VALUE_TYPE_MASK_bm)) {
					case SENSOR_VALUE_TYPE_GROSS : fRound = pLc-> grossWt;		break;
					case SENSOR_VALUE_TYPE_NET	 : fRound = pLc-> netWt;		break;
					case SENSOR_VALUE_TYPE_TOTAL : fRound = *(pLc-> pTotalWt);	break;
					case SENSOR_VALUE_TYPE_TARE	 : fRound = *(pLc-> pTareWt);	break;
					case SENSOR_VALUE_TYPE_ZERO	 : fRound = *(pLc-> pZeroWt);	break;
					case SENSOR_VALUE_TYPE_PEAK	 : fRound = pLc-> peakHoldWt;	break;		// 2011-03-28 -WFC-
					case SENSOR_VALUE_TYPE_CUR_MODE :
							if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) // if loadcell is in NET mode
								fRound = pLc-> netWt;
							else
								fRound = pLc-> grossWt;
							break;
					// 2012-02-23 -WFC- v
					case SENSOR_VALUE_TYPE_ADC_COUNT:
							fRound = (float)(gaLSensorDescriptor[lc].curADCcount);
						break;
					case SENSOR_VALUE_TYPE_TOTAL_COUNT:
							fRound = (float)(*(pLc-> pNumTotal));
						break;
					// 2012-02-23 -WFC- ^
				}
			}
//2012-02-29 -WFC-	if ( SENSOR_VALUE_TYPE_ADC_COUNT != valueType || 		// countby does not apply to ADC counts.   2012-02-23 -WFC-
//2012-02-29 -WFC-				SENSOR_VALUE_TYPE_TOTAL_COUNT != valueType ) 	// countby does not apply to Total counts. 2012-02-23 -WFC-
			//2012-06-28 -WFC- v
//			if ( SENSOR_VALUE_TYPE_ADC_COUNT == valueType || 		// countby does not apply to ADC counts.   2012-02-29 -WFC-
//					SENSOR_VALUE_TYPE_TOTAL_COUNT == valueType ) 	// countby does not apply to Total counts. 2012-02-29 -WFC-
//				precision = 0;										// integer type, no decimal point, 2012-02-23 -WFC-
//			else {
//				if ( pLc-> viewCB.decPt > 0 )
//					precision = pLc-> viewCB.decPt;
//				else
//					precision = 0;
//			}
//			float_format( formatBuf, 8, precision);
//			fRound = float_round( fRound, pLc-> viewCB.fValue);
//
			if ( SENSOR_VALUE_TYPE_ADC_COUNT == valueType || 		// countby does not apply to ADC counts.
					SENSOR_VALUE_TYPE_TOTAL_COUNT == valueType ) {	// countby does not apply to Total counts.
				float_format( formatBuf, 8, precision);
				precision = 0;										// integer type, no decimal point.
				fRound = float_round( fRound, 1.0f);
			}
			else {
				if ( pLc-> viewCB.decPt > 0 )
					precision = pLc-> viewCB.decPt;
				else
					precision = 0;
				float_format( formatBuf, 8, precision);
				fRound = float_round( fRound, pLc-> viewCB.fValue);
			}
			//2012-06-28 -WFC- ^
			len += (BYTE) sprintf( pOutStr + len, formatBuf, fRound );
		}
	}
	return len;	// number of char in the output string buffer.
} // end loadcell_format_weight()


/**
 * It formats loadcell data in Ascii string.
 *  grossWt; netWt; tareWt; numLift|numTtlCnt; iCb; decPoint; unit; numSeg; annunciator1; annunciator2; annunciator3; annunciator4; SysErrorCode
 * Caller ensured lc is a loadcell type sensor.
 *
 * @param  pOutStr	-- points to an allocated output string buffer.
 * @param  lc		-- loadcell number.
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2011-08-10 by Wai Fai Chin
 * 2012-02-23 -WFC- Added code to set High Resolution flag on packet output format. passed lc to loadcell_format_weight(),
 */

BYTE loadcell_format_gnt_packet_output( char *pOutStr, BYTE lc )
{
	BYTE	len;
	BYTE    precision;
	BYTE	extraFlag;				// 2012-02-23 -WFC-
	LOADCELL_T *pLc;

	len = 0;
	if ( lc <= SENSOR_NUM_LAST_PV_LOADCELL ) {							// ensured sensor number is valid.
		pLc = &gaLoadcell[ lc ];
		if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
			// 2012-02-23 -WFC- len  = loadcell_format_weight( pLc, pOutStr, SENSOR_VALUE_TYPE_GROSS);
			len  = loadcell_format_weight( pLc, pOutStr, SENSOR_VALUE_TYPE_GROSS, lc);		// 2012-02-23 -WFC-
			if ( len ) {
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				// 2012-02-23 -WFC- len += loadcell_format_weight( pLc, pOutStr + len, SENSOR_VALUE_TYPE_NET);
				len += loadcell_format_weight( pLc, pOutStr + len, SENSOR_VALUE_TYPE_NET, lc); // 2012-02-23 -WFC-
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				// 2012-02-23 -WFC- len += loadcell_format_weight( pLc, pOutStr + len, SENSOR_VALUE_TYPE_TARE);
				len += loadcell_format_weight( pLc, pOutStr + len, SENSOR_VALUE_TYPE_TARE, lc);	// 2012-02-23 -WFC-
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				precision = (BYTE) gaulLiftCntFNV[lc];
				precision <<=4;
				precision |= (BYTE) (0x0F & *(pLc->pNumTotal));			// now MSB 4bits has liftCnt; LSB 4bits has number of total count.
				len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) precision );
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLc-> viewCB.iValue );
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLc-> viewCB.decPt );
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLc-> viewCB.unit );
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
			}
		}

		if ( 0 == len ) {
			len = (BYTE) sprintf_P( pOutStr, gcStrDefaultLoadcell_gnt_OutFormat );		// formated default as: 0;0;0;0;0;0;0;0; for grossWt; netWt; tareWt; numLift|numTtlCnt; iCb; decPoint; unit; numSeg;
		}
		else {
			if ( lc == gtSystemFeatureFNV.bargraphSensorID )	{				// if this sensor is selected for output bargraph, then format it for output.
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

		len += loadcell_format_annunciator( pLc, pOutStr + len );
		// 2012-02-23 -WFC- v
		// len += sensor_format_sys_annunciator( pOutStr + len );
		if ( LC_STATUS3_IN_HI_RESOLUTION & pLc-> status3 )
			extraFlag = SYS_STATUS_ANNC_HIRES_MODE;
		else
			extraFlag = 0;
		len += sensor_format_sys_annunciator( pOutStr + len, extraFlag );
		// 2012-02-23 -WFC- ^
	}
	return len;	// number of char in the output string buffer.
} // end loadcell_format_gnt_packet_output()


/**
 * It formats loadcell data in Ascii string.
 *  valueType; data; numLift|numTtlCnt; iCb; decPoint; unit; annunciator1; annunciator2; annunciator3; annunciator4;
 * Caller ensured lc is a loadcell type sensor.
 *
 * @param  pOutStr	-- points to an allocated output string buffer.
 * @param  lc		-- loadcell number.
 * @param  valueType-- value type.
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2012-02-13 by Wai Fai Chin
 */

BYTE loadcell_format_vcb_packet_output( char *pOutStr, BYTE lc, BYTE valueType )
{
	BYTE	len;
	BYTE    precision;
	BYTE	extraFlag;				// 2012-02-23 -WFC-
	LOADCELL_T *pLc;

	len = 0;
	if ( lc <= SENSOR_NUM_LAST_PV_LOADCELL ) {							// ensured sensor number is valid.
		pLc = &gaLoadcell[ lc ];
		if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
			len = sprintf_P( pOutStr, gcStrFmt_pct_d_c, (int) valueType, CMD_ITEM_DELIMITER );
			len += loadcell_format_weight( pLc, pOutStr + len, valueType, lc);
			if ( len ) {
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				precision = (BYTE) gaulLiftCntFNV[lc];
				precision <<=4;
				precision |= (BYTE) (0x0F & *(pLc->pNumTotal));			// now MSB 4bits has liftCnt; LSB 4bits has number of total count.
				len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) precision );
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLc-> viewCB.iValue );
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLc-> viewCB.decPt );
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
				len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLc-> viewCB.unit );
				pOutStr[len]= CMD_ITEM_DELIMITER;
				len++;
			}
		}

		if ( 0 == len ) {
			len = (BYTE) sprintf_P( pOutStr, gcStrDefaultLoadcell_vcb_OutFormat );		// formated default as: 0;0;0;0;0;0;0;0; for grossWt; netWt; tareWt; numLift|numTtlCnt; iCb; decPoint; unit; numSeg;
		}

		len += loadcell_format_annunciator( pLc, pOutStr + len );
		if ( LC_STATUS3_IN_HI_RESOLUTION & pLc-> status3 )
			extraFlag = SYS_STATUS_ANNC_HIRES_MODE;
		else
			extraFlag = 0;
		len += sensor_format_sys_annunciator( pOutStr + len, extraFlag );
	}
	return len;	// number of char in the output string buffer.
} // end loadcell_format_vcb_packet_output()


/**
 * It formats loadcell data in Ascii string without leading or trailing spaces.
 *
 * @param  pOutStr	 -- points to an allocated output string buffer.
 * @param  lc		 -- loadcell number.
 * @param  valueType -- value type such as gross, net, tare, total, etc...
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2012-02-13 by Wai Fai Chin
 * 2012-02-29 -WFC- Make sure ADC count and total count are integer display with no decimal point.
 */

BYTE loadcell_format_weight_no_space( char *pOutStr, BYTE lc, BYTE valueType )
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	BYTE 	formatBuf[6];
	LOADCELL_T *pLc;


	len = 0;
	if ( lc <= SENSOR_NUM_LAST_PV_LOADCELL ) {							// ensured sensor number is valid.
		pLc = &gaLoadcell[ lc ];
		if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
			if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) ||			// if this loadcell has valid value OR
				( SENSOR_VALUE_TYPE_NON_FILTERED_bm & valueType)) {		// non filter type
				if ( valueType & SENSOR_VALUE_TYPE_NON_FILTERED_bm ) {
					fRound = pLc-> grossWtUnFiltered;
				}
				else {
					switch ( (valueType & SENSOR_VALUE_TYPE_MASK_bm)) {
						case SENSOR_VALUE_TYPE_GROSS : fRound = pLc-> grossWt;		break;
						case SENSOR_VALUE_TYPE_NET	 : fRound = pLc-> netWt;		break;
						case SENSOR_VALUE_TYPE_TOTAL : fRound = *(pLc-> pTotalWt);	break;
						case SENSOR_VALUE_TYPE_TARE	 : fRound = *(pLc-> pTareWt);	break;
						case SENSOR_VALUE_TYPE_ZERO	 : fRound = *(pLc-> pZeroWt);	break;
						case SENSOR_VALUE_TYPE_PEAK	 : fRound = pLc-> peakHoldWt;	break;		// 2011-03-28 -WFC-
						case SENSOR_VALUE_TYPE_CUR_MODE :
								if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) // if loadcell is in NET mode
									fRound = pLc-> netWt;
								else
									fRound = pLc-> grossWt;
								break;
						case SENSOR_VALUE_TYPE_ADC_COUNT:
								fRound = (float)(gaLSensorDescriptor[lc].curADCcount);
							break;
						case SENSOR_VALUE_TYPE_TOTAL_COUNT:
								fRound = (float)(*(pLc-> pNumTotal));
							break;
					}
				}

//2012-02-29 -WFC-	if ( SENSOR_VALUE_TYPE_ADC_COUNT != valueType || 		// countby does not apply to ADC counts.   2012-02-23 -WFC-
//2012-02-29 -WFC-				SENSOR_VALUE_TYPE_TOTAL_COUNT != valueType ) 	// countby does not apply to Total counts. 2012-02-23 -WFC-
				if ( SENSOR_VALUE_TYPE_ADC_COUNT == valueType || 		// countby does not apply to ADC counts.   2012-02-29 -WFC-
						SENSOR_VALUE_TYPE_TOTAL_COUNT == valueType ) 	// countby does not apply to Total counts. 2012-02-29 -WFC-
					precision = 0;										// integer type, no decimal point, 2012-02-23 -WFC-
				else {
					if ( pLc-> viewCB.decPt > 0 )
						precision = pLc-> viewCB.decPt;
					else
						precision = 0;
				}

				formatBuf[0]='%';
				formatBuf[1]='.';
				formatBuf[2]='0' + precision;
				formatBuf[3]= 'f';
				formatBuf[4]= 0;

				fRound = float_round( fRound, pLc-> viewCB.fValue);
				len += (BYTE) sprintf( pOutStr + len, formatBuf, fRound );
			}
		}
	}
	return len;	// number of char in the output string buffer.
} // end loadcell_format_weight_no_space(,,)


/**
 * It formats annunciator of this loadcell.
 *  
 *
 * @param  pLc		 -- pointer to loadcell structure; it is an input parameter.
 * @param  pOutStr	 -- points to an allocated output string buffer.
 *
 * @post   format annuciators are stored in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/01/20 by Wai Fai Chin
 */
BYTE loadcell_format_annunciator( LOADCELL_T *pLc, char *pOutStr )
{
	BYTE len;
	BYTE	anc3;
	
	len  = sprintf_P( pOutStr, gcStrFmt_pct_d, (int) pLc-> runModes );
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pLc-> status );
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;

	anc3 = pLc-> status2;
	if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 				// if loadcell is in NET mode
		anc3 |= LC_ANC3_NET_MODE;

	len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) anc3 );
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	return len;
}

/**
 * It initialized loadcell when the application software runs the very 1st time for the device.
 * It initializes gaLoadcell[] data structure of lc based on gabMotionDetectPeriodTimeNV[],
 * gabLcOpModesFNV[], gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  lc	-- loadcell number
 *
 * @post   updated gaLoadcell[] data structure of this loadcell.
 *
 * History:  Created on 2007/11/27 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 *
 */

void loadcell_1st_init( BYTE lc )
{
	LOADCELL_T *pLc;

	if ( lc < MAX_NUM_PV_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		gafLcZeroOffsetWtNV[ lc ] = 0.0;

		pLc-> viewCapacity	= gafSensorShowCapacityFNV[lc];
		pLc-> viewCB.fValue	= gafSensorShowCBFNV[lc];
		pLc-> viewCB.iValue	= gawSensorShowCBFNV[lc];
		pLc-> viewCB.decPt	= gabSensorShowCBdecPtFNV[lc];
		// 2010-08-30 	-WFC- pLc-> viewCB.unit	= gabSensorShowCBunitsFNV[lc];
		// 2010-08-30 	-WFC-	pLc-> oldUnit		= pLc-> viewCB.unit;
		pLc-> oldUnit		= pLc-> viewCB.unit	= gabSensorViewUnitsFNV[lc] = gabSensorShowCBunitsFNV[lc];	// 2010-08-30 -WFC-
		pLc-> type			= gabSensorTypeFNV[ lc ];
	}

} // end loadcell_1st_init()


/**
 * It initializes gaLoadcell[] data structure of lc based on gabMotionDetectPeriodTimeNV[],
 * gabLcOpModesFNV[], gaSensorCalNV[].
 *
 * @param  lc	-- loadcell number
 *
 * @post   updated gaLoadcell[] data structure of this loadcell.
 *
 * History:  Created on 2007/08/03 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 * 2011-03-28 -WFC- clear peak hold weight.
 * 2011-08-15 -WFC- removed call to lc_zero_setup_zero_powerup() because loadcell_init() is called during normal operation. This could be a potential bug for zero on power up.
 * 2011-11-17 -WFC- clear stableCnt for motion detection.
 */

void loadcell_init( BYTE lc )
{
	LOADCELL_T *pLc;
	BYTE n;

	if ( lc < MAX_NUM_PV_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		pLc-> prvRawWt = pLc-> rawWt = pLc-> peakHoldWt = 0.0;
		pLc-> status   = 									// clear status
		pLc-> status2  = 									// clear status2
		pLc-> status3  = 									// clear status3
		pLc-> runModes = 									// disabled this loadcell.
		//pLc-> stableCnt=									// stable event counter counts number of consecutive stable event. 2011-11-17 -WFC-
		pLc-> prvMotionWt = 								// clear previous motion weight
		pLc-> serviceCnt_status = 0;				// Not OK to count service counters, this prevent bogus counting during power up or right after init while there is loaded weight.
		// 2010-09-27 -WFC- pLc-> serviceCnt_status = LC_CNT_STATUS_OK_CNT_LIFT | LC_CNT_STATUS_OK_CNT_25PCT_CAP | LC_CNT_STATUS_OK_CNT_OVERLOAD;

		if ( lc < MAX_NUM_LOADCELL ) {		// only physical loadcell has cal table.
			n = sensor_get_cal_table_array_index( lc );
			pLc-> pCal = &gaSensorCalNV[n];					// points to base of a cal table.
		}

		if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ lc ] ||
			 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[ lc ] ) {
			// 2010-08-30 -WFC- 	pLc-> oldUnit = pLc-> viewCB.unit = gabSensorShowCBunitsFNV[lc];	// this will prevent unit changed recompute weights and threshold.
			pLc-> viewCB.unit = gabSensorViewUnitsFNV[lc];		// 2010-08-30 -WFC-
			loadcell_update_param( lc );
			// lc_zero_init( lc );		loadcell_update_param() calls loadcell_recompute_weights_unit(), in turn, it calls lc_zero_init().
			// 2011-08-15 -WFC- lc_zero_setup_zero_powerup( lc );					// setup zero on power up if enabled.
		}
	}

} // end loadcell_init()


/**
 * It updates parameter of gaLoadcell[] data structure of lc based on changed value of gabMotionDetectPeriodTimeNV[],
 * gabLcOpModesFNV[], gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 * It calls loadcell_recompute_weights_unit() which in turn calls lc_zero_init().
 *
 * @param  lc	-- loadcell number
 *
 * @post   updated gaLoadcell[] data structure of this loadcell.
 *
 * History:  Created on 2007/08/03 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 * 2010-08-30 -WFC- replaced gabVSMathUnitFNV[] with gabSensorShowCBunitsFNV[].
 * 2010-08-30 -WFC- Added programmable lift weight threshold for lift counter.
 *  expand service counters from 16bits to 32bits. Removed 25perCapCnt related
 *  variables and logics.
 * 2010-11-10 -WFC- converted capacity according to metric system convention.
 * 2012-10-29 -WFC- converted capacity based on LC_OP_MODE_TRUE_CAP_UNIT_CNV flag of pOpModes.
 */

void  loadcell_update_param( BYTE lc )
{
	BYTE 		refUnit;
	LOADCELL_T *pLc;
    float		fV, fTmp;
	
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ lc ] ||
			 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[ lc ] ) {
			pLc = &gaLoadcell[ lc ];
			pLc-> pNumTotal		= &gawNumTotalFNV [lc];
			pLc-> pOpModes		= &gabLcOpModesFNV[lc];
			pLc-> pTareWt		= &gafTareWtFNV[lc];
			pLc-> pTotalWt		= &gafTotalWtFNV[lc];
			pLc-> pSumSqTotalWt	= &gafSumSqTotalWtFNV[lc];
			pLc-> pMaxTotalWt	= &gafMaxTotalWtFNV[lc];
			pLc-> pMinTotalWt	= &gafMinTotalWtFNV[lc];
			pLc-> pZeroWt		= &gafZeroWtFNV[lc];
			
			pLc-> oldUnit		= pLc-> viewCB.unit;
			pLc-> type			= gabSensorTypeFNV[ lc ];			// 2010-06-18 -WFC-
			// 2010-08-30 -WFC- pLc-> viewCB.unit	= gabSensorShowCBunitsFNV[lc];		// unit was updated through panel key or host command.
			pLc-> viewCB.unit	= gabSensorViewUnitsFNV[lc];		// unit was updated through panel key or host command. // 2010-08-30 -WFC-

			loadcell_recompute_weights_unit( lc );					// note that it changed pLc->viewCB and pLc->viewCapacity if ViewUnit different than oldUnit.

			pLc-> pendingTime	= gab_Stable_Pending_TimeNV[lc];
			if (gabSensorFeaturesFNV[ lc ] & SENSOR_FEATURE_SENSOR_ENABLE)
				pLc-> runModes	   |= SENSOR_FEATURE_SENSOR_ENABLE;
			else
				pLc-> runModes	   &= ~SENSOR_FEATURE_SENSOR_ENABLE;

			if ( 0 == gabMotionDetectBand_dNV[lc])				// if motion detection band is 0 d, then the motion threshold is half countby.
				pLc-> motionThresholdWeight = pLc-> viewCB.fValue * 0.5;
			else		
				pLc-> motionThresholdWeight = pLc-> viewCB.fValue * (float)gabMotionDetectBand_dNV[lc];

			pLc-> motionDetectPeriodTime = gabMotionDetectPeriodTimeNV[ lc];

			if ( lc < MAX_NUM_PV_LOADCELL )	{ 					// if it is a valid physicall or virtual loadcell.
				fV = gafSensorShowCapacityFNV[lc];
				refUnit = gabSensorShowCBunitsFNV[lc];
			}

			if ( 0 == gabDropThresholdPctCapFNV[lc] )
				fTmp = 0.005;											// 0 == 0.5%
			else
				fTmp = (float) gabDropThresholdPctCapFNV[lc] * 0.01;	// 1 == 1%
			pLc-> dropWtThreshold = fV * fTmp;							// drop weight threshold % of capacity.

			if ( 0 == gabLiftThresholdPctCapFNV[lc] )
				fTmp = 0.005;											// 0 == 0.5%
			else
				fTmp = (float) gabLiftThresholdPctCapFNV[lc] * 0.01;	// 1 == 1%
			pLc-> liftWtThreshold = fV * fTmp;							// lift weight threshold % of capacity.

			if ( pLc-> viewCB.unit != refUnit ) {				// if display unit not the same as cal unit, then
				// convert them from reference unit to viewing unit weight
				// 2012-10-29 -WFC- v
				// 2010-11-10 -WFC- memcpy_P ( &fV,  &gafLoadcellUnitsTbl[ refUnit ][pLc-> viewCB.unit], sizeof(float));
				// memcpy_P ( &fV,  &gafLoadcellCapacityUnitsTbl[ refUnit ][pLc-> viewCB.unit], sizeof(float));	// 2010-11-10 -WFC- converted capacity according to metric system convention.
				if ( LC_OP_MODE_TRUE_CAP_UNIT_CNV & *(pLc-> pOpModes) )
					memcpy_P ( &fV,  &gafLoadcellUnitsTbl[ refUnit ][pLc-> viewCB.unit], sizeof(float));			// converted capacity according true unit lookup table.
				else
					memcpy_P ( &fV,  &gafLoadcellCapacityUnitsTbl[ refUnit ][pLc-> viewCB.unit], sizeof(float));	// converted capacity according to metric system convention.
				// 2012-10-29 -WFC- ^
				pLc-> dropWtThreshold 	= pLc-> dropWtThreshold * fV;
				pLc-> liftWtThreshold	= pLc-> liftWtThreshold * fV;
			}

			loadcell_update_overload_threshold( pLc );
			// timer_mSec_set( &(pLc-> motionDetectPeriodTimer), pLc-> motionDetectPeriodTime);
			lc_total_update_param( lc );
			lc_zero_init( lc );
			setpoint_unit_conversion_all();		// 2010-10-07 -WFC-
		}
		else {
			pLc-> runModes = 0;		// disabled loadcell.
		}
	}

} // end loadcell_update_param()


/**
 * It recomputes gross, net, zero and tare weights when unit had changed.
 *
 * @param  lc	-- loadcell number
 *
 * @post   updated net, tare weight and change net/gross mode.
 *
 * @note It is safe to use with host command fucntions.
 *       Assumed countby and view capacity had already in current unit.
 *
 * History:  Created on 2007/11/28 by Wai Fai Chin
 * 2010-07-22 -WFC- removed loadcell_change_countby() because it did twice which gave a wrong viewCapacity which in turn screwed up the countby, zero and tare limit.
 * 2010-08-30 -WFC- replaced gabVSMathUnitFNV[] with gabSensorShowCBunitsFNV[].
 * 2010-11-10 -WFC- converted capacity according to metric system convention.
 * 2011-03-28 -WFC- converted peak hold weight value.
 * 2012-10-29 -WFC- converted capacity based on LC_OP_MODE_TRUE_CAP_UNIT_CNV flag of pOpModes.
 *
 */

void loadcell_recompute_weights_unit( BYTE lc )
{
	BYTE	calUnit;
	LOADCELL_T 	*pLc;		// points to a loadcell
    float		unitCnv;
	
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[lc] ||
			  SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[lc] )	{
			pLc = &gaLoadcell[ lc ];
			loadcell_change_countby_capacity( pLc, lc, pLc-> viewCB.unit );
			if ( pLc-> oldUnit != pLc-> viewCB.unit ) {		//
				memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pLc-> oldUnit ][pLc-> viewCB.unit], sizeof(float));
				pLc-> grossWt			= pLc-> grossWt		* unitCnv;
				pLc-> netWt				= pLc-> netWt		* unitCnv;
				pLc-> rawWt				= pLc-> rawWt		* unitCnv;
				pLc-> prvRawWt			= pLc-> prvRawWt	* unitCnv;
				pLc-> prvMotionWt		= pLc-> prvMotionWt	* unitCnv;
				pLc-> prvZeroWt			= pLc-> prvZeroWt	* unitCnv;
				*(pLc-> pZeroWt)		= *(pLc-> pZeroWt)	* unitCnv;
				*(pLc-> pTareWt)		= *(pLc-> pTareWt)	* unitCnv;
				*(pLc-> pTotalWt)		= *(pLc-> pTotalWt)		* unitCnv;
				*(pLc-> pSumSqTotalWt)	= *(pLc-> pSumSqTotalWt)	* unitCnv;
				*(pLc-> pMaxTotalWt)	= *(pLc-> pMaxTotalWt)		* unitCnv;
				*(pLc-> pMinTotalWt)	= *(pLc-> pMinTotalWt)		* unitCnv;
				pLc-> grossWtUnFiltered = pLc-> grossWtUnFiltered	* unitCnv;
				pLc-> peakHoldWt		= pLc-> peakHoldWt	* unitCnv;				// 2011-03-28 -WFC-

				// done by cmd_current_unit_post_update(), pLc-> viewCapacity	= pLc-> viewCapacity * unitCnv;
				// done by loadcell_update_overload_threshold()
				// pLc-> overloadThresholdWt	= pLc-> overloadThresholdWt	* unitCnv;

				/* done by loadcell_update_param()
				pLc-> dropWtThreshold = pLc-> dropWtThreshold * unitCnv;
				pLc-> liftWtThreshold = pLc-> liftWtThreshold * unitCnv;
				pLc-> weightOf25pctCapacity = pLc-> weightOf25pctCapacity * unitCnv;
				*/

				pLc-> grossWt	= float_round( pLc-> grossWt,	pLc-> viewCB.fValue);
				pLc-> netWt		= float_round( pLc-> netWt,	pLc-> viewCB.fValue);
				*(pLc-> pTareWt)	= float_round( *(pLc-> pTareWt), pLc-> viewCB.fValue);
				// one shot only, no recomputing until unit changed again.
				pLc-> oldUnit = pLc-> viewCB.unit;
			}

			if ( SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[lc] )
				// 2010-08-30 -WFC-	calUnit = gabVSMathUnitFNV[0];
				calUnit = gabSensorShowCBunitsFNV[0];			// 2010-08-30 -WFC- math channel use ShowCBunitsFNV[] as reference unit.
			else
				calUnit = pLc-> pCal-> countby.unit;

			if ( pLc-> viewCB.unit == calUnit ) {		// if current unit is the same as the calibration unit,
				pLc-> totalT.onAcceptUpperWt = gafTotalOnAcceptUpperWtNV[ lc ];
				pLc-> totalT.onAcceptLowerWt = gafTotalOnAcceptLowerWtNV[ lc ];
			}
			else {
				// convert configured values from cal unit to current unit value.
				// 2012-10-29 -WFC- v
				// 2010-11-10 -WFC-  memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ calUnit ][  pLc-> viewCB.unit ], sizeof(float));
				// memcpy_P ( &unitCnv,  &gafLoadcellCapacityUnitsTbl[ calUnit ][  pLc-> viewCB.unit ], sizeof(float));	// 2010-11-10 -WFC- converted capacity according to metric system convention.
				if ( LC_OP_MODE_TRUE_CAP_UNIT_CNV & *(pLc-> pOpModes) )
					memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ calUnit ][  pLc-> viewCB.unit ], sizeof(float));
				else
					memcpy_P ( &unitCnv,  &gafLoadcellCapacityUnitsTbl[ calUnit ][  pLc-> viewCB.unit ], sizeof(float));	// converted capacity according to metric system convention.
				// 2012-10-29 -WFC- ^
				pLc-> totalT.onAcceptUpperWt = gafTotalOnAcceptUpperWtNV[ lc ] * unitCnv;
				pLc-> totalT.onAcceptLowerWt = gafTotalOnAcceptLowerWtNV[ lc ] * unitCnv;
			}

			// lc_zero_init( lc ); moved it back to loadcell_update_param()
		}
	}
} // end loadcell_recompute_weights_unit()


/**
 * update overload threshold weight based on this loadcell's zero weight and scale standard mode.
 *
 * Allow overload limit increased by 7 countby for OIML else 9 countby.
 * If zero weight > 4% of capactiy then reduced the overload limit by (zeroWt - 4%cap).
 *
 * @param  pLc		-- pointer to loadcell structure; it is an input parameter.
 *
 * @post   updated overloadThresholdWt.
 *
 * History:  Created on 2007/11/15 by Wai Fai Chin
 *  2010/06/18	-WFC- Modified to handle math type loadcell.
 */

void loadcell_update_overload_threshold( LOADCELL_T *pLc )
{
	float	fV, zeroWt, reducedCap;
	
	if ( SENSOR_TYPE_LOADCELL == pLc-> type )
		loadcell_update_overload_threshold_real_loadcell( pLc );
	else if (  SENSOR_TYPE_MATH_LOADCELL == pLc-> type )
		loadcell_update_overload_threshold_virtual_loadcell( pLc );
	
} // end loadcell_update_overload_threshold()


/**
 * update overload threshold weight based on this loadcell's zero weight and scale standard mode.
 * Physical loadcell overload threshold use calibrated capacity to compute overload threshold.
 *
 * Allow overload limit increased by 8 countby regardless of standard modes.
 * If zero weight > 5% of capactiy then reduced the overload limit by (zeroWt - 5%cap).
 *
 * @param  pLc		-- pointer to loadcell structure; it is an input parameter.
 *
 * @post   updated overloadThresholdWt.
 *
 * History:  Created on 2007-11-15 by Wai Fai Chin
 * 2010-08-30 -WFC- removed unit conversion for zeroWt because loadcell_recompute_weights_unit() converted it.
 *   Converted calibrate capacity to viewUnit before compute overload threshold.
 * 2010-11-10 -WFC- converted capacity according to metric system convention.
 * 2011-08-15 -WFC- Due to new legal for trade requirement, overload threshold is changed.
 * 2012-10-29 -WFC- converted capacity based on LC_OP_MODE_TRUE_CAP_UNIT_CNV flag of pOpModes.
 * 2013-05-01 -WFC- Adjusted overload threshold unit converted cal.countby.
 * 2015-05-07 -WFC- only allow adjusted overload threshold during initial zero at power up.
 */

void loadcell_update_overload_threshold_real_loadcell( LOADCELL_T *pLc )
{
	float	fV, zeroWt, reducedCap;
	float	unitCnv,	fCapacity;
	float   f8countByCap;  //2013-05-01 -WFC-

	fCapacity = pLc-> pCal-> capacity;
	f8countByCap = pLc-> pCal-> countby.fValue * 8.0;
	// Note that overloadTHresholdwt is in cal unit.
	if ( pLc-> viewCB.unit != pLc-> pCal-> countby.unit ) {
		// 2012-10-29 -WFC- v
		// 2010-11-10 -WFC- memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pLc-> pCal-> countby.unit ][pLc-> viewCB.unit], sizeof(float));
		// memcpy_P ( &unitCnv,  &gafLoadcellCapacityUnitsTbl[ pLc-> pCal-> countby.unit ][pLc-> viewCB.unit], sizeof(float));	// 2010-11-10 -WFC- converted capacity according to metric system convention.
		if ( LC_OP_MODE_TRUE_CAP_UNIT_CNV & *(pLc-> pOpModes) )
			memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pLc-> pCal-> countby.unit ][pLc-> viewCB.unit], sizeof(float));
		else
			memcpy_P ( &unitCnv,  &gafLoadcellCapacityUnitsTbl[ pLc-> pCal-> countby.unit ][pLc-> viewCB.unit], sizeof(float));	// converted capacity according to metric system convention.
		// 2012-10-29 -WFC- ^
		fCapacity = fCapacity * unitCnv;
		f8countByCap = f8countByCap *  unitCnv;
	}

	fV = fCapacity * 0.05;
	zeroWt = *(pLc-> pZeroWt);

	if ( zeroWt > fV )							// if zeroWt > 5% Capacity
		reducedCap = zeroWt - fV;
	else
		reducedCap = 0.0;

	if ( !(LC_RUN_MODE_ZERO_ON_POWERUP & (pLc-> runModes )) )					// 2015-05-07 -WFC- only allow adjusted overload threshold during initial zero at power up.
		// 2013-05-01 -WFC- fV = pLc-> pCal-> countby.fValue * 8.0;						// allow overload limit increased by 8 countby.
		pLc-> overloadThresholdWt = fCapacity + f8countByCap - reducedCap;	// overloadThresholdWt = capacity + allowCounby - reducedCap;
} // end loadcell_update_overload_threshold_real_loadcell()

/*
/* 2011-08-15 -WFC- Removed because the following rules are obsolete:
 * Allow overload limit increased by 7 countby for OIML else 9 countby.
 * If zero weight > 4% of capactiy then reduced the overload limit by (zeroWt - 4%cap).
void loadcell_update_overload_threshold_real_loadcell( LOADCELL_T *pLc )
{
	float	fV, zeroWt, reducedCap;
	float	unitCnv,	fCapacity;

	fCapacity = pLc-> pCal-> capacity;
	// Note that overloadTHresholdwt is in cal unit.
	if ( pLc-> viewCB.unit != pLc-> pCal-> countby.unit ) {
		// 2010-11-10 -WFC- memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pLc-> pCal-> countby.unit ][pLc-> viewCB.unit], sizeof(float));
		memcpy_P ( &unitCnv,  &gafLoadcellCapacityUnitsTbl[ pLc-> pCal-> countby.unit ][pLc-> viewCB.unit], sizeof(float));	// 2010-11-10 -WFC- converted capacity according to metric system convention.
		fCapacity = fCapacity * unitCnv;
	}

	fV = fCapacity * 0.04;
	zeroWt = *(pLc-> pZeroWt);

	if ( zeroWt > fV )							// if zeroWt > 4% Capacity
		reducedCap = zeroWt - fV;
	else
		reducedCap = 0.0;

	if ( SCALE_STD_MODE_OIML == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK))
		fV = 7.0;								// allow overload limit increased by 7 countby.
	else
		fV = 9.0;								// allow overload limit increased by 9 countby.

	fV = pLc-> pCal-> countby.fValue * fV;						// allow overload limit increased by fV countby.
	pLc-> overloadThresholdWt = fCapacity + fV - reducedCap;	// overloadThresholdWt = capacity + allowCounby - reducedCap;
} // end loadcell_update_overload_threshold_real_loadcell()
*/

/**
 * update overload threshold weight based on this virtual math type loadcell's
 * zero weight and scale standard mode.
 *
 * Allow overload limit increased by 8 countby regardless of standard modes.
 * If zero weight > 5% of capactiy then reduced the overload limit by (zeroWt - 5%cap).
 *
 * @param  pLc		-- pointer to loadcell structure; it is an input parameter.
 *
 * @post   updated overloadThresholdWt.
 *
 * History:  Created on 2007-11-15 by Wai Fai Chin
 * 2010-08-30 -WFC- removed unit conversion for zeroWt because loadcell_recompute_weights_unit() converted it.
 * 2011-08-15 -WFC- Due to new legal for trade requirement, overload threshold is changed.
 */

void loadcell_update_overload_threshold_virtual_loadcell( LOADCELL_T *pLc )
{
	BYTE	calUnit;
	float	fV, zeroWt, reducedCap;

	fV = pLc-> viewCapacity * 0.05;
	zeroWt = *(pLc-> pZeroWt);

	if ( zeroWt > fV )							// if zeroWt > 5% Capacity
		reducedCap = zeroWt - fV;
	else
		reducedCap = 0.0;

	fV = pLc->viewCB.fValue * 8.0;				// allow overload limit increased by 8 countby.
	pLc-> overloadThresholdWt = pLc-> viewCapacity + fV - reducedCap;		// overloadThresholdWt = capacity + allowCounby - reducedCap;
} // end loadcell_update_overload_threshold_virtual_loadcell()

/* 2011-08-15 -WFC- Removed because the following rules are obsolete:
 * Allow overload limit increased by 7 countby for OIML else 9 countby.
 * If zero weight > 4% of capactiy then reduced the overload limit by (zeroWt - 4%cap).
void loadcell_update_overload_threshold_virtual_loadcell( LOADCELL_T *pLc )
{
	BYTE	calUnit;
	float	fV, zeroWt, reducedCap;

	fV = pLc-> viewCapacity * 0.04;
	zeroWt = *(pLc-> pZeroWt);

	if ( zeroWt > fV )					// if zeroWt > 4% Capacity
		reducedCap = zeroWt - fV;
	else
		reducedCap = 0.0;

	if ( SCALE_STD_MODE_OIML == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK))
		fV = 7.0;								// allow overload limit increased by 7 countby.
	else
		fV = 9.0;								// allow overload limit increased by 9 countby.

	fV = pLc->viewCB.fValue * fV;				// allow overload limit increased by fV countby.
	pLc-> overloadThresholdWt = pLc-> viewCapacity + fV - reducedCap;		// overloadThresholdWt = capacity + allowCounby - reducedCap;
} // end loadcell_update_overload_threshold_virtual_loadcell()
*/

/**
 * It keeps track net and tare weight and may change to gross mode if tare weight == zero.
 * This procedure is call whenever there is changed in either gross or tare weight.
 *
 * @param  pLc	-- pointer to loadcell structure; it is an input parameter.
 * @param  lc	-- loadcell number
 *
 * @post   updated net, tare weight and change net/gross mode.
 *
 * History:  Created on 2007/11/28 by Wai Fai Chin
 */

void loadcell_tracks_net_gross_tare( LOADCELL_T *pLc, BYTE lc )
{
	pLc-> netWt   = pLc-> grossWt - *(pLc-> pTareWt);
	if ( float_eq_zero( *(pLc-> pTareWt)))			{	// if tareWt == 0.0, then change to gross mode.
		lc_tare_change_to_gross( pLc );
		// nv_cnfg_fram_save_loadcell_dynamic_data( lc );
	}
	
	if ( (LC_OP_MODE_TARE_AUTO_CLR_MASK & *(pLc-> pOpModes)) == LC_TARE_AUTO_CLR_YES ) {	// if tare auto clear.
		if ( float_eq_zero( *(pLc-> pTareWt)))			// if tareWt == 0.0, 
			if ( float_lte_zero( pLc-> netWt))				// if netWt <= 0.0, 
				lc_tare_set( pLc, 0.0f, lc );
	}
} // end loadcell_tracks_net_gross_tare(,)


/**
 * It performs ADC to weight, gross, net, zero, AZM, tare, total and setpoint checking.
 *
 * @param  lc	-- loadcell number
 *
 * @post	It may updated net, zero, tare weight, change net/gross mode and other loadcell status.
 *
 * @note This is a helper function to be call by sersor module's sensor_compute_all_values().
 *
 * History:  Created on 2007/12/19 by Wai Fai Chin
 * 2011-05-09 -WFC- Only check motion if it had a new valid weight. It check total, zero, and tare if it either has a new or old valid weight.
 * 2012-02-03 -WFC- Only check motion, if it had a new valid weight and valid ADC count.
 *           Only check azm, coz if it had a new valid weight. This ensured this operation in sync with loadcell filter sample window.
 * 2012-02-06 -WFC- added lc parameter to loadcell_detect_motion() and lc_zero_coz()
 */

void loadcell_tasks( BYTE lc )
{
	BYTE gotValid_ADCcnt;									// 2012-02-03 -WFC- got a valid ADC count, if filter is enabled, this will be in sync with filter
	LOADCELL_T 	*pLc;											// points to a loadcell
	pLc = &gaLoadcell[ lc ];
	if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
		loadcell_compute_weight( lc );
		if (  SENSOR_STATUS_GOT_ADC_CNT & gaLSensorDescriptor[ lc ].status )
			gotValid_ADCcnt = TRUE;
		else
			gotValid_ADCcnt = FALSE;
		// 2011-05-09 -WFC- if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status)) {	// if this loadcell has a new valid value
		//if ( ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status)) ||
		//	 ( LC_STATUS3_GOT_PREV_VALID_VALUE	& (pLc-> status3)) ) {	// if this loadcell has a new OR old valid value // 2011-05-09 -WFC-
		if ( ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status)) ) {
			// 2012-02-03 -WFC- if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status))		// 2011-05-09 -WFC- Only check motion if it had a new valid weight.
			if ( gotValid_ADCcnt && (LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status)))	{	// 2012-02-03 -WFC- Only check motion if it had a new valid weight and a new valid ADC count.
				// 2012-02-06 -WFC- loadcell_detect_motion( pLc );
				loadcell_detect_motion( pLc, lc );		// 2012-02-06 -WFC-
				// 2012-02-03 -WFC- lc_zero_azm( pLc, gafLcZeroOffsetWtNV[ lc ], lc );		// 2012-02-03 -WFC-
				// 2012-02-03 -WFC- lc_zero_coz( pLc );										// 2012-02-03 -WFC-
			}
			// 2012-02-09 -WFC- pLc-> status	|= LC_STATUS_GOT_VALID_WEIGHT;				// 2011-05-09 -WFC- since it had old valid weight, make it as new valid weight for the system to work.
			lc_total_evaluate( pLc, lc );
			if ( LC_RUN_MODE_PENDING_TOTAL & pLc-> runModes)  {				// if it has total pending,
				if ( timer_mSec_expired( &(pLc-> totalT.pendingTimer) ) )		// if pending total time expired,
					pLc-> runModes &= ~LC_RUN_MODE_PENDING_TOTAL;				// flagged NO MORE pending total.
				else 
					lc_total_handle_command( lc );
			}
			lc_zero_azm( pLc, gafLcZeroOffsetWtNV[ lc ], lc );
			// 2012-02-08 -WFC-  lc_zero_coz( pLc );
			lc_zero_coz( pLc, lc );							// 2012-02-08 -WFC-
		}// end if loadcell has a new value.

		if ( LC_STATUS3_GOT_PREV_VALID_VALUE	& (pLc-> status3))			// 2012-02-09 -WFC-
				pLc-> status	|= LC_STATUS_GOT_VALID_WEIGHT;				// 2012-02-09 -WFC- since it had old valid weight, make it as new valid weight for the system to work.
		lc_zero_check_pending( pLc, lc );			// 2012-02-03 -WFC-
		lc_tare_check_pending( pLc, lc );			// 2012-02-03 -WFC-
	}
} // end loadcell_tasks()


/**
 * It changes loadcell unit.
 *
 * @param  lc	-- loadcell number
 * @param  unit	-- new unit
 *
 * @post   updated gaLoadcell[] data structure of this loadcell.
 *
 * History:  Created on 2009/08/07 by Wai Fai Chin
 * 2010-08-30 -WFC- replaced gabSensorShowCBunitsFNV[] with gabSensorViewUnitsFNV[].
 *
 */

void loadcell_change_unit( BYTE lc, BYTE unit )
{
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		// 2010-08-30	-WFC-	gabSensorShowCBunitsFNV[lc] = unit;
		gabSensorViewUnitsFNV[lc] = unit;						// 2010-08-30	-WFC-
		loadcell_update_param( lc );		// loadcell_update_param() calls loadcell_recompute_weights_unit(), in turn, it calls lc_zero_init(). 
		nv_cnfg_fram_save_a_sensor_feature( lc );
		nv_cnfg_fram_save_totaling_statistics( lc );
		nv_cnfg_fram_save_loadcell_dynamic_data( lc );
	}
} // end loadcell_change_unit()


/**
 * It changes dynamic viewCB countby and view capacity variables of a specified
 * loadcell based the changed of unit.
 *
 * @param  pLc		-- pointer to loadcell structure; 
 * @param  	lc		-- loadcell index.
 * @param  newUnit	-- new unit
 *
 * @return none.
 *
 * @post   updated viewCB and viewCapacity of a loadcell.
 * obsoleted 2010-08-30 -WFC- post   updated gafSensorShowCBFNV[], gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gafSensorShowCapacityFNV[].
 *
 * @note It uses gabSensorShowCBunitsFNV as a reference unit to convert gaXSensorShowCBFNV[] and
 * gafSensorShowCapacityFNV[] to pLc-> viewCB and pLc-> viewCapacity in a new unit.
 *
 * History:  Created on 2009-08-13 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 * 2010-08-30 -WFC- replaced (pCal-> countby.unit) with gabSensorShowCBunitsFNV[].
 * 2010-11-10 -WFC- converted capacity according to metric system convention.
 * 2011-07-08 -WFC- if this loadcell is in high resolution mode, it makes the viewCB to next step lower.
 * 2011-09-09 -WFC- if change countby from English to metric of same level unit and if countby is the same, then get the next lower countby. This prevents it from has the same resolution for both units.
 * 2012-10-29 -WFC- converted capacity based on LC_OP_MODE_TRUE_CAP_UNIT_CNV flag of pOpModes.
 */

void loadcell_change_countby_capacity( LOADCELL_T *pLc, BYTE lc, BYTE newUnit )
{
	BYTE	showingUnit;		// 2011-09-09 -WFC-
	BYTE	needToCheckCB;		// 2011-09-09 -WFC-
	MSI_CB	cb;
	float	unitCnv;

	if ( lc < MAX_NUM_PV_LOADCELL ) {
		// always use gafSensorShowNNNNN as reference parameters
		cb.fValue = gafSensorShowCBFNV[lc];
		cb.iValue = gawSensorShowCBFNV[lc];
		cb.decPt  = gabSensorShowCBdecPtFNV[lc];
		cb.unit	  = showingUnit = gabSensorShowCBunitsFNV[lc];
		pLc-> viewCapacity	= gafSensorShowCapacityFNV[lc];

		if ( newUnit != gabSensorShowCBunitsFNV[lc] ) {
			// 2012-10-29 -WFC- v
			// 2010-11-10 -WFC- memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ gabSensorShowCBunitsFNV[lc]][newUnit],  sizeof(float));
			//memcpy_P ( &unitCnv,  &gafLoadcellCapacityUnitsTbl[ gabSensorShowCBunitsFNV[lc]][newUnit],  sizeof(float));	// 2010-11-10 -WFC- converted capacity according to metric system convention.
			//pLc-> viewCapacity	= gafSensorShowCapacityFNV[lc] * unitCnv;
			//pLc-> viewCB.unit	= cb.unit = newUnit;
			//cb.fValue = gafSensorShowCBFNV[lc] * unitCnv;
			if ( LC_OP_MODE_TRUE_CAP_UNIT_CNV & *(pLc-> pOpModes) )
				memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ gabSensorShowCBunitsFNV[lc]][newUnit],  sizeof(float));
			else
				memcpy_P ( &unitCnv,  &gafLoadcellCapacityUnitsTbl[ gabSensorShowCBunitsFNV[lc]][newUnit],  sizeof(float));	// converted capacity according to metric system convention.
			pLc-> viewCapacity	= gafSensorShowCapacityFNV[lc] * unitCnv;

			memcpy_P ( &unitCnv,  &gafLoadcellCapacityUnitsTbl[ gabSensorShowCBunitsFNV[lc]][newUnit],  sizeof(float));	// converted capacity according to metric system convention.
			pLc-> viewCB.unit	= cb.unit = newUnit;
			cb.fValue = gafSensorShowCBFNV[lc] * unitCnv;
			// 2012-10-29 -WFC- ^
			cal_normalize_verify_input_countby( &cb);
			// 2011-09-09 -WFC- v
			needToCheckCB = FALSE;
			if ( SENSOR_UNIT_LB == showingUnit ) {
				if ( SENSOR_UNIT_KG == newUnit )
					needToCheckCB = TRUE;
			}
			else if ( SENSOR_UNIT_TON == showingUnit ) {
				if ( SENSOR_UNIT_MTON == newUnit || SENSOR_UNIT_KNWT == newUnit )
					needToCheckCB = TRUE;
			}
			else if ( SENSOR_UNIT_OZ == showingUnit ) {
				if ( SENSOR_UNIT_GRAM == newUnit )
					needToCheckCB = TRUE;
			}

			if ( needToCheckCB ) {
				// if new cb == showingCB, then get the next lower countby.
				if ( cb.decPt == gabSensorShowCBdecPtFNV[lc] && cb.iValue == gawSensorShowCBFNV[lc] )
					cal_next_lower_countby( &cb );
			}
			// 2011-09-09 -WFC- ^
		}

		// 2011-07-08 -WFC- v
		if ( LC_STATUS3_IN_HI_RESOLUTION & pLc-> status3  ) {
			cal_next_lower_countby( &cb );
		}
		// 2011-07-08 -WFC- ^
		pLc-> viewCB.fValue	= cb.fValue;
		pLc-> viewCB.iValue	= cb.iValue;
		pLc-> viewCB.decPt	= cb.decPt;
		pLc-> viewCB.unit	= cb.unit;
	}
} // end loadcell_change_countby_capacity()


/**
 * It selected the input channel of ADC chip that connect to a loadcell.
 * It is based on product id and version number.
 *
 * @param  sensorID	  -- sensor ID or channel number.
 *
 * @return ADC channel of ADC chip that connects to loadcell.
 * @post	it may override gabSensorFeaturesFNV[] based on product and version ID.
 *          gabSensorTypeFNV[] may also be override.
 *
 * History:  Created on 2008/12/16 by Wai Fai Chin
 */

BYTE loadcell_config_hardware( BYTE sensorID  )
{
	BYTE channel;
	switch ( sensorID )	{
		case	0:	channel = CHANNEL_NUM_OF_LOADCELL_0;	break;
		case	1:	channel = CHANNEL_NUM_OF_LOADCELL_1;	break;
		case	2:	channel = CHANNEL_NUM_OF_LOADCELL_2;	break;
		case	3:	channel = CHANNEL_NUM_OF_LOADCELL_3;	break;
	}
	return channel;
} // end loadcell_config_hardware()


/**
 * It fetches the value of specified type of a loadcell.
 *
 * @param	sn		-- sensore ID;
 * @param	valueType	-- value type such as current mode, gross, net, adc count etc...
 * @param	unit	-- wanted unit for the request value.
 * @param	pfV		-- point to floating variable that save the sensor value.
 * @return	state of this loadcell such as valid value, uncal, in cal, overload etc...
 *
 * History:  Created on 2010-03-19 by Wai Fai Chin
 * 2010-10-15 -WFC- updated pLc-> status2 ADC busy status.
 * 2011-06-23 -WFC- Added support for value type of Total, and number of Total.
 * 2011-07-06 -WFC- removed statement to clear LC_STATUS2_GOT_NEW_PEAK_VALUE. It is done in panelmain module for product that has panelmain module otherwise it is done in dataoutputs module.
 * 2012-09-21 -WFC- Added support for value type of Tare.
 * 2012-10-18 -WFC- Added support for value type peakhold.
 */

BYTE	loadcell_get_value_of_type( BYTE sn, BYTE valueType, BYTE unit, float *pfV )
{
	BYTE		state;
	float		unitCnv;
	LOADCELL_T *pLc;

	*pfV = 0.0;
	state = SENSOR_STATE_ADC_BUSY;									// assumed sensor is busy.
	if ( sn <= SENSOR_NUM_LAST_PV_LOADCELL ) {							// ensured sensor number is valid.
		if ( ( SENSOR_TYPE_LOADCELL ==  gabSensorTypeFNV[ sn ] ) || ( SENSOR_TYPE_MATH_LOADCELL ==  gabSensorTypeFNV[ sn ] ) )	{
			pLc = gaLSensorDescriptor[ sn ].pDev;
			if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
				if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) ||
					 LC_STATUS3_GOT_UNFILTER_VALUE & (pLc->status3)) {	// if this loadcell has valid value

					if ( SENSOR_VALUE_TYPE_NON_FILTERED_bm & valueType ) {
						*pfV = pLc-> grossWtUnFiltered;
						state = SENSOR_STATE_GOT_VALID_VALUE;
					}
					else {
						switch ( ( ~SENSOR_VALUE_TYPE_NON_FILTERED_bm & valueType ) ){
						case SENSOR_VALUE_TYPE_CUR_MODE:
							// 2012-10-18 -WFC- v
							if ( LC_RUN_MODE_PEAK_HOLD_ENABLED & gaLoadcell[ sn ].runModes ) {
								*pfV = pLc->peakHoldWt;
							}
							else {
							// 2012-10-18 -WFC- ^
								if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) // if loadcell is in NET mode
									*pfV = pLc-> netWt;
								else
									*pfV = pLc-> grossWt;
							}
							break;
						case SENSOR_VALUE_TYPE_GROSS:
								*pfV = pLc-> grossWt;
							break;
						case SENSOR_VALUE_TYPE_NET:
								*pfV = pLc-> netWt;
							break;
						case SENSOR_VALUE_TYPE_TOTAL:								// 2011-06-23 -WFC-
								*pfV = *(pLc-> pTotalWt);
							break;
						case SENSOR_VALUE_TYPE_TARE:								// 2012-09-21 -WFC-
								*pfV = *(pLc-> pTareWt);
							break;
						case SENSOR_VALUE_TYPE_ADC_COUNT:
								*pfV = (float)(gaLSensorDescriptor[sn].curADCcount);
							break;
						case SENSOR_VALUE_TYPE_TOTAL_COUNT:								// 2011-06-23 -WFC-
								*pfV = (float)(*(pLc-> pNumTotal));
							break;
							// 2012-10-18 -WFC- v
						case SENSOR_VALUE_TYPE_PEAK	 :
								*pfV = pLc->peakHoldWt;
							break;
							// 2012-10-18 -WFC- ^
						} // end switch()
					}

					if ( SENSOR_VALUE_TYPE_ADC_COUNT != valueType || 		// unit does not apply to ADC counts.
							SENSOR_VALUE_TYPE_TOTAL_COUNT != valueType ) {	// unit does not apply to Total counts. 2011-06-23 -WFC-
						if ( unit != SENSOR_VALUE_UNIT_CUR_MODE ) {
							// if unit is different then perform unit conversion
							if ( unit!= pLc-> viewCB.unit) {
								memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pLc-> viewCB.unit ][unit],  sizeof(float));
								*pfV = *pfV * unitCnv;
							}
						}
					}

					// if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) )	// this IF statement caused SENSOR_STATE_ADC_BUSY if filtering was not settled.
					state = SENSOR_STATE_GOT_VALID_VALUE;

					if ( !(SENSOR_VALUE_TYPE_NON_FILTERED_bm & valueType )) {	// only filtered type value perform over, under range and load check.
						if ( LC_STATUS2_OVER_RANGE & (pLc-> status2)) {
							state = SENSOR_STATE_OVER_RANGE;
						}
						else if ( LC_STATUS_OVERLOAD & (pLc-> status)) {
							state = SENSOR_STATE_OVERLOAD;
						}
						else if ( LC_STATUS2_UNER_RANGE & (pLc-> status2)) {
							state = SENSOR_STATE_UNDER_RANGE;
						}
						else if ( LC_STATUS_UNDERLOAD & (pLc-> status)) {
							state = SENSOR_STATE_UNDERLOAD;
						}
					}
				}
				else { // it has no valid weight
					if ( CAL_STATUS_COMPLETED != pLc-> pCal[0].status )	{
						state = SENSOR_STATE_UNCAL;
					}
					else if ( LC_STATUS_GOT_CAL_WEIGHT & (pLc-> status)) {
						state = SENSOR_STATE_GOT_VALUE_IN_CAL;
						*pfV = pLc-> rawWt;
					}
					else
						state = SENSOR_STATE_ADC_BUSY;

					if ( (pLc-> runModes) & LC_RUN_MODE_IN_CAL ) {
						state |= SENSOR_STATE_IN_CAL_bm;
					}
				}
			}
			else
				state = SENSOR_STATE_DISABLED;
		}
	}

	// 2010-10-15 -WFC- v
	if ( sn <= SENSOR_NUM_LAST_LOADCELL ) {							// ensured sensor number is valid.
		if ( SENSOR_TYPE_LOADCELL ==  gabSensorTypeFNV[ sn ] )	{
			pLc = gaLSensorDescriptor[ sn ].pDev;
			if ( (~SENSOR_STATE_IN_CAL_bm & state) == SENSOR_STATE_ADC_BUSY )
				pLc-> status2 |= LC_STATUS2_ADC_BUSY;
			else
				pLc-> status2 &= ~LC_STATUS2_ADC_BUSY;
			//2011-07-06 -WFC- pLc-> status2 &= ~LC_STATUS2_GOT_NEW_PEAK_VALUE; this should be done in panelmain or data output module.
		}
	}
	// 2010-10-15 -WFC- ^
	return state;
} // end loadcell_get_value_of_type()


/**
 * It formats a specified loadcell Rcal value in ASCII string and store it in the supplied string buffer..
 *
 * @param  lc		--	loadcell ID.
 * @param  *pStr	--	points to a string buffer where to save the Rcal value in ASCII.
 *
 * @return true if it has a Rcal value.
 * @note: This only for ScaleCore2.
 *
 * History:  Created on 2010-04-21	by Wai Fai Chin
 * 2011-04-28 -WFC- use constant cal at value 10% of capacity as Rcal value in ADC count instead of weight value.
 */

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-05-19 -WFC-
BYTE loadcell_format_rcal_string( BYTE lc, char *pStr )
{
	// gStrAcmdTmp	== value in ASCII string;

	BYTE					status;
	SENSOR_CAL_T			*pSensorCal;
	LSENSOR_DESCRIPTOR_T	*pRcalSnDesc;	// points to Rcal input sensor descriptor
	MSI_CB					cb;

	status = FALSE;
	fill_string_buffer( pStr, '0', 6);
	if ( lc < MAX_NUM_LOADCELL )	{
		if ( sensor_get_cal_base( lc, &pSensorCal ) ) {			// if lcID is a valid sensorID.
			if ( ((pSensorCal-> status > 0) && (pSensorCal-> status <= MAX_CAL_POINTS)) ||	// if it has at least 2 valid cal points OR
					(CAL_STATUS_COMPLETED == pSensorCal-> status)) { 						// have a completed cal table.
				pRcalSnDesc = &gaLSensorDescriptor[ SENSOR_NUM_RCAL ];
				pRcalSnDesc-> value = adc_to_value( pRcalSnDesc-> curADCcount, &(pSensorCal-> adcCnt[0]), &(pSensorCal-> value[0]));
				cb = pSensorCal-> countby;
				cal_next_lower_countby( &cb );
				float_round_to_string( pRcalSnDesc-> value, &cb, 8, pStr );
				status = TRUE;
			}
		}
	}
	return status;
} // end loadcell_format_rcal_string()

#else
BYTE loadcell_format_rcal_string( BYTE lc, char *pStr )
{

	BYTE			status;
	SENSOR_CAL_T	*pSensorCal;
	INT32			cal10pctCap_ADCcnt;

	status = FALSE;
	fill_string_buffer( pStr, '0', 6);
	if ( lc < MAX_NUM_LOADCELL )	{
		if ( sensor_get_cal_base( lc, &pSensorCal ) ) {			// if lcID is a valid sensorID.
			if ( ((pSensorCal-> status > 0) && (pSensorCal-> status <= MAX_CAL_POINTS)) ||	// if it has at least 2 valid cal points OR
				(CAL_STATUS_COMPLETED == pSensorCal-> status)) { 						// have a completed cal table.
				cal10pctCap_ADCcnt = get_constant_cal( &(pSensorCal-> adcCnt[0]), &(pSensorCal-> value[0]), pSensorCal-> capacity );
				sprintf_P( pStr, gcStrFmt_pct_ld, cal10pctCap_ADCcnt );
				status = TRUE;
			}
		}
	}
	return status;
} // end loadcell_format_rcal_string()

/**
 * It computes constant cal in weight equivalent value.
 *
 * @param  lc		--	loadcell ID.
 *
 * @return constant cal in weight equivalent.
 *
 * History:  Created on 2011-05-19	by Wai Fai Chin
 */

float loadcell_get_ccal_weight( BYTE lc )
{
	float fV;
	SENSOR_CAL_T	*pSensorCal;

	fV = 0.0f;
	if ( lc < MAX_NUM_LOADCELL )	{
		if ( sensor_get_cal_base( lc, &pSensorCal ) ) {			// if lcID is a valid sensorID.
			if ( ((pSensorCal-> status > 0) && (pSensorCal-> status <= MAX_CAL_POINTS)) ||	// if it has at least 2 valid cal points OR
				(CAL_STATUS_COMPLETED == pSensorCal-> status)) { 							// have a completed cal table.
				fV = pSensorCal-> capacity * 0.1f;
			}
		}
	}
	return fV;
} // end loadcell_get_ccal_weight()

#endif


/**
 * It initializes zero on power up for all local physical loadcell sensors.
 *
 * @post   if a loadcell is enabled zero on powerup, it will set zero pending request flag with 10 seconds time out.
 *
 * History:  Created on 2011-08-15 by Wai Fai Chin
 */

void loadcell_init_all_loadcells_zero_powerup( void )
{
	BYTE lc;

	for ( lc = 0; lc < MAX_NUM_LOADCELL; lc++ ) {
		lc_zero_setup_zero_powerup( lc );					// setup zero on power up if enabled.
	}
} // end loadcell_init_all_loadcells_zero_powerup()
