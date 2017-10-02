/*! \file print_string.c \brief user defined print string related functions.*/
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
/// This module process user defined print strings and formated output based on configured print strings.
//
// ****************************************************************************


#include  "config.h"
#include  <stdio.h>

#include  "print_string.h"
#include  "sensor.h"
#include  "cmdparser.h"
#include  "commonlib.h"
#include  "timer.h"
#if ( CONFIG_PRODUCT_AS	!= CONFIG_AS_IDSC )				// 2011-12-02 -WFC-
#include  "panelmain.h"
#endif
#include  "nv_cnfg_mem.h"

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

/* 2011-07-25 -WFC- v moved to nv_cnfg_mem.c file:
/// print string control mode
BYTE	gabPrintStringCtrlModeFNV[ MAX_NUM_STREAM_LISTENER ];

/// print string composite for each listener as defined in command {1A}.
UINT32	gaulPrintStringCompositeFNV[ MAX_NUM_STREAM_LISTENER ];

/// print string interval in seconds for each listener as defined in command {1A}.
UINT16	gawPrintStringIntervalFNV[ MAX_NUM_STREAM_LISTENER ];

/// User defined print string formatter
BYTE	gabPrintStringUserFormatterFNV[ PRINT_STRING_MAX_NUM_FORMATER ][ PRINT_STRING_MAX_FORMATER_LENGTH + 1 ];
 2011-07-25 -WFC- ^ */

// 2011-10-28 -WFC-
/// print string loadcell lift counter for print on new stable loaded mode. It stored LSB of lift counter of loadcell number specified in cmd{1A}, gabListenerWantSensorFNV[].
BYTE	gabPrintStringLiftCnt[ MAX_NUM_STREAM_LISTENER ];

/// seconds timer for control continues interval output mode of a print string object.
TIMER_Sec_16_T gaPrintStringTimer[ MAX_NUM_STREAM_LISTENER ];

/// milliseconds timer for control continues interval output mode of a print string object. 2012-09-24 -WFC-
TIMER_T gaPrintStringTimerMs[ MAX_NUM_STREAM_LISTENER ];

/// clone-able thread class for send out built print string to a device specified by stream type and destination ID.
PRINT_STRING_BUILD_SEND_CLASS_T 	gaPrintStringBuildSendObj[ PRINT_STRING_BUILD_SEND_MAX_NUM_OBJ ];

/// clone-able thread class for print string manager
PRINT_STRING_MANAGER_CLASS_T 		gaPrintStringManagerObj[ PRINT_STRING_MANAGER_MAX_NUM_OBJ ];

/// Is print string command pressed. 2015-09-24 -WFC-
BYTE gbIsPrintStr_CmdPressed;


// default print string formatters
// 2011-07-25 -WFC- moved to nv_cnfg_mem.c and h files, const char gcStrDefPrintFormatter0[]		PROGMEM = "R7S0T7V_U_Mrn";		// Right justify 7 chars, Sensor0 current mode weight, space, unit, space, Weight Mode string, carriage return, line feed.
const char gcStrDefPrintFormatter1[]		PROGMEM = "R7S0T1V_U_Mrn";		// Right justify 7 chars, Sensor0 Net weight, space, unit, space, Weight Mode string, carriage return, line feed.
const char gcStrDefPrintFormatter2[]		PROGMEM = "R7S0T0V_U_Mrn";		// Right justify 7 chars, Sensor0 Gross weight, space, unit, space, Weight Mode string, carriage return, line feed.
const char gcStrDefPrintFormatter3[]		PROGMEM = "R7S0T3V_U_Mrn";		// Right justify 7 chars, Sensor0 Tare weight, space, unit, space, Weight Mode string, carriage return, line feed.
const char gcStrDefPrintFormatter4[]		PROGMEM = "R9S0T2V_U^ TTL^rnn";	// Right justify 9 chars, Sensor0 Total weight, space, unit, " TTL", carriage return, line feed, line feed.
const char gcStrDefPrintFormatter5[]		PROGMEM = "R13S0T8V_M_rn";		// Right justify 13 chars, Sensor0 Total counts, space, Weight Mode string, space, carriage return, line feed.
const char gcStrDefPrintFormatter6[]		PROGMEM = "R6S0T7Irn";			// Right justify 6 chars, Sensor0 current mode weight in integer, carriage return, line feed.

// default print string formatters table
PGM_P gcaStrDefPrintFormatters[] PROGMEM = {
		gcStrDefPrintFormatter0, gcStrDefPrintFormatter1, gcStrDefPrintFormatter2, gcStrDefPrintFormatter3,
		gcStrDefPrintFormatter4, gcStrDefPrintFormatter5, gcStrDefPrintFormatter6
};


const char gcStr_lb[]	PROGMEM = "lb";
const char gcStr_kg[]	PROGMEM = "kg";
const char gcStr_t[]	PROGMEM = "t ";
const char gcStr_Mt[]	PROGMEM = "Mt";
const char gcStr_oz[]	PROGMEM = "oz";
const char gcStr_g[]	PROGMEM = "g ";
const char gcStr_kN[]	PROGMEM = "kN";
const char gcStr_V[]	PROGMEM = "V ";
const char gcStr_A[]	PROGMEM = "A ";
const char gcStr_C[]	PROGMEM = "C ";
const char gcStr_F[]	PROGMEM = "F ";
const char gcStr_Ke[]	PROGMEM = "Ke";
const char gcStr_Lx[]	PROGMEM = "Lx";
const char gcStr_2sp[]	PROGMEM = "  ";
PGM_P gcPrintStringUnitNameTbl[]	PROGMEM = {gcStr_lb, gcStr_kg, gcStr_t, gcStr_Mt, gcStr_oz, gcStr_g, gcStr_kN, gcStr_V, gcStr_A, gcStr_C, gcStr_F, gcStr_Ke, gcStr_Lx, gcStr_2sp};

const char gcPsStr_GROSS[]	PROGMEM = "GROSS";
const char gcStr_NET[]		PROGMEM = "NET  ";
const char gcStr_TOTAL[]	PROGMEM = "TOTAL";
const char gcStr_TARE[]		PROGMEM = "TARE ";
const char gcStr_ZERO[]		PROGMEM = "ZERO ";
const char gcStr_PEAK[]		PROGMEM = "PEAK ";
const char gcStr_AdcCN[]	PROGMEM = "AdcCN";
//gcStr5Spaces for current mode
const char gcStr_T_CNT[]	PROGMEM = "T-CNT";
PGM_P gcPrintStringValueModeNameTbl[]	PROGMEM = {gcPsStr_GROSS, gcStr_NET, gcStr_TOTAL, gcStr_TARE, gcStr_ZERO, gcStr_PEAK, gcStr_AdcCN, gcStr5Spaces, gcStr_T_CNT};

/// feedback print string to pin point which formatter has error.
const char gcPrintFormatterErrorMsg[]	PROGMEM = "Formatter#%c Error\n\r";			// Right justify 6 chars, Sensor0 current mode weight in integer, carriage return, line feed.


// private methods or functions:
BYTE  print_string_format_unit_name( BYTE *pOutStr, BYTE sn, BYTE valueType );
BYTE  print_string_format_value_mode( BYTE *pOutStr, BYTE sn, BYTE valueType );
BYTE  print_string_build_string_from_a_formatter( BYTE *pFmt, BYTE *pOutStr, BYTE *pOutLen );
BYTE  print_string_format_value( BYTE *pOutStr, BYTE sn, BYTE valueType, INT8 fieldWidth, BYTE numberType );
void  print_string_built_and_send( PRINT_STRING_BUILD_SEND_CLASS_T *pBnSobj, IO_STREAM_CLASS	*pOstream );

char  print_string_main_thread( BYTE managerID );
char  print_string_build_send_thread( PRINT_STRING_BUILD_SEND_CLASS_T *pBnSobj );

/**
 * It initialized print string formatters when app code runs the very 1st time for the device.
 *
 * @return none
 *
 * History:  Created on 2011/06/20 by Wai Fai Chin
 * 2011-08-17 -WFC- Ensured number of formatter within range. This fixed memory corruption bug after master reset.
 * 2015-09-25 -WFC- Changed formatter composite from 12345 to 1.
 */

void  print_string_1st_init_formatters( void )
{
	PGM_P	ptr_P;
	BYTE	*destPtr;
	BYTE	i;

	// default formatters
	for ( i=0; i < PRINT_STRING_MAX_NUM_FORMATER; i++) {
		memcpy_P ( &ptr_P, &gcaStrDefPrintFormatters[ i ], sizeof(PGM_P));
		destPtr = &gabPrintStringUserFormatterFNV[i][0];
		strcpy_P( destPtr, ptr_P);
	}

	for ( i=0; i < MAX_NUM_STREAM_LISTENER; i++ ) {
		// gaulPrintStringCompositeFNV[ i ] = 12345;
		gaulPrintStringCompositeFNV[ i ] = 1;			// 2015-09-25 -WFC-
		gabPrintStringCtrlModeFNV[ i ] = PRINT_STRING_CTRL_MODE_COMMAND;
		gawPrintStringIntervalFNV[ i ] = 2;
	}

} // end print_string_1st_init_formatters()

/**
 * It initialized print string formatters when app code runs the very 1st time for the device.
 *
 * @return none
 *
 * History:  Created on 2011/06/20 by Wai Fai Chin
 */

void  print_string_init_formatters( void )
{
	// print_string_1st_init_formatters();			//TODO: to be remove
	//TODO: recall from non volatile memory.
} // end print_string_init_formatters()

/**
 * It initialized print string formatters, timer and objects
 *
 * @return none
 *
 * History:  Created on 2011/06/23 by Wai Fai Chin
 * 2011-10-28 -WFC- init gabPrintStringLiftCnt[];
 * 2012-09-24 -WFC- init gaPrintStringTimerMs[];
 * 2015-09-24 -WFC- init gbIsPrintStr_CmdPressed;
 */

void  print_string_main_init( void )
{
	BYTE	i;

	// print_string_init_formatters();

	// walk through all print string manager objects
	for ( i = 0; i < PRINT_STRING_MANAGER_MAX_NUM_OBJ; i++) {
		PT_INIT( &(gaPrintStringManagerObj[i].m_pt));
		gaPrintStringManagerObj[i].status = PRINT_STRING_MANAGER_STATUS_UNUSED;
	}

	// walk through all BnSobj objects
	for ( i = 0; i < PRINT_STRING_BUILD_SEND_MAX_NUM_OBJ; i++) {
		PT_INIT( &(gaPrintStringBuildSendObj[i].m_pt));
		gaPrintStringBuildSendObj[i].status = PRINT_STRING_BUILD_SEND_STATUS_UNUSED;
	}

	for ( i=0; i < MAX_NUM_STREAM_LISTENER; i++ ) {
		timer_second_set( &gaPrintStringTimer[ i ], gawPrintStringIntervalFNV[ i ]);
		timer_mSec_set( &gaPrintStringTimerMs[i], 4);										// 2012-09-24 -WFC-
		TIMER_T gaPrintStringTimerMs[ MAX_NUM_STREAM_LISTENER ];
		gabPrintStringLiftCnt[i] = (BYTE) gaulLiftCntFNV[ gabListenerWantSensorFNV[i] ];	// 2011-10-28 -WFC-
	}

	gbIsPrintStr_CmdPressed = FALSE;		// 2015-09-24 -WFC-

} // end print_string_main_init()


/**
 * It builds composite from UINT32 representation to string form.
 * Each digit represents formatter number and it is 1 based.
 * This also means that maximum number of formatter is limited to 9.
 *
 * @param  pOutStr		-- points to an output string where it store the newly built composite.
 * @param  lComposite	-- long integer representation of composite formatters.
 *
 * @return 0xFF, PRINT_STRING_BUILD_SEND_STATUS_BAD_COMPOSITE if it is a ZERO value.
 * 		   else return number of formatters in this string buffer.
 *
 * History:  Created on 2011/06/23 by Wai Fai Chin
 * 2011-10-28 -WFC- break out the loop on the first encounter of composite error.
 */

BYTE  print_string_build_validate_composite( BYTE *pOutStr, UINT32 lComposite )
{
	BYTE	i;
	BYTE	len;
	BYTE	strBuf[12];

	len = sprintf_P( strBuf, gcStrFmt_pct_lu, lComposite);

	if ( len > PRINT_STRING_MAX_NUM_COMPOSITE )
		len = PRINT_STRING_MAX_NUM_COMPOSITE;

	memcpy( pOutStr, strBuf, len);
	for ( i=0; i < len; i++ ) {
		if ( !( (pOutStr[i] <= '9') && pOutStr[i] > '0') ) {
			len = PRINT_STRING_BUILD_SEND_STATUS_BAD_COMPOSITE;			// bad because number is 0.
			break;														// 2011-10-28 -WFC- break out the loop on the first encounter of composite error.
		}
	}

	return len;
} // end print_string_format_unit_name(,,)


/**
 * It validating a string formatter.
 *
 * @param  pStr 		-- points to a formatter string.
 * @return 0 if no error.
 *
 * History:  Created on 2011/06/20 by Wai Fai Chin
 * 2011-10-19 -WFC- Added support of letter 'U' to fix a bug that reported invalid formatter error for valid formatter.
 */

BYTE  print_string_validate_formatter( BYTE *pStr )
{
	BYTE	formatter_strLen;			// formatter string length
	BYTE	digitLen;
	BYTE	lookFor;
	BYTE	lookForFieldType;
	BYTE	status;
	BYTE	bNum;
	BYTE	chIsNotAdigit;

	// PRINT_STRING_MAX_FORMATER_LENGTH
	status = CMD_STATUS_NO_ERROR;
	formatter_strLen = 0;
	lookForFieldType = PRINT_STRING_FIELD_TYPE_NEW;
	for ( ;; ) {
		switch ( lookForFieldType ) {
		case PRINT_STRING_FIELD_TYPE_NEW:
			digitLen = 0;
			if ( '^' == *pStr ) {
				lookForFieldType = PRINT_STRING_FIELD_TYPE_STRING;
			}
			else if ( 'L' == *pStr || 'R' == *pStr ) {
				lookForFieldType = PRINT_STRING_FIELD_TYPE_FIELD_WIDTH;
			}
			else if ( 'S' == *pStr ) {
				lookForFieldType = PRINT_STRING_FIELD_TYPE_SENSOR_NUMBER;
			}
			else if ( 'T' == *pStr ) {
				lookForFieldType = PRINT_STRING_FIELD_TYPE_VALUE_TYPE;
			}
			else if ( !( 'V' == *pStr || 'I' == *pStr || 'M' == *pStr || 'N' == *pStr ||  'U' == *pStr || '_' == *pStr || 'r' == *pStr || 'n' == *pStr) ) {
				status = CMD_STATUS_ERROR_INVALID_STRING_FORMATTER;
			}
			break;
		case PRINT_STRING_FIELD_TYPE_STRING:
			if ( '^' == *pStr ) {
				lookForFieldType = PRINT_STRING_FIELD_TYPE_NEW;
			}
			break;
		case PRINT_STRING_FIELD_TYPE_FIELD_WIDTH:
		case PRINT_STRING_FIELD_TYPE_SENSOR_NUMBER:
		case PRINT_STRING_FIELD_TYPE_VALUE_TYPE:
			chIsNotAdigit = FALSE;
			if ( *pStr >= '0' && *pStr <= '9') {
				digitLen++;
				if ( 1 == digitLen ) {
					bNum = *pStr - '0';
				}
				else {
					bNum *= 10;
					bNum += ( *pStr - '0' );
				}
			}
			else
				chIsNotAdigit = TRUE;

			if ( 2 == digitLen || chIsNotAdigit ) {
				if ( PRINT_STRING_FIELD_TYPE_FIELD_WIDTH == lookForFieldType ) {
					if ( bNum > PRINT_STRING_MAX_FIELD_LENGTH )
						status = CMD_STATUS_ERROR_FORMATTER_WIDTH_TOO_BIG;
				}
				else if ( PRINT_STRING_FIELD_TYPE_SENSOR_NUMBER == lookForFieldType ) {
					if ( bNum > (MAX_NUM_SENSORS - 1) ) {
						status = CMD_STATUS_ERROR_INVALID_SENSOR_NUMBER;
					}
				}
				else if ( PRINT_STRING_FIELD_TYPE_VALUE_TYPE == lookForFieldType ) {
					if ( bNum > SENSOR_VALUE_TYPE_MAX ) {
						status = CMD_STATUS_ERROR_INVALID_VALUE_TYPE;
					}
				}
				lookForFieldType = PRINT_STRING_FIELD_TYPE_NEW;
			}
			if ( chIsNotAdigit ) {
				pStr--;										// rewinded to look at this none digit char as new field type.
				formatter_strLen--;
				lookForFieldType = PRINT_STRING_FIELD_TYPE_NEW;
			}
			break;
		} // end switch() {}
		pStr++;
		formatter_strLen++;
		if ( formatter_strLen > PRINT_STRING_MAX_FORMATER_LENGTH )
			status = CMD_STATUS_PRE_TO_MANY_CHAR;
		if ( CMD_STATUS_NO_ERROR != status || 0 == *pStr )
			break;
	} // end for(;;)

	return status;
} // end print_string_validate_formatter()


/**
 * It builds an output string based on an input formatter.
 *
 * @param  pFmt		-- points to an input formatter string.
 * @param  pOutStr	-- points to an output string where it store the newly built output string based on input formatter.
 * @param  pOutLen	-- points to an output string length variable;
 * @return 0 if no error.
 *
 * History:  Created on 2011/06/21 by Wai Fai Chin
 */

BYTE  print_string_build_string_from_a_formatter( BYTE *pFmt, BYTE *pOutStr, BYTE *pOutLen )
{
	register BYTE	outLen;					// output string length
	register BYTE	*pFmtCh;				// point at formatter string char
	BYTE	formatter_strLen;				// Formatter string length
	BYTE	digitLen;
	BYTE	bSpecifier;
	BYTE	sensorNum;
	BYTE	valueType;
	BYTE	lookForFieldType;
	BYTE	status;
	BYTE	bNum;
	BYTE	chIsNotAdigit;
	INT8	fieldWidth;			// 0 == variable, Negative number == Left Justify, Positive number == Right Justify.
	BYTE	bCformatter[12];

	// PRINT_STRING_MAX_FORMATER_LENGTH
	status = CMD_STATUS_NO_ERROR;
	valueType = sensorNum = formatter_strLen = outLen = 0;
	fieldWidth = 0;
	lookForFieldType = PRINT_STRING_FIELD_TYPE_NEW;
	pFmtCh = pFmt;
	for ( ;; ) {
		switch ( lookForFieldType ) {
		case PRINT_STRING_FIELD_TYPE_NEW:
			digitLen = 0;
			if ( '^' == *pFmtCh ) {
				lookForFieldType = PRINT_STRING_FIELD_TYPE_STRING;
			}
			else if ( 'L' == *pFmtCh || 'R' == *pFmtCh ) {
				lookForFieldType = PRINT_STRING_FIELD_TYPE_FIELD_WIDTH;
			}
			else if ( 'S' == *pFmtCh ) {
				lookForFieldType = PRINT_STRING_FIELD_TYPE_SENSOR_NUMBER;
			}
			else if ( 'T' == *pFmtCh ) {
				lookForFieldType = PRINT_STRING_FIELD_TYPE_VALUE_TYPE;
			}
			else if ( 'V' == *pFmtCh || 'I' == *pFmtCh  ) {
				outLen += print_string_format_value( pOutStr + outLen, sensorNum, valueType, fieldWidth, *pFmtCh );
			}
			else if ( 'M' == *pFmtCh ) {
				outLen += print_string_format_value_mode( pOutStr + outLen, sensorNum, valueType );
			}
			else if ( 'N' == *pFmtCh ) {
				cml_build_c_formatter( bCformatter, fieldWidth, fieldWidth, 's' );
				outLen += sprintf( pOutStr + outLen, bCformatter, &gabSensorNameFNV[sensorNum][0] );	// format sensor name.
			}
			else if ( 'U' == *pFmtCh ) {
				outLen += print_string_format_unit_name( pOutStr + outLen, sensorNum, valueType );
			}
			else if ( '_' == *pFmtCh ) {
				pOutStr[ outLen ] = ' ';
				outLen++;
			}
			else if ( 'r' == *pFmtCh ) {
				pOutStr[ outLen ] = '\r';
				outLen++;
			}
			else if ( 'n' == *pFmtCh ) {
				pOutStr[ outLen ] = '\n';
				outLen++;
			}
			else
				status = CMD_STATUS_ERROR_INVALID_STRING_FORMATTER;
			bSpecifier = *pFmtCh;
			break;
		case PRINT_STRING_FIELD_TYPE_STRING:
			if ( '^' == *pFmtCh ) {								// if another ^, then it is end of string.
				lookForFieldType = PRINT_STRING_FIELD_TYPE_NEW;
			}
			else {
				pOutStr[ outLen ] = *pFmtCh;						// save this char to build a string.
				outLen++;
			}
			break;
		case PRINT_STRING_FIELD_TYPE_FIELD_WIDTH:
		case PRINT_STRING_FIELD_TYPE_SENSOR_NUMBER:
		case PRINT_STRING_FIELD_TYPE_VALUE_TYPE:
			chIsNotAdigit = FALSE;
			if ( *pFmtCh >= '0' && *pFmtCh <= '9') {
				digitLen++;
				if ( 1 == digitLen ) {
					bNum = *pFmtCh - '0';
				}
				else {
					bNum *= 10;
					bNum += ( *pFmtCh - '0' );
				}
			}
			else
				chIsNotAdigit = TRUE;

			if ( 2 == digitLen || chIsNotAdigit ) {
				if ( PRINT_STRING_FIELD_TYPE_FIELD_WIDTH == lookForFieldType ) {
					if ( bNum > PRINT_STRING_MAX_FIELD_LENGTH )
						status = CMD_STATUS_ERROR_FORMATTER_WIDTH_TOO_BIG;
					else {
						fieldWidth = bNum;
						if ( 'L' == bSpecifier ) {				// if it is Left Justify specifier, then set it negative.
							fieldWidth = - fieldWidth;
						}
					}
				}
				else if ( PRINT_STRING_FIELD_TYPE_SENSOR_NUMBER == lookForFieldType ) {
					if ( bNum > (MAX_NUM_SENSORS - 1) ) {
						status = CMD_STATUS_ERROR_INVALID_SENSOR_NUMBER;
					}
					else
						sensorNum = bNum;
				}
				else if ( PRINT_STRING_FIELD_TYPE_VALUE_TYPE == lookForFieldType ) {
					if ( bNum > SENSOR_VALUE_TYPE_MAX ) {
						status = CMD_STATUS_ERROR_INVALID_VALUE_TYPE;
					}
					else {
						valueType = bNum;
					}
				}
				lookForFieldType = PRINT_STRING_FIELD_TYPE_NEW;
			}
			if ( chIsNotAdigit ) {
				pFmtCh--;										// rewinded to look at this none digit char as new field type.
				formatter_strLen--;
				lookForFieldType = PRINT_STRING_FIELD_TYPE_NEW;
			}
			break;
		} // end switch() {}

		if ( outLen > PRINT_STRING_MAX_OUTPUT_LENGTH ) {
			status = CMD_STATUS_ERROR_FORMATTER_GEN_STRING_TOO_LONG;		// this will force it exit the for loop.
			pOutStr[ outLen ] = 0;											// marked the end of output string.
		}

		*pOutLen = outLen;
		pFmtCh++;
		formatter_strLen++;
		if ( formatter_strLen > PRINT_STRING_MAX_FORMATER_LENGTH )
			status = CMD_STATUS_PRE_TO_MANY_CHAR;
		if ( CMD_STATUS_NO_ERROR != status || 0 == *pFmtCh )	// if encountered error or end of string, exit this for loop.
			break;
	} // end for(;;)

	return status;
} // end print_string_build_string_from_a_formatter(,,)


/**
 * It builds unit name string based on sensor number and value type.
 *
 * @param  pOutStr	-- points to an output string where it store the newly built output string based on input formatter.
 * @param  sn		-- sensor number.
 * @param  valueType-- sensor value type such as gross, net, ...etc..
 * @return number characters in this string.
 *
 * History:  Created on 2011/06/22 by Wai Fai Chin
 */

BYTE  print_string_format_unit_name( BYTE *pOutStr, BYTE sn, BYTE valueType )
{
	BYTE	unit;
	PGM_P	ptr_P;

	switch ( gabSensorTypeFNV[ sn ] ) {
		case SENSOR_TYPE_LOADCELL:
		case SENSOR_TYPE_MATH_LOADCELL:
				unit = gaLoadcell[ sn ].viewCB.unit;
			break;
		default:
			unit = gabSensorViewUnitsFNV[ sn ];
			break;
	}

	if ( unit > SENSOR_UNIT_MAX )
		unit = SENSOR_UNIT_MAX;

	if ( SENSOR_VALUE_TYPE_ADC_COUNT == valueType || SENSOR_VALUE_TYPE_TOTAL_COUNT == valueType ) {
		// if value type is ADC count OR total count, then override sensor unit to just blank.
		unit = SENSOR_UNIT_MAX;
	}

	memcpy_P( &ptr_P, &gcPrintStringUnitNameTbl[ unit ], sizeof(PGM_P));
	strcpy_P( pOutStr, ptr_P);
	return strlen( pOutStr );

} // end print_string_format_unit_name(,,)


/**
 * It builds value mode name string based on sensor number and value type.
 *
 * @param  pOutStr	-- points to an output string where it store the newly built output string based on input formatter.
 * @param  sn		-- sensor number.
 * @param  valueType-- sensor value type such as gross, net, ...etc..
 * @return number characters in this string.
 *
 * History:  Created on 2011/06/22 by Wai Fai Chin
 * 2011-12-02 -WFC- handle conditional compilation for none IDSC product.
 */

BYTE  print_string_format_value_mode( BYTE *pOutStr, BYTE sn, BYTE valueType )
{
	BYTE	valueMode;
	PGM_P	ptr_P;

	valueMode = valueType;
	switch ( gabSensorTypeFNV[ sn ] ) {
		case SENSOR_TYPE_LOADCELL:
		case SENSOR_TYPE_MATH_LOADCELL:
				if ( SENSOR_VALUE_TYPE_CUR_MODE == valueType ) {
					if ( LC_RUN_MODE_PEAK_HOLD_ENABLED & gaLoadcell[ sn ].runModes )
						valueMode = SENSOR_VALUE_TYPE_PEAK;
					#if ( CONFIG_PRODUCT_AS	!= CONFIG_AS_IDSC )				// 2011-12-02 -WFC-
					else if ( DISPLAY_WEIGHT_MODE_TOTAL == gbPanelDisplayWeightMode )
						valueMode = SENSOR_VALUE_TYPE_TOTAL;
					#endif
					else if ( LC_OP_MODE_NET_GROSS &  *(gaLoadcell[ sn ].pOpModes)  )
						valueMode = SENSOR_VALUE_TYPE_NET;
					else
						valueMode = SENSOR_VALUE_TYPE_GROSS;
				}
			break;
		default:	// for other sensors:
			if ( SENSOR_VALUE_TYPE_ADC_COUNT != valueType )
				valueMode = SENSOR_VALUE_TYPE_CUR_MODE;			// 5 blank spaces.
			break;
	}

	if ( valueMode > SENSOR_VALUE_TYPE_MAX )
		valueMode = SENSOR_VALUE_TYPE_CUR_MODE;

	memcpy_P( &ptr_P, &gcPrintStringValueModeNameTbl[ valueMode ], sizeof(PGM_P));
	strcpy_P( pOutStr, ptr_P);
	return strlen( pOutStr );
} // end print_string_format_value_mode(,,)


/**
 * It builds value string based on sensor number and value type.
 *
 * @param  pOutStr		-- points to an output string where it store the newly built output string based on input formatter.
 * @param  sn			-- sensor number.
 * @param  valueType	-- sensor value type such as gross, net, ...etc..
 * @param  fieldWidth	-- width of value string. Negative is left justify, Positive is right justify pad with spaces to fill to width.
 * @param  numberType	-- 'V' is based on count by. 'I' is for print integer portion of value.
 * @return number characters in this string.
 *
 * History:  Created on 2011/06/22 by Wai Fai Chin
 */
//print_string_format_value( pOutStr + outLen, sensorNum, valueType, fieldWidth, *pStr );
BYTE  print_string_format_value( BYTE *pOutStr, BYTE sn, BYTE valueType, INT8 fieldWidth, BYTE numberType )
{
	BYTE	len;
	BYTE	sensorState;
	BYTE	precision;
	PGM_P	ptr_P;
	float	fV;
	MSI_CB  cb;
	BYTE	bCformatter[12];

	switch ( gabSensorTypeFNV[ sn ] ) {
		case SENSOR_TYPE_LOADCELL:
			sensorState = loadcell_get_value_of_type( sn, valueType, SENSOR_VALUE_UNIT_CUR_MODE, &fV );
				// if ( SENSOR_STATE_GOT_VALID_VALUE == sensorState )
			cb = gaLoadcell[sn].viewCB;
			break;
		#if ( CONFIG_INCLUDED_VS_MATH_MODULE ==	TRUE )
		case SENSOR_TYPE_MATH_LOADCELL:
			sensorState = loadcell_get_value_of_type( sn, valueType, SENSOR_VALUE_UNIT_CUR_MODE, &fV );
			cb = gaLoadcell[sn].viewCB;
			break;
		#endif
		case SENSOR_TYPE_VOLTMON:
			sensorState = voltmon_get_value_of_type( sn, valueType, &fV, &cb );
			break;

		default:	// for other sensors:
			sensorState = SENSOR_STATE_DISABLED;		// not support other sensor yet...
			break;
	}

	if ( SENSOR_VALUE_TYPE_ADC_COUNT == valueType ||
			SENSOR_VALUE_TYPE_TOTAL_COUNT == valueType	) {
		numberType = 'I';		// force to integer type
		cb.decPt = 0;
		cb.fValue = 1.0f;
		cb.iValue = 1;
	}

	if ( SENSOR_STATE_GOT_VALID_VALUE == sensorState ) {
		if ( 'I' == numberType )
			precision = 0;
		else {
			if ( cb.decPt > 0 )
				precision = cb.decPt;
			else
				precision = 0;
		}
		fV = float_round( fV, cb.fValue);
		cml_build_c_formatter( bCformatter, fieldWidth, precision, 'f' );
		len = (BYTE) sprintf( pOutStr, bCformatter, fV );
	}
	else { // invalid value or unsupported sensor will fill with '-' up to fieldWidth.
		fill_string_buffer( pOutStr, '-', fieldWidth );
		len += fieldWidth;
	}

	return len;
} // end print_string_format_value(,,,,)


/**
 * It manages listeners print string mode.
 * It generates print string event based on print string mode such as stable load, interval etc...
 * It calls print_string_main_thread();
 *
 * History:  Created on 2011/06/23 by Wai Fai Chin
 * 2011-10-28 -WFC- implement print string on new stable loaded.
 * 2012-09-21 -WFC- treat interval time 0 as 4 timer ticks.
 * 2015-09-24 -WFC- handle PRINT_STRING_CTRL_MODE_COMMAND.
 */

void  print_string_main_tasks( void )
{
	BYTE i;
	BYTE lc;	// 2011-10-28 -WFC-
	BYTE liftCnt;
	BYTE setPrintStringEvent;

	for ( i=0; i < MAX_NUM_STREAM_LISTENER; i++ ) {
		setPrintStringEvent = FALSE;
		if (  PRINT_STRING_CTRL_MODE_CONTINUOUS == gabPrintStringCtrlModeFNV[ i ]) {
			if ( 0 == gawPrintStringIntervalFNV[ i ] ) {
				if ( timer_mSec_expired( &gaPrintStringTimerMs[i] ) ) {
					setPrintStringEvent = TRUE;
					timer_mSec_set( &gaPrintStringTimerMs[ i ], 4);
				}
			}
			else {
				if ( timer_second_expired( &gaPrintStringTimer[i] ) ) {
					setPrintStringEvent = TRUE;
					timer_second_set( &gaPrintStringTimer[ i ], gawPrintStringIntervalFNV[ i ]);
				}
			}
		}
		else if ( PRINT_STRING_CTRL_MODE_STABLE_LOAD == gabPrintStringCtrlModeFNV[ i ]) {
			// check a new load stable print string event...
			// 2011-10-28 -WFC- v
			lc = gabListenerWantSensorFNV[i];
			liftCnt = (BYTE) gaulLiftCntFNV[lc];
			if ( liftCnt != gabPrintStringLiftCnt[i]) {		// if loadcell lc has a new load
				if ( !(LC_STATUS_MOTION & gaLoadcell[ lc ].status) ) {	// and if it is stable, then
					gabPrintStringLiftCnt[i] = liftCnt;		// updated print string lift counter for detect new load.
					setPrintStringEvent = TRUE;				// yes, it qualified for print string on a new stable load event.
				}
			}
		}
		// 2015-09-24 -WFC- v
		else if ( PRINT_STRING_CTRL_MODE_COMMAND == gabPrintStringCtrlModeFNV[i] ) {
			if ( gbIsPrintStr_CmdPressed )
				setPrintStringEvent = TRUE;
		}
		// 2015-09-24 -WFC- ^

		if ( setPrintStringEvent )
			print_string_setup_for_BnSobj( i );		// setup and constructed an object of print string. The print string thread will automatically taking of it.
		// 2011-10-28 -WFC- ^
	}// end for() check listener print string event.

	// walk through all Print String Manager objects
	for ( i = 0; i< PRINT_STRING_MANAGER_MAX_NUM_OBJ; i++) {
		print_string_main_thread( i );
	}

	gbIsPrintStr_CmdPressed = FALSE;		// 2015-09-24 -WFC-


} // end print_string_main_tasks()


/*
void  print_string_main_tasks( void )
{
	BYTE i;
	BYTE lc;	// 2011-10-28 -WFC-
	BYTE liftCnt;
	BYTE setPrintStringEvent;

	for ( i=0; i < MAX_NUM_STREAM_LISTENER; i++ ) {
		setPrintStringEvent = FALSE;
		if (  PRINT_STRING_CTRL_MODE_CONTINUOUS == gabPrintStringCtrlModeFNV[ i ]) {
			if ( 0 == gawPrintStringIntervalFNV[ i ] ) {
				setPrintStringEvent = TRUE;
			}
			else {
				if ( timer_second_expired( &gaPrintStringTimer[i] ) ) {
					setPrintStringEvent = TRUE;
					timer_second_set( &gaPrintStringTimer[ i ], gawPrintStringIntervalFNV[ i ]);
				}
			}
//			if ( setPrintStringEvent )
//				print_string_setup_for_BnSobj( i );		// setup and constructed an object of print string. The print string thread will automatically taking of it.
		}
		else if ( PRINT_STRING_CTRL_MODE_STABLE_LOAD == gabPrintStringCtrlModeFNV[ i ]) {
			// check a new load stable print string event...
			// 2011-10-28 -WFC- v
			lc = gabListenerWantSensorFNV[i];
			liftCnt = (BYTE) gaulLiftCntFNV[lc];
			if ( liftCnt != gabPrintStringLiftCnt[i]) {		// if loadcell lc has a new load
				if ( !(LC_STATUS_MOTION & gaLoadcell[ lc ].status) ) {	// and if it is stable, then
					gabPrintStringLiftCnt[i] = liftCnt;		// updated print string lift counter for detect new load.
					setPrintStringEvent = TRUE;				// yes, it qualified for print string on a new stable load event.
				}
			}
		}

		if ( setPrintStringEvent )
			print_string_setup_for_BnSobj( i );		// setup and constructed an object of print string. The print string thread will automatically taking of it.
		// 2011-10-28 -WFC- ^
	}// end for() check listener print string event.

	// walk through all Print String Manager objects
	for ( i = 0; i< PRINT_STRING_MANAGER_MAX_NUM_OBJ; i++) {
		print_string_main_thread( i );
	}

} // end print_string_main_tasks()
*/

/**
 * It is a reliable command communication main thread. It is clone-able by gaCmdReliableSendManagerObj[]
 * which is index by managerID.
 *
 * @param  managerID	-- index of gaCmdReliableSendManagerObj[]. It became a cloned object working in conjunction with this main thread.
 *
 * History:  Created on 2010/07/19 by Wai Fai Chin
 */

//PT_THREAD( print_string_main_thread( BYTE managerID )) // Doxygen cannot handle this macro
char print_string_main_thread( BYTE managerID )
{
  PT_BEGIN( &(gaPrintStringManagerObj[ managerID].m_pt) );
  for ( ;;) {
	  if ( PRINT_STRING_MANAGER_STATUS_UNUSED == gaPrintStringManagerObj[ managerID].status ) {
		  if ( print_string_get_a_request_send_BnSobj( &(gaPrintStringManagerObj[ managerID].pBnSobj) )) {
			  gaPrintStringManagerObj[ managerID].status = PRINT_STRING_MANAGER_STATUS_HAS_BNSOBJ;
			  PT_SPAWN( &(gaPrintStringManagerObj[ managerID].m_pt),
					  &(gaPrintStringManagerObj[ managerID].pBnSobj->m_pt),
					  print_string_build_send_thread( gaPrintStringManagerObj[ managerID].pBnSobj) );
			  gaPrintStringManagerObj[ managerID].status = PRINT_STRING_MANAGER_STATUS_UNUSED;
		  }
	  }
	  PT_YIELD( &(gaPrintStringManagerObj[ managerID].m_pt) );
  }// end endless for loop.

  PT_END(&(gaPrintStringManagerObj[ managerID].m_pt));

} // end print_string_main_thread()

/**
 * It guarantees to send out a built composite print string to output stream.
 *
 * @param  pBnSobj	-- pointer to reliable send command object.
 *
 * History:  Created on 2011/06/23 by Wai Fai Chin
 */

//PT_THREAD( print_string_build_send_thread( struct pt *pt )) // Doxygen cannot handle this macro
char print_string_build_send_thread( PRINT_STRING_BUILD_SEND_CLASS_T *pBnSobj )
{
  IO_STREAM_CLASS			*pOstream;	// pointer to a dynamic out stream object.

  PT_BEGIN( &(pBnSobj-> m_pt) );
	pBnSobj-> status = PRINT_STRING_BUILD_SEND_STATUS_BUSY;
	pBnSobj->compositeIndex = 0;
	for (;;) {
		if (  pBnSobj-> compositeIndex >= pBnSobj-> numFormatter ) {
			break;
		}
		else {
			// wait until there is available output stream.
			PT_WAIT_UNTIL( &(pBnSobj-> m_pt), stream_router_get_a_new_stream( &pOstream ));
			print_string_built_and_send( pBnSobj, pOstream );
			pBnSobj-> compositeIndex++;		// points to next formatterIndex in the pBnSobj-> compositeFmt[].
		}
	}
	pBnSobj-> status = PRINT_STRING_BUILD_SEND_STATUS_UNUSED;
  PT_END( &(pBnSobj-> m_pt) );
} // end print_string_build_send_thread()

/**
 * It tries to find a PRINT_STRING_BUILD_SEND_CLASS_T object, BnSobj of a given type and saved its pointer to ppBnSobj.
 *
 * @param	ppBnSobj	-- pointer of pointer to a ppBnSobj. It is for saved the pointer of a ppBnSobj.
 * @param	type		-- type of ppBnSobj, such as UNUSED, REQ_SEND etc..
 * @return  PASSED if it found a ppBnSobj of given type, else FAILED.
 *
 * History:  Created on 2011/06/23 by Wai Fai Chin
 */

BYTE print_string_get_BnSobj_of_given_type( PRINT_STRING_BUILD_SEND_CLASS_T **ppBnSobj, BYTE type )
{
	BYTE	i;
	BYTE	status;

	status = FAILED;
	for ( i = 0; i < PRINT_STRING_BUILD_SEND_MAX_NUM_OBJ; i++) {
		if ( type == gaPrintStringBuildSendObj[i].status) {
			*ppBnSobj = &gaPrintStringBuildSendObj[i];
			status = PASSED;
			break;
		}
	}
	return status;
} // end print_string_get_BnSobj_of_given_type()


/**
 * 1st it tries to find an unused cmdRSobj. Once it found an unused cmdRSobj,
 * it fills in the cmdRSobj members based on the caller supplied parameters.
 *
 * @param  streamType	-- stream type. IO_STREAM_TYPE_UART =0, IO_STREAM_TYPE_UART_1 = 1, IO_STREAM_TYPE_SPI = 2, IO_STREAM_TYPE_RFMODEM = 3,
 * @param  destID		-- destination ID of a device.
 * @param  cmd			-- command.
 * @param  pStr			-- points to a command parameter string buffer.
 *
 * @return PASSED if it success, else FAILED.
 *
 * History:  Created on 2011/06/23 by Wai Fai Chin
 */

BYTE	print_string_setup_for_BnSobj( BYTE listener )
{
	BYTE len;
	BYTE status;
	PRINT_STRING_BUILD_SEND_CLASS_T		*pBnSobj;

	status = CMD_STATUS_NO_ERROR;
	if ( print_string_get_an_unused_BnSobj( &pBnSobj ) ) {
		pBnSobj-> streamType		= gabListenerStreamTypeFNV[ listener ];
		pBnSobj-> destID			= gabListenerDevIdFNV[ listener ];
		pBnSobj-> compositeIndex	= 0;
		len = print_string_build_validate_composite( pBnSobj-> compositeFmt, gaulPrintStringCompositeFNV[ listener ]);
		if ( PRINT_STRING_BUILD_SEND_STATUS_BAD_COMPOSITE != len ) {
			pBnSobj-> status		= PRINT_STRING_BUILD_SEND_STATUS_REQ_SEND;
			pBnSobj-> numFormatter	= len;
		}
		else {
			pBnSobj-> status = PRINT_STRING_BUILD_SEND_STATUS_UNUSED; // invalid composite, return this object back to resource pool.
			status = CMD_STATUS_ERROR_ZERO_IN_PRINT_STRING_COMPOSITE;
		}
	}
	else {
		status = CMD_STATUS_ERROR_OUT_OF_PRINT_STRING_OBJECT;
	}
	return status;
} // end print_string_setup_for_BnSobj()


/**
 * It built a print string based on formatter and then send it out via pOstream.
 *
 * @param	pBnSobj		-- pointer to reliable send command object.
 * @param	pOstream	-- pointer to an out stream object.
 *
 * @return none
 *
 * History:  Created on 2011/06/23 by Wai Fai Chin
 * 2011-07-14 if Formatter # is 8, and 9, then output carriage return and line feed.
 */

//char	outStr[ (PRINT_STRING_MAX_OUTPUT_LENGTH * 2) ];
void	print_string_built_and_send( PRINT_STRING_BUILD_SEND_CLASS_T *pBnSobj, IO_STREAM_CLASS	*pOstream )
{
	BYTE	fmtIndex;
	BYTE	len;
	char	outStr[ (PRINT_STRING_MAX_OUTPUT_LENGTH * 2) ];

	if (stream_router_get_a_new_stream( &pOstream ) ) {			// get an output stream to this listener.
		pOstream-> type		= pBnSobj-> streamType;						// stream type for the listener.
		pOstream-> status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
		pOstream-> sourceID	= gtProductInfoFNV.devID;
		pOstream-> destID	= pBnSobj-> destID;

		fmtIndex = pBnSobj-> compositeFmt[pBnSobj->compositeIndex] - '1';

		outStr[0] = 0;
		len = 0;
		if ( fmtIndex > 6 ) {			// 2011-07-14 -WFC- v if Formatter # was 8, and 9, then output carriage return line feed.
			outStr[0] = '\r';
			outStr[1] = '\n';
			outStr[2] = 0;
			len = 2;
		}
		else if ( CMD_STATUS_NO_ERROR !=
					print_string_build_string_from_a_formatter(
							&gabPrintStringUserFormatterFNV[ fmtIndex ][0], outStr, &len ) ) {
				len = (BYTE) sprintf_P( outStr, gcPrintFormatterErrorMsg, fmtIndex + '1' );
		}

		if ( len > 0 ) {										// if it has data to output.
			stream_router_routes_a_ostream_now( pOstream, outStr, len );
		} // end if ( len >0 ) {}.
		stream_router_free_stream( pOstream );	// put resource back to stream pool.
	} // end if (stream_router_get_a_new_stream( &pOstream ) ) {}

} // end print_string_built_and_send()

