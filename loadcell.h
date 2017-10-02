/*! \file loadcell.h \brief loadcell related functions.*/
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
/// \ingroup Application
/// \defgroup loadcell Loadcell converts raw ADC to weight value and manages high level logics(loadcell.c)
/// \code #include "loadcell.h" \endcode
/// \par Overview
///   This is a high level module to convert filtered ADC count into a weight data base on the calibration table.
/// It does not know how the sensor got the ADC data. It just know how to compute the weight,
/// zero, AZM, tare, net or gross and other loadcell error checking. It formats output weight data string
/// to a caller supplied string buffer but it does not know how to output the data.
/// \code
///
///          Application           Abstract Object        Hardware driver
///         +--------------+      +---------------+     +----------------+
///         |   LOADCELL   |      |               |     | Linear Tech    |
///    -----|    MODULE    |<-----|SENSOR MODULE  |<----| ADC chip       |
///         | ADC to Weight|      |      ADC      |     | ADC value      |
///         |              |      |               |     |                |
///         +--------------+      +---------------+     +----------------+
///
///	\endcode
/// \note
/// \code
///		raw_weight   = scale_factor * filtered_Adc_count;
///		zero_weight  = user selected raw_weight;
///		gross_weight = raw_weight - zero_weight;
///		net_weight   = gross_weight - tare_weight;
///		gross_wieght, net_weight and tare_weight are rounded based on countby.
///		raw_weight, zeroWt and gafLcZeroOffsetWtNV[] are never round.
///	\endcode
///
/// Threshold weight values will be saved in the same unit as the cal unit.
/// If the user enter the value in unit different than cal unit, it will do
/// convered entered value to cal unit and then saved it; these values have suffix of NV.
/// The tare, zero, total, Sum square total etc... weight values are saved in display unit;
/// these values have the suffix of FNV.
//
//	
// ****************************************************************************
//@{
 

#ifndef MSI_LOADCELL_H
#define MSI_LOADCELL_H

#include  "config.h"

#include  "timer.h"

#include  "sensor.h"

//#include  "lc_zero.h"
//#include  "lc_tare.h"

#include  "calibrate.h"

// Maximum number of local physical loadcell.
#define  MAX_NUM_LOADCELL	4

// Maximum number of virtual math type loadcell.
#define  MAX_NUM_MATH_LOADCELL	1

// Maximum number of local physical loadcell + virtual math type loadcell.
#define  MAX_NUM_PV_LOADCELL	(MAX_NUM_LOADCELL + MAX_NUM_MATH_LOADCELL)

// #define  MAX_NUM_LOADCELL_UNITS		(SENSOR_UNIT_MAX_W)
#define  MAX_NUM_LOADCELL_UNITS		7



/*! scale standard mode such as industry==0, NTEP==1, OIML==2.
	bit7 ==> motion detection 1==enabled;
	bit6 ==> AZM, 1== enabled;
	bit5 ==> zero on powerup, 1== enabled.

	bit3 \
	      >--> 0 == no power saving, 1 == power saving, 2 == Safety mode, 2014-09-09 -WFC-
    bit2 /

	bit1 \
	      >--> industry==0, NTEP==1, OIML==2. 3==1unit.
	bit0 /

*/

#define  SCALE_STD_MODE_MOTION_DETECT	0X80	// MOTION DETECT ENABLED
#define  SCALE_STD_MODE_AZM				0X40	// AZM ENABLED
#define  SCALE_STD_MODE_ZERO_POWERUP	0X20	// ZERO ON POWER UP.
#define  SCALE_STD_MODE_AZM_ZERO_MASK	0XE0	// scale standard mode AZM ZERO FLAG MASK.
//2014-09-09 -WFC- v
#define  SCALE_STD_MODE_POWER_SAVING_DISABLED	0X00	// No POWER SAVING MODE.
#define  SCALE_STD_MODE_POWER_SAVING_ENABLED	0X04	// POWER SAVING MODE.
#define  SCALE_STD_MODE_POWER_SAVING_SAFETY		0X08	// SAFTY MODE override power saving.
#define  SCALE_STD_MODE_POWER_SAVING_MASK		0X0C	// POWER SAVING mask .
// 2014-09-09 -WFC- ^

#define  SCALE_STD_MODE_MASK			3		// scale standard mode MASK.
#define  SCALE_STD_MODE_INDUSTRY		0		// scale standard mode industry
#define  SCALE_STD_MODE_NTEP			1		// scale standard mode NTEP
#define  SCALE_STD_MODE_OIML			2		// scale standard mode OIML
#define  SCALE_STD_MODE_1UNIT			3		// scale standard mode 1 UNIT. 2010-11-09 -WFC-

#define  SCALE_STD_MODE_MAX				3		// MAX NUMBER OF SCALE STANDARD MODES

/*
#define  LC_OP_MODE_MOTION_DETECT		SCALE_STD_MODE_MOTION
#define  LC_OP_MODE_AZM					SCALE_STD_MODE_AZM
#define  LC_OP_MODE_ZERO_POWERUP		SCALE_STD_MODE_ZERO_PWUP
#define  LC_OP_MODE_AZM_ZERO_MASK		SCALE_STD_MODE_AZM_ZERO_MASK
*/

#define  LC_OP_MODE_TRUE_CAP_UNIT_CNV	0x80		// 2012-10-29 -WFC- true capacity unit conversion == 1.
#define  LC_OP_MODE_NET_GROSS			0x10
#define  LC_OP_MODE_TARE_AUTO_CLR1		0x02
#define  LC_OP_MODE_TARE_AUTO_CLR0		0x01

#define  LC_OP_MODE_TARE_AUTO_CLR_MASK	0x03

/// use with LC_OP_MODE_TARE_AUTO_CLR AND MASK.
#define  LC_TARE_AUTO_CLR_NO		0
#define  LC_TARE_AUTO_CLR_YES		1
#define  LC_TARE_AUTO_CLR_ON_TOTAL	2


#define  LC_STATUS_GOT_VALID_WEIGHT 0x80
#define  LC_STATUS_GOT_CAL_WEIGHT	0x40
#define  LC_STATUS_MOTION			0x20
#define  LC_STATUS_COZ				0x10
#define  LC_STATUS_OK_TO_TOTAL		0x08
#define  LC_STATUS_HAS_NEW_TOTAL	0x04
#define  LC_STATUS_OVERLOAD			0x02
#define  LC_STATUS_UNDERLOAD		0x01

#define  LC_STATUS2_UN_CAL			0x80
#define  LC_STATUS2_INPUTS_DISABLED	0x40
#define  LC_STATUS2_OVER_RANGE		0x20
#define  LC_STATUS2_UNER_RANGE		0x10
#define  LC_ANC3_NET_MODE			0x08
#define  LC_STATUS2_NET_MODE		0x08				// 2011-10-14 -WFC-
// 2010-10-15 -WFC- v
#define  LC_STATUS2_WRONG_MATH_EXPRESSION		0x04
#define  LC_STATUS2_ADC_BUSY		0x02
// 2010-10-15 -WFC- ^
#define  LC_STATUS2_GOT_NEW_PEAK_VALUE	0x01		// -WFC- 2011-03-28

// 2010-09-22 -WFC- v mainly for internal use
//#define		LC_STATUS3_GOT_ADC_CNT			0X80
//#define		LC_STATUS3_GOT_VALUE			0X40
#define		LC_STATUS3_GOT_PREV_VALID_VALUE		0X40		// 2011-05-09 -WFC- previously, it had a valid value.
#define		LC_STATUS3_GOT_UNFILTER_ADC_CNT		0X20
#define		LC_STATUS3_GOT_UNFILTER_VALUE		0X10
#define		LC_STATUS3_GOT_DATA_MASK			0XF0
#define		LC_STATUS3_SMALL_MOTION				0X08		// for use save power if no small motion has occurred in a given time window.
#define		LC_STATUS3_IN_HI_RESOLUTION			0X04		// 2011-07-08 -WFC-
// 2010-09-22 -WFC- ^


#define  LC_RUN_MODE_ENABLED			SENSOR_FEATURE_SENSOR_ENABLE
#define  LC_RUN_MODE_IN_CAL				0x40
#define  LC_RUN_MODE_NORMAL_ACTIVE		0x20
#define  LC_RUN_MODE_PEAK_HOLD_ENABLED	0x10				// -WFC- 2011-03-28
#define  LC_RUN_MODE_PENDING_ZERO		0x08
#define  LC_RUN_MODE_PENDING_TARE		0x04
#define  LC_RUN_MODE_PENDING_TOTAL		0x02
#define  LC_RUN_MODE_ZERO_ON_POWERUP	0x01

/// OK to count the lift event
#define  LC_CNT_STATUS_OK_CNT_LIFT		1
/// OK to count the 25% of capacity
#define  LC_CNT_STATUS_OK_CNT_25PCT_CAP	2
/// OK to count the overload.
#define  LC_CNT_STATUS_OK_CNT_OVERLOAD	4

#define LC_ADC_CNT_UNDER_RANGE_THRESHOLD	-8388600
#define LC_ADC_CNT_OVER_RANGE_THRESHOLD		8388600

/*!
  \brief loadcell zeroing data structure.
   
   
*/

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


/* ! This LC_TOTAL_TAG is moved to loadcell.h file because GCC cannot handle recursed include file.
  \brief loadcell total data structure.
   
 */

typedef struct  LC_TOTAL_TAG {
									/// latest qualified weight that can be total.
	float	qualifiedWt;
									/// last qualified weight added to total.
	float	lastWt;
									/// Accumulated qualified weight of NOT beging dropped of Load Drop Mode to be use for average computation.
	float	LDMAccWtUp;
									/// Accumulated qualified weight of begin dropped of Load Drop Mode to be use for average computation.
	float	LDMAccWtDown;
									/// event count of LDMAccWtUp
	UINT16	LDMAccWtUpCnt;
									/// event count of LDMAccWtDown;
	UINT16	LDMAccWtDownCnt;
									/// 90% of averge qualified weight.
	float	LDMavgQWt90pct;
									/// weight threshold to drop below before total allowed
	float	dropWtThreshold;
									/// weight threshold to rise above before total allowed
	float	riseWtThreshold;
									/// upper bound weight of on accept total mode.
	float	onAcceptUpperWt;
									/// lower bound weight of on accept total mode.
	float	onAcceptLowerWt;
									/// total pending timer. Try to total as long as pending time is NOT expired.
	TIMER_T pendingTimer;
									/// minimum stable period timer
	TIMER_T minStableTimer;
									/// minimum stable period before total.
	BYTE	minStableTime;
									/// status.
	BYTE	status;
}LC_TOTAL_T;


/*!
  \brief loadcell data structure.
   
   
*/

typedef struct  LOADCELL_TAG {
									/// previous raw weight.
	float	prvRawWt;	
									/// raw weight.
	float	rawWt;
									/// gross weight.
	float	grossWt;
									/// net weight.
	float	netWt;
									/// non filtered gross weight.
	float	grossWtUnFiltered;
									/// non filtered peak hold weight.
	float	peakHoldWt;				//  -WFC- 2011-03-28
									/// previous zero weight. It is use for undo zeroing without unload a load.
	float	prvZeroWt;
									/// pointer to zero weight. Its value will save to NVram everytime it has changed.
	float	*pZeroWt;
									/// pointer to tare weight. Its value will save to NVram everytime it has changed.
	float	*pTareWt;
									/// pointer to total weight. Its value will save to NVram everytime it has changed.
	float	*pTotalWt;
									/// pointer to sum of square total weight. Its value will save to NVram everytime it has changed.
	float	*pSumSqTotalWt;
									/// pointer to maximum total weight. Its value will save to NVram everytime it has changed.
	float	*pMaxTotalWt;
									/// pointer to minimum total weight. Its value will save to NVram everytime it has changed.
	float	*pMinTotalWt;
									/// pointer to number of total events. Its value will save to NVram everytime it has changed.
	UINT16	*pNumTotal;
									/// user configured operation mode such as motion detection, AZM, zero on powerup, net/gross(1/0), total  
	BYTE	*pOpModes;
									/// motion threshold weight. If delta weight > it, then the loadcell is in motion.
	float	motionThresholdWeight;
									/// pre motion weight.
	float	prvMotionWt;
									/// overload threshold weight for detect overload condition and for counting gawOverloadedCntFNV.
	float	overloadThresholdWt;
									/// weight threshold to drop below before it can count the service counters.
	float	dropWtThreshold;	
									/// weight threshold qualify as lift a load. It is for counting gaulLiftCntFNV lift counter.
	float	liftWtThreshold;	
									// weight of 25% capacity for counting gaw25perCapCntFNV counter.
	// 2010-08-30 -WFC- float	weightOf25pctCapacity;
									/// capacity for dispaly or viewing.
	float	viewCapacity;
									/// maximum Negative Displayable Value allow by legal for trade mode;	2015-08-03 -WFC-
	float	maxNegativeDisplayableValue;
									/// tare pending timer
	TIMER_T tarePendingTimer;
									/// motion detection period timer, once it expired, treat loadcell is stable. This give output a chance to tell the user that motion had been occured.
	TIMER_T motionDetectPeriodTimer;
									/// motion detection period time, once it expired, treat loadcell is stable. This give output a chance to tell the user that motion had been occured.
	BYTE	motionDetectPeriodTime;	
									/// pending time for zeroing, tare and total.
	BYTE	pendingTime;	
									/// loadcell running mode, enabled, in calibration, pending zero, tare, total, and zero on powerup etc...
	BYTE	runModes;
									/// total mode
	BYTE	totalMode;
									/// status valid weight, cal weight, in motion, center of zero, ok to total, overload,...
	BYTE	status;
									/// service counter status for control counting of lift, 25% of Capacity and overload counters.
	BYTE	serviceCnt_status;
									/// old unit. use for recompute weight when unit changed.
	BYTE	oldUnit;
									/// loadcell type, local physical, virtual math, remote loadcell etc... It use same as SENSOR_TYPE_  2010-06-18 -WFC-
	BYTE	type;
									/// pointer to calibration table structure.
	SENSOR_CAL_T *pCal;
									/// zeroing data structure,
	LC_ZERO_T	zeroT;
									/// total data structure,
	LC_TOTAL_T	totalT;
									/// countby for display or show weight data
	MSI_CB		viewCB;
									/// status2
	BYTE	status2;
									/// status3
	BYTE	status3;
									/// stable event counter counts number of consecutive stable event. It got clear when there is motion or initialize. 2011-11-17 -WFC-
	//BYTE	stableCnt;
									/// unused for even memory boundary. 2011-11-17 -WFC-
	BYTE	unused;

}LOADCELL_T;


/*

#define	TTL_MODE_DISABLED	0		// Total mode disabled.     
#define	TTL_MODE_AUTOLOAD	1		// Total mode auto, on load.
#define	TTL_MODE_AUTONORM	2		// Total mode auto.         
#define	TTL_MODE_AUTOPEAK	3		// Total mode auto, on peak.
#define	TTL_MODE_ONACCEPT	4		// Total mode on accept.    
#define	TTL_MODE_ONCOMAND	5		// Total mode when commanded
#define	TTL_MODE_LOADDROP	6		// Total mode load/drop mode

*/

/// user programmable capacity threshold lift counters.
extern UINT32	gaulLiftCntFNV[ MAX_NUM_PV_LOADCELL ];		// 2010-08-30 -WFC-
/// user lift counter is the same as gaulLiftCntFNV except it let user reset and not use to for service notification.
extern UINT32	gaulUserLiftCntFNV[ MAX_NUM_PV_LOADCELL ];		// 2014-10-17 -WFC-

// 5% of Capacity counters use a lift counters
// 2010-08-30 -WFC- extern UINT16	gawLiftCntFNV[ MAX_NUM_PV_LOADCELL ];

// 25% of Capacity counters
// 2010-08-30 -WFC- extern UINT16	gaw25perCapCntFNV[ MAX_NUM_PV_LOADCELL ];

/// Over Capacity counters or Overloaded counter.
extern UINT32	gaulOverloadedCntFNV[ MAX_NUM_PV_LOADCELL ];

// Over Capacity counters or Overloaded counter.
// 2010-08-30 -WFC- extern UINT16	gawOverloadedCntFNV[ MAX_NUM_PV_LOADCELL ];

///  percentage of capcity to meet lift count requirement. 0 == 0.5% of capacity; 1 == 1%. 2==2% etc..
extern	BYTE	gabLiftThresholdPctCapFNV[ MAX_NUM_PV_LOADCELL ];			// 2010-08-30 -WFC-

///  percentage of capcity as drop threshold. 0 == 0.5% of capacity; 1 == 1%. 2==2% etc..
extern	BYTE	gabDropThresholdPctCapFNV[ MAX_NUM_PV_LOADCELL ];			// 2010-08-30 -WFC-

// 2010-08-30 -WFC- v
/// service status, bit0: 1== excess service count. bit1: 1==require user acknowledge service request. bit2: 1== excess overload count; bit3: 1== require user acknowledge too overload events.
extern	BYTE	gabServiceStatusFNV[ MAX_NUM_PV_LOADCELL ];		// 2010-08-30 -WFC-

#define		LC_SERVICE_STATUS_LIFT_CNT_MET_SERVICE_CNT		0x01
#define		LC_SERVICE_STATUS_LIFT_CNT_NEED_USER_ACK		0x02
#define		LC_SERVICE_STATUS_OVERLOAD_CNT_MET_SERVICE_CNT	0x04
#define		LC_SERVICE_STATUS_OVERLOAD_CNT_NEED_USER_ACK	0x08

#define		LC_SERVICE_LIFT_CNT_INTERVAL		0x3FFF
#define		LC_SERVICE_OVERLOAD_CNT_INTERVAL	0x3FF
/// for test only
//#define		LC_SERVICE_LIFT_CNT_INTERVAL		0x7
//#define		LC_SERVICE_OVERLOAD_CNT_INTERVAL	0x7

// 2010-08-30 -WFC- ^

extern const float	gafLoadcellUnitsTbl[ MAX_NUM_LOADCELL_UNITS ][ MAX_NUM_LOADCELL_UNITS ] PROGMEM;
extern const float	gafLoadcellUnitsSquareTbl[ MAX_NUM_LOADCELL_UNITS ][ MAX_NUM_LOADCELL_UNITS ] PROGMEM;
extern const float	gafLoadcellCapacityUnitsTbl[ MAX_NUM_LOADCELL_UNITS ][ MAX_NUM_LOADCELL_UNITS ] PROGMEM;	// 2010-11-10 -WFC- unit conversion for capacity and countby.

extern	LOADCELL_T	gaLoadcell[ MAX_NUM_PV_LOADCELL ];
extern	BYTE		gabMotionDetectBand_dNV[ MAX_NUM_PV_LOADCELL ];
extern	BYTE		gabMotionDetectPeriodTimeNV[ MAX_NUM_PV_LOADCELL ];
extern	float		gafLcZeroOffsetWtNV[ MAX_NUM_PV_LOADCELL];
extern	BYTE		gbScaleStandardModeNV;
extern	BYTE		gab_Stable_Pending_TimeNV[ MAX_NUM_PV_LOADCELL];
extern	BYTE		gabLcOpModesFNV[ MAX_NUM_PV_LOADCELL ];

/// Rcal on enabled flag of a loadcell, 0==off, 1==on.
extern	BYTE	gabLoadcellRcalEnabled[ MAX_NUM_RCAL_LOADCELL ];


void	loadcell_compute_weight( BYTE sensorChannel );
BYTE	loadcell_config_hardware( BYTE sensorID  );
// 2012-02-06 -WFC- void	loadcell_detect_motion( LOADCELL_T *pLc );
void	loadcell_detect_motion( LOADCELL_T *pLc, BYTE lc );			// 2012-02-06 -WFC-
BYTE	loadcell_format_output( LOADCELL_T *pLc, char *pOutStr );
void	loadcell_1st_init( BYTE lc );
void	loadcell_init( BYTE lc );
void    loadcell_recompute_weights_unit( BYTE lc );
void	loadcell_tasks( BYTE lc );
void	loadcell_tracks_net_gross_tare( LOADCELL_T *pLc, BYTE lc );
void	loadcell_update_overload_threshold( LOADCELL_T *pLc );
void	loadcell_update_param( BYTE lc );
void	loadcell_change_unit( BYTE lc, BYTE unit );
BYTE	loadcell_get_value_of_type( BYTE sn, BYTE valueType, BYTE unit, float *pfV );
BYTE	loadcell_format_rcal_string( BYTE lc, char *pStr );

BYTE	loadcell_format_gnt_packet_output( char *pOutStr, BYTE lc );				// 2011-08-10 -WFC-
BYTE	loadcell_format_vcb_packet_output( char *pOutStr, BYTE lc, BYTE valueType );// 2012-02-13 -WFC-
BYTE	loadcell_format_weight_no_space( char *pOutStr, BYTE lc, BYTE valueType );	// 2012-02-13 -WFC-
void	loadcell_init_all_loadcells_zero_powerup( void );							// 2011-08-15 -WFC-
BYTE	loadcell_format_packet_output( LOADCELL_T *pLc, char *pOutStr, BYTE valueType, BYTE lc );		// 2012-02-23 -WFC-

#if ( CONFIG_PRODUCT_AS	!= CONFIG_AS_DSC || CONFIG_PRODUCT_AS != CONFIG_AS_HD || CONFIG_PRODUCT_AS != CONFIG_AS_HLI )		// 2011-05-19 -WFC-
float	loadcell_get_ccal_weight( BYTE lc );	// 2011-05-19 -WFC-
#endif

#endif		// end MSI_LOADCELL_H
//@}

