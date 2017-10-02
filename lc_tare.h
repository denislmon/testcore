/*! \file lc_tare.h \brief loadcell tare related functions.*/
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
//  History:  Created on 2007/11/19 by Wai Fai Chin
// 
/// \ingroup loadcell
/// \defgroup lc_tare Loadcell tare module (lc_tare.c)
/// \code #include "lc_tare.h" \endcode
/// \par Overview
///
/// \note
///	\code
///		raw_weight   = scale_factor * filtered_Adc_count;
///		zero_weight  = user selected pure raw_weight;
///		gross_weight = raw_weight - zero_weight;
///		net_weight   = gross_weight - tare_weight;
///		gross_weight, net_weight and tare_weight are rounded based on countby.
///	\endcode
//
// ****************************************************************************
//@{
 

#ifndef MSI_LC_TARE_H
#define MSI_LC_TARE_H
#include  "config.h"
#include  "loadcell.h"

/// values need to be save even if power was removed.
extern float	gafTareWtFNV[ MAX_NUM_PV_LOADCELL];


/**
 * It change current loadcell to GROSS mode.
 *
 * @param  	pLc	-- pointer to loadcell data structure.
 *
 * @post   update opModes of this loadcell.
 *
 * History:  Created on 2007/11/19 by Wai Fai Chin
 */

#define lc_tare_change_to_gross(pLc)	*((pLc)-> pOpModes) &= ~LC_OP_MODE_NET_GROSS

void	lc_tare_check_pending( LOADCELL_T *pLc, BYTE lc );
BYTE	lc_tare_gross( LOADCELL_T *pLc, BYTE lc );
BYTE	lc_tare_set( LOADCELL_T *pLc, float fValue, BYTE lc );
void	lc_tare_toggle_net_gross( LOADCELL_T *pLc, BYTE lc );


#endif		// end MSI_LC_TARE_H
//@}

