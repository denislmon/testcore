/*! \file i2c.h \brief i2c Peripheral Interface routines. */
// ****************************************************************************
//
//                      MSI CellScale
//
//                  Copyright (c) 2011 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: i2c.h
// Hardware: ATMEL AVR Xmga series
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2007/09/07 by Wai Fai Chin
//
/// \ingroup driver_avr_cpu
/// \defgroup i2c I2C Serial Peripheral Interface Driver/Function Library (i2c.c)
/// \code #include "i2c.h" \endcode
/// \par Overview
///   Serial Peripheral Interface related functions.
///   It only took 50~60uSec to read four bytes of data with SPI at 1,843,200 Hz. Thus, I decided to just use
///   polling mode for the UART_SPI on both xmitting and receiving. SPI interrupt mode would be more complex and
///   overhead excuction and memory size. Since this UART_SPI is mainly for xmit 4 bytes or less per transaction
///   with its attached chips, it is better to have poll mode than interrupt mode.
// 
// ****************************************************************************
//@{
#ifndef MSI_I2C_H
#define MSI_I2C_H

#include "config.h"

/*!
  \def MAX_I2C_IN_LEN
  Maximum size of input buffer of TWI port. Its size must be in the power of 2.
  \def MAX_I2C_OUT_LEN
  Maximum size of output buffer of TWI port. Its size must be in the power of 2.
*/

#define MAX_I2C_IN_LEN		16	// not used with PFC8562 display chip
#define MAX_I2C_OUT_LEN		16

// 2011-09-27 -WFC- BYTE i2c_transmitt( BYTE bData[], BYTE cnt );
BYTE i2c_write_lcd( BYTE *pbSrc, BYTE dataLen );						// 2011-09-27 -WFC-


#endif  // MSI_I2C_H
//@}
