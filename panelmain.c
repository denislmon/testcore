/*! \file panelmain.c \brief product specific panel device related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATMEGA1281
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2009/04/29 by Wai Fai Chin
//  History:  Modified on 2011/12/13 by Denis Monteiro
// 
//   This is a product specific panel device main module. It manages all aspect
//  of the product functionality such as panel user interface
//  between LED display and the keypad, etc..
//
// ****************************************************************************
// 2011-06-10 -WFC- Divided this file for auto inclusion of correct file based on product configuration.

#include	"config.h"

#if (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 )
#include "panelmain_challenger3.c"
#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
#include "panelmain_dynalink2.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-06-17 -WFC-
#include "panelmain_hli.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_HD  )		// 2011-09-22 -WFC-
#include "panelmain_hd.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-09-26 -WFC-
#include "panelmain_rm.c"
#elif (CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )	// 2011-12-13 -DLM-
#include "panelmain_portaweigh2.c"
#endif

