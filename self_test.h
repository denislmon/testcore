/*! \file self_test.h \brief self test of a product related functions.*/
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
// Software layer:  Middle layer
//
//  History:  Created on 2010/04/07 by Wai Fai Chin
//
/// \ingroup product_module
/// \defgroup self_test test all required feature of the ScaleCore based on product type. (self_test.c)
/// \code #include "self_test.h" \endcode
/// \par Overview
///   It is a high level module test all require ios, functions and features.
/// This is product specific module. Special custom software should modify this file and module.
//
// ****************************************************************************
//@{

#ifndef MSI_SELF_TEST_H_
#define MSI_SELF_TEST_H_

#include  "pt.h"
#include  "loadcell.h"

#define	LOADCELL_INPUT_STATUS_IS_GOOD						0
#define	LOADCELL_INPUT_STATUS_POS_SIG_LOWER_BRIDGE_OPEN		1
#define	LOADCELL_INPUT_STATUS_POS_SIG_UPPER_BRIDGE_OPEN		2
#define	LOADCELL_INPUT_STATUS_NEG_SIG_LOWER_BRIDGE_OPEN		3
#define	LOADCELL_INPUT_STATUS_NEG_SIG_UPPER_BRIDGE_OPEN		4
#define	LOADCELL_INPUT_STATUS_POS_SIG_EXCESSIVE_POS_OFFSET	5
#define	LOADCELL_INPUT_STATUS_POS_SIG_EXCESSIVE_NEG_OFFSET	6
#define	LOADCELL_INPUT_STATUS_NEG_SIG_EXCESSIVE_POS_OFFSET	7
#define	LOADCELL_INPUT_STATUS_NEG_SIG_EXCESSIVE_NEG_OFFSET	8
#define	LOADCELL_INPUT_STATUS_SPAN_POS_OFFSET_OUT_TOLERANCE_POS	9
#define	LOADCELL_INPUT_STATUS_SPAN_POS_OFFSET_OUT_TOLERANCE_NEG	10
#define	LOADCELL_INPUT_STATUS_BAD_OR_NO_ADC_CHIP	11			// -WFC- 2010-07-12

// 2010-10-15 -WFC- v
#define	SELF_TEST_ERROR_UNDEFINED_MODEL		30
#define	SELF_TEST_ERROR_UNDER_VOLTAGE		31
#define	SELF_TEST_ERROR_OVER_TEMPERATURE	32
#define	SELF_TEST_ERROR_UNDER_TEMPERATURE	33
// 2010-10-15 -WFC- ^

/// It uses for add the SENSOR_STATE_status. For example SELF_TEST_ERROR_SENSOR_BASE + LC_STATE_OVERLOAD = 60 + 9 = 69 to encode ERROR SENSOR OVERLOAD;
#define	SELF_TEST_ERROR_SENSOR_BASE		60


void	self_test_init( void );
void	self_test_init_and_start_thread( void );
void	self_test_main_tasks( void );
void	self_test_stop_all_threads( void );
void	self_test_one_shot_main_tasks( void );

char	self_test_main_thread( struct pt *pt );
BYTE	self_test_customer_special_format_value( BYTE sensorID, char *pOutStr );
BYTE	self_test_check_sys_error_code_for_user( BYTE sn, BYTE *pState, float *pfRound );
void 	self_test_update_sys_error_code_for_user( BYTE sn, BYTE normalMode);

void	self_test_time_critical_sys_behavior_update( void );	// 2011-05-12 -WFC-
void	self_test_set_event_timers_interval( void );			// 2011-06-15 -WFC-
void	self_test_timer_reset_event_timers( void );				// 2011-06-15 -WFC-

void	self_test_power_saving_init( void );					// 2014-09-12 -WFC-
void	self_test_detect_loadcell_motion( BYTE lc );			// 2014-09-15 -WFC-

extern	BYTE	gbSysErrorCodeForUser;			// 2010-12-29 -WFC-
extern	BYTE	gbSysPowerSaveState;			// 2014-09-09 -WFC-
extern	BYTE	gbTmpDisabledPowerSaving;		// 2014-11-05 -WFC-

extern	BYTE	gabLoadcellBridgeInputsStatus[ MAX_NUM_LOADCELL ];

#endif /* MSI_SELF_TEST_H_ */
//@}
