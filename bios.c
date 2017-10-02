/*! \file bios.c \brief bios implementation. */
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: bios.c
// Hardware: ATMEL XMEGA128A1
// OS:       independent
// Compiler: avr-gcc
// Software layer: Product System Hardware
//
//  History:  Created on 2009/10/28 by Wai Fai Chin
//
// This provides all the basic input, output functions of a hardware system
// which included cpu peripherals and cpu external peripheral.
// It sets up all required control registers to run the cpu.
// This file will manage different hardware modules from other module files like:
// Serial I/O, SPI, Timer tick count, and other ISRs.
// Hardware independent application codes should always use bios.h file access the
// hardware dependent functions. This will help isolate application codes from the
// hardware dependent codes.
//	
// ****************************************************************************


/*!
	Atmel AVR CPU pin configuration.
Overview
--------
XMEGA has flexible General Purpose I/O (GPIO) Ports. A port consists of up to 8 pins ranging
from pin 0 to 7, where each pin can be configured as input or output with highly configurable
driver and pull settings. The ports also implement several functions including interrupts, synchronous/
asynchronous input sensing and asynchronous wake-up signaling.

All functions are individual per pin, but several pins may be configured in a single operation. All
ports have true Read-Modify-Write (RMW) functionality when used as general purpose I/O ports.
The direction of one port pin can be changed without unintentionally changing the direction of
any other pin. The same applies when changing drive value when configured as output, or
enabling/disabling of pull-up or pull-down resistors when configured as input.

Using the I/O Pin
-----------------
Use of an I/O pin is controlled from the user software. Each port has one Data Direction (DIR),
Data Output Value (OUT) that is used for port pin control. The Data Input Value (IN) register is
used for reading the port pins. In addition each pin has a Pin Configuration (PINnCTRL) register
for additional pin configuration.

Direction of the pin is decided by the DIRn bit in the DIR register. If DIRn is written to one, pin n
is configured as an output pin. If DIRn is written to zero, pin n is configured as an input pin.

When direction is set as output, the OUTn bit in OUT is used to set the value of the pin. If OUTn
is written to one, pin n is driven high. If OUTn is written to zero, pin n is driven low.

The IN register is used for reading the pin value. The pin value can always be read regardless of
the pin being configured as input or output, except if digital input is disabled.

I/O pins are tri-stated when reset condition becomes active, even if no clocks are running.

I/O Pin Configuration
---------------------
The Pin n Configuration (PINnCTRL) register is used for additional I/O pin configuration. A pin
can be set in a totem-pole, wired-AND, or wired-OR configuration. It is also possible to enable
inverted input and output for the pin.

For totem-pole output there are four possible pull configurations: Totem-pole (Push-pull), Pulldown,
Pull-up and Bus-keeper. The bus-keeper is active in both directions. This is to avoid oscillation
when disabling the output. The totem-pole configurations with pull-up and pull-down only
have active resistors when the pin is set as input. This feature eliminates unnecessary power
consumption.

For wired-AND and wired-OR configuration, the optional pull-up and pull-down resistors are
active in both input and output direction.

Since pull configuration is configured through the pin configuration register, all intermediate port
states during switching of pin direction and pin values are avoided.

Input Sense Configuration
-------------------------
Input sensing is used to detect an edge or level on the I/O pin input. The different sense configurations
that are available for each pin are detection of rising edge, falling edge or both edges, or
detection of low level. High level can be detected by using inverted input. Input sensing can be
used to trigger interrupt requests (IREQ) or s when there is a change on the pin.
The I/O pins support synchronous and asynchronous input sensing. Synchronous sensing
requires presence of the peripheral clock, while asynchronous sensing does not require any
clock.

Port Interrupt
--------------
Each port has two interrupt vectors, and it is configurable which pins on the port that can be
used to trigger each interrupt request. Port interrupts must be enabled before they can be used.
Which sense configurations that can be used to generate interrupts is dependent on whether
synchronous or asynchronous input sensing is used.

For synchronous sensing, all sense configurations can be used to generate interrupts. For edge
detection, the changed pin value must be sampled once by the peripheral clock for an interrupt
request to be generated.

For asynchronous sensing, only port pin 2 on each port has full asynchronous sense support.
This means that for edge detection, pin 2 will detect and latch any edge and it will always trigger
an interrupt request. The other port pins have limited asynchronous sense support. This means
that for edge detection the changed value must be held until the device wakes up and a clock is
present. If the pin value returns to its initial value before the end of the device start-up time, the
device will still wake up, but no interrupt request will be generated.

A low level can always be detected by all pins, regardless of a peripheral clock being present or
not. If a pin is configured for low level sensing, the interrupt will trigger as long as the pin is held
low. In active mode the low level must be kept until the completion of the currently executing
instructions for an interrupt to be generated. In all sleep modes the low level must be kept until
the end of the device start-up time for an interrupt to be generated. If the low level disappears
before the end of the start-up time, the device will still wake up, but no interrupt will be
generated.

Port Event
----------
Port pins can generate an event when there is a change on the pin. The sense configurations
decide when each pin will generate events. Event generation requires the presence of a peripheral
clock, hence asynchronous event generation is not possible. For edge sensing, the
changed pin value must be sampled once by the peripheral clock for an event to be generated.
For low level sensing, events generation will follow the pin value.

A pin change from high to low (falling edge) will not generate an event, the pin change must be
from low to high (rising edge) for events to be generated. In order to generate events on falling
edge, the pin configuration must be set to inverted I/O. A low pin value will not generate events,
and a high pin value will continuously generate events.

Alternate Port Functions
------------------------
Most port pins have alternate pin functions in addition to being a general purpose I/O pin. When
an alternate function is enabled this might OVERRIDE normal port pin function or pin value. This
happens when other peripherals that require pins are enabled or configured to use pins. If, and
how a peripheral will override and use pins is described in section for that peripheral.

Multi-configuration
-------------------
MPCMASK can be used to set a bit mask for the pin configuration registers. When setting bit n in
MPCMASK, PINnCTRL is added to the pin configuration mask. During the next write to any of
the port's pin configuration registers, the same value will be written to all the port's pin configuration
registers set by the mask. The MPCMASK register is cleared automatically after the write
operation to the pin configuration registers is finished.

Virtual Registers
-----------------
Virtual port registers allows for port registers in the extended I/O memory space to be mapped
virtually in the I/O memory space. When mapping a port, writing to the virtual port register will be
the same as writing to the real port register. This enables use of I/O memory specific instructions
for bit-manipulation, and the I/O memory specific instructions IN and OUT on port register that
normally resides in the extended I/O memory space. There are four virtual ports, so up to four
ports can be mapped virtually at the same time. The mapped registers are IN, OUT, DIR and
INTFLAGS.

*/

#include	"bios.h"
#include	"clksys_driver.h"
#include	"dac_cpu.h"

// 2011-12-15 -WFC- v
#include	"cmdaction.h"			// 2011-12-15 -WFC- for gbCmdSysRunMode.
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 || CONFIG_PRODUCT_AS == CONFIG_AS_DYNA_LINK_II || \
	  CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI || \
	  CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )
#include	"panel_setup.h"
#include	"panelmain.h"
#endif
// 2011-12-15 -WFC- ^

// 2013-10-18 -WFC- WORD16 gTestDelay;

// #define	BIOS_RUN_STATUS_PEND_POWER_OFF		0x80
// #define	BIOS_RUN_STATUS_POWER_OFF			0x40
// #define	BIOS_RUN_STATUS_DEBOUNCE_SCAN		0x02
// #define	BIOS_RUN_STATUS_LB_LED_ON			0x01

BYTE	gbBiosRunStatus;

///	Bio System status for voltage, temperature, active communication, small scale motion etc..
BYTE	gbBiosSysStatus;		// 2011-04-18 -WFC-, 2011-09-27 -WFC-

BYTE	gbBiosNotOKupateSwLog;
BYTE	gbBiosBinarySwitchesLog;


/// Prev debounced switch state, 0==low, 1=high.
BYTE gbBiosBinarySwitchesPrvDebounce;

/// debounced switch state, 0==low, 1=high.
BYTE	gbBiosBinarySwitches;

/// Previous debounced switch state, 0==low, 1=high.
BYTE	gbPrvBiosBinarySwitches;

/// Register of Falling Edge of switches.
BYTE	gbBiosBinarySwitchesFallingEdge;

/// debounce timer to be use for scan key switch or other binary level inputs.
TIMER_T		gDebounceTimer;


void bios_io_ports_init( void );

void bios_extern_interrupts_init( void );
void bios_system_init_clocks( void );

void bios_configure_interrupt_for_power_key( void );		// 2011-12-14 -WFC-
void bios_cnfg_io_ports_for_power_off( void );				// 2011-12-14 -WFC-
void bios_power_off( void );									// 2011-12-14 -WFC-
void bios_sleep( void );									// 2011-12-15 -WFC-
void bios_power_off_and_auto_wakeup( void );				// 2014-09-17 -WFC-
void bios_sleep_idle( void );								// 2014-09-26 -WFC-


/**
 * It initializes hardware system.
 * This routine must call before init application.
 *
 * History:  Created on 2007/06/27 by Wai Fai Chin
 */

// WORD16 idata gwDebug;

void bios_system_init( void )
{
	// 2013-10-18 -WFC-  gTestDelay = 300;

	bios_io_ports_init();			// must initialized ports before switching system clock source. Otherwise, it would not work.
	// not in D3 	BIOS_TURN_OFF_ALL_ON_BOARD_LEDS;		// -WFC- 2010-08-23
	bios_system_init_clocks();
	gbBiosRunStatus = 0;
	gbBiosBinarySwitchesFallingEdge = 0;
	//bios_io_ports_init();
	dac_cpu_init_hardware();
	bios_extern_interrupts_init();
	timer_task_init();
	spi1_master_init();
	spi2_master_init();
	#if ( !(CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI) )
	i2c_init();							// 2011-09-28 -WFC-
	#endif
}


/**
 * It initializes hardware io ports during power up...
 *
 * This routine must call before init application.
 *	 PORTA is for ADCA and DAC
 *	 PORTB is for ADCB , DAC and JTAG
 *	 PORTC is for TWI, SPI1 ports. pins: ss1 o, SPIINT1 i and PWRINT i. NOTE: SPI1 is for FRAM chip and P4 connector Master port for both SPI and I2C.
 *	 PORTD is for RS232 serial0 port, SPI2 port. pin: ss2. NOTE: SPI2 is for P5 connector.
 *	 PORTE is for RS232 serial1, 2 ports, pins: CFG o, DTR o, DCD0 i, RFreset o
 *	 PORTF is for SPI3 port. pins: excitation XCLK o, XCLKEN o, AUX1,2,3 inputs.
 *	 PORTH is for external peripheral chip select pins. PORTH is mapped to virtual port0.
 *	 PORTJ is for pins: RCAL o, D4F i, ADBUSY i, SPIINT2 i, SW3R, SW2R, SW1R CAL inputs. PORTJ is mapped to virtual port1.
 *	 PORTK is for pins: ON o, P1ON o, VAON o, 232ON o, BLULED1 o, REDLED1 o, GRNLED1 o, GRNLED2 o. PORTK is mapped to virtual port2.
 *	 PORTQ is for pins: PQ2 o, PQ3 o. Both of this output pins set low to drive a LED for GRNLED2.
 *
 * History:  Created on 2009/10/28 by Wai Fai Chin
 * 2011-10-07 -WFC- Configured PORT E and H io pins for XBEE RF module for Remote Meter.
 */

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER)	// 2011-10-07 -WFC-
// 2012-06-08 -WFC- configure J1, A1, A3 and A3 pins as pull up inputs instead of Totempole for MSI8000.

void bios_io_ports_init( void )
{
	// PORTA is for ADCA and DAC

	PORT_ConfigurePins( &PORTA,
			BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_PIN4_bm,	// configure pin1, pin3 and pin4
	        false,							// no slew rate control
	        false,							// no inversion
	        // 2012-06-08 -WFC- PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs. 2012-06-08 -WFC-
	        PORT_ISC_BOTHEDGES_gc );

	// set all pins on port A as inputs for now. The CPU ADC module will take care the rest.
	PORT_SetDirection( &PORTA, 0);

	// PORTB pin0, pin1 configured as Totempole.
	PORT_ConfigurePins( &PORTB,
			BIOS_PIN0_bm | BIOS_PIN1_bm,	// configure pin0, pin1
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );

	// set all pins on port B as inputs except pin2 for now. JTAG is override pin4 to pin7. The CPU ADC module will take care the rest. PIN2 is for DAC0 output
	PORT_SetDirection( &PORTB, BIOS_PIN2_bm);


	// PORTC is for TWI, SPI1 ports. pins: ss1 o, SPIINT1 i and PWRINT i. NOTE: SPI1 is for FRAM chip and P4 connector Master port for both SPI and I2C.
	// 0==in, 1==out,  x==don't care. xxx100xx ==> 00010000 =0x10
	// PORT_SetDirection( &PORTC, 0x10 );
	// PORT_SetDirection( &PORTC, BIOS_SPI_SS_bm );		//set SPI1 ss as output

	// PORTC spi_ss, pin0, pin1 configured as Totempole.
	PORT_ConfigurePins( &PORTC,
			BIOS_SPI_SS_bm | BIOS_PIN0_bm | BIOS_PIN1_bm,	// configure spi_ss, pin0, pin1
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );

	PORT_SetDirection( &PORTC, (BIOS_SPI_SS_bm | BIOS_PIN0_bm | BIOS_PIN1_bm)  );		//set SPI1 ss, PIN0 and PIN1 as outputs

	// PORTD is for RS232 serial0 port, SPI2 port. pin: ss2. NOTE: SPI2 is for P5 connector.
	//
	//PORT_SetDirection( &PORTD, BIOS_SPI_SS_bm );		// SPI init routine will take care of it.

	(&PORTE)->OUT |= BIOS_RF_RESET_bm;		// set RF_RESET 2011-10-07 -WFC-
	(&PORTE)->OUT &= ~(BIOS_RF_SLEEP_bm);	// set RF SLEEP pin low to wake up the RF module. 2011-10-21 -WFC-
	// PORTE is for RS232 serial1, 2 ports, pins: CFG o, DTR o, DCD0 i, RFreset o
	PORT_SetDirection( &PORTE,  BIOS_RF_SLEEP_bm | BIOS_RF_RESET_bm );		//set these pins as output

	// 2011-05-19 -WFC- (&PORTE)->OUT |= BIOS_RF_RESET_bm;		// set RF_RESET pin high 2011-05-10 -WFC-

	PORT_ConfigurePins( &PORTF,   0x01F,				// configure pin0 to pin5
	                    false,							// no slew rate control
	                    false,							// no inversion
	                    PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs.
	                    PORT_ISC_BOTHEDGES_gc );

	// PORTF is for SPI3 port. pins: excitation XCLK o, XCLKEN o, AUX1,2,3 inputs.
	//TODO: PORTF SPI is now in UART SPI master mode. The SPI init routine will take care of it. pin7 is MOSI.
	// Note that UART SPI master mode will not automatically configure MOSI as output, you have to manual configure it.
	// Otherwise it will not work.
	PORT_SetDirection( &PORTF, BIOS_EXCITE_XCLK_bm | BIOS_EXCITE_XCLKEN_bm | BIOS_PIN7_bm );			//set XCLK, XCLKEN, and MOSI pins as output

	// PORTH is mapped to virtual port0.
	PORT_MapVirtualPort0( PORTCFG_VP0MAP_PORTH_gc );
	// PORTH is for external peripheral chip select pins and CTS0, RTS0. RTS is an output pin.
	PORT_SetDirection( &VPORT0, 0xFF & ~BIOS_CTS0_bm);	// set all pins of PORTH as output, except CTS0.
	PORT_SetOutputValue( &VPORT0, 0xFF & ~BIOS_RTS0_bm);	// de-select all chips, rts low for RF module.

	/* 2012-06-08 -WFC-  v
	// configure PORT J pin0, and pin2 to pin7 to Totempole pullup.
	PORT_ConfigurePins( &PORTJ,   0x0FD,				// configure all pins except pin1.
	                    false,							// no slew rate control
	                    false,							// no inversion
	                    PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs.
	                    PORT_ISC_BOTHEDGES_gc );

	// configure PORT J pin1 to Totempole only.
	PORT_ConfigurePins( &PORTJ,   BIOS_PIN1_bm,			// configure pin1.
	                    false,							// no slew rate control
	                    false,							// no inversion
	        	        PORT_OPC_TOTEM_gc,				// Totempole
	                    PORT_ISC_BOTHEDGES_gc );
	*/
	// configure PORT J pin0, and pin2 to pin7 to Totempole pullup.
	PORT_ConfigurePins( &PORTJ,   0x0FF,				// configure all pins.
	                    false,							// no slew rate control
	                    false,							// no inversion
	                    PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs.
	                    PORT_ISC_BOTHEDGES_gc );

	// 2012-06-08 -WFC-  ^

	// set all pins on port J as inputs for now. The CPU ADC module will take care the rest.
	PORT_SetDirection( &PORTJ, 0);

	// PORTJ is for pins:  D4F i, ADBUSY i, SPIINT2 i, SW3R, SW2R, SW1R CAL inputs. PORTJ is mapped to virtual port1.
	PORT_MapVirtualPort1( PORTCFG_VP1MAP_PORTJ_gc );
	// PORT_SetDirection( &VPORT1, BIOS_RCAL_bm );		// set RCAL as output, the rest of pins as inputs.
	PORT_SetDirection( &VPORT1, 0);						// configured all pins as inputs.

	// PORTQ is for pins: PQ2 o, PQ3 o. Both of this output pins set low to drive a LED for GRNLED2.
	PORT_ConfigurePins( &PORTQ,   0x03,					// configure pin0 and pin1 are unused.
	                    false,							// no slew rate control
	                    false,							// no inversion
	                    PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs.
	                    PORT_ISC_BOTHEDGES_gc );

	PORT_SetDirection( &PORTQ, BIOS_LED2_SRC1_bm | BIOS_LED2_SRC2_bm );		//set these pins as output

	// PORTK is for pins: ON o, P1ON o, VAON o, 232ON o, BLULED1 o, REDLED1 o, GRNLED1 o, GRNLED2 o. PORTK is mapped to virtual port2.
	PORT_MapVirtualPort2( PORTCFG_VP2MAP_PORTK_gc );
	PORT_SetDirection( &VPORT2, 0xFF );					// set all pins of PORTK as output.

	//BIOS_TURN_ON_P4_POWER;
	BIOS_TURN_ON_RS232_POWER;
	BIOS_TURN_ON_P1_POWER;
	BIOS_TURN_ON_ANALOG_POWER;

}// end bios_io_ports_init()

#else
void bios_io_ports_init( void )
{

	// PORTA is for ADCA and DAC

	//turn off interrupt
	PORTA.INTCTRL=(PORTA.INTCTRL & (~(PORT_INT1LVL_gm | PORT_INT0LVL_gm))) |
		PORT_INT1LVL_OFF_gc | PORT_INT0LVL_OFF_gc;
	PORTA.INT0MASK=0x00;  //turn off interrupt
	PORTA.INT1MASK=0x00; //turn off interrupt

	PORT_ConfigurePins( &PORTA,
			// BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_PIN4_bm,	// configure pin1, pin3 and pin4
			0xFF,							// configure pin1 to pin7.
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );

	// set all pins on port A as inputs for now. The CPU ADC module will take care the rest.
	PORT_SetDirection( &PORTA, 0);

	PORTB.INTCTRL=(PORTB.INTCTRL & (~(PORT_INT1LVL_gm | PORT_INT0LVL_gm))) |
		PORT_INT1LVL_OFF_gc | PORT_INT0LVL_OFF_gc;
	PORTB.INT0MASK=0x00;  //turn off interrupt
	PORTB.INT1MASK=0x00; //turn off interrupt

	// PORTB pin0, pin1 configured as Totempole.
	PORT_ConfigurePins( &PORTB,
			// BIOS_PIN0_bm | BIOS_PIN1_bm,	// configure pin0, pin1
			0xFF,							// configure pin1 to pin7.
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_PULLUP_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );

	// set all pins on port B as inputs.
	PORT_SetDirection( &PORTB, 0);

	// PortC:
	// pin0: Output == SDA
	// pin1: Output == SCL
	// pin2: Input	== RXD1
	// pin3: Output	== TXD1
	// pin4: Output == SS1
	// pin5: Output == MOSI1
	// pin6: Input  == MISO1
	// pin7: Output == SCK1

	// PORTC is for TWI, SPI1 and serial0 ports. pins: ss1 o,  NOTE: SPI1 is for P4 and P5 connector.  P4 connector Master port for both SPI and I2C.
	// 0==in, 1==out,  x==don't care. xxx100xx ==> 00010000 =0x10
	// PORT_SetDirection( &PORTC, 0x10 );
	// PORT_SetDirection( &PORTC, BIOS_SPI_SS_bm );		//set SPI1 ss as output

	PORTC.INTCTRL=(PORTC.INTCTRL & (~(PORT_INT1LVL_gm | PORT_INT0LVL_gm))) |
		PORT_INT1LVL_OFF_gc | PORT_INT0LVL_OFF_gc;
	PORTC.INT0MASK=0x00;  //turn off interrupt
	PORTC.INT1MASK=0x00; //turn off interrupt

	// PORTC configured as Totempole.
	PORT_ConfigurePins( &PORTC,
			BIOS_PIN0_bm | BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_SPI_SS_bm | BIOS_PIN5_bm | BIOS_PIN7_bm,	// configure pin0, pin1, pin3, pin4, pin5, pin7.
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );

	PORT_SetDirection( &PORTC, (BIOS_PIN0_bm | BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_SPI_SS_bm | BIOS_PIN5_bm | BIOS_PIN7_bm)  );		//set pin0, pin1, pin3, pin4, pin5, pin7 as outputs, pin2 and pin6 as inputs.

	// PortD:
	// pin0: Input	== ADBUSY
	// pin1: Output	== FRAMCS
	// pin2: Input	== RXD2
	// pin3: Output	== TXD2
	// pin4: Output	== A2DCS
	// pin5: Output	== MOSI3
	// pin6: Input	== MISO3
	// pin7: Output	== SCK3

	PORTD.INTCTRL=(PORTD.INTCTRL & (~(PORT_INT1LVL_gm | PORT_INT0LVL_gm))) |
		PORT_INT1LVL_OFF_gc | PORT_INT0LVL_OFF_gc;
	PORTD.INT0MASK=0x00;  //turn off interrupt
	PORTD.INT1MASK=0x00; //turn off interrupt

	// PORTD is for RS232 serial2 port, SPI2 port. pin: ss2. NOTE: SPI2 is for loadcell ADC and FRAM chips. Serial2 is never use in DynaLink2, Challenger3, etc...
	PORT_ConfigurePins( &PORTD,
			BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_PIN4_bm | BIOS_PIN5_bm | BIOS_PIN7_bm,	// configure pin0, pin1, pin3, pin4, pin5, pin7.
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );

	//
	PORT_SetDirection( &PORTD, BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_PIN4_bm | BIOS_PIN5_bm | BIOS_PIN7_bm );

	// PortE:
	// pin0: Input	== CTS0
	// pin1: Output	== RTS0
	// pin2: Input	== RXD0
	// pin3: Output	== TXD0
	// pin4: Output	== DTR0 , RF_SLEEP.
	// pin5: Input	== DCD0
	// pin6: Output	== SPICS4
	// pin7: Input	== SPINT

	PORTE.INTCTRL=(PORTE.INTCTRL & (~(PORT_INT1LVL_gm | PORT_INT0LVL_gm))) |
		PORT_INT1LVL_OFF_gc | PORT_INT0LVL_OFF_gc;
	PORTE.INT0MASK=0x00;  //turn off interrupt
	PORTE.INT1MASK=0x00; //turn off interrupt

	// PORTE
	PORT_ConfigurePins( &PORTE,
			BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_PIN4_bm | BIOS_PIN6_bm,	// configure pin1, pin3, pin4, pin6.
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );
	//
	PORT_SetDirection( &PORTE, BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_PIN4_bm | BIOS_PIN6_bm );

//	PORT_SetDirection( &VPORT0, 0xFF & ~BIOS_CTS0_bm);	// set all pins of PORTH as output, except CTS0.
//	#if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER)	// 2011-10-07 -WFC-
//	PORT_SetOutputValue( &VPORT0, 0xFF & ~BIOS_RTS0_bm);	// de-select all chips, rts low for RF module.
//	#else
//	PORT_SetOutputValue( &VPORT0, 0xFF);				// de-select all chips.
//	#endif

	// PORTF:
	// pin0: Output	== ON       high == on
	// pin1: Output	== VAON     high == on
	// pin2: Output	== P1ON     low  == on
	// pin3: Output	== EXCON    high == on
	// pin4: Output	== RFRESET  high == reset
	// pin5: Output	== CFG      high == config
	// pin6: Output	== AUX1     default to low output
	// pin7: Input	== AUX2

	PORTF.INTCTRL=(PORTF.INTCTRL & (~(PORT_INT1LVL_gm | PORT_INT0LVL_gm))) |
		PORT_INT1LVL_OFF_gc | PORT_INT0LVL_OFF_gc;
	PORTF.INT0MASK=0x00;  //turn off interrupt
	PORTF.INT1MASK=0x00; //turn off interrupt

	(&PORTF)->OUT |=  BIOS_ON_bm | BIOS_VAON_bm | BIOS_P1ON_bm | BIOS_EXCON_bm;	// turn on all device

	PORT_ConfigurePins( &PORTF,
			BIOS_ON_bm | BIOS_VAON_bm | BIOS_P1ON_bm | BIOS_EXCON_bm | BIOS_RF_RESET_bm | BIOS_RF_CFG_bm | BIOS_AUX1_bm,
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );

	(&PORTE)->OUT &= ~(BIOS_RF_SLEEP_bm);	// set RF SLEEP pin low to wake up the RF module. 2011-10-21 -WFC-
	PORT_SetDirection( &PORTF, BIOS_ON_bm | BIOS_VAON_bm | BIOS_P1ON_bm | BIOS_EXCON_bm | BIOS_RF_RESET_bm | BIOS_RF_CFG_bm | BIOS_AUX1_bm);

	// PORTR:
	// pin0: Output	== SPICS2
	// pin1: Output	== SPICS3

	PORTR.INTCTRL=(PORTR.INTCTRL & (~(PORT_INT1LVL_gm | PORT_INT0LVL_gm))) |
		PORT_INT1LVL_OFF_gc | PORT_INT0LVL_OFF_gc;
	PORTR.INT0MASK=0x00;  //turn off interrupt
	PORTR.INT1MASK=0x00; //turn off interrupt

	PORT_ConfigurePins( &PORTR,
			BIOS_PIN0_bm | BIOS_PIN1_bm,	// configure pin1, pin3, pin4, pin6.
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );
//	(&PORTE)->OUT = BIOS_PIN0_bm | BIOS_PIN1_bm;				// set both pins high to deselect SPI devices.
	(&PORTR)->OUT = BIOS_PIN0_bm | BIOS_PIN1_bm;				// set both pins high to deselect SPI devices.
	PORT_SetDirection( &PORTR, BIOS_PIN0_bm | BIOS_PIN1_bm );

	BIOS_TURN_ON_P1_POWER;
	BIOS_TURN_ON_ANALOG_POWER;
	// ON_P1_POWER replaced BIOS_TURN_ON_RS232_POWER;

}// end bios_io_ports_init()

#endif

/*
void bios_io_ports_init( void )
{

	// PORTA is for ADCA and DAC

	PORT_ConfigurePins( &PORTA,
			BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_PIN4_bm,	// configure pin1, pin3 and pin4
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs.
	        PORT_ISC_BOTHEDGES_gc );

	// set all pins on port A as inputs for now. The CPU ADC module will take care the rest.
	PORT_SetDirection( &PORTA, 0);

	// PORTB pin0, pin1 configured as Totempole pullup.
	PORT_ConfigurePins( &PORTB,
			BIOS_PIN0_bm | BIOS_PIN1_bm,	// configure pin0, pin1
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs.
	        PORT_ISC_BOTHEDGES_gc );

	// set all pins on port B as inputs except pin2 for now. JTAG is override pin4 to pin7. The CPU ADC module will take care the rest. PIN2 is for DAC0 output
	PORT_SetDirection( &PORTB, BIOS_PIN2_bm);


	// PORTC is for TWI, SPI1 ports. pins: ss1 o, SPIINT1 i and PWRINT i. NOTE: SPI1 is for FRAM chip and P4 connector Master port for both SPI and I2C.
	// 0==in, 1==out,  x==don't care. xxx100xx ==> 00010000 =0x10
	// PORT_SetDirection( &PORTC, 0x10 );
	PORT_SetDirection( &PORTC, BIOS_SPI_SS_bm );		//set SPI1 ss as output

	// PORTD is for RS232 serial0 port, SPI2 port. pin: ss2. NOTE: SPI2 is for P5 connector.
	//
	//PORT_SetDirection( &PORTD, BIOS_SPI_SS_bm );		// SPI init routine will take care of it.

	// PORTE is for RS232 serial1, 2 ports, pins: CFG o, DTR o, DCD0 i, RFreset o
	PORT_SetDirection( &PORTE, BIOS_RF_CFG_bm | BIOS_DTR_bm | BIOS_RF_RESET_bm );		//set these pins as output

	PORT_ConfigurePins( &PORTF,   0x03F,				// configure pin0 to pin5
	                    false,							// no slew rate control
	                    false,							// no inversion
	                    PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs.
	                    PORT_ISC_BOTHEDGES_gc );

	// PORTF pins: BIOS_ON_bm | BIOS_VAON_bm | BIOS_P1ON_bm | BIOS_EXCON_bm | BIOS_RF_RESET_bm | BIOS_RF_CFG_bm	as outputs, AUX1, AUX2 as input.
	PORT_SetDirection( &PORTF, BIOS_ON_bm | BIOS_VAON_bm | BIOS_P1ON_bm | BIOS_EXCON_bm | BIOS_RF_RESET_bm | BIOS_RF_CFG_bm	 );

	// PORTH is mapped to virtual port0.
	PORT_MapVirtualPort0( PORTCFG_VP0MAP_PORTH_gc );
	// PORTH is for external peripheral chip select pins and CTS0, RTS0. RTS is an output pin.
	PORT_SetDirection( &VPORT0, 0xFF & ~BIOS_CTS0_bm);	// set all pins of PORTH as output, except CTS0.
	PORT_SetOutputValue( &VPORT0, 0xFF );				// de-select all chips.

	// PORT J pin0 and pin7 are unused.
	PORT_ConfigurePins( &PORTJ,   0x0FF,				// configure all pins
	                    false,							// no slew rate control
	                    false,							// no inversion
	                    PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs.
	                    PORT_ISC_BOTHEDGES_gc );

	// PORTJ is for pins:  D4F i, ADBUSY i, SPIINT2 i, SW3R, SW2R, SW1R CAL inputs. PORTJ is mapped to virtual port1.
	PORT_MapVirtualPort1( PORTCFG_VP1MAP_PORTJ_gc );
	// PORT_SetDirection( &VPORT1, BIOS_RCAL_bm );		// set RCAL as output, the rest of pins as inputs.
	PORT_SetDirection( &VPORT1, 0);						// configured all pins as inputs.

	// PORTQ is for pins: PQ2 o, PQ3 o. Both of this output pins set low to drive a LED for GRNLED2.
	PORT_ConfigurePins( &PORTQ,   0x03,					// configure pin0 and pin1 are unused.
	                    false,							// no slew rate control
	                    false,							// no inversion
	                    PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs.
	                    PORT_ISC_BOTHEDGES_gc );

	PORT_SetDirection( &PORTQ, BIOS_LED2_SRC1_bm | BIOS_LED2_SRC2_bm );		//set these pins as output

	// PORTK is for pins: ON o, P1ON o, VAON o, 232ON o, BLULED1 o, REDLED1 o, GRNLED1 o, GRNLED2 o. PORTK is mapped to virtual port2.
	PORT_MapVirtualPort2( PORTCFG_VP2MAP_PORTK_gc );
	PORT_SetDirection( &VPORT2, 0xFF );					// set all pins of PORTK as output.

	BIOS_TURN_ON_P1_POWER;
	BIOS_TURN_ON_ANALOG_POWER;
	BIOS_TURN_ON_RS232_POWER;

}// end bios_io_ports_init()
*/
 
/**
 * External Interrupts initialization
 *
 * History: Created on  2011-12-09  by Wai Fai Chin
 *
 */

void bios_extern_interrupts_init( void )
{
	bios_disabled_interrupt_for_power_key();
} //end bios_extern_interrupts_init()



/**
 *  It tries to transmit a byte of data without waiting.
 *  If it successed transmitted a byte, it returns true else return false.
 *
 * @param  state	 -- a 0 == off, 1 == on
 * @param  LEDnumber -- led number range from 0 to 3, 0==red, 1==blue, 2==green. for ScaleCore1
 * 						0 to 3, 0==blue, 1==red, 2==green, 3=green2 for ScaleCore2
 * @return none
 *
 * History:  Created on 2008/08/22 by Wai Fai Chin
 * 2009/10/30	Modified for ScaleCore2.
 * 2013-10-25 -WFC- ported to ScaleCore3.
 */

void bios_on_board_led( BYTE state, BYTE LEDnumber )
{
	BYTE ledPattern;
	
	if ( LEDnumber > 2 ) {	// for test only
		switch ( LEDnumber ) {
			case 4:	BIOS_ENABLE_AC_EXCITATION;
//					BIOS_AC_EXCITATION_POS;
				break;
//			case 5:	BIOS_ENABLE_AC_EXCITATION;
//					BIOS_AC_EXCITATION_NEG;
//					break;
			case 6:	BIOS_TURN_OFF_AC_EXCITATION;
					break;
//			case 7:
//					/*
//					if ( ON == state )
//						BIOS_SET_RCAL_ON;
//					else
//						BIOS_SET_RCAL_OFF;
//					*/
//					break;
			#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )
			case 8:
					if ( ON == state )
						BIOS_SET_EXTIO_HIGH;
					else
						BIOS_SET_EXTIO_LOW;
					break;
			#endif
		}
	}
	// not in D3 v
//	else {
//		ledPattern = 1<<(LEDnumber + 5);
//		if ( ON == state )
//			BIOS_ON_BOARD_LED_PORT_OUT_REG |= ledPattern;
//		else
//			BIOS_ON_BOARD_LED_PORT_OUT_REG &= ~ledPattern;
//	}
	// not in D3 ^
}// end bios_on_board_led()


/**
 * It resets system registers to its default state as in a hardware reset.
 * It must be called before software switch to between app and boot codes,
 * otherwise, the destination code may not run.
 *
 * History:  Created on 2009/09/16 by Wai Fai Chin
 */

void bios_system_reset_sys_regs( void )
{
	/*
	serial0_disabled_tx_isr();
	serial0_disabled_rx_isr();
	//serial0_disabled_rx();
	//serial0_disabled_tx();
	serial0_clear_txcif();

	serial1_disabled_tx_isr();
	serial1_disabled_rx_isr();
	//serial1_disabled_rx();
	//serial1_disabled_tx();
	serial1_clear_txcif();

	// PORTE is for RS232 serial1, 2 ports, pins: CFG o, DTR o, DCD0 i, RFreset o
	// PORT_SetDirection( &PORTE, BIOS_RF_CFG_bm | BIOS_DTR_bm | BIOS_RF_RESET_bm );		//set these pins as output
	*/
}


/**
 * It configure system clock source and speed.
 *
 * History:  Created on 2009/11/16 by Wai Fai Chin
 */

void bios_system_init_clocks( void )
{

	/* Enable for external 12-16 MHz crystal with quick startup time
	 * (256CLK). Check if it's stable and set the external
	 * oscillator as the main clock source.
	 * / // This example from application note is not working. It got hang on wait while (CLKSYS_IsReady( OSC_XOSCRDY_bm ) == 0)
	CLKSYS_XOSC_Config( OSC_FRQRANGE_12TO16_gc,  false, OSC_XOSCSEL_EXTCLK_gc );	// 6 CLK start up
	CLKSYS_Enable( OSC_XOSCEN_bm );
	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );		//The output cpu and peripheral clocks are the same as the original source.
	do {} while ( CLKSYS_IsReady( OSC_XOSCRDY_bm ) == 0 );
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_XOSC_gc );
	*/

	/* Clock Settings. It works.
	OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc | OSC_XOSCSEL_XTAL_1KCLK_gc;    // 0.4-16 MHz XTAL - 1K CLK Start Up. IT IS IMPORTANT TO CONFIGURE THIS BEFORE SETTING OSC.CTRL.
	OSC.PLLCTRL = OSC_PLLSRC_gm | OSC_PLLFAC0_bm;	// XOSC is PLL Source - 1x Factor
	OSC.CTRL = OSC_PLLEN_bm | OSC_XOSCEN_bm;		// Enable PLL & External Oscillator
	while(!(OSC.STATUS & OSC_PLLRDY_bm)); 			// wait for PLL
	CCP = CCP_IOREG_gc;     						// Configuration Change Protection
	CLK.CTRL = CLK_SCLKSEL_PLL_gc;					// Select Phase Locked Loop
    */

	#if ( CONFIG_XTAL_FREQUENCY  == INTERNAL_32MHZ )
		/*  Enable internal 32 MHz ring oscillator and wait until it's
		 *  stable. Divide clock by two with the prescaler C and set the
		 *  32 MHz ring oscillator as the main clock source. Wait for
		 *  user input while the LEDs toggle.
		 */
	//		CLKSYS_Enable( OSC_RC32MEN_bm );
	//		// CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_2_gc );		//The output cpu and peripheral clocks are divided by 2 of the original source.
	//		CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );		//The output cpu and peripheral clocks are the same as the original source.
	//		do {} while ( CLKSYS_IsReady( OSC_RC32MRDY_bm ) == 0 );
	//		CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_RC32M_gc );
			bios_clock_normal();
	#elif ( CONFIG_XTAL_FREQUENCY  == XTAL_14P746MHZ )
		/* Enable for external 12-16 MHz crystal with startup time
		 * (1K CLK). Check if it's stable and set the external
		 * oscillator as the main clock source.
		 */ // This version works because it has 1K clk start up time.
		CLKSYS_XOSC_Config( OSC_FRQRANGE_12TO16_gc,  false, OSC_XOSCSEL_XTAL_1KCLK_gc );	// 1K CLK start up. IT IS IMPORTANT TO CONFIGURE THIS BEFORE SETTING OSC.CTRL=OSC_XOSCEN_bm.
		CLKSYS_Enable( OSC_XOSCEN_bm );
		CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );		//The output cpu and peripheral clocks are the same as the original source.
		// CLKSYS_Prescalers_Config( CLK_PSADIV_8_gc, CLK_PSBCDIV_1_1_gc );		//The output cpu and peripheral clocks are divided by 8 of the original source.
		do {} while ( CLKSYS_IsReady( OSC_XOSCRDY_bm ) == 0 );
		CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_XOSC_gc );
	#endif

} // end bios_system_init_clocks()

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
void	bios_clock_slow ( void ) {}			// 2013-09-23 -WFC-
void	bios_clock_normal ( void ) {}		// 2013-09-23 -WFC-
#endif
/**
 * Set CPU system clock and timer1 to normal speed.
 *
 * History:  Created on 2011-07-29 by Wai Fai Chin
 * 2011-08-18 -WFC- use CPU CLKPR register to find out system clock speed instead of gbBiosRunStatus, this prevent out of synch problem.
 * 2013-10-09 -WFC- modified for ScaleCore3.
 * 2016-05-25 -WFC- Based on Alex and Rob's finding, Enabled CLKSYS auto calibration in order to work with Rev.I ATxmega192D3 chip.
 */

void bios_clock_normal( void )
{
	// if ( BIOS_RUN_STATUS_SLOW_CPU_CLOCK & gbBiosRunStatus ) {			// if it is in slow cpu clock speed, then
	gbBiosRunStatus &= ~BIOS_RUN_STATUS_SLOW_CPU_CLOCK;	// flag normal speed
	// CLKSYS_Enable( OSC_RC32MEN_bm );
	CLKSYS_Enable( OSC_RC32MEN_bm | OSC_RC32KEN_bm);						// enabled RC32KHz internal oscillator as source for 32MHz calibrated source. 2016-05-25 -WFC-
	// CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_2_gc );		//The output cpu and peripheral clocks are divided by 2 of the original source.
	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );		//The output cpu and peripheral clocks are the same as the original source.
	do {} while ( CLKSYS_IsReady( OSC_RC32KRDY_bm ) == 0 );					// Wait for RC 32KHz stabilized 2016-05-25 -WFC-
	do {} while ( CLKSYS_IsReady( OSC_RC32MRDY_bm ) == 0 );
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_RC32M_gc );
	CLKSYS_AutoCalibration_Enable( OSC_RC32MCREF_bm, FALSE );				// Enabled CLKSYS auto calibration in order to work with Rev.I ATxmega192D3 chip. 2016-05-25 -WFC-


	// configure timer0
	TC_SetPeriod( &TCC0, 25000 );		// for counting up
	//  Select clock source.
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV64_gc;
//	serial0_port_init( SR_BAUD_9600_V16MHZ );		// reconfigure serial port based on new system clock speed.
//	serial1_port_init( SR_BAUD_9600_V16MHZ );		// reconfigure serial port based on new system clock speed.
} // end bios_clock_normal()

/**
 * Slow down CPU system clock and reconfigure timer1 to 50mSec tick.
 *
 * History:  Created on 2011-07-29 by Wai Fai Chin
 * 2011-08-18 -WFC- use CPU CLKPR register to find out system clock speed instead of gbBiosRunStatus, this prevent out of synch problem.
 * 2013-10-09 -WFC- modified for ScaleCore3.
 */

//void bios_clock_slow( void )
//{
//	CLKSYS_Enable( OSC_RC32MEN_bm );
//	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );		//The output cpu and peripheral clocks are divided by 1 of the original source.
//	do {} while ( CLKSYS_IsReady( OSC_RC32MRDY_bm ) == 0 );
//	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_RC2M_gc );
//
//	// configure timer0
//	TC_SetPeriod( &TCC0, 12500);		// for counting up
//	//  Select clock source.
//	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV8_gc;
//
//	gbBiosRunStatus |= BIOS_RUN_STATUS_SLOW_CPU_CLOCK;	// flag normal speed
//	serial0_port_init( SR_BAUD_9600_V2MHZ );		// reconfigure serial port based on new system clock speed.
//	serial1_port_init( SR_BAUD_9600_V2MHZ );		// reconfigure serial port based on new system clock speed.
//} // end bios_clock_slow()


#if ( CONFIG_SOFTWARE_AS_BOOTLOADER )
/**
 * Scan keypad.
 *
 * @return  current key pattern
 *
 * History:  Created on 2010/01/08 by Wai Fai Chin
 */

BYTE bios_key_scan( void )
{
  BYTE curKey;								// current key

  curKey = PORT_GetPortValue( &PORTF );		// aux inputs are in port F.
  curKey = (~curKey);						// inverted it.
  curKey &= BIOS_AUX_INPUTS_MASK;			// read aux input pins only
  return curKey;
}// end bios_key_scan()
#endif

/**
 * Scan keypad.
 *
 * @return  current key pattern
 *
 * History:  Created on 2010/01/08 by Wai Fai Chin
 */

BYTE bios_key_scan_pin_inputs( void )
{
  BYTE curKey;								// current key
/*
  curKey = PORT_GetPortValue( &PORTF );		// aux inputs are in port F.
  curKey = (~curKey);						// inverted it.
  curKey &= BIOS_AUX_INPUTS_MASK;			// read aux input pins only
  */
  return curKey;
}// end bios_key_scan_pin_inputs()


#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS == CONFIG_AS_HLI || CONFIG_PRODUCT_AS	== CONFIG_AS_IDSC )
/**
 * Scan binary level switch inputs. It is on P3 connector.
 * D1 is hook up to ADC3A, D2==> ADC4A, D3==> ADC1A, D4==> PJ1 as DF4 in VPORT1.
 *
 * D4 zero switch. 					Closed = 0;
 * D3 is wheels switch.				Closed = 0;
 * D2 is lower bonus hook switch.	Closed = 28V = 1;
 * D1 is lower hook switch.			Closed = 28V = 1;
 *
 * @return  current key pattern
 *
 * History:  Created on 2010/04/20 by Wai Fai Chin
 */

BYTE bios_scan_binary_switch_inputs( void )
{
  BYTE b;
  BYTE sw;								// switch pattern.
  sw = 0;

  b = PORT_GetPortValue( &VPORT1 );		// b = pins inputs from port J.
  if ( BIOS_D4F_bm & b )
	  sw = BIOS_BINARY_INPUTS_D4;

  b = PORT_GetPortValue( &PORTA );		// b = pins inputs from port A.
  if ( BIOS_PIN3_bm & b)
	  sw |= BIOS_BINARY_INPUTS_D1;
  if ( BIOS_PIN4_bm & b)
	  sw |= BIOS_BINARY_INPUTS_D2;
  if ( BIOS_PIN1_bm & b)
	  sw |= BIOS_BINARY_INPUTS_D3;

  b = PORT_GetPortValue( &PORTF );		// aux inputs are in port F.
  if ( BIOS_AUX1_bm & b)
	  sw |= BIOS_BINARY_INPUTS_AUX1;
  if ( BIOS_AUX2_bm & b)
	  sw |= BIOS_BINARY_INPUTS_AUX2;

  return sw;
}// end bios_scan_binary_switch_inputs()

/* normal function
BYTE bios_scan_binary_switch_inputs( void )
{
  BYTE b;
  BYTE sw;								// switch pattern.
  sw = 0;

  b = PORT_GetPortValue( &VPORT1 );		// b = pins inputs from port J.
  if ( BIOS_D4F_bm & b )
	  sw = BIOS_BINARY_INPUTS_D4;

  b = PORT_GetPortValue( &PORTA );		// b = pins inputs from port A.
  if ( BIOS_PIN3_bm & b)
	  sw |= BIOS_BINARY_INPUTS_D1;
  if ( BIOS_PIN4_bm & b)
	  sw |= BIOS_BINARY_INPUTS_D2;
  if ( BIOS_PIN1_bm & b)
	  sw |= BIOS_BINARY_INPUTS_D3;

  return sw;
}// end bios_scan_binary_switch_inputs()
*/
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-09-27 -WFC-
BYTE bios_scan_binary_switch_inputs( void )
{
  BYTE b;
  BYTE sw;								// switch pattern.
  sw = 0;

  b = PORT_GetPortValue( &VPORT1 );		// b = pins inputs from port J.
  if ( BIOS_D4F_bm & b )
	  sw = BIOS_BINARY_INPUTS_D4;

  b = PORT_GetPortValue( &PORTA );		// b = pins inputs from port A.
  if ( BIOS_PIN3_bm & b)
	  sw |= BIOS_BINARY_INPUTS_D1;
  if ( BIOS_PIN4_bm & b)
	  sw |= BIOS_BINARY_INPUTS_D2;
  if ( BIOS_PIN1_bm & b)
	  sw |= BIOS_BINARY_INPUTS_D3;

  return sw;
}// end bios_scan_binary_switch_inputs()

#else
BYTE bios_scan_binary_switch_inputs( void )
{
  return 0;
}// end bios_scan_binary_switch_inputs()

#endif


/**
 * This tasks manage de-bouncing scanning of inputs.
 *
 * You can add more scanning functions here.
 *
 * History:  Created on 2010/05/25 by Wai Fai Chin
 * 2010-10-20 -WFC- log edge detection for switch logging output to a host.
 */

void bios_scan_inputs_tasks( void )
{
	BYTE i;
	BYTE bMask;

	if ( BIOS_RUN_STATUS_DEBOUNCE_SCAN & gbBiosRunStatus) {
		gbBiosBinarySwitchesPrvDebounce &= bios_scan_binary_switch_inputs();
		gbBiosBinarySwitches = gbBiosBinarySwitchesPrvDebounce;
		gbBiosRunStatus &= ~ BIOS_RUN_STATUS_DEBOUNCE_SCAN;					// reset debounce state.
		gbBiosBinarySwitchesFallingEdge = gbPrvBiosBinarySwitches & ( gbPrvBiosBinarySwitches ^ gbBiosBinarySwitches);
		gbPrvBiosBinarySwitches = gbBiosBinarySwitches;
		bMask = 1;
		for ( i=0; i < 6; i++){						// for D1, D2, D3, D4, AUX1, AUX2,
			if ( !(bMask & gbBiosNotOKupateSwLog) ) {	// if it is OK to update switch log
				if ( bMask & gbBiosBinarySwitchesFallingEdge ) {
					if ( bMask < 3 ) { // D1 and D2 are normal 0, so log it as 1.
						gbBiosBinarySwitchesLog |= bMask;
					}
					else {
						gbBiosBinarySwitchesLog &= ~bMask;
					}
					gbBiosNotOKupateSwLog |= bMask;			// flag this switch log cannot be update until printed out to a host.
				}
				else { // log level state of a switch
					gbBiosBinarySwitchesLog &= ~bMask;
					if ( bMask & gbBiosBinarySwitches )
						gbBiosBinarySwitchesLog |= bMask;
				}
			} // end  OK to update switch log.
			bMask <<= 1;
		} // end for()
	}
	else {
		gbBiosBinarySwitchesPrvDebounce = bios_scan_binary_switch_inputs();
		gbBiosRunStatus |= BIOS_RUN_STATUS_DEBOUNCE_SCAN;					// set debounce state for next reading.
	}
}// end bios_scan_inputs_tasks()

/*
void bios_scan_inputs_tasks( void )
{
	if ( timer_mSec_expired( &gDebounceTimer ) ) {
		if ( BIOS_RUN_STATUS_DEBOUNCE_SCAN & gbBiosRunStatus) {
			gbBiosBinarySwitchesPrvDebounce &= bios_scan_binary_switch_inputs();
			gbBiosBinarySwitches = gbBiosBinarySwitchesPrvDebounce;
			gbBiosRunStatus &= ~ BIOS_RUN_STATUS_DEBOUNCE_SCAN;					// reset debounce state.
			gbBiosBinarySwitchesFallingEdge = gbPrvBiosBinarySwitches & ( gbPrvBiosBinarySwitches ^ gbBiosBinarySwitches);
			gbPrvBiosBinarySwitches = gbBiosBinarySwitches;
		}
		else {
			gbBiosBinarySwitchesPrvDebounce = bios_scan_binary_switch_inputs();
			gbBiosRunStatus |= BIOS_RUN_STATUS_DEBOUNCE_SCAN;					// set debounce state for next reading.
		}
		timer_mSec_set( &gDebounceTimer, TT_100mSEC);
	}
}// end bios_scan_inputs_tasks()
*/

/**
 * This bios scan inputs to initialize debounced pre state and current states of switches.
 *
 * History:  Created on 2010/06/28 by Wai Fai Chin
 */

void bios_scan_inputs_init( void )
{
	gbBiosBinarySwitchesPrvDebounce = bios_scan_binary_switch_inputs();
	timer_mSec_set( &gDebounceTimer, TT_100mSEC);

	for ( ;;) {
		if ( timer_mSec_expired( &gDebounceTimer ) ) {
			gbBiosBinarySwitchesPrvDebounce &= bios_scan_binary_switch_inputs();
			gbPrvBiosBinarySwitches = gbBiosBinarySwitches = gbBiosBinarySwitchesPrvDebounce;
			break;
		}
	}
	gbBiosNotOKupateSwLog = 0;
	timer_mSec_set( &gDebounceTimer, TT_100mSEC);
}// end bios_scan_inputs_init()



//
// #if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-09-27 -WFC-

/**
 * Debounce power key before power down system and put CPU in power down sleep mode.
 *
 * History:  Created on 2011-12-14 by Wai Fai Chin
 */

void bios_power_off_by_power_key( void )
{
	soft_delay( 300 );			// delay 50 milliseconds, So key can stop bouncing.  10 may be enough
	while (!(BIOS_PIN3_bm & PORT_GetPortValue( &PORTB )));	//Wait for power key to be released
	soft_delay( 100 );			// delay 50 milliseconds, So key can stop bouncing.  10 may be enough
	bios_power_off();
}

/**
 * Power down system and put CPU in power down sleep mode.
 *
 * History:  Created on 2011-12-14 by Wai Fai Chin
 */

// void bios_power_off_timer ( void )
void bios_power_off_by_shutdown_event( void )
{
	gbPanelMainRunMode = PANEL_RUN_MODE_POWER_OFF;
	led_display_power_off();
	soft_delay( 50 );		// delay 50 milliseconds, so the LED driver chip has time to react to initialization commands before the power supply cut off the power to it..
//	TURN_OFF_RS232_CHIP;	// this also turn off LED driver chip power supply too.
	bios_power_off();
}

/**
 * Power down system and put CPU in power down sleep mode.
 *
 * History:  Created on 2011-12-14 by Wai Fai Chin
 * 2012-04-23 -WFC- Called rf_config_thread_runner();
 * 2013-10-18 -WFC- Power key is in portB instead of portA.
 * 2013-11-08 -WFC- improved wait for power key released. 2013-11-13 -DLM-
 */

// WORD16 gTestDelay;
void bios_power_off( void )
{
	DISABLE_GLOBAL_INTERRUPT;	//	cli();  test with ints off
//	UCSR1B &= ~(0X18);			// turn off SPI pin overrides
//	SPCR &= ~(0X40);
//	UCSR0B &= ~(0X18);			// Turn off USART pin overrides
	bios_cnfg_io_ports_for_power_off();	// Set ports to low power state
	// bios_configure_interrupt_for_power_key();	// configure and enabled interrupt for power key.
	soft_delay( 50 );			// delay milliseconds, So key can stop bouncing.
	// while (!(BIOS_PIN3_bm & PORT_GetPortValue( &PORTB )));	//Wait for power key to be released
	bios_configure_interrupt_for_power_key();	// configure and enabled interrupt for power key.

	bios_sleep();				// Will return here when sleep ends

	bios_system_init_clocks();

	gbBiosRunStatus &= ~(BIOS_RUN_STATUS_PEND_POWER_OFF | BIOS_RUN_STATUS_POWER_OFF);
	bios_io_ports_init();			// must initialized ports before switching system clock source. Otherwise, it would not work.
	dac_cpu_init_hardware();
	bios_extern_interrupts_init();
	timer_task_init();
	spi1_master_init();
	spi2_master_init();
	#if ( !(CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI) )
	i2c_init();
	#endif

	soft_delay( 50 );			// delay 50 milliseconds, So key can stop bouncing.  10 may be enough
	// while (!(BIOS_PIN3_bm & PORT_GetPortValue( &PORTB )));	//Wait for power key to be released
	while (!(BIOS_PIN3_bm & PORT_GetPortValue( &PORTB )) &&
			(BIOS_PIN1_bm & PORT_GetPortValue( &PORTB )) &&
			(BIOS_PIN5_bm & PORT_GetPortValue( &PORTB )) );	//Wait for power key to be released
	soft_delay( 50 );			// delay 50 milliseconds, So key can stop bouncing.  10 may be enough
	main_system_init();
	ENABLE_GLOBAL_INTERRUPT
	#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-23 -WFC-
		rf_config_thread_runner();
	#endif

	panel_main_power_up();
	if ( ! (BIOS_PIN1_bm & PORT_GetPortValue( &PORTB )) )					//  If user pressed F1 key
	{													// enter setup menu if user pressed on power and F1 keys.
		gbPanelMainRunMode = PANEL_RUN_MODE_PANEL_SETUP_MENU;
					// construct gPanelSetupTopMenuObj into a thread.
		gPanelSetupTopMenuObj.msgID				=
		gPanelSetupTopMenuObj.msgDisplayTime	= 0;						// no msg to display at the beginning of the menu.
		gPanelSetupTopMenuObj.pMethod			= 0;						// no method.
		gPanelSetupTopMenuObj.pRootMenu			= gacPanel_Top_Menu_Cnfg;	// points to setup top menu.
		gPanelSetupTopMenuObj.maxNumOfItem		= PANEL_TOP_MENU_CNFG_MAX_ITEMS;
		gPanelSetupTopMenuObj.parentIs 			= PSTMC_PARENT_NONE;		// no group parent, it is just a list of top menu item.
		gPanelSetupTopMenuObj.disabledItemFlags = 0;						// enabled all menu items.
		PT_INIT( &gPanelSetupTopMenuObj.m_pt );								// init panel setup top menu thread.
	}
	else {
		gbCmdSysRunMode = gbSysRunMode = SYS_RUN_MODE_SELF_TEST;	//  force it to perform a one shot system wide self test.
	}

	gbPanelMainRunMode = PANEL_RUN_MODE_POWER_OFF;
} // end bios_power_off().

/**
 * It configured io pins of all ports for power off the device.
 *
 * @post   configured io pins of all ports to save the most power consumption.
 *
 * History:  Created on 2011-12-14 by Wai Fai Chin
 * 2012-06-08	-WFC- configure PA1, PA3 and PA3 pins as pull up inputs instead of Totempole for MSI8000.
 * 2012-11-12 -WFC- Configured PORTH to outputs except RTS0 and CTS0. This guarantee no random glitch on chip select lines which fixed the LED driver chip turned on all LEDs while CPU in sleep mode.
 * 2013-10-28 -WFC- ported to ScaleCore3.
 * 2016-03-31 -WFC- keep RF Device on if selected RF_DEVICE_STATUS_ALWAYS_ON_bm.
 *
 */


void bios_cnfg_io_ports_for_power_off( void )
{

	// PORTA is for ADCA and DAC
	ADCA.CTRLA = ADCA.CTRLA & (~ADC_ENABLE_bm); // disabled ADC of PORTA which turn PORTA into generic io port.

	PORT_ConfigurePins( &PORTA,
			BIOS_PIN1_bm | BIOS_PIN3_bm | BIOS_PIN4_bm,	// configure pin1, pin3 and pin4
	        false,							// no slew rate control
	        false,							// no inversion
	        // 2012-06-08 -WFC- PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_OPC_PULLUP_gc,				// Totempole pull up on inputs. 2012-06-08 -WFC-
	        PORT_ISC_BOTHEDGES_gc );

	// set all pins on port A as inputs for now. The CPU ADC module will take care the rest.
	PORT_SetDirection( &PORTA, 0);
	PORT_SetOutputValue( &PORTA, 0xFF );

	// Port B is an input port.
	PORT_ConfigurePins( &PORTB,
			0xFF,
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_PULLUP_gc,				// pull up
	        PORT_ISC_BOTHEDGES_gc );

	// set all pins on port B as inputs and set them to high except pin6.
	PORT_SetDirection( &PORTB, BIOS_PIN6_bm);
	PORT_SetOutputValue( &PORTB, 0xFF );

	// PortC:
	// pin0: Output == SDA
	// pin1: Output == SCL
	// pin2: Input	== RXD1
	// pin3: Output	== TXD1
	// pin4: Output == SS1
	// pin5: Output == MOSI1
	// pin6: Input  == MISO1
	// pin7: Output == SCK1

	// PORTC is for TWI, SPI1 and serial0 ports. pins: ss1 o,  NOTE: SPI1 is for P4 and P5 connector.  P4 connector Master port for both SPI and I2C.
	// 0==in, 1==out,  x==don't care. xxx100xx ==> 00010000 =0x10

	// disabled TWI
	TWIC.MASTER.CTRLA &= ~TWI_MASTER_ENABLE_bm;
	TWIC.MASTER.CTRLB &= ~TWI_MASTER_SMEN_bm;
	// disabled SPI on portC.
	SPIC.CTRL &= ~SPI_ENABLE_bm;
	// PORTC spi_ss, pin0, pin1 configured as Totempole.
	PORT_ConfigurePins( &PORTC,
			BIOS_SPI_SS_bm | BIOS_PIN0_bm | BIOS_PIN1_bm,	// configure spi_ss, pin0, pin1
	        false,							// no slew rate control
	        false,							// no inversion
	        PORT_OPC_TOTEM_gc,				// Totempole
	        PORT_ISC_BOTHEDGES_gc );

	USARTC0.CTRLB &= ~( USART_RXEN_bm | USART_TXEN_bm );	// disabled USARTC0 receiver and transmitter.
	PORT_SetDirection( &PORTC,  0x00 );		//set all pins as inputs.
	PORT_SetOutputValue( &PORTC, 0xFF );	// set them to high

	// PORTD is for RS232 serial0 port, SPI2 port. pin: ss2. NOTE: SPI2 is for P5 connector.
	// 0==in, 1==out,  x==don't care. xxx100xx ==> 00010000 =0x10
	// PortD:
	// pin0: Input	== ADBUSY
	// pin1: Output	== FRAMCS
	// pin2: Input	== RXD2
	// pin3: Output	== TXD2
	// pin4: Output	== A2DCS
	// pin5: Output	== MOSI3
	// pin6: Input	== MISO3
	// pin7: Output	== SCK3

	// disabled SPI on portD.
	SPID.CTRL &= ~SPI_ENABLE_bm;
	USARTD0.CTRLB &= ~( USART_RXEN_bm | USART_TXEN_bm );	// disabled USARTD0 receiver and transmitter.
	//PORT_SetDirection( &PORTD, BIOS_SPI_SS_bm );		// SPI init routine will take care of it.
	PORT_SetDirection( &PORTD, 0  );		//set all pins as inputs.
	PORT_SetOutputValue( &PORTD, 0xFF );	// set them to high

	// 2016-03-31 -WFC- v
	if ( RF_DEVICE_STATUS_ALWAYS_ON_bm & gRfDeviceSettingsFNV.status ) {

		// PORTE is for RS232 serial1 and serial1 ports, control lines for RF modem or XBee etc..
		// pin0: Input	== CTS0
		// pin1: Output	== RTS0
		// pin2: Input	== RXD0
		// pin3: Output	== TXD0
		// pin4: Output	== DTR0 , RF_SLEEP.
		// pin5: Input	== DCD0
		// pin6: Output	== SPICS4
		// pin7: Input	== SPINT

		(&PORTE)->OUT &= ~( BIOS_RF_SLEEP_bm | BIOS_RTS0_bm);	// Keep XBee on
		USARTE0.CTRLB &= ~( USART_RXEN_bm | USART_TXEN_bm );	// disabled USARTE0 receiver and transmitter.
		// PORTE is for RS232 serial1, port, pins: CFG o, DTR o, DCD0 i, RFreset o
		PORT_SetDirection( &PORTE,  0x12 );		//set these pins as inputs, excepted BIOS_RF_SLEEP_bm, BIOS_RTS0_bm
		PORT_SetOutputValue( &PORTE, 0xED );	// set them to high excepted BIOS_RF_SLEEP_bm, BIOS_RTS0_bm

		// PORTF:
		// pin0: Output	== ON       high == on
		// pin1: Output	== VAON     high == on
		// pin2: Output	== P1ON     high == on
		// pin3: Output	== EXCON    high == on
		// pin4: Output	== RFRESET  high == reset
		// pin5: Output	== CFG      high == config
		// pin6: Output	== AUX1     default to low output
		// pin7: Input	== AUX2

		PORT_SetDirection( &PORTF,  0x7F );		//set these pins as outputs
		PORT_SetOutputValue( &PORTF, 0x90 );	// set all outputs to low, excepted BIOS_RF_RESET_bm.
	}
	else {
		// PORTE is for RS232 serial1 and serial1 ports, control lines for RF modem or XBee etc..
		// pin0: Input	== CTS0
		// pin1: Output	== RTS0
		// pin2: Input	== RXD0
		// pin3: Output	== TXD0
		// pin4: Output	== DTR0 , RF_SLEEP.
		// pin5: Input	== DCD0
		// pin6: Output	== SPICS4
		// pin7: Input	== SPINT

		(&PORTE)->OUT &= ~(BIOS_RF_SLEEP_bm);	// set RF SLEEP pin low to set RF module in sleep mode.
		USARTE0.CTRLB &= ~( USART_RXEN_bm | USART_TXEN_bm );	// disabled USARTE0 receiver and transmitter.
		// PORTE is for RS232 serial1, port, pins: CFG o, DTR o, DCD0 i, RFreset o
		PORT_SetDirection( &PORTE,  0 );		//set these pins as inputs
		PORT_SetDirection( &PORTE, BIOS_PIN1_bm | BIOS_PIN4_bm  );
		PORT_SetOutputValue( &PORTE, 0xFF );	// set them to high

		// PORTF:
		// pin0: Output	== ON       high == on
		// pin1: Output	== VAON     high == on
		// pin2: Output	== P1ON     high == on
		// pin3: Output	== EXCON    high == on
		// pin4: Output	== RFRESET  high == reset
		// pin5: Output	== CFG      high == config
		// pin6: Output	== AUX1     default to low output
		// pin7: Input	== AUX2

		PORT_SetDirection( &PORTF,  0x7F );		//set these pins as outputs
		//	PORT_SetOutputValue( &PORTF, 0x80 );	// set all outputs to low
		#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )
			PORT_SetOutputValue( &PORTF, 0x90 );	// set all outputs to low
		#else
			PORT_SetOutputValue( &PORTF, 0x80 );	// set all outputs to low
		#endif

	}
	// 2016-03-31 -WFC- ^

	// PORTR:
	// pin0: Output	== SPICS2
	// pin1: Output	== SPICS3
	PORT_SetDirection( &PORTR,  0x03 );		//set these pins as outputs.
	PORT_SetOutputValue( &PORTR, 0x03 );	// set them to high

} // end bios_cnfg_io_ports_for_power_off()


/**
 * Configured and enabled interrupt for power key.
 *
 * History: Created on  2011-12-14  by Wai Fai Chin
 * 2013-06-13 -WFC- ported to ScaleCore3.
 */

void bios_configure_interrupt_for_power_key( void )
{
	// Configure Interrupt0 to have low priority interrupt level, triggered by pin 3 of PortB where Power Key is connected. Its corresponding ISR is ISR(PORTB_INT0_vect);
	// PORT_ConfigureInterrupt0( &PORTA, PORT_INT0LVL_LO_gc, BIOS_PIN3_bm );
	PORTB.INTCTRL = ( PORTB.INTCTRL & ~PORT_INT0LVL_gm ) | PORT_INT0LVL_LO_gc;
	PORTB.INT0MASK = BIOS_PIN3_bm;

	// Enable medium level interrupts in the PMIC. NOTE that interrupt level must matched by the PORT_ConfigureInterrupt0(); otherwise it would not work.
	PMIC.CTRL |= PMIC_LOLVLEN_bm;
	// clear pending interrupt for both int0 and int1 by writing 1's to INTFLAGS.
	PORTB.INTFLAGS = PORT_INT1IF_bm | PORT_INT0IF_bm;
} //end bios_configure_interrupt_for_power_key()


/* *
 * Configured and enabled interrupt for power key.
 *
 * History: Created on  2011-12-14  by Wai Fai Chin
 *
 * /
 moved to bios.h as macro defined.
void bios_disabled_interrupt_for_power_key( void )
{
	// disabled all io pins of PORTA as interrupt source pins.
	PORTA.INT0MASK =
	PORTA.INT1MASK = 0;
} //end bios_configure_interrupt_for_power_key()
*/

/**
 * External Interrupts for Power key.
 *
 * History: Created on  2011-12-16  by Wai Fai Chin
 * 2013-06-13 -WFC- ported to ScaleCore3.
 *
 */
ISR(PORTB_INT0_vect)
{
	DISABLE_GLOBAL_INTERRUPT
	bios_disabled_interrupt_for_power_key();
	// clear pending interrupt for both int0 and int1 by writing 1's to INTFLAGS.
	PORTB.INTFLAGS = PORT_INT1IF_bm | PORT_INT0IF_bm;
}


//#endif // ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-09-27 -WFC-

/**
 * Put the entire system to sleep in power down mode.
 * NOTE: assumed callerd had already disabled global interrupts.
 *
 * History: Created on  2011-12-15  by Wai Fai Chin
 *
 */

void bios_sleep( void )
{
	// ENTER_CRITICAL_REGION();
	// bios_configure_interrupt_for_power_key();
	//TODO: TURN OFF JTAG here

	// set sleep mode in powerdown mode and enabled sleep mode.
	// SLEEP_CTRL = (SLEEP_CTRL & ~SLEEP_SMODE_gm) | (SLEEP_SMODE_t.SLEEP_SMODE_PDOWN_gc) | SLEEP_SEN_bm;
	SLEEP_CTRL = (SLEEP_CTRL & ~SLEEP_SMODE_gm) | SLEEP_SMODE_PDOWN_gc | SLEEP_SEN_bm;
	ENABLE_GLOBAL_INTERRUPT
	asm("sleep");

	SLEEP_CTRL &= ~SLEEP_SEN_bm; 	// disabled sleep mode.
	//TODO: TURN On JTAG here

//	PRR0 = 0x00;		// Enable clocks to all on board peripherals
//	PRR1 = 0x00;
} // end bios_sleep()


/**
 * Power down system and put CPU in power down sleep mode for a fixed amount of time then wakeup.
 *
 * History:  Created on 2014-09-17 by Wai Fai Chin
 */

/*
// WORD16 gTestDelay;
void bios_power_off_and_auto_wakeup( void )
{
	gbPanelMainRunMode = PANEL_RUN_MODE_POWER_OFF;
	led_display_power_off();
	soft_delay( 50 );		// delay 50 milliseconds, so the LED driver chip has time to react to initialization commands before the power supply cut off the power to it..

	DISABLE_GLOBAL_INTERRUPT;	//	cli();  test with ints off
	bios_cnfg_io_ports_for_power_off();	// Set ports to low power state
	bios_configure_interrupt_for_power_key();	// configure and enabled interrupt for power key.

	bios_sleep();				// Will return here when sleep ends

	bios_system_init_clocks();

	gbBiosRunStatus &= ~(BIOS_RUN_STATUS_PEND_POWER_OFF | BIOS_RUN_STATUS_POWER_OFF);
	bios_io_ports_init();			// must initialized ports before switching system clock source. Otherwise, it would not work.
	dac_cpu_init_hardware();
	bios_extern_interrupts_init();
	timer_task_init();
	spi1_master_init();
	spi2_master_init();
	#if ( !(CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI) )
	i2c_init();
	#endif

//	main_system_init();
	ENABLE_GLOBAL_INTERRUPT
//	#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-23 -WFC-
//		rf_config_thread_runner();
//	#endif

//	panel_main_power_up();

	gbPanelMainRunMode = PANEL_RUN_MODE_POWER_OFF;
} // end bios_power_off_and_auto_wakeup().
*/

extern TIMER_T	gSysPowerSaveTimer;		// 2014-09-09 -WFC- system power saving timer for use to detect delta weight.
extern BYTE gbLoadcellWeightChanged;

/**
 * Power down system and put CPU in power down sleep mode for a fixed amount of time then wakeup.
 *
 * History:  Created on 2014-09-17 by Wai Fai Chin
 */

/*
// WORD16 gTestDelay;
void bios_power_off_and_auto_wakeup( void )
{
	gbPanelMainRunMode = PANEL_RUN_MODE_POWER_OFF;
	led_display_power_off();
	soft_delay( 50 );		// delay 50 milliseconds, so the LED driver chip has time to react to initialization commands before the power supply cut off the power to it..

	DISABLE_GLOBAL_INTERRUPT;	//	cli();  test with ints off
	bios_cnfg_io_ports_for_power_off();	// Set ports to low power state
	bios_configure_interrupt_for_power_key();	// configure and enabled interrupt for power key.

	bios_sleep();				// Will return here when sleep ends

	bios_system_init_clocks();

	gbBiosRunStatus &= ~(BIOS_RUN_STATUS_PEND_POWER_OFF | BIOS_RUN_STATUS_POWER_OFF);
	bios_io_ports_init();			// must initialized ports before switching system clock source. Otherwise, it would not work.
	dac_cpu_init_hardware();
	bios_extern_interrupts_init();
	timer_task_init();
	spi1_master_init();
	spi2_master_init();
	#if ( !(CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI) )
	i2c_init();
	#endif

//	main_system_init();
	ENABLE_GLOBAL_INTERRUPT
//	#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-23 -WFC-
//		rf_config_thread_runner();
//	#endif

//	panel_main_power_up();

	gbPanelMainRunMode = PANEL_RUN_MODE_POWER_OFF;
} // end bios_power_off_and_auto_wakeup().
*/

extern TIMER_T	gSysPowerSaveTimer;		// 2014-09-09 -WFC- system power saving timer for use to detect delta weight.
extern BYTE gbLoadcellWeightChanged;

void bios_power_off_and_auto_wakeup( void )
{
	gbPanelMainRunMode = PANEL_RUN_MODE_POWER_OFF;
	led_display_power_off();
	soft_delay( 50 );		// delay 50 milliseconds, so the LED driver chip has time to react to initialization commands before the power supply cut off the power to it..

	DISABLE_GLOBAL_INTERRUPT;	//	cli();  test with ints off
	bios_cnfg_io_ports_for_power_off();	// Set ports to low power state
	bios_configure_interrupt_for_power_key();	// configure and enabled interrupt for power key.
	bios_sleep_idle();				// Will return here when sleep ends
//	soft_delay( 1000 );    // simulated sleep cycle.

	gbBiosRunStatus &= ~(BIOS_RUN_STATUS_PEND_POWER_OFF | BIOS_RUN_STATUS_POWER_OFF);
	bios_io_ports_init();			// must initialized ports before switching system clock source. Otherwise, it would not work.
	bios_extern_interrupts_init();
	spi1_master_init();
	spi2_master_init();
	#if ( !(CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI) )
	i2c_init();
	#endif

	ENABLE_GLOBAL_INTERRUPT

	timer_mSec_set( &gSysPowerSaveTimer, TT_5SEC);
	gbLoadcellWeightChanged = FALSE;

	gbPanelMainRunMode = PANEL_RUN_MODE_POWER_OFF;
} // end bios_power_off_and_auto_wakeup().

/**
 * Put the entire system to sleep in idle mode.
 * NOTE: assumed caller had already disabled global interrupts.
 *
 * History: Created on  2014-09-26  by Wai Fai Chin
 *
 */

extern BYTE	gbSleepCycleCnt;
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
void bios_sleep_idle( void )
{
	serial0_disabled_rx_isr();
	serial0_disabled_tx_isr();
	serial1_disabled_rx_isr();
	serial1_disabled_tx_isr();

	// configure timer0
	TCC0.CTRLB = 0;		// disable counting.
	// Select clock source.
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV1024_gc;
	TCC0.CNT = 1;
	TC_SetPeriod( &TCC0, 9766);		// for counting up 5 seconds
	timer_task_init();

	// set sleep mode in idle mode and enabled sleep mode.
	SLEEP_CTRL = (SLEEP_CTRL & ~SLEEP_SMODE_gm) | SLEEP_SMODE_IDLE_gc | SLEEP_SEN_bm;
	ENABLE_GLOBAL_INTERRUPT
	asm("sleep");
	SLEEP_CTRL &= ~SLEEP_SEN_bm; 	// disabled sleep mode.

	TCC0.CTRLB = 0;		// disable counting.
	TCC0.CNT = 1;
	TC_SetPeriod( &TCC0, 12500);		// for counting up
	//  Select clock source.
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV8_gc;
	timer_task_init();
	gbSleepCycleCnt++;
} // end bios_sleep_idle()
#else
void bios_sleep_idle( void )
{
	serial0_disabled_rx_isr();
	serial0_disabled_tx_isr();
	serial1_disabled_rx_isr();
	serial1_disabled_tx_isr();

	CLKSYS_Enable( OSC_RC32MEN_bm );
	CLKSYS_Prescalers_Config( CLK_PSADIV_1_gc, CLK_PSBCDIV_1_1_gc );		//The output cpu and peripheral clocks are divided by 1 of the original source.
	do {} while ( CLKSYS_IsReady( OSC_RC32MRDY_bm ) == 0 );
	CLKSYS_Main_ClockSource_Select( CLK_SCLKSEL_RC2M_gc );					// selected 2MHz clock source.

	// configure timer0
	TCC0.CTRLB = 0;		// disable counting.
	// Select clock source.
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV1024_gc;
	TCC0.CNT = 1;
	TC_SetPeriod( &TCC0, 9766);		// for counting up 5 seconds
	TCC0.CTRLB = TC0_CCAEN_bm | TC_WGMODE_SS_gc;
	TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;

	// set sleep mode in idle mode and enabled sleep mode.
	SLEEP_CTRL = (SLEEP_CTRL & ~SLEEP_SMODE_gm) | SLEEP_SMODE_IDLE_gc | SLEEP_SEN_bm;
	ENABLE_GLOBAL_INTERRUPT
	asm("sleep");
	SLEEP_CTRL &= ~SLEEP_SEN_bm; 	// disabled sleep mode.

//TODO: if power key was pressed, get it out from power saving mode and use normal clock speed.

	TCC0.CTRLB = 0;		// disable counting.
	TCC0.CNT = 1;
	TC_SetPeriod( &TCC0, 12500);		// for counting up
	//  Select clock source.
	TCC0.CTRLA = ( TCC0.CTRLA & ~TC0_CLKSEL_gm ) | TC_CLKSEL_DIV8_gc;
	TCC0.CTRLB = TC0_CCAEN_bm | TC_WGMODE_SS_gc;
	TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc;
	gbSleepCycleCnt++;
} // end bios_sleep_idle()

#endif

/**
 * If power saving mode is enabled, force to enabled serial ports.
 *
 * @return none.
 *
 * History:  Created on 2014-11-05 by Wai Fai Chin
 * 2015-01-14 -WFC- modified for Challenger3
 */

void	bios_enabled_comports_in_power_save_state( void )
{
	if ( SYS_POWER_SAVE_STATE_ACTIVE == gbSysPowerSaveState ) {
		TURN_ON_RS232_CHIP;		// it also turn on LED power supply too.
		gbLoadcellWeightChanged = FALSE;
		gbSleepCycleCnt = 0;
		gbSysPowerSaveState = SYS_POWER_SAVE_STATE_INACTIVE;
		bios_clock_normal();			// 2015-01-14 -WFC-
		serial0_port_init( SR_BAUD_38400_V );
		serial1_port_init( SR_BAUD_38400_V );
		BIOS_ENABLE_AC_EXCITATION;		// turn on excitation

		#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )
		BIOS_TURN_ON_XBEE;
		#endif
	}
} // end bios_enabled_comports_in_power_save_state()

