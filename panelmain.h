/*! \file panelmain.h \brief product specific panel device related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATMEGA1281
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2009/04/29 by Wai Fai Chin
// 
/// \ingroup product_module
/// \defgroup panelmain manage all behave of a panel of a product. (panelmain.c)
/// \code #include "panelmain.h" \endcode
/// \par Overview
///   This is a product specific panel device main module. It manages all aspect
///  of the product functionality such as panel user interface
///  between LED display and the keypad, etc..
//
// ****************************************************************************
//@{
 

#ifndef MSI_PANEL_MAIN_H
#define MSI_PANEL_MAIN_H

#include	"config.h"
#include	"spi.h"
#include	"pt.h"
#include	"led_display.h"
//#include	"panelmain.h"
#include	"timer.h"

#define  PANEL_RUN_MODE_NORMAL					SYS_RUN_MODE_NORMAL
#define  PANEL_RUN_MODE_IN_CNFG					SYS_RUN_MODE_IN_CNFG
#define  PANEL_RUN_MODE_SELF_TEST				SYS_RUN_MODE_SELF_TEST
#define  PANEL_RUN_MODE_ONE_SHOT_SELF_TEST		SYS_RUN_MODE_ONE_SHOT_SELF_TEST		// 2011-03-31 -WFC-
#define  PANEL_RUN_MODE_AUTO_SECONDARY_TEST		SYS_RUN_MODE_AUTO_SECONDARY_TEST	// 2011-03-31 -WFC-
#define  PANEL_RUN_MODE_UI_SECONDARY_TEST		SYS_RUN_MODE_UI_SECONDARY_TEST		// 2011-03-31 -WFC-
#define  PANEL_RUN_MODE_EXIT_CNFG_SAVE_FRAM		SYS_RUN_MODE_EXIT_CNFG_SAVE_FRAM
#define  PANEL_RUN_MODE_EXIT_CNFG_SAVE_ALL		SYS_RUN_MODE_EXIT_CNFG_SAVE_ALL
#define  PANEL_RUN_MODE_EXIT_CNFG_NO_SAVE		SYS_RUN_MODE_EXIT_CNFG_NO_SAVE
#define  PANEL_RUN_MODE_RF_REMOTE_LEARN			SYS_RUN_MODE_RF_REMOTE_LEARN		//	PHJ

#define  PANEL_RUN_MODE_PANEL_SETUP_MENU		100
#define  PANEL_RUN_MODE_CAL_LOADCELL			101
#define  PANEL_RUN_MODE_POWER_OFF				102
#define  PANEL_RUN_MODE_MASTER_RESET_MENU		103
#define  PANEL_RUN_MODE_COM_PORT_MENU			104			// 2011-06-30 -WFC-

#define  PANEL_NORMAL_RUN_MODE_STATE_NORMAL		0
#define  PANEL_NORMAL_RUN_MODE_STATE_TARE		1
#define  PANEL_NORMAL_RUN_MODE_STATE_TOTAL		2

#define	DISPLAY_WEIGHT_MODE_NORMAL		0
#define	DISPLAY_WEIGHT_MODE_TOTAL_X1000	1		//	PHJ	v
#define	DISPLAY_WEIGHT_MODE_TOTAL		3		//  cAN BE PUT IN ANY ORDER EXCEPT THAT X1000'S  if used
#define	DISPLAY_WEIGHT_MODE_COUNT		2		//  MUST BE 1 AHEAD LSD'S DISPLAY
#define	DISPLAY_WEIGHT_MODE_MAX_X1000	4		//	LAST IS THE LAST DISPLAYED BEFORE STARTING OVER
#define	DISPLAY_WEIGHT_MODE_MAX			5
#define	DISPLAY_WEIGHT_MODE_MIN_X1000	6
#define	DISPLAY_WEIGHT_MODE_MIN			7
#define	DISPLAY_WEIGHT_MODE_AVE_X1000	8
#define	DISPLAY_WEIGHT_MODE_AVE			9
#define	DISPLAY_WEIGHT_MODE_SD_X1000	10		//	SD_X1000, SD, & CoV NOT YET CALCULATED
#define	DISPLAY_WEIGHT_MODE_SD			11
#define	DISPLAY_WEIGHT_MODE_C0V			12
#define	DISPLAY_WEIGHT_MODE_LAST		2		//	PHJ	^ change to 12 when sD and CoV are added

#define DISPLAY_WEIGHT_MODE_TOTAL_OFF	15
#define DISPLAY_WEIGHT_MODE_TOTAL_ON	16
#define DISPLAY_WEIGHT_MODE_HIRES_OFF	17
#define DISPLAY_WEIGHT_MODE_HIRES_ON	18

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
	#define	PANEL_SETUP_LOADCELL_NUM	0
	#define PANEL_SETUP_DEFAULT_LISTENER	0
#else
	#define	PANEL_SETUP_LOADCELL_NUM		0
	#define PANEL_SETUP_DEFAULT_LISTENER	0
#endif

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2012-03-05 -WFC-
#include "vs_sensor.h"
/// main panel display object. 2012-03-05 -WFC-
typedef struct MAIN_PANEL_DISPLAY_CLASS_TAG {
											/// this thread
  struct pt 	m_pt;
											/// timer of this thread.
  TIMER_T		timer;						// timer of this thread.
											/// pointer to current focus virtual sensor.
  VS_SENSOR_INFO_T	*pVs;
											/// sensor or loadcell number
  BYTE			sn;
											/// key from parent
  BYTE			key;
} MAIN_PANEL_DISPLAY_CLASS;

#else
/// main panel display object. 2011-04-12 -WFC-
typedef struct MAIN_PANEL_DISPLAY_CLASS_TAG {
											/// this thread
  struct pt 	m_pt;
											/// timer of this thread.
  TIMER_T		timer;						// timer of this thread.
											/// pointer to current focus loadcell.
  LOADCELL_T	*pLc;
											/// sensor or loadcell number
  BYTE			lc;
											/// key from parent
  BYTE			key;
} MAIN_PANEL_DISPLAY_CLASS;
#endif


void	panel_main_init( void );
void	panel_main_main_tasks( void );
void	panel_main_display_float_in_integer( float	fV );		// 2011-04-22 -WFC-

// 2011-09-27 -WFC- #if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD )
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-09-27 -WFC-
void	panel_main_update_unit_annunciator_state( BYTE unit, BYTE state );

/** \def panel_main_update_unit_leds
 * It updates unit annunciator state either on or off based on unit input.
 *
 * @return none.
 *
 * History:  Created on 2011/07/05 by Wai Fai Chin
 */
#define panel_main_update_unit_leds( unit )		panel_main_update_unit_annunciator_state( (BYTE) (unit), LED_SEG_STATE_STEADY )

#else
	void	panel_main_update_unit_leds( BYTE unit );
#endif

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )	// 2012-02-23 -WFC-
	void panel_main_toggle_hires_display( LOADCELL_T *pLc, BYTE lc,  BYTE annc );	// 2012-02-23 -WFC-
#endif

/* 2011-03-31 -WFC- removed because of wrong declaration in h file and they don't need to be export to other file.
/// use by panel_main_xxxxx_run_mode_thread();
struct	pt		gPanelMainSelfTest_thread_pt;
struct	pt		gPanelMainRFRemote_thread_pt;
*/

/// panel_main system run mode uses for syncing with ScaleCore sys run mode.
extern BYTE gbPanelMainSysRunMode;
extern BYTE gbPanelDisplayWeightMode;

/// use by panel_main run mode.
extern BYTE gbPanelMainRunMode;

extern TIMER_T	gPanelMainRunModeThreadTimer;		// This timer use by many panel_main_xxxxx_run_mode_thread() because it runs one thread at a time.

/// display weight mode timer
extern TIMER_T	gPanelDisplayWeightModeTimer;		// panel display weight mode timer, for display total, or tare for a while then back to normal weight mode.

extern TIMER_T	gPanelMainLoopPauseTimer;			// This timer is use in loop pause for each loop by many panel_main_xxxxx_run_mode_thread() because it runs one thread at a time.

#endif  // MSI_PANEL_MAIN_H

//@}
