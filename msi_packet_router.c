/*! \file msi_packet_router.c \brief packet router related functions.*/
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
// Software layer:  application, concrete layer
//
//  History:  Created on 2009/03/17 by Wai Fai Chin
// 
//     It routes high level IO stream between application and stream drivers.
//  It maitains a registry of streams with sourceID, stream type, command and status.
//  This registry keep track command status of each device. Other object uses this registry
//  to dynamically create ostream for send data to registered device.
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

#include "msi_packet_router.h"
#include "stream_driver.h"
#include "stream_router.h"
#include "cmdparser.h"
#include "nvmem.h"			// for PRODUCT_INFO_DEVICE_ID
#include "bios.h"			// 2011-04-18 -WFC-

/**
 * \code
 * Host Command Structure:
 *    {src dest cmdxP1;P2;;;Pn}
 *    spaces are for clarity. No space between src dest and cmd.
 *    where:
 *           {  is the command start character marke the start of the command.
 *         src  is the source id in two digit hex char range from 00 to FF. FF is a free ID use by host before first connection with this device.
 *         dest is the command id in two digit hex char range from 00 to FE. FF is for every device, broadcast mode.
 *         cmd  is the command id in two digit hex char range from 00 to FE. FF is invalid command.
 *           x  is the command attribute field has the following char:
 *                 r -- answers the command query with contents of items that associated
 *                      with the command. It is for output to the host computer.
 *                 i -- answers the command query of 1D array index type.
 *                      The number followed by i is the index of all the following array outputs.
 *
 *                 ? -- command query items that associated with the command.
 *                 = -- assigns input contents to items that associated with the command.
 *                 z -- reset items that associated with the command to its default contents.
 *              g, = -- denoted for action type command. Action type command has no input parameter.
 *                      The resulted action might output data to I/O devices.
 *                 0 to F are reserved for future use.
 *          Pn  is the input parameters or contents of items that associated with the command.
 *           }  is the end character marked the end of the command.
 * Command Version:  00-00-01
 * \endcode
 * History:  Created on 2006/11/07 by Wai Fai Chin
 *			 2009/03/31 -WFC- modified it to include source and destination ID.
 */



CMD_PRE_PARSER_STATE_T	gCmdPreParserState;


/*!
  \brief It dynamically keep tracks istream object and its command status of each connected device.
 \note
	It only keep tracks last MAX_NUM_STREAM_REGISTRY devices.
    The oldest one will bump out and replaced with new device.
	It does not keep tracks the entire stream object. It just registered the sourceID and its stream type.
	This help save lots of memory.
*/

/*
typedef struct STREAM_REGISTRY_TAG {
	BYTE	sourceID;
	BYTE	streamType;
	BYTE	cmd;
	BYTE	status;
}STREAM_REGISTRY_T;
*/

// make this into arrays instead of structure is for easy host command interface. It also use less code memory too.
															/// source ID of a stream registry
BYTE	gaStreamReg_sourceID[ MAX_NUM_STREAM_REGISTRY ];
															/// stream type of a stream registry
BYTE	gaStreamReg_type[ MAX_NUM_STREAM_REGISTRY ];
															/// command of a stream registry
BYTE	gaStreamReg_cmd[ MAX_NUM_STREAM_REGISTRY ];
															/// command status of a stream registry
BYTE	gaStreamReg_cmdStatus[ MAX_NUM_STREAM_REGISTRY ];


/// private: circular index of gaStreamReg. It uses for bump the oldest registry and put a new record in its place.
BYTE	gbStreamReg_OldestIndex;


/// private: keeps track which stream is being process, so every stream has chance to take turn.
BYTE	gbPR_CurStreamNum;

// private methods
BYTE	msi_packet_router_scan_byte_hex( IO_STREAM_CLASS *pStream, BYTE *pbV );
void	msi_packet_router_parse_a_stream( IO_STREAM_CLASS *pStream );


/**
 * It initialize the dynamic stream registry and its private variables.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/03/23 by Wai Fai Chin
 */

void msi_packet_router_init( void )
{
	BYTE i;
	
	gbStreamReg_OldestIndex = gbPR_CurStreamNum = 0;
	for ( i=0; i< MAX_NUM_STREAM_REGISTRY; i++) {
		gaStreamReg_sourceID[i] = IO_STREAM_INVALID_SRCID;
		gaStreamReg_type[i]  = gaStreamReg_cmd[i] = gaStreamReg_cmdStatus[i] = CMD_STATUS_NO_ERROR;
	}
}


/**
 * It looks for a packet and parse source and destination IDs in the given input stream.
 *
 * @param	pStream -- pointer to iostream object.
 * @return	none.
 *
 *
 * History:  Created on 2009/04/03 by Wai Fai Chin
 */

void msi_packet_router_parse_a_stream( IO_STREAM_CLASS *pStream )
{
	UINT16 wStatus;
	BYTE ch;
	BYTE lookFor;

	lookFor = pStream-> status & IO_STREAM_STATUS_LOOK4_MASK;
	switch( lookFor ) {
		case IO_STREAM_STATUS_LOOK4_START:
			if ( io_stream_scan_left_side( pStream, MSI_PACKET_START_CHAR ) ) {	// if found the packet START marker.
				pStream-> status &= ~IO_STREAM_STATUS_LOOK4_MASK;
				pStream-> status |= IO_STREAM_STATUS_LOOK4_END;					// changed status to look for packet END marker.
				DEBUG_TRACE("%s", "\nFound Start");
			}
			else
				break;
		case IO_STREAM_STATUS_LOOK4_END:
			wStatus = io_stream_scan_right_side( pStream, MSI_PACKET_END_CHAR, MSI_PACKET_START_CHAR );
			if ( MSI_LIB_FOUND_START == (wStatus & MSI_LIB_FOUND_MATCH)   ) {	// if found a new START marker before find the END marker,
				pStream-> status &= ~(IO_STREAM_STATUS_LOOK4_MASK | IO_STREAM_STATUS_PACKET_ERROR );	// look for start, clear status and error bits.
				pStream-> status |= IO_STREAM_STATUS_LOOK4_END;					// changed status to look for packet END marker.
				DEBUG_TRACE("%s", "\nFound Start before it find the END");
				break;
			}
			if ( MSI_LIB_FOUND_MATCH == (wStatus & MSI_LIB_FOUND_MATCH)   ) {		// if found the packet END marker.
				pStream-> packetLen = (BYTE) ( 0x00FF & wStatus);
				if ( pStream-> packetLen >= 8 ) {
					pStream-> status &= ~IO_STREAM_STATUS_LOOK4_MASK;
					pStream-> status |= IO_STREAM_STATUS_LOOK4_SRC_ID;					// changed status to look for SOURCE ID.
					DEBUG_TRACE("\nFound END: len=%d", pStream-> packetLen);
				}
				else {
					pStream-> status |= IO_STREAM_STATUS_PACKET_ERROR;					// invalid packet because lenght too short.
					io_stream_advance_readindex( pStream, pStream-> packetLen);	// skip passed the END marker of this bad packet.
					DEBUG_TRACE("\nFound END: WRONG len=%d", pStream-> packetLen);
					break;
				}
			}
			else {
				ch = (BYTE) ( wStatus & 0x0FF);
				if ( ch >= io_stream_buffer_size( pStream )) {
					pStream-> status |= IO_STREAM_STATUS_PACKET_ERROR;				// invalid hex string.
					io_stream_advance_readindex( pStream, 1);							// index at next one passed the START marker.
					DEBUG_TRACE("\n Found No END marked in the entire buffer: len= %d", ch);
				}
				break;
			}
		case IO_STREAM_STATUS_LOOK4_SRC_ID:
			io_stream_rewind_peekindex( pStream );									// rewind peek index back at the packet START marker.
			io_stream_peek_a_byte( pStream, &ch ); 								// peak at the START marker, peek index advance to the next char.
			if ( msi_packet_router_scan_byte_hex( pStream, &ch ) ) {				// if got a valid hex string.
				pStream-> sourceID	 = ch;
				pStream-> status &= ~IO_STREAM_STATUS_LOOK4_MASK;
				pStream-> status	|= IO_STREAM_STATUS_LOOK4_DEST_ID;			// changed status to look for DESTINATION ID.
				DEBUG_TRACE("\nFound SRCID: %02X", ch);
			}
			else {
				pStream-> status |= IO_STREAM_STATUS_PACKET_ERROR;				// invalid hex string.
				io_stream_advance_readindex( pStream, 1);							// index at next one passed the START marker.
				DEBUG_TRACE("\n Invalid SrcID string: status= %d", (pStream-> status & IO_STREAM_STATUS_LOOK4_MASK));
				break;
			}
		case IO_STREAM_STATUS_LOOK4_DEST_ID:
			if ( msi_packet_router_scan_byte_hex( pStream, &ch ) ) {				// if got a valid hex string.
				pStream-> destID	 = ch;
				pStream-> status &= ~IO_STREAM_STATUS_LOOK4_MASK;
				pStream-> status	|= IO_STREAM_STATUS_LOOK4_DONE;				// had a valid packet that includes start, srcID, destID and end marker.
				DEBUG_TRACE("\nFound DestID: %02X", ch);
			}
			else {
				pStream-> status |= IO_STREAM_STATUS_PACKET_ERROR;				// invalid hex string.
				io_stream_advance_readindex( pStream, 3);							// index at next one passed the SOURCE ID.
				DEBUG_TRACE("\n Invalid DestID string: status= %d", (pStream-> status & IO_STREAM_STATUS_LOOK4_MASK));
			}
			break;
	} // end switch()
}// end msi_packet_router_parse_a_stream()
	

/**
 * It looks for a packet and parse source and destination IDs of all input streams.
 * If it found a valid host command packet for this device, then it will handle by
 * command server and break out the loop. This give other tasks to run in the system.
 *
 * @return none.
 *
 *
 * History:  Created on 2009/04/03 by Wai Fai Chin
 * 2011-04-18 -WFC- flag communication is active. System monitor module use this info to save power.
 */

void msi_packet_router_parse_all_stream( void )
{
	IO_STREAM_CLASS *piStream;
	BYTE i;
	BYTE ch;
	BYTE remainPacketLen;
	BYTE gotValidHex;
	BYTE status;
	BYTE needAdvReadindex;
	
	for ( i=0; i < MAX_NUM_IO_STREAM; i++) {
		piStream = &gaIoStream[ gbPR_CurStreamNum++ ];
		if ( gbPR_CurStreamNum >= MAX_NUM_IO_STREAM )
			gbPR_CurStreamNum = 0;
		status = piStream-> status;
		if ( (IO_STREAM_STATUS_ACTIVE & status) &&
			 (IO_STREAM_STATUS_DIR_IN & status) ) {					// only process input and active stream.
			msi_packet_router_parse_a_stream( piStream );
			
			status = piStream-> status;
			if ( IO_STREAM_STATUS_PACKET_ERROR & status ) { 			// if it has error
				// msi_packet_router_parse_a_stream() already advance readindex when it encountered error.
				piStream-> status &= ~(IO_STREAM_STATUS_LOOK4_MASK | IO_STREAM_STATUS_PACKET_ERROR );	// clear status and error bits. look for START marker.
			} // end if it has error.
			else { // no error
				status = status & IO_STREAM_STATUS_LOOK4_MASK;
				if ( IO_STREAM_STATUS_LOOK4_DONE == status ) {	// here, we have a valid packet with both valid srcID and destID.
					needAdvReadindex = TRUE;
					gbBiosSysStatus |= BIOS_SYS_STATUS_ACTIVE_COMMUNICATION;		// 2011-04-18 -WFC- flag communication is active. System monitor module use this info to save power.
					msi_packet_router_update_stream_registry( piStream );
					if (piStream-> destID == gtProductInfoFNV.devID ||						//If this packet is for this device OR
						piStream-> destID == IO_STREAM_BROADCAST_ID ) {					// for everyone in broadcast mode.
						gotValidHex = msi_packet_router_scan_byte_hex( piStream, &ch );	// scan command ID.
						if ( stream_router_get_a_new_stream( &gCmdPreParserState.poStream )) {		// if got a new output stream
							// construct the parser state object.
							gCmdPreParserState.poStream-> type	= piStream-> type;
							gCmdPreParserState.poStream-> status = IO_STREAM_STATUS_ACTIVE;		// output dir, active
							gCmdPreParserState.poStream-> sourceID	= gtProductInfoFNV.devID;
							gCmdPreParserState.poStream-> destID	= piStream-> sourceID;
							gCmdPreParserState.poStream-> packetLen = 0;
							gCmdPreParserState.streamObjNum = 0;		//TODO: search listener number based on device ID of registered listener.
							gCmdPreParserState.state	= CMD_STATE_LOOK_FOR_PARAMETERS;
							gCmdPreParserState.status	= CMD_STATUS_NO_ERROR ;
							gCmdPreParserState.index	= 0;
							
							if ( gotValidHex ) {
								gCmdPreParserState.cmd = ch;
								io_stream_readindex_eq_peekindex( piStream );			// read index is now point after cmdID.
								if ( piStream-> packetLen < MAX_STREAM_CIR_BUF_SIZE ) {
									remainPacketLen = piStream-> packetLen - 7;
									if ( remainPacketLen < MAX_STREAM_CIR_BUF_SIZE )
										gCmdPreParserState.strBuf[ remainPacketLen ] = 0;		// marked the end of string.
									io_stream_read_bytes( piStream, gCmdPreParserState.strBuf, remainPacketLen ); // copy the packet to strBuf excluded {srcID destID cmdID
								}
								needAdvReadindex = FALSE;								// readIndex is already advanced by io_stream_read_bytes();
							}
							else {
								gCmdPreParserState.status = CMD_STATUS_PRE_WRONG_CMD_FIELD;	// PRE PARSER HAD WRONG CHAR IN THE COMMAND FIELD.
							}
								
							//if ( piStream-> destID == IO_STREAM_BROADCAST_ID )  {
								// TODO: delay a little bit based on the device ID of this device. This will prevent over run the network.
							//}
							#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )
								cmd_act_execute( &gCmdPreParserState );				// handover the newly constructed parser object to the cmdaction module, a command server..
							#endif
							DEBUG_TRACE("\n Success: %s", gCmdPreParserState.strBuf);
							stream_router_free_stream( gCmdPreParserState.poStream );	// put resource back to stream pool.
						}
					} // end if this packet is for me.
					//else { TODO: implement to handle a packet not for this device. We should only forward this packet to other router or master unit.
					//	io_stream_advance_readindex( piStream, piStream-> packetLen);	// advance read index
					//}
					if ( needAdvReadindex ) 
						io_stream_advance_readindex( piStream, piStream-> packetLen);	// completed this packet, advance read index to the next possible packet.
					// clear status of this packet in this stream for ready next packet.
					piStream-> packetLen = 0;
					piStream-> status &= ~(IO_STREAM_STATUS_LOOK4_MASK | IO_STREAM_STATUS_PACKET_ERROR );	// clear status and error bits.
					break;		// done with one packet, EXIT the loop to do other tasks.
				} // end if ( IO_STREAM_STATUS_LOOK4_DONE == status )
			} // end if it has error {} else {}
		} // end if this stream is active and dirction is input.
	} // end for() loop to process each stream
}// end msi_packet_router_parse_all_stream()
	


/**
 * It looks for source ID in hex format in the given input stream.
 *
 * @param	pStream -- pointer to iostream object.
 * @param	pbV		-- pointer to binary value of the converted 2 hex characters.
 * @return TRUE if it found a valid hex value in binary byte.
 *
 *
 * History:  Created on 2009/04/03 by Wai Fai Chin
 */

BYTE msi_packet_router_scan_byte_hex( IO_STREAM_CLASS *pStream, BYTE *pbV )
{
	BYTE i;
	BYTE ch;
	BYTE status;
	
	status = TRUE;				// assumed found valid hex digits.

	for ( i=0; i<2; i++ )	{
		io_stream_peek_a_byte( pStream, &ch ); // ch = hex digit. 
		ch = hex_char_to_nibble( ch );
		if ( 0xFF != ch  ) {					// if ch is a valid hex char
			*pbV <<= 4;
			*pbV |= ch;
		}
		else {
			status = FALSE;		// found no hex digits.
			break;
		}
	} 
	
	return status;
}// end msi_packet_router_scan_byte_hex()



/**
 * It gets an index of the stream registry of a given sourceID.
 * If it cannot find an existing registry, it will create a new one.
 * If registry runs out of memory, it will bump the oldest one out and 
 * put a new record in its place.
 *
 * @param	sourceID	-- source ID of a stream.
 *
 * @return	index of stream registry of a specified source ID.
 *
 * History:  Created on 2009/04/21 by Wai Fai Chin
 */

BYTE msi_packet_router_get_stream_registry_index( BYTE sourceID )
{
	BYTE i;
	
	for ( i = 0; i < MAX_NUM_STREAM_REGISTRY; i++) {
		if ( (sourceID == gaStreamReg_sourceID[i] ) ||
			(IO_STREAM_INVALID_SRCID == gaStreamReg_sourceID[i] ))		// if found existing record OR unused record, exit the for loop.
			break;
	}
	
	if ( i >= MAX_NUM_STREAM_REGISTRY ) {				// if it has no more room for new record.
		i = gbStreamReg_OldestIndex++;					// We will replace the oldest one with the new record in its place.
		if ( gbStreamReg_OldestIndex >= MAX_NUM_STREAM_REGISTRY )
			gbStreamReg_OldestIndex = 0;
	}
	
	return i;
} // end msi_packet_router_get_stream_registry_index()


/**
 * It sets stream type of a stream of a given sourceID.
 * If it cannot find an existing record, it will create a new one.
 * If parser status records runs out of memory, it will bump the oldest one and 
 * put a new record in its place.
 *
 * It also delegates other stream drivers to build its internal destination lookup table based on stream type.
 * Other stream driver may or may not has its own internal destination lookup table,
 * e.g. RF modem has its own address scheme. High level Uart stream driver has none.
 * Ethernet driver has ip address. It delegates this task to the concrete stream driver
 * based on the stream type.
 *
 * @param	piStream	-- pointer to a concrete input stream
 *
 * @return	none.
 *
 * History:  Created on 2009/04/21 by Wai Fai Chin
 */

void msi_packet_router_update_stream_registry( IO_STREAM_CLASS *piStream )
{
	BYTE i;
	
	i = msi_packet_router_get_stream_registry_index( piStream-> sourceID );
	gaStreamReg_sourceID[ i ]	= piStream-> sourceID;
	gaStreamReg_type[ i ]		= piStream-> type;
	
	// other stream driver may or may not has its own internal destination lookup table,
	// e.g. RF modem has its own address scheme. High level Uart stream driver has none.
	// Ethernet driver has ip address. It delegates this task to the concrete stream driver
	// based on the stream type.
	stream_router_build_stream_driver_internal_route_table( piStream );
	
} // end msi_packet_router_update_stream_registry(,)


/**
 * It stores parser status of a stream of a given sourceID.
 * If it cannot find an existing record, it will create a new one.
 * If parser status records runs out of memory, it will bump the oldest one and 
 * put a new record in its place.
 *
 * @param	sourceID	-- source ID of a stream.
 * @param	cmd			-- command code.
 * @param	status		-- status of parser object.
 *
 * @return	none.
 *
 * History:  Created on 2009/04/21 by Wai Fai Chin
 */

void msi_packet_router_update_parser_status( BYTE sourceID, BYTE cmd, BYTE status )
{
	BYTE i;
	
	i = msi_packet_router_get_stream_registry_index( sourceID );
	gaStreamReg_sourceID[ i ]	= sourceID;
	gaStreamReg_cmd[ i ]		= cmd;
	gaStreamReg_cmdStatus[ i ]	= status;
	
} // end msi_packet_router_update_parser_status(,)


/**
 * It provides both previous command and status of a stream of a given sourceID.
 *
 * @param	sourceID	-- source ID of a stream.
 * @param	*pCmd		-- to get command code.
 * @param	*pStatus	-- to get cmd status.
 *
 * @return	TRUE if it found a valid sourceID.
 *
 * History:  Created on 2009/04/21 by Wai Fai Chin
 */

BYTE msi_packet_router_get_parser_status( BYTE sourceID, BYTE *pCmd, BYTE *pStatus )
{
	BYTE i;
	BYTE result;
	
	result = FALSE;
	for ( i = 0; i < MAX_NUM_STREAM_REGISTRY; i++) {
		if ( sourceID == gaStreamReg_sourceID[i])		// if found existing record, exit the for loop.
			break;
	}
	
	if ( i < MAX_NUM_STREAM_REGISTRY ) {						// if found existing record
		*pCmd	= gaStreamReg_cmd[i];
		*pStatus = gaStreamReg_cmdStatus[i];
		result = TRUE;
	}
	
	return result;
} // end msi_packet_router_get_parser_status()

