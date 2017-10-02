/*! \file hw3460_led.c \brief external LED display related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//               Copyright (c) 2009 - 2011 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATMEGA1281
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2009-02-12 by Wai Fai Chin
// 
//   This is a drive for LED driver on the hardware 3460 PCB.
//   This module will use by Challenger3, MSI3550, etc.. It is not a generic core
//   driver of SCaleCore. It is product specific driver.
//
// ****************************************************************************
 

#include	"led_display.h"
#include	"commonlib.h"
#include	"sensor.h"		// 
#include	"bios.h"

/*!
*/


// LED digits, [0] is for digit5, [1] for digit6 and [2] for digit7. Digit5 and 6 are for annunciator5 and 6. Digit7 is sensor number.
//BYTE	gabLastLedDig[3];	// note that LED driver chip address 6 is for digit 5. 

// Lookup table for the seven segment Led character map.
//   7 6 5 4 3 2 1 0
//  DP A B C D E F G  segments where DP == decimal point.
BYTE	gcbLED7SegTbl[] PROGMEM = {
0x00, // 		0
0x00, // 		1
0x00, // 		2
0x00, // 		3
0x00, // 		4

0x00, // 		5
0x00, // 		6
0x00, // 		7
0x00, // 		8
0x00, // 		9

0x00, // 		10
0x00, // 		11
0x00, // 		12
0x00, // 		13
0x00, // 		14

0x00, // 		15		
0x00, // 		16
0x00, // 		17
0x00, // 		18
0x00, // 		19

0x00, // 		20
0x00, // 		21
0x00, // 		22
0x00, // 		23
0x00, // 		24

0x00, // 		25
0x00, // 		26
0x00, // 		27
0x00, // 		28
0x00, // 		29

0x00, // 		30
0x00, // 		31

0x00, // space  32
LED_SEG_IMG_DP | LED_SEG_IMG_B | LED_SEG_IMG_C, 	//	0xB0, //  !
0x00, //  "
0x00, //  #
0x00, //  $
0x00, //  %
0x00, //  &
LED_SEG_IMG_B, //  '	PHJ
0x00, //  (
0x00, //  )
0x00, //  *
0x00, //  +
0x00, //  ,
LED_SEG_IMG_G, 	//	0x01, //  -
LED_SEG_IMG_DP, 	//	0x80, //  .
0x00, //  /
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F, 	//	0x7E, //  0
LED_SEG_IMG_B | LED_SEG_IMG_C, 	//	0x30, //  1
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x6D, //  2
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_G, 	//	0x79, //  3
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x33, //  4
LED_SEG_IMG_A | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x5B, //  5
LED_SEG_IMG_A | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x5F, //  6
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C, 	//	0x70, //  7
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x7F, //  8
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x7B, //  9
0x00, //  :
0x00, //  ;
LED_SEG_IMG_D | LED_SEG_IMG_E, 	//	0x0C, //  <
LED_SEG_IMG_D | LED_SEG_IMG_G, 	//	0x09, //  =
LED_SEG_IMG_A | LED_SEG_IMG_B, 	//	0x60, //  >
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x65, //  ?
0x00, //  @
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x77, //  A
LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x1F, //  B
LED_SEG_IMG_A | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F, 	//	0x4E, //  C
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x3D, //  D
LED_SEG_IMG_A | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x4F, //  E
LED_SEG_IMG_A | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x47, //  F
//	LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	DRT, //  G
LED_SEG_IMG_A | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F , 	//	0x7B, //  G
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x37, //  H
LED_SEG_IMG_C, 	//	0x10, //  I
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E, 	//	0x3C, //  J
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, //	DRT, //  K
//	0x00, //  K
LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F, 	//	0x0E, //  L
LED_SEG_IMG_A | LED_SEG_IMG_C | LED_SEG_IMG_E, 	//	DRT, //  M
//	0x00, //  M
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_F, 	//	0x76, //  N
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F, 	//	0x7E, //  O
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x67, //  P
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x73, //  Q
LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x05, //  R
LED_SEG_IMG_A | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x5B, //  S
LED_SEG_IMG_A | LED_SEG_IMG_E | LED_SEG_IMG_F, 	//	0x46, //  T
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F, 	//	0x3E, //  U
LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E, 	//	DRT, //  V
//	0x00, //  V
LED_SEG_IMG_B | LED_SEG_IMG_D | LED_SEG_IMG_F, 	//	DRT, //  W
//	0x00, //  W
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, //	DRT, //  X
//	0x00, //  X
LED_SEG_IMG_B | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	DRT, //  Y
//	LED_SEG_IMG_B | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x27, //  Y
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_D | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x6D, //  Z		90
0x00, //  [		91
0x00, //  \		92
0x00, //  ]		93
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x63, //  ^		94 use it to represent degree in temperature 
LED_SEG_IMG_D, 	//	0x08,  //  _	95
0x00, //  `		96
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x77, //  a		97
LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x1F, //  b
LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x0D, //  c
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x3D, //  d
LED_SEG_IMG_A | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x4F, //  e
LED_SEG_IMG_A | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x47, //  f
//	LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	DRT, //  G
LED_SEG_IMG_A | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F , 	//	0x7B, //  G
LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x17, //  h
LED_SEG_IMG_C, 	//	0x10, //  I
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E, 	//	0x3C, //  j
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, //	DRT, //  K
//	0x00, //  K
LED_SEG_IMG_D | LED_SEG_IMG_E, 	//	DRT	, //  l
//	LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F, 	//	0x0E, //  l
LED_SEG_IMG_A | LED_SEG_IMG_C | LED_SEG_IMG_E, 	//	DRT, //  M
//	0x00, //  M
LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x15, //  n
LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x1D, //  o
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x67, //  p
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x73, //  q
LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x05, //  r
LED_SEG_IMG_A | LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x5B, //  s
LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x0F, //  t
LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E, 	//	0x1C, //  u
LED_SEG_IMG_C | LED_SEG_IMG_D | LED_SEG_IMG_E, 	//	DRT, //  V
//	0x00, //  V
LED_SEG_IMG_B | LED_SEG_IMG_D | LED_SEG_IMG_F, 	//	DRT, //  W
//	0x00, //  W
LED_SEG_IMG_B | LED_SEG_IMG_C | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, //	DRT, //  X
//	0x00, //  X
LED_SEG_IMG_B | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	DRT, //  Y
//	LED_SEG_IMG_B | LED_SEG_IMG_E | LED_SEG_IMG_F | LED_SEG_IMG_G, 	//	0x27, //  Y
LED_SEG_IMG_A | LED_SEG_IMG_B | LED_SEG_IMG_D | LED_SEG_IMG_E | LED_SEG_IMG_G, 	//	0x6D  //  z
};

/// bit map image of annunciators: usage: to turn on: ledSegImg |= LED_ANC_LB; turn off: ledSegImg &= ~LED_ANC_LB;
#define	LED_ANC_IMG_MINUS	(1 << LED_D0 )
#define	LED_ANC_IMG_BAT		(1 << LED_D1 )
#define	LED_ANC_IMG_PEAK	(1 << LED_D2 )
#define	LED_ANC_IMG_NET		(1 << LED_D3 )
#define	LED_ANC_IMG_GROSS	(1 << LED_D4 )
#define	LED_ANC_IMG_TOTAL	(1 << LED_D6 )
#define	LED_ANC_IMG_X1000	(1 << LED_D5 )
#define	LED_ANC_IMG_KG		(1 << LED_D7 )

#define	LED_ANC_IMG_MOTION	(1 << LED_D0 )
#define	LED_ANC_IMG_SP1		(1 << LED_D1 )
#define	LED_ANC_IMG_ACKIN	(1 << LED_D2 )
#define	LED_ANC_IMG_ACKOUT	(1 << LED_D3 )
#define	LED_ANC_IMG_RF		(1 << LED_D4 )
#define	LED_ANC_IMG_COZ		(1 << LED_D5 )
#define	LED_ANC_IMG_SP2		(1 << LED_D6 )
#define	LED_ANC_IMG_SP3		(1 << LED_D7 )

#define	LED_ANC_IMG_LB		(1 << LED_SEG_DP )

// -WFC- 2011-03-08
const LED_ANNUNCIATOR_DIGIT_MAP_T	gcaLedAncDigitMap[]	PROGMEM = {
	{ 0, LED_ANC_IMG_MINUS },							//	LED_DISPLAY_ANC_MINUS				0
	{ 0, LED_ANC_IMG_BAT },								//	LED_DISPLAY_ANC_BAT					1
	{ 0, LED_ANC_IMG_PEAK },							//	LED_DISPLAY_ANC_PEAK				2
	{ 0, LED_ANC_IMG_NET },								//	LED_DISPLAY_ANC_NET					3
	{ 0, LED_ANC_IMG_GROSS },							//	LED_DISPLAY_ANC_GROSS				4
	{ 0, LED_ANC_IMG_X1000 },							//	LED_DISPLAY_ANC_X1000				5
	{ 0, LED_ANC_IMG_TOTAL },							//	LED_DISPLAY_ANC_TOTAL				6
	{ 0, LED_ANC_IMG_KG },								//	LED_DISPLAY_ANC_KG					7

	{ 1, LED_ANC_IMG_MOTION },							//	LED_DISPLAY_ANC_MOTION				8
	{ 1, LED_ANC_IMG_SP1 },								//	LED_DISPLAY_ANC_SP1					9
	{ 1, LED_ANC_IMG_ACKIN },							//	LED_DISPLAY_ANC_ACKIN				10
	{ 1, LED_ANC_IMG_ACKOUT },							//	LED_DISPLAY_ANC_ACKOUT				11
	{ 1, LED_ANC_IMG_RF },								//	LED_DISPLAY_ANC_SP1					12
	{ 1, LED_ANC_IMG_COZ },								//	LED_DISPLAY_ANC_COZ					13
	{ 1, LED_ANC_IMG_SP2 },								//	LED_DISPLAY_ANC_SP2					14
	{ 1, LED_ANC_IMG_SP3 },								//	LED_DISPLAY_ANC_SP3					15

	{ 0, LED_ANC_IMG_LB }								//	LED_DISPLAY_ANC_LB					16 not belong direct mapping because it shares with last digit of 5 digit LEDs pannel.
};

// 2011-04-29 -WFC- BYTE gbLbFlag; // PHJ to keep LB anc on when setting units


/**
 * init LED driver chip.
 *
 * History:  Created on 2009/02/12 by Wai Fai Chin
 * 2015-01-19 -WFC- set scan limit after init LED image.
 */

void hw3460_led_init( void )
{
	BYTE i;
	
//	bios_on_board_led( ONLY, BIOS_ON_BOARD_LED_RED );
	// since MAX7221 power up to shutdown mode. We force it to normal mode
	hw3460_led_send_cmd( LED_MAX7221_SHUTDOWN_CMD, LED_MAX7221_SHUTDOWN_NO);
	// Set it in bit map character mode 
	hw3460_led_send_cmd( LED_MAX7221_DECODE_MODE_CMD, LED_MAX7221_DECODE_MODE_NO);

	// normal operation, not in test mode
	hw3460_led_send_cmd( LED_MAX7221_TEST_CMD, LED_MAX7221_TEST_NO); 
	// scan digit position range from 0 to 7. Scan all digits. 
//2015-01-19 -WFC- 	hw3460_led_send_cmd( LED_MAX7221_SCANLIMIT_CMD, 7);

	for ( i=0 ; i<9; i++) {
		hw3460_led_send_cmd( i, 0);		// turn off all digits and annunciators.
	}

	// scan digit position range from 0 to 7. Scan all digits.
	hw3460_led_send_cmd( LED_MAX7221_SCANLIMIT_CMD, 7);				// 2015-01-19 -WFC-

	//hw3460_led_display_string( str );
} // end hw3460_led_init()


/**
 * Send command to LED driver chip.
 *
 * @param	bLedCMD -- led chip command
 * @param	bValue  -- segment pattern or command value.
 *
 * History:  Created on 2009/02/12 by Wai Fai Chin
 * 2013-04-18 -WFC- replaced spi0_transceive() with display_spi_transceive().
 */

void hw3460_led_send_cmd( const BYTE bLedCMD, const BYTE bValue )
{
	HW3460_LED_CS_PORT &= ~( 1 << HW3460_LED_CS);	// Chip select low to select the LED driver chip.
	display_spi_transceive( bLedCMD );
	display_spi_transceive( bValue  );
	HW3460_LED_CS_PORT |= ( 1 << HW3460_LED_CS);	// Chip select high to deselect the LED driver chip.
}/* end hw3460_led_send_cmd(,) */


/**
 * It displays a digit with or without a decimal point.
 *
 * @param	loc		-- location or position of a digit
 * @param	ch		-- character to be display
 * @param	isDecPt	-- is a decimal point at the same digit loacation?, 1==yes, 0== no.
 * 
 * @return none.
 *
 * @note loc 1 is for position 0, 6 is for position 5 etc...
 * 
 * History:  Created on 2009/05/28 by Wai Fai Chin
 */

void hw3460_led_display_a_digit( BYTE loc, const BYTE ch, const BYTE isDecPt )
{
	BYTE ledSegImg;	// LED segment image of a digit.
	
	memcpy_P ( &ledSegImg,  &gcbLED7SegTbl[ch],  sizeof(BYTE));

	if ( isDecPt ) {			// if this location also has the decimal point
		ledSegImg |= 0x80;
	}
	
	if ( loc == 1 ) { // Led LB Annunciator use the decimal point of right most digit.
		// 2011-04-29 -WFC- 		if ( gbBiosRunStatus & BIOS_RUN_STATUS_LB_LED_ON || gbLbFlag )
		if ( gbBiosRunStatus & BIOS_RUN_STATUS_LB_LED_ON )		// 2011-04-29 -WFC-
//		if ( gLedDisplayManager.descr.baAnc[LED_ANC_LOC_LB - LOCOFF].state[ LED_ANC_LB ] ) 
			ledSegImg |= LED_ANC_IMG_LB;
		else
			ledSegImg &= ~LED_ANC_IMG_LB;
	}

	hw3460_led_send_cmd( loc, ledSegImg );
} // end hw3460_led_display_a_digit()


/**
 * It outputs a string of char point by pbStr to LED Display0 device.
 * It starts from the right most digit positon to display the last char of the string first;
 * follow by next left digit position to display the last 2nd char and so on.
 * Decimal point will assigned to next high digit position; this can be accomplished
 * by ORing the next char's memory bit map with 0x80 before call the LED_Display0_CMD.
 * The right most decimal point is use to indicate LB unit. If LB is ON, need to OR the
 * right most digit with 0x80 else AND with 0x7F. 
 *
 * @param	pbStr  -- pointer to string of char. It must use null to marke the end.
 * 
 * @note To turn off LB annunciator, set gbLED_display_unit to SENSOR_UNIT_TMPK;
 *
 * History:  Created on 2009/02/12 by Wai Fai Chin
 */

void hw3460_led_display_string( const BYTE *pbStr )
{
	BYTE i;
	BYTE bStrLen;
	BYTE ch;
	BYTE isDecPt;
	
	bStrLen = 0;
	for (;;) {
		if ( *pbStr == 0 ) break;
		pbStr++;
		bStrLen++;
	}
	
	if ( bStrLen ) {										// ensured that the string is not empty.
		pbStr--;
		i = 1;
		for (;;) {
			isDecPt = FALSE;
			ch = *pbStr;
			if ( ch == '.') {
				bStrLen--;
				isDecPt = TRUE;
				if ( bStrLen > 0) {
					pbStr--;
					ch = *pbStr;
				}
			}
			hw3460_led_display_a_digit( i, ch, isDecPt );

			pbStr--;
			i++;
			if ( i > 5 ) break;
			bStrLen--;
			if ( bStrLen == 0) break;
		} // end for(;;)
	} // end if ( bStrLen ) {}
} // end hw3460_led_display_string()


/**
 * Due to there are only five LED digits for display, the following rules are apply:
 *
 * if a negative number length == 6 then set Led_Minus to TRUE to be display.
 * if a negative number length > 6 or a positive number length > 5,  then tell
 * caller that it may needs to scale down a number by 1000 if decimal point > 4th place to the right,
 *
 * @param	pbStr  -- an input string to be validate.
 * @param	pbStrLen		-- string length of pbStr not count the null char.
 *
 * @return
 *	if the input string is not a number,
 *		return LED_FORMAT_CTRL_NOT_NUMBER.
 *	if the string is ready for display without any control format modification,
 *		return LED_FORMAT_CTRL_NORMAL.
 *  if the string needs to display the extract minus sign,
 *		return LED_FORMAT_CTRL_MINUS.
 *  if the number has too many digits to fit into the LED digits.
 *		return LED_FORMAT_TOO_MANY_DIGITS.
 *
 * History:  2009/02/17  Original coded by Wai Fai Chin
*/


//BYTE hw3460_led_get_format_ctrl( BYTE *pbStr,BYTE *pbStart, BYTE *pbDecPtPosition, INT8 *pbDigitLength, BYTE *pbStrLen )
BYTE hw3460_led_get_format_ctrl( BYTE *pbStr, BYTE *pbStrLen )
{
	BYTE status;
	INT8 bDigitLength;

	// calculate the length of the string
	for ( *pbStrLen=0; *pbStrLen < 10;  (*pbStrLen)++) {
		if ( 0 == pbStr[*pbStrLen] ) break;
	}

	bDigitLength = is_a_led_number( pbStr );
	status = LED_FORMAT_CTRL_NORMAL;
	if ( 0 == bDigitLength ) {
		status = LED_FORMAT_CTRL_NOT_NUMBER;
	}
	
	// check for number length and put six '-' into pbStr[] if require.
	if ( *pbStrLen >= (HW3460_LED_MAX_DIGIT_LENGTH + 1) ) {
		if ( bDigitLength == -(HW3460_LED_MAX_DIGIT_LENGTH + 1) ) {
			status = LED_FORMAT_CTRL_MINUS;
		}
		else if ( bDigitLength > HW3460_LED_MAX_DIGIT_LENGTH || bDigitLength < -(HW3460_LED_MAX_DIGIT_LENGTH + 1) ) {
			// tell the caller need to scale down by 1000 to able display on the digits.
			status = LED_FORMAT_TOO_MANY_DIGITS;
		}
	}// end if ( strLen > (HW3460_LED_MAX_DIGIT_LENGTH + 1) ) 

	return status;

}// end hw3460_led_get_format_ctrl(,)


/* -WFC- 2011-03-08 removed it and placed it in led_display.h as led_display_set_intensity( level )
void display_auto_dim_timer (void)
{
	led_display_send_hw_cmd( LED_HW_INTENSITY_CMD, 0 );		// set LEDs to dim

}
*/

/**
 * It maps display of annunciator behavior based on the pAncStates.
 *  0 == off; other number is on.
 *
 * @param  pLedM		-- points to led display manager object.
 * @param  pAncStateBuf	-- points to led annunciators states buffer, it tells which annunciators will be on or off.
 * @return none.
 *
 * History:  Created on 2011-03-08 by Wai Fai Chin
 *
 */

/// address of a digit.
BYTE gcbAddrTable[ HW3460_LED_MAX_NUM_ANC_DIGIT ] 		PROGMEM = { LED_ANC_IMAGE_BUFFER_0_DIGIT_ADDR,	LED_ANC_IMAGE_BUFFER_1_DIGIT_ADDR };

void hw3460_led_maps_annunciators( LED_DISPLAY_MANAGER_CLASS *pLedM, BYTE *pAncStateBuf )
{
  BYTE	i, j;
  BYTE	imgData;
  BYTE	digitAddr;
  LED_ANNUNCIATOR_DIGIT_MAP_T	ancMap;
  BYTE	ledDigitImg[ HW3460_LED_MAX_NUM_ANC_DIGIT ];

	for (i = 0; i < HW3460_LED_MAX_NUM_ANC_DIGIT; i++)
		ledDigitImg[i] = 0;	// turn off segments used for annunciators and bargraph

	// build led segment images based on annunciator image map.
	for( i=0; i < LED_DISPLAY_MAX_DIRECT_UPDATE_ANC_INDEX; i++ )	{
		if ( pAncStateBuf[i] ) {
			memcpy_P ( &ancMap,  &gcaLedAncDigitMap[i],  sizeof(LED_ANNUNCIATOR_DIGIT_MAP_T));
			ledDigitImg[ ancMap.digitLocation ] |= ancMap.imageData;
		}
	}

	// walk through all LED digits that contains annunciator image.
	for ( i = 0; i < HW3460_LED_MAX_NUM_ANC_DIGIT; i++ ) {
		memcpy_P ( &digitAddr,  &gcbAddrTable[i],  sizeof(BYTE));
		hw3460_led_send_cmd( digitAddr, ledDigitImg[ i ] );
	}

} // end hw3460_led_maps_annunciators()

