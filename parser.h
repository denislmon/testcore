/*! \file parser.h \brief parser related functions definition.*/
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
// Compiler: any C
// Software layer:  Application
//
//  History:  Created on 2007/07/10 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup paser_number Paser number related functions (parser.c)
/// \code #include "parser.h" \endcode
/// \par Overview
///   Parser related functions.
//
// ****************************************************************************
//@{


#ifndef MSI_PARSER_H
#define MSI_PARSER_H

#include "config.h"

char is_a_number( const char *pbStr );
BYTE is_psw_string_has_qualify_chars( const char *pbStr );


    #ifdef  CONFIG_TEST_PARSER_MODULE
        BYTE test_parser_module( void);
    #endif

#endif
//@}
