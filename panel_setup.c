/*! \file panel_setup.c \brief  Panel setup definition.*/
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
//  History:  Created on 2009/07/08 by Wai Fai Chin
//  History:  Modified on 2011/12/13 by Denis Monteiro
// 
//   Panel Menu setup
//
// ****************************************************************************
// 2011-04-11 -WFC- added thread pointer item to all PANEL_TOP_MENU_T items.
// 2011-06-10 -WFC- Divided this file for auto inclusion of correct file based on product configuration.

#include	"config.h"

#if (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 )
#include "panel_setup_challenger3.c"
#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II )
#include "panel_setup_dynalink2.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-06-17 -WFC-
#include "panel_setup_hli.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_HD  )		// 2011-09-22 -WFC-
#include "panel_setup_hd.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-09-26 -WFC-
#include "panel_setup_rm.c"
#elif (CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )	// 2011-12-13 -DLM-
#include "panel_setup_portaweigh2.c"
#endif
