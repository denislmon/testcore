/*! \file rf_config.h \brief high level RF device configuration functions.*/
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
/// \ingroup Application
/// \defgroup RF_Device It manages RF device configuration through well defined high level API.
/// \code #include "rf_config.h" \endcode
/// \par Overview
///     This is an abstract rf_device configuration API. The low level RF module will implement all abstract API. It manages high and low levels rf device configuration.
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


#ifndef MSI_RF_DEVICE_CONFIG_H
#define MSI_RF_DEVICE_CONFIG_H

#include "commonlib.h"
#include "io_stream.h"
#include "pt.h"
#include "timer.h"
#include "nv_cnfg_mem.h"

#define RF_DEVICE_RUN_MODE_NORMAL			0
#define RF_DEVICE_RUN_MODE_QUERY_CONFIG		1
#define RF_DEVICE_RUN_MODE_SET_CONFIG		2

extern	BYTE	gbRFDeviceRunMode;

/*!
  \brief This is a clone-able thread class for query or set RF device configuration.
  \par overview
	  It query or set RF device configuration.
 \note
*/

typedef struct RF_DEVICE_CONFIG_CLASS_TAG {
								/// main, or parent thread
	struct pt 	m_mainPt;
								/// child thread
	struct pt 	m_pt;
								/// timer
	TIMER_T		timer;
								/// pointer to IO stream. It could use as an Input and Output stream.
	IO_STREAM_CLASS 		*pStream;
								/// pointer to RF device settings data structure.
	RF_DEVICE_SETTINGS_T	*pRfSettings;
								/// status bits,
	BYTE					status;
								/// Temporary variable to use by a thread.
	BYTE					bTmpA;
								/// Temporary variable to use by a thread.
	BYTE					bTmpB;

}RF_DEVICE_CONFIG_CLASS_T;

#define RF_CONFIG_OBJ_STATUS_IN_CONFIG_bm			0x01
#define RF_CONFIG_OBJ_STATUS_GOT_VALID_READS_bm		0x02
#define RF_CONFIG_OBJ_STATUS_EXIT_THREAD_bm			0x40
#define RF_CONFIG_OBJ_STATUS_CONFIG_COMPLETE_bm		0x80

// extern	RF_DEVICE_SETTINGS_T 	gRfDeviceSettingsFNV;

#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )		// 2012-04-23 -WFC-
extern BYTE		gbXBeeChannelID;
extern WORD16	gwXBeeNetworkID;
extern BYTE		gbXBeePowerLevel; // 2013-04-02 -DLM-

/** \def rf_config_save_read_config_value
 * It saves valid read values from RF device
 *
 * @return none.
 *
 * History:  Created on 2012-04-23 by Wai Fai Chin
 */
#define rf_config_save_read_config_value()		gRfDeviceSettingsFNV.channel = (WORD16)gbXBeeChannelID; gRfDeviceSettingsFNV.networkID = gwXBeeNetworkID; gRfDeviceSettingsFNV.powerlevel = (WORD16)gbXBeePowerLevel

/** \def rf_config_save_read_config_value
 * It saves valid read values from RF device
 *
 * @return none.
 *
 * History:  Created on 2012-04-23 by Wai Fai Chin
 */
#define rf_config_is_read_value_eq_user_defined()		( gRfDeviceSettingsFNV.channel == (WORD16)gbXBeeChannelID && gRfDeviceSettingsFNV.networkID == gwXBeeNetworkID && gRfDeviceSettingsFNV.powerlevel == (WORD16)gbXBeePowerLevel) ? 1 : 0

#endif

// The following are high level abstract methods. They need to implement concrete methods by the lower level module.
void	rf_device_settings_1st_init( void );
void	rf_device_settings_init( void );
void	rf_config_thread_runner( void );
char	rf_device_config_main_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj );


#endif	// MSI_RF_DEVICE_CONFIG_H
//@}
