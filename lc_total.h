/*! \file lc_zero.h \brief loadcell totaling related functions.*/
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
//  History:  Created on 2007/12/21 by Wai Fai Chin
// 
/// \ingroup loadcell
/// \defgroup lc_total Loadcell total module (lc_total.c)
/// \code #include "lc_total.h" \endcode
/// \par Overview
///	\code
///
///  NOTE:
///		raw_weight   = scale_factor * filtered_Adc_count;
///		zero_weight  = user selected pure raw_weight;
///		gross_weight = raw_weight - zero_weight;
///		net_weight   = gross_weight - tare_weight;
///		gross_wieght, net_weight and tare_weight are rounded based on countby.
///	\endcode
// ****************************************************************************
//@{
 

#ifndef MSI_LC_TOTAL_H
#define MSI_LC_TOTAL_H
#include  "config.h"

#include  "loadcell.h"


extern	BYTE	gabTotalDropThresholdPctCapNV[ MAX_NUM_PV_LOADCELL ];
extern	BYTE	gabTotalRiseThresholdPctCapNV[ MAX_NUM_PV_LOADCELL ];
extern	float	gafTotalOnAcceptUpperWtNV[ MAX_NUM_PV_LOADCELL];
extern	float	gafTotalOnAcceptLowerWtNV[ MAX_NUM_PV_LOADCELL];
extern	BYTE	gabTotalMinStableTimeNV[ MAX_NUM_PV_LOADCELL];

/// values need to be save even if power was removed.
extern	float	gafTotalWtFNV[ MAX_NUM_PV_LOADCELL];
extern	UINT16	gawNumTotalFNV[ MAX_NUM_PV_LOADCELL];
extern	float	gafSumSqTotalWtFNV[ MAX_NUM_PV_LOADCELL];
extern	float	gafMaxTotalWtFNV[ MAX_NUM_PV_LOADCELL];
extern	float	gafMinTotalWtFNV[ MAX_NUM_PV_LOADCELL];
extern	BYTE	gabTotalModeFNV[ MAX_NUM_PV_LOADCELL];
extern	BYTE	gabTotalMinStableTimeFNV[ MAX_NUM_PV_LOADCELL];


#define  LC_TOTAL_MODE_DISABLED			0
#define  LC_TOTAL_MODE_AUTO_LOAD		1
#define  LC_TOTAL_MODE_AUTO_NORMAL		2
#define  LC_TOTAL_MODE_AUTO_PEAK		3
#define  LC_TOTAL_MODE_LOAD_DROP		4
#define  LC_TOTAL_MODE_ON_ACCEPT		5
#define  LC_TOTAL_MODE_ON_COMMAND		6

#define  LC_TOTAL_STATUS_NOT_ALLOW				0x80	// NOTE!!!, you can added qualified weight to total once only until the load has dropped.
#define  LC_TOTAL_STATUS_START_LOAD_DROP		0x40	// User had press total key or command to start a load drop total cycle.
														/// use total key as on/off switch for auto total modes. 1==disalbed, 0==enabled.
#define  LC_TOTAL_STATUS_DISABLED_AUTO_MODES	0x20	// User had press total key or command in auto total mode acts as toggle disabled or enabled.
#define  LC_TOTAL_STATUS_SKIP_TOTAL				0x04	// Skip total because of zero or tare command generated a 0 weight which could triger a total.
#define  LC_TOTAL_STATUS_NEW_BLINK_EVENT		0x02	// Tell meter to blink total annunciator
#define  LC_TOTAL_STATUS_NOT_ALLOW_BLINK_LAMP	0x01	// Tell total function not to set new blink event flag.

/*
1LB = 0.5Kg
4LB = 2Kg

16LB^2 = 4Kg^2

1LB^2 = .25Kg^2

3LB   = 1.5Kg
9LB^2 = 2.25Kg^2

*/

void	lc_total_init( BYTE lc );
void	lc_total_evaluate( LOADCELL_T *pLc, BYTE lc );
void	lc_total_handle_command( BYTE lc );
void	lc_total_update_param( BYTE lc );
void	lc_total_clear_total( LOADCELL_T *pLc, BYTE lc );
void	lc_total_remove_last_total( LOADCELL_T *pLc, BYTE lc );

#endif		// end MSI_LC_TOTAL_H
//@}

