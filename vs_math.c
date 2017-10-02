/*! \file vs_math.c \brief virtual sensors math channel type related functions.*/
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
//  History:  Created on 2010/02/16 by Wai Fai Chin
//
// Description:
//   It is a high level module manages all aspect math type virtual sensors.
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
// ****************************************************************************
//TODO: tomorrow save math channel related stuff to Fram. debug math channel output.

#include	"vs_math.h"
#include	<stdio.h>
#include	"commonlib.h"
#include	"cmdparser.h"		// for CMD_ITEM_DELIMITER
#include	"dataoutputs.h"
#include	"nvmem.h"
#include	"sensor.h"
#include	"loadcell.h"
#include	"lc_tare.h"
#include	"lc_total.h"
#include	"lc_zero.h"


#if ( CONFIG_COMPILER == CONFIG_USE_PC_GCC )
#include	"emulate_mcu_functions.h"
#endif

/// sensor ID, range from 9 to 15 which assigned to this math channel
BYTE	gabVSMathSensorIdFNV[ MAX_NUM_VS_MATH ];

/// user defined overload threshold for math channel
//float	gafVSMathOverloadThresholdFNV[ MAX_NUM_VS_MATH ];

/// user defined overload threshold unit for math channel. It is a reference unit and should not change again. Unit change should only be done with gabSensorShowCBunitsFNV[].
//BYTE	gabVSMathOverloadThresholdUnitFNV[ MAX_NUM_VS_MATH ];

// unit of math channel. It is a reference unit for math type loadcell. It is like the cal unit and should not change again. Unit change should only be done with gabSensorShowCBunitsFNV[].
// 2010-08-30	-WFC- BYTE	gabVSMathUnitFNV[ MAX_NUM_VS_MATH ];


/// Math type Virtual Sensor math expression string.
BYTE	gabVSMathRawExprsFNV[ MAX_NUM_VS_MATH ][ MAX_VS_RAW_EXPRS_SIZE + 1 ];

/// Array of Math type Virtual Sensor.
VIRTUAL_SENSOR_MATH_T	gaVsMath[ MAX_NUM_VS_MATH ];

/// Array of Math channel status.
BYTE	gaVsMathStatus[ MAX_NUM_VS_MATH ];

/// default math expression.
const char PROGMEM gcMathExprsDefault[]   = "0+1";

/// private methods
BYTE	vs_math_get_sensor_value( BYTE sn, BYTE filterType, BYTE unit, float *pfV );
void	vs_math_compute_weight( BYTE n );
BYTE	vs_math_evaluate( BYTE mc, BYTE filterType, float *pfResult );
void	vs_math_update_host_sensor_status( LOADCELL_T 	*pLc, BYTE mc);
void	vs_math_update_status( BYTE mc, BYTE sn, BYTE state);

/**
 * It initializes gaVsMath[] data structure of Virtual Sensor math channel.
 *
 * @param  sn	-- virtual sensor number or ID.
 *
 * @post   updated gaVsMath[] data structure of this math type Virtual Sensor.
 *
 * History:  Created on 2010/02/17 by Wai Fai Chin
 */

void vs_math_init( BYTE sn )
{
	vs_math_update_param( sn );
} // end vs_math_init()

/**
 * It initializes gaVsMath[] data structure of Virtual Sensor math channel.
 *
 * @param  mc	-- math channel number.
 * @param  sn	-- virtual sensor number or ID.
 *
 * @post   updated gaVsMath[] data structure of this math type Virtual Sensor.
 *
 * History:  Created on 2010/03/10 by Wai Fai Chin
 */

void vs_math_init_mc( BYTE mc, BYTE sn )
{
	if ( mc < MAX_NUM_VS_MATH ) {
		if ( vs_math_compile_expression( mc, &gabVSMathRawExprsFNV[ mc ][0] ))
			gaVsMathStatus[mc] = 0;
		else
			gaVsMathStatus[mc] |= VS_MATH_STATUS_WRONG_EXPRESSION;
	}

} // end vs_math_init_mc()


/**
 * It initializes gaVsMath[] data structure of Virtual Sensor math channel.
 *
 * @param  sn	-- virtual sensor number or ID.
 *
 * @post   updated gaVsMath[] data structure of this math type Virtual Sensor.
 *
 * History:  Created on 2010/05/10 by Wai Fai Chin
 */

void vs_math_update_param( BYTE sn )
{
	// BYTE mc;
	BYTE type;

	if ( SENSOR_NUM_MATH_LOADCELL == sn  ) {			// ensured sn is a math type loadcell sensor.
		type = gabSensorTypeFNV[ sn ];
		if ( SENSOR_TYPE_MATH_LOADCELL == type ) {
			// mc = vs_math_get_math_channel_id( sn );
			vs_math_init_mc( 0, sn );					// hardcode for ScaleCore because it only has 1 math channel.
		}
	}

} // end vs_math_update_param()


/**
 * Compile and validate math expression and compiled it into byte codes.
 *  math expression byte code. bit7 = 1 means that bit6 to bit 0
 *	are math operation or function. bit7 = 0 means that bit6 to bit 0 are sensor number.
 *	0x80 means end.
 *
 * @param	mc		-- math channel ID,
 * @param	pStr	-- points to math expression string.
 *
 * @post   updated gaVsMath[] data structure of this math type Virtual Sensor.
 * @return PASSED if valid, else FAILED.
 * @note	This version only handle + expression for summing channel.
 * 			It will be extend to handle more complex math expression in future version.
 *
 * History:  Created on 2010/02/17 by Wai Fai Chin
 */

#define VSM_LOOKFOR_OP				0
#define VSM_LOOKFOR_SENSOR_NUM		1
#define VSM_LOOKFOR_CONSTANT		2		// not implement for this version.

BYTE vs_math_compile_expression( BYTE mc, BYTE *pStr )
{
	BYTE	i;
	BYTE	m;
	BYTE	lookFor;
	BYTE	status;
	GENERIC_UNION u;

	status = FAILED;
	if ( mc < MAX_NUM_VS_MATH ) {
		status = PASSED;
		m = 0;
		lookFor = VSM_LOOKFOR_SENSOR_NUM;
		for ( i=0; i < MAX_VS_RAW_EXPRS_SIZE; i++) {
			if ( *pStr == 0 )	{
				if (  VSM_LOOKFOR_SENSOR_NUM == lookFor ){
					status = FAILED;
					gaVsMath[mc].mathExprs[0] = VS_MATH_EXPRS_END;
				}
				else {
					if ( m < MAX_VS_RAW_EXPRS_SIZE )	{
						gaVsMath[ mc ].mathExprs[ m ] = VS_MATH_EXPRS_END;
					}
				}
				break;
			}
			switch ( lookFor ){
				case	VSM_LOOKFOR_SENSOR_NUM:
					status = FAILED;
					if ( *pStr <= '9' && *pStr >= '0') {
						if ( sscanf_P( pStr, gcStrFmt_pct_d, &u.i16.l )!= EOF ) {		// if it has a valid value.
							if ( u.bv < MAX_NUM_SENSORS )	{							// ensured sensor number is valid
								if ( m < MAX_VS_RAW_EXPRS_SIZE )	{
									if ( u.bv < MAX_NUM_LOADCELL ) {			// only allow real physical loadcell for now.
										gaVsMath[ mc ].mathExprs[ m ] = u.bv;
										m++;
										lookFor = VSM_LOOKFOR_OP;
										status = PASSED;
									}
								}
							}
						}
					}
					break;
				case	VSM_LOOKFOR_OP:
					status = FAILED;
					if ( '+' == *pStr ) {		// if it has a valid value.
						if ( m < MAX_VS_RAW_EXPRS_SIZE )	{
							gaVsMath[ mc ].mathExprs[ m ] = VS_MATH_EXPRS_OP | VS_MATH_EXPRS_ADD;
							m++;
							lookFor = VSM_LOOKFOR_SENSOR_NUM;
							status = PASSED;
						}
					}
					break;
			} // end switch
			if ( FAILED == status ) {
				gaVsMath[mc].mathExprs[0] = VS_MATH_EXPRS_END;
				break;
			}
			pStr++;
		}// end for
	} // end if ( mc < MAX_NUM_VS_MATH ) {}
	return status;
} // end vs_math_compile_expression()


/**
 * It computes math type loadcell weight based math expression.
 * It tracks net, gross, tare and also check overload and update service counter.
 * If math channel's grossWt > overloadThresholdWt, it is overloaded.
 * Note that if one of physical loadcell is overloaded,
 * then the math type loadcell is also overloaded; it is performed by vs_math_update_host_sensor_status();
 * Under load, under range, and over range checks are not perform at this high level.
 * They are performed by its input loadcells. That is, if one of the input loadcell is under load,
 * then the math sensor is also declared under load.
 *
 * @param  n  -- Sensor channel number.
 *
 * @post updated contents in gaLSensorDescriptor[ n ] and gLoadcell[n].
 *
 * @return none.
 *
 * History:  Created on 2010/06/17 by Wai Fai Chin
 * 2010-08-30 -WFC- Added programmable lift weight threshold for lift counter.
 *  expand service counters from 16bits to 32bits. Removed 25perCapCnt related
 *  variables and logics.
 * 2014-10-20 -WFC- added user lift counter logic.
 */

void  vs_math_compute_weight( BYTE n )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	LOADCELL_T 				*pLc;			// points to a loadcell
    float					unitCnv;
	BYTE					cti;			// calibrate table index
	BYTE					sensorStatus;
	BYTE					mc;
	BYTE					state;

	mc = 0;
	if (  SENSOR_NUM_MATH_LOADCELL == n )	{
		pSnDesc = &gaLSensorDescriptor[ n ];
		pLc = (LOADCELL_T *) pSnDesc-> pDev;
		sensorStatus = pSnDesc-> status & ~SENSOR_STATUS_GOT_VALUE;		// Assume it has no valid value.

		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {					// if this sensor is enabled.
			pLc-> status &= ~(LC_STATUS_GOT_VALID_WEIGHT | LC_STATUS_GOT_CAL_WEIGHT);	// assumed it has no valid weight
			pLc-> status3 &= ~LC_STATUS3_GOT_UNFILTER_VALUE;

			vs_math_evaluate( mc, SENSOR_VALUE_TYPE_NON_FILTERED_bm, &pLc-> grossWtUnFiltered );
			// vs_math_evaluate() already performs unit conversion.
//			if ( gabVSMathUnitFNV[mc] != pLc-> viewCB.unit ) {
//				memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ gabVSMathUnitFNV[mc] ][pLc-> viewCB.unit], sizeof(float));
//				pLc-> grossWtUnFiltered  = pLc-> grossWtUnFiltered * unitCnv;
//			}
			pLc-> grossWtUnFiltered  = float_round( pLc-> grossWtUnFiltered, pLc-> viewCB.fValue);
			pLc-> status3 |= LC_STATUS3_GOT_UNFILTER_VALUE;

			if ( vs_math_evaluate( mc, SENSOR_VALUE_TYPE_CUR_MODE, &pLc-> rawWt ) ) {
				pLc-> grossWt = float_round( pLc-> rawWt, pLc-> viewCB.fValue);
				loadcell_tracks_net_gross_tare(	pLc, n );					// it also update pLc-> netWt.

				// update service counters
				if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 			// if loadcell is in NET mode
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
				else if ( float_a_lte_b( unitCnv, pLc-> dropWtThreshold ) ) 	// if the load has unloaded,
					pLc-> serviceCnt_status |= LC_CNT_STATUS_OK_CNT_LIFT;	// flag OK to count lift event


				// if math channel's grossWt > overloadThresholdWt, it is overloaded. Note that if one of physical loadcell is overloaded, then the math type loadcell is also overloaded. It is perform by vs_math_update_host_sensor_status();
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

				if ( float_a_lte_b( pLc-> rawWt, pLc-> dropWtThreshold ) ) 		// if the load has unloaded,
					pLc-> serviceCnt_status |= LC_CNT_STATUS_OK_CNT_OVERLOAD;	// flag OK to count overload event and 25% cap weight event.

				if ( cti )	// if need to save service counter to FRAM.
					nv_cnfg_fram_save_service_counters( n );

				//Under load, under range, and over range checks are not perform at this high level.
				//They are performed by its input loadcells. That is, if one of the input loadcell is under load,
//				unitCnv = pLc-> viewCapacity * ((float) gabPcentCapUnderloadFNV[n]) * 0.01f;		// underload threshold.
//				if ( float_a_lt_b( pLc-> grossWt, unitCnv ) ) {							// if current weight < overload threshold
//					pLc-> status	|= LC_STATUS_UNDERLOAD;
//				}
//				else {
//					pLc-> status	&= ~LC_STATUS_UNDERLOAD;							// clear overload flag.
//				}

				pLc-> status	|= LC_STATUS_GOT_VALID_WEIGHT;			// flag this loadcell has a valid weight
				sensorStatus	|= SENSOR_STATUS_GOT_VALUE;				// flag this sensor has a valid value.
				pLc-> runModes &= ~LC_RUN_MODE_IN_CAL;					// flag this loadcell is not in calibration mode.
				pLc-> runModes	|= LC_RUN_MODE_NORMAL_ACTIVE;			// flag it is in normal active mode that it has a cal table and not in cal mode.
			} // end if  ( vs_math_evaluate( mc, SENSOR_VALUE_TYPE_NON_FILTERED_bm, &pLc-> rawWt ) ){}
			// else {
			vs_math_update_host_sensor_status( pLc,  mc);
			// }
		} // end if ( SENSOR_STATUS_ENABLED...) if this sensor is enabled.
		pSnDesc-> status |= sensorStatus;					// update sensor status.
	} // end if (  SENSOR_NUM_MATH_LOADCELL == n )	{}
} // end vs_math_compute_weight()

/**
 * It process math type virtual sensor based on its math expression.
 *
 * @param  sn	-- sensor number for virtual sensor must within range 9 to 15.
 *
 * History:  Created on 2010/02/17 by Wai Fai Chin
 * 2012-02-06 -WFC- added lc parameter to loadcell_detect_motion() and lc_zero_coz()
 */

void vs_math_tasks( BYTE lc )
{
	LOADCELL_T 	*pLc;	// points to a loadcell

	if (  SENSOR_NUM_MATH_LOADCELL == lc )	{					// ensure it is a math type loadcell.
		pLc = &gaLoadcell[ lc ];
		if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
			vs_math_compute_weight( lc );
			if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status)) {	// if this loadcell has a new valid value
				// 2012-02-06 -WFC- loadcell_detect_motion( pLc );
				loadcell_detect_motion( pLc, lc );		// 2012-02-06 -WFC-
				lc_total_evaluate( pLc, lc );
				if ( LC_RUN_MODE_PENDING_TOTAL & pLc-> runModes)  {				// if it has total pending,
					if ( timer_mSec_expired( &(pLc-> totalT.pendingTimer) ) )		// if pending total time expired,
						pLc-> runModes &= ~LC_RUN_MODE_PENDING_TOTAL;				// flagged NO MORE pending total.
					else
						lc_total_handle_command( lc );
				}
				// lc_zero_azm( pLc, gafLcZeroOffsetWtNV[ lc ], lc );  Math type loadcell has no azm because only physical loadcell can perform AZM and zeroing.
				// 2012-02-06 -WFC-  lc_zero_coz( pLc );
				lc_zero_coz( pLc, lc );		// 2012-02-06 -WFC-
			}// end if loadcell has a new value.
			// lc_zero_check_pending( pLc, lc );		Math type loadcell has no azm because only physical loadcell can perform AZM and zeroing.
			lc_tare_check_pending( pLc, lc );
		}
		vs_math_update_host_sensor_status( pLc, 0);
	}
} // end vs_math_tasks()

/**
 * It process math type virtual sensor based on its math expression.
 *
 * @param  pLc	-- host sensor, virtual math type loadcell object pointer.
 * @param	mc	-- math channel number.
 *
 * History:  Created on 2010-02-17 by Wai Fai Chin
 * 2010-10-15 -WFC- included wrong math expression status.
 *
 */

void vs_math_update_host_sensor_status( LOADCELL_T 	*pLc, BYTE mc)
{
	BYTE	*pbStatus;
	if ( mc < MAX_NUM_VS_MATH) {
		pbStatus = &gaVsMathStatus[mc];
		if ( VS_MATH_STATUS_IN_CAL & (*pbStatus ))
			pLc-> runModes |= LC_RUN_MODE_IN_CAL;
		else
			pLc-> runModes &= ~LC_RUN_MODE_IN_CAL;

		if ( VS_MATH_STATUS_OVERLOAD & (*pbStatus ))			// if one or more input sensors were overloaded, flagged it but not increment the service counters.
			pLc-> status |= LC_STATUS_OVERLOAD;
		else if ( !(LC_STATUS_OVERLOAD & pLc-> status) )		// cannot overide overload loadcell status itself because math sensor also check overload with its resultant weight.
			pLc-> status &= ~LC_STATUS_OVERLOAD;				// clear overload flag.

		if ( VS_MATH_STATUS_UNDERLOAD & (*pbStatus ))
			pLc-> status |= LC_STATUS_UNDERLOAD;
		else
			pLc-> status &= ~LC_STATUS_UNDERLOAD;				// clear underload flag.

		if ( VS_MATH_STATUS_OVER_RANGE & (*pbStatus ))
			pLc-> status2 |= LC_STATUS2_OVER_RANGE;
		else
			pLc-> status2 &= ~LC_STATUS2_OVER_RANGE;

		if ( VS_MATH_STATUS_UNER_RANGE & (*pbStatus ))
			pLc-> status2 |= LC_STATUS2_UNER_RANGE;
		else
			pLc-> status2 &= ~LC_STATUS2_UNER_RANGE;

		if ( VS_MATH_STATUS_UN_CAL & (*pbStatus ))
			pLc-> status2 |= LC_STATUS2_UN_CAL;
		else
			pLc-> status2 &= ~LC_STATUS2_UN_CAL;

		if ( VS_MATH_STATUS_DISABLED & (*pbStatus ))
			pLc-> status2 |= LC_STATUS2_INPUTS_DISABLED;
		else
			pLc-> status2 &= ~LC_STATUS2_INPUTS_DISABLED;

		if ( VS_MATH_STATUS_WRONG_EXPRESSION & (*pbStatus) )			// if it has a wrong math expression, 2010-10-15 -WFC-
			pLc-> status2 |= LC_STATUS2_WRONG_MATH_EXPRESSION;
		else
			pLc-> status2 &= ~LC_STATUS2_WRONG_MATH_EXPRESSION;
	}
} // end vs_math_update_host_sensor_status(,)

/**
 * It process math type virtual sensor based on its math expression.
 *
 * @param	mc			-- math channel number
 * @param	filterType	-- filter Type, 0 == filter type, 0x80 == wanted un-filter type..
 * @param	pfResult	-- points to result
 *
 * @return PASSED or FAILED.
 *
 * @note	This version only process adding operand. Other complex math
 *			might include in the future version.
 *
 * History:  Created on 2010/02/17 by Wai Fai Chin
 * 2010-08-30 -WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 */

BYTE vs_math_evaluate( BYTE mc, BYTE filterType, float *pfResult )
{
	BYTE	i;
	BYTE	status;
	BYTE	state;
	BYTE	unit;											// unit of this math channel.
	BYTE	vStackIndex;									// value stack index
	BYTE	opStackIndex;									// operator stack index.
	BYTE	opStack[ MAX_VS_COMPILED_EXPRS_SIZE ];			// operator stack
	float	faValueStack[ MAX_VS_COMPILED_EXPRS_SIZE ];
	float	fTmp;
	BYTE	*pExprs;

	status = FAILED;
	if ( mc < MAX_NUM_VS_MATH ) {
		status = FAILED;												// assumed failed.
		vStackIndex = opStackIndex = 0;
		faValueStack[0] = 0.0f;
		pExprs = &gaVsMath[ mc ].mathExprs[ 0 ];
		// 2010-08-30	-WFC- unit = gabSensorShowCBunitsFNV[ gabVSMathSensorIdFNV[ mc ] ];
		unit = gabSensorViewUnitsFNV[ gabVSMathSensorIdFNV[ mc ] ];			// 2010-08-30	-WFC-
		if ( !(VS_MATH_STATUS_WRONG_EXPRESSION & gaVsMathStatus[mc]) ) {	// if it has a valid math expression.
			gaVsMathStatus[mc] &= VS_MATH_STATUS_CLEAR_MASK;
			for ( i=0; i < MAX_VS_COMPILED_EXPRS_SIZE; i++) {
				if ( *pExprs < VS_MATH_EXPRS_OP )	{			// if this is a sensor id
					if ( vStackIndex < MAX_VS_COMPILED_EXPRS_SIZE ) {
						state = vs_math_get_sensor_value( *pExprs, filterType, unit, &fTmp );
						vs_math_update_status( mc, *pExprs, state);
						if ( ( SENSOR_STATE_GOT_VALID_VALUE == state ) ||
							 (SENSOR_STATE_OVERLOAD == state ) ||
							 (SENSOR_STATE_UNDERLOAD == state ) ||
							SENSOR_VALUE_TYPE_NON_FILTERED_bm &	filterType ) {		// non filtered value type trump everything.
							faValueStack[ vStackIndex ] = fTmp;		// push sensor value on stack.
							vStackIndex++;
						}
						else {
							// vs_math_update_status( mc, state);
							break;		// break out the loop.
						}
					}
					if ( opStackIndex > 0  ) {					// if it already had a operator, then process it.
						opStackIndex--;
						if ( VS_MATH_EXPRS_ADD == opStack[ opStackIndex] ) {
							if ( vStackIndex > 1 ) {
								faValueStack[ vStackIndex - 2 ] = faValueStack[ vStackIndex - 1 ] + faValueStack[ vStackIndex - 2 ];
								vStackIndex--;					// points at [vStackIndex - 1]
							}
						}
					}
				}
				else if ( (VS_MATH_EXPRS_OP | VS_MATH_EXPRS_ADD) == *pExprs) {		// if this is an operator, then push it on operator stack.
					if ( opStackIndex < MAX_VS_COMPILED_EXPRS_SIZE ) {
						opStack[ opStackIndex ] = VS_MATH_EXPRS_ADD;
						opStackIndex++;
					}
				}
				else if ( VS_MATH_EXPRS_END == *pExprs )	{
					// to do future version process remaining complex math expression here before exit.
					status = PASSED;
					break;
				}
				pExprs++;
			}// end for
		} // end it has valid math expression.
	} // end if ( mc < MAX_NUM_VS_MATH ) {}
	*pfResult = faValueStack[0];
	return status;
} // end vs_math_evaluate()

/**
 * It zeros all its input sensors based on its math expression.
 *
 * @param	lc	-- math type loadcell sensor number.
 *
 *
 * History:  Created on 2010/06/19 by Wai Fai Chin
 */

BYTE vs_math_zero_input_sensors( BYTE lc )
{
	BYTE	i;
	BYTE	status;
	BYTE	mc;
	BYTE	*pExprs;
	LOADCELL_T *pLc;

	mc = 0;		// for ScaleCore, there is only one math channel.
	status = FAILED;

	if (  SENSOR_NUM_MATH_LOADCELL == lc )	{					// ensure it is a math type loadcell.
		pLc = &gaLoadcell[ lc ];
		if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
			if ( mc < MAX_NUM_VS_MATH ) {
				status = FAILED;								// assumed failed.
				pExprs = &gaVsMath[ mc ].mathExprs[ 0 ];
				if ( !(VS_MATH_STATUS_WRONG_EXPRESSION & gaVsMathStatus[mc]) ) {	// if it has a valid math expression.
					for ( i=0; i < MAX_VS_COMPILED_EXPRS_SIZE; i++) {
						if ( *pExprs < VS_MATH_EXPRS_OP )	{			// if this is a sensor id
							if ( *pExprs < MAX_NUM_LOADCELL )	{ // make sure it is a physical loadcell
								pLc = &gaLoadcell[ *pExprs ];
								lc_zero( pLc, gafLcZeroOffsetWtNV[ *pExprs ], *pExprs );
							}
							//TODO: handle zero remote sensor in future version.
						}
						else if ( VS_MATH_EXPRS_END == *pExprs )	{
							status = PASSED;
							break;
						}
						pExprs++;
					}// end for
				} // end it has valid math expression.
			} // end if ( mc < MAX_NUM_VS_MATH ) {}
		}
	} // end if (  SENSOR_NUM_MATH_LOADCELL == lc ) {}.

	return status;
} // end vs_math_zero_input_sensors()

// ********************

/**
 * It zeros all its input sensors based on its math expression.
 *
 * @param	lc	-- math type loadcell sensor number.
 *
 *
 * History:  Created on 2010/06/19 by Wai Fai Chin
 */

BYTE vs_math_undo_zero_input_sensors( BYTE lc )
{
	BYTE	i;
	BYTE	status;
	BYTE	mc;
	BYTE	*pExprs;
	LOADCELL_T *pLc;

	mc = 0;		// for ScaleCore, there is only one math channel.
	status = FAILED;

	if (  SENSOR_NUM_MATH_LOADCELL == lc )	{					// ensure it is a math type loadcell.
		pLc = &gaLoadcell[ lc ];
		if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
			if ( mc < MAX_NUM_VS_MATH ) {
				status = FAILED;								// assumed failed.
				pExprs = &gaVsMath[ mc ].mathExprs[ 0 ];
				if ( !(VS_MATH_STATUS_WRONG_EXPRESSION & gaVsMathStatus[mc]) ) {	// if it has a valid math expression.
					for ( i=0; i < MAX_VS_COMPILED_EXPRS_SIZE; i++) {
						if ( *pExprs < VS_MATH_EXPRS_OP )	{			// if this is a sensor id
							if ( *pExprs < MAX_NUM_LOADCELL )	{ // make sure it is a physical loadcell
								lc_zero_undo( *pExprs );
							}
							//TODO: handle zero remote sensor in future version.
						}
						else if ( VS_MATH_EXPRS_END == *pExprs )	{
							status = PASSED;
							break;
						}
						pExprs++;
					}// end for
				} // end it has valid math expression.
			} // end if ( mc < MAX_NUM_VS_MATH ) {}
		}
	} // end if (  SENSOR_NUM_MATH_LOADCELL == lc ) {}.

	return status;
} // end vs_math_undo_zero_input_sensors()


#if ( CONFIG_TEST_VS_MATH_MODULE == FALSE )
/**
 * It fetches the value of a specified sensor and unit.
 *
 * @param	sn		-- sensor ID,
 * @param	filterType	-- filter Type, 0 == filter type, 0x80 == wanted un-filter type..
 * @param	unit	-- wanted unit for the request value.
 * @param	pfV		-- point to floating variable that save the sensor value.
 * @return	state of this sensor such as valid value, uncal, in cal, overload etc...
 *
 * History:  Created on 2010/02/23 by Wai Fai Chin
 */

BYTE	vs_math_get_sensor_value( BYTE sn, BYTE filterType, BYTE unit, float *pfV )
{
	BYTE state;

	*pfV = 0.0;
	state = FAILED;												// assumed failed.
	if ( sn < MAX_NUM_SENSORS ) {									// ensured sensor number is valid.
		switch ( gabSensorTypeFNV[ sn ] )	{
			case SENSOR_TYPE_LOADCELL:
					state = loadcell_get_value_of_type( sn, filterType | SENSOR_VALUE_TYPE_CUR_MODE, unit, pfV );
				break;
			// TODO: SENSOR_TYPE_REMOTE
		} // end switch() {}
	}
	return state;
} // end vs_math_get_sensor_value()

#endif

/**
 * It fetches the value of specified type of a loadcell.
 *
 * @param	sn		-- sensore ID;
 * @param	valueType	-- value type such as current mode, gross, net etc...
 * @param	unit	-- wanted unit for the request value.
 * @param	pfV		-- point to floating variable that save the sensor value.
 * @return	state of this loadcell such as valid value, uncal, in cal, overload etc...
 *
 * History:  Created on 2010-06-19 by Wai Fai Chin
 * 2010-10-15 -WFC- updated pLc-> status2 ADC busy status.
 *
 */

BYTE	vs_math_get_this_value( BYTE sn, BYTE valueType, BYTE unit, float *pfV )
{
	BYTE		state;
	float		unitCnv;
	LOADCELL_T *pLc;

	*pfV = 0.0;
	state = SENSOR_STATE_ADC_BUSY;								// assumed sensor is busy.
	if ( SENSOR_NUM_MATH_LOADCELL == sn ) {							// ensured sensor number is valid.
		if ( SENSOR_TYPE_MATH_LOADCELL ==  gabSensorTypeFNV[ sn ] )	{
			pLc = &gaLoadcell[ sn ];
			if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
				if ( (( LC_STATUS_GOT_VALID_WEIGHT | LC_STATUS_OVERLOAD | LC_STATUS_UNDERLOAD ) & (pLc-> status)) ||
						LC_STATUS3_GOT_UNFILTER_VALUE & (pLc->status3) ) {	// if this loadcell has valid value

					if ( SENSOR_VALUE_TYPE_NON_FILTERED_bm & valueType ) {
						*pfV = pLc-> grossWtUnFiltered;
						state = SENSOR_STATE_GOT_VALID_VALUE;
					}
					else {
						switch ( ( ~SENSOR_VALUE_TYPE_NON_FILTERED_bm & valueType ) ){
						case SENSOR_VALUE_TYPE_CUR_MODE:
								if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) // if loadcell is in NET mode
									*pfV = pLc-> netWt;
								else
									*pfV = pLc-> grossWt;
							break;
						case SENSOR_VALUE_TYPE_GROSS:
								*pfV = pLc-> grossWt;
							break;
						case SENSOR_VALUE_TYPE_NET:
								*pfV = pLc-> netWt;
							break;
						case SENSOR_VALUE_TYPE_ADC_COUNT:
								*pfV = 0.0f;
							break;
						} // end switch()
					}

					if ( SENSOR_VALUE_TYPE_ADC_COUNT != valueType ) {		// unit does not apply to ADC counts.
						if ( unit != SENSOR_VALUE_UNIT_CUR_MODE ) {
							// if unit is different then perform unit conversion
							if ( unit!= pLc-> viewCB.unit) {
								memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ pLc-> viewCB.unit ][unit],  sizeof(float));
								*pfV = *pfV * unitCnv;
							}
						}
					}

					// if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) )		// this IF statement caused SENSOR_STATE_ADC_BUSY if filtering was not settled.
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
					if ( VS_MATH_STATUS_UN_CAL & gaVsMathStatus[0] )	{
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
		}
	}
	// 2010-10-15 -WFC- ^

	return state;
} // end vs_math_get_this_value()

/**
 * It returns the value of a specified sensor.
 *
 * @param	sn	-- sensor ID,
 * @return	vs_math channel id, 0xFF == SENSOR_TYPE_UNDEFINED.
 *
 * History:  Created on 2010/02/23 by Wai Fai Chin
 */

BYTE	vs_math_get_math_channel_id( BYTE sn )
{
	BYTE i;
	for ( i = 0; i < MAX_NUM_VS_MATH; i++) {
		if ( sn == gabVSMathSensorIdFNV[i] )
			break;
	}
	if ( i >= MAX_NUM_VS_MATH )
		i = SENSOR_TYPE_UNDEFINED;
	return i;
} // end vs_math_get_math_channel_id()


/**
 * It updates status of math channel.
 *
 * @param	mc	-- math channel number
 * @param	sn	-- sn sensor number.
 * @param	state -- state of input senor.
 *
 * History:  Created on 2010-06-19 by Wai Fai Chin
 */

void vs_math_update_status( BYTE mc, BYTE sn, BYTE state)
{
	BYTE isInCal;
	BYTE *pbStatus;
	LOADCELL_T *pLc;

	if ( mc < MAX_NUM_VS_MATH) {
		pbStatus = &gaVsMathStatus[mc];
		isInCal = state & SENSOR_STATE_IN_CAL_bm;
		state &= ~SENSOR_STATE_IN_CAL_bm;			// strip in cal flag out.
		if ( isInCal )
			*pbStatus |= VS_MATH_STATUS_IN_CAL;

		switch ( state ) {
			case SENSOR_STATE_DISABLED:
				*pbStatus |= VS_MATH_STATUS_DISABLED;
				break;
			case SENSOR_STATE_UNCAL:
				*pbStatus |= VS_MATH_STATUS_UN_CAL;
				break;
			case SENSOR_STATE_OVERLOAD:
				*pbStatus |= VS_MATH_STATUS_OVERLOAD;
				break;
			case SENSOR_STATE_UNDERLOAD:
				*pbStatus |= VS_MATH_STATUS_UNDERLOAD;
				break;
			case SENSOR_STATE_UNDER_RANGE:
				*pbStatus |= VS_MATH_STATUS_UNER_RANGE;
				break;
			case SENSOR_STATE_OVER_RANGE:
				*pbStatus |= VS_MATH_STATUS_OVER_RANGE;
				break;
		}

		if ( sn < (MAX_NUM_LOADCELL - 1) ) {		// if sensor is real phsyical loadcell.
			pLc = &gaLoadcell[ sn ];
			if ( LC_RUN_MODE_IN_CAL & pLc-> runModes )
				*pbStatus |= VS_MATH_STATUS_IN_CAL;

			if ( LC_STATUS_OVERLOAD & pLc-> status)
				*pbStatus |= VS_MATH_STATUS_OVERLOAD;

			if ( LC_STATUS_UNDERLOAD & pLc-> status )
				*pbStatus |= VS_MATH_STATUS_UNDERLOAD;

			if ( LC_STATUS2_OVER_RANGE & pLc-> status2 )
				*pbStatus |= VS_MATH_STATUS_OVER_RANGE;

			if ( LC_STATUS2_UNER_RANGE & pLc-> status2 )
				*pbStatus |= VS_MATH_STATUS_UNER_RANGE;

			if ( LC_STATUS2_UN_CAL & pLc-> status2 )
				*pbStatus |= VS_MATH_STATUS_UN_CAL;

			if ( !(LC_RUN_MODE_ENABLED & pLc-> runModes) )		// if this input loadcell is disabled,
				*pbStatus |= VS_MATH_STATUS_DISABLED;
		}
	}

} // end vs_math_update_status()

/*



		if ( VS_MATH_STATUS_DISABLED & (*pbStatus ))
			pLc-> status2 |= LC_STATUS2_INPUTS_DISABLED;
		else
			pLc-> status2 &= ~LC_STATUS2_INPUTS_DISABLED;
	}
 *
 */

///////////////////////////////////////////////////////////////////////////////
//                      Test functions for this module.                      //
///////////////////////////////////////////////////////////////////////////////

#if  ( CONFIG_TEST_VS_MATH_MODULE == TRUE )

const char gcVsMathTestStr1[] PROGMEM = "1+2";
const char gcVsMathExpectResult1[] PROGMEM = {1,0x81, 2, 0x80};

const char gcVsMathTestStr2[] PROGMEM = "+2";
const char gcVsMathExpectResult2[] PROGMEM = {0x80};

const char gcVsMathTestStr3[] PROGMEM = "2+";
const char gcVsMathExpectResult3[] PROGMEM = {0x80};

const char gcVsMathTestStr4[] PROGMEM = "2";
const char gcVsMathExpectResult4[] PROGMEM = {2, 0x80};

const char gcVsMathTestStr5[] PROGMEM = "1+2+30";
const char gcVsMathExpectResult5[] PROGMEM = {0x80};

const char gcVsMathTestStr6[] PROGMEM = "AB";
const char gcVsMathExpectResult6[] PROGMEM = {0x80};

const char gcVsMathTestStr7[] PROGMEM = "A+";
const char gcVsMathExpectResult7[] PROGMEM = {0x80};

const char gcVsMathTestStr8[] PROGMEM = "2+B";
const char gcVsMathExpectResult8[] PROGMEM = {0x80};

const char gcVsMathTestStr9[] PROGMEM = "0+1+2";
const char gcVsMathExpectResult9[] PROGMEM = {0, 0x81, 1, 0x81, 2, 0x80};


// Array of vs_math module test string.
const char *gcaVS_Math_Test_Str[]	PROGMEM = { gcVsMathTestStr1, gcVsMathTestStr2,
		gcVsMathTestStr3, gcVsMathTestStr4,	gcVsMathTestStr5, gcVsMathTestStr6,
		gcVsMathTestStr7, gcVsMathTestStr8,	gcVsMathTestStr9
};

// Array of vs_math module test string.
const char *gcaVS_Math_Expected_Result[]	PROGMEM = { gcVsMathExpectResult1, gcVsMathExpectResult2,
		gcVsMathExpectResult3, gcVsMathExpectResult4, gcVsMathExpectResult5, gcVsMathExpectResult6,
		gcVsMathExpectResult7, gcVsMathExpectResult8, gcVsMathExpectResult9
};

#define VS_MATH_MAX_NUM_TEST_STRING		9

const char gcVs_TestHeader[] PROGMEM = "\nVs math tested %u items, %u errors";

const float gcVs_TestSensorValue[] PROGMEM	= { 1000.3, 2000.3, 3000.3};

BYTE vs_math_test_cmp_expres_result( BYTE *pResult, BYTE *pExpectedResult );


/**
 * It tests functions in the vs_math module.
 *
 * @return number of errors. 0 == Passed.
 *
 * History:  Created on 2010/02/19 by Wai Fai Chin
 */

BYTE vs_math_test( void )
{
	BYTE	i;
	BYTE	errorCnt;
	BYTE	*pSrcStr;								// points to source string.
	BYTE	exprsStr[ MAX_VS_RAW_EXPRS_SIZE * 3 ];
	float	fResult;

	errorCnt = 0;
	for ( i=0; i < VS_MATH_MAX_NUM_TEST_STRING; i++)	{
		memcpy_P( &pSrcStr, &gcaVS_Math_Test_Str[ i ], sizeof(BYTE *));
		strcpy_P( exprsStr, pSrcStr );
		vs_math_compile_expression( 0, exprsStr );
		memcpy_P( &pSrcStr, &gcaVS_Math_Expected_Result[ i ], sizeof(BYTE *));
		memcpy_P( exprsStr, pSrcStr, MAX_VS_COMPILED_EXPRS_SIZE);
		if ( !vs_math_test_cmp_expres_result( &gaVsMath[0].mathExprs[0], exprsStr))
				errorCnt++;
	}

	// last math expression is "0+1+2" sum of value of sensor 0 to 2; 
	vs_math_evaluate( 0, &fResult );
	if ( !float_a_eq_b( 6000.9, fResult ))
		errorCnt++;

	gaVsMath[0].mathExprs[0] = 0;
	gaVsMath[0].mathExprs[1] = VS_MATH_EXPRS_END;
	// new math expression is "0" which is the value of sensor 0
	vs_math_evaluate( 0, &fResult );
	if ( !float_a_eq_b( 1000.3, fResult ))
		errorCnt++;

	sprintf_P( exprsStr, gcVs_TestHeader, VS_MATH_MAX_NUM_TEST_STRING + 2, ( UINT16) errorCnt );
	serial0_send_string( exprsStr );
	return errorCnt;
} // end vs_math_test()

/**
 * It compares the result with the expected result.
 * It compares the result until it encountered 0x80.
 *
 * @param	pResult	-- points to result buffer.
 * @param	pExpectedReusult	-- points to expected result buffer.
 *
 * @return PASSED it matched.
 *
 * History:  Created on 2010/02/19 by Wai Fai Chin
 */

BYTE vs_math_test_cmp_expres_result( BYTE *pResult, BYTE *pExpectedResult )
{
	BYTE	i;
	BYTE	status;

	status = PASSED;
	for ( i=0; i < MAX_VS_COMPILED_EXPRS_SIZE; i++)	{
		if ( *pResult != *pExpectedResult ) {
			status = FAILED;
			break;
		}
		else {
			if ( VS_MATH_EXPRS_END == *pResult )
				break;
		}
		pResult++;
		pExpectedResult++;
	}
	return status;
} // end vs_math_test_cmp_expres_result(,)


/**
 * It fetches the value of a specified sensor and unit.
 *
 * @param	sn		-- sensor ID,
 * @param	unit	-- wanted unit for the request value.
 * @param	pfV		-- point to floating variable that save the sensor value.
 * @return	PASSED if it has a valid sensor value.
 *
 * History:  Created on 2010/02/23 by Wai Fai Chin
 */

BYTE	vs_math_get_sensor_value( BYTE sn, BYTE filterType, BYTE unit, float *pfV )
{
	memcpy_P( pfV, &gcVs_TestSensorValue[ sn ], sizeof(float));
	return PASSED;
} // end vs_math_get_sensor_value()

#endif // ( CONFIG_TEST_VS_MATH_MODULE == TRUE )


