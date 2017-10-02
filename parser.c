/*! \file parser.c \brief parser related functions implementation.*/
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
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2007/07/10 by Wai Fai Chin
// 
// ****************************************************************************


#include "parser.h"

/**
 *   A number defined by MSI is ([ ])*|[-]([0-9])+|[.]([0-9])* .
 *   Length of a number will not include leading spaces or a decimal point.
 *   It includes '-'.
 *
 * @param  pbStr  -- an input string to be validate. The length must be < 128 because
 *                   the function return sign char.
 *
 * @return 0 if it is non number.
 *         Negative value is the length of negative number exclude spaces and decimal point.
 *         Positive value is the length of positive number exclude spaces and decimal point.
 *
 * History:  Created on 2001/09/27 by Wai Fai Chin
 */

char is_a_number( const char *pbStr )
{
  BYTE bNumDigit;
  BYTE ch;
  BYTE fIsMinus;

  fIsMinus = FALSE;

  bNumDigit = 0;
  // skip spaces 
  for (;; ) {
	ch = *pbStr++;
	if (  ch == 0 )
	  return 0;			// end of string, the string is not a number 
	if ( ch != ' ' )
	  break;			// encountered a none space char 
  }// end for (;;) 

  // the next char should be '-' or a digit 
  if ( ch == '-' ) {
  	bNumDigit++;
    fIsMinus = TRUE;
  }
  else if (ch >= '0' && ch <= '9' ) {
		bNumDigit++;
  }
  else
    return 0;	// not a number 

  // the next section should be digit until it find a decimal point 
  for (;; ) {
	ch = *pbStr++;
	if ( ch == 0 ) break;
    if ( ch >= '0' && ch <= '9' )
		bNumDigit++;
	else if ( ch == '.' )
		break;
	else 
		return 0;		// not a number 
  }// end for (;;) 

  // the next section should be digit until it reach the end of string 
  if ( ch ) {
  	for (;; ) {
		ch = *pbStr++;
		if ( ch == 0 ) break;
    	if ( ch >= '0' && ch <= '9' )
			bNumDigit++;
		else 
			return 0;		// not a number 
	} // end for (;;) 
  } // end if (ch) 

  if ( fIsMinus )
    return  -((char) bNumDigit);
  else
    return (char) bNumDigit;
}// end is_a_number() 



/**
 *	See if all characters in the input string are qualify for use in a password string.
 * A qualify character value is ([0-9]|[A-Z]|[a-z])*
 *
 * @param  pbStr  -- an input string to be validate.
 *
 * @return the length of the string if characters are qualify.
 *			else 0.
 *
 * History:  Created on 2008/08/01 by Wai Fai Chin
 */

BYTE is_psw_string_has_qualify_chars( const char *pbStr )
{
	BYTE	ch;
	BYTE	len;
	
	len = 0;
  	for (;;) {
		ch = *pbStr++;
		if ( ch == 0 ) break;
    	if ( (ch >= '0' && ch <= '9') ||
			 (ch >= 'A' && ch <= 'Z') ||
			 (ch >= 'a' && ch <= 'z') )
			len++;
		else {
			len = 0;
			break;
		}
	} // end for (;;) 
	
	return len;

}// end is_psw_string_has_qualify_chars() 


///////////////////////////////////////////////////////////////////////////////
//                      Test functions for this module.                      //
///////////////////////////////////////////////////////////////////////////////

#if  ( CONFIG_TEST_PARSER_MODULE == TRUE)
#include <stdio.h>                /* prototype declarations for I/O functions */
#include <string.h>               
#include  "bios.h"

const char testStrA_P[] PROGMEM = "12345678.A9";
const char testStrB_P[] PROGMEM = "12345678";
const char testStrC_P[] PROGMEM = "123456789";
const char testStrD_P[] PROGMEM = " -12345";
const char testStrE_P[] PROGMEM = "123456789B";
const char testStrF_P[] PROGMEM = "1234567A89";

const char parserfmtStr1_P[] PROGMEM ="\n\r%s : r=%i";

void test_is_a_number( void);

BYTE test_parser_module( void)
{
  test_is_a_number();
  return TRUE;
}

void test_is_a_number( void)
{
	char r;
	char strBuf1[40];
	char strBuf2[80];

	strcpy_P(strBuf1, testStrA_P);
	r = is_a_number( strBuf1 );
	sprintf_P( strBuf2, parserfmtStr1_P, strBuf1, (int)r);
	serial0_send_string( (BYTE *)strBuf2 );

	strcpy_P(strBuf1, testStrB_P);
	r = is_a_number( strBuf1 );
	sprintf_P( strBuf2, parserfmtStr1_P, strBuf1, (int)r);
	serial0_send_string( (BYTE *)strBuf2 );

	strcpy_P(strBuf1, testStrC_P);
	r = is_a_number( strBuf1 );
	sprintf_P( strBuf2, parserfmtStr1_P, strBuf1, (int)r);
	serial0_send_string( (BYTE *)strBuf2 );

	strcpy_P(strBuf1, testStrD_P);
	r = is_a_number( strBuf1 );
	sprintf_P( strBuf2, parserfmtStr1_P, strBuf1, (int)r);
	serial0_send_string( (BYTE *)strBuf2 );

	strcpy_P(strBuf1, testStrE_P);
	r = is_a_number( strBuf1 );
	sprintf_P( strBuf2, parserfmtStr1_P, strBuf1, (int)r);
	serial0_send_string( (BYTE *)strBuf2 );

}

#endif   // end ifdef  CONFIG_TEST_PARSER_MODULE
