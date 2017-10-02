/*! \file cmdparser.h \brief command parser functions.*/
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
// 2009/03/31 -WFC- modified it to include source and destination ID.
// 
/// \ingroup Application
/// \defgroup cmd_parser Commands Parser related definition.
/// \code #include "cmdparser.h" \endcode
/// \see cmdparser, commonlib and calibrate modules
/// \par Overview
///     It parse user entered command from an IO_STREAM object and handover
///  the result over to cmdaction module for execution based
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




#ifndef MSI_CMDPARSER_H
#define MSI_CMDPARSER_H

#include  "config.h"
#include  "stream_router.h"
// #include  "nvmem.h"			// for NvRAM_EEMEM_FAIL etc..


#define  CMD_PARSER_MAX_STR_LEN   40
#define  CMD_START_CHAR        '{'
#define  CMD_END_CHAR          '}'
#define  CMD_ITEM_DELIMITER    ';'

#define  CMD_STATE_LOOK_FOR_START      1
#define  CMD_STATE_LOOK_FOR_CMD        2
#define  CMD_STATE_LOOK_FOR_PARAMETERS 3
#define  CMD_STATE_ACKED		       0XFE

#define  CMD_STATUS_NO_ERROR							0
#define  CMD_STATUS_ERROR_INDEX							1
#define  CMD_STATUS_ERROR_2ND_INDEX						2
#define  CMD_STATUS_OUT_RANGE_INPUT						3
#define  CMD_STATUS_READ_ONLY							4
#define  CMD_STATUS_ERROR_MIN_GT_MAX_SWAPPED			5	// minimum value greater than maximum. Its values are auto corrected by swapped them. resolved problem report #932. 2011-11-04 -WFC-
#define  CMD_STATUS_ERROR_CANNOT_UPDATE					20	// because other condition is not met.
#define  CMD_STATUS_ERROR_UNCAL_SENSOR					21	// Sensor has not been calibrated.
#define  CMD_STATUS_ERROR_RCAL_DISABLED					22	// Rcal input disabled for this sensor.
#define  CMD_STATUS_ERROR_INVALID_COUNTBY				23	// Invalid countby number. (valid countby are 1, 2, 5). 2010-09-10 -WFC-
#define  CMD_STATUS_ERROR_SENSOR_NOT_SUPPORTED			24	// This device does not support or have this type of sensor. 2011-05-18 -WFC-
#define  CMD_STATUS_ERROR_INVALID_STRING_FORMATTER		25	// Invalid print string formatter. 2011-06-20 -WFC-
#define  CMD_STATUS_ERROR_FORMATTER_EXPECT_DIGIT		26	// one of print string formatter's field expect digit bug but got a none digit. 2011-06-20 -WFC-
#define  CMD_STATUS_ERROR_FORMATTER_WIDTH_TOO_BIG		27	// print string formatter's field width to big. 2011-06-20 -WFC-
#define  CMD_STATUS_ERROR_FORMATTER_GEN_STRING_TOO_LONG	28	// print string formatter's will generated a string longer than allocated buffer. 2011-06-20 -WFC-
#define  CMD_STATUS_ERROR_INVALID_SENSOR_NUMBER			29	// invalid sensor number. 2011-06-20 -WFC-
#define  CMD_STATUS_ERROR_INVALID_VALUE_TYPE			30	// invalid value type. 2011-06-20 -WFC-
#define  CMD_STATUS_ERROR_FEATURE_NOT_SUPPORT			31	// This product or version does not support this feature. 2011-07-26 -WFC-
#define  CMD_STATUS_ERROR_ZERO_IN_PRINT_STRING_COMPOSITE 32	// Zero digit(s) in print string composite.  2011-07-26 -WFC-
#define  CMD_STATUS_ERROR_OUT_OF_PRINT_STRING_OBJECT	33	// Running out of print string object due to memory limitation at this moment.  2011-07-26 -WFC-
#define  CMD_STATUS_ERROR_NOT_ALLOW						34	// this operation is not allow at this state. 2015-05-12 -WFC-
#define  CMD_STATUS_CMD_LOCKED							40
#define  CMD_STATUS_WRONG_PASSWORD						41
#define  CMD_STATUS_NOT_ALLOW_TO_SELFTEST				42	//Because self test mode can only change from normal system running mode.
#define  CMD_STATUS_NOT_ALLOW_TO_CONFIG					43	//Because configuration command can only operate in system configuration mode.
#define  CMD_STATUS_OPERATION_FAILED_TIMEOUT			44	// Operation failed because of timeout during this task. 2012-03-20 -WFC-

#define  CMD_STATUS_SYS_USER_ERROR_CODE_OFFSET			60	// Use this offset to add SYS_USER_ERROR_CODE_. 2012-02-23 -WFC-
															// 60 - 60 = 0 == SYS_USER_ERROR_CODE_NO_ERROR
															// 61 - 60 = 1 == SYS_USER_ERROR_CODE_UNDEF_MODEL
															// see following SYS_USER_ERROR_CODE_ section for more detail. It is mainly reported of cmd{45} error codes.
//	Added CMD_STATUS_SYS_USER_ERROR_CODE_OFFSET to the following SYS_USER_ERROR_CODE_
//	#define  SYS_USER_ERROR_CODE_NO_ERROR           0
//	#define  SYS_USER_ERROR_CODE_UNDEF_MODEL        1
//	#define  SYS_USER_ERROR_CODE_LC_ERROR           2
//	#define  SYS_USER_ERROR_CODE_LC_DISABLED        3
//	#define  SYS_USER_ERROR_CODE_IN_CAL             4
//	#define  SYS_USER_ERROR_CODE_UN_CAL             5
//	#define  SYS_USER_ERROR_CODE_WRONG_MATH_EXPRS   6
//	#define  SYS_USER_ERROR_CODE_ADC_CHIP_BUSY      7
//	#define  SYS_USER_ERROR_CODE_OVERLOAD           8
//	#define  SYS_USER_ERROR_CODE_OVERRANGE          9
//	#define  SYS_USER_ERROR_CODE_UNDERLOAD          10
//	#define  SYS_USER_ERROR_CODE_UNDERRANGE         11
//	#define  SYS_USER_ERROR_CODE_UNDER_VOLTAGE      12
//	#define  SYS_USER_ERROR_CODE_OVER_TEMPERATURE   13
//	#define  SYS_USER_ERROR_CODE_UNDER_TEMPERATURE  14
//	#define  SYS_USER_ERROR_CODE_MAX_ITEM           15

#define  CMD_STATUS_SYS_USER_ERROR_CODE_MAX				80	// allocated number 60 to 80 for SYS_USER_ERROR_CODE_

#define  CMD_STATUS_NO_SUCH_SUB_CMD		0xA0
#define  CMD_STATUS_NO_SUCH_CMD			0xA1
#define  CMD_STATUS_PRE_TO_MANY_CHAR	0xA2
#define  CMD_STATUS_PRE_WRONG_CMD_FIELD	0xA3
#define  CMD_STATUS_NV_MEMORY_FAIL		0xED	//All non volatile memory failed that included EEMEM and FRAM etc..
#define  CMD_STATUS_EEMEM_FAIL			0xEE	//NVRAM_EEMEM_FAIL
#define  CMD_STATUS_FRAM_FAIL			0xEF	//NVRAM_FRAM_FAIL

#define  CMD_STATUS_CAL_ERROR_NOT_ALLOW					0xFF
#define  CMD_STATUS_CAL_ERROR_CANNOT_CHANGE_COUNTBY		0xEC
#define  CMD_STATUS_CAL_ERROR_WRONG_CAL_SEQUENCE		0xEB
#define  CMD_STATUS_CAL_ERROR_INVALID_CAL_INFO			0xEA
#define  CMD_STATUS_CAL_ERROR_WRONG_SENSOR_ID			0xE9
#define  CMD_STATUS_CAL_ERROR_LESS_THAN_4CNT_PER_D		0xE3			// 2011-02-01 -WFC-
#define  CMD_STATUS_CAL_ERROR_DIFFERENT_VALUE_ON_SAME_LOAD	0xE2		// 2011-01-12 -WFC-
#define  CMD_STATUS_CAL_ERROR_CANNOT_CHANGE_UNIT		0xE1
#define  CMD_STATUS_CAL_ERROR_NEED_UNIT					0xE0
#define  CMD_STATUS_CAL_ERROR_NEED_UNIT_CAP				0xDF
#define  CMD_STATUS_CAL_ERROR_NEED_COUNTBY				0xDD
#define  CMD_STATUS_CAL_ERROR_FAILED_CAL				0xDC
#define  CMD_STATUS_CAL_ERROR_TEST_LOAD_GT_CAPACITY		0xD3			// 2011-11-03 -WFC- implement for problem report #928.
#define  CMD_STATUS_CAL_ERROR_TEST_LOAD_TOO_SMALL		0xD2
#define  CMD_STATUS_CAL_ERROR_INVALID_CAPACITY			0xD1
#define  CMD_STATUS_CAL_ERROR_CANNOT_CHANGE_CAPACITY	0xD0

// 2014-11-12 -WFC- v
#define  CMD_OP_CODE_CLEAR_USER_LIFT_COUNTER	10
#define  CMD_OP_CODE_SET_TMP_POWER_SAVING		11
// 2014-11-12 -WFC- ^



// 2009/04/09 -WFC-
/*!
  \brief preprocess command parser class.
  \par overview
  It uses for preprocess a command from an IO_STREAM object. Once it found a completed command,
  it passes the command to command action module for further processing. The command
  action module will validate the command based on the command type and taking
  action according the command descriptor and its input parameters.
 \note
	An abstract output stream object must constructed before use this class.
*/
 
typedef struct CMD_PRE_PARSER_STATE_TAG {
							/// streamObjNum uses as index link to gabStreamConsoleTypeFNV[], gabStreamOutSensorFNV[], gabStreamOutModesFNV[], gabStreamOutIntervalFNV[].
  BYTE    streamObjNum;
							/// command parser state. e.g. CMD_PARSER_LOOK_FOR, has a valid command, ack this command, etc.
  BYTE    state;
							/// command parser status, 0 = no error;
  BYTE    status;
							/// command id
  BYTE    cmd;
							/// index for an index type command. Index type only allow in the first parameter field and treats the rest of different type parameters as an 1D array type of byte, integer, etc...; it is index by this index. 1D array only is due to memory constrain.
  BYTE    index;
							/// maximum index range. 0 means this command is normal variables, not 1D array type.
  BYTE    maxIndexRange;
							/// string buffer holds user input command string.
  char    strBuf[ MAX_STREAM_CIR_BUF_SIZE ];  //
  //						/// points to an abstract in stream object. User must supply a concret stream before use this object.
  // IO_STREAM_CLASS	*piStream;
							/// points to an abstract out stream object. User must supply a concret stream before use this object.
  IO_STREAM_CLASS	*poStream;
}CMD_PRE_PARSER_STATE_T;


///////////////////////////////////////////////////////////////////////////////
//                      Test related items for this module.                  //
///////////////////////////////////////////////////////////////////////////////


#endif
//@}
