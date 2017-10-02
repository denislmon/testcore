/*! \file hw_outputs_handlers.h \brief High Level behavior of output pins related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2010 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATXMEGA128A1
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2010/06/30 by Wai Fai Chin
// 
/// \ingroup product_module
/// \defgroup hw_outputs_handlers manage high level behavior of output pins.(hw_outputs_handlers.c)
/// \code #include "hw_outputs_handlers.h" \endcode
/// \par Overview
//   This is a high level output behavior manager module.
//   It manages the behavior of output pins.
//
// ****************************************************************************
//@{
 

#ifndef MSI_HW_OUTPUTS_HANDLERS_H
#define MSI_HW_OUTPUTS_HANDLERS_H

#include  "config.h"
#include  "timer.h"
#include  "bios.h"

/* !
  \brief Output pin behavior descriptor. It controls how the output pins output by
   hw_outputs_handlers().
* /

typedef struct OUTPUTPIN_BEHAVIOR_DESCRIPTOR_TAG {
									///
  BYTE		mode;
									/// blink timer
  TIMER_T	gBlinkTimer;
}OUTPUTPIN_BEHAVIOR_DESCRIPTOR_T;
*/

void hw_outputs_handlers_init( void );
void hw_outputs_handlers( void );

#endif  // MSI_HW_OUTPUTS_HANDLERS_H

//@}
