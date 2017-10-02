/*! \file eeprm.c \brief eeprom routines. */
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: eeprm.c
// Hardware: ATMEL ATMEGA1281 CPU
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2007/08/20 by Wai Fai Chin
//
// 
// ****************************************************************************


#include "eeprm.h"

// EE_READY_vect

/**
 * read a byte from EEPROM.
 * 
 * @param eepromAddr -- EEPROM address of a content to be read.
 * @param ch -- points at the destination byte.
 * @return false if eeprom failed.
 *         true if it successed.
 * @note
 *  This is intent for use in beginning of the main function to check if 
 *  the EEPROM is working.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */

BYTE eeprm_read_byte( BYTE *eepromAddr, BYTE *ch)
{
	UINT16 timeroutCnt;
	BYTE status;
	
	timeroutCnt = 0;
	status = TRUE;					// assume success
	while ( !eeprom_is_ready()) {
		timeroutCnt++;
		if ( timeroutCnt > 0xFFF0 ) {
			status = FALSE;				// timeout, the EEPROM is not working.
			break;
		}
	}
	
	if ( status ) {
		*ch = eeprom_read_byte( eepromAddr );
	}
	return status;
}// end eeprm_read_byte(,)

