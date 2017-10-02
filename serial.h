/*! \file serial.h \brief serial port uart routines. */
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2010 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: serial.h
// Hardware: ATMEL AVR series
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2010/01/29 by Wai Fai Chin
//
/// \ingroup driver_avr_cpu
/// \defgroup uart Serial port UART Driver/Function Library (serial.c)
/// \code #include "serial.h" \endcode
/// \par Overview
///   Serial interrupt driven buffered related functions.
///
/// \note
/// The TIMER assignment:
// 
// ****************************************************************************
//@{
#ifndef MSI_SERIAL_H
#define MSI_SERIAL_H

#include "config.h"
#include "usart_driver.h"
#include  "bios.h"


#define  SR_BAUD_300                1
#define  SR_BAUD_600                2
#define  SR_BAUD_1200               3
#define  SR_BAUD_2400               4
#define  SR_BAUD_4800               5
#define  SR_BAUD_9600               6
#define  SR_BAUD_14400              7
#define  SR_BAUD_19200              8
#define  SR_BAUD_28800              9
#define  SR_BAUD_38400              10
#define  SR_BAUD_57600              11
#define  SR_BAUD_76800              12
#define  SR_BAUD_115200             13
#define  SR_BAUD_230400             14

#if ( CONFIG_XTAL_FREQUENCY == INTERNAL_32MHZ )
#define  SR_BAUD_300_V		6666
#define  SR_BAUD_600_V		3332
#define  SR_BAUD_1200_V		1666
#define  SR_BAUD_2400_V		832
#define  SR_BAUD_4800_V		416
#define  SR_BAUD_9600_V		207
#define  SR_BAUD_14400_V	138
#define  SR_BAUD_19200_V	103
#define  SR_BAUD_28800_V	68
#define  SR_BAUD_38400_V	51
#define  SR_BAUD_57600_V	34
#define  SR_BAUD_76800_V	25
#define  SR_BAUD_115200_V	16
#define  SR_BAUD_230400_V	8
#elif ( CONFIG_XTAL_FREQUENCY == XTAL_14P746MHZ )

#define  SR_BAUD_300_V		3071
#define  SR_BAUD_600_V		1535
#define  SR_BAUD_1200_V		767
#define  SR_BAUD_2400_V		383
#define  SR_BAUD_4800_V		191
#define  SR_BAUD_9600_V		95
#define  SR_BAUD_14400_V	63
#define  SR_BAUD_19200_V	47
#define  SR_BAUD_28800_V	31
#define  SR_BAUD_38400_V	23
#define  SR_BAUD_57600_V	15
#define  SR_BAUD_76800_V	11
#define  SR_BAUD_115200_V	7
#define  SR_BAUD_230400_V	3
#endif


WORD16 serial0_get_a_byte_no_wait( void );
// 2014-06-02 -WFC-  void   serial0_port_init( BYTE buad );
void   serial0_port_init( UINT16 buad );   // 2014-06-02 -WFC-
void   serial0_send_byte( BYTE bData );
void   serial0_send_bytes( const char *pbData, BYTE n );
void   serial0_send_bytes_P( const char *pbData_flash, BYTE n );
void   serial0_send_string( const char *pStr );
void   serial0_send_string_P( const char *pStr_flash);

WORD16 serial1_get_a_byte_no_wait( void );
// 2014-06-02 -WFC-  void   serial1_port_init( BYTE buad );
void   serial1_port_init( UINT16 buad );   // 2014-06-02 -WFC-
void   serial1_send_byte( BYTE bData );
void   serial1_send_bytes( const char *pbData, BYTE n );
void   serial1_send_bytes_P( const char *pbData_flash, BYTE n );
void   serial1_send_string( const char *pStr );
void   serial1_send_string_P( const char *pStr_flash);

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_IDSC )
WORD16 serial2_get_a_byte_no_wait( void );
void   serial2_port_init( BYTE buad );
void   serial2_send_byte( BYTE bData );
void   serial2_send_bytes( const char *pbData, BYTE n );
void   serial2_send_bytes_P( const char *pbData_flash, BYTE n );
void   serial2_send_string( const char *pStr );
void   serial2_send_string_P( const char *pStr_flash);
#endif

// extern BYTE gfSerial0_Outbuf_Full;		// flag gSerial0_OutBuf is full

/// USART data struct that contains USART and data buffer.
extern	USART_data_t gtUsart0_data;
extern	USART_data_t gtUsart1_data;


/** \def serial0_is_txbuff_free
 * See if there is free space in transmit buffer.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define  serial0_is_txbuff_free()   USART_TXBuffer_FreeSpace( &gtUsart0_data )

#define  serial1_is_txbuff_free()   USART_TXBuffer_FreeSpace( &gtUsart1_data )

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_IDSC || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER)	// 2011-10-07 -WFC-
extern	USART_data_t gtUsart2_data;
#define  serial2_is_txbuff_free()   USART_TXBuffer_FreeSpace( &gtUsart2_data )
#endif

#define	 serial0_disabled_rx()		USART_Rx_Disable( gtUsart0_data.usart )
#define	 serial0_disabled_tx()		USART_Tx_Disable( gtUsart0_data.usart )
#define	 serial0_disabled_rx_isr()	USART_RxdInterruptLevel_Set( gtUsart0_data.usart, USART_RXCINTLVL_OFF_gc)
#define	 serial0_disabled_tx_isr()	USART_DreInterruptLevel_Set( gtUsart0_data.usart, USART_DREINTLVL_OFF_gc)

#define	 serial1_disabled_rx()		USART_Rx_Disable( gtUsart1_data.usart )
#define	 serial1_disabled_tx()		USART_Tx_Disable( gtUsart1_data.usart )
#define	 serial1_disabled_rx_isr()	USART_RxdInterruptLevel_Set( gtUsart1_data.usart, USART_RXCINTLVL_OFF_gc)
#define	 serial1_disabled_tx_isr()	USART_DreInterruptLevel_Set( gtUsart1_data.usart, USART_DREINTLVL_OFF_gc)

#define  serial0_clear_txcif()		gtUsart0_data.usart->STATUS = 0x40
#define  serial1_clear_txcif()		gtUsart1_data.usart->STATUS = 0x40

#if ( CONFIG_TEST_SERIAL0_MODULE == TRUE ) 
    #include "pt.h"
    extern struct pt gSerial0TestThread_pt;
    // PT_THREAD( test_serial0_thread(struct pt *pt) );  // Doxygen cannot handle this macro
    char test_serial0_thread(struct pt *pt);
#endif

#if defined( CONFIG_USE_DOXYGEN )
/*
 * Doxygen doesn't work on the appended attribute syntax of
 * GCC, and confuses the typedefs with function decls, so
 * supply a doxygen-friendly view.
 */
 
extern const WORD16 PROGMEM gwSerial0_BaudrateTbl[];

#else // not DOXYGEN

extern const WORD16 gwSerial0_BaudrateTbl[] PROGMEM;
#endif // DOXYGEN

#endif  // MSI_SERIAL_H
//@}
