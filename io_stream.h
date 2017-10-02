/*! \file io_stream.h \brief pure abstract IO stream class related functions.*/
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
// Software layer:  abstract layer
//
//  History:  Created on 2009/03/16 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup io_stream abstract data IO stream interface between all level of objects and software modules.
/// \code #include "io_stream.h" \endcode
/// \par Overview
///     This is an abstract data IO stream class use by all level of objects and software modules.
///  It contains source ID, destination ID, other communication layer IDs, stream type and abstract data buffer.
///  This abstract stream could become either a concrete high level or low level stream.
///  This concrete stream is use by application level, packet router, stream router,
///  stream driver and low level hardware communication driver.
///
///
///	\code
///
///                            STREAM FLOWS and OBJECT INTERACTION DIAGRAM
///
///                                                 High level stream driver
///           APPLICATION           Stream_Router     RFmodem_Stream_driver   low level hardware com driver
///         +---------------+     +----------------+   +----------------+      +--------------+
///         |               |     |                |   |                |      | UART         |
///         |  Hight level  |---->| route stream to|-->| RFmodem_Stream |----->| Serial port  |
///         |stream write() |     | the appropriate|   | send()         |      | send()       |
///         |       read () |     | stream driver  |<--| Receive()      |<-----| Receive()    |
///         +---------------+     +----------------+   +----------------+      +--------------+
///                    ^               ^        |
///                    |               |        |
///                    |               |        |
///                    |   +---------------+    |
///                    |   |               |    |
///                    +---| Packet Router |<---+
///                        |               |
///                        +---------------+ 
///	\endcode
//
//
// ****************************************************************************
//@{


#ifndef MSI_IO_STREAM_CLASS_H
#define MSI_IO_STREAM_CLASS_H

#include "commonlib.h"

#define IO_STREAM_STATUS_DIR_IN			0X80
#define IO_STREAM_STATUS_STATIC			0X40
#define IO_STREAM_STATUS_ACTIVE			0X20
#define IO_STREAM_STATUS_PACKET_ERROR	0X10
#define IO_STREAM_STATUS_LOOK4_START	0X00
#define IO_STREAM_STATUS_LOOK4_END		0X01
#define IO_STREAM_STATUS_LOOK4_SRC_ID	0X02
#define IO_STREAM_STATUS_LOOK4_DEST_ID	0X03
#define IO_STREAM_STATUS_LOOK4_CMD		0X04
#define IO_STREAM_STATUS_LOOK4_DONE		0X05
#define IO_STREAM_STATUS_LOOK4_MASK		0X07

/* 2011-05-10 -WFC- v
#define IO_STREAM_TYPE_UART		0X00
#define IO_STREAM_TYPE_UART_1	0X01
#define IO_STREAM_TYPE_SPI		0X02
#define IO_STREAM_TYPE_RFMODEM	0X03
*/

#define IO_STREAM_TYPE_UART		0X00
#define IO_STREAM_TYPE_UART_1	0X01
#define IO_STREAM_TYPE_UART_2	0X02
#define IO_STREAM_TYPE_RFMODEM	0X03
#define IO_STREAM_TYPE_SPI		0X04
// 2011-10-26 -WFC- ^

#if ( CONFIG_PCB_AS == CONFIG_PCB_AS_SCALECORE1 )	// 2011-12-13 -WFC-
#define MAX_STREAM_TYPE			0
#else
	#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_IDSC || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER)	// 2011-12-13 -WFC-
		#define MAX_STREAM_TYPE			2
	#else
		#define MAX_STREAM_TYPE			1
	#endif
#endif

#define IO_STREAM_IGNOR_ID		0
#define IO_STREAM_BROADCAST_ID	0xFF
#define IO_STREAM_INVALID_SRCID	0xFF


/*!
  \brief abstract IO stream class
  \par overview
   This is an abstract data IO stream class use by all level of objects and software modules.
  \note
  This object must initialized its abstract members and methods by assigned concrete members and methods before it can be use.  
*/

typedef struct IO_STREAM_CLASS_TAG {
							/// 0=UART; 1=SPI; 2=RF modem;
  BYTE	type;
							/// bit7: direction 1=in, 0=Out; bit6: 1=static, 0=dynamic; bit5: 1=active; 0=not in use; bit4:got start of packet; bit3: got IDs; bit2 to bit0:reserved;
  BYTE	status;
							/// packet length of the current packet in this stream.
  BYTE	packetLen;
							/// high level packet source ID, which also use in the host command packet.
  BYTE	sourceID;
							/// high level packet destinatin ID, which also use in the host command packet.
  BYTE	destID;
							/// points specific low level transport layer of source and destination ID such as, ip addresses, RF source and destination IDs.
  void  *pTransID;
							/// private, points to circular buffer. This is where you get your data from.
  BYTE_CIRCULAR_BUF_T *pCirBuf;
  /* not to do the following because it wastes RAM. Trade RAM at the expense of flash code space.
							/// abstract method to read a byte of data
  void    ( *pEmpty_buffer)  ( void  );
							/// abstract method to read a byte of data
  BYTE    ( *pPeek_byte)  ( BYTE *pData  );
							/// abstract method to read a byte of data
  BYTE    ( *pRead_byte)  ( BYTE *pData  );
							/// abstract method to write n number of bytes of data from ram space.
  BYTE    ( *pWrite_bytes) ( const char *pbData, BYTE n);
							/// abstract method to write n number of bytes of data from program memory space.
  BYTE    ( *pWrite_bytes_P) ( const char *pbData, BYTE n);
  */
}IO_STREAM_CLASS;


/** \def io_stream_empty_buffer
 * It empties the buffer in the iostream object.
 * 
 * @param  pStream -- pointer to IO_STREAM object.
 *
 * History:  Created on 2009/03/25 by Wai Fai Chin
 */
// #define  iostream_empty_buffer( pStream )   reset_byte_circular_buf ((BYTE_CIRCULAR_BUF_T *)(pStream-> ptrCir)) 
#define  io_stream_empty_buffer( pStream )   reset_byte_circular_buf ((pStream)-> pCirBuf)


/** \def io_stream_peek_a_byte
 * It peak a byte from cicular buffer without affect any read index.
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
#define  io_stream_peek_a_byte( pStream, pbData )   peek_byte_circular_buf ((pStream)-> pCirBuf, (pbData) )
// BYTE	peek_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *ptrData );


/** \def io_stream_rewind_peekindex
 * It rewinds the peekindex to the same as the read index.
 *
 * @param  pStream -- pointer to IO_STREAM object.
 *
 * History:  Created on 2009/04/02 by Wai Fai Chin
 */
#define  io_stream_rewind_peekindex( pStream )		rewind_peekindex_byte_circular_buf( (pStream)-> pCirBuf )


/** \def io_stream_advance_readindex
 * It advances readindex by a specific length.
 *
 * @param  pStream	-- pointer to IO_STREAM object.
 * @param  len		-- length to be advance.
 *
 * History:  Created on 2009/04/06 by Wai Fai Chin
 */
#define  io_stream_advance_readindex( pStream, len )		advance_readindex_byte_circular_buf( (pStream)-> pCirBuf, (len) )


/** \def io_stream_readindex_eq_peekindex
 * It set readindex to the same as peekindex.
 *
 * @param  pStream	-- pointer to IO_STREAM object.
 *
 * History:  Created on 2009/04/08 by Wai Fai Chin
 */
#define  io_stream_readindex_eq_peekindex( pStream )		 (pStream)-> pCirBuf-> readIndex = (pStream)-> pCirBuf-> peekIndex


/** \def io_stream_empty_buffer
 * It empties the buffer in the iostream object.
 * 
 * @param  pStream -- pointer to IO_STREAM object.
 *
 * History:  Created on 2009/03/25 by Wai Fai Chin
 */
// #define  iostream_empty_buffer( pStream )   reset_byte_circular_buf ((BYTE_CIRCULAR_BUF_T *)(pStream-> ptrCir)) 
#define  io_stream_is_full( pStream )   (pStream)-> pCirBuf-> isFull


/** \def io_stream_buffer_size
 * It empties the buffer in the iostream object.
 * 
 * @param  pStream -- pointer to IO_STREAM object.
 *
 * History:  Created on 2009/04/13 by Wai Fai Chin
 */
// #define  iostream_empty_buffer( pStream )   reset_byte_circular_buf ((BYTE_CIRCULAR_BUF_T *)(pStream-> ptrCir)) 
#define  io_stream_buffer_size( pStream )   (pStream)-> pCirBuf-> size



/** \def io_stream_scan_left_side
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
#define  io_stream_scan_left_side( pStream, match )   scan_left_side_byte_circular_buf ((pStream)-> pCirBuf, (match) )
// BYTE 	scan_left_side_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE match );


/** \def io_stream_scan_right_side
 * It looks for a specified match data on the right side of the packet.
 * It advances peek index only.
 * It is designe to look for the END of packet on the right side.
 *
 * @param  ptrCir	-- point to circular buff object
 * @param  match	-- match data to look for.
 *
 * @return 0xFFnn if it found the match. Where nn the lower order byte is the length from the readIndex or the 1st match data.
 *         0x00nn if it found NO match.
 *         0x0001 if the buffer is empty since the first match data.
 *         0xF0nn if it found NO match, but found another start marker
 *
 *          
 * @post   the peek index advanced.
 *
 * @note   readIndex NEVER changed.
 *         peekIndex always advanced.
 *
 * History:  Created on 2009/03/26 by Wai Fai Chin
 */
#define  io_stream_scan_right_side( pStream, match, start )   scan_right_side_byte_circular_buf ((pStream)-> pCirBuf, (match), (start) )
// UINT16	scan_right_side_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE match );


/** \def io_stream_read_a_byte
 * It read a byte from the stream object.
 * 
 * @param  pStream -- pointer to IO_STREAM object.
 * @return 1==has a new data, 0==none.
 *
 * History:  Created on 2009/03/25 by Wai Fai Chin
 */
#define  io_stream_read_a_byte( pStream, pbData )   read_byte_circular_buf ((pStream)-> pCirBuf, (pbData) )
// BYTE 	read_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *ptrData );


/** \def io_stream_read_bytes
 * It read specified number of byte from the stream object.
 * 
 * @param  pStream -- pointer to IO_STREAM object.
 * @return number of byte read from the stream.
 *
 * History:  Created on 2009/04/09 by Wai Fai Chin
 */
#define  io_stream_read_bytes( pStream, pbDest, n )   read_bytes_from_byte_circular_buf ((pStream)-> pCirBuf, (pbDest), (n) )
// BYTE	read_bytes_from_byte_circular_buf( BYTE_CIRCULAR_BUF_T *ptrCir, BYTE *pbDest, BYTE n );


/** \def io_stream_write_bytes
 * It writes specified number of bytes into the stream.
 * @param	pStream	-- pointer to io stream.
 * @param	pbSrc	-- pointer to source data.
 * @param	n		-- number of bytes to write out.
 * @return  number of byte had writen into buffer.
 *
 * History:  Created on 2009/03/25 by Wai Fai Chin
 */
#define  io_stream_write_bytes( pStream, pbSrc, n )   write_bytes_to_byte_circular_buf ((pStream)-> pCirBuf, (pbSrc), (n) )


/** \def io_stream_write_bytes_P
 * It writes specified number of bytes from program space into the stream.
 * @param	pStream	-- pointer to io stream.
 * @param	pbSrc	-- pointer to source data from program space.
 * @param	n		-- number of bytes to write out.
 * @return  number of byte had writen into buffer.
 *
 * History:  Created on 2009/03/25 by Wai Fai Chin
 */
#define  io_stream_write_bytes_P( pStream, pbSrc, n )   write_bytes_to_byte_circular_buf_P ((pStream)-> pCirBuf, (pbSrc), (n) )


#endif	// MSI_IO_STREAM_CLASS_H
//@}
