/*! \file print_string.h \brief user defined print string related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2011 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: independent
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2011/06/20 by Wai Fai Chin
// 
/// \ingroup dataOutputs
/// \defgroup print_string PrintString manages data output to all IO_STREAM(print_string.c)
/// \code #include "print_string.h" \endcode
/// \par Overview
/// This module process user defined print strings and formated output based on configured print strings.
//
// ****************************************************************************
//@{


#ifndef MSI_PRINT_STRING_H
#define MSI_PRINT_STRING_H

#include  "dataoutputs.h"
#include  "nv_cnfg_mem.h"
#include  "vs_sensor.h"			// 2012-03-22 -WFC-
/*!
 *  User defined print string formatters reserved key characters. They are case sensitive.
 *  R## -- Right justify length of next field. Example R5 means next item maximum width is 5 characters with padding leading spaces if needed.
 *         R0 means variable width without justify. It is only valid for one next field.
 *         ## maximum value is 12.
 *  L## -- Left justify length of next field. Example R5 means next item maximum width is 5 characters with padding trailing spaces if needed.
 *         L0 means variable width without justify. It is only valid for one next field.
 *         ## maximum value is 12.
 *  S## -- Sensor number is always require for V, I, M, N and U fields. It tells where these fields belong to which sensor.
 *         If formatter does not specified S##, then it assumed all other specifiers are referred to Sensor#0.
 *         Once S## is specified, the following V, I, M, N and U fields are referred to current S## until new S##.
 *         ## maximum value is 15.
 *  T#  -- Value of data type. #: 0==GROSS, 1==NET, 2==TOTAL, 3==TARE, 4==ZERO, 5==PEAK, 6==ADC COUNT, 7==CURRENT MODE. 8 == Total count.
 *         Example, T1 means following value field is for net weight value.
 *         # maximum number of data type is 8.
 *
 *  The following specifiers are print field control by the above specifiers:
 *
 *  V	-- Value of a specified data type and sensorID. Its precision output is based on configured count by d.
 *         If data type is no specified before V, default type is GROSS.
 *         If sensor number is no specified before V, default sensor number is 0.
 *  I	-- Same as V with output integer portion of data only.
 *  M   -- Mode string name of data type. It is a 5 characters fixed width with trailing padding spaces.
 *  	   Its name is based on data type, for example, T1 for NET mode, The M field will print "NET  ", without quotes of course.
 *         If data type is no specified before M, default type is GROSS.
 *  N	-- Name of sensor. Its width could be control by optional R## or L##.
 *         If sensor number is no specified before N, default sensor number is 0.
 *  U   -- Unit, always two Characters. kg, lb, T=Metric Ton, TN = English Ton,
 *  	   It is current unit of a specified sensor.
 *         If sensor number is no specified before U, default sensor number is 0.
 *
 *  The following specifiers are print field and it is independent from other specifiers.
 *
 *  _   -- a space character.
 *  r   -- carriage return.
 *  n   -- new line feed.
 *  ^   -- a string quote. ^ABC D^ it means "ABC D".
 *
 *  Put it all together:
 *  R7S0T0V_U_Mrn == 1st field is right justify with 7 characters width, value from Sensor 0 "S0",
 *             data type is GROSS mode "T0", data value precision based on couuntby "V",
 *             follow by a space '_', Unit "U", space '_',"GROSS" then carriage return and a line feed.
 *             It outputs "  12345 lb GROSS" and then cr LF without quotes of course.
 *
 *  S0T0MR7V_Urn == It outputs "GROSS  12345 lb" and then cr LF without quotes of course.
 *
 *  Let say sensor name is "WestSide"
 *  S0R4NT0R7V_U_Mrn == It outputs "West  12345 lb GROSS" and then cr LF without quotes of course.
 *                  Note that it only print sensor name "West" because R4 tells it outputs maximum 4 characters only.
 *
 *
 *  ^Crane:1 ^S0T0R7V_U_Mrn == 1st field is a string "Crane:1 ".
 *            It outputs "Crane:1   12345 lb GROSS" and then carriage return, line feed without quotes of course.
 *
 *
 */

/* 2011-07-25 -WFC- v moved to nv_cnfg_mem.h file:
/// Maximum number of formatter. Note: must not > 9.
#define PRINT_STRING_MAX_NUM_FORMATER			7
#define PRINT_STRING_MAX_FORMATER_LENGTH		18
#define PRINT_STRING_MAX_FIELD_LENGTH			18

#define PRINT_STRING_MAX_NUM_COMPOSITE			5
#define PRINT_STRING_MAX_COMPOSITE_LENGTH		5
2011-07-25 -WFC- */

/// Maximum length of output string per formatter.
#define PRINT_STRING_MAX_OUTPUT_LENGTH			40

/// print string control mode
#define PRINT_STRING_CTRL_MODE_DISABLED			0
// Key press or host command
#define PRINT_STRING_CTRL_MODE_COMMAND			1
#define PRINT_STRING_CTRL_MODE_STABLE_LOAD		2
#define PRINT_STRING_CTRL_MODE_CONTINUOUS		3

#define PRINT_STRING_FIELD_TYPE_NEW				0
#define PRINT_STRING_FIELD_TYPE_STRING			1
#define PRINT_STRING_FIELD_TYPE_FIELD_WIDTH		2
#define PRINT_STRING_FIELD_TYPE_SENSOR_NUMBER	3
#define PRINT_STRING_FIELD_TYPE_VALUE_TYPE		4

/* 2011-07-25 -WFC- v moved to nv_cnfg_mem.h file:
/// print string control mode
extern BYTE	gabPrintStringCtrlModeFNV[ MAX_NUM_STREAM_LISTENER ];

/// print string composite for each listener as defined in command {1A}.
extern UINT32	gaulPrintStringCompositeFNV[ MAX_NUM_STREAM_LISTENER ];

/// print string interval in seconds for each listener as defined in command {1A}.
extern UINT16	gawPrintStringIntervalFNV[ MAX_NUM_STREAM_LISTENER ];

/// User defined print string formatter
extern BYTE	gabPrintStringUserFormatterFNV[ PRINT_STRING_MAX_NUM_FORMATER ][ PRINT_STRING_MAX_FORMATER_LENGTH + 1 ];
 2011-07-25 -WFC- v */

#define PRINT_STRING_BUILD_SEND_STATUS_BAD_COMPOSITE	0xFF
#define PRINT_STRING_BUILD_SEND_STATUS_SUCCESS			0x80
#define PRINT_STRING_BUILD_SEND_STATUS_TIMEOUT			0x40
#define PRINT_STRING_BUILD_SEND_STATUS_BUSY				0X20
#define PRINT_STRING_BUILD_SEND_STATUS_REQ_SEND			0X10
#define PRINT_STRING_BUILD_SEND_STATUS_UNUSED			0

/*!
  \brief This is a clone-able thread class for send out built print string to a device specified by stream type and destination ID.
  \par overview
	  It just know how to build and send a user defined print string.
	  It does not know when to start build and send. Other object tell it to start to build and send.
 \note
*/

typedef struct PRINT_STRING_BUILD_SEND_CLASS_TAG {
							/// this thread
	struct pt 	m_pt;
							/// timer
	TIMER_T		timer;
							/// destination ID
	BYTE		destID;		// RF, ip may need destID, but single point to point does not require it.
							/// stream type
	BYTE		streamType;
							/// 0x80 = success, 0x40 = request to send; 0 = unused
	BYTE		status;
							/// number of formatter in this composite buffer
	BYTE		numFormatter;
							/// current index of composite buffer to process.
	BYTE		compositeIndex;
							/// composite of formatters string. 1 base system because no leading 0 in a number.
	BYTE		compositeFmt[ PRINT_STRING_MAX_COMPOSITE_LENGTH ];  //
}PRINT_STRING_BUILD_SEND_CLASS_T;


#define PRINT_STRING_MANAGER_STATUS_HAS_BNSOBJ		0x80
#define PRINT_STRING_MANAGER_STATUS_UNUSED			0

// 2012-03-21 -WFC- v
/*!
  \brief This is a clone-able thread class for query data from a remote device.
  \par overview
	  It query data based on user defined print string.
 \note
*/

typedef struct PRINT_STRING_QUERY_REMOTE_DATA_CLASS_TAG {
								/// this thread
	struct pt 	m_pt;
								/// timer
	TIMER_T		timer;
								/// pointer to current focus virtual sensor.
	VS_SENSOR_INFO_T	*pVs;
	BYTE	*pFmtCh;			// point at formatter string char
	BYTE	formatter_strLen;	// Formatter string length
	BYTE	bSpecifier;
	BYTE	sensorNum;
	BYTE	valueType;
	BYTE	lookForFieldType;
								/// status of query result.
	BYTE	status;
	BYTE	digitLen;
	INT8	fieldWidth;			// 0 == variable, Negative number == Left Justify, Positive number == Right Justify.
}PRINT_STRING_QUERY_REMOTE_DATA_CLASS_T;
// 2012-03-21 -WFC- ^


/*!
  \brief This is a clone-able thread class for print string manager.
  \par overview
    It manages one PRINT_STRING_BUILD_SEND_CLASS_T object at a time.
 \note
    If a system has more ram memory, you can cloned more PRINT_STRING_BUILD_SEND_CLASS_T objects
    for higher through put.
    2012-03-21 -WFC- Added item PRINT_STRING_QUERY_REMOTE_DATA_CLASS_T *pQRDobj;
*/

typedef struct PRINT_STRING_MANAGER_CLASS_TAG {
														/// this thread
	struct pt 	m_pt;
														/// points at build and send object.
	PRINT_STRING_BUILD_SEND_CLASS_T		*pBnSobj;
														/// points at query remote data object. 2012-03-21 -WFC-
	PRINT_STRING_QUERY_REMOTE_DATA_CLASS_T *pQRDobj;
														/// 0x80 = has valid pBnSobj
	BYTE		status;
}PRINT_STRING_MANAGER_CLASS_T;

#define PRINT_STRING_BUILD_SEND_MAX_NUM_OBJ		2
extern	PRINT_STRING_BUILD_SEND_CLASS_T 	gaPrintStringBuildSendObj[ PRINT_STRING_BUILD_SEND_MAX_NUM_OBJ ];

#define PRINT_STRING_MANAGER_MAX_NUM_OBJ		2
extern	PRINT_STRING_MANAGER_CLASS_T 		gaPrintStringManagerObj[ PRINT_STRING_MANAGER_MAX_NUM_OBJ ];

// 2012-03-21 -WFC- v
#define PRINT_STRING_QUERY_RM_DATA_MAX_NUM_OBJ		PRINT_STRING_MANAGER_MAX_NUM_OBJ
/// clone-able thread class for query remote data. It is adapted by PRINT_STRING_MANAGER_CLASS_T.
extern PRINT_STRING_QUERY_REMOTE_DATA_CLASS_T gaQueryRemoteDataObj[ PRINT_STRING_QUERY_RM_DATA_MAX_NUM_OBJ ];
// 2012-03-21 -WFC- ^

/// Is print string command pressed. 2015-09-24 -WFC-
extern BYTE gbIsPrintStr_CmdPressed;

void  print_string_1st_init_formatters( void );
BYTE  print_string_build_validate_composite( BYTE *pOutStr, UINT32 lComposite );
void  print_string_init_formatters( void );
void  print_string_main_init( void );
BYTE  print_string_get_BnSobj_of_given_type( PRINT_STRING_BUILD_SEND_CLASS_T **ppBnSobj, BYTE type );
void  print_string_main_tasks( void );
BYTE  print_string_setup_for_BnSobj( BYTE listener );
BYTE  print_string_validate_formatter( BYTE *pStr );

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )			// 2012-03-23 -WFC-
BYTE  print_string_is_main_thread_active( void );				// 2012-03-26 -WFC-
#endif

/** \def print_string_get_an_unused_BnSobj
 * It tries to find an unused BnSobj and saved its pointer to BnSobj.
 *
 * @param	ppBnSobj	-- pointer of pointer to a ppBnSobj. It is for saved the pointer of a ppBnSobj.
 * @return  PASSED if it found an unused BnSobj, else FAILED.
 *
 * History:  Created on 2011/06/23 by Wai Fai Chin
 */

#define print_string_get_an_unused_BnSobj( ppBnSobj )		print_string_get_BnSobj_of_given_type( (ppBnSobj), PRINT_STRING_BUILD_SEND_STATUS_UNUSED)


/** \def print_string_get_a_request_send_BnSobj
 * It tries to find a request_send BnSobj and saved its pointer to BnSobj.
 *
 * @param	ppBnSobj	-- pointer of pointer to a ppBnSobj. It is for saved the pointer of a ppBnSobj.
 * @return  PASSED if it found a request_send BnSobj, else FAILED.
 *
 * History:  Created on 2011/06/23 by Wai Fai Chin
 */

#define print_string_get_a_request_send_BnSobj( ppBnSobj )		print_string_get_BnSobj_of_given_type( (ppBnSobj), PRINT_STRING_BUILD_SEND_STATUS_REQ_SEND)



#endif	// MSI_PRINT_STRING_H
//@}
