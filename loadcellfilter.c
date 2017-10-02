/*! \file loadcellfilter.c \brief loadcell filter algorithm.*/
// ****************************************************************************
//
//                      MSI CellScale
//
//                  Copyright (c) 2007, 2010 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: independent
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2007-10-11 by Wai Fai Chin
//  2010-09-21 -WFC- modified for Linear Tech ADC chip.
//  2011-05-06 -WFC- No window size changed from motion to flat state.
//				 Mininum motion or flat state window size > 1 to filter out mechanical noise.
//				Increased kStepThreshold_slope_nnn threshold.
//
//		Loadcell filter is based on raw data pattern states of the loadcell consist of
//  flat, new step and motion states. It performs a triangle filter with
//  dynamic window size and threshold based on the pattern states.
//  threshold
//
// ****************************************************************************


#include <stdlib.h>
#include "commonlib.h"
#include "loadcellfilter.h"

#if ( CONFIG_COMPILER == CONFIG_USE_PC_GCC )
	#define  MAX_NUM_LOADCELL	4
	INT32		gDeltaRefRunAvg;
#else
	#include "loadcell.h"
#endif


/// simple running average filter0
RUNNING_AVG_FILTER32_T gLcFilter0[ MAX_NUM_LOADCELL ];

/// simple running average filter1 cascade with filter0 to form an triangle filter. This is displayable value or final value of filtered data depends on filterEvent.
RUNNING_AVG_FILTER32_T gLcFilter1[ MAX_NUM_LOADCELL ];

/// simple running average filter of raw data as a reference filter for generated absDAccDiff.
RUNNING_AVG_FILTER32_T gLcRefFilter[ MAX_NUM_LOADCELL ];

/// simple running average filter of absDAccDiff of gLcRefFilter.
RUNNING_AVG_FILTER32_T gABSdAccDiffFilter[ MAX_NUM_LOADCELL ];

/// This data structure is for manage the filter state machine.
LC_FILTER_MANAGER_T gFilterManager[ MAX_NUM_LOADCELL ];

/// It uses as the concrete buffer for gFilterManager.absDAccDiffBuf circular buffer object as sliding window of absDAccDiff;
INT32 gABSdAccDiffBuf[ MAX_NUM_LOADCELL ][ MAX_ABS_D_ACCDIFF_BUF_SIZE ];

// BYTE gFilterCnfg;

/* 2011-05-06 -WFC-
// 2010-09-21 -WFC-
#define		NEW_STEP_EXPIRED_CNT1_HI	3
#define		NEW_STEP_EXPIRED_CNT2_HI	4
#define		NEW_STEP_EXPIRED_CNT3_HI	5
#define		NEW_STEP_EXPIRED_CNT4_HI	6
#define		NEW_STEP_EXPIRED_CNT5_HI	7
#define		NEW_STEP_EXPIRED_CNT6_HI	8

#define		NEW_STEP_EXPIRED_CNT1_LO	2
#define		NEW_STEP_EXPIRED_CNT2_LO	3
#define		NEW_STEP_EXPIRED_CNT3_LO	4
#define		NEW_STEP_EXPIRED_CNT4_LO	5
#define		NEW_STEP_EXPIRED_CNT5_LO	6
#define		NEW_STEP_EXPIRED_CNT6_LO	7

#define		FLAT_LINE_EXPIRED_CNT1_HI	3		// allow it has 0.2 second to filter out small ripples. 20 for crane, 4 for small plater
#define		FLAT_LINE_EXPIRED_CNT2_HI	4
#define		FLAT_LINE_EXPIRED_CNT3_HI	5
#define		FLAT_LINE_EXPIRED_CNT4_HI	6
#define		FLAT_LINE_EXPIRED_CNT5_HI	7
#define		FLAT_LINE_EXPIRED_CNT6_HI	8

#define		FLAT_LINE_EXPIRED_CNT1_LO	2		// allow it has 0.1 seconds to filter out small ripples.
#define		FLAT_LINE_EXPIRED_CNT2_LO	3
#define		FLAT_LINE_EXPIRED_CNT3_LO	4
#define		FLAT_LINE_EXPIRED_CNT4_LO	5
#define		FLAT_LINE_EXPIRED_CNT5_LO	6
#define		FLAT_LINE_EXPIRED_CNT6_LO	7		// 15 ticks about 0.75 seconds

#define		FILTER_STATE_FLAT		0
//#define	FILTER_STATE_VFLAT		1
#define		FILTER_STATE_NEW_STEP	2
#define		FILTER_STATE_MIN_STEP	3
#define		FILTER_STATE_MOTION		4

#define		FILTER_LO_MAX_WINDOW_SIZE		2
#define		FILTER_HI_MAX_WINDOW_SIZE		3
#define		FILTER_VHI_MAX_WINDOW_SIZE		4


#define		LC_FILTER_REF_FLAT_CNT_THRESHOLD	4		// 2010-10-29 -WFC-
*/

// 2011-05-06 -WFC- v
#define		NEW_STEP_EXPIRED_CNT1_HI	3
#define		NEW_STEP_EXPIRED_CNT2_HI	4
#define		NEW_STEP_EXPIRED_CNT3_HI	5
#define		NEW_STEP_EXPIRED_CNT4_HI	6
#define		NEW_STEP_EXPIRED_CNT5_HI	7
#define		NEW_STEP_EXPIRED_CNT6_HI	8

#define		NEW_STEP_EXPIRED_CNT1_LO	2
#define		NEW_STEP_EXPIRED_CNT2_LO	3
#define		NEW_STEP_EXPIRED_CNT3_LO	4
#define		NEW_STEP_EXPIRED_CNT4_LO	5
#define		NEW_STEP_EXPIRED_CNT5_LO	6
#define		NEW_STEP_EXPIRED_CNT6_LO	7

#define		FLAT_LINE_EXPIRED_CNT1_HI	3		// allow it has 0.2 second to filter out small ripples. 20 for crane, 4 for small plater
#define		FLAT_LINE_EXPIRED_CNT2_HI	4
#define		FLAT_LINE_EXPIRED_CNT3_HI	5
#define		FLAT_LINE_EXPIRED_CNT4_HI	6
#define		FLAT_LINE_EXPIRED_CNT5_HI	7
#define		FLAT_LINE_EXPIRED_CNT6_HI	8

#define		FLAT_LINE_EXPIRED_CNT1_LO	2		// allow it has 0.1 seconds to filter out small ripples.
#define		FLAT_LINE_EXPIRED_CNT2_LO	3
#define		FLAT_LINE_EXPIRED_CNT3_LO	4
#define		FLAT_LINE_EXPIRED_CNT4_LO	5
#define		FLAT_LINE_EXPIRED_CNT5_LO	6
#define		FLAT_LINE_EXPIRED_CNT6_LO	7		// 15 ticks about 0.75 seconds

#define		FILTER_STATE_FLAT		0
//#define	FILTER_STATE_VFLAT		1
#define		FILTER_STATE_NEW_STEP	2
#define		FILTER_STATE_MIN_STEP	3
#define		FILTER_STATE_MOTION		4

#define		FILTER_LO_MAX_WINDOW_SIZE		3
// 2012-02-03 -WFC- #define		FILTER_HI_MAX_WINDOW_SIZE		5
#define		FILTER_HI_MAX_WINDOW_SIZE		4					// 2012-02-03 -WFC-
#define		FILTER_VHI_MAX_WINDOW_SIZE		5

// 2011-05-06 -WFC- V
#define		FILTER_LO_MIN_WINDOW_SIZE		2
#define		FILTER_HI_MIN_WINDOW_SIZE		3
#define		FILTER_VHI_MIN_WINDOW_SIZE		4
#define		FILTER_NEW_STEP_WINDOW_SIZE		1
// 2011-05-06 -WFC- ^
// 2011-05-06 -WFC- ^


#define		LC_FILTER_REF_FLAT_CNT_THRESHOLD	4		// 2010-10-29 -WFC-



// private function of this module.
void lc_init_triangle_filter( BYTE lc, INT32 sample );


/**
 * It performs triangle filter on sampled data with window size 4 == 2^2. It will stepwise change if accumulated
 * difference > 100.
 *
 * @param			lc  -- load cell index
 * @param	filterCnfg	-- filter configuration such as disabled, level 1, 2, 3 etc... never use in this function.
 * @param		sample  -- sample value such as raw adc counts.
 * @param	pFilteredData -- pointer to an output filtered data.
 *
 * @return LC_FILTER_EVENT_DISPLAYABLE always, filtered value is useable for display to user.
 *
 * History:  Created on 2007/10/15 by Wai Fai Chin
 * 2010-09-21 -WFC- modified for Linear Tech ADC chip.
 *
 */

BYTE lc_stepwise_triangle_filter( BYTE lc, BYTE filterCnfg, INT32 sample, INT32 *pFilteredData )
{
	gLcFilter0[lc].prvRunAvg = running_avg32_step_threshold( sample, gLcFilter0[lc].prvRunAvg, 100, &gLcFilter0[lc].accDiff, 2);
	gLcFilter1[lc].prvRunAvg = running_avg32_step_threshold( gLcFilter0[lc].prvRunAvg, gLcFilter1[lc].prvRunAvg, 100, &gLcFilter1[lc].accDiff, 2);
	*pFilteredData = gLcFilter1[lc].prvRunAvg;		// cascaded two running average filters to form a triangle filtered value.
	return LC_FILTER_EVENT_DISPLAYABLE;
} // end lc_triangle_filter(,,,)


/**
 * It performs pattern based triangle filter on loadcell ADC data.
 *
 * @param			lc  -- load cell index
 * @param	filterCnfg	-- filter configuration such as disabled, level 1, 2, 3 etc..
 * @param		sample  -- sample value such as raw adc counts.
 * @param	pFilteredData -- pointer to an output filtered data.
 *
 * @return filterEvent, LC_FILTER_EVENT_DISPLAYABLE if the filtered value is useable for display to user.
 *
 * @post 
 *    house keeping for gFilterManager
 *
 * History:  Created on 2007-04-03 by Wai Fai Chin
 * 2010-09-21 -WFC- modified for Linear Tech ADC chip.
 * 2010-10-29 -WFC- modified for faster settling time on flat state.
 *
 */

BYTE lc_filter( BYTE lc, BYTE filterCnfg, INT32 sample, INT32 *pFilteredData )
{
	BYTE		filterEvent;
	BYTE		nextFilterState;
	BYTE		wSize[3];	
	LC_FILTER_MANAGER_T		*pFilterManager;
	RUNNING_AVG_FILTER32_T	*pFilter, *pABSdAccDiffFilter;
	UINT16		reqNumSample;
	INT32		oldAccDiff;
	INT32		i32Tmp;			// 2010-10-29 -WFC-

	pFilterManager = &gFilterManager[lc];
	pABSdAccDiffFilter = &gABSdAccDiffFilter[lc];
	pFilter = &gLcRefFilter[lc];

    filterEvent = LC_FILTER_EVENT_RUN;

		oldAccDiff = gLcRefFilter[lc].accDiff;
		if ( pFilterManager->isVeryFirstTime ) { // if this is the very first time after init
			lc_init_triangle_filter( lc, sample );			
			// pFilter = &gLcRefFilter[lc];
			pFilter->accDiff = 0;
			pFilter->accDiffRoundOff = 0;
			pFilter->prvRunAvg = sample;
			
			pABSdAccDiffFilter->accDiff = 
			pABSdAccDiffFilter->accDiffRoundOff = 
			pABSdAccDiffFilter->prvRunAvg = 0;
			
			pFilterManager->newStepNumSample =
			pFilterManager->simpleFlatNumSample =
			pFilterManager->flatNumSample = 0;
			pFilterManager->exportWindowSize = 1;
			pFilterManager->absAccDiffPeak = 
			pFilterManager->maxAbsAccDiff =  0;
			pFilterManager->minAbsAccDiff = 0x7FFFFFFF;
			pFilterManager->prvState = FILTER_STATE_FLAT;
			pFilterManager->curState = FILTER_STATE_FLAT;
			
			switch ( filterCnfg ) {
			// 2012-02-03 -WFC- v
//				case LC_FILTER_HI :		// 2007/04/30 -WFC-
//				case LC_FILTER_VERY_HI :
//					pFilterManager->stepThresholdAccDiffAvg = gFilterManager[lc].adcCountPerD * 150;	// 150d
//					pFilterManager->flatThreshold = gFilterManager[lc].adcCountPerD << 2;				// 4d
//					break;
			// 2012-02-03 -WFC- ^
 			case LC_FILTER_HI :		// 2007/04/30 -WFC-
				pFilterManager->stepThresholdAccDiffAvg = gFilterManager[lc].adcCountPerD * 150;	// 150d
				pFilterManager->flatThreshold = gFilterManager[lc].adcCountPerD << 2;				// 4d
				break;
			case LC_FILTER_VERY_HI :
				pFilterManager->stepThresholdAccDiffAvg = gFilterManager[lc].adcCountPerD * 150;	// 150d
				pFilterManager->flatThreshold = gFilterManager[lc].adcCountPerD << 2;				// 4d
				break;
			case LC_FILTER_LO :
					pFilterManager->stepThresholdAccDiffAvg = gFilterManager[lc].adcCountPerD * 150;	// 150d
					pFilterManager->flatThreshold = gFilterManager[lc].adcCountPerD << 1;				// 2d
					break;
			}
			pFilterManager->stepThreshold_slope = pFilterManager->kStepThreshold_slope_low;
			pFilterManager->isVeryFirstTime = FALSE;
			*pFilteredData = sample;					// 2010-09-27 -WFC- This prevents bogus 0 value weight right after filter init.
			filterEvent = LC_FILTER_EVENT_DISPLAYABLE;	// 2010-09-27 -WFC- This prevents bogus 0 value weight right after filter init.
		}
		else {
			// use reference filter gLcRefFilter to generate a reference accDiff
			// gLcRefFilter[lc].prvRunAvg = running_avg32( sample, gLcRefFilter[lc].prvRunAvg,
			// 			   			   &gLcRefFilter[lc].accDiff, &gLcRefFilter[lc].accDiffRoundOff, 1);
			// pFilter = &gLcRefFilter[lc];
			i32Tmp = pFilter->prvRunAvg;		// 2010-10-29 -WFC-
			pFilter->prvRunAvg = running_avg32( sample, pFilter->prvRunAvg,
						   			   &(pFilter->accDiff), &(pFilter->accDiffRoundOff), 1);
			/*
			i32Tmp = pFilter->prvRunAvg - i32Tmp;
			i32Tmp = labs( i32Tmp );
			if ( oldAccDiff < FILTER_FLAT_LINE_SIMPLE_THRESHOLD )
				pFilterManager->simpleFlatNumSample++;
			else
				pFilterManager->simpleFlatNumSample = 0;

			#if ( CONFIG_COMPILER == CONFIG_USE_PC_GCC )
				gDeltaRefRunAvg = i32Tmp;
			#endif
			*/
			
			// 2010-10-29 -WFC- v
			i32Tmp = pFilter->prvRunAvg - i32Tmp;
			i32Tmp = labs( i32Tmp );
			if ( i32Tmp < (gFilterManager[lc].adcCountPerD << 1) ) {
				if ( pFilterManager-> refFlatNumSample < 254 )
				pFilterManager-> refFlatNumSample++;
			}
			else
				pFilterManager-> refFlatNumSample = 0;
			// 2010-10-29 -WFC- ^

			if ( LC_FILTER_LO == filterCnfg ) {
				wSize[0] = 2;
				//reqNumSample = 8;
				reqNumSample = 1;
			}
			else {
				wSize[0] = 3;
				//reqNumSample = 10;
				reqNumSample = 2;
			}

			pFilterManager->prvAbsAccDiff = pABSdAccDiffFilter->prvRunAvg;

            // running avg of abs(dAccDiff) of gLcRefFilter. It is for pattern detection such as new step(pickup, drop), motion and flat(stable).
        	pABSdAccDiffFilter->prvRunAvg = running_avg32( labs(gLcRefFilter[lc].accDiff- oldAccDiff),
									   pABSdAccDiffFilter->prvRunAvg, &(pABSdAccDiffFilter->accDiff), 
									   &(pABSdAccDiffFilter->accDiffRoundOff), wSize[0]);

			read_int32_circular_buf( &(pFilterManager->absDAccDiffBuf), &oldAccDiff );
			write_int32_circular_buf( &(pFilterManager->absDAccDiffBuf), pABSdAccDiffFilter->prvRunAvg );
            
			pFilterManager->absDAccDiff_slope = pABSdAccDiffFilter->prvRunAvg - oldAccDiff;

			// Handle different pattern states such as motion, new step and flat etc...
			switch ( pFilterManager->curState ) {
				case FILTER_STATE_MOTION :
						if ( pABSdAccDiffFilter->prvRunAvg> pFilterManager->kMotionHigh_d ||
							pFilterManager->absDAccDiff_slope > pFilterManager->kMotionHigh_slope_d ) {
							pFilterManager->stepThreshold_slope = pFilterManager->kStepThreshold_slope_high;   // for general
							pFilterManager->stepThresholdAccDiffAvg = pFilterManager->kStepThreshold_high;
						}
						else if ( pABSdAccDiffFilter->prvRunAvg > pFilterManager->kMotionMedium_d ) {
							pFilterManager->stepThreshold_slope = pFilterManager->kStepThreshold_slope_med;
							//gFilterManager[lc].stepThresholdAccDiffAvg = gFilterManager[lc].kStepThreshold_med;
						}
						else  {
							pFilterManager->stepThreshold_slope = pFilterManager->kStepThreshold_slope_low;
							//gFilterManager[lc].stepThresholdAccDiffAvg = gFilterManager[lc].kStepThreshold_low;
						}

						switch ( filterCnfg ) {			// 2007/04/30 -WFC-
							case LC_FILTER_HI :
									wSize[2] = FILTER_HI_MAX_WINDOW_SIZE;
									wSize[1] = 2;
									wSize[0] = FILTER_NEW_STEP_WINDOW_SIZE;
								break;
							case LC_FILTER_VERY_HI :
									wSize[2] = FILTER_VHI_MAX_WINDOW_SIZE;
									wSize[1] = 3;
									wSize[0] = FILTER_NEW_STEP_WINDOW_SIZE;
								break;
							case LC_FILTER_LO :
									wSize[2] = FILTER_LO_MAX_WINDOW_SIZE;
									wSize[1] = 2;
									wSize[0] = FILTER_NEW_STEP_WINDOW_SIZE;
								break;
						}

						if ( pABSdAccDiffFilter->prvRunAvg> pFilterManager->kStepThreshold_high ) {
							pFilterManager->exportWindowSize = wSize[0];
						}
						else if ( pABSdAccDiffFilter->prvRunAvg > pFilterManager->kStepThreshold_med ) {
							pFilterManager->exportWindowSize = wSize[1];
						}
						else  {
							if ( LC_FILTER_LO == filterCnfg ) {
								if ( pABSdAccDiffFilter->prvRunAvg < (gFilterManager[lc].adcCountPerD << 1) )
									// pFilterManager->exportWindowSize = 1;
									pFilterManager->exportWindowSize = 2;			// 2011-05-06 -WFC-
								else
									// pFilterManager->exportWindowSize = 2;
									pFilterManager->exportWindowSize = 3;			// 2011-05-06 -WFC-
							}
							else {
								pFilterManager->exportWindowSize = wSize[2];
							}
						}

						// 2010-10-29 -WFC- v
						if ( pFilterManager->refFlatNumSample > LC_FILTER_REF_FLAT_CNT_THRESHOLD ) {	// 2010-10-29 -WFC-
							filterEvent = LC_FILTER_EVENT_SKIP_RUN;
							lc_init_flat_state( lc, filterCnfg, sample);
						}
						else if ( pFilterManager->absDAccDiff_slope > pFilterManager->stepThreshold_slope ||
							pABSdAccDiffFilter->prvRunAvg  > pFilterManager->stepThresholdAccDiffAvg ||
							( (pFilterManager->absDAccDiff_slope < 0) &&
							  (pABSdAccDiffFilter->prvRunAvg  > pFilterManager->kStepThreshold_high) )) {
							filterEvent = lc_init_new_step_state( lc, sample );
						}
						// 2010-10-29 -WFC- ^
						else if ( pABSdAccDiffFilter->prvRunAvg < pFilterManager->flatThreshold) {
							// It enters in flat segment
							pFilterManager->flatNumSample++;
							if ( pFilterManager->flatNumSample > reqNumSample ) {
								filterEvent = LC_FILTER_EVENT_SKIP_RUN;
								lc_init_flat_state( lc, filterCnfg, sample);
							}
						}
						else { 	// it is in motion. Fuzzy area.
							lc_run_motion_state( lc, filterCnfg );
						} // end if ( gABSdAccDiffFilter[lc].prvRunAvg > FILTER_NEW_STEP_THRESHOLD ) {) else if {} else {}

					break;
				case FILTER_STATE_FLAT :
						pFilterManager->stepThreshold_slope     = pFilterManager->kStepThreshold_slope_low;
						pFilterManager->stepThresholdAccDiffAvg = pFilterManager->kStepThreshold_low;
						if ( pFilterManager->absDAccDiff_slope > pFilterManager->stepThreshold_slope  ||
							pABSdAccDiffFilter->prvRunAvg  > pFilterManager->stepThresholdAccDiffAvg ) {
							filterEvent = lc_init_new_step_state( lc, sample );
						}
						else if ( pABSdAccDiffFilter->prvRunAvg < pFilterManager->flatThreshold ) {
							if ( 0 == pFilterManager->flatNumSample  ) 
								pFilterManager->flatNumSample++;
							pFilterManager->motionNumSample = 0;
							lc_run_flat_state( lc, filterCnfg, FILTER_STATE_FLAT );
						}
						else { 	// it is in motion. Fuzzy area.
							pFilterManager->motionNumSample++;
							// 2010-10-29 -WFC- if ( pFilterManager->motionNumSample > reqNumSample )
							if ( pFilterManager->motionNumSample > reqNumSample && pFilterManager->refFlatNumSample < 3 )		// 2010-10-29 -WFC- 
								nextFilterState = FILTER_STATE_MOTION;
							else
								nextFilterState = FILTER_STATE_FLAT;
							lc_run_flat_state( lc, filterCnfg, nextFilterState );
						} // end if ( gABSdAccDiffFilter[lc].prvRunAvg > FILTER_NEW_STEP_THRESHOLD ) {) else if {} else {}
					break;
				case FILTER_STATE_NEW_STEP :
						// 2010-10-29 -WFC- v
						if ( pFilterManager->refFlatNumSample > LC_FILTER_REF_FLAT_CNT_THRESHOLD ) {	// 2010-10-29 -WFC-
							filterEvent = LC_FILTER_EVENT_SKIP_RUN;
							lc_init_flat_state( lc, filterCnfg, sample);
						}
						else if ( pFilterManager->absDAccDiff_slope > pFilterManager->stepThreshold_slope  ||
							pABSdAccDiffFilter->prvRunAvg  > pFilterManager->stepThresholdAccDiffAvg ||
							( (pFilterManager->absDAccDiff_slope < 0) &&
							  (pABSdAccDiffFilter->prvRunAvg  > pFilterManager->kStepThreshold_high) )) {
							filterEvent = lc_init_new_step_state( lc, sample );
						}
						// 2010-10-29 -WFC- ^
						else if ( pABSdAccDiffFilter->prvRunAvg < pFilterManager->flatThreshold ) {
							// It enters in flat segment
							pFilterManager->motionNumSample = 0;
							pFilterManager->flatNumSample++;
							if ( pFilterManager->flatNumSample > reqNumSample ) {
								filterEvent = LC_FILTER_EVENT_SKIP_RUN;
								lc_init_flat_state( lc, filterCnfg, sample);
							}
							else {
								lc_run_new_step_state( lc, filterCnfg, FILTER_STATE_FLAT );
							}
						}
						else { // it is in motion. Fuzzy area.
							pFilterManager->flatNumSample = 0 ;
							pFilterManager->motionNumSample++;
							if ( pFilterManager->motionNumSample > reqNumSample )
								nextFilterState = FILTER_STATE_MOTION;
							else
								nextFilterState = FILTER_STATE_FLAT;
							lc_run_new_step_state( lc, filterCnfg, nextFilterState );
						} // end if ( gABSdAccDiffFilter[lc].prvRunAvg > FILTER_NEW_STEP_THRESHOLD ) {} else if {} else {}
						if ( FILTER_STATE_NEW_STEP == pFilterManager->curState )
							lc_update_max_min_AbsAccDiff( lc );

					break;
			}// end switch (gFilterManager[lc].curState )


			if ( LC_FILTER_EVENT_RUN == filterEvent ) {
				/*
	       		gLcFilter0[lc].prvRunAvg = running_avg32( sample, gLcFilter0[lc].prvRunAvg,
						   			   &gLcFilter0[lc].accDiff,  &gLcFilter0[lc].accDiffRoundOff,
									   pFilterManager->exportWindowSize);
				// cascade both filter0 and filter1 to form a triangle filter
        		gLcFilter1[lc].prvRunAvg = running_avg32( gLcFilter0[lc].prvRunAvg,	gLcFilter1[lc].prvRunAvg,
						   			   &gLcFilter1[lc].accDiff, &gLcFilter1[lc].accDiffRoundOff,
									   pFilterManager->exportWindowSize);
				*pFilteredData = gLcFilter1[lc].prvRunAvg;
				*/
				
				pFilter = &gLcFilter0[lc];

	       		pFilter->prvRunAvg = running_avg32( sample, pFilter->prvRunAvg,
						   			   &(pFilter->accDiff),  &(pFilter->accDiffRoundOff),
									   pFilterManager->exportWindowSize);

				oldAccDiff = pFilter->prvRunAvg;   
				// cascade both filter0 and filter1 to form a triangle filter
				pFilter = &gLcFilter1[lc];
				*pFilteredData = pFilter->prvRunAvg;
        		pFilter->prvRunAvg = running_avg32( oldAccDiff, pFilter->prvRunAvg,
						   			   &(pFilter->accDiff), &(pFilter->accDiffRoundOff),
									   pFilterManager->exportWindowSize);

				
				*pFilteredData = pFilter->prvRunAvg - *pFilteredData;
				*pFilteredData = labs( *pFilteredData );
				if ( *pFilteredData < (gFilterManager[lc].adcCountPerD << 1) )
					pFilterManager->simpleFlatNumSample++;
				else
					pFilterManager->simpleFlatNumSample = 0;

				#if ( CONFIG_COMPILER == CONFIG_USE_PC_GCC )
					gDeltaRefRunAvg = *pFilteredData ;
				#endif
				

				// now *pFilteredData has the final result.
				*pFilteredData = pFilter->prvRunAvg;

				if ( FILTER_STATE_NEW_STEP == pFilterManager->curState ) {
					//if ( pFilterManager->newStepNumSample > 10 )	// 0.5 seconds
					if ( pFilterManager->newStepNumSample > 1 || pFilterManager->simpleFlatNumSample > 1)	// 0.15 seconds at 13Hz sample rate.
						filterEvent = LC_FILTER_EVENT_DISPLAYABLE;
				}
				else {
					filterEvent = LC_FILTER_EVENT_DISPLAYABLE;
				}
			}
		} // end if ( 0 == gLcRefFilter[lc].prvRunAvg ) {} else {}

	return filterEvent;
} // end lcFilter(,)


/**
 * It initializes triangle filter and update gFilterManager.
 *
 * @param		lc	--	load cell index
 * @param	sample	--	data to be filter
 *
 * @return filterEvent
 *
 * @post house keeping for gFilterManager
 *
 *
 * History:  Created on 2007/04/03 by Wai Fai Chin
 */

BYTE lc_init_new_step_state( BYTE lc, INT32 sample )
{
	LC_FILTER_MANAGER_T		*pFilterManager;
	RUNNING_AVG_FILTER32_T	*pFilter;
	BYTE filterEvent;
 
	filterEvent = LC_FILTER_EVENT_RUN;						// assumed will run lcFilter() for this sample.
	// It has a new larger step, reset triangle filter.
	lc_init_triangle_filter(lc, sample );
	
	pFilterManager = &gFilterManager[lc];
	pFilterManager->newStepNumSample = 1;
	pFilterManager->flatNumSample =
	// pFilterManager->simpleFlatNumSample =
	pFilterManager->motionNumSample = 0;
	pFilterManager->refFlatNumSample = 0;				// 2010-10-29 -WFC-
	pFilterManager->minAbsAccDiff = 0x7FFFFFFF;

	if ( pFilterManager->prvState != pFilterManager->curState )
		pFilterManager->prvState = pFilterManager->curState;
	pFilterManager->curState = FILTER_STATE_NEW_STEP;

	pFilterManager->exportWindowSize = 1;
	//gFilterManager[lc].exportWindowSize = 2;  	// This produced overshoot and longer decay.
	if ( pFilterManager->absDAccDiff_slope > pFilterManager->kStepThreshold_low )
		filterEvent = LC_FILTER_EVENT_SKIP_RUN;		// Skip call lcFilter() for this sample.

	pFilter = &gABSdAccDiffFilter[lc];

	if ( pFilter->prvRunAvg  > pFilterManager->stepThresholdAccDiffAvg )
		pFilterManager->stepThresholdAccDiffAvg =  pFilter->prvRunAvg;

	if ( gABSdAccDiffFilter[lc].prvRunAvg > pFilterManager->maxAbsAccDiff ) {
		pFilterManager->maxAbsAccDiff = pFilter->prvRunAvg;
		if ( pFilterManager->maxAbsAccDiff > pFilterManager->stepThresholdAccDiffAvg )
			pFilterManager->stepThresholdAccDiffAvg = pFilter->prvRunAvg;
	}
	
	if ( pFilter->prvRunAvg > pFilterManager->absAccDiffPeak )
		pFilterManager->absAccDiffPeak = pFilter->prvRunAvg;

	if ( pFilter->prvRunAvg < pFilterManager->minAbsAccDiff )
		pFilterManager->minAbsAccDiff = pFilter->prvRunAvg;

	return filterEvent;
}// end lc_init_new_step_state(,)


/**
 * initialize flat state for gFilterManager.
 *
 * @param			lc	-- load cell index
 * @param	filterCnfg	-- filter configuration such as disabled, level 1, 2, 3 etc..
 * @param		sample	-- raw sample value.
 *
 * @return none
 *
 * @post house keeping for gFilterManager
 *
 * History:  Created on 2007/04/03 by Wai Fai Chin
 * 2010-09-21 -WFC- modified for Linear Tech ADC chip.
 *
 */

void lc_init_flat_state( BYTE lc, BYTE filterCnfg, INT32 sample )
{
	LC_FILTER_MANAGER_T		*pFilterManager;
	RUNNING_AVG_FILTER32_T	*pFilter0, *pFilter1;
	INT32	adc5D;
	INT32	adc6D;


	pFilterManager = &gFilterManager[lc];
	pFilter0 = &gLcFilter0[lc];
	pFilter1 = &gLcFilter1[lc];


// 2011-05-06 -WFC-	if ( pFilterManager->prvState > FILTER_STATE_FLAT ||
// 2011-05-06 -WFC-		 LC_FILTER_LO == filterCnfg )  { // 2007/04/19 -WFC-  if previous state is not flat 

	if ( FILTER_STATE_NEW_STEP == pFilterManager->prvState ) {		// 2011-05-06 -WFC- only set this if previous state was new step. This means no window size change from flat or motion state to keep filtered weight steady.

		adc5D = gFilterManager[lc].adcCountPerD * 5;
		adc6D = gFilterManager[lc].adcCountPerD + adc5D;

		if ( LC_FILTER_LO == filterCnfg ) {
			//if ( gFilterManager[lc].prvAbsAccDiff > 5000 ||
			//	 FILTER_STATE_NEW_STEP == gFilterManager[lc].prvState ) {
			if ( pFilterManager->prvAbsAccDiff > adc5D ) {  // prevent small fluctuation on flat line.
				pFilter1->prvRunAvg = 
				pFilter0->prvRunAvg = sample;					
				// 2011-05-06 -WFC- pFilterManager->exportWindowSize = 1;	// smaller beginning window size, faster settle time to its true value.
				pFilterManager->exportWindowSize = 2;	// smaller beginning window size, faster settle time to its true value.	// 2011-05-06 -WFC- 
			}
		}
		else {
			if ( pFilterManager->prvAbsAccDiff > adc6D || gABSdAccDiffFilter[lc].prvRunAvg < adc5D ||
				 FILTER_STATE_NEW_STEP == pFilterManager->prvState ) {
				//gLcFilter1[lc].prvRunAvg = gLcFilter0[lc].prvRunAvg;
				pFilter1->prvRunAvg = 
				pFilter0->prvRunAvg = sample;					
				pFilterManager->exportWindowSize = 2;	// smaller beginning window size, faster settle time to its true value.
			}
		}
					
		pFilter0->accDiff =
		pFilter0->accDiffRoundOff = 0;
		pFilter1->accDiff = 0;
		pFilter1->accDiffRoundOff = 0;
		//gFilterManager[lc].exportWindowSize = 3;	// for final window size 7, high filter, the smaller beginning window size, faster settle time to its true value.
	}


	pFilterManager->newStepNumSample = 0;
	pFilterManager->motionNumSample = 0;
	pFilterManager->flatNumSample=1;
	if ( pFilterManager->prvState != pFilterManager->curState )
		pFilterManager->prvState = pFilterManager->curState;
	pFilterManager->curState = FILTER_STATE_FLAT;
	// This give fast clean rising or falling edge.
	// gFilterManager[lc].exportWindowSize = 3;	// smaller beginning window size, faster settle time to its true value.
	pFilterManager->minAbsAccDiff = 0x7FFFFFFF;

	switch ( filterCnfg ) {
		case LC_FILTER_HI :			// 2007-04-30 -WFC-
		case LC_FILTER_VERY_HI :
			pFilterManager->flatThreshold = gFilterManager[lc].adcCountPerD << 2;
			break;
		case LC_FILTER_LO :
			pFilterManager->flatThreshold = gFilterManager[lc].adcCountPerD << 1;		// 2d.
			break;
	}
} // end lc_init_flat_state(,,)


/**
 * house keeping in flat state for gFilterManager.
 *
 * @param			lc	-- load cell index
 * @param	filterCnfg	-- filter configuration such as disabled, level 1, 2, 3 etc..
 * @param	nextState	-- next state value when require to change pattern state.
 *
 * @return none
 *
 * @post house keeping of gFilterManager
 *
 *
 * History:  Created on 2007-04-03 by Wai Fai Chin
 * 2010-09-21 -WFC- modified for Linear Tech ADC chip.
 *
 */

void lc_run_flat_state( BYTE lc, BYTE filterCnfg, BYTE nextState )
{
	LC_FILTER_MANAGER_T		*pFilterManager;
	RUNNING_AVG_FILTER32_T	*pFilter0, *pFilter1;

	BYTE  wSize[2];
	UINT16 flatExpCnt[2];

	pFilterManager = &gFilterManager[lc];
	pFilter0 = &gLcFilter0[lc];
	pFilter1 = &gLcFilter1[lc];
	
	switch ( filterCnfg ) {			// 2007-04-30 -WFC-
			case LC_FILTER_HI :
				wSize[1] = FILTER_HI_MAX_WINDOW_SIZE;
				flatExpCnt[1] = FLAT_LINE_EXPIRED_CNT2_HI;
				// 2011-05-06 -WFC-  wSize[0] = 1;
				wSize[0] = FILTER_HI_MIN_WINDOW_SIZE;		// 2011-05-06 -WFC- 
				flatExpCnt[0] = FLAT_LINE_EXPIRED_CNT1_HI;
			break;
		case LC_FILTER_VERY_HI :
				wSize[1] = FILTER_VHI_MAX_WINDOW_SIZE;
				flatExpCnt[1] = FLAT_LINE_EXPIRED_CNT2_HI;
				// 2011-05-06 -WFC-  wSize[0] = 2;
				wSize[0] = FILTER_VHI_MIN_WINDOW_SIZE;		// 2011-05-06 -WFC-  
				flatExpCnt[0] = FLAT_LINE_EXPIRED_CNT1_HI;
			break;
		case LC_FILTER_LO :
				wSize[1] = FILTER_LO_MAX_WINDOW_SIZE;
				flatExpCnt[1] = FLAT_LINE_EXPIRED_CNT2_LO;
				// 2011-05-06 -WFC- wSize[0] = 1;
				wSize[0] = FILTER_LO_MIN_WINDOW_SIZE;		// 2011-05-06 -WFC- 
				flatExpCnt[0] = FLAT_LINE_EXPIRED_CNT1_LO;
			break;
	}


	if ( pFilterManager->flatNumSample > 0 ) {
		pFilterManager->flatNumSample++;

		if ( pFilterManager->flatNumSample > flatExpCnt[1] ) {
			// pFilterManager->exportWindowSize = wSize[1];
			pFilterManager->newStepNumSample = 0;
			if ( pFilterManager->prvState != pFilterManager->curState )
				pFilterManager->prvState = pFilterManager->curState;
			pFilterManager->curState = nextState;
			pFilterManager->maxAbsAccDiff =  0;
			if ( FILTER_STATE_MOTION == nextState )
				pFilterManager->exportWindowSize = wSize[1];
			else
				pFilterManager->exportWindowSize = wSize[0];

		}
		else if ( pFilterManager->flatNumSample > flatExpCnt[0] ) {
			if ( pFilterManager->prvState > FILTER_STATE_FLAT )   { // 2007-04-19 -WFC- if previous state is not flat
				if ( gABSdAccDiffFilter[lc].prvRunAvg > (gFilterManager[lc].adcCountPerD * 5)) {
					pFilterManager->exportWindowSize = wSize[0];
					pFilter1->prvRunAvg = pFilter0->prvRunAvg;
				}
			}
			//gLcFilter1[lc].prvRunAvg = gLcFilter0[lc].prvRunAvg;
		}
		else { // 2007-04-19 -WFC-
			if ( pFilterManager-> prvState < FILTER_STATE_NEW_STEP )   { // 2007-04-19 -WFC- if previous state is flat
				pFilter1->prvRunAvg = pFilter0->prvRunAvg;
			}
		}
	} // end if ( gFilterManager[lc].newStepNumSample > 0 ) {}

} // end lc_run_flat_state(,,)


/**
 * house keeping in new step state for gFilterManager.
 *
 * @param			lc	-- load cell index
 * @param	filterCnfg	-- filter configuration such as disabled, level 1, 2, 3 etc..
 * @param	nextState	-- next state value when require to change pattern state.
 *
 * @return	none
 *
 * @post	house keeping for gFilterManager
 *
 * History:  Created on 2007-04-03 by Wai Fai Chin
 * 2010-09-21 -WFC- modified for Linear Tech ADC chip.
 *
 */

 
void lc_run_new_step_state( BYTE lc, BYTE filterCnfg, BYTE nextState )
{

	LC_FILTER_MANAGER_T		*pFilterManager;
	BYTE  wSize[4];
	UINT16 newStepExpCnt[4];

	pFilterManager = &gFilterManager[lc];

	switch ( filterCnfg ) {			// 2007-04-30 -WFC-
			case LC_FILTER_HI :
				wSize[3] = 3;
				wSize[2] = 2;
				wSize[1] = 1;
				wSize[0] = 1;
				newStepExpCnt[3] = NEW_STEP_EXPIRED_CNT6_HI;
				newStepExpCnt[2] = NEW_STEP_EXPIRED_CNT5_HI;
				newStepExpCnt[1] = NEW_STEP_EXPIRED_CNT4_HI;
				newStepExpCnt[0] = NEW_STEP_EXPIRED_CNT1_HI;
			break;
		case LC_FILTER_VERY_HI :
				wSize[3] = 4;
				wSize[2] = 3;
				wSize[1] = 2;
				wSize[0] = 1;
				newStepExpCnt[3] = NEW_STEP_EXPIRED_CNT6_HI;
				newStepExpCnt[2] = NEW_STEP_EXPIRED_CNT5_HI;
				newStepExpCnt[1] = NEW_STEP_EXPIRED_CNT4_HI;
				newStepExpCnt[0] = NEW_STEP_EXPIRED_CNT1_HI;
			break;
		case LC_FILTER_LO :
				wSize[3] = FILTER_LO_MAX_WINDOW_SIZE;
				wSize[2] = 2;
				wSize[1] = 1;
				wSize[0] = 1;
				newStepExpCnt[3] = NEW_STEP_EXPIRED_CNT6_LO;
				newStepExpCnt[2] = NEW_STEP_EXPIRED_CNT5_LO;
				newStepExpCnt[1] = NEW_STEP_EXPIRED_CNT3_LO;
				newStepExpCnt[0] = NEW_STEP_EXPIRED_CNT1_LO;
			break;
	}



	if ( pFilterManager->newStepNumSample > 0 ) {
		pFilterManager->newStepNumSample++;
		if ( pFilterManager->newStepNumSample > newStepExpCnt[3] ) {
			pFilterManager->newStepNumSample = 0;		// prevent re-enter untill new step occured
			//pFilterManager->exportWindowSize = wSize[3];
			if ( pFilterManager->prvState != pFilterManager->curState ) {
				pFilterManager->prvState = pFilterManager->curState;
			}
			pFilterManager->curState = nextState;
			pFilterManager->maxAbsAccDiff =  0;
			if ( FILTER_STATE_MOTION == nextState )
				pFilterManager->exportWindowSize = wSize[3];
			else
				pFilterManager->exportWindowSize = wSize[0];
		}
		else if ( pFilterManager->newStepNumSample > newStepExpCnt[2] ) {
			pFilterManager->exportWindowSize = wSize[2];  
		}
		else if ( pFilterManager->newStepNumSample > newStepExpCnt[1] ) {
			pFilterManager->exportWindowSize = wSize[1];  
		}
		else if ( pFilterManager->newStepNumSample > newStepExpCnt[0] ) {
			pFilterManager->exportWindowSize = wSize[0];
		}
	} // end if ( gFilterManager[lc].newStepNumSample > 0 ) {}
} // end lc_run_new_step_state(,,)


/**
 * It update gFilterManager based on gABSdAccDiffFilter.
 *
 * @param  lc			-- load cell index.
 *
 * @return new filter event
 *
 * @post house keeping for gFilterManager
 *
 * @note Only new step state can call this function.
 *
 * History:  Created on 2007/04/03 by Wai Fai Chin
 */

void lc_update_max_min_AbsAccDiff( BYTE lc )
{
	LC_FILTER_MANAGER_T		*pFilterManager;
	RUNNING_AVG_FILTER32_T	*pFilter;

	pFilterManager = &gFilterManager[lc];
	
	pFilter = &gABSdAccDiffFilter[lc];
	
	if ( pFilter->prvRunAvg > pFilterManager->maxAbsAccDiff ) {
		pFilterManager->maxAbsAccDiff = pFilter->prvRunAvg;
		pFilterManager->stepThresholdAccDiffAvg = pFilterManager->maxAbsAccDiff;
	}

	if ( pFilter->prvRunAvg > pFilterManager->absAccDiffPeak )
		pFilterManager->absAccDiffPeak = gABSdAccDiffFilter[lc].prvRunAvg;

	if ( pFilter->prvRunAvg < pFilterManager->minAbsAccDiff )
		pFilterManager->minAbsAccDiff = pFilter->prvRunAvg;

} // end lc_update_max_min_AbsAccDiff()


/**
 * house keeping in motion state for gFilterManager.
 *
 * @param			lc	-- load cell index
 * @param	filterCnfg	-- filter configuration such as disabled, level 1, 2, 3 etc..
 *
 * @return none
 *
 * @post house keeping for gFilterManager
 *
 * History:  Created on 2007-04-03 by Wai Fai Chin
 *
 * 2010-09-21 -WFC- modified for Linear Tech ADC chip.
 */

void lc_run_motion_state( BYTE lc, BYTE filterCnfg )
{

	UINT16 motionCntNeed;
	LC_FILTER_MANAGER_T *pFilterManager;
	
	pFilterManager = &gFilterManager[lc];

	if ( LC_FILTER_LO == filterCnfg ) {
		motionCntNeed = 2;				// >2 ==3 about 5 ticks ==> 0.25 second
	}
	else {
		motionCntNeed = 4;				// >4 ==5 about 9 ticks ==> 0.45 seconds
	}

	pFilterManager->flatNumSample = 0;
	// 2010-10-29 -WFC- Don't reset it because it will missed flat state dection. pFilterManager->refFlatNumSample = 0;				// 2010-10-29 -WFC-
	
	pFilterManager->motionNumSample++;
	if ( pFilterManager->motionNumSample >= motionCntNeed ) {
		pFilterManager->flatNumSample =		 // reset for the possible next new flat segment again.
		pFilterManager->newStepNumSample = 0; //  reset for the possible next new step segment again.
		if ( pFilterManager->prvState != pFilterManager->curState )
			pFilterManager->prvState = pFilterManager->curState;
		pFilterManager->curState = FILTER_STATE_MOTION;				// tell filter to switch state in the next pass.
		pFilterManager->maxAbsAccDiff =  0;
	}
} // end lc_run_motion_state(,)


/**
 * initialize loadcell filters and its filter manager based on filter configuration.
 *
 * @param			lc	-- load cell index
 * @param	filterCnfg	-- filter configuration such as disabled, level 1, 2, 3 etc..
 *
 * @return none
 *
 * @post house keeping for gFilterManager
 *
 * History:  Created on 2007/04/03 by Wai Fai Chin
 * 2010-10-29 -WFC- step threshold is based on calibrated capacity.
 *
 */

void lc_filter_init( BYTE lc, BYTE filterCnfg)
{
	// save 966 bytes just use pointer to FilterManager instead of index like gFilterManager[lc].
	LC_FILTER_MANAGER_T 	*pFilterManager;
	RUNNING_AVG_FILTER32_T	*pFilter;
	INT32	adcCntPerD;
	INT32	adcCntAtCapacity;					// 2010-10-29 -WFC-

	#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )
		LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
		SENSOR_CAL_T			*pCal;			// points to a cal table
		LOADCELL_T 				*pLc;			// points to a loadcell
		float					fA, fB;
	#endif

	pFilterManager = &gFilterManager[lc];

	pFilterManager->isVeryFirstTime = TRUE;
	pFilterManager->exportWindowSize = 1;
	pFilterManager->newStepNumSample =
	pFilterManager->flatNumSample =
	pFilterManager->simpleFlatNumSample =
	pFilterManager->motionNumSample = 0;
	pFilterManager->absAccDiffPeak = 
	pFilterManager->maxAbsAccDiff =  0;
	pFilterManager->minAbsAccDiff = 0x7FFFFFFF;
	pFilterManager->prvState = FILTER_STATE_FLAT;
	pFilterManager->curState = FILTER_STATE_FLAT;
	pFilterManager->refFlatNumSample = 0;				// 2010-10-29 -WFC-

	//adcCntPerD = gFilterManager[lc].adcCountPerD = 65;		// default value based on 2mV loadcell.
	//adcCntPerD = gFilterManager[lc].adcCountPerD = 202;		// default value based on 0.5mV loadcell.
	//adcCntAtCapacity = 809920;								// default value based on 0.5mV loadcell.
	adcCntPerD = gFilterManager[lc].adcCountPerD = 173;		// default value based on 1.5mV to 2.0mV loadcell.
	adcCntAtCapacity = 308876;								// default value based on 1.5mV to 2.0mV loadcell.
	#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )
		if ( lc < MAX_NUM_LOADCELL )	{
			pSnDesc = &gaLSensorDescriptor[ lc ];
			pLc = (LOADCELL_T *) pSnDesc-> pDev;
			pCal = &(pLc-> pCal[0]);
			if ( CAL_STATUS_COMPLETED == pCal-> status ) {
				fA = ( float) (pCal->adcCnt[ MAX_CAL_POINTS - 1 ] - pCal->adcCnt[0]);
				fB = pCal->value[ MAX_CAL_POINTS - 1 ] - pCal->value[0];
				if ( fB > 0.0001 ) {
					fA = fA / fB;					// AdcCount / value
					fB = fA * pCal->countby.fValue;	// AdcCount per D;
					gFilterManager[lc].adcCountPerD = (INT32) fB;
					adcCntAtCapacity = (INT32) (pCal-> capacity * fA);		// 2010-10-29 -WFC- Maximum ADC count at capacity.
				}
			}
		}
	#endif

	switch ( filterCnfg ) {
		case LC_FILTER_VERY_HI :
				/* // 2011-05-06 -WFC-
				pFilterManager->kStepThreshold_slope_low  = adcCntPerD * 10;	// about 10d 2010-09-21 -WFC-
				pFilterManager->kStepThreshold_slope_med  = adcCntPerD * 15;
				pFilterManager->kStepThreshold_slope_high = adcCntPerD * 20;
				*/

				pFilterManager->kStepThreshold_slope_low  = adcCntPerD * 20;	// 2011-05-06 -WFC-
				pFilterManager->kStepThreshold_slope_med  = adcCntPerD * 30;	// 2011-05-06 -WFC-
				pFilterManager->kStepThreshold_slope_high = adcCntPerD * 50;	// 2011-05-06 -WFC- 


				pFilterManager->kStepThreshold_low  = ( adcCntAtCapacity << 3 ) / 100;		// 8%		2010-10-29 -WFC-
				pFilterManager->kStepThreshold_med  = pFilterManager->kStepThreshold_low;
				pFilterManager->kStepThreshold_high = pFilterManager->kStepThreshold_low;

				pFilterManager->kMotionHigh_d       = adcCntPerD << 3;
				pFilterManager->kMotionHigh_slope_d = adcCntPerD * 13;
				pFilterManager->kMotionMedium_d     = adcCntPerD << 3;

				pFilterManager->stepThresholdAccDiffAvg = pFilterManager->kMotionMedium_d;
				pFilterManager->flatThreshold = adcCntPerD << 2;
			break;
		case LC_FILTER_HI :
				/* // 2011-05-06 -WFC-
				pFilterManager->kStepThreshold_slope_low  = adcCntPerD * 10;	// about 10d 2010-09-21 -WFC-
				pFilterManager->kStepThreshold_slope_med  = adcCntPerD * 15;
				pFilterManager->kStepThreshold_slope_high = adcCntPerD * 20;
				*/

				pFilterManager->kStepThreshold_slope_low  = adcCntPerD * 15;	// 2011-05-06 -WFC-
				pFilterManager->kStepThreshold_slope_med  = adcCntPerD * 20;	// 2011-05-06 -WFC-
				pFilterManager->kStepThreshold_slope_high = adcCntPerD * 40;	// 2011-05-06 -WFC- 

				pFilterManager->kStepThreshold_low  = ( adcCntAtCapacity << 2 ) / 100;		// 4%		2010-10-29 -WFC-
				pFilterManager->kStepThreshold_med  = pFilterManager->kStepThreshold_low;
				pFilterManager->kStepThreshold_high = pFilterManager->kStepThreshold_low;

				pFilterManager->kMotionHigh_d       = adcCntPerD << 3;
				pFilterManager->kMotionHigh_slope_d = adcCntPerD * 13;
				pFilterManager->kMotionMedium_d     = adcCntPerD << 3;

				pFilterManager->stepThresholdAccDiffAvg = pFilterManager->kMotionMedium_d;
				pFilterManager->flatThreshold = adcCntPerD << 2;
			break;
		case LC_FILTER_LO :
				pFilterManager->kStepThreshold_slope_low  = adcCntPerD * 10;	// about 10d 2010-09-21 -WFC-
				pFilterManager->kStepThreshold_slope_med  = adcCntPerD * 15;
				// 2011-05-06 -WFC- pFilterManager->kStepThreshold_slope_high = adcCntPerD * 20;
				pFilterManager->kStepThreshold_slope_high = adcCntPerD * 30;	// 2011-05-06 -WFC- 

				// 2011-05-06 -WFC- to small pFilterManager->kStepThreshold_low  = ( adcCntAtCapacity << 1 ) / 100;		// 2%		2010-10-29 -WFC-
				pFilterManager->kStepThreshold_low  = ( adcCntAtCapacity << 2 ) / 100;		// 4%		2011-05-06 -WFC-
				pFilterManager->kStepThreshold_med  = pFilterManager->kStepThreshold_low;
				pFilterManager->kStepThreshold_high = pFilterManager->kStepThreshold_low;

				/* 2011-05-06 -WFC- too small.
				pFilterManager->kMotionHigh_d       = adcCntPerD << 2;
				pFilterManager->kMotionHigh_slope_d = adcCntPerD * 13;
				pFilterManager->kMotionMedium_d     = adcCntPerD << 2;
				*/

				pFilterManager->kMotionHigh_d       = adcCntPerD << 3;
				pFilterManager->kMotionHigh_slope_d = adcCntPerD * 13;
				pFilterManager->kMotionMedium_d     = adcCntPerD << 3;
				
				pFilterManager->stepThresholdAccDiffAvg = pFilterManager->kMotionMedium_d;
				pFilterManager->flatThreshold = adcCntPerD << 1;
				break;
	}

	pFilterManager->stepThreshold_slope = pFilterManager->kStepThreshold_slope_low;

	// gFilterManager[lc].absDAccDiffBuf is the circular buffer object, gABSdAccDiffBuf[][] is the concrete data buffer, fill 0 in the data buffer.
	init_int32_circular_buf( &(pFilterManager->absDAccDiffBuf), &gABSdAccDiffBuf[ lc ][0], 0, MAX_ABS_D_ACCDIFF_BUF_SIZE );
   
	pFilter = &gLcFilter0[lc];
	pFilter->prvRunAvg = 
	pFilter->accDiff = 
	pFilter->accDiffRoundOff = 0;
	
	pFilter = &gLcFilter1[lc];
	pFilter->prvRunAvg = 
	pFilter->accDiff = 
	pFilter->accDiffRoundOff = 0;
	
	pFilter = &gLcRefFilter[lc];
	pFilter->prvRunAvg = 
	pFilter->accDiff = 
	pFilter->accDiffRoundOff = 0;
	
	pFilter = &gABSdAccDiffFilter[lc];
	pFilter->prvRunAvg = 
	pFilter->accDiff = 
	pFilter->accDiffRoundOff = 0;
} // end lc_filter_init(,)


/**
 * It initializes triangle filter which it formed by cascaded both gLcFilter0[lc] and gLcFilter1[lc].
 *
 * @param		lc	-- load cell index
 * @param	sample	--	data to be filter
 *
 * @return none
 *
 * @post initialized both gLcFilter0[lc] and gLcFilter1[lc].
 *
 *
 * History:  Created on 2007/10/19 by Wai Fai Chin
 */

void lc_init_triangle_filter( BYTE lc, INT32 sample )
{
	RUNNING_AVG_FILTER32_T	*pFilter;

	pFilter = &gLcFilter0[lc];
	pFilter->accDiff =
	pFilter->accDiffRoundOff = 0;
	pFilter->prvRunAvg = sample;							// load current sample as new filtered value
 
	pFilter = &gLcFilter1[lc];
	pFilter->accDiff =
	pFilter->accDiffRoundOff = 0;
	pFilter->prvRunAvg = sample;							// load current sample as new filtered value
} // end lc_init_triangle_filter(,)
