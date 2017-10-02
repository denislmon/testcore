/*! \file msi_packet_router.h \brief packet router functions.*/
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
//  History:  Created on 2009/03/18 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup msi_packet_router It routes high level IO stream between application and stream drivers.
/// \code #include "msi_packet_router.h" \endcode
/// \par Overview
///     It routes high level IO stream between application and stream drivers.
///  It maitains a registry of streams with sourceID, stream type, command and status.
///  This registry keep track command status of each device. Other object uses this registry
///  to dynamically create ostream for send data to registered device.
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


#ifndef MSI_PACKET_ROUTER_H
#define MSI_PACKET_ROUTER_H

#include "commonlib.h"
#include "io_stream.h"
#include "stream_router.h"

#define  MAX_NUM_STREAM_REGISTRY	8

#define  MSI_PACKET_START_CHAR        '{'
#define  MSI_PACKET_END_CHAR          '}'

/*
#define MSI_PP_FOUND_NO_PACKET_YET		0
#define MSI_PP_VALID_PACKET_FOR_ME		1
#define MSI_PP_VALID_PACKET_NOT_4_ME	2
*/


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
extern BYTE	gaStreamReg_sourceID[ MAX_NUM_STREAM_REGISTRY ];
															/// stream type of a stream registry
extern BYTE	gaStreamReg_type[ MAX_NUM_STREAM_REGISTRY ];
															/// command of a stream registry
extern BYTE	gaStreamReg_cmd[ MAX_NUM_STREAM_REGISTRY ];
															/// command status of a stream registry
extern BYTE	gaStreamReg_cmdStatus[ MAX_NUM_STREAM_REGISTRY ];

/// private: circular index of gaStreamReg. It uses for bump the oldest registry and put a new record in its place.
extern BYTE	gbStreamReg_OldestIndex;


void	msi_packet_router_init( void );
void	msi_packet_router_parse_all_stream( void );
BYTE	msi_packet_router_get_stream_registry_index( BYTE sourceID );
BYTE	msi_packet_router_get_parser_status( BYTE sourceID, BYTE *pCmd, BYTE *pStatus );
void	msi_packet_router_update_parser_status( BYTE sourceID, BYTE cmd, BYTE status );
void	msi_packet_router_update_stream_registry( IO_STREAM_CLASS *piStream );

#endif	// MSI_PACKET_ROUTER_H
//@}
