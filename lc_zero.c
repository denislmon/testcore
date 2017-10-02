/*! \file lc_zero.c \brief loadcell zeroing related functions.*/
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
//  History:  Created on 2007/11/07 by Wai Fai Chin
// 
//   Zeroing of loadcell based on user definition of zero such as zero threshold.
//  AZM == AUTO ZERO MAINTENANCE.
//  NV suffix stands for nonvolatile memory. It is in RAM and recalled from nonvolatile memory during powerup.
//
//  note:
//		raw_weight   = scale_factor * filtered_Adc_count;
//		zero_weight  = user selected raw_weight;
//		gross_weight = raw_weight - zero_weight;
//		net_weight   = gross_weight - tare_weight;
//		gross_wieght, net_weight and tare_weight are rounded based on countby.
//
// ****************************************************************************


/// \par Overview on Zeroing loadcell
///	\code
///   Zeroing of loadcell based on user definition of zero such as zeroing band threshold
///   and zero configuration features which are zero on powerup, auto zero maitanence(AZM).
///
///	  If enabled zero on powerup, here is how it works:
///     after init the entire system, call setup zero on powerup function.
///     This function set the zero threshold band based on powerup zero threshold setting.
///		set zero pending flag and pending timer.
///     The sensor module will try to zero the loadcell until it successed zero or pending time expried,
///     then it will recomputed the zero threshold weight based on the normal zero threshold setting.
///
///   If zero by command or key pressed, here is how it works:
///      if the loadcell is in motion, then
///			 if no pending zero, then set pending zero flag and pending zero timer.
///      else
///          if current weight is within the zero threshold band then zero the loadcell by set the current weight as zero weight.
///
///	  If AZM enabled, then it will check AZM on a specified interval time.
///   	It will set the new zero weight with current pure raw weight if current weight (net or gross) within azm band threshold weight AND
///     current pure raw weight within zero band threshold weight.
///
///   Center of Zero is always check whenever there is a new weight value.
///
/// NOTE:
///		raw_weight   = scale_factor * filtered_Adc_count;
///		zero_weight  = user selected pure raw_weight;
///		gross_weight = raw_weight - zero_weight;
///		net_weight   = gross_weight - tare_weight;
///		gross_wieght, net_weight and tare_weight are rounded based on countby.
///		raw_weight, zeroWt and gafLcZeroOffsetWtNV[] are never round.
///	\endcode

#include "commonlib.h"
#include "lc_zero.h"
#include "loadcell.h"
#include "timer.h"
#include "lc_total.h"

#include  "nvmem.h"

#define		LC_ZERO_PENDING_TIME			TT_2SEC
#define		LC_ZERO_POWERUP_PENDING_TIME	TT_10SEC

/// azm interval time
BYTE	gab_STD_AZM_IntervalTimeNV[ MAX_NUM_PV_LOADCELL];
BYTE	gab_NTEP_AZM_IntervalTimeNV[ MAX_NUM_PV_LOADCELL];
BYTE	gab_OIML_AZM_IntervalTimeNV[ MAX_NUM_PV_LOADCELL];

/// Auto zero maintenance countby range band. It will use to compute azmThresholdWt.
BYTE	gab_STD_AZM_CBRangeNV[ MAX_NUM_PV_LOADCELL];		// Auto zero maintenance countby range band.
BYTE	gab_NTEP_AZM_CBRangeNV[ MAX_NUM_PV_LOADCELL];		// Auto zero maintenance countby range band.
BYTE	gab_OIML_AZM_CBRangeNV[ MAX_NUM_PV_LOADCELL];		// Auto zero maintenance countby range band.

/// Percent of capacity above cal zero that can be zeroed off if below this limit, 1 = 1%, for compute zeroBandHiWt zero band high limit weight;
BYTE	gab_STD_pcentCapZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off, 1 = 1%
BYTE	gab_NTEP_pcentCapZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off, 1 = 1%
BYTE	gab_OIML_pcentCapZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off, 1 = 1%

/// Percent of capacity below cal zero that can be zeroed off if above this limit, 1 = 1%, for compute zeroBandLoWt zero band low limit weight
BYTE	gab_STD_pcentCapZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off, 1 = 1%
BYTE	gab_NTEP_pcentCapZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off, 1 = 1%
BYTE	gab_OIML_pcentCapZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off, 1 = 1%

/// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.
BYTE	gab_STD_pwupZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.
BYTE	gab_NTEP_pwupZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.
BYTE	gab_OIML_pwupZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.

/// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.
BYTE	gab_STD_pwupZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.
BYTE	gab_NTEP_pwupZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.
BYTE	gab_OIML_pwupZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.


/// Values need to save to Nonvolatile Ferri Memory everytime it has changed.
float	gafZeroWtFNV[ MAX_NUM_PV_LOADCELL];

// 2016-04-29 -WFC- void  lc_zero_init_zero_config( BYTE lc );
void  lc_zero_init_zero_powerup_config( BYTE lc );						// 2016-04-29 -WFC-

/**
 * It initializes zeroing data structure of lc based on ga_xxxx_AZM_IntervalTimeNV[],
 * ga_xxxx_AZM_CBRangeNV[], ga_xxxx_pcentCapZeroBandHiNV[], ga_xxxx_pcentCapZeroBandLoNV[],
 * ga_xxxx_pwupZeroBandHiNV[], ga_xxxx_pwupZeroBandLoNV[] and gab_Stable_Pending_TimeNV[].
 *
 * @param  lc	-- loadcell number
 *
 * @post   updated gaLoadcell[] data structure of this loadcell.
 *
 * @note The loadcell must has a valid viewCB before call this function.
 *
 * History:  Created on 2007/11/09 by Wai Fai Chin
 * 2011-01-06 -WFC- helicopter scale use 1d instead of 1/4d.
 * 2011-11-21 -WFC- 0 AZM_CBRangeNV means 0.75 d;
 * 2015-05-06 -WFC- 0 AZM_CBRangeNV means 0.50 d for OIML mode;
 */

void lc_zero_init( BYTE lc )
{
	LOADCELL_T *pLc;
	float		f1pctCap;				// 1% of capacity.
	float fV;		// 2011-11-21 -WFC-
	if ( lc < MAX_NUM_PV_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		f1pctCap = pLc-> viewCapacity * 0.01;
		switch ( gbScaleStandardModeNV & SCALE_STD_MODE_MASK ) {
			case SCALE_STD_MODE_INDUSTRY :
				// 2011-11-21 -WFC-  v
				// pLc-> zeroT.azmThresholdWt = (float) gab_STD_AZM_CBRangeNV[lc] * pLc-> viewCB.fValue;
				if ( 0 == gab_STD_AZM_CBRangeNV[lc] )
					fV = 0.75f;
				else
					fV = (float) gab_STD_AZM_CBRangeNV[lc];
				pLc-> zeroT.azmThresholdWt = fV * pLc-> viewCB.fValue;
				// 2011-11-21 -WFC-  ^
				pLc-> zeroT.zeroBandHiWt = (float) gab_STD_pcentCapZeroBandHiNV[lc] * f1pctCap;
				pLc-> zeroT.zeroBandLoWt = (float) gab_STD_pcentCapZeroBandLoNV[lc] * f1pctCap * (-1.0);
				pLc-> zeroT.azmIntervalTime = 	gab_STD_AZM_IntervalTimeNV[lc];
				break;
			case SCALE_STD_MODE_NTEP :
				// 2011-11-21 -WFC- v
				// pLc-> zeroT.azmThresholdWt = (float) gab_NTEP_AZM_CBRangeNV[lc] * pLc-> viewCB.fValue;
				if ( 0 == gab_NTEP_AZM_CBRangeNV[lc] )
					fV = 0.75f;
				else
					fV = (float) gab_NTEP_AZM_CBRangeNV[lc];
				pLc-> zeroT.azmThresholdWt = fV * pLc-> viewCB.fValue;
				// 2011-11-21 -WFC-  ^
				pLc-> zeroT.zeroBandHiWt = (float) gab_NTEP_pcentCapZeroBandHiNV[lc] * f1pctCap;
				pLc-> zeroT.zeroBandLoWt = (float) gab_NTEP_pcentCapZeroBandLoNV[lc] * f1pctCap * (-1.0);
				pLc-> zeroT.azmIntervalTime =	gab_NTEP_AZM_IntervalTimeNV[lc];
				break;
			case SCALE_STD_MODE_OIML :
				// 2011-11-21 -WFC- v
				// pLc-> zeroT.azmThresholdWt = (float) gab_OIML_AZM_CBRangeNV[lc] * pLc-> viewCB.fValue;
				if ( 0 == gab_OIML_AZM_CBRangeNV[lc] )
					// fV = 0.75f;
					fV = 0.50f;			// 2015-05-06 -WFC- OIML requirement.
				else
					fV = (float) gab_OIML_AZM_CBRangeNV[lc];
				pLc-> zeroT.azmThresholdWt = fV * pLc-> viewCB.fValue;
				// 2011-11-21 -WFC-  ^
				pLc-> zeroT.zeroBandHiWt = (float) gab_OIML_pcentCapZeroBandHiNV[lc] * f1pctCap;
				pLc-> zeroT.zeroBandLoWt = (float) gab_OIML_pcentCapZeroBandLoNV[lc] * f1pctCap * (-1.0);
				pLc-> zeroT.azmIntervalTime =	gab_OIML_AZM_IntervalTimeNV[lc];
				break;
		}

		if ( (gAppBootShareNV.productID >= CONFIG_AS_DSC) &&
			 (gAppBootShareNV.productID <= CONFIG_AS_HLI))
			pLc-> zeroT.quarterCBWt  = pLc-> viewCB.fValue;				// 2011-01-06 -WFC- helicopter scale use 1d instead of 1/4d.
		else
			pLc-> zeroT.quarterCBWt  = pLc-> viewCB.fValue * 0.25;		// may remove it in the future for trade off of being slow for more ram space.
		timer_mSec_set( &(pLc-> zeroT.azmIntervalTimer), pLc-> zeroT.azmIntervalTime);
		// timer_mSec_set( &(pLc-> zeroT.pendingTimer), pLc-> pendingTime);
	}
} // end lc_zero_init()


/**
 * It performs auto zero maintanence on a specified loadcell.
 *
 * @param  			pLc	-- pointer to loadcell data structure.
 * @param  zeroOffsetWt -- user specified zero offset weight
 * @param			lc	-- loadcell index.
 *
 * @post   updated zeroWt in a loadcell data structure point by pLc.
 *
 * @note caller must ensured that this loadcell is enabled.
 *
 * History:  Created on 2007-11-09 by Wai Fai Chin
 * 2011-11-21 -WFC- Changed logic for AZM to ignore motion.
 * 2012-02-08 -WFC- Rewrote lc_zero_azm() use raw weight instead of net or gross because these values have been rounded with countby.
 *                  It only checks azm if there is no motion. New zero weight generated by AZM will not changed prvZeroWt.
 */

void lc_zero_azm( LOADCELL_T *pLc, float zeroOffsetWt, BYTE lc)
{
	float	curWt;
	float	deltaWt;
	
	if ( SCALE_STD_MODE_AZM & gbScaleStandardModeNV ) {					// if AZM enabled,
		if ( timer_mSec_expired( &(pLc-> zeroT.azmIntervalTimer) ) ) {	// if it is time to check azm, then
			if ( !(LC_STATUS_MOTION & (pLc-> status)) ) {					// if loadcell NOT in motion
				curWt  = pLc-> rawWt;								// note that rawWt is never rounded. It means that zeroWt is never round in zero_Key mode. However, zeroWt is a rounded value because net and gross weight were rounded.
				curWt -= zeroOffsetWt;								// removed the user defined zero offset weight because it was added to it when computing raw weight.
				deltaWt = curWt - *(pLc-> pZeroWt);					// deltaWt is also a gross weight.

				if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) {	// if loadcell is in NET mode
					deltaWt = deltaWt - *(pLc-> pTareWt);			// deltaWt is also a NET weight = gross - tare;
					curWt = deltaWt +  *(pLc-> pZeroWt);			// added this deltaWt with current zero wt to from a new zero wt.
				}

				if ( float_a_gte_b( deltaWt, -(pLc -> zeroT.azmThresholdWt )) &&
					 float_a_lte_b( deltaWt,   pLc -> zeroT.azmThresholdWt ))	{	// if deltaWt weight is within azm band, then
					if ( float_a_gte_b( curWt, pLc -> zeroT.zeroBandLoWt ) &&		// if current weight is within zero band, then
						 float_a_lte_b( curWt, pLc -> zeroT.zeroBandHiWt ))	{
						*(pLc-> pZeroWt) = curWt;					// all condition were met, got a new zero weight as a raw weight minus zero offset weight.
						loadcell_update_overload_threshold( pLc );
						nv_cnfg_fram_save_loadcell_dynamic_data( lc );
					}
				}
			} // end if loadcell NOT in motion
			timer_mSec_set( &(pLc-> zeroT.azmIntervalTimer), pLc-> zeroT.azmIntervalTime);
		} // end if it is time to check azm
	} // end if AZM enabled.
} // end lc_zero_azm()


/* 2012-02-08 -WFC- v
void lc_zero_azm( LOADCELL_T *pLc, float zeroOffsetWt, BYTE lc)
{
	float	curWt;

//	if ( LC_OP_MODE_AZM & *(pLc-> pOpModes) ) {							// if AZM enabled,
	if ( SCALE_STD_MODE_AZM & gbScaleStandardModeNV ) {					// if AZM enabled,
		if ( timer_mSec_expired( &(pLc-> zeroT.azmIntervalTimer) ) ) {	// if it is time to check azm, then
			// 2011-11-21 -WFC- 			if ( !(LC_STATUS_MOTION & (pLc-> status)) ) {					// if loadcell NOT in motion

				if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 				// if loadcell is in NET mode
					curWt = pLc -> netWt;
				else
					curWt = pLc -> grossWt;

				curWt -= zeroOffsetWt;						// removed the user defined zero offset weight because it was added to it when computing raw weight.
				if ( float_a_gte_b( curWt, -(pLc -> zeroT.azmThresholdWt )) &&
					 float_a_lte_b( curWt,   pLc -> zeroT.azmThresholdWt ))	{	// if current weight is within azm band, then
					curWt += *(pLc-> pZeroWt);										// now the current is pure raw weight.
					if ( float_a_gte_b( curWt, pLc -> zeroT.zeroBandLoWt ) &&		// if current weight is within zero band, then
						 float_a_lte_b( curWt, pLc -> zeroT.zeroBandHiWt ))	{
						pLc->prvZeroWt = *(pLc-> pZeroWt) = curWt;					// all condition were met, got a new zero weight
						loadcell_update_overload_threshold( pLc );
						nv_cnfg_fram_save_loadcell_dynamic_data( lc );
					}
				}
			// 2011-11-21 -WFC- 			} // end if loadcell NOT in motion
			timer_mSec_set( &(pLc-> zeroT.azmIntervalTimer), pLc-> zeroT.azmIntervalTime);
		} // end if it is time to check azm
	} // end if AZM enabled.
} // end lc_zero_azm()
2012-02-08 -WFC- ^ */

/**
 * It performs zeroing on a specified loadcell.
 *
 * @param  			pLc	-- pointer to loadcell data structure.
 * @param  zeroOffsetWt -- user specified zero offset weight
 * @param			lc	-- loadcell index.
 *
 * @post   updated zeroWt in a loadcell data structure point by pLc.
 *
 * \ref	lc_zero_check_pending(), lc_zero_setup_zero_powerup() and sensor_compute_all_values().
 * 
 * History:  Created on 2007/11/09 by Wai Fai Chin
 * 2012-02-06 -WFC- Ensure pending time is 3 times greater than filter sampling window. Use LC_STATUS3_GOT_PREV_VALID_VALUE condition to ensure to set the pending flag.
 *
 */

void lc_zero( LOADCELL_T *pLc, float zeroOffsetWt, BYTE lc)
{
	BYTE pendingTime;			// 2012-02-06 -WFC-
	float	curWt;

	if ( (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) == 		// if this loadcell is enabled AND in normal active mode.
		 ( (pLc-> runModes) & (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) ) )	{	
		if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) ||					// if this loadcell has valid value // 2012-02-06 -WFC-
			( LC_STATUS3_GOT_PREV_VALID_VALUE & (pLc-> status3)) ) {		// or got a prv valid value. // 2012-02-06 -WFC-
			if ( LC_STATUS_MOTION & (pLc-> status) ) {					// if loadcell is in motion
				if ( !( LC_RUN_MODE_PENDING_ZERO & (pLc-> runModes )) ) {	// if no pending zero
					pLc-> runModes |= LC_RUN_MODE_PENDING_ZERO;			// flag pending zero.
					timer_mSec_set( &(pLc-> zeroT.pendingTimer), pLc-> pendingTime);
					// 2012-02-06 -WFC- v
					// timer_mSec_set( &(pLc-> zeroT.pendingTimer), pLc-> pendingTime);
					pendingTime = pLc-> pendingTime;
					if ( (gaLSensorDescriptor[lc].cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK ) ) {
						pendingTime = gaLSensorDescriptor[lc].filterTimer.interval<<1 + gaLSensorDescriptor[lc].filterTimer.interval + 10; 		// 3x filter sampling time plus 0.5 seconds.
						if ( pendingTime < pLc-> pendingTime ) 		// if new pendingTime < user defined pending time, then use user defined time.
							pendingTime = pLc-> pendingTime;
					}
					timer_mSec_set( &(pLc-> zeroT.pendingTimer), pendingTime);
					// 2012-02-06 -WFC- ^
				}
			}
			else { //  loadcell is NOT in motion
				curWt  = pLc-> rawWt;								// note that rawWt is never rounded. It means that zeroWt is never round in zero_Key mode. However, zeroWt is a rounded value because net and gross weight were rounded.
				curWt -= zeroOffsetWt;								// removed the user defined zero offset weight because it was added to it when computing raw weight.
				if ( ( curWt > pLc -> zeroT.zeroBandLoWt ) &&		// if current weight is within zero band, then
					( curWt < pLc -> zeroT.zeroBandHiWt ) )	{
					pLc->prvZeroWt = *(pLc-> pZeroWt);
					*(pLc-> pZeroWt)  = curWt;						// all condition were met, assinged current pure raw weight to a new zero weight.
					pLc-> runModes &= ~LC_RUN_MODE_PENDING_ZERO;	// clear pending zero action.
					loadcell_update_overload_threshold( pLc );
					pLc-> totalT.status |= LC_TOTAL_STATUS_SKIP_TOTAL;	// flag skip total to prevent bogus totaling.
					//pLc-> totalT.status |= LC_TOTAL_STATUS_NOT_ALLOW;	// flag NOT ALLOW TOTAL to prevent bogus totaling.
					//pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;			// flag NOT ok to total to prevent bogus totaling.
					nv_cnfg_fram_save_loadcell_dynamic_data( lc );
				}
			} // end if loadcell NOT in motion
		} // end if got valid weight
	} // end if this loadcell is enabled.
} // end lc_zero()

/**
 * It performs zeroing on a specified loadcell lc.
 * This function is designed to hide the complexity of zeroing loadcell.
 * It is mainly use by command or user interface modules.
 *
 * @param	lc	-- loadcell number
 *
 * @post   updated zeroWt in a loadcell data structure point by pLc.
 *
 * \ref	lc_zero(), lc_zero_check_pending() and sensor_compute_all_values().
 * 
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

void	lc_zero_by_command( BYTE lc )
{
	LOADCELL_T *pLc;

	if ( lc < MAX_NUM_LOADCELL ) {						// make sure it is physical loadcell.
		pLc = &gaLoadcell[ lc ];
		lc_zero( pLc, gafLcZeroOffsetWtNV[ lc ], lc );
	}
	else if ( SENSOR_NUM_MATH_LOADCELL == lc ) {
		vs_math_zero_input_sensors( lc );				// Not allow to zero math type loadcell, instead, it zeros all it is input sensors.
	}
}// end	lc_zero_by_command()

/**
 * It undoes recent zeroing of a specified loadcell.
 *
 * @param	lc	-- loadcell number
 *
 * @post   updated zeroWt in a loadcell data structure point by pLc.
 *
 * \ref	lc_zero(), lc_zero_check_pending() and sensor_compute_all_values().
 *
 * History:  Created on 2010/06/27 by Wai Fai Chin
 */

void	lc_zero_undo_by_command( BYTE lc )
{
	if ( lc < MAX_NUM_LOADCELL ) {						// make sure it is physical loadcell.
		lc_zero_undo( lc );
	}
	else if ( SENSOR_NUM_MATH_LOADCELL == lc ) {
		vs_math_undo_zero_input_sensors( lc );			// Not allow to undo zero math type loadcell, instead, it undoes zeroing all it is input sensors.
	}
}// end	lc_zero_undo_by_command()


/**
 * It undoes recent zeroing of a specified loadcell.
 *
 * @param	lc	-- loadcell number
 *
 * @post   updated zeroWt in a loadcell data structure point by pLc.
 *
 * \ref	lc_zero(), lc_zero_check_pending() and sensor_compute_all_values().
 *
 * History:  Created on 2010/06/27 by Wai Fai Chin
 */

void	lc_zero_undo( BYTE lc )
{
	LOADCELL_T *pLc;

	if ( lc < MAX_NUM_LOADCELL ) {						// make sure it is physical loadcell.
		pLc = &gaLoadcell[ lc ];
		if ( (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) == 		// if this loadcell is enabled AND in normal active mode.
			 ( (pLc-> runModes) & (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) ) )	{
			*(pLc-> pZeroWt)= pLc->prvZeroWt;
			pLc-> runModes &= ~LC_RUN_MODE_PENDING_ZERO;	// clear pending zero action.
			loadcell_update_overload_threshold( pLc );
			pLc-> totalT.status |= LC_TOTAL_STATUS_SKIP_TOTAL;	// flag skip total to prevent bogus totaling.
			nv_cnfg_fram_save_loadcell_dynamic_data( lc );
		}
	}
}// end	lc_zero_undo()



/**
 * It checks to see if the loadcell is center of zero.
 *
 * @param  pLc	-- pointer to loadcell data structure.
 * @param  lc	-- loadcell number.		// 2012-02-08 -WFC-
 *
 * @post   updated status in a loadcell data structure point by pLc.
 *
 * @note caller must ensured that this loadcell is enabled.
 *
 * History:  Created on 2007/11/12 by Wai Fai Chin
 * 2011-07-21 -WFC- widen COZ threshold countby 4 cb if this loadcell is in Peakhold mode.
 * 2012-02-08 -WFC- Rewrote lc_zero_coz(), added lc input parameter and use raw weight instead of net or gross because these values have been rounded with countby.
 */

// 2012-02-08 -WFC- v Rewrote widen COZ threshold if this loadcell is in Peakhold mode.
void lc_zero_coz( LOADCELL_T *pLc, BYTE lc )
{
	float	curWt;
	float	cozThresholdCB;

	curWt  = pLc-> rawWt;										// note that rawWt is never rounded. It means that zeroWt is never round in zero_Key mode. However, zeroWt is a rounded value because net and gross weight were rounded.
	curWt -=  gafLcZeroOffsetWtNV[ lc ];						// removed the user defined zero offset weight because it was added to it when computing raw weight.
	curWt = curWt - *(pLc-> pZeroWt);							// curWt = GROSS weight
	if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 			// if loadcell is in NET mode
		curWt = curWt - *(pLc-> pTareWt);						// curWt = NET weight

	if ( float_lt_zero(curWt) )
		curWt = -curWt;

	if ( pLc-> runModes &  LC_RUN_MODE_PEAK_HOLD_ENABLED )  	// if it is in peak hold mode.
		cozThresholdCB = pLc->viewCB.fValue * 4;
	else
		cozThresholdCB =  pLc -> zeroT.quarterCBWt;

	if ( (!(LC_STATUS_MOTION & ( pLc-> status )) )	&& 			// if NOT in motion
		  float_a_lt_b( curWt,  cozThresholdCB ) )	{			// if current weight is less than coz threshold countby, then
		pLc-> status |= LC_STATUS_COZ;							// the loadcell is COZ.
	}
	else {
		pLc-> status &= ~LC_STATUS_COZ;							// else clear COZ status.
	}
} // end lc_zero_coz()
// 2012-02-08 -WFC- ^


/* 2012-02-08 -WFC- v
// 2011-07-21 -WFC- v Rewrote widen COZ threshold if this loadcell is in Peakhold mode.
void lc_zero_coz( LOADCELL_T *pLc )
{
	float	curWt;
	float	cozThresholdCB;
	
	if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 				// if loadcell is in NET mode
		curWt = pLc -> netWt;
	else 
		curWt = pLc -> grossWt;
		
	if ( float_lt_zero(curWt) )
		curWt = -curWt;

	if ( pLc-> runModes &  LC_RUN_MODE_PEAK_HOLD_ENABLED )  			// if it is in peak hold mode.
		cozThresholdCB = pLc->viewCB.fValue * 4;
	else
		cozThresholdCB =  pLc -> zeroT.quarterCBWt;

	if ( (!(LC_STATUS_MOTION & ( pLc-> status )) )	&& 			// if NOT in motion
		  float_a_lt_b( curWt,  cozThresholdCB ) )	{			// if current weight is less than coz threshold countby, then
		pLc-> status |= LC_STATUS_COZ;							// the loadcell is COZ.
	}
	else {
		pLc-> status &= ~LC_STATUS_COZ;							// else clear COZ status.
	}
} // end lc_zero_coz()
// 2011-07-21 -WFC- ^
*/
/* 2011-07-21 -WFC-
void lc_zero_coz( LOADCELL_T *pLc )
{
	float	curWt;

	if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) 				// if loadcell is in NET mode
		curWt = pLc -> netWt;
	else
		curWt = pLc -> grossWt;

	if ( float_lt_zero(curWt) )
		curWt = -curWt;

	if ( (!(LC_STATUS_MOTION & (pLc-> status)) )	&& 			// if NOT in motion
		  float_a_lt_b(curWt,  pLc -> zeroT.quarterCBWt) )	{	// if current weight is less than 1/4 of countby, then
		pLc-> status |= LC_STATUS_COZ;							// the loadcell is COZ.
	}
	else {
		pLc-> status &= ~LC_STATUS_COZ;						// else clear COZ status.
	}
} // end lc_zero_coz()
 */

/**
 * It checks pending zero request and act on it.
 * It also handle zero on power up.
 *
 * @param  pLc	-- pointer to loadcell data structure.
 * @param	lc	-- loadcell number
 *
 * @post   updated zeroWt in a loadcell data structure point by pLc.
 *
 * \ref	lc_zero_setup_zero_powerup(), lc_zero() and sensor_compute_all_values().
 * 
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

void	lc_zero_check_pending( LOADCELL_T *pLc, BYTE lc )
{
	if ( LC_RUN_MODE_PENDING_ZERO & (pLc-> runModes)) {				// if it has a pending zero request
		if ( timer_mSec_expired( &(pLc-> zeroT.pendingTimer) ) ) {	// if zero pending time expired
			pLc-> runModes &= ~LC_RUN_MODE_PENDING_ZERO;				// clear pending zero action.
		}
		else {
			lc_zero( pLc, gafLcZeroOffsetWtNV[ lc ], lc );
		}
			
		if ( !( LC_RUN_MODE_PENDING_ZERO & (pLc-> runModes)) ) {		// if it has no more pending zero request, it means either time had expired or successed zeroing a loadcell.
			if ( LC_RUN_MODE_ZERO_ON_POWERUP & (pLc-> runModes ) )	{	// if this was zero on power up event
				pLc-> runModes &= ~LC_RUN_MODE_ZERO_ON_POWERUP;		// clear zero on power up request. This is one time event only happed during powerup.
				lc_zero_init( lc );										// set the normal zero band threshold instead of powerup zero band threshold.
			}
		}
	}
}// end	lc_zero_check_pending(,)


/**
 * It setups all the require variables for zero on power up.
 * Sensor module will call lc_zero_check_pending();
 *
 * @param	lc	-- loadcell number
 *
 * @post   updated zeroWt in a loadcell data structure point by pLc.
 *
 * \ref	lc_zero_check_pending(), lc_zero() and sensor_compute_all_values().
 *
 * History:  Created on 2007/11/15 by Wai Fai Chin
 * 2015-08-03 -WFC- init maxNegativeDisplayableValue to -20d for OIML mode.
 * 2016-03-21 -WFC- called lc_zero_init_zero_powerup_config().
 * 2016-04-29 -WFC- call different init zero method based on zero on powerup enabled status.
 */
void	lc_zero_setup_zero_powerup( BYTE lc )
{
	LOADCELL_T	*pLc;
	
	if ( lc < MAX_NUM_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		// 2016-04-29 -WFC- lc_zero_init_zero_powerup_config( lc );
		if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
			if ( SCALE_STD_MODE_ZERO_POWERUP & gbScaleStandardModeNV ) {	// if zero on powerup enabled,
				lc_zero_init_zero_powerup_config( lc );
				pLc-> runModes |= (LC_RUN_MODE_ZERO_ON_POWERUP | LC_RUN_MODE_PENDING_ZERO);	// tell sensor module that this loadcell needs to zero.
				timer_mSec_set( &(pLc-> zeroT.pendingTimer), LC_ZERO_POWERUP_PENDING_TIME);	// after first power up, there is ds window to try zero the loadcell.
			} // end if zero on powerup enabled.
			else												// 2016-04-29 -WFC-
				lc_zero_init( lc );								// 2016-04-29 -WFC-  set the normal zero band threshold instead of powerup zero band threshold.
		} // end if this loadcell is enabled.
	}
} // end lc_zero_setup_zero_powerup()


//void	lc_zero_setup_zero_powerup( BYTE lc )
//{
//	LOADCELL_T	*pLc;
//	float		f1pctCap;				// 1% of capacity.
//
//	if ( lc < MAX_NUM_LOADCELL ) {
//		pLc = &gaLoadcell[ lc ];
//		if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {			// if this loadcell is enabled.
//			f1pctCap = pLc-> viewCapacity * 0.01;
//			// if ( LC_OP_MODE_ZERO_POWERUP & *( pLc-> pOpModes) ) {		// if zero on powerup enabled,
//			if ( SCALE_STD_MODE_ZERO_POWERUP & gbScaleStandardModeNV ) {	// if zero on powerup enabled,
//				switch ( gbScaleStandardModeNV & SCALE_STD_MODE_MASK ) {
//					case SCALE_STD_MODE_INDUSTRY :
//						pLc-> zeroT.zeroBandHiWt = (float) gab_STD_pwupZeroBandHiNV[lc] * f1pctCap;
//						pLc-> zeroT.zeroBandLoWt = (float) gab_STD_pwupZeroBandLoNV[lc] * f1pctCap * (-1.0);
//						break;
//					case SCALE_STD_MODE_NTEP :
//						pLc-> zeroT.zeroBandHiWt = (float) gab_NTEP_pwupZeroBandHiNV[lc] * f1pctCap;
//						pLc-> zeroT.zeroBandLoWt = (float) gab_NTEP_pwupZeroBandLoNV[lc] * f1pctCap * (-1.0);
//						break;
//					case SCALE_STD_MODE_OIML :
//						pLc-> zeroT.zeroBandHiWt = (float) gab_OIML_pwupZeroBandHiNV[lc] * f1pctCap;
//						pLc-> zeroT.zeroBandLoWt = (float) gab_OIML_pwupZeroBandLoNV[lc] * f1pctCap * (-1.0);
//						pLc-> maxNegativeDisplayableValue = -20.0f * pLc->viewCB.fValue;	// 2015-08-03 -WFC-
//						break;
//				}
//				pLc-> runModes |= (LC_RUN_MODE_ZERO_ON_POWERUP | LC_RUN_MODE_PENDING_ZERO);	// tell sensor module that this loadcell needs to zero.
//				timer_mSec_set( &(pLc-> zeroT.pendingTimer), LC_ZERO_POWERUP_PENDING_TIME);	// after first power up, there is ds window to try zero the loadcell.
//			} // end if zero on powerup enabled.
//		} // end if this loadcell is enabled.
//	}
//} // end lc_zero_setup_zero_powerup()


/**
 * It initializes zero configuration.
 *
 * @param	lc	-- loadcell number
 *
 * @post   updated zeroWt in a loadcell data structure point by pLc.
 *
 * History:  Created on 2016-03-21 by Wai Fai Chin
 * 2016-04-29 -WFC- renamed lc_zero_init_zero_config() to  lc_zero_init_zero_powerup_config().
 */

void  lc_zero_init_zero_powerup_config( BYTE lc )
{
	LOADCELL_T	*pLc;
	float		f1pctCap;				// 1% of capacity.

	if ( lc < MAX_NUM_LOADCELL ) {
		pLc = &gaLoadcell[ lc ];
		f1pctCap = pLc-> viewCapacity * 0.01;
		switch ( gbScaleStandardModeNV & SCALE_STD_MODE_MASK ) {
			case SCALE_STD_MODE_INDUSTRY :
				pLc-> zeroT.zeroBandHiWt = (float) gab_STD_pwupZeroBandHiNV[lc] * f1pctCap;
				pLc-> zeroT.zeroBandLoWt = (float) gab_STD_pwupZeroBandLoNV[lc] * f1pctCap * (-1.0);
				break;
			case SCALE_STD_MODE_NTEP :
				pLc-> zeroT.zeroBandHiWt = (float) gab_NTEP_pwupZeroBandHiNV[lc] * f1pctCap;
				pLc-> zeroT.zeroBandLoWt = (float) gab_NTEP_pwupZeroBandLoNV[lc] * f1pctCap * (-1.0);
				break;
			case SCALE_STD_MODE_OIML :
				pLc-> zeroT.zeroBandHiWt = (float) gab_OIML_pwupZeroBandHiNV[lc] * f1pctCap;
				pLc-> zeroT.zeroBandLoWt = (float) gab_OIML_pwupZeroBandLoNV[lc] * f1pctCap * (-1.0);
				pLc-> maxNegativeDisplayableValue = -20.0f * pLc->viewCB.fValue;	// 2015-08-03 -WFC-
				break;
		}
	}
} // end lc_zero_init_zero_powerup_config()

/**
 * It initializes zero configuration for all local physical loadcell sensors.
 *
 * History:  Created on 2016-03-21 by Wai Fai Chin
 * 2016-04-29 -WFC- set the normal zero band threshold instead of powerup zero band threshold.
 */

void lc_zero_init_all_lc_zero_config( void )
{
	BYTE lc;

	for ( lc = 0; lc < MAX_NUM_LOADCELL; lc++ ) {
		// 2016-04-29 -WFC-lc_zero_init_zero_powerup_config( lc );					//
		lc_zero_init( lc );					// 2016-04-29 -WFC-  set the normal zero band threshold instead of powerup zero band threshold.
	}
} // end lc_zero_init_all_lc_zero_config()
