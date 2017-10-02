/*! \file serial.c \brief serial port uart implementation.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: serial.c
// Hardware: ATMEL ATXMEGA128A1
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2009/11/03 by Wai Fai Chin
//
//
//   Serial interrupt driven buffered related functions.
// 
// 
// ****************************************************************************


#include "serial.h"

#if defined( CONFIG_USE_DOXYGEN )
/*
 * Doxygen doesn't append attribute syntax of
 * GCC, and confuses the typedefs with function decls, so
 * supply a doxygen-friendly view.
 */

/**
	Uart baudrate lookup table.
 */
 
const WORD16 PROGMEM gwSerial0_BaudrateTbl[] = {
2303,
1151,
575,
287,
143,
71,
47,
35,
23,
17,
11,
8,
5,
2
};

#else // not DOXYGEN
// real codes are here:
#if ( CONFIG_XTAL_FREQUENCY  == XTAL_11P0592MHZ )

const WORD16  gwSerial0_BaudrateTbl[] PROGMEM= {
2303,
1151,
575,
287,
143,
71,
47,
35,
23,
17,
11,
8,
5,
2
};
#elif	( CONFIG_XTAL_FREQUENCY  == INTERNAL_32MHZ )
//TODO: need recompute the baudrate value.
const WORD16  gwSerial0_BaudrateTbl[] PROGMEM= {
1535,
767,
383,
191,
95,
47,
31,
23,
15,
11,
7,
5,
3,
1
};
#else  // default
const WORD16  gwSerial0_BaudrateTbl[] PROGMEM= {
1535,
767,
383,
191,
95,
47,
31,
23,
15,
11,
7,
5,
3,
1
};

#endif

#endif // DOXYGEN


/// use USARTE0 for USART0, C is in port C. In PCB term it is TXD1 and RXD1. It is for HOST computer, and boot loader.
#define gtUsart0 USARTC0
/// USART data struct that contains USART and data buffer.
USART_data_t 	gtUsart0_data;

/// use USARTE0 for USART1, E is in port E. In PCB term it is TXD0 and RXD0. It is for talk to XBEE, RF modem or DSC ( Digital Signal Conditioner).
#define gtUsart1 USARTE0
/// USART data struct that contains USART and data buffer.
USART_data_t 	gtUsart1_data;


/**************************************************************************
			Serial Port 0 I/O functions
**************************************************************************/

/**
 * Configure USARTE0 for Serial Port 0.
 *
 * @param  baud --  baudrate selection range from 1 to 14. 1 => 300 baud ... 14 => 230400 baud.
 * 
 * @return none
 *
 * \sa
 *
 * Description:
 *  Configure USARTD0 based on the specified baudrate selection.
 *
 * Example useage:
 * \code
 *   serial0_port_init( SR_BAUD_19200 );
 * \endcode
 * 2014-06-02 -WFC- modified for direct baudrate setting.
 */

// void serial0_port_init( BYTE baud )
void serial0_port_init( UINT16 baud )
{
  	// PC3 (TXD0) as output.
	PORTC.DIRSET   = PIN3_bm;
	// PC2 (RXD0) as input.
	PORTC.DIRCLR   = PIN2_bm;

	// Use USARTE0 and initialize buffers.
	USART_InterruptDriver_Initialize( &gtUsart0_data, &gtUsart0, USART_DREINTLVL_LO_gc);

	// 8 Data bits, No Parity, 2 Stop bit.
	USART_Format_Set( gtUsart0_data.usart, USART_CHSIZE_8BIT_gc,
                     USART_PMODE_DISABLED_gc, TRUE);

	// Enable RXC interrupt.
	USART_RxdInterruptLevel_Set( gtUsart0_data.usart, USART_RXCINTLVL_LO_gc);

	USART_Baudrate_Set( &gtUsart0, baud , 0 );

	// Enable both RX and TX.
	USART_Rx_Enable( gtUsart0_data.usart );
	USART_Tx_Enable( gtUsart0_data.usart );

	/* Enable PMIC interrupt level low. */
	PMIC.CTRL |= PMIC_LOLVLEX_bm;
} // end serial0_port_init()


/**
 *  It transmits a byte of data.
 *
 * @param  bData -- a byte of data to be send
 * 
 * @return none
 *
 * Dependencies:
 *   serial0_port_init(),
 *   serial0_buf_send_byte(),
 *   USART0_TX_vect(), USART0_RX_vect()
 *
 * History:  Created on 2006/10/27 by Wai Fai Chin
 */

void serial0_send_byte( BYTE bData )
{
	while( !USART_TXBuffer_FreeSpace( &gtUsart0_data ) );		// wait for available buf
	USART_TXBuffer_PutByte( &gtUsart0_data, bData );
   
}// end serial0_send_byte() 


/**
 *  It tries to transmit a byte of data without waiting.
 *  If it successes transmitted a byte, it returns true else return false.
 *
 * @param  bData -- a byte of data to be send
 * 
 * @return true if it successes transmitted a byte else false.
 *
 * Dependencies:
 *   serial0_port_init(),
 *   serial0_buf_send_byte(),
 *   USART0_TX_vect(), USART0_RX_vect()
 *
 * History:  Created on 2006/11/01 by Wai Fai Chin
 */

BYTE serial0_send_byte_no_wait( BYTE bData )
{
  BYTE status;
	status = FALSE;                    // assumed failed to send a byte.
	if ( USART_TXBuffer_FreeSpace( &gtUsart0_data ) )    {
		USART_TXBuffer_PutByte( &gtUsart0_data, bData );
		status = TRUE;
	}
	return status;
}// end serial0_send_byte_no_wait() 


/**
 *  It transmits n bytes of data point by the pbPtr.
 *
 * @param  pbData --   pointer to data to be send.
 * @param       n --   number of bytes to be send.
 * 
 * @return none
 *
 * @note n must be less than 255;
 *
 * @see
 *   serial0_port_init(),
 *   serial0_buf_send_byte(),
 *   USART0_TX_vect(), USART0_RX_vect()
 *
 * History:  Created on 2006/10/27 by Wai Fai Chin
 */

void serial0_send_bytes( const char *pbData, BYTE n)
{
  BYTE i;
	for ( i=0; i < n; i++ ) {
		while( !USART_TXBuffer_FreeSpace( &gtUsart0_data ) );		// wait for available buf
		USART_TXBuffer_PutByte( &gtUsart0_data, (BYTE)pbData[i] );
	}
}// end serial0_send_bytes()


/**
 *  It transmits n bytes of data in program memory space point by the pbPtr.
 *
 * @param  pbData --   pointer to data in program memory space to be send.
 * @param       n --   number of bytes to be send.
 * 
 * @return none
 *
 * @note n must be less than 255;
 *
 * @see
 *   serial0_port_init(),
 *
 * History:  Created on 2007/07/25 by Wai Fai Chin
 */

void serial0_send_bytes_P( const char *pbData, BYTE n)
{
  BYTE i;
  BYTE ch;
	for ( i=0; i < n; i++ ) {
		while( !USART_TXBuffer_FreeSpace( &gtUsart0_data ) );		// wait for available buf
		ch = pgm_read_byte( &pbData[i] );							// copy a byte from program memory space
		USART_TXBuffer_PutByte( &gtUsart0_data, ch );
	}
}// end serial0_send_bytes_P()


/**
 *  It transmits a string of text point by the pStr.
 *
 * @param  pStr --   pointer to a string of text to be send.
 *                   A null marks the end of a string.
 * 
 * @return none
 *
 *
 * History:  Created on 2006/10/27 by Wai Fai Chin
 */

void serial0_send_string( const char *pStr)
{
	for (;;) {
		if ( *pStr == 0 ) break;		  							// if end of string, done
		while( !USART_TXBuffer_FreeSpace( &gtUsart0_data ) );		// wait for available buf
		USART_TXBuffer_PutByte( &gtUsart0_data, *pStr++ );
	}
} // end serial0_send_string() 


/**
 *  It transmits a string of text in program memory space point by the pStr.
 *
 * @param  pStr --   pointer to a string of text in program memory space to be send.
 *                   A null marks the end of a string.
 * 
 * @return none
 *
 * History:  Created on 2006/10/27 by Wai Fai Chin
 */

void serial0_send_string_P( const char *pStr)
{
  BYTE ch;
  
	for (;;) {
		ch = pgm_read_byte( pStr++ );		// copy a byte from program memory space
		if ( ch == 0 ) break;				// if end of string, done
		while( !USART_TXBuffer_FreeSpace( &gtUsart0_data ) );		// wait for available buf
		USART_TXBuffer_PutByte( &gtUsart0_data, ch );
	}
} // end serial0_send_string_P() 


/**
 *  Wait until it received a byte of data.
 *
 * @param  none
 * 
 * @return a newly received valid byte in lower order byte if high order byte is 0xFF.
 *
 * @post
 *      Increment gbSerial0_InReadCnt by one;
 *
 *
 *
 * History:  Created on 2009/11/05 by Wai Fai Chin
 */

WORD16 serial0_get_a_byte_no_wait( void)
{
	WORD16 ch;
	ch = FALSE;

	if ( USART_RXBufferData_Available( &gtUsart0_data ) ) {
		ch = USART_RXBuffer_GetByte( &gtUsart0_data );
		ch |= 0xFF00; // flag it has valid new byte of data.
	}
	return ch;
} // end serial0_get_a_byte_no_wait()

/**  Receive complete interrupt service routine.
 *
 *  Receive complete interrupt service routine.
 *  Calls the common receive complete handler with pointer to the correct USART_data structure.
 */
//ISR(USARTC0_RXC_vect)  // Doxygen cannot handle ISR() macro.
void USARTC0_RXC_vect (void) __attribute__ ((signal,__INTR_ATTRS));
void USARTC0_RXC_vect (void)
{
	USART_RXComplete( &gtUsart0_data );
}


/**  Data register empty  interrupt service routine.
 *
 *  Data register empty  interrupt service routine.
 *  Calls the common data register empty complete handler with pointer to the correct USART_data structure.
 */

//ISR(USARTC0_DRE_vect)  // Doxygen cannot handle ISR() macro.
void USARTC0_DRE_vect (void) __attribute__ ((signal,__INTR_ATTRS));
void USARTC0_DRE_vect (void)
{
	USART_DataRegEmpty( &gtUsart0_data );
}

/**************************************************************************
* 		SERIAL PORT 1 RELATED FUNCTIONS
**************************************************************************/
/**
 * Configure USARTE1 for Serial Port 1.
 *
 * @param  baud --  baudrate selection range from 1 to 14. 1 => 300 baud ... 14 => 230400 baud.
 *
 * @return none
 *
 * \sa
 *
 * Description:
 *  Configure USARTD0 based on the specified baudrate selection.
 *
 * Example useage:
 * \code
 *   serial0_port_init( SR_BAUD_19200 );
 * \endcode
 * History:  Created on 2006/10/27 by Wai Fai Chin
 * 2012-05-09 -WFC- configure serial port1 to 9600 baud as default for MSI8000 remote meter.
 * 2013-06-11 -WFC- port to ScaleCore3.
 * 2014-06-02 -WFC- modified for direct baudrate setting.
 */

// void serial1_port_init( BYTE baud )
void serial1_port_init( UINT16 baud )
{

  	// PE3 (TXD0) as output.
	PORTE.DIRSET   = PIN3_bm;
	// PE2 (RXD0) as input.
	PORTE.DIRCLR   = PIN2_bm;


	// Use USARTD0 and initialize buffers.
	USART_InterruptDriver_Initialize( &gtUsart1_data, &gtUsart1, USART_DREINTLVL_LO_gc);

	// 8 Data bits, No Parity, 2 Stop bit.
	USART_Format_Set( gtUsart1_data.usart, USART_CHSIZE_8BIT_gc,
                     USART_PMODE_DISABLED_gc, TRUE);

	// Enable RXC interrupt.
	USART_RxdInterruptLevel_Set( gtUsart1_data.usart, USART_RXCINTLVL_LO_gc);

	USART_Baudrate_Set( &gtUsart1, baud, 0 );			//38400 for 32MHz clock

	// Enable both RX and TX.
	USART_Rx_Enable( gtUsart1_data.usart );
	USART_Tx_Enable( gtUsart1_data.usart );

	/* Enable PMIC interrupt level low. */
	PMIC.CTRL |= PMIC_LOLVLEX_bm;
} // end serial1_port_init()


/**
 *  It transmits a byte of data.
 *
 * @param  bData -- a byte of data to be send
 *
 * @return none
 *
 * Dependencies:
 *   serial1_port_init(),
 *   serial1_buf_send_byte(),
 *   USART1_TX_vect(), USART1_RX_vect()
 *
 * History:  Created on 2006/10/27 by Wai Fai Chin
 */

void serial1_send_byte( BYTE bData )
{
	while( !USART_TXBuffer_FreeSpace( &gtUsart1_data ) );		// wait for available buf
	USART_TXBuffer_PutByte( &gtUsart1_data, bData );

}// end serial1_send_byte()


/**
 *  It tries to transmit a byte of data without waiting.
 *  If it successes transmitted a byte, it returns true else return false.
 *
 * @param  bData -- a byte of data to be send
 *
 * @return true if it successes transmitted a byte else false.
 *
 * Dependencies:
 *   serial1_port_init(),
 *   serial1_buf_send_byte(),
 *   USART1_TX_vect(), USART1_RX_vect()
 *
 * History:  Created on 2006/11/01 by Wai Fai Chin
 */

BYTE serial1_send_byte_no_wait( BYTE bData )
{
  BYTE status;
	status = FALSE;                    // assumed failed to send a byte.
	if ( !USART_TXBuffer_FreeSpace( &gtUsart1_data ) )    {
		USART_TXBuffer_PutByte( &gtUsart1_data, bData );
		status = TRUE;
	}
	return status;
}// end serial1_send_byte_no_wait()


/**
 *  It transmits n bytes of data point by the pbPtr.
 *
 * @param  pbData --   pointer to data to be send.
 * @param       n --   number of bytes to be send.
 *
 * @return none
 *
 * @note n must be less than 255;
 *
 * @see
 *   serial1_port_init(),
 *   serial1_buf_send_byte(),
 *   USART1_TX_vect(), USART1_RX_vect()
 *
 * History:  Created on 2006/10/27 by Wai Fai Chin
 */

void serial1_send_bytes( const char *pbData, BYTE n)
{
  BYTE i;
	for ( i=0; i < n; i++ ) {
		while( !USART_TXBuffer_FreeSpace( &gtUsart1_data ) );		// wait for available buf
		USART_TXBuffer_PutByte( &gtUsart1_data, (BYTE)pbData[i] );
	}
}// end serial1_send_bytes()


/**
 *  It transmits n bytes of data in program memory space point by the pbPtr.
 *
 * @param  pbData --   pointer to data in program memory space to be send.
 * @param       n --   number of bytes to be send.
 *
 * @return none
 *
 * @note n must be less than 255;
 *
 * @see
 *   serial1_port_init(),
 *
 * History:  Created on 2007/07/25 by Wai Fai Chin
 */

void serial1_send_bytes_P( const char *pbData, BYTE n)
{
  BYTE i;
  BYTE ch;
	for ( i=0; i < n; i++ ) {
		while( !USART_TXBuffer_FreeSpace( &gtUsart1_data ) );		// wait for available buf
		ch = pgm_read_byte( &pbData[i] );							// copy a byte from program memory space
		USART_TXBuffer_PutByte( &gtUsart1_data, ch );
	}
}// end serial1_send_bytes_P()


/**
 *  It transmits a string of text point by the pStr.
 *
 * @param  pStr --   pointer to a string of text to be send.
 *                   A null marks the end of a string.
 *
 * @return none
 *
 *
 * History:  Created on 2006/10/27 by Wai Fai Chin
 */

void serial1_send_string( const char *pStr)
{
	for (;;) {
		if ( *pStr == 0 ) break;		  							// if end of string, done
		while( !USART_TXBuffer_FreeSpace( &gtUsart1_data ) );		// wait for available buf
		USART_TXBuffer_PutByte( &gtUsart1_data, *pStr++ );
	}
} // end serial1_send_string()


/**
 *  It transmits a string of text in program memory space point by the pStr.
 *
 * @param  pStr --   pointer to a string of text in program memory space to be send.
 *                   A null marks the end of a string.
 *
 * @return none
 *
 * History:  Created on 2006/10/27 by Wai Fai Chin
 */

void serial1_send_string_P( const char *pStr)
{
  BYTE ch;

	for (;;) {
		ch = pgm_read_byte( pStr++ );		// copy a byte from program memory space
		if ( ch == 0 ) break;				// if end of string, done
		while( !USART_TXBuffer_FreeSpace( &gtUsart1_data ) );		// wait for available buf
		USART_TXBuffer_PutByte( &gtUsart1_data, ch );
	}
} // end serial0_send_string_P()


/**
 *  Wait until it received a byte of data.
 *
 * @param  none
 *
 * @return a newly received valid byte in lower order byte if high order byte is 0xFF.
 *
 * @post
 *      Increment gbSerial1_InReadCnt by one;
 *
 *
 *
 * History:  Created on 2009/11/05 by Wai Fai Chin
 */

WORD16 serial1_get_a_byte_no_wait( void)
{
	WORD16 ch;
	ch = FALSE;

	if ( USART_RXBufferData_Available( &gtUsart1_data ) ) {
		ch = USART_RXBuffer_GetByte( &gtUsart1_data );
		ch |= 0xFF00; // flag it has valid new byte of data.
	}
	return ch;
} // end serial0_get_a_byte_no_wait()

/**  Receive complete interrupt service routine.
 *
 *  Receive complete interrupt service routine.
 *  Calls the common receive complete handler with pointer to the correct USART_data structure.
 */
//ISR(USARTE1_RXC_vect)  // Doxygen cannot handle ISR() macro.
void USARTE0_RXC_vect (void) __attribute__ ((signal,__INTR_ATTRS));
void USARTE0_RXC_vect (void)
{
	USART_RXComplete( &gtUsart1_data );
}


/**  Data register empty  interrupt service routine.
 *
 *  Data register empty  interrupt service routine.
 *  Calls the common data register empty complete handler with pointer to the correct USART_data structure.
 */

//ISR(USARTE1_DRE_vect)  // Doxygen cannot handle ISR() macro.
void USARTE0_DRE_vect (void) __attribute__ ((signal,__INTR_ATTRS));
void USARTE0_DRE_vect (void)
{
	USART_DataRegEmpty( &gtUsart1_data );
}

/// use USARTD0 for USART2, D is in port D. In PCB, it named as TXD0 and RXD0. It is for talk to Ethernet or RF modem device
#define gtUsart2 USARTD0
/// USART data struct that contains USART and data buffer.
USART_data_t 	gtUsart2_data;

/**************************************************************************
* 		SERIAL PORT 2 RELATED FUNCTIONS
**************************************************************************/
/**
 * Configure USARTD0 for Serial Port 2.
 *
 * @param  baud --  baudrate selection range from 1 to 14. 1 => 300 baud ... 14 => 230400 baud.
 *
 * @return none
 *
 * \sa
 *
 * Description:
 *  Configure USARTD0 based on the specified baudrate selection.
 *
 * Example useage:
 * \code
 *   serial2_port_init( SR_BAUD_19200 );
 * \endcode
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */

UINT16 gTestBaud2;

void serial2_port_init( BYTE baud )
{
  	// PD3 (TXD0) as output.
	PORTD.DIRSET   = PIN3_bm;
	// PD2 (RXD0) as input.
	PORTD.DIRCLR   = PIN2_bm;

	// Use USARTD0 and initialize buffers.
	USART_InterruptDriver_Initialize( &gtUsart2_data, &gtUsart2, USART_DREINTLVL_LO_gc);

	// 8 Data bits, No Parity, 1 Stop bit.
	USART_Format_Set( gtUsart2_data.usart, USART_CHSIZE_8BIT_gc,
                     USART_PMODE_DISABLED_gc, FALSE);

	// Enable RXC interrupt.
	USART_RxdInterruptLevel_Set( gtUsart2_data.usart, USART_RXCINTLVL_LO_gc);

	//= SR_BAUD_38400_V
	gTestBaud2 = SR_BAUD_9600_V;
	USART_Baudrate_Set( &gtUsart2, gTestBaud2, 0 );

	// Enable both RX and TX.
	USART_Rx_Enable( gtUsart2_data.usart );
	USART_Tx_Enable( gtUsart2_data.usart );

	/* Enable PMIC interrupt level low. */
	PMIC.CTRL |= PMIC_LOLVLEX_bm;
} // end serial2_port_init()


/*
void serial2_port_init( BYTE baud )
{
  	// PD3 (TXD0) as output.
	PORTD.DIRSET   = PIN3_bm;
	// PD2 (RXD0) as input.
	PORTD.DIRCLR   = PIN2_bm;

	// Use USARTD0 and initialize buffers.
	USART_InterruptDriver_Initialize( &gtUsart2_data, &gtUsart2, USART_DREINTLVL_LO_gc);

	// 8 Data bits, No Parity, 2 Stop bit.
	USART_Format_Set( gtUsart2_data.usart, USART_CHSIZE_8BIT_gc,
                     USART_PMODE_DISABLED_gc, TRUE);

	// Enable RXC interrupt.
	USART_RxdInterruptLevel_Set( gtUsart2_data.usart, USART_RXCINTLVL_LO_gc);

	#if ( CONFIG_XTAL_FREQUENCY  == INTERNAL_32MHZ )
		USART_Baudrate_Set( &gtUsart2, SR_BAUD_38400_V, 0 );
	#elif ( CONFIG_XTAL_FREQUENCY  == XTAL_14P746MHZ )
		USART_Baudrate_Set( &gtUsart2, 23 , 0 );			//38400 for 14.746MHz clock
	#endif

	// Enable both RX and TX.
	USART_Rx_Enable( gtUsart2_data.usart );
	USART_Tx_Enable( gtUsart2_data.usart );

	/ * Enable PMIC interrupt level low. /
	PMIC.CTRL |= PMIC_LOLVLEX_bm;
} // end serial2_port_init()
*/

/**
 *  It transmits a byte of data.
 *
 * @param  bData -- a byte of data to be send
 *
 * @return none
 *
 * Dependencies:
 *   serial2_port_init(),
 *   serial2_buf_send_byte(),
 *   USART2_TX_vect(), USART2_RX_vect()
 *
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */

void serial2_send_byte( BYTE bData )
{
	while( !USART_TXBuffer_FreeSpace( &gtUsart2_data ) );		// wait for available buf
	USART_TXBuffer_PutByte( &gtUsart2_data, bData );

}// end serial2_send_byte()


/**
 *  It tries to transmit a byte of data without waiting.
 *  If it successes transmitted a byte, it returns true else return false.
 *
 * @param  bData -- a byte of data to be send
 *
 * @return true if it successes transmitted a byte else false.
 *
 * Dependencies:
 *   serial2_port_init(),
 *   serial2_buf_send_byte(),
 *   USART2_TX_vect(), USART2_RX_vect()
 *
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */

BYTE serial2_send_byte_no_wait( BYTE bData )
{
  BYTE status;
	status = FALSE;                    // assumed failed to send a byte.
	if ( !USART_TXBuffer_FreeSpace( &gtUsart2_data ) )    {
		USART_TXBuffer_PutByte( &gtUsart2_data, bData );
		status = TRUE;
	}
	return status;
}// end serial2_send_byte_no_wait()


/**
 *  It transmits n bytes of data point by the pbPtr.
 *
 * @param  pbData --   pointer to data to be send.
 * @param       n --   number of bytes to be send.
 *
 * @return none
 *
 * @note n must be less than 255;
 *
 * @see
 *   serial2_port_init(),
 *   serial2_buf_send_byte(),
 *   USART2_TX_vect(), USART2_RX_vect()
 *
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */

void serial2_send_bytes( const char *pbData, BYTE n)
{
  BYTE i;
	for ( i=0; i < n; i++ ) {
		while( !USART_TXBuffer_FreeSpace( &gtUsart2_data ) );		// wait for available buf
		USART_TXBuffer_PutByte( &gtUsart2_data, (BYTE)pbData[i] );
	}
}// end serial2_send_bytes()


/**
 *  It transmits n bytes of data in program memory space point by the pbPtr.
 *
 * @param  pbData --   pointer to data in program memory space to be send.
 * @param       n --   number of bytes to be send.
 *
 * @return none
 *
 * @note n must be less than 255;
 *
 * @see
 *   serial1_port_init(),
 *
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */

void serial2_send_bytes_P( const char *pbData, BYTE n)
{
  BYTE i;
  BYTE ch;
	for ( i=0; i < n; i++ ) {
		while( !USART_TXBuffer_FreeSpace( &gtUsart2_data ) );		// wait for available buf
		ch = pgm_read_byte( &pbData[i] );							// copy a byte from program memory space
		USART_TXBuffer_PutByte( &gtUsart2_data, ch );
	}
}// end serial2_send_bytes_P()


/**
 *  It transmits a string of text point by the pStr.
 *
 * @param  pStr --   pointer to a string of text to be send.
 *                   A null marks the end of a string.
 *
 * @return none
 *
 *
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */

void serial2_send_string( const char *pStr)
{
	for (;;) {
		if ( *pStr == 0 ) break;		  							// if end of string, done
		while( !USART_TXBuffer_FreeSpace( &gtUsart2_data ) );		// wait for available buf
		USART_TXBuffer_PutByte( &gtUsart2_data, *pStr++ );
	}
} // end serial2_send_string()


/**
 *  It transmits a string of text in program memory space point by the pStr.
 *
 * @param  pStr --   pointer to a string of text in program memory space to be send.
 *                   A null marks the end of a string.
 *
 * @return none
 *
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */

void serial2_send_string_P( const char *pStr)
{
  BYTE ch;

	for (;;) {
		ch = pgm_read_byte( pStr++ );		// copy a byte from program memory space
		if ( ch == 0 ) break;				// if end of string, done
		while( !USART_TXBuffer_FreeSpace( &gtUsart2_data ) );		// wait for available buf
		USART_TXBuffer_PutByte( &gtUsart2_data, ch );
	}
} // end serial2_send_string_P()


/**
 *  Wait until it received a byte of data.
 *
 * @param  none
 *
 * @return a newly received valid byte in lower order byte if high order byte is 0xFF.
 *
 * @post
 *      Increment gbSerial2_InReadCnt by one;
 *
 *
 *
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */

WORD16 serial2_get_a_byte_no_wait( void)
{
	WORD16 ch;
	ch = FALSE;

	if ( USART_RXBufferData_Available( &gtUsart2_data ) ) {
		ch = USART_RXBuffer_GetByte( &gtUsart2_data );
		ch |= 0xFF00; // flag it has valid new byte of data.
	}
	return ch;
} // end serial2_get_a_byte_no_wait()

/**  Receive complete interrupt service routine.
 *
 *  Receive complete interrupt service routine.
 *  Calls the common receive complete handler with pointer to the correct USART_data structure.
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */
//ISR(USARTD0_RXC_vect)  // Doxygen cannot handle ISR() macro.
void USARTD0_RXC_vect (void) __attribute__ ((signal,__INTR_ATTRS));
void USARTD0_RXC_vect (void)
{
	USART_RXComplete( &gtUsart2_data );
}


/**  Data register empty  interrupt service routine.
 *
 *  Data register empty  interrupt service routine.
 *  Calls the common data register empty complete handler with pointer to the correct USART_data structure.
 * History:  Created on 2011-05-10 by Wai Fai Chin
 */

//ISR(USARTD0_DRE_vect)  // Doxygen cannot handle ISR() macro.
void USARTD0_DRE_vect (void) __attribute__ ((signal,__INTR_ATTRS));
void USARTD0_DRE_vect (void)
{
	USART_DataRegEmpty( &gtUsart2_data );
}



///////////////////////////////////////////////////////////////////////////////
//                      Test functions for this module.                      //
///////////////////////////////////////////////////////////////////////////////

#if  ( CONFIG_TEST_SERIAL0_MODULE == TRUE )

struct pt gSerial0TestThread_pt;

/**
 *  This thread listens to serial port 0 and echo recieved data.
 *
 * @param  pt -- points to ProtoThread structure of this thread.
 * 
 * @return status of this thread.
 *
 * Usage:
 * \code
	main(void)
	{
		//
		//
	
		PT_INIT( &gSerial0TestThread_pt );

		for(;;) {
			//
			//
			
			test_serial0_thread( &gSerial0TestThread_pt );
			
			//
			//
		}
	} // end main()
 * \endcode
 * Dependencies:
 *   serial0_port_init(), serial0_buf_send_byte(),
 *   PT_INIT( &gSerial0TestThread_pt );
 *
 *
 * History:  Created on 2006/10/27 by Wai Fai Chin
 */


// PT_THREAD( test_serial0_thread(struct pt *pt) )  // Doxygen cannot handle this macro
char test_serial0_thread(struct pt *pt)
{
  WORD16 ch;

  PT_BEGIN( pt );
  while(1) {
    PT_WAIT_UNTIL( pt, ((ch = serial0_get_a_byte_no_wait()) & 0xFF00) );
    serial0_send_byte_no_wait( (BYTE)ch );
  }
  
  PT_END( pt );
} // end test_serial0_thread()


#endif // ( CONFIG_TEST_SERIAL0_MODULE == TRUE )

