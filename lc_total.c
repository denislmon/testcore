/*! \file lc_total.c \brief loadcell totaling related functions.*/
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
//  History:  Created on 2007/12/21 by Wai Fai Chin
//
//  NOTE:
//		raw_weight   = scale_factor * filtered_Adc_count;
//		zero_weight  = user selected pure raw_weight;
//		gross_weight = raw_weight - zero_weight;
//		net_weight   = gross_weight - tare_weight;
//		gross_wieght, net_weight and tare_weight are rounded based on countby.
//
// ****************************************************************************
 
#include  "lc_total.h"
#include  "scalecore_sys.h"   // 2015-05-12 -WFC-

															/// threshold in percentage of capacity to drop below before total allowed 
BYTE	gabTotalDropThresholdPctCapNV[ MAX_NUM_PV_LOADCELL ];
															/// threshold in percentage of capacity to rise above before total allowed 
BYTE	gabTotalRiseThresholdPctCapNV[ MAX_NUM_PV_LOADCELL ];

														/// upper bound weight of on accept total mode.
float	gafTotalOnAcceptUpperWtNV[ MAX_NUM_PV_LOADCELL];
														/// lower bound weight of on accept total mode.
float	gafTotalOnAcceptLowerWtNV[ MAX_NUM_PV_LOADCELL];

														/// minimum stable time before it can be total.
BYTE	gabTotalMinStableTimeNV[ MAX_NUM_PV_LOADCELL];

/// Values need to save to Nonvolatile Ferri Memory everytime it has changed.
														/// total weight of the loadcell
float	gafTotalWtFNV[ MAX_NUM_PV_LOADCELL];
														/// number of total events
UINT16	gawNumTotalFNV[ MAX_NUM_PV_LOADCELL];
														/// sum of squar total weight of the loadcell
float	gafSumSqTotalWtFNV[ MAX_NUM_PV_LOADCELL];
														/// maximum total weight of the loadcell
float	gafMaxTotalWtFNV[ MAX_NUM_PV_LOADCELL];
														/// minimum total weight of the loadcell
float	gafMinTotalWtFNV[ MAX_NUM_PV_LOADCELL];
														/// total mode
BYTE	gabTotalModeFNV[ MAX_NUM_PV_LOADCELL];


// private helper functions:
void  lc_total_compute( LOADCELL_T *pLc, BYTE lc );
void  lc_total_eval_load_drop_mode( LOADCELL_T *pLc, float curWt, BYTE lc );
void  lc_total_eval_on_accept_mode( LOADCELL_T *pLc, float curWt, BYTE lc );
void  lc_total_eval_on_command_mode( LOADCELL_T *pLc, float curWt );
void  lc_total_eval_auto_mode( LOADCELL_T *pLc, float curWt, BYTE lc );


/**
 * It initializes total data structure of lc.
 *
 * @param		lc	-- loadcell number.
 *
 * @post   updated gaLoadcell[] data structure of this loadcell.
 *
 * @note The loadcell must has a valid viewCB before call this function.
 *
 * History:  Created on 2008/01/02 by Wai Fai Chin
 */

void lc_total_init( BYTE lc )
{
	LOADCELL_T *pLc;
	
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		pLc-> totalT.qualifiedWt	= 
		pLc-> totalT.lastWt			= 0.0;
		lc_total_update_param(lc);
		timer_mSec_set( &(pLc-> totalT.minStableTimer), pLc-> totalT.minStableTime);
	}
} // end lc_total_init()


/**
 * It updates parameter of gaLoadcell[] data structure of lc based on changed value of
 * gabTotalDropThresholdPctCapNV[], gabTotalRiseThresholdPctCapNV[].
 * 
 *
 * @param  lc	-- loadcell number
 *
 * @post   updated totalT in gaLoadcell[] data structure of this loadcell.
 * @note This assumed that viewCapacity value is already in current unit.
 *
 * History:  Created on 2008/01/02 by Wai Fai Chin
 * 2011-04-28 -WFC- 0==0.5%, 1=1%, 100=100%. handle 0.5% threshold setting.
 */

void  lc_total_update_param( BYTE lc )
{
	LOADCELL_T *pLc;
	float		f1pctCap;				// 1% of capacity.
	
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ lc ] ||
			 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[ lc ]	) {
			pLc = &gaLoadcell[ lc ];
			f1pctCap = pLc-> viewCapacity * 0.01;
			// 2011-04-28 -WFC- V 0==0.5%, 1=1%, 100=100%. handle 0.5% threshold setting.
			if ( gabTotalDropThresholdPctCapNV[ lc ] )
				pLc-> totalT.dropWtThreshold = (float) gabTotalDropThresholdPctCapNV[ lc ] * f1pctCap;
			else
				pLc-> totalT.dropWtThreshold = f1pctCap * 0.5f;

			if ( gabTotalRiseThresholdPctCapNV[ lc ] )
				pLc-> totalT.riseWtThreshold = (float) gabTotalRiseThresholdPctCapNV[ lc ] * f1pctCap;
			else
				pLc-> totalT.riseWtThreshold =  f1pctCap * 0.5f;
			// 2011-04-28 -WFC- ^

			// should be done in loadcell_recompute_weights_unit().
			//pLc-> totalT.onAcceptUpperWt = gafTotalOnAcceptUpperWtNV[ lc ];
			//pLc-> totalT.onAcceptLowerWt = gafTotalOnAcceptLowerWtNV[ lc ];
			pLc-> totalMode = gabTotalModeFNV[ lc ];
			pLc-> totalT.minStableTime = gabTotalMinStableTimeNV[ lc ];
			// timer_mSec_set( &(pLc-> totalT.minStableTimer), pLc-> totalT.minStableTime); it should not be set here because every update command will affect the timer.
		}
	}

} // end lc_total_update_param()



/**
 * It evaluates the current weight condition to check for it qualification to be total
 * based on total mode.
 *
 * @param  pLc	-- pointer to loadcell structure.
 * @param	lc	-- loadcell number.
 *
 * @note	Caller must supplied a valid pLc pointer.
 *
 * History:  Created on 2008/01/03 by Wai Fai Chin
 * 2011-04-28 -WFC- Not allow to total if loadcell is overloaded.
 * 2014-10-30 -WFC- put back condition checking for auto total for all products
 * 2015-05-07 -WFC- prevent totaling a load that is already in placed during first power up.
 */

void  lc_total_evaluate( LOADCELL_T *pLc, BYTE lc )
{
	float curWt;
	
	if ( !(LC_STATUS_OVERLOAD & (pLc-> status))) {							// if loadcell is NOT overloaded, then 2011-04-28 -WFC-
		if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 						// if loadcell is in NET mode
			curWt = pLc-> netWt;
		else
			curWt = pLc-> grossWt;

		// 2015-05-07 -WFC- v
		if ( SYS_STATUS_DURING_POWER_UP & gbSysStatus)	{ 		// if system is in power up state.
			if ( curWt > pLc-> totalT.riseWtThreshold ) {		// if the scale has a load on it
				pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;			// flagged NOT ok to total to prevent totaling a load that is already in placed.
			}
		}
		// 2015-05-07 -WFC- ^

		switch ( pLc-> totalMode ) {
			case	LC_TOTAL_MODE_DISABLED		:
					pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;					// flagged not ok to total.
				break;
			case	LC_TOTAL_MODE_AUTO_LOAD		:
			case	LC_TOTAL_MODE_AUTO_NORMAL	:
			case	LC_TOTAL_MODE_AUTO_PEAK		:
					// NOT APPLY TO CHALLENGER3 if ( !(LC_TOTAL_STATUS_DISABLED_AUTO_MODES & pLc-> totalT.status) ) 	// if auto total enabled
					if ( !(LC_TOTAL_STATUS_DISABLED_AUTO_MODES & pLc-> totalT.status) ) 	// if auto total enabled,   2014-10-30 -WFC- put back for all products
						lc_total_eval_auto_mode( pLc, curWt, lc );
				break;
			case	LC_TOTAL_MODE_ON_COMMAND	:
					lc_total_eval_on_command_mode( pLc, curWt );
				break;
			case	LC_TOTAL_MODE_ON_ACCEPT	:
					lc_total_eval_on_accept_mode( pLc, curWt, lc );
				break;
			case	LC_TOTAL_MODE_LOAD_DROP		:
					lc_total_eval_load_drop_mode( pLc, curWt, lc );
				break;
		}
	}
} // end lc_total_evaluate(,).


/**
 * It evaluates current weight for auto total modes.
 * This is a support function for lc_total_evaluate().
 *
 * AUTO LOAD MODE: adds the very first qualified weight to the current total.
 *
 * AUTO NORMAL and PEAK MODES: keep track and update new qualified weight. It
 *            wait until the load had dropped and then add the last qualified
 *            weight to current total.
 *
 * @param  pLc	 -- pointer to loadcell structure.
 * @param  curWt -- weight of current mode such as net or gross.
 * @param	lc	 -- loadcell number.
 *
 * @note	Caller must supplied a valid pLc pointer.
 *
 * History:  Created on 2008/01/03 by Wai Fai Chin
 * 2011-04-28 -WFC- clear totalT.qualifiedWt after called lc_total_compute() to fixed auto peak total mode bug.
 */

void  lc_total_eval_auto_mode( LOADCELL_T *pLc, float curWt, BYTE lc )
{

	if ( LC_TOTAL_MODE_AUTO_LOAD == pLc-> totalMode ) {
		if ( (LC_TOTAL_STATUS_NOT_ALLOW & pLc-> totalT.status) ) {	// if not allow for new total because a total weight has added in this load. We have to wait the to drop before to allow the new total.
			if ( float_a_lte_b( curWt, pLc-> totalT.dropWtThreshold ) ) 	// if the load has unloaded,
				pLc-> totalT.status &= ~LC_TOTAL_STATUS_NOT_ALLOW;		// then allow for new total cycle.
			return;
		}
	}

	if ( float_a_gte_b( curWt, pLc-> totalT.riseWtThreshold ) )	{		// if the load has rose above the threshold,
		if ( LC_STATUS_MOTION & (pLc-> status) ) {						// if loadcell is in motion
			timer_mSec_set( &(pLc-> totalT.minStableTimer), pLc-> totalT.minStableTime);
			pLc-> totalT.status &= ~LC_TOTAL_STATUS_NOT_ALLOW_BLINK_LAMP;		// allow to flag new blink event.
		}
		else {	// it has a stabled load.
			if ( timer_mSec_expired( &(pLc-> totalT.minStableTimer) ) ) {	// if the load has stabled for a minimum period of time,
				if ( LC_TOTAL_MODE_AUTO_PEAK == pLc-> totalMode ) {
					if ( curWt > pLc-> totalT.qualifiedWt ) {
						pLc-> totalT.qualifiedWt = curWt;
						pLc-> status |= LC_STATUS_OK_TO_TOTAL;				// flagged ok to total.
						if ( !( LC_TOTAL_STATUS_NOT_ALLOW_BLINK_LAMP & pLc-> totalT.status) ) {
							// single shot, not allow to blink a new event until this current one is complete, 
							// flag current new blink event.
							pLc-> totalT.status |= (LC_TOTAL_STATUS_NOT_ALLOW_BLINK_LAMP | LC_TOTAL_STATUS_NEW_BLINK_EVENT);
						}
					}
				}
				else {	// at this point, Total Mode is either AUTO NORMAL or AUTO LOAD mode.
					pLc-> totalT.qualifiedWt = curWt;
					pLc-> status |= LC_STATUS_OK_TO_TOTAL;					// flagged ok to total.
					if ( LC_TOTAL_MODE_AUTO_LOAD == pLc-> totalMode )		// if it is in auto load mode,
						lc_total_compute( pLc, lc );							// then adds the very FIRST qualified weight to current total value.
					if ( !( LC_TOTAL_STATUS_NOT_ALLOW_BLINK_LAMP & pLc-> totalT.status) ) {
						// single shot, not allow to blink a new event until this current one is complete, 
						// flag current new blink event.
						pLc-> totalT.status |= (LC_TOTAL_STATUS_NOT_ALLOW_BLINK_LAMP | LC_TOTAL_STATUS_NEW_BLINK_EVENT);
					}
				}
			}
		}
	} // end if load had rose above threshold
	else { // handle auto normal and peak modes.
		if ( LC_STATUS_OK_TO_TOTAL & pLc-> status )	{					// if it had a qualified weight to be add to total
			// 2011-04-28 -WFC- v fixed auto peak total mode bug
//			if ( float_a_lte_b( curWt, pLc-> totalT.dropWtThreshold ) ) 	// if the load has unloaded,
//				lc_total_compute( pLc, lc );									// add the LAST qualified weight to current total value.
			if ( float_a_lte_b( curWt, pLc-> totalT.dropWtThreshold ) ) { 	// if the load has unloaded,
				lc_total_compute( pLc, lc );									// add the LAST qualified weight to current total value.
				if ( LC_TOTAL_MODE_AUTO_PEAK == pLc-> totalMode )
					pLc-> totalT.qualifiedWt = 0.0f;							// clear qualified total Wt if it is in auto total peak mode.
			}
			// 2011-04-28 -WFC- ^ fixed auto peak total mode bug
		}
	}
} // end lc_total_eval_auto_mode().

/**
 * It evaluates on command total mode.
 * This is a support function for lc_total_evaluate().
 *
 * In a new total cycle, it keeps track and update new qualified weight
 * and waits for total command. Once it received a total command,
 * the lc_total_handle_command() added the last qualified weight to 
 * current total and flagged not allow to total until the load had dropped.
 *
 * @param  pLc	 -- pointer to loadcell structure.
 * @param  curWt -- weight of current mode such as net or gross.
 *
 * @note	Caller must supplied a valid pLc pointer.
 *
 * History:  Created on 2008/01/07 by Wai Fai Chin
 */


void  lc_total_eval_on_command_mode( LOADCELL_T *pLc, float curWt )
{
	if ( (LC_TOTAL_STATUS_NOT_ALLOW & pLc-> totalT.status) ) {	// if not allow for new total because a total weight has added in this load. We have to wait the to drop before to allow the new total.
		if ( float_a_lte_b( curWt, pLc-> totalT.dropWtThreshold ) ) 	{// if the load has unloaded,
			//if ( LC_RUN_MODE_PENDING_TOTAL & pLc-> runModes)		// if it has total pending,
			pLc-> totalT.status &= ~LC_TOTAL_STATUS_NOT_ALLOW;		// allow for new total cycle.
		}
	}
	else {
		if ( float_a_gte_b( curWt, pLc-> totalT.riseWtThreshold ) )	{	// if the load has rose above the threshold, 
			pLc-> totalT.qualifiedWt = 0.0f;							// assumed NOT qualified by clear qualifiedWt to be add to total
			pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;					// assumed NOT ok to total.
			if ( LC_STATUS_MOTION & (pLc-> status) ) {				// if loadcell is in motion
				timer_mSec_set( &(	pLc-> totalT.minStableTimer),		// then set the minimum stable period before it can be total.
								pLc-> totalT.minStableTime );				
			}
			else {	// it has a stabled load.
				if ( timer_mSec_expired( &(pLc-> totalT.minStableTimer) ) ) {	// if the load has stabled for a minimum period of time,
					pLc-> totalT.qualifiedWt = curWt;
					pLc-> status |= LC_STATUS_OK_TO_TOTAL;				// flagged ok to total.
				}
			}
		}
	} // end else if the load has rose above the threshold, 
} // end lc_total_eval_on_command_mode().


/**
 * It evaluates on accept total mode.
 * This is a support function for lc_total_evaluate().
 *
 * In a new total cycle, it adds the very first qualified weight to the current total.
 * It waits for load dropped to allow a new total cycel.
 *
 * @param  pLc	 -- pointer to loadcell structure.
 * @param  curWt -- weight of current mode such as net or gross.
 * @param	lc	 -- loadcell number.
 *
 * @note	Caller must supplied a valid pLc pointer.
 *
 * History:  Created on 2008/01/07 by Wai Fai Chin
 */


void  lc_total_eval_on_accept_mode( LOADCELL_T *pLc, float curWt, BYTE lc )
{
	if ( (LC_TOTAL_STATUS_NOT_ALLOW & pLc-> totalT.status) ) {	// if not allow for new total because a total weight has added in this load. We have to wait the to drop before to allow the new total.
		if ( float_a_lte_b( curWt, pLc-> totalT.dropWtThreshold ) ) 	// if the load has unloaded,
			pLc-> totalT.status &= ~LC_TOTAL_STATUS_NOT_ALLOW;		// then allow for new total cycle.
	}
	else {
		if ( float_a_gte_b( curWt, pLc-> totalT.onAcceptLowerWt ) &&
			 float_a_lte_b( curWt, pLc-> totalT.onAcceptUpperWt ))	{	// if the load is within the accept theshold window, 
			pLc-> totalT.qualifiedWt = 0.0f;							// assumed NOT qualified by clear qualifiedWt to be add to total
			pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;					// assumed NOT ok to total.
			if ( LC_STATUS_MOTION & (pLc-> status) ) {				// if loadcell is in motion
				timer_mSec_set( &(	pLc-> totalT.minStableTimer),		// then set the minimum stable period before it can be total.
								pLc-> totalT.minStableTime );				
			}
			else {	// it has a stabled load.
				if ( timer_mSec_expired( &(pLc-> totalT.minStableTimer) ) ) {	// if the load has stabled for a minimum period of time,
					pLc-> totalT.qualifiedWt = curWt;
					pLc-> status |= LC_STATUS_OK_TO_TOTAL;				// flagged ok to total.
					lc_total_compute( pLc, lc );							// then adds the very FIRST qualified weight to current total value.
				}
			}
		}
	} // end else if the load has rose above the threshold, 
} // end lc_total_eval_on_accept_mode(,).


/**
 * It evaluates load drop total mode.
 * This is a support function for lc_total_evaluate().
 *
 * @param  pLc	 -- pointer to loadcell structure.
 * @param  curWt -- weight of current mode such as net or gross.
 * @param	lc	 -- loadcell number.
 *
 * @note	Caller must supplied a valid pLc pointer.
 *
 * History:  Created on 2008/01/08 by Wai Fai Chin
 */


void  lc_total_eval_load_drop_mode( LOADCELL_T *pLc, float curWt, BYTE lc )
{
	
	// if the total button has pressed or received a total command in load drop mode.
	if ( (LC_TOTAL_STATUS_START_LOAD_DROP & pLc-> totalT.status) ) {	
		if ( pLc-> totalT.LDMAccWtUpCnt > 10000) {
			pLc-> totalT.LDMAccWtUp	 = pLc-> totalT.LDMAccWtUp /
										(float) pLc-> totalT.LDMAccWtUpCnt;
			pLc-> totalT.LDMAccWtUpCnt = 1;
		}

		if ( pLc-> totalT.LDMAccWtUpCnt > 10000) {
			pLc-> totalT.LDMAccWtUp	 = pLc-> totalT.LDMAccWtUp /
										(float) pLc-> totalT.LDMAccWtUpCnt;
			pLc-> totalT.LDMAccWtUpCnt = 1;
		}
		
		// if the load has rose above the threshold 
		if ( float_a_gte_b( curWt, pLc-> totalT.riseWtThreshold ) ) {	// if the load has rose above the threshold, 
			// if already had a qualifiedWt
			if ( !float_eq_zero( pLc-> totalT.qualifiedWt ) ) {		// if already had a qualifiedWt
				if ( LC_STATUS_MOTION & (pLc-> status) ) {				// if loadcell is in motion
					timer_mSec_set( &(	pLc-> totalT.minStableTimer),		// then set the minimum stable period before it can be total.
									pLc-> totalT.minStableTime );
				}
				if ( curWt > pLc-> totalT.LDMavgQWt90pct ) {					// current weight is NOT being dropped.
					pLc-> totalT.LDMAccWtUp		+= curWt;						// includes the current weight
					pLc-> totalT.LDMAccWtUp		+= pLc-> totalT.LDMAccWtDown;	// includes the downward weight
					pLc-> totalT.LDMAccWtUpCnt	+= pLc-> totalT.LDMAccWtDownCnt;
					pLc-> totalT.LDMAccWtUpCnt	+= 1;
					pLc-> totalT.qualifiedWt	= pLc-> totalT.LDMAccWtUp /
													(float) pLc-> totalT.LDMAccWtUpCnt;
					pLc-> totalT.LDMavgQWt90pct	= pLc-> totalT.qualifiedWt * 0.9;
					pLc-> totalT.qualifiedWt	= float_round(	pLc-> totalT.qualifiedWt,
																pLc-> viewCB.fValue);
					pLc-> status 				   |= LC_STATUS_OK_TO_TOTAL;	// flagged ok to total.
					pLc-> totalT.LDMAccWtDown	 	= 0.0;						// no downward weight event yet
					pLc-> totalT.LDMAccWtDownCnt	= 0;
				}
				else { // the load was in downward motion
					pLc-> totalT.LDMAccWtDown		+= curWt;					// accumulated downward weight.
					pLc-> totalT.LDMAccWtDownCnt	+= 1;						// count the downward event
				}
			}
			else { // this is the very first time to have qualifiedWt in this total cycle.
				if (!( LC_STATUS_MOTION & (pLc-> status)) &&					// if loadcell is NOT in motion AND
					timer_mSec_expired( &(pLc-> totalT.minStableTimer))  ) {	// has been stable for a minimum period.
					pLc-> totalT.qualifiedWt		= curWt;
					pLc-> totalT.LDMAccWtUp			= curWt;					// the curWt was rose up or stable
					pLc-> totalT.LDMAccWtUpCnt		= 1;
					pLc-> totalT.LDMavgQWt90pct		= curWt * 0.9;
					pLc-> status 				   |= LC_STATUS_OK_TO_TOTAL;	// flagged ok to total.
					pLc-> totalT.LDMAccWtDown		= 0.0;						// no downward weight event yet
					pLc-> totalT.LDMAccWtDownCnt	= 0;
				}
			}
		}
		else {	// load may dropping.
			if ( LC_STATUS_OK_TO_TOTAL & pLc-> status )	{					// if it had a qualified weight to be add to total
				if ( float_a_lte_b( curWt, pLc-> totalT.dropWtThreshold ) ) 	// if the load has unloaded,
					lc_total_compute( pLc, lc );								// 		add qualified weight to current total value.
			}
		}
	} // end if total button has pressed
} // end lc_total_eval_load_drop_mode().


/**
 * It handles total command for a specified loadcell.
 *
 * @param  lc	 -- loadcell number
 *
 * @note	Caller must supplied a valid pLc pointer.
 *
 * History:  Created on 2008/01/15 by Wai Fai Chin
 * 2011-04-28 -WFC- Not allow to total if loadcell is overloaded.
 * 2012-02-06 -WFC- Ensure pending time is 3 times greater than filter sampling window.
 *
 */

void  lc_total_handle_command( BYTE lc )
{
	BYTE 			ok2Total;
	BYTE 			pendingTime;			// 2012-02-06 -WFC-
	LOADCELL_T 		*pLc;		// points to a loadcell
	float 			curWt;
	
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[ lc ] ||
			 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[ lc ]	) {
			pLc = &gaLoadcell[ lc ];
			if ( (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) == 		// if this loadcell is enabled AND in normal active mode.
				  ( (pLc-> runModes) & (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) ) )	{

				if ( !(LC_STATUS_OVERLOAD & (pLc-> status))) {					// if loadcell is NOT overloaded, then 2011-04-28 -WFC-
					if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 			// if loadcell is in NET mode
						curWt = pLc-> netWt;
					else
						curWt = pLc-> grossWt;

					if ( pLc-> totalMode > LC_TOTAL_MODE_DISABLED &&
						 pLc-> totalMode < LC_TOTAL_MODE_LOAD_DROP )	{		// if in auto total modes,
						// toggle auto total enabled.
						if ( LC_TOTAL_STATUS_DISABLED_AUTO_MODES & pLc-> totalT.status ) 	{	// if auto total modes disabled,
							pLc-> totalT.status	&= ~LC_TOTAL_STATUS_DISABLED_AUTO_MODES;		// enabled auto total.
						}
						else {
							pLc-> totalT.status	|= LC_TOTAL_STATUS_DISABLED_AUTO_MODES;			// disabled auto total.
						}
						pLc-> totalT.qualifiedWt = 0.0;							// this ensured that no new total will be added until condition were met
						pLc-> status			 &= ~LC_STATUS_OK_TO_TOTAL;		// flagged NOT ok to total.
					}
					else if ( LC_TOTAL_MODE_LOAD_DROP == pLc-> totalMode ) {
						if ( float_a_gt_b( curWt, pLc-> totalT.dropWtThreshold ) ) {	// if it has a load
							pLc-> totalT.status |= LC_TOTAL_STATUS_START_LOAD_DROP;	// flagged start to do load_drop total operation
							pLc-> totalT.qualifiedWt 		= 							// this ensured that no new total will be added until condition were met
							pLc-> totalT.LDMAccWtUp 		=
							pLc-> totalT.LDMAccWtDown		= 0.0;
							pLc-> totalT.LDMAccWtDownCnt	=
							pLc-> totalT.LDMAccWtUpCnt		= 0;
							pLc-> status					&= ~LC_STATUS_OK_TO_TOTAL;	// flagged NOT ok to total.
						}
					}
					else if ( LC_STATUS_OK_TO_TOTAL & pLc-> status ) { // at this point, it is either in manual or on accept total mode.
						ok2Total = YES;
						if ( LC_TOTAL_MODE_ON_COMMAND == pLc-> totalMode ) {
							if ( float_a_lte_b( curWt, pLc-> totalT.dropWtThreshold ) )	// if the load has unloaded,
								ok2Total = NO;
						}
						if ( ok2Total )
							lc_total_compute( pLc, lc );									//	add qualified weight to current total value.
					}
					else {	// manual or on accept mode and NOT ok to total,
						if ( !(LC_RUN_MODE_PENDING_TOTAL & pLc-> runModes) ) {		// if it have no total pending yet,
							pLc-> runModes |= LC_RUN_MODE_PENDING_TOTAL;					 // flag pending total.
							timer_mSec_set( &(pLc-> totalT.pendingTimer), pLc-> pendingTime);

							// 2012-02-06 -WFC- v
							// timer_mSec_set( &(pLc-> totalT.pendingTimer), pLc-> pendingTime);
							pendingTime = pLc-> pendingTime;
							if ( (gaLSensorDescriptor[lc].cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK ) ) {
								pendingTime = gaLSensorDescriptor[lc].filterTimer.interval<<1 + gaLSensorDescriptor[lc].filterTimer.interval + 10; 		// 3x filter sampling time plus 0.5 seconds.
								if ( pendingTime < pLc-> pendingTime ) 		// if new pendingTime < user defined pending time, then use user defined time.
									pendingTime = pLc-> pendingTime;
							}
							timer_mSec_set( &(pLc-> totalT.pendingTimer), pendingTime);
							// 2012-02-06 -WFC- ^

						} // end if
					}
			    } // end if ( !(LC_STATUS_OVERLOAD & (pLc-> status))) {
			} // end if loadcell is normal active mode.
		}
	}
} // end lc_total_handle_command().


/**
 * It adds qualified weight to current total and compute statistic values.
 * This is a support function for lc_total_evaluate().
 *
 * @param  pLc	 -- pointer to loadcell structure.
 * @param  lc	 -- loadcell number
 *
 * @note	Caller must supplied a valid pLc pointer.
 *
 * History:  Created on 2008/01/10 by Wai Fai Chin
 */


void  lc_total_compute( LOADCELL_T *pLc, BYTE lc )
{
	float qWt;
	
	qWt = pLc-> totalT.qualifiedWt;
	if ( float_eq_zero( qWt ))
		return;

	if ( pLc-> totalT.status & LC_TOTAL_STATUS_SKIP_TOTAL ) {	// if need to skip total to prevent bogus totaling,
		pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;				// flagged NOT ok to total.
		pLc-> totalT.status &= ~LC_TOTAL_STATUS_SKIP_TOTAL; 	// one shot only, from now on, don't skip
	}
	else if ( (*(pLc-> pNumTotal)) < 0xFFFF )	{		// limit number of total to 0xFFFF until user reset it.
			(*(pLc-> pNumTotal))++;

			pLc-> totalT.lastWt = qWt;					// last qualified weight that added to current total.
		
			*(pLc-> pTotalWt) += qWt;
		
			if ( 1 == *(pLc-> pNumTotal) ) {				// if this is the very first time,
				*(pLc-> pSumSqTotalWt) = (qWt * qWt);
				*(pLc-> pMaxTotalWt)	= qWt;
				*(pLc-> pMinTotalWt)	= qWt;
			}
			else {
				*(pLc-> pSumSqTotalWt) += (qWt * qWt);
				if ( float_a_gt_b( qWt, *(pLc-> pMaxTotalWt)) ) {
					*(pLc-> pMaxTotalWt)	= qWt;
				}
				else if ( float_a_lt_b( qWt, *(pLc-> pMinTotalWt)) ) {
					*(pLc-> pMinTotalWt)	= qWt;
				}
			}
			//pLc-> totalT.status |= LC_TOTAL_STATUS_NOT_ALLOW;			// NOT ALLOW TOTAL again until the load had dropped.
			pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;					// flagged NOT ok to total.
			pLc-> status |= LC_STATUS_HAS_NEW_TOTAL;					// flagged it has a new total weight for notify host and remote devices.
			nv_cnfg_fram_save_totaling_statistics( lc );
	}
	
	pLc-> totalT.status |= LC_TOTAL_STATUS_NOT_ALLOW;			// NOT ALLOW TOTAL again until the load had dropped.
	pLc-> runModes &= ~LC_RUN_MODE_PENDING_TOTAL;				// flagged NO MORE pending total.
} // end lc_total_compute(,).


/**
 * It clears total weight and total event statistics of a specified loadcell.
 * This is a support function for lc_total_evaluate().
 *
 * @param  pLc	 -- pointer to loadcell structure.
 * @param  lc	 -- loadcell number
 *
 * @note	Caller must supplied a valid pLc pointer.
 *
 * History:  Created on 2009/08/10 by Wai Fai Chin
 */

void  lc_total_clear_total( LOADCELL_T *pLc, BYTE lc )
{

	if ( (*(pLc-> pNumTotal)) )	{					// if number of total is not zero
		*(pLc-> pNumTotal) 			=
		pLc-> totalT.LDMAccWtDownCnt	= 
		pLc-> totalT.LDMAccWtUpCnt		= 0;
		pLc-> totalT.lastWt				= 		
		*(pLc-> pSumSqTotalWt) 		=
		*(pLc-> pMaxTotalWt)			=
		*(pLc-> pMinTotalWt)			=
		*(pLc-> pTotalWt)				=
		pLc-> totalT.qualifiedWt 		= 
		pLc-> totalT.LDMAccWtUp 		= 							
		pLc-> totalT.LDMAccWtDown		= 0.0f;						
		
		pLc-> status |= LC_STATUS_HAS_NEW_TOTAL;				// flagged it has a new total weight for notify host and remote devices.
		pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;				// flagged NOT ok to total.
		pLc-> runModes &= ~LC_RUN_MODE_PENDING_TOTAL;			// flagged NO MORE pending total.
		nv_cnfg_fram_save_totaling_statistics( lc );
	}
} // end lc_total_clear_total(,).


/**
 * It removes last total weight and total event statistics of a specified loadcell.
 * This is a support function for lc_total_evaluate().
 *
 * @param  pLc	 -- pointer to loadcell structure.
 * @param  lc	 -- loadcell number
 *
 * @note	Caller must supplied a valid pLc pointer.
 *
 * History:  Created on 2009/08/10 by Wai Fai Chin
 */

void  lc_total_remove_last_total( LOADCELL_T *pLc, BYTE lc )
{

	float remWt;

	remWt = pLc-> totalT.lastWt;
	if ( (*(pLc-> pNumTotal)) && remWt != 0.0f )	{			// if number of total is not zero
		pLc-> totalT.LDMAccWtDownCnt	= 
		pLc-> totalT.LDMAccWtUpCnt		= 0;
		pLc-> totalT.qualifiedWt 		= 
		pLc-> totalT.LDMAccWtUp 		= 
		pLc-> totalT.LDMAccWtDown		= 
		pLc-> totalT.lastWt 			= 0.0f;					// ensure one time only until the next total event.			
		
		(*(pLc-> pNumTotal))--;
		*(pLc-> pTotalWt)				-=	remWt;
		*(pLc-> pSumSqTotalWt) 		-=	(remWt * remWt);
		
		if ( float_a_gt_b( remWt, *(pLc-> pMaxTotalWt)) )
			*(pLc-> pMaxTotalWt)	= remWt;

		if ( float_a_lt_b( remWt, *(pLc-> pMinTotalWt)) )
				*(pLc-> pMinTotalWt)	= remWt;

		pLc-> status |= LC_STATUS_HAS_NEW_TOTAL;				// flagged it has a new total weight for notify host and remote devices.
		pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;				// flagged NOT ok to total.
		pLc-> runModes &= ~LC_RUN_MODE_PENDING_TOTAL;			// flagged NO MORE pending total.
		nv_cnfg_fram_save_totaling_statistics( lc );
	}
} // end lc_total_remove_last_total(,).

