/*! \file panel_setup.h \brief Panel setup definition.*/
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
//  History:  Modified on 2011/12/15 by Denis Monteiro
// 
/// \ingroup panelmain
/// \defgroup panel_setup  Panel Menu setup. (panel_setup.c )
/// \code #include "panel_setup.h" \endcode
/// \par Overview
//   Panel Menu setup.
//
// ****************************************************************************
//@{
 

#ifndef MSI_PANEL_SETUP_H
#define MSI_PANEL_SETUP_H

#include	"config.h"
#include	"led_display.h"
#include	"scalecore_sys.h"

#include	"pt.h"

#include	"v_key.h"		// -WFC- 2011-03-11

#define PANEL_SETUP_LOCAL_MAX_STRING_SIZE	24

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 )		// 2011-06-10 -WFC-
// the following are the single key pressed code.
#define PANEL_NO_NEW_KEY		V_KEY_NO_NEW_KEY
#define	PANEL_POWER_KEY			V_KEY_POWER_KEY
#define PANEL_ZERO_KEY			V_KEY_ZERO_KEY
#define PANEL_USER_KEY			V_KEY_USER_KEY
#define PANEL_TARE_KEY			V_KEY_TARE_KEY
#define PANEL_USER2_KEY			V_KEY_USER2_KEY
#define	PANEL_VIEW_KEY			V_KEY_VIEW_KEY
#define	PANEL_TOTAL_KEY			V_KEY_TOTAL_KEY
#define	PANEL_NEG_G_KEY			V_KEY_NET_G_KEY

#define PANEL_MENU_ENTER_KEY	PANEL_TARE_KEY
#define PANEL_MENU_SCROLL_KEY	PANEL_USER_KEY

// The following are the combination key code
#define PANEL_SETUP_KEY			V_KEY_SETUP_KEY
#define PANEL_CAL_KEY			V_KEY_CAL_KEY
#define PANEL_MASTER_RESET_KEY	V_KEY_MASTER_RESET_KEY
#define PANEL_KEY_COM_PORT_SETUP_KEY	V_KEY_COM_PORT_SETUP_KEY	// 2012-05-10 -WFC-

#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
// the following are the single key pressed code.
#define PANEL_NO_NEW_KEY		V_KEY_NO_NEW_KEY
#define	PANEL_POWER_KEY			V_KEY_POWER_KEY
#define PANEL_ZERO_KEY			V_KEY_ZERO_KEY
// #define PANEL_USER_KEY			V_KEY_USER_KEY
// #define PANEL_TARE_KEY			V_KEY_TARE_KEY
#define PANEL_USER1_KEY			V_KEY_USER1_KEY
#define PANEL_USER2_KEY			V_KEY_USER2_KEY
#define	PANEL_VIEW_KEY			V_KEY_VIEW_KEY
#define	PANEL_TOTAL_KEY			V_KEY_TOTAL_KEY
#define	PANEL_NEG_G_KEY			V_KEY_NET_G_KEY

#define PANEL_MENU_ENTER_KEY	PANEL_USER1_KEY
#define PANEL_MENU_SCROLL_KEY	PANEL_USER2_KEY

// The following are the combination key code
#define PANEL_SETUP_KEY			V_KEY_SETUP_KEY
#define PANEL_CAL_KEY			V_KEY_CAL_KEY
#define PANEL_MASTER_RESET_KEY	V_KEY_MASTER_RESET_KEY
/// COMPORT SETUP KEY is by pressed USER1 and USER2 key at the same time. 2011-06-30 -WFC-
#define PANEL_KEY_COM_PORT_SETUP_KEY	V_KEY_COM_PORT_SETUP_KEY
#elif (  CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-09-27 -WFC-
// the following are the single key pressed code.
#define PANEL_NO_NEW_KEY		V_KEY_NO_NEW_KEY
#define	PANEL_POWER_KEY			V_KEY_POWER_KEY
#define PANEL_ZERO_KEY			V_KEY_ZERO_KEY
// #define PANEL_USER_KEY			V_KEY_USER_KEY
#define PANEL_TARE_KEY			V_KEY_TARE_KEY
#define PANEL_USER1_KEY			V_KEY_USER1_KEY
#define PANEL_USER2_KEY			V_KEY_USER2_KEY
//#define	PANEL_VIEW_KEY			V_KEY_VIEW_KEY
//#define	PANEL_TOTAL_KEY			V_KEY_TOTAL_KEY
//#define	PANEL_NEG_G_KEY			V_KEY_NET_G_KEY

#define	PANEL_PRINT_KEY			V_KEY_PRINT_KEY		// 2011-09-27 -WFC-

#define PANEL_MENU_ENTER_KEY	PANEL_USER1_KEY
#define PANEL_MENU_SCROLL_KEY	PANEL_USER2_KEY

// The following are the combination key code
#define PANEL_SETUP_KEY			V_KEY_SETUP_KEY
#define PANEL_CAL_KEY			V_KEY_CAL_KEY
#define PANEL_MASTER_RESET_KEY	V_KEY_MASTER_RESET_KEY
/// COMPORT SETUP KEY is by pressed USER1 and USER2 key at the same time. 2011-06-30 -WFC-
#define PANEL_KEY_COM_PORT_SETUP_KEY	V_KEY_COM_PORT_SETUP_KEY

#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_HLI || CONFIG_PRODUCT_AS == CONFIG_AS_HD )
#define PANEL_NO_NEW_KEY	KEY_NO_NEW_KEY
#define	PANEL_POWER_KEY		KEY_POWER_KEY
#define PANEL_ZERO_KEY		KEY_ZERO_KEY
#define PANEL_USER_KEY		KEY_USER_KEY
#define PANEL_TARE_KEY		KEY_TARE_KEY

// HD panel keys
#define PANEL_F1_KEY		KEY_USER_KEY
#define PANEL_TEST_KEY		KEY_TEST_KEY
#define PANEL_UNDO_ZERO_KEY	KEY_UNDO_ZERO_KEY

#define PANEL_DEBUG_COMBO_KEY	(PANEL_F1_KEY | PANEL_TEST_KEY )

// The following are the combination key code
#define PANEL_SETUP_KEY			KEY_SETUP_KEY
#define PANEL_CAL_KEY			KEY_CAL_KEY
#define PANEL_MASTER_RESET_KEY	KEY_MASTER_RESET_KEY

#define	FUNC_KEY_DISABLED		0
#define	FUNC_KEY_TEST			1
#define	FUNC_KEY_TOTAL			2
#define	FUNC_KEY_UNIT			3
#define	FUNC_KEY_PHOLD			4
#define	FUNC_KEY_NET_GROSS		5


#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )		// 2011-12-15 -DLM-
// the following are the single key pressed code.
#define PANEL_NO_NEW_KEY		V_KEY_NO_NEW_KEY
#define	PANEL_POWER_KEY			V_KEY_POWER_KEY
#define PANEL_ZERO_KEY			V_KEY_ZERO_KEY
#define PANEL_USER_KEY			V_KEY_USER_KEY
#define PANEL_TARE_KEY			V_KEY_TARE_KEY
#define PANEL_USER2_KEY			V_KEY_USER2_KEY
#define	PANEL_VIEW_KEY			V_KEY_VIEW_KEY
#define	PANEL_TOTAL_KEY			V_KEY_TOTAL_KEY
#define	PANEL_NEG_G_KEY			V_KEY_NET_G_KEY

#define PANEL_MENU_ENTER_KEY	PANEL_TARE_KEY
#define PANEL_MENU_SCROLL_KEY	PANEL_USER_KEY

// The following are the combination key code
#define PANEL_SETUP_KEY			V_KEY_SETUP_KEY
#define PANEL_CAL_KEY			V_KEY_CAL_KEY
#define PANEL_MASTER_RESET_KEY	V_KEY_MASTER_RESET_KEY
#define PANEL_KEY_COM_PORT_SETUP_KEY	V_KEY_COM_PORT_SETUP_KEY	// 2012-05-10 -WFC- 2012-06-20 -DLM-

#endif

/* 2011-07-22 -WFC- moved to v_key.h file
#define	FUNC_KEY_DISABLED		0
#define	FUNC_KEY_TEST			1
#define	FUNC_KEY_TOTAL			2
#define	FUNC_KEY_VIEW_TOTAL		3
#define	FUNC_KEY_NET_GROSS		4
#define	FUNC_KEY_LEARN			5
#define	FUNC_KEY_PHOLD			6
/// only switch between KG and LB
#define	FUNC_KEY_UNIT			7
/// cycle through LB, KG, TON, MTON and KN.
#define	FUNC_KEY_5UNITS			8
#define FUNC_KEY_HIRES			9
#define FUNC_KEY_PRINT			10
#define	FUNC_KEY_TARE			11

/// Maximum number of functions for a function key.
#define	FUNC_KEY_MAX_NUM		11
*/

/** \def panel_key_get
 * High level key reading function. The name of this function API should never change
 * because the change of low level key module will not affect the high level function.
 *
 * @param   pbKey  -- pointer to key variable to store the new valid debounced key
 *
 * @return  return 0 if no new key
 *          1 if got a new valid debounced key(s).
 * @post	updated gbIsKeyStuck.
 *
 * History:  Created on 2009/07/10 by Wai Fai Chin
 */
#define panel_key_get( pbKey )		v_key_get( (BYTE *)(pbKey) )


/**
 *	It initializes the keypad and its related variables and buffer..
 * High level key reading function. The name of this function API should never change
 * because the change of low level key module will not affect the high level function.
 *
 * @return none
 *
 * History:  Created on 2009/07/22 by Wai Fai Chin
 */

#define panel_key_init() 	 v_key_init()


/**
 * Use with PANEL_MENU_ITEM_T pMethod action mode parameter.
 * The panel setup thread will map cnfg variable to menu select index so
 * it can display the data of the cnfg contents.
 * When the user pressed the enter key (TARE key), it will map the menu select index
 * into cnfg variable settings.
 */

#define		PANEL_MENU_ACTION_MODE_GET_CNFG_INDEX		0
#define		PANEL_MENU_ACTION_MODE_SET_CNFG_INDEX		1
#define		PANEL_MENU_ACTION_MODE_GET_VALUE			2
#define		PANEL_MENU_ACTION_MODE_SET_VALUE			3
#define		PANEL_MENU_ACTION_MODE_GET_DEFAULT_VALUE	4
#define		PANEL_MENU_ACTION_MODE_SET_DEFAULT_VALUE	5
#define		PANEL_MENU_ACTION_MODE_SHOW_VALUE			6
#define		PANEL_MENU_ACTION_MODE_GET_DEFAULT_CHOICE	7
#define		PANEL_MENU_ACTION_MODE_GET_CHOICE			8
#define		PANEL_MENU_ACTION_MODE_SET_CHOICE			9
#define		PANEL_MENU_ACTION_MODE_SHOW_CHOICE			10

#define		PANEL_MENU_ACTION_MODE_EXIT_ITEM			16
#define		PANEL_MENU_ACTION_MODE_CANCEL_AND_EXIT_ITEM	17

#define		PANEL_MENU_ACTION_MODE_EXIT_STEP_1			101
#define		PANEL_MENU_ACTION_MODE_EXIT_STEP_2			102
#define		PANEL_MENU_ACTION_MODE_EXIT_THIS_THREAD_NOW	255

/**
 * MENU TYPE and MODE for PANEL_TOP_MENU_T's numOfItem member in higher nibble.
 */

/// COMPLEX MENU TYPE that may have submenu, or numeric entry etc...
#define		PANEL_TOP_MENU_TYPE_COMPLEX			0XE0
#define		PANEL_TOP_MENU_TYPE_SIMPLE			0XC0
#define		PANEL_TOP_MENU_TYPE_DYNAMIC_CHOICE	0X80
#define		PANEL_TOP_MENU_TYPE_STRING			0X60
#define		PANEL_TOP_MENU_TYPE_INTEGER			0X40
#define		PANEL_TOP_MENU_TYPE_FLOAT			0X20
#define		PANEL_TOP_MENU_TYPE_MASK			0XE0
#define		PANEL_TOP_MENU_NUM_ITEM_MASK		0X1F


/// MENU ITEM TYPE 
#define		PANEL_MENU_ITEM_TYPE_COMPLEX		0XE0
#define		PANEL_MENU_ITEM_TYPE_SIMPLE			0XC0
#define		PANEL_MENU_ITEM_TYPE_DYNAMIC_CHOICE	0X80
#define		PANEL_MENU_ITEM_TYPE_STRING			0X60
#define		PANEL_MENU_ITEM_TYPE_INTEGER		0X40
#define		PANEL_MENU_ITEM_TYPE_FLOAT			0X20
#define		PANEL_MENU_ITEM_TYPE_MASK			0XE0
#define		PANEL_MENU_ITEM_NUM_ITEM_MASK		0X1F

#define		PANEL_MENU_ITEM_DECPT_USE_CB				0XFF
#define		PANEL_MENU_ITEM_DECPT_USE_NEXT_LOWER_CB		0xFE

/// Menu message ID

#define		PANEL_MENU_ITEM_PERMISSION_ENABLED_ALL		0

enum { 
	MENU_MSG_ID_CAL,
	MENU_MSG_ID_CAL_D,
	MENU_MSG_ID_CAL_R,
	MENU_MSG_ID_R_CAL,
	MENU_MSG_ID_STORE,
	MENU_MSG_ID_CERR,
	MENU_MSG_ID_R_ERR,
	MENU_MSG_ID_NO,
	MENU_MSG_ID_PASS,
	MENU_MSG_ID_FAIL,
	MENU_MSG_ID_Cancl,
	MENU_MSG_ID_Good,
	MENU_MSG_ID_Littl,
	MENU_MSG_ID_Func1,
	MENU_MSG_ID_Func2,
	MENU_MSG_ID_StAnd,
	MENU_MSG_ID_SEtuP,
	MENU_MSG_ID_SEtP1,
	MENU_MSG_ID_SEtP2,
	MENU_MSG_ID_SEtP3,
	MENU_MSG_ID_AZero,
	MENU_MSG_ID_TtlOn,
	MENU_MSG_ID_TtOff,
	MENU_MSG_ID_Print,
	MENU_MSG_ID_rF,
	MENU_MSG_ID_Ethernet
};


#define	PMI_TYPE_READ_ONLY	0x80

typedef struct PANEL_SETUP_SUB_MENU_CLASS_TAG   PANEL_SETUP_SUB_MENU_CLASS;


typedef struct PANEL_MENU_ITEM_TAG {
											/// points to a menu message string or a list of message for simple menu items.
  char *pMsg;
											/// points to a method that maps between cnfg variable and menu select index.
  BYTE ( *pMethod) ( BYTE actionMode, PANEL_SETUP_SUB_MENU_CLASS *pSetupObj, void *pV );
											/// decimal point for floating point to the left. 0== no decimal point. bit7 is flaged read only OR PMI_TYPE_READ_ONLY.
  BYTE 				decPt;
											/// number of items in this menu in the lower nibble. Its higher nibble contains menu item type such as float, integer, string, simple.
  BYTE 				maxNumOfItem;
}PANEL_MENU_ITEM_T;


typedef struct PANEL_TOP_MENU_TAG {
											/// points to a top menu message string.
  char 				*pMsg;
											/// points to a root menu item. Note: if it is a simple menu type, it only contained one menu item regardless of maxNumOfItem. The PANEL_MENU_ITEM_T pMsg points to a list of msg. This approach save lots of memory.
  PANEL_MENU_ITEM_T	*pRootItem;
											/// points to current sub menu item thread
  char ( *pSubMenuThread) (PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );				// 2011-04-11 -WFC-
											/// number of items in this menu in the lower nibble. Its higher nibble contains menu type and msg mode.
  BYTE 				maxNumOfItem;
}PANEL_TOP_MENU_T;

typedef struct PANEL_SETUP_TOP_MENU_CLASS_TAG	PANEL_SETUP_TOP_MENU_CLASS;

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HLI || CONFIG_PRODUCT_AS == CONFIG_AS_HD )
struct PANEL_SETUP_TOP_MENU_CLASS_TAG {
											/// this thread
  struct pt 		m_pt;
											/// points to the root of top menu.
  PANEL_TOP_MENU_T *pRootMenu;
											/// points to methods such as see if the this top menu is allow to use; exit menu procedure etc..
  BYTE ( *pMethod) ( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );
											/// PANEL_MENU_SCROLL_KEY key will increment select index,
  BYTE  			selectIndex;
											/// number of menu
  BYTE 				maxNumOfItem;
											/// Menu message ID.
  BYTE 				msgID;
											/// message display time, 0 means no msg to display.
  BYTE 				msgDisplayTime;
											/// current move, 0 == init state, 
  BYTE 				curMove;
											/// 16bit menu item disabled flags bits. 1 == disabled. 0 == enabled. Disabled prevents user from access to a menu item or group of menu items. 2011-04-26 -WFC-
  UINT16 			disabledItemFlags;		// 2011-04-26 -WFC-
											/// parent type, 0 == none.
  BYTE 				parentIs;
  char ( *pSubMenuThread) (PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );
};

#else
struct PANEL_SETUP_TOP_MENU_CLASS_TAG {
											/// this thread
  struct pt 		m_pt;
											/// points to the root of top menu.
  PANEL_TOP_MENU_T *pRootMenu;
											/// points to methods such as see if the this top menu is allow to use; exit menu procedure etc..
  BYTE ( *pMethod) ( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );
											/// PANEL_MENU_SCROLL_KEY key will increment select index,
  BYTE  			selectIndex;
											/// number of menu
  BYTE 				maxNumOfItem;
											/// Menu message ID.
  BYTE 				msgID;
											/// message display time, 0 means no msg to display.
  BYTE 				msgDisplayTime;
											/// current move, 0 == init state,
  BYTE 				curMove;
											/// 16bit menu item disabled flags bits. 1 == disabled. 0 == enabled. Disabled prevents user from access to a menu item or group of menu items. 2011-04-26 -WFC-
  UINT16 			disabledItemFlags;		// 2011-04-26 -WFC-
											/// parent type, 0 == none.
  BYTE 				parentIs;
};
#endif

#define PSTMC_PARENT_NONE					0
#define PSTMC_PARENT_IS_TOP_MENU_GROUP		1

#define PSTMC_CURMOVE_INIT_STATE			0
#define PSTMC_CURMOVE_NEXT					1
#define PSTMC_CURMOVE_ENTER					2
#define PSTMC_CURMOVE_EXIT					3		// 2011-04-27 -WFC-

struct PANEL_SETUP_SUB_MENU_CLASS_TAG {
											/// this thread
  struct pt 		m_pt;
											/// points to current sub menu item thread
  char ( *pSubMenuThread) (PANEL_SETUP_SUB_MENU_CLASS *pSetupObj );				// 2011-04-11 -WFC-
											/// points to the root of this sub menu item.
  PANEL_MENU_ITEM_T *pRootItem;
											/// Top level menu index that points at pRootItem.
  INT8  			topLevelMenuIndex;		// 2011-04-11	-WFC-
											/// PANEL_MENU_SCROLL_KEY key will increment select index,
  BYTE  			selectIndex;
											/// number of menu item
  BYTE 				maxNumOfItem;
											/// Menu message ID.
  BYTE 				msgID;
											/// message display time, 0 means no msg to display.
  BYTE 				msgDisplayTime;
											/// next move for the top menu item. 0==no move, 1-31 move foreward, 32 to 63 move backward. 255== special case exit mainly for two steps exit give more time house keeping before exit..
  BYTE 				nextMove;
};

#define PSSMC_NEXTMOVE_ENTER_INTO_NEXT_PM_ITEM	252		// 2011-7-18 -WFC- next action is to enter into next PANEL_MENU_ITEM_T.
#define PSSMC_NEXTMOVE_DISPLAY_FLOAT_KEY_EXIT	253		// 2011-7-18 -WFC- next action is to display float value and wait for user key press.
#define PSSMC_NEXTMOVE_DISPLAY_FLOAT_EXIT		254		// next action is to display float  value and exit this thread.
#define PSSMC_NEXTMOVE_SPECIAL_EXIT				255



typedef struct PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_TAG {
											/// points to the root of top menu.
  PANEL_TOP_MENU_T *pRootMenu;
											/// number of menu
  BYTE 				maxNumOfItem;
											/// Menu message ID.
  BYTE 				msgID;
											/// message display time, 0 means no msg to display.
  BYTE 				msgDisplayTime;
											/// 16bit menu item disabled flags bits. 1 == disabled. 0 == enabled. Disabled prevents user from access to a menu item or group of menu items. 2011-04-26 -WFC-
  UINT16 			disabledItemFlags;		// 2011-04-26 -WFC-
											/// points to methods such as see if the this top menu is allow to use; exit menu procedure etc..
  BYTE ( *pMethod) ( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );
}PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T;


/*
typedef struct PANEL_SETUP_TOP_MENU_GROUP_CLASS_TAG {
											/// this thread
  struct pt 		m_pt;
											/// points to the root of this group menu item.
  PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T	*pRootItem;
											/// USER key will increment select index, TARE key is the enter key.
  BYTE  			selectIndex;
											/// number of menu group item in this TOP Menu Group.
  BYTE 				maxNumOfItem;
											/// Menu message ID.
  BYTE 				msgID;
											// / message display time, 0 means no msg to display.
  // BYTE 				msgDisplayTime;
											/// next move for the top menu item. 0==no move, 1-31 move foreward, 32 to 63 move backward.
  BYTE 				nextMove;
											/// 16bit menu item disabled flags bits. 1 == disabled. 0 == enabled. Disabled prevents user from access to a menu item or group of menu items. 2011-04-26 -WFC-
  UINT16 			disabledItemFlags;		// 2011-04-26 -WFC-
											/// points to methods such as see if the this top menu is allow to use; exit menu procedure etc..
  BYTE ( *pMethod) ( PANEL_SETUP_TOP_MENU_GROUP_CLASS *pSetupObj );
}PANEL_TOP_MAIN_MENU_GROUP_T;
*/

typedef struct PANEL_SETUP_TOP_MENU_GROUP_CLASS_TAG		PANEL_SETUP_TOP_MENU_GROUP_CLASS;

struct PANEL_SETUP_TOP_MENU_GROUP_CLASS_TAG {
											/// this thread
  struct pt 		m_pt;
											/// points to the root of this group menu item.
  PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T	*pRootItem;
											/// PANEL_MENU_SCROLL_KEY key will increment select index,
  BYTE  			selectIndex;
											/// number of menu group item in this TOP Menu Group.
  BYTE 				maxNumOfItem;
											/// Menu message ID.
  BYTE 				msgID;
											// / message display time, 0 means no msg to display.
  // BYTE 				msgDisplayTime;
											/// next move for the top menu item. 0==no move, 1-31 move foreward, 32 to 63 move backward.
  BYTE 				nextMove;
											/// 16bit menu item disabled flags bits. 1 == disabled. 0 == enabled. Disabled prevents user from access to a menu item or group of menu items. 2011-04-26 -WFC-
  UINT16 			disabledItemFlags;		// 2011-04-26 -WFC-
											/// points to methods such as see if the this top menu is allow to use; exit menu procedure etc..
  BYTE ( *pMethod) ( PANEL_SETUP_TOP_MENU_GROUP_CLASS *pSetupObj );
};



// PANEL NUMERIC ENTRY RELATED STUFF::
#define PANEL_NUMERIC_RUNNING		3
#define PANEL_NUMERIC_ENTERED		2
#define PANEL_NUMERIC_INVALID		1
#define PANEL_NUMERIC_CANCELLED		0

typedef struct PANEL_NUMERIC_ENTRY_CLASS_TAG {
											/// this thread
  struct pt m_pt;
											/// a string buffer to store input number
  BYTE  	str[ LED_MAX_STRING_SIZE + 1];
											/// index of str
  BYTE  	strIndex;
											/// decimal point location.
  BYTE  	decPtLoc;
											/// selected x1000
  BYTE  	x1000;
											/// status of numeric entry thread. 1==entered a number, 0==cancelled, 2==RUNNING.
  BYTE  	status;
											/// entered value in 32bit unsiged integer
  UINT32	u32V;
											/// entered value in floating point
  float 	fV;
}PANEL_NUMERIC_ENTRY_CLASS;


extern BYTE	gbPanelSetupStatus;

extern PANEL_NUMERIC_ENTRY_CLASS			gPanelNumEntryObj;

extern PANEL_SETUP_TOP_MENU_CLASS			gPanelSetupTopMenuObj;

extern PANEL_SETUP_SUB_MENU_CLASS			gPanelSetupSubMenuObj1;		// sub level 1

extern PANEL_SETUP_TOP_MENU_GROUP_CLASS		gPanelSetupTopMenuGroupObj;

extern PANEL_MENU_ITEM_T			gacPanel_Menu_UserKey_Func[]	PROGMEM;
extern PANEL_MENU_ITEM_T			gacPanel_Menu_A_OFF[]			PROGMEM;

extern PANEL_TOP_MENU_T				gacPanel_Top_Menu_Cnfg[]		PROGMEM;

extern PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T		gacPanel_Top_Menu_Cnfg_Group_Item_Entry[]	PROGMEM;
extern PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T		gacPanel_Menu_Cal_Group_Item_Entry[]		PROGMEM;
extern PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T		gacPanel_Menu_SetPoint_Group_Item_Entry[]	PROGMEM;

#define MENU_MASTER_RESET_MAX_ITEMS		2
extern PANEL_MENU_ITEM_T		gacPanel_Menu_Master_Reset[]	PROGMEM;



//#define PANEL_TOP_MENU_CNFG_MAX_ITEMS			(sizeof(gacPanel_Top_Menu_Cnfg) / sizeof(PANEL_TOP_MENU_T))    COMPILER CANNOT HANDLE THIS EXPRESSION
//#define PANEL_TOP_MENU_CNFG_MAX_ITEMS			(sizeof(gacPanel_Top_Menu_Cnfg[9]) / sizeof(PANEL_TOP_MENU_T)) COMPILER CAN HANDLE THIS EXPRESSION, WHY BOTHER SINCE YOU have to HARDCODE THE ARRAY SIZE.
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )		// 2011-06-29 -WFC-
	// 2015-10-29 -WFC- #define PANEL_TOP_MENU_CNFG_MAX_ITEMS			12
	#define PANEL_TOP_MENU_CNFG_MAX_ITEMS			17
	// 2011-08-31 -WFC- v
	// 2015-10-29 -WFC-  #define PANEL_TOP_MENU_CNFG_LFT_MAX_ITEMS		9	// Max Configuration items for Legal For Trade mode.
	#define PANEL_TOP_MENU_CNFG_LFT_MAX_ITEMS		14		// Max Configuration items for Legal For Trade mode.
	void	panel_setup_construct_cnfg_menu_object( void );
	extern  PANEL_TOP_MENU_T    gacPanel_Top_Menu_Cnfg_LFT[] PROGMEM;
	// 2011-08-31 -WFC- ^

	//2011-04-20 -WFC- #define PANEL_TOP_MENU_NEW_CAL_MAX_ITEMS		6
	#define PANEL_TOP_MENU_NEW_CAL_MAX_ITEMS		9					//2011-04-20 -WFC-
	#define PANEL_TOP_MENU_NEW_RCAL_MAX_ITEMS		6
	//2011-04-20 -WFC- #define PANEL_TOP_MENU_RECAL_MAX_ITEMS			3
	#define PANEL_TOP_MENU_RECAL_MAX_ITEMS			6					//2011-04-20 -WFC-
	#define PANEL_TOP_MENU_RE_RCAL_MAX_ITEMS		3
	#define PANEL_TOP_MENU_CAL_GROUP_MAX_ITEMS		3					// 2011-04-26 -WFC-

#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-06-29, 2011-09-27 -WFC-
	#define PANEL_TOP_MENU_CNFG_MAX_ITEMS			8
	extern PANEL_SETUP_TOP_MENU_GROUP_ITEM_ENTRY_T		gacPanel_Menu_Com_Port_Group_Item_Entry[]	PROGMEM;	// 2011-06-30 -WFC-
	void	panel_setup_construct_cnfg_menu_object( void );			// 2012-02-08 -WFC-
	void	panel_setup_construct_com_port_menu_object( void );

	//2011-04-20 -WFC- #define PANEL_TOP_MENU_NEW_CAL_MAX_ITEMS		6
	#define PANEL_TOP_MENU_NEW_CAL_MAX_ITEMS		9					//2011-04-20 -WFC-
	#define PANEL_TOP_MENU_NEW_RCAL_MAX_ITEMS		6
	//2011-04-20 -WFC- #define PANEL_TOP_MENU_RECAL_MAX_ITEMS			3
	#define PANEL_TOP_MENU_RECAL_MAX_ITEMS			6					//2011-04-20 -WFC-
	#define PANEL_TOP_MENU_RE_RCAL_MAX_ITEMS		3

	// 2011-04-26 -WFC-  #define PANEL_TOP_MENU_CAL_GROUP_MAX_ITEMS		5
	#define PANEL_TOP_MENU_CAL_GROUP_MAX_ITEMS		3					// 2011-04-26 -WFC-

#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_HLI || CONFIG_PRODUCT_AS == CONFIG_AS_HD )
#define PANEL_TOP_MENU_CNFG_MAX_ITEMS			9
#define PANEL_TOP_MENU_NEW_CAL_MAX_ITEMS		6
#define PANEL_TOP_MENU_NEW_RCAL_MAX_ITEMS		6
#define PANEL_TOP_MENU_RECAL_MAX_ITEMS			3
#define PANEL_TOP_MENU_RCAL_MAX_ITEMS			3
#define PANEL_TOP_MENU_RESTRICT_CNFG_MAX_ITEMS	2
#define PANEL_TOP_MENU_CAL_GROUP_MAX_ITEMS		5
#endif

// 2011-08-31 -WFC- v
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 )
	#define PANEL_TOP_MENU_RESTRICT_CNFG_MAX_ITEMS	3
	#define PANEL_TOP_MENU_COM_PORT_GROUP_MAX_ITEMS		2				// 2012-06-28 -WFC-
#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 ) // 2012-06-20 -DLM-
	// #define PANEL_TOP_MENU_RESTRICT_CNFG_MAX_ITEMS	3
	#define PANEL_TOP_MENU_RESTRICT_CNFG_MAX_ITEMS	4					// 2016-03-23 -WFC-
//	#define PANEL_TOP_MENU_COM_PORT_GROUP_MAX_ITEMS		2				// 2012-06-28 -WFC-
	#define PANEL_TOP_MENU_COM_PORT_GROUP_MAX_ITEMS		3				// 2012-07-06 -DLM-
	#define PANEL_TOP_MENU_ETHERNET_DEVICE_CNFG_MAX_ITEMS		2		// 2012-07-06 -DLM-
#else
	#define PANEL_TOP_MENU_RESTRICT_CNFG_MAX_ITEMS	2
	#define PANEL_TOP_MENU_COM_PORT_GROUP_MAX_ITEMS		2				// 2012-05-10 -WFC-
#endif
// 2011-08-31 -WFC- ^

#define PANEL_TOP_MENU_RECAL_GROUP_MAX_ITEMS	3						// 2011-04-26 -WFC-
// 2012-05-10 -WFC- #define PANEL_TOP_MENU_COM_PORT_GROUP_MAX_ITEMS		2					// 2011-06-30 -WFC-
// #define PANEL_TOP_MENU_PRINT_STRING_MAX_ITEMS		3				// 2011-06-30 -WFC-
#define PANEL_TOP_MENU_PRINT_STRING_MAX_ITEMS		5					// 2015-09-25 -WFC-

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )							// 2013-04-02 -DLM-
	#define PANEL_TOP_MENU_RF_DEVICE_CNFG_MAX_ITEMS		5					// 2013-04-02 -DLM-
#else
	// #define PANEL_TOP_MENU_RF_DEVICE_CNFG_MAX_ITEMS		4				// 2012-04-27 -WFC-
	#define PANEL_TOP_MENU_RF_DEVICE_CNFG_MAX_ITEMS		6					// 2016-03-31 -WFC-
#endif

// 2011-04-11 -WFC- moved from panel_setup.c
#define	PANEL_SETUP_STATUS_EXIT_SAVE	0x80
#define	PANEL_SETUP_STATUS_EXIT_NO_SAVE	0x40
#define	PANEL_SETUP_STATUS_EXIT_NOW		0x20
#define	PANEL_SETUP_STATUS_CONTINUE		0x10

char	panel_setup_top_menu_thread( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );
char	panel_setup_numeric_entry_thread( struct pt *pt );
char	panel_setup_top_menu_group_thread( PANEL_SETUP_TOP_MENU_GROUP_CLASS *pSetupObj );

BYTE	panel_setup_does_it_allows_new_cal( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );
BYTE	panel_setup_does_it_allows_new_rcal( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );				// 2011-04-27 -WFC-
BYTE	panel_setup_cal_menu_group_method( PANEL_SETUP_TOP_MENU_GROUP_CLASS *pSetupObj );
void	panel_setup_construct_cal_menu_object( void );
BYTE	panel_setup_get_next_select_item_index( BYTE selectIndex, BYTE maxNumItem, UINT16 disabledItemFlags ); // 2011-04-26 -WFC-

void	panel_setup_show_rcal( BYTE sn );		// 2011-04-08 -WFC-

extern  BYTE	gbIsKeyStuck;

#endif  // MSI_PANEL_SETUP_H

//@}
