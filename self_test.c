/*! \file self_test.c \brief self test of a product related functions.*/
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
//  History:  Created on 2010/04/07 by Wai Fai Chin
//  History:  Modified on 2011/12/14 by Denis Monteiro
//
//   It is a high level module test all require ios, functions and features.
// This is product specific module. Special custom software should modify this file and module.
//
// ****************************************************************************
// 2011-06-17 -WFC- Divided this file for auto inclusion of correct file based on product configuration.

///
#include	"bios.h"
#include	"nvmem.h"
#include	"self_test.h"
#include	"dataoutputs.h"
#include	"sensor.h"
#include	"adc_lt.h"
#include	"scalecore_sys.h"
#include	"msi_packet_router.h"
#include	"stream_router.h"
#include	<stdio.h>
#include	"commonlib.h"
#include	"cmdaction.h"		// for gbCmdSysRunMode

#if ( CONFIG_PRODUCT_AS	!= CONFIG_AS_IDSC )				// 2011-12-02 -WFC-
#include	"panelmain.h"
#endif

#include	"voltmon.h"

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_DYNA_LINK_II )		// 2011-06-17 -WFC-
#include	"self_test_challenger3_dynalink2.c"
#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_DSC || CONFIG_PRODUCT_AS == CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI )		// 2011-06-17 -WFC-
#include	"self_test_dsc_hli_hd.c"
#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_IDSC )				// 2011-09-23 -WFC-
#include	"self_test_idsc.c"
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-09-26 -WFC-
#include	"self_test_rm.c"
#elif ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )		// 2011-12-14 -DLM-
#include	"self_test_challenger3_dynalink2.c"
#endif

