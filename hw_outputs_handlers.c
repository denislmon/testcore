/*! \file hw_outputs_handlers.c \brief High Level behavior of output pins related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2010 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATMEGA1281
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Product dependent Middle layer.
//
//  History:  Created on 2010-06-30 by Wai Fai Chin
//			  Modified on 2012-04-12 by Denis Monteiro
//					- Added setpoint outputs for 4260B
// 
//   This is a high level output behavior manager module.
//   It manages the behavior of output pins.
//
// ****************************************************************************
 

#include	"config.h"
#include	"hw_outputs_handlers.h"
#include	"sensor.h"
#include	"scalecore_sys.h"
#include	"nvmem.h"

#if (( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC ) || ( CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ))

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC )

#include	"port_driver.h"

// PC4 is COZ flag output pin
// PH6 is Test flag output pin

#define HW_OUTPUTS_HANDLERS_TEST_LED_PORT				&VPORT0
#define HW_OUTPUTS_HANDLERS_COZ_LED_PORT				&PORTC

#define HW_OUTPUTS_HANDLERS_TEST_LED_OFF	(HW_OUTPUTS_HANDLERS_TEST_LED_PORT)->OUT |=	BIOS_PIN6_bm
#define HW_OUTPUTS_HANDLERS_TEST_LED_ON		(HW_OUTPUTS_HANDLERS_TEST_LED_PORT)->OUT &= ~BIOS_PIN6_bm

#define HW_OUTPUTS_HANDLERS_COZ_LED_OFF		(HW_OUTPUTS_HANDLERS_COZ_LED_PORT)->OUT |=	BIOS_PIN4_bm
#define HW_OUTPUTS_HANDLERS_COZ_LED_ON		(HW_OUTPUTS_HANDLERS_COZ_LED_PORT)->OUT &= ~BIOS_PIN4_bm
// PortH is mapped to VPORT0, set points are assigned to portH.
#define HW_OUTPUTS_HANDLERS_SETPOINT_PORT				&VPORT0

#define HW_OUTPUTS_HANDLERS_SP1_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT_PORT)->OUT &=	~BIOS_PIN4_bm
#define HW_OUTPUTS_HANDLERS_SP1_ON		(HW_OUTPUTS_HANDLERS_SETPOINT_PORT)->OUT |=	BIOS_PIN4_bm

#define HW_OUTPUTS_HANDLERS_SP2_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT_PORT)->OUT &=	~BIOS_PIN5_bm
#define HW_OUTPUTS_HANDLERS_SP2_ON		(HW_OUTPUTS_HANDLERS_SETPOINT_PORT)->OUT |=	BIOS_PIN5_bm


#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )

#if(  CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )					// 2013-09-03 -DLM-

#ifdef CONFIG_WITHOUT_JTAG
#define HW_OUTPUTS_HANDLERS_SP1_OFF		PORTF &= 	~(1 << PF5) //PF5
#define HW_OUTPUTS_HANDLERS_SP1_ON		PORTF |=	(1 << PF5)  //PF5
#else
#define HW_OUTPUTS_HANDLERS_SP1_OFF
#define HW_OUTPUTS_HANDLERS_SP1_ON
#endif // CONFIG_WITHOUT_JTAG

#define HW_OUTPUTS_HANDLERS_SP2_OFF		PORTE &= 	~(1 << PE5)	//PE5
#define HW_OUTPUTS_HANDLERS_SP2_ON		PORTE |=	(1 << PE5)	//PE5

#define HW_OUTPUTS_HANDLERS_SP3_OFF		PORTE &= 	~(1 << PE6)	//PE6
#define HW_OUTPUTS_HANDLERS_SP3_ON		PORTE |=	(1 << PE6)	//PE6

#define HW_OUTPUTS_HANDLERS_BZZ_OFF		PORTB &= 	~(1 << PB0)	//PB0
#define HW_OUTPUTS_HANDLERS_BZZ_ON		PORTB |=	(1 << PB0)	//PB0

#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )

#define HW_OUTPUTS_HANDLERS_SETPOINT_ALARM_PORT			&PORTC
#define HW_OUTPUTS_HANDLERS_SETPOINT1_PORT				&PORTB
#define HW_OUTPUTS_HANDLERS_SETPOINT2_PORT				&PORTA
#define HW_OUTPUTS_HANDLERS_SETPOINT3_PORT				&PORTE

#define HW_OUTPUTS_HANDLERS_BZZ_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT_ALARM_PORT)->OUT &= ~BIOS_PIN4_bm
#define HW_OUTPUTS_HANDLERS_BZZ_ON		(HW_OUTPUTS_HANDLERS_SETPOINT_ALARM_PORT)->OUT |= BIOS_PIN4_bm

#define HW_OUTPUTS_HANDLERS_SP1_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT1_PORT)->OUT &= ~BIOS_PIN4_bm
#define HW_OUTPUTS_HANDLERS_SP1_ON		(HW_OUTPUTS_HANDLERS_SETPOINT1_PORT)->OUT |= BIOS_PIN4_bm

#define HW_OUTPUTS_HANDLERS_SP2_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT2_PORT)->OUT &= ~BIOS_PIN6_bm
#define HW_OUTPUTS_HANDLERS_SP2_ON		(HW_OUTPUTS_HANDLERS_SETPOINT2_PORT)->OUT |= BIOS_PIN6_bm

#define HW_OUTPUTS_HANDLERS_SP3_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT3_PORT)->OUT &= ~BIOS_PIN7_bm
#define HW_OUTPUTS_HANDLERS_SP3_ON		(HW_OUTPUTS_HANDLERS_SETPOINT3_PORT)->OUT |= BIOS_PIN7_bm

#endif // CONFIG_USE_CPU

#endif


TIMER_T	gBlinkTimer;

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC )

BYTE	gbHWoLedState;

void hw_outputs_handlers_normal_mode( void );

/**
 * It manages hardware outputs and based on system flags such as COZ, ERRORs, setpoints and testing etc...
 *
 * History:  Created on 2010/06/30 by Wai Fai Chin
 */

void hw_outputs_handlers_init( void )
{
	HW_OUTPUTS_HANDLERS_SP1_OFF;
	HW_OUTPUTS_HANDLERS_SP2_OFF;
	HW_OUTPUTS_HANDLERS_COZ_LED_OFF;
	HW_OUTPUTS_HANDLERS_TEST_LED_OFF;
	gbHWoLedState = OFF;
	timer_mSec_set( &(gBlinkTimer), TT_500mSEC );
} // end hw_outputs_handlers_init();


/**
 * It manages hardware outputs and based on system flags such as COZ, ERRORs, setpoints and testing etc...
 *
 * History:  Created on 2010/06/30 by Wai Fai Chin
 */

void hw_outputs_handlers( void )
{
	if ( SYS_RUN_MODE_NORMAL == gbSysRunMode ) {
		hw_outputs_handlers_normal_mode();
	}
	else if ( SYS_RUN_MODE_ONE_SHOT_SELF_TEST == gbSysRunMode  ||
		SYS_RUN_MODE_SELF_TEST == gbSysRunMode ) {
		HW_OUTPUTS_HANDLERS_TEST_LED_ON;
		gbHWoLedState = ON;
	}
	else {
		HW_OUTPUTS_HANDLERS_TEST_LED_OFF;
		gbHWoLedState = OFF;
	}
} // end hw_outputs_handlers();


/**
 * It manages hardware outputs and based on system flags such as COZ, ERRORs, setpoints and testing etc...
 *
 * History:  Created on 2010-06-30 by Wai Fai Chin
 * 2010-10-25 -WFC- Instead of use user defined model, we use bargraph source sensor ID dictated focus of sensor for a remote device.
 *
 */

void hw_outputs_handlers_normal_mode( void )
{
	BYTE status;
	BYTE status2;
	BYTE runMode;
	BYTE hasError;

//	if ( USER_DEF_MODEL_CHI107 == gtProductInfoFNV.userDefModelCode ) {			// CHI 107 has 2 loadcells and sum by math type loadcell.
//		status  = gaLoadcell[ SENSOR_NUM_MATH_LOADCELL ].status;
//		status2  = gaLoadcell[ SENSOR_NUM_MATH_LOADCELL ].status2;
//		runMode = gaLoadcell[ SENSOR_NUM_MATH_LOADCELL ].runModes;
//	}
//	else {
//		status  = gaLoadcell[ 0 ].status;
//		status2  = gaLoadcell[ 0 ].status2;
//		runMode = gaLoadcell[ 0 ].runModes;
//	}

	// bargraph source sensor ID dictated focus of sensor for a remote device.
	if (  gtSystemFeatureFNV.bargraphSensorID <= SENSOR_NUM_MATH_LOADCELL ) {
		status  = gaLoadcell[  gtSystemFeatureFNV.bargraphSensorID ].status;
		status2  = gaLoadcell[ gtSystemFeatureFNV.bargraphSensorID ].status2;
		runMode = gaLoadcell[  gtSystemFeatureFNV.bargraphSensorID ].runModes;
	}

	hasError = FALSE;
	if (!( LC_RUN_MODE_ENABLED & runMode ))
		hasError = TRUE;
	else {
		if ( (( LC_STATUS_OVERLOAD | LC_STATUS_UNDERLOAD) & status ) ||
			((LC_STATUS2_UN_CAL | LC_STATUS2_INPUTS_DISABLED | LC_STATUS2_OVER_RANGE | LC_STATUS2_UNER_RANGE) & status2) )
			hasError = TRUE;
	}

	if ( hasError ) {
		HW_OUTPUTS_HANDLERS_COZ_LED_OFF;
		if ( timer_mSec_expired( &gBlinkTimer )) {
			timer_mSec_set( &(gBlinkTimer), TT_500mSEC );
			if ( ON == gbHWoLedState ) {
				HW_OUTPUTS_HANDLERS_TEST_LED_OFF;
				gbHWoLedState = OFF;
			}
			else {
				HW_OUTPUTS_HANDLERS_TEST_LED_ON;
				gbHWoLedState = ON;
			}
		}
	}
	else { // no error
		HW_OUTPUTS_HANDLERS_TEST_LED_OFF;
		gbHWoLedState = OFF;

		if ( LC_STATUS_COZ & status )
			HW_OUTPUTS_HANDLERS_COZ_LED_ON;
		else
			HW_OUTPUTS_HANDLERS_COZ_LED_OFF;
	}

	// handle set points
	if ( (SP_LATCH_STATE_1 & gSP_Registry)  )
		HW_OUTPUTS_HANDLERS_SP1_ON;
	else
		HW_OUTPUTS_HANDLERS_SP1_OFF;

	if ( (SP_LATCH_STATE_2 & gSP_Registry)  )
		HW_OUTPUTS_HANDLERS_SP2_ON;
	else
		HW_OUTPUTS_HANDLERS_SP2_OFF;

} // end hw_outputs_handlers_normal_mode();
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )

BYTE	gbHWoBzzState;

void hw_outputs_handlers_normal_mode( void );

/**
 * It manages hardware outputs and based on system flags such as COZ, ERRORs, setpoints and testing etc...
 *
 * History:  Created on 2010/06/30 by Wai Fai Chin
 */

void hw_outputs_handlers_init( void )
{
	HW_OUTPUTS_HANDLERS_SP1_OFF;
	HW_OUTPUTS_HANDLERS_SP2_OFF;
	HW_OUTPUTS_HANDLERS_SP3_OFF;
	HW_OUTPUTS_HANDLERS_BZZ_OFF;
	gbHWoBzzState = OFF;
	timer_mSec_set( &(gBlinkTimer), TT_500mSEC );
} // end hw_outputs_handlers_init();


/**
 * It manages hardware outputs and based on system flags such as COZ, ERRORs, setpoints and testing etc...
 *
 * History:  Created on 2010/06/30 by Wai Fai Chin
 */

void hw_outputs_handlers( void )
{
	if ( SYS_RUN_MODE_NORMAL == gbSysRunMode ) {
		hw_outputs_handlers_normal_mode();
	}
	else if ( SYS_RUN_MODE_ONE_SHOT_SELF_TEST == gbSysRunMode  ||
		SYS_RUN_MODE_SELF_TEST == gbSysRunMode ) {
		HW_OUTPUTS_HANDLERS_BZZ_ON;
		gbHWoBzzState = ON;
	}
	else {
		HW_OUTPUTS_HANDLERS_BZZ_OFF;
		gbHWoBzzState = OFF;
	}
} // end hw_outputs_handlers();


/**
 * It manages hardware outputs and based on system flags such as COZ, ERRORs, setpoints and testing etc...
 *
 * History:  Created on 2010-06-30 by Wai Fai Chin
 * 2010-10-25 -WFC- Instead of use user defined model, we use bargraph source sensor ID dictated focus of sensor for a remote device.
 *
 */

void hw_outputs_handlers_normal_mode( void )
{
/*  BYTE status;
	BYTE status2;
	BYTE runMode;
	BYTE hasError;

//	if ( USER_DEF_MODEL_CHI107 == gtProductInfoFNV.userDefModelCode ) {			// CHI 107 has 2 loadcells and sum by math type loadcell.
//		status  = gaLoadcell[ SENSOR_NUM_MATH_LOADCELL ].status;
//		status2  = gaLoadcell[ SENSOR_NUM_MATH_LOADCELL ].status2;
//		runMode = gaLoadcell[ SENSOR_NUM_MATH_LOADCELL ].runModes;
//	}
//	else {
//		status  = gaLoadcell[ 0 ].status;
//		status2  = gaLoadcell[ 0 ].status2;
//		runMode = gaLoadcell[ 0 ].runModes;
//	}

	// bargraph source sensor ID dictated focus of sensor for a remote device.
	if (  gtSystemFeatureFNV.bargraphSensorID <= SENSOR_NUM_MATH_LOADCELL ) {
		status  = gaLoadcell[  gtSystemFeatureFNV.bargraphSensorID ].status;
		status2  = gaLoadcell[ gtSystemFeatureFNV.bargraphSensorID ].status2;
		runMode = gaLoadcell[  gtSystemFeatureFNV.bargraphSensorID ].runModes;
	}

	hasError = FALSE;
	if (!( LC_RUN_MODE_ENABLED & runMode ))
		hasError = TRUE;
	else {
		if ( (( LC_STATUS_OVERLOAD | LC_STATUS_UNDERLOAD) & status ) ||
			((LC_STATUS2_UN_CAL | LC_STATUS2_INPUTS_DISABLED | LC_STATUS2_OVER_RANGE | LC_STATUS2_UNER_RANGE) & status2) )
			hasError = TRUE;
	}

	if ( hasError ) {
		HW_OUTPUTS_HANDLERS_COZ_LED_OFF;
		if ( timer_mSec_expired( &gBlinkTimer )) {
			timer_mSec_set( &(gBlinkTimer), TT_500mSEC );
			if ( ON == gbHWoLedState ) {
				HW_OUTPUTS_HANDLERS_TEST_LED_OFF;
				gbHWoLedState = OFF;
			}
			else {
				HW_OUTPUTS_HANDLERS_TEST_LED_ON;
				gbHWoLedState = ON;
			}
		}
	}
	else { // no error
		HW_OUTPUTS_HANDLERS_TEST_LED_OFF;
		gbHWoLedState = OFF;

		if ( LC_STATUS_COZ & status )
			HW_OUTPUTS_HANDLERS_COZ_LED_ON;
		else
			HW_OUTPUTS_HANDLERS_COZ_LED_OFF;
	}
*/
	// handle set points
	if ( (SP_LATCH_STATE_1 & gSP_Registry)  ){
		HW_OUTPUTS_HANDLERS_SP1_ON;
		if ( timer_mSec_expired( &gBlinkTimer )) {
			timer_mSec_set( &(gBlinkTimer), TT_500mSEC );
			if ( ON == gbHWoBzzState ) {
				HW_OUTPUTS_HANDLERS_BZZ_OFF;
				gbHWoBzzState = OFF;
			}
			else {
				HW_OUTPUTS_HANDLERS_BZZ_ON;
				gbHWoBzzState = ON;
			}
		}
	}
	else
	{
		HW_OUTPUTS_HANDLERS_SP1_OFF;
		HW_OUTPUTS_HANDLERS_BZZ_OFF;
		gbHWoBzzState = OFF;
	}

	if ( (SP_LATCH_STATE_2 & gSP_Registry)  )
		HW_OUTPUTS_HANDLERS_SP2_ON;
	else
		HW_OUTPUTS_HANDLERS_SP2_OFF;

	if ( (SP_LATCH_STATE_3 & gSP_Registry)  )
		HW_OUTPUTS_HANDLERS_SP3_ON;
	else
		HW_OUTPUTS_HANDLERS_SP3_OFF;

} // end hw_outputs_handlers_normal_mode();

#endif //( CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )

#else

#include	"vs_sensor.h"
#include	"panelmain.h"

/*
Configure Challenger SP outputs:

SP1 Triggered:
SP1, SC port F5, logic 1
SP1A toggling on and off for driving alarm. SC Port C4.

SP2 Triggered:
SP2, SC port F6, logic 1

SP3 Triggered:
SP3, SC port E6, logic 1

All setpoints when not triggered, should be outputs, logic 0.
Note that SP1 and SP2 cannot be emulated as they are JTAG pins. SP1A and SP3 are available with emulation.
 */

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3  )		// 2012-04-11 -WFC-
#if(  CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )					// 2013-04-19 -WFC-
#define HW_OUTPUTS_HANDLERS_SP1A_OFF	PORTC &=  ~( 1 << PC4 )
#define HW_OUTPUTS_HANDLERS_SP1A_ON		PORTC |=  ( 1 << PC4 )

#define HW_OUTPUTS_HANDLERS_SP1_OFF		PORTF &=  ~( 1 << PF5 )
#define HW_OUTPUTS_HANDLERS_SP1_ON		PORTF |=  ( 1 << PF5 )

#define HW_OUTPUTS_HANDLERS_SP2_OFF		PORTF &=  ~( 1 << PF6 )
#define HW_OUTPUTS_HANDLERS_SP2_ON		PORTF |=  ( 1 << PF6 )

#define HW_OUTPUTS_HANDLERS_SP3_OFF		PORTE &=  ~( 1 << PE6 )
#define HW_OUTPUTS_HANDLERS_SP3_ON		PORTE |=  ( 1 << PE6 )
#define SETPOINT_OUTPUT_STATE_SP1A_ON_bm	0x01

#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )			// 2013-04-19 -WFC-  v
#define HW_OUTPUTS_HANDLERS_SETPOINT_ALARM_PORT			&PORTF
#define HW_OUTPUTS_HANDLERS_SETPOINT1_2_PORT			&PORTB
#define HW_OUTPUTS_HANDLERS_SETPOINT3_PORT				&PORTE

//#define HW_OUTPUTS_HANDLERS_SP1_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT_PORT)->OUT &=	~BIOS_PIN4_bm
//#define HW_OUTPUTS_HANDLERS_SP1_ON		(HW_OUTPUTS_HANDLERS_SETPOINT_PORT)->OUT |=	BIOS_PIN4_bm
//
//#define HW_OUTPUTS_HANDLERS_SP2_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT_PORT)->OUT &=	~BIOS_PIN5_bm
//#define HW_OUTPUTS_HANDLERS_SP2_ON		(HW_OUTPUTS_HANDLERS_SETPOINT_PORT)->OUT |=	BIOS_PIN5_bm

//#define HW_OUTPUTS_HANDLERS_SP1A_OFF	(HW_OUTPUTS_HANDLERS_SETPOINT_ALARM_PORT)->OUT &= ~BIOS_PIN2_bm		// P1 : 14 >> PORTF PF2
//#define HW_OUTPUTS_HANDLERS_SP1A_ON		(HW_OUTPUTS_HANDLERS_SETPOINT_ALARM_PORT)->OUT |= BIOS_PIN2_bm		// P1 : 14 >> PORTF PF2

#define HW_OUTPUTS_HANDLERS_SP1A_OFF
#define HW_OUTPUTS_HANDLERS_SP1A_ON

#define HW_OUTPUTS_HANDLERS_SP1_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT1_2_PORT)->OUT &= ~BIOS_PIN4_bm		// (P5 : 1), ( P3 : 5 ) >> PB4
#define HW_OUTPUTS_HANDLERS_SP1_ON		(HW_OUTPUTS_HANDLERS_SETPOINT1_2_PORT)->OUT |= BIOS_PIN4_bm

#define HW_OUTPUTS_HANDLERS_SP2_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT1_2_PORT)->OUT &= ~BIOS_PIN7_bm		// ( P3 : 3 ) >> PB7
#define HW_OUTPUTS_HANDLERS_SP2_ON		(HW_OUTPUTS_HANDLERS_SETPOINT1_2_PORT)->OUT |= BIOS_PIN7_bm

#define HW_OUTPUTS_HANDLERS_SP3_OFF		(HW_OUTPUTS_HANDLERS_SETPOINT3_PORT)->OUT &= ~BIOS_PIN7_bm		// ( P4 : 13 ) >> PE7
#define HW_OUTPUTS_HANDLERS_SP3_ON		(HW_OUTPUTS_HANDLERS_SETPOINT3_PORT)->OUT |= BIOS_PIN7_bm

#define SETPOINT_OUTPUT_STATE_SP1A_ON_bm	0x01

#endif
// 2013-04-19 -WFC-  ^

TIMER_T	gBlinkTimer;
BYTE	gbHWsetpointOutputState;

void hw_outputs_handlers_normal_mode( void );

/**
 * It initialized all output states of a hardware.
 *
 * History:  Created on 2010-06-30 by Wai Fai Chin
 * 2012-04-11 -WFC- handle Challenger3 setpoints outputs.
 */

void hw_outputs_handlers_init( void )
{
	HW_OUTPUTS_HANDLERS_SP1A_OFF;
	HW_OUTPUTS_HANDLERS_SP1_OFF;
	HW_OUTPUTS_HANDLERS_SP2_OFF;
	HW_OUTPUTS_HANDLERS_SP3_OFF;
	gbHWsetpointOutputState = 0;
	timer_mSec_set( &(gBlinkTimer), TT_500mSEC );
} // end hw_outputs_handlers_init();

/**
 * It manages hardware outputs and based on system run mode.
 *
 * History:  Created on 2010-06-30 by Wai Fai Chin
 * 2012-04-11 -WFC- handle Challenger3 setpoints outputs.
 */

void hw_outputs_handlers( void )
{
	//if ( SYS_RUN_MODE_NORMAL == gbSysRunMode ) {
	if ( PANEL_RUN_MODE_NORMAL == gbPanelMainRunMode ){
		hw_outputs_handlers_normal_mode();
	}
	else {
		hw_outputs_handlers_init();			// turn off all setpoint output. output = low.
	}
} // end hw_outputs_handlers();

/**
 * It manages hardware outputs and based on system flags such as COZ, ERRORs, setpoints and testing etc...
 *
 * SP1 Triggered:
 * SP1, SC port F5, logic 1
 * SP1A toggling on and off for driving alarm. SC Port C4.
 *
 * SP2 Triggered:
 * SP2, SC port F6, logic 1
 *
 * SP3 Triggered:
 * SP3, SC port E6, logic 1
 *
 * All setpoints when not triggered, should be outputs, logic 0.
 *
 * History:  Created on 2010-06-30 by Wai Fai Chin
 * 2012-04-11 -WFC- handle Challenger3 setpoints outputs.
 *
 */

void hw_outputs_handlers_normal_mode( void )
{
	// handle set points
	if ( (SP_LATCH_STATE_1 & gSP_Registry)  ) {
		HW_OUTPUTS_HANDLERS_SP1_ON;
		if ( timer_mSec_expired( &gBlinkTimer )) {
			if (  SETPOINT_OUTPUT_STATE_SP1A_ON_bm	& gbHWsetpointOutputState ) {
				gbHWsetpointOutputState &= ~SETPOINT_OUTPUT_STATE_SP1A_ON_bm;
				HW_OUTPUTS_HANDLERS_SP1A_OFF;
				timer_mSec_set( &(gBlinkTimer), TT_200mSEC );
			}
			else {
				HW_OUTPUTS_HANDLERS_SP1A_ON;
				gbHWsetpointOutputState |= SETPOINT_OUTPUT_STATE_SP1A_ON_bm;
				timer_mSec_set( &(gBlinkTimer), TT_300mSEC );
			}
		}
	}
	else {
		HW_OUTPUTS_HANDLERS_SP1_OFF;
		HW_OUTPUTS_HANDLERS_SP1A_OFF;
		gbHWsetpointOutputState &= ~SETPOINT_OUTPUT_STATE_SP1A_ON_bm;
	}

	if ( (SP_LATCH_STATE_2 & gSP_Registry)  )
		HW_OUTPUTS_HANDLERS_SP2_ON;
	else
		HW_OUTPUTS_HANDLERS_SP2_OFF;

	if ( (SP_LATCH_STATE_3 & gSP_Registry)  )
		HW_OUTPUTS_HANDLERS_SP3_ON;
	else
		HW_OUTPUTS_HANDLERS_SP3_OFF;

} // end hw_outputs_handlers();
#else

void hw_outputs_handlers_init( void )
{

}

void hw_outputs_handlers( void )
{
}
#endif

#endif // ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC )
