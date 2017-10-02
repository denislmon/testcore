/*! \file fer_nvram.c \brief external ferroelectric nonvolatile ram related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATXMEGA128A1
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2009/11/03 by Wai Fai Chin
// 
//   This is a drive for external ferroelectric nonvolatile ram.
//
// ****************************************************************************


#include  "fer_nvram.h"
#include  "spi.h"
#include  "cmdparser.h"


// BYTE gTestStrFNV[81];

/**
 * initialize FRAM chip by removed protect status bit in the chip.
 *
 * @return none
 *
 * History:  Created on 2009/01/26 by Wai Fai Chin
 */

void  fram_init( void )
{
	FRAM_CS_SELECT;											// Chip select low to select the FRAM chip.
	fram_spi_transceive( FRAM_WRITE_ENABLE_CMD );
	FRAM_CS_DESELECT;										// Chip select high to deselect the FRAM chip. It will not work without this deselect.

	FRAM_CS_SELECT;											// Chip select low to select the FRAM chip.
	fram_spi_transceive( FRAM_WRITE_STATUS_CMD );
	fram_spi_transceive( FRAM_STATUS_UNPROTECT );
	FRAM_CS_DESELECT;										// Chip select high to deselect the FRAM chip.
	
} // end fram_init()


/**
 * write a specified number of bytes to the specified FRAM address.
 *
 * @param  destAddr	-- destination address of FRAM chip ( internal memory address of FRAM chip).
 * @param  pSrc		-- pointer to source buffer.
 * @param  nSize	-- number of bytes to be written.
 * @return none
 *
 * History:  Created on 2009/01/26 by Wai Fai Chin
 */

void  fram_write_bytes( UINT16 destAddr, BYTE *pSrc, UINT16 nSize )
{
	GENERIC_UNION u;
	
	u.ui16.l = destAddr;
	if ( ( destAddr + nSize ) <= (FRAM_MAX_ADDR_RANGE + 1) )	{	// ensured that it will not wrap around.
		FRAM_CS_SELECT;												// Chip select low to select the FRAM chip.
		fram_spi_transceive( FRAM_WRITE_ENABLE_CMD );					// send write enabled command.
		FRAM_CS_DESELECT;											// Chip select high to deselect the FRAM chip. It will not work without this deselect.

		FRAM_CS_SELECT;												// Chip select low to select the FRAM chip.
		fram_spi_transceive( FRAM_WRITE_DATA_CMD );						// send write data command.
		fram_spi_transceive( u.b.b1);									// send high order byte of address
		fram_spi_transceive( u.b.b0);									// send low  order byte of address
		for (;  nSize > 0; nSize--)
			fram_spi_transceive( *pSrc++ );
		FRAM_CS_DESELECT;											// Chip select high to deselect the FRAM chip.
	}
	
} // end fram_write_bytes(,,)

/**
 * read a specified number of bytes to the specified FRAM address.
 *
 * @param  pDest		-- pointer to source buffer.
 * @param  sourceAddr	-- source address of FRAM chip ( internal memory address of FRAM chip).
 * @param  nSize		-- number of bytes to be read.
 * @return none
 *
 * History:  Created on 2009/01/26 by Wai Fai Chin
 */

void  fram_read_bytes( BYTE *pDest, UINT16 sourceAddr, UINT16 nSize )
{
	GENERIC_UNION u;

	u.ui16.l = sourceAddr;
	if ( ( sourceAddr + nSize ) <= (FRAM_MAX_ADDR_RANGE + 1) )	{	// ensured that it will not wrap around.
		FRAM_CS_SELECT;										// Chip select low to select the FRAM chip.
		fram_spi_transceive( FRAM_READ_DATA_CMD );				// send read data command.
		fram_spi_transceive( u.b.b1);							// send high order byte of address
		fram_spi_transceive( u.b.b0);							// send low  order byte of address
		for (; nSize > 0; nSize--)
			*pDest++ = fram_spi_transceive( 0 );				// read a byte of data and stored it.
		FRAM_CS_DESELECT;									// Chip select high to deselect the FRAM chip.
	}

} // end fram_read_bytes(,,)

//////////////////////////////////////////////////////////////////////////
/**
 * write a specified number of bytes to the specified FRAM address.
 *
 * @param  filler		-- a value to be fill.
 * @param  startAddr	-- starting address of FRAM chip ( internal memory address of FRAM chip).
 * @param  nSize		-- number of bytes to be fill.
 * @return none
 *
 * History:  Created on 2010-12-07 by Wai Fai Chin
 */

void  fram_fill_memory( BYTE filler, UINT16 startAddr, UINT16 nSize )
{
	GENERIC_UNION u;

	u.ui16.l = startAddr;
	if ( ( startAddr + nSize ) <= (FRAM_MAX_ADDR_RANGE + 1) )	{	// ensured that it will not wrap around.
		FRAM_CS_SELECT;												// Chip select low to select the FRAM chip.
		fram_spi_transceive( FRAM_WRITE_ENABLE_CMD );					// send write enabled command.
		FRAM_CS_DESELECT;											// Chip select high to deselect the FRAM chip. It will not work without this deselect.

		FRAM_CS_SELECT;												// Chip select low to select the FRAM chip.
		fram_spi_transceive( FRAM_WRITE_DATA_CMD );						// send write data command.
		fram_spi_transceive( u.b.b1);									// send high order byte of address
		fram_spi_transceive( u.b.b0);									// send low  order byte of address
		for (;  nSize > 0; nSize--)
			fram_spi_transceive( filler );
		FRAM_CS_DESELECT;											// Chip select high to deselect the FRAM chip.
	}

} // end fram_fill_memory(,,)

/**
 * write a specified number of bytes to the specified FRAM address.
 *
 * @param  cmpData		-- comparing value
 * @param  startAddr	-- starting address of FRAM chip ( internal memory address of FRAM chip).
 * @param  nSize		-- number of bytes to be written.
 * @return 0 if no error else CMD_STATUS_FRAM_FAIL;
 *
 * History:  Created on 2010-12-07 by Wai Fai Chin
 */

BYTE  fram_compare_memory( BYTE cmpData, UINT16 startAddr, UINT16 nSize )
{
	BYTE status;
	GENERIC_UNION u;
	
	u.ui16.l = startAddr;
	status = 0;
	if ( ( startAddr + nSize ) <= (FRAM_MAX_ADDR_RANGE + 1) )	{	// ensured that it will not wrap around.
		FRAM_CS_SELECT;										// Chip select low to select the FRAM chip.
		fram_spi_transceive( FRAM_READ_DATA_CMD );				// send read data command.
		fram_spi_transceive( u.b.b1);							// send high order byte of address
		fram_spi_transceive( u.b.b0);							// send low  order byte of address
		for (; nSize > 0; nSize--) {
			if ( cmpData != fram_spi_transceive( 0 ) ) {		// read a byte of data and comparing it
				status = CMD_STATUS_FRAM_FAIL;					// not equal cmpData, failed.
				break;
			}
		}
		FRAM_CS_DESELECT;									// Chip select high to deselect the FRAM chip.
	}
	
	return status;
} // end fram_compare_memory(,,)


/**
 * read status register of FRAM chip.
 *
 * @return status of FRAM chip.
 *
 * History:  Created on 2010-12-14 by Wai Fai Chin
 */

BYTE  fram_read_status( void )
{

	BYTE status;
	FRAM_CS_SELECT;											// Chip select low to select the FRAM chip.
	fram_spi_transceive( FRAM_READ_STATUS_CMD );
	status = fram_spi_transceive( 0 );
	FRAM_CS_DESELECT;										// Chip select high to deselect the FRAM chip.

	return status;
} // end fram_read_status()


/*
void fram_test_write( void )
{
	BYTE i;
	BYTE *pB;
	pB = &gTestStrFNV;
	for ( i=48; i<123; i++)
		*pB++ = i;
	*pB = 0;
	
	fram_write_bytes( 0, &gTestStrFNV, 76 );
}


void fram_test_read( void )
{
	
	fram_read_bytes(  &gTestStrFNV, 0, 76 );
}
*/

