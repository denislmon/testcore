/*! \file lc_zero.h \brief loadcell zeroing related functions.*/
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
/// \ingroup loadcell
/// \defgroup lc_zero Loadcell zeroing module (lc_zero.c)
/// \code #include "lc_zero.h" \endcode
/// \par Overview
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
///  NOTE:
///		raw_weight   = scale_factor * filtered_Adc_count;
///		zero_weight  = user selected pure raw_weight;
///		gross_weight = raw_weight - zero_weight;
///		net_weight   = gross_weight - tare_weight;
///		gross_wieght, net_weight and tare_weight are rounded based on countby.
///	\endcode
// ****************************************************************************
//@{
 

#ifndef MSI_LC_ZERO_H
#define MSI_LC_ZERO_H
#include  "config.h"

#include  "loadcell.h"


/// azm interval time
extern BYTE	gab_STD_AZM_IntervalTimeNV[ MAX_NUM_PV_LOADCELL];
extern BYTE	gab_NTEP_AZM_IntervalTimeNV[ MAX_NUM_PV_LOADCELL];
extern BYTE	gab_OIML_AZM_IntervalTimeNV[ MAX_NUM_PV_LOADCELL];

/// Auto zero maintenance countby range band. It will use to compute azmThresholdWt.
extern BYTE	gab_STD_AZM_CBRangeNV[ MAX_NUM_PV_LOADCELL];		// Auto zero maintenance countby range band.
extern BYTE	gab_NTEP_AZM_CBRangeNV[ MAX_NUM_PV_LOADCELL];		// Auto zero maintenance countby range band.
extern BYTE	gab_OIML_AZM_CBRangeNV[ MAX_NUM_PV_LOADCELL];		// Auto zero maintenance countby range band.

/// Percent of capacity above cal zero that can be zeroed off if below this limit, 1 = 1%, for compute zeroBandHiWt zero band high limit weight;
extern BYTE	gab_STD_pcentCapZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off, 1 = 1%
extern BYTE	gab_NTEP_pcentCapZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off, 1 = 1%
extern BYTE	gab_OIML_pcentCapZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off, 1 = 1%

/// Percent of capacity below cal zero that can be zeroed off if above this limit, 1 = 1%, for compute zeroBandLoWt zero band low limit weight
extern BYTE	gab_STD_pcentCapZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off, 1 = 1%
extern BYTE	gab_NTEP_pcentCapZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off, 1 = 1%
extern BYTE	gab_OIML_pcentCapZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off, 1 = 1%

/// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.
extern BYTE	gab_STD_pwupZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.
extern BYTE	gab_NTEP_pwupZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.
extern BYTE	gab_OIML_pwupZeroBandHiNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity above cal zero that can be zeroed off if below this limit during power up.

/// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.
extern BYTE	gab_STD_pwupZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.
extern BYTE	gab_NTEP_pwupZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.
extern BYTE	gab_OIML_pwupZeroBandLoNV[ MAX_NUM_PV_LOADCELL];	// Percent of capacity below cal zero that can be zeroed off if above this limit during power up.

/// values need to be save even if power was removed.
extern float	gafZeroWtFNV[ MAX_NUM_PV_LOADCELL];

/* ! This LC_ZERO_TAG is moved to loadcell.h file because GCC cannot handle recursed include file.
  \brief loadcell zeroing data structure.
   
   
 * /

typedef struct  LC_ZERO_TAG {
									/// azm threshold weight.
	float	azmThresholdWt;
									/// zero band high limit weight above cal zero that can be zero off if below this limit.
	float	zeroBandHiWt;
									/// zero band high limit weight below cal zero that can be zero off if above this limit.
	float	zeroBandLoWt;
									/// quarter countby weight, it is mainly for speedup execution in the expensive of memory space.
	float	quarterCBWt;
									/// zero pending timer.
	TIMER_T pendingTimer;
									/// azm Interval timer.
	TIMER_T azmIntervalTimer;
									/// azm Interval time.
	BYTE	azmIntervalTime;	
}LC_ZERO_T;
*/

void	lc_zero( LOADCELL_T *pLc, float zeroOffsetWt, BYTE lc );
void	lc_zero_azm( LOADCELL_T *pLc, float zeroOffsetWt, BYTE lc);
void	lc_zero_by_command( BYTE lc );
void	lc_zero_check_pending( LOADCELL_T *pLc, BYTE lc );
// 2012-02-08 -WFC- void	lc_zero_coz( LOADCELL_T *pLc );
void	lc_zero_coz( LOADCELL_T *pLc, BYTE lc );		// 2012-02-08 -WFC-
void	lc_zero_init( BYTE lc );
void	lc_zero_setup_zero_powerup( BYTE lc );
void	lc_zero_undo_by_command( BYTE lc );
void	lc_zero_undo( BYTE lc );
void 	lc_zero_init_all_lc_zero_config( void );		// 2016-03-21 -WFC-


#endif		// end MSI_LC_ZERO_H
//@}

