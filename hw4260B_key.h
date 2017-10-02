/*! \file hw4260B_key.h \brief external button related functions.*/
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
// Software layer:  Hardware
//
//  History:  Created on 2009/05/26 by Wai Fai Chin
//  History:  Modified for 4260B on 2011/05/26 by Wai Fai Chin
// 
/// \ingroup product_module
/// \defgroup hw4260B_key manage external key driver for PCB4260B.(hw4260B_key.c)
/// \code #include "hw4260B_key.h" \endcode
/// \par Overview
///   This is a drive for Key driver on the hardware 4260B PCB.
///   This module will use by Challenger3, MSI3550, etc.. It is not a generic core
///   driver of SCaleCore. It is product specific driver.
//
// ****************************************************************************
//@{
 

#ifndef MSI_HW4260B_KEY_H
#define MSI_HW4260B_KEY_H

#include  "config.h"

// #define KEY_MAX_KEYBUF_SIZE  8
#define KEY_MAX_KEYBUF_SIZE  2

#define SW1_KEY         1
#define SW2_KEY         2
#define SW3_KEY         4
#define SW4_KEY         8

/*
PB7  7 -- TARE KEY	8 after a nibble right shifted.
PB6  6 -- USER		4
PB5	 5 -- ZERO		2
PB4	 4 -- POWER		1
*/

// the following are the single key pressed code.
// front panel keypad.
#define KEY_NO_NEW_KEY	0x0
#define	KEY_POWER_KEY	0x01
#define KEY_ZERO_KEY	0x02
#define KEY_USER_KEY	0x04
#define KEY_TARE_KEY	0x08
// RF remote keypad
#define	KEY_TOTAL_KEY	0x10
#define	KEY_NET_G_KEY	0x20
#define KEY_USER2_KEY	0x40
#define	KEY_VIEW_KEY	0x80

// The following are the combination key code
#define KEY_SETUP_KEY			5

// 2013-04-26 -WFC- v  //2013-09-03 -DLM
#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281)
// The following are for the RF Remote				//	PHJ
#define RF_LEARN		128							//	PHJ
#define RF_IN			32							//	PHJ
#define SET_RF_LEARN	PORTA |= RF_LEARN			// Pulse High to Learn  2013-04-26 -WFC-
#define CLEAR_RF_LEARN  PORTA &= ~RF_LEARN			// Release learn input to receiver decoder. 2013-04-26 -WFC-
#define IS_RF_LED_ON    (PINA & RF_IN)				// is rf remote control receiver ON?  2013-04-26 -WFC-
#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )  //2013-09-03 -DLM
#define RF_LEARN		0x10
#define RF_IN			32
#define SET_RF_LEARN	(&PORTF)-> OUT |= RF_LEARN				// Pulse High to Learn  2013-04-26 -WFC-
#define CLEAR_RF_LEARN  (&PORTF)-> OUT &= ~RF_LEARN				// Release learn input to receiver decoder. 2013-04-26 -WFC-
#define IS_RF_LED_ON    (PORT_GetPortValue( &PORTE ) & RF_IN)	// is rf remote control receiver ON?  2013-04-26 -WFC-
#endif
// 2013-04-26 -WFC- ^  //2013-09-03 -DLM


BYTE hw4260B_key_get( BYTE *pbKey );
void hw4260B_key_init( void );
void hw4260B_key_read( void );
BYTE hw4260B_key_detect_pressed( void );		// 2011-04-28 -WFC-


#endif  // MSI_HW4260B_KEY_H

//@}













