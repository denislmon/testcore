/*! \file dataOutputs.h \brief data output manager related functions.*/
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
//  History:  Created on 2007/08/06 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup dataOutputs Dataoutput manages data output to all IO_STREAM(dataOutputs.c)
/// \code #include "dataoutputs.h" \endcode
/// \par Overview
///     This module manages output behavior of each listener. A listener associated 
///  with a device ID of a registered listener device, and output stream type of the assigned sensor.
///  The stream router will manage to route it to one of following stream drivers: Uart, SPI, RF modem, USB, or ethernet etc...
//
// ****************************************************************************
//@{


#ifndef MSI_DATAOUTPUTS_H
#define MSI_DATAOUTPUTS_H

#include  "timer.h"
#include "stream_router.h"

/// Maximum number of stream port.
//#define		MAX_NUM_IOSTREAM_OBJECTS		1
#define		MAX_NUM_STREAM_LISTENER				3

#define		LISTENER_MODE_DISABLED					0
#define		LISTENER_MODE_COMMAND					1
#define		LISTENER_MODE_INTERVAL					2
#define		LISTENER_MODE_ADC_TEST					3
#define		LISTENER_MODE_ADC_TEST_CMD_PACKET		4
#define		LISTENER_MODE_ADC_TEST_INTERVAL_PACKET	5
#define		LISTENER_MODE_ADC_TEST_FILTERED_INPUT	6
#define		LISTENER_MODE_CUSTOMER_SPECIAL			7
#define		LISTENER_MODE_ADC_TEST_UNFILTERED_INPUT	8
#define		LISTENER_MODE_USER_PRINT_STRINGS		9

// maxium number of listener mode
#define		MAX_LISTENER_MODE			9

extern	TIMER_T gatListenerTimer[ MAX_NUM_STREAM_LISTENER ];		// interval timer of Listener output stream object

																/// sensor channel number of the assigned IO_STREAM object
extern BYTE	gabListenerWantSensorFNV[ MAX_NUM_STREAM_LISTENER ];	//  sensor channel number of the assigned IO_STREAM object
																/// 0 == disabled,	1==command mode, 2==interval, 9==user defined print strings.
extern BYTE	gabListenerModesFNV[ MAX_NUM_STREAM_LISTENER ];	//  0 == disabled,	1==command mode, 2==interval
																/// 0 == continuous, 1 tick = 50 mSec or depends on task timer tick.
extern BYTE	gabListenerIntervalFNV[ MAX_NUM_STREAM_LISTENER ];	//  0 == continuous, 1 tick = 50 mSec or depends on task timer tick.
																/// Device ID of registered listener
extern BYTE	gabListenerDevIdFNV[ MAX_NUM_STREAM_LISTENER ];	//  Device ID of registered listener
															/// 0 == uart, 1 == uart1, 2 == uart2,  3 == rf modem stream type, 4 == spi.
extern BYTE	gabListenerStreamTypeFNV[ MAX_NUM_STREAM_LISTENER ];	//  0 == uart, 1 == uart1, 2 == uart2,  3 == rf modem stream type, 4 == spi.

/// number of lit segment of the system bar graph.
extern BYTE	gbBargraphNumLitSeg;

/// bar graph resolution defined as inverse of value per segment, so to speed up computation by use multiplication instead of division.
extern float gfBargraphResolution;


void	data_out_to_all_listeners( void);
void	data_out_init( void);
void	data_out_1st_init( void);
BYTE 	data_out_manage_format_test_data_packet( BYTE listenerNum, char *pOutStr );

void	data_out_compute_bargraph_resolution( void );
void	data_out_compute_number_bargraph_segment( void );

#if ( CONFIG_PRODUCT_AS == CONFIG_AS_DSC )
void	data_out_time_critical_data( void );
#endif

#endif	// MSI_DATAOUTPUTS_H
//@}
