/*! \file lc_tare.c \brief loadcell tare related functions.*/
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
//  History:  Created on 2007/11/19 by Wai Fai Chin
// 
//
// ****************************************************************************


#include  "commonlib.h"
#include  "loadcell.h"
#include  "lc_tare.h"
#include  "lc_total.h"

/// Values need to save to Nonvolatile Ferri Memory everytime it has changed.
float	gafTareWtFNV[ MAX_NUM_PV_LOADCELL];

/**
 * It change current loadcell to net mode if tare weight not = 0.0.
 *
 * @param  	pLc	-- pointer to loadcell data structure.
 *
 * @post   update opModes of this loadcell.
 *
 * History:  Created on 2007/11/19 by Wai Fai Chin
 */

void lc_tare_change_to_net( LOADCELL_T *pLc )
{
	if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {		// if this loadcell is enabled.
		if ( !(float_eq_zero( *(pLc-> pTareWt))))	{		// if tareWt != 0.0, then
			*(pLc-> pOpModes)	|= LC_OP_MODE_NET_GROSS;	// 		set it in NET mode.
			// should also write to ferro ram too.
		}
	} // end if this loadcell is enabled.
} // end lc_tare_change_to_net()


/**
 * It toggles loadcell mode between NET and GROSS.
 *
 * @param  	pLc	-- pointer to loadcell data structure.
 * @param	lc	-- loadcell index.
 *
 * @post   update opModes of this loadcell.
 *
 * History:  Created on 2007/11/19 by Wai Fai Chin
 */

void lc_tare_toggle_net_gross( LOADCELL_T *pLc, BYTE lc )
{
	if ( LC_RUN_MODE_ENABLED & (pLc-> runModes)) {		// if this loadcell is enabled.
		if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) {	// if it is in NET mode,
			lc_tare_change_to_gross( pLc );					// 		set it in GROSS mode.
		}
		else	{											// else 
			lc_tare_change_to_net( pLc );					//		set it in NET mode.	
		}
		pLc-> totalT.status |= LC_TOTAL_STATUS_SKIP_TOTAL;	// flag skip total to prevent bogus totaling.
		//pLc-> totalT.status |= LC_TOTAL_STATUS_NOT_ALLOW;	// flag NOT ALLOW TOTAL to prevent bogus totaling.
		//pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;			// flag NOT ok to total to prevent bogus totaling.
		nv_cnfg_fram_save_loadcell_dynamic_data( lc );
	} // end if this loadcell is enabled.
} // end lc_tare_toggle_net_gross()



/**
 * It tares current gross weight.
 *
 * @param  	pLc	-- pointer to loadcell data structure.
 * @param	lc	-- loadcell index.
 *
 * @return	true if it successed.
 * @post   update opModes of this loadcell.
 *
 * History:  Created on 2007/11/21 by Wai Fai Chin
 * 2011-10-25 -WFC- clear pending tare flag.
 * 2012-02-06 -WFC- Ensure pending time is 3 times greater than filter sampling window. Use LC_STATUS3_GOT_PREV_VALID_VALUE condition to ensure to set the pending flag.
 */

BYTE lc_tare_gross( LOADCELL_T *pLc, BYTE lc )
{
	BYTE status;
	BYTE pendingTime;			// 2012-02-06 -WFC-
	status = FALSE;

	if ( (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) == 		// if this loadcell is enabled AND in normal active mode.
		 ( (pLc-> runModes) & (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) ) )	{	
		// 2012-02-06 -WFC- if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status)) {				// if this loadcell has valid value
		if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) ||					// if this loadcell has valid value // 2012-02-06 -WFC-
			( LC_STATUS3_GOT_PREV_VALID_VALUE & (pLc-> status3)) ) {		// or got a prv valid value. // 2012-02-06 -WFC-
			if ( LC_STATUS_MOTION & (pLc-> status) ) {					// if loadcell is in motion
				if ( !( LC_RUN_MODE_PENDING_TARE & (pLc-> runModes )) ) {	// if no pending tare
					pLc-> runModes |= LC_RUN_MODE_PENDING_TARE;			// flag pending tare.
					// 2012-02-06 -WFC- v
					// timer_mSec_set( &(pLc-> tarePendingTimer), pLc-> pendingTime);
					pendingTime = pLc-> pendingTime;
					if ( (gaLSensorDescriptor[lc].cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK ) ) {
						pendingTime = gaLSensorDescriptor[lc].filterTimer.interval<<1 + gaLSensorDescriptor[lc].filterTimer.interval + 10; 		// 3x filter sampling time plus 0.5 seconds.
						if ( pendingTime < pLc-> pendingTime ) 		// if new pendingTime < user defined pending time, then use user defined time.
							pendingTime = pLc-> pendingTime;
					}
					timer_mSec_set( &(pLc-> tarePendingTimer), pendingTime);
					// 2012-02-06 -WFC- ^
				}
			} //
			else { // loadcell is stable
				status = lc_tare_set( pLc, pLc-> grossWt, lc );
				pLc-> runModes &= ~LC_RUN_MODE_PENDING_TARE;			// 2011-10-25 -WFC- clear pending tare action.
			}
		}
	} // end if this loadcell is enabled.
	return status;
} // end lc_tare_gross(,)


/**
 * It toggles loadcell mode between NET and GROSS.
 *
 * @param		pLc	-- pointer to loadcell data structure.
 * @param	fValue	-- value to be use as tare weight.
 * @param		lc	-- loadcell number.
 *
 * @return	true if it successed.
 * @post   update opModes of this loadcell.
 *
 * History:  Created on 2007-11-21 by Wai Fai Chin
 * 2011-05-02 -WFC-  compare overload threshold instead of viewCapacity for permission to tare.
 */

BYTE lc_tare_set( LOADCELL_T *pLc, float fValue, BYTE lc )
{
	BYTE status;
	status = FALSE;
	if ( (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) == 		// if this loadcell is enabled AND in normal active mode.
		 ( (pLc-> runModes) & (LC_RUN_MODE_ENABLED | LC_RUN_MODE_NORMAL_ACTIVE) ) )	{	
		// 2011-05-02 -WFC- if (float_gte_zero( fValue ) &&	float_a_lt_b( fValue, pLc-> viewCapacity) ) {
		if (float_gte_zero( fValue ) &&
			float_a_lt_b( fValue, pLc-> overloadThresholdWt ) ) {	// 2011-05-02 -WFC-
			fValue = float_round( fValue, pLc-> viewCB.fValue);
			*(pLc-> pTareWt) = fValue;
			status = TRUE;
			if ( float_eq_zero( fValue ) )							// if tare weight is 0.0, then
				lc_tare_change_to_gross( pLc );
			else {
				*(pLc-> pOpModes) |= LC_OP_MODE_NET_GROSS;			// set it in NET mode.	
				// should also write to ferro ram too.
			}
			pLc-> totalT.status |= LC_TOTAL_STATUS_SKIP_TOTAL;	// flag skip total to prevent bogus totaling.
			//pLc-> totalT.status |= LC_TOTAL_STATUS_NOT_ALLOW;	// flag NOT ALLOW TOTAL to prevent bogus totaling.
			//pLc-> status &= ~LC_STATUS_OK_TO_TOTAL;			// flag NOT ok to total to prevent bogus totaling.
			nv_cnfg_fram_save_loadcell_dynamic_data( lc );
		}
	} // end if this loadcell is enabled.
	return status;
} // end lc_tare_set(,,)


/**
 * It checks pending tare request and act on it.
 *
 * @param  pLc	-- pointer to loadcell data structure.
 * @param		lc	-- loadcell index.
 *
 * @post   updated *pTareWt in a loadcell data structure point by pLc.
 *
 * 
 * History:  Created on 2007/11/26 by Wai Fai Chin
 */

void	lc_tare_check_pending( LOADCELL_T *pLc, BYTE lc )
{
	if ( LC_RUN_MODE_PENDING_TARE & (pLc-> runModes)) {				// if it has a pending tare request
		if ( timer_mSec_expired( &(pLc-> tarePendingTimer) ) ) {		// if tare pending time had expired
			pLc-> runModes &= ~LC_RUN_MODE_PENDING_TARE;				// clear pending tare action.
		}
		else {
			lc_tare_gross( pLc, lc );	// need to update overload limit???? No.
		}
	}
}// end	lc_tare_check_pending(,)

  
