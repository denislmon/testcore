/*! \file cmdaction.h \brief functions for process commands.*/
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
// Compiler: any C
// Software layer:  Application
//
//  History:  Created on 2007/07/24 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup commands Command execution related functions (cmdaction.c)
/// \code #include "cmdaction.h" \endcode
/// \see cmdparser, commonlib and calibrate modules
/// \par Overview
///     It processes a valid command from cmdparser module. Its execution based
///  on the input command and the command descriptor.
//
// ****************************************************************************
//@{

/**
 * \code
 * Host Command Structure:
 *    {src dest cmdxP1;P2;;;Pn}
 *    spaces are for clarity. No space between src dest and cmd.
 *    where:
 *           {  is the command start character marke the start of the command.
 *         src  is the source id in two digit hex char range from 00 to FF. FF is a free ID use by host before first connection with this device.
 *         dest is the command id in two digit hex char range from 00 to FE. FF is for every device, broadcast mode.
 *         cmd  is the command id in two digit hex char range from 00 to FE. FF is invalid command.
 *           x  is the command attribute field has the following char:
 *                 r -- answers the command query with contents of items that associated
 *                      with the command. It is for output to the host computer.
 *                 i -- answers the command query of 1D array index type.
 *                      The number followed by i is the index of all the following array outputs.
 *
 *                 ? -- command query items that associated with the command.
 *                 = -- assigns input contents to items that associated with the command.
 *                 z -- reset items that associated with the command to its default contents.
 *              g, = -- denoted for action type command. Action type command normally has no input parameter.
 *                      In some case, it needs an array index number such as sensor ID, after the 'g' or '='.
 *                      The resulted action might output data to a registered stream device or other I/O devices.
 *                 0 to F are reserved for future use.
 *          Pn  is the input parameters or contents of items that associated with the command.
 *           }  is the end character marked the end of the command.
 * Command Version:  00-00-01
 * \endcode
 * \note every input command will always has an corresponding respond output.
 *       The output could be a queried value, status, or some other output.
 *
 * History:  Created on 2006/11/07 by Wai Fai Chin
 *			 2009/03/31 -WFC- modified it to include source and destination ID.
 */

#ifndef MSI_CMDACTION_H
#define MSI_CMDACTION_H

#include  "config.h"
#include  "cmdparser.h"
#include  "scalecore_sys.h"
#include  "pt.h"
#include  "timer.h"

#define CMD_MAX_STRING_LEN_PER_OUTPUT	40
#define CMD_MAX_TMP_STRING_LENGTH		12

enum {
//{00} Command Status Query.
	CMD_STATUS_QUERY,
//{01} Application Software Info Query.
	CMD_APP_SOFTWARE_INFO_QUERY,
//{02} System Status Query
	CMD_SYS_STATUS_QUERY,
//{03} Product Info.
	CMD_PRODUCT_INFO,
//{04} Password Setting and Locking Feature.
	CMD_PASSWORD_LOCKING,
//{05} Packetized Sensor Value Query.
	CMD_PACKET_SENSOR_VALUE_QUERY,
//{06} Non Packetized Sensor Value Query.
	CMD_NON_PACKET_SENSOR_VALUE_QUERY,
//{07} System Run Mode.
	CMD_SYS_RUN_MODE,
//{08} Test On Board IOs.
	CMD_TEST_ON_BOARD_IO,
//{09} Goto Bootloader.
	CMD_GOTO_BOOTLOADER,
//{0A} Get Cal Point Data in Cal Mode.
	CMD_GET_CAL_POINT_IN_CAL_MODE,
//{0B} Calibartion Status.
	CMD_GET_CAL_STATUS,
//{0C} Cal Start a Brand New Calibration Step.
	CMD_START_NEW_CAL_STEP,
//{0D} Cal Set Countby Step.
	CMD_CAL_SET_COUNTBY_STEP,
//{0E} Cal Zeroing Step Or Start Re Cal Step.
	CMD_CAL_ZEROING_OR_RE_CAL_STEP,
//{0F} Cal Set Test Point Step.
	CMD_CAL_SET_TEST_POINT,
//{10} Cal Save and Exit Step.
	CMD_SAVE_EXIT_STEP,
//{11} Cal Abort and Exit Step.
	CMD_CAL_ABORT_EXIT_STEP,
//{12} Standard Mode AZM and Motion Detect.
	CMD_STANDARD_MODE_AZM_MOTION,
////{13} Rcal Circuit On/Off.
	CMD_RCAL_CIRCUIT_ON_OFF,
//{14} Loadcell ADC and Weight.
	CMD_LOADCELL_ADC_WEIGHT,
//{15} Cal Table Header Info.
	CMD_CAL_TABLE_HEADER_INFO,
//{16} Cal Point Info.
	CMD_CAL_POINT_INFO,
//{17} Sensor Countby For Viewing.
	CMD_SENSOR_COUNTBY_FOR_VIEWING,
//{18} Sensor Unit For Viewing.
	CMD_SENSOR_UNIT_FOR_VIEWING,
//{19} Sensor Capacity and Underload Threshold For Viewing.
	CMD_SENSOR_CAP_UNDERLOAD_THRESHOLD,
//{1A} Stream Output Mode.
	CMD_STREAM_OUTPUT_CTRL,
//{1B} Stream Registry Info.
	CMD_GET_STREAM_REGISTRY_INFO,
//{1C} Reserved Future Command.
	CMD_RESERVED_0X1C,
//{1D} Reserved Future Command.
	CMD_RESERVED_0X1D,
//{1E} Sensor Features.
	CMD_SENSOR_FEATURES,
//{1F} Loadcell Motion Pending Time.
	CMD_LOADCELL_MOTION_PENDING_TIME,
//{20} Zeroing Setting For Standard Mode.
	CMD_ZEROING_SETTING_FOR_STANDARD_MODE,
//{21} Zeroing Setting For NTEP Mode.
	CMD_ZEROING_SETTING_FOR_NTEP_MODE,
//{22} Zeroing Setting For OIML Mode.
	CMD_ZEROING_SETTING_FOR_OIML_MODE,
//{23} Loadcell Operation Modes And Total Modes.
	CMD_LOADCELL_OPERATION_TOTAL_MODES,
//{24} Total On Accept Window.
	CMD_TOTAL_ON_ACCEPT_WINDOW,
//{25} Total Weight Reading.
	CMD_TOTAL_WEIGHT_READING,
//{26} Total Statistics Reading.
	CMD_TOTAL_STATISTICS,
//{27} Toggle NET GROSS Mode.
	CMD_TOGGLE_NET_GROSS_MODE,
//{28} Zeroing a Loadcell.
	CMD_ZEROING_A_LOADCELL,
//{29} Tare a Loadcell.
	CMD_TARE_A_LOADCELL,
//{2A} Clear Tare Weight of a Loadcell.
	CMD_CLEAR_WEIGHT,
//{2B} Sensor Name.
	CMD_SENSOR_NAME,
//{2C} Set Point Setup.
	CMD_SETPOINT_SETUP,
//{2D} Overload Counter Reading.
	CMD_READ_OVERLOAD_COUNTER,
//{2E} Lift Counter.
	CMD_READ_LIFT_COUNTER,
//{2F} Test Display Board.
	CMD_TEST_DISPLAY_BOARD,
//{30} DAC High Level Output Settings.
	CMD_DAC_HIGH_LEVEL_OUTPUT_SETTINGS,
//{31} DAC Calibration.
	CMD_DAC_CALIBRATION,
//{32} DAC Low Level Settings.
	CMD_DAC_LOW_LEVEL_SETTINGS,
//{33} DAC Manual Output.
	CMD_DAC_MANUAL_OUTPUT,
//{34} Math Channel Configuration.
	CMD_MATH_CHANNEL_CONFIG,
//{35} Rcal Value of a Loadcell.
	CMD_READ_RCAL,
//{36} Manual Total.
	CMD_MANUAL_TOTAL,
//{37} Manual Zero Total.
	CMD_MANUAL_ZERO_TOTAL,
//{38} Loadcell Bridge Inputs Status.
	CMD_READ_LOADCELL_BRIDGE_INPUTS,
//{39} Bar Graph Setup.
	CMD_BAR_GRAPH_SETUP,
//{3A} Number of Lit Segment of Bar Graph.
	CMD_GET_NUMBER_LIT_SEGMENT,
//{3B} Software Master Reset.
	CMD_SOFTWARE_MASTER_RESET,
//{3C} Non Volatile Memory Check.
	CMD_NON_VOLATILE_MEMORY_CHECK,
//{3D} Undo Zeroing a Loadcell.
	CMD_UNDO_ZERO,
//{3E} Clear All Service Counters and status -WFC- 2012-01-31 v
	CMD_CLEAR_ALL_SERVICE_CNT,
//{3F} Configure Peak Hold.
	CMD_CNFG_PEAKHOLD,
//{40} Print String Mode and Composite.
	CMD_PRINT_STR_COMPOSITE,
//{41} Print String Formatter.
	CMD_PRINT_STR_FORMATTER,
//{42} Print String Output.
	CMD_PRINT_STR_OUTPUT_QUERY,
//{43} Gross, Net, Tare, flags packet output.
	CMD_PACKET_GNT_VALUE_QUERY,
//{44} Sensor Value, cb flags packet output.
	CMD_PACKET_VALUE_CB_QUERY,
//{45} Simple sensor value, no leading spaces, no flags packet output.
	CMD_PACKET_SIMPLE_VALUE_QUERY,
//{46} Toggle High Resolution mode of a loadcell.
	CMD_PACKET_TOGGLE_HIRES_MODE,
//{47} Remove last total of a loadcell.
	CMD_REMOVE_LAST_TOTAL
};

#define  PASSWORD_NORMAL_MODE	0
#define  PASSWORD_SET_MODE		1
#define  CMD_UNLOCKED			0
#define  CMD_LOCKED				1

/// command type of CMD_DESCRIPTOR_T
#define  CMD_TYPE_NORMAL					0x00
#define  CMD_TYPE_ACT_PRE_QUERY				0x01
#define  CMD_TYPE_HANDLE_RETURN_QUERY		0x02
#define  CMD_TYPE_ACT_POST_DEFAULT			0x04
#define  CMD_TYPE_DYNAMIC_INDEX				0x08
#define  CMD_TYPE_UPDATE_ONLY_IN_CNFG_MODE	0x10
#define  CMD_TYPE_PASSWORD_PROTECTED		0x80

/// Item type data type in the lower nibble of a byte.
#define  TYPE_END      0
#define  TYPE_ACTION   0
#define  TYPE_UINT8    1
#define  TYPE_BYTE     1
#define  TYPE_INT8     2
#define  TYPE_UINT16   3
#define  TYPE_INT16    4
#define  TYPE_UINT32   5
#define  TYPE_INT32    6
#define  TYPE_FLOAT    7
#define  TYPE_STRING   8

/*! The index item type only allow at the beginning of the command table to tell the
 command parser that following variables in this command are 1D array type.

 TYPE_ACTION only allow at the beginning of the command talbe in the
 CMD_DESCRIPTOR_T structure. It has no parameter CMD_NEXT_ITEM_T, however,
 it must have at least one CMD_ACTION_ITEM_T. The command format is {cmdg}, {cmdG} or {cmd=}.

 Data structure type needs supplied method to set or retrieve contents of the data structure.
 Due to memory constrain, this feature will not implement in the 8052 family of chip.
*/

#define  TYPE_1D_INDEX 9	// for low end cpu, 1D index is in byte type ( range 0 to 255).
                            

// #define  TYPE_STRUCT   0x10

/// Item type bit field in upper nibble of byte.
#define  TYPE_READ_ONLY				0x80
#define  TYPE_READ_ONLY_PGM			0xC0
#define  TYPE_READ_ONLY_PGM_CHECK	0x40
/// REQUIRED TO CALL POST ACTION AFTER DEFAULT.
//#define  TYPE_NEED_DEFAULT_POST_ACTION	0x20
//#define  TYPE_PASSWORD_PROTECTED	0x10

#define  CMD_NO_NEXT_ITEM           0
#define  CMD_NO_ITEM				0
#define  CMD_NO_ACTION_ITEM         0


/*!
 \brief a link list structure describes a next item in a command.
*/
typedef struct CMD_NEXT_ITEM_TAG {
						/// next item type
  BYTE        type;
  //PGM_VOID_P  pNext;
						/// points to next item. Thex next item can be any type. This forms a link list data structure to support a multi variables command.
  void  	*pNext;
}CMD_NEXT_ITEM_T;

/* use the following in case the AVRStudio crashed.

struct CmdActionItem { 
  BYTE ( *pMethod) ( CMD_PRE_PARSER_STATE_T *pCmdS ); // PROGMEM;
  struct CmdActionItem  *pNext;				// next item type
};

typedef struct CmdActionItem  CMD_ACTION_ITEM_T;

*/


typedef struct CmdActionItem  CMD_ACTION_ITEM_T;

/*!
 \brief contains method of action and next action item.
*/
struct CmdActionItem {
	/// points to an action method
  BYTE ( *pMethod) ( CMD_PRE_PARSER_STATE_T *pCmdS );
	/// points to next action item. This forms a link list data structure to support a multi variables command.
  CMD_ACTION_ITEM_T  *pNext;
};


/*!
 \brief 1D index item of a command
 \note that index type item is identical to byte type item for small cpu.
 We can treat index item as byte item. Use index item type to construct 
 command table for easy reading and documentation.
 The index item type only allow at the beginning of the command table to tell the
 command parser that following variables in this command are 1D array type.
*/

typedef struct CMD_1D_INDEX_ITEM_TAG {
					/// minimum range of a variable
  BYTE    min;
					/// maximum range of a variable
  BYTE    max;
					/// default value of a variable
  BYTE    defaultV;
					/// points to a variable, 
  BYTE    *pV;
  /// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T 	nextItem;
}CMD_1D_INDEX_ITEM_T;

/*!
 \brief BYTE item in a command
*/
typedef struct CMD_BYTE_ITEM_TAG {
					/// minimum range of a variable
  BYTE    min;
					/// maximum range of a variable
  BYTE    max;
					/// default value of a variable
  BYTE    defaultV;
					/// points to a variable, 
  BYTE    *pV;
					/// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T 	nextItem;
}CMD_BYTE_ITEM_T;

/*!
 \brief signed byte item in a command
*/
typedef struct CMD_INT8_ITEM_TAG {
					/// minimum range of a variable
  INT8    min;
					/// maximum range of a variable
  INT8    max;
					/// default value of a variable
  INT8    defaultV;
					/// points to a variable, 
  INT8    *pV;
					/// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T 	nextItem;
}CMD_INT8_ITEM_T;

/*!
 \brief unsigned 16bit integer item in a command
*/
typedef struct CMD_UINT16_ITEM_TAG {
					/// minimum range of a variable
  UINT16	min;
					/// maximum range of a variable
  UINT16    max;
					/// default value of a variable
  UINT16    defaultV;
					/// points to a variable, 
  UINT16    *pV;
					/// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T 	nextItem;
}CMD_UINT16_ITEM_T;

/*!
 \brief signed 16bit integer item in a command
*/
typedef struct CMD_INT16_ITEM_TAG {
					/// minimum range of a variable
  INT16		min;
					/// maximum range of a variable
  INT16		max;
					/// default value of a variable
  INT16		defaultV;
					/// points to a variable, 
  INT16		*pV;
					/// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T 	nextItem;
}CMD_INT16_ITEM_T;


/*!
 \brief unsigned 32bit integer item in a command
*/
typedef struct CMD_UINT32_ITEM_TAG {
					/// minimum range of a variable
  UINT32	min;
					/// maximum range of a variable
  UINT32	max;
					/// default value of a variable
  UINT32	defaultV;
					/// points to a variable, 
  UINT32	*pV;
					/// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T 	nextItem;
}CMD_UINT32_ITEM_T;


/*!
 \brief signed 32bit integer item in a command
*/
typedef struct CMD_INT32_ITEM_TAG {
					/// minimum range of a variable
  INT32		min;
					/// maximum range of a variable
  INT32		max;
					/// default value of a variable
  INT32		defaultV;
					/// points to a variable, 
  INT32		*pV;
					/// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T 	nextItem;
}CMD_INT32_ITEM_T;


/*!
 \brief floating point item in a command
*/
typedef struct CMD_FLOAT_ITEM_TAG {
					/// minimum range of a variable
  float		min;
					/// maximum range of a variable
  float		max;
					/// default value of a variable
  float		defaultV;
					/// points to a variable, 
  float		*pV;
					/// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T 	nextItem;
}CMD_FLOAT_ITEM_T;


/*!
 \brief string item in a command.
  NOTE that string buffer size does not counts the null character,
  thus, the actual string buffer size is bufferSize + 1. For example, strBuf[ MAX_STR_SIZE + 1].
*/
typedef struct CMD_STRING_ITEM_TAG {
					/// minimum string buffer size
  BYTE    min;
					/// maximum string buffer size
  BYTE    max;
					/// default string of this the string buffer
  char    *pDefStr;	
					/// points to string buffer.
  char    *pStr;
					/// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T 	nextItem;
}CMD_STRING_ITEM_T;

/*!
  \brief The command descriptor describe a command.
  \par overview 
  Since both nextItem and action item are link list structure,
  it can form a multi field command with multiple action methods.
  The command module can have many commands by adding each command descriptor to an array of CMD_DESCRIPTOR_T.
  If a command is stored in location 0 of an array descriptor, then its command is {00;;;};
  if it is stored in location 254 then the command is {FE;;;}
  \note
   For assignment and action type command, the first action method will execute before process input data.
   The second action method will execute after completed processed input data.
   TYPE_ACTION only command is only allow at the beginning of the command talbe in the
   CMD_DESCRIPTOR_T structure. It has no parameter CMD_NEXT_ITEM_T, however,
   it must have at least one CMD_ACTION_ITEM_T. The command format is {cmdg}, {cmdG} or {cmd=}.

  For example:
  \code
// Commands Decriptor, each line is a root of a command.
const CMD_DESCRIPTOR_T gcCommandTable[] PROGMEM = {
   {{CMD_TYPE_NORMAL},{ TYPE_BYTE, &gcCmdBItem_000 }, { cmd_000_test_method, &gcCmdAction_001 }},		// {00}
   {{CMD_TYPE_NORMAL},{ TYPE_BYTE, &gcCmdBItem_010 }, { cmd_001_test_method, &gcCmdAction_001 }},		// {01}
   {{CMD_TYPE_NORMAL},{ TYPE_READ_ONLY | TYPE_BYTE, &gcCmdBItem_020 }, { CMD_NO_ACTION_ITEM, &gcCmdAction_001 }},
   {{CMD_TYPE_NORMAL},{ TYPE_BYTE, &gcCmdBItem_030 }, { cmd_000_test_method, CMD_NO_ACTION_ITEM }},   // {03}
};
  \endcode
  
*/

typedef struct  CMD_DESCRIPTOR_TAG {
								/// command type.
  BYTE   cmdType;
								/// contain info about next item. This forms a link list data structure to support a multi variables command.
  CMD_NEXT_ITEM_T    nextItem;
								/// contain info about action item. It is also a link list data structure to support a multi variables command.
  CMD_ACTION_ITEM_T  action;
}CMD_DESCRIPTOR_T;

/*!
 \brief an union of all command item types.
*/
/*
typedef union CMD_TYPE_UNION_U
{
	CMD_NEXT_ITEM_T			cmd_next_item;
	CMD_1D_INDEX_ITEM_T		cmd1D_index_item;
	CMD_BYTE_ITEM_T			cmd_byte_item;
	CMD_INT8_ITEM_T			cmd_i8_item;
	CMD_INT16_ITEM_T		cmd_i16_item;
	CMD_UINT16_ITEM_T		cmd_ui16_item;
	CMD_INT32_ITEM_T		cmd_i32_item;
	CMD_UINT32_ITEM_T		cmd_ui32_item;
	CMD_FLOAT_ITEM_T		cmd_float_item;
	CMD_STRING_ITEM_T		cmd_string_item;
}CMD_TYPE_UNION;
*/

typedef union CMD_ITEM_TYPE_UNION_U
{
	CMD_NEXT_ITEM_T			cmd_next_item;
	CMD_1D_INDEX_ITEM_T		cmd1D_index_item;
	CMD_BYTE_ITEM_T			cmd_byte_item;
	CMD_INT8_ITEM_T			cmd_i8_item;
	CMD_INT16_ITEM_T		cmd_i16_item;
	CMD_UINT16_ITEM_T		cmd_ui16_item;
	CMD_INT32_ITEM_T		cmd_i32_item;
	CMD_UINT32_ITEM_T		cmd_ui32_item;
	CMD_FLOAT_ITEM_T		cmd_float_item;
	CMD_STRING_ITEM_T		cmd_string_item;
}CMD_ITEM_TYPE_UNION;


#define CMD_SEND_OBJ_STATUS_SUCCESS		0x80
#define CMD_SEND_OBJ_STATUS_TIMEOUT		0x40
#define CMD_SEND_OBJ_STATUS_BUSY		0X20
#define CMD_SEND_OBJ_STATUS_REQ_SEND	0X10
#define CMD_SEND_OBJ_STATUS_UNUSED		0

#define CMD_SEND_MAX_PARAMETER_STR_LEN	30

extern	BYTE	gbCmdError;		// error status of a command of gbCmdID. For e.g. If a command is password protect, it will respond {00rCmd,error}
extern	BYTE	gbCmdID;

/// System Running Mode use by command {07=sysRunMode} private for cmdAction.c.
extern	BYTE	gbCmdSysRunMode;

// The following two variables will init by NV during power up.
// It uses as scratch variable during parsing the command.
// If the post action validated the input, then it copied into NV.
//
extern	BYTE	gbPasswordAct;
extern	BYTE	gbCmdLock;
extern	BYTE	gStrPassword[];
extern	const char PROGMEM gcStrSoftwareVersion[];

/// Acknowledgment of remote device or this device Rcal value in ASCII string.
extern	BYTE	gabAckRcalStr[ MAX_NUM_RCAL_LOADCELL ][CMD_MAX_TMP_STRING_LENGTH];

/** \def stream_router_free_stream
 * It free this stream back into the system by marked it not active.
 *
 * @param	pStream	-- pointer to io stream.
 *
 * History:  Created on 2009/04/28 by Wai Fai Chin
 */
// 2011-09-27 -WFC- #if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD )
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-09-27 -WFC-
/// For track which IO stream type has connected remote device. IO stream types are IO_STREAM_TYPE_UART, IO_STREAM_TYPE_UART_1, IO_STREAM_TYPE_UART_2, IO_STREAM_TYPE_RFMODEM, etc.. 2012-02-29 -WFC-
extern BYTE	gbIOstreamTypeConnectedRemoteDevice;
#define  cmd_act_init()   gbCmdSysRunMode = gbSysRunMode = SYS_RUN_MODE_NORMAL; gbConnectionStatus = 0; gbIOstreamTypeConnectedRemoteDevice = IO_STREAM_TYPE_UART_2;
#else
#define  cmd_act_init()   gbCmdSysRunMode = gbSysRunMode = SYS_RUN_MODE_NORMAL;
#endif

BYTE  cmd_act_execute( CMD_PRE_PARSER_STATE_T *pCmdS );
void  cmd_act_set_defaults( BYTE cmd );
void  cmd_mask_password( void );
void  cmd_send_command( BYTE streamType, BYTE destID, BYTE cmd, BYTE *pStr);
void  cmd_send_self_query_result( BYTE streamType, BYTE destID, BYTE cmd, BYTE *pStr );
BYTE  cmd_read_loadcell_rcal_value( BYTE lc, BYTE *pStr );		// 2010-11-17 -WFC-


// 2011-09-27 -WFC- #if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD )
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-09-27 -WFC-
/*!
  \brief This is a clone-able thread class for reliable sending command to a specified destination device.
  \par overview
 \note
*/

typedef struct CMD_RELIABLE_SENDING_CLASS_TAG {
							/// this thread
	struct pt 	m_pt;
							/// timer
	TIMER_T		timer;
							/// command ID to be send
	BYTE		cmd;
							/// destination ID
	BYTE		destID;
							/// stream type
	BYTE		streamType;
							/// 0x80 = success, 0x40 = request to send; 0 = unused
	BYTE		status;
							/// string buffer holds command parameter.
	BYTE		parameterStr[ CMD_SEND_MAX_PARAMETER_STR_LEN + 1];  //
}CMD_RELIABLE_SENDING_CLASS_T;


#define CMD_SEND_MANAGER_STATUS_HAS_CMDRSOBJ		0x80
#define CMD_SEND_MANAGER_STATUS_UNUSED				0

/*!
  \brief This is a clone-able thread class for reliable sending command manager. Available
  \par overview
    It manages one CMD_RELIABLE_SENDING_CLASS_T object at a time.
 \note
    If a system has more ram memory, you can cloned more CMD_RELIABLE_SENDING_MANAGER_CLASS_T objects
    for higher through put. If
*/

typedef struct CMD_RELIABLE_SENDING_MANAGER_CLASS_TAG {
							/// this thread
	struct pt 	m_pt;
							/// points at command reliable send object.
	CMD_RELIABLE_SENDING_CLASS_T		*pCmdRSobj;
							/// 0x80 = has valid CmdRSobj,
	BYTE		status;
}CMD_RELIABLE_SENDING_MANAGER_CLASS_T;


#define CMD_RELIABLE_SEND_MAX_NUM_OBJ	2

extern	CMD_RELIABLE_SENDING_CLASS_T gaCmdReliableSendObj[ CMD_RELIABLE_SEND_MAX_NUM_OBJ ];


#define CMD_RELIABLE_SEND_MANAGER_MAX_NUM_OBJ	2

extern	CMD_RELIABLE_SENDING_MANAGER_CLASS_T gaCmdReliableSendManagerObj[ CMD_RELIABLE_SEND_MANAGER_MAX_NUM_OBJ ];


#define	CMD_ACT_CONNECT_STATUS_NO_ACK_FROM_REMOTE		0x01
#define	CMD_ACT_CONNECT_STATUS_RECEIVED_FROM_REMOTE		0x02

// Receive
/// a remote meter is not able to connect to ScaleCore. Note that future version can handle connection status of many device. Right now, any device communication timeout is no connection.
extern	BYTE	gbConnectionStatus;

// Acknowledgment of remote device Rcal value in ASCII string.
// extern	BYTE	gabAckRcalStr[ MAX_NUM_RCAL_LOADCELL ][CMD_MAX_TMP_STRING_LENGTH];

/// Acknowledgment from a remote device of its bargraph sensor id.
extern	BYTE  gbAckBargraphSensorID;

/// Acknowledgment from a remote device of its number of lit segment of bargraph of its sensor value.
extern	BYTE  gbAckBargraphNumLitSeg;

/// Acknowledgment of remote device system error code for user.
// 2012-02-28 -WFC- extern	BYTE	gbAckSysErrorCodeForUser;			// 2011-01-03 -WFC-

void	cmd_reliable_send_commands_main_tasks( void );
void	cmd_reliable_send_commands_main_init( void );
BYTE	cmd_get_cmdRSobj_of_given_type( CMD_RELIABLE_SENDING_CLASS_T **ppCmdRSobj, BYTE type );
void	cmd_cmdRSobj_garbage_collector( void );
BYTE	cmd_setup_command_for_cmdRSobj( BYTE streamType, BYTE destID, BYTE cmd, BYTE *pStr);

/** \def cmd_get_an_unused_cmdRSobj
 * It trys to find an unused cmdRSobj and saved its pointer to ppCmdRSobj.
 *
 * @param	ppCmdRSobj	-- pointer of pointer to a cmdRSobj. It is for saved the pointer of an unused cmdRSobj.
 * @return  PASSED if it found an unused cmdRSobj, else FAILED.
 *
 * History:  Created on 2010/07/20 by Wai Fai Chin
 */

#define cmd_get_an_unused_cmdRSobj( ppCmdRSobj )		cmd_get_cmdRSobj_of_given_type( (ppCmdRSobj), CMD_SEND_OBJ_STATUS_UNUSED)


/** \def cmd_get_a_request_send_cmdRSobj
 * It trys to find a request_send cmdRSobj and saved its pointer to ppCmdRSobj.
 *
 * @param	ppCmdRSobj	-- pointer of pointer to a cmdRSobj. It is for saved the pointer of a request_send cmdRSobj.
 * @return  PASSED if it found a request_send cmdRSobj, else FAILED.
 *
 * History:  Created on 2010/07/20 by Wai Fai Chin
 */

#define cmd_get_a_request_send_cmdRSobj( ppCmdRSobj )		cmd_get_cmdRSobj_of_given_type( (ppCmdRSobj), CMD_SEND_OBJ_STATUS_REQ_SEND)


#endif

#endif
//@}
