/*! \file led_display_hw4260B.h \brief High Level LED display related functions.*/
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
//  History:  Created on 2011-06-08 by Wai Fai Chin
//  History:  Modified for 4260B on 2011-12-12 by Denis Monteiro
// 
/// \ingroup product_module
/// \defgroup led_display manages LED display behavior.(led_display.c)
/// \code #include "led_display_hw4260B.h" \endcode
/// \par Overview
//   This is a middle layer LED display manager module and it is hardware specific.
//   It manages the behavior of LED display.
//
// ****************************************************************************
//@{
 

#ifndef MSI_LED_DISPLAY_HW4260B_H
#define MSI_LED_DISPLAY_HW4260B_H

#include  "hw4260B_led.h"

// intensity value range from 0 to 15
#define LED_HW_INTENSITY_CMD    LED_AS1116_INTENSITY_CMD
#define LED_HW_SHUTDOWN_CMD     LED_AS1116_SHUTDOWN_CMD
#define LED_HW_SHUTDOWN_YES     LED_AS1116_SHUTDOWN_YES
#define LED_HW_SHUTDOWN_NO      LED_AS1116_SHUTDOWN_NO

/** The following defines are for hide hardware specific driver and functions. 
 *  This keep led_display module at higher application hardware independent level.
 */

/** \def led_display_start_hw_display_test
 *  Set display driver chip in test mode.
 *
 * @return none.
 *
 * History:  Created on 2009/08/13 by Wai Fai Chin
 */
#define led_display_start_hw_display_test()		hw4260B_led_send_cmd_toboth( LED_AS1116_TEST_CMD, LED_AS1116_TEST_YES); // set the LED driver chip in self test mode.


/** \def led_display_stop_hw_display_test
 *  End display driver chip in test mode.
 *
 * @return none.
 *
 * History:  Created on 2009/08/13 by Wai Fai Chin
 */
#define led_display_stop_hw_display_test()		hw4260B_led_send_cmd_toboth( LED_AS1116_TEST_CMD, LED_AS1116_TEST_NO);



/** \def led_display_string
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
 * History:  Created on 2009/05/28 by Wai Fai Chin
 */
#define led_display_string( pbStr )		hw4260B_led_display_string( (BYTE *)(pbStr))

/** \def led_display_a_digit
 * 
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
#define led_display_a_digit( loc, ch, isDecPt )		hw4260B_led_display_a_digit( (BYTE) (loc), (const BYTE) (ch), (const BYTE) (isDecPt) )


/** \def led_display_send_hw_cmd
 * Send command to LED driver chip.
 *
 * @param	cmd		-- commnad to control the LED driver chip.
 * @param	value	-- the value associated with this command.
 * 
 * @return none.
 *
 * History:  Created on 2009/06/02 by Wai Fai Chin
 */
#define led_display_send_hw_cmd( cmd, value )		hw4260B_led_send_cmd_toboth( (const BYTE) (cmd), (const BYTE) (value) )

/** \def led_display_power_off
 *
 * @return none.
 *
 * History:  Created on 2009/08/12 by Wai Fai Chin
 */
#define led_display_power_off()		hw4260B_led_send_cmd_toboth( LED_AS1116_SHUTDOWN_CMD, LED_AS1116_SHUTDOWN_YES )

/** \def led_display_power_on
 *
 * @return none.
 *
 * History:  Created on 2009/08/12 by Wai Fai Chin
 */
#define led_display_power_on()		hw4260B_led_send_cmd_toboth( LED_AS1116_SHUTDOWN_CMD, LED_AS1116_SHUTDOWN_NO )


/** \def led_display_hardware_init
 *
 * @return none.
 *
 * History:  Created on 2009/08/12 by Wai Fai Chin
 */
#define led_display_hardware_init()		hw4260B_led_init()

/** \def led_display_get_format_ctrl
 * Due to there are only five LED digits for display, the following rules are apply:
 *
 * if a negative number length == 6 then set Led_Minus to TRUE to be display.
 * if a negative number length > 6 or a positive number length > 5,  then tell
 * caller that it may needs to scale down a number by 1000 if decimal point > 4th place to the right,
 *
 * @param	pbStr  -- an input string to be validate.
 * @param	pbStart	-- output start position of the digit exclude space, include '-'. 
 * @param	pbDecPtPosition	-- output decimal point position of from a digit exclude '-' and spaces.
 * @param	pbDigitLength	-- output digit length of this number exclude leading spaces and decimal point.
 *                             Negative length is for negative number.
 * @param	pbStrLen		-- string length of pbStr.
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
 * History:  2009/06/02  Original coded by Wai Fai Chin
*/
#define  led_display_get_format_ctrl( pbStr, pbStrLen )		hw4260B_led_get_format_ctrl( (BYTE *) (pbStr), (BYTE *)( pbStrLen))


#define	LED_DISPLAY_MAX_DIGIT_LENGTH		HW4260B_LED_MAX_DIGIT_LENGTH

#define	LED_DISPLAY_MODE_NORMAL				0
#define	LED_DISPLAY_MODE_BLINK_STRING		1
#define	LED_DISPLAY_MODE_ERROR_MSG			2
#define	LED_DISPLAY_MODE_ERROR_MSG_1SHOT	3
#define	LED_DISPLAY_MODE_UPDATE_NOW			0X80

// MERGE_TASK: to be remove
#define	LED_DISPLAY_MODE_STATUS_MASK	0x8F		//	PHJ aslo want to reset _UPDATE_NOW

#define	LED_MAX_STRING_SIZE			( HW4260B_LED_MAX_DIGIT_LENGTH + 1 )

// high level definition of annunciator for application level module.
// Note that the name must be the same, but the value can be different depends on hardware display platform.
/// They are use for index of baAnc_state[] in LED_BEHAVIOR_DESCRIPTOR_T.
// Low level hardware module use these information to map the actual LED segment.

#define	LED_DISPLAY_ANC_RF		0
#define	LED_DISPLAY_ANC_ACKOUT	0  // Shared same annunciator with LED_DISPLAY_ANC_RF
#define	LED_DISPLAY_ANC_ACKIN	1
#define	LED_DISPLAY_ANC_TOTAL	2
#define	LED_DISPLAY_ANC_PEAK	3
#define	LED_DISPLAY_ANC_NET		4
#define	LED_DISPLAY_ANC_LB		5
#define	LED_DISPLAY_ANC_KG		6
#define	LED_DISPLAY_ANC_X1000	7
#define	LED_DISPLAY_ANC_SP3		8
#define	LED_DISPLAY_ANC_SP2		9
#define	LED_DISPLAY_ANC_SP1		10
#define	LED_DISPLAY_ANC_BAT		11
#define	LED_DISPLAY_ANC_STABLE	12
#define	LED_DISPLAY_ANC_COZ		13




// #define	LED_DISPLAY_NEEDS_COZ			1
// #define	LED_DISPLAY_NEEDS_SETPOINTS		1

// The above are for helicopter LED display. If you modified the above, you must modify the following.
/// Maximum number of LED annunciator.
#define	LED_DISPLAY_MAX_ANC_INDEX				14
#define	LED_DISPLAY_MAX_DIRECT_UPDATE_ANC_INDEX	14
#define	LED_DISPLAY_FIRST_ANC_NUM				0
#define	LED_DISPLAY_LAST_ANC_NUM				LED_DISPLAY_ANC_COZ

#define	LED_DISPLAY_FORMAT_OK			1
#define	LED_DISPLAY_FORMAT_NEED_X1000	LED_FORMAT_CTRL_X1000	//	 PHJ	doesn't exist on this display


/// LED segment state for LED_BEHAVIOR_DESCRIPTOR_T,
// 255 is always on.
// 220<state<255 stay on until timeout reach 220 then is off;
// 219 is always blink.
// 200<state<219 blink until it reach timeout and stay on when it reaches 200; Thus 203 will blink 3 times then stay steady on.
// 1<state<200 blink until it reach 0;
#define	LED_SEG_STATE_STEADY						255
#define	LED_SEG_STATE_ON_TIMEOUT_THEN_STAY_OFF		220
#define	LED_SEG_STATE_KEEP_BLINKING					219
#define	LED_SEG_STATE_BLINK_ONCE_ON					201
#define	LED_SEG_STATE_BLINK_TWICE_ON				202
#define	LED_SEG_STATE_BLINK_THREE_ON				203
#define	LED_SEG_STATE_BLINK_FOUR_ON					204
#define	LED_SEG_STATE_BLINK_TIMEOUT_THEN_STAY_ON	200
#define	LED_SEG_STATE_BLINK_ONCE					1
#define	LED_SEG_STATE_BLINK_TWICE					2
#define	LED_SEG_STATE_BLINK_THREE					3
#define	LED_SEG_STATE_BLINK_FOUR					4
#define	LED_SEG_STATE_OFF							0


/** \def led_display_Kg_on
 * Turn KG annunciator steady on.
 *
 * @return none.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 */
//	#define led_display_kg_on()		gLedDisplayManager.descr.baAnc5_state[ LED_ANC5_KG ] = LED_SEG_STATE_STEADY
//	PHJ
// -WFC- 2010-08-17 #define led_display_kg_on()		gLedDisplayManager.descr.baAnc[LED_ANC_LOC_KG - LOCOFF].state[ LED_ANC_KG ] = LED_SEG_STATE_STEADY
// -WFC- 2010-08-17
// #define led_display_kg_on()		gLedDisplayManager.descr.baAnc_state[ LED_DISPLAY_ANC_KG ] = LED_SEG_STATE_STEADY
#define led_display_kg_on() 		led_display_set_annunciator( LED_DISPLAY_ANC_KG, LED_SEG_STATE_STEADY) // -DLM- 2012-03-16

/** \def led_display_Kg_off
 * Turn KG annunciator steady on.
 *
 * @return none.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 */
//	#define led_display_kg_off()		gLedDisplayManager.descr.baAnc5_state[ LED_ANC5_KG ] = LED_SEG_STATE_OFF
//	PHJ
// -WFC- 2010-08-17 #define led_display_kg_off()		gLedDisplayManager.descr.baAnc[LED_ANC_LOC_KG - LOCOFF].state[ LED_ANC_KG ] = LED_SEG_STATE_OFF
// -WFC- 2010-08-17
// #define led_display_kg_off()		gLedDisplayManager.descr.baAnc_state[ LED_DISPLAY_ANC_KG ] = LED_SEG_STATE_OFF
#define led_display_kg_off() 		led_display_set_annunciator( LED_DISPLAY_ANC_KG, LED_SEG_STATE_OFF) // -DLM- 2012-03-16


/** \def led_display_kg_keeps_blink
 * Turn KG annunciator steady on.
 *
 * @return none.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 */
//	#define led_display_kg_keeps_blink()		gLedDisplayManager.descr.baAnc5_state[ LED_ANC5_KG ] = LED_SEG_STATE_KEEP_BLINKING
//	PHJ
// -WFC- 2010-08-17 #define led_display_kg_keeps_blink()		gLedDisplayManager.descr.baAnc[LED_ANC_LOC_KG - LOCOFF].state[ LED_ANC_KG ] = LED_SEG_STATE_KEEP_BLINKING
// -WFC- 2010-08-17
// #define led_display_kg_keeps_blink()		gLedDisplayManager.descr.baAnc_state[ LED_DISPLAY_ANC_KG ] = LED_SEG_STATE_KEEP_BLINKING
#define led_display_kg_keeps_blink() 		led_display_set_annunciator( LED_DISPLAY_ANC_KG, LED_SEG_STATE_KEEP_BLINKING) // -DLM- 2012-03-16


/** \def led_display_kg_blink
 * It blinks kg annunciator of a given number of times.
 *
 * @param	numBlink -- number of blinks before it turn itself off.
 * @return none.
 *
 * History:  Created on 2010-08-19 by Wai Fai Chin
 */
// #define led_display_kg_blink( numBlink )		gLedDisplayManager.descr.baAnc_state[ LED_DISPLAY_ANC_KG ] = (numBlink)
#define led_display_kg_blink( numBlink ) 		led_display_set_annunciator( LED_DISPLAY_ANC_KG, numBlink) // -DLM- 2012-03-16



/** \def led_display_lb_on
 * Turn lb annunciator steady on.
 *
 * @return none.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 */
//	#define led_display_lb_on()		gLedDisplayManager.descr.lbAnc_state = LED_SEG_STATE_STEADY
//	PHJ
// -WFC- 2010-08-17 #define led_display_lb_on()		gLedDisplayManager.descr.baAnc[LED_ANC_LOC_LB - LOCOFF].state[ LED_ANC_LB ] = LED_SEG_STATE_STEADY
// -WFC- 2010-08-17
// #define led_display_lb_on()		gLedDisplayManager.descr.baAnc_state[ LED_DISPLAY_ANC_LB ] = LED_SEG_STATE_STEADY
#define led_display_lb_on() 		led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_STEADY) // -DLM- 2012-03-16


/** \def led_display_lb_off
 * It turns lb annunciator off.
 *
 * @return none.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 */
//	#define led_display_lb_off()	gLedDisplayManager.descr.lbAnc_state = LED_SEG_STATE_OFF
//	PHJ
// -WFC- 2010-08-17 #define led_display_lb_off()	gLedDisplayManager.descr.baAnc[LED_ANC_LOC_LB - LOCOFF].state[ LED_ANC_LB ] = LED_SEG_STATE_OFF
// -WFC- 2010-08-17
// #define led_display_lb_off()		gLedDisplayManager.descr.baAnc_state[ LED_DISPLAY_ANC_LB ] = LED_SEG_STATE_OFF
#define led_display_lb_off() 		led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_OFF) // -DLM- 2012-03-16


/** \def led_display_lb_keeps_blink
 * It keeps blink lb annunciator.
 *
 * @return none.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 */
//	#define led_display_lb_keeps_blink()		gLedDisplayManager.descr.lbAnc_state = LED_SEG_STATE_KEEP_BLINKING	//	PHJ
// -WFC- 2010-08-17 #define led_display_lb_keeps_blink()		gLedDisplayManager.descr.baAnc[LED_ANC_LOC_LB - LOCOFF].state[ LED_ANC_LB ] = LED_SEG_STATE_KEEP_BLINKING
// -WFC- 2010-08-17
// #define led_display_lb_keeps_blink()		gLedDisplayManager.descr.baAnc_state[ LED_DISPLAY_ANC_LB ] = LED_SEG_STATE_KEEP_BLINKING
#define led_display_lb_keeps_blink() 		led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_KEEP_BLINKING) // -DLM- 2012-03-16


/** \def led_display_lb_blink
 * It blinks lb annunciator of given number of times.
 *
 * @param	numBlink -- number of blinks before it turn itself off.
 * @return none.
 *
 * History:  Created on 2010-08-19 by Wai Fai Chin
 */
//#define led_display_lb_blink( numBlink )		gLedDisplayManager.descr.baAnc_state[ LED_DISPLAY_ANC_LB ] = (numBlink)
#define led_display_lb_blink( numBlink ) 		led_display_set_annunciator( LED_DISPLAY_ANC_LB, numBlink) // -DLM- 2012-03-16


/** \def led_display_lb_direct_on
 * It directly turns lb annunciator on, bypass gLedDisplayManager.
 *
 * @return none.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 */
// #define led_display_lb_direct_on()		gbBiosRunStatus |= BIOS_RUN_STATUS_LB_LED_ON	// PHJ	NOT 7300
#define led_display_lb_direct_on() 		led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_STEADY) // -DLM- 2012-03-16


/** \def led_display_lb direct_off
 * It directly turns lb annunciator off, bypass gLedDisplayManager.
 *
 * @return none.
 *
 * History:  Created on 2009/07/20 by Wai Fai Chin
 */
//	PHJ	NOT 7300
// #define led_display_lb_direct_off()		gbBiosRunStatus &= ~BIOS_RUN_STATUS_LB_LED_ON
#define led_display_lb_direct_off() 	led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_OFF) // -DLM- 2012-03-16



/** \def led_display_set_annunciator
 * It sets a given annunciator and its state.
 *
 * @param	annunc -- annunciator, see LED_DISPLAY_ANC_nnn definition for more detail.
 * @param	state -- annunciator state. See LED_SEG_STATE_nnn definition for more detail.
 * @return none.
 *
 * usage:
 * led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_KEEP_BLINKING );
 * History:  Created on 2011-03-10 by Wai Fai Chin
 */
#define led_display_set_annunciator( annunc, state )		gLedDisplayManager.descr.baAnc_state[ (BYTE) (annunc) ] = (BYTE)(state)


/** \def led_display_set_annunciator
 * Is annunciator state equals the given state?
 *
 * @param	annunc -- annunciator, see LED_DISPLAY_ANC_nnn definition for more detail.
 * @param	state -- annunciator state. See LED_SEG_STATE_nnn definition for more detail.
 * @return none.
 *
 * usage:
 * led_display_set_annunciator( LED_DISPLAY_ANC_LB, LED_SEG_STATE_KEEP_BLINKING );
 * History:  Created on 2011-03-16 by Wai Fai Chin
 */
#define led_display_is_annunciator_state_equal( annunc, state )		(BYTE)(state) == gLedDisplayManager.descr.baAnc_state[ (BYTE) (annunc) ]


/** \def led_display_maps_annunciators
 *  It maps annuncioators and bar graph into LED segments.
 * It maps display of annunciator behavior based on the pAncStates.
 *  0 == off; other number is on.
 *
 * @param  pLedM		-- points to led display manager object.
 * @param  pAncStateBuf	-- points to led annunciators states buffer, it tells which annunciators will be on or off.
 * @return none.
 *
 * History:  Created on 2011-03-08 by Wai Fai Chin
 */
#define led_display_maps_annunciators( pLedM, pAncStateBuf )		hw4260B_led_maps_annunciators((pLedM), (pAncStateBuf))


/** \def led_display_send_hw_cmd
 * Send command to LED driver chip.
 *
 * @param	level	-- intensity level.
 *
 * @return none.
 *
 * History:  Created on 2011-03-10 by Wai Fai Chin
 */
#define led_display_set_intensity( level )		hw4260B_led_send_cmd_toboth( LED_HW_INTENSITY_CMD, (const BYTE) (level) )


/** \def led_display_default_intensity
 *  It sets default intensity of individual LED segment.
 *
 * @return none.
 *
 * History:  Created on 2010-08-19 by Wai Fai Chin
 */
#define led_display_default_intensity()		led_display_set_intensity( 1 )

/* * \def led_display_auto_intensity
 *  It automatically set intensity of LEDs based on reading of dimmer ADC counts.
 *
 * @return none.
 *
 * History:  Created on 2010-08-23 by Wai Fai Chin
 * /
#define led_display_auto_intensity()		hw6954_led_auto_intensity()
*/

// a pause between commands of LED driver chip.
#define	LED_DISPLAY_INTER_CMD_DELAY_TIME	1


#endif  // MSI_LED_DISPLAY_HW4260B_H

//@}
