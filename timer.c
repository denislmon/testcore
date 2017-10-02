/*! \file timer.c \brief hardware timer functions. */
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: serial.h
// Hardware: ATMEL ATXMEGA128A1
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2009/11/10 by Wai Fai Chin
//
//  Hardware cpu timer functions.
// 
// ****************************************************************************
// 2011-06-14 -WFC- created new functions:	timer_minute_set(); timer_minute_expired();
//			timer_secon_set(); timer_second_expired();


#include "timer.h"
#include "scalecore_sys.h"
#include "nv_cnfg_mem.h"

UINT8  gTTimer_mSec;     // task timer update value in 50 millisecond per count. It uses by Timer1
UINT8	gTTimer_mSecGenSec;		// task timer update value in 50 millisecond per count. use for generate second value. 2011-06-14 -WFC-
UINT8	gTTimer_SecGenMinute;	// task timer update value in second per count. use for generate minute value. 2011-06-14 -WFC-

UINT8	gTTimer_minute;			// task timer update value in minute.  2011-06-14 -WFC-
UINT16	gTTimer_Sec_16;			// task timer update 16bit value in second.  2011-06-14 -WFC-

/**************************************************************************
					 Timer Functions and ISR
**************************************************************************/

/**
 * Initialized Task timer.
 *   Use Timer1 as a system task tick to run other schedule tasks.
 *
 *  It will run the CTC mode interrupt every 50mSec.
 *
 * @note
 *
 * History:  Created on 2009/11/10 by Wai Fai Chin
 */

void timer_task_init( void )
{
	// Set period/TOP value.
	// system clock is 14746000 Hz,  divided by 16 is 921625. 1 mSec = 921.625;  50 mSec = 921.625 * 50 = 46081;
	// system clock is 14746000 Hz,  divided by 64 is 230406. 1 mSec = 230.406;  50 mSec = 230.406 * 50 = 11520;
	// system clock is 32000000 Hz,  divided by 64 is 500000. 1 mSec = 500.000;  50 mSec = 500 * 50 = 25000;
	// TC_SetPeriod( &TCC0, 46081 ); 	// for counting down

	//	TC_SetPeriod( &TCC0, 25000 );		// for counting up
	//	//TC_SetPeriod( &TCC0, 11520 );		// for counting up
	//	//TC_SetPeriod( &TCC0, 0xFFFE );	// for counting up
	//
	//	//  Select clock source.
	//	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV64_gc;

	//TCC0.CTRLB = TC0_CCAEN_bm | TC_WGMODE_NORMAL_gc;
	// TCC0.CTRLB = TC0_CCAEN_bm | TC_WGMODE_DS_T_gc;
	TCC0.CTRLB = TC0_CCAEN_bm | TC_WGMODE_SS_gc;
	TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;

}/* end timer_task_init() */


/**
 * It initializes a millisecond timer. It works in conjunction with Timer_mSec_Expired().
 *
 * @param  pT       -- pointer to a timer. The timer is in milliseconds ticks in this context.
 * @param  interval -- the time value in milliSec ticks for the timer to expire.
 * 
 * @return none
 *
 * History:  Created on 2006/10/23 by Wai Fai Chin
 */

void  timer_mSec_set( TIMER_T *pT, BYTE interval)
{
  pT-> interval = interval;
  pT-> start  = gTTimer_mSec;    // gTTimer_mSec is generated from the tasktimer interrupt function.
} // end timer_mSec_set(,)

/**
 * Millisecond timer checker to see if the input timer has expired.
 *
 * @param   pT  -- pointer to a timer. The timer is in milliseconds ticks in this context.
 *
 * @return true if timer t is expired otherwise false.
 *
 * History:  Created on 2006/10/23 by Wai Fai Chin
 */

BYTE  timer_mSec_expired( TIMER_T *pT )
{
  BYTE curTime;
  BYTE elaps;
    curTime = gTTimer_mSec;     // It is important to copy the interrupt generated value mSec.
    if ( curTime > pT-> start )
        elaps = curTime - pT-> start;
    else { 
        elaps = curTime + ~( pT-> start);
        elaps++;
    }    
      
    return ( elaps >= pT-> interval);
} // end timer_mSec_expired( )

/**
 * It initializes a minute timer. It works in conjunction with timer_minute_expired().
 *
 * @param  pT       -- pointer to a timer. The timer is in minute ticks in this context.
 * @param  interval -- the time value in minute ticks for the timer to expire.
 *
 * @return none
 *
 * History:  Created on 2011/06/14 by Wai Fai Chin
 */

void  timer_minute_set( TIMER_T *pT, BYTE interval)
{
  pT-> interval = interval;
  pT-> start  = gTTimer_minute;    // gTTimer_minute is generated from the TIMER1_COMPA_vect interrupt function.
} // end timer_minute_set(,)

/**
 * Minute timer checker to see if the input timer has expired.
 *
 * @param   pT  -- pointer to a timer. The timer is in minute ticks in this context.
 *
 * @return true if timer t is expired otherwise false.
 *
 * History:  Created on 2011/06/14 by Wai Fai Chin
 */

BYTE  timer_minute_expired( TIMER_T *pT )
{
  BYTE curTime;
  BYTE elaps;
    curTime = gTTimer_minute;     // It is important to copy the interrupt generated minute value to prevent interrupt changed its value during the following computation.
    if ( curTime > pT-> start )
        elaps = curTime - pT-> start;
    else {
        elaps = curTime + ~( pT-> start);
        elaps++;
    }

    return ( elaps >= pT-> interval);
} // end timer_minute_expired( )

/**
 * It initializes a second timer. It works in conjunction with timer_second_expired().
 *
 * @param  pT       -- pointer to a timer. The timer value is in second.
 * @param  interval -- the time value in second ticks for the timer to expire.
 *
 * @return none
 *
 * History:  Created on 2011/06/14 by Wai Fai Chin
 */

void  timer_second_set( TIMER_Sec_16_T *pT, UINT16 interval )
{
  pT-> interval = interval;
  pT-> start  = gTTimer_Sec_16;    // gTTimer_Sec_16 is generated from the TIMER1_COMPA_vect interrupt function.
} // end timer_second_set(,)


/**
 * Second timer checker to see if the input timer has expired.
 *
 * @param   pT  -- pointer to a timer with 16 bits value. The timer is in second ticks in this context.
 *
 * @return true if timer t is expired otherwise false.
 *
 * History:  Created on 2011/06/14 by Wai Fai Chin
 */

BYTE  timer_second_expired( TIMER_Sec_16_T *pT )
{
  UINT16 curTime;
  UINT16 elaps;
    curTime = gTTimer_Sec_16;     // It is important to copy the interrupt generated value of seconds to prevent interrupt changed its value during the following computation.
    if ( curTime > pT-> start )
        elaps = curTime - pT-> start;
    else {
        elaps = curTime + ~( pT-> start);
        elaps++;
    }

    return ( elaps >= pT-> interval);
} // end timer_second_expired( )


/**
 * Interrupt routine handle TIMER compare A match interrupt.
 * This interrupt happened every 50 miliseconds.
 *
 * @return none
 * @post
 *   It increament gTTimer_mSec by 1. 1 count = 50 miliseconds.
 *   timer_task_init();
 *
 * Created on 2009/11/10 by Wai Fai Chin
 * 2011-09-13 -WFC- Added codes to generate seconds and minutes. This is merged from ScaleCore1 codes.
 */

// ISR(TCC0_OVF_vect)		// Doxygen cannot handle ISR() macro.
void TCC0_OVF_vect (void) __attribute__ ((signal,__INTR_ATTRS));
void TCC0_OVF_vect (void)
{
	gTTimer_mSec++;
	// 2011-09-13 -WFC- v
	gTTimer_mSecGenSec++;
	if ( gTTimer_mSecGenSec >= 20 ) {
		gTTimer_mSecGenSec = 0;
		gTTimer_Sec_16++;
		gTTimer_SecGenMinute++;
		if ( gTTimer_SecGenMinute > 59 ) {
			gTTimer_SecGenMinute = 0;
			gTTimer_minute++;
		}
	}
	// 2011-09-13 -WFC- ^
}



///////////////////////////////////////////////////////////////////////////////
//                      Test functions for this module.                      //
///////////////////////////////////////////////////////////////////////////////

#if  ( CONFIG_TEST_TIMER_MODULE == TRUE)
#include <stdio.h>                /* prototype declarations for I/O functions */
#include  "serial.h"

struct pt gFastTimeOutThread_pt;
static TIMER_T gFastThreadTimer;

struct pt gSlowTimeOutThread_pt;
static TIMER_T gSlowThreadTimer;

PT_THREAD( test_fast_timeout_thread(struct pt *pt) )
{
  BYTE n;
  BYTE str[80];
  
  PT_BEGIN( pt );
  
  while(1) {
    timer_mSec_set( &gFastThreadTimer, TT_1SEC);    // Note this statment belongs to PT_BEGIN() case:0, and while(1) loop.
    // It was called by the very first time of excution of this Thread and untill every timer expired event.
    PT_WAIT_UNTIL( pt, timer_mSec_expired( &gFastThreadTimer ) );
    n = (BYTE) sprintf_P( str,PSTR("\n\r Fast Thread ticks: %d"), (int)gTTimer_mSec);
	serial0_send_bytes( str, n);
  }
  
  PT_END( pt );
} // end test_fast_timeout_thread()


PT_THREAD( test_slow_timeout_thread(struct pt *pt) )
{
  BYTE n;
  BYTE str[80];

  PT_BEGIN( pt );
  
  while(1) {
    timer_mSec_set( &gSlowThreadTimer, TT_4SEC);
    PT_WAIT_UNTIL( pt, timer_mSec_expired( &gSlowThreadTimer ) );
    n = (BYTE) sprintf_P(str, PSTR("\n\r Slow Thread ticks: %d"), (int)gTTimer_mSec);
	serial0_send_bytes( str, n);
  }
  
  PT_END( pt );
} // end Test_slow_timeout_thread()



/*  none Thread test modules.
TIMER_T gTestTimer;
BYTE code testStrTime[] = "Task Tick";

void test_timer_module( void )
{
  gTTimer_mSec = 1;
  timer_mSec_set( &gTestTimer, 10);
  gTTimer_mSec = 254;
  timer_mSec_expired( &gTestTimer );

  gTTimer_mSec = 254;
  timer_mSec_set( &gTestTimer, 10);
  gTTimer_mSec = 1;
  timer_mSec_expired( &gTestTimer );
}

void test_timer_module( void )
{
  timer_mSec_set( &gTestTimer, TT_1SEC);
  while ( !timer_mSec_expired( &gTestTimer ) ) {};
  printf("\n%s A: %d", testStrTime, (int)gTTimer_mSec);

  timer_mSec_set( &gTestTimer, TT_4SEC);
  while ( !timer_mSec_expired( &gTestTimer ) ) {};
  printf("\n%s B: %d", testStrTime, (int)gTTimer_mSec);
}
*/

#endif // ( CONFIG_TEST_TIMER_MODULE == TRUE)

