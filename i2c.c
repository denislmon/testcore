/*! \file i2c.c \brief Serial Peripheral Interface implementation.*/
// ****************************************************************************
//
//                      MSI CellScale
//
//                  Copyright (c) 2011 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: i2c.c
// Hardware: ATMEL AVR Xmga series
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2011-09-20 by Wai Fai Chin
//
//   Two Wire Interface (TWI) related functions.
// 
// 
// ****************************************************************************


#include "i2c.h"
#include "bios.h"

void i2c_twi_init(TWI_t *pTWI, BYTE baud );

/**
 * Configure TWI as Master I2C port.
 * Description:
 *  Configure TWI as a Master I2C port to interface with the NXP LCD display driver chip.
 *  The I2C mode is clock low when inactive, data sample when high if data changes when
 *  high it is a START or STOP flag data is eight bits with MSB first followed by one 
 *  acknowledge bit.  Clock is hardcoded 200KHz.
 * 
 * @return none
 *
 * @note
 *  TWI baud = Fsys /(2*Ftwi) - 5;
 * 
 *  History:  Created on 2011-09-20 by Wai Fai Chin
 */

void i2c_init( void )
{
	// Note: need to adjust baudrate when changed system clock speed.
	// i2c_twi_init(&TWIC, 155 );		// 100 KHz
	i2c_twi_init(&TWIC, 75 );			// 200 KHz
} // end I2C_init()

/**
 * Configure TWI as Master I2C port.
 * Description:
 *  Configure TWI as a Master I2C port to interface with the NXP LCD display driver chip.
 *  The I2C mode is clock low when inactive, data sample when high if data changes when
 *  high it is a START or STOP flag data is eight bits with MSB first followed by one
 *  acknowage bit.
 *
 * @return none
 *
 * @note
 *  TWI baud = Fsys /(2*Ftwi) - 5;
 *  History:  Created on 2011-09-20 by Wai Fai Chin
 */

void i2c_twi_init(TWI_t *pTWI, BYTE baud )
{
	pTWI-> MASTER.CTRLB = TWI_MASTER_SMEN_bm;
	pTWI-> MASTER.BAUD = baud;
	pTWI-> MASTER.CTRLA = TWI_MASTER_ENABLE_bm;
	pTWI-> MASTER.STATUS = TWI_MASTER_BUSSTATE_IDLE_gc;
	return;
}

/**
 *  i2c polling mode transmitter
 *
 *  It takes at least 25 mS to write one byte with the I2C at 345.600 b/s. CPU attension is required
 *  after every byte but the port can wait.  Thus, we decided to just use
 *  polling mode for the I2C for xmitting.  I2C interrupt mode would be more complex and
 *  overhead excuction and memory size.
 *
 * @param  data -- data to xmit.
 *
 * History:  Created on 2007/09/14 by Wai Fai Chin
 * 2011-09-27 -WFC- Renamed i2c_transmitt() to i2c_write_lcd() and created a version for ScaleCore2 AtXmega128A1 CPU.
 */

BYTE i2c_write_lcd( BYTE *pbSrc, BYTE dataLen )
{
	BYTE i;
	if ( dataLen ) {
		// TWIC.MASTER.ADDR = LCD_PCF8562_ADDRESS;
		TWIC.MASTER.ADDR = 0x70;
		while(!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm));
		for(i=0; i< dataLen; i++){
		   TWIC.MASTER.DATA = pbSrc[i];
	      while(!(TWIC.MASTER.STATUS & TWI_MASTER_WIF_bm));
	    }
	}
	return 1;
} // end i2c_write_lcd()

