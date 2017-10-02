/*! \file hw_inputs_handlers.h \brief process inputs and respond to them product related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2010 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: independent
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Middle layer
//
//  History:  Created on 2010/06/28 by Wai Fai Chin
//
/// \ingroup product_module
/// \defgroup hw_inputs_handlers It processes inputs and respond to them accordingly based programmable input functions and product type. (hw_inputs_handlers.c)
/// \code #include "hw_inputs_handlers.h" \endcode
/// \par Overview
/// It processes inputs and respond to them accordingly based programmable input functions and product type.
//
// ****************************************************************************
//@{

#ifndef MSI_HW_INPUTS_HANDLERS_H_
#define MSI_HW_INPUTS_HANDLERS_H_

#include  "bios.h"

void hw_inputs_handlers( void );


/** \def hw_inputs_handlers_clear_inputs_state
 * It clears hardware inputs state.
 *
 * History:  Created on 2010/06/28 by Wai Fai Chin
 */
#define  hw_inputs_handlers_clear_inputs_state()


/** \def hw_inputs_handlers_scan_inputs_tasks
 * It clears hardware inputs state.
 *
 * History:  Created on 2010/06/30 by Wai Fai Chin
 */
#define  hw_inputs_handlers_scan_inputs_tasks()   bios_scan_inputs_tasks()

#endif /* MSI_HW_INPUTS_HANDLERS_H_ */
//@}
