/*! \file eeprm.h \brief eeprom routines. */
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: eeprm.h
// Hardware: ATMEL AVR series
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2007/08/20 by Wai Fai Chin
//
/// \ingroup driver_avr_cpu
/// \defgroup eeprm EEPROM functions (eeprm.c)
/// \code #include "eeprm.h" \endcode
/// \par Overview
///   EEPROM read and write functions.
//
// 
// ****************************************************************************
//@{

#ifndef MSI_EEPROM_H
#define MSI_EEPROM_H

#include "config.h"
#include <avr/eeprom.h>

//#define  EEMEM __attribute__((section(".eeprom")))

#define MAX_EEMEM_BUF_LEN

BYTE eeprm_read_byte( BYTE *eepromAddr, BYTE *ch);

#endif  // MSI_EEPROM_H
//@}
