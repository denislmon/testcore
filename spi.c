/*! \file spi.c \brief Serial Peripheral Interface implementation.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: spi.h
// Hardware: ATMEL ATXMEGA192D3
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2013-04-18 by Wai Fai Chin
//
//
//   Serial Peripheral Interface related functions.
// 
// 
// ****************************************************************************


#include "spi.h"
#include "serial.h"


/// SPI1 master module on PORT C.
SPI_Master_t gSpi1MasterC;

/// SPI2 master module on PORT D.
// 2011-09-28 -WFC- SPI_Master_t gSpi2MasterC;

// SPI3 master module on PORT F.
// SPI_Master_t gSpi3MasterF;

/// SPI  module on PORT D.

#ifdef 		SLAVE_DISPLAY								//	PHJ	v
	SPI_Master_t gSpi2MasterD;	//	master
#else //	SLAVE_DISPLAY								
	SPI_Slave_t spiSlaveD;		//	slave 
#endif //	SLAVE_DISPLAY								//	PHJ	^	


///////////////////////////////////////////////////////////////////////////////
//                           			SPI1		                         //
///////////////////////////////////////////////////////////////////////////////

/**
 * Configure SPI on PORTC as SPI1 port in master mode at fck/4 clock speed.
 *
 * @return none
 * @note
 *
 * Important NOTE: in Master mode, /ss pin must configured as output even if you are not use it to control the slave,
 * otherwise it got hang on while(!SPSR&(1<<SPIF)); statement.
 * If /ss configure as input, it must be held high by a circuit to ensure MASTER SPI operation.
 *
 * fck/4 is the fastest guaranteed speed when it is in slave mode.
 * fck/8 is the fastest guaranteed speed for SPI EEPROM chip at 11.0592 MHz Xtal.
 *
 * History:  Created on 2013-04-18 by Wai Fai Chin
 */
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 ) // 2013-09-04 -DLM-
void spi1_master_init( void )
{
	// NOTE: Init SS pin as output with wired AND and pull-up.
	PORTC.DIRSET = PIN4_bm;
	PORTC.PIN4CTRL = PORT_OPC_WIREDANDPULL_gc;
	//Set SS output to high for default as no slave addressed.
	PORTC.OUTSET = PIN4_bm;

	//NOTE::: PORTF pin4 must configured as input for now until the new PCB. bios_io_ports_init() configured it as input.

	// Init SPI master on port C
	SPI_MasterInit( &gSpi1MasterC, &SPIC,  &PORTC,
				   true,								// LSB bit first // 2013-09-04 -DLM-
				   SPI_MODE_0_gc,
				   SPI_INTLVL_OFF_gc,					// disabled SPI1 interrupt
				   false,								// no double speed clock
				   SPI_PRESCALER_DIV64_gc );
				   //SPI_PRESCALER_DIV4_gc );

} // end spi1_master_init()

#else

void spi1_master_init( void )
{
	// NOTE: Init SS pin as output with wired AND and pull-up.
	PORTC.DIRSET = PIN4_bm;
	PORTC.PIN4CTRL = PORT_OPC_WIREDANDPULL_gc;
	//Set SS output to high for default as no slave addressed.
	PORTC.OUTSET = PIN4_bm;

	//NOTE::: PORTF pin4 must configured as input for now until the new PCB. bios_io_ports_init() configured it as input.

	// Init SPI master on port C
	SPI_MasterInit( &gSpi1MasterC, &SPIC,  &PORTC,
	               false,								// MSB bit first
	               SPI_MODE_0_gc,
	               SPI_INTLVL_OFF_gc,					// disabled SPI1 interrupt
	               false,								// no double speed clock
	               SPI_PRESCALER_DIV64_gc );
	               //SPI_PRESCALER_DIV4_gc );

} // end spi1_master_init()
#endif

/**
 *  SPI1 polling mode transceiver
 *
 * @param  bData -- data to xmit.
 *
 * @return received data.
 *
 * History:  Created on 2013-04-18 by Wai Fai Chin
 */

BYTE spi1_transceive( BYTE bData )
{
	BYTE receivedByte;
	// receivedByte= SPI_MasterTransceiveByte(&gSpi1MasterC, bData);
	// return receivedByte;
	gSpi1MasterC.module->DATA = bData;
	while(!(gSpi1MasterC.module->STATUS & SPI_IF_bm)){}		// Wait for transmission complete.
	receivedByte = gSpi1MasterC.module->DATA;				// Read received data.
	return receivedByte;
} // end spi1_transceive()


///////////////////////////////////////////////////////////////////////////////
//                           			SPI2		                         //
///////////////////////////////////////////////////////////////////////////////

/**
 * Configure SPI on PORTD as SPI2 port in master mode at fck/4 clock speed.
 *
 * @return none
 * @note
 *
 * Important NOTE: in Master mode, /ss pin must configured as output even if you are not use it to control the slave,
 * otherwise it got hang on while(!SPSR&(1<<SPIF)); statement.
 * If /ss configure as input, it must be held high by a circuit to ensure MASTER SPI operation.
 *
 * fck/4 is the fastest guaranteed speed when it is in slave mode.
 * fck/8 is the fastest guaranteed speed for SPI EEPROM chip at 11.0592 MHz Xtal.
 *
 * History:  Created on 2013-04-18 by Wai Fai Chin
 */

void spi2_master_init( void )
{
	// NOTE: Init SS pin as output with wired AND and pull-up.
	PORTD.DIRSET = PIN4_bm;
	PORTD.PIN4CTRL = PORT_OPC_WIREDANDPULL_gc;
	//Set SS output to high for default as no slave addressed.
	PORTD.OUTSET = PIN4_bm;

	// Init SPI master on port D
	SPI_MasterInit( &gSpi2MasterD, &SPID,  &PORTD,
	               false,								// MSB bit first
	               SPI_MODE_0_gc,
	               SPI_INTLVL_OFF_gc,					// disabled SPI2 interrupt
	               false,								// no double speed clock
	               SPI_PRESCALER_DIV16_gc );			// 2000KHz
	               // SPI_PRESCALER_DIV64_gc );			// 500KHz
	               // SPI_PRESCALER_DIV128_gc);			// 250KHz
	               // SPI_PRESCALER_DIV4_gc );

} // end spi2_master_init()

/**
 *  SPI2 polling mode transceiver
 *
 * @param  bData -- data to xmit.
 *
 * @return received data.
 *
 * History:  Created on 2013-04-18 by Wai Fai Chin
 */

BYTE spi2_transceive( BYTE bData )
{
	BYTE receivedByte;
	// receivedByte= SPI_MasterTransceiveByte(&gSpi1MasterC, bData);
	// return receivedByte;
	gSpi2MasterD.module->DATA = bData;
	while(!(gSpi2MasterD.module->STATUS & SPI_IF_bm)){}		// Wait for transmission complete.
	receivedByte = gSpi2MasterD.module->DATA;				// Read received data.
	return receivedByte;
} // end spi2_transceive()

