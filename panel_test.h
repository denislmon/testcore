/*! \file panel_test.h \brief Panel setup definition.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2010, 2011 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATmega Family
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Application
//
//  History:  Created on 2010-08-06 by Wai Fai Chin
//  History:  Modified on 2011-12-15 by Denis Monteiro
// 
/// \ingroup panel_test
/// \defgroup panel_test  Panel test. (panel_test.c)
/// \code #include "panel_test.h" \endcode
/// \par Overview
//   Test events show on display panel.
//
// ****************************************************************************
// History:
// 2011-04-06 -WFC- written for Challenger3 requirement.
//
//@{
 

#ifndef MSI_PANEL_TEST_H
#define MSI_PANEL_TEST_H

#include	"config.h"
#include	"led_display.h"
#include	"scalecore_sys.h"
#include	"panel_setup.h"

#if (CONFIG_PRODUCT_AS == CONFIG_AS_HLI )
	#define PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS			8
#elif (CONFIG_PRODUCT_AS == CONFIG_AS_HD )
	#define PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS			9
#elif (CONFIG_PRODUCT_AS == CONFIG_AS_CHALLENGER3 )
	#define PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS			4
	#define PANEL_TEST_TOP_MENU_SERVICE_INFO_MAX_ITEMS		3
#elif (CONFIG_PRODUCT_AS == CONFIG_AS_DYNA_LINK_II )
	#define PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS			4
	#define PANEL_TEST_TOP_MENU_SERVICE_INFO_MAX_ITEMS		3
#elif ( CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER  )		// 2011-09-27 -WFC-
	#define PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS			4
	#define PANEL_TEST_TOP_MENU_SERVICE_INFO_MAX_ITEMS		3
#elif (CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )			// 2011-12-15 -DLM-
	#define PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS			4
	#define PANEL_TEST_TOP_MENU_SERVICE_INFO_MAX_ITEMS		3
#else
	#define PANEL_TEST_TOP_MENU_SCONDARY_MAX_ITEMS			4
#endif

extern	PANEL_TOP_MENU_T    gacPanel_Top_Menu_Secondary_Test[] PROGMEM;
extern	PANEL_TOP_MENU_T    gacPanel_Top_Menu_Service_Info_Test[] PROGMEM;

char panel_test_top_menu_secondary_test_thread( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );
void panel_test_background_tasks( PANEL_SETUP_TOP_MENU_CLASS *pSetupObj );

#endif  // MSI_PANEL_TEST_H

//@}
