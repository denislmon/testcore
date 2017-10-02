/*! \file commonlib.c \brief common useful libraries.*/
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
// Software layer:  Application
//
//  History:  Created on 2007/07/10 by Wai Fai Chin
// 
//   Home grown common useful functions like binary to BCD, to Hex and 16bits divide etc..
//   This will avoid use the standard C library because of its memory hog.
//
// ****************************************************************************


#include <stdlib.h>
#include "commonlib.h"

#define CMD_END_CHAR '}'
#include <stdio.h>

#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )
#include "calibrate.h"		// for MAX_CAL_POINTS defined
#include "cmdparser.h"		// for CMD_END_CHAR

#if defined( CONFIG_USE_DOXYGEN )
/*
 * Doxygen doesn't append attribute syntax of
 * GCC, and confuses the typedefs with function decls, so
 * supply a doxygen-friendly view.
 */

const char PROGMEM gcStrFmtDash10LfCr_P[] = "----------\n\r";
// -WFC- 2011-03-03 const char PROGMEM gcStrFmtBLoadLfCr_P[] = "bLOAD \n\r";	//	PHJ
// -WFC- 2011-03-03 const char PROGMEM gcStrFmtUnCalLfCr_P[] = "unCAL \n\r";	//	PHJ
const char PROGMEM gcStrFmtLfCr_G_l_P[] = "\n\r%G; %ld";
const char PROGMEM gcStrFmtLfCr_G_G_P[] = "\n\r%G; %G";

const char PROGMEM gcStrFmt_pct_d [] = "%d";
const char PROGMEM gcStrFmt_pct_f [] = "%f";
const char PROGMEM gcStrFmt_pct_G [] = "%G";
const char PROGMEM gcStrFmt_pct_ld[] = "%ld";
const char PROGMEM gcStrFmt_pct_lu[] = "%lu";
const char PROGMEM gcStrFmt_pct_u [] = "%u";
const char PROGMEM gcStrFmt_pct_ud[] = "%ud";
const char PROGMEM gcStrFmt_pct_s [] = "%s";
const char PROGMEM gcStrFmt_pct_X [] = "%X";
const char PROGMEM gcStrFmt_pct_02X [] = "%02X";

const char PROGMEM gcStrFmt_pct_lx_ld_ld[] = "%lx; %ld; %ld";
const char PROGMEM gcStrFmt_PacketHdr_Cmd[]	= "%c%02X%02X%02X";
const char PROGMEM gcStrFmt_pct_d_c [] = "%d%c";
const char PROGMEM gcStr_CR_LF[]	= "\r\n";

const char PROGMEM gcStrDefaultSensorOutFormat[] = "0;0;";	// data;unit;

const char PROGMEM gcStr5Spaces[]	= "     ";

#else // not DOXYGEN

const char gcStrFmtDash10LfCr_P[] PROGMEM = "----------\n\r";
// -WFC- 2011-03-03 const char gcStrFmtUnCalLfCr_P[] PROGMEM = "unCAL \n\r";	//	PHJ
// -WFC- 2011-03-03 const char gcStrFmtBLoadLfCr_P[] PROGMEM = "bLOAD \n\r";	//	PHJ
const char gcStrFmtLfCr_G_l_P[] PROGMEM = "\n\r%G; %ld";
const char gcStrFmtLfCr_G_G_P[] PROGMEM = "\n\r%G; %G";

const char gcStrFmt_pct_d [] PROGMEM = "%d";
const char gcStrFmt_pct_f [] PROGMEM = "%f";
const char gcStrFmt_pct_G [] PROGMEM = "%G";
const char gcStrFmt_pct_ld[] PROGMEM = "%ld";
const char gcStrFmt_pct_lu[] PROGMEM = "%lu";
const char gcStrFmt_pct_u [] PROGMEM = "%u";
const char gcStrFmt_pct_ud[] PROGMEM = "%ud";
const char gcStrFmt_pct_s [] PROGMEM = "%s";
const char gcStrFmt_pct_X [] PROGMEM = "%X";
const char gcStrFmt_pct_02X [] PROGMEM = "%02X";

const char gcStrFmt_pct_lx_ld_ld[] PROGMEM = "%lx;%ld;%ld;";
const char gcStrFmt_PacketHdr_Cmd[]PROGMEM = "%c%02X%02X%02X";
const char gcStrFmt_pct_d_c [] PROGMEM 	= "%d%c";
const char gcStr_CR_LF[]	PROGMEM = "\r\n";

// 2011-05-04 -WFC- const char gcStrDefaultSensorOutFormat[] PROGMEM = "0;0;";	// data;unit;
const char gcStrDefaultSensorOutFormat[] PROGMEM = "0;0;0;";	// data;unit;numLitSeg; 2011-05-04 -WFC-

const char gcStrDefaultLoadcell_gnt_OutFormat[] PROGMEM = "0;0;0;0;0;0;0;0;";	// grossWt; netWt; tareWt; numLift|numTtlCnt; iCb; decPoint; unit; numSeg; 2011-08-10 -WFC-
const char gcStrDefaultLoadcell_vcb_OutFormat[] PROGMEM = "0;0;0;0;0;0;";		// valueType; data; numLift|numTtlCnt; iCb; decPoint; unit; 2012-02-13 -WFC-

const char gcStr5Spaces[]	PROGMEM = "     ";
const char gcStr6Spaces[]	PROGMEM = "      ";					// 2011-09-21 -WFC-

const char gcStrFmt_pct_4p2f	  [] PROGMEM = "%4.2f";			// 2011-04-07 -WFC-
const char gcStrFmt_pct_4sNewLine [] PROGMEM = " %4s\r\n";		// 2011-04-11 -WFC-

#endif

#else	// use PC GCC

const char gcStrFmtDash10LfCr_P[] PROGMEM = "----------\n\r";
// -WFC- 2011-03-03 const char gcStrFmtUnCalLfCr_P[] PROGMEM = "unCAL \n\r";	//	PHJ
// -WFC- 2011-03-03 const char gcStrFmtBLoadLfCr_P[] PROGMEM = "bLOAD \n\r";	//	PHJ
const char gcStrFmtLfCr_G_l_P[] PROGMEM = "\n\r%G; %ld";
const char gcStrFmtLfCr_G_G_P[] PROGMEM = "\n\r%G; %G";

const char gcStrFmt_pct_d [] PROGMEM = "%d";
const char gcStrFmt_pct_f [] PROGMEM = "%f";
const char gcStrFmt_pct_G [] PROGMEM = "%G";
const char gcStrFmt_pct_ld[] PROGMEM = "%ld";
const char gcStrFmt_pct_lu[] PROGMEM = "%lu";
const char gcStrFmt_pct_u [] PROGMEM = "%u";
const char gcStrFmt_pct_ud[] PROGMEM = "%ud";
const char gcStrFmt_pct_s [] PROGMEM = "%s";
const char gcStrFmt_pct_X [] PROGMEM = "%X";
const char gcStrFmt_pct_02X [] PROGMEM = "%02X";

const char gcStrFmt_pct_lx_ld_ld[] PROGMEM = "%lx;%ld;%ld;";
const char gcStrFmt_PacketHdr_Cmd[]PROGMEM = "%c%02X%02X%02X";
const char gcStrFmt_pct_d_c [] PROGMEM 	= "%d%c";
const char gcStr_CR_LF[]	PROGMEM = "\r\n";

const char gcStrDefaultSensorOutFormat[] PROGMEM = "0;0;";	// data;unit;

const char gcStr5Spaces[]	PROGMEM = "     ";

#endif // #if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )


/**
 * Convert hex character to a binary nibble value.
 *
 * @param ch -- hex character to be convert to a binary nibble value.
 * @return 0xFF if it is a invalid hex character.
 * 
 * History:  Created on 2006/11/08 by Wai Fai Chin
 */


BYTE  hex_char_to_nibble( BYTE ch )
{
   BYTE b;

   if ( ch >= '0' && ch <= '9')
      b = ch - '0';
   else if( ch >= 'A' && ch <= 'F')
      b = ch - '7';
   else if( ch >= 'a' && ch <= 'f')
      b = ch - 'W';
   else
     b = 0xFF;
   return b;
} // end hex_char_to_nibble()


/**
 * It finds the location of the given match char.
 *
 * @param    pStr -- points at source string
 * @param   match -- a match character to be look for.
 *
 * @return pointer points at the matched location or end of string.
 *
 * @note  pStr also changed too.
 *
 * History:  Created on 2006/11/08 by Wai Fai Chin
 */
char*  find_a_char_in_string( char *pStr, char match )
{
	BYTE ch;

	for (;;) {
		ch = *pStr++;
		if ( ch == match || ch == 0 ) break;
	}
	// return pStr--;
	return --pStr;
}


/**
 * It copies data from the source buffer to destination buffer until it encoutered
 * a match char or null or up to a max length. It also copied the null char or fill in null char if lenght < maxLen.
 * The matched char is replaced by the null char.
 *
 * @param    pDest -- points at destination buffer.
 * @param     pSrc -- points at source string
 * @param    match -- a match character to be look for.
 * @param   maxLen -- max length of string to copy even if it cannot find the match.
 *
 * @return number of bytes copied.
 *
 * @note  pSrc also changed too.
 *
 * History:  Created on 2006/11/08 by Wai Fai Chin
 */
BYTE  copy_until_match_char( char *pDest, const char *pSrc, char match, BYTE maxLen )
{
	BYTE ch;
	BYTE i;

	i=0;
	for (;;) {
		ch = *pDest++ = *pSrc++;
		i++;
		if ( ch == match || ch == 0 || CMD_END_CHAR == ch || i >= maxLen ) {
			if ( ch != 0 && i < maxLen ) {
				*(--pDest) = 0;					// marked the end of string
				--i;
			}
			break;
		}
	}
	return i;
} // end copy_until_match_char( ,,,)


/**
 * It copies data from the source buffer to destination buffer until it encoutered
 * a match char or null or up to a max length. The return number of byte not include the null char.
 *
 * @param    pDest -- points at destination buffer.
 * @param     pSrc -- points at source string
 * @param    match -- a match character to be look for.
 * @param   maxLen -- max length of string to copy even if it cannot find the match.
 *
 * @return number of bytes copied, excluded null char.
 *
 * @note  pSrc also changed too.
 *
 * History:  Created on 2006/11/08 by Wai Fai Chin
 */
BYTE  copy_until_match_char_xnull( char *pDest, const char *pSrc, char match, BYTE maxLen )
{
	BYTE ch;
	BYTE i;

	i=0;
	for (;;) {
		ch = *pDest++ = *pSrc++;
		i++;
		if ( ch == match || ch == 0 || CMD_END_CHAR == ch || i >= maxLen ) {
			if ( ch != 0 && i < maxLen ) {
				*(--pDest) = 0;					// marked the end of string
			}
			--i;
			break;
		}
	}
	return i;
} // end copy_until_match_char( ,,,)



#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )

/**
 * It copies data from the source buffer in program memory to destination buffer until it encoutered
 * a match char or null or up to a max length. It also copied the null char or fill in null char if lenght < maxLen.
 * The matched char is replaced by the null char.
 *
 * @param    pDest -- points at destination buffer.
 * @param     pSrc -- points at source string in program memory
 * @param    match -- a match character to be look for.
 * @param   maxLen -- max length of string to copy even if it cannot find the match.
 *
 * @return number of bytes copied.
 *
 * @note  pSrc also changed too.
 *
 * History:  Created on 2006/11/08 by Wai Fai Chin
 */
BYTE  copy_until_match_char_P( char *pDest, const char *pSrc, char match, BYTE maxLen )
{
	BYTE ch;
	BYTE i;

	i=0;
	for (;;) {
		// ch = *pDest++ = *pSrc++;
		ch = pgm_read_byte(pSrc++);
		*pDest++ = ch;
		i++;
		if ( ch == match || ch == 0 || CMD_END_CHAR == ch || i >= maxLen ) {
			if ( ch != 0 && i < maxLen ) {
				*(--pDest) = 0;					// marked the end of string
				--i;
			}
			break;
		}
	}
	return i;
} // end copy_until_match_char_P( ,,,)

/**
 * It copies data from the source buffer to destination buffer until it encoutered
 * a match char or null or up to a max length. The return number of byte not include the null char.
 *
 * @param    pDest -- points at destination buffer.
 * @param     pSrc -- points at source string in program memory
 * @param    match -- a match character to be look for.
 * @param   maxLen -- max length of string to copy even if it cannot find the match.
 *
 * @return number of bytes copied, excluded null char.
 *
 * @note  pSrc also changed too.
 *
 * History:  Created on 2008/08/27 by Wai Fai Chin
 */
BYTE  copy_until_match_char_xnull_P( char *pDest, const char *pSrc, char match, BYTE maxLen )
{
	BYTE ch;
	BYTE i;

	i=0;
	for (;;) {
		// ch = *pDest++ = *pSrc++;
		ch = pgm_read_byte(pSrc++);
		*pDest++ = ch;
		i++;
		if ( ch == match || ch == 0 || CMD_END_CHAR == ch || i >= maxLen ) {
			if ( ch != 0 && i < maxLen ) {
				*(--pDest) = 0;					// marked the end of string
			}
			--i;
			break;
		}
	}
	return i;
} // end copy_until_match_char_xnull_P( ,,,)


#endif //end if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )


/* *
 *   This is a software delay by loop through wDelay counts.
 * Each outer loop is about 1.0 milliseconds for the 11.052 MHz clock.
 * Each inner loop has 7 cpu cycles which is .633 micro seconds.
 * Thus, inner loop loops 1579 times for approximately every 1.0 milliseconds.
 *
 * @param  wDelay -- delay count. 1 count ~ 1 milliseconds
 *
 * @return meaningless.
 *
 * History:  Created on 2009/09/16 by Wai Fai Chin
 * /

WORD16 soft_delay( WORD16 wDelay )
{
  WORD16 i;
  WORD16 a;
  
  while ( wDelay--) {	// each outer loop is about 1 milliseconds
      for ( i=0; i < SOFTWARE_DELAY_LOOPS; i++ )	// with timer interrupt running 
		a = i + wDelay;
  }// end for (; wDelay != 0; wDelay-- )
  return a;
}// end soft_delay( WORD wDelay )
*/


/* just to document the C compiler generated code with s option setting for optimization.
 575               	.global	soft_delay
 577               	soft_delay:
 578               	.LFB12:
 579               	.LM61:
 580               	/ * prologue: frame size=0 * /
 581               	/ * prologue end (size=0) * /
 582               	.LVL46:
 583 0282 8F5C      		subi r24,lo8(-(1577))	;
 584 0284 984F      		sbci r25,hi8(-(1577))	;
 585               	.LVL47:
 586 0286 00C0      		rjmp .L65				;2 cycles
 587               	.LVL48:
 588               	.L66:
 589               	.LM62:
 590 0288 9C01      		movw r18,r24			;1
 591 028a 0197      		sbiw r24,1				;2
 592               	.L65:
 593 028c 47E0      		ldi r20,hi8(1577)		;1
 594 028e 8133      		cpi r24,lo8(1577)		;1
 595 0290 9407      		cpc r25,r20				;1
 596 0292 01F4      		brne .L66				;1
 597               	.LM63:
 598 0294 C901      		movw r24,r18			;1
 599               	.LVL49:
 600               	/ * epilogue: frame size=0 * /
 601 0296 0895      		ret

 GCC will optimized this routin to a single return instruction when turn on optimize option s.
 GCC is an amazing compiler, just to be careful it may optimize out of your original coding intention.
void soft_delay( WORD16 wDelay )
{
  WORD16 i;
  WORD16 a;
  
  while ( wDelay--) {	// each outer loop is about 1 milliseconds
      for ( i=0; i<1579; i++ )	// with timer interrupt running 
		a = i + wDelay;
  }// end for (; wDelay != 0; wDelay-- )
}// end soft_delay( WORD wDelay )
*/

/**
 *   This is a software delay by loop through wDelay counts.
 * Each outer loop is about 1.0 milliseconds for the 11.052 MHz clock.
 * Each inner loop has 10 cpu cycles which is .90481 micro seconds.
 * Thus, inner loop loops 1105 times for approximately every 1.0 milliseconds.
 *
 * @param  wDelay -- delay count. 1 count ~ 1 milliseconds
 *
 * @note embedded NOP asm instructions prevented compiler optimized out of my intention.
 *       It generated the codes I wanted with embedded asm instructions; it generated
 *       a double loops instead of optimized single loop. Now, we have a true
 *       software delay routine.
 * 
 * History:  Created on 2009/10/16 by Wai Fai Chin
 * 2011-08-30 -WFC- Scaled number of delay loops according to CPU system clock speed to ensure delay time is correct.
 * 2011-12-02 -WFC- Condictional compilation based on configured PCB hardware.
 */

void soft_delay( WORD16 wDelay )
{
  WORD16 i;
  // 2011-08-30 -WFC- v
  WORD16 numDelayLoops;
  #if ( CONFIG_PCB_AS	== CONFIG_PCB_AS_SCALECORE1 )		// 2011-12-02 -WFC-
  if ( 0 == CLKPR ) 														// if it is in normal cpu clock speed, then
	 numDelayLoops = SOFTWARE_DELAY_LOOPS;
  else
	 numDelayLoops = (SOFTWARE_DELAY_LOOPS / 8);
  #else
	 numDelayLoops = SOFTWARE_DELAY_LOOPS;					// 2011-12-02 -WFC-
  #endif
  // 2011-08-30 -WFC- ^

  while ( wDelay--) {	// each outer loop is about 1 milliseconds
	  // 2011-08-30 -WFC- for ( i=0; i < SOFTWARE_DELAY_LOOPS; i++ )	{// with timer interrupt running
	  for ( i=0; i < numDelayLoops; i++ )	{// with timer interrupt running // 2011-08-30 -WFC-
		asm("nop");
		asm("nop");
	  }
	  asm("nop");
  }// end for (; wDelay != 0; wDelay-- )
}// end soft_delay( WORD wDelay )

/*
 Embedded assembly instructions prevent GCC optimized this routin to a single return instruction when turn on optimize option s.

 642               	.global	soft_delay
 644               	soft_delay:
 645               	.LFB12:
 646               	.LM84:
 647               	.LVL74:
 648               	/ * prologue: function * /
 649               	/ * frame size = 0 * /
 650 02bc 00C0      		rjmp .L51
 651               	.LVL75:
 652               	.L53:
 653               	.LM85:
 654 02be 20E0      		ldi r18,lo8(0)			;1     // i=0; r19:r18==i.
 655 02c0 30E0      		ldi r19,hi8(0)			;1
 656               	.L52:
 657               	.LM86:
 658               	/ * #APP * /
 659               	 ;  384 "commonlib.c" 1
 660 02c2 0000      		nop
 661               	 ;  0 "" 2						;1
 662               	.LM87:
 663               	 ;  385 "commonlib.c" 1
 664 02c4 0000      		nop						;1
 665               	 ;  0 "" 2
 666               	.LM88:
 667               	/ * #NOAPP * /
 668 02c6 2F5F      		subi r18,lo8(-(1))		;1
 669 02c8 3F4F      		sbci r19,hi8(-(1))		;1
 670 02ca 46E0      		ldi r20,hi8(1105)		;1
 671 02cc 2B32      		cpi r18,lo8(1105)		;1
 672 02ce 3407      		cpc r19,r20				;1
 673 02d0 01F4      		brne .L52				;1
									;-------------------------
									;inner loop is  10 clock cycles.
 674               	.LM89:				
 675               	/ * #APP * /
 676               	 ;  387 "commonlib.c" 1
 677 02d2 0000      		nop						;1
 678               	 ;  0 "" 2
 679               	/ * #NOAPP * /
 680 02d4 0197      		sbiw r24,1				;2
 681               	.LVL76:
 682               	.L51:
 683               	.LM90:
 684 02d6 0097      		sbiw r24,0				;2
 685 02d8 01F4      		brne .L53				;1
									;-------------------------
									;outer loop is   6 clock cycles.
 686               	/ * epilogue start * /
 687               	.LM91:
 688 02da 0895      		ret

*/


/**
 * Init circular buffer such that it has a delay of buffer size.
 * First in first out type circular after the entire buffer is filled up.
 *
 * @param	ptrCir     -- point to circular buff object
 * @param	ptrDataBuf -- point to data buffer
 * @param	fillValue  -- initial fill in value
 * @param	size       -- size of the data buffer of circular buffer.
 *
 * @post initialize a specified circular buffer.
 *
 * @note
 *	Set writeIndex = readIndex = 0;	for normal circular buffer, first in first out without delay. \n
 *	Set writeIndex = 0; readIndex = 1; 	for use as a special circular buffer, first in first out with delay of entir buffer size.
 * 
 * History:  Created on 2007/12/18 by Wai Fai Chin
 */

void init_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *ptrDataBuf, BYTE fillValue, BYTE size)
{
	UINT16 i;

	ptrCir-> writeIndex = ptrCir-> readIndex = ptrCir-> peekIndex = 0;	// normal circular buffer, first in first out without delay.
	ptrCir-> size = size;
	ptrCir-> isFull = FALSE;
	ptrCir-> dataPtr = ptrDataBuf;
	for ( i=0; i < size; i++)
		*ptrDataBuf++ = fillValue;
}// end init_byte_circular_buf(,,,)



/**
 * reset a specified circular buffer so that the buffer is empty by reset read and write indexes.
 *
 * @param	ptrCir     -- point to circular buff object
 *
 * @post reset a specified circular buffer so that the buffer is empty.
 *
 * @note
 *	Set writeIndex = readIndex = 0;	for normal circular buffer, first in first out without delay. \n
 *	Set writeIndex = 0; readIndex = 1; 	for use as a special circular buffer, first in first out with delay of entir buffer size.
 * 
 * History:  Created on 2009/03/20 by Wai Fai Chin
 */

void reset_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir)
{
	ptrCir-> writeIndex = ptrCir-> readIndex = ptrCir-> peekIndex = 0;	// normal circular buffer, first in first out without delay.
	ptrCir-> isFull = FALSE;
}// end reset_byte_circular_buf(,,,)



/** 
 * It advances read index and peek index by a specific length.
 *
 * @param  ptrCir	-- pointer to IO_STREAM object.
 * @param  len		-- a positive number, length to be advance.
 *
 * History:  Created on 2009/04/06 by Wai Fai Chin
 */
void advance_readindex_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE len )
{
	UINT16 newIndex;
	UINT16 size;
	
	size = (UINT16) ptrCir-> size;
	newIndex = ptrCir-> readIndex + len;
	if ( newIndex >= size )
		newIndex -= size;
	
	ptrCir-> peekIndex = ptrCir-> readIndex = (BYTE) newIndex;
	ptrCir-> isFull = FALSE;
}

/*
      r
i	 0 1 2 3 4
	 p

		  r
i	 0 1 2 3 4
	 p

      r 
i	 0 1 2 3 4
	     p
*/

/**
 * write the given data into cicular buffer.
 *
 * @param	ptrCir	-- point to circular buff object
 * @param	data	-- input data
 *
 * @return 1 if full.
 * @post   updated ptrCir.
 *
 * History:  Created on 2007/12/18 by Wai Fai Chin
 */

BYTE write_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE data )
{
	if ( ! ptrCir-> isFull ) {
		ptrCir-> dataPtr[ ptrCir-> writeIndex ] = data;
		ptrCir-> writeIndex++;
		if ( ptrCir-> writeIndex >= ptrCir-> size )
			ptrCir-> writeIndex = 0;
		if ( ptrCir-> writeIndex == ptrCir-> readIndex  )
			ptrCir-> isFull = TRUE;
	}

	return (ptrCir-> isFull);
} // end write_byte_circular_buf();



/**
 * write specified number of bytes into cicular buffer.
 *
 * @param	ptrCir	-- point to circular buff object
 * @param	pbSrc	-- pointer to data source
 * @param	n		-- number of byte to be writen.
 *
 * @return number of byte wrote to the buffer.
 * @post   updated ptrCir.
 * @note Maxium buffer size is 255.
 *
 * History:  Created on 2009/03/27 by Wai Fai Chin
 */

BYTE write_bytes_to_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, const BYTE *pbSrc, BYTE n )
{
	BYTE wrn;
	
	for ( wrn=0; wrn < n;	wrn++) {
		if ( ! ptrCir-> isFull ) {
			ptrCir-> dataPtr[ ptrCir-> writeIndex ] = *pbSrc++;
			ptrCir->writeIndex++;
			if ( ptrCir->writeIndex >= ptrCir-> size )
				ptrCir->writeIndex = 0;
			if ( ptrCir-> writeIndex == ptrCir-> readIndex  ) {
				ptrCir-> isFull = TRUE;
				break;
			}
		}
	} // end for(;;)

	return wrn;			// tell caller that number of byte had written
} // end write_bytes_to_byte_circular_buf();


/**
 * read specified number of bytes into cicular buffer.
 *
 * @param	ptrCir	-- point to circular buff object
 * @param	pbDest	-- pointer to destination data buffer
 * @param	n		-- number of byte to be writen.
 *
 * @return number of byte read from the buffer.
 * @post   It advance both read and peek indexes.
 * @note Maxium buffer size is 255.
 *
 * History:  Created on 2009/04/09 by Wai Fai Chin
 */

BYTE read_bytes_from_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *pbDest, BYTE n )
{
	BYTE rn;
	
	for ( rn=0; rn < n;	rn++) {
		if ( ptrCir-> writeIndex != ptrCir-> readIndex || ptrCir-> isFull ) {
			*pbDest++ = ptrCir-> dataPtr[ ptrCir-> readIndex++ ];
			if ( ptrCir-> readIndex >= ptrCir-> size )
				ptrCir-> readIndex = 0;
			ptrCir-> isFull = FALSE;
		}
		else
			break;
	} 

	ptrCir-> peekIndex = ptrCir-> readIndex;

	return rn;			// tell caller that number of byte had written
} // end read_bytes_from_byte_circular_buf();



#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )

/**
 * write specified number of bytes from program space into cicular buffer.
 *
 * @param	ptrCir	-- point to circular buff object
 * @param	pbSrc	-- pointer to data source from program space.
 * @param	n		-- number of byte to be writen.
 *
 * @return number of byte wrote to the buffer.
 * @post   updated ptrCir.
 * @note Maxium buffer size is 255.
 *
 * History:  Created on 2009/03/27 by Wai Fai Chin
 */

BYTE write_bytes_to_byte_circular_buf_P( BYTE_CIRCULAR_BUF_T *ptrCir, const BYTE *pbSrc, BYTE n )
{
	BYTE wrn;
	
	for ( wrn=0; wrn < n;	wrn++) {
		if ( ! ptrCir-> isFull ) {
			ptrCir-> dataPtr[ ptrCir-> writeIndex ] = pgm_read_byte(pbSrc++);
			ptrCir-> writeIndex++;
			if ( ptrCir-> writeIndex >= ptrCir-> size )
				ptrCir-> writeIndex = 0;
			if ( ptrCir-> writeIndex == ptrCir-> readIndex  ) {
				ptrCir-> isFull = TRUE;
				break;
			}
		}
	} // end for(;;)

	return wrn;			// tell caller that number of byte had written
} // end write_bytes_to_byte_circular_buf_P();
#endif // ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )



/**
 * It reads data from cicular buffer.
 * First in first out type circular buffer.
 *
 * @param  ptrCir   -- point to circular buff object
 * @param  ptrData  -- point to caller's read variable which is an output of this function.
 *
 * @return TRUE if it has new data, else FALSE if it is empty.
 *          
 * @post   *ptrData has new read value from the circular buffer *ptrCir.
 * 			It increament read and peek indexes by 1.
 *
 * @note caller should check the status before accepted *ptrData;
 *
 * History:  Created on 2007/12/18 by Wai Fai Chin
 */

BYTE read_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *ptrData )
{
	BYTE status;

	status = TRUE;		// assumed buffer is not empty.
	if ( ptrCir-> writeIndex != ptrCir-> readIndex || ptrCir-> isFull ) {
		*ptrData = ptrCir-> dataPtr[ ptrCir-> readIndex ];
		ptrCir-> readIndex++;
		if ( ptrCir-> readIndex >= ptrCir-> size )
			ptrCir-> readIndex = 0;
		ptrCir-> isFull = FALSE;
	}
	else 
		status = FALSE;

	ptrCir-> peekIndex = ptrCir-> readIndex;
	return status;

} // end read_byte_circular_buf();


/**
 * It peeks a byte from cicular buffer without affect any read index.
 * First in first out type circular buffer.
 *
 * @param  ptrCir   -- point to circular buff object
 * @param  ptrData  -- point to caller's read variable which is an output of this function.
 *
 * @return TRUE if it has new data, else FALSE if it is empty.
 *          
 * @post   *ptrData has new read value from the circular buffer *ptrCir. Increamed peek index by 1.
 *
 * @note caller should check the status before accepted *ptrData;
 *
 * History:  Created on 2009/03/26 by Wai Fai Chin
 */

BYTE peek_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *ptrData )
{
	BYTE status;

	status = TRUE;		// assumed buffer is not empty.
	if ( ptrCir-> writeIndex != ptrCir-> peekIndex  || ptrCir-> isFull ) {
		*ptrData = ptrCir-> dataPtr[ ptrCir-> peekIndex ];
		ptrCir-> peekIndex++;
		if ( ptrCir-> peekIndex >= ptrCir-> size )
			ptrCir-> peekIndex = 0;
	}
	else 
		status = FALSE;

	return status;

} // end peek_byte_circular_buf();


/**
 * It looks for a specified match data on the left side of the packet.
 * It advances both read and peek indexes.
 * It is designe to look for the START of packet on the left side.
 *
 * @param  ptrCir	-- point to circular buff object
 * @param  match	-- match data to look for.
 *
 * @return TRUE if it found the match, the read index still point at the matched byte.
 *         peekIndex always advanced next one passed matched byte.
 *          
 * @post   the peek and read indexes were advanced at the same step if no match found.
 *         readIndex points at the matched data if a matched is found.
 *         peekIndex always advanced.
 *
 * History:  Created on 2009/03/26 by Wai Fai Chin
 */

BYTE scan_left_side_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE match )
{
	BYTE ch;
	BYTE peekIndex;
	BYTE status;

	status = FALSE;		// assumed no match is found.
	peekIndex = ptrCir-> peekIndex;
	for (;;) {
		if ( ptrCir-> writeIndex != peekIndex  || ptrCir-> isFull ) {
			ch = ptrCir-> dataPtr[ peekIndex ];
			peekIndex++;							// advance peek index
			if ( peekIndex >= ptrCir-> size )		// check for wrap around.
				peekIndex = 0;
			ptrCir-> peekIndex = peekIndex;
			
			if ( ch == match ) {
				status = TRUE;
				break;
			}
			else {
				ptrCir-> readIndex = peekIndex;	// advance the read index.
				ptrCir-> isFull = FALSE;
			}
		}
		else {		// empty, no more data
			break;
		}
	} // end for(;;)

	return status;
} // end scan_left_side_byte_circular_buf();


/**
 * It looks for both specified match, start marker data on the right side of the packet.
 * Peek index is always changed. If it found START marker before it found the END marker,
 * then it will advance the read index to that position.
 * It is designe to look for the END of packet on the right side.
 *
 * @param  ptrCir	-- point to circular buff object
 * @param  match	-- match data to look for.
 * @param  startCh -- start marker of a packet.
 *
 * @return 0xFFnn if it found the match. Where nn the lower order byte is the length from the readIndex or the 1st match data.
 *         0x00nn if it found NO match.
 *         0x0001 if the buffer is empty since the first match data.
 *         0xF0nn if it found NO match, but found another START marker
 *          
 * @post   the peek index advanced.
 *
 * @note   readIndex never changed.
 *         peekIndex always advanced.
 *
 * History:  Created on 2009/03/26 by Wai Fai Chin
 */

UINT16 scan_right_side_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE match, BYTE startCh )
{
	UINT16	status;
	BYTE	ch;
	BYTE	peekIndex;
	BYTE	readIndex;


	status = 0x00;		// assumed no match is found.
	peekIndex = ptrCir-> peekIndex;
	for (;;) {
		if ( ptrCir-> writeIndex != peekIndex   || ptrCir-> isFull) {
			ch = ptrCir-> dataPtr[ peekIndex ];
			readIndex = peekIndex;
			peekIndex++;
			if ( peekIndex >= ptrCir-> size )		// check for wrap around.
				peekIndex = 0;
			ptrCir-> peekIndex = peekIndex;
			
			if ( ch == match ) {
				status = MSI_LIB_FOUND_MATCH;			// found a matched
				break;
			}
			else if ( ch == startCh ) {
				status = MSI_LIB_FOUND_START;			// found a new START marker before it find the match char.
				ptrCir-> readIndex = readIndex;		// make readIndex point at the new START marker.
				ptrCir-> isFull = FALSE;
				break;
			}

			if ( ptrCir-> writeIndex == ptrCir-> peekIndex )
				break;
		}
		else {		// empty, no more data
			break;
		}
	} // end for(;;)

	
	ch = ptrCir-> readIndex;
	if ( peekIndex < ch ) {
		ch = ch - peekIndex;
		ch = ptrCir-> size - ch;
	}
	else {
		ch = peekIndex - ch;			// NOTE that peekIndex is always advance to a next unpeek data, thus it does not require to add ONE.
	}
	// now ch has the length from the read index.
	status |= ch;

	return status;
} // end scan_right_side_byte_circular_buf();


/*
      r
i	 0 1 2 3 4
	 p

		  r
i	 0 1 2 3 4
	 p

      r 
i	 0 1 2 3 4
	       p
*/

/**
 * Init circular buffer such that it has a delay of buffer size.
 * First in first out type circular after the entire buffer is filled up.
 *
 * @param	ptrCir     -- point to circular buff object
 * @param	ptrDataBuf -- point to data buffer
 * @param	fillValue  -- initial fill in value
 * @param	size       -- size of the data buffer of circular buffer.
 *
 * @post initialize a specified circular buffer.
 *
 * @note
 *	Set writeIndex = readIndex = 0;	for normal circular buffer, first in first out without delay. \n
 *	Set writeIndex = 0; readIndex = 1; 	for use as a special circular buffer, first in first out with delay of entir buffer size.
 * 
 * History:  Created on 2007/04/09 by Wai Fai Chin
 */

void init_int32_circular_buf( INT32_CIRCULAR_BUF_T *ptrCir, INT32 *ptrDataBuf, INT32 fillValue, UINT16 size)
{
	UINT16 i;

	// ptrCir-> writeIndex = ptrCir-> readIndex = 0;	// normal circular buffer, first in first out without delay.
	ptrCir-> writeIndex = 0;
	ptrCir-> readIndex = 1;	// special circular buffer, first in first out with delay of entir buffer size.
	ptrCir-> size = size;
	ptrCir-> isFull = FALSE;
	ptrCir-> dataPtr = ptrDataBuf;
	for ( i=0; i < size; i++)
		*ptrDataBuf++ = fillValue;
}// end init_int32_circular_buf(,,,)


/**
 * write the given data into cicular buffer.
 *
 * @param	ptrCir	-- point to circular buff object
 * @param	data	-- input data
 *
 * @return none.
 * @post   updated ptrCir.
 *
 * History:  Created on 2007/04/09 by Wai Fai Chin
 */

void write_int32_circular_buf( INT32_CIRCULAR_BUF_T *ptrCir, INT32 data )
{
	if ( ! ptrCir-> isFull ) {
		ptrCir-> dataPtr[ ptrCir-> writeIndex ] = data;
		ptrCir->writeIndex++;
		if ( ptrCir->writeIndex >= ptrCir-> size )
			ptrCir->writeIndex = 0;
		if ( ptrCir-> writeIndex == ptrCir-> readIndex  )
			ptrCir-> isFull = TRUE;
	}

} // end write_int32_circular_buf();


/**
 * It reads data from cicular buffer.
 * First in first out type circular buffer.
 *
 * @param  ptrCir   -- point to circular buff object
 * @param  ptrData  -- point to caller's read variable which is an output of this function.
 *
 * @return TRUE if it has new data, else FALSE if it is empty.
 *          
 * @post   *ptrData has new read value from the circular buffer *ptrCir. updated *ptrCir structure.
 *
 * @note caller should check the status before accepted *ptrData;
 *
 * History:  Created on 2007/04/09 by Wai Fai Chin
 */

BYTE read_int32_circular_buf( INT32_CIRCULAR_BUF_T *ptrCir, INT32 *ptrData )
{
	BYTE status;

	status = TRUE;		// assumed buffer is not empty.
	if ( ptrCir-> writeIndex != ptrCir-> readIndex || ptrCir-> isFull ) {
		*ptrData = ptrCir-> dataPtr[ ptrCir-> readIndex ];
		ptrCir-> readIndex++;
		if ( ptrCir-> readIndex >= ptrCir-> size )
			ptrCir-> readIndex = 0;
		ptrCir-> isFull = FALSE;
	}
	else 
		status = FALSE;

	return status;

} // end read_int32_circular_buf();

/* ****************************************************************************

			SIMPLE RUNNING AVERAGE FILTER

   ****************************************************************************
*/

/**
 * It computes running average in integer math only with stepwise change if accumulate
 * difference > stepthreshold.
 *
 * @param	curSample       -- current sample value.
 * @param	prvRunAvg       -- previous running average.
 * @param	stepThreshold   -- This function use this value to reload current sample as the new running average.
 *                              0xFFFFFFFF means no step change detection will perform.
 * @param	pAccDiff        -- accumulated difference of ( Sample(n) - RunAvg(n-1).
 * @param	windowSizePower -- filter window size in power of base 2.
 *
 * @return new RunAvg(n)= (Sample(n) - RunAvg(n-1))/n  if step delta is < stepThreshold else current Sample(n)
 *
 * @post updated *pAccDiff
 *
 * usage:
 * \code
 *       	gLcFilter0[lc].prvRunAvg = running_avg32_step_threshold( sampleData, gLcFilter0[lc].prvRunAvg, 1000, &gLcFilter0[lc].accDiff, 4);
 *       	gLcFilter1[lc].prvRunAvg = running_avg32_step_threshold( gLcFilter0[lc].prvRunAvg, gLcFilter1[lc].prvRunAvg, 1000, &gLcFilter1[lc].accDiff, 4);
 *       	triangleFilteredValue = gLcFilter1[lc].prvRunAvg;		// cascaded two running average filters to form a triangle filtered value.
 *
 * \endcode
 *
 * History:  Created on 2007/02/27 by Wai Fai Chin
 */

INT32 running_avg32_step_threshold( INT32 curSample, INT32 prvRunAvg, INT32 stepThreshold,
					 INT32 *pAccDiff, BYTE windowSizePower )
{
	INT32		sampleSUBprvRunAvg;
	INT32		stepDelta;			// absolute value of ( Sample(n) - RunAvg(n-1) )
	INT32		n;		  			// n = 2^divByNumShift.
	BYTE		sign;	  			// sign flag.

	sampleSUBprvRunAvg = (curSample - prvRunAvg);	// ( Sample(n) - RunAvg(n-1));
	*pAccDiff += sampleSUBprvRunAvg;				// added ( Sample(n) - RunAvg(n-1)) to difference accumulator.
													// The accumulate difference caused by the 0 quotient.
	stepDelta = labs( *pAccDiff );					// step changed value = absolute value of accumulated difference.

	sign = ( *pAccDiff > 0 ) ? 1 : 0;				// Determine sign for correction when residue exceeds n

	if ( (0xFFFFFFFF != stepThreshold) && (stepDelta >= stepThreshold)  ) {
		*pAccDiff = 0;				  	// clear accumulated difference.
   	    sampleSUBprvRunAvg = curSample;	// New running average =  RunAvg(n-1) + ( Sample(n) - RunAvg(n-1)) = Sample(n); use current sample as new RunAvg.
    }
    else {
		n = 1 << windowSizePower;					// n = 2^gbDivByNumShift
		sampleSUBprvRunAvg >>= windowSizePower;	// Now sampleSUBprvRunAvg = ( Sample(n) - RunAvg(n-1))/n

		if ( sampleSUBprvRunAvg == -1)		// the quotient is -1, then
			sampleSUBprvRunAvg++;			// add 1 to cancel it out.

		if ( stepDelta > n ) {				// if abs( *pAccDiff ) > n, then compensate rounding error.
			if ( sign ) { 					// if sign bit is positive
				*pAccDiff -= n;			// *pAccDiff -= n;
				sampleSUBprvRunAvg++;		// ( Sample(n) - RunAvg(n-1))/n + 1
			}
			else {
				*pAccDiff += n;			// *pAccDiff += n;
				sampleSUBprvRunAvg--;		// ( Sample(n) - RunAvg(n-1))/n - 1
			}
		} // end if ( stepDelta > n ) {}
		sampleSUBprvRunAvg += prvRunAvg;	// sampleSUBprvRunAvg = new RunAvg(n)= RunAvg(n-1) + ( Sample(n) - RunAvg(n-1))/n
	} // end else {}
	return sampleSUBprvRunAvg;				// return New RunAvg(n) or just  Sample(n) if stepDelta is > stepThreshold.
} // end running_avg32_step_threshold(,,,,)


/**
 * It computes running average in 32bit integer math only with no step change.
 *
 * @param	curSample  -- current sample value.
 * @param	prvRunAvg  -- previous running average.
 * @param	pAccDiff   -- accumulated difference of ( Sample(n) - RunAvg(n-1), use for raw data pattern detection. It never reset by this function.
 * @param	pAccDiffRoundoff -- accumulated difference of ( Sample(n) - RunAvg(n-1), use for compensate integer rounding error. It will reset it self if > 2^window size.
 * @param	windowSizePower  -- filter window size in power of base 2.
 *
 * @return  new RunAvg(n)= (Sample(n) - RunAvg(n-1))/n 
 *
 * @post  updated *pAccDiff and *pAccDiffRoundoff
 *
 * usage:
 * \code
 * 		gLcFilter0[lc].prvRunAvg = running_avg32( sample, gLcFilter0[lc].prvRunAvg,
 *					   			   &gLcFilter0[lc].accDiff,  &gLcFilter0[lc].accDiffRoundOff,
 *								   gFilterManager[lc].exportWindowSize);
 *		// cascade both filter0 and filter1 to form a triangle filter
 * 		gLcFilter1[lc].prvRunAvg = running_avg32( gLcFilter0[lc].prvRunAvg,	gLcFilter1[lc].prvRunAvg,
 *					   			   &gLcFilter1[lc].accDiff, &gLcFilter1[lc].accDiffRoundOff,
 *								   gFilterManager[lc].exportWindowSize);
 *		triangleFilteredValue = gLcFilter1[lc].prvRunAvg;
 *
 * \endcode
 *
 * History:  Created on 2007/02/27 by Wai Fai Chin
 */


INT32 running_avg32( INT32 curSample, INT32 prvRunAvg,
					 INT32 *pAccDiff, INT32 *pAccDiffRoundoff, BYTE windowSizePower )
{
	INT32		sampleSUBprvRunAvg;
	INT32		stepDelta;			// absolute value of ( Sample(n) - RunAvg(n-1) )
	INT32		n;		  			// n = 2^divByNumShift.
	BYTE		sign;	  			// sign flag.

	sampleSUBprvRunAvg = (curSample - prvRunAvg);	// ( Sample(n) - RunAvg(n-1));
	*pAccDiff += sampleSUBprvRunAvg;				// added ( Sample(n) - RunAvg(n-1)) to difference accumulator.
													// The accumulate difference caused by the 0 quotient.
	*pAccDiffRoundoff += sampleSUBprvRunAvg;
	stepDelta = labs( *pAccDiff );				// step changed value = absolute value of accumulated difference.

	sign = ( *pAccDiff > 0 ) ? 1 : 0;			// Determine sign for correction when residue exceeds n

	n = 1 << windowSizePower;					// n = 2^gbDivByNumShift
	sampleSUBprvRunAvg >>= windowSizePower;	// Now sampleSUBprvRunAvg = ( Sample(n) - RunAvg(n-1))/n

	if ( sampleSUBprvRunAvg == -1)		// the quotient is -1, then
		sampleSUBprvRunAvg++;			// add 1 to cancel it out.

	if ( stepDelta > n ) {				// if abs( *pAccDiff ) > n, then compensate rounding error.
		if ( sign ) { 					// if sign bit is positive
			*pAccDiff -= n;			// *pAccDiff -= n;
		}
		else {
			*pAccDiff += n;			// *pAccDiff += n;
		}
	} // end if ( stepDelta > n ) {}

	stepDelta = labs( *pAccDiffRoundoff );		// correction delta value = absolute value of accumulated difference.
	sign = ( *pAccDiffRoundoff > 0 ) ? 1 : 0;	// Determine sign for correction when residue exceeds n
	if ( stepDelta > n ) {				// if abs( *pAccDiffRoundoff ) > n, then compensate rounding error.
		if ( sign ) { 					// if sign bit is positive
			*pAccDiffRoundoff = 0;		
			sampleSUBprvRunAvg++;		// ( Sample(n) - RunAvg(n-1))/n + 1
		}
		else {
			*pAccDiffRoundoff = 0;		
			sampleSUBprvRunAvg--;		// ( Sample(n) - RunAvg(n-1))/n - 1
		}
	} // end if ( stepDelta > n ) {}
	
	sampleSUBprvRunAvg += prvRunAvg;	// sampleSUBprvRunAvg = new RunAvg(n)= RunAvg(n-1) + ( Sample(n) - RunAvg(n-1))/n
	return sampleSUBprvRunAvg;			// return New RunAvg(n) or just  Sample(n) if stepDelta is > stepThreshold.
} // end running_avg32(,,,,)


/**
 * It rounds input floating point value based on the specified increment value.
 *
 * @param  fValue		-- input floating point value to be round.
 * @param  increment	-- increment.
 *
 * @return rounded float pointing value.
 *          
 * History:  Created on 2007/11/05 by Wai Fai Chin
 */


float float_round(float fValue, float increment)
{
	INT32	iTmp;	
	float	fRound;

	if ( increment <= 0 )
		return fValue;
		
	if ( float_lt_zero( fValue ) )
		fRound = -0.5;
	else
		fRound =  0.5;

	iTmp = (INT32) ((fValue / increment) + fRound);
	fRound = (float)iTmp * increment;

	return fRound;
} // end float_round(,)


/**
 * Configured printf format for floating point print out.
 *
 * @param  pFormat		-- points to an output floating point format buffer.
 * @param  width		-- maxium number of digit plus decimal point.
 * @param  precision	-- number of digits right of decimal point
 *
 * @note caller must supplied a valid format string buffer with at least 6 bytes length.
 *       I have to write this function because GCC does not support parameters input of width and precision.
 *   
 * History:  Created on 2007/11/06 by Wai Fai Chin
 */


void float_format( BYTE *pFormat, BYTE width, BYTE precision)
{
   pFormat[0]='%';
   pFormat[1]='0' + width;
   pFormat[2]='.';
   pFormat[3]='0' + precision;
   pFormat[4]= 'f';
   pFormat[5]= 0;
   
} // end float_format(,,)

////
/**
 * It builds c formatter based on input parameters.
 *
 * @param  pFormat		-- points to an output floating point format buffer.
 * @param  width		-- width of a field. Negative number is Left Justify. 0== omitted.
 * @param  precision	-- number of digits right of decimal point. if type is 's' then 0 == omitted.
 * @param  type			-- data type, s==string, f==float, d=decimal integer, see printf format doc for more detail.
 *
 * @note caller must supplied a valid format string buffer with at least 12 bytes length.
 *       I have to write this function because GCC does not support parameters input of width and precision.
 *
 * History:  Created on 2011/01/21 by Wai Fai Chin
 */

void cml_build_c_formatter( BYTE *pFormat, INT8 width, BYTE precision, BYTE type )
{
	BYTE len;

	*pFormat = '%';
	len = 1;
	if ( 0 != width )
		len += sprintf_P( pFormat + len, gcStrFmt_pct_d, (int) width );

	if ( 0 != precision || 'f' == type) {
		pFormat[ len ]='.';
		len++;
		len += sprintf_P( pFormat + len, gcStrFmt_pct_u, (UINT16) precision);
	}
	pFormat[ len ]= type;
	len++;
	pFormat[ len ]= 0;
} // end cml_build_c_formatter(,,)


/**
 * format a floating point value to a string with specified width and precision through countby.
 *
 * @param  fV			-- input floating point value to be 
 * @param  pCB			-- points to input countby.
 * @param  width		-- maxium number of digit plus decimal point.
 * @param  pOutStr		-- points to an output string buffer floating point format buffer.
 *
 * @return	Length of the string.
 *
 * @note caller must supplied a valid format string buffer with at least > width.
 *   
 * History:  Created on 2009/06/05 by Wai Fai Chin
 */


BYTE float_round_to_string( float fV, MSI_CB *pCB, BYTE width, BYTE *pOutStr)
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	BYTE 	formatBuf[6];


	fRound = float_round( fV, pCB-> fValue);
	if ( pCB-> decPt > 0 )
		precision = pCB-> decPt;
	else
		precision = 0;
	float_format( formatBuf, width, precision);
	len = (BYTE) sprintf( pOutStr, formatBuf, fRound );
	
	return len;
   
} // end float_round_to_string(,,,)



/**
 * check if the value is equal to zero.
 *
 * @param  value -- floating point value to be check.
 *
 * @return true if value nearly equal to zero, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_eq_zero( float value)
{
	if ( (value <= FLOAT_0_RESOLUTION) &&
		 (value >= -FLOAT_0_RESOLUTION) )
		return TRUE;
	else
		return FALSE;
}

/**
 * check if the value is greater than zero.
 *
 * @param  value -- floating point value to be check.
 *
 * @return true if value greater than zero, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_gt_zero( float value)
{
	if ( value >=  FLOAT_0_RESOLUTION )
		return TRUE;
	else
		return FALSE;
}


/**
 * check if the value is greater or equal zero.
 *
 * @param  value -- floating point value to be check.
 *
 * @return true if value greater or nearly equal zero, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_gte_zero( float value)
{
	if ( value >= -FLOAT_0_RESOLUTION )
		return TRUE;
	else
		return FALSE;
}


/**
 * check if the value is lesser than zero.
 *
 * @param  value -- floating point value to be check.
 *
 * @return true if value lesser than zero, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_lt_zero( float value)
{
	if ( value <= -FLOAT_0_RESOLUTION )
		return TRUE;
	else
		return FALSE;
}


/**
 * check if the value is lesser or equal zero.
 *
 * @param  value -- floating point value to be check.
 *
 * @return true if value lesser or equal zero, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_lte_zero( float value)
{
	if ( value <=  FLOAT_0_RESOLUTION )
		return TRUE;
	else
		return FALSE;
}

/**
 * check if fA == fB.
 *
 * @param  fA -- floating point value A
 * @param  fB -- floating point value B
 *
 * @return true if fA == fB, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_a_eq_b( float fA, float fB)
{
	float fd;		// difference between A and B

	fd = fA - fB;
	if ( (fd <= FLOAT_RESOLUTION ) && ( fd >= -FLOAT_RESOLUTION))
		return TRUE;
	else
		return FALSE;
}

/**
 * check if fA > fB.
 *
 * @param  fA -- floating point value A
 * @param  fB -- floating point value B
 *
 * @return true if fA > fB, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_a_gt_b( float fA, float fB)
{
	float fd;		// difference between A and B. for debug too.

	fd = fA - fB;
	if ( fd > FLOAT_RESOLUTION )
		return TRUE;
	else
		return FALSE;
}

/**
 * check if fA >= fB.
 *
 * @param  fA -- floating point value A
 * @param  fB -- floating point value B
 *
 * @return true if fA >= fB, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_a_gte_b( float fA, float fB)
{
	float fd;		// difference between A and B. for debug too.

	fd = fA - fB;
	if ( fd >= -FLOAT_RESOLUTION )
		return TRUE;
	else
		return FALSE;
}


/**
 * check if fA < fB.
 *
 * @param  fA -- floating point value A
 * @param  fB -- floating point value B
 *
 * @return true if fA < fB, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_a_lt_b( float fA, float fB)
{
	float fd;		// difference between A and B. for debug too.

	fd = fA - fB;
	if ( fd < -FLOAT_RESOLUTION )
		return TRUE;
	else
		return FALSE;
}

/**
 * check if fA <= fB.
 *
 * @param  fA -- floating point value A
 * @param  fB -- floating point value B
 *
 * @return true if fA <= fB, else false.
 *   
 * History:  Created on 2007/11/14 by Wai Fai Chin
 */

BYTE	float_a_lte_b( float fA, float fB)
{
	float fd;		// difference between A and B. for debug too.

	fd = fA - fB;
	if ( fd <= FLOAT_RESOLUTION )
		return TRUE;
	else
		return FALSE;
}


#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )

/**
 * It computes value ( in weight, voltage etc...) based on ADC counts of a sensor.
 *
 * @param  adcCnt			-- filtered, and/or temperature, accelerated adjusted ADC count.
 * @param  pCalAdcCntTable	-- pointer to base of calibrated ADC count table.
 * @param  pCalValueTable	-- pointer to base of calibrated value table.
 *
 * @return value in float type.
 * 
 *
 * History:  Created on 2008/12/11 by Wai Fai Chin
 * 2011-09-15 -WFC- Fixed a rare bug happened when looked up multi point cal with end marker (did not filled up entire table) and when ADC count is greater than the last cal point.
 */


float  adc_to_value( INT32 adcCnt, INT32 *pCalAdcCntTable, float *pCalValueTable )
{
	float	fV;
	INT32	i32;
	BYTE 	i;
	BYTE 	offsetIndex;
	BYTE	gotSlope;									// 2011-09-15 -WFC-

	gotSlope = FALSE; 									// 2011-09-15 -WFC-
	for ( i=0; i < MAX_CAL_POINTS; i++)		{
		if ( adcCnt <= pCalAdcCntTable[i] )
			break;
		if ( i > 1 ) {
			if ( pCalAdcCntTable[i] == pCalAdcCntTable[i-1] )	{// if the remained cal points are the same, done.
				i -=2;		// points at one point ahead of the first occurrence of the same cal point value.
				offsetIndex = i;						// 2011-09-15 -WFC-
				gotSlope = TRUE; 						// 2011-09-15 -WFC-
				break;
			}
		}
	} // end  for ( i=0; i < MAX_CAL_POINTS; i++) {}

	if ( FALSE == gotSlope ) 	{					// 2011-09-15 -WFC-
		if ( i > (MAX_CAL_POINTS -1) ) {		// the input ADC count > all counts in the cal table.
			offsetIndex = --i ;					// points at the max cal point as its weight and ADC_counts offset index
			i--;								// points at the last second cal point, last cal point pair.
		}
		else if ( i > 0 )
			offsetIndex = --i ;				// points at the max cal point as its weight and ADC_counts offset index
		else
			offsetIndex = i;
	}
	//if ( pCalAdcCntTable[i] == pCalAdcCntTable[i+1] ) {	// if the points are the same, use different pair on one cal point to the left.
	//	i--;
	//}

	i32 = pCalAdcCntTable[ offsetIndex ];				// i32 = offset ADC counts of this cal point pair.
	adcCnt -= i32;										// now adcCnt is the relative counts of this cal point pair.
	// compute scale factor
	i32	= pCalAdcCntTable[i+1] - pCalAdcCntTable[i];	// i32 = scaled count of this cal point pair.
	if ( 0 == i32 )
		fV	= 1.0;		// scale factor = 1;
	else
		fV = (pCalValueTable[i+1] - pCalValueTable[i])/ (float) (i32);		// scale factor
	fV = (float) ( adcCnt) * fV   + pCalValueTable[ offsetIndex ];
	return fV;
} // end adc_to_value(,,)

/**
 * It computes an ADC count difference between 0 weight and 10% of capacity weight. Unit is irrelevant.
 * This is intent to replace traditional Rcal value.
 * Caller must ensure the cal table has at least zero point and 1 span point.
 *
 * @param  pCalAdcCntTable	-- pointer to base of calibrated ADC count table.
 * @param  pCalValueTable	-- pointer to base of calibrated value table.
 *
 * @return Rcal value in the form of ADC count at value of 10% of capacity weight excluded ADC count at 0 weight.
 *
 * History:  Created on 2011-04-22 by Wai Fai Chin
 * 2011-04-28 -WFC- Changed from fixed 1000 value to 10% of capacity.
 */


INT32  get_constant_cal( INT32 *pCalAdcCntTable, float *pCalValueTable, float capacity )
{
	float	fV;
	INT32	dADCcnt;

	// compute scale factor
	dADCcnt	= pCalAdcCntTable[1] - pCalAdcCntTable[0];	// dADCcnt = scaled count of this cal point pair.
	fV = pCalValueTable[1] - pCalValueTable[0];
	if ( fV < 0.00001f )
		fV	= 1.0;

	fV = (float) (dADCcnt) / fV;				// scaled factor in (ADCcnt / Value).
	fV *= (capacity * 0.1f);					// compute ADC count at value 10% of capacity weight. NOTE this ADC count is not included the ADC count at 0 weight.
	dADCcnt = (INT32) (fV);
	return dADCcnt;
} // end get_constant_cal(,)


#endif // ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )


/**
 *   Validate an input string is a number.
 *   A number defined by MSI is ([ ])*|[-]([0-9])+|[.]([0-9])* .
 *   Length of a number will not include leading spaces or a decimal point.
 *   It includes '-'.
 *
 * @param	pbStr  -- an input string to be validate. The length must be < 128 because
 *                 the function return sign char.
 *
 * @return	0 if it is none number.
 *			Negative value is the length of negative number exclude leading spaces and decimal point.
 *			Positive value is the length of positive number exclude leading spaces and decimal point.
 *
 * History:  2009/02/17  Original coded by Wai Fai Chin
 * 
*/

INT8 is_a_led_number( const BYTE *pbStr )
{
	BYTE bNumDigit;
	BYTE ch;
	BYTE bIsMinus;

	bIsMinus = FALSE;

	bNumDigit = 0;
	// skip spaces
	for (;; ) {
		ch = *pbStr++;
		if (  ch == 0 )
			return 0;			// end of string, the string is not a number
		if ( ch != ' ' )
			break;				// encountered a none space char
	}// end for (;;)

	// the next char should be '-' or a digit
	if ( ch == '-' ) {
		bNumDigit++;
		bIsMinus = TRUE;
	}
	else if (ch >= '0' && ch <= '9' ) {
		bNumDigit++;
	}
	else
		return 0;	// not a number

	// the next section should be digit until it find a decimal point
	for (;; ) {
		ch = *pbStr++;
		if ( ch == 0 ) break;
		if ( ch >= '0' && ch <= '9' )
			bNumDigit++;
		else if ( ch == '.' )
			break;
		else 
			return 0;		// not a number
	}// end for (;;)

	// the next section should be digit until it reach the end of string
	if ( ch ) {
		for (;; ) {
			ch = *pbStr++;
			if ( ch == 0 ) break;
			if ( ch >= '0' && ch <= '9' )
				bNumDigit++;
			else 
				return 0;		// not a number
		}// end for (;;)
	}// end if (ch)

	if ( bIsMinus )
		return  -((INT8) bNumDigit);
	else
		return (INT8) bNumDigit;
}// end is_a_led_number()

/**
 * It fills number n of ch into a string buffer pointed by pStr.
 *
 * @param	pStr	--  pointer to string buffer to be fill.
 * @param	ch		--	a character to be fill with.
 * @param	n		--	number of ch to be fill.
 *
 * @note	caller must supplied the string buffer larger than n bytes.
 *
 * History:  2010/04/21  Original coded by Wai Fai Chin
 *
*/

void	fill_string_buffer( char *pStr, char ch, BYTE n)
{
	while ( n ) {
		*pStr++ = ch;
		n--;
	}
	*pStr = 0;	// marked the end of string.
} // end fill_string_buffer(,)


/* *
 *   Validate an input string is a number.
 *   A number defined by MSI is ([ ])*|[-]([0-9])+|[.]([0-9])* .
 *   Length of a number will not include leading spaces or a decimal point.
 *   It includes '-'.
 *
 * @param	pbStr  -- an input string to be validate. The length must be < 128 because
 *                 the function return sign char.
 * @param	pbStart	-- output start position of the digit exclude space, include '-'. 
 * @param	pbDecPtPosition	-- output decimal point position of from a digit exclude '-' and spaces.
 *
 * @return	0 if it is none number.
 *			Negative value is the length of negative number exclude leading spaces and decimal point.
 *			Positive value is the length of positive number exclude leading spaces and decimal point.
 *
 * History:  2009/02/17  Original coded by Wai Fai Chin
 * 
* /

INT8 is_a_led_number( const BYTE *pbStr, BYTE *pbStart, BYTE *pbDecPtPosition )
{
	BYTE bNumDigit;
	BYTE ch;
	BYTE bIsMinus;

	bIsMinus = FALSE;

	*pbDecPtPosition = *pbStart = 	bNumDigit = 0;
	// skip spaces
	for (;; ) {
		ch = *pbStr++;
		if (  ch == 0 )
			return 0;			// end of string, the string is not a number
		if ( ch != ' ' )
			break;				// encountered a none space char
		(*pbStart)++;
	}// end for (;;)

	// the next char should be '-' or a digit
	if ( ch == '-' ) {
		bNumDigit++;
		bIsMinus = TRUE;
	}
	else if (ch >= '0' && ch <= '9' ) {
		bNumDigit++;
	}
	else
		return 0;	// not a number

	// the next section should be digit until it find a decimal point
	for (;; ) {
		ch = *pbStr++;
		if ( ch == 0 ) break;
		if ( ch >= '0' && ch <= '9' )
			bNumDigit++;
		else if ( ch == '.' ) {
			*pbDecPtPosition = bNumDigit;
			if (bIsMinus) {
				if ( bNumDigit > 1 )
					(*pbDecPtPosition)--;
			}
			break;
		}
		else 
			return 0;		// not a number
	}// end for (;;)

	// the next section should be digit until it reach the end of string
	if ( ch ) {
		for (;; ) {
			ch = *pbStr++;
			if ( ch == 0 ) break;
			if ( ch >= '0' && ch <= '9' )
				bNumDigit++;
			else 
				return 0;		// not a number
		}// end for (;;)
	}// end if (ch)

	if ( 0 == *pbDecPtPosition ) {		// if it has no decimal point
		if ( bIsMinus ) {
			if ( bNumDigit > 0 )
				*pbDecPtPosition = bNumDigit - 1;
		}
		else
			*pbDecPtPosition = bNumDigit;
	}
	
	if ( bIsMinus )
		return  -((INT8) bNumDigit);
	else
		return (INT8) bNumDigit;
}// end is_a_led_number()


/ **
 * It insert a specified char in a given location in the same buffer.
 * This will save RAM memory space too.
 *
 * @param	pbStr		-- i/o string. 
 * @param	insertChar	-- points to a source string.
 * @param	insertLoc	-- start position of the source string exclude space, include '-'. 
 * @param	strLen		-- last location of source string which index at the null character.
 *
 * @return	none
 * @post	the input string has the modified string.
 * @note	caller must ensured the pbStr buffer is larger enought to handle strLen size.
 *
 * History:  Created on 2009/06/03 by Wai Fai Chin
 * /

void insert_a_char_in_string( BYTE *pbStr, BYTE insertChar, BYTE insertLoc, BYTE strLen )
{
	BYTE i;
	BYTE ch1, ch2;

	if ( insertLoc < strLen )	{
		i = insertLoc;
		ch1 = pbStr[ i ];
		pbStr[ i ] = insertChar;
		i++;
		ch2 = pbStr[ i ];
		while( i < strLen ) {
			pbStr[ i ] = ch1;
			ch1 = ch2;
			i++;
			ch2 = pbStr[ i ]
		}
	}
} // end insert_a_char_in_string(,,,)

*/
