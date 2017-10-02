/*! \file hw4260B_led.h \brief external LED display related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 - 2011 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATMEGA1281
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2009/02/12 by Wai Fai Chin
//  History:  Modified for 4260B on 2011-12-12 by Denis Monteiro
// 
/// \ingroup driver_cpu_extern
/// \defgroup hw4260B_led manage external LED driver for PCB4260B.(hw4260B_led.c)
/// \code #include "hw4260B_led.h" \endcode
/// \par Overview
///   This is a drive for LED driver on the hardware 4260B PCB.ied sensor object.
///   This module will use by Challenger3, MSI3550, etc.. It is not a generic core
///   driver of SCaleCore. It is product specific driver.
//
// ****************************************************************************
//@{
 

#ifndef MSI_HW4260B_LED_H
#define MSI_HW4260B_LED_H

#include  "config.h"
#include  "spi.h"
/*!
    The AS 1116 LED display driver data format is 8 bit COMMAND follow by 8 bit VALUE.
  COMMAND is the register address of AS1116 chip. Each COMMAND must accompany
  with a value. The value could be a 7 segment map, intensity value, or mode etc.
  MSB shifts out first. Each bit is clock in on the rising edge of the clock.
  The SCK clock must be low when the LED driver chip is not selected.

  Usage:
  \code
  
    hw4260B_led_send_cmd_toboth( LED_AS1116_TEST_CMD, LED_AS1116_TEST_YES); 	// set the LED driver chip in test mode.
    hw4260B_led_send_cmd_toboth( LED_AS1116_INTENSITY_CMD, 12);				// set intensity at level 12.
	memcpy_P ( &led_char3,  &gcbLED7SegTbl['3'],  sizeof(BYTE));
    hw4260B_led_send_cmd( 2, led_char3);									// Display '3' on digit 1.

  \endcode

*/

/*!
  \brief LED Annunciator lookup table of LED Digit location and image data.
  Created by Wai Fai Chin, 	2010-08-16
*/
typedef struct LED_ANNUNCIATOR_DIGIT_MAP_TAG {
								/// Logical digit location of this annunciator group. e.g. A LED digit can hold up to 8 annunciator if 1 annunciator per segment. BYTE ledDigitImage[2] can hold up to 16 annunciators.
  BYTE	digitLocation;
								/// image data of an annunciator in this diditLocation. Some annunciator needs two or more segments to form an annunciator.
  BYTE	imageData;
}LED_ANNUNCIATOR_DIGIT_MAP_T;

// 2011-04-29 -WFC- extern BYTE	gbLbFlag;	//	PHJ to keep LB anc on when setting units


/*
  The following constant are define to work with hw4260B_led_send_cmd(,) functions.
*/
#define LED_AS1116_DECODE_MODE_CMD   9
#define LED_AS1116_DECODE_MODE_NO    0

#define LED_AS1116_FEATURE_MODE_CMD 14
#define LED_AS1116_FEATURE_MODE_CLK  1
//External clock active.
//0 = Internal oscillator is used for system clock.
//1 = Pin CLK of the serial interface operates as system clock input.

#define LED_AS1116_INTENSITY_CMD    10
// intensity value range from 0 to 15

#define LED_AS1116_SCANLIMIT_CMD    11
// scan limit value range from 0 to 7. 7 means scan from digit 0 to digit 7 

#define LED_AS1116_SHUTDOWN_CMD     12
//#define LED_AS1116_SHUTDOWN_YES      0
//#define LED_AS1116_SHUTDOWN_NO       1
#define LED_AS1116_SHUTDOWN_YES    128
#define LED_AS1116_SHUTDOWN_NO     129

#define LED_AS1116_TEST_CMD         15
#define LED_AS1116_TEST_YES          1
#define LED_AS1116_TEST_NO           0


#define HW4260B_LED_MAX_DIGIT_LENGTH	CONFIG_NUMBER_DIGITS // 2012-07-31 -DLM-
#define HW4260B_LED_MAX_NUM_ANC_DIGIT	2

#define LED_FORMAT_CTRL_NOT_NUMBER	0
#define LED_FORMAT_CTRL_NORMAL		1
/// need to display minus annunciator.
#define LED_FORMAT_CTRL_MINUS		2
/// need to normalize the input number to x1000.
#define LED_FORMAT_CTRL_X1000		3
/// digits are too long to display in the LED digits.
#define LED_FORMAT_TOO_MANY_DIGITS	4

//   7 6 5 4 3 2 1 0
//  DP A B C D E F G  segments where DP == decimal point, other annunciators.
#define	LED_SEG_G	0
#define	LED_SEG_F	1
#define	LED_SEG_E	2
#define	LED_SEG_D	3
#define	LED_SEG_C	4
#define	LED_SEG_B	5
#define	LED_SEG_A	6
#define	LED_SEG_DP	7

#define	LED_SEG_IMG_G	(1 << LED_SEG_G)
#define	LED_SEG_IMG_F	(1 << LED_SEG_F)
#define	LED_SEG_IMG_E	(1 << LED_SEG_E)
#define	LED_SEG_IMG_D	(1 << LED_SEG_D)
#define	LED_SEG_IMG_C	(1 << LED_SEG_C)
#define	LED_SEG_IMG_B	(1 << LED_SEG_B)
#define	LED_SEG_IMG_A	(1 << LED_SEG_A)
#define	LED_SEG_IMG_DP	(1 << LED_SEG_DP)

#define LED_D0		0
#define LED_D1		1
#define LED_D2		2
#define LED_D3		3
#define LED_D4		4
#define LED_D5		5
#define LED_D6		6
#define LED_D7		7

//   7 6 5 4 3 2 1 0
//  DP A B C D E F G  segments where DP == decimal point, other annunciators.
/// bit flag of annunciators
#define	LED_ANC_RF		LED_D0
#define	LED_ANC_ACKIN	LED_D1
#define	LED_ANC_TOTAL	LED_D2

#if ( LED_FILTER_WINDOW_REV_C == TRUE )
	#define	LED_ANC_PEAK	LED_D4
	#define	LED_ANC_NET		LED_D3
#else
	#define	LED_ANC_PEAK	LED_D3
	#define	LED_ANC_NET		LED_D4
#endif
#define	LED_ANC_LB		LED_D5
#define	LED_ANC_KG		LED_D6
#define	LED_ANC_X1000	LED_D7

#define	LED_ANC_SP1		LED_D0
#define	LED_ANC_SP2		LED_D1
#define	LED_ANC_SP3		LED_D2
#define	LED_ANC_BAT		LED_D3
#define	LED_ANC_STABLE	LED_D4
#define	LED_ANC_COZ		LED_D5


//
#define LED_ANC_IMAGE_BUFFER_0_DIGIT_ADDR		7		// annunciator image buffer 0 has digit address 7.
// The following annunciators are in annunciator image buffer 0 digit address.
/// bit map image of annunciators: usage: to turn on: ledSegImg |= LED_ANC_LB; turn off: ledSegImg &= ~LED_ANC_LB;
#define	LED_ANC_IMG_RF		(1 << LED_D0 )
#define	LED_ANC_IMG_ACKIN	(1 << LED_D1 )
#define	LED_ANC_IMG_TOTAL	(1 << LED_D2 )
#if ( LED_FILTER_WINDOW_REV_C == TRUE )
	#define	LED_ANC_IMG_PEAK	(1 << LED_D4 )
	#define	LED_ANC_IMG_NET		(1 << LED_D3 )
#else
	#define	LED_ANC_IMG_PEAK	(1 << LED_D3 )
	#define	LED_ANC_IMG_NET		(1 << LED_D4 )
#endif
#define	LED_ANC_IMG_LB		(1 << LED_D5 )
#define	LED_ANC_IMG_KG		(1 << LED_D6 )
#define	LED_ANC_IMG_X1000	(1 << LED_D7 )

#define LED_ANC_IMAGE_BUFFER_1_DIGIT_ADDR		8		// annunciator image buffer 1 has digit address 8.
// The following annunciators are in annunciator image buffer 1.
#define	LED_ANC_IMG_SP1		(1 << LED_D0 )
#define	LED_ANC_IMG_SP2		(1 << LED_D1 )
#define	LED_ANC_IMG_SP3		(1 << LED_D2 )
#define	LED_ANC_IMG_BAT		(1 << LED_D3 )
#define	LED_ANC_IMG_STABLE	(1 << LED_D4 )
#define	LED_ANC_IMG_COZ		(1 << LED_D5 )



#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )

/*!
  AVR CPU IO PORT for Linear Tech ADC chip select.
*/
#define HW4260B_LED_CS_PORT	PORTC

/*!
  AVR CPU IO PORT pin for LED driver chip select.
  Usage:
  \code
	HW4260B_LED_CS_PORT &= ~( 1 << HW4260B_LED_CS);	// Chip select low to select the LED driver chip.
	HW4260B_LED_CS_PORT |= ( 1 << HW4260B_LED_CS);	// Chip select high to deselect the LED driver chip.
  \endcode
  
*/
#define HW4260B_LED_CS_1		PC2
#define HW4260B_LED_CS_2		PC3

#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )
/*!
  AVR CPU IO PORT for Linear Tech ADC chip select.
*/
#define HW4260B_LED_CS_PORT	((&PORTR)->OUT)

/*!
  AVR CPU IO PORT pin for LED driver chip select.
  Usage:
  \code
	HW4260B_LED_CS_PORT &= ~( 1 <<HW4260B_LED_CS);	// Chip select low to select the LED driver chip.
	HW4260B_LED_CS_PORT |= ( 1 << HW4260B_LED_CS);	// Chip select high to deselect the LED driver chip.
  \endcode
*/
#define HW4260B_LED_CS_1		1
#define HW4260B_LED_CS_2		0

#endif // ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )

void hw4260B_led_send_digit(BYTE loc, BYTE ledSegImg);
void hw4260B_led_display_a_digit( BYTE loc, const BYTE ch, const BYTE isDecPt );
void hw4260B_led_display_string( const BYTE *pbStr );
void hw4260B_led_init( void );
void hw4260B_led_send_cmd_toboth( const BYTE bLedCMD, const BYTE bValue );
void hw4260B_led_send_cmd_cs( const BYTE bCS, const BYTE bLedCMD, const BYTE bValue );
BYTE hw4260B_led_get_format_ctrl( BYTE *pbStr, BYTE *pbStrLen );

#endif  // MSI_HW4260B_LED_H

//@}
