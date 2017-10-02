/*! \file panel_test.c \brief  Panel test menu definition.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2010, 2011 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL Atmega Family
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2010-08-06 by Wai Fai Chin
//  History:  Modified on 2011-12-13 by Denis Monteiro
// 
//   Test events show on display panel.
//
// ****************************************************************************
// History:
// 2011-04-06 -WFC- written for Challenger3 requirement.
// 2011-06-10 -WFC- Divided this file for auto inclusion of correct file based on product configuration.
 
#include	"config.h"

#if (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 )
#include "panel_test_challenger3.c"
#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
#include "panel_test_dynalink2.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-09-26 -WFC-
#include "panel_test_rm.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI ) 	// 2011-12-13 -DLM-
#include "panel_test_hli_hd.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )	// 2011-12-13 -DLM-
#include "panel_test_portaweigh2.c"
#endif
