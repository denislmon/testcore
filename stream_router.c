/*! \file stream_router.c \brief stream router functions.*/
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
//     It routes high level IO stream between application and stream drivers.
//  It routes stream object to concrete stream driver based on stream type, not on destination ID.
//  The specific concret stream driver will manage stream base on destination ID.
//  It maintains a records of all registered stream objects and stream drivers.
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

// public:

/// array of high level stream driver.
STREAM_DRIVER_CLASS		gaStreamDriver[ MAX_NUM_STREAM_DRIVER ];

/// array of io stream objects.
IO_STREAM_CLASS			gaIoStream[ MAX_NUM_IO_STREAM ];

// private:
/// circular buffer object for IO_STREAM_CLASS
BYTE_CIRCULAR_BUF_T		gabStreamCirBuf[ MAX_NUM_IO_STREAM ];

/// memory pool for circular buffer.
BYTE					gabCirBufPool[ MAX_STREAM_CIR_BUF_SIZE * MAX_NUM_IO_STREAM ];


/**
 * A nop register function for stream driver object that does not required to build a route lookup table.
 *
 * @param pStream -- pointer to a stream object.
 * @return none.
 *
 *
 * History:  Created on 2009/03/18 by Wai Fai Chin
 */

void stream_register_nop( IO_STREAM_CLASS *pStream )
{

}


/**
 * It delegates registry task to a specific stream driver object based on the stream type.
 * The stream driver builds its internal destination lookup table based on the source and destination ID with
 * respect to its internal address schem such as ip address, radio ID etc..
 *
 * @param pStream -- pointer to a concrete stream object.
 * @return none.
 *
 *
 * History:  Created on 2009/04/23 by Wai Fai Chin
 */

void stream_router_build_stream_driver_internal_route_table( IO_STREAM_CLASS *pStream )
{
	BYTE type;
	
	type = pStream-> type;
	if ( type <= MAX_STREAM_TYPE ) {
		gaStreamDriver[type].pRegister( pStream );	// delegate to appropriate stream driver to build its internal destination lookup registry table.
	}
} // end stream_router_build_stream_driver_internal_route_table()



/**
 * A provide an unused IO_STREAM object for caller.
 *
 * @param ppStream -- pointer to pointer to a stream object. It use for store a pointer to a new iostream object.
 * @return true if it has a new unused IO_STREAM_CLASS object, else false;
 *
 *
 * History:  Created on 2009/03/19 by Wai Fai Chin
 */

BYTE stream_router_get_a_new_stream( IO_STREAM_CLASS **ppStream )
{

	BYTE i;
	BYTE status;
	
	status = FALSE;
	for ( i=0; i < MAX_NUM_IO_STREAM; i++) {
		if ( !(IO_STREAM_STATUS_ACTIVE & gaIoStream[i].status) ) {
			*ppStream = &gaIoStream[i];
			status = TRUE;
			break;
		}
	}

	return status;
} // end stream_router_get_a_new_stream();


/**
 * A provide an unused IO_STREAM object for caller.
 *
 * @param ppStream		-- pointer to pointer to a stream object. It use for store a pointer to a new iostream object.
 * @param streamType	-- stream type.
 * @param io_dir		-- io direction, 0x80==in, 0==out.
 * @param destID		-- destination ID, 0==ignor the ID.
 *
 * @return true if it found the specified IO_STREAM_CLASS object, else false;
 *
 *
 * History:  Created on 2009/03/30 by Wai Fai Chin
 */

BYTE stream_router_get_a_stream( IO_STREAM_CLASS **ppStream, BYTE streamType, BYTE io_dir, BYTE destID )
{
	BYTE i;
	BYTE result;
	BYTE status;
	
	result = FALSE;
	for ( i=0; i < MAX_NUM_IO_STREAM; i++) {
		status = gaIoStream[i].status;
		if ( (IO_STREAM_STATUS_ACTIVE & status) &&
			 ((IO_STREAM_STATUS_DIR_IN & status) == io_dir) &&
			 ( streamType == gaIoStream[i].type) &&
			 ( IO_STREAM_IGNOR_ID == destID || destID == gaIoStream[i].sourceID)
			)	{
			*ppStream = &gaIoStream[i];
			result = TRUE;
			break;
		}
	}

	return result;
} // end stream_router_get_a_stream(,,,);


/**
 * Init and construct all iostreams which include concrete circular buffers.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/03/19 by Wai Fai Chin
 */

void stream_router_init_build_all_iostreams( void )
{
	UINT16  len;
	BYTE	i;

	len = 0;
	for ( i=0; i < MAX_NUM_IO_STREAM; i++) {
		gaIoStream[i].type = gaIoStream[i].status = gaIoStream[i].packetLen = 0;
		gaIoStream[i].pCirBuf = &gabStreamCirBuf[i];
		init_byte_circular_buf( gaIoStream[i].pCirBuf, &gabCirBufPool[ len], 0, MAX_STREAM_CIR_BUF_SIZE);
		len += MAX_STREAM_CIR_BUF_SIZE;
	}
	
} // stream_router_init_build_all_iostreams();



/**
 * It routes out a oStream to its appropriate stream driver base on the stream type.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/03/23 by Wai Fai Chin
 */

void stream_router_routes_a_ostream( IO_STREAM_CLASS *pOstream )
{
	BYTE type;
	BYTE status;
	
	status = pOstream-> status;
	if ( !( status & IO_STREAM_STATUS_DIR_IN) &&		// if it is an output stream object.
		  ( status & IO_STREAM_STATUS_ACTIVE )) {		// and if this stream is active and alive.
		type = pOstream-> type;
		if ( type <= MAX_STREAM_TYPE ) {
			gaStreamDriver[type].pSend( pOstream );	// route to its appropriate stream driver.
		}
	}
	
}// end stream_router_routes_a_ostream()
	


/**
 * It routes out a oStream to its appropriate stream driver base on the stream type.
 *
 * @param	pOstream	-- pointer to output stream.
 * @param	pSrc		-- pointer to source data.
 * @param	n			-- number of bytes to write out.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/04/23 by Wai Fai Chin
 */

void stream_router_routes_a_ostream_now( IO_STREAM_CLASS *pOstream, BYTE *pSrc, BYTE n )
{
	BYTE wrn;		// number of byte has written.
	
	wrn = io_stream_write_bytes( pOstream, pSrc, n );
	stream_router_routes_a_ostream( pOstream );
	if ( n > wrn ) {
		wrn = io_stream_write_bytes( pOstream, pSrc + wrn, n - wrn );	// xmit the remaining data.
		stream_router_routes_a_ostream( pOstream );
	}
}// end stream_router_routes_a_ostream_now()
	

/**
 * It routes out all oStreams to its appropriate stream driver base on the stream type.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/03/23 by Wai Fai Chin
 */

void stream_router_routes_all_ostreams( void )
{
	IO_STREAM_CLASS *pOstream;
	BYTE i;
	
	for ( i=0; i < MAX_NUM_IO_STREAM; i++) {
		pOstream = &gaIoStream[i];
		stream_router_routes_a_ostream( pOstream );
	}
	
}// end stream_router_routes_all_ostreams()


/**
 * It tells all stream drivers to receive data.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/04/21 by Wai Fai Chin
 */

void stream_router_all_stream_drivers_receive( void )
{
	BYTE i;

	for ( i=0; i < MAX_NUM_STREAM_DRIVER; i++) {
		gaStreamDriver[ i ].pReceive();
	}
	
}// end stream_router_all_stream_drivers_receive()
	

///////////////////////////////////////////////////////////////////////////////
//                      Test functions for this module.                      //
///////////////////////////////////////////////////////////////////////////////

#if  ( CONFIG_TEST_STREAM_ROUTER_MODULE == TRUE )


/**
 * Test stream router, stream driver modules.
 * It illustrates how to use all the methods and interfaces between methods.
 * Assumed the above modules had initialized.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/03/23 by Wai Fai Chin
 */

void stream_router_module_test_bench( void )
{
	IO_STREAM_CLASS *pOstream;
	IO_STREAM_CLASS *pIstream;
	BYTE ch;
	BYTE status;

	if ( stream_router_get_a_new_stream( &pOstream ) ) {
		pOstream-> type		= IO_STREAM_TYPE_UART;
		pOstream-> status	= IO_STREAM_STATUS_ACTIVE;	// ostream type

		io_stream_write_bytes_P( pOstream, PSTR("\nStreaming Works!\n\r"), 19 );
		stream_router_routes_a_ostream( pOstream );

		if ( stream_router_get_a_stream( &pIstream, IO_STREAM_TYPE_UART, IO_STREAM_STATUS_DIR_IN, IO_STREAM_IGNOR_ID) ) {
			for (;;) {
				gaStreamDriver[IO_STREAM_TYPE_UART].pReceive();
				status = io_stream_read_a_byte( pIstream, &ch );
				if ( status ) {
					io_stream_write_bytes( pOstream, &ch, 1 );
				}
				stream_router_routes_all_ostreams();
				if ( 'X' == ch ) break;
			}
		}
		stream_router_free_stream( pOstream );	// put resource back to stream pool.
	}
}



#endif // ( CONFIG_TEST_STREAM_ROUTER_MODULE == TRUE )






