/*! \file commonlib.h \brief common useful libraries.*/
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
// Compiler: any C
// Software layer:  Application
//
//  History:  Created on 2007/07/10 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup commonlib Common libraries functions (commonlib.c)
/// \code #include "parser.h" \endcode
/// \par Overview
///   Home grown common useful functions like binary to BCD, to Hex and 16bits divide etc..
///   This will avoid use the standard C library because of its memory hog.
//
// ****************************************************************************
//@{


#ifndef MSI_COMMONLIB_H
#define MSI_COMMONLIB_H
#include "config.h"

/// found a match
#define		MSI_LIB_FOUND_MATCH	0xFF00
/// found a start marker before found the specified match.
#define		MSI_LIB_FOUND_START	0xF000


/// for compare floating point value around 0.0.
#define		FLOAT_0_RESOLUTION	0.0001

/// for compare two floating point value
#define		FLOAT_RESOLUTION	0.0002

#if defined( CONFIG_USE_DOXYGEN )
/*
 * Doxygen doesn't append attribute syntax of
 * GCC, and confuses the typedefs with function decls, so
 * supply a doxygen-friendly view.
 */
extern const char PROGMEM gcStrFmtDash10LfCr_P[];
extern const char PROGMEM gcStrFmtUnCalLfCr_P[];
extern const char PROGMEM gcStrFmtBLoadLfCr_P[];
extern const char PROGMEM gcStrFmtLfCr_G_l_P[];
extern const char PROGMEM gcStrFmtLfCr_G_G_P[];

extern const char PROGMEM gcStrFmt_pct_d [];
extern const char PROGMEM gcStrFmt_pct_f [];
extern const char PROGMEM gcStrFmt_pct_G [];
extern const char PROGMEM gcStrFmt_pct_ld[];
extern const char PROGMEM gcStrFmt_pct_lu[];
extern const char PROGMEM gcStrFmt_pct_u [];
extern const char PROGMEM gcStrFmt_pct_ud[];
extern const char PROGMEM gcStrFmt_pct_s [];
extern const char PROGMEM gcStrFmt_pct_X [];
extern const char PROGMEM gcStrFmt_pct_02X[];

extern const char PROGMEM gcStrFmt_pct_lx_ld_ld[];
extern const char PROGMEM gcStrFmt_PacketHdr_Cmd[];
extern const char PROGMEM gcStrFmt_pct_d_c [];
extern const char PROGMEM gcStr_CR_LF[];

extern const char PROGMEM gcStrDefaultSensorOutFormat[];
extern const char PROGMEM gcStr5Spaces[];

#else // not DOXYGEN
extern const char gcStrFmtDash10LfCr_P[] PROGMEM;
// -WFC- 2011-03-03 extern const char gcStrFmtUnCalLfCr_P[] PROGMEM;
// -WFC- 2011-03-03 extern const char gcStrFmtBLoadLfCr_P[] PROGMEM;
extern const char gcStrFmtLfCr_G_l_P[] PROGMEM;
extern const char gcStrFmtLfCr_G_G_P[] PROGMEM;

extern const char gcStrFmt_pct_d [] PROGMEM;
extern const char gcStrFmt_pct_f [] PROGMEM;
extern const char gcStrFmt_pct_G [] PROGMEM;
extern const char gcStrFmt_pct_ld[] PROGMEM;
extern const char gcStrFmt_pct_lu[] PROGMEM;
extern const char gcStrFmt_pct_u [] PROGMEM;
extern const char gcStrFmt_pct_ud[] PROGMEM;
extern const char gcStrFmt_pct_s [] PROGMEM;
extern const char gcStrFmt_pct_X [] PROGMEM;
extern const char gcStrFmt_pct_02X[] PROGMEM;

extern const char gcStrFmt_pct_lx_ld_ld[]  PROGMEM;
extern const char gcStrFmt_PacketHdr_Cmd[] PROGMEM;
extern const char gcStrFmt_pct_d_c [] PROGMEM ;
extern const char gcStr_CR_LF[]	PROGMEM;

extern const char gcStrDefaultSensorOutFormat[] PROGMEM;
extern const char gcStrDefaultLoadcell_gnt_OutFormat[] PROGMEM;		// grossWt; netWt; tareWt; numLift|numTtlCnt; iCb; decPoint; unit; numSeg; 2011-08-10 -WFC-
extern const char gcStrDefaultLoadcell_vcb_OutFormat[] PROGMEM;		// valueType; data; numLift|numTtlCnt; iCb; decPoint; unit; 2012-02-13 -WFC-
extern const char gcStr5Spaces[]	PROGMEM;
extern const char gcStr6Spaces[]	PROGMEM;					// 2011-09-21 -WFC-

extern const char gcStrFmt_pct_4p2f [] PROGMEM;			// 2011-04-07 -WFC-
extern const char gcStrFmt_pct_4sNewLine [] PROGMEM;	// 2011-04-11 -WFC-

#endif // CONFIG_USE_DOXYGEN

/*!
  \brief Structure of circular buffer of byte type.

  \note For save memory space, this only support up to 255 elements. To increase support
   larger buffer size, changed writeIndex, readIndex, peekIndex and size from BYTE type to UINT16 or UINT32 type.
  
	Set writeIndex = readIndex = 0;	for normal circular buffer, first in first out without delay. \n
	Set writeIndex = 0; readIndex = 1; 	for use as a special circular buffer, first in first out with delay of entir buffer size.
  
*/

typedef struct BYTE_CIRCULAR_BUF_TAG {
							/// write index to dataPtr buffer.
  BYTE	writeIndex;			// write index to dataPtr buffer.
							/// read index
  BYTE	readIndex;			// read index
							/// peek index
  BYTE	peekIndex;			// peek index
							/// is data buffer full, TRUE or FALSE
  BYTE	isFull;				// is data buffer full, TRUE or FALSE
							/// size of data buffer
  BYTE	size;				// size of data buffer
							/// point to data buffer
  BYTE  *dataPtr;			// point to data buffer
}BYTE_CIRCULAR_BUF_T;



/*!
  \brief Structure of circular buffer of 32bits integer type.

  \note For save memory space, this only support up to 255 elements. To increase support
   larger buffer size, changed writeIndex and readIndex from BYTE type to UINT16 or UINT32 type.
  
	Set writeIndex = readIndex = 0;	for normal circular buffer, first in first out without delay. \n
	Set writeIndex = 0; readIndex = 1; 	for use as a special circular buffer, first in first out with delay of entir buffer size.
  
*/

typedef struct INT32_CIRCULAR_BUF_TAG {
							/// write index to dataPtr buffer.
  BYTE	writeIndex;			// write index to dataPtr buffer.
							/// read index
  BYTE	readIndex;			// read index
							/// is data buffer full, TRUE or FALSE
  BYTE	isFull;				// is data buffer full, TRUE or FALSE
  //BYTE  unused; allocate this for 16bits or higher CPU chip.
							/// size of data buffer
  UINT16 size;				// size of data buffer
							/// point to data buffer
  INT32  *dataPtr;			// point to data buffer
}INT32_CIRCULAR_BUF_T;



/*!
  \brief Structure of simple running moving average filter to compute in 32bits integer type.
    It only uses one variable prvRunAvg to hold the filtered value regardless of window size.
*/

typedef struct RUNNING_AVG_FILTER32_TAG {
								/// RunAvg(n-1)  previous running average
  INT32  prvRunAvg;				//  RunAvg(n-1)
								/// accumulated difference between (Sample(n) - RunAvg(n-1)) = accDiff. // for detecte step change, motion, stable, etc..
  INT32	 accDiff;				//  accumulated difference between (Sample(n) - RunAvg(n-1)) = accDiff. // for detecte step change, motion, stable, etc..
								/// accumulated difference between (Sample(n) - RunAvg(n-1)) = accDiff. for compensate rounding error.
  INT32	 accDiffRoundOff;		//  accumulated difference between (Sample(n) - RunAvg(n-1)) = accDiff. for compensate rounding error.
}RUNNING_AVG_FILTER32_T;


// DATA Manipulation functions.
void	cml_build_c_formatter( BYTE *pFormat, INT8 width, BYTE precision, BYTE type );	// 2011-06-21 -WFC-
BYTE	copy_until_match_char(		 char *pDest, const char *pSrc, char match, BYTE maxLen );
BYTE	copy_until_match_char_xnull( char *pDest, const char *pSrc, char match, BYTE maxLen );
char*	find_a_char_in_string( char *pStr, char match );
BYTE	hex_char_to_nibble( BYTE ch );
INT8	is_a_led_number( const BYTE *pbStr );
void	fill_string_buffer( char *pStr, char ch, BYTE n);

void	advance_readindex_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE len );
void	init_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *ptrDataBuf, BYTE fillValue, BYTE size);
BYTE 	read_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *ptrData );
BYTE	write_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE data );
BYTE	write_bytes_to_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, const BYTE *pbSrc, BYTE n );
BYTE	read_bytes_from_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *pbDest, BYTE n );
void	reset_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir);
BYTE	peek_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *ptrData );
BYTE 	scan_left_side_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE match );
UINT16  scan_right_side_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE match, BYTE startCh );

void	init_int32_circular_buf( INT32_CIRCULAR_BUF_T *ptrCir, INT32 *ptrDataBuf, INT32 fillValue, UINT16 size);
BYTE 	read_int32_circular_buf( INT32_CIRCULAR_BUF_T *ptrCir, INT32 *ptrData );
void	write_int32_circular_buf( INT32_CIRCULAR_BUF_T *ptrCir, INT32 data );

// math functions

// float	adc_to_value( INT32 adcCnt, INT32 *pCalAdcCntTable, float *pCalValueTable );

float	float_round(float fvalue, float increment);
void	float_format( BYTE *pFormat, BYTE width, BYTE precision);
BYTE	float_round_to_string( float fV, MSI_CB *pCB, BYTE width, BYTE *pOutStr);

BYTE	float_eq_zero( float value);
BYTE	float_gt_zero( float value);
BYTE	float_gte_zero( float value);
BYTE	float_lt_zero( float value);
BYTE	float_lte_zero( float value);

BYTE	float_a_eq_b( float fA, float fB);
BYTE	float_a_gt_b( float fA, float fB);
BYTE	float_a_gte_b( float fA, float fB);
BYTE	float_a_lt_b( float fA, float fB);
BYTE	float_a_lte_b( float fA, float fB);

// filter functions;
INT32	running_avg32( INT32 curSample, INT32 prvRunAvg,
					 INT32 *pAccDiff, INT32 *pAccDiffRoundoff, BYTE windowSizePower );

INT32	running_avg32_step_threshold( INT32 curSample, INT32 prvRunAvg, INT32 stepThreshold,
					 INT32 *pAccDiff, BYTE windowSizePower );


void	soft_delay( WORD16 wDelay );



/** \def rewind_peekindex_byte_circular_buf
 * It rewinds the peekindex to the same as the read index.
 *
 * @param	pCirBuf	-- pointer to circular buffer
 *
 * History:  Created on 2009/04/02 by Wai Fai Chin
 */
#define  rewind_peekindex_byte_circular_buf( pCirBuf )   (pCirBuf)-> peekIndex = (pCirBuf)-> readIndex


#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )
BYTE	copy_until_match_char_P( char *pDest, const char *pSrc, char match, BYTE maxLen );
BYTE	copy_until_match_char_xnull_P( char *pDest, const char *pSrc, char match, BYTE maxLen );
BYTE	write_bytes_to_byte_circular_buf_P( BYTE_CIRCULAR_BUF_T *ptrCir, const BYTE *pbSrc, BYTE n );
float	adc_to_value( INT32 adcCnt, INT32 *pCalAdcCntTable, float *pCalValueTable );
INT32	get_constant_cal( INT32 *pCalAdcCntTable, float *pCalValueTable, float capacity );		// 2011-04-28 -WFC-
#endif // #if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )


#if ( CONFIG_COMPILER == CONFIG_USE_PC_GCC )
	#define	DEBUG_TRACE( s, p )	printf((s), (p))
#else
	#define	DEBUG_TRACE( s, p )
#endif // #if ( CONFIG_COMPILER == CONFIG_USE_PC_GCC )

#endif	// MSI_COMMONLIB_H
//@}
