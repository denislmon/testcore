/*! \file hw4260B_key.c \brief external key driver related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATMEGA192D3
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2009/05/26 by Wai Fai Chin
//  History:  Modified for 4260B on 2011-12-12 by Denis Monteiro
// 
//   This is a key driver on the hardware 4260B PCB.
//   This module will use by Challenger3, MSI3550, etc.. It is not a generic core
//   driver of SCaleCore. It is product specific driver.
//
// ****************************************************************************
// 2013-04-19 -DLM- ported to ATMEGA192D3.

#include  "v_key.h"
#include  "led_display.h"
#include  "timer.h"
#include  "scalecore_sys.h"   // 2015-05-12 -WFC-

// public
BYTE gbIsKeyStuck;

// private module variables, function and marco

BYTE hw4260B_key_scan( BYTE *pbKey );
void hw4260B_RF_remote_DCD_read( void );
BYTE hw4260B_key_map_hardware_to_vkey( BYTE hw_key );		// -WFC- 2011-03-15.

BYTE  gLastValidKey;
BYTE  gabKeyBuf[ KEY_MAX_KEYBUF_SIZE ];   // circular buffer stores high level key codes.

BYTE  gbKeyGetCnt;		// circular gbKeyBuf get index, use by keypad get functions
BYTE  gbKeyPutCnt;		// circular gbKeyBuf put index, use by keypad read function to put a new key in buffer.

BYTE  gbKeyStuckCnt;

// -WFC- 2011-03-11 this is a no no expression in embedded system: static BYTE RFExists = FALSE; because it allocated in both rom and ram.	// PHJ Is receiver for RF Remote installed?
BYTE gbRFkeyDCD;	// Is receiver for RF Remote installed?

/**
 * Scan keypad.
 *
 * @param   pbKey  -- pointer to key variable to store the new valid debounced key
 *
 * @return  return 0 if no new key
 *          1 if got a new valid debounced key(s).
 * @post	updated gbIsKeyStuck.
 * 
 * @note  This function should be call periodically at no less than 50 milliseconds interval
 *       during normal operation. The period which it uses for debounce pressed keys.
 *		 This function is called in the main task loop.
 *  
 * History:  Created on 2009-05-26 by Wai Fai Chin
 * 2012-05-15 -WFC- Created a key scan version for XBEE module. 2012-06-20 -DLM-
 * 2013-04-26 -WFC- called hw3460_key_detect_pressed() instead of direct reading.  //2013-09-03 -DLM
 */
BYTE hw4260B_key_scan( BYTE *pbKey )
{
  static BYTE oldKey;
  BYTE curKey;					// current key

//  curKey  = PINB;				// bit4 == POWER KEY, bit7 == USER KEY
//  curKey  = (~curKey)>>4;		// pressed key is 0. invert it to make it 1.
//  curKey &= 0X0F;				// in case the compiler use rotate shift carry instruction, I clear the upper nibble.
  curKey = hw4260B_key_detect_pressed();				//2013-04-26 -WFC-
  if ( !curKey )  {				// no new key
    gLastValidKey = oldKey = curKey;
    gbIsKeyStuck = NO;
    gbKeyStuckCnt = 0;
    return gbKeyStuckCnt;		// flag no new key
  }

  // here we detected a key has been pressed
  if ( curKey == oldKey ) {		// if current key is the same as the previous key, then we have a valid debounced key.
    if ( curKey != gLastValidKey )  {
      *pbKey = gLastValidKey = curKey;
      gbIsKeyStuck = NO;
      gbKeyStuckCnt = 0;
      return 1;
    }
    else {
      gbKeyStuckCnt++;
      if ( gbKeyStuckCnt > 50 ) {
         gbIsKeyStuck = YES;
         gbKeyStuckCnt = 50;
      }
    }
  }// end if ( curKey == oldKey ) {}

  // This scan got a new key and need to debounce ( scan it one more time) before accept it, so treated it as no new key.
  oldKey = curKey;
  return 0;               // flag no new key
}// end hw4260B_key_scan()

/*
#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-05-15 -WFC-
BYTE hw4260B_key_scan( BYTE *pbKey )
{
  static BYTE oldKey;
  BYTE b, curKey;					// current key

  curKey  = PINB;				// bit4 == POWER KEY, bit7 == USER KEY
  curKey  = (~curKey)>>4;		// pressed key is 0. invert it to make it 1.
  curKey &= 0X0F;				// in case the compiler use rotate shift carry instruction, I clear the upper nibble.

  if ( !curKey )  {				// no new key
    gLastValidKey = oldKey = curKey;
    gbIsKeyStuck = NO;
    gbKeyStuckCnt = 0;
    return gbKeyStuckCnt;		// flag no new key
  }

  // here we detected a key has been pressed
  if ( curKey == oldKey ) {		// if current key is the same as the previous key, then we have a valid debounced key.
    if ( curKey != gLastValidKey )  {
      *pbKey = gLastValidKey = curKey;
      gbIsKeyStuck = NO;
      gbKeyStuckCnt = 0;
      return 1;
    }
    else {
      gbKeyStuckCnt++;
      if ( gbKeyStuckCnt > 50 ) {
         gbIsKeyStuck = YES;
         gbKeyStuckCnt = 50;
      }
    }
  }// end if ( curKey == oldKey ) {}

  // This scan got a new key and need to debounce ( scan it one more time) before accept it, so treated it as no new key.
  oldKey = curKey;
  return 0;               // flag no new key
}// end hw3460_key_scan()

#else
BYTE hw4260B_key_scan( BYTE *pbKey )
{
  static BYTE oldKey;
  BYTE curKey;					// current key
  BYTE rfKey;					// current key from rf

	rfKey = PINB;					// read rf keypad receiver because bit4=Power Key to bit7=USER key are share the same inputs.
	PORTG |= ( 1<<PG0 );			// turn on analog switches chip to scan RF remote keypad receiver.
	rfKey = (~rfKey);				// pressed key is 0. invert it to make it 1.
	rfKey &= 0XF0;					// only keep upper nibbles.
	if ( rfKey ) {  				// if panel or RF remote key pressed
		DDRG &= ~( 1<< DDG0 );		//  pg0 float (scan only first four)
		curKey = PINB;				// bit4 == POWER KEY, bit7 == USER KEY
		curKey = (~curKey)>>4;		// pressed key is 0. invert it to make it 1.
		curKey &= 0X0F;				// in case the compiler use rotate shift carry instruction, I clear the upper nibble.
		if ( !curKey ) 				// was one of upper four RF keys
			curKey |= rfKey;
	}
	else
		curKey = 0;
	PORTG &= ~( 1<<PG0 );			// remove pullup from pin so it can be forced low
	DDRG |= ( 1<<DDG0 );			// force scan pin low

  if ( !curKey )  {			// no new key
    gLastValidKey = oldKey = curKey;
    gbIsKeyStuck = NO;
    gbKeyStuckCnt = 0;
    return gbKeyStuckCnt;              	// flag no new key
  }

  // here we detected a key has been pressed
  if ( curKey == oldKey ) {   // if current key is the same as the previous key, then we have a valid debounced key.
    if ( curKey != gLastValidKey )  {
      *pbKey = gLastValidKey = curKey;
      gbIsKeyStuck = NO;
      gbKeyStuckCnt = 0;
      return 1;
    }
    else {
      gbKeyStuckCnt++;
      if ( gbKeyStuckCnt > 50 ) {
         gbIsKeyStuck = YES;
         gbKeyStuckCnt = 50;
      }
    }
  }// end if ( curKey == oldKey ) {}

  // This scan got a new key and need to debounce ( scan it one more time) before accept it, so treated it as no new key.
  oldKey = curKey;
  return 0;               // flag no new key
}// end hw4260B_key_scan()
#endif
*/
/* -WFC- 2011-03-14 commented out
BYTE hw4260B_key_scan( UINT16 *pbKey )
{
  static UINT16 oldKey;
  UINT16 curKey;				// current key
  UINT16 rfKey;					// current key from rf

	rfKey = PINB;					// bit4 == POWER KEY, bit7 == USER KEY
	PORTG |= ( 1<<PG0 );			// force output high
	rfKey = (~rfKey);				// pressed key is 0. invert it to make it 1.
	rfKey &= 0X00F0;				// in case the compiler use rotate shift carry instruction, I clear the upper nibble.
	if ( rfKey )  					// Anything pressed
		{							// panel or RF remote key pressed
		DDRG &= ~( 1<< DDG0 );		//  pg0 float (scan only first four)
		curKey = PINB;				// bit4 == POWER KEY, bit7 == USER KEY
		curKey = (~curKey)>>4;		// pressed key is 0. invert it to make it 1.
		curKey &= 0X000F;			// in case the compiler use rotate shift carry instruction, I clear the upper nibble.
		if ( !curKey ) 				// was one of upper four RF keys
			curKey = rfKey << 4;
	}
	else
		curKey = 0;
	PORTG &= ~( 1<<PG0 );			// remove pullup from pin so it can be forced low
	DDRG |= ( 1<<DDG0 );			// force scan pin low
  
#ifdef CONFIG_WITHOUT_JTAG 
  curKey |= 0x80 & ~PINF;		// or in calwswitch
#endif
  if ( !curKey )  {			// no new key
    gLastValidKey = oldKey = curKey;
    gbIsKeyStuck = NO;
    gbKeyStuckCnt = 0;
    return gbKeyStuckCnt;              	// flag no new key
  }
   
  // here we detected a key has been pressed
  if ( curKey == oldKey ) {   // if current key is the same as the previous key, then we have a valid debounced key.
    if ( curKey != gLastValidKey )  { 
      *pbKey = gLastValidKey = curKey;
      gbIsKeyStuck = NO;
      gbKeyStuckCnt = 0;
      return 1;
    }
    else {
      gbKeyStuckCnt++;
      if ( gbKeyStuckCnt > 50 ) { 
         gbIsKeyStuck = YES;
         gbKeyStuckCnt = 50;
      }   
    }
  }// end if ( curKey == oldKey ) {}

  // This scan got a new key and need to debounce ( scan it one more time) before accept it, so treated it as no new key.
  oldKey = curKey;
  return 0;               // flag no new key
}// end hw4260B_key_scan()
*/


/**													//	PHJ	v
 *	check to see remote receiving data light is on
 *
 * @return none
 * @note	This function flags that the Acknowledge In LED should be turned on
 *		The flag is cleared by the annunciator control routine after the LED is turned on
 *		This permits the LED to be on at least one annunciator even for a quick pulse
 *
 *	History:  Created on 2010/07/27 by Pete Jensen
 */
/*
void hw4260B_RF_remote_DCD_read(void)
{
#if ( LED_ANC_LOC_ACKIN )
	if ( PINA & RF_IN )	// is RF LED on remote on
	{
		if ( RFExists )
			gLedDisplayManager.descr.baAnc[LED_ANC_LOC_ACKIN - LOCOFF].state[LED_ANC_ACKIN] = LED_SEG_STATE_BLINK_ONCE;
	}
	else
		RFExists = TRUE;
#endif

}													//	PHJ	^
*/

void hw4260B_RF_remote_DCD_read(void)
{
#ifdef  LED_DISPLAY_ANC_ACKIN
#if (( CONFIG_RF_MODULE_AS != CONFIG_RF_MODULE_AS_XBEE ) && ( CONFIG_ETHERNET_MODULE_AS != CONFIG_ETHERNET_MODULE_AS_DIGI )) // 2012-06-27 -DLM-
	if ( IS_RF_LED_ON )	// is RF LED on remote on											// 2013-04-26 -WFC-
		led_display_set_annunciator( LED_DISPLAY_ANC_ACKIN, LED_SEG_STATE_BLINK_TWICE );	// 2011-04-29 -WFC-
#endif
#endif
}													//	PHJ	^

/**
 *	Scan keypad and put new valid key value in a circular buffer.
 *
 * @return none
 * @note   This function should be call periodically at no less than 50 milliseconds interval.
 *  	 The periodically called is also use as debounced pressed keys.
 *		 This function is called in the main task loop.
 *
 * History:  Created on 2009-05-26 by Wai Fai Chin
 *  2011-04-25 -WFC- Also handle cal switch pressed.
 */

void hw4260B_key_read( void )
{
	BYTE	v_key;	// high level virtual key code
	BYTE	hw_key;
  
  // if this scan has a valid new key
  // 2011-04-25 -WFC-	if ( hw3460_key_scan( &hw_key ) )  {
  if ( hw4260B_key_scan( &hw_key ) || bios_is_cal_switch_pressed() )  {		// 2011-04-25 -WFC- if key pad or cal switch pressed, then
	  v_key = hw4260B_key_map_hardware_to_vkey( hw_key );
	  if ( ( gbKeyGetCnt + KEY_MAX_KEYBUF_SIZE ) != gbKeyPutCnt ) {
		gabKeyBuf[ gbKeyPutCnt++ & ( KEY_MAX_KEYBUF_SIZE - 1 ) ] = v_key;
    }
  }

  #if ( CONFIG_RF_MODULE_AS == CONFIG_RF_MODULE_AS_NONE )		// 2012-07-02 -WFC-
  hw4260B_RF_remote_DCD_read();						//	PHJ
  #endif
}// end hw4260B_key_read()

/**
 *	It maps hardware key into high level virtual key code.
 *
 * @param  hw_key	-- hardware key code
 * @return high level virtual key code.
 *
 * History:  Created on 2011-03-15 by Wai Fai Chin
 * 2015-05-08 -WFC- flagged cal switch tripped
 */

BYTE hw4260B_key_map_hardware_to_vkey( BYTE hw_key )
{
	BYTE	v_key;	// high level virtual key code

	v_key = hw_key;				// assumed none combination keys.
	#ifdef CONFIG_WITHOUT_JTAG
		if ( bios_is_cal_switch_pressed()) {
			if ( KEY_POWER_KEY == hw_key ) {
				v_key = V_KEY_MASTER_RESET_KEY;
			}
			else {
				v_key = V_KEY_CAL_KEY;
			}
			gbSysStatus |= SYS_STATUS_CAL_SWITCH_TRIPPED;		// 2015-05-08 -WFC- flagged cal switch tripped
		}
	#else
		if ( ( KEY_ZERO_KEY | KEY_USER_KEY) == hw_key )
			v_key = V_KEY_CAL_KEY;
		//else if ( ( KEY_ZERO_KEY | KEY_POWER_KEY ) == hw_key )
		else if ( ( KEY_ZERO_KEY | KEY_TARE_KEY ) == hw_key )
			v_key = V_KEY_MASTER_RESET_KEY;
	#endif

	return v_key;
}// end hw4260B_key_map_hardware_to_vkey()



/**
 *  Get a valid logical key from a circular key buffer.
 *
 * @param   pbKey  -- pointer to key variable to store the new valid debounced key
 *
 * @return  return 0 if no new key
 *          1 if got a new valid debounced key(s).
 * @post	updated gbIsKeyStuck.
 * 
 * @note  This function should be call periodically at no less than 50 milliseconds interval
 *       during normal operation. The period it use for debounce pressed keys.
 *		 This function is called in the main task loop.
 *  
 * History:  Created on 2009/05/26 by Wai Fai Chin
 */

BYTE hw4260B_key_get( BYTE *pbKey )
{
	UINT8 temptime;
  
	*pbKey = V_KEY_NO_NEW_KEY;				// 2011-04-08 -WFC- assumed no new key;
	if ( gbKeyPutCnt == gbKeyGetCnt )
		return 0;
	// temptime = gTTimer_minute_to_dim_LED;	//  timer with be reset in next statement
	// timer_reset_power_off_timer();		//	PHJ	reset power off timer
	self_test_timer_reset_event_timers();	// reset event timers so it would not trigger an expiration event that would execute LED sleep or power off etc..
	*pbKey = gabKeyBuf[ gbKeyGetCnt++ & ( KEY_MAX_KEYBUF_SIZE - 1 ) ];
//	if ( temptime )					// unless LEDs are dim return a key was found	//	PHJ
//		return 1;
//	else
//		return 0;					// key is removed but not reported found
	return TRUE;
  
}// end hw4260B_key_get()



/**
 *	It initializes the keypad and its related variables and buffer..
 *
 * @return none
 *
 * History:  Created on 2009/05/26 by Wai Fai Chin
 */

void hw4260B_key_init( void )
{
  gbKeyGetCnt = 0;		// circular gbKeyBuf get index, use by keypad get functions
  gbKeyPutCnt = 0;		// circular gbKeyBuf put index, use by keypad read function to put a new key in buffer.
  gLastValidKey = KEY_NO_NEW_KEY;
  gbIsKeyStuck = NO;
  gbKeyStuckCnt = 0;
  //RFExists
}

/**
 * Detect key pressed. It is use to ensure all key had released.
 *
 * @return  non zero == key pressed.
 * 			0 == no key had pressed.
 *
 * History:  Created on 2011-04-28 by Wai Fai Chin
 */

#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281)
BYTE hw4260B_key_detect_pressed( void )
{
  static BYTE oldKey;
  BYTE curKey;					// current key
  BYTE rfKey;					// current key from rf

	rfKey = PINB;					// read rf keypad receiver because bit4=Power Key to bit7=USER key are share the same inputs.
	PORTG |= ( 1<<PG0 );			// turn on analog switches chip to scan RF remote keypad receiver.
	rfKey = (~rfKey);				// pressed key is 0. invert it to make it 1.
	rfKey &= 0XF0;					// only keep upper nibbles.
	if ( rfKey ) {  				// if panel or RF remote key pressed
		DDRG &= ~( 1<< DDG0 );		//  pg0 float (scan only first four)
		curKey = PINB;				// bit4 == POWER KEY, bit7 == USER KEY
		curKey = (~curKey)>>4;		// pressed key is 0. invert it to make it 1.
		curKey &= 0X0F;				// in case the compiler use rotate shift carry instruction, I clear the upper nibble.
		if ( !curKey ) 				// was one of upper four RF keys
			curKey |= rfKey;
	}
	else
		curKey = 0;

	return curKey;
}// end hw4260B_key_detect_pressed(()
#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )
#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2013-04-25 -WFC-
/*
#define SW1_KEY         1
#define SW2_KEY         2
#define SW3_KEY         4
#define SW4_KEY         8

PB0  D4 ==> TARE KEY	1 ==> 8
PB1  D3 ==> USER		2 ==> 4
PB2	 D2 ==> ZERO		4 ==> 2
PB3	 D1 ==> POWER		8 ==> 1

#define	BIOS_ON_bm					0x01
#define	BIOS_VAON_bm				0x02
#define	BIOS_P1ON_bm				0x04
#define	BIOS_EXCON_bm				0x08
#define	BIOS_RF_RESET_bm			0x10
#define	BIOS_RF_CFG_bm				0x20
#define	BIOS_AUX1_bm				0x40
#define	BIOS_AUX2_bm				0x80

*/

#define PIN0_TARE_KEY	1
#define PIN1_USER_KEY	2
#define PIN2_ZERO_KEY	4
#define PIN3_POWER_KEY	8


BYTE hw4260B_key_detect_pressed( void )
{
	BYTE b;
	BYTE curKey;				// current key

	b = ~(PORT_GetPortValue( &PORTB ));		// read pins inputs from port B which read in PB0=D4, PB1=D3, PB2=D2, PB3=D1
	curKey = 0;
	if ( PIN3_POWER_KEY & b)				// If PB3, SW1 = Power key is pressed.
		curKey |= KEY_POWER_KEY;
	if ( PIN2_ZERO_KEY & b)					// If PB2, SW2 = Zero key is pressed.
		curKey |= KEY_ZERO_KEY;
	if ( PIN1_USER_KEY & b)					// If PB1, SW3 = F1 (USER1) key is pressed.
		curKey |= KEY_USER_KEY;
	if ( PIN0_TARE_KEY & b)					// If PB0, SW4 = TARE key is pressed.
		curKey |= KEY_TARE_KEY;

	return curKey;
}// end hw3460_key_detect_pressed(()

#else
/*
#define SW1_KEY         1
#define SW2_KEY         2
#define SW3_KEY         4
#define SW4_KEY         8

PB0  D4 ==> TARE KEY	1 ==> 8   view
PB1  D3 ==> USER		2 ==> 4   user2
PB2	 D2 ==> ZERO		4 ==> 2   net_gross
PB3	 D1 ==> POWER		8 ==> 1   total

#define	BIOS_ON_bm					0x01
#define	BIOS_VAON_bm				0x02
#define	BIOS_P1ON_bm				0x04
#define	BIOS_EXCON_bm				0x08
#define	BIOS_RF_RESET_bm			0x10
#define	BIOS_RF_CFG_bm				0x20
#define	BIOS_AUX1_bm				0x40
#define	BIOS_AUX2_bm				0x80

*/

#define PIN0_TARE_KEY	1
#define PIN1_USER_KEY	2
#define PIN2_ZERO_KEY	4
#define PIN3_POWER_KEY	8


BYTE hw4260B_key_detect_pressed( void )
{
	BYTE b;
	BYTE curKey;				// current key
	BYTE rfKey;					// current key from rf

		b = ~(PORT_GetPortValue( &PORTB ));		// read pins inputs from port B which read in PB0=D4, PB1=D3, PB2=D2, PB3=D1
		(&PORTF)-> OUT |= BIOS_AUX1_bm;			// set AUX1 output HIGH
		// PORTF pins: BIOS_ON_bm | BIOS_VAON_bm | BIOS_P1ON_bm | BIOS_EXCON_bm | BIOS_RF_RESET_bm | BIOS_RF_CFG_bm	as outputs, AUX1, AUX2 as inputs.
		PORT_SetDirection( &PORTF, BIOS_ON_bm | BIOS_VAON_bm | BIOS_P1ON_bm | BIOS_EXCON_bm | BIOS_RF_RESET_bm | BIOS_RF_CFG_bm	 );
		rfKey = 0;
		if ( PIN3_POWER_KEY & b)				// If PB3, SW1 = Power key is pressed.
			rfKey |= KEY_POWER_KEY << 4;
		if ( PIN2_ZERO_KEY & b)					// If PB2, SW2 = Zero key is pressed.
			rfKey |= KEY_ZERO_KEY << 4;
		if ( PIN1_USER_KEY & b)					// If PB1, SW3 = F1 (USER1) key is pressed.
			rfKey |= KEY_USER_KEY << 4;
		if ( PIN0_TARE_KEY & b)					// If PB0, SW4 = TARE key is pressed.
			rfKey |= KEY_TARE_KEY << 4;

		if ( rfKey ) {  				// if panel or RF remote key pressed
			b = ~(PORT_GetPortValue( &PORTB ));		// read pins inputs from port B which read in PB0=D4, PB1=D3, PB2=D2, PB3=D1
			curKey = 0;
			if ( PIN3_POWER_KEY & b)				// If PB3, SW1 = Power key is pressed.
				curKey |= KEY_POWER_KEY;
			if ( PIN2_ZERO_KEY & b)					// If PB2, SW2 = Zero key is pressed.
				curKey |= KEY_ZERO_KEY;
			if ( PIN1_USER_KEY & b)					// If PB1, SW3 = F1 (USER1) key is pressed.
				curKey |= KEY_USER_KEY;
			if ( PIN0_TARE_KEY & b)					// If PB0, SW4 = TARE key is pressed.
				curKey |= KEY_TARE_KEY;

			if ( !curKey ) 				// was one of upper four RF keys
				curKey |= rfKey;
		}
		else
			curKey = 0;

		(&PORTF)-> OUT &= ~BIOS_AUX1_bm;		// set AUX1
		PORT_SetDirection( &PORTF, BIOS_ON_bm | BIOS_VAON_bm | BIOS_P1ON_bm | BIOS_EXCON_bm | BIOS_RF_RESET_bm | BIOS_RF_CFG_bm	| BIOS_AUX1_bm );

		return curKey;
}// end hw4260B_key_detect_pressed(()
#endif
#endif
