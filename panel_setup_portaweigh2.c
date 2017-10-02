/*! \file panel_setup_portaweigh2.c \brief Implementation of Panel setup menu for PortaWeigh2.*/
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
//  History:  Created on 2009/07/08 by Wai Fai Chin
//  History:  Modified for PortaWeigh2 on 2011/12/13 by Denis Monteiro
// 
//   Panel Menu setup
//
// ****************************************************************************
// 2011-04-11 -WFC- added thread pointer item to all PANEL_TOP_MENU_T items.
// 2011-06-10 -WFC- derived from panel_setup.c into Challenger3 specific setup menu.

/**

 \code
 A Top MENU GROUP is makeup of at least two or more of group items.
 A group item is make up of list of Top menu items.

      a group        a group        a group 
       item           item           item
Top  +--------+    +---------+    +---------+
Menu |        |    |         |    |         |
Group|  Cal   |--->|  Rcal   |--->|  Stand  |--->.... <== manage by gPanelSetupTopMenuGroupObj with panel_setup_top_menu_group_thread()
     |        |    |         |    |         |
     +--------+    +---------+    +---------+
         |           
         |           
         V           

List   Simple         Complex    dynamic choice
of 
Top  +--------+    +---------+    +---------+
Menu |        |    |         |    | countby |
items|  Unit  |--->|   Cap   |--->|    d    |--->.... <== manage by gPanelSetupTopMenuObj with panel_setup_top_menu_thread()
of a |        |    |         |    |         |
Cal  +--------+    +---------+    +---------+
group    |              |              |    
Item     |              |              |
         V sub level 1  |              V sub level 1
     +--------+    	    |         +---------+ 
     |        |         |         | Dynamic |
     |   LB   |         |         | Choice  | <== manage by gPanelSetupSubMenuObj1 with panel_setup_dynamic_choice_sub_menu_thread()
     |        |         |         |         | 
     +--------+         |         +---------+ 
         |              |      
         |              | float
         V              V      
     +--------+    +---------+    +---------+
     |        |    |Capacity |    |  Enter  |
     |   Kg   |    |  Value  |--->| a float | <== manage by gPanelNumEntryObj with panel_setup_numeric_entry_thread()
     |        |    |         |    |  Value  |
     +--------+    +---------+    +---------+ 
     sub level 1   sub level 1
         /\             /\
         ||             || 
     manage by          |+-- manage by 
gPanelSetupSubMenuObj1  +--- gPanelSetupSubMenuObj1 with panel_setup_complex_sub_menu_thread()
     with 
panel_setup_simple_sub_menu_thread()



 A TYPICAL GROUP ITEM looks as follow:

List   Simple         Simple        Complex
of 
Top  +--------+    +---------+    +---------+             This group item does not formed a Top Menu Group becaus it is a single group.
Menu |        |    |         |    |         |             Thus, it directly use panel_setup_top_menu_thread() instead of panel_setup_top_menu_group_thread().
items|  Func  |--->|  A-OFF  |--->|  StPt1  |--->.... <== manage by gPanelSetupTopMenuObj with panel_setup_top_menu_thread()
of a |        |    |         |    |         |
Group+--------+    +---------+    +---------+
Item     |              |              |    
         |              |              |
         V sub level 1  V              V simple   sub level 2 
     +--------+    +---------+    +---------+    +-------+    +---------+    +--------+
     |        |    |         |    |         |    |       |    |         |    |        |
     |  A-OFF |    |  A-OFF  |    | Compare |--->|  OFF  |--->|  GrEAt  |--->|  LESS  | <== manage by gPanelSetupSubMenuObj2 with
     |        |    |         |    |         |    |       |    |         |    |        |     panel_setup_simple_sub_menu_thread()
     +--------+    +---------+    +---------+    +-------+    +---------+    +--------+
         |              |              |    
         |              |              |
         V sub level 1  V              V simple   sub level 2
     +--------+    +---------+    +---------+    +-------+    +-------+    +-------+    +-------+
     |        |    |         |    |  Source |    |       |    |       |    |       |    |       |
     |  tESt  |    |    15   |    |   Mode  |--->| GroSS |--->| nEtGr |--->| totAl |--->| t-cnt |  <== manage by gPanelSetupSubMenuObj2 with
     |        |    |         |    |         |    |       |    |       |    |       |    |       |      panel_setup_simple_sub_menu_thread()
     +--------+    +---------+    +---------+    +-------+    +-------+    +-------+    +-------+
         |              |              |
         |              |              |
         V              V              V float
     +--------+    +---------+    +---------+    +---------+
     |        |    |         |    |A compare|    |  Enter  |
     |  tESt  |    |    15   |    |  Value  |--->| a float | <== manage by gPanelNumEntryObj with panel_setup_numeric_entry_thread()
     |        |    |         |    |         |    |  Value  |
     +--------+    +---------+    +---------+    +---------+ 
         |              |         sub level 1
         |              |              /\
         V              V              ||
     +--------+    +---------+     manage by 
     |        |    |         |   gPanelSetupSubMenuObj1 with
     | totAL  |    |    30   |   panel_setup_complex_sub_menu_thread()
     |        |    |         |
     +--------+    +---------+
         |              |
         |              |
         V              V
     +--------+    +---------+
     |        |    |         |
     |  Unit  |    |    45   |
     |        |    |         |
     +--------+    +---------+
         |              |  
         |              |  
         V              V  
     +--------+    +---------+
     |        |    |         |
     |  Phold |    |  1hour  |
     |        |    |         |
     +--------+    +---------+
         |         sub level 1
         |             /\
         V             || 
     +--------+     manage by
     |        |    gPanelSetupSubMenuObj1 with
     |  nEtGr |    panel_setup_simple_sub_menu_thread()
     |        |
     +--------+
     sub level 1
         /\
         || 
      manage by
     gPanelSetupSubMenuObj1 with
     panel_setup_simple_sub_menu_thread()

 \endcode

 */

#include	"led_lcd_msg_def.h"
#include	"panel_setup.h"
#include	"led_display.h"
#include	"commonlib.h"
#include	"panelmain.h"
#include	"sensor.h"
#include	"calibrate.h"
#include	"adc_lt.h"
#include	"setpoint.h"
#include	"nv_cnfg_mem.h"
#include	"lc_total.h"
#include	"lc_zero.h"		// 2016-03-23 -WFC-
#include	<stdio.h>		// for sprintf_P(). without included <stdio.h>, it acts unpredictable or crash the program. EOF
#include	"adc_cpu.h"

// private methods for menu item methods.
BYTE	panel_setup_auto_power_off_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_led_intensity_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_led_sleep_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );		//	PHJ
BYTE	panel_setup_sensor_filter_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_setpoint_cmp_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_setpoint_value_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_setpoint_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_total_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_unit_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_cal_unit_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_user_key_func_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );

// calibration related mapper methods.
BYTE	panel_setup_std_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_azm_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_zop_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );			// 2016-03-23 -WFC-
//BYTE	panel_setup_cal_new_cal_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_capacity_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_countby_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_cal_zero_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_cal_load_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_rcal_zero_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_rcal_load_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_show_rcal_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV );
BYTE	panel_setup_new_rcal_zero_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );

BYTE	panel_setup_master_reset_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV );
// 2012-06-28 -WFC- v 2012-07-03 -DLM-
BYTE	panel_setup_print_composite_string_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV );
BYTE	panel_setup_print_control_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV );
BYTE	panel_setup_print_interval_value_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV );
// 2012-06-28 -WFC- ^

// 2012-04-27 -WFC- v 2012-06-20 -DLM-
BYTE	panel_setup_rf_device_on_off_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_rf_sc_id_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_rf_channel_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_rf_network_id_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );		//
// 2012-04-27 -WFC- ^
BYTE	panel_setup_rf_device_always_on_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );	// 2016-03-31 -WFC-
BYTE	panel_setup_rf_device_type_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );	// 2016-03-31 -WFC-

// 2012-07-06 -DLM- v
BYTE	panel_setup_ethernet_device_on_off_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
BYTE	panel_setup_ethernet_sc_id_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
// 2012-07-06 -DLM- ^


void	panel_setup_display_complex_menu_item( PANEL_MENU_ITEM_T	*pMenuItem, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj);
BYTE	panel_setup_display_default_float_value( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );

void	panel_setup_display_msg_duration_P( BYTE msgID, BYTE duration );
BYTE	panel_setup_does_it_allows_new_cal( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );
BYTE	panel_setup_does_it_allows_recal( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );
BYTE	panel_setup_does_it_allows_re_rcal( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );

char	panel_setup_simple_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );
char	panel_setup_complex_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );
char	panel_setup_dynamic_choice_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );

BYTE	panel_setup_save_cal_talbe( BYTE n );

void	panel_setup_abort_cal( BYTE n );		// 2011-05-03 -WFC-

BYTE	panel_setup_print_string_top_menu_manger(	PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );		// 2012-05-10 -WFC- 2012-06-20 -DLM-
BYTE	panel_setup_battery_life_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV ); // 2014-09-30 -WFC-

BYTE	panel_setup_print_output_port_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );	// 2014-07-15 -WFC-
BYTE	panel_setup_print_listener_id_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );	// 2014-07-15 -WFC-


// 2011-04-11 -WFC- moved to panel_setup.h
//#define	PANEL_SETUP_STATUS_EXIT_SAVE	0x80
//#define	PANEL_SETUP_STATUS_EXIT_NO_SAVE	0x40
//#define	PANEL_SETUP_STATUS_EXIT_NOW		0x20
//#define	PANEL_SETUP_STATUS_CONTINUE		0x10

BYTE	gbPanelSetupStatus;

/// clonable menu thread objects
PANEL_SETUP_TOP_MENU_GROUP_CLASS	gPanelSetupTopMenuGroupObj;
PANEL_SETUP_TOP_MENU_CLASS			gPanelSetupTopMenuObj;
PANEL_SETUP_SUB_MENU_CLASS			gPanelSetupSubMenuObj1;		// sub level 1
PANEL_SETUP_SUB_MENU_CLASS			gPanelSetupSubMenuObj2;		// deeper level of sub menu

PANEL_NUMERIC_ENTRY_CLASS			gPanelNumEntryObj;

/// Msg table index by MENU_MSG_ID_nnnnn
const char *gcMsg_List_Msg[]	PROGMEM = {
	gcStr_CAL, gcStr_CAL_d, gcStr_C_CAL, gcStr_C_CAL,gcStr_StorE, gcStr_CErr, gcStr_r_Err, gcStr_No,
	gcStr_PASS, gcStr_FAIL, gcStr_Cancl, gcStr_Good, gcStr_Littl, gcStr_Func1, gcStr_Func2, gcStr_StAnd,
	gcStr_SEtuP, gcStr_StPt1, gcStr_StPt2, gcStr_StPt3, gcStr_Azt, gcStr_PrESS, gcStr_T_Off, gcStr_Print,
	gcStr_rF, gcStr_Ethnt
 };

const char *gcMsg_List_rESEt[]			PROGMEM = {gcStr_rESEt, gcStr_SurE_Q};
//#define MENU_MASTER_RESET_MAX_ITEMS		sizeof(gcMsg_List_rESEt) / sizeof(char *)
PANEL_MENU_ITEM_T	gacPanel_Menu_Master_Reset[]	PROGMEM = { gcMsg_List_rESEt,	panel_setup_master_reset_mapper,	0,	MENU_MASTER_RESET_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};


#ifdef CONFIG_WITH_HIRES			
const char *gcMsg_List_Func[]			PROGMEM = {gcStr_OFF, gcStr_tESt, gcStr_totAL, gcStr_v_ttL, 
											gcStr_nEtGr, gcStr_Learn, gcStr_Phold, gcStr_Unit, gcStr_HIrES };	//	DRT	PHJ  
#else // CONFIG_WITH_HIRES			
//const char *gcMsg_List_Func[]			PROGMEM = {gcStr_OFF, gcStr_tESt, gcStr_totAL, gcStr_v_ttL,
//											gcStr_nEtGr, gcStr_Learn, gcStr_Phold, gcStr_Unit };	//	DRT	PHJ

const char *gcMsg_List_Func[]			PROGMEM = {gcStr_OFF, gcStr_tESt, gcStr_totAL, gcStr_v_ttL,
											gcStr_nEtGr, gcStr_Learn, gcStr_Phold, gcStr_Unit, gcStr_Print };	//	2015-09-24 -WFC-


#endif // CONFIG_WITH_HIRES			
//	const char *gcMsg_List_Func[]			PROGMEM = {gcStr_OFF, gcStr_tESt, gcStr_totAL, gcStr_Unit, gcStr_Phold, 
//												gcStr_nEtGr, gcStr_Learn, // gcStr_StPt1, gcStr_StPt2, gcStr_StPt3,
//												gcStr_v_ttL };	//	PHJ
const char *gcMsg_List_A_OFF[]			PROGMEM = {gcStr_OFF, gcStr_15, gcStr_30, gcStr_45, gcStr_60};
// 2012-02-03 -WFC- const char *gcMsg_List_Filter[]			PROGMEM = {gcStr_OFF, gcStr_LO, gcStr_HI_1, gcStr_HI_2};
const char *gcMsg_List_Filter[]			PROGMEM = {gcStr_OFF, gcStr_LO, gcStr_HI_1}; // 2012-02-03 -WFC- removed HI_2 filter level setting.
#if  ( CONFIG_4260IS == TRUE)
const char *gcMsg_List_Led_Intensity[]	PROGMEM = {gcStr_Auto, gcStr_LO_1, gcStr_LO_2, gcStr_HI_1}; // 2015-05-27 -DLM- Removed HI_2 from Led Intensity
#else
const char *gcMsg_List_Led_Intensity[]	PROGMEM = {gcStr_Auto, gcStr_LO_1, gcStr_LO_2, gcStr_HI_1, gcStr_HI_2};
#endif
const char *gcMsg_List_Led_Sleep	[]	PROGMEM = {gcStr_OFF, gcStr_5, gcStr_15, gcStr_30};	//	PHJ
const char *gcMsg_List_Unit[]			PROGMEM = {gcStr_Unit, gcStr_Unit};
const char *gcMsg_List_Hires[]			PROGMEM = {gcStr_OFF, gcStr_On};
//	const char *gcMsg_List_Total_Mode[]		PROGMEM = {gcStr_OFF, gcStr_A_LoAd, gcStr_A_nor, gcStr_A_PEA, gcStr_A_droP, gcStr_A_AcP, gcStr_PrESS};
//                                                   0            1              2            3           4
const char *gcMsg_List_Total_Mode[]		PROGMEM = {gcStr_OFF, gcStr_PrESS, gcStr_A_LoAd, gcStr_A_nor, gcStr_A_PEA};	//	PHJ	DRT
// 2011-04-26 -WFC- v
// TRUE TOTAL MODE enum is:                          0            6            1              2           3
const BYTE *gbMenuIndexToTotalMode[]	PROGMEM = {0, 6, 1, 2, 3};
const BYTE *gbTotalModeToMenuIndex[]	PROGMEM = {0, 2, 3, 4, 0, 0, 1};
//                                         Selection index
//#define  LC_TOTAL_MODE_DISABLED		0    	0
//#define  LC_TOTAL_MODE_AUTO_LOAD		1    	2
//#define  LC_TOTAL_MODE_AUTO_NORMAL	2   	3
//#define  LC_TOTAL_MODE_AUTO_PEAK		3    	4
//#define  LC_TOTAL_MODE_LOAD_DROP		4    	0
//#define  LC_TOTAL_MODE_ON_ACCEPT		5    	0
//#define  LC_TOTAL_MODE_ON_COMMAND		6    	1
// 2011-04-26 -WFC- ^



const char *gcMsg_List_Setpoint_Cmp[]	PROGMEM = {gcStr_OFF, gcStr_GrEAt, gcStr_LESS};
// 2012-09-19 -WFC- const char *gcMsg_List_Setpoint_Mode[]	PROGMEM = {gcStr_GROSS, gcStr_nEtGr, gcStr_totAL, gcStr_t_Cnt};
// 2014-10-22 -WFC- const char *gcMsg_List_Setpoint_Mode[]	PROGMEM = {gcStr_nEtGr, gcStr_GROSS, gcStr_totAL, gcStr_t_Cnt};		// 2012-09-19 -WFC-
const char *gcMsg_List_Setpoint_Mode[]	PROGMEM = {gcStr_nEtGr, gcStr_GROSS, gcStr_totAL, gcStr_t_Cnt, gcStr_LFcnt};		// 2014-10-22 -WFC-

const char *gcMsg_List_Cal_Zero[]		PROGMEM = {gcStr_O};

const char *gcMsg_List_Std_Mode[]		PROGMEM = {gcStr_IndUS, gcStr_nISt, gcStr_EuroP, gcStr_1Unit};
const char *gcMsg_List_Off_On[]			PROGMEM = {gcStr_OFF, gcStr_On};

// 2011-04-28 -WFC- const char *gcMsg_List_r_Cal[]			PROGMEM = {gcStr_r_CAL};
const char *gcMsg_List_r_Cal[]			PROGMEM = {gcStr_C_CAL};
const char *gcMsg_List_Battery_Life_Mode[]	PROGMEM = {gcStr_StAnd, gcStr_Long};		// 2014-09-30 -WFC-
const char *gcMsg_List_ZBEE_Other[]		PROGMEM = {gcStr_ZBEE, gcStr_OthEr};			// 2016-03-31 -WFC-

#define MENU_FUNC_MAX_ITEMS			sizeof(gcMsg_List_Func) / sizeof(char *)
#define MENU_A_OFF_MAX_ITEMS		sizeof(gcMsg_List_A_OFF) / sizeof(char *)
#define MENU_FILTER_MAX_ITEMS		sizeof(gcMsg_List_Filter) / sizeof(char *)
#define MENU_LED_INTENSITY_ITEMS	sizeof(gcMsg_List_Led_Intensity) / sizeof(char *)
#define MENU_LED_SLEEP_ITEMS		sizeof(gcMsg_List_Led_Sleep) / sizeof(char *)		//	PHJ
#define MENU_UNIT_MAX_ITEMS			sizeof(gcMsg_List_Unit) / sizeof(char *)
#define MENU_TOTAL_MODE_MAX_ITEMS	sizeof(gcMsg_List_Total_Mode) / sizeof(char *)
#define MENU_HIRES_MAX_ITEMS		sizeof(gcMsg_List_Hires) / sizeof(char *)

#define MENU_SETPOINT_CMP_MAX_ITEMS	sizeof(gcMsg_List_Setpoint_Cmp) / sizeof(char *)
#define MENU_SETPOINT_MODE_MAX_ITEMS sizeof(gcMsg_List_Setpoint_Mode) / sizeof(char *)

#define	MENU_STD_MODE_MAX_ITEMS		sizeof(gcMsg_List_Std_Mode) / sizeof(char *)

#define	MENU_OFF_ON_MAX_ITEMS		sizeof(gcMsg_List_Off_On) / sizeof(char *)

#define	MENU_RUN_MODE_CFG_MAX_ITEMS	sizeof(gcMsg_List_Battery_Life_Mode) / sizeof(char *)	// 2014-09-30 -WFC-
#define	MENU_RF_DEV_TYPE_MAX_ITEMS	sizeof(gcMsg_List_ZBEE_Other) / sizeof(char *)			// 2016-03-31 -WFC-


PANEL_MENU_ITEM_T	gacPanel_Menu_UserKey_Func[]	PROGMEM = { gcMsg_List_Func,	panel_setup_user_key_func_mapper,	0,	MENU_FUNC_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_A_OFF[]			PROGMEM = { gcMsg_List_A_OFF,	panel_setup_auto_power_off_mapper,	0,	MENU_A_OFF_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_Filter[]			PROGMEM = { gcMsg_List_Filter,	panel_setup_sensor_filter_mapper,	0,	MENU_FILTER_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_Led_intensity[]	PROGMEM = { gcMsg_List_Led_Intensity, panel_setup_led_intensity_mapper,0, MENU_LED_INTENSITY_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_Led_Sleep[]		PROGMEM = { gcMsg_List_Led_Sleep, panel_setup_led_sleep_mapper,		0, MENU_LED_SLEEP_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};		//	PHJ
PANEL_MENU_ITEM_T	gacPanel_Menu_Unit[]			PROGMEM = { gcMsg_List_Unit,	panel_setup_unit_mapper,			0,	MENU_UNIT_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_Total_Mode[]		PROGMEM = { gcMsg_List_Total_Mode,	panel_setup_total_mode_mapper,	0,	MENU_TOTAL_MODE_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_Battery_Life_Mode[] PROGMEM = { gcMsg_List_Battery_Life_Mode, panel_setup_battery_life_mode_mapper,0,	MENU_RUN_MODE_CFG_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};

// gacPanel_Menu_SetPoint is use by all SetPoint1,2,3 menus, because I program
// the pMethod to handle setpoint setting variables based on the TOP MENU selection index.
// This saved lots of program memory.
PANEL_MENU_ITEM_T	gacPanel_Menu_SetPoint[] PROGMEM = {
	{ gcMsg_List_Setpoint_Cmp, panel_setup_setpoint_cmp_mapper, 0, MENU_SETPOINT_CMP_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE},
	{ gcMsg_List_Setpoint_Mode, panel_setup_setpoint_mode_mapper, 0, MENU_SETPOINT_MODE_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE}, // 2012-09-18 -DLM-
	{ 0, panel_setup_setpoint_value_mapper, 0, PANEL_MENU_ITEM_TYPE_FLOAT},
};

#define PANEL_SETPOINT_MENU_MAX_ITEMS	sizeof(gacPanel_Menu_SetPoint) / sizeof(PANEL_MENU_ITEM_T)

/// List of Top Menus, NOTE: you must update PANEL_TOP_MENU_CNFG_MAX_ITEMS in panel_setup.h file after you added a new top menu.

PANEL_TOP_MENU_T    gacPanel_Top_Menu_Cnfg[ PANEL_TOP_MENU_CNFG_MAX_ITEMS ] PROGMEM = {
 {gcStr_Func1,	gacPanel_Menu_UserKey_Func,	0, MENU_FUNC_MAX_ITEMS				| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_Func2,	gacPanel_Menu_UserKey_Func,	0, MENU_FUNC_MAX_ITEMS				| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_A_OFF,	gacPanel_Menu_A_OFF,		0, MENU_A_OFF_MAX_ITEMS				| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_SLEEP,	gacPanel_Menu_Led_Sleep,	0, MENU_LED_SLEEP_ITEMS				| PANEL_TOP_MENU_TYPE_SIMPLE},	//	PHJ
 {gcStr_LEdS,	gacPanel_Menu_Led_intensity,0, MENU_LED_INTENSITY_ITEMS			| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_StPt1,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt2,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt3,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt4,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt5,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt6,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt7,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt8,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_totAL,	gacPanel_Menu_Total_Mode,	0, MENU_TOTAL_MODE_MAX_ITEMS		| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_Filtr,	gacPanel_Menu_Filter,		0, MENU_FILTER_MAX_ITEMS			| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_Unit,	gacPanel_Menu_Unit,			0, MENU_UNIT_MAX_ITEMS				| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_bLiFE,	gacPanel_Menu_Battery_Life_Mode,	0, MENU_RUN_MODE_CFG_MAX_ITEMS		| PANEL_TOP_MENU_TYPE_SIMPLE}   // 2014-09-30 -WFC-
};

#define PANEL_TOP_MENU_FUNCTION_BASE_INDEX		0	//	PHJ
#define PANEL_TOP_MENU_SETPOINT_BASE_INDEX		5	//	PHJ

// 2011-08-31 -WFC- v
PANEL_TOP_MENU_T    gacPanel_Top_Menu_Cnfg_LFT[ PANEL_TOP_MENU_CNFG_LFT_MAX_ITEMS ] PROGMEM = {
 {gcStr_Func1,	gacPanel_Menu_UserKey_Func,	0, MENU_FUNC_MAX_ITEMS				| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_Func2,	gacPanel_Menu_UserKey_Func,	0, MENU_FUNC_MAX_ITEMS				| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_A_OFF,	gacPanel_Menu_A_OFF,		0, MENU_A_OFF_MAX_ITEMS				| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_SLEEP,	gacPanel_Menu_Led_Sleep,	0, MENU_LED_SLEEP_ITEMS				| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_LEdS,	gacPanel_Menu_Led_intensity,0, MENU_LED_INTENSITY_ITEMS			| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_StPt1,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt2,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt3,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt4,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt5,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt6,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt7,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_StPt8,	gacPanel_Menu_SetPoint,		0, PANEL_SETPOINT_MENU_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_totAL,	gacPanel_Menu_Total_Mode,	0, MENU_TOTAL_MODE_MAX_ITEMS		| PANEL_TOP_MENU_TYPE_SIMPLE}
};
// 2011-08-31 -WFC- ^


// ============================================================================
//				COMMUNICATION PORT GROUP MENU
// ============================================================================
//const char *gcMsg_List_Print_Cntrl_Mode[]	PROGMEM = {gcStr_OFF, gcStr_USEr, gcStr_Load, gcStr_Cont};
//#define MENU_PRINT_CNTRL_MODE_MAX_ITEMS 	sizeof(gcMsg_List_Print_Cntrl_Mode) / sizeof(char *)
//
//PANEL_MENU_ITEM_T	gacPanel_Menu_Print_Cntrl_Mode[]		PROGMEM = { gcMsg_List_Print_Cntrl_Mode, panel_setup_print_control_mode_mapper, 0, MENU_PRINT_CNTRL_MODE_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
//PANEL_MENU_ITEM_T	gacPanel_Menu_Print_Compostie_String[]	PROGMEM = { 0, panel_setup_print_composite_string_mapper,	0, PANEL_MENU_ITEM_TYPE_FLOAT};
//PANEL_MENU_ITEM_T	gacPanel_Menu_Print_Cont_Interval[]		PROGMEM = { 0, panel_setup_print_interval_value_mapper,		0, PANEL_MENU_ITEM_TYPE_FLOAT};
//
///// Print string setting menu items. 2011-06-29 -WFC-
//PANEL_TOP_MENU_T    gacPanel_Top_Menu_Print_String[ PANEL_TOP_MENU_PRINT_STRING_MAX_ITEMS ] PROGMEM = {
// {gcStr_StrnG,	gacPanel_Menu_Print_Compostie_String,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},
// {gcStr_Cntrl,	gacPanel_Menu_Print_Cntrl_Mode,			0,	MENU_PRINT_CNTRL_MODE_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE},
// {gcStr_rAtE,	gacPanel_Menu_Print_Cont_Interval,		0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX}								// MENU_NEW_COM_CONT_INTERVAL
//};

// 2015-09-25 -WFC- v
const char *gcMsg_List_Print_Output_Port[]	PROGMEM = {gcStr_Port_0, gcStr_rF};
#define MENU_PRINT_OUTPUT_PORT_MAX_ITEMS 	sizeof(gcMsg_List_Print_Output_Port) / sizeof(char *)

const char *gcMsg_List_Print_Cntrl_Mode[]	PROGMEM = {gcStr_OFF, gcStr_USEr, gcStr_Load, gcStr_Cont};
#define MENU_PRINT_CNTRL_MODE_MAX_ITEMS 	sizeof(gcMsg_List_Print_Cntrl_Mode) / sizeof(char *)

PANEL_MENU_ITEM_T	gacPanel_Menu_Print_ListenerID[]		PROGMEM = { 0, panel_setup_print_listener_id_mapper, 0, PANEL_MENU_ITEM_TYPE_FLOAT}; // 2014-07-15 -WFC-
PANEL_MENU_ITEM_T	gacPanel_Menu_Print_Output_Port[]		PROGMEM = { gcMsg_List_Print_Output_Port, panel_setup_print_output_port_mapper, 0, MENU_PRINT_OUTPUT_PORT_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_Print_Cntrl_Mode[]		PROGMEM = { gcMsg_List_Print_Cntrl_Mode, panel_setup_print_control_mode_mapper, 0, MENU_PRINT_CNTRL_MODE_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_Print_Compostie_String[]	PROGMEM = { 0, panel_setup_print_composite_string_mapper,	0, PANEL_MENU_ITEM_TYPE_FLOAT};
PANEL_MENU_ITEM_T	gacPanel_Menu_Print_Cont_Interval[]		PROGMEM = { 0, panel_setup_print_interval_value_mapper,		0, PANEL_MENU_ITEM_TYPE_FLOAT};

/// Print string setting menu items. 2011-06-29 -WFC-
PANEL_TOP_MENU_T    gacPanel_Top_Menu_Print_String[ PANEL_TOP_MENU_PRINT_STRING_MAX_ITEMS ] PROGMEM = {
 {gcStr_Listnr,	gacPanel_Menu_Print_ListenerID,			0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_Output,	gacPanel_Menu_Print_Output_Port,		0,	MENU_PRINT_OUTPUT_PORT_MAX_ITEMS | PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_StrnG,	gacPanel_Menu_Print_Compostie_String,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_Cntrl,	gacPanel_Menu_Print_Cntrl_Mode,			0,	MENU_PRINT_CNTRL_MODE_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_rAtE,	gacPanel_Menu_Print_Cont_Interval,		0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX}								// MENU_NEW_COM_CONT_INTERVAL
};

//PANEL_TOP_MENU_T    gacPanel_Top_Menu_Print_String[ PANEL_TOP_MENU_PRINT_STRING_MAX_ITEMS ] PROGMEM = {
// {gcStr_StrnG,	gacPanel_Menu_Print_Compostie_String,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},
// {gcStr_Cntrl,	gacPanel_Menu_Print_Cntrl_Mode,			0,	MENU_PRINT_CNTRL_MODE_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE},
// {gcStr_rAtE,	gacPanel_Menu_Print_Cont_Interval,		0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX}								// MENU_NEW_COM_CONT_INTERVAL
//};
// 2015-09-25 -WFC- ^


/// enum for above Print String menu.
enum {
	MENU_PRINT_COMPOSITE_STRING,
	MENU_PRINT_CNTRL_MODE,
	MENU_PRINT_CONT_INTERVAL
};

/// RF device setting menu items. 2012-04-27 -WFC- v
PANEL_MENU_ITEM_T	gacPanel_Menu_RF_Device_OnOFF[]	PROGMEM = { gcMsg_List_Off_On, panel_setup_rf_device_on_off_mapper, 0, MENU_OFF_ON_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_RF_Sc_ID[]		PROGMEM = { 0, panel_setup_rf_sc_id_mapper,	0, PANEL_MENU_ITEM_TYPE_FLOAT};
PANEL_MENU_ITEM_T	gacPanel_Menu_RF_Channel[]		PROGMEM = { 0, panel_setup_rf_channel_mapper, 0, PANEL_MENU_ITEM_TYPE_FLOAT};
PANEL_MENU_ITEM_T	gacPanel_Menu_RF_networkID[]	PROGMEM = { 0, panel_setup_rf_network_id_mapper, 0, PANEL_MENU_ITEM_TYPE_FLOAT};
PANEL_MENU_ITEM_T	gacPanel_Menu_RF_Dev_type[]		PROGMEM = { gcMsg_List_ZBEE_Other, panel_setup_rf_device_type_mapper, 0, MENU_RF_DEV_TYPE_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};	// 2016-03-31 -WFC-
PANEL_MENU_ITEM_T	gacPanel_Menu_RF_Dev_Always_On[]	PROGMEM = { gcMsg_List_Off_On, panel_setup_rf_device_always_on_mapper, 0, MENU_OFF_ON_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};		// 2016-03-31 -WFC-

PANEL_TOP_MENU_T    gacPanel_Top_Menu_RF_Device_Cnfg[ PANEL_TOP_MENU_RF_DEVICE_CNFG_MAX_ITEMS ] PROGMEM = {
 {gcStr_On_OFF,	gacPanel_Menu_RF_Device_OnOFF,	0,	MENU_OFF_ON_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_ScId,	gacPanel_Menu_RF_Sc_ID,			0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_ChnL,	gacPanel_Menu_RF_Channel,		0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_nEtId,	gacPanel_Menu_RF_networkID,		0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_tyPE,	gacPanel_Menu_RF_Dev_type,		0,	MENU_RF_DEV_TYPE_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE},		// 2016-03-31 -WFC-
 {gcStr_Hold,	gacPanel_Menu_RF_Dev_Always_On,	0,	MENU_OFF_ON_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE}			// 2016-03-31 -WFC-
};

/// Ethernet device setting menu items. 2012-07-06 -DLM- v
PANEL_MENU_ITEM_T	gacPanel_Menu_Ethernet_Device_OnOFF[]	PROGMEM = { gcMsg_List_Off_On, panel_setup_ethernet_device_on_off_mapper, 0, MENU_OFF_ON_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};
PANEL_MENU_ITEM_T	gacPanel_Menu_Ethernet_Sc_ID[]			PROGMEM = { 0, panel_setup_ethernet_sc_id_mapper,	0, PANEL_MENU_ITEM_TYPE_FLOAT};

PANEL_TOP_MENU_T    gacPanel_Top_Menu_Ethernet_Device_Cnfg[ PANEL_TOP_MENU_ETHERNET_DEVICE_CNFG_MAX_ITEMS ] PROGMEM = {
 {gcStr_On_OFF,	gacPanel_Menu_Ethernet_Device_OnOFF,	0,	MENU_OFF_ON_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_ScId,	gacPanel_Menu_Ethernet_Sc_ID,			0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX}
};


/// Communication group menus.
PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T		gacPanel_Menu_Com_Port_Group_Item_Entry[ PANEL_TOP_MENU_COM_PORT_GROUP_MAX_ITEMS ]	PROGMEM = {
 {gacPanel_Top_Menu_Print_String,	PANEL_TOP_MENU_PRINT_STRING_MAX_ITEMS,	MENU_MSG_ID_Print,	TT_1SEC, PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL, panel_setup_print_string_top_menu_manger},
 {gacPanel_Top_Menu_RF_Device_Cnfg,	PANEL_TOP_MENU_RF_DEVICE_CNFG_MAX_ITEMS, MENU_MSG_ID_rF,	TT_1SEC, PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL, panel_setup_print_string_top_menu_manger},
 {gacPanel_Top_Menu_Ethernet_Device_Cnfg,	PANEL_TOP_MENU_ETHERNET_DEVICE_CNFG_MAX_ITEMS, MENU_MSG_ID_Ethernet,	TT_1SEC, PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL, panel_setup_print_string_top_menu_manger}
};

// 2012-04-27 -WFC- ^


PANEL_MENU_ITEM_T	gacPanel_Menu_Std_Mode[]		PROGMEM = { gcMsg_List_Std_Mode,	panel_setup_std_mode_mapper,	0,	MENU_STD_MODE_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};

PANEL_MENU_ITEM_T	gacPanel_Menu_Azm[]				PROGMEM = { gcMsg_List_Off_On,	panel_setup_azm_mapper,	0,	MENU_OFF_ON_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};

// 2016-03-23 -WFC- Zero on Power up menu item.
PANEL_MENU_ITEM_T	gacPanel_Menu_Zop[]				PROGMEM = { gcMsg_List_Off_On,	panel_setup_zop_mapper,	0,	MENU_OFF_ON_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};

PANEL_MENU_ITEM_T	gacPanel_Menu_Cal_Unit[]		PROGMEM = { gcMsg_List_Unit,	panel_setup_cal_unit_mapper,	0,	MENU_UNIT_MAX_ITEMS | PANEL_MENU_ITEM_TYPE_SIMPLE};

PANEL_MENU_ITEM_T	gacPanel_Menu_Capacity[]		PROGMEM = { 0, panel_setup_capacity_mapper, 0, PANEL_MENU_ITEM_TYPE_FLOAT };

PANEL_MENU_ITEM_T	gacPanel_Menu_Countby[] 		PROGMEM = { 0, panel_setup_countby_mapper, 0, PANEL_MENU_ITEM_TYPE_DYNAMIC_CHOICE };

PANEL_MENU_ITEM_T	gacPanel_Menu_Cal_Zero[] 		PROGMEM = { gcMsg_List_Cal_Zero, panel_setup_cal_zero_mapper, 0, 1 | PANEL_MENU_ITEM_TYPE_SIMPLE };

PANEL_MENU_ITEM_T	gacPanel_Menu_Cal_Load[] 		PROGMEM = { 0, panel_setup_cal_load_mapper, PANEL_MENU_ITEM_DECPT_USE_CB, PANEL_MENU_ITEM_TYPE_FLOAT };

PANEL_MENU_ITEM_T	gacPanel_Menu_New_RCal_Zero[]	PROGMEM = { gcMsg_List_Cal_Zero, panel_setup_new_rcal_zero_mapper, 0, 1 | PANEL_MENU_ITEM_TYPE_SIMPLE };

PANEL_MENU_ITEM_T	gacPanel_Menu_RCal_Zero[] 		PROGMEM = { gcMsg_List_Cal_Zero, panel_setup_rcal_zero_mapper, 0, 1 | PANEL_MENU_ITEM_TYPE_SIMPLE };

PANEL_MENU_ITEM_T	gacPanel_Menu_RCal_Load[] 		PROGMEM = { 0, panel_setup_rcal_load_mapper, PANEL_MENU_ITEM_DECPT_USE_CB, PANEL_MENU_ITEM_TYPE_FLOAT };

PANEL_MENU_ITEM_T	gacPanel_Menu_Show_Rcal[] 		PROGMEM = { gcMsg_List_r_Cal, panel_setup_show_rcal_mapper, 0, 1 | PANEL_MENU_ITEM_TYPE_SIMPLE };

// 2011-04-20 -WFC- v added for multi point calibration.
/// Initial brand new Calibration menu group
PANEL_TOP_MENU_T    gacPanel_Top_Menu_New_Cal[ PANEL_TOP_MENU_NEW_CAL_MAX_ITEMS ] PROGMEM = {
 {gcStr_Unit,	gacPanel_Menu_Cal_Unit,	0,	MENU_UNIT_MAX_ITEMS		| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_CAP,	gacPanel_Menu_Capacity,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_d,		gacPanel_Menu_Countby,	0,	1	| PANEL_TOP_MENU_TYPE_DYNAMIC_CHOICE},
 {gcStr_UnLd,	gacPanel_Menu_Cal_Zero,	0,	1	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_Load1,	gacPanel_Menu_Cal_Load,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},		//MENU_NEW_CAL_STEP_LOAD_1
 {gcStr_Load2,	gacPanel_Menu_Cal_Load,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},		//MENU_NEW_CAL_STEP_LOAD_2
 {gcStr_Load3,	gacPanel_Menu_Cal_Load,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},		//MENU_NEW_CAL_STEP_LOAD_3
 {gcStr_Load4,	gacPanel_Menu_Cal_Load,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},		//MENU_NEW_CAL_STEP_LOAD_LAST

 {gcStr_CAL_d ,	gacPanel_Menu_Show_Rcal, 0,	1	| PANEL_TOP_MENU_TYPE_SIMPLE}
};

/// enum for above brand new calibration menu group.
enum {
	MENU_NEW_CAL_STEP_UNIT,
	MENU_NEW_CAL_STEP_CAP,
	MENU_NEW_CAL_STEP_COUNTBY,
	MENU_NEW_CAL_STEP_ZEROING,
	MENU_NEW_CAL_STEP_LOAD_1,
	MENU_NEW_CAL_STEP_LOAD_2,
	MENU_NEW_CAL_STEP_LOAD_3,
	MENU_NEW_CAL_STEP_LOAD_LAST,
	MENU_NEW_CAL_STEP_READ_RCAL
};



/// Initial brand new RCal Calibration menu group
PANEL_TOP_MENU_T    gacPanel_Top_Menu_New_RCal[ PANEL_TOP_MENU_NEW_RCAL_MAX_ITEMS ] PROGMEM = {
 {gcStr_Unit,	gacPanel_Menu_Cal_Unit,	0,	MENU_UNIT_MAX_ITEMS		| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_CAP,	gacPanel_Menu_Capacity,	0,		1	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_d,		gacPanel_Menu_Countby,	0,		1	| PANEL_TOP_MENU_TYPE_DYNAMIC_CHOICE},
 {gcStr_UnLd,	gacPanel_Menu_New_RCal_Zero, 0,	1	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_C_CAL,	gacPanel_Menu_RCal_Load, 0,		1	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_CAL_d ,	gacPanel_Menu_Show_Rcal, 0,		1	| PANEL_TOP_MENU_TYPE_SIMPLE}
};

/// enum for above brand new R calibration menu group.
enum {
	MENU_NEW_R_CAL_STEP_UNIT,
	MENU_NEW_R_CAL_STEP_CAP,
	MENU_NEW_R_CAL_STEP_COUNTBY,
	MENU_NEW_R_CAL_STEP_ZEROING,
	MENU_NEW_R_CAL_STEP_GET_R_CAL,
	MENU_NEW_R_CAL_STEP_READ_RCAL
};


/// Re calibration or subsequent calibration
PANEL_TOP_MENU_T    gacPanel_Top_Menu_ReCal[ PANEL_TOP_MENU_RECAL_MAX_ITEMS ] PROGMEM = {
 {gcStr_UnLd,	gacPanel_Menu_Cal_Zero,	0,	1	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_Load1,	gacPanel_Menu_Cal_Load,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},	//MENU_RE_CAL_STEP_LOAD_1
 {gcStr_Load2,	gacPanel_Menu_Cal_Load,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},	//MENU_RE_CAL_STEP_LOAD_2
 {gcStr_Load3,	gacPanel_Menu_Cal_Load,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},	//MENU_RE_CAL_STEP_LOAD_3
 {gcStr_Load4,	gacPanel_Menu_Cal_Load,	0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},	//MENU_RE_CAL_STEP_LOAD_LAST
 {gcStr_CAL_d,	gacPanel_Menu_Show_Rcal,0,	1	| PANEL_TOP_MENU_TYPE_SIMPLE}
};

/// enum for above subsequent calibration menu group.
enum { 
	MENU_RE_CAL_STEP_ZEROING,
	MENU_RE_CAL_STEP_LOAD_1,
	MENU_RE_CAL_STEP_LOAD_2,
	MENU_RE_CAL_STEP_LOAD_3,
	MENU_RE_CAL_STEP_LOAD_LAST,
	MENU_RE_CAL_STEP_READ_RCAL
};
// 2011-04-20 -WFC- ^

/// Subsequent Re RCal calibration
PANEL_TOP_MENU_T    gacPanel_Top_Menu_Re_RCal[ PANEL_TOP_MENU_RE_RCAL_MAX_ITEMS ] PROGMEM = {
 {gcStr_UnLd,	gacPanel_Menu_RCal_Zero, 0,	1	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_C_CAL,	gacPanel_Menu_RCal_Load, 0,	1	| PANEL_TOP_MENU_TYPE_COMPLEX},
 {gcStr_CAL_d,	gacPanel_Menu_Show_Rcal, 0,	1	| PANEL_TOP_MENU_TYPE_SIMPLE}
};

/// enum for above subsequent Re R calibration menu group.
enum {
	MENU_RE_R_CAL_STEP_ZEROING,
	MENU_RE_R_CAL_STEP_GET_R_CAL,
	MENU_RE_R_CAL_STEP_READ_RCAL
};

/// Restrict settings such as standard mode, AZM
PANEL_TOP_MENU_T    gacPanel_Top_Menu_Restrict_Cnfg[ PANEL_TOP_MENU_RESTRICT_CNFG_MAX_ITEMS ] PROGMEM = {
 {gcStr_StAnd,	gacPanel_Menu_Std_Mode,	0,	MENU_STD_MODE_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_Azt,	gacPanel_Menu_Azm,		0,	MENU_OFF_ON_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE},
 {gcStr_Zop,	gacPanel_Menu_Zop,		0,	MENU_OFF_ON_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE},		// 2016-03-23 -WFC-
 {gcStr_Filtr,	gacPanel_Menu_Filter,	0,	MENU_FILTER_MAX_ITEMS	| PANEL_TOP_MENU_TYPE_SIMPLE}		// 2011-08-31 -WFC-
};

/// calibration group menus. Note that after added or removed an entry, you need to modify panel_setup_cal_menu_group_method() to skip ReCal and RCal based on cal status.

PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T		gacPanel_Menu_Cal_Group_Item_Entry[ PANEL_TOP_MENU_CAL_GROUP_MAX_ITEMS ]	PROGMEM = {
 {gacPanel_Top_Menu_New_Cal,		PANEL_TOP_MENU_NEW_CAL_MAX_ITEMS,		MENU_MSG_ID_CAL,	TT_1SEC, PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL, panel_setup_does_it_allows_new_cal},
 {gacPanel_Top_Menu_New_RCal,		PANEL_TOP_MENU_NEW_RCAL_MAX_ITEMS,		MENU_MSG_ID_CAL_R,	TT_1SEC, PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL, panel_setup_does_it_allows_new_rcal},
 {gacPanel_Top_Menu_Restrict_Cnfg,	PANEL_TOP_MENU_RESTRICT_CNFG_MAX_ITEMS,	MENU_MSG_ID_SEtuP,	TT_1SEC, PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL, 0 },
};

// 2011-04-20 -WFC- v
/// enum for above calibration group menu item.
enum {
	MENU_CAL_GROUP_NEW_CAL,
	MENU_CAL_GROUP_NEW_RCAL,
	MENU_CAL_GROUP_RESTRICT_CNFG
};
// 2011-04-20 -WFC- ^


// 2011-04-26 -WFC- v
PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T		gacPanel_Menu_ReCal_Group_Item_Entry[ PANEL_TOP_MENU_RECAL_GROUP_MAX_ITEMS ]	PROGMEM = {
 {gacPanel_Top_Menu_ReCal,			PANEL_TOP_MENU_RECAL_MAX_ITEMS,			MENU_MSG_ID_CAL,	TT_1SEC, PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL, panel_setup_does_it_allows_recal},
 {gacPanel_Top_Menu_Re_RCal,		PANEL_TOP_MENU_RE_RCAL_MAX_ITEMS,		MENU_MSG_ID_CAL_R,	TT_1SEC, PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL, panel_setup_does_it_allows_re_rcal},
 {gacPanel_Top_Menu_Restrict_Cnfg,	PANEL_TOP_MENU_RESTRICT_CNFG_MAX_ITEMS,	MENU_MSG_ID_SEtuP,	TT_1SEC, PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL, 0 },
};

/// enum for above re-calibration group menu item.
enum {
	MENU_RECAL_GROUP_CAL,
	MENU_RECAL_GROUP_RCAL,
	MENU_RECAL_GROUP_RESTRICT_CNFG
};

// 2011-04-26 -WFC- ^

/// setup listener ID for configure print string and its output port. 2015-09-25 -WFC-
BYTE gbSetupListenerID;

/**
 * Panel setup TOP menu group thread. This is the entry main thread of the root of TOP menu group.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu group object
 *
 * History:  Created on 2009/09/28 by Wai Fai Chin
 * 2011-04-26 -WFC- Added logic to control item permission to interact with user.
 * 2011-05-03 -WFC- Clear all annunciators at the beginning of thread.
 * 2012-05-16 -WFC- init to no blinking. 2012-06-20 -DLM-
 */

//PT_THREAD( panel_setup_top_menu_group_thread( PANEL_SETUP_TOP_MENU_GROUP_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_setup_top_menu_group_thread( PANEL_SETUP_TOP_MENU_GROUP_CLASS *pSetupObj )
{
  BYTE				key;
  PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T	aGroupItem;

  PT_BEGIN( &pSetupObj->m_pt );
	gLedDisplayManager.descr.digitBlinkPos = 0;			// 2012-05-16 -WFC- init to no blinking.
    // 2011-05-03 -WFC- v
	led_display_turn_all_annunciators( OFF);
	PT_INIT( &gLedDisplayManager.anc_pt );
	led_display_manage_annunciator_thread( &gLedDisplayManager );
    // 2011-05-03 -WFC- ^

	// init object variables at the very first time of this thread.
	gbPanelSetupStatus = 0;			// clear status.
	pSetupObj-> selectIndex = 0;	// default selection
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
	if ( pSetupObj-> pMethod ) {
		(*pSetupObj-> pMethod)( pSetupObj );		// this method determine which selection is the first and which one is allow.
	}
	
	// display first menu message.
	memcpy_P( &aGroupItem,  &pSetupObj-> pRootItem[ pSetupObj-> selectIndex ],  sizeof( PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T ));	
	panel_setup_display_msg_duration_P( aGroupItem.msgID, aGroupItem.msgDisplayTime );
	PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for displayTime seconds delay.

	panel_key_init();	// empty key buffer so the power key will not power down the device.
		
	for(;;) {
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_key_get( &key ) );				// wait until get a key.
		if ( PANEL_MENU_SCROLL_KEY == key ) {			// selection key
//			pSetupObj-> selectIndex++;
//			if ( pSetupObj-> selectIndex > (pSetupObj-> maxNumOfItem - 1) )
//				pSetupObj-> selectIndex = 0;

			pSetupObj-> selectIndex = panel_setup_get_next_select_item_index(
					pSetupObj-> selectIndex, pSetupObj-> maxNumOfItem, pSetupObj->disabledItemFlags ); // 2011-04-26 -WFC-
			
			if ( pSetupObj-> pMethod ) {
				(*pSetupObj-> pMethod)( pSetupObj );		// this method dynamically set the next selection index
			}

			memcpy_P( &aGroupItem,  &pSetupObj-> pRootItem[ pSetupObj-> selectIndex ],  sizeof( PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T ));
			panel_setup_display_msg_duration_P( aGroupItem.msgID, aGroupItem.msgDisplayTime );
			if (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) ) {
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				break;
			}
		}
		else if ( PANEL_MENU_ENTER_KEY == key ) {		// enter key
			// construct gPanelSetupSubMenuObj1 into a thread.
			memcpy_P( &aGroupItem,  &pSetupObj-> pRootItem[ pSetupObj-> selectIndex ],  sizeof( PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T ));
			// construct gPanelSetupTopMenuObj into a thread.
			gPanelSetupTopMenuObj.msgID				= aGroupItem.msgID;				// name of this group menu
			gPanelSetupTopMenuObj.msgDisplayTime	= aGroupItem.msgDisplayTime;		// display time at the beginning.
			gPanelSetupTopMenuObj.pMethod			= aGroupItem.pMethod;
			gPanelSetupTopMenuObj.pRootMenu			= aGroupItem.pRootMenu;			// points to a root of top menu.
			gPanelSetupTopMenuObj.maxNumOfItem		= aGroupItem.maxNumOfItem;
			gPanelSetupTopMenuObj.curMove			= PSTMC_CURMOVE_INIT_STATE;
			gPanelSetupTopMenuObj.parentIs 			= PSTMC_PARENT_IS_TOP_MENU_GROUP;	// Its parent is top menu group.
			gPanelSetupTopMenuObj.disabledItemFlags = 0;	// enabled all menu items. 2011-04-26 -WFC-
			PT_SPAWN( &pSetupObj-> m_pt, &gPanelSetupTopMenuObj.m_pt, panel_setup_top_menu_thread( &gPanelSetupTopMenuObj));

			gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
			//if (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) ) {
			//	break;
			//}
			
			if (gbPanelSetupStatus & PANEL_SETUP_STATUS_EXIT_SAVE ) {
				if ( pSetupObj-> pMethod ) {
					(*pSetupObj-> pMethod)( pSetupObj );		// Caller supplies this method to save settings, set the gPanelMainRunModeThreadTimer;
					PT_WAIT_UNTIL( &pSetupObj-> m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for duration of msgDisplayTime.
				}
			}
			
			// here, the thread completed a top menu group entry, advanced to the next top menu group entry.
			pSetupObj-> selectIndex = panel_setup_get_next_select_item_index(
					pSetupObj-> selectIndex, pSetupObj-> maxNumOfItem, pSetupObj->disabledItemFlags ); // 2011-04-26 -WFC-
				
			if ( pSetupObj-> pMethod ) {
				(*pSetupObj-> pMethod)( pSetupObj );		// this method dynamically set the next selection index
			}
				
			memcpy_P( &aGroupItem,  &pSetupObj-> pRootItem[ pSetupObj-> selectIndex ],  sizeof( PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T ));
			panel_setup_display_msg_duration_P( aGroupItem.msgID, aGroupItem.msgDisplayTime );
		}
		else if ( PANEL_ZERO_KEY == key || (gbPanelSetupStatus & PANEL_SETUP_STATUS_EXIT_SAVE) ) {		// quit key
			key = YES;								// yes to save.
			break;
		}
		else if ( PANEL_POWER_KEY == key ) {		// power off key.
			key = NO;
			// TODO: power off this device
			break;
		}
	} // end for(;;)

	panel_setup_exit( key );	// perform exit house keeping includes display exit msg.

	PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for 2 seconds delay to display exit msg.
	panel_key_init();	// empty key buffer so the power key will not power down the device.

	gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;			// tell the panel to run in normal mode.
	
  PT_END(&pSetupObj->m_pt);
} // end panel_setup_top_menu_group_thread()


/**
 * It tries to advance to next item if new item has permission to interact.
 *
 * @param  selectIndex		 -- current selection index
 * @param  maxNumItem		 -- maximum number of items in this group.
 * @param  disabledItemFlags -- bit value 1== disabled, 0== enabled.
 * @return new selected index based on disabledItemFlags
 *
 * History:  Created on 2011-04-26 by Wai Fai Chin
 */

BYTE	panel_setup_get_next_select_item_index( BYTE selectIndex, BYTE maxNumItem, UINT16 disabledItemFlags )
{
	BYTE	i;
	BYTE	newIndex;

	newIndex = selectIndex;
	for ( i = 0; i < maxNumItem; i++) {
		newIndex++;
		if ( newIndex > (maxNumItem - 1) )
			newIndex = 0;

		if ( !(disabledItemFlags & ~(1 << newIndex)) ) {
			break;
		}
	}
	return newIndex;
} // end panel_setup_get_next_select_item_index(,,)

/**
 * Panel setup TOP menu thread. This is the entry main thread of the root of TOP menu.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu object
 *
 * History:  Created on 2009/07/10 by Wai Fai Chin
 * 2011-04-26 -WFC- Added logic to dynamically lock out menu items.
 * 2011-04-27 -WFC- call object method up on ESC (zero) key to determine to exit or stay in a menu item group of a new selectIndex
 * 2011-04-28 -WFC- At the beginning of the thread, wait until all keys are released. This prevent user hold down a key caused to skip the first menu item.
 * 2011-05-03 -WFC- Clear all annunciators at the beginning of thread.
 * 2011-05-03 -WFC- if object does not have its own method, then assumed it wants to save. This fixed zero key caused cancel setup in setup mode.
 * 2012-05-16 -WFC- init to no blinking. 2012-06-20 -DLM-
 *
 */

//PT_THREAD( panel_setup_top_menu_thread( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_setup_top_menu_thread( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )
{
  BYTE 				key;
  PANEL_TOP_MENU_T	topMenu;

  PT_BEGIN( &pSetupObj->m_pt );
	gLedDisplayManager.descr.digitBlinkPos = 0;			// 2012-05-16 -WFC- init to no blinking.
	// 2011-05-03 -WFC- v
	led_display_turn_all_annunciators( OFF);
	PT_INIT( &gLedDisplayManager.anc_pt );
	led_display_manage_annunciator_thread( &gLedDisplayManager );
	// 2011-05-03 -WFC- ^
	// init object variables at the very first time of this thread.
	gbPanelSetupStatus = 0;		// clear status.
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
	
	if ( pSetupObj-> msgDisplayTime ) {		// if a message required to display before the rest of the menu.
		panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );
		if ( pSetupObj-> pMethod ) {
			key = (*pSetupObj-> pMethod)( pSetupObj );
			if ( !key ) {	// if this top menu setup NOT ALLOW:
				panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
				PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;
				panel_key_init();	// empty key buffer so the power key will not power down the device.
				PT_EXIT( &pSetupObj->m_pt );	// end this thread.
			}
		}
	}
	
	// clear no more header msg to display.
	pSetupObj-> msgDisplayTime =
	pSetupObj-> selectIndex = 0;
	// display first menu message.
	memcpy_P( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));	
	led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, topMenu.pMsg);
	led_display_string( gLedDisplayManager.descr.str1 );		// display it immediately.

	// led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ].pMsg); cannot do this because points at SRAM and give you bogus content.
	// The compiler can only get first level of code space content, which is pRootMenu, then the compiler thinks that it is in RAM space, which in turn give you bogus content.
	// end init thread.

	for (;;) {
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_200mSEC);
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for 0.5 second delay.
		if ( !v_key_detect_pressed() ) // direct scan, wait until all keys are released. This prevent user hold down a key caused to skip the first menu item. 2011-04-28 -WFC-
			break;
	}

	panel_key_init();	// empty key buffer so the power key will not power down the device.
	
	for(;;) {
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_key_get( &key ) );				// wait until get a key.
		if ( PANEL_MENU_SCROLL_KEY == key ) {			// selection key
//			pSetupObj-> selectIndex++;
//			if ( pSetupObj-> selectIndex > (pSetupObj-> maxNumOfItem - 1) )
//				pSetupObj-> selectIndex = 0;

			pSetupObj-> selectIndex = panel_setup_get_next_select_item_index(
					pSetupObj-> selectIndex, pSetupObj-> maxNumOfItem, pSetupObj->disabledItemFlags ); // 2011-04-26 -WFC-
				
			if ( pSetupObj-> pMethod ) {
				pSetupObj-> curMove = PSTMC_CURMOVE_NEXT;
				(*pSetupObj-> pMethod)( pSetupObj );		// this method dictates the selection index base on the obj's internal logics.
			}
			
			memcpy_P ( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));	
			led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, topMenu.pMsg);
			if (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) ) {
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				break;
			}
		}
		else if ( PANEL_MENU_ENTER_KEY == key ) {		// enter key
			// construct gPanelSetupSubMenuObj1 into a thread.
			memcpy_P( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));
			gPanelSetupSubMenuObj1.pRootItem	= topMenu.pRootItem;
			gPanelSetupSubMenuObj1.maxNumOfItem	= topMenu.maxNumOfItem;
			gPanelSetupSubMenuObj1.nextMove		= 1;		// default next move is one step.
			gPanelSetupSubMenuObj1.msgDisplayTime= 0;		// assume no msg to be display.
			if ( PANEL_TOP_MENU_TYPE_COMPLEX == (topMenu.maxNumOfItem & PANEL_TOP_MENU_TYPE_MASK) ) {		// It is a complex menu type.
				PT_SPAWN( &pSetupObj->m_pt, &gPanelSetupSubMenuObj1.m_pt, panel_setup_complex_sub_menu_thread( &gPanelSetupSubMenuObj1));
			}
			else if ( PANEL_TOP_MENU_TYPE_SIMPLE == (topMenu.maxNumOfItem & PANEL_TOP_MENU_TYPE_MASK) ) {		// It is a simple list menu type.{
				PT_SPAWN( &pSetupObj->m_pt, &gPanelSetupSubMenuObj1.m_pt, panel_setup_simple_sub_menu_thread( &gPanelSetupSubMenuObj1));
			}
			else if ( PANEL_TOP_MENU_TYPE_DYNAMIC_CHOICE == (topMenu.maxNumOfItem & PANEL_TOP_MENU_TYPE_MASK) ) {		// It is a dyanamic choice menu type.{
				PT_SPAWN( &pSetupObj->m_pt, &gPanelSetupSubMenuObj1.m_pt, panel_setup_dynamic_choice_sub_menu_thread( &gPanelSetupSubMenuObj1));
			}

			gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
			if (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) ) {
				break;
			}
			
			// here, the thread completed sub menu entry, advanced to the next top menu entry.
			// pSetupObj-> selectIndex += gPanelSetupSubMenuObj1.nextMove;
			pSetupObj-> selectIndex = panel_setup_get_next_select_item_index(
					pSetupObj-> selectIndex, pSetupObj-> maxNumOfItem, pSetupObj->disabledItemFlags ); // 2011-04-26 -WFC-
				
			if ( pSetupObj-> pMethod ) {
				pSetupObj-> curMove = PSTMC_CURMOVE_ENTER;
				(*pSetupObj-> pMethod)( pSetupObj );		// this method dictates the selection index base on the obj's internal logics.
			}
				
			memcpy_P( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));	
			led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, topMenu.pMsg);
		}
		else if ( PANEL_ZERO_KEY == key || (gbPanelSetupStatus & PANEL_SETUP_STATUS_EXIT_SAVE) ) {		// quit key
			key = NO;
			// 2011-04-27 -WFC- v
			if ( !(gbPanelSetupStatus & PANEL_SETUP_STATUS_EXIT_SAVE) ) {
				// call object method to determine to exit or stay in a menu item group of a new selectIndex.
				if ( pSetupObj-> pMethod ) {
					pSetupObj-> curMove = PSTMC_CURMOVE_EXIT;
					(*pSetupObj-> pMethod)( pSetupObj );		// this method dictates the selection index base on the obj's internal logics.
					if ( PSTMC_CURMOVE_NEXT == pSetupObj->curMove ) {
						memcpy_P( &topMenu,  &pSetupObj-> pRootMenu[ pSetupObj-> selectIndex ],  sizeof(PANEL_TOP_MENU_T));
						led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, topMenu.pMsg);
					}
					else {
						break;
					}
				}
				else {
					key = YES;			// 2011-05-03 -WFC- if object does not have its own method, then assumed it wants to save. This fixed zero key caused cancel setup in setup mode.
					break;
				}
			}
			else {
				key = YES;								// yes to save.
				// 2011-04-27 -WFC- gbLbFlag = NO;							//	PHJ
				break;
			}
			// 2011-04-27 -WFC- ^
		}
		else if ( PANEL_POWER_KEY == key ) {		// power off key.
			key = NO;
			// 2011-04-27 -WFC-  gbLbFlag = NO;							//	PHJ
			break;
		}
	} // end for(;;)

	if ( PSTMC_PARENT_NONE == pSetupObj-> parentIs ) {		// if this object has no parent
		panel_setup_exit( key );	// perform exit house keeping includes display exit msg.

		PT_WAIT_UNTIL( &pSetupObj-> m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for 2 seconds delay to display exit msg.
		panel_key_init();	// empty key buffer so the power key will not power down the device.

		gbPanelMainRunMode = PANEL_RUN_MODE_NORMAL;			// tell the panel to run in normal mode.
	}
	
  PT_END(&pSetupObj->m_pt);
} // end panel_setup_top_menu_thread()




/**
 * Panel setup of simple sub menu thread. This thread is spawned off by either
 *  panel_setup_top_menu_thread() or panel_setup_complex_sub_menu_thread().
 *
 * @param  pSetupObj	-- pointer to panel setup sub simple menu object
 * @note simple sub menu type only has ONE menuItem. Its menuItem.pMsg points
 *       to a list of msg instead just one msg. Its menuItem.maxNumOfItem is
 *       number of simple selection in a list, which also means number of msg in the list points by pMsg.
 *
 * History:  Created on 2009/07/13 by Wai Fai Chin
 * 2011-07-18 -WFC- Default behaviors is to flash this message. However, the menuItem.pMethod() can override this behaviors.
 */

//PT_THREAD( panel_setup_simple_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_setup_simple_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )
{
  BYTE				key;
  BYTE				*pStr;
  PANEL_MENU_ITEM_T	menuItem;

  PT_BEGIN( &pSetupObj->m_pt );
  	// init object variables at the very first time of this thread.
	pSetupObj-> msgID			=
	pSetupObj-> msgDisplayTime	= 0;

	memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// Default behaviors is to flash this message. However, the menuItem.pMethod() can override this behaviors. 2011-07-18 -WFC-
	pSetupObj-> selectIndex = 0;
	// map variable content into selection index.
	pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_GET_CNFG_INDEX, pSetupObj, 0);
	// simple menuItem pMsg contains list of pointers to string. Copy the string pointer from PGM to RAM variable pointer pStr which still points at PGM.
	memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
	// show what is in the setting variable on LED display.
	led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
	// 2011-07-18 -WFC- move up to the above statements.  gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.

	if ( 0 == pSetupObj-> nextMove ) {									// If menu selection has an error OR the method wants to exit.
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds
		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
		panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display selection error msg.
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// for duration of msgDisplayTime.
		PT_EXIT( &pSetupObj->m_pt );
	}
	else if ( PSSMC_NEXTMOVE_SPECIAL_EXIT == pSetupObj-> nextMove ) {
		// Special case exit is design for depend on time delay to get sensor values or other values before exit.
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds

		memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
		// map variable content into selection index.
		pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_STEP_1, pSetupObj, 0);
		// simple menuItem pMsg contains list of pointers to string. Copy the string pointer from PGM to RAM variable pointer pStr which still points at PGM.
		memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
		// show what is in the setting variable on LED display.
		led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds

		memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
		(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_STEP_2, pSetupObj, 0);

		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
		panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display selection error msg.
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// for duration of msgDisplayTime.
		PT_EXIT( &pSetupObj->m_pt );
	}
	// 2011-07-18 -WFC- v
	else if ( PSSMC_NEXTMOVE_DISPLAY_FLOAT_KEY_EXIT == pSetupObj-> nextMove ) {
		// Special case exit is design for depend on time delay to get sensor values or other values and display them before exit.
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_2SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds
		memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
		// this step will read a sensor value and format the display.
		(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_STEP_1, pSetupObj, 0);
		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_10SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_key_get( &key ) );	// wait until press any key
		PT_EXIT( &pSetupObj->m_pt );
	}
	// 2011-07-18 -WFC- removed per Taylor requested to remove time out auot exit.
//	else if ( PSSMC_NEXTMOVE_DISPLAY_FLOAT_EXIT == pSetupObj-> nextMove ) {
//		// Special case exit is design for depend on time delay to get sensor values or other values and display them before exit.
//		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
//		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds
//		memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
//		// this step will read a sensor value and format the display.
//		(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_STEP_1, pSetupObj, 0);
//		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
//		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_10SEC );
//		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) ||
//						panel_key_get( &key ) );	// display the content for 10 seconds or until press any key
//		PT_EXIT( &pSetupObj->m_pt );
//	}
	// 2011-07-18 -WFC- ^
	// end of init thread.
	
	for(;;) {
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_key_get( &key ) );				// wait until get a key.
		if ( PANEL_MENU_SCROLL_KEY == key ) {			// selection key
			gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// Default behaviors is to flash this message. However, the menuItem.pMethod() can override this behaviors. 2011-07-18 -WFC-
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			pSetupObj-> selectIndex++;
			if ( pSetupObj-> selectIndex > ((menuItem.maxNumOfItem & PANEL_TOP_MENU_NUM_ITEM_MASK) - 1) )
				pSetupObj-> selectIndex = 0;
			memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
			// other method may use selected index to perform other task such as change LED intensity,
			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SHOW_VALUE, pSetupObj, 0);
			if ( gbPanelSetupStatus & PANEL_SETUP_STATUS_EXIT_NOW ) {
				break;
			}
			led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
			// 2011-07-18 -WFC- gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
			if (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) ) {
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				break;
			}
		}
		else if ( PANEL_MENU_ENTER_KEY == key ) {		// enter key
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			// map selection index into variable content. Changing variable setting but not save to nonvolatile memory yet.
			// The top menu thread will save configuration settings to the nonvolatile memory.
			pSetupObj-> msgDisplayTime = 0;	// assume no msg to be display.
			pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX, pSetupObj, 0);
			if ( pSetupObj-> msgDisplayTime ) {						// If menuItem.pMethod request a msg to display
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display msg.
				PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for duration of msgDisplayTime.
			}
			
			if ( gbPanelSetupStatus & PANEL_SETUP_STATUS_CONTINUE ) {
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// Default behaviors is to flash this message. However, the menuItem.pMethod() can override this behaviors. 2011-07-18 -WFC-
				memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
				memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
				led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
				// 2011-07-18 -WFC- gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
				timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_500mSEC );
				PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// prevents repeat key
				panel_key_init();	// empty key buffer to prevent repeat key.
			}
			else
				break;
		}
		else if ( PANEL_ZERO_KEY == key || (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) )) {		// quit key
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW, pSetupObj, 0);
			break;
		}
		else if ( PANEL_POWER_KEY == key ) {		// power off key.
			// TODO: power off this device
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW, pSetupObj, 0);
			break;
		}
	} // end for(;;)

  PT_END(&pSetupObj->m_pt);
} // end panel_setup_simple_sub_menu_thread()

/* 2012-09-19 -WFC-
//PT_THREAD( panel_setup_simple_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_setup_simple_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )
{
  BYTE				key;
  BYTE				*pStr;
  PANEL_MENU_ITEM_T	menuItem;

  PT_BEGIN( &pSetupObj->m_pt );
  	// init object variables at the very first time of this thread.
	pSetupObj-> msgID			=
	pSetupObj-> msgDisplayTime	= 0;

	memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
	pSetupObj-> selectIndex = 0;
	// map variable content into selection index.
	pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_GET_CNFG_INDEX, pSetupObj, 0);
	// simple menuItem pMsg contains list of pointers to string. Copy the string pointer from PGM to RAM variable pointer pStr which still points at PGM.
	memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
	// show what is in the setting variable on LED display.
	led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.

	if ( 0 == pSetupObj-> nextMove ) {									// If menu selection has an error OR the method wants to exit.
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds
		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
		panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display selection error msg.
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// for duration of msgDisplayTime.
		PT_EXIT( &pSetupObj->m_pt );
	}
	else if ( PSSMC_NEXTMOVE_SPECIAL_EXIT == pSetupObj-> nextMove ) {
		// Special case exit is design for depend on time delay to get sensor values or other values before exit.
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds

		memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
		// map variable content into selection index.
		pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_STEP_1, pSetupObj, 0);
		// simple menuItem pMsg contains list of pointers to string. Copy the string pointer from PGM to RAM variable pointer pStr which still points at PGM.
		memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
		// show what is in the setting variable on LED display.
		led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds
		
		memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
		(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_STEP_2, pSetupObj, 0);
		
		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
		panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display selection error msg.
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// for duration of msgDisplayTime.
		PT_EXIT( &pSetupObj->m_pt );
	}
	else if ( PSSMC_NEXTMOVE_DISPLAY_FLOAT_EXIT == pSetupObj-> nextMove ) {
		// Special case exit is design for depend on time delay to get sensor values or other values and display them before exit.
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds
		memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
		// this step will read a sensor value and format the display.
		(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_STEP_1, pSetupObj, 0);
		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_10SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) || 
						panel_key_get( &key ) );	// display the content for 10 seconds or until press any key
		PT_EXIT( &pSetupObj->m_pt );
	}

	// end of init thread.
	
	for(;;) {
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_key_get( &key ) );				// wait until get a key.
		if ( PANEL_MENU_SCROLL_KEY == key ) {			// selection key
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			pSetupObj-> selectIndex++;
			if ( pSetupObj-> selectIndex > ((menuItem.maxNumOfItem & PANEL_TOP_MENU_NUM_ITEM_MASK) - 1) )
				pSetupObj-> selectIndex = 0;
			memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
			// other method may use selected index to perform other task such as change LED intensity,
			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SHOW_VALUE, pSetupObj, 0);
			if ( gbPanelSetupStatus & PANEL_SETUP_STATUS_EXIT_NOW ) {
				break;
			}
			led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
			gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
			if (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) ) {
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				break;
			}
		}
		else if ( PANEL_MENU_ENTER_KEY == key ) {		// enter key
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			// map selection index into variable content. Changing variable setting but not save to nonvolatile memory yet.
			// The top menu thread will save configuration settings to the nonvolatile memory.
			pSetupObj-> msgDisplayTime = 0;	// assume no msg to be display.
			pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX, pSetupObj, 0);
			if ( pSetupObj-> msgDisplayTime ) {						// If menuItem.pMethod request a msg to display
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display msg.
				PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for duration of msgDisplayTime.
			}
			
			if ( gbPanelSetupStatus & PANEL_SETUP_STATUS_CONTINUE ) {
				memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
				memcpy_P( &pStr,  &(menuItem.pMsg[(pSetupObj-> selectIndex << 1)]),  sizeof(BYTE *));
				led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
				timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_500mSEC );
				PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// prevents repeat key
				panel_key_init();	// empty key buffer to prevent repeat key.
			}
			else
				break;
		}
		else if ( PANEL_ZERO_KEY == key || (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) )) {		// quit key
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW, pSetupObj, 0);
			break;
		}
		else if ( PANEL_POWER_KEY == key ) {		// power off key.
			// TODO: power off this device
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW, pSetupObj, 0);
			break;
		}
	} // end for(;;)

  PT_END(&pSetupObj->m_pt);
} // end panel_setup_simple_sub_menu_thread()
*/

/**
 * Panel setup of complex sub menu thread. This thread is spawned off by panel_setup_top_menu_thread().
 *
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 *
 * History:  Created on 2009/07/13 by Wai Fai Chin
 * 2011-07-19 -WFC- Added nextMove feature to enter next item without user pressed enter key.
 */

//PT_THREAD( panel_setup_complex_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_setup_complex_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )
{
  BYTE			key;
  PANEL_MENU_ITEM_T	menuItem;

  PT_BEGIN( &pSetupObj->m_pt );
	// init object variables at the very first time of this thread.
	pSetupObj-> selectIndex = 0;

	memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
	panel_setup_display_complex_menu_item( &menuItem, pSetupObj );
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
	if ( 0 == pSetupObj-> nextMove ) {									// If menu selection has an error or the method wants to exit
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds
		panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display selection error msg.
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// for duration of msgDisplayTime.
		PT_EXIT( &pSetupObj->m_pt );
	}
	
	memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
	// The next statement makes user use one enter key press instead of double pressed of enter key.
	if ( PANEL_MENU_ITEM_TYPE_SIMPLE == (menuItem.maxNumOfItem & PANEL_MENU_ITEM_TYPE_MASK) ) { // if it is a simple type
		gPanelSetupSubMenuObj2.nextMove = 1;			// default next move is one step.
		gPanelSetupSubMenuObj2.msgDisplayTime = 0;		// default no msg to be display
		gPanelSetupSubMenuObj2.pRootItem = pSetupObj-> pRootItem;	// It is legal to access the first level of program code space.
		PT_SPAWN( &pSetupObj->m_pt, &gPanelSetupSubMenuObj2.m_pt, panel_setup_simple_sub_menu_thread( &gPanelSetupSubMenuObj2));
		pSetupObj-> nextMove = gPanelSetupSubMenuObj2.nextMove;		// 2011-07-19 -WFC- nextMove might carried special instruction from specific item method() executed in panel_setup_simple_sub_menu_thread().
		pSetupObj-> selectIndex++;
		if ( pSetupObj-> selectIndex > ((pSetupObj-> maxNumOfItem & PANEL_TOP_MENU_NUM_ITEM_MASK) - 1) ) {
			pSetupObj-> selectIndex = 0;
			PT_EXIT( &pSetupObj->m_pt );	// completed visited all menu items, end this thread.
		}
	}
	// end of init thread.
	
	for(;;) {
		// the following for display loadcell weight etc...
		memcpy_P( &menuItem,  &pSetupObj-> pRootItem[pSetupObj-> selectIndex], sizeof(PANEL_MENU_ITEM_T));
		if ( PANEL_MENU_ITEM_TYPE_FLOAT == (menuItem.maxNumOfItem &  PANEL_MENU_ITEM_TYPE_MASK)) {
			panel_setup_display_default_float_value( pSetupObj );
		}
		
		// 2011-07-19 -WFC- PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_setup_display_default_float_value( pSetupObj ) || panel_key_get( &key ));		// wait until get a key.
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_setup_display_default_float_value( pSetupObj ) ||
						panel_key_get( &key ) || ( PSSMC_NEXTMOVE_ENTER_INTO_NEXT_PM_ITEM == pSetupObj-> nextMove ));		// wait until get a key OR next move is enter into next panel menu item. 2011-07-19 -WFC-
		if ( PANEL_MENU_SCROLL_KEY == key ) {			// selection key
			memcpy_P( &menuItem,  &pSetupObj-> pRootItem[pSetupObj-> selectIndex],  sizeof(PANEL_MENU_ITEM_T));
			if ( PANEL_MENU_ITEM_TYPE_FLOAT == (menuItem.maxNumOfItem &  PANEL_MENU_ITEM_TYPE_MASK)) {
				PT_SPAWN( &pSetupObj->m_pt, &gPanelNumEntryObj.m_pt, panel_setup_numeric_entry_thread( &gPanelNumEntryObj.m_pt));
				// NOTE: menuItem is in stack memory and was destroyed by PT_SPAWN(), it must recopy from location pointed by pSetupObj.
				memcpy_P( &menuItem,  &pSetupObj-> pRootItem[pSetupObj-> selectIndex],  sizeof(PANEL_MENU_ITEM_T));
				if ( PANEL_NUMERIC_ENTERED == gPanelNumEntryObj.status ) {		// user entered a numeric number.
					pSetupObj-> msgDisplayTime = 0;							// default no msg to be display
					(*(menuItem.pMethod))( PANEL_MENU_ACTION_MODE_SET_VALUE, pSetupObj, &gPanelNumEntryObj.fV);	// set the user input value.
					if ( pSetupObj-> msgDisplayTime ) {						// If menuItem.pMethod request a msg to display
						gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
						panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display msg.
						PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for duration of msgDisplayTime.
					}
				}
			} 
			if (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) ) {
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				break;
			}
			//pSetupObj-> selectIndex++;
			//if ( pSetupObj-> selectIndex > ((pSetupObj-> maxNumOfItem & PANEL_TOP_MENU_NUM_ITEM_MASK) - 1) )
			//	pSetupObj-> selectIndex = 0;
			pSetupObj-> selectIndex++;
			if ( pSetupObj-> selectIndex > ((pSetupObj-> maxNumOfItem & PANEL_TOP_MENU_NUM_ITEM_MASK) - 1) ) {
				pSetupObj-> selectIndex = 0;
				break;	// completed visited all menu items, end this thread.
			}
			memcpy_P( &menuItem,  &pSetupObj-> pRootItem[pSetupObj-> selectIndex],  sizeof(PANEL_MENU_ITEM_T));
			panel_setup_display_complex_menu_item( &menuItem, pSetupObj );
			gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
		}
		// 2011-07-19 -WFC- else if ( PANEL_MENU_ENTER_KEY == key ) {		// enter key
		else if ( PANEL_MENU_ENTER_KEY == key || ( PSSMC_NEXTMOVE_ENTER_INTO_NEXT_PM_ITEM == pSetupObj-> nextMove )) {		// enter key OR nexMove is to enter into next panel menu item.
			memcpy_P( &menuItem,  &pSetupObj-> pRootItem[pSetupObj-> selectIndex],  sizeof(PANEL_MENU_ITEM_T));
			gPanelSetupSubMenuObj2.nextMove = 1;			// default next move is one step.
			gPanelSetupSubMenuObj2.msgDisplayTime = 0;		// default no msg to be display
			gPanelSetupSubMenuObj2.pRootItem = &pSetupObj-> pRootItem[ pSetupObj-> selectIndex ];	// It is legal to access the first level of program code space.

			if ( PANEL_MENU_ITEM_TYPE_SIMPLE == (menuItem.maxNumOfItem & PANEL_MENU_ITEM_TYPE_MASK) ) { // if it is a simple type
				PT_SPAWN( &pSetupObj->m_pt, &gPanelSetupSubMenuObj2.m_pt, panel_setup_simple_sub_menu_thread( &gPanelSetupSubMenuObj2));
				pSetupObj-> nextMove = gPanelSetupSubMenuObj2.nextMove;
			}
			else if ( PANEL_MENU_ITEM_TYPE_FLOAT == (menuItem.maxNumOfItem &  PANEL_MENU_ITEM_TYPE_MASK)) {
				(*(menuItem.pMethod))( PANEL_MENU_ACTION_MODE_SET_DEFAULT_VALUE, pSetupObj, &gPanelNumEntryObj.fV);
				if ( pSetupObj-> msgDisplayTime ) {						// If menuItem.pMethod request a msg to display
					gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
					panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display msg.
					PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for duration of msgDisplayTime.
				}
			}
			else if ( PANEL_MENU_ITEM_TYPE_DYNAMIC_CHOICE == (menuItem.maxNumOfItem & PANEL_MENU_ITEM_TYPE_MASK) ) {		// It is a dyanamic choice menu type.{
				PT_SPAWN( &pSetupObj->m_pt, &gPanelSetupSubMenuObj2.m_pt, panel_setup_dynamic_choice_sub_menu_thread( &gPanelSetupSubMenuObj2));
				pSetupObj-> nextMove = gPanelSetupSubMenuObj2.nextMove;
			}

			if (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) ) {
				//mapper method here:
				break;
			}
			// else if ( PANEL_MENU_ITEM_TYPE_STRING == (menuItem.maxNumOfItem &  PANEL_MENU_ITEM_TYPE_MASK)) { }; We don't need it now, and may not need it at all.
			// here, the thread completed sub menu entry, advanced to the next top menu entry.
			//pSetupObj-> selectIndex += pSetupObj-> nextMove;
			pSetupObj-> selectIndex++;
			if ( pSetupObj-> selectIndex > ((pSetupObj-> maxNumOfItem & PANEL_TOP_MENU_NUM_ITEM_MASK) - 1) ) {
				pSetupObj-> selectIndex = 0;
				//mapper method here:
				break;	// completed visited all menu items, end this thread.
			}
		}
		else if ( PANEL_ZERO_KEY == key || (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) )) {		// quit key
			break;
			//mapper method here:
		}
		else if ( PANEL_POWER_KEY == key ) {		// power off key.
			// TODO: power off this device
			//mapper method here:
			break;
		}
	} // end for(;;)

  PT_END(&pSetupObj->m_pt);
} // end panel_setup_complex_sub_menu_thread()


/**
 * Panel setup of dynamic choice sub menu thread. This thread is spawned off by either
 *  panel_setup_top_menu_thread() or panel_setup_complex_sub_menu_thread().
 *
 * @param  pSetupObj	-- pointer to panel setup sub simple menu object
 * @note Dynamic choice sub menu type only has ONE menuItem a compiled time.
 *       It dynamically generated maxNumOfItem by the menuItem.pMethod.
 *       It also dynamically generated the content of the choice item string.
 *
 * History:  Created on 2009/07/28 by Wai Fai Chin
 */


//PT_THREAD( panel_setup_dynamic_choice_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )) // Doxygen cannot handle this macro
char panel_setup_dynamic_choice_sub_menu_thread( PANEL_SETUP_SUB_MENU_CLASS *pSetupObj )
{
  BYTE			key;
  BYTE			strBuf[ PANEL_SETUP_LOCAL_MAX_STRING_SIZE ];
  PANEL_MENU_ITEM_T	menuItem;

  PT_BEGIN( &pSetupObj->m_pt );
  	// init object variables at the very first time of this thread.
	pSetupObj-> msgID			=
	pSetupObj-> msgDisplayTime	= 0;

	memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
	pSetupObj-> selectIndex = 0;
	// get default choice index.
	pSetupObj-> selectIndex = (*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_GET_DEFAULT_CHOICE, pSetupObj, strBuf);
	// show what is in the setting variable on LED display.
	led_display_format_output( strBuf );
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.

	if ( 0 == pSetupObj-> nextMove ) {									// If menu selection has an error
		timer_mSec_set( &gPanelMainRunModeThreadTimer, TT_3SEC );
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// display the content for 3 seconds
		gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
		panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display selection error msg.
		PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// for duration of msgDisplayTime.
		PT_EXIT( &pSetupObj->m_pt );
	}

	// end of init thread.
	
	for(;;) {
		PT_WAIT_UNTIL( &pSetupObj->m_pt, panel_key_get( &key ) );				// wait until get a key.
		if ( PANEL_MENU_SCROLL_KEY == key ) {			// selection key
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			pSetupObj-> selectIndex++;
			if ( pSetupObj-> selectIndex > pSetupObj-> maxNumOfItem )
				pSetupObj-> selectIndex = 0;
			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SHOW_CHOICE, pSetupObj, strBuf);
			if (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) ) {
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				break;
			}
			led_display_format_output( strBuf );
			gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_BLINK_STRING;		// flash this message.
		}
		else if ( PANEL_MENU_ENTER_KEY == key ) {		// enter key
			memcpy_P( &menuItem,  pSetupObj-> pRootItem,  sizeof(PANEL_MENU_ITEM_T));
			// map selection index into variable content. Changing variable setting but not save to nonvolatile memory yet.
			// The top menu thread will save configuration settings to the nonvolatile memory.
			pSetupObj-> msgDisplayTime = 0;	// assume no msg to be display.
			(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_SET_CHOICE, pSetupObj, strBuf);
			if ( pSetupObj-> msgDisplayTime ) {						// If menuItem.pMethod request a msg to be display
				gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
				panel_setup_display_msg_duration_P(pSetupObj-> msgID, pSetupObj-> msgDisplayTime );		// display msg.
				PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );	// wait for duration of msgDisplayTime.
			}
			break;
		}
		else if ( PANEL_ZERO_KEY == key || (gbPanelSetupStatus & (PANEL_SETUP_STATUS_EXIT_SAVE | PANEL_SETUP_STATUS_EXIT_NO_SAVE) )) {		// quit key
			break;
		}
		else if ( PANEL_POWER_KEY == key ) {		// power off key.
			// TODO: power off this device
			break;
		}
	} // end for(;;)

  PT_END(&pSetupObj->m_pt);
} // end panel_setup_dynamic_choice_sub_menu_thread()



/**
 * Show contents of a complext menu item. It could be a string, integer or floating point value.
 *
 * @param  pMenuItem	-- pointer to a menu item.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 *
 * @post	format a value into a string in LED string buffer.
 *
 * History:  Created on 2009-07-14 by Wai Fai Chin
 * 2011-05-02 -WFC- Use higher resolution x1000 function to fit all digit in max LED digit length.
 */

void panel_setup_display_complex_menu_item( PANEL_MENU_ITEM_T	*pMenuItem, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj)
{
  BYTE 		index;
  BYTE		bStrBuf[ PANEL_SETUP_LOCAL_MAX_STRING_SIZE ];
  MSI_CB	countby;
  BYTE		*pStr;
  float		fV;
  SENSOR_CAL_T	*pSensorCal;

	index = pMenuItem-> maxNumOfItem & PANEL_MENU_ITEM_TYPE_MASK;
	if ( PANEL_MENU_ITEM_TYPE_SIMPLE == index ) { // if it is simple type
		index = (*(pMenuItem-> pMethod))( PANEL_MENU_ACTION_MODE_GET_CNFG_INDEX, pSetupObj, 0);
		memcpy_P( &pStr,  &(pMenuItem-> pMsg[index << 1 ]),  sizeof(BYTE *));
		// show what is in the setting variable on LED display.
		led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
	}
	else if ( PANEL_MENU_ITEM_TYPE_FLOAT== index ) {
		countby.fValue = 1; // default decimal point.
		countby.decPt = 0;
		index = (*(pMenuItem-> pMethod))( PANEL_MENU_ACTION_MODE_GET_VALUE, pSetupObj, &fV);
		
		if ( sensor_get_cal_base( PANEL_SETUP_LOADCELL_NUM, &pSensorCal ) ) {		// get sensor 0 calibration pointer.
			if ( PANEL_MENU_ITEM_DECPT_USE_CB == pMenuItem-> decPt ) {
				countby = pSensorCal-> countby;
			}
			else if ( PANEL_MENU_ITEM_DECPT_USE_NEXT_LOWER_CB == pMenuItem-> decPt ) {
				countby = pSensorCal-> countby;
				cal_next_lower_countby( &countby );
			}
		}
	}
		
	// 2011-05-02 -WFC- led_display_format_float_string( fV, &countby, FALSE, bStrBuf );
	led_display_format_high_resolution_float_string( fV, &countby, TRUE, bStrBuf );			// 2011-05-02 -WFC-
	led_display_format_output( bStrBuf );
} // end panel_setup_display_complex_menu_item();


/**
 * Show contents of a default float menu item.
 *
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 *
 * @return always 0 because it use with PT_WAIT_UNTIL( , condition)
 *
 * @post	format a value into a string in LED string buffer.
 *
 * History:  Created on 2009-07-31 by Wai Fai Chin
 * 2011-05-02 -WFC- Use higher resolution x1000 function to fit all digit in max LED digit length.
 */

BYTE panel_setup_display_default_float_value(  PANEL_SETUP_SUB_MENU_CLASS *pSetupObj)
{
  BYTE		bStrBuf[ PANEL_SETUP_LOCAL_MAX_STRING_SIZE ];
  MSI_CB	countby;
  float		fV;
  PANEL_MENU_ITEM_T	menuItem;
  SENSOR_CAL_T	*pSensorCal;

	countby.fValue = 1; // default decimal point.
	countby.decPt = 0;
	memcpy_P( &menuItem,  &pSetupObj-> pRootItem[pSetupObj-> selectIndex], sizeof(PANEL_MENU_ITEM_T));
	if ( PANEL_MENU_ITEM_TYPE_FLOAT == (menuItem.maxNumOfItem &  PANEL_MENU_ITEM_TYPE_MASK)) {
		(*menuItem.pMethod)( PANEL_MENU_ACTION_MODE_GET_DEFAULT_VALUE, pSetupObj, &fV);
		if ( PANEL_MENU_ITEM_DECPT_USE_CB == menuItem.decPt ) {
			if ( sensor_get_cal_base( PANEL_SETUP_LOADCELL_NUM, &pSensorCal ) ) {		// get sensor 0 calibration pointer.
				countby = pSensorCal-> countby; 
			}
		}
//		led_display_format_float_string( fV, &countby, TRUE, bStrBuf );
		// 2011-05-02 -WFC- 		led_display_format_float_string( fV, &countby, FALSE, bStrBuf );	//	PHJ
		led_display_format_high_resolution_float_string( fV, &countby, TRUE, bStrBuf );			// 2011-05-02 -WFC-

		led_display_format_output( bStrBuf );
	}
	
	return 0;
} // end panel_setup_display_default_float_value();


/**
 * copy a specified msg to Led string buffer of gLedDisplayManager
 * and set the duration time to gPanelMainRunModeThreadTimer.
 * caller need to PT_WAIT_UNTIL( &pSetupObj->m_pt, timer_mSec_expired( &gPanelMainRunModeThreadTimer ) );
 *
 * @param  msgID	-- index to list of msg.
 * @param  duration	-- 
 *
 * History:  Created on 2009/07/24 by Wai Fai Chin
 */

void panel_setup_display_msg_duration_P( BYTE msgID, BYTE duration )
{
	BYTE	*pStr;

	memcpy_P( &pStr, &gcMsg_List_Msg[msgID], sizeof(BYTE *));
	led_display_copy_string_and_pad_P( gLedDisplayManager.descr.str1, pStr);
	timer_mSec_set( &gPanelMainRunModeThreadTimer, duration );
} // end panel_setup_display_msg_by_id(,);


/**
 * Panel numeric entry thread.
 *
 * @param  pt	-- pointer to Proto thread structure.
 *
 * @post  PANEL_NUMERIC_ENTERED, PANEL_NUMERIC_INVALID or PANEL_NUMERIC_CANCELLED depends on the user input.
 *
 * History:  Created on 2009/06/10 by Wai Fai Chin
 * 2011-05-02 -WFC-  Once all LED digits are used without decimal point, the power key treat as user want a x1000.
 */

//PT_THREAD( panel_setup_numeric_entry_thread( struct pt *pt )) // Doxygen cannot handle this macro
char panel_setup_numeric_entry_thread( struct pt *pt )
{
  BYTE 		*pCh;
  BYTE		digitLenLimit;
  BYTE		key;
  
  PT_BEGIN( pt );
	// init object variables at the very first time of this thread.
	gPanelNumEntryObj.status = PANEL_NUMERIC_RUNNING;
	// 2011-05-02 -WFC- gPanelNumEntryObj.decPtLoc = 	gPanelNumEntryObj.strIndex = 0;
	gPanelNumEntryObj.decPtLoc = gPanelNumEntryObj.strIndex = gPanelNumEntryObj.x1000 = 0;		// 2011-05-02 -WFC-
	pCh = gPanelNumEntryObj.str;
	*pCh++ = '0';
	for(digitLenLimit = 1; digitLenLimit < LED_DISPLAY_MAX_DIGIT_LENGTH ; digitLenLimit++) // 2012-07-16 -DLM-
		*pCh++ = ' ';
	*pCh++ = 0;	// null for none decimal point. Pad space for decimal point number is required in order correctly display, because the decimal point share the same digit. .
	*pCh++ = 0;
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
	gLedDisplayManager.descr.digitBlinkPos = 1;					// blink the left most digit, str[0].
	copy_until_match_char_xnull( gLedDisplayManager.descr.str1, gPanelNumEntryObj.str, 0, LED_MAX_STRING_SIZE );
	for(;;) {
		PT_WAIT_UNTIL( pt, panel_key_get( &key ) );				// wait until get a key.
		pCh = &gPanelNumEntryObj.str[ gPanelNumEntryObj.strIndex ];
		if ( PANEL_MENU_SCROLL_KEY == key ) {		// select a single digit number
			if ( gLedDisplayManager.descr.digitBlinkPos ) {	// if current digit is in change mode
				if ( '.' == *pCh ) {
					gPanelNumEntryObj.strIndex++;
					pCh++;
					*pCh = '0'-1;
				}
				(*pCh)++;
				
				if ( *pCh > '9' ) 
					*pCh = '0';
				gLedDisplayManager.descr.digitBlinkPos = gPanelNumEntryObj.strIndex + 1;	// NOTE digitBlinkPos 0==no blink. 1== position 0 of a string or right most digit.
			}
			else {													// all digits had been selected.
				digitLenLimit = LED_DISPLAY_MAX_DIGIT_LENGTH;
				if ( gPanelNumEntryObj.decPtLoc )
					digitLenLimit++;									// increment the length limit to include a decimal point.
				if ( gPanelNumEntryObj.strIndex < digitLenLimit) {
					*pCh = '0';																// new digit start with a '0'.
					gLedDisplayManager.descr.digitBlinkPos = gPanelNumEntryObj.strIndex + 1;		// blink this new digit
				}
			}
		}
		else if ( PANEL_MENU_ENTER_KEY == key ) {
			if ( gLedDisplayManager.descr.digitBlinkPos ) {			// if a digit is blinking, a TARE key pressed will selected this digit
				gLedDisplayManager.descr.digitBlinkPos = 0;			// user selected this digit.
				digitLenLimit = LED_DISPLAY_MAX_DIGIT_LENGTH;
				if ( gPanelNumEntryObj.decPtLoc )
					digitLenLimit++;									// increment the length limit to include a decimal point.
				if ( gPanelNumEntryObj.strIndex < digitLenLimit ) {
					gPanelNumEntryObj.strIndex++;
				}
			}
			else {													// all digits had been select, a TARE key pressed will accepted this entry number.
				if ( sscanf_P( gPanelNumEntryObj.str, gcStrFmt_pct_f, &gPanelNumEntryObj.fV )!= EOF ) {			// if it has a valid value.
					if ( gPanelNumEntryObj.x1000 )
						gPanelNumEntryObj.fV *= 1000.0;
					gPanelNumEntryObj.u32V = (UINT32) gPanelNumEntryObj.fV;
					gPanelNumEntryObj.status = PANEL_NUMERIC_ENTERED;			// user enter a numeric number.
				}
				else
					gPanelNumEntryObj.status = PANEL_NUMERIC_INVALID;
			}
		}
		else if ( PANEL_POWER_KEY == key ) {
			// select decimal point and x1000
			if ( gPanelNumEntryObj.decPtLoc ) {			// if a decimal point had been selected, just toggle x1000 flag.
				if ( gPanelNumEntryObj.x1000 )
					gPanelNumEntryObj.x1000 = FALSE;
				else
					gPanelNumEntryObj.x1000 = TRUE;
			}
			else {
				if ( gPanelNumEntryObj.strIndex < (LED_DISPLAY_MAX_DIGIT_LENGTH - 1) ) {
					if ( gLedDisplayManager.descr.digitBlinkPos ) {	// 2009/07/17 -WFC- if a digit is blinking, a TARE key pressed will selected this digit	
						gPanelNumEntryObj.decPtLoc = ++gPanelNumEntryObj.strIndex;
						pCh++;
					}
					else {
						gPanelNumEntryObj.decPtLoc = gPanelNumEntryObj.strIndex;
					}
					*pCh = '.';
					gLedDisplayManager.descr.digitBlinkPos = gPanelNumEntryObj.strIndex + 1;		// blink this decimal point
					gPanelNumEntryObj.str[LED_DISPLAY_MAX_DIGIT_LENGTH] = ' ';		//need a pad space for a decimal point number because the decimal point share the same digit.
				}
				// 2011-05-02 -WFC- v
				else {	// Once all LED digits are used without decimal point, the power key treat as user want a x1000.
					if ( gPanelNumEntryObj.x1000 )
						gPanelNumEntryObj.x1000 = FALSE;
					else
						gPanelNumEntryObj.x1000 = TRUE;
				}
				// 2011-05-02 -WFC- ^
			}
		}
		else if ( PANEL_ZERO_KEY == key ) {
			if (gPanelNumEntryObj.strIndex ) {
				if ( gLedDisplayManager.descr.digitBlinkPos ) {
					if ( '.' == *pCh ) {
						gPanelNumEntryObj.decPtLoc = 0;
						gPanelNumEntryObj.str[LED_DISPLAY_MAX_DIGIT_LENGTH] = 0;		//null char replaced a pad space char for a none decimal point number because the decimal point share the same digit..
					}
					*pCh = ' ';
					gLedDisplayManager.descr.digitBlinkPos = gPanelNumEntryObj.strIndex;	// NOTE digitBlinkPos 0==no blink. 1== position 0 of a string or right most digit.
					gPanelNumEntryObj.strIndex--;
				}
				else { // digit is selected and not blinking, the strIndex points at the next new location.
					gLedDisplayManager.descr.digitBlinkPos = gPanelNumEntryObj.strIndex;
					gPanelNumEntryObj.strIndex--;
				}
			}
			else {
				gPanelNumEntryObj.status = PANEL_NUMERIC_CANCELLED;	// user cancelled enter a numeric number.
			}
		}
	
		if ( gPanelNumEntryObj.x1000 )
			digitLenLimit = LED_SEG_STATE_STEADY;
		else 
			digitLenLimit = LED_SEG_STATE_OFF;

		/* -WFC- 2011-03-09 v commented out Pete's codes.
		#if ( LED_ANC_LOC_X1000 )
			gLedDisplayManager.descr.baAnc[LED_ANC_LOC_X1000 - LOCOFF].state[LED_ANC_X1000] = digitLenLimit;
		#endif
		   -WFC- 2011-03-09 ^ */
		// -WFC- 2011-03-09 v
		#ifdef  LED_DISPLAY_ANC_X1000	// -WFC- 2011-03-09
			led_display_set_annunciator( LED_DISPLAY_ANC_X1000, digitLenLimit);
		#endif
		// -WFC- 2011-03-09 ^
		copy_until_match_char_xnull( gLedDisplayManager.descr.str1, gPanelNumEntryObj.str, 0, LED_MAX_STRING_SIZE );
		if ( gPanelNumEntryObj.status < PANEL_NUMERIC_RUNNING ){
			#ifdef  LED_DISPLAY_ANC_X1000	// 2012-07-16 -DLM-
				led_display_set_annunciator( LED_DISPLAY_ANC_X1000, LED_SEG_STATE_OFF);
			#endif
			break;
		}
	} // end for(;;)
  
  PT_END(pt);
} // end panel_setup_numeric_entry_thread()


/**
 * Method to map between user key function variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  selectIndex	-- selection index, mainly for setting.
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009/07/15 by Wai Fai Chin
 */


BYTE	panel_setup_user_key_func_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{

	BYTE functionKeyNum;
	BYTE functType;

	functionKeyNum = gPanelSetupTopMenuObj.selectIndex;

	functType = pSetupObj-> selectIndex;
	if ( functType > FUNC_KEY_UNIT)
		functType = FUNC_KEY_PRINT;
	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode )
		gtSystemFeatureFNV.userKeyFunc[ functionKeyNum ] = functType;

	functType = gtSystemFeatureFNV.userKeyFunc[ functionKeyNum ];
	if ( functType  > FUNC_KEY_UNIT )			// TARE msg is in the place of LEARN.
		functType =  FUNC_KEY_UNIT + 1;
	return functType;
} // end 


//BYTE	panel_setup_user_key_func_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
//{
//
//	BYTE functionNum;
//
//	functionNum = gPanelSetupTopMenuObj.selectIndex;
//	// convert top menu selection index into setpoint number.
//	if ( functionNum >= PANEL_TOP_MENU_FUNCTION_BASE_INDEX ) {
//		functionNum -= PANEL_TOP_MENU_FUNCTION_BASE_INDEX;
//	}
//	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode )
//		gtSystemFeatureFNV.userKeyFunc[ functionNum ] = pSetupObj-> selectIndex;
//
//	return gtSystemFeatureFNV.userKeyFunc[ functionNum ];
//} // end


/**
 * Method to map between auto power off setting variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009/07/15 by Wai Fai Chin
 */


BYTE	panel_setup_auto_power_off_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		gtSystemFeatureFNV.autoOffMode = pSetupObj-> selectIndex;
		index = pSetupObj-> selectIndex;
	}
	else {
		index = gtSystemFeatureFNV.autoOffMode;
		if ( index > (MENU_A_OFF_MAX_ITEMS - 1) )
			index = MENU_A_OFF_MAX_ITEMS - 1;
	}

	return index;
} // end panel_setup_auto_power_off_mapper(,,)


/**
 * Method to map between sensor filter level variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 *  SENSOR_CNFG_FILTER_LEVEL_MASK	0X03
 *  SENSOR_CNFG_FILTER_DISABLED		0
 *  SENSOR_CNFG_FILTER_LEVEL_LOW	1
 *  SENSOR_CNFG_FILTER_LEVEL_MEDIUM	2
 *  SENSOR_CNFG_FILTER_LEVEL_HIGH	3
 *
 * Sensor featurs, Bit7 enabled sensor; Bit6 AC excitation, Bit 5 temperature compensation, Bit4-3 voltage reference. Bit2 reserved, Bit1-0 filter level, 0==disalbed.
 *
 * @note Challenger3 and other stand alone scale only handle 1 loadcell and it is loadcell 0.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 * 2011-04-20 -WFC- If software filter is off, then set the ADC to a slowest sample rate, 6.875Hz.
 *  Any filter setting set ADC sample at 27.5Hz.
 * 2011-05-04 -WFC- If software filter is off, then set the ADC to a slowest sample rate, 6.875Hz.
 *  Any filter setting set ADC sample at 13.75Hz.
 * 2011-05-06 -WFC- Software filter off, sample speed is 6.875Hz, LO and Hi sample at 27.75Hz, Hi-2 sample at 55 Hz.
 */


BYTE	panel_setup_sensor_filter_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		gabSensorFeaturesFNV[ PANEL_SETUP_LOADCELL_NUM ] &= ~SENSOR_CNFG_FILTER_LEVEL_MASK;	// clear filter bits
		gabSensorFeaturesFNV[ PANEL_SETUP_LOADCELL_NUM ] |= pSetupObj-> selectIndex;			// set new filter setting
//		if ( 0 == pSetupObj-> selectIndex )		// if filter disabled, then set the slowest ADC sample rate. 2011-04-20 -WFC-
//			gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 15;		// Linear Tech ADC, 6.875 HZ sample rate
//		else
//			// 2011-05-04 -WFC- too noisy gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 8;		// Linear Tech ADC, 27.5 HZ	sample rate.
//			gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 10;		// Linear Tech ADC, 13.75 HZ	sample rate. // 2011-05-04 -WFC-

		if ( SENSOR_CNFG_FILTER_DISABLED == pSetupObj-> selectIndex )		// if filter disabled, then set the slowest ADC sample rate. 2011-04-20 -WFC-
			gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 15;		// Linear Tech ADC, 6.875 HZ sample rate
		else if ( SENSOR_CNFG_FILTER_LEVEL_LOW == pSetupObj-> selectIndex )
			// gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 10;		// Linear Tech ADC, 13.75 HZ	sample rate. // 2011-05-04 -WFC-
			#if ( CONFIG_SUBPRODUCT_AS == CONFIG_AS_PORTAWEIGH2_PC )
				gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 7;		// Linear Tech ADC, 27.75 HZ	sample rate. // 2011-05-06 -WFC-
			#else
				gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 8;		// Linear Tech ADC, 27.75 HZ	sample rate. // 2011-05-06 -WFC-
			#endif

		else if ( SENSOR_CNFG_FILTER_LEVEL_MEDIUM == pSetupObj-> selectIndex )
			gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 8;		// Linear Tech ADC, 27.75 HZ	sample rate. // 2011-05-04 -WFC-
		else if ( SENSOR_CNFG_FILTER_LEVEL_HIGH == pSetupObj-> selectIndex )
			gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 7;		// Linear Tech ADC, 55 HZ	sample rate. // 2011-05-04 -WFC-

		index = pSetupObj-> selectIndex;
	}
	else { // map configuration content to menu selection index.
		index =  gabSensorFeaturesFNV[ PANEL_SETUP_LOADCELL_NUM ];
		index &= SENSOR_CNFG_FILTER_LEVEL_MASK;							// get filter settings.
	}

	return index;
} // end panel_setup_sensor_filter_mapper(,,)


/**
 * Method to map between LED intensity setting variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 *  2011-05-12 -WFC- replaced led_display_send_hw_cmd() with led_display_set_intensity().
 */


BYTE	panel_setup_led_intensity_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV)
{
	BYTE index;

	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;

	if ( pSetupObj-> selectIndex )		// note that 0 is auto intensity based light sensor.
		// 2011-05-12 -WFC- led_display_send_hw_cmd( LED_HW_INTENSITY_CMD, (1<< pSetupObj-> selectIndex ) - 2);
		// led_display_set_intensity((1<< pSetupObj-> selectIndex ) - 2); // 2011-05-12 -WFC-
		led_display_set_intensity((1<< pSetupObj-> selectIndex ) - 1); // 2012-05-24 -DLM-

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		gtSystemFeatureFNV.ledIntensity = pSetupObj-> selectIndex;
		index = pSetupObj-> selectIndex;
	}
	else {
		index = gtSystemFeatureFNV.ledIntensity;
		if ( index > (MENU_LED_INTENSITY_ITEMS - 1) )
			index = MENU_LED_INTENSITY_ITEMS - 1;
	}

	return index;
} // end panel_setup_led_intensity_mapper(,,)


/**
 * Method to map between LED sleep setting variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2010/09/14 by Pete Jensen
 */


BYTE	panel_setup_led_sleep_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV)
{
	BYTE index;

	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		gtSystemFeatureFNV.ledSleep = pSetupObj-> selectIndex;
		index = pSetupObj-> selectIndex;
	}
	else {
		index = gtSystemFeatureFNV.ledSleep;
		if ( index > (MENU_LED_SLEEP_ITEMS - 1) )
			index = MENU_LED_SLEEP_ITEMS - 1;
	}

	return index;
} // end panel_setup_led_sleep_mapper(,,)


/**
 * Method to map between loadcell unit setting variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 * 2011-04-29 -WFC- removed gbLbFlag because of duplication and caused a bug that set LB annunciator on when switch to Kg.
 */


BYTE	panel_setup_unit_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;
	BYTE unit;
    BYTE anncState;

    anncState = LED_SEG_STATE_KEEP_BLINKING;
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;		// Default behaviors was set to flash by the panel_setup_simple_sub_menu_thread(), this method override it to normal display.
	index = pSetupObj-> selectIndex;
	unit = index;
	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		gabSensorShowCBunitsFNV[ 0 ] = pSetupObj-> selectIndex;					// set new unit for loadcell 0.
		loadcell_change_unit( 0, index );					//	PHJ	change unit for loadcell zero
	}
	else if ( PANEL_MENU_ACTION_MODE_GET_CNFG_INDEX == actionMode ) {
		// map configuration content to menu selection index.
		index =  gabSensorShowCBunitsFNV[ 0 ];
	}
	else if (actionMode >= PANEL_MENU_ACTION_MODE_EXIT_ITEM )
	    // anncState = LED_SEG_STATE_STEADY;
		anncState = LED_SEG_STATE_OFF;				// 2011-07-18 -WFC- turn off unit annunciator after exit unit menu item.

	//panel_main_update_unit_annunciator_state( unit, anncState );
	if ( SENSOR_UNIT_KG == index ) {
		//led_display_kg_blink();
		led_display_kg_on();
		led_display_lb_direct_off();			//	PHJ
		// 2011-04-29 -WFC- gbLbFlag = OFF;							//	PHJ
	}
	else if ( SENSOR_UNIT_LB == index )  {
		//led_display_lb_blink();
		led_display_lb_direct_on();				//	PHJ
		// 2011-04-29 -WFC-  gbLbFlag = ON;							//	PHJ
		led_display_kg_off();
	}
	else {
		index = 0;
		// unit is not lb or kg, turn them off.
		led_display_lb_direct_off();			//	PHJ
		// 2011-04-29 -WFC-  gbLbFlag = OFF;							//	PHJ
		led_display_kg_off();
	}

	return index;
} // end panel_setup_unit_mapper(,,)



/**
 * Method to map between setpoint logic variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 *  cmp logic and value mode. bit 7== 0=disabled, 1== enabled, bit 6 to 4 compare logic, 1== '<', 2== '>'; bit 3 to 0; value mode; 0 == net_gross, 1== gross, 2==total, 3==total count
 *
 * History:  Created on 2009/07/15 by Wai Fai Chin
 * 2011-07-19 -WFC- When user enter a set action, it forces its host thread to enter into next menu item without user press an enter key.
 */


BYTE	panel_setup_setpoint_cmp_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE setpointNum;
	BYTE index;
	BYTE b;

	setpointNum = gPanelSetupTopMenuObj.selectIndex;
	// convert top menu selection index into setpoint number.
	if ( setpointNum >= PANEL_TOP_MENU_SETPOINT_BASE_INDEX ) { 
		setpointNum -= PANEL_TOP_MENU_SETPOINT_BASE_INDEX;
	}	

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		b = pSetupObj-> selectIndex;
		if ( pSetupObj-> selectIndex ) {
			b <<=4;
			b |= SP_CMP_ENABLED_BIT;
			gaSP_cmp_logic_value_modeFNV[ setpointNum ] &= 0x0F;
			gaSP_cmp_logic_value_modeFNV[ setpointNum ] |= b;
		}
		else { // selectIndex = 0; disabled setpoint.
			gaSP_cmp_logic_value_modeFNV[ setpointNum ] &= ~SP_CMP_ENABLED_BIT;
		}
		index = pSetupObj-> selectIndex;
		pSetupObj-> nextMove = PSSMC_NEXTMOVE_ENTER_INTO_NEXT_PM_ITEM;		// 2011-07-19 -WFC- This force its host thread to enter into next menu item without user press an enter key.
	}
	else { // map configuration content to menu selection index.
		b = gaSP_cmp_logic_value_modeFNV[ setpointNum ];
		if ( b & SP_CMP_ENABLED_BIT ) {
			b &= SP_CMP_LOGIC_MASK;
			b >>=4;
			if ( b > 2 )
				b = 2;
			index = b;
		}
		else { // setpoint disabled.
			index = 0;
		}
	}

	return index;
} // end panel_setup_setpoint_cmp_mapper(,,)



/**
 * Method to get and set setpoint value. The setpoint number is based on the gPanelSetupTopMenuObj.selectIndex.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on input selectIndex;
 *
 * History:  Created on 2009/07/15 by Wai Fai Chin
 */


BYTE	panel_setup_setpoint_value_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE setpointNum;
	float *pfV;
	
	pfV = pV;
	
	setpointNum = gPanelSetupTopMenuObj.selectIndex;
	if ( setpointNum >= PANEL_TOP_MENU_SETPOINT_BASE_INDEX ) {
		setpointNum -= PANEL_TOP_MENU_SETPOINT_BASE_INDEX;
	}

	if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) {
		gaSP_fCmpValueFNV[ setpointNum ] = *pfV;
	}
	else {
		*pfV = gaSP_fCmpValueFNV[ setpointNum ];
	}

	return pSetupObj-> selectIndex;

} // end  panel_setup_setpoint_value_mapper(,,)


/**
 * Method to get and set setpoint mode. The setpoint number is based on the gPanelSetupTopMenuObj.selectIndex.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on input selectIndex;
 *
 * History:  Created on 2009/07/15 by Wai Fai Chin
 * 2012-09-20 -WFC- Rewrote setpoint mode mapper. 2012-10-03 -DLM-
 * 2015-09-09 -WFC- gaSP_cmp_logic_value_modeFNV, value mode is now bit2 to bit0 instead of bit3 to bit0.
 */


BYTE	panel_setup_setpoint_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE setpointNum;

	setpointNum = gPanelSetupTopMenuObj.selectIndex;
	// convert top menu selection index into setpoint number.
	if ( setpointNum >= PANEL_TOP_MENU_SETPOINT_BASE_INDEX ) { 
		setpointNum -= PANEL_TOP_MENU_SETPOINT_BASE_INDEX;
	}	

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		// gaSP_cmp_logic_value_modeFNV[ setpointNum ] &= 0xF0;
		gaSP_cmp_logic_value_modeFNV[ setpointNum ] &= 0xF8;		// 2015-09-09 -WFC- bit2 to bit0 is value mode
		gaSP_cmp_logic_value_modeFNV[ setpointNum ] |= pSetupObj-> selectIndex;
	}

	setpointNum = gaSP_cmp_logic_value_modeFNV[ setpointNum ];
	setpointNum &= 0x0F;
	return setpointNum;
} // end  panel_setup_setpoint_mode_mapper(,,)


/**
 * Method to map between total mode and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009-07-21 by Wai Fai Chin
 * 2011-04-26 -WFC- Added logic to map menu item selection index to total mode because of LED menu item ordering is different.
 */

BYTE	panel_setup_total_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE		totalMode;

	// map menu item selection index to total mode.
	memcpy_P ( &totalMode,  &gbMenuIndexToTotalMode[ pSetupObj-> selectIndex ], sizeof(BYTE));

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode )
		gabTotalModeFNV[ PANEL_SETUP_LOADCELL_NUM ] = totalMode;

	// map total mode to menu item selection index
	memcpy_P ( &totalMode,  &gbTotalModeToMenuIndex[ gabTotalModeFNV[ PANEL_SETUP_LOADCELL_NUM ] ], sizeof(BYTE));

	return totalMode;	// return menu item index
} // end	panel_setup_total_mode_mapper(,,)
/* Old function before re order LED total menu items.
BYTE	panel_setup_total_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	LOADCELL_T *pLc;
	pLc = &gaLoadcell[ 0 ];

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode )
		gabTotalModeFNV[0] = pSetupObj-> selectIndex;

//		pLc-> totalT.status &= ~LC_TOTAL_STATUS_NOT_ALLOW;			// NOT ALLOW TOTAL again until the load had dropped.

	return gabTotalModeFNV[0];
} // end	panel_setup_total_mode_mapper(,,) 
*/


// ****************************************************************************
// ****************************************************************************
//					C A L I B R A T I O N	M E N U   M E T H O D S
// ****************************************************************************
// ****************************************************************************


/**
 * Check if this loadcell allow to calibrate.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu object
 *
 * @return 0 == not allow. 1== allow.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2009/07/24 by Wai Fai Chin
 * 2011-04-12 -WFC- Setup loadcell runmode, disabled peak hold before calls adc_lt_construct_op_desc().
 * 2011-04-20 -WFC- Added multipoint cal with MENU_NEW_CAL_STEP_LOAD_LAST.
 * 2011-04-26 -WFC- Added dynamic disabled menu items.
 * 2011-04-28 -WFC- handle exit event.
 * 2011-05-03 -WFC- fixed undefined state after exit uncompleted cal by calling panel_setup_abort_cal().
 */

//enum {
//	MENU_NEW_CAL_STEP_UNIT,
//	MENU_NEW_CAL_STEP_CAP,
//	MENU_NEW_CAL_STEP_COUNTBY,
//	MENU_NEW_CAL_STEP_ZEROING,
//	MENU_NEW_CAL_STEP_LOAD_1,
//	MENU_NEW_CAL_STEP_LOAD_2,
//	MENU_NEW_CAL_STEP_LOAD_3,
//	MENU_NEW_CAL_STEP_LOAD_LAST,
//	MENU_NEW_CAL_STEP_READ_RCAL
//};


BYTE	panel_setup_does_it_allows_new_cal( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )
{
	BYTE			n;
	BYTE			status;
	SENSOR_CAL_T	*pSensorCal;
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to sensor local descriptor

	status = NO;
	n = PANEL_SETUP_LOADCELL_NUM;

	if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMove ) {
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {		// get sensor 0 calibration pointer.
			status = cal_allows_new_cal( pSensorCal );
			if ( CAL_ERROR_NONE == status ) {
				cmd_act_set_defaults( 0x0C );
				pSnDesc = &gaLSensorDescriptor[ n ];
				pSnDesc-> conversion_cnfg |= SENSOR_CNFG_ENABLED; // force to enable sensor because user wants to calibrate this sensor.
				if ( gabCalTmpZone[n] ) {					// if specified a none zero temperature zone, then
														// force to enable temperature compensation because user specified temperature zone.
					pSnDesc-> conversion_cnfg |= SENSOR_FEATURE_TEMPERATURE_CMP;
				}
				pSnDesc-> cnfg |= SENSOR_CNFG_ENABLED;				// force to enable sensor because user wants to calibrate this sensor.
				if ( sensor_descriptor_init( n ) ) {				// if it has valid contents,
					// NOTE that pSnDesc-> type has been init based on gabSensorTypeFNV[] by sensor_descriptor_init();
					// TODO: if sensor type is loadcell,then else adc_cpu_construct_op_desc(). 
					switch ( pSnDesc-> type )	{
						case SENSOR_TYPE_LOADCELL :
								((LOADCELL_T *) (pSnDesc-> pDev))-> runModes |= LC_RUN_MODE_IN_CAL;			// flag this loadcell is in calibration mode
								((LOADCELL_T *) (pSnDesc-> pDev))-> runModes &= ~( LC_RUN_MODE_NORMAL_ACTIVE | LC_RUN_MODE_PEAK_HOLD_ENABLED);	// flag it is NOT in normal active mode that it may not has a cal table and it is in cal mode. Disabled Peak hold mode.
					}
					adc_lt_construct_op_desc( pSnDesc, n );		// then construct ADC operation descriptor.
				}
				pSensorCal-> countby.unit	= gabCalCB_unit[ n ];
				pSensorCal-> capacity 		= gafCal_capacity[ n ];
						
				pSensorCal-> status = CAL_STATUS_GOT_UNIT;			// next step is to get unit setting.
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_UNIT );		// disabled all items except unit item.
				status = YES;
				gabCalOpStep[ n ] = pSensorCal-> status;			// set cal status to cal op status for user command interface.
			}
		}
		
		if ( NO == status ){
			pSetupObj-> msgID 			= MENU_MSG_ID_NO;			// display "No" for 3 seconds to tell user that NOT ALLOW to calibrate.
			pSetupObj-> msgDisplayTime	= TT_3SEC;
			gabCalErrorStatus[ n ] = CAL_ERROR_NOT_ALLOW;			// set cal status to cal op status for user command interface.
		}
	} // end if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMOve ) {}
	else if ( (PSTMC_CURMOVE_NEXT == pSetupObj-> curMove ) || (PSTMC_CURMOVE_ENTER == pSetupObj-> curMove ) ) {
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {			// get sensor 0 calibration pointer.
			if ( CAL_STATUS_GOT_UNIT == pSensorCal-> status ) {	// if cal step is to get unit,
				pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_UNIT;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_UNIT );		// disabled all items except unit item.
			}
			else if ( CAL_STATUS_GOT_UNIT_CAP == pSensorCal-> status ) {	// if cal step is to get capacity,
				pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_CAP;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_CAP );		// disabled all items except capacity item.
			}
			else if ( CAL_STATUS_GET_COUNTBY == pSensorCal-> status ) {	// if cal step is to get countby
				pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_COUNTBY;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_COUNTBY );		// disabled all items except countby item.
			}
			else if ( CAL_STATUS_GOT_COUNTBY == pSensorCal-> status ) {	// if cal step had completed got countby and ready for zero point cal
				pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_ZEROING;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_ZEROING );		// disabled all items except unload item.
			}
			else if ( 0 == pSensorCal-> status ) {										// if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_LOAD_1;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_LOAD_1 );		// disabled all items except load1 item.
			}
			else if ( 1 == pSensorCal-> status ) {										// if cal step had completed load1 span cal point and ready for 2nd loaded loadcell cal
				pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_LOAD_2;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_LOAD_2 );		// disabled all items except load2 item.
			}
			else if ( 2 == pSensorCal-> status ) {										// if cal step had completed load2 span cal point and ready for 3rd loaded loadcell cal
				pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_LOAD_3;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_LOAD_3 );		// disabled all items except load3 item.
			}
			else if ( 3 == pSensorCal-> status ) {										// if cal step had completed load3 span cal point and ready for 3rd loaded loadcell cal
				pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_LOAD_LAST;
				pSetupObj->disabledItemFlags = ~( 1 << MENU_NEW_CAL_STEP_LOAD_LAST );	// disabled all items except load4 item.
			}
			else if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_READ_RCAL;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_READ_RCAL );		// disabled all items except read r cal item.
			}
		}
	}
	else if ( PSTMC_CURMOVE_EXIT == pSetupObj-> curMove ) {								// user decided to exit
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {										// if it has valid cal table
			if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// see if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				if ( MENU_NEW_CAL_STEP_READ_RCAL != pSetupObj-> selectIndex ) {				// Yes, calibration can be completed as is. If it was not in Read RCal step, goto there.
					pSetupObj-> selectIndex = MENU_NEW_CAL_STEP_READ_RCAL;
					pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_CAL_STEP_READ_RCAL );	// disabled all items except read r cal item.
					pSetupObj-> curMove = PSTMC_CURMOVE_NEXT;								// tell panel_setup_top_menu_thread() to not to exit and goto new menu item of selectIndex.
				}
			}
			// 2011-05-03 -WFC v
			else {
				panel_setup_abort_cal( n );
			}
			// 2011-05-03 -WFC ^
		}
	}
	
	return status;
} // end panel_setup_does_it_allows_new_cal()


/**
 * Check if this loadcell allow to perform a new r calibrate.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu object
 *
 * @return 0 == not allow. 1== allow.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2009-07-24 by Wai Fai Chin
 * 2011-04-12 -WFC- Setup loadcell runmode, disabled peak hold before calls adc_lt_construct_op_desc().
 * 2011-04-20 -WFC- Added multipoint cal with MENU_NEW_CAL_STEP_LOAD_LAST.
 * 2011-04-27 -WFC- Added dynamic disabled menu items.
 * 2011-04-28 -WFC- handle exit event.
 * 2011-05-03 -WFC- fixed undefined state after exit uncompleted cal by calling panel_setup_abort_cal().
 */

/// enum for above brand new R calibration menu group.
//enum {
//	MENU_NEW_R_CAL_STEP_UNIT,
//	MENU_NEW_R_CAL_STEP_CAP,
//	MENU_NEW_R_CAL_STEP_COUNTBY,
//	MENU_NEW_R_CAL_STEP_ZEROING,
//	MENU_NEW_R_CAL_STEP_GET_R_CAL,
//	MENU_NEW_R_CAL_STEP_READ_RCAL
//};

BYTE	panel_setup_does_it_allows_new_rcal( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )
{
	BYTE			n;
	BYTE			status;
	SENSOR_CAL_T	*pSensorCal;
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to sensor local descriptor

	status = NO;
	n = PANEL_SETUP_LOADCELL_NUM;

	if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMove ) {
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {		// get sensor 0 calibration pointer.
			status = cal_allows_new_cal( pSensorCal );
			if ( CAL_ERROR_NONE == status ) {
				cmd_act_set_defaults( 0x0C );
				pSnDesc = &gaLSensorDescriptor[ n ];
				pSnDesc-> conversion_cnfg |= SENSOR_CNFG_ENABLED; // force to enable sensor because user wants to calibrate this sensor.
				if ( gabCalTmpZone[n] ) {					// if specified a none zero temperature zone, then
														// force to enable temperature compensation because user specified temperature zone.
					pSnDesc-> conversion_cnfg |= SENSOR_FEATURE_TEMPERATURE_CMP;
				}
				pSnDesc-> cnfg |= SENSOR_CNFG_ENABLED;				// force to enable sensor because user wants to calibrate this sensor.
				if ( sensor_descriptor_init( n ) ) {				// if it has valid contents,
					// NOTE that pSnDesc-> type has been init based on gabSensorTypeFNV[] by sensor_descriptor_init();
					// TODO: if sensor type is loadcell,then else adc_cpu_construct_op_desc().
					switch ( pSnDesc-> type )	{
						case SENSOR_TYPE_LOADCELL :
								((LOADCELL_T *) (pSnDesc-> pDev))-> runModes |= LC_RUN_MODE_IN_CAL;			// flag this loadcell is in calibration mode
								((LOADCELL_T *) (pSnDesc-> pDev))-> runModes &= ~( LC_RUN_MODE_NORMAL_ACTIVE | LC_RUN_MODE_PEAK_HOLD_ENABLED);	// flag it is NOT in normal active mode that it may not has a cal table and it is in cal mode. Disabled Peak hold mode.
					}
					adc_lt_construct_op_desc( pSnDesc, n );		// then construct ADC operation descriptor.
				}
				pSensorCal-> countby.unit	= gabCalCB_unit[ n ];
				pSensorCal-> capacity 		= gafCal_capacity[ n ];

				pSensorCal-> status = CAL_STATUS_GOT_UNIT;			// next step is to get unit setting.
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_R_CAL_STEP_UNIT );		// disabled all items except unit item.
				pSetupObj-> selectIndex = MENU_NEW_R_CAL_STEP_UNIT;
				status = YES;
				gabCalOpStep[ n ] = pSensorCal-> status;			// set cal status to cal op status for user command interface.
			}
		}

		if ( NO == status ){
			pSetupObj-> msgID 			= MENU_MSG_ID_NO;			// display "No" for 3 seconds to tell user that NOT ALLOW to calibrate.
			pSetupObj-> msgDisplayTime	= TT_3SEC;
			gabCalErrorStatus[ n ] = CAL_ERROR_NOT_ALLOW;			// set cal status to cal op status for user command interface.
		}
	} // end if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMOve ) {}
	else if ( (PSTMC_CURMOVE_NEXT == pSetupObj-> curMove ) || (PSTMC_CURMOVE_ENTER == pSetupObj-> curMove ) ) {
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {			// get sensor 0 calibration pointer.
			if ( CAL_STATUS_GOT_UNIT == pSensorCal-> status ) {	// if cal step is to get unit,
				pSetupObj-> selectIndex = MENU_NEW_R_CAL_STEP_UNIT;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_R_CAL_STEP_UNIT );		// disabled all items except unit item.
			}
			else if ( CAL_STATUS_GOT_UNIT_CAP == pSensorCal-> status ) {	// if cal step is to get capacity,
				pSetupObj-> selectIndex = MENU_NEW_R_CAL_STEP_CAP;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_R_CAL_STEP_CAP );		// disabled all items except capacity item.
			}
			else if ( CAL_STATUS_GET_COUNTBY == pSensorCal-> status ) {	// if cal step is to get countby
				pSetupObj-> selectIndex = MENU_NEW_R_CAL_STEP_COUNTBY;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_R_CAL_STEP_COUNTBY );		// disabled all items except countby item.
			}
			else if ( CAL_STATUS_GOT_COUNTBY == pSensorCal-> status ) {	// if cal step had completed got countby and ready for zero point cal
				pSetupObj-> selectIndex = MENU_NEW_R_CAL_STEP_ZEROING;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_R_CAL_STEP_ZEROING );		// disabled all items except unload item.
			}
			else if ( 0 == pSensorCal-> status ) {										// if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				pSetupObj-> selectIndex = MENU_NEW_R_CAL_STEP_GET_R_CAL;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_R_CAL_STEP_GET_R_CAL );		// disabled all items except load1 item.
			}
			else if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				pSetupObj-> selectIndex = MENU_NEW_R_CAL_STEP_READ_RCAL;
				pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_R_CAL_STEP_READ_RCAL );		// disabled all items except read r cal item.
			}
		}
	}
	else if ( PSTMC_CURMOVE_EXIT == pSetupObj-> curMove ) {								// user decided to exit
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {										// if it has valid cal table
			if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// see if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				if ( MENU_NEW_R_CAL_STEP_READ_RCAL != pSetupObj-> selectIndex ) {			// Yes, calibration can be completed as is. If it was not in Read RCal step, goto there.
					pSetupObj-> selectIndex = MENU_NEW_R_CAL_STEP_READ_RCAL;
					pSetupObj->disabledItemFlags = ~(1 << MENU_NEW_R_CAL_STEP_READ_RCAL );	// disabled all items except read r cal item.
					pSetupObj-> curMove = PSTMC_CURMOVE_NEXT;								// tell panel_setup_top_menu_thread() to not to exit and goto new menu item of selectIndex.
				}
			}
			// 2011-05-03 -WFC v
			else {
				panel_setup_abort_cal( n );
			}
			// 2011-05-03 -WFC ^
		}
	}

	return status;
} // end panel_setup_does_it_allows_new_rcal()

/**
 * Check if this loadcell can be re-calibrate WITHOUT re-enter capacity, unit and countby.
 * Rcal must required that loadcell channel already calibrated.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu object
 *
 * @return 0 == not allow. 1== allow.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2009/09/30 by Wai Fai Chin
 * 2011-04-12 -WFC- Setup loadcell runmode, disabled peak hold before calls adc_lt_construct_op_desc().
 * 2011-04-20 -WFC- Added multipoint cal with MENU_RE_CAL_STEP_LOAD_LAST.
 * 2011-04-26 -WFC- Added dynamic disabled menu items.
 * 2011-04-28 -WFC- handle exit event.
 * 2011-05-03 -WFC- fixed undefined state after exit uncompleted cal by calling panel_setup_abort_cal().
 */

BYTE	panel_setup_does_it_allows_recal( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )
{
	BYTE			n;
	BYTE			status;
	SENSOR_CAL_T	*pSensorCal;
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to sensor local descriptor

	status = NO;
	n = PANEL_SETUP_LOADCELL_NUM;

	if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMove ) {
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {		// get sensor 0 calibration pointer.
			if ( CAL_STATUS_COMPLETED == pSensorCal-> status ) {	// if this sensor has a completed calibration table.
				pSnDesc = &gaLSensorDescriptor[ n ];
				pSnDesc-> conversion_cnfg |= SENSOR_CNFG_ENABLED; // force to enable sensor because user wants to calibrate this sensor.
				if ( gabCalTmpZone[n] ) {				// if specified a none zero temperature zone, then
														// force to enable temperature compensation because user specified temperature zone.
					pSnDesc-> conversion_cnfg |= SENSOR_FEATURE_TEMPERATURE_CMP;
				}
				pSnDesc-> cnfg |= SENSOR_CNFG_ENABLED;				// force to enable sensor because user wants to calibrate this sensor.
				if ( sensor_descriptor_init( n ) ) {				// if it has valid contents,
					// NOTE that pSnDesc-> type has been init based on gabSensorTypeFNV[] by sensor_descriptor_init();
					((LOADCELL_T *) (pSnDesc-> pDev))-> runModes |= LC_RUN_MODE_IN_CAL;			// flag this loadcell is in calibration mode
					((LOADCELL_T *) (pSnDesc-> pDev))-> runModes &= ~( LC_RUN_MODE_NORMAL_ACTIVE | LC_RUN_MODE_PEAK_HOLD_ENABLED);	// flag it is NOT in normal active mode that it may not has a cal table and it is in cal mode. Disabled Peak hold mode.
					adc_lt_construct_op_desc( pSnDesc, n );		// then contruct ADC operation descriptor.
				}
						
				status = YES;
				pSetupObj->disabledItemFlags = ~(1 << MENU_RE_CAL_STEP_ZEROING );		// disabled all items except zero item.
				pSetupObj-> selectIndex = MENU_RE_CAL_STEP_ZEROING;
			}
		}

		if ( NO == status ){
			pSetupObj-> msgID 			= MENU_MSG_ID_NO;			// display "No" for 3 seconds to tell user that NOT ALLOW to calibrate.
			pSetupObj-> msgDisplayTime	= TT_3SEC;
			gabCalErrorStatus[ n ] = CAL_ERROR_NOT_ALLOW;			// set cal status to cal op status for user command interface.
		}
	} // end if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMove ) {
	else if ( (PSTMC_CURMOVE_NEXT == pSetupObj-> curMove ) || (PSTMC_CURMOVE_ENTER == pSetupObj-> curMove ) ) {
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {			// get sensor 0 calibration pointer.
			if ( CAL_STATUS_GOT_COUNTBY == pSensorCal-> status ||
				CAL_STATUS_COMPLETED == pSensorCal-> status ) {							// if cal step had completed got countby and ready for zero point cal
				pSetupObj-> selectIndex = MENU_RE_CAL_STEP_ZEROING;
				pSetupObj->disabledItemFlags = ~(1 << MENU_RE_CAL_STEP_ZEROING );		// disabled all items except unload item.
			}
			else if ( 0 == pSensorCal-> status ) {										// if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				pSetupObj-> selectIndex = MENU_RE_CAL_STEP_LOAD_1;
				pSetupObj->disabledItemFlags = ~(1 << MENU_RE_CAL_STEP_LOAD_1 );		// disabled all items except load1 item.
			}
			else if ( 1 == pSensorCal-> status ) {										// if cal step had completed load1 span cal point and ready for 2nd loaded loadcell cal
				pSetupObj-> selectIndex = MENU_RE_CAL_STEP_LOAD_2;
				pSetupObj->disabledItemFlags = ~(1 << MENU_RE_CAL_STEP_LOAD_2 );		// disabled all items except load2 item.
			}
			else if ( 2 == pSensorCal-> status ) {										// if cal step had completed load2 span cal point and ready for 3rd loaded loadcell cal
				pSetupObj-> selectIndex = MENU_RE_CAL_STEP_LOAD_3;
				pSetupObj->disabledItemFlags = ~(1 << MENU_RE_CAL_STEP_LOAD_3 );		// disabled all items except load3 item.
			}
			else if ( 3 == pSensorCal-> status ) {										// if cal step had completed load3 span cal point and ready for 3rd loaded loadcell cal
				pSetupObj-> selectIndex = MENU_RE_CAL_STEP_LOAD_LAST;
				pSetupObj->disabledItemFlags = ~(1 << MENU_RE_CAL_STEP_LOAD_LAST );	// disabled all items except load4 item.
			}
			else if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				pSetupObj-> selectIndex = MENU_RE_CAL_STEP_READ_RCAL;
				pSetupObj->disabledItemFlags = ~(1 << MENU_RE_CAL_STEP_READ_RCAL );	// disabled all items except read r cal item.
			}
		}
	}
	else if ( PSTMC_CURMOVE_EXIT == pSetupObj-> curMove ) {								// user decided to exit
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {										// if it has valid cal table
			if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// see if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				if ( MENU_RE_CAL_STEP_READ_RCAL != pSetupObj-> selectIndex ) {			// Yes, calibration can be completed as is. If it was not in Read RCal step, goto there.
					pSetupObj-> selectIndex = MENU_RE_CAL_STEP_READ_RCAL;
					pSetupObj->disabledItemFlags = ~(1 << MENU_RE_CAL_STEP_READ_RCAL );		// disabled all items except read r cal item.
					pSetupObj-> curMove = PSTMC_CURMOVE_NEXT;								// tell panel_setup_top_menu_thread() to not to exit and goto new menu item of selectIndex.
				}
			}
			// 2011-05-03 -WFC v
			else {
				panel_setup_abort_cal( n );
			}
			// 2011-05-03 -WFC ^
		}
	}
	
	return status;
} // end panel_setup_does_it_allows_recal()


/**
 * Check if this loadcell can be re-calibrate r cal WITHOUT re-enter capacity, unit and countby.
 * Rcal must required that loadcell channel already calibrated.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu object
 *
 * @return 0 == not allow. 1== allow.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2011-04-27 by Wai Fai Chin
 * 2011-04-28 -WFC- handle exit event.
 * 2011-05-03 -WFC- fixed undefined state after exit uncompleted cal by calling panel_setup_abort_cal().
 */

/// enum for above subsequent Re R calibration menu group.
//enum {
//	MENU_RE_R_CAL_STEP_ZEROING,
//	MENU_RE_R_CAL_STEP_GET_R_CAL,
//	MENU_RE_R_CAL_STEP_READ_RCAL
//};

BYTE	panel_setup_does_it_allows_re_rcal( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )
{
	BYTE			n;
	BYTE			status;
	SENSOR_CAL_T	*pSensorCal;
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to sensor local descriptor

	status = NO;
	n = PANEL_SETUP_LOADCELL_NUM;

	if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMove ) {
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {		// get sensor 0 calibration pointer.
			if ( CAL_STATUS_COMPLETED == pSensorCal-> status ) {	// if this sensor has a completed calibration table.
				pSnDesc = &gaLSensorDescriptor[ n ];
				pSnDesc-> conversion_cnfg |= SENSOR_CNFG_ENABLED; // force to enable sensor because user wants to calibrate this sensor.
				if ( gabCalTmpZone[n] ) {				// if specified a none zero temperature zone, then
														// force to enable temperature compensation because user specified temperature zone.
					pSnDesc-> conversion_cnfg |= SENSOR_FEATURE_TEMPERATURE_CMP;
				}
				pSnDesc-> cnfg |= SENSOR_CNFG_ENABLED;				// force to enable sensor because user wants to calibrate this sensor.
				if ( sensor_descriptor_init( n ) ) {				// if it has valid contents,
					// NOTE that pSnDesc-> type has been init based on gabSensorTypeFNV[] by sensor_descriptor_init();
					((LOADCELL_T *) (pSnDesc-> pDev))-> runModes |= LC_RUN_MODE_IN_CAL;			// flag this loadcell is in calibration mode
					((LOADCELL_T *) (pSnDesc-> pDev))-> runModes &= ~( LC_RUN_MODE_NORMAL_ACTIVE | LC_RUN_MODE_PEAK_HOLD_ENABLED);	// flag it is NOT in normal active mode that it may not has a cal table and it is in cal mode. Disabled Peak hold mode.
					adc_lt_construct_op_desc( pSnDesc, n );		// then contruct ADC operation descriptor.
				}

				status = YES;
				pSetupObj->disabledItemFlags = ~(1 << MENU_RE_R_CAL_STEP_ZEROING );		// disabled all items except zero item.
				pSetupObj-> selectIndex = MENU_RE_R_CAL_STEP_ZEROING;
			}
		}

		if ( NO == status ){
			pSetupObj-> msgID 			= MENU_MSG_ID_NO;			// display "No" for 3 seconds to tell user that NOT ALLOW to calibrate.
			pSetupObj-> msgDisplayTime	= TT_3SEC;
			gabCalErrorStatus[ n ] = CAL_ERROR_NOT_ALLOW;			// set cal status to cal op status for user command interface.
		}
	} // end if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMove ) {
	else if ( (PSTMC_CURMOVE_NEXT == pSetupObj-> curMove ) || (PSTMC_CURMOVE_ENTER == pSetupObj-> curMove ) ) {
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {			// get sensor 0 calibration pointer.
			if ( CAL_STATUS_GOT_COUNTBY == pSensorCal-> status ||
				CAL_STATUS_COMPLETED == pSensorCal-> status ) {							// if cal step had completed got countby and ready for zero point cal
				pSetupObj-> selectIndex = MENU_RE_R_CAL_STEP_ZEROING;
				pSetupObj-> disabledItemFlags = ~(1 << MENU_RE_R_CAL_STEP_ZEROING );		// disabled all items except unload item.
			}
			else if ( 0 == pSensorCal-> status ) {										// if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				pSetupObj-> selectIndex = MENU_RE_R_CAL_STEP_GET_R_CAL;
				pSetupObj-> disabledItemFlags = ~(1 << MENU_RE_R_CAL_STEP_GET_R_CAL );		// disabled all items except load1 item.
			}
			else if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				pSetupObj-> selectIndex = MENU_RE_R_CAL_STEP_READ_RCAL;
				pSetupObj-> disabledItemFlags = ~(1 << MENU_RE_R_CAL_STEP_READ_RCAL );		// disabled all items except read r cal item.
			}
		}
	}
	else if ( PSTMC_CURMOVE_EXIT == pSetupObj-> curMove ) {								// user decided to exit
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {										// if it has valid cal table
			if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// see if cal step had completed zero point cal and ready for 1st loaded loadcell cal
				if ( MENU_RE_R_CAL_STEP_READ_RCAL != pSetupObj-> selectIndex ) {			// Yes, calibration can be completed as is. If it was not in Read RCal step, goto there.
					pSetupObj-> selectIndex = MENU_RE_R_CAL_STEP_READ_RCAL;
					pSetupObj-> disabledItemFlags = ~(1 << MENU_RE_R_CAL_STEP_READ_RCAL );	// disabled all items except read r cal item.
					pSetupObj-> curMove = PSTMC_CURMOVE_NEXT;								// tell panel_setup_top_menu_thread() to not to exit and goto new menu item of selectIndex.
				}
			}
			// 2011-05-03 -WFC v
			else {
				panel_setup_abort_cal( n );
			}
			// 2011-05-03 -WFC ^
		}
	}

	return status;
} // end panel_setup_does_it_allows_re_rcal()



/**
 * It coordinates selection of cal menu group.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu group object
 *
 * @return index of next group item dynamically based on the sensor cal status.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2009-09-29 by Wai Fai Chin
 * 2011-04-27 -WFC- Rewrote: If calibration had completed or it has 0 and a span point, then only allow to standard setup menu.
 */

BYTE	panel_setup_cal_menu_group_method( PANEL_SETUP_TOP_MENU_GROUP_CLASS *pSetupObj )
{
	BYTE			n;
	BYTE			status;
	SENSOR_CAL_T	*pSensorCal;

	n = PANEL_SETUP_LOADCELL_NUM;

	if ( PANEL_SETUP_STATUS_EXIT_SAVE == gbPanelSetupStatus ) {
		status = panel_setup_save_cal_talbe( n );
		if ( status ) { // if error had occurred
			panel_setup_display_msg_duration_P( MENU_MSG_ID_FAIL, TT_2SEC );		// display "FAIL"
			pSetupObj->disabledItemFlags = PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL;
		}
		else {
			pSetupObj->disabledItemFlags = ~(1 << MENU_CAL_GROUP_RESTRICT_CNFG );	// disabled all items except standard setup item.
			panel_setup_display_msg_duration_P( MENU_MSG_ID_STORE, TT_2SEC );		// display "StorE"
			pSetupObj-> selectIndex = MENU_CAL_GROUP_RESTRICT_CNFG;		// the parent thread will increment it to goto to standard mode and azm stuff menus after save calibration. 2011-04-26 -WFC-
		}
		gbPanelSetupStatus = 0;		// clear status
		panel_key_init();	// empty key buffer to prevent user randomly press other keys.
	}
	else if ( sensor_get_cal_base( n, &pSensorCal ) ) {						// get sensor 0 calibration pointer.
		if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// if had a zero and a span point, then it may exit by an ESC key.
			pSetupObj-> selectIndex = MENU_CAL_GROUP_RESTRICT_CNFG;			// goto the recalibration group.
			pSetupObj->disabledItemFlags = ~(1 << MENU_CAL_GROUP_RESTRICT_CNFG );		// disabled all items except standard setup item.
		}
	}

	return pSetupObj-> selectIndex;
} // end panel_setup_cal_menu_group_method()

/**
 * It coordinates selection of Re cal menu group.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu group object
 *
 * @return index of next group item dynamically based on the sensor cal status.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 * If calibration had completed or it has 0 and a span point, then only allow to standard setup menu.
 *
 * History:  Created on 2011-04-26 by Wai Fai Chin
 */


BYTE	panel_setup_recal_menu_group_method( PANEL_SETUP_TOP_MENU_GROUP_CLASS *pSetupObj )
{
	BYTE			n;
	BYTE			status;
	SENSOR_CAL_T	*pSensorCal;

	n = PANEL_SETUP_LOADCELL_NUM;

	if ( PANEL_SETUP_STATUS_EXIT_SAVE == gbPanelSetupStatus ) {
		status = panel_setup_save_cal_talbe( n );
		if ( status ) { // if error had occurred
			panel_setup_display_msg_duration_P( MENU_MSG_ID_FAIL, TT_2SEC );		// display "FAIL"
			pSetupObj->disabledItemFlags = PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL;
			pSetupObj-> selectIndex = MENU_RECAL_GROUP_CAL;		// the parent thread will increment it to goto to standard mode and azm stuff menus after save calibration. 2011-04-26 -WFC-
		}
		else {
			pSetupObj->disabledItemFlags = ~(1 << MENU_RECAL_GROUP_RESTRICT_CNFG );	// disabled all items except standard setup item.
			panel_setup_display_msg_duration_P( MENU_MSG_ID_STORE, TT_2SEC );		// display "StorE"
			pSetupObj-> selectIndex = MENU_RECAL_GROUP_RESTRICT_CNFG;		// the parent thread will increment it to goto to standard mode and azm stuff menus after save calibration. 2011-04-26 -WFC-
		}
		gbPanelSetupStatus = 0;		// clear status
		panel_key_init();	// empty key buffer to prevent user randomly press other keys.
	}
	else if ( sensor_get_cal_base( n, &pSensorCal ) ) {						// get sensor 0 calibration pointer.
		if ( (pSensorCal-> status > 0 ) && ( pSensorCal-> status < MAX_CAL_POINTS)) {	// if had a zero and a span point, then it may exit by an ESC key.
			pSetupObj-> selectIndex = MENU_RECAL_GROUP_RESTRICT_CNFG;					// goto the recalibration group.
			pSetupObj->disabledItemFlags = ~(1 << MENU_RECAL_GROUP_RESTRICT_CNFG );		// disabled all items except standard setup item.
		}
	}

	return pSetupObj-> selectIndex;
} // end panel_setup_recal_menu_group_method()




/**
 * Method to map between standard mode variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 *
 * @note Challenger3 and other stand alone scale only handle 1 loadcell and it is loadcell 0.
 *
 * History:  Created on 2009/08/04 by Wai Fai Chin
 * 2011-09-01 -WFC- Auto configured all required settings based on different standard mode such as NTEP, OILM etc.
 * 2011-10-25 -WFC- Legal for Trade mode Motion Detection band is 1 d, standard is 3d.
 *          Note: setting action in any mode will default loadcell software filter to LO. User can changed filter level later from filter menu.
 * 2012-02-22 -WFC- No auto default AZM and filter level settings based on different standard mode because it confuses user.
 *          However, it enabled motion detection for any standard mode. Motion detection band is set to 1d for NTEP and OIML, 3d for Industry.
 * 2012-02-28 -WFC-  set standard mode and motion detect enabled and preserved other SCALE_STD_MODE_ bits when set a standard mode.
 * 2016-03-21 -WFC- removed enabled zero on powerup for OIML.
 */

BYTE	panel_setup_std_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		index = pSetupObj-> selectIndex;
		// 2012-02-28 -WFC- v
		// 2012-02-28 -WFC- gbScaleStandardModeNV = index | SCALE_STD_MODE_MOTION_DETECT;
		gbScaleStandardModeNV &= ~SCALE_STD_MODE_MASK;						// clear scale standard mode
		gbScaleStandardModeNV |= (index | SCALE_STD_MODE_MOTION_DETECT);	// set standard mode and motion detect enabled and preserved other SCALE_STD_MODE_ bits.
		// 2012-02-28 -WFC- ^
		// 2016-03-21 -WFC- removed enabled zero on powerup for OIML
//		if ( SCALE_STD_MODE_OIML == index )
//			gbScaleStandardModeNV |= SCALE_STD_MODE_ZERO_POWERUP;
		if ( SCALE_STD_MODE_NTEP == index || SCALE_STD_MODE_OIML == index )
			gabMotionDetectBand_dNV[ PANEL_SETUP_LOADCELL_NUM ] = 1;		// Legal for Trade mode Motion Detection band is 1 d.
		else
			gabMotionDetectBand_dNV[ PANEL_SETUP_LOADCELL_NUM ] = 3;		// Standard mode Motion Detection band is 3 d.
	}
	else { // map configuration content to menu selection index.
		index =  gbScaleStandardModeNV;
		index &= SCALE_STD_MODE_MASK;								// get scale standard mode
	}
	return index;
} // end panel_setup_std_mode_mapper(,,)

/* 2012-02-22 -WFC- commented out and rewrote in the above.
BYTE	panel_setup_std_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		// 2011-09-01	-WFC- v
//		gbScaleStandardModeNV &= ~SCALE_STD_MODE_MASK;				// clear scale standard mode bits field
//		gbScaleStandardModeNV |= pSetupObj-> selectIndex;			// set new scale standard mode.
		// default loadcell software filter to LO. User can changed filter level later from filter menu.
		gabSensorFeaturesFNV[ PANEL_SETUP_LOADCELL_NUM ] &= ~SENSOR_CNFG_FILTER_LEVEL_MASK;	// clear filter bits
		gabSensorFeaturesFNV[ PANEL_SETUP_LOADCELL_NUM ] |= SENSOR_CNFG_FILTER_LEVEL_LOW;	// set loadcell software filter LO.
		gabSensorSpeedFNV[ PANEL_SETUP_LOADCELL_NUM ] = 8;		// Linear Tech ADC, 27.75 HZ sample rate for low filter setting.
		index = pSetupObj-> selectIndex;
//		switch ( index )	{
//			case SCALE_STD_MODE_NTEP:
//				gbScaleStandardModeNV = index | (SCALE_STD_MODE_MOTION_DETECT | SCALE_STD_MODE_AZM );
//				break;
//			case SCALE_STD_MODE_OIML:
//				gbScaleStandardModeNV = index | (SCALE_STD_MODE_MOTION_DETECT | SCALE_STD_MODE_AZM | SCALE_STD_MODE_ZERO_POWERUP );
//				break;
//			default:
//				gbScaleStandardModeNV = index | (SCALE_STD_MODE_MOTION_DETECT | SCALE_STD_MODE_AZM );
//
//		}
		gbScaleStandardModeNV = index | (SCALE_STD_MODE_MOTION_DETECT | SCALE_STD_MODE_AZM );
		if ( SCALE_STD_MODE_OIML == index )
			gbScaleStandardModeNV |= SCALE_STD_MODE_ZERO_POWERUP;
		// 2011-09-01	-WFC- ^
		// 2011-10-25	-WFC- V
		if ( SCALE_STD_MODE_NTEP == index || SCALE_STD_MODE_OIML == index )
			gabMotionDetectBand_dNV[ PANEL_SETUP_LOADCELL_NUM ] = 1;		// Legal for Trade mode Motion Detection band is 1 d.
		else
			gabMotionDetectBand_dNV[ PANEL_SETUP_LOADCELL_NUM ] = 3;		// Standard mode Motion Detection band is 3 d.
		// 2011-10-25	-WFC- ^
	}
	else { // map configuration content to menu selection index.
		index =  gbScaleStandardModeNV;
		index &= SCALE_STD_MODE_MASK;								// get scale standard mode
	}

	return index;
} // end panel_setup_std_mode_mapper(,,)
*/

/**
 * Method to map between azm variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 *
 * @note Challenger3 and other stand alone scale only handle 1 loadcell and it is loadcell 0.
 *
 * History:  Created on 2009/08/04 by Wai Fai Chin
 */


BYTE	panel_setup_azm_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;
	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		index = pSetupObj-> selectIndex;
		if ( index )
			gbScaleStandardModeNV |= SCALE_STD_MODE_AZM;
		else
			gbScaleStandardModeNV &= ~SCALE_STD_MODE_AZM;
	}
	else { // map configuration content to menu selection index.
		if (  SCALE_STD_MODE_AZM & gbScaleStandardModeNV )
			index =  1;
		else
			index =  0;
	}

	return index;
} // end panel_setup_azm_mapper(,,)

///
/**
 * Method to map between zero on power up (zop) variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 *
 * @note Challenger3 and other stand alone scale only handle 1 loadcell and it is loadcell 0.
 *
 * History:  Created on 2016-03-21 by Wai Fai Chin
 */


BYTE	panel_setup_zop_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;
	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		index = pSetupObj-> selectIndex;
		if ( index )
			gbScaleStandardModeNV |= SCALE_STD_MODE_ZERO_POWERUP;
		else
			gbScaleStandardModeNV &= ~SCALE_STD_MODE_ZERO_POWERUP;
	}
	else { // map configuration content to menu selection index.
		if (  SCALE_STD_MODE_ZERO_POWERUP & gbScaleStandardModeNV )
			index =  1;
		else
			index =  0;
	}

	return index;
} // end panel_setup_zop_mapper(,,)


/**
 * Method to map between loadcell unit setting variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2009/07/23 by Wai Fai Chin
 * 2011-04-29 -WFC- removed gbLbFlag because of duplication and caused a bug that set LB annunciator on when switch to Kg.
 */


BYTE	panel_setup_cal_unit_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;
	BYTE unit;
    BYTE anncState;
	BYTE			status;
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;

    anncState = LED_SEG_STATE_KEEP_BLINKING;						// 2011-07-19 -WFC-
	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;

	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;		// Default behaviors was set to flash by the panel_setup_simple_sub_menu_thread(), this method override it to normal display.
	status = NO;
	index = pSetupObj-> selectIndex;
	errorStatus = CAL_ERROR_NONE;
	if ( sensor_get_cal_base( PANEL_SETUP_LOADCELL_NUM, &pSensorCal ) ) {		// get sensor 0 calibration pointer.
		if (  pSensorCal-> status >= CAL_STATUS_GOT_UNIT_CAP &&  pSensorCal-> status < CAL_STATUS_COMPLETED ) 
			status = YES;
		else {
			pSetupObj-> nextMove		= 0;						// flag as an error and tell the host thread to exit.
			pSetupObj-> msgID 			= MENU_MSG_ID_NO;			// display "No" for 3 seconds to tell user that is NOT ALLOW to change.
			pSetupObj-> msgDisplayTime	= TT_3SEC;
			actionMode = PANEL_MENU_ACTION_MODE_GET_CNFG_INDEX;		// ensure it will not change unit settings.
			errorStatus = CAL_ERROR_CANNOT_CHANGE_UNIT;
		}

		if ( YES == status ) {
			if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
				gabSensorShowCBunitsFNV[ PANEL_SETUP_LOADCELL_NUM ] = 
				gabCalCB_unit[ PANEL_SETUP_LOADCELL_NUM  ]	= 
				pSensorCal-> countby.unit	= pSetupObj-> selectIndex;			// set new unit for loadcell 0.
				gabCalOpStep[ PANEL_SETUP_LOADCELL_NUM ] = 						// set cal status to cal op status for user command interface.
				pSensorCal-> status = CAL_STATUS_GOT_UNIT_CAP;					// next step is to get cap setting.
			    anncState = LED_SEG_STATE_STEADY;								// 2011-07-19 -WFC-
			}
			else if ( PANEL_MENU_ACTION_MODE_GET_CNFG_INDEX == actionMode ) {
				// map configuration content to menu selection index.
				index =  gabSensorShowCBunitsFNV[ PANEL_SETUP_LOADCELL_NUM ];
				pSensorCal-> countby.unit	=									// use as default unit in case user never press enter key.
				gabCalCB_unit[ PANEL_SETUP_LOADCELL_NUM  ]	= index;
				gabCalOpStep[ PANEL_SETUP_LOADCELL_NUM ] = 					// set cal status to cal op status for user command interface.
				pSensorCal-> status = CAL_STATUS_GOT_UNIT_CAP;					// next step is to get cap setting.
			}
		}
		gabCalErrorStatus[ PANEL_SETUP_LOADCELL_NUM ] = errorStatus;				// set cal status to cal op status for user command interface.
	} // end if ( sensor_get_cal_base( PANEL_SETUP_LOADCELL_NUM, &pSensorCal ) ) {}

	if ( SENSOR_UNIT_KG == index ) {
		//led_display_kg_blink();
		led_display_kg_on();
		led_display_lb_direct_off();			//	PHJ
		// 2011-04-29 -WFC- gbLbFlag = OFF;							//	PHJ
	}
	else if ( SENSOR_UNIT_LB == index )  {
		//led_display_lb_blink();
		led_display_lb_direct_on();				//	PHJ
		// 2011-04-29 -WFC- gbLbFlag = ON;							//	PHJ
		led_display_kg_off();
	}
	else {
		index = 0;
		// unit is not lb or kg, turn them off.
		led_display_lb_direct_off();			//	PHJ
		// 2011-04-29 -WFC- gbLbFlag = OFF;							//	PHJ
		led_display_kg_off();
	}

	return index;
} // end panel_setup_cal_unit_mapper(,,)



/**
 * Method to map float value into capacity variable.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009-07-23 by Wai Fai Chin
 * 2011-04-29 -WFC- rewrote it to prevent user enter small or 0 capacity value.
 * 2011-05-02 -WFC- turn off x1000 annunciator when encounter error.
 */

BYTE	panel_setup_capacity_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV )
{
	float			*pfV;
	SENSOR_CAL_T	*pSensorCal;


	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;

	if ( sensor_get_cal_base( PANEL_SETUP_LOADCELL_NUM, &pSensorCal ) ) {		// get sensor 0 calibration pointer.
//		if (  pSensorCal-> status >= CAL_STATUS_GOT_UNIT_CAP &&  pSensorCal-> status < CAL_STATUS_COMPLETED )
//			status = YES;
//		else {
//			pSetupObj-> nextMove		= 0;					// flag as an error and tell the host thread to exit.
//			pSetupObj-> msgID 			= MENU_MSG_ID_NO;		// display "No" for 3 seconds to tell user that is NOT ALLOW to change.
//			pSetupObj-> msgDisplayTime	= TT_3SEC;
//			actionMode = PANEL_MENU_ACTION_MODE_GET_VALUE;		// ensure it will not change unit settings.
//			gabCalErrorStatus[ PANEL_SETUP_LOADCELL_NUM ] = CAL_ERROR_CANNOT_CHANGE_CAPACITY;		// set cal status to cal op status for user command interface.
//		}

		pfV = pV;

		if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) {
			if ( float_a_gte_b( *pfV, 0.5f) ) {
				pSensorCal-> capacity = gafCal_capacity[ PANEL_SETUP_LOADCELL_NUM ] = *pfV;
				gabCalOpStep[ PANEL_SETUP_LOADCELL_NUM ] = 			// set cal status to cal op status for user command interface.
				pSensorCal-> status = CAL_STATUS_GET_COUNTBY;			// next step is to get countby
			}
			else {
				pSetupObj-> msgID = MENU_MSG_ID_Littl;		// display "Littl"
				pSetupObj-> msgDisplayTime	= TT_2SEC;		// display the above message for about 2 seconds.
				gabCalErrorStatus[ PANEL_SETUP_LOADCELL_NUM ] = CAL_ERROR_INVALID_CAPACITY;
			}
			// 2011-05-02 -WFC- v
			#ifdef  LED_DISPLAY_ANC_X1000
				led_display_set_annunciator( LED_DISPLAY_ANC_X1000, LED_SEG_STATE_OFF );
			#endif
			// 2011-05-02 -WFC- ^
		}
		else if ( PANEL_MENU_ACTION_MODE_GET_DEFAULT_VALUE == actionMode || PANEL_MENU_ACTION_MODE_SET_DEFAULT_VALUE == actionMode ) {
			*pfV = pSensorCal-> capacity = gafCal_capacity[ PANEL_SETUP_LOADCELL_NUM ];
			if ( PANEL_MENU_ACTION_MODE_SET_DEFAULT_VALUE == actionMode  )
				pSensorCal-> status = CAL_STATUS_GET_COUNTBY;		// next step is to get countby
		}
		else {
			*pfV = pSensorCal-> capacity;
		}
	} // end if ( sensor_get_cal_base( PANEL_SETUP_LOADCELL_NUM, &pSensorCal ) ) {}

	return pSetupObj-> selectIndex;


} // end panel_setup_capacity_mapper(,,)


/**
 * Method to handle countby selection by user.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- io generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009/07/23 by Wai Fai Chin
 */


BYTE	panel_setup_countby_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV )
{
	BYTE			n;
	BYTE			b;
	BYTE			errorStatus;
	BYTE			bStdCBIndex;
	UINT32			u32Tmp;
	BYTE			*pStr;
	BYTE			*pMsg;
	MSI_CB			wantedCB;
	SENSOR_CAL_T	*pSensorCal;

	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;

	pStr = pV;
	n = PANEL_SETUP_LOADCELL_NUM;
	errorStatus = CAL_ERROR_NONE ;	// assumed no error.
	
	copy_until_match_char_P(  pStr,	gcStr_No, 0, PANEL_SETUP_LOCAL_MAX_STRING_SIZE );	// default output msg "No"
	
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {				// if n is a valid channel number.
		if (  pSensorCal-> status >= CAL_STATUS_GET_COUNTBY &&  pSensorCal-> status < CAL_STATUS_COMPLETED ) {
			u32Tmp = (UINT32) pSensorCal-> capacity;
			if ( u32Tmp > 1 )	{			// ensured capacity > 1
				b = cal_gen_countby_table( pSensorCal-> capacity, &bStdCBIndex, &(pSetupObj-> selectIndex), &wantedCB );
				pSetupObj-> maxNumOfItem &= PANEL_MENU_ITEM_TYPE_MASK;			// persevere menu item type.
				pSetupObj-> maxNumOfItem |= b;
				if ( PANEL_MENU_ACTION_MODE_SET_CHOICE == actionMode ) {
					//pSetupObj-> maxNumOfItem = cal_gen_countby_table( pSensorCal-> capacity, &bStdCBIndex, &(pSetupObj-> selectIndex), &wantedCB );
					//pSensorCal->countby = wantedCB;
					pSensorCal->countby.fValue = wantedCB.fValue;
					pSensorCal->countby.iValue = wantedCB.iValue;
					pSensorCal->countby.decPt  = wantedCB.decPt;
					gabSensorShowCBunitsFNV[n]	= pSensorCal->countby.unit;
					gabSensorShowCBdecPtFNV[n]	= pSensorCal->countby.decPt;
					gafSensorShowCBFNV[n]		= pSensorCal->countby.fValue;
					pSensorCal-> status = CAL_STATUS_GOT_COUNTBY;
					gabCalOpStep[ n ] = pSensorCal-> status;			// set cal status to cal op status for user command interface.
				}
				//else if ( PANEL_MENU_ACTION_MODE_SHOW_CHOICE == actionMode ) {
				//}
				else if ( PANEL_MENU_ACTION_MODE_GET_DEFAULT_CHOICE == actionMode ) {
					pSetupObj-> selectIndex = bStdCBIndex;
					cal_gen_countby_table( pSensorCal-> capacity, &bStdCBIndex, &(pSetupObj-> selectIndex), &wantedCB );					
				}
//				led_display_format_float_string( wantedCB.fValue, &wantedCB, TRUE, pStr );
				led_display_format_float_string( wantedCB.fValue, &wantedCB, FALSE, pStr );	//	PHJ
				led_display_format_output( pStr );
				
			} // end if ( u32Tmp > 1 )	{}
			else
				errorStatus = CAL_ERROR_INVALID_CAPACITY;	// cannot update because capacity <= 1;
		} // end if (  pSensorCal-> status >= CAL_STATUS_GOT_UNIT_CAP &&  pSensorCal->status < CAL_STATUS_COMPLETED ) {}
		else {
			errorStatus = CAL_ERROR_CANNOT_CHANGE_COUNTBY;
		}
	}// end if ( sensor_get_cal_base(,)) {}
	else {
		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
	}

	if ( CAL_ERROR_NONE != errorStatus ) {
		pSetupObj-> nextMove		= 0;					// flag as an error and tell the host thread to exit.
		pSetupObj-> msgID 			= MENU_MSG_ID_NO;		// display "No" for 3 seconds to tell user that is NOT ALLOW to change.
		pSetupObj-> msgDisplayTime	= TT_3SEC;
	}
	gabCalErrorStatus[ n ] = errorStatus;				// set cal status to cal op status for user command interface.

	return pSetupObj-> selectIndex;

} // end panel_setup_countby_mapper(,,)


/**
 * Method to  calibrate a zero weight cal point.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009/07/23 by Wai Fai Chin
 */


BYTE	panel_setup_cal_zero_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV )
{
	BYTE			n;
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;
	
	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;
	
	n = PANEL_SETUP_LOADCELL_NUM;
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {		// if n is a valid channel number.
		// handle different type of sensor here:
		// the following logic is to handle loadcell type.
		errorStatus = cal_zero_point( n, pSensorCal );
		gabCalOpStep[ n ] = pSensorCal-> status;			// set cal status to cal op status for user command interface.
	}// end if ( sensor_get_cal_base(,)) {}
	else {
		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
	}

	if ( CAL_ERROR_NONE != errorStatus ) {		// if it has error.
		if ( pSensorCal-> status > 0 &&  pSensorCal-> status < CAL_STATUS_COMPLETED ) { // if it had already zero cal point
			pSetupObj-> msgID = MENU_MSG_ID_NO;		// display "No" 
		}
		else {
			pSetupObj-> msgID = MENU_MSG_ID_FAIL;		// display "FAIL" 
		}
	}
	else {
		pSetupObj-> msgID = MENU_MSG_ID_PASS;			// display "PASS" 
	
	}

	pSetupObj-> msgDisplayTime	= TT_2SEC;				// display the above message for about 2 seconds.
	pSetupObj-> nextMove		= 0;					// flag to tell the host thread to exit.
	gabCalErrorStatus[ n ] = errorStatus;				// set cal status to cal op status for user command interface.
	
	return 0;											// return 0 so it index into "0" during the 1st time to display list msg.
} // end panel_setup_cal_zero_mapper(,,)


/**
 * Method to  calibrate a zero weight cal point in a new rcal calibration.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009/10/07 by Wai Fai Chin
 */

BYTE	panel_setup_new_rcal_zero_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	
	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;
	
	panel_setup_cal_zero_mapper( actionMode, pSetupObj, pV );
	
	return 0;											// return 0 so it index into "0" during the 1st time to display list msg.
} // end panel_setup_new_rcal_zero_mapper(,,)


/**
 * Method to  calibrate a zero weight cal point in Rcal.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009/09/30 by Wai Fai Chin
 * 2011-04-22 -WFC- rewrote because new Rcal system.
 */


BYTE	panel_setup_rcal_zero_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{

	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;

	if ( PANEL_MENU_ACTION_MODE_GET_CNFG_INDEX == actionMode ) {
		pSetupObj-> nextMove	= PSSMC_NEXTMOVE_SPECIAL_EXIT;			// flag it as a special case to exit current thread, so it can re-enter here with PANEL_MENU_ACTION_MODE_EXIT_STEP 1 and 2.
	}
	else if ( PANEL_MENU_ACTION_MODE_EXIT_STEP_2 == actionMode ) {
		panel_setup_cal_zero_mapper( actionMode, pSetupObj , pV );
	}
	return 0;														// return 0 so it index into "0" during the 1st time to display list msg.
} // end panel_setup_rcal_zero_mapper(,,)



/**
 * Method to handle calibrate a load of a loadcell.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009-07-23 by Wai Fai Chin
 * 2011-05-02 -WFC- turn off x1000 annunciator when encounter error.
 */

#define PANEL_MIN_TEST_LOAD_PCT_CAPACITY		0.005f

BYTE	panel_setup_cal_load_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV )
{
	BYTE			n;
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;
	LOADCELL_T 		*pLc;			// points to a loadcell
	float			*pfV;
	
	pfV = pV;
	
	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;
	
	errorStatus = CAL_ERROR_NONE;									// assumed passed.
	n = PANEL_SETUP_LOADCELL_NUM;									// hardcode for now... since Challenger3 only handle 1 loadcell, number 0.
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {				// if n is a valid channel number.
		if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) { 
			if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] )	{
				if ( float_a_gte_b( *pfV,( pSensorCal-> capacity * PANEL_MIN_TEST_LOAD_PCT_CAPACITY )) ) {
					gafCalValue[n] = *pfV;
					errorStatus = cal_build_table( gaLSensorDescriptor[n].curADCcount, *pfV, pSensorCal);
				}
				else {
					errorStatus = CAL_ERROR_TEST_LOAD_TOO_SMALL;
				}
			} // end if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {}
			gabCalOpStep[ n ] 		= pSensorCal-> status;				// set cal status to cal op status for user command interface.
		} //
		else {
			if ( 0 == pSensorCal-> status ) {
				*pfV = pSensorCal-> capacity;
			} 
			else if ( (pSensorCal-> status > 0) && (pSensorCal-> status <= MAX_CAL_POINTS) ) { // if it has at least 2 valid cal points
				pLc = &gaLoadcell[ n ];
				if ( LC_STATUS_GOT_CAL_WEIGHT & (pLc-> status) ) {
					*pfV = pLc-> rawWt;
				}
			}
			
			if ( PANEL_MENU_ACTION_MODE_SET_DEFAULT_VALUE == actionMode ) {
				if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] )	{
					if ( float_a_gte_b( *pfV,( pSensorCal-> capacity * PANEL_MIN_TEST_LOAD_PCT_CAPACITY )) ) {
						gafCalValue[n] = *pfV;
						errorStatus = cal_build_table( gaLSensorDescriptor[n].curADCcount, *pfV, pSensorCal);
					}
					else {
						errorStatus = CAL_ERROR_TEST_LOAD_TOO_SMALL;
					}
				} // end if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {}
				gabCalOpStep[ n ] = pSensorCal-> status;				// set cal status to cal op status for user command interface.
			}
		}
	} // end if ( sensor_get_cal_base( channel, &pSensorCal ) )  {}
	else {
		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
	}
	
	if ( CAL_ERROR_NONE != errorStatus ) {		// if it has error.
		if ( CAL_ERROR_TEST_LOAD_TOO_SMALL == errorStatus ) {
			pSetupObj-> msgID = MENU_MSG_ID_Littl;		// display "Littl" 
		}
		else if ( pSensorCal-> status > MAX_CAL_POINTS ) { // if it has no more cal point memory left.
			pSetupObj-> msgID = MENU_MSG_ID_NO;		// display "No" 
		}
		else {
			pSetupObj-> msgID = MENU_MSG_ID_FAIL;		// display "FAIL" 
		}
		pSetupObj-> msgDisplayTime	= TT_3SEC;			// display the above message for about 3 seconds.
		pSetupObj-> nextMove  = 0;						// flag to display and exit current thread.
		// 2011-05-02 -WFC- v
		#ifdef  LED_DISPLAY_ANC_X1000
			led_display_set_annunciator( LED_DISPLAY_ANC_X1000, LED_SEG_STATE_OFF );
		#endif
		// 2011-05-02 -WFC- ^
	}
	else {
		if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ||
		     PANEL_MENU_ACTION_MODE_SET_DEFAULT_VALUE == actionMode  ) { 
			pSetupObj-> msgID = MENU_MSG_ID_PASS;				// display "PASS" 
			pSetupObj-> msgDisplayTime	= TT_2SEC;				// display the above message for about 2 seconds.
		}
	}

	gabCalErrorStatus[ n ]	= errorStatus;						// set cal status to cal op status for user command interface.
	return 0;
	
} // end panel_setup_cal_load_mapper(;;)


/**
 * Method to handle resistor calibration of a loaded loadcell.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009-09-30 by Wai Fai Chin
 * 2011-04-22 -WFC- rewrote to use ADC count at value 10% of capacity as Rcal to set a span point.
 * 2011-05-02 -WFC- turn off x1000 annunciator when encounter error.
 */

/// 10% of
#define PANEL_SETUP_CONSTANT_CAL_PCT_CAPACITY	0.1f

BYTE	panel_setup_rcal_load_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV )
{
	BYTE			n;
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;
	float			*pfV;
	INT32			count;
	float			fV;
	
	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;
	
	pfV = pV;
	
	errorStatus = CAL_ERROR_NONE;									// assumed passed.
	n = PANEL_SETUP_LOADCELL_NUM;									// hardcode for now... since Challenger3 only handle 1 loadcell, number 0.
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {				// if n is a valid channel number.
		if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) { 
			if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] )	{
				fV = pSensorCal->capacity * PANEL_SETUP_CONSTANT_CAL_PCT_CAPACITY;		// weight value at 10% of Capacity
				if ( float_a_lte_b( *pfV, fV )) {
					errorStatus = CAL_ERROR_TEST_LOAD_TOO_SMALL;
				}
				else {
					gafCalValue[n] = fV;
					count = pSensorCal-> adcCnt[0] + (INT32) (*pfV);
					errorStatus = cal_build_table( count, fV, pSensorCal);
				}	
			} // end if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {}
			gabCalOpStep[ n ] 		= pSensorCal-> status;				// set cal status to cal op status for user command interface.
		} //
		else {
			if ( ((pSensorCal-> status > 0) && (pSensorCal-> status <= MAX_CAL_POINTS)) ||	// if it has a 0 and at least 1 span point OR
				(CAL_STATUS_COMPLETED == pSensorCal-> status)) { 							// have a completed cal table.
				*pfV  = (float) get_constant_cal( &(pSensorCal-> adcCnt[0]), &(pSensorCal-> value[0]), pSensorCal-> capacity );
			}
			else
				*pfV = 1.0f;

			if ( PANEL_MENU_ACTION_MODE_SET_DEFAULT_VALUE == actionMode ) {
				if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] )	{
					fV = pSensorCal->capacity * PANEL_SETUP_CONSTANT_CAL_PCT_CAPACITY;		// weight value at 10% of Capacity
					if ( float_a_lte_b( *pfV, fV )) {
						errorStatus = CAL_ERROR_TEST_LOAD_TOO_SMALL;
					}
					else {
						gafCalValue[n] = fV;
						count = pSensorCal-> adcCnt[0] + (INT32) (*pfV);
						errorStatus = cal_build_table( count, fV, pSensorCal);
					}
				} // end if (  SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {}
				gabCalOpStep[ n ] = pSensorCal-> status;				// set cal status to cal op status for user command interface.
			}
		}
	} // end if ( sensor_get_cal_base( channel, &pSensorCal ) )  {}
	else {
		errorStatus = CAL_ERROR_WRONG_SENSOR_ID;
	}
	
	if ( CAL_ERROR_NONE != errorStatus ) {		// if it has error.
		if ( CAL_ERROR_TEST_LOAD_TOO_SMALL == errorStatus ) {
			pSetupObj-> msgID = MENU_MSG_ID_Littl;		// display "Littl" 
		}
		else if ( pSensorCal-> status > MAX_CAL_POINTS ) { // if it has no more cal point memory left.
			pSetupObj-> msgID = MENU_MSG_ID_NO;		// display "No" 
		}
		else {
			pSetupObj-> msgID = MENU_MSG_ID_FAIL;		// display "FAIL" 
		}
		pSetupObj-> msgDisplayTime	= TT_3SEC;			// display the above message for about 3 seconds.
		pSetupObj-> nextMove  = 0;						// flag to display and exit current thread.
		// 2011-05-02 -WFC- v
		#ifdef  LED_DISPLAY_ANC_X1000
			led_display_set_annunciator( LED_DISPLAY_ANC_X1000, LED_SEG_STATE_OFF );
		#endif
		// 2011-05-02 -WFC- ^
	}
	else {
		if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ||
		     PANEL_MENU_ACTION_MODE_SET_DEFAULT_VALUE == actionMode  ) { 
			pSetupObj-> msgID = MENU_MSG_ID_PASS;				// display "PASS" 
			pSetupObj-> msgDisplayTime	= TT_2SEC;				// display the above message for about 2 seconds.
			// gbPanelSetupStatus = PANEL_SETUP_STATUS_EXIT_SAVE;	// tell parent thread to exit.
		}
	}

	gabCalErrorStatus[ n ]	= errorStatus;						// set cal status to cal op status for user command interface.
	return 0;
} // end panel_setup_rcal_load_mapper(;;)

/**
 * Method to show Rcal value of a loadcell.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2009-10-01 by Wai Fai Chin
 * 2011-04-22 -WFC- simplfied it with panel_setup_show_rcal().
 * 2011-07-18 -WFC- changed nextMove from auto timeout exit to wait for key press exit.
 */

BYTE	panel_setup_show_rcal_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj , void *pV )
{

	if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode )
		return 0;

	if ( PANEL_MENU_ACTION_MODE_GET_CNFG_INDEX == actionMode ) {
		// 2011-07-18 -WFC- pSetupObj-> nextMove	= PSSMC_NEXTMOVE_DISPLAY_FLOAT_EXIT;	// flag it as a special case to exit current thread, so it can re-enter here with PANEL_MENU_ACTION_MODE_EXIT_STEP_1,
		pSetupObj-> nextMove	= PSSMC_NEXTMOVE_DISPLAY_FLOAT_KEY_EXIT;					// 2011-07-18 -WFC-  flag it as a special case to display a value, so it can re-enter here with PANEL_MENU_ACTION_MODE_EXIT_STEP_1,
	}
	else if ( PANEL_MENU_ACTION_MODE_SHOW_VALUE == actionMode ) {
		panel_setup_show_rcal( PANEL_SETUP_LOADCELL_NUM );
	}
	else if ( PANEL_MENU_ACTION_MODE_EXIT_STEP_1 == actionMode ) {
		panel_setup_show_rcal( PANEL_SETUP_LOADCELL_NUM );
		gbPanelSetupStatus = PANEL_SETUP_STATUS_EXIT_SAVE;				// tell parent thread to exit.
	}
	return 0;														// return 0 so it index into "0" during the 1st time to display list msg.
} // end panel_setup_show_rcal_mapper(;;)


/**
 * Method to show Rcal value of a loadcell.
 *
 * @param  sn	-- sensor number.
 *
 * @return none.
 *
 * History:  Created on 2011-04-08 by Wai Fai Chin
 * 2011-04-28 -WFC- use constant cal at value 10% of capacity as Rcal value in ADC count instead of weight value.
 */

void	panel_setup_show_rcal( BYTE sn )
{
	SENSOR_CAL_T	*pSensorCal;
	INT32			cal10pctCap_ADCcnt;

	if ( sensor_get_cal_base( sn, &pSensorCal ) ) {				// if sn is a valid channel number.
		if ( ((pSensorCal-> status > 0) && (pSensorCal-> status <= MAX_CAL_POINTS)) ||	// if it has at least 2 valid cal points OR
			(CAL_STATUS_COMPLETED == pSensorCal-> status)) { 						// have a completed cal table.
			cal10pctCap_ADCcnt = get_constant_cal( &(pSensorCal-> adcCnt[0]), &(pSensorCal-> value[0]), pSensorCal-> capacity );
		}
		panel_main_display_float_in_integer( (float) cal10pctCap_ADCcnt );
	}

} // end panel_setup_show_rcal()


/**
 * Method to map between loadcell unit setting variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2009-07-20 by Wai Fai Chin
 * 2011-04-19 -WFC- calling main_system_master_default_system_configuration() instead of 3 functions calls.
 */


BYTE	panel_setup_master_reset_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;

	index = pSetupObj-> selectIndex;
	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		if ( 1 == index ) {
			// 2011-04-19 -WFC- v
//			nvmem_default_all_loadcell_statistics_fram();		// default it to zero.
//			main_system_1st_init();
//			nvmem_save_all_config();
			main_system_master_default_system_configuration();
			// 2011-04-19 -WFC- ^
			main_system_normal_init();
			panel_setup_construct_cal_menu_object();
			gbPanelSetupStatus = PANEL_SETUP_STATUS_EXIT_NOW;	// tell this thread to exit NOW.
		}
		else {
			gbPanelSetupStatus = PANEL_SETUP_STATUS_CONTINUE;	
			index = pSetupObj-> selectIndex = 1;				// advance to next item. In this case it asks "SurE?"
		}
	}
	else if ( PANEL_MENU_ACTION_MODE_SHOW_VALUE == actionMode ) {
		if ( 0 == index )  {
			panel_setup_construct_cal_menu_object();
			gbPanelSetupStatus = PANEL_SETUP_STATUS_EXIT_NOW;	// tell this thread to exit NOW.
		}
	}
	else if ( PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW == actionMode ) {
			panel_setup_construct_cal_menu_object();
	}
	
	return index;
} // end panel_setup_master_reset_mapper(,,)



/**
 * Post process of exit setup menu and save configuration settings if need.
 *
 * @param needToSave -- need to save configuration.
 *
 * History:  Created on 2009/07/22 by Wai Fai Chin
 * 2011-05-12 -WFC- called timer_reset_power_off_timer() and set LED intensity based on its settings.
 * 2011-06-15 -WFC- called 	self_test_set_event_timers_interval();	self_test_timer_reset_event_timers() before exit.
 *            timer_reset_power_off_timer() is removed so timer.c module is independent module.
 * 2012-02-08 -WFC- Need to save all configuration after calibration because we put a setup menu along with cal menu group.
 * 2012-02-27 -WFC- Initialized sensor related init() after calibration menu, so it can take effect immediately without cycle power.
 * 2012-04-30 -WFC- Called rf_config_thread_runner();
 * 2014-10-03 -WFC- save scale standard novram variable because it contains power saving mode bits.
 * 2016-03-21 -WFC- called lc_zero_init_all_lc_zero_config().
 */

void	panel_setup_exit( BYTE needToSave )
{
	BYTE			n;
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;

	errorStatus = CAL_ERROR_NONE;									// assumed passed.
	
	if ( gbPanelSetupStatus & PANEL_SETUP_STATUS_EXIT_SAVE)
		needToSave = TRUE;
		
	if ( PANEL_RUN_MODE_CAL_LOADCELL == gbPanelMainRunMode ) {
		n = PANEL_SETUP_LOADCELL_NUM;
		if ( sensor_get_cal_base( n, &pSensorCal ) ) {			// if n is a valid channel number,then it has a valid pointer to cal data structure.
			if ( needToSave ) {
				errorStatus = panel_setup_save_cal_talbe(n);
				errorStatus = nv_cnfg_eemem_save_scale_standard();
				errorStatus  = nvmem_save_all_essential_config_fram();		// 2012-02-08 -WFC- Need to save all configuration after calibration because we put a setup menu along with cal menu group.
				errorStatus |= nvmem_save_all_loadcell_statistics_fram();	// 2012-02-08 -WFC-
				// 2012-02-27 -WFC- v Initialized it so it can take effect immediately without cycle power.
				sensor_init_all();
				adc_lt_init();
				adc_cpu_1st_init();
				setpoint_init_all();
				// 2012-02-27 -WFC- ^
			} // end if ( needToSave ) {}
			else { // abort cal.
				//if ( pSensorCal-> status < CAL_STATUS_COMPLETED ) {	// if it is in calibration mode,
					gabCalOpStep[ n ] 		= pSensorCal-> status;		// set cal status to cal op status for user command interface.
					errorStatus  = nv_cnfg_eemem_recall_scale_standard();
					errorStatus |= cal_recall_a_sensor_cnfg( n );
					gabCalErrorStatus[ n ]	= errorStatus;				// set cal status to cal op status for user command interface.
				//}
			}
			if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[n] ) {
				gaLoadcell[n].runModes &= ~LC_RUN_MODE_IN_CAL;
			}
		}
	}// end if ( PANEL_RUN_MODE_CAL_LOADCELL == gbPanelMainRunMode ) {}
	else { // regular configuration setup.
		if ( needToSave ) {
			errorStatus  = nvmem_save_all_essential_config_fram();
			errorStatus |= nv_cnfg_eemem_save_scale_standard();				// 2014-10-03 -WFC-
			errorStatus |= nvmem_save_all_loadcell_statistics_fram();
		}
		else { // user cancel setup.
			errorStatus = nvmem_recall_all_config();
			errorStatus |= nvmem_recall_all_loadcell_statistics_fram();
		}
		sensor_init_all();
		adc_lt_init();
		adc_cpu_1st_init();
		setpoint_init_all();
		lc_zero_init_all_lc_zero_config();		// 2016-03-21 -WFC-
	}

	if ( errorStatus ) {			// if it has error.
		if ( needToSave ) {
			panel_setup_display_msg_duration_P( MENU_MSG_ID_FAIL, TT_2SEC );		// display "FAIL"
		}
		else {
			panel_setup_display_msg_duration_P( MENU_MSG_ID_Cancl, TT_2SEC );		// display "Cancl"
		}
	}
	else {	// no error
		if ( needToSave ) {
			panel_setup_display_msg_duration_P( MENU_MSG_ID_STORE, TT_2SEC );		// display "StorE"
		}
		else {
			panel_setup_display_msg_duration_P( MENU_MSG_ID_Cancl, TT_2SEC );		// display "Cancl"
		}
	}

	panel_main_update_unit_leds(gabSensorShowCBunitsFNV[ 0 ]);

	// 2011-05-12 -WFC- v
	self_test_set_event_timers_interval();	// 2011-06-15 -WFC- set event timer interval because user may have configured LED sleep and power off timer period.
	self_test_timer_reset_event_timers();	// 2011-06-15 -WFC- reset event timers so it would not trigger an expiration event that would execute LED sleep or power off etc..
#if CONFIG_4260IS
	led_display_set_intensity( ((1 << gtSystemFeatureFNV.ledIntensity ) - 1 ) ); // 2012-05-24 -DLM-
#else
	if ( gtSystemFeatureFNV.ledIntensity )						// turn up if NOT in auto intensity
		//led_display_set_intensity( ((1 << gtSystemFeatureFNV.ledIntensity ) - 2 ) ); // dimming LED based on user led intensity setting.
		led_display_set_intensity( ((1 << gtSystemFeatureFNV.ledIntensity ) - 1 ) ); // 2012-05-24 -DLM-
	else														// turn up if in auto
		led_display_set_intensity(((BYTE)(gaLSensorDescriptor[ SENSOR_NUM_LIGHT_SENSOR ].curADCcount >> 6)) );	// dimming LED based on light sensor ADC count.
#endif
	// 2011-05-12 -WFC- ^

	// 2012-04-30 -WFC- v 2012-06-20 -DLM-
	#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )
		gLedDisplayManager.descr.mode |= LED_DISPLAY_MODE_UPDATE_NOW;
		led_display_string( gLedDisplayManager.descr.str1 );	// display it now, not to wait for led_display_manage_string_thread.
		rf_config_thread_runner();
	#endif
	// 2012-04-30 -WFC- ^
	
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


} // end panel_setup_exit()


/**
 * It constructs cal menu thread object to handle calibration relation menu items.
 *
 *
 * History:  Created on 2009/10/05 by Wai Fai Chin
 */

void	panel_setup_construct_cal_menu_object( void )
{
	gbPanelMainRunMode = PANEL_RUN_MODE_CAL_LOADCELL;
	// construct gPanelSetupTopMenuObj into a thread.
	gPanelSetupTopMenuGroupObj.msgID			= MENU_MSG_ID_CAL;
	gPanelSetupTopMenuGroupObj.pMethod			= panel_setup_cal_menu_group_method;	// method to cordinate this group of top menu item.
	gPanelSetupTopMenuGroupObj.pRootItem		= gacPanel_Menu_Cal_Group_Item_Entry;	// points to top menu cal group.
	gPanelSetupTopMenuGroupObj.maxNumOfItem		= PANEL_TOP_MENU_CAL_GROUP_MAX_ITEMS;
	gPanelSetupTopMenuGroupObj.disabledItemFlags = 0;		// enabled all menu items. 2011-04-26 -WFC-
	PT_INIT( &gPanelSetupTopMenuGroupObj.m_pt );										// init panel setup top menu group thread.
}

/**
 * It constructs Re Cal menu thread object to handle calibration relation menu items.
 *
 *
 * History:  Created on 2011-04-26 by Wai Fai Chin
 */

void	panel_setup_construct_recal_menu_object( void )
{
	gbPanelMainRunMode = PANEL_RUN_MODE_CAL_LOADCELL;
	// construct gPanelSetupTopMenuObj into a thread.
	gPanelSetupTopMenuGroupObj.msgID			= MENU_MSG_ID_CAL;
	gPanelSetupTopMenuGroupObj.pMethod			= panel_setup_recal_menu_group_method;	// method to cordinate this group of top menu item.
	gPanelSetupTopMenuGroupObj.pRootItem		= gacPanel_Menu_ReCal_Group_Item_Entry;	// points to top menu cal group.
	gPanelSetupTopMenuGroupObj.maxNumOfItem		= PANEL_TOP_MENU_RECAL_GROUP_MAX_ITEMS;
	gPanelSetupTopMenuGroupObj.disabledItemFlags = 0;									// enabled all menu items.
	PT_INIT( &gPanelSetupTopMenuGroupObj.m_pt );										// init panel setup top menu group thread.
}


/**
 * It constructs normal setup configuration menu object.
 *
 * History:  Created on 2011-08-31 by Wai Fai Chin
 */

void	panel_setup_construct_cnfg_menu_object( void )
{
	BYTE bStdMode;

	gbPanelMainRunMode = PANEL_RUN_MODE_PANEL_SETUP_MENU;
	// construct gPanelSetupTopMenuObj into a thread.
	gPanelSetupTopMenuObj.msgID				=
	gPanelSetupTopMenuObj.msgDisplayTime	= 0;						// no msg to display at the beginning of the menu.
	gPanelSetupTopMenuObj.pMethod			= 0;						// no method.

	bStdMode = gbScaleStandardModeNV & SCALE_STD_MODE_MASK;
	if ( SCALE_STD_MODE_INDUSTRY == bStdMode || SCALE_STD_MODE_1UNIT == bStdMode ) {
		gPanelSetupTopMenuObj.pRootMenu			= gacPanel_Top_Menu_Cnfg;	// points to setup top menu.
		gPanelSetupTopMenuObj.maxNumOfItem		= PANEL_TOP_MENU_CNFG_MAX_ITEMS;
	}
	else {
		gPanelSetupTopMenuObj.pRootMenu			= gacPanel_Top_Menu_Cnfg_LFT;	// points to setup top menu for Legal For Trade mode.
		gPanelSetupTopMenuObj.maxNumOfItem		= PANEL_TOP_MENU_CNFG_LFT_MAX_ITEMS;
	}
	gPanelSetupTopMenuObj.parentIs 			= PSTMC_PARENT_NONE;		// no group parent, it is just a list of top menu item.
	gPanelSetupTopMenuObj.disabledItemFlags = 0;	// enabled all menu items. 2011-04-26 -WFC-
	PT_INIT( &gPanelSetupTopMenuObj.m_pt );								// init panel setup top menu thread.
} // end panel_setup_construct_cnfg_menu_object()


/**
 * It saves calibration table of the specified sensor.
 *
 * @param  n	-- sensor ID,
 *
 * @return 0 if no error, else it has error.
 *
 * History:  Created on 2009/10/05 by Wai Fai Chin
 */

BYTE	panel_setup_save_cal_talbe( BYTE n )
{
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;

	errorStatus = CAL_ERROR_NONE;								// assumed error
	if ( sensor_get_cal_base( n, &pSensorCal ) ) {			// if n is a valid channel number,then it has a valid pointer to cal data structure.
		if ( (pSensorCal-> status > 0) && (pSensorCal-> status <= MAX_CAL_POINTS) ) {
			errorStatus = cal_save_exit( n, pSensorCal );	
			gabCalErrorStatus[ n ]	= errorStatus;				// set cal status to cal op status for user command interface.
			gabCalOpStep[ n ] 		= pSensorCal-> status;		// set cal status to cal op status for user command interface.
		}
	}
	
	return errorStatus;
}


/**
 * Post process of exit setup menu and save configuration settings if need.
 *
 * @param  n -- sensor number.
 *
 * History:  Created on 2011-05-03 by Wai Fai Chin
 * 2011-09-01 -WFC- get a valid pointer to cal data structure.
 */

void	panel_setup_abort_cal( BYTE n )
{
	BYTE			errorStatus;
	SENSOR_CAL_T	*pSensorCal;

	sensor_get_cal_base( n, &pSensorCal );				// 2011-09-01 -WFC- get a valid pointer to cal data structure.
	gabCalOpStep[ n ]		= pSensorCal-> status;		// set cal status to cal op status for user command interface.
	errorStatus				= nv_cnfg_eemem_recall_scale_standard();
	errorStatus		   	   |= cal_recall_a_sensor_cnfg( n );
	gabCalErrorStatus[ n ]	= errorStatus;				// set cal status to cal op status for user command interface.
} // end panel_setup_abort_cal()


/**
 * Method to get and set listener number.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2014-07-15 by Wai Fai Chin
 */

BYTE	panel_setup_print_listener_id_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	float *pfV;

	pfV = pV;
	if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) {
		if ( *pfV > 3.1f ) {	// ensure less than 4.
			*pfV = 3.0f;
		}
		gbSetupListenerID = (BYTE)*pfV;
	}
	else {
		*pfV = (float) gbSetupListenerID;
	}

	return pSetupObj-> selectIndex;

} // end  panel_setup_print_listener_id_mapper(,,)


/**
 * Method to get and set output port of a given listener number.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 *
 * History:  Created on 2014-07-15 by Wai Fai Chin
 */


BYTE	panel_setup_print_output_port_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;

	if ( gbSetupListenerID >= MAX_NUM_STREAM_LISTENER )
		gbSetupListenerID = MAX_NUM_STREAM_LISTENER - 1;

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode )
		index = gabListenerStreamTypeFNV[ gbSetupListenerID ] = pSetupObj-> selectIndex;
	else {
		index = gabListenerStreamTypeFNV[ gbSetupListenerID ];
		if ( index > (MAX_NUM_STREAM_DRIVER - 1))
			index = MAX_NUM_STREAM_DRIVER - 1;
	}

	return index;
} // end  panel_setup_print_output_port_mapper(,,)



// 2012-06-28 -WFC- v 2012-07-03 -DLM-


/**
 * Method to get and set composite formatter string in integer form for Listener #0.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on input selectIndex;
 *
 * Note:  Formatter # start with 1. 0 number is not allow.
 *
 * History:  Created on 2011/06/30 by Wai Fai Chin
 * 2011-07-13 -WFC- if user entered a 0 value, it force it to 1.
 * 2014-07-15 -WFC- print string output interval of listener number instead of hardcode to listener number 0.
 */


BYTE	panel_setup_print_composite_string_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	float *pfV;
	UINT32 ul;

	if ( gbSetupListenerID >= MAX_NUM_STREAM_LISTENER )
		gbSetupListenerID = MAX_NUM_STREAM_LISTENER - 1;

	pfV = pV;

	if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) {
		ul = (UINT32)*pfV;
		if ( 0L == ul ) {		// if 0, forced it to 1 because Formatter # start with 1. 0 number is not allow. 2011-07-13 -WFC-
			*pfV = 1;
			ul = 1L;
		}
		gaulPrintStringCompositeFNV[ gbSetupListenerID ] = (UINT32)*pfV;
	}
	else {
		*pfV = (float) gaulPrintStringCompositeFNV[ gbSetupListenerID ];
	}

	return pSetupObj-> selectIndex;

} // end  panel_setup_print_composite_string_mapper(,,)

/**
 * Method to get and set print string output interval time in seconds in integer form for Listener #0.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on input selectIndex;
 *
 * History:  Created on 2011/06/30 by Wai Fai Chin
 * 2011-07-14 -WFC- if user entered a value > 16bit, force it to equal 65535.
 * 2014-07-15 -WFC- print string output interval of listener number instead of hardcode to listener number 0.
 */


BYTE	panel_setup_print_interval_value_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	float *pfV;

	if ( gbSetupListenerID >= MAX_NUM_STREAM_LISTENER )
		gbSetupListenerID = MAX_NUM_STREAM_LISTENER - 1;

	pfV = pV;
	if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) {
		gawPrintStringIntervalFNV[ gbSetupListenerID ] = (UINT16)*pfV;
		if ( *pfV > 65535.0f ) {	// if user entered a value > 16bit, force it to equal 65535. 2011-07-14 -WFC-
			*pfV = 65535;
		}
		gawPrintStringIntervalFNV[ gbSetupListenerID ] = (UINT16)*pfV;
	}
	else {
		*pfV = (float) gawPrintStringIntervalFNV[ gbSetupListenerID ];
	}

	return pSetupObj-> selectIndex;

} // end  panel_setup_print_interval_value_mapper(,,)


/**
 * Method to get and set print string output control mode for Listener #0.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 *  cmp logic and value mode. bit 7== 0=disabled, 1== enabled, bit 6 to 4 compare logic, 1== '<', 2== '>'; bit 3 to 0; value mode; 0 == net_gross, 1== gross, 2==total, 3==total count
 *
 * History:  Created on 2011/06/29 by Wai Fai Chin
 * 2014-07-15 -WFC- print string output interval of listener number instead of hardcode to listener number 0.
 */


BYTE	panel_setup_print_control_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;
	if ( gbSetupListenerID >= MAX_NUM_STREAM_LISTENER )
		gbSetupListenerID = MAX_NUM_STREAM_LISTENER - 1;

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode )
		index = gabPrintStringCtrlModeFNV[ gbSetupListenerID ] = pSetupObj-> selectIndex;
	else {
		index = gabPrintStringCtrlModeFNV[ gbSetupListenerID ];
		if ( index > (MENU_PRINT_CNTRL_MODE_MAX_ITEMS - 1))
			index = MENU_PRINT_CNTRL_MODE_MAX_ITEMS - 1;
	}
	return index;
} // end  panel_setup_print_control_mode_mapper(,,)


/**
 * It constructs communication port menu thread object to handle communication and print string related items.
 *
 * History:  Created on 2011/06/30 by Wai Fai Chin
 */

void	panel_setup_construct_com_port_menu_object( void )
{
	gbPanelMainRunMode = PANEL_RUN_MODE_COM_PORT_MENU;
	// construct gPanelSetupTopMenuObj into a thread.
	gPanelSetupTopMenuGroupObj.msgID			= MENU_MSG_ID_Print;
	gPanelSetupTopMenuGroupObj.pMethod			= 0;											// method to cordinate this group of top menu item.
	gPanelSetupTopMenuGroupObj.pRootItem		= gacPanel_Menu_Com_Port_Group_Item_Entry;		// points to top menu cal group.
	gPanelSetupTopMenuGroupObj.maxNumOfItem		= PANEL_TOP_MENU_COM_PORT_GROUP_MAX_ITEMS;
	gPanelSetupTopMenuGroupObj.disabledItemFlags = 0;		// enabled all menu items. 2011-04-26 -WFC-
	gbSetupListenerID = 0;					// init scan list id and listener ID 2015-09-25 -WFC-
	PT_INIT( &gPanelSetupTopMenuGroupObj.m_pt );												// init panel setup top menu group thread.
} // end panel_setup_construct_com_port_menu_object().

/**
 * It coordinates selection of communication port menu group.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu group object
 *
 * @return index of next group item dynamically based on the sensor cal status.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2009-09-29 by Wai Fai Chin
 * 2011-04-27 -WFC- Rewrote: If calibration had completed or it has 0 and a span point, then only allow to standard setup menu.
 */
/*
BYTE	panel_setup_print_string_menu_group_method( PANEL_SETUP_TOP_MENU_GROUP_CLASS *pSetupObj )
{
	return pSetupObj-> selectIndex;
} // end panel_setup_print_string_menu_group_method()
*/

/**
 * Check if print string menu item is allow to display.
 *
 * @param  pSetupObj	-- pointer to panel setup top menu object
 *
 * @return 0 == not allow. 1== allow.
 *
 * @note Challenger3 and other stand alone scale has one loadcell only that is loadcell 0.
 *
 * History:  Created on 2011-06-30 by Wai Fai Chin
 */

BYTE	panel_setup_print_string_top_menu_manger( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )
{
	return YES;
} // end panel_setup_print_string_top_menu_manger()


/* This version is dynamically hidden interval period menu item.
BYTE	panel_setup_print_string_top_menu_manger( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj )
{
	BYTE			n;
	BYTE			status;

	status = NO;
	n = PANEL_SETUP_LOADCELL_NUM;

	if ( PRINT_STRING_CTRL_MODE_CONTINUOUS != gabPrintStringCtrlModeFNV[ PANEL_SETUP_DEFAULT_LISTENER ])
		pSetupObj->disabledItemFlags |= (1 << MENU_PRINT_CONT_INTERVAL );	// disabled continuous interval item.
	else {
		pSetupObj->disabledItemFlags &= ~(1 << MENU_PRINT_CONT_INTERVAL );	// enabled continuous interval item.
	}

	if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMove ) {
		pSetupObj-> selectIndex = MENU_PRINT_COMPOSITE_STRING;
		status = YES;
	} // end if ( PSTMC_CURMOVE_INIT_STATE == pSetupObj-> curMOve ) {}
	else if ( PSTMC_CURMOVE_ENTER == pSetupObj-> curMove ) {
		if ( MENU_PRINT_CNTRL_MODE == pSetupObj-> selectIndex &&
			 PRINT_STRING_CTRL_MODE_CONTINUOUS == gabPrintStringCtrlModeFNV[ PANEL_SETUP_DEFAULT_LISTENER ]	) {
			pSetupObj-> selectIndex = MENU_PRINT_CONT_INTERVAL;   // forced it into continuous interval time setting menu item.
		}
	}

	return status;
} // end panel_setup_print_string_top_menu_manger()
*/

/**
 * Method to get and set RF device Disabled or Enabled.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2012-04-27 by Wai Fai Chin
 */

BYTE	panel_setup_rf_device_on_off_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;
	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		index =  pSetupObj-> selectIndex;
		if ( index > 0 ) {
			gRfDeviceSettingsFNV.status |= ( RF_DEVICE_STATUS_ENABLED_bm | RF_DEVICE_STATUS_INSTALLED_bm);
		}
		else {
			gRfDeviceSettingsFNV.status &= ~( RF_DEVICE_STATUS_ENABLED_bm | RF_DEVICE_STATUS_INSTALLED_bm);
		}
		gRfDeviceSettingsFNV.status |= RF_DEVICE_STATUS_SET_CONFIG_bm;
	}
	else {
		if ( ( RF_DEVICE_STATUS_ENABLED_bm | RF_DEVICE_STATUS_INSTALLED_bm) & gRfDeviceSettingsFNV.status )
			index = 1;
		else
			index = 0;
	}
	return index;
} // end  panel_setup_rf_device_on_off_mapper(,,)


/**
 * Method to get and set ScaleCore ID.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2012-04-27 by Wai Fai Chin
 */


BYTE	panel_setup_rf_sc_id_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	float *pfV;

	pfV = pV;
	if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) {
		if ( *pfV > 255.0f ) {	// if user entered a value > 8bit, force it to equal 255.
			*pfV = 255;
		}
		gtProductInfoFNV.devID = (BYTE)*pfV;
	}
	else {
		*pfV = (float) gtProductInfoFNV.devID;
	}

	return pSetupObj-> selectIndex;

} // end  panel_setup_rf_sc_id_mapper(,,)

/**
 * Method to get and set RF device channel number.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2012-04-27 by Wai Fai Chin
 */


BYTE	panel_setup_rf_channel_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	float *pfV;

	pfV = pV;
	if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) {
		if ( *pfV > 255.0f ) {	// if user entered a value > 8bit, force it to equal 255
			*pfV = 255;
		}
		gRfDeviceSettingsFNV.channel = (WORD16)*pfV;
		gRfDeviceSettingsFNV.status |= RF_DEVICE_STATUS_SET_CONFIG_bm;
	}
	else {
		*pfV = (float) gRfDeviceSettingsFNV.channel;
	}

	return pSetupObj-> selectIndex;

} // end  panel_setup_rf_channel_mapper(,,)


/**
 * Method to get and set RF device channel number.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2012-04-27 by Wai Fai Chin
 */


BYTE	panel_setup_rf_network_id_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	float *pfV;

	pfV = pV;
	if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) {
		if ( *pfV > 65535.0f ) {	// if user entered a value > 16bit, force it to equal 65535.
			*pfV = 65535;
		}
		gRfDeviceSettingsFNV.networkID = (WORD16)*pfV;
		gRfDeviceSettingsFNV.status |= RF_DEVICE_STATUS_SET_CONFIG_bm;
	}
	else {
		*pfV = (float) gRfDeviceSettingsFNV.networkID;
	}

	return pSetupObj-> selectIndex;

} // end  panel_setup_rf_network_id_mapper(,,)

// 2012-06-28 -WFC- ^ 2012-07-03 -DLM-


/**
 * Method to get and set Ethernet device Disabled or Enabled.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2012-07-06 by Denis Monteiro
 */

BYTE	panel_setup_ethernet_device_on_off_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;
	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		index =  pSetupObj-> selectIndex;
		if ( index > 0 ) {
//			gEthernetDeviceSettingsFNV.status |= ( ETHERNET_DEVICE_STATUS_ENABLED_bm | ETHERNET_DEVICE_STATUS_INSTALLED_bm);
			gEthernetDeviceSettingsFNV.status |= ( ETHERNET_DEVICE_STATUS_ENABLED_bm );
		}
		else {
//			gEthernetDeviceSettingsFNV.status &= ~( ETHERNET_DEVICE_STATUS_ENABLED_bm | ETHERNET_DEVICE_STATUS_INSTALLED_bm);
			gEthernetDeviceSettingsFNV.status &= ~( ETHERNET_DEVICE_STATUS_ENABLED_bm );
		}
		//gEthernetDeviceSettingsFNV.status |= ETHERNET_DEVICE_STATUS_SET_CONFIG_bm;
	}
	else {
//		if ( ( ETHERNET_DEVICE_STATUS_ENABLED_bm | ETHERNET_DEVICE_STATUS_INSTALLED_bm) & gEthernetDeviceSettingsFNV.status )
		if ( ETHERNET_DEVICE_STATUS_ENABLED_bm & gEthernetDeviceSettingsFNV.status )
			index = 1;
		else
			index = 0;
	}
	return index;
} // end  panel_setup_ethernet_device_on_off_mapper(,,)


/**
 * Method to get and set ScaleCore ID.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2012-04-27 by Wai Fai Chin
 */


BYTE	panel_setup_ethernet_sc_id_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	float *pfV;

	pfV = pV;
	if ( PANEL_MENU_ACTION_MODE_SET_VALUE == actionMode ) {
		if ( *pfV > 255.0f ) {	// if user entered a value > 8bit, force it to equal 255.
			*pfV = 255;
		}
		gtProductInfoFNV.devID = (BYTE)*pfV;
	}
	else {
		*pfV = (float) gtProductInfoFNV.devID;
	}

	return pSetupObj-> selectIndex;

} // end  panel_setup_ethernet_sc_id_mapper(,,)


// 2016-03-21 -WFC- v
/**
 * Method to get and set RF device always on setting.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2016-03-31 by Wai Fai Chin
 */

BYTE	panel_setup_rf_device_always_on_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;
	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		index =  pSetupObj-> selectIndex;
		if ( index > 0 ) {
			gRfDeviceSettingsFNV.status |= RF_DEVICE_STATUS_ALWAYS_ON_bm;
		}
		else {
			gRfDeviceSettingsFNV.status &= ~RF_DEVICE_STATUS_ALWAYS_ON_bm;
		}
		// gRfDeviceSettingsFNV.status |= RF_DEVICE_STATUS_SET_CONFIG_bm;
	}
	else {
		if ( RF_DEVICE_STATUS_ALWAYS_ON_bm & gRfDeviceSettingsFNV.status )
			index = 1;
		else
			index = 0;
	}
	return index;
} // end  panel_setup_rf_device_always_on_mapper(,,)


/**
 * Method to get and set RF device type.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2016-03-31 by Wai Fai Chin
 */

BYTE	panel_setup_rf_device_type_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;
	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		index =  pSetupObj-> selectIndex;
		if ( index > 0 ) {
			gRfDeviceSettingsFNV.deviceType = RF_DEVICE_TYPE_UNDEFINED;
		}
		else {
			gRfDeviceSettingsFNV.deviceType = RF_DEVICE_TYPE_XBEE;
		}
	}
	else {
		if ( RF_DEVICE_TYPE_XBEE == gRfDeviceSettingsFNV.deviceType )
			index = 0;
		else
			index = 1;
	}
	return index;
} // end  panel_setup_rf_device_type_mapper(,,)


// 2016-03-21 -WFC- ^


/**
 * Method to map between power saving mode variable and menu selection index.
 *
 * @param  actionMode	-- 0== map variable content into menu selection.
 * @param  pSetupObj	-- pointer to panel setup sub menu object
 * @param  pV			-- generic pointer to any data type.
 *
 * @return memu selection index based on the setting variable content.
 *
 * History:  Created on 2014-09-30 by Wai Fai Chin
 *
 */

BYTE	panel_setup_battery_life_mode_mapper( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV )
{
	BYTE index;

	if ( PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX == actionMode ) {
		index = pSetupObj-> selectIndex;
		gbScaleStandardModeNV &= ~SCALE_STD_MODE_POWER_SAVING_MASK;		// clear power saving mode to normal
		if ( 1 == index ) {
			gbScaleStandardModeNV |= SCALE_STD_MODE_POWER_SAVING_ENABLED;	// set standard mode and motion detect enabled and preserved other SCALE_STD_MODE_ bits.
		}
	}
	else { // map configuration content to menu selection index.
		index =  gbScaleStandardModeNV;
		index &= SCALE_STD_MODE_POWER_SAVING_MASK;							// get scale standard mode
		if ( SCALE_STD_MODE_POWER_SAVING_ENABLED == index )
			index = 1;
		else
			index = 0;
	}

	return index;
} // end panel_setup_battery_life_mode_mapper(,,)

