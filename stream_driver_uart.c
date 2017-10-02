/*! \file stream_driver_uart.c \brief uart stream driver functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: independent
// OS:       independent
// Compiler: avr-gcc
// Software layer:  concrete layer
//
//  History:  Created on 2009/03/17 by Wai Fai Chin
// 
//     It routes high level IO stream between application and stream drivers to the low level hardware uart driver.
//
//
//
//                            STREAM FLOWS and OBJECT INTERACTION DIAGRAM
//
//                                                 High level stream driver
//           APPLICATION           Stream_Router     RFmodem_Stream_driver   low level hardware com driver
//         +---------------+     +----------------+   +----------------+      +--------------+
//         |               |     |                |   |                |      | UART         |
//         |  Hight level  |---->| route stream to|-->| RFmodem_Stream |----->| Serial port  |
//         |stream write() |     | the appropriate|   | send()         |      | send()       |
//         |       read () |     | stream driver  |<--| Receive()      |<-----| Receive()    |
//         +---------------+     +----------------+   +----------------+      +--------------+
//                    ^               ^        |
//                    |               |        |
//                    |               |        |
//                    |   +---------------+    |
//                    |   |               |    |
//                    +---| Packet Router |<---+
//                        |               |
//                        +---------------+ 
//
// ****************************************************************************


#include "stream_router.h"

/// STREAM_DRIVER_CLASS gStreamDriverUart;
IO_STREAM_CLASS		*gpUartIstream;

void stream_driver_uart_receive( void );
void stream_driver_uart_send( IO_STREAM_CLASS *pStream );


void stream_driver_uart1_receive( void );
void stream_driver_uart1_send( IO_STREAM_CLASS *pStream );
IO_STREAM_CLASS		*gpUart1Istream;

// 2011-05-09 -WFC- v
#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_IDSC || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER)	// 2011-10-07 -WFC-
void stream_driver_uart2_send( IO_STREAM_CLASS *pStream );
void stream_driver_uart2_receive( void );
IO_STREAM_CLASS		*gpUart2Istream;
#endif
// 2011-05-09 -WFC- ^

#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )

#include "serial.h"

/**
 * It implements concrete constructor uart stream driver object for STREAM_DRIVER_CLASS.
 * It acquired a new iStream and initialized it as UART type and active.
 *
 * @return none.
 * @note Must call stream_router_init_build_all_iostream() before call this method.
 *
 * History:  Created on 2009/03/18 by Wai Fai Chin
 */

void stream_driver_uart_constructor( void )
{
	STREAM_DRIVER_CLASS *pStreamDrv;
	
	pStreamDrv = &gaStreamDriver[IO_STREAM_TYPE_UART];
	if ( stream_router_get_a_new_stream( &gpUartIstream ) ) {
		pStreamDrv-> type	=
		gpUartIstream-> type	= IO_STREAM_TYPE_UART;
		gpUartIstream-> status	= IO_STREAM_STATUS_DIR_IN | IO_STREAM_STATUS_ACTIVE;
		pStreamDrv-> status 	= STREAM_DRIVER_STATUS_HAS_ISTREAM;	// safe and reliable computing first.
		pStreamDrv-> pRegister	= stream_register_nop;				// uart stream driver does not require register operation. Just supplies empty method.
		pStreamDrv-> pSend		= stream_driver_uart_send;
		pStreamDrv-> pReceive	= stream_driver_uart_receive;
	}
	else {
		pStreamDrv-> status &= ~STREAM_DRIVER_STATUS_HAS_ISTREAM;
	}
	
} // end stream_driver_uart_constructor()


/**
 * It implements concrete send method of STREAM_DRIVER_CLASS for uart stream driver.
 * 
 * @param	pStream -- pointer to iostream object.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/03/18 by Wai Fai Chin
 */

void stream_driver_uart_send( IO_STREAM_CLASS *pStream )
{
	BYTE bData;
	
	while ( read_byte_circular_buf( pStream-> pCirBuf, &bData ) ) {
		while( !serial0_is_txbuff_free() );		// wait for available buf
		serial0_send_byte( bData );
	}
} // end stream_driver_uart_constructor()


/**
 * It implements concrete method to receive data from low level hardware driver.
 * It grabs data from uart0 input buffer and put it into iStream until no more
 * data from uart0 or iStream buffer is full. 
 *
 * @return none.
 * @note Ethernet, RFmodem stream drivers have further process by disassemble Ethernet,
 * RF packet and put it into each iStream for each remote device.
 *
 * History:  Created on 2009/03/18 by Wai Fai Chin
 */

void stream_driver_uart_receive( void )
{
  WORD16 ch;
	
	while ((ch = serial0_get_a_byte_no_wait()) & 0xFF00) {
		if (write_byte_circular_buf( gpUartIstream-> pCirBuf, (BYTE) ch ) ) 	// saved a byte of new input uart data into the iStream of this this driver object.
			break;	// UartIstream buffer is full.
	}
	
} // end stream_driver_uart_receive()


/**************************************************************************
* 		SERIAL PORT 1 STREAM DRIVER
**************************************************************************/
/**
 * It implements concrete constructor uart1 stream driver object for STREAM_DRIVER_CLASS.
 * It acquired a new iStream and initialized it as UART type and active.
 *
 * @return none.
 * @note Must call stream_router_init_build_all_iostream() before call this method.
 *
 * History:  Created on 2010/03/26 by Wai Fai Chin
 */

void stream_driver_uart1_constructor( void )
{
	STREAM_DRIVER_CLASS *pStreamDrv;

	pStreamDrv = &gaStreamDriver[IO_STREAM_TYPE_UART_1];
	if ( stream_router_get_a_new_stream( &gpUart1Istream ) ) {
		pStreamDrv-> type	=
		gpUart1Istream-> type	= IO_STREAM_TYPE_UART_1;
		gpUart1Istream-> status	= IO_STREAM_STATUS_DIR_IN | IO_STREAM_STATUS_ACTIVE;
		pStreamDrv-> status		= STREAM_DRIVER_STATUS_HAS_ISTREAM;	// safe and reliable computing first.
		pStreamDrv-> pRegister	= stream_register_nop;				// uart stream driver does not require register operation. Just supplies empty method.
		pStreamDrv-> pSend		= stream_driver_uart1_send;
		pStreamDrv-> pReceive	= stream_driver_uart1_receive;
	}
	else {
		pStreamDrv-> status &= ~STREAM_DRIVER_STATUS_HAS_ISTREAM;
	}

} // end stream_driver_uart1_constructor()


/**
 * It implements concrete send method of STREAM_DRIVER_CLASS for uart1 stream driver.
 *
 * @param	pStream -- pointer to iostream object.
 *
 * @return none.
 *
 *
 * History:  Created on 2010/03/26 by Wai Fai Chin
 */

void stream_driver_uart1_send( IO_STREAM_CLASS *pStream )
{
	BYTE bData;

	while ( read_byte_circular_buf( pStream-> pCirBuf, &bData ) ) {
		while( !serial1_is_txbuff_free() );		// wait for available buf
		serial1_send_byte( bData );
	}
} // end stream_driver_uart1_constructor()


/**
 * It implements concrete method to receive data from low level hardware driver.
 * It grabs data from uart1 input buffer and put it into iStream until no more
 * data from uart1 or iStream buffer is full.
 *
 * @return none.
 * @note Ethernet, RFmodem stream drivers have further process by disassemble Ethernet,
 * RF packet and put it into each iStream for each remote device.
 *
 * History:  Created on 2010/03/26 by Wai Fai Chin
 */

void stream_driver_uart1_receive( void )
{
  WORD16 ch;

	while ((ch = serial1_get_a_byte_no_wait()) & 0xFF00) {
		if (write_byte_circular_buf( gpUart1Istream-> pCirBuf, (BYTE) ch ) ) 	// saved a byte of new input uart data into the iStream of this this driver object.
			break;	// Uart1Istream buffer is full.
	}

} // end stream_driver_uart1_receive()

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_IDSC || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER)	// 2011-10-07 -WFC-

/**************************************************************************
* 		SERIAL PORT 2 STREAM DRIVER
**************************************************************************/
/**
 * It implements concrete constructor uart stream driver object for STREAM_DRIVER_CLASS.
 * It acquired a new iStream and initialized it as UART type and active.
 *
 * @return none.
 * @note Must call stream_router_init_build_all_iostream() before call this method.
 *
 * History:  Created on 2011-06-09 by Wai Fai Chin
 */

void stream_driver_uart2_constructor( void )
{
	STREAM_DRIVER_CLASS *pStreamDrv;

	pStreamDrv = &gaStreamDriver[IO_STREAM_TYPE_UART_2];
	if ( stream_router_get_a_new_stream( &gpUart2Istream ) ) {
		pStreamDrv-> type	=
		gpUart2Istream-> type	= IO_STREAM_TYPE_UART_2;
		gpUart2Istream-> status	= IO_STREAM_STATUS_DIR_IN | IO_STREAM_STATUS_ACTIVE;
		pStreamDrv-> status		= STREAM_DRIVER_STATUS_HAS_ISTREAM;	// safe and reliable computing first.
		pStreamDrv-> pRegister	= stream_register_nop;				// uart stream driver does not require register operation. Just supplies empty method.
		pStreamDrv-> pSend		= stream_driver_uart2_send;
		pStreamDrv-> pReceive	= stream_driver_uart2_receive;
	}
	else {
		pStreamDrv-> status &= ~STREAM_DRIVER_STATUS_HAS_ISTREAM;
	}

} // end stream_driver_uart2_constructor()


/**
 * It implements concrete send method of STREAM_DRIVER_CLASS for uart2 stream driver.
 *
 * @param	pStream -- pointer to iostream object.
 *
 * @return none.
 *
 *
 * History:  Created on 2011-06-09 by Wai Fai Chin
 */

void stream_driver_uart2_send( IO_STREAM_CLASS *pStream )
{
	BYTE bData;

	while ( read_byte_circular_buf( pStream-> pCirBuf, &bData ) ) {
		while( !serial2_is_txbuff_free() );		// wait for available buf
		serial2_send_byte( bData );
	}
} // end stream_driver_uart2_constructor()


/**
 * It implements concrete method to receive data from low level hardware driver.
 * It grabs data from uart2 input buffer and put it into iStream until no more
 * data from uart2 or iStream buffer is full.
 *
 * @return none.
 * @note Ethernet, RFmodem stream drivers have further process by disassemble Ethernet,
 * RF packet and put it into each iStream for each remote device.
 *
 * History:  Created on 2011-05-09 by Wai Fai Chin
 */

void stream_driver_uart2_receive( void )
{
  WORD16 ch;

	while ((ch = serial2_get_a_byte_no_wait()) & 0xFF00) {
		if (write_byte_circular_buf( gpUart2Istream-> pCirBuf, (BYTE) ch ) ) 	// saved a byte of new input uart data into the iStream of this this driver object.
			break;	// Uart1Istream buffer is full.
	}

} // end stream_driver_uart2_receive()

#endif //( CONFIG_PRODUCT_AS	== CONFIG_AS_IDSC )


#else // US PC GCC COMPILER
// *************************************************************************

/// The following packet string is for test the msi_packet_router module on a PC.

#define MAX_TEST_UART_STREAM_BUF_SIZE	 128

// for stress test the following pakcet string that fill up the stream buffer before it get the END marker,
// you must define MAX_STREAM_CIR_BUF_SIZE to	16 in the stream_router.h file.

//BYTE gabTestPacketString[] = {"{30FF03}{30AW01}{30FF02?}{3{ACFF04=}{}{;;;}{30{ACFF04r}"};
//BYTE gabTestPacketString[] = {"{12345678{012{456{30FF02?}{30AW01}{3{ACFF04=}{}{;;;}{30{ACFF04r}"};
//BYTE gabTestPacketString[] = {"{12345678{012{456{01FF02?}{44AW01}{3{02FF04=}{}{;;;}{30{03FF04r}{04FF06?}{05FF07r}"};
BYTE gabTestPacketString[] = {"{01FF02?}{02FF04=}{03FF05r}{04FF06?}{05FF07r}{02FF08=}{03FF09r}"};
//BYTE gabTestPacketString[] = {"{1234567890123456{30FF02?}{30AW01}{3{ACFF04=}{}{;;;}{30{ACFF04r}"};

BYTE	gbCirBufDataBuf[ MAX_TEST_UART_STREAM_BUF_SIZE ];

BYTE_CIRCULAR_BUF_T		gTestPacketStringCirBuf;


/**
 * It implements concrete constructor uart stream driver object for STREAM_DRIVER_CLASS.
 * It acquired a new iStream and initialized it as UART type and active.
 *
 * @return none.
 * @note Must call stream_router_init_build_all_iostream() before call this method.
 *
 * History:  Created on 2009/03/18 by Wai Fai Chin
 */

void stream_driver_uart_constructor( void )
{
	STREAM_DRIVER_CLASS *pStreamDrv;
	int len;
	
	pStreamDrv = &gaStreamDriver[IO_STREAM_TYPE_UART];
	if ( stream_router_get_a_new_stream( &gpUartIstream ) ) {
		pStreamDrv-> type	=
		gpUartIstream-> type	= IO_STREAM_TYPE_UART;
		gpUartIstream-> status	= IO_STREAM_STATUS_DIR_IN | IO_STREAM_STATUS_ACTIVE;
		pStreamDrv-> status = STREAM_DRIVER_STATUS_HAS_ISTREAM;	// safe and reliable computing first.
		pStreamDrv-> pRegister	= stream_register_nop;				// uart stream driver does not require register operation. Just supplies empty method.
		pStreamDrv-> pSend		= stream_driver_uart_send;
		pStreamDrv-> pReceive	= stream_driver_uart_receive;
	}
	else {
		pStreamDrv-> status &= ~STREAM_DRIVER_STATUS_HAS_ISTREAM;
	}
	
	init_byte_circular_buf( &gTestPacketStringCirBuf, gbCirBufDataBuf, 0, MAX_TEST_UART_STREAM_BUF_SIZE);
	
	len = strlen( gabTestPacketString );
	write_bytes_to_byte_circular_buf( &gTestPacketStringCirBuf, gabTestPacketString, len );

} // end stream_driver_uart_constructor()


/**
 * It implements concrete send method of STREAM_DRIVER_CLASS for uart stream driver.
 * 
 * @param	pStream -- pointer to iostream object.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/03/18 by Wai Fai Chin
 */

void stream_driver_uart_send( IO_STREAM_CLASS *pStream )
{
	BYTE bData;
	
	while ( read_byte_circular_buf( pStream-> pCirBuf, &bData ) ) {
		printf("%c", bData );
	}
	
} // end stream_driver_uart_constructor()


/**
 * It implements concrete method to receive data from low level hardware driver.
 * It grabs data from uart0 input buffer and put it into iStream until no more
 * data from uart0 or iStream buffer is full. 
 *
 * @return none.
 * @note Ethernet, RFmodem stream drivers have further process by disasmble Ethernet,
 * RF packet and put it into each iStream for each remote device.
 *
 * History:  Created on 2009/03/18 by Wai Fai Chin
 */

void stream_driver_uart_receive( void )
{
	int len;
	BYTE bData;

	for (;;) {
		if ( read_byte_circular_buf( &gTestPacketStringCirBuf, &bData ) ) {
			if (write_byte_circular_buf( gpUartIstream-> pCirBuf, bData ) ) 	// saved a byte of new input uart data into the iStream of this this driver object.
				break;	// UartIstream buffer is full.
		}
		else { // test string buffer is empty, fill it up again.
			len = strlen( gabTestPacketString );
			reset_byte_circular_buf( &gTestPacketStringCirBuf );
			write_bytes_to_byte_circular_buf( &gTestPacketStringCirBuf, gabTestPacketString, len );
		}
	}
	
} // end stream_driver_uart_receive()


#endif //#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )
