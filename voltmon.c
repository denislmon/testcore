/*! \file voltmon.c \brief voltmon related functions.*/
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
//  History:  Created on 2008/12/08 by Wai Fai Chin
// 
//   This is a high level module to convert filtered ADC count into a voltage data
// base on the calibration table. It does not know how the sensor got the ADC data.
// It just know how to compute the voltage. It formats output voltage data string to
// a caller supplied string buffer but it does not know how to output the data.
// 
//
//            Application          Abstract Object        Hardware driver
//         +---------------+      +---------------+     +----------------+
//         |VOLTAGE MONITOR|      |               |     |                |
//    -----|     MODULE    |<-----| SENSOR MODULE |<----| CPU ADC MODULE |
//         | ADC to VOLTAGE|      |      ADC      |     |                |
//         |               |      |               |     |                |
//         +---------------+      +---------------+     +----------------+
//
//	
//
//
// NOTE:
// NV suffix stands for nonvolatile memory. It is in RAM and recalled from nonvolatile memory during powerup.
// FNV suffix stands for ferri nonvolatile memory. It is in RAM and frequently write to ferri memory everytime the value has changed.
// ****************************************************************************

#include  "config.h"  //2011-12-06 -WFC-

#if (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 )
#include "voltmon_challenger3.c"
#elif (CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2013-09-03 -DLM-
#include "voltmon_portaweigh2.c"
#endif

