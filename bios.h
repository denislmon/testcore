/*! \file bios.h \brief bios routine and associated routines. */
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: bios.h
// Hardware: ATMEL AVR series
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2007/06/27 by Wai Fai Chin
//
/// \ingroup driver_system 
/// \defgroup bios Bios Function module (bios.c)
/// \code #include "bios.h" \endcode
/// \par Overview
/// This provides all the basic input, output functions of a hardware system
/// which included cpu peripherals and cpu external peripheral.
/// It sets up all required control registers to run the cpu.
/// This file will manage different hardware modules from other module files like:
/// Serial I/O, SPI, Timer tick count, and other ISRs.
/// Hardware independent application codes should always use bios.h file access the
/// hardware dependent functions. This will help isolate application codes from the
/// hardware dependent codes.
/// \ref driver_avr_cpu
// \ref driver_cpu_extern
//	
// ****************************************************************************
//@{


#ifndef MSI_BIOS_H
#define MSI_BIOS_H

#include	"config.h"
#include	"port_driver.h"
#include	"adc_lt.h"
#include	"timer.h"
#include	"serial.h"
#include	"spi.h"

//! Initializes bios system
/// You must call this function the first thing in the main() before you do any other stuff.
void    bios_system_init( void );
void	bios_on_board_led( BYTE state, BYTE LEDnumber );
void	bios_system_reset_sys_regs( void );

// IO pin number of a port, generic port
#define	BIOS_PIN0		0
#define	BIOS_PIN1		1
#define	BIOS_PIN2		2
#define	BIOS_PIN3		3
#define	BIOS_PIN4		4
#define	BIOS_PIN5		5
#define	BIOS_PIN6		6
#define	BIOS_PIN7		7

#define	BIOS_PIN0_bm		0x01
#define	BIOS_PIN1_bm		0x02
#define	BIOS_PIN2_bm		0x04
#define	BIOS_PIN3_bm		0x08
#define	BIOS_PIN4_bm		0x10
#define	BIOS_PIN5_bm		0x20
#define	BIOS_PIN6_bm		0x40
#define	BIOS_PIN7_bm		0x80


#define ENABLE_GLOBAL_INTERRUPT		sei();
#define DISABLE_GLOBAL_INTERRUPT	cli();

#define	BIOS_ON_BOARD_LED_RED		0
#define	BIOS_ON_BOARD_LED_BLUE		1
#define	BIOS_ON_BOARD_LED_GREEN		2

// DEFINED CPU IO connections of the hardware.

#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )
#define BIOS_ON_BOARD_LED_PORT		PORTC
#define BIOS_TURN_ON_ANALOG_POWER	PORTG |=  (1<<PG2)

#define BIOS_SET_EXTIO_HIGH			PORTG |=  (1<<PG0)
#define BIOS_SET_EXTIO_LOW			PORTG &= ~(1<<PG0)


// #define BIOS_SET_RCAL_HIGH			PORTD |=  ( 1 << PD7 )
// #define BIOS_SET_RCAL_LOW			PORTD &= ~( 1 << PD7 )
#define BIOS_SET_RCAL_OFF			PORTD |=  ( 1 << PD7 )
#define BIOS_SET_RCAL_ON			PORTD &= ~( 1 << PD7 )


#define BIOS_TURN_OFF_AC_EXCITATION	ADC_LT_TURN_OFF_AC_EXCITATION
#define BIOS_ENABLE_AC_EXCITATION	ADC_LT_ENABLE_AC_EXCITATION
#define BIOS_AC_EXCITATION_POS		ADC_LT_AC_EXCITATION_POS
#define BIOS_AC_EXCITATION_NEG		ADC_LT_AC_EXCITATION_NEG

#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 )

#define	BIOS_SPI_SS_bm		0x10

/// PORTE is for RS232 serial1, 2 ports, pins: CFG o, DTR o, DCD0 i, RFreset o
#define	BIOS_RF_CFG_bm		0x01
#define	BIOS_DTR_bm			0x02
#define	BIOS_DCD0_bm		0x10
#define	BIOS_RF_RESET_bm	0x20

/// XBEE RF module of PORTE
#define	BIOS_RF_SLEEP_bm	0x02
#define	BIOS_RF_ASSC_bm		0x10


/// PORTF is for SPI3 port. pins: excitation XCLK o, XCLKEN o, AUX1,2,3 inputs.
#define	BIOS_EXCITE_XCLK_bm			0x01
#define	BIOS_EXCITE_XCLKEN_bm		0x02
#define	BIOS_AUX1_bm				0x04
#define	BIOS_AUX2_bm				0x08
#define	BIOS_AUX3_bm				0x10

/// PORTH is for external peripheral chip select pins. PORTH is mapped to virtual port0.
#define	BIOS_A2DCS_bm				0x01
#define	BIOS_SPICS1_bm				0x02
// #define	BIOS_SPICS2_bm				0x04
// #define	BIOS_SPICS3_bm				0x08
#define	BIOS_CTS0_bm				0x04
#define	BIOS_RTS0_bm				0x08
#define	BIOS_SPICS4_bm				0x10
#define	BIOS_SPICS5_bm				0x20
#define	BIOS_SPICS6_bm				0x40
#define	BIOS_FRAMCS_bm				0x80

/// PORTJ is for pins: RCAL o, D4F i, ADBUSY i, SPIINT2 i, SW3R, SW2R, SW1R CAL inputs. PORTJ is mapped to virtual port1.
#ifdef  SCALE_CORE2_REV_E
#define	BIOS_RCAL_bm				0x01
#endif

#define	BIOS_D4F_bm					0x02
#define	BIOS_ADBUSY_bm				0x04
#define	BIOS_SPIINT2_bm				0x08
#define	BIOS_SW3R_bm				0x10
#define	BIOS_SW2R_bm				0x20
#define	BIOS_SW1_CAL_bm				0x40

/// PORTK is for pins: ON o, P1ON o, VAON o, 232ON o, BLULED1 o, REDLED1 o, GRNLED1 o, GRNLED2 o. PORTK is mapped to virtual port2.
#define	BIOS_ON_bm					0x01
#define	BIOS_P1ON_bm				0x02
#define	BIOS_VAON_bm				0x04
#define	BIOS_232ON_bm				0x08
#define	BIOS_BLULED1_bm				0x10
#define	BIOS_REDLED1_bm				0x20
#define	BIOS_GRNLED1_bm				0x40
#define	BIOS_GRNLED2_bm				0x80

/// PORTQ is for pins: PQ2 o, PQ3 o. Both of this output pins set low to drive a LED for GRNLED2.
#define	BIOS_LED2_SRC1_bm		0x04
#define	BIOS_LED2_SRC2_bm		0x08

#define BIOS_ON_BOARD_LED_PORT				&VPORT2
#define BIOS_ON_BOARD_LED_PORT_OUT_REG		(&VPORT2)->OUT
#define BIOS_POWER_CTRL_PORT				&VPORT2

// -WFC- 2010-08-23 v
#define BIOS_ON_BOARD_LED_PORT_Q				&PORTQ
#define BIOS_ON_BOARD_LED_PORT_Q_OUT_REG		(&PORTQ)->OUT
/// Turn off two exc LEDs and GREEN LED#2.
#define BIOS_TURN_ON_EXC_GRNLED2		BIOS_ON_BOARD_LED_PORT_Q_OUT_REG &= ( BIOS_PIN2_bm | BIOS_PIN3_bm )
#define BIOS_TURN_OFF_EXC_GRNLED2		BIOS_ON_BOARD_LED_PORT_Q_OUT_REG &= ~( BIOS_PIN2_bm | BIOS_PIN3_bm )
#define BIOS_TURN_OFF_ALL_ON_BOARD_LEDS	BIOS_ON_BOARD_LED_PORT_OUT_REG |= 0XF0; BIOS_TURN_OFF_EXC_GRNLED2
// -WFC- 2010-08-23 ^


#define BIOS_TURN_ON_P4_POWER	(BIOS_POWER_CTRL_PORT)->OUT |=	BIOS_ON_bm
#define BIOS_TURN_OFF_P4_POWER	(BIOS_POWER_CTRL_PORT)->OUT &= ~BIOS_ON_bm

#define BIOS_TURN_ON_P1_POWER	(BIOS_POWER_CTRL_PORT)->OUT |=	BIOS_P1ON_bm
#define BIOS_TURN_OFF_P1_POWER	(BIOS_POWER_CTRL_PORT)->OUT &= ~BIOS_P1ON_bm

#define BIOS_TURN_ON_ANALOG_POWER	(BIOS_POWER_CTRL_PORT)->OUT |=	BIOS_VAON_bm
#define BIOS_TURN_OFF_ANALOG_POWER	(BIOS_POWER_CTRL_PORT)->OUT &= ~BIOS_VAON_bm

#define BIOS_TURN_ON_RS232_POWER	(BIOS_POWER_CTRL_PORT)->OUT |=	BIOS_232ON_bm
#define BIOS_TURN_OFF_RS232_POWER	(BIOS_POWER_CTRL_PORT)->OUT &= ~BIOS_232ON_bm

// Not applicable for ScaleCore2
// #define BIOS_SET_EXTIO_HIGH			PORTG |=  (1<<PG0)
// #define BIOS_SET_EXTIO_LOW			PORTG &= ~(1<<PG0)

#ifdef  SCALE_CORE2_REV_E
// NO MORE RCAL control pin for Rev. J.
#define BIOS_SET_RCAL_OFF			(&VPORT1)->OUT |=  BIOS_RCAL_bm
#define BIOS_SET_RCAL_ON			(&VPORT1)->OUT &= ~BIOS_RCAL_bm
#endif

#define BIOS_TURN_OFF_AC_EXCITATION	ADC_LT_TURN_OFF_AC_EXCITATION
#define BIOS_ENABLE_AC_EXCITATION	ADC_LT_ENABLE_AC_EXCITATION
#define BIOS_AC_EXCITATION_POS		ADC_LT_AC_EXCITATION_POS
#define BIOS_AC_EXCITATION_NEG		ADC_LT_AC_EXCITATION_NEG

#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )

#define	BIOS_SPI_SS_bm		0x10

///// PORTE is for RS232 serial1, 2 ports, pins: CFG o, DTR o, DCD0 i, RFreset o
//#define	BIOS_DTR_bm			0x02
//#define	BIOS_DCD0_bm		0x10

/// PORTE is for RS232 serial1, (TXD0, RXD0), pins: CFG o, DTR0 o, DCD0 i, SPICS4 o, SPIINT i
#define	BIOS_CTS0_bm		0x01
#define	BIOS_RTS0_bm		0x02
#define	BIOS_DTR_bm			0x10
#define	BIOS_DCD0_bm		0x20
#define	BIOS_SPICS4_bm		0x40
#define	BIOS_SPIINT_bm		0x80


/// XBEE RF module of PORTE, alternative definition
#define	BIOS_RF_SLEEP_bm	0x10
#define	BIOS_RF_ASSC_bm		0x20

/// Ethernet module also uses the PORTE
#define BIOS_ETHERNET_SLEEP_bm	0x10


/// PORTF is for SPI3 port. pins: excitation XCLK o, XCLKEN o, AUX1,2,3 inputs.
// not in D3 #define	BIOS_EXCITE_XCLK_bm			0x01
// not in D3 #define	BIOS_EXCITE_XCLKEN_bm		0x02
// not in D3 #define	BIOS_AUX3_bm				0x10

/// PORTH is for external peripheral chip select pins. PORTH is mapped to virtual port0.
//#define	BIOS_A2DCS_bm				0x01
//#define	BIOS_SPICS1_bm				0x02
//#define	BIOS_SPICS2_bm				0x04
//#define	BIOS_SPICS3_bm				0x08
//#define	BIOS_CTS0_bm				0x04
//#define	BIOS_RTS0_bm				0x08
//#define	BIOS_SPICS4_bm				0x10
//#define	BIOS_SPICS5_bm				0x20
//#define	BIOS_SPICS6_bm				0x40
//#define	BIOS_FRAMCS_bm				0x80

/// PORTJ is for pins: RCAL o, D4F i, ADBUSY i, SPIINT2 i, SW3R, SW2R, SW1R CAL inputs. PORTJ is mapped to virtual port1.
#ifdef  SCALE_CORE2_REV_E
#define	BIOS_RCAL_bm				0x01
#endif
// not in D3 #define	BIOS_D4F_bm					0x02
//#define	BIOS_ADBUSY_bm				0x04
//#define	BIOS_SPIINT2_bm				0x08
// not in D3 #define	BIOS_SW3R_bm				0x10
// not in D3 #define	BIOS_SW2R_bm				0x20
//??? #define	BIOS_SW1_CAL_bm				0x40

/// PORTD is for pins: ADBUSY i, FRAMCS o, A2DCS o
#define	BIOS_ADBUSY_bm				0x01
#define	BIOS_FRAMCS_bm				0x02
#define	BIOS_A2DCS_bm				0x10

/// PORTR is for pins: SPICS2 o, SPICS3 o
#define	BIOS_SPICS2_bm		0x01
#define	BIOS_SPICS3_bm		0x02

/// PORTF is for pins: ON o, VAON o, P1ON o, EXCON o, AUX1, 2 inputs.
#define	BIOS_ON_bm					0x01
#define	BIOS_VAON_bm				0x02
#define	BIOS_P1ON_bm				0x04
#define	BIOS_EXCON_bm				0x08
#define	BIOS_RF_RESET_bm			0x10
#define	BIOS_RF_CFG_bm				0x20
#define	BIOS_AUX1_bm				0x40
#define	BIOS_AUX2_bm				0x80

// not in D3 #define	BIOS_232ON_bm				0x08	// it is replaced by BIOS_P1ON_bm
// not in D3 #define	BIOS_BLULED1_bm				0x10
// not in D3 #define	BIOS_REDLED1_bm				0x20
// not in D3 #define	BIOS_GRNLED1_bm				0x40
// not in D3 #define	BIOS_GRNLED2_bm				0x80

/// PORTQ is for pins: PQ2 o, PQ3 o. Both of this output pins set low to drive a LED for GRNLED2.
// not in D3 #define	BIOS_LED2_SRC1_bm		0x04
// not in D3 #define	BIOS_LED2_SRC2_bm		0x08

// not in D3 #define BIOS_ON_BOARD_LED_PORT				&VPORT2
// not in D3 #define BIOS_ON_BOARD_LED_PORT_OUT_REG		(&VPORT2)->OUT
#define BIOS_POWER_CTRL_PORT				&PORTF

// -WFC- 2010-08-23 v
// not in D3 #define BIOS_ON_BOARD_LED_PORT_Q				&PORTQ
// not in D3 #define BIOS_ON_BOARD_LED_PORT_Q_OUT_REG		(&PORTQ)->OUT
/// Turn off two exc LEDs and GREEN LED#2.
// not in D3 #define BIOS_TURN_ON_EXC_GRNLED2		BIOS_ON_BOARD_LED_PORT_Q_OUT_REG &= ( BIOS_PIN2_bm | BIOS_PIN3_bm )
// not in D3 #define BIOS_TURN_OFF_EXC_GRNLED2		BIOS_ON_BOARD_LED_PORT_Q_OUT_REG &= ~( BIOS_PIN2_bm | BIOS_PIN3_bm )
// not in D3 #define BIOS_TURN_OFF_ALL_ON_BOARD_LEDS	BIOS_ON_BOARD_LED_PORT_OUT_REG |= 0XF0; BIOS_TURN_OFF_EXC_GRNLED2
// -WFC- 2010-08-23 ^


// not in D3 #define BIOS_TURN_ON_P4_POWER	(BIOS_POWER_CTRL_PORT)->OUT |=	BIOS_ON_bm
// not in D3 #define BIOS_TURN_OFF_P4_POWER	(BIOS_POWER_CTRL_PORT)->OUT &= ~BIOS_ON_bm

#define BIOS_TURN_ON_P1_POWER	(BIOS_POWER_CTRL_PORT)->OUT |=	BIOS_P1ON_bm
#define BIOS_TURN_OFF_P1_POWER	(BIOS_POWER_CTRL_PORT)->OUT &= ~BIOS_P1ON_bm

#define BIOS_TURN_ON_ANALOG_POWER	(BIOS_POWER_CTRL_PORT)->OUT |=	BIOS_VAON_bm
#define BIOS_TURN_OFF_ANALOG_POWER	(BIOS_POWER_CTRL_PORT)->OUT &= ~BIOS_VAON_bm

#define BIOS_TURN_ON_RS232_POWER	(BIOS_POWER_CTRL_PORT)->OUT |=	BIOS_232ON_bm
#define BIOS_TURN_OFF_RS232_POWER	(BIOS_POWER_CTRL_PORT)->OUT &= ~BIOS_232ON_bm

// Not applicable for ScaleCore2
// #define BIOS_SET_EXTIO_HIGH			PORTG |=  (1<<PG0)
// #define BIOS_SET_EXTIO_LOW			PORTG &= ~(1<<PG0)

#define TURN_OFF_RS232_CHIP			BIOS_TURN_OFF_P1_POWER				//2013-04-26 -WFC-
#define TURN_ON_RS232_CHIP			BIOS_TURN_ON_P1_POWER				//2013-04-26 -WFC-

//#define BIOS_TURN_OFF_XBEE				PORTA |= ( BIOS_RF_SLEEP_bm | BIOS_RTS0_bm)
//#define BIOS_TURN_ON_XBEE				PORTA |= BIOS_RF_RESET_bm; PORTA &= ~( BIOS_RF_SLEEP_bm | BIOS_RTS0_bm)

//#define BIOS_TURN_OFF_XBEE			(&PORTE)->OUT |= ( BIOS_RF_SLEEP_bm | BIOS_RTS0_bm)
//#define BIOS_TURN_ON_XBEE			(&PORTF)->OUT |= BIOS_RF_RESET_bm;  (&PORTE)->OUT &=  ~( BIOS_RF_SLEEP_bm | BIOS_RTS0_bm)

//#define BIOS_TURN_OFF_XBEE			(&PORTE)->OUT |= ( BIOS_RF_SLEEP_bm )
#define BIOS_TURN_OFF_XBEE			(&PORTE)->OUT |= ( BIOS_RF_SLEEP_bm | BIOS_RTS0_bm)
#define BIOS_TURN_ON_XBEE			(&PORTF)->OUT |= BIOS_RF_RESET_bm;  (&PORTE)->OUT &=  ~( BIOS_RF_SLEEP_bm | BIOS_RTS0_bm)
#define BIOS_TURN_OFF_XBEE_SLEEP		(&PORTE)->OUT |= ( BIOS_RF_SLEEP_bm )
#define BIOS_TURN_ON_XBEE_SLEEP			(&PORTE)->OUT &=  ~( BIOS_RF_SLEEP_bm )

// -DLM- 2013-10-15
#define BIOS_TURN_OFF_ETHERNET		(&PORTE)->OUT |=  BIOS_ETHERNET_SLEEP_bm; (&PORTE)->OUT &= ~BIOS_RTS0_bm; (&PORTF)->OUT &= ~(BIOS_RF_CFG_bm | BIOS_RF_RESET_bm)
#define BIOS_TURN_ON_ETHERNET		(&PORTE)->OUT &= ~BIOS_ETHERNET_SLEEP_bm; (&PORTE)->OUT |=  BIOS_RTS0_bm; (&PORTF)->OUT |=  (BIOS_RF_CFG_bm | BIOS_RF_RESET_bm)


#ifdef  SCALE_CORE2_REV_E
// NO MORE RCAL control pin for Rev. J.
#define BIOS_SET_RCAL_OFF			(&VPORT1)->OUT |=  BIOS_RCAL_bm
#define BIOS_SET_RCAL_ON			(&VPORT1)->OUT &= ~BIOS_RCAL_bm
#endif

#define BIOS_TURN_OFF_AC_EXCITATION	ADC_LT_TURN_OFF_AC_EXCITATION
#define BIOS_ENABLE_AC_EXCITATION	ADC_LT_ENABLE_AC_EXCITATION
#define BIOS_AC_EXCITATION_POS		ADC_LT_AC_EXCITATION_POS
#define BIOS_AC_EXCITATION_NEG		ADC_LT_AC_EXCITATION_NEG

#endif // ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 ) #elif ( CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 ) #elif ( CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )

#define	BIOS_RUN_STATUS_PEND_POWER_OFF			0x80
#define	BIOS_RUN_STATUS_POWER_OFF				0x40
#define BIOS_RUN_STATUS_SLOW_CPU_CLOCK			0x20		// 2015-01-14 -WFC- copied from Dynalink2 project.
#define	BIOS_RUN_STATUS_NOT_OK_UPDATE_SW_LOG	0x03
#define	BIOS_RUN_STATUS_DEBOUNCE_SCAN			0x02
#define	BIOS_RUN_STATUS_LB_LED_ON				0x01

extern	BYTE	gbBiosRunStatus;
extern	BYTE	gbBiosBinarySwitches;
extern	BYTE	gbBiosNotOKupateSwLog;
extern	BYTE	gbBiosBinarySwitchesLog;

/// Register of Falling Edge of switches.
extern	BYTE	gbBiosBinarySwitchesFallingEdge;


/// debounce timer to be use for scan key switch or other binary level inputs.
extern	TIMER_T		gDebounceTimer;


#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS	== CONFIG_AS_HD || \
	CONFIG_PRODUCT_AS == CONFIG_AS_HLI  || CONFIG_PRODUCT_AS	== CONFIG_AS_IDSC || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )

/*
 * Scan binary level switch inputs. It is on P3 connector.
 * D1 is hook up to ADC3A, D2==> ADC4A, D3==> ADC1A, D4==> PJ1 as DF4 in VPORT1.
 *
 * D4 zero switch.
 * D3 is wheels switch.
 * D2 is lower bonus hook switch.
 * D1 is lower hook switch.
 *
 * @return  current key pattern
 *
 * History:  Created on 2010/04/20 by Wai Fai Chin
 */

#define BIOS_BINARY_INPUTS_AUX2	0x20
#define BIOS_BINARY_INPUTS_AUX1	0x10
#define BIOS_BINARY_INPUTS_D4	0x08
#define BIOS_BINARY_INPUTS_D3	0x04
#define BIOS_BINARY_INPUTS_D2	0x02
#define BIOS_BINARY_INPUTS_D1	0x01

#define BIOS_EDGE_INPUTS_AUX2	0x20
#define BIOS_EDGE_INPUTS_AUX1	0x10
#define BIOS_EDGE_INPUTS_D4		0x08
#define BIOS_EDGE_INPUTS_D3		0x04
#define BIOS_EDGE_INPUTS_D2		0x02
#define BIOS_EDGE_INPUTS_D1		0x01

BYTE	bios_scan_binary_switch_inputs( void );

#endif


// 2011-09-26 -WFC- v Merged from ScaleCore1 bios.h file.
// BIOS SYS STATUS: Voltage, Temperature, active com, small motion etc... 2011-04-18 -WFC-
#define  BIOS_SYS_STATUS_UNDER_VOLTAGE				0x80
#define  BIOS_SYS_STATUS_ANNC_OVER_TEMPERATURE		0x40
#define  BIOS_SYS_STATUS_ANNC_UNDER_TEMPERATURE		0x20
#define  BIOS_SYS_STATUS_VOLTAGE_BLINK_WARNING		0x10
#define  BIOS_SYS_STATUS_ACTIVE_COMMUNICATION		0x08

///	Bio System status for voltage, temperature, active communication, small scale motion etc..
extern	BYTE	gbBiosSysStatus;		// 2011-04-18 -WFC-
// 2011-09-26 -WFC- ^

void bios_scan_inputs_tasks( void );
void bios_scan_inputs_init( void );
void bios_power_off_and_auto_wakeup( void );			// 2014-09-17 -WFC-
void bios_enabled_comports_in_power_save_state( void ); // 2014-11-05 -WFC-
void bios_clock_normal( void );								// 2015-01-14 -WFC-


/** \def bios_disabled_interrupt_for_power_key(
 *
 * @return none.
 *
 * History:  Created on 2011-12-14 by Wai Fai Chin
 * 2013-06-13 -WFC- ported to ScaleCore3.
 */
#define bios_disabled_interrupt_for_power_key()			PORTB.INT0MASK = PORTB.INT1MASK = 0;


#if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-12-16 -WFC-
///** \def bios_disabled_interrupt_for_power_key(
// *
// * @return none.
// *
// * History:  Created on 2011-12-14 by Wai Fai Chin
// */
//#define bios_disabled_interrupt_for_power_key()			PORTA.INT0MASK = PORTA.INT1MASK = 0;

void bios_power_off_by_power_key( void );
void bios_power_off_by_shutdown_event( void );
#endif

// 2013-05-06 -WFC-
#define	BIOS_BINARY_SWITCH_CAL					BIOS_PIN5_bm
#define bios_is_cal_switch_pressed()		( (~(PORT_GetPortValue( &PORTB )) & BIOS_PIN5_bm))

#endif
//@}
