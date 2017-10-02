/*! \file productdef.h \brief product definition. */
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2010 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: productdef.h
// Hardware: ATMEL AVR series
// OS:       independent
// Compiler: avr-gcc
// Software layer:  project wide
//
//  History:  Created on 2010-11-18 by Wai Fai Chin
//  History:  Modified for 4260B on 2011-12-12 by Denis Monteiro
//
/// \code #include "productdef.h" \endcode
/// \par Overview
///     Definitions of product codes.
//
// ****************************************************************************
// 2011-09-06 -WFC- Created a new defined CONFIG_AS_REMOTE_METER to 7.
//@{

#ifndef MSI_PRODUCTDEF_H
#define MSI_PRODUCTDEF_H

// product definition
#define CONFIG_AS_GENERIC_SCALECORE		0
#define CONFIG_AS_CHALLENGER3			1
#define CONFIG_AS_DYNA_LINK_II			2
#define CONFIG_AS_DSC					3
#define CONFIG_AS_HD					4
#define CONFIG_AS_HLI					5
#define CONFIG_AS_IDSC					6
#define CONFIG_AS_REMOTE_METER			7			// 2011-09-06 -WFC-
#define CONFIG_AS_PORTAWEIGH2			8			// 2011-12-12 -DLM-

#define LCD_7300						0			// 2013-03-26 -DLM-
#define LCD_8000						1			// 2013-03-26 -DLM-

// Target/sub-product definition
#define CONFIG_AS_PORTAWEIGH2_BB		0
#define CONFIG_AS_PORTAWEIGH2_BC		1
#define CONFIG_AS_PORTAWEIGH2_NB		2
#define CONFIG_AS_PORTAWEIGH2_NC		3
#define CONFIG_AS_PORTAWEIGH2_RB		4
#define CONFIG_AS_PORTAWEIGH2_RC		5
#define CONFIG_AS_PORTAWEIGH2_PC		6
#define CONFIG_AS_PORTAWEIGH2_IS		7			// 2017-09-12 -DLM-


#endif
//@}
