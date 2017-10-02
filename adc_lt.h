/*! \file adc_lt.h \brief external ADC related functions.*/
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
//  History:  Created on 2007/09/10 by Wai Fai Chin
// 
/// \ingroup driver_cpu_extern
/// \defgroup ADC_LT manage external Linear Tech ADC chip.(adc_lt.c)
/// \code #include "adc_lt.h" \endcode
/// \par Overview
///   This is a drive for external ADC chip. It configures its input selection,
/// conversion speed and sensor filter algorithm based on the sensor descriptor
/// from sensor module. It does not know what type of sensor hooked up to it.
/// It just know how to read and supply ADC data to the specified sensor object.
//
// ****************************************************************************
//@{
 

#ifndef MSI_ADC_LT_H
#define MSI_ADC_LT_H

#include	"config.h"
#if (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 )
#include	"port_driver.h"
#endif
#include	"sensor.h"
#include	"bios.h"

/*!
   We are use the Linear Tech LTC2447 delta sigma ADC chip.
   We use the Linear Tech LTC2447 delta sigma ADC chip.
   I found that polling on ADBUSY working in conjunction with a none delay main task loop is the best approached and it
   is the most reliabled method without corrupt the final ADC result during running the filter function.
   It only took 50~60uS to read four bytes of data with SPI at 1,843,200 Hz. Thus, I decided to just use
   polling mode for the UART_SPI on both xmitting and receiving. SPI interrupt mode would be more complex and
   overhead excuction and memory size.


    input command format:
	
	ctrl2 ctrl1 ctrl0 SGL SIGN GLBL A1 A0       OSR3 OSR2 OSR1 OSR0  TwoX x x x
	
	If control bits = 101, then following 5 bits select the input channel/reference
    for the following conversion. The next 5 bits select the speed/resolution and
    mode 1x (no Latency) 2x (double output rate with one conversion latency).
	
	If control bits = 100 or 000, the following input data is ignored (don�t care)
    and the previously selected speed/resolution, channel and reference remain
	valid for the next conversion.
	
	If SGL=0, input selection is differential.
	If SGL=1, input selection is single-ended.

    If GLBL=0 selects of channels has a corresponding differential input reference.
	If GLBL = 1, a global reference VREFG+/VREFG� is selected.
    The global reference input may be used for any input channel selected.
	
	(SIGN, A1, A0) determine which channel is selected.
	
	(OSR3, OSR2, OSR1, OSR0), (0000) Keep Previous conversion Speed/Resolution.
	(0001) is 3.52KHz with 23uV RMS noise. (1111) is 6.875Hz with 200nV RMS noise.
	
	When TwoX=1, it will double the above output rate with one conversion latency
    and with the same noise performance.
	
	Output format:
	Bit 31 (first output bit) is the end of conversion (/EOC) indicator.
		This bit is HIGH during the conversion and goes LOW when the conversion is complete.

	Bit 30 (second output bit) is a dummy bit (DMY) and is always LOW.
	Bit 29 (third output bit) is the conversion result sign indicator (SIG).
			If VIN is >0, this bit is HIGH. If VIN is <0, this bit is LOW.
	Bit 28 (fourth output bit) is the most significant bit (MSB) of the result.
		This bit in conjunction with Bit 29 also provides the underrange or overrange indication.
		If both Bit 29 and Bit 28 are HIGH, the differential input voltage is above +FS.
		If both Bit 29 and Bit 28 are LOW, the differential input voltage is below �FS.
	
	Bits 28-5 are the 24-bit conversion result MSB first.
	Bit 5 is the least significant bit (LSB).
	Bits 4-0 are sub LSBs below the 24-bit level.
    Bits 4-0 may be included in averaging or discarded without loss of resolution.
	
*/

/*!
  Linear Tech ADC enabled input command.
  To be use with logic OR operation.
*/

#define ADC_LT_CMD_ENABLED		0xA0


/*!
  Linear Tech ADC disabled input command.
  To be use with logic AND operation.
*/

#define ADC_LT_CMD_DISABLED		0x1F

/*!

	SGL SIGN GLBL A1 A0
	 0   1    1   0   0
	
	Thus the mask value is 0xFC for logic AND operation before
	OR with channel number, S.
	 
*/

#define ADC_LT_DIF_CHANNEL_MASK	0x03

#define ADC_LT_DIF_CHANNEL_0	0x00
#define ADC_LT_DIF_CHANNEL_1	0x01
#define ADC_LT_DIF_CHANNEL_2	0x02
#define ADC_LT_DIF_CHANNEL_3	0x03

#define ADC_LT_SINGLE_END_CHANNEL_0	0x00
#define ADC_LT_SINGLE_END_CHANNEL_1	0x08
#define ADC_LT_SINGLE_END_CHANNEL_2	0x01
#define ADC_LT_SINGLE_END_CHANNEL_3	0x09
#define ADC_LT_SINGLE_END_CHANNEL_4	0x02
#define ADC_LT_SINGLE_END_CHANNEL_5	0x0A
#define ADC_LT_SINGLE_END_CHANNEL_6	0x03
#define ADC_LT_SINGLE_END_CHANNEL_7	0x0B

/*!
  Linear Tech ADC reference selection.
  If GLBL=0 selects of channels has a corresponding differential input reference.
  If GLBL = 1, a global reference VREFG+/VREFG� is selected.
  Logic OR operation will set it. To clear it by first invert it then AND it.
*/

#define ADC_LT_GLOBAL_REF_V		0x04

#define ADC_LT_ODD_CHANNEL_BIT	0x08

#define ADC_LT_SINGLE_END		0x10

/*!
  Linear Tech ADC count sign bit in the most significant byte.
  If Vin is >0, this bit is HIGH. If Vin is <0, this bit is LOW.
  To check sign bit by use AND logic.
*/

#define ADC_LT_SIGN_BIT		0x20

/*!
  Linear Tech ADC  the most significant byte.
  If Vin is >0, this bit is HIGH. If Vin is <0, this bit is LOW.
  To strip the first 3bits by AND with the most significant byte.
*/

#define ADC_LT_STRIP_3MSBITS_CNT		0x1F


#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )

/*!
  AVR CPU IO PORT for Linear Tech ADC chip select.
*/
#define ADC_LT_CS_PORT	PORTD	

/*!
  AVR CPU IO PORT pin for Linear Tech ADC chip select.
  Usage:
  \code
	ADC_LT_CS_PORT &= ~( 1 << ADC_LT_CS);	// Chip select low to select the ADC chip.
	ADC_LT_CS_PORT |= ( 1 << ADC_LT_CS);	// Chip select high to deselect the ADC chip.
  \endcode
  
*/
#define ADC_LT_CS		PD4

/// set chip select LOW to select the ADC chip.
#define	ADC_LT_CS_SELECT	ADC_LT_CS_PORT &= ~( 1 << ADC_LT_CS)

/// set chip select HIGH to deselect the ADC chip.
#define	ADC_LT_CS_DESELECT	ADC_LT_CS_PORT |= ( 1 << ADC_LT_CS)

/*!
  \brief Maxium number of channel in the ADC chip
  \note Must be in power of 2 even if it does not use up all channels.
    
*/
#define ADC_LT_MAX_CHANNEL	8


/*!
  AVR CPU IO PORT for Linear Tech ADC chip control IO port.
*/
#define ADC_LT_CTRL_PORT_OUT	PORTE	

///  AVR CPU IO PORT pin for Linear Tech ADC AC EXCITATION ENABLE, output
#define ADC_LT_CTRL_ACEXCENB	PE2

/*!  AVR CPU IO PORT pin for Linear Tech ADC AC EXCITATION CLOCK, output
		ACEXCENB	EXCCLK		EXCITATION MODE
			X			1			ON +
			1			0			ON -
			0			0			OFF
*/
#define ADC_LT_CTRL_EXCCLK		PE3


///  AVR CPU IO PORT pin for Linear Tech ADC CTRL signal, input port.
#define ADC_LT_CTRL_PORT_IN		PINE

///  AVR CPU IO PORT pin for Linear Tech ADC busy signal, input.
#define ADC_LT_CTRL_ADBUSY		(1<<PINE4)


#define ADC_LT_TURN_OFF_AC_EXCITATION	ADC_LT_CTRL_PORT_OUT &= ~((1 << ADC_LT_CTRL_ACEXCENB ) | ( 1 << ADC_LT_CTRL_EXCCLK ))
#define ADC_LT_ENABLE_AC_EXCITATION		ADC_LT_CTRL_PORT_OUT |=  ( 1 << ADC_LT_CTRL_ACEXCENB )
#define ADC_LT_AC_EXCITATION_POS		ADC_LT_CTRL_PORT_OUT |=  ( 1 << ADC_LT_CTRL_EXCCLK )
#define ADC_LT_AC_EXCITATION_NEG		ADC_LT_CTRL_PORT_OUT &= ~( 1 << ADC_LT_CTRL_EXCCLK )

#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 )
/*!
  AVR CPU IO PORT for Linear Tech ADC chip select.
*/

#define ADC_LT_CS_PORT_OUT_REG	(&VPORT0)->OUT

#define ADC_LT_CS		BIOS_PIN0_bm

/// set chip select LOW to select the ADC chip.
#define	ADC_LT_CS_SELECT	ADC_LT_CS_PORT_OUT_REG &= ~ADC_LT_CS

/// set chip select HIGH to deselect the ADC chip.
#define	ADC_LT_CS_DESELECT	ADC_LT_CS_PORT_OUT_REG |= ADC_LT_CS

/*!
  \brief Maximum number of channel in the ADC chip
  \note Must be in power of 2 even if it does not use up all channels.
*/
#define ADC_LT_MAX_CHANNEL	8

/*!
  AVR CPU IO PORT for Linear Tech ADC chip control IO port.
*/
#define ADC_LT_CTRL_PORT_OUT_REG	(&PORTF)->OUT

#define	BIOS_EXCITE_XCLK_bm			0x01
#define	BIOS_EXCITE_XCLKEN_bm		0x02


///  AVR CPU IO PORT pin for Linear Tech ADC AC EXCITATION ENABLE, output
#define ADC_LT_CTRL_ACEXCENB	BIOS_EXCITE_XCLKEN_bm

/*!  AVR CPU IO PORT pin for Linear Tech ADC AC EXCITATION CLOCK, output
		ACEXCENB	EXCCLK		EXCITATION MODE
			X			1			ON +
			1			0			ON -
			0			0			OFF
*/
#define ADC_LT_CTRL_EXCCLK		BIOS_EXCITE_XCLK_bm


///  AVR CPU IO PORT pin for Linear Tech ADC CTRL signal, input port.
#define ADC_LT_CTRL_PORT_IN		(&VPORT1)->IN

///  AVR CPU IO PORT pin for Linear Tech ADC busy signal, input.
#define ADC_LT_CTRL_ADBUSY		BIOS_ADBUSY_bm

///  Linear Tech ADC data is ready for read.
#define ADC_LT_READY	(!((ADC_LT_CTRL_PORT_IN) & ADC_LT_CTRL_ADBUSY))

#define ADC_LT_TURN_OFF_AC_EXCITATION	ADC_LT_CTRL_PORT_OUT_REG &= ~( ADC_LT_CTRL_ACEXCENB | ADC_LT_CTRL_EXCCLK )
#define ADC_LT_ENABLE_AC_EXCITATION		ADC_LT_CTRL_PORT_OUT_REG |=  ADC_LT_CTRL_ACEXCENB
#define ADC_LT_AC_EXCITATION_POS		ADC_LT_CTRL_PORT_OUT_REG |=  ADC_LT_CTRL_EXCCLK
#define ADC_LT_AC_EXCITATION_NEG		ADC_LT_CTRL_PORT_OUT_REG &= ~ADC_LT_CTRL_EXCCLK

#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )		// 2013-10-25 -WFC-

/*!
  AVR CPU IO PORT for Linear Tech ADC chip select.
*/

#define ADC_LT_CS_PORT_OUT_REG	(&PORTD)->OUT

#define ADC_LT_CS		BIOS_PIN4_bm

/// set chip select LOW to select the ADC chip.
#define	ADC_LT_CS_SELECT	ADC_LT_CS_PORT_OUT_REG &= ~ADC_LT_CS

/// set chip select HIGH to deselect the ADC chip.
#define	ADC_LT_CS_DESELECT	ADC_LT_CS_PORT_OUT_REG |= ADC_LT_CS

/*!
  \brief Maximum number of channel in the ADC chip
  \note Must be in power of 2 even if it does not use up all channels.
*/
#define ADC_LT_MAX_CHANNEL	8

/*!
  AVR CPU IO PORT for Linear Tech ADC chip control IO port.
*/
#define ADC_LT_CTRL_PORT_OUT_REG	(&PORTF)->OUT

///  AVR CPU IO PORT pin for Linear Tech ADC CTRL signal, input port.
#define ADC_LT_CTRL_PORT_IN		(&PORTD)->IN

///  AVR CPU IO PORT pin for Linear Tech ADC busy signal, input.
#define ADC_LT_CTRL_ADBUSY		BIOS_ADBUSY_bm

///  Linear Tech ADC data is ready for read.
#define ADC_LT_READY	(!((ADC_LT_CTRL_PORT_IN) & ADC_LT_CTRL_ADBUSY))

#define ADC_LT_TURN_OFF_AC_EXCITATION	ADC_LT_CTRL_PORT_OUT_REG &= ~( BIOS_EXCON_bm )
#define ADC_LT_ENABLE_AC_EXCITATION		ADC_LT_CTRL_PORT_OUT_REG |=  BIOS_EXCON_bm

#endif // ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 ) #elif ()

/*!
  \brief Linear Tech ADC operation descriptor.
  \note It must initialize based on sensor descriptor first before use it.
    If corresponding sensor descriptor changed, the corresponding channel operation record must changed also.
*/

typedef struct  ADC_LT_OP_DESC_TAG {
							/// sensor ID number, range 0 to (MAX_NUMBER_SENSOR - 1). 0xFF ==  no such sensor. 
	BYTE    sensorN;
							/// command0 for this chip. The command was configured based on the sensor descriptor and Linear Tech ADC command format from databook.
	BYTE	cmd0;
							/// conversion configuration such as speed,
	BYTE	cmd1;
							/// sensor status, Bit7 1==enabled; Bit6 AC excitation enabled, Bit5, excitation state 1==+, 0==-; Bit4-0 reserved
	BYTE	status;
}ADC_LT_OP_DESC_T;

// An array of ADC operation descriptor. should be a privat data of this module
// extern	ADC_LT_OP_DESC_T	gaAdcLT_op_desc[ ADC_LT_MAX_CHANNEL ];

void	adc_lt_1st_init( void );
void	adc_lt_init( void );
void  	adc_lt_read( BYTE curChannel, BYTE nextChannel );
INT32	adc_lt_read_count( BYTE nextChannelCmd, BYTE sampleSpeedCmd );
INT32	adc_lt_read_spi( BYTE cmd0, BYTE cmd1 );
void	adc_lt_update( void );
void	adc_lt_construct_op_desc( LSENSOR_DESCRIPTOR_T *pSD, BYTE sensorN );

#endif  // MSI_ADC_LT_H

//@}
