/*! \file led_display.c \brief High Level LED display related functions.*/
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
// Software layer:  Application
//
//  History:  Created on 2009-05-28 by Wai Fai Chin
//  History:  Edited for Helicopter on 2010-04-27 by Pete Jensen
//  History:  Edited for 4260 on 2011-12-12 by Denis Monteiro
// 
//   This is a high level LED display manager module.
//   It manages the behavior of LED dispaly.
//
// ****************************************************************************
// 2011-06-09 -WFC- rewrote to separate it in different file based on product configuration in compile time.
 
#include	"led_display.h"

/// led display manager object which is a clone-able thread.
LED_DISPLAY_MANAGER_CLASS	gLedDisplayManager;

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 )
#include "led_display_hw3460.c"
#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )
	#if ( CONFIG_LCD == LCD_8000 )
		#include "led_display_hw8000.c"
	#else
		#include "led_display_hw7300.c"
	#endif
#elif  ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI )
#include "led_display_hw6954.c"
#elif  ( CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )
#include "led_display_hw4260B.c"
#endif
