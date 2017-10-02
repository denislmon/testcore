/*! \file loadcellfilter.h \brief loadcell filter algorithm.*/
// ****************************************************************************
//
//                      MSI CellScale
//
//                  Copyright (c) 2007 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: independent
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2007/10/11 by Wai Fai Chin
// 
/// \ingroup loadcell
/// \defgroup loadcellfilter Loadcell filter related functions (loadcellfilter.c)
/// \code #include "loadcellfilter.h" \endcode
/// \par Overview
///		Loadcell filter is based on raw data pattern states of the loadcell consist of
///  flat, new step and motion states. It performs a triangle filter with
///  dynamic window size and threshold based on the pattern states.
///  threshold
//
// ****************************************************************************
//@{
 

#ifndef MSI_LOADCELLFILTER_H
#define MSI_LOADCELLFILTER_H

#define MAX_ABS_D_ACCDIFF_BUF_SIZE	2

#include "commonlib.h"

/*!
  \brief The loadcell filter is a triangle filter with dynamic window size and threshold
         based on three pattern states of raw data pattern which is flat, motion and new step.
		 It is for manage the filter state machine.
*/

typedef struct LC_FILTER_MANAGER_TAG {
									/// previous state of the filter
  BYTE  prvState;
									/// current state of the filter
  BYTE  curState;
									/// export window size for cascaded filters.
  BYTE  exportWindowSize;			//  export window size for cascaded filters.
									/// 0==use current sample, 1== skip current sample,  2== reload filter with current sample.
  BYTE  filterAction;				//  0==use current sample, 1== skip current sample,  2== reload filter with current sample.
									/// it is the very first time for this filter.
  BYTE	isVeryFirstTime;			//  it is the very first time for this filter.
									/// number of sample counts in a flat line based on Ref flilter. 2010-10-29 -WFC-
  BYTE	refFlatNumSample;			//  number of sample counts in a flat line based on Ref flilter. 2010-10-29 -WFC-
									/// number of sample counts in a new step. This will override any wave shape until it reached 20 samples.
  UINT16 newStepNumSample;			//  number of sample counts in a new step. This will override any wave shape until it reached 20 samples.
									/// number of sample counts in a flat line, could be interpret as settling number count.
  UINT16 flatNumSample;	    		//  number of sample counts in a flat line, could be interpret as settling number count.
									/// number of sample counts in a motion segment, could be interpret as unsettling number count.
  UINT16 motionNumSample;    		//  number of sample counts in a motion segment, could be interpret as unsettling number count.
									/// new step threshold based on the states of the raw data pattern.
  INT32  stepThreshold_slope;		//  new step threshold based on the states of the raw data pattern.
									/// new step threshold based on the states of the raw data pattern of gABSdAccDiff.
  INT32  stepThresholdAccDiffAvg;	//  new step threshold based on the states of the raw data pattern of gABSdAccDiff.
									///	new step threshold based on the states of the raw data pattern. about 5.5d
  INT32  flatThreshold;				//  new step threshold based on the states of the raw data pattern. about 5.5d
									///	maxium absAccDiff during the new step changing.
  INT32  maxAbsAccDiff;				//  maxium absAccDiff during the new step changing.
									///	minium absAccDiff during the new step changing.
  INT32  minAbsAccDiff;				//  minium absAccDiff during the new step changing.
									///	previous absAccDiff.
  INT32  prvAbsAccDiff;				//  previous absAccDiff.
									///	peak absAccDiff during the new step changing.
  INT32  absAccDiffPeak;			//  peak absAccDiff during the new step changing.
									///	ADC value per d.
  INT32  adcCountPerD;				//  ADC value per d.
									///	pre calculated step threshold as 222d.
  INT32  kStepThreshold_low;		//  pre calculated step threshold as 222d.
									///	pre calculated step threshold as 312d.
  INT32  kStepThreshold_med;		//  pre calculated step threshold as 312d.
									///	pre calculated step threshold as 371d.
  INT32  kStepThreshold_high;		//  pre calculated step threshold as 371d.
									///	pre calculated step threshold as 130d.
  INT32  kStepThreshold_slope_low;	//  pre calculated step threshold as 130d.
									///	pre calculated step threshold as 180d.
  INT32  kStepThreshold_slope_med;	//  pre calculated step threshold as 180d.
									///	pre calculated step threshold as 200d.
  INT32  kStepThreshold_slope_high;	//  pre calculated step threshold as 200d.
									///	highest d in motion and not qualified as new step 400d
  INT32  kMotionHigh_d;				//  highest d in motion and not qualified as new step 400d
									///	highest slope d in motion	200d
  INT32  kMotionHigh_slope_d;		//  highest slope d in motion	200d
									///	highest d in motion and not qualified as new step 150d
  INT32  kMotionMedium_d;			//  highest d in motion and not qualified as new step 150d
									///	Circular buffer use as a sliding window to store absDAccDiff for compute absDAccDiff_slope.
  INT32_CIRCULAR_BUF_T  absDAccDiffBuf; //	Circular buffer use as a sliding window to store absDAccDiff for compute absDAccDiff_slope.
									///	slope of absDAccDiff between size of absDAccDiffBuf circular buffer. It is used for detect parttern state. One can think of absDAccDiff as DY and size of circular buffer as DX.
  INT32  absDAccDiff_slope;			//  slope of absDAccDiff between size of absDAccDiffBuf circular buffer. One can think of absDAccDiff as DY and size of circular buffer as DX.
									/// simple method of counting number of sample counts in a flat line, could be interpret as settling number count.
									/// If the delta between current and previous final result is less than 300, then it is in flat line state, count it as flat smaple.
  UINT16 simpleFlatNumSample;	   //  simple method of counting number of sample counts in a flat line, could be interpret as settling number count.
}LC_FILTER_MANAGER_T;


/// filterEvent definitions.
#define LC_FILTER_EVENT_SKIP_RUN			0
#define LC_FILTER_EVENT_RUN					1
#define LC_FILTER_EVENT_USE_CUR_SAMPLE		2
#define LC_FILTER_EVENT_SKIP_CUR_SAMPLE		3
#define LC_FILTER_EVENT_DISPLAYABLE			4

#define  LC_FILTER_NO               0              
#define  LC_FILTER_LO               1              
#define  LC_FILTER_HI               2              
#define  LC_FILTER_VERY_HI          3


BYTE	lc_filter( BYTE lc, BYTE filterCnfg, INT32 sample, INT32 *pFilteredData );
void	lc_filter_init( BYTE lc, BYTE filterCnfg);
void	lc_init_flat_state( BYTE lc, BYTE filterCnfg, INT32 sample );
BYTE	lc_init_new_step_state( BYTE lc, INT32 sample );
void	lc_run_flat_state( BYTE lc, BYTE filterCnfg, BYTE nextState );
void	lc_run_motion_state( BYTE lc, BYTE filterCnfg );
void	lc_run_new_step_state( BYTE lc, BYTE filterCnfg, BYTE nextState );
BYTE 	lc_stepwise_triangle_filter( BYTE lc, BYTE filterCnfg, INT32 sample, INT32 *pFilteredData );
void	lc_update_max_min_AbsAccDiff( BYTE lc );



#endif		// end MSI_LOADCELLFILTER_H
//@}

