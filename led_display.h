/*! \file led_display.h \brief High Level LED display related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009, 2010, 2011 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATMEGA1281, ATXMEGAnnnn
// OS:       independent
// Compiler: avr-gcc
// Software layer:  .h file is Middle layer, functions names are the same, but link to hardware specified low level functions.
//
//  History:  Created on 2009-05-28 by Wai Fai Chin
//  History:  Modified for 4260 on 2011-12-12 by Denis Monteiro
// 
/// \ingroup product_module
/// \defgroup led_display manages LED display behavior.(led_display.c)
/// \code #include "led_display.h" \endcode
/// \par Overview
//   This is a high level LED display manager module.
//	 It auto selects display module based on product configuration.
//   all modules must maintain the same function names and macro defines;
//	 actual functions implementation are in the hardware specific module.
//   It manages the behavior of LED display.
//
// ****************************************************************************
//@{
 

#ifndef MSI_LED_DISPLAY_H
#define MSI_LED_DISPLAY_H

#include  "config.h"
#include  "timer.h"
#include  "bios.h"

void	led_display_init( void );
void	led_display_manager( void );
void	led_display_copy_string_and_pad( BYTE *pbDestStr,  BYTE *pbSrcStr, BYTE lastLoc );
void	led_display_copy_string_and_pad_P( BYTE *pbDestStr, const BYTE *pbSrcStr );
void	led_display_format_dash_line( void );
BYTE	led_display_format_float_string( float fV, MSI_CB *pCB, BYTE needX1000, BYTE *pOutStr );
BYTE	led_display_format_high_resolution_float_string( float fV, MSI_CB *pCB, BYTE needX1000, BYTE *pOutStr ); // 2011-04-29 -WFC-
void	led_display_format_output( BYTE *pbSrcStr );
void	led_display_turn_all_annunciators( BYTE state );
void	led_display_turn_all_leds( BYTE state );
void	led_display_turn_all_led_digits( BYTE state );
void	led_display_string_P( const BYTE *pbSrcStr );

extern const char gcStrMaxSpaces[]	PROGMEM;
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )
extern BYTE gbNChDisplayed; // number of char displayed
#endif // ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )

// the above functions, variable and constant members are available for all product that has display hardware module.

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 )
#include "led_display_hw3460.h"
#endif // ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 )

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )
// #include "led_display_hw7300.h" // 25-01-2013 -DLM-
	#if ( CONFIG_LCD == LCD_8000 )
		#include "led_display_hw8000.h"
	#else
		#include "led_display_hw7300.h"
	#endif
#endif // ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI)
#include "led_display_hw6954.h"
#endif // ( CONFIG_PRODUCT_AS	==CONFIG_AS_HD  || CONFIG_PRODUCT_AS == CONFIG_AS_HLI)

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )
#include "led_display_hw4260B.h"
#endif // ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )

#if  ( CONFIG_TEST_LED_DISPLAY_MODULE == TRUE)
extern struct pt gLedTest_pt;
void test_led_display_manager_thread( struct pt *pThread );
#endif

/*!
 \brief -WFC- 2010-08-16, redesigned to fix annunciators bugs and save 70 bytes of RAM memory.
 LED_DISPLAY_ANC_INDEX_nnnn are high level definition of an annunciator. This high
 level definition name will not change so make it easy to port to different hardware platform.
 This high level number will map to low level hardware specific LED driver board.
 For example, simple LED driver may just need 1D array direct map.
 The complex LED hardware driver may need 2D array map.

 The hw6954 board needs an address and imageData  to map an annunciator.
 LED_DISPLAY_ANC_INDEX_LB, use a lookup table to fetch (digitLocation, imageData),
 LB annunciator send a LED driver command (address, imageData) ==>  (13, 1)
 to light up LB annunciator.  The underline low level hardware mapping logics
 are hidden from application level module. This will free programmer from
 try to remember all mapping details and slow development time.

 baAnc_state[]:
  state == 255 steady on;
  220<state<255 stay on until timeout reach 220 then is off;
  219 is always blink.
  200<state<219 blink until it reach timeout and stay on when it reaches 200; Thus 203 will blink 3 times then stay steady on.
  1<state<200 blink until it reach 0;

*/

typedef struct LED_BEHAVIOR_DESCRIPTOR_TAG {
											/// a string buffer 1 use as a main string
  BYTE  str1[ LED_MAX_STRING_SIZE + 1];
											/// a string buffer 2 use as 2nd string of error msg.
  BYTE  str2[ LED_MAX_STRING_SIZE + 1];
											/// annunciators state, see the above for detail explanation.
  BYTE  baAnc_state[ LED_DISPLAY_MAX_ANC_INDEX ];
											/// display mode, >0 == error message, 0==normal,
  BYTE	mode;
											/// digit blink position. range 0 to 7, 0==string no blink, 1 == blink the Left most digit, 7== blink the right most digit. 5 digits plused '-' and '.' = 7 characters.
  BYTE	digitBlinkPos;
											///	digit brightness, range 0 to 31, 0 = dimmest,  15 is the brightest. 31 means DayTime mode at full intensity, bit4 means day light, bit3 to 0 is intensity value..
  BYTE	digitIntensity;
											/// back light intensity, range 0 to 15, 0 = dimmest,  15 is the brightest.
  BYTE	backLightIntensity;
											/// number of lit segments in bargraph
  BYTE	barGraphValue;
											/// global LEDS, status, bit0, 1== dimmed and sleep, 2011-06-15 -WFC-
  BYTE	status;
}LED_BEHAVIOR_DESCRIPTOR_T;

/// Entire LED segments had dimmed. 2011-06-15 -WFC-
#define LED_BEHAVIOR_STATUS_DIMMED_SLEEP		0X01

typedef struct LED_DISPLAY_MANAGER_CLASS_TAG {
											/// main parent thread.
  struct pt m_pt;
											/// string display thread
  struct pt string_pt;
											/// annunciator display thread
  struct pt anc_pt;
											/// refresh timer to update new data
  TIMER_T	refreshTimer;
											/// digit blinking timer
  TIMER_T	digitBlinkTimer;
											/// annunciator blinking timer
  TIMER_T	ancBlinkTimer;
											/// LED behaviors descriptor.
  LED_BEHAVIOR_DESCRIPTOR_T	descr;
}LED_DISPLAY_MANAGER_CLASS;

/// led display manager object which is a clone-able thread.
extern LED_DISPLAY_MANAGER_CLASS	gLedDisplayManager;

char	led_display_manage_string_thread( LED_DISPLAY_MANAGER_CLASS *pLedM );
char	led_display_manage_annunciator_thread( LED_DISPLAY_MANAGER_CLASS *pLedM );

/** \def led_display_is_led_dimmed
 * Is led dimmed and in sleep mode ?
 * @return true if dimmed.
 *
 * History:  Created on 2011-06-15 by Wai Fai Chin
 */
#define led_display_is_led_dimmed_sleep()		( gLedDisplayManager.descr.status & LED_BEHAVIOR_STATUS_DIMMED_SLEEP )

/** \def led_display_set_led_dimmed_sleep_flag
 * It sets led dimmed and sleep mode flag?
 * @return true if dimmed.
 *
 * History:  Created on 2011-06-15 by Wai Fai Chin
 */
#define led_display_set_led_dimmed_sleep_flag()		( gLedDisplayManager.descr.status |= LED_BEHAVIOR_STATUS_DIMMED_SLEEP )

/** \def led_display_clear_led_dimmed_sleep_flag
 * It clears led dimmed and sleep mode flag?
 * @return true if dimmed.
 *
 * History:  Created on 2011-06-15 by Wai Fai Chin
 */
#define led_display_clear_led_dimmed_sleep_flag()		( gLedDisplayManager.descr.status &= ~LED_BEHAVIOR_STATUS_DIMMED_SLEEP )

/** \def led_display_update_annunciator_blink_timer
 * Update annunciator blink timer to system timer.
 *
 * History:  Created on 2011-07-21 by Wai Fai Chin
 */
#define led_display_update_annunciator_blink_timer()		( gLedDisplayManager.ancBlinkTimer.start = gTTimer_mSec )


/** \def led_display_set_annunciator_blink_timer_interval
 * Update annunciator blink timer to system timer.
 *
 * History:  Created on 2011-07-21 by Wai Fai Chin
 */
#define led_display_set_annunciator_blink_timer_interval( mSec )	( gLedDisplayManager.ancBlinkTimer.interval = ( mSec ) )



#endif  // MSI_LED_DISPLAY_H

//@}
