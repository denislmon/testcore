/*! \file panel_test_portaweigh2.c \brief  Panel test menu definition.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2010, 2011 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL Atmega Family
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2010-08-06 by Wai Fai Chin
//  History:  Modified to PortaWeigh2 on 2011-12-13 by Denis Monteiro
// 
//   Test events show on display panel.
//
// ****************************************************************************
// History:
// 2011-04-06 -WFC- written for Challenger3 requirement.
// 2011-06-10 -WFC- derived from panel_test.c into Challenger3 specific setup menu.
 
#include	"config.h"

#include	"led_lcd_msg_def.h"
#include	"panel_test.h"
#include	"led_display.h"
#include	"commonlib.h"
#include	"panelmain.h"
#include	"sensor.h"
#include	"calibrate.h"
#include	"adc_lt.h"
#include	"setpoint.h"
#include	"nv_cnfg_mem.h"
#include	"lc_total.h"
#include	<stdio.h>		// for sprintf_P(). without included <stdio.h>, it acts unpredictable or crash the program. EOF
#include	"adc_cpu.h"
#include	"cmdaction.h"
#include	"vs_sensor.h"
#include	"loadcell.h"	// 2011-04-11 -WFC-


#define 	PANEL_TEST_AUTO_STEP_INTERVAL			TT_1SEC
#define		PANEL_TEST_AUTO_STEP_MOVE_ENTER			2
#define		PANEL_TEST_AUTO_STEP_MOVE_MAX_SEQUENCE	3

char	panel_test_simple_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );
char	panel_test_led_test0to9_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );
char	panel_test_service_counter_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj  );
char	panel_test_rcal_reading_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );

BYTE	panel_test_menu_software_version_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_test_menu_show_rcal_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_sub_menu_dummy_empty_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_test_menu_show_service_counter_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_test_menu_battery_reading_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );

// **********************************************************************
//				Secondary Test Object
// **********************************************************************

// _mapper method of each test menu item is designed to display contents of test result.
/// Secondary test menu items
PANEL_MENU_ITEM_T	gacPanel_Test_Menu_Soft_Version[]			PROGMEM = { 0, panel_test_menu_software_version_mapper, 0, 1 | PANEL_MENU_ITEM_TYPE_SIMPLE };

PANEL_MENU_ITEM_T	gacPanel_Test_Menu_Battery_Test[]			PROGMEM = { 0, panel_test_menu_battery_reading_mapper, 0, 1 | PANEL_MENU_ITEM_TYPE_SIMPLE };

PANEL_MENU_ITEM_T	gacPanel_Test_Menu_Led_Test0to9[]			PROGMEM = { 0, panel_sub_menu_dummy_empty_mapper, 0, 1 | PANEL_MENU_ITEM_TYPE_SIMPLE };

PANEL_MENU_ITEM_T	gacPanel_Test_Menu_RCal[]					PROGMEM = { 0, panel_test_menu_show_rcal_mapper, 0, 1 | PANEL_MENU_ITEM_TYPE_SIMPLE };

/// Secondary Test Top Menu Items.
PANEL_TOP_MENU_T    gacPanel_Top_Menu_Secondary_Test[ PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS ] PROGMEM = {
 {gcStr_SoFt,	gacPanel_Test_Menu_Soft_Version, 	panel_test_simple_sub_menu_thread,		1 | PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_batt,	gacPanel_Test_Menu_Battery_Test, 	panel_test_simple_sub_menu_thread,		1 | PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_d_tESt,	gacPanel_Test_Menu_Led_Test0to9, 	panel_test_led_test0to9_thread,			1 | PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_C_CAL,	gacPanel_Test_Menu_RCal, 			panel_test_rcal_reading_sub_menu_thread,1 | PANEL_TOP_MENU_TYPE_SIMPLE},
};

// **********************************************************************
//				Service Info Test Object
// **********************************************************************

// _mapper method of each test menu item is designed to display contents of test result.

/// Secondary test menu items
PANEL_MENU_ITEM_T	gacPanel_Test_Menu_Service_Counter[]			PROGMEM = { 0, panel_test_menu_show_service_counter_mapper, 0, 1 | PANEL_MENU_ITEM_TYPE_SIMPLE };

/// Service Info Top Menu Items.
PANEL_TOP_MENU_T    gacPanel_Top_Menu_Service_Info_Test[ PANEL_TEST_TOP_MENU_SERVICE_INFO_MAX_ITEMS ] PROGMEM = {
 {gcStr_LFCnt,	gacPanel_Test_Menu_Service_Counter,	panel_test_service_counter_sub_menu_thread,	1	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_OLCnt,	gacPanel_Test_Menu_Service_Counter,	panel_test_service_counter_sub_menu_thread,	1	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_C_CAL,	gacPanel_Test_Menu_RCal,			panel_test_rcal_reading_sub_menu_thread,	1	| PANEL_TOP_MENU_TYPE_SIMPLE},
};

/// Need to change the following when add or removed item in gacPanel_Top_Menu_Service_Info_Test[].
#define		PANEL_TEST_LIFT_COUNTER_STEP_NUM			0
#define		PANEL_TEST_OVERLOAD_COUNTER_STEP_NUM		1

// **********************************************************************

TIMER_T	gPanelTest_Backgound_Task_Timer;	// background task timer.
TIMER_T	gPanelTest_Auto_Step_Timer;			// use for auto stepping.

// BYTE	gbPanelTest_step;
BYTE	gbPanelTest_AutoStep_OK_Move;

extern TIMER_T	gPanelMainLoopPauseTimer;	// This timer is use in loop pause for each loop by many panel_main_xxxxx_run_mode_thread() because it runs one thread at a time.

#define		PANEL_TEST_LED_TEST0to9_STEP_INTERVAL		TT_300mSEC

/**
 * Panel_test TOP menu thread. This is the entry main thread of the root of TOP menu.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu object
 *
 * History:  Created on 2010-08-06 by Wai Fai Chin
 * 2011-04-07 -WFC- modified for Challenger3.
 *
 */

//PT_THREAD( panel_setup_top_menu_secondary_test_thread( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_test_top_menu_secondary_test_thread( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )
{
  BYTE 				key;
  PANEL_TOP_MENU_T	topMenu;

  PT_BEGIN( &pSetupObj->m_pt );

	// init object variables at the very first time of this thread.
	gbPanelSetupStatus = 0;		// clear status.
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;

	gbPanelTest_AutoStep_OK_Move = PANEL_TEST_AUTO_STEP_MOVE_ENTER;		// prepare for enter key step.
	timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL);

	// clear no more header msg to display.
	pSetupObj-> msgDisplayTime =
	pSetupObj-> selectIndex = 0;				// first item
	// display first menu message.
	memcpy_P( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));
	led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, topMenu.pMsg);
	led_display_string( gLedDisplayManager.descr.str1 );

	led_display_turn_all_annunciators( OFF);

	PT_INIT( &gLedDisplayManager.anc_pt );
	led_display_manage_annunciator_thread( &gLedDisplayManager );

	// led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ].pMsg); cannot do this because points at SRAM and give you bogus content.
	// The compiler can only get first level of code space content, which is pRootMenu, then the compiler thinks that it is in RAM space, which in turn give you bogus content.
	// end init thread.
	timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_500mSEC);
	PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for 0.5 second delay.
	panel_key_init();	// empty key buffer so the power key will not power down the device.

	for(;;) {
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_key_get( &key ) || (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode) );				// wait until get a key.
		if ( (PANEL_MENU_SCROLL_KEY == key ) ||											// If pressed forward selection key OR
			 ( (1 == gbPanelTest_AutoStep_OK_Move ) && (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode))) { // in auto step mode AND AutoStep is move forward step.
			gbPanelTest_AutoStep_OK_Move++;		// Now it is number 2, prepare for enter key step.
			timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );

			if ( PANEL_MENU_SCROLL_KEY == key ) {
				gbPanelMainSysRunMode = gbPanelMainRunMode =
				gbSysRunMode = PANEL_RUN_MODE_UI_SECONDARY_TEST;		// User Interactive secondary test mode.
			}

			pSetupObj-> selectIndex++;
			if ( pSetupObj-> selectIndex > (pSetupObj-> maxNumOfItem - 1) ) {
				pSetupObj-> selectIndex = 0;
				if ( PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode)		// if it is in auto step mode, exit.
						break;
			}

			memcpy_P ( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));
			led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, topMenu.pMsg);
			led_display_string( gLedDisplayManager.descr.str1 );
		}
// No backward selection for Challenger3.
//		else if ( PANEL_TEST_KEY == key ) {			// backward selection key
//			if (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode) {	// if it is in auto mode, assumed user wants to change it to single step mode.
//				gbPanelMainSysRunMode = gbPanelMainRunMode =
//				gbSysRunMode = PANEL_RUN_MODE_UI_SECONDARY_TEST;		// User Interactive secondary test mode.
//			}
//			else {
//				pSetupObj-> selectIndex--;
//				if ( pSetupObj-> selectIndex <0 )		// if selectIndex < 0;
//					pSetupObj-> selectIndex = pSetupObj-> maxNumOfItem - 1;
//
//				memcpy_P ( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));
//				led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, topMenu.pMsg);
//				led_display_string( gLedDisplayManager.descr.str1 );
//			}
//		}
		else if ( (PANEL_MENU_ENTER_KEY == key) || 																	// If pressed enter key  OR
				( PANEL_TEST_AUTO_STEP_MOVE_MAX_SEQUENCE == gbPanelTest_AutoStep_OK_Move && (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode))) { //   in auto step mode AND OK to move forward.
			// construct gPanelSetupSubMenuObj1 into a thread.
			memcpy_P( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));
			gPanelSetupSubMenuObj1.pRootItem		= topMenu.pRootItem;
			gPanelSetupSubMenuObj1.pSubMenuThread	= topMenu.pSubMenuThread;
			gPanelSetupSubMenuObj1.topLevelMenuIndex= pSetupObj-> selectIndex;
			gPanelSetupSubMenuObj1.maxNumOfItem		= (topMenu.maxNumOfItem & PANEL_TOP_MENU_NUM_ITEM_MASK);
			gPanelSetupSubMenuObj1.nextMove			= 1;		// default next move is one step.
			gPanelSetupSubMenuObj1.msgDisplayTime	= 0;		// assume no msg to be display.

			if ( PANEL_TOP_MENU_TYPE_SIMPLE == (topMenu.maxNumOfItem & PANEL_TOP_MENU_TYPE_MASK) ) {		// It is a simple list menu type.{
				PT_SPAWN( &pSetupObj->m_pt, &gPanelSetupSubMenuObj1.m_pt, (* gPanelSetupSubMenuObj1.pSubMenuThread )( &gPanelSetupSubMenuObj1));
			}

			// here, the thread completed sub menu entry, advanced to the next top menu entry.
			pSetupObj-> selectIndex += gPanelSetupSubMenuObj1.nextMove;
			// pSetupObj-> selectIndex++;
			if ( pSetupObj-> selectIndex > (pSetupObj-> maxNumOfItem - 1) ) {
				pSetupObj-> selectIndex = 0;
				if ( PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode)		// if it is in auto step mode, exit.
						break;
			}
			else if ( pSetupObj-> selectIndex < 0 )
				pSetupObj-> selectIndex = pSetupObj-> maxNumOfItem - 1;

			gbPanelTest_AutoStep_OK_Move = PANEL_TEST_AUTO_STEP_MOVE_ENTER;			// Now it is number 2, prepare for enter key step.
			timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );

			memcpy_P( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));
			led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, topMenu.pMsg);
			led_display_string( gLedDisplayManager.descr.str1 );
		}
		else if ( PANEL_ZERO_KEY == key || (gbPanelSetupStatus & PANEL_SETUP_STATUS_EXIT_NOW) ) {		// quit key
			break;
		}

		PT_YIELD( &pSetupObj->m_pt );  // yeild() to handle when it is in auto step testing mode. This give other threads a chance to run.
	} // end for(;;)

	panel_key_init();	// empty key buffer so the power key will not power down the device.
	led_display_turn_all_annunciators( OFF);

	sensor_init_all();

	gbPanelMainRunMode = gbSysRunMode = PANEL_RUN_MODE_NORMAL;	// This is a single shot thread only. Once test is done, it goes back to normal mode.
	gwSysStatus = SYS_STATUS_SELF_TEST_STEP_END_TEST;			// Challenger3 does not execute self_test_main_thread, so we put this statement here to show the end of test step.

	panel_key_init();	// empty key buffer so the power key will not power down the device.

  PT_END(&pSetupObj->m_pt);
} // end panel_test_top_menu_secondary_test_thread()


/**
 * Panel test of simple sub menu thread. This thread is spawned off by
 *  panel_test_top_menu_secondary_test_thread().
 *
 * @param  pSetupObj	-- pointer to panel setup sub simple menu object
 * @note panel_test simple sub menu type only has ONE menuItem. Its menuItem.maxNumOfItem is
 *       number of simple selection in a list, which also means number of msg in the list points by pMsg.
 *
 * History:  Created on 2010/08/09 by Wai Fai Chin
 */

//PT_THREAD( panel_test_simple_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_test_simple_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )
{
  BYTE 				key;
  BYTE				*pStr;
  PANEL_MENU_ITEM_T	menuItem;

  PT_BEGIN( &pSetupObj->m_pt );
  	// init object variables at the very first time of this thread.
	pSetupObj-> msgID			=
	pSetupObj-> msgDisplayTime	= 0;

	memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
	pSetupObj-> selectIndex = 0;
	// each method of menuItem performs display of test result.
	pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SHOW_VALUE, pSetupObj, 0);

	gbPanelTest_AutoStep_OK_Move = NO;		//reset for forward auto step.
	timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );
	led_display_turn_all_annunciators( OFF);
	// end of init thread.

	for(;;) {
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_key_get( &key ) || (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode) );				// wait until get a key.
		if ( (PANEL_MENU_SCROLL_KEY == key ) || 											// If pressed USER key OR
			 ( (1 == gbPanelTest_AutoStep_OK_Move ) && (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode))) { // in auto step mode AND AutoStep is move forward.
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));

			if ( PANEL_MENU_SCROLL_KEY == key ) {
				gbPanelMainSysRunMode = gbPanelMainRunMode =
				gbSysRunMode = PANEL_RUN_MODE_UI_SECONDARY_TEST;		// User Interactive secondary test mode.
			}

			pSetupObj-> selectIndex++;
			if ( pSetupObj-> selectIndex > ((menuItem.maxNumOfItem & PANEL_TOP_MENU_NUM_ITEM_MASK) - 1) ) {
				// next move will increment by 1 step.
				pSetupObj-> nextMove = 1;
				break;
			}
			// other method may use selected index to perform other task such as change LED intensity,
			else {
				memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
				led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
				led_display_string( gLedDisplayManager.descr.str1 );
			}
			gbPanelTest_AutoStep_OK_Move++;		// Now it is number 2, prepare for enter key step.
			timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );
		}
// No backward selection for Challenger3.
//		else if ( PANEL_TEST_KEY == key ) {			// backward selection key
//			if (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode) {		// if it is in auto mode, assumed user wants to change it to single step mode.
//				gbPanelMainSysRunMode = gbPanelMainRunMode =
//				gbSysRunMode = PANEL_RUN_MODE_UI_SECONDARY_TEST;		// User Interactive secondary test mode.
//			}
//			else {
//				memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
//				pSetupObj-> selectIndex--;
//				if ( pSetupObj-> selectIndex < 0 )	{	// if pSetupObj-> selectIndex < 0, exit.
//					// next move will decrement by 1 step.
//					pSetupObj-> nextMove = -1;
//					break;
//				}
//				// other method may use selected index to perform other task such as change LED intensity,
//				else {
//					memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
//					led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
//					led_display_string( gLedDisplayManager.descr.str1 );
//				}
//			}
//		}
		else if ( (PANEL_MENU_ENTER_KEY == key) ||								// If pressed enter key OR
			 ( (PANEL_TEST_AUTO_STEP_MOVE_MAX_SEQUENCE == gbPanelTest_AutoStep_OK_Move) && (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode))) { // in auto step mode AND AutoStep is enter step.
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SHOW_VALUE, pSetupObj, 0);			//menuItem.pMethod() may updated the gLedDisplayManager.descr.str1
			gbPanelTest_AutoStep_OK_Move = NO;									// reset for forward step key.
			timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );
		}
		else if ( PANEL_ZERO_KEY == key ) {		// quit key
			// next move will increment by 1 step.
			pSetupObj-> nextMove = 1;
			if ( PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode )		// if it is in auto mode, then assumed the user wants to end the secondary test.
				gbPanelSetupStatus  |= PANEL_SETUP_STATUS_EXIT_NOW;
			break;
		}
		PT_YIELD( &pSetupObj->m_pt );  // yeild() to handle when it is in auto step testing mode. This give other threads a chance to run.
	} // end for(;;)

  PT_END(&pSetupObj->m_pt);
} // end panel_test_simple_sub_menu_thread()


/**
 * It loops 0.0.0.0.0.0 to 9.9.9.9.9.9 and turn on 1 bar segment on at a time.
 *
 * @param  pSetupObj	-- pointer to panel setup sub simple menu object
 *
 * History:  Created on 2010-08-11 by Wai Fai Chin
 */

//PT_THREAD( panel_test_led_test0to9_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_test_led_test0to9_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj  )
{
  static BYTE i;		// need to be static because protothread always destroys local variables.
  BYTE	ch;
  BYTE	key;
  BYTE	exitThread;
  BYTE str[14];

  PT_BEGIN( &pSetupObj->m_pt );
	for (;;) {
		for ( i=0; i < 10; i++ ) {
			ch = '0'|i;
			str[0] = str[2] = str[4] = str[6] = str[8] = ch;
			str[1] = str[3] = str[5] = str[7] = '.';
			str[9] = 0;

			led_display_string( str );
			timer_mSec_set( &gPanelMainLoopPauseTimer, PANEL_TEST_LED_TEST0to9_STEP_INTERVAL );
			PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainLoopPauseTimer ) || panel_key_get( &key ) );	// wait for delay.
			// PT_YIELD( &pSetupObj->m_pt );  I don't need Yield because I have PT_WAIT_UNTIL() statement above; It has the same effect as yield.

			exitThread = NO;

			if ( PANEL_MENU_SCROLL_KEY == key ) {				// forward selection key
				if (PANEL_RUN_MODE_UI_SECONDARY_TEST == gbPanelMainRunMode) {		// if it is in auto mode, assumed user wants to change it to single step mode.
					exitThread = YES;
					pSetupObj->nextMove = 1;
					break;
				}
			}
// No backward selection for Challenger3.
//			else if ( PANEL_TEST_KEY == key ) {				// backward selection key
//				if (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode) {		// if it is in auto mode, assumed user wants to change it to single step mode.
//					gbPanelMainSysRunMode = gbPanelMainRunMode =
//					gbSysRunMode = PANEL_RUN_MODE_UI_SECONDARY_TEST;		// User Interactive secondary test mode.
//				}
//				else {
//					exitThread = YES;
//					pSetupObj->nextMove = -1;
//					break;
//				}
//			}
			else if ( PANEL_ZERO_KEY == key ) {		// quit key
				exitThread = YES;
				if ( PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode )		// if it is in auto mode, then assumed the user wants to end the secondary test.
					gbPanelSetupStatus  |= PANEL_SETUP_STATUS_EXIT_NOW;
				else
					pSetupObj->nextMove = 1;
				break;
			}
		} // end for ( i=0; i < 10; i++ ) {}

		if ( PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode ) {			// if it is in auto mode, exit.
			exitThread = YES;
		}

		if ( YES == exitThread)
			break;
	} // end for(;;)

	led_display_turn_all_leds( OFF );

  PT_END( &pSetupObj->m_pt );
} // end panel_test_led_test0to9_thread()


/**
 * It displays software version number.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2010-08-06 by Wai Fai Chin
 */

BYTE	panel_test_menu_software_version_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	led_display_format_output( gAppBootShareNV.softVersionStr );
	led_display_string( gLedDisplayManager.descr.str1 );
	return 0;
} // end panel_test_menu_software_version_mapper(,,)


/**
 * It displays battery voltage.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2011-04-07 by Wai Fai Chin
 */

BYTE	panel_test_menu_battery_reading_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE	bStrBuf[ PANEL_SETUP_LOCAL_MAX_STRING_SIZE ];
	MSI_CB	countby;

	countby.fValue = 0.01;	// default decimal point.
	countby.decPt = 2;		// two decimal point.	that is 0.01.
	led_display_format_high_resolution_float_string( gaLSensorDescriptor[SENSOR_NUM_INPUT_VOLTAGEMON].value, &countby, FALSE, bStrBuf );
	// For dual battery - 2012-05-03 -DLM-
	if ( gaLSensorDescriptor[SENSOR_NUM_INPUT_VOLTAGEMON_BATT1].value > gaLSensorDescriptor[SENSOR_NUM_INPUT_VOLTAGEMON_BATT2].value )
		led_display_format_high_resolution_float_string( gaLSensorDescriptor[SENSOR_NUM_INPUT_VOLTAGEMON_BATT1].value, &countby, FALSE, bStrBuf );
	else
		led_display_format_high_resolution_float_string( gaLSensorDescriptor[SENSOR_NUM_INPUT_VOLTAGEMON_BATT2].value, &countby, FALSE, bStrBuf );
	led_display_format_output( bStrBuf );					// The result is stored in gLedDisplayManager.descr.str1.
	led_display_string( gLedDisplayManager.descr.str1 );	// display it now since the test thread bypassed led_display_manage_string_thread.
	return 0;
} // end panel_test_menu_battery_reading_mapper(,,)


/**
 * It displays Rcal value of loadcell based on the menu item select index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return 0
 *
 * History:  Created on 2010-08-06 by Wai Fai Chin
 * 2011-04-22 -WFC- simplfied it with panel_setup_show_rcal().
 */

BYTE	panel_test_menu_show_rcal_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	panel_setup_show_rcal( PANEL_SETUP_LOADCELL_NUM );
	return 0;
} // end panel_test_menu_show_rcal_mapper(,,)


/**
 * A dummy empty mapper function.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return 0
 *
 * History:  Created on 2010-08-09 by Wai Fai Chin
 */

BYTE	panel_sub_menu_dummy_empty_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	return 0;
} // end panel_sub_menu_dummy_empty_mapper(,,)

/**
 * It displays lift counter.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2011-04-07 by Wai Fai Chin
 * 2011-04-22 -WFC- Its display logic is replaced by calling panel_main_display_float_in_integer().
 */

BYTE	panel_test_menu_show_service_counter_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{

	float	fV;

	if ( PANEL_TEST_LIFT_COUNTER_STEP_NUM == pSetupObj->topLevelMenuIndex ) {
		fV = (float) gaulLiftCntFNV[0];
	}
	else {
		fV = (float) gaulOverloadedCntFNV[0];
	}

	panel_main_display_float_in_integer( fV );
	return 0;
} // end panel_test_menu_show_lift_counter_mapper(,,)


/**
 * This thread display service counters and clear service ack flag.
 * It display lift or overload counter based on topLevelMenuIndex;
 *
 * @param  pSetupObj	-- pointer to panel setup sub simple menu object
 *
 * History:  Created on 2011-04-08 by Wai Fai Chin
 */

//PT_THREAD( panel_test_service_counter_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_test_service_counter_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj  )
{
  BYTE	key;
  BYTE	exitThread;
  PANEL_MENU_ITEM_T	menuItem;


  PT_BEGIN( &pSetupObj->m_pt );
	// init object variables at the very first time of this thread.
	pSetupObj-> msgID			=
	pSetupObj-> msgDisplayTime	= 0;
	led_display_turn_all_annunciators( OFF);

	memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
	pSetupObj-> selectIndex = 0;
	// each method of menuItem performs display of test result.
	pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SHOW_VALUE, pSetupObj, 0);

	gbPanelTest_AutoStep_OK_Move = NO;		//reset for forward auto step.
	// end of init thread.

	for (;;) {
		timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelTest_Auto_Step_Timer)|| panel_key_get( &key ) );	// wait for delay or get a key.
		exitThread = NO;
		if ( PANEL_MENU_SCROLL_KEY == key ) {				// forward selection key
			if (PANEL_RUN_MODE_UI_SECONDARY_TEST == gbPanelMainRunMode) {		// if it is in auto mode, assumed user wants to change it to single step mode.
				exitThread = YES;
				pSetupObj->nextMove = 1;
				break;
			}
		}
		else if ( PANEL_ZERO_KEY == key ) {		// quit key
			exitThread = YES;
			if ( PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode )		// if it is in auto mode, then assumed the user wants to end the secondary test.
				gbPanelSetupStatus  |= PANEL_SETUP_STATUS_EXIT_NOW;
			else
				pSetupObj->nextMove = 1;

			if ( PANEL_TEST_LIFT_COUNTER_STEP_NUM == pSetupObj->topLevelMenuIndex ) {
				gabServiceStatusFNV[ PANEL_SETUP_LOADCELL_NUM ] &= (~( LC_SERVICE_STATUS_LIFT_CNT_NEED_USER_ACK | LC_SERVICE_STATUS_LIFT_CNT_MET_SERVICE_CNT ));
				nv_cnfg_fram_save_service_counters( PANEL_SETUP_LOADCELL_NUM );

			}
			else {
				gabServiceStatusFNV[ PANEL_SETUP_LOADCELL_NUM ] &= (~( LC_SERVICE_STATUS_OVERLOAD_CNT_NEED_USER_ACK | LC_SERVICE_STATUS_OVERLOAD_CNT_MET_SERVICE_CNT ));
				nv_cnfg_fram_save_service_counters( PANEL_SETUP_LOADCELL_NUM );
			}
			break;
		}

		if ( PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode ) {		// if it is in auto mode, exit.
			exitThread = YES;
		}

		if ( YES == exitThread)
			break;
	} // end for(;;)

  PT_END( &pSetupObj->m_pt );
} // end panel_test_service_counter_sub_menu_thread()


/**
 * Panel test of simple sub menu thread. This thread is spawned off by
 *  panel_test_top_menu_secondary_test_thread() to read rcal value.
 *
 * @param  pSetupObj	-- pointer to panel setup sub simple menu object
 * @note panel_test simple sub menu type only has ONE menuItem. Its menuItem.maxNumOfItem is
 *       number of simple selection in a list, which also means number of msg in the list points by pMsg.
 *
 * History:  Created on 2010/08/09 by Wai Fai Chin
 */

//PT_THREAD( panel_test_rcal_reading_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_test_rcal_reading_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )
{
  BYTE 				key;
  BYTE				*pStr;
  PANEL_MENU_ITEM_T	menuItem;

  PT_BEGIN( &pSetupObj->m_pt );
	// init object variables at the very first time of this thread.
	pSetupObj-> msgID			=
	pSetupObj-> msgDisplayTime	= 0;

	memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
	pSetupObj-> selectIndex = 0;
	// each method of menuItem performs display of test result.
	pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SHOW_VALUE, pSetupObj, 0);

	gbPanelTest_AutoStep_OK_Move = NO;		//reset for forward auto step.
	timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );
	led_display_turn_all_annunciators( OFF);
	// end of init thread.

	for(;;) {
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_key_get( &key ) || (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode) );				// wait until get a key.
		if ( (PANEL_MENU_SCROLL_KEY == key ) || 											// If pressed USER key OR
			 ( (1 == gbPanelTest_AutoStep_OK_Move ) && (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode))) { // in auto step mode AND AutoStep is move forward.
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));

			if ( PANEL_MENU_SCROLL_KEY == key ) {
				gbPanelMainSysRunMode = gbPanelMainRunMode =
				gbSysRunMode = PANEL_RUN_MODE_UI_SECONDARY_TEST;		// User Interactive secondary test mode.
			}

			pSetupObj-> selectIndex++;
			if ( pSetupObj-> selectIndex > ((menuItem.maxNumOfItem & PANEL_TOP_MENU_NUM_ITEM_MASK) - 1) ) {
				// next move will increment by 1 step.
				pSetupObj-> nextMove = 1;
				break;
			}
			// other method may use selected index to perform other task such as change LED intensity,
			else {
				memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
				led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
				led_display_string( gLedDisplayManager.descr.str1 );
			}
			gbPanelTest_AutoStep_OK_Move++;		// Now it is number 2, prepare for enter key step.
			timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );
		}
//		else if ( (PANEL_MENU_ENTER_KEY == key) ||								// If pressed enter key OR
//			 ( (PANEL_TEST_AUTO_STEP_MOVE_MAX_SEQUENCE == gbPanelTest_AutoStep_OK_Move) && (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode))) { // in auto step mode AND AutoStep is enter step.
//			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
//			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SHOW_VALUE, pSetupObj, 0);			//menuItem.pMethod() may updated the gLedDisplayManager.descr.str1
//			gbPanelTest_AutoStep_OK_Move = NO;									// reset for forward step key.
//			timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );
//		}
		else if ( PANEL_ZERO_KEY == key ) {		// quit key
			// next move will increment by 1 step.
			pSetupObj-> nextMove = 1;
			if ( PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode )		// if it is in auto mode, then assumed the user wants to end the secondary test.
				gbPanelSetupStatus  |= PANEL_SETUP_STATUS_EXIT_NOW;
			break;
		}
		PT_YIELD( &pSetupObj->m_pt );  // yeild() to handle when it is in auto step testing mode. This give other threads a chance to run.
	} // end for(;;)

  PT_END(&pSetupObj->m_pt);
} // end panel_test_rcal_reading_sub_menu_thread()


/**
 * Panel test background tasks. It generates auto step instructions.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu object
 *
 * History:  Created on 2010-08-10 by Wai Fai Chin
 */

void panel_test_background_tasks( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )
{
	// generate auto step instruction. 1 means forward key, 3 means enter key, other number has no meaning.
	if (PANEL_RUN_MODE_AUTO_SECONDARY_TEST == gbPanelMainRunMode) {
		if ( timer_mSec_expired( &gPanelTest_Auto_Step_Timer ) )		{
			gbPanelTest_AutoStep_OK_Move++;
			if ( gbPanelTest_AutoStep_OK_Move > PANEL_TEST_AUTO_STEP_MOVE_MAX_SEQUENCE )
				gbPanelTest_AutoStep_OK_Move = 0;
			timer_mSec_set( &gPanelTest_Auto_Step_Timer, PANEL_TEST_AUTO_STEP_INTERVAL );
		}
	}
} // end panel_test_background_tasks()

