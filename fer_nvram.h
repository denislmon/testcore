/*! \file fer_nvram.h \brief external ferroelectric nonvolatile ram related functions.*/
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
//  History:  Created on 2008/12/26 by Wai Fai Chin
// 
/// \ingroup driver_cpu_extern
/// \defgroup fer_nvram manage external ferroelectric nonvolatile ram chip FM25L16.(fer_nvram.c)
/// \code #include "fer_nvram.h" \endcode
/// \par Overview
//
// ****************************************************************************
//@{
 

#ifndef MSI_FER_NVRAM_H
#define MSI_FER_NVRAM_H

#include	"config.h"
#include	"bios.h"

/*!

	WRITE OPERATION:
	
	First, a FRAM_WRITE_ENABLE_CMD op-code must be issued prior
	to any write operation.
	
	All writes to the memory array begin with a FRAM_WRITE_ENABLE_CMD
	op-code. The next op-code is the FRAM_WRITE_DATA_CMD instruction.
	This op-code is followed by a two-byte address
	value. The upper 5-bits of the address are ignored. In
	total, the 11-bits specify the address of the first data
	byte of the write operation. Subsequent bytes are data
	and they are written sequentially. Addresses are
	incremented internally as long as the bus master
	continues to issue clocks.	If the last address of 7FFh
	is reached, the counter will roll over to 000h. Data is
	written MSB first.

	
	READ OPERATION:
	After the falling edge of /CS, the bus master can issue
	a FRAM_READ_DATA_CMD op-code. Following this instruction
	is a twobyte address value. The upper 5-bits of the address
	are ignored. In total, the 11-bits specify the address of
	the first byte of the read operation. After the op-code
	and address are complete, the SI line is ignored. The
	bus master issues 8 clocks, with one bit read out for
	each. Addresses are incremented internally as long as
	the bus master continues to issue clocks. If the last
	address of 7FFh is reached, the counter will roll over
	to 000h. Data is read MSB first. The rising edge of
	/CS terminates a READ op-code operation.

*/

#define FRAM_WRITE_ENABLE_CMD	0X06
#define FRAM_WRITE_DISABLE_CMD	0X04
#define FRAM_WRITE_STATUS_CMD	0X01
#define FRAM_WRITE_DATA_CMD		0X02
#define FRAM_READ_STATUS_CMD	0X05
#define FRAM_READ_DATA_CMD		0X03

#define FRAM_STATUS_UNPROTECT	0

#if ( CONFIG_PCB_AS == CONFIG_PCB_AS_SCALECORE2)
/// FM25CL64
#define FRAM_MAX_ADDR_RANGE		0X1FFF
#else
/// FM25L16
#define FRAM_MAX_ADDR_RANGE		0X7FF
#endif

// extern BYTE gTestStrFNV[81];


#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )

/*!
  AVR CPU IO PORT for FRAM chip select.
*/
#define FRAM_CS_PORT_OUT_REG	PORTD

/*!
  AVR CPU IO PORT pin for FRAM chip select.
  Usage:
  \code
	// set chip select LOW to select the ADC chip.
	#define	FRAM_CS_SELECT	FRAM_CS_PORT_OUT_REG &= ~FRAM_CS_bm

	// set chip select HIGH to deselect the ADC chip.
	#define	FRAM_CS_DESELECT	FRAM_CS_PORT_OUT_REG |= FRAM_CS_bm
  \endcode
  
*/
#define FRAM_CS_bm		(1 << PD6 )

#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 )

#define FRAM_CS_PORT_OUT_REG	(&VPORT0)->OUT

#define FRAM_CS_bm		BIOS_PIN7_bm
// 2013-04-17 -WFC- v
#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )

#define FRAM_CS_PORT_OUT_REG	(&PORTD)->OUT

#define FRAM_CS_bm		BIOS_PIN1_bm
// 2013-04-17 -WFC- ^
#endif // ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281 )

/// set chip select LOW to select the ADC chip.
#define	FRAM_CS_SELECT	FRAM_CS_PORT_OUT_REG &= ~FRAM_CS_bm

/// set chip select HIGH to deselect the ADC chip.
#define	FRAM_CS_DESELECT	FRAM_CS_PORT_OUT_REG |= FRAM_CS_bm

#define FRAM_BASE	0
#define	gafSensorShowCapacityFRAM	0

#define  FRAM_TYPE_UINT8    1
#define  FRAM_TYPE_BYTE     1
#define  FRAM_TYPE_INT8     2
#define  FRAM_TYPE_UINT16   3
#define  FRAM_TYPE_INT16    4
#define  FRAM_TYPE_UINT32   5
#define  FRAM_TYPE_INT32    6
#define  FRAM_TYPE_FLOAT    7
#define  FRAM_TYPE_STRING   8
#define  FRAM_TYPE_1D		0X80



/*!
 \brief structure for map SRAM variables onto FRAM.
*/
typedef struct FRAM_SRAM_MAP_DESC_TAG {
						///  data type
  BYTE		type;
						///  array size
  BYTE		arraySize;
						///  data type
  UINT16	framAddr;
						/// points to variable in SRAM
  void		*pV;
}FRAM_SRAM_MAP_DESC_T;


void  fram_init( void );
void  fram_read_bytes( BYTE *pDest, UINT16 sourceAddr, UINT16 nSize );
void  fram_write_bytes( UINT16 destAddr, BYTE *pSrc, UINT16 nSize );
void  fram_fill_memory( BYTE filler, UINT16 startAddr, UINT16 nSize );
BYTE  fram_compare_memory( BYTE cmpData, UINT16 startAddr, UINT16 nSize );
BYTE  fram_read_status( void );

//void fram_test_read( void );
//void fram_test_write( void );


#endif  // MSI_FER_NVRAM_H

//@}
