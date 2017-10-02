/*! \file rf_config.c \brief high level RF device configuration functions.*/
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
//  History:  Created on 2012-03-28 by Wai Fai Chin
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
//
//
// ****************************************************************************

#include	"config.h"
#include	"rf_config.h"

BYTE		gbRFDeviceRunMode;

#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )
#include "rf_config_xbee.c"
#endif


