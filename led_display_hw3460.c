/*! \file led_display_hw3460.c \brief Middle layer Level LED display related functions.*/
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
//  History:  Created on 2011-06-09 by Wai Fai Chin
// 
//   This is a middle layer level LED display manager module.
//   It implements product specific features such extract '-' sign,
//   and annunciators only apply to this product.
//   It manages the behavior of LED display.
//
// ****************************************************************************
 

#include	"led_display.h"
#include	"commonlib.h"
#include	"nv_cnfg_mem.h"
#include	"led_lcd_msg_def.h"
#include	"scalecore_sys.h"		// 2015-01-16 -WFC-
#include	"self_test.h"			// 2015-01-16 -WFC-

void	led_display_build_annunciator_on_off( LED_DISPLAY_MANAGER_CLASS *pLedM, BYTE *pAncStateBuf );
void	led_display_build_blink_annunciator_on_off( LED_DISPLAY_MANAGER_CLASS *pLedM, BYTE *pAncStateBuf, BYTE needUpdateState );

const char gcStrMaxSpaces[]	PROGMEM = "     ";

/**
 * Init led display manager object.
 *
 * History:  Created on 2009/05/28 by Wai Fai Chin
 * 2012-09-20 -WFC- Copied led intensity codes from panel_main_power_up().
 * 2015-01-16 -WFC- Set intensity to lowest if in power saving state.
 */

void led_display_init( void )
{
	BYTE	i;
	BYTE	*pB;
	
	pB = (BYTE *) &gLedDisplayManager;
	
	for ( i=0; i < (BYTE) sizeof( LED_DISPLAY_MANAGER_CLASS);	i++) {
		*pB++ = 0;
	}

	// 2012-09-10 -WFC- v
	if ( SYS_POWER_SAVE_STATE_ACTIVE == gbSysPowerSaveState )		// 2015-01-16 -WFC-
		led_display_set_intensity( 0 );								// 2015-01-16 -WFC-
	else if ( gtSystemFeatureFNV.ledIntensity )						// turn up if NOT in auto intensity
		led_display_set_intensity( ((1 << gtSystemFeatureFNV.ledIntensity ) - 2 ) ); // dimming LED based on user led intensity setting.
	else														// turn up if in auto
		led_display_set_intensity(((BYTE)(gaLSensorDescriptor[ SENSOR_NUM_LIGHT_SENSOR ].curADCcount >> 6)) );	// dimming LED based on light sensor ADC count.
	// 2012-09-20 -WFC- ^
	timer_mSec_set( &(gLedDisplayManager.refreshTimer), TT_500mSEC );
}// end led_display_init() 


/**
 * Manage to present data and annunciators on the LED board.
 *
 * @param	pbStr	-- point to string to be output.
 *
 * History:  Created on 2009-05-28 by Wai Fai Chin
 * 2010-10-11 -WFC- CHI required only turn on "load" back light in night mode.
 * 2011-01-14 -WFC- uses 500mSec refresh rate instead of 200mSec to fix problem due to HLI use 5V that could cause uneven brightness display of bar graph LEDs in cold temperature.
 *
 */

void led_display_manager( void )
{
	BYTE isFOrceDisplay;

	isFOrceDisplay = LED_DISPLAY_MODE_UPDATE_NOW & gLedDisplayManager.descr.mode;
	if ( timer_mSec_expired( &(gLedDisplayManager.refreshTimer)) || isFOrceDisplay ) {
		timer_mSec_set( &(gLedDisplayManager.refreshTimer), TT_200mSEC);
		if ( isFOrceDisplay ) {
			gLedDisplayManager.descr.mode &= ~LED_DISPLAY_MODE_UPDATE_NOW;		// reset force update now flag
			PT_INIT( &gLedDisplayManager.string_pt );							// init string display thread
		}
		led_display_manage_annunciator_thread( &gLedDisplayManager );		// -WFC- 2011-03-16
		if ( !led_display_is_led_dimmed_sleep()) {						// if LED segments were not in dimmed and in sleep mode, then
			led_display_manage_string_thread( &gLedDisplayManager );
		}
		// 2010-08-03 -WFC- led_display_manage_annunciator_thread( &gLedDisplayManager );
	}
	// -WFC- 2010-08-18	led_display_manage_annunciator_thread( &gLedDisplayManager );	// 2010-08-03 -WFC-

}// end led_display_manager()


/**
 * It manages display of string behavior based on the LED_BEHAVIOR_DESCRIPTOR_T descriptor.
 *
 * @param  pLedM	-- points to led display manager object.
 *
 * @note in order to make digit blinking to work, caller must pad space chars on the left,
 *    such that the total digit length is 5 excluded a decimal point char because
 *  decimal point share a same digit.
 *  for example, 325 left justfy must set as 325__  by padded two space characters.
 *  325.__ still need two space chars. Note that the string length is 6 instead of 5 without the decimal point.
 *  blinkPosition 1 is the left most digit, in this case, a 3.
 *
 * History:  Created on 2009/05/27 by Wai Fai Chin
 */

//PT_THREAD( led_display_manage_string_thread( LED_DISPLAY_MANAGER_CLASS *pLedM )) // Doxygen cannot handle this macro
char led_display_manage_string_thread( LED_DISPLAY_MANAGER_CLASS *pLedM )
{
  BYTE *ptrB;
  BYTE index;

  PT_BEGIN( &(pLedM-> string_pt) );

	if ( LED_DISPLAY_MODE_NORMAL == (pLedM-> descr.mode ) ) {
		if ( pLedM-> descr.digitBlinkPos  ) {
			led_display_string( pLedM-> descr.str1 );
			timer_mSec_set( &(pLedM-> digitBlinkTimer), TT_300mSEC );
			PT_WAIT_UNTIL( &(pLedM-> string_pt), timer_mSec_expired( &(pLedM-> digitBlinkTimer) ) );	// wait for .3 seconds,
			copy_until_match_char_xnull( pLedM-> descr.str2, pLedM-> descr.str1, 0, LED_MAX_STRING_SIZE );
			index = pLedM-> descr.digitBlinkPos -1;
			ptrB = & (pLedM-> descr.str2[ index ]);

			if ( '.' == *ptrB ) {
				copy_until_match_char_xnull( ptrB, &(pLedM-> descr.str1[ index + 1 ]), 0, LED_MAX_STRING_SIZE - index );
			}
			else
				*ptrB = ' ';

			led_display_string( pLedM-> descr.str2 );
			timer_mSec_set( &(pLedM-> digitBlinkTimer), TT_300mSEC );
			PT_WAIT_UNTIL( &(pLedM-> string_pt), timer_mSec_expired( &(pLedM-> digitBlinkTimer) ) );	// wait for .3 seconds,
		}
		else {
			led_display_string( pLedM-> descr.str1 );
		}
	}
	else if ( LED_DISPLAY_MODE_BLINK_STRING == (pLedM-> descr.mode ) ) {
		led_display_string( pLedM-> descr.str1 );
		timer_mSec_set( &(pLedM-> digitBlinkTimer), TT_400mSEC );
		PT_WAIT_UNTIL( &(pLedM-> string_pt), timer_mSec_expired( &(pLedM-> digitBlinkTimer) ) );	// wait for .4 seconds,

		strcpy_P( pLedM-> descr.str2, gcStrMaxSpaces);
		led_display_string( pLedM-> descr.str2 );
		timer_mSec_set( &(pLedM-> digitBlinkTimer), TT_400mSEC );
		PT_WAIT_UNTIL( &(pLedM-> string_pt), timer_mSec_expired( &(pLedM-> digitBlinkTimer) ) );	// wait for .3 seconds,
	}
	else if ( LED_DISPLAY_MODE_ERROR_MSG_1SHOT == (pLedM-> descr.mode ) ) {
		led_display_string( pLedM-> descr.str1 );
		timer_mSec_set( &(pLedM-> digitBlinkTimer), TT_700mSEC );
		PT_WAIT_UNTIL( &(pLedM-> string_pt), timer_mSec_expired( &(pLedM-> digitBlinkTimer) ) );	// wait for 0.7 seconds,
		pLedM-> descr.mode = LED_DISPLAY_MODE_NORMAL;
		PT_RESTART( &(pLedM-> string_pt) );
	}
	// need to display total mode here??? or somewhere else..
	else {

		led_display_string( pLedM-> descr.str1 );
		timer_mSec_set( &(pLedM-> digitBlinkTimer), TT_1SEC );
		PT_WAIT_UNTIL( &(pLedM-> string_pt), timer_mSec_expired( &(pLedM-> digitBlinkTimer) ) );	// wait for 1 seconds,

		led_display_string( pLedM-> descr.str2 );
		timer_mSec_set( &(pLedM-> digitBlinkTimer), TT_1SEC );
		PT_WAIT_UNTIL( &(pLedM-> string_pt), timer_mSec_expired( &(pLedM-> digitBlinkTimer) ) );	// wait for 1 seconds,
	}

  PT_END( &(pLedM-> string_pt) );

} // end led_display_manage_string_thread()


/**																//	PHJ
 * It manages display of string behavior based on the LED_BEHAVIOR_DESCRIPTOR_T descriptor.
 *
 * @param  pLedM	-- points to led display manager object.
 *
 * @note in order to make digit blinking to work, caller must pad space chars on the left,
 *    such that the total digit length is 5 excluded a decimal point char because
 *  decimal point share a same digit.
 *  for example, 325 left justfy must set as 325__  by padded two space characters.
 *  325.__ still need two space chars. Note that the string length is 6 instead of 5 without the decimal point.
 *  blinkPosition 1 is the left most digit, in this case, a 3.
 *
 * History:  Created on 2009/05/27 by Wai Fai Chin
 */

/**
 * It manages display of annunciator behavior based on the LED_BEHAVIOR_DESCRIPTOR_T descriptor.
 *
 * @param  pLedM	-- points to led display manager object.
 *
 * History:  Created on 2009-05-27 by Wai Fai Chin
 * 2010-08-17 -WFC-	rewrote for remote HD display.
 * 2012-09-28 -WFC- 40 timer ticks off if LED is in sleep mode.
 */

#define	LED_DISPLAY_BLINK_TIME	TT_150mSEC

//PT_THREAD( led_display_manage_annunciator_thread( LED_DISPLAY_MANAGER_CLASS *pLedM )) // Doxygen cannot handle this macro
char led_display_manage_annunciator_thread( LED_DISPLAY_MANAGER_CLASS *pLedM )
{
	BYTE	i;
	BYTE	ancStateBuf[ LED_DISPLAY_MAX_ANC_INDEX ];

	PT_BEGIN( &(pLedM-> anc_pt) );
	// translate baAnc_state[i] to scratch state buffer
	led_display_build_annunciator_on_off( pLedM, ancStateBuf);
	led_display_maps_annunciators( pLedM, ancStateBuf );
	timer_mSec_set( &(pLedM-> ancBlinkTimer ), LED_DISPLAY_BLINK_TIME );
	PT_WAIT_UNTIL( &(pLedM-> anc_pt), timer_mSec_expired( &(pLedM-> ancBlinkTimer) ) );	// wait for blinktime seconds, for On state,

	// ancStateBuf[] is destroyed after PT_WAIT_UNTIL(), thus need to call led_display_build_annunciator_on_off() again.
	// translate baAnc_state[i] to scratch state buffer
	led_display_build_annunciator_on_off( pLedM, ancStateBuf);
	led_display_maps_annunciators( pLedM, ancStateBuf );		// call led_display_maps_annunciators() every wait for blinktime seconds, to speed up updating bargraph.
	timer_mSec_set( &(pLedM-> ancBlinkTimer ), LED_DISPLAY_BLINK_TIME );
	PT_WAIT_UNTIL( &(pLedM-> anc_pt), timer_mSec_expired( &(pLedM-> ancBlinkTimer) ) );	// wait for blinktime seconds, for On state,

	// translate baAnc_state[i] to scratch state buffer and updated baAnc_state[i] to a new state.
	led_display_build_blink_annunciator_on_off( pLedM, ancStateBuf, NO);
	led_display_maps_annunciators( pLedM, ancStateBuf );					// update LED hardware driver chip.
	timer_mSec_set( &(pLedM-> ancBlinkTimer ), LED_DISPLAY_BLINK_TIME );
	PT_WAIT_UNTIL( &(pLedM-> anc_pt), timer_mSec_expired( &(pLedM-> ancBlinkTimer) ) );	// wait for blinktime seconds,

	// ancStateBuf[] is destroyed after PT_WAIT_UNTIL(), thus need to call led_display_build_annunciator_on_off() again.
	led_display_build_blink_annunciator_on_off( pLedM, ancStateBuf, YES );
	led_display_maps_annunciators( pLedM, ancStateBuf );		// call led_display_maps_annunciators() every wait for blinktime seconds, to speed up updating bargraph.
	// 2012-09-28 -WFC- v
	if ( led_display_is_led_dimmed_sleep()) 					// if LED segments were dimmed and in sleep mode, then
		timer_mSec_set( &(pLedM-> ancBlinkTimer ), 40 );
	else
	// 2012-09-28 -WFC- ^
		timer_mSec_set( &(pLedM-> ancBlinkTimer ), LED_DISPLAY_BLINK_TIME );
	PT_WAIT_UNTIL( &(pLedM-> anc_pt), timer_mSec_expired( &(pLedM-> ancBlinkTimer) ) );	// wait for blinktime seconds,

	PT_END( &(pLedM-> anc_pt) );
} // end led_display_manage_annunciator_thread()

/**
 * It translates pLedM->baAnc_state[i] to scratch state buffer.
 *  0 == OFF; none zero == ON.
 *
 * @param  pLedM		-- points to led display manager object.
 * @param  pAncStateBuf	-- points to led annunciators states buffer, it tells which annunciators will on or off.
 *
 * History:  Created on 2010-08-17 by Wai Fai Chin
 */

void	led_display_build_annunciator_on_off( LED_DISPLAY_MANAGER_CLASS *pLedM, BYTE *pAncStateBuf  )
{
	BYTE	i;
	BYTE	ancState;

	// translate baAnc_state[i] to scratch state buffer
	for( i=0; i < LED_DISPLAY_MAX_ANC_INDEX; i++ ) {
		if ( pLedM-> descr.baAnc_state[i] )
			ancState = LED_SEG_STATE_STEADY;
		else
			ancState = LED_SEG_STATE_OFF;

		pAncStateBuf[i] = ancState;
	}
} // end led_display_build_annunciator_on_off()

/**
 * It translates pLedM->baAnc_state[i] to scratch state buffer.
 * annunciators state, state == 255 steady on;
 * 220<state<255 stay on until timeout reach 220 then is off;
 * 219 is always blink.
 * 200<state<219 blink until it reach timeout and stay on when it reaches 200; Thus 203 will blink 3 times then stay steady on.
 * 1<state<200 blink until it reach 0;
 *
 * @param  pLedM		-- points to led display manager object.
 * @param  pAncStateBuf	-- points to led annunciators states buffer, it tells which annunciators will on or off.
 * @param  needUpdateState -- does it need to update descr.baAnc_state[].
 *
 *	#define	LED_SEG_STATE_STEADY						255
 *	#define	LED_SEG_STATE_ON_TIMEOUT_THEN_STAY_OFF		220
 *	#define	LED_SEG_STATE_KEEP_BLINKING					219
 *	#define	LED_SEG_STATE_BLINK_TIMEOUT_THEN_STAY_ON	200
 *	#define	LED_SEG_STATE_OFF				0
 *
 * History:  Created on 2010-08-17 by Wai Fai Chin
 */

void	led_display_build_blink_annunciator_on_off( LED_DISPLAY_MANAGER_CLASS *pLedM, BYTE *pAncStateBuf, BYTE needUpdateState )
{
	BYTE	i;
	BYTE	*pAnc;

	// translate baAnc_state[i] to scratch state buffer and updated baAnc_state[i] to a new state.
	for( i=0; i < LED_DISPLAY_MAX_ANC_INDEX; i++ ) {
		pAnc = &(pLedM-> descr.baAnc_state[i]);

		pAncStateBuf[i] = LED_SEG_STATE_STEADY;								// assume it is on for this annunciator
		if ( *pAnc > LED_SEG_STATE_ON_TIMEOUT_THEN_STAY_OFF ) {
			if ( *pAnc < LED_SEG_STATE_STEADY )
				if ( needUpdateState ) {
						(*pAnc)--;
					if ( LED_SEG_STATE_ON_TIMEOUT_THEN_STAY_OFF == *pAnc ) {
						*pAnc = LED_SEG_STATE_OFF;
						pAncStateBuf[i] = LED_SEG_STATE_OFF;				// turn off this annunciator
					}
				}
		}
		else  if ( *pAnc < LED_SEG_STATE_ON_TIMEOUT_THEN_STAY_OFF ) {
			pAncStateBuf[i] = LED_SEG_STATE_OFF;							// turn off this annunciator
			if ( needUpdateState ) {
				if ( *pAnc < LED_SEG_STATE_KEEP_BLINKING && *pAnc > 0) {
					(*pAnc)--;
					if ( LED_SEG_STATE_BLINK_TIMEOUT_THEN_STAY_ON == *pAnc ) {
						*pAnc = LED_SEG_STATE_STEADY;
						pAncStateBuf[i] = LED_SEG_STATE_STEADY;				// turn on this annunciator
					}
				}
			}
		}
	}
} // end led_display_build_blink_annunciator_on_off()


/**
 * It formats incoming string to fit the LED display board.
 * It may scales down by 1000 if the number is too big to fit in the five digit LEDs.
 * It may displays an outside '-' sign.
 * It will truncated right decimal number after the first 5 digit whole number.
 *
 * @param  fV			-- input floating point value.
 * @param  pCB			-- countby pointer
 * @param  needX1000	-- need to scale X1000, true or false
 * @param  pOutStr		-- points to an output string.
 *
 * @return	Length of the string.
 *
 * @post It formats the input value and store in pOutStr.
 *
 * History:  Created on 2009/06/02 by Wai Fai Chin
 */

BYTE led_display_format_float_string( float fV, MSI_CB *pCB, BYTE needX1000, BYTE *pOutStr )
{
	#ifdef  LED_DISPLAY_ANC_X1000
	led_display_set_annunciator( LED_DISPLAY_ANC_X1000, LED_SEG_STATE_OFF );
	if ( needX1000 ) {
		if ( fV > 99999.4 || fV < -99999.4) {
			fV /= 1000.0;
			led_display_set_annunciator( LED_DISPLAY_ANC_X1000, LED_SEG_STATE_STEADY );
		}
	}
	#endif

	return (float_round_to_string( fV, pCB, LED_MAX_STRING_SIZE, pOutStr));
} // end led_display_format_float_string(,,,) resolution


/**
 * It formats incoming string to fit the LED display board.
 * It may scales down by 1000 if the number is too big to fit in the five digit LEDs.
 * It may displays an outside '-' sign.
 * It tries to fit all integer component and decimal number in 5 digit LEDs.
 *
 * @param  fV			-- input floating point value.
 * @param  pCB			-- countby pointer
 * @param  needX1000	-- need to scale X1000, true or false
 * @param  pOutStr		-- points to an output string.
 *
 * @return	Length of the string.
 *
 * @post It formats the input value and store in pOutStr.
 *
 * History:  Created on 2011-04-29 by Wai Fai Chin
 */


BYTE led_display_format_high_resolution_float_string( float fV, MSI_CB *pCB, BYTE needX1000, BYTE *pOutStr )
{
	MSI_CB	countby;
/*
	after divided by 1000,

	99999   0  12345
	9999    1  1234.5
	999     2   123.45
	99      3    12.345
	9       4     1.2345
*/

	countby = *pCB;
	#ifdef  LED_DISPLAY_ANC_X1000
	led_display_set_annunciator( LED_DISPLAY_ANC_X1000, LED_SEG_STATE_OFF );
	if ( needX1000 ) {

		if ( fV > 99999.4 || fV < -99999.4) {	// show decimal point
			led_display_set_annunciator( LED_DISPLAY_ANC_X1000, LED_SEG_STATE_STEADY );
			fV /= 1000.0;
			if ( fV > 9999.94 || fV < -9999.94 ) {
				countby.decPt  = 0;
				countby.fValue = 1.0f;
			}
			else if ( fV > 999.94 || fV < -999.94 ) {
				countby.decPt  = 1;
				countby.fValue = 0.1f;
			}
			else if ( fV > 99.94 || fV < -99.94 ) {
				countby.decPt  = 2;
				countby.fValue = 0.01f;
			}
			else if ( fV > 9.94 || fV < -9.94 ) {
				countby.decPt  = 3;
				countby.fValue = 0.001f;
			}
			else {
				countby.decPt  = 4;
				countby.fValue = 0.0001f;
			}
		}
	}
	#endif
	return (float_round_to_string( fV, &countby, LED_MAX_STRING_SIZE, pOutStr));
} // end led_display_format_high_resolution_float_string(,,,)



/**
 * It formats incoming string to fit the LED display board.
 * It may scales down by 1000 if the number is too big to fit in the five digit LEDs.
 * It may displays an outside '-' sign.
 *
 * @param	pbSrcStr		-- points to an input source string.
 *
 * @return none
 *
 * @post It formats the input string and put it in gLedDisplayManager.descr.str1[]
 *  to be handle by led_display_manage_string_thread();
 *
 * History:  Created on 2009/06/02 by Wai Fai Chin
 */

void led_display_format_output( BYTE *pbSrcStr )
{
	BYTE formatCtrl;
	BYTE ancState;
	BYTE bStrLen;

	formatCtrl = led_display_get_format_ctrl( pbSrcStr, &bStrLen);
	switch ( formatCtrl ) {
		case LED_FORMAT_CTRL_NOT_NUMBER:
		case LED_FORMAT_CTRL_NORMAL:
		case LED_FORMAT_CTRL_MINUS:
				if ( LED_FORMAT_CTRL_MINUS == formatCtrl ) {		// if it requires out of bound '-' sign, then
					ancState = LED_SEG_STATE_STEADY;
				}
				else
					ancState = LED_SEG_STATE_OFF;
				#ifdef LED_DISPLAY_ANC_MINUS
					led_display_set_annunciator( LED_DISPLAY_ANC_MINUS, ancState );
				#endif
				led_display_copy_string_and_pad( gLedDisplayManager.descr.str1, pbSrcStr, bStrLen);
			break;
		case LED_FORMAT_TOO_MANY_DIGITS:
				led_display_format_dash_line();
			break;
	}
} // end led_display_format_output(,)


/**
 * It formats a line with 5 dashes.
 *
 * @return none
 *
 * @post It formats the input string and put it in gLedDisplayManager.descr.str1[]
 *  to be handle by led_display_manage_string_thread();
 *
 * History:  Created on 2009/06/02 by Wai Fai Chin
 */

void led_display_format_dash_line( void )
{
	memcpy_P ( gLedDisplayManager.descr.str1,  gcStrFmtDash10LfCr_P,  LED_DISPLAY_MAX_DIGIT_LENGTH );
	gLedDisplayManager.descr.str1[ LED_DISPLAY_MAX_DIGIT_LENGTH ] = 0;
}

// TO BE REMOVE
void led_display_format_un_cal( void )								//	PHJ	v
{
	memcpy_P ( gLedDisplayManager.descr.str1,  gcStr_unCal,  LED_DISPLAY_MAX_DIGIT_LENGTH );
	gLedDisplayManager.descr.str1[ LED_DISPLAY_MAX_DIGIT_LENGTH ] = 0;
}																	//	PHJ	^

/**
 * It copies a source string from right to left and pad space characters to the remaining
 * left most digits. It also copied the null char too.
 *
 * @param	pbDestStr	-- points to a destination string.
 * @param	pbSrcStr	-- points to a source string.
 * @param	lastLoc		-- last location of source string which index at the null character.
 *
 * @return none
 *
 * History:  Created on 2009/06/03 by Wai Fai Chin
 */

void led_display_copy_string_and_pad( BYTE *pbDestStr, BYTE *pbSrcStr, BYTE lastLoc )
{
	BYTE i;

	i = LED_MAX_STRING_SIZE;
	for (;;) {
		pbDestStr[ i ] = pbSrcStr[ lastLoc ];
		if ( 0 == i || 0 == lastLoc)
			break;
		i--;
		lastLoc--;
	}

	if ( i ) {		// if there are remaining length, then
		for (;;) {
			i--;						// pre decrement because of 0 based array index.
			pbDestStr[ i ] = ' ';		// pad space to the remaining left most digits.
			if ( 0 == i )
				break;
			// i--;
		}
	}
} // end led_display_copy_string_and_pad(,,,)


/**
 * It copies a source string from right to left and pad space characters to the remaining
 * left most digits. It also copied the null char too.
 *
 * @param	pbDestStr	-- points to a destination string.
 * @param	pbSrcStr	-- points to a source string in program memory space.
 *
 * @return none
 *
 * History:  Created on 2009/07/07 by Wai Fai Chin
 */

void led_display_copy_string_and_pad_P( BYTE *pbDestStr, const BYTE *pbSrcStr )
{
	BYTE i;
	BYTE lastLoc;

	lastLoc = strlen_P(pbSrcStr);
	i = LED_MAX_STRING_SIZE;
	for (;;) {
		pbDestStr[ i ] = pgm_read_byte( &pbSrcStr[ lastLoc ]);
		if ( 0 == i || 0 == lastLoc) {
			// if ( i ) i--;	// decrement i by 1 so it will pad the first char with the space.
			break;
		}
		i--;
		lastLoc--;
	}

	if ( i ) {		// if there are remaining length, then
		for (;;) {
			i--;						// pre decrement because of 0 based array index.
			pbDestStr[ i ] = ' ';		// pad space to the remaining left most digits.
			if ( 0 == i )
				break;
			// i--;
		}
	}
} // end led_display_copy_string_and_pad_P(,,,)

/**
 * It turns on or off of all LEDs digits and annunciators base on input state variable.
 *
 * @param  state	-- on/off state, 1 == on,  0 == off.
 *
 * History:  Created on 2010-08-12 by Wai Fai Chin
 */

void led_display_turn_all_leds( BYTE state )
{

	if ( state ) {
		led_display_turn_all_annunciators( ON );
		led_display_turn_all_led_digits( ON );
	}
	else {
		led_display_turn_all_annunciators( OFF );
		led_display_turn_all_led_digits( OFF );
	}

} // end panel_test_turn_all_annunciators()

/**
 * It turns on or off of all LEDs digits base on input state variable.
 *
 * @param  state	-- on/off state, 1 == on,  0 == off.
 *
 * History:  Created on 2010-08-12 by Wai Fai Chin
 */

void led_display_turn_all_led_digits( BYTE state )
{
	BYTE ledState;
	BYTE i;
	BYTE	str[14];

	if ( state ) {
		for ( i=0; i < 11; i +=2 ) {
			str[i] = '8';				// turn on all segment of a digit.
			// str[i] = '0';			//test only 2010-09-01 turn on all segment of a digit.
			str[i+1] = '.';
		}
		str[12] = 0;
	}
	else {
		for ( i=0; i < LED_DISPLAY_MAX_DIGIT_LENGTH; i++ ) {
			str[i] = ' ';			// turn off led digit.
		}
		str[i] = 0;
	}

	led_display_string( str );
} // end led_display_turn_all_led_digits()


/**
 * It turns on or off of all annunciators base on input state variable.
 *
 * @param  state	-- on/off state, 1 == on,  0 == off.
 *
 * History:  Created on 2010-08-12 by Wai Fai Chin
 */

void led_display_turn_all_annunciators( BYTE state )
{
	BYTE ledState;
	BYTE i;

	if ( state ) {
		ledState = LED_SEG_STATE_STEADY;
	}
	else {
		ledState = LED_SEG_STATE_OFF;
	}

	for( i = LED_DISPLAY_FIRST_ANC_NUM; i <= LED_DISPLAY_LAST_ANC_NUM; i++ ) {
		gLedDisplayManager.descr.baAnc_state[ i ] = ledState;
	}
	led_display_maps_annunciators( &gLedDisplayManager, &gLedDisplayManager.descr.baAnc_state[0]);
} // end led_display_turn_all_annunciators()


/**
 * It set on or off state of all annunciators base on input state variable, except a specified annunciator.
 *
 * @param	state		-- on/off state, 1 == on,  0 == off.
 * @param	exceptAnnc	-- exception annunciator.
 *
 * History:  Created on 2012-10-01 by Wai Fai Chin
 */

void led_display_set_all_annunciators_state( BYTE state )
{
	BYTE ledState;
	BYTE i;

	if ( state ) {
		ledState = LED_SEG_STATE_STEADY;
	}
	else {
		ledState = LED_SEG_STATE_OFF;
	}

	for( i = LED_DISPLAY_FIRST_ANC_NUM; i <= LED_DISPLAY_LAST_ANC_NUM; i++ ) {
			gLedDisplayManager.descr.baAnc_state[ i ] = ledState;
	}
} // end led_display_set_all_annunciators_state()


/**
 * It outputs a string of char point by pbSrcStr to LED Display0 device.
 * It starts from the right most digit position to display the last char of the string first;
 * follow by next left digit position to display the last 2nd char and so on.
 * Decimal point will assigned to next high digit position; this can be accomplished
 * by ORing the next char's memory bit map with 0x80 before call the LED_Display0_CMD.
 * The right most decimal point is use to indicate LB unit. If LB is ON, need to OR the
 * right most digit with 0x80 else AND with 0x7F.
 *
 * @param	pbSrcStr  -- pointer to string of char in Program memory space. It must uses null to mark the end.
 *
 * History:  Created on 2011-03-03 by Wai Fai Chin
 */

void	led_display_string_P( const BYTE *pbSrcStr )
{
	BYTE	bStr[ LED_MAX_STRING_SIZE + 1];

	led_display_copy_string_and_pad_P( bStr, pbSrcStr );
	led_display_string( bStr );
} // end led_display_string_P();


///////////////////////////////////////////////////////////////////////////////
//                      Test functions for this module.                      //
///////////////////////////////////////////////////////////////////////////////

#if  ( CONFIG_TEST_LED_DISPLAY_MODULE == TRUE)

#include  "dataoutputs.h"
#include  "stream_router.h"
#include  "nvmem.h"
#include  <stdio.h>		// for sprintf_P(). without included .<stdio.h>, unpredictable or crash the program.
#include  "panelmain.h"
#include  "scalecore_sys.h"

const char gcLedTestStr1[]	PROGMEM = "start";
const char gcLedTestStr2[]	PROGMEM = " Load";

#define LED_TEST_FLOAT_ARRAY_SIZE	12
const float gcafLedTestNum[LED_TEST_FLOAT_ARRAY_SIZE]	PROGMEM = {
	-0.123,
	-99999.4,
	-1234.53,
	-99999.6,
	-12345.64,
	-99999009,
	-123456.4,
	-12.31,
	-99999999.0,
	-5432.1,
	-100000001.0,
	-7771.66,
};


struct pt gLedTest_pt;

TIMER_T gLedTestTimer;

void test_led_display_stream_result( float fV, BYTE *pRoundBuf, BYTE *pLedBuf, BYTE *pX1000Str );
void test_led_display_builtX1000str( BYTE *pX1000Str );

/**
 * It tests all functionalities of the LED_DISPLAY manager module.
 *
 * @param  pThread	-- points to the test thread structure.
 *
 * @note	This function had tested and passed. It is also demo on how to use
 *     all its related functions. It also tested HW3450_LED.c module too.
 *
 * History:  Created on 2009/06/01 by Wai Fai Chin
 */

void test_led_display_manager_thread( struct pt *pThread )
{
  static	BYTE i;
  static IO_STREAM_CLASS		*pOstream;	// pointer to a dynamic out stream object.
  
  BYTE		bStrBuf[24];
  float 	fV;
  MSI_CB	countby;
  BYTE		x1000Str[8];
  
  PT_BEGIN( pThread );

	gbPanelMainSysRunMode = gbPanelMainRunMode = gbSysRunMode =  PANEL_RUN_MODE_IN_CNFG;		// This prevent it runs in normal weight display mode.
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_NORMAL;
	PT_INIT( &gLedDisplayManager.string_pt );						// reset string display tread to guarantee it executes in the correct order.

	strcpy_P ( gLedDisplayManager.descr.str1, gcLedTestStr1);
	#ifdef  LED_DISPLAY_ANC_TOTAL
		led_display_set_annunciator( LED_DISPLAY_ANC_TOTAL, LED_SEG_STATE_BLINK_THREE);			// total annunciator blink 3 times then turn off.
	#endif

	#ifdef  LED_DISPLAY_ANC_NET
		led_display_set_annunciator( LED_DISPLAY_ANC_NET, LED_SEG_STATE_STEADY);
	#endif

	#ifdef  LED_DISPLAY_ANC_MOTION
		led_display_set_annunciator( LED_DISPLAY_ANC_MOTION, LED_SEG_STATE_KEEP_BLINKING);
	#endif

	#ifdef  LED_DISPLAY_ANC_SP1
		led_display_set_annunciator( LED_DISPLAY_ANC_SP1, LED_SEG_STATE_BLINK_FOUR_ON);
	#endif

	#ifdef  LED_DISPLAY_ANC_SP2
		led_display_set_annunciator( LED_DISPLAY_ANC_SP2, LED_SEG_STATE_BLINK_FOUR);
	#endif

	#ifdef  LED_DISPLAY_ANC_SP3
		led_display_set_annunciator( LED_DISPLAY_ANC_SP3, LED_SEG_STATE_BLINK_FOUR_ON);
	#endif

	timer_mSec_set( &gLedTestTimer, TT_2SEC);
	PT_WAIT_UNTIL( pThread, timer_mSec_expired( &gLedTestTimer)  );							// wait for 2 seconds,

	for ( i=0; i < LED_TEST_FLOAT_ARRAY_SIZE; i++ ) {											// Test all negative floating point numbers with countby 0.1
		memcpy_P ( &fV,  &gcafLedTestNum[i], sizeof(float));
		countby.fValue = 0.1;
		countby.decPt = 1;
		led_display_format_float_string( fV, &countby, TRUE, bStrBuf );
		led_display_format_output( bStrBuf );
		test_led_display_builtX1000str( x1000Str );
		test_led_display_stream_result( fV, bStrBuf, gLedDisplayManager.descr.str1, x1000Str);
		timer_mSec_set( &gLedTestTimer, TT_4SEC);
		PT_WAIT_UNTIL( pThread, timer_mSec_expired( &gLedTestTimer)  );						
	}

	for ( i=0; i < LED_TEST_FLOAT_ARRAY_SIZE; i++ ) {											// Test all positive floating point numbers  with countby 0.1
		memcpy_P ( &fV,  &gcafLedTestNum[i], sizeof(float));
		fV = -fV;
		countby.fValue = 0.1;
		countby.decPt = 1;
		led_display_format_float_string( fV, &countby, TRUE, bStrBuf );
		led_display_format_output( bStrBuf );
		test_led_display_builtX1000str( x1000Str );
		test_led_display_stream_result( fV, bStrBuf, gLedDisplayManager.descr.str1, x1000Str);
		timer_mSec_set( &gLedTestTimer, TT_4SEC);
		PT_WAIT_UNTIL( pThread, timer_mSec_expired( &gLedTestTimer)  );						
	}


	for ( i=0; i < LED_TEST_FLOAT_ARRAY_SIZE; i++ ) {											// Test all negative floating point numbers with countby 1
		memcpy_P ( &fV,  &gcafLedTestNum[i], sizeof(float));
		countby.fValue = 1;
		countby.decPt = 0;
		led_display_format_float_string( fV, &countby, TRUE, bStrBuf );
		led_display_format_output( bStrBuf );
		test_led_display_builtX1000str( x1000Str );
		test_led_display_stream_result( fV, bStrBuf, gLedDisplayManager.descr.str1, x1000Str);
		timer_mSec_set( &gLedTestTimer, TT_4SEC);
		PT_WAIT_UNTIL( pThread, timer_mSec_expired( &gLedTestTimer)  );						
	}

	for ( i=0; i < LED_TEST_FLOAT_ARRAY_SIZE; i++ ) {											// Test all positive floating point numbers with countby 1
		memcpy_P ( &fV,  &gcafLedTestNum[i], sizeof(float));
		fV = -fV;
		countby.fValue = 1;
		countby.decPt = 0;
		led_display_format_float_string( fV, &countby, TRUE, bStrBuf );
		led_display_format_output( bStrBuf );
		test_led_display_builtX1000str( x1000Str );
		test_led_display_stream_result( fV, bStrBuf, gLedDisplayManager.descr.str1, x1000Str);
		timer_mSec_set( &gLedTestTimer, TT_4SEC);
		PT_WAIT_UNTIL( pThread, timer_mSec_expired( &gLedTestTimer)  );						
	}

//	for ( i=1; i < 7; i++ ) {																	// blink one digit at a time from left to right.
//		gLedDisplayManager.descr.digitBlinkPos =i;
//		timer_mSec_set( &gLedTestTimer, TT_4SEC);
//		PT_WAIT_UNTIL( pThread, timer_mSec_expired( &gLedTestTimer)  );						// wait for 4 seconds for each blinking digit,
//	}

	gLedDisplayManager.descr.digitBlinkPos = 0;												// this string will not blink.
	gLedDisplayManager.descr.mode = LED_DISPLAY_MODE_ERROR_MSG;								// set to display error message mode
	PT_INIT( &gLedDisplayManager.string_pt );						// reset string display tread to guarantee it executes in the correct order.
	strcpy_P ( gLedDisplayManager.descr.str1, gcStr_Error); 
	strcpy_P ( gLedDisplayManager.descr.str2, gcLedTestStr2);
	timer_mSec_set( &gLedTestTimer, TT_10SEC);
	PT_WAIT_UNTIL( pThread, timer_mSec_expired( &gLedTestTimer)  );							// wait for 10 seconds,

  PT_END( pThread );
}

void test_led_display_stream_result( float fV, BYTE *pRoundBuf, BYTE *pLedBuf, BYTE *pX1000Str )
{
	BYTE n;
	IO_STREAM_CLASS	*pOstream;	// pointer to a dynamic out stream object.
	BYTE			oStr[120];

	if (stream_router_get_a_new_stream( &pOstream ) ) {		// get an output stream to this listener.
		pOstream-> type		= gabListenerStreamTypeFNV[0];		// stream type for the listener.
		pOstream-> status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
		pOstream-> sourceID	= gtProductInfoFNV.devID;
		pOstream-> destID	= gabListenerDevIdFNV[ 0 ];
		n = (BYTE) sprintf_P( oStr, PSTR("\n\r%.3f=%s=%s%s"), fV, pRoundBuf, pLedBuf, pX1000Str);
		stream_router_routes_a_ostream_now( pOstream, oStr, n );
		stream_router_free_stream( pOstream );	// put resource back to stream pool.
	}
		
}

void test_led_display_builtX1000str( BYTE *pX1000Str )
{
	if ( LED_SEG_STATE_STEADY == gLedDisplayManager.descr.baAnc_state[LED_DISPLAY_ANC_X1000]) {
		pX1000Str[0] = '*';
		pX1000Str[1] = 'X';
		pX1000Str[2] = '1';
		pX1000Str[3] = '0';
		pX1000Str[4] = '0';
		pX1000Str[5] = '0';
		pX1000Str[6] = 0;
	}
	else {
		pX1000Str[0] = ' ';
		pX1000Str[1] = 0;
	}
}



/*
   OUTPUTED BY THE ABOVE TEST BENCH MODULE. PASSED TEST on June 08, 2009. 

***************
Original floating value, formated string based on countby, final LED formated with x1000 annunciator.

    countby = 0.1

-0.123=   -0.1=   -0.1
-99999.398=-99999.4=-----
-1234.530=-1234.5=-1234.5
-99999.602= -100.0= -100.0*X1000
-12345.640=-12345.6=-----
-99999010.000=-99999.0=-----*X1000
-123456.398= -123.5= -123.5*X1000
-12.310=  -12.3=  -12.3
-100000000.000=-100000.0=-----*X1000
-100005430.000=-100005.4=-----*X1000
-7771.660=-7771.7=-7771.7

    countby = 0.1
0.123=    0.1=    0.1
99999.398=99999.4=-----
1234.530= 1234.5= 1234.5
99999.602=  100.0=  100.0*X1000
12345.640=12345.6=-----
99999010.000=99999.0=-----*X1000
123456.398=  123.5=  123.5*X1000
12.310=   12.3=   12.3
100000000.000=100000.0=-----*X1000
100005430.000=100005.4=-----*X1000
7771.660= 7771.7= 7771.7

    countby = 1
-0.123=      0=      0
-99999.398= -99999= -99999
-1234.530=  -1235=  -1235
-99999.602=   -100=   -100*X1000
-12345.640= -12346= -12346
-99999010.000= -99999= -99999*X1000
-123456.398=   -123=   -123*X1000
-12.310=    -12=    -12
-100000000.000=-100000=-----*X1000
-100005430.000=-100005=-----*X1000
-7771.660=  -7772=  -7772

    countby = 1
0.123=      0=      0
99999.398=  99999=  99999
1234.530=   1235=   1235
99999.602=    100=    100*X1000
12345.640=  12346=  12346
99999010.000=  99999=  99999*X1000
123456.398=    123=    123*X1000
12.310=     12=     12
100000000.000= 100000=-----*X1000
100005430.000= 100005=-----*X1000
7771.660=   7772=   7772


*/


#endif // ( CONFIG_TEST_LED_DISPLAY_MODULE == TRUE)
