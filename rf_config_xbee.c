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
// Software layer:  Concrete layer
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
#include	"stream_router.h"
#include	<stdio.h>
#include	"nvmem.h"
#include	"bios.h"

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER)
extern  IO_STREAM_CLASS		*gpUart2Istream;
#define XBEE_STREAM_PTR		gpUart2Istream
#define XBEE_IO_STREAM_TYPE	IO_STREAM_TYPE_UART_2
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )
extern  IO_STREAM_CLASS		*gpUart1Istream;
#define XBEE_STREAM_PTR		gpUart1Istream
#define XBEE_IO_STREAM_TYPE	IO_STREAM_TYPE_UART_1
#endif

#define IOS_XBEE_STATUS_LOOK4_O			0X00
#define IOS_XBEE_STATUS_LOOK4_CR		0X01
#define IOS_XBEE_STATUS_LOOK4_K			0X02
#define IOS_XBEE_STATUS_LOOK4_VALUE		0X03
#define IOS_XBEE_STATUS_LOOK4_MASK		0X07
#define IOS_XBEE_STATUS_GOT_WHAT_LOOKING4		0X08

// const char gcStrFmt_XBeeMinCnfg_Cmd[]PROGMEM = "ATCH%X,ID%X,CE0,A10,SM1,WR\r"; // -DLM- 2013-04-02
// const char gcStrFmt_XBeeMinCnfg_Cmd[]PROGMEM = "ATCH%X,ID%X,PL%X,CE0,A10,SM1,WR\r";
const char gcStrFmt_XBeeMinCnfg_Cmd[]PROGMEM = "ATCH%X,ID%X,PL%X,BD%X,CE0,A10,SM1,WR\r";  // -WFC- 2014-06-02


BYTE	gbXBeeChannelID;
WORD16	gwXBeeNetworkID;
BYTE	gbXBeePowerLevel; // 2013-04-02 -DLM-


// private methods:
BYTE	rf_config_xbee_got_ok_cr( IO_STREAM_CLASS *pStream );
char	rf_device_try_enter_config_mode_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj );
char	rf_device_query_config_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj );
char	rf_device_set_config_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj );
BYTE	rf_config_xbee_get_byte( IO_STREAM_CLASS *pStream, BYTE *pbV );
BYTE 	rf_config_xbee_get_word( IO_STREAM_CLASS *pStream, WORD16 *pwV );
BYTE	rf_config_scan_byte_hex( IO_STREAM_CLASS *pStream, BYTE *pbV );
BYTE	rf_config_scan_word_hex( IO_STREAM_CLASS *pStream, WORD16 *pwV );

/**
 * It sets rf configuration settings data structure.
 * @post
 *
 * History:  Created on 2012-03-28 by Wai Fai Chin
 */

void rf_device_settings_1st_init( void )
{
	nv_cnfg_fram_default_rf_device_cnfg();
} // end rf_device_settings_1st_init()

/**
 * It sets rf configuration settings data structure.
 * @post
 *
 * History:  Created on 2012-03-28 by Wai Fai Chin
 */

void rf_device_settings_init( void )
{
	gbXBeeChannelID = 0;
	gwXBeeNetworkID = 0;
	gbXBeePowerLevel = 1;
} // end rf_device_settings_init()


/**
 * It runs rf config related threads.
 *
 * History:  Created on 2012-04-17 by Wai Fai Chin
 * 2016-03-30 -WFC- Only configure XBee if device type is an XBee.
 */

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER)

void rf_config_thread_runner( void )
{
	RF_DEVICE_CONFIG_CLASS_T rfConfigThread;
	if ( ( RF_DEVICE_STATUS_QUERY_bm | RF_DEVICE_STATUS_SET_CONFIG_bm ) & gRfDeviceSettingsFNV.status ) {
		rfConfigThread.status = RF_CONFIG_OBJ_STATUS_IN_CONFIG_bm;
		rfConfigThread.pRfSettings = &gRfDeviceSettingsFNV;
		PT_INIT( &rfConfigThread.m_mainPt );						// init thread.
		PT_INIT( &rfConfigThread.m_pt );
		while ( ! ( (RF_CONFIG_OBJ_STATUS_CONFIG_COMPLETE_bm  | RF_CONFIG_OBJ_STATUS_EXIT_THREAD_bm ) & rfConfigThread.status ) ) {
			stream_router_all_stream_drivers_receive();
			rf_device_config_main_thread( &rfConfigThread );
		}
		gRfDeviceSettingsFNV.status &= ~( RF_DEVICE_STATUS_QUERY_bm | RF_DEVICE_STATUS_SET_CONFIG_bm );
		if ( (RF_CONFIG_OBJ_STATUS_GOT_VALID_READS_bm | RF_CONFIG_OBJ_STATUS_CONFIG_COMPLETE_bm) & rfConfigThread.status ) {
			//RF_DEVICE_STATUS_VALUES_FROM_DEV_bm
			if ( rf_config_is_read_value_eq_user_defined()) {
				rf_config_save_read_config_value();
				gRfDeviceSettingsFNV.status |= RF_DEVICE_STATUS_VALUES_FROM_DEV_bm;
				nv_cnfg_fram_save_rf_device_cnfg();
			}
		}
	}
} // end rf_config_thread_runner()
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 ||  CONFIG_PRODUCT_AS == CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )

#if ( CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281)

void rf_config_thread_runner( void )
{
	RF_DEVICE_CONFIG_CLASS_T rfConfigThread;
	if ( ( RF_DEVICE_STATUS_QUERY_bm | RF_DEVICE_STATUS_SET_CONFIG_bm ) & gRfDeviceSettingsFNV.status ) {
		BIOS_TURN_ON_XBEE;
		BIOS_DISABLED_RS232_RECEIVE;
		rfConfigThread.status = RF_CONFIG_OBJ_STATUS_IN_CONFIG_bm;
		rfConfigThread.pRfSettings = &gRfDeviceSettingsFNV;
		PT_INIT( &rfConfigThread.m_mainPt );						// init thread.
		PT_INIT( &rfConfigThread.m_pt );
		while ( ! ( (RF_CONFIG_OBJ_STATUS_CONFIG_COMPLETE_bm  | RF_CONFIG_OBJ_STATUS_EXIT_THREAD_bm ) & rfConfigThread.status ) ) {
			stream_router_all_stream_drivers_receive();
			rf_device_config_main_thread( &rfConfigThread );
		}
		gRfDeviceSettingsFNV.status &= ~( RF_DEVICE_STATUS_QUERY_bm | RF_DEVICE_STATUS_SET_CONFIG_bm );
		if ( (RF_CONFIG_OBJ_STATUS_GOT_VALID_READS_bm | RF_CONFIG_OBJ_STATUS_CONFIG_COMPLETE_bm) & rfConfigThread.status ) {
			//RF_DEVICE_STATUS_VALUES_FROM_DEV_bm
			if ( rf_config_is_read_value_eq_user_defined()) {
				rf_config_save_read_config_value();
				gRfDeviceSettingsFNV.status |= RF_DEVICE_STATUS_VALUES_FROM_DEV_bm;
				nv_cnfg_fram_save_rf_device_cnfg();
			}
		}
	}

	if ( RF_DEVICE_STATUS_ENABLED_bm & gRfDeviceSettingsFNV.status  ) {
		BIOS_TURN_ON_XBEE;
		BIOS_DISABLED_RS232_RECEIVE;
	}
	else {
		BIOS_TURN_OFF_XBEE;
		BIOS_ENABLED_RS232_RECEIVE;
	}
} // end rf_config_thread_runner()

#elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )    // 2013-04-26 -WFC-
void rf_config_thread_runner( void )
{
	RF_DEVICE_CONFIG_CLASS_T rfConfigThread;

	if ( RF_DEVICE_TYPE_XBEE == gRfDeviceSettingsFNV.deviceType  ) { // 2016-03-30 -WFC-
		if ( ( RF_DEVICE_STATUS_QUERY_bm | RF_DEVICE_STATUS_SET_CONFIG_bm ) & gRfDeviceSettingsFNV.status ) {
			BIOS_TURN_ON_XBEE;
			rfConfigThread.status = RF_CONFIG_OBJ_STATUS_IN_CONFIG_bm;
			rfConfigThread.pRfSettings = &gRfDeviceSettingsFNV;
			PT_INIT( &rfConfigThread.m_mainPt );						// init thread.
			PT_INIT( &rfConfigThread.m_pt );
			while ( ! ( (RF_CONFIG_OBJ_STATUS_CONFIG_COMPLETE_bm  | RF_CONFIG_OBJ_STATUS_EXIT_THREAD_bm ) & rfConfigThread.status ) ) {
				stream_router_all_stream_drivers_receive();
				rf_device_config_main_thread( &rfConfigThread );
			}
			gRfDeviceSettingsFNV.status &= ~( RF_DEVICE_STATUS_QUERY_bm | RF_DEVICE_STATUS_SET_CONFIG_bm );
			if ( (RF_CONFIG_OBJ_STATUS_GOT_VALID_READS_bm | RF_CONFIG_OBJ_STATUS_CONFIG_COMPLETE_bm) & rfConfigThread.status ) {
				//RF_DEVICE_STATUS_VALUES_FROM_DEV_bm
				if ( rf_config_is_read_value_eq_user_defined()) {
					rf_config_save_read_config_value();
					gRfDeviceSettingsFNV.status |= RF_DEVICE_STATUS_VALUES_FROM_DEV_bm;
					nv_cnfg_fram_save_rf_device_cnfg();
				}
			}
		}
	}

	if ( RF_DEVICE_STATUS_ENABLED_bm & gRfDeviceSettingsFNV.status  ) {
		BIOS_TURN_ON_XBEE;
	}
	else {
		BIOS_TURN_OFF_XBEE;
	}
} // end rf_config_thread_runner()
#endif  // elif (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA192D3 )
#endif

/**
 * It tries to make XBEE enter into ATT command mode. It will tries three times before it give up.
 * @post it query RF configuration setting from an RF device and save it to pRFDCobj->pRfSettings.
 *
 * History:  Created on 2012-04-16 by Wai Fai Chin
 * 2014-06-02 -WFC- changed Xbee baudrate based on speed mode.
 */

//PT_THREAD( rf_device_config_main_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj )) // Doxygen cannot handle this macro
char rf_device_config_main_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj )
{
	BYTE	str[8];

	PT_BEGIN( &(pRFDCobj->m_mainPt) );
		PT_SPAWN( &(pRFDCobj->m_mainPt), &(pRFDCobj->m_pt), rf_device_try_enter_config_mode_thread( pRFDCobj ) );
		if ( RF_CONFIG_OBJ_STATUS_IN_CONFIG_bm & pRFDCobj->status ) {
			if ( RF_DEVICE_STATUS_QUERY_bm & pRFDCobj->pRfSettings->status ) {
				PT_SPAWN( &(pRFDCobj->m_mainPt), &(pRFDCobj->m_pt), rf_device_query_config_thread( pRFDCobj ) );
			}

			if ((RF_DEVICE_STATUS_SET_CONFIG_bm & pRFDCobj->pRfSettings->status) &&
				( 0 != gRfDeviceSettingsFNV.channel) &&
				( 0 != gRfDeviceSettingsFNV.networkID) ) {
				PT_SPAWN( &(pRFDCobj->m_mainPt), &(pRFDCobj->m_pt), rf_device_set_config_thread( pRFDCobj ) );
				PT_SPAWN( &(pRFDCobj->m_mainPt), &(pRFDCobj->m_pt), rf_device_query_config_thread( pRFDCobj ) );
			}

			// exit config mode.
			if (stream_router_get_a_new_stream( &(pRFDCobj->pStream) ) ) {		// get an output stream to this listener.
				pRFDCobj->pStream->type		= XBEE_IO_STREAM_TYPE;
				pRFDCobj->pStream->status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
				// prepare for reading channel number
				str[0] = 'A';	str[1] = 'T';	str[2] = 'C';	str[3] = 'N';	str[4] = '\r'; str[5] = 0;
				stream_router_routes_a_ostream_now( pRFDCobj->pStream, str, 5 );
				stream_router_free_stream( pRFDCobj->pStream );	// put resource back to stream pool.
			}
			timer_mSec_set( &pRFDCobj->timer, TT_1SEC);
			PT_WAIT_UNTIL( &(pRFDCobj->m_mainPt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// wait for 1.5 seconds as required by the XBee OR got a OK<cr> pattern.
			pRFDCobj->status |= RF_CONFIG_OBJ_STATUS_CONFIG_COMPLETE_bm;
		}// in config mode
		//2014-06-02 -WFC- v
		#if (CONFIG_HIGH_SPEED_RF_MSG == TRUE )
			serial1_port_init( SR_BAUD_38400_V );		// reconfigure serial port based to match XBee baudrate
		#else
			serial1_port_init( SR_BAUD_9600_V );		// reconfigure serial port based to match XBee baudrate
		#endif
		//2014-06-02 -WFC- ^

		pRFDCobj->status |= RF_CONFIG_OBJ_STATUS_EXIT_THREAD_bm;
	PT_END( &(pRFDCobj->m_mainPt) );
} // end rf_device_config_main_thread()


/**
 * It tries to make XBEE enter into ATT command mode. It will tries three times before it give up.
 * @post it query RF configuration setting from an RF device and save it to pRFDCobj->pRfSettings.
 *
 * History:  Created on 2012-04-16 by Wai Fai Chin
 * 2014-06-02 -WFC- try different baudrate to enter Xbee config.
 *
 */
const UINT16  gwBaudrateTbl[] PROGMEM= { SR_BAUD_38400_V, SR_BAUD_9600_V };

//PT_THREAD( rf_device_try_enter_config_mode_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj )) // Doxygen cannot handle this macro
char rf_device_try_enter_config_mode_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj )
{
	BYTE	str[8];
	WORD16  wBaudrate;

	PT_BEGIN( &(pRFDCobj->m_pt) );
		pRFDCobj->bTmpB = 0;
		for ( ; pRFDCobj->bTmpB < 2; pRFDCobj->bTmpB++) {
			memcpy_P( &wBaudrate,  &gwBaudrateTbl[ pRFDCobj->bTmpB ],  sizeof(UINT16));
			serial1_port_init( wBaudrate );		// reconfigure serial port based on new system clock speed.
			pRFDCobj->status &= ~RF_CONFIG_OBJ_STATUS_IN_CONFIG_bm;
			for ( pRFDCobj->bTmpA =0; pRFDCobj->bTmpA < 3; pRFDCobj->bTmpA++ ) {
				// Prepare to set XBEE into ATT command mode
				timer_mSec_set( &pRFDCobj->timer, TT_2SEC);
				PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) );		// wait for 2 seconds to give enought time for the xmit data to empty out.
				if (stream_router_get_a_new_stream( &(pRFDCobj->pStream) ) ) {		// get an output stream to this listener.
					pRFDCobj->pStream->type		= XBEE_IO_STREAM_TYPE;
					pRFDCobj->pStream->status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
					str[0] = '+';	str[1] = '+';	str[2] = '+';		str[3] = 0;
					stream_router_routes_a_ostream_now( pRFDCobj->pStream, str, 3 );
					stream_router_free_stream( pRFDCobj->pStream );	// put resource back to stream pool.
				}
				timer_mSec_set( &pRFDCobj->timer, TT_2SEC);
				XBEE_STREAM_PTR-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);
				PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// wait for 1.5 seconds as required by the XBee OR got a OK<cr> pattern.
				// by now, XBee is in ATT command mode.
				if (  IOS_XBEE_STATUS_GOT_WHAT_LOOKING4 & XBEE_STREAM_PTR->status ) {
					pRFDCobj->status |= RF_CONFIG_OBJ_STATUS_IN_CONFIG_bm;
					break;
				}
			} // end for()
			if ( RF_CONFIG_OBJ_STATUS_IN_CONFIG_bm & pRFDCobj->status )
				break;
		} // end for (;

	PT_END( &(pRFDCobj->m_pt) );
} // end rf_device_try_enter_config_mode_thread()



/**
 * It queries rf configuration settings. It is assumed XBEE is already in ATT command mode.
 * @post it query RF configuration setting from an RF device and save it global variable of this module.
 *
 * History:  Created on 2012-03-28 by Wai Fai Chin
 */

//PT_THREAD( rf_device_query_config_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj )) // Doxygen cannot handle this macro
char rf_device_query_config_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj )
{
	BYTE	str[12];

	PT_BEGIN( &(pRFDCobj->m_pt) );
		if (stream_router_get_a_new_stream( &(pRFDCobj->pStream) ) ) {		// get an output stream to this listener.
			pRFDCobj->pStream->type		= XBEE_IO_STREAM_TYPE;
			pRFDCobj->pStream->status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
			// prepare for reading channel number
			str[0] = 'A';	str[1] = 'T';	str[2] = 'C';	str[3] = 'H';	str[4] = '\r'; str[5] = 0;
			stream_router_routes_a_ostream_now( pRFDCobj->pStream, str, 5 );
			stream_router_free_stream( pRFDCobj->pStream );	// put resource back to stream pool.
		}
		timer_mSec_set( &pRFDCobj->timer, TT_1SEC);
		XBEE_STREAM_PTR-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);
		XBEE_STREAM_PTR-> status |= IOS_XBEE_STATUS_LOOK4_CR;
		PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_get_byte( XBEE_STREAM_PTR, &gbXBeeChannelID ));
		if (  IOS_XBEE_STATUS_GOT_WHAT_LOOKING4 & XBEE_STREAM_PTR->status ) {
			if (stream_router_get_a_new_stream( &(pRFDCobj->pStream) ) ) {		// get an output stream to this listener.
				pRFDCobj->pStream->type		= XBEE_IO_STREAM_TYPE;
				pRFDCobj->pStream->status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
				// prepare for reading network ID.
				str[0] = 'A';	str[1] = 'T';	str[2] = 'I';	str[3] = 'D';	str[4] = '\r'; str[5] = 0;
				stream_router_routes_a_ostream_now( pRFDCobj->pStream, str, 5 );
				stream_router_free_stream( pRFDCobj->pStream );	// put resource back to stream pool.
			}
			timer_mSec_set( &pRFDCobj->timer, TT_1SEC);
			XBEE_STREAM_PTR-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);
			XBEE_STREAM_PTR-> status |= IOS_XBEE_STATUS_LOOK4_CR;
			PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_get_word( XBEE_STREAM_PTR, &gwXBeeNetworkID ));
			if (  IOS_XBEE_STATUS_GOT_WHAT_LOOKING4 & XBEE_STREAM_PTR->status ) {
				if (stream_router_get_a_new_stream( &(pRFDCobj->pStream) ) ) {		// get an output stream to this listener.
					pRFDCobj->pStream->type		= XBEE_IO_STREAM_TYPE;
					pRFDCobj->pStream->status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
					// prepare for reading power level.
					str[0] = 'A';	str[1] = 'T';	str[2] = 'P';	str[3] = 'L';	str[4] = '\r'; str[5] = 0;
					stream_router_routes_a_ostream_now( pRFDCobj->pStream, str, 5 );
					stream_router_free_stream( pRFDCobj->pStream );	// put resource back to stream pool.
				}
				timer_mSec_set( &pRFDCobj->timer, TT_1SEC);
				XBEE_STREAM_PTR-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);
				XBEE_STREAM_PTR-> status |= IOS_XBEE_STATUS_LOOK4_CR;
				PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_get_byte( XBEE_STREAM_PTR, &gbXBeePowerLevel ));
				if (  IOS_XBEE_STATUS_GOT_WHAT_LOOKING4 & XBEE_STREAM_PTR->status ) {
					pRFDCobj->status |= RF_CONFIG_OBJ_STATUS_GOT_VALID_READS_bm;
				}
			}
		}

	PT_END( &(pRFDCobj->m_pt) );
} // end rf_device_query_config_thread()

/**
 * It set rf configuration settings.
 * @post it sets RF configuration setting to an RF device based on the pRFDCobj->pRfSettings.
 *
 * History:  Created on 2012-04-19 by Wai Fai Chin
 * 2014-06-02 -WFC- changed Xbee baudrate based on speed mode.
 */

//PT_THREAD( rf_device_set_config_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj )) // Doxygen cannot handle this macro
char rf_device_set_config_thread( RF_DEVICE_CONFIG_CLASS_T *pRFDCobj )
{
	BYTE	n;
	BYTE	str[40];

	PT_BEGIN( &(pRFDCobj->m_pt) );
		if (stream_router_get_a_new_stream( &(pRFDCobj->pStream) ) ) {		// get an output stream to this listener.
			pRFDCobj->pStream->type		= XBEE_IO_STREAM_TYPE;
			pRFDCobj->pStream->status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
			// prepare for reading channel number
		    // n = sprintf_P( str, gcStrFmt_XBeeMinCnfg_Cmd, pRFDCobj->pRfSettings->channel, pRFDCobj->pRfSettings->networkID); // 2013-04-02 -DLM-
			// 2014-06-02 -WFC- v
			#if (CONFIG_HIGH_SPEED_RF_MSG == TRUE )
				n = sprintf_P( str, gcStrFmt_XBeeMinCnfg_Cmd, pRFDCobj->pRfSettings->channel, pRFDCobj->pRfSettings->networkID, pRFDCobj->pRfSettings->powerlevel, 5);
			#else
				n = sprintf_P( str, gcStrFmt_XBeeMinCnfg_Cmd, pRFDCobj->pRfSettings->channel, pRFDCobj->pRfSettings->networkID, pRFDCobj->pRfSettings->powerlevel, 3);
			#endif
			// 2014-06-02 -WFC- ^
			stream_router_routes_a_ostream_now( pRFDCobj->pStream, str, n );
			stream_router_free_stream( pRFDCobj->pStream );	// put resource back to stream pool.
		}
		timer_mSec_set( &pRFDCobj->timer, TT_2SEC);
		XBEE_STREAM_PTR-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);
		PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// wait for 1.5 seconds as required by the XBee OR got a OK<cr> pattern.
		PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// wait for 1.5 seconds as required by the XBee OR got a OK<cr> pattern.
		PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// wait for 1.5 seconds as required by the XBee OR got a OK<cr> pattern.
		PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// wait for 1.5 seconds as required by the XBee OR got a OK<cr> pattern.
		PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// wait for 1.5 seconds as required by the XBee OR got a OK<cr> pattern.
		PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// wait for 1.5 seconds as required by the XBee OR got a OK<cr> pattern.
		// 2013-04-02 -DLM- v
		PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// wait for 1.5 seconds as required by the XBee OR got a OK<cr> pattern.
		PT_WAIT_UNTIL( &(pRFDCobj->m_pt), timer_mSec_expired( &pRFDCobj->timer ) || rf_config_xbee_got_ok_cr( XBEE_STREAM_PTR ));	// -WFC- 2014-06-02
		if (  IOS_XBEE_STATUS_GOT_WHAT_LOOKING4 & XBEE_STREAM_PTR->status ) {
			pRFDCobj->status |= RF_CONFIG_OBJ_STATUS_IN_CONFIG_bm;
			break;
		}

	PT_END( &(pRFDCobj->m_pt) );

} // end rf_device_set_config_thread()


/**
 * It looks for OK<cr> pattern in the given input stream.
 *
 * @param	pStream -- pointer to iostream object.
 * @return	TRUE if found OKcr pattern, else FALSE.
 *
 *
 * History:  Created on 2012-04-03 by Wai Fai Chin
 */

BYTE rf_config_xbee_got_ok_cr( IO_STREAM_CLASS *pStream )
{
	UINT16 wStatus;
	BYTE status;
	BYTE lookFor;
	BYTE strBuf[8];

	status = FALSE;
	lookFor = pStream-> status & IOS_XBEE_STATUS_LOOK4_MASK;
	switch( lookFor ) {
		case IOS_XBEE_STATUS_LOOK4_O:
			if ( io_stream_scan_left_side( pStream, 'O' ) ) {					// if found the packet START marker.
				pStream-> status &= ~IOS_XBEE_STATUS_LOOK4_MASK;
				pStream-> status |= IOS_XBEE_STATUS_LOOK4_CR;					// changed status to look for cr
				DEBUG_TRACE("%s", "\nO");
			}
			break;
		case IOS_XBEE_STATUS_LOOK4_CR:
			wStatus = io_stream_scan_right_side( pStream, '\r', 'O' );
			if ( MSI_LIB_FOUND_START == (wStatus & MSI_LIB_FOUND_MATCH)   ) {	// if found a new 'O' before find the cr,
				pStream-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);
				pStream-> status |= IOS_XBEE_STATUS_LOOK4_CR;					// changed status to look for packet END marker.
				DEBUG_TRACE("%s", "\nFound Start");
				break;
			}
			if ( MSI_LIB_FOUND_MATCH == (wStatus & MSI_LIB_FOUND_MATCH)   ) {		// if found the cr marker.
				pStream-> packetLen = (BYTE) ( 0x00FF & wStatus);
				if ( 3 == pStream-> packetLen ) {
					DEBUG_TRACE("\nFound END: len=%d", pStream-> packetLen);
					io_stream_read_bytes( pStream, strBuf, 3 ); // copy the packet to strBuf excluded {srcID destID cmdID
					if ( 'K' == strBuf[1] ) {
						status = TRUE;							// Found OKcr pattern
						pStream-> status |=  IOS_XBEE_STATUS_GOT_WHAT_LOOKING4;
					}
					else {
						pStream-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);	// look for Start Marker 'O' again.
					}
				}
				else {
					pStream-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);		// look for Start Marker 'O' again.
					io_stream_advance_readindex( pStream, pStream-> packetLen);	// skip passed the END marker of this bad packet.
					DEBUG_TRACE("\nFound END: WRONG len=%d", pStream-> packetLen);
				}
				break;
			}
	} // end switch()
	return status;
}// end rf_config_xbee_got_ok_cr()


/**
 * It looks for HH<cr> pattern in the given input stream.
 * Where HH is a byte value in HEX.
 *
 * @param	pStream -- pointer to iostream object.
 * @return	true if it got a valid byte data.
 * @post
 *
 * History:  Created on 2012-04-17 by Wai Fai Chin
 */


BYTE rf_config_xbee_get_byte( IO_STREAM_CLASS *pStream, BYTE *pbV )
{
	UINT16 wStatus;
	BYTE status;
	BYTE lookFor;

	status = FALSE;
	lookFor = pStream-> status & (IOS_XBEE_STATUS_LOOK4_MASK );
	switch( lookFor ) {
		case IOS_XBEE_STATUS_LOOK4_CR:
			wStatus = io_stream_scan_right_side( pStream, '\r', 'O' );
			if ( MSI_LIB_FOUND_START == (wStatus & MSI_LIB_FOUND_MATCH)   ) {	// if found a new 'O' before find the cr,
				pStream-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);
				pStream-> status |= IOS_XBEE_STATUS_LOOK4_CR;					// changed status to look for packet END marker.
				DEBUG_TRACE("%s", "\nFound Start");
				break;
			}
			if ( MSI_LIB_FOUND_MATCH == (wStatus & MSI_LIB_FOUND_MATCH)   ) {		// if found the cr marker.
				pStream-> packetLen = (BYTE) ( 0x00FF & wStatus);
				if ( pStream-> packetLen > 1 && pStream-> packetLen < 4) {
					DEBUG_TRACE("\nFound END: len=%d", pStream-> packetLen);
					io_stream_rewind_peekindex( pStream );							// rewind peek index back at the packet START marker.
					if ( rf_config_scan_byte_hex( pStream, pbV ) ) {				// if got a valid hex string.
						pStream-> status &= ~IO_STREAM_STATUS_LOOK4_MASK;
						pStream-> status |=  IOS_XBEE_STATUS_GOT_WHAT_LOOKING4;
						status = TRUE;
						DEBUG_TRACE("\nFound Value: %02X", ch);
					}
					else {
						pStream-> status &= ~IO_STREAM_STATUS_LOOK4_MASK;
						DEBUG_TRACE("\n Invalid value string: status= %d", (pStream-> status & IO_STREAM_STATUS_LOOK4_MASK));
						break;
					}
				}
				else {
					pStream-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);		// look for Start Marker 'O' again.
					DEBUG_TRACE("\nFound END: WRONG len=%d", pStream-> packetLen);
				}
				io_stream_advance_readindex( pStream, pStream-> packetLen);	// skip passed the END marker of this bad packet.
				break;
			}
	} // end switch()
	return status;

} // end rf_config_xbee_get_byte(,)


/**
 * It looks for HHHH<cr> pattern in the given input stream.
 * Where HHHH is a word value in HEX.
 *
 * @param	pStream -- pointer to iostream object.
 * @param	pwV		-- pointer to a word.
 *
 * @return	true if it got a valid byte data.
 * @post
 *
 * History:  Created on 2012-04-17 by Wai Fai Chin
 */


BYTE rf_config_xbee_get_word( IO_STREAM_CLASS *pStream, WORD16 *pwV )
{
	UINT16 wStatus;
	BYTE status;
	BYTE lookFor;

	status = FALSE;
	lookFor = pStream-> status & (IOS_XBEE_STATUS_LOOK4_MASK );
	switch( lookFor ) {
		case IOS_XBEE_STATUS_LOOK4_CR:
			wStatus = io_stream_scan_right_side( pStream, '\r', 'O' );
			if ( MSI_LIB_FOUND_START == (wStatus & MSI_LIB_FOUND_MATCH)   ) {	// if found a new 'O' before find the cr,
				pStream-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);
				pStream-> status |= IOS_XBEE_STATUS_LOOK4_CR;					// changed status to look for packet END marker.
				DEBUG_TRACE("%s", "\nFound Start");
				break;
			}
			if ( MSI_LIB_FOUND_MATCH == (wStatus & MSI_LIB_FOUND_MATCH)   ) {		// if found the cr marker.
				pStream-> packetLen = (BYTE) ( 0x00FF & wStatus);
				if ( pStream-> packetLen > 1 && pStream-> packetLen < 6) {
					DEBUG_TRACE("\nFound END: len=%d", pStream-> packetLen);
					io_stream_rewind_peekindex( pStream );								// rewind peek index back at the packet START marker.
					if ( rf_config_scan_word_hex( pStream, pwV ) ) {			// if got a valid hex string.
						pStream-> status &= ~IO_STREAM_STATUS_LOOK4_MASK;
						pStream-> status |=  IOS_XBEE_STATUS_GOT_WHAT_LOOKING4;
						status = TRUE;
						DEBUG_TRACE("\nFound Value: %02X", *wPtr);
					}
					else {
						pStream-> status &= ~IO_STREAM_STATUS_LOOK4_MASK;
						DEBUG_TRACE("\n Invalid value string: status= %d", (pStream-> status & IO_STREAM_STATUS_LOOK4_MASK));
						break;
					}
				}
				else {
					pStream-> status &= ~(IOS_XBEE_STATUS_LOOK4_MASK | IOS_XBEE_STATUS_GOT_WHAT_LOOKING4);		// look for Start Marker 'O' again.
					DEBUG_TRACE("\nFound END: WRONG len=%d", pStream-> packetLen);
				}
				io_stream_advance_readindex( pStream, pStream-> packetLen);	// skip passed the END marker of this bad packet.
				break;
			}
	} // end switch()
	return status;
} // end rf_config_xbee_get_word(,)


/**
 * It looks for a byte in hex format in the given input stream.
 *
 * @param	pStream -- pointer to iostream object.
 * @param	pbV	-- pointer to binary value of a converted hex characters.
 * @return TRUE if it found a valid hex value in binary word.
 *
 *
 * History:  Created on 2012-04-17 by Wai Fai Chin
 */

BYTE rf_config_scan_byte_hex( IO_STREAM_CLASS *pStream, BYTE *pbV )
{
	BYTE i;
	BYTE ch;
	BYTE status;

	status = TRUE;				// assumed found valid hex digits.
	*pbV = 0;
	for ( i=1; i < pStream->packetLen; i++ )	{
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
}// end rf_config_scan_byte_hex()


/**
 * It looks for a word in hex format in the given input stream.
 *
 * @param	pStream -- pointer to iostream object.
 * @param	pwV	-- pointer to binary value of a converted hex characters.
 * @return TRUE if it found a valid hex value in binary word.
 *
 *
 * History:  Created on 2012-04-17 by Wai Fai Chin
 */

BYTE rf_config_scan_word_hex( IO_STREAM_CLASS *pStream, WORD16 *pwV )
{
	BYTE i;
	BYTE ch;
	BYTE status;

	status = TRUE;				// assumed found valid hex digits.
	*pwV = 0;
	for ( i=1; i < pStream->packetLen; i++ )	{
		io_stream_peek_a_byte( pStream, &ch ); // ch = hex digit.
		ch = hex_char_to_nibble( ch );
		if ( 0xFF != ch  ) {					// if ch is a valid hex char
			*pwV <<= 4;
			*pwV |= ch;
		}
		else {
			status = FALSE;		// found no hex digits.
			break;
		}
	}

	return status;
}// end rf_config_scan_word_hex()
