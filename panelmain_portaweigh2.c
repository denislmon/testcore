/*! \file panelmain_challenger3.c \brief product specific panel device related functions.*/
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
//   This is a product specific panel device main module. It manages all aspect
//  of the product functionality such as panel user interface
//  between LED display and the keypad, etc..
//
// ****************************************************************************
// 2011-06-10 -WFC- derived from panelmain.c into Challenger3 specific panel main module.


#include	"panelmain.h"
#include	"scalecore_sys.h"
#include	"led_display.h"
#include	"sensor.h"
#include	"lc_zero.h"
#include	"lc_tare.h"
#include	"setpoint.h"
#include	"panel_setup.h"
#include	"led_lcd_msg_def.h"
#include	"lc_total.h"
#include	"nv_cnfg_mem.h"

#include  <stdio.h>			// for sprintf_P(). without included <stdio.h>, it acts unpredictable or crash the program.
#include  "v_key.h"			// 2011-03-11 -WFC-
#include  "panel_test.h"	// 2011-04-11 -WFC-
#include  "print_string.h"	// 2015-09-24 -WFC-

/// use by panel_main_xxxxx_run_mode_thread(); This type of thread is running ONE of the kind at a time depends on run mode. Thus, this type of thread is reuse the same thread variable to save memory.
struct	pt		gPanelMainSelfTest_thread_pt;
// 2011-03-31 -WFC- struct	pt		gPanelMainRFRemote_thread_pt;
// aliased thread variable for easy reading source code.
#define		gPanelMainRFRemote_thread_pt		gPanelMainSelfTest_thread_pt		// 2011-03-31 -WFC-
#define		gPanelMain_total_display_thread_pt	gPanelMainSelfTest_thread_pt		// 2011-03-31 -WFC-

TIMER_T	gPanelMainRunModeThreadTimer;	// This timer use by many panel_main_xxxxx_run_mode_thread() because it runs one thread at a time.
TIMER_T	gPanelMainTaskTimer;			// main task timer use by panel_main_main_tasks().
TIMER_T	gPanelMainLoopPauseTimer;		// This timer is use in loop pause for each loop by many panel_main_xxxxx_run_mode_thread() because it runs one thread at a time.

/// Panel device system run mode uses for syncing with ScaleCore sys run mode.
BYTE gbPanelMainSysRunMode;

/// use by Panel device run mode.
BYTE gbPanelMainRunMode;

/// use by panel_main_update_display_data();
BYTE gbPanelDisplayWeightMode;

// 2014-11-03 -WFC- v
/// use by Panel device run status bit map flags.
BYTE gbPanelMain_run_status_bm;
#define PM_RUN_STATUS_SHOW_AUTO_TOTAL_STATUS	0x01
// 2014-11-03 -WFC- ^

/// display weight mode timer
TIMER_T	gPanelDisplayWeightModeTimer;		// panel display weight mode timer, for display total, or tare for a while then back to normal weight mode.

/** application use keyEvent variable instead of direct read of keys because
 a key has different meaning or function base on the run mode of the Challenger3.
 e.g. in numeric input mode, ZERO key use as clear or exit, TARE key as enter,
 USER key uses as number input.
*/

// BYTE gbKeyEvent;
	BYTE	gbColorFlag;

/// main panel display thread object. 2011-04-12 -WFC-
MAIN_PANEL_DISPLAY_CLASS gMainPanelDispalyThreadObj;

// private methods
char panel_main_selftest_thread( struct pt *pt );

char panel_main_remote_learn_thread( struct pt *pt );

char panel_main_one_shot_selftest_thread( struct pt *pt );		// 2011-04-06 -WFC-

char panel_main_total_mode_display_thread( MAIN_PANEL_DISPLAY_CLASS *pObj ); // 2011-04-12 -WFC-


void panel_main_normal_run_mode( void);
void panel_main_power_up( void );
void panel_main_power_off_mode( void);
void panel_main_update_display_data( LOADCELL_T *pLc );
void panel_main_normal_mode_handle_user_key( LOADCELL_T *pLc, BYTE lc, BYTE fKeyNum );
void panel_main_format_number_of_total( UINT16 n, BYTE *pStr );		// 2011-04-12	-WFC-

void panel_main_update_sleep_mode_display( void );				// 2012-09-27 -WFC-
// void panel_main_enabled_comports( void );	// 2014-10-03 -WFC-

/**
 * init Challenger specific stuff.
 *
 * History:  Created on 2009/04/29 by Wai Fai Chin
 */

void panel_main_init( void )
{
	gbPanelMainSysRunMode = gbPanelMainRunMode = SYS_RUN_MODE_NORMAL;
	gbPanelDisplayWeightMode = DISPLAY_WEIGHT_MODE_NORMAL;
	led_display_hardware_init();

	led_display_init();
	v_key_init();			// -WFC- 2011-03-11
	timer_mSec_set( &gPanelMainTaskTimer, TT_50mSEC);	

} // end panel_main_init()


/**
 * Main tasks entry for the Challenger3.
 * It manage tasks based on its run mode. The run mode can be change
 * by both key press or host command.
 *
 * @param  pbSysRunMode	-- pointer ScaleCore system run mode.
 *
 * History:  Created on 2009/04/29 by Wai Fai Chin
 * 2012-05-14 -WFC- com port menu uses the same thread as PANEL_RUN_MODE_CAL_LOADCELL, because menu object is clone able.
 * 2012-05-15 -WFC- It only has RF_RemoteKey learning if it has no XBEE. 2012-06-20 -DLM-
 * 2012-09-27 -WFC- Call panel_main_update_sleep_mode_display(); skip call panel_main_normal_run_mode() if LED is in sleep state. 2012-10-03 -DLM-
 */

void panel_main_main_tasks( void )
{
	if (!( SYS_STATUS_DURING_POWER_UP & gbSysStatus))	{
		if ( timer_mSec_expired( &gPanelMainTaskTimer ) )		{		// perform time sensitive or schedule tasks here.
			timer_mSec_set( &gPanelMainTaskTimer, TT_50mSEC);
			v_key_read();		//-WFC- 2011-03-11
		}
	}

	if ( PANEL_RUN_MODE_SELF_TEST != gbPanelMainRunMode &&
		PANEL_RUN_MODE_ONE_SHOT_SELF_TEST != gbPanelMainRunMode ) {	// if current run mode is not self test and
		if ( SYS_RUN_MODE_SELF_TEST == gbSysRunMode ||
			 SYS_RUN_MODE_ONE_SHOT_SELF_TEST == gbSysRunMode ) {	// if ScaleCore system run mode is now in self test, then
			gbPanelMainSysRunMode = gbPanelMainRunMode = gbSysRunMode;	// set Challenger3 in self test run mode.
			PT_INIT( &gPanelMainSelfTest_thread_pt );				// init self test thread.
		}
	}

	switch ( gbPanelMainRunMode ) {
		case PANEL_RUN_MODE_NORMAL:
				panel_main_normal_run_mode();
			break;
		case PANEL_RUN_MODE_ONE_SHOT_SELF_TEST:				// This is invoked by USER key.
				panel_main_one_shot_selftest_thread( &gPanelMainSelfTest_thread_pt );
			break;
		case PANEL_RUN_MODE_AUTO_SECONDARY_TEST:			// This is invoked by USER key or require User ack service info.
		case PANEL_RUN_MODE_UI_SECONDARY_TEST:
				// configuration menu group is single group. It is a list of top menu items, so just directly call top menu thread.
				panel_test_top_menu_secondary_test_thread( &gPanelSetupTopMenuObj );
				panel_test_background_tasks( &gPanelSetupTopMenuObj );
			break;
		case PANEL_RUN_MODE_SELF_TEST:								// This is invoked mainly by power up.
				if ( SYS_RUN_MODE_SELF_TEST != gbSysRunMode) {
					//TODO: if a zero or power key is pressed while in selftest mode, it will terminated the test mode.
					led_display_hardware_init();						// end test and init led display board.
					gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;
					PT_INIT( &gPanelMainSelfTest_thread_pt );		// Force to end panel_main_selftest_thread();
				}
				else {
					panel_main_selftest_thread( &gPanelMainSelfTest_thread_pt );
				}
			break;
		case PANEL_RUN_MODE_RF_REMOTE_LEARN:						//	PHJ	v
			#if ( CONFIG_RF_MODULE_AS == CONFIG_RF_MODULE_AS_NONE )		// 2012-05-15 -WFC-
				if ( PANEL_RUN_MODE_RF_REMOTE_LEARN != gbSysRunMode) {
					gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;
					PT_INIT( &gPanelMainRFRemote_thread_pt );		// Force to end panel_main_selftest_thread();
				}
				else {
					panel_main_remote_learn_thread( &gPanelMainRFRemote_thread_pt );
				}
			#endif
			break;													//	PHJ	^
		case PANEL_RUN_MODE_CAL_LOADCELL:
		case PANEL_RUN_MODE_COM_PORT_MENU:							// 2012-05-14 -WFC- com port menu uses the same thread because menu object is clone able.
				// cal menu is makeup groups of top menu lists.
				panel_setup_top_menu_group_thread( &gPanelSetupTopMenuGroupObj );
			break;
		case PANEL_RUN_MODE_PANEL_SETUP_MENU:
				// configuration menu group is single group. It is a list of top menu items, so just directly call top menu thread.
				panel_setup_top_menu_thread( &gPanelSetupTopMenuObj );
			break;
		case PANEL_RUN_MODE_POWER_OFF:
				panel_main_power_off_mode();				//	PHJ v
//				gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;	//	PHJ ^ was commented out 6/24/10	~~~~
			break;
		case PANEL_RUN_MODE_MASTER_RESET_MENU:
				panel_setup_simple_sub_menu_thread( &gPanelSetupSubMenuObj1 );
			break;
	}

	if ( !( PANEL_RUN_MODE_SELF_TEST == gbPanelMainRunMode ||
			PANEL_RUN_MODE_ONE_SHOT_SELF_TEST == gbPanelMainRunMode ||
			PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode ||
			PANEL_RUN_MODE_UI_SECONDARY_TEST == gbPanelMainRunMode ||
			PANEL_RUN_MODE_POWER_OFF == gbPanelMainRunMode)	 ) {
		panel_main_update_sleep_mode_display();							// 2012-09-27 -WFC- 2012-10-03 -DLM-
		led_display_manager();
	}

	gbPanelMainSysRunMode = gbSysRunMode;
} // end panel_main_main_tasks()


/**
 * Challenger specific self test thread.
 * 1. Turns on all LED segments for 1.5 seconds then turn off.
 * 2. display software version number.
 * 3. If either service counters requires user acknowledgement,
 * 		 it invokes service info object to display service counter and rcal numbers.
 *    else
 *       it ends self test and into normal operation.
 *
 * @param  pt		-- pointer to Proto thread structure.
 *
 * History:  Created on 2011-04-02 by Wai Fai Chin
 */

//PT_THREAD( panel_main_selftest_thread( struct pt *pt )) // Doxygen cannot handle this macro
char panel_main_selftest_thread( struct pt *pt )
{
  PT_BEGIN( pt );

	led_display_default_intensity();
    led_display_start_hw_display_test();			// set the LED driver chip in self test mode.
    timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_1p5SEC);    // Note this statment belongs to PT_BEGIN() which after the expanded statement case:0.
    // It only called by the very first time of excution of this thread and untill every timer expired event.
    PT_WAIT_UNTIL( pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for 1.5 seconds, then end LED driver self test mode.
	// normal operation, not in test mode
	led_display_stop_hw_display_test();

	led_display_format_output( gAppBootShareNV.softVersionStr );
	led_display_string( gLedDisplayManager.descr.str1 );
    timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_2SEC);
    PT_WAIT_UNTIL( pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for 1.5 seconds, then end LED driver self test mode.

	if ( gabServiceStatusFNV[0] & (LC_SERVICE_STATUS_LIFT_CNT_NEED_USER_ACK | LC_SERVICE_STATUS_OVERLOAD_CNT_NEED_USER_ACK ) ) {
		gbPanelMainSysRunMode = gbPanelMainRunMode =
		gbSysRunMode = PANEL_RUN_MODE_UI_SECONDARY_TEST;		// User interactive secondary test mode to force user pay attention.
		// construct gPanelSetupTopMenuObj into a thread.
		gPanelSetupTopMenuObj.msgID				=
		gPanelSetupTopMenuObj.curMove			=
		gPanelSetupTopMenuObj.msgDisplayTime	= 0;						// no msg to display at the beginning of the menu.
		gPanelSetupTopMenuObj.pMethod			= 0;						// no method.
		gPanelSetupTopMenuObj.parentIs 			= PSTMC_PARENT_NONE;		// no group parent, it is just a list of top menu item.
		gPanelSetupTopMenuObj.pRootMenu			= gacPanel_Top_Menu_Service_Info_Test;		// points to secondary Service Info test top menu.
		gPanelSetupTopMenuObj.maxNumOfItem		= PANEL_TEST_TOP_MENU_SERVICE_INFO_MAX_ITEMS;
		gPanelSetupTopMenuObj.disabledItemFlags = 0;	// enabled all menu items. 2011-04-26 -WFC-
		gPanelSetupTopMenuObj.disabledItemFlags = 0;	// enabled all menu items. 2011-04-26 -WFC-
		PT_INIT( &gPanelSetupTopMenuObj.m_pt );								// init panel setup top menu thread.
	}
	else {
		gbPanelMainRunMode = gbSysRunMode = PANEL_RUN_MODE_NORMAL;	// This is a single shot thread only. Once test is done, it goes back to normal mode.
		led_display_hardware_init();								// end test and init led display board.
		led_display_init();
	}
  PT_END(pt);
} // end panel_main_selftest_thread()


/**
 * one shot self test thread does the following:
 * 1. Turns on all LED segments.
 * 2. If TARE key is pressed,
 * 		 it invokes service info object to display service counter.
 *    else
 *       it invokes secondary test object.
 *
 * @param  pt	-- pointer to Proto thread structure.
 *
 * History:  Created on 2011-04-06 by Wai Fai Chin
 */

//PT_THREAD( panel_main_one_shot_selftest_thread( struct pt *pt )) // Doxygen cannot handle this macro
char panel_main_one_shot_selftest_thread( struct pt *pt )
{
  BYTE key;

  PT_BEGIN( pt );

	led_display_default_intensity();
	led_display_start_hw_display_test();			// set the LED driver chip in self test mode.
	timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_2SEC);    // Note this statement belongs to PT_BEGIN() which after the expanded statement case:0.
	// It only called by the very first time of execution of this thread and until every timer expired event.

	PT_WAIT_UNTIL( pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer) || panel_key_get( &key ));	// wait for delay OR a key pressed.
	// end led test mode and back in normal LED operation mode
	led_display_stop_hw_display_test();

	if ( (PANEL_MENU_ENTER_KEY == key ) ||														// if pressed the TARE key or
		 (gabServiceStatusFNV[0] & (LC_SERVICE_STATUS_LIFT_CNT_NEED_USER_ACK | LC_SERVICE_STATUS_OVERLOAD_CNT_NEED_USER_ACK )) ) { // require user to acknowledge service counters
		if ( gabServiceStatusFNV[0] & (LC_SERVICE_STATUS_LIFT_CNT_NEED_USER_ACK | LC_SERVICE_STATUS_OVERLOAD_CNT_NEED_USER_ACK ) ) { // if require user to acknowledge service counters
			gbPanelMainSysRunMode = gbPanelMainRunMode =
			gbSysRunMode = PANEL_RUN_MODE_UI_SECONDARY_TEST;		// User interactive secondary test mode to force user pay attention.
		}
		else {
			gbPanelMainSysRunMode = gbPanelMainRunMode =
			gbSysRunMode = PANEL_RUN_MODE_AUTO_SECONDARY_TEST;		// Auto secondary test, it displays each item one second at a time.
		}

		// construct gPanelSetupTopMenuObj into a thread.
		gPanelSetupTopMenuObj.pRootMenu			= gacPanel_Top_Menu_Service_Info_Test;		// points to secondary Service Info test top menu.
		gPanelSetupTopMenuObj.maxNumOfItem		= PANEL_TEST_TOP_MENU_SERVICE_INFO_MAX_ITEMS;
	}
	else if ( PANEL_USER_KEY == key ) {
		gbPanelMainSysRunMode = gbPanelMainRunMode =
		gbSysRunMode = PANEL_RUN_MODE_UI_SECONDARY_TEST;		// User Interactive secondary test mode.
		gPanelSetupTopMenuObj.pRootMenu			= gacPanel_Top_Menu_Secondary_Test;			// points to secondary test top menu.
		gPanelSetupTopMenuObj.maxNumOfItem		= PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS;
	}
	else {
		// construct gPanelSetupTopMenuObj into a thread.
		gbPanelMainSysRunMode = gbPanelMainRunMode =
		gbSysRunMode = PANEL_RUN_MODE_AUTO_SECONDARY_TEST;		// Auto secondary test, it displays each item one second at a time.
		gPanelSetupTopMenuObj.pRootMenu			= gacPanel_Top_Menu_Secondary_Test;			// points to secondary test top menu.
		gPanelSetupTopMenuObj.maxNumOfItem		= PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS;
	}

	// construct gPanelSetupTopMenuObj into a thread.
	gPanelSetupTopMenuObj.msgID				=
	gPanelSetupTopMenuObj.curMove			=
	gPanelSetupTopMenuObj.msgDisplayTime	= 0;						// no msg to display at the beginning of the menu.
	gPanelSetupTopMenuObj.pMethod			= 0;						// no method.
	gPanelSetupTopMenuObj.parentIs 			= PSTMC_PARENT_NONE;		// no group parent, it is just a list of top menu item.
	gPanelSetupTopMenuObj.disabledItemFlags = 0;	// enabled all menu items. 2011-04-26 -WFC-
	PT_INIT( &gPanelSetupTopMenuObj.m_pt );								// init panel setup top menu thread.

  PT_END(pt);
} // end panel_main_one_shot_selftest_thread()



/**														//	PHJ	v
 * Challenger specific remote learn thread.
 *
 * @param  pt			-- pointer to Proto thread structure.
 *
 * History:  Created on 2010/08/09 by Pete Jensen
 * 2013-04-26 -WFC- Use high level MACRO SET_RF_LEARN and CLEAR_RF_LEARN instead of low level direct pin toggle. //2013-09-03 -DLM
 */

//PT_THREAD( panel_main_remote_learn_thread( struct pt *pt )) // Doxygen cannot handle this macro
char panel_main_remote_learn_thread( struct pt *pt )
{
	// -WFC- 2011-03-16 UINT16			key;
	BYTE	key;		// -WFC- 2011-03-16
  
  PT_BEGIN( pt );

	// 2013-04-26 -WFC- PORTA |= RF_LEARN;						// Pulse High to Learn
	SET_RF_LEARN;		// 2013-04-26 -WFC- //2013-09-03 -DLM
	/* -WFC- 2011-03-09 v commented out Pete's codes.
	#if ( LED_ANC_LOC_RF )
		gLedDisplayManager.descr.baAnc[LED_ANC_LOC_RF - LOCOFF].state[LED_ANC_RF] = LED_SEG_STATE_STEADY;
	#endif							
	   -WFC- 2011-03-09 ^ */
	// -WFC- 2011-03-09 v
	#ifdef LED_DISPLAY_ANC_RF
		led_display_set_annunciator( LED_DISPLAY_ANC_RF, LED_SEG_STATE_STEADY);
	#endif
	// -WFC- 2011-03-09 ^
    timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_1p5SEC);    // Note this statment belongs to PT_BEGIN() which after the expanded statement case:0.
    // It only called by the very first time of excution of this thread and until every timer expired event.
    PT_WAIT_UNTIL( pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for 1.5 seconds, then end LED driver self test mode.
	// normal operation, not in test mode
	if ( panel_key_get( &key ) && key == PANEL_ZERO_KEY )	// if key is pressed want to erase RF_Remote
	{
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_10SEC);    // Note this statment belongs to PT_BEGIN() which after the expanded statement case:0.
		PT_WAIT_UNTIL( pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for 10 seconds, then end LED driver self test mode.
	}
	// 2013-04-26 -WFC- PORTA &= ~RF_LEARN;						// Release learn input to receiver decoder
	CLEAR_RF_LEARN;		// 2013-04-26 -WFC-  //2013-09-03 -DLM
	/* -WFC- 2011-03-09 v commented out Pete's codes.
	#if ( LED_ANC_LOC_RF )
		gLedDisplayManager.descr.baAnc[LED_ANC_LOC_RF - LOCOFF].state[LED_ANC_RF] = LED_SEG_STATE_OFF;
	#endif							
	   -WFC- 2011-03-09 ^ */
	// -WFC- 2011-03-09 v
	#ifdef LED_DISPLAY_ANC_RF
	led_display_set_annunciator( LED_DISPLAY_ANC_RF, LED_SEG_STATE_OFF );
	#endif
	// -WFC- 2011-03-09 ^

	gbPanelMainRunMode = gbSysRunMode = PANEL_RUN_MODE_NORMAL;	// This is a single shot thread only. Once learn is done, it goes back to normal mode.
	
  PT_END(pt);
} // end panel_main_remote_learn_thread()				//	PHJ	^


/**
 * Total mode display thread.
 *
 * @param  pObj	-- pointer to a data display object.
 *
 * History:  Created on 2011-04-12 by Wai Fai Chin
 * 2014-11-03 -WFC- In auto total mode, displays auto total status if run statu required it.
 */

//PT_THREAD( panel_main_total_mode_display_thread( struct pt *pObj )) // Doxygen cannot handle this macro
char panel_main_total_mode_display_thread( MAIN_PANEL_DISPLAY_CLASS *pObj )
{

  BYTE		bStrBuf[12];

  PT_BEGIN( &pObj->m_pt );
	gMainPanelDispalyThreadObj.key = V_KEY_NO_NEW_KEY;				// assumed no new key.
	// 2014-11-03 -WFC- v
	if ( pObj->pLc-> totalMode > LC_TOTAL_MODE_DISABLED &&
		 pObj->pLc-> totalMode < LC_TOTAL_MODE_LOAD_DROP )	{		// if in auto total modes,
		if ( PM_RUN_STATUS_SHOW_AUTO_TOTAL_STATUS & gbPanelMain_run_status_bm ) {
			gbPanelMain_run_status_bm &= ~PM_RUN_STATUS_SHOW_AUTO_TOTAL_STATUS;			// clear show auto total status
			if ( LC_TOTAL_STATUS_DISABLED_AUTO_MODES & pObj->pLc-> totalT.status ) 		// if auto total modes disabled,
				led_display_copy_string_and_pad_P ( gLedDisplayManager.descr.str1, gcStr_T_Off);
			else
				led_display_copy_string_and_pad_P ( gLedDisplayManager.descr.str1, gcStr_PrESS);

			gLedDisplayManager.descr.mode |= LED_DISPLAY_MODE_UPDATE_NOW;
			led_display_string( gLedDisplayManager.descr.str1 );	// display it now, not to wait for led_display_manage_string_thread.
			timer_mSec_set( &pObj->timer, TT_1p5SEC);
			PT_WAIT_UNTIL( &pObj->m_pt, timer_mSec_expired( &pObj->timer) || ( PANEL_ZERO_KEY == pObj->key) || ( PANEL_MENU_ENTER_KEY == pObj->key ));	// wait for delay OR a zero key, or tare key
			gbPanelDisplayWeightMode = DISPLAY_WEIGHT_MODE_NORMAL;
			PT_EXIT( &pObj->m_pt );										// exit
		}
	}
	// 2014-11-03 -WFC- ^

	#ifdef LED_DISPLAY_ANC_TOTAL
		led_display_set_annunciator( LED_DISPLAY_ANC_TOTAL, LED_SEG_STATE_STEADY);
	#endif
	led_display_format_high_resolution_float_string( *(pObj->pLc-> pTotalWt), &(pObj->pLc-> viewCB), TRUE, bStrBuf );
    led_display_format_output( bStrBuf );
	gLedDisplayManager.descr.mode |= LED_DISPLAY_MODE_UPDATE_NOW;
	led_display_string( gLedDisplayManager.descr.str1 );	// display it now, not to wait for led_display_manage_string_thread.
	timer_mSec_set( &pObj->timer, TT_1p5SEC);
	PT_WAIT_UNTIL( &pObj->m_pt, timer_mSec_expired( &pObj->timer) || ( PANEL_ZERO_KEY == pObj->key) || ( PANEL_MENU_ENTER_KEY == pObj->key ));	// wait for delay OR a zero key, or tare key

	if ( PANEL_ZERO_KEY == pObj->key) {
		lc_total_clear_total( pObj->pLc, pObj->lc );
		gMainPanelDispalyThreadObj.key = V_KEY_NO_NEW_KEY;			// clear current key.
		PT_EXIT( &pObj->m_pt );										// exit and it will execute from the beginning of this thread.
	}
	else if ( PANEL_MENU_ENTER_KEY == pObj->key )  {
		lc_total_remove_last_total( pObj->pLc, pObj->lc );			// remove last total and its event statistics.
		gMainPanelDispalyThreadObj.key = V_KEY_NO_NEW_KEY;			// clear current key.
		PT_EXIT( &pObj->m_pt );										// exit and it will execute from the beginning of this thread.
	}
	panel_main_format_number_of_total( *(pObj->pLc-> pNumTotal), bStrBuf);
    led_display_format_output( bStrBuf );
	gLedDisplayManager.descr.mode |= LED_DISPLAY_MODE_UPDATE_NOW;
	led_display_string( gLedDisplayManager.descr.str1 );	// display it now, not to wait for led_display_manage_string_thread.
	timer_mSec_set( &pObj->timer, TT_1p5SEC);
	PT_WAIT_UNTIL( &pObj->m_pt, timer_mSec_expired( &pObj->timer) || ( PANEL_ZERO_KEY == pObj->key) || ( PANEL_MENU_ENTER_KEY == pObj->key ));	// wait for delay OR a zero key, or tare key

	if ( PANEL_ZERO_KEY == pObj->key) {
		lc_total_clear_total( pObj->pLc, pObj->lc );
		panel_main_format_number_of_total( *(pObj->pLc-> pNumTotal), bStrBuf);
	    led_display_format_output( bStrBuf );
		led_display_string( gLedDisplayManager.descr.str1 );			// display it now, not to wait for led_display_manage_string_thread.
		gMainPanelDispalyThreadObj.key = V_KEY_NO_NEW_KEY;				// clear current key.
		timer_mSec_set( &pObj->timer, TT_1p5SEC);
		PT_WAIT_UNTIL( &pObj->m_pt, timer_mSec_expired( &pObj->timer));	// wait for delay.
	}
	else if ( PANEL_MENU_ENTER_KEY == pObj->key ) {
		lc_total_remove_last_total( pObj->pLc, pObj->lc );			// remove last total and its event statistics.
		gMainPanelDispalyThreadObj.key = V_KEY_NO_NEW_KEY;			// clear current key.
		PT_EXIT( &pObj->m_pt );										// exit and it will execute from the beginning of this thread.
	}

	#ifdef LED_DISPLAY_ANC_TOTAL
		led_display_set_annunciator( LED_DISPLAY_ANC_TOTAL, LED_SEG_STATE_OFF );
	#endif
	gbPanelDisplayWeightMode = DISPLAY_WEIGHT_MODE_NORMAL;

  PT_END( &pObj->m_pt );
} // end panel_main_one_shot_selftest_thread()

/**
 * Format number of total in string format.
 *
 * @param  n	-- integer to be format for display.
 * @param  pObj	-- pointer to string buffer for saved new formated string.
 *
 * History:  Created on 2011-04-12 by Wai Fai Chin
 */

void panel_main_format_number_of_total( UINT16 n, BYTE *pStr )
{
	#if ( LED_DISPLAY_MAX_DIGIT_LENGTH == 5 )
	if ( n > 9999 )
		sprintf_P( pStr, PSTR("%5d"), n );
	else if ( n > 999 )
		sprintf_P( pStr, PSTR("n%4d"), n );
	else
		sprintf_P( pStr, PSTR("n=%3d"), n );
	#elif ( LED_DISPLAY_MAX_DIGIT_LENGTH == 6 )
	if ( n > 9999 )
		sprintf_P( pStr, PSTR("n%5d"), n );
	else
		sprintf_P( pStr, PSTR("n=%4d"), n );
	#endif

} // end panel_main_format_number_of_total(,)



/**
 * Panel main normal run mode method.
 * It is mainly control run mode and display mode.
 *
 * History:  Created on 2011-04-12 by Wai Fai Chin
 * 2011-08-31 -WFC- Called panel_setup_construct_cnfg_menu_object() to handle Legal For Trade mode cnfg menu.
 * 2012-03-09 -WFC- Ensure some key only work on DISPLAY_WEIGHT_MODE_NORMAL mode.
 * 2012-05-10 -WFC- Added code to handle com port setup menu for RF setup etc...
 * 2014-10-03 -WFC- Called panel_main_enabled_comports() before setup menu.
 * 2014-10-03 -WFC- Called bios_enabled_comports_in_power_save_state() before setup menu.
 * 2014-11-03 -WFC- pressed total key set show auto total status flag.
 * 2015-05-11 -WFC- If it is in OIML mode, disabled zero key if it is in NET mode.
 */

void panel_main_normal_run_mode( void)
{
	BYTE 	key;
	BYTE	lc;
	BYTE 	okToProcess;					// 2015-05-11 -WFC-
	LOADCELL_T 				*pLc;			// points to a loadcell
  
	gMainPanelDispalyThreadObj.lc = lc = PANEL_SETUP_LOADCELL_NUM;			// hardcode for now... since Challenger3 only handle 1 loadcell.
	gMainPanelDispalyThreadObj.pLc =  pLc = &gaLoadcell[ lc ];
	if ( panel_key_get( &key ) ) {
		gMainPanelDispalyThreadObj.key = key;				// propagate key event to display thread obj
		switch ( key ) {
			case PANEL_ZERO_KEY:
				// 2015-05-11 -WFC- v
				okToProcess = YES;
				if ( (SCALE_STD_MODE_OIML == (gbScaleStandardModeNV & SCALE_STD_MODE_MASK))) {
					if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) {			// if in NET mode,
						okToProcess = NO;
					}
				}
				if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode ) {		// if in normal display mode,
					if ( okToProcess)
						lc_zero_by_command( lc );
				}
				// 2015-05-11 -WFC- ^

//					if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode )		// if in normal display mode,
//						lc_zero_by_command( lc );
			break;

			case PANEL_USER_KEY:
					if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode )	{ // if in normal display mode, 2012-03-09 -WFC-
						panel_main_normal_mode_handle_user_key( pLc, lc, 0 );
						if ( SYS_RUN_MODE_SELF_TEST == gbSysRunMode ||		// if user changed run mode from normal to self test,
								SYS_RUN_MODE_RF_REMOTE_LEARN == gbSysRunMode )	// PHJ
							return;											//  then return right the way, so it skip executing panel_main_update_display_data();
					}
				break;
			case PANEL_TARE_KEY:
					if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode ) {		// if in normal display mode,
						if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ) {	// if it is in NET mode,
							lc_tare_change_to_gross( pLc );					// 		set it in GROSS mode.
							nv_cnfg_fram_save_loadcell_dynamic_data( lc );
						}
						else												// else tare the gross weight.
							lc_tare_gross( pLc, lc );
					}
				break;
			case PANEL_SETUP_KEY:
					// 2011-08-31 -WFC- v
//					gbPanelMainRunMode = PANEL_RUN_MODE_PANEL_SETUP_MENU;
//					// construct gPanelSetupTopMenuObj into a thread.
//					gPanelSetupTopMenuObj.msgID				=
//					gPanelSetupTopMenuObj.msgDisplayTime	= 0;						// no msg to display at the beginning of the menu.
//					gPanelSetupTopMenuObj.pMethod			= 0;						// no method.
//					gPanelSetupTopMenuObj.pRootMenu			= gacPanel_Top_Menu_Cnfg;	// points to setup top menu.
//					gPanelSetupTopMenuObj.maxNumOfItem		= PANEL_TOP_MENU_CNFG_MAX_ITEMS;
//					gPanelSetupTopMenuObj.parentIs 			= PSTMC_PARENT_NONE;		// no group parent, it is just a list of top menu item.
//					gPanelSetupTopMenuObj.disabledItemFlags = 0;	// enabled all menu items. 2011-04-26 -WFC-
//					PT_INIT( &gPanelSetupTopMenuObj.m_pt );								// init panel setup top menu thread.
					if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode )	{ // if in normal display mode, 2012-03-09 -WFC-
						bios_enabled_comports_in_power_save_state();							// 2014-10-03 -WFC-
						panel_setup_construct_cnfg_menu_object();
						// 2011-08-31 -WFC- ^
					}
				break;
			case PANEL_MASTER_RESET_KEY:
					bios_enabled_comports_in_power_save_state();							// 2014-10-03 -WFC-
					gbPanelMainRunMode = PANEL_RUN_MODE_MASTER_RESET_MENU;
					gPanelSetupSubMenuObj1.pRootItem	= gacPanel_Menu_Master_Reset;
					gPanelSetupSubMenuObj1.maxNumOfItem	= MENU_MASTER_RESET_MAX_ITEMS;
					gPanelSetupSubMenuObj1.nextMove		= 1;		// default next move is one step.
					gPanelSetupSubMenuObj1.msgDisplayTime= 0;		// assume no msg to be display.
					// init object variables at the very first time of this thread.
					gbPanelSetupStatus = 0;		// clear status.
					PT_INIT( &gPanelSetupSubMenuObj1.m_pt );		// init panel setup sub menu thread.
				break;
			case PANEL_CAL_KEY:
					bios_enabled_comports_in_power_save_state();							// 2014-10-03 -WFC-
					#ifdef  LED_DISPLAY_ANC_PEAK
						led_display_set_annunciator( LED_DISPLAY_ANC_PEAK, LED_SEG_STATE_OFF);
						// peak hold disabled is taking care by panel_setup_does_it_allows_new_cal(), panel_setup_does_it_allows_rcal_recal() and cmd_is_ok_start_new_cal().
					#endif
					// 2011-04-27 -WFC- v
					//	panel_setup_construct_cal_menu_object();
					if ( CAL_STATUS_COMPLETED == pLc->pCal->status )			// If calibration had completed, then
						panel_setup_construct_recal_menu_object();				// just recal
					else
						panel_setup_construct_cal_menu_object();				// else do a new cal
					// 2011-04-27 -WFC- ^
				break;
			case PANEL_POWER_KEY:
					// set power down event here:
					gbPanelMainRunMode = PANEL_RUN_MODE_POWER_OFF;
//					gbBiosRunStatus |= BIOS_RUN_STATUS_POWER_OFF;		PHJ
					led_display_power_off();						//	PHJ
					soft_delay( 50 );		// delay 50 milliseconds, so the LED driver chip has time to react to initialization commands before the power supply cut off the power to it..
					//timer_mSec_set( &gPanelDisplayWeightModeTimer, TT_50mSEC);
					//while ( !timer_mSec_expired( &gPanelMainTaskTimer ));		// delay 50 milliseconds, so the LED driver chip has time to react to initialization commands before the power supply cut off the power to it..
					TURN_OFF_RS232_CHIP;	// this also turn off LED driver chip power supply too.
					// 2013-05-06 -WFC- bios_power_off_key_on();						//	PHJ
					bios_power_off_by_power_key();		// 2013-05-06 -WFC-
//					return;											 	
				break;
			case PANEL_USER2_KEY:
					if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode )	{ // if in normal display mode, 2012-03-09 -WFC-
						panel_main_normal_mode_handle_user_key( pLc, lc, 1 );
						if ( SYS_RUN_MODE_SELF_TEST == gbSysRunMode ||		// if user changed run mode from normal to self test,
							SYS_RUN_MODE_RF_REMOTE_LEARN == gbSysRunMode )	// PHJ
							return;											// then return right the way, so it skip executing panel_main_update_display_data();
					}
				break;
			case PANEL_VIEW_KEY:
					if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode )	{ // if in normal display mode, 2012-03-09 -WFC-
						gbPanelDisplayWeightMode = DISPLAY_WEIGHT_MODE_TOTAL;
						PT_INIT( &gMainPanelDispalyThreadObj.m_pt );
					}
				break;
			case PANEL_TOTAL_KEY:
					if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode )	{ // if in normal display mode, 2012-03-09 -WFC-
						gbPanelMain_run_status_bm |= PM_RUN_STATUS_SHOW_AUTO_TOTAL_STATUS;		// set show auto total status 2014-11-03 -WFC-
						if ( LC_TOTAL_MODE_ON_COMMAND == pLc-> totalMode ) {
							lc_total_evaluate( pLc, lc );
						}
						lc_total_handle_command( lc );
						if ( !(LC_STATUS_HAS_NEW_TOTAL & pLc-> status )) {		// if no new total, tell panel_main_update_display_data() to display existing current total weight.
							gbPanelDisplayWeightMode = DISPLAY_WEIGHT_MODE_TOTAL;
							PT_INIT( &gMainPanelDispalyThreadObj.m_pt );
						}
						else {
							#ifdef LED_DISPLAY_ANC_ACKOUT
							led_display_set_annunciator( LED_DISPLAY_ANC_ACKOUT, LED_SEG_STATE_BLINK_FOUR);
							#endif
						}
					}
				break;
				// 2012-05-10 -WFC- v
			case PANEL_KEY_COM_PORT_SETUP_KEY:						// This key invokes a menu to setup com port and print string settings.
					bios_enabled_comports_in_power_save_state();							// 2014-10-03 -WFC-
					if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode )	{ // if in normal display mode, 2012-03-09 -WFC-
						panel_setup_construct_com_port_menu_object();
					}
				break;
				// 2012-05-10 -WFC- ^
			case PANEL_NEG_G_KEY:
					if ( DISPLAY_WEIGHT_MODE_NORMAL == gbPanelDisplayWeightMode )	{ // if in normal display mode, 2012-03-09 -WFC-
						lc_tare_toggle_net_gross( pLc, lc );
					}
				break;
		}
	}

	panel_main_update_display_data( pLc );			// update weight data and annunciators by fill in the gLedDisplayManager.descr behavior descriptor.
} // end panel_main_normal_run_mode()

/**
 * Handle user key function in normal run mode.
 *
 * @param	pLc		-- pointer to loadcell structure;
 * @param  	lc		-- loadcell index.
 * @param	fKeyNum	-- function key number
 *
 * History:  Created on 2011-04-12 by Wai Fai Chin
 * 2011-04-25 -WFC- Use viewing unit instead of reference unit of showing. This fixed a bug that cannot change unit from keypad.
 * 2011-07-14 -WFC- fixed A bug that Total key also trigger unit changed.
 * 2011-09-01 -WFC- Only allows non Legal For Trade mode to perform Peak Hold function.
 * 2012-05-15 -WFC- It only has RF_RemoteKey learning if it has no XBEE. 2012-06-20 -DLM-
 * 2014-10-03 -WFC- When switched to peak hold mode, turn on RF device.
 * 2014-11-03 -WFC- pressed total key set show auto total status flag.
 */

void panel_main_normal_mode_handle_user_key( LOADCELL_T *pLc, BYTE lc, BYTE fKeyNum )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	BYTE unit;

	switch ( gtSystemFeatureFNV.userKeyFunc[ fKeyNum ] ) {
		case FUNC_KEY_TEST:
				gbPanelMainSysRunMode = gbPanelMainRunMode =
				gbSysRunMode = PANEL_RUN_MODE_ONE_SHOT_SELF_TEST;	// set Challenger3 in self test run mode.
				PT_INIT( &gPanelMainSelfTest_thread_pt );			// init self test thread.
			break;
		case FUNC_KEY_TOTAL:
				gbPanelMain_run_status_bm |= PM_RUN_STATUS_SHOW_AUTO_TOTAL_STATUS;		// set show auto total status 2014-11-03 -WFC-
				if ( LC_TOTAL_MODE_ON_COMMAND == pLc-> totalMode ) {
					lc_total_evaluate( pLc, lc );
				}
				lc_total_handle_command( lc );
				if ( !(LC_STATUS_HAS_NEW_TOTAL & pLc-> status )) {		// if no new total, tell panel_main_update_display_data() to display existing current total weight.
					gbPanelDisplayWeightMode = DISPLAY_WEIGHT_MODE_TOTAL;
					PT_INIT( &gMainPanelDispalyThreadObj.m_pt );
				}
				#ifdef LED_DISPLAY_ANC_ACKOUT			// fixed Pete's bug that Total key also trigger unit changed. 2011-07-14 -WFC-
				else
					//2011-07-14 -WFC- #ifdef LED_DISPLAY_ANC_ACKOUT
					led_display_set_annunciator( LED_DISPLAY_ANC_ACKOUT, LED_SEG_STATE_BLINK_FOUR);
				#endif
			break;
		case FUNC_KEY_UNIT:
				if	( ( gbScaleStandardModeNV & SCALE_STD_MODE_MASK ) <= SCALE_STD_MODE_NTEP )	//	PHJ	okey to switch units
				{	
					// 2011-04-25 -WFC- if ( SENSOR_UNIT_LB == gabSensorShowCBunitsFNV[ lc ] )
					if ( SENSOR_UNIT_LB == gabSensorViewUnitsFNV[ lc ] )			// 2011-04-25 -WFC- Use viewing unit instead of reference unit of showing. This fixed a bug that cannot change unit from keypad.
						unit = SENSOR_UNIT_KG;
					else
						unit = SENSOR_UNIT_LB;
					loadcell_change_unit( lc, unit );
					#ifdef LED_DISPLAY_ANC_ACKOUT
						led_display_set_annunciator( LED_DISPLAY_ANC_ACKOUT, LED_SEG_STATE_BLINK_TWICE);
					#endif
				}
			break;
		case FUNC_KEY_NET_GROSS:
				lc_tare_toggle_net_gross( pLc, lc );
			break;
			// 2015-09-24 -WFC- v
		case FUNC_KEY_PRINT:
				gbIsPrintStr_CmdPressed = TRUE;
			break;
			// 2015-09-24 -WFC- ^
		case FUNC_KEY_PHOLD:
			unit = gbScaleStandardModeNV & SCALE_STD_MODE_MASK;						// 2011-09-01 -WFC- use unit variable as stdMode variable.
			if ( SCALE_STD_MODE_INDUSTRY == unit || SCALE_STD_MODE_1UNIT == unit ) { // 2011-09-01 -WFC- Only allows non Legal For Trade mode to perform Peak Hold function.
				pSnDesc = &gaLSensorDescriptor[ lc ];
				pLc-> peakHoldWt = 0.0;
				pLc-> status2 &= ~LC_STATUS2_GOT_NEW_PEAK_VALUE;
				pSnDesc-> maxRawADCcount = -2147483648;						// This will force loadcell to compute a new peak hold value.
				pSnDesc-> status &= ~SENSOR_STATUS_GOT_NEW_ADC_PEAK;		// clear got a new adc peak flag.
				if ( pLc-> runModes &  LC_RUN_MODE_PEAK_HOLD_ENABLED )  {
					pLc-> runModes &= ~LC_RUN_MODE_PEAK_HOLD_ENABLED;		// disabled peak hold mode
				}
				else {
					// 2014-10-03 -WFC- v
					#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )
						BIOS_TURN_ON_XBEE;
					#endif
					// 2014-10-03 -WFC- ^

					pLc-> runModes |= LC_RUN_MODE_PEAK_HOLD_ENABLED;		// enabled peak hold mode
					#ifdef  LED_DISPLAY_ANC_PEAK
						led_display_set_annunciator( LED_DISPLAY_ANC_PEAK, LED_SEG_STATE_STEADY);
					#endif
				}
				adc_lt_construct_op_desc( pSnDesc, lc );		//construct ADC operation descriptor.
			}
			break;
		case FUNC_KEY_LEARN:									//	PHJ	v
			#if ( CONFIG_RF_MODULE_AS == CONFIG_RF_MODULE_AS_NONE )		// 2012-05-15 -WFC-
				gbPanelMainSysRunMode = gbPanelMainRunMode =
				gbSysRunMode = SYS_RUN_MODE_RF_REMOTE_LEARN;	// set Challenger3 in learn run mode.
				// 2011-03-31 -WFC- removed a bug. PT_INIT( &gPanelMainSelfTest_thread_pt );		// init RF_Remote learn thread.
				PT_INIT( &gPanelMainRFRemote_thread_pt );		// // 2011-03-31 -WFC- 	init RF_Remote learn thread.
			#endif
			break;												//	PHJ	^
		case FUNC_KEY_VIEW_TOTAL:
				gbPanelDisplayWeightMode = DISPLAY_WEIGHT_MODE_TOTAL;
				PT_INIT( &gMainPanelDispalyThreadObj.m_pt );
			break;
	} // end switch{}
} // end panel_main_normal_mode_handle_user_key(,,)

/**
 * Update weight data and annunciators based on current focused loadcell and other status
 *
 * @param  pLc	-- pointer to loadcell structure; 
 *
 * @post   updated weight data and annunciators of the Challenger3 display board.
 *
 * History:  Created on 2009-06-10 by Wai Fai Chin
 * 2011-04-12 -WFC- Rewrote it with panel_main_total_mode_display_thread().
 * 2011-05-02 -WFC- Set "unCal" through gLedDisplayManager instead of direct display.
 * 2011-08-24 -WFC- Display "Lc.OFF" if this loadcell is disabled.
 * 2011-11-22 -WFC- Only none NTEP mode use COZ because not require by NTEP.
 */

void panel_main_update_display_data( LOADCELL_T *pLc )
{
	BYTE	ancOnIndex	= 0;
	BYTE	ancOffIndex	= 0;
	BYTE	bStrBuf[24];
	float	fV;

	panel_main_update_unit_leds(pLc-> viewCB.unit);

	#ifdef LED_DISPLAY_ANC_BAT
		if ( BIOS_SYS_STATUS_VOLTAGE_BLINK_WARNING & gbBiosSysStatus )
			led_display_set_annunciator( LED_DISPLAY_ANC_BAT, LED_SEG_STATE_KEEP_BLINKING);
		else if ( BIOS_SYS_STATUS_UNDER_VOLTAGE & gbBiosSysStatus )
			led_display_set_annunciator( LED_DISPLAY_ANC_BAT, LED_SEG_STATE_STEADY);
		else
			led_display_set_annunciator( LED_DISPLAY_ANC_BAT, LED_SEG_STATE_OFF);
	#endif

	if ( gbIsKeyStuck ) {
		if ( LED_DISPLAY_MODE_ERROR_MSG != ( gLedDisplayManager.descr.mode & LED_DISPLAY_MODE_STATUS_MASK ))
			PT_INIT( &gLedDisplayManager.string_pt );						// reset string display tread to guarantee it executes in the correct order.
		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_ERROR_MSG;		// set to display error message mode
		//strcpy_P ( gLedDisplayManager.descr.str1, gcStr_Error); 2012-07-16 -DLM-
		//strcpy_P ( gLedDisplayManager.descr.str2, gcStr_buttn); 2012-07-16 -DLM-
		led_display_copy_string_and_pad_P ( gLedDisplayManager.descr.str1, gcStr_Error);
		led_display_copy_string_and_pad_P ( gLedDisplayManager.descr.str2, gcStr_buttn);
	}
	// 2011-08-24 -WFC- v
	else if ( !((pLc-> runModes) & LC_RUN_MODE_ENABLED) ) {			// if this channel is disabled,
		led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, gcStr_Lc_OFF);	// displays "Lc.OFF"
	}
	// 2011-08-24 -WFC- ^
	else if ( (pLc-> runModes) & LC_RUN_MODE_IN_CAL ) {			// if this channel is in calibration mode,
		led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, gcStr_CAL);	// displays "  CAL"
	}
	else if ( CAL_STATUS_COMPLETED == pLc-> pCal-> status )	{	// if this loadcell has a calibration table.

		if ( LC_STATUS_HAS_NEW_TOTAL & pLc-> status ) {
			gbPanelDisplayWeightMode = DISPLAY_WEIGHT_MODE_TOTAL;
			PT_INIT( &gMainPanelDispalyThreadObj.m_pt );
			pLc-> status &= ~LC_STATUS_HAS_NEW_TOTAL;					// one shot only.
		}
		else if (	LC_TOTAL_STATUS_NEW_BLINK_EVENT & pLc-> totalT.status ) {
			#ifdef LED_DISPLAY_ANC_TOTAL
				led_display_set_annunciator( LED_DISPLAY_ANC_TOTAL, LED_SEG_STATE_BLINK_FOUR );
			#endif
			pLc-> totalT.status &= ~LC_TOTAL_STATUS_NEW_BLINK_EVENT;			// re-arm new total blink event flag.
		}
		if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) ||
//		if ( LC_STATUS_GOT_VALID_WEIGHT & (pLc-> status) || (LC_STATUS3_GOT_PREV_VALID_VALUE & (pLc-> status3)) || // 2011-05-09 -WFC-
			 (DISPLAY_WEIGHT_MODE_NORMAL != gbPanelDisplayWeightMode ))	  {		//	PHJ	if this loadcell has valid value, or in Total display mode, then
			if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) ){		// if loadcell is in NET mode
				fV = pLc-> netWt;
				ancOnIndex	=	LED_SEG_STATE_STEADY;
				ancOffIndex	=	LED_SEG_STATE_OFF;
			}
			else {
				fV = pLc-> grossWt;
				ancOnIndex	=	LED_SEG_STATE_OFF;
				ancOffIndex =	LED_SEG_STATE_STEADY;
			}

			if ( pLc-> runModes &  LC_RUN_MODE_PEAK_HOLD_ENABLED )  {
				fV = pLc-> peakHoldWt;
				#ifdef  LED_DISPLAY_ANC_PEAK // -DLM- 2012-03-15
					led_display_set_annunciator( LED_DISPLAY_ANC_PEAK, LED_SEG_STATE_STEADY);
				#endif
			}
			else {
				#ifdef  LED_DISPLAY_ANC_PEAK // -DLM- 2012-03-15
					led_display_set_annunciator( LED_DISPLAY_ANC_PEAK, LED_SEG_STATE_OFF);
				#endif
			}

			#ifdef LED_DISPLAY_ANC_NET
				led_display_set_annunciator( LED_DISPLAY_ANC_NET, ancOnIndex );
			#endif
			#ifdef LED_DISPLAY_ANC_GROSS
				led_display_set_annunciator( LED_DISPLAY_ANC_GROSS, ancOffIndex );
			#endif

			if ( LC_STATUS_MOTION & (pLc-> status)  ) {
				ancOnIndex	= LED_SEG_STATE_STEADY;
				ancOffIndex	= LED_SEG_STATE_OFF;
			}
			else {
				ancOnIndex	= LED_SEG_STATE_OFF;
				ancOffIndex	= LED_SEG_STATE_STEADY;
			}

			#ifdef  LED_DISPLAY_ANC_STABLE // -DLM- 2012-03-15
				led_display_set_annunciator( LED_DISPLAY_ANC_STABLE, ancOffIndex );
			#endif

			#ifdef LED_DISPLAY_ANC_MOTION
				led_display_set_annunciator( LED_DISPLAY_ANC_MOTION, ancOnIndex );
			#endif

			#ifdef LED_DISPLAY_ANC_COZ
//2012-02-06 -WFC-				if ( SCALE_STD_MODE_NTEP != ( gbScaleStandardModeNV & SCALE_STD_MODE_MASK ) ) {		// 2011-11-22 -WFC- Only none NTEP mode use COZ because not require by NTEP.
					if ( LC_STATUS_COZ & (pLc-> status)  ) {
						ancOnIndex = LED_SEG_STATE_STEADY;
					}
					else
						ancOnIndex = LED_SEG_STATE_OFF;
					led_display_set_annunciator( LED_DISPLAY_ANC_COZ, ancOnIndex );
//2012-02-06 -WFC-				}
			#endif


			if ( LC_STATUS_OVERLOAD & (pLc-> status)  ) {
				if ( LED_DISPLAY_MODE_ERROR_MSG != ( gLedDisplayManager.descr.mode & LED_DISPLAY_MODE_STATUS_MASK ))
					PT_INIT( &gLedDisplayManager.string_pt );						// reset string display tread to guarantee it executes in the correct order.
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_ERROR_MSG;		// set to display error message mode
				// strcpy_P ( gLedDisplayManager.descr.str1, gcStr_Error); 2012-07-16 -DLM-
				// strcpy_P ( gLedDisplayManager.descr.str2, gcStr_Load);  2012-07-16 -DLM-
				led_display_copy_string_and_pad_P ( gLedDisplayManager.descr.str1, gcStr_Error);
				led_display_copy_string_and_pad_P ( gLedDisplayManager.descr.str2, gcStr_Load);
			}

			else if ( LC_STATUS_UNDERLOAD & (pLc-> status)  ) {
				if ( LED_DISPLAY_MODE_ERROR_MSG != ( gLedDisplayManager.descr.mode & LED_DISPLAY_MODE_STATUS_MASK ))
					PT_INIT( &gLedDisplayManager.string_pt );						// reset string display tread to guarantee it executes in the correct order.
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_ERROR_MSG;		// set to display error message mode
				// strcpy_P ( gLedDisplayManager.descr.str1, gcStr_Error); 2012-07-16 -DLM-
				// strcpy_P ( gLedDisplayManager.descr.str2, gcStr_UnLd);  2012-07-16 -DLM-
				led_display_copy_string_and_pad_P ( gLedDisplayManager.descr.str1, gcStr_Error);
				led_display_copy_string_and_pad_P ( gLedDisplayManager.descr.str2, gcStr_UnLd);
			}
			else {
				if ( DISPLAY_WEIGHT_MODE_TOTAL == gbPanelDisplayWeightMode && !(pLc-> runModes &  LC_RUN_MODE_PEAK_HOLD_ENABLED)) { // total mode and not in peak hold mode.
					panel_main_total_mode_display_thread( &gMainPanelDispalyThreadObj );
				}
				else {
					if ( LED_DISPLAY_MODE_NORMAL != gLedDisplayManager.descr.mode )
						PT_INIT( &gLedDisplayManager.string_pt );						// reset string display tread to guarantee it executes in the correct order.
					gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;			// set normal display mode.
					// led_display_format_float_string( fV, &(pLc-> viewCB), FALSE, bStrBuf );
					led_display_format_high_resolution_float_string( fV, &(pLc-> viewCB), TRUE, bStrBuf ); // x1k enabled 2012-05-02 -DLM-
					led_display_format_output( bStrBuf );
				}
			}
		}
	}
	else {
		// 2011-05-02 -WFC- led_display_string_P( gcStr_unCal );
		led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, gcStr_unCal);	// displays " unCAL" 2011-05-02 -WFC-
	}

	#ifdef LED_DISPLAY_ANC_SP1
	if ( gSP_Registry & SP_LATCH_STATE_1 )
		ancOnIndex = LED_SEG_STATE_KEEP_BLINKING;
	else
		ancOnIndex = LED_SEG_STATE_OFF;
	led_display_set_annunciator( LED_DISPLAY_ANC_SP1, ancOnIndex );
	#endif


	#ifdef LED_DISPLAY_ANC_SP2
	if ( gSP_Registry & SP_LATCH_STATE_2 )
		ancOnIndex = LED_SEG_STATE_KEEP_BLINKING;
	else
		ancOnIndex = LED_SEG_STATE_OFF;

	led_display_set_annunciator( LED_DISPLAY_ANC_SP2, ancOnIndex );
	#endif

	#ifdef LED_DISPLAY_ANC_SP3
	if ( gSP_Registry & SP_LATCH_STATE_3 )
		ancOnIndex = LED_SEG_STATE_KEEP_BLINKING;
	else
		ancOnIndex = LED_SEG_STATE_OFF;

	led_display_set_annunciator( LED_DISPLAY_ANC_SP3, ancOnIndex );
	#endif

} // end panel_main_update_display_data();

/**
 * Panel main power off mode method.
 *
 * History:  Created on 2009/08/12 by Wai Fai Chin
 * 2011-08-31 -WFC- Called panel_setup_construct_cnfg_menu_object() to handle Legal For Trade mode cnfg menu.
 * 2013-11-08 -WFC- empty key buffer clear previous pressed keys after called panel_main_power_up(). 2013-11-13 -DLM-
 *
 */

void panel_main_power_off_mode( void)
{
	BYTE key;

 	if (panel_key_get( &key ) ) {	//	if got a new key
		switch ( key ) {
			case PANEL_SETUP_KEY:
					// 2011-08-31 -WFC- v
//					gbPanelMainRunMode = PANEL_RUN_MODE_PANEL_SETUP_MENU;
//					// construct gPanelSetupTopMenuObj into a thread.
//					gPanelSetupTopMenuObj.msgID				=
//					gPanelSetupTopMenuObj.msgDisplayTime	= 0;						// no msg to display at the beginning of the menu.
//					gPanelSetupTopMenuObj.pMethod			= 0;						// no method.
//					gPanelSetupTopMenuObj.pRootMenu			= gacPanel_Top_Menu_Cnfg;		// points to setup top menu.
//					gPanelSetupTopMenuObj.maxNumOfItem		= PANEL_TOP_MENU_CNFG_MAX_ITEMS;
//					gPanelSetupTopMenuObj.parentIs 			= PSTMC_PARENT_NONE;		// no group parent, it is just a list of top menu item.
//					gPanelSetupTopMenuObj.disabledItemFlags = 0;	// enabled all menu items. 2011-04-26 -WFC-
//					PT_INIT( &gPanelSetupTopMenuObj.m_pt );							// init panel setup top menu thread.
					panel_setup_construct_cnfg_menu_object();
					// 2011-08-31 -WFC- ^
					panel_main_power_up();
					panel_key_init();	// empty key buffer clear previous pressed keys.  2013-11-08 -WFC-
				break;
			case PANEL_CAL_KEY:
					#ifdef  LED_DISPLAY_ANC_PEAK
						led_display_set_annunciator( LED_DISPLAY_ANC_PEAK, LED_SEG_STATE_OFF);
						// peak hold disabled is taking care by panel_setup_does_it_allows_new_cal(), panel_setup_does_it_allows_rcal_recal() and cmd_is_ok_start_new_cal().
					#endif
					// 2011-04-26 -WFC- panel_setup_construct_cal_menu_object();
					panel_setup_construct_recal_menu_object();						// 2011-04-26 -WFC-
					panel_main_power_up();
					panel_key_init();	// empty key buffer clear previous pressed keys.  2013-11-08 -WFC-
				break;
//			case PANEL_POWER_KEY:
//					gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;
//					panel_main_power_up();
//				break;
//			case KEY_ZERO_KEY:
			case PANEL_MASTER_RESET_KEY:
//					bios_on_board_led( ONLY, BIOS_ON_BOARD_LED_GREEN );
					gbPanelMainRunMode = PANEL_RUN_MODE_MASTER_RESET_MENU;
					gPanelSetupSubMenuObj1.pRootItem	= gacPanel_Menu_Master_Reset;
					gPanelSetupSubMenuObj1.maxNumOfItem	= MENU_MASTER_RESET_MAX_ITEMS;
					gPanelSetupSubMenuObj1.nextMove		= 1;		// default next move is one step.
					gPanelSetupSubMenuObj1.msgDisplayTime= 0;		// assume no msg to be display.
					// init object variables at the very first time of this thread.
					gbPanelSetupStatus = 0;		// clear status.
					PT_INIT( &gPanelSetupSubMenuObj1.m_pt );		// init panel setup sub menu thread.
					panel_main_power_up();
					panel_key_init();	// empty key buffer clear previous pressed keys.
//					gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;
				break;
//			default:
//					gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;
//					panel_main_power_up();
//				break;
		}
	}
	else
	{
		gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;
		panel_main_power_up();
		panel_key_init();	// empty key buffer clear previous pressed keys.  2013-11-08 -WFC-
	}
} // end panel_main_power_off_mode()


/**
 * power up the device.
 *
 * @note  Power up and down problem with the MAX7221 LED driver chip.
 * If you power up and immediately send a turn on command to the chip without wait,
 * then it never works. I put a 50 milliseconds delay before I send a turn on
 * commmand. This delay fixed the problem. The delay gives time for the LED
 * power supply to charged up to deliver power to the LED driver chip.
 *
 * History:  Created on 2009/08/12 by Wai Fai Chin
 * 2011-05-12 -WFC- set LED intensity based on its settings.
 * 2012-09-20 -WFC- Moved LED intensity settings codes to led_display_init(). 2012-10-03 -DLM-
 * 2014-11-03 -WFC- clear run status flags.
 * 2015-01-19 -WFC- turn on RS232 chip only in normal power mode.
 */

void panel_main_power_up( void )
{
	gbBiosRunStatus &= ~BIOS_RUN_STATUS_POWER_OFF;
	if ( SYS_POWER_SAVE_STATE_ACTIVE != gbSysPowerSaveState )		//2015-01-19 -WFC-
		TURN_ON_RS232_CHIP;		// it also turn on LED power supply too.
	soft_delay( 100 );		// delay 100 milliseconds, for the LED power supply to charged up, so the LED driver chip has time to react to initialization commands.
	// timer_mSec_set( &gPanelDisplayWeightModeTimer, TT_100mSEC);
	// while ( !timer_mSec_expired( &gPanelMainTaskTimer ));		// delay 100 milliseconds, so the LED driver chip has time to react to initialization commands before the power supply cut off the power to it..
	led_display_hardware_init();
	led_display_init();
	self_test_timer_reset_event_timers();	// reset event timers so it would not trigger an expiration event that would execute LED sleep or power off etc..
	// 2011-05-12 -WFC- v
//	if ( gtSystemFeatureFNV.ledIntensity )						// turn up if NOT in auto intensity
//		//led_display_set_intensity( ((1 << gtSystemFeatureFNV.ledIntensity ) - 2 ) ); // dimming LED based on user led intensity setting.
//		led_display_set_intensity( ((1 << gtSystemFeatureFNV.ledIntensity ) - 1 ) ); // 2012-05-24 -DLM-
//
//	else														// turn up if in auto
//		led_display_set_intensity(((BYTE)(gaLSensorDescriptor[ SENSOR_NUM_LIGHT_SENSOR ].curADCcount >> 6)) );	// dimming LED based on light sensor ADC count.
	// 2011-05-12 -WFC- ^

	#if ( CONFIG_ETHERNET_MODULE_AS == CONFIG_ETHERNET_MODULE_AS_DIGI ) // 2012-07-05 -DLM-
		if ( ETHERNET_DEVICE_STATUS_ENABLED_bm & gEthernetDeviceSettingsFNV.status  ) {
			BIOS_TURN_ON_ETHERNET;
			TURN_OFF_RS232_CHIP;
			// BIOS_DISABLED_RS232_RECEIVE;
		}
		else {
			BIOS_TURN_OFF_ETHERNET;
			TURN_ON_RS232_CHIP;
			// BIOS_ENABLED_RS232_RECEIVE;
		}
	#endif

	gbPanelMain_run_status_bm = 0;			// clear run status flags 2014-11-03 -WFC-

} // end panel_main_power_up()


/**
 * Panel main update unit LEDs base on loadcell unit.
 *
 * @param  unit	-- unit of a loadcell.
 *
 * History:  Created on 2009/07/22 by Wai Fai Chin
 */

void panel_main_update_unit_leds( BYTE unit )
{
	if ( SENSOR_UNIT_KG == unit ) {				
		led_display_kg_on();
		led_display_lb_direct_off();			//	PHJ
		//led_display_set_annunciator( LED_DISPLAY_ANC_KG, LED_SEG_STATE_STEADY); // -DLM- 2012-03-15
		//led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_OFF);	// -DLM- 2012-03-15
	}
	else if ( SENSOR_UNIT_LB == unit )  {
		led_display_lb_direct_on();				//	PHJ
		led_display_kg_off();
		//led_display_set_annunciator( LED_DISPLAY_ANC_KG, LED_SEG_STATE_OFF); 	// -DLM- 2012-03-15
		//led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_STEADY);	// -DLM- 2012-03-15
	}
	else {
		// unit is not lb or kg, turn them off.
		led_display_lb_direct_off();			//	PHJ
		led_display_kg_off();
		//led_display_set_annunciator( LED_DISPLAY_ANC_KG, LED_SEG_STATE_OFF); 	// -DLM- 2012-03-15
		//led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_OFF);	// -DLM- 2012-03-15
	}

} // end panel_main_update_unit_leds()

/**
 * It displays a float value in integer form, no decimal point in LEDs.
 *
 * @param  fV	-- a floating point value to be display.
 *
 * History:  Created on 2011-04-22 by Wai Fai Chin
 * 2011-05-02 -WFC- Use higher resolution x1000 function to fit all digit in max LED digit.
 */

void	panel_main_display_float_in_integer( float	fV )
{
	BYTE	bStrBuf[24];
	MSI_CB	countby;

	countby.fValue = 1; // default decimal point.
	countby.decPt = 0;

	// 2011-05-02 -WFC- led_display_format_float_string( fV, &countby, TRUE, bStrBuf );
	led_display_format_high_resolution_float_string( fV, &countby, TRUE, bStrBuf );						// 2011-05-02 -WFC-
	led_display_format_output( bStrBuf );					// The result is stored in gLedDisplayManager.descr.str1.
	led_display_string( gLedDisplayManager.descr.str1 );	// display it now since the test thread bypassed led_display_manage_string_thread.
	PT_INIT( &gLedDisplayManager.anc_pt);					// force led_display_manage_annunciator_thread() to update now.
	led_display_manage_annunciator_thread( &gLedDisplayManager );
} // end panel_main_display_float_in_integer(,,)


/**
 * It displays a float value in integer form, no decimal point in LEDs.
 *
 * @param  fV	-- a floating point value to be display.
 *
 * History:  Created on 2012-09-27 by Wai Fai Chin
 */

void	panel_main_update_sleep_mode_display( void )
{
	if ( led_display_is_led_dimmed_sleep()) {						// if LED segments were dimmed and in sleep mode, then
		led_display_set_all_annunciators_state( OFF );
		led_display_set_annunciator( LED_DISPLAY_ANC_ACKOUT, LED_SEG_STATE_KEEP_BLINKING);
		led_display_lb_direct_off();
		led_display_turn_all_led_digits( OFF );
	}
} // end panel_main_update_sleep_mode_display()
