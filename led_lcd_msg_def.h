/*! \file led_lcd_msg_def.h \brief LED and LCD message string definition.*/
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
//  History:  Created on 2009/07/07 by Wai Fai Chin
//  History:  Modified on 2011/12/15 by Denis Monteiro
// 
/// \ingroup product_module
/// \defgroup led_lcd_msg_def message string definition for LED and LCD (led_lcd_msg_def.c)
/// \code #include "led_lcd_msg_def.h" \endcode
/// \par Overview
//   It defines message string for LED and LCD.
//
// ****************************************************************************
//@{
 

#ifndef MSI_LED_LCD_MSG_DEF_H
#define MSI_LED_LCD_MSG_DEF_H
#include  "config.h"


extern const char gcStr_Error[]		PROGMEM;
extern const char gcStr_Load[]		PROGMEM;
// 2011-04-20 -WFC- v
extern const char gcStr_Load1[]		PROGMEM;
extern const char gcStr_Load2[]		PROGMEM;
extern const char gcStr_Load3[]		PROGMEM;
extern const char gcStr_Load4[]		PROGMEM;
// 2011-04-20 -WFC- ^
extern const char gcStr_UnLd[]		PROGMEM;
extern const char gcStr_buttn[]		PROGMEM;		// PHJ

extern const char gcStr_Func1[]		PROGMEM;		// PHJ
extern const char gcStr_Func2[]		PROGMEM;		// PHJ
extern const char gcStr_Func[]		PROGMEM;
extern const char gcStr_A_OFF[]		PROGMEM;
extern const char gcStr_Filtr[]		PROGMEM;
extern const char gcStr_LEdS[]		PROGMEM;
extern const char gcStr_SLEEP[]		PROGMEM;		// PHJ
extern const char gcStr_Unit[]		PROGMEM;
extern const char gcStr_StPt1[]		PROGMEM;
extern const char gcStr_StPt2[]		PROGMEM;
extern const char gcStr_StPt3[]		PROGMEM;
// 2015-09-09 -WFC- added setpoint 4 to 8
extern const char gcStr_StPt4[]		PROGMEM;
extern const char gcStr_StPt5[]		PROGMEM;
extern const char gcStr_StPt6[]		PROGMEM;
extern const char gcStr_StPt7[]		PROGMEM;
extern const char gcStr_StPt8[]		PROGMEM;
extern const char gcStr_totAL[]		PROGMEM;
extern const char gcStr_v_ttL[]		PROGMEM;		// PHJ
extern const char gcStr_HIrES[]		PROGMEM;		// PHJ

//extern const char gcStr_Port[]	PROGMEM;		// 2011-06-30 -WFC-
extern const char gcStr_Print[]		PROGMEM;		// 2011-06-29 -WFC-
extern const char gcStr_rF[]		PROGMEM;		// 2012-04-27 -WFC-
extern const char gcStr_On_OFF[]	PROGMEM;		// 2012-04-27 -WFC-
extern const char gcStr_ScId[]		PROGMEM;		// 2012-04-27 -WFC-
extern const char gcStr_ChnL[]		PROGMEM;		// 2012-04-27 -WFC-
extern const char gcStr_nEtId[]		PROGMEM;		// 2012-04-27 -WFC-
extern const char gcStr_StrEn[]		PROGMEM;		// 2013-04-02 -DLM-
extern const char gcStr_Hold[]		PROGMEM;		// 2016-04-01 -WFC-
extern const char gcStr_ZBEE[]		PROGMEM;		// 2016-04-01 -WFC-
extern const char gcStr_OthEr[]		PROGMEM;		// 2016-04-01 -WFC-
extern const char gcStr_tyPE[]		PROGMEM;		// 2016-04-01 -WFC-

extern const char gcStr_tESt[]		PROGMEM;
extern const char gcStr_OFF[]		PROGMEM;
extern const char gcStr_On[]		PROGMEM;
extern const char gcStr_LO[]		PROGMEM;
extern const char gcStr_HI_1[]		PROGMEM;
extern const char gcStr_HI_2[]		PROGMEM;
extern const char gcStr_Auto[]		PROGMEM;
extern const char gcStr_GrEAt[]		PROGMEM;
extern const char gcStr_LESS[]		PROGMEM;
extern const char gcStr_Phold[]		PROGMEM;
extern const char gcStr_nEtGr[]		PROGMEM;
extern const char gcStr_GROSS[]		PROGMEM;		// PHJ
extern const char gcStr_t_Cnt[]		PROGMEM;		// PHJ
extern const char gcStr_Learn[]		PROGMEM;		// PHJ
//extern const char gcStr_p2hr[]	PROGMEM;
extern const char gcStr_0[]			PROGMEM;		// 2013-04-02 -DLM-
extern const char gcStr_1[]			PROGMEM;		// 2013-04-02 -DLM-
extern const char gcStr_2[]			PROGMEM;		// 2013-04-02 -DLM-
extern const char gcStr_3[]			PROGMEM;		// 2013-04-02 -DLM-
extern const char gcStr_4[]			PROGMEM;		// 2013-04-02 -DLM-
extern const char gcStr_5[]			PROGMEM;		// PHJ
extern const char gcStr_15[]		PROGMEM;
extern const char gcStr_30[]		PROGMEM;
extern const char gcStr_45[]		PROGMEM;
extern const char gcStr_60[]		PROGMEM;

extern const char gcStr_CAL[]		PROGMEM;
extern const char gcStr_F_CAL[]		PROGMEM;
extern const char gcStr_CAL_r[]		PROGMEM;
extern const char gcStr_CAL_d[]		PROGMEM;
extern const char gcStr_r_CAL[]		PROGMEM;
// 2011-04-28 -WFC- extern const char gcStr_rCAL_Q[]	PROGMEM;
extern const char gcStr_StAnd[]		PROGMEM;
extern const char gcStr_Azt[]		PROGMEM;
extern const char gcStr_Zop[]		PROGMEM;		// 2016-03-23 -WFC- Zero on Power Up
extern const char gcStr_IndUS[]		PROGMEM;
extern const char gcStr_nISt[]		PROGMEM;
extern const char gcStr_EuroP[]		PROGMEM;
extern const char gcStr_1Unit[]		PROGMEM;

extern const char gcStr_C_CAL[]		PROGMEM;		// 2011-04-28 -WFC-

extern const char gcStr_C_SEt[]		PROGMEM;
extern const char gcStr_CAP[]		PROGMEM;
//extern const char gcStr_Unld[]		PROGMEM;
extern const char gcStr_CErr[]		PROGMEM;
extern const char gcStr_r_Err[]		PROGMEM;
extern const char gcStr_StorE[]		PROGMEM;
extern const char gcStr_d[]			PROGMEM;
extern const char gcStr_LFCnt[]		PROGMEM;
extern const char gcStr_OLCnt[]		PROGMEM;

extern const char gcStr_A_LoAd[]	PROGMEM;
extern const char gcStr_A_nor[]		PROGMEM;
extern const char gcStr_A_PEA[]		PROGMEM;
extern const char gcStr_A_droP[]	PROGMEM;
extern const char gcStr_A_AcP[]		PROGMEM;
extern const char gcStr_PrESS[]		PROGMEM;
extern const char gcStr_O[]			PROGMEM;
extern const char gcStr_No[]		PROGMEM;
extern const char gcStr_PASS[]		PROGMEM;
extern const char gcStr_FAIL[]		PROGMEM;
extern const char gcStr_Cancl[]		PROGMEM;
extern const char gcStr_Good[]		PROGMEM;
extern const char gcStr_Littl[]		PROGMEM;
extern const char gcStr_T_Off[]		PROGMEM;		//	PHJ

extern const char gcStr_rESEt[]		PROGMEM;
extern const char gcStr_SurE_Q[]	PROGMEM;
extern const char gcStr_SEtuP[]		PROGMEM;
extern const char gcStr_unCal[]		PROGMEM;

extern const char gcStr_Cn_2[]		PROGMEM;
extern const char gcStr_Ut[]		PROGMEM;
extern const char gcStr_Ot[]		PROGMEM;
extern const char gcStr_H_nd[]		PROGMEM;
extern const char gcStr_OLoad[]		PROGMEM;
extern const char gcStr_LcErr[]		PROGMEM;
extern const char gcStr_Eq_Eprs[]	PROGMEM;

extern const char gcStr_bLoad[] 	PROGMEM;

extern const char gcStr_batt[] 		PROGMEM;

extern const char gcStr_Lc_OFF[]	PROGMEM;			// 2011-08-24 -WFC- loadcell disabled.

extern const char gcStr_SoFt[]		PROGMEM;			//2012-09-20 -WFC- Soft version

extern const char gcStr_bLiFE[]		PROGMEM;			//2014-09-30 -WFC-
extern const char gcStr_Long[]		PROGMEM;			//2014-09-30 -WFC-

extern const char gcStr_LFcnt[]		PROGMEM;			//2014-10-22 -WFC-



#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 )
extern const char gcStr_unCal[]		PROGMEM;
//2012-09-20 -WFC- extern const char gcStr_CH3_Sn[]	PROGMEM;
extern const char gcStr_diSabL[]	PROGMEM;
extern const char gcStr_UndrLd[]	PROGMEM;
extern const char gcStr_d_tESt[]	PROGMEM;
extern const char gcStr_LO_1[]		PROGMEM;
extern const char gcStr_LO_2[]		PROGMEM;
extern const char gcStr_tArE[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_StrnG[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_Cntrl[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_USEr[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_Cont[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_5Unit[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_2Unit[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_rAtE[]		PROGMEM;			// 2011-07-14 -WFC-
extern const char gcStr_No_rF[]		PROGMEM;			// 2011-10-21 -WFC-

#endif

extern const char gcStr_Listnr[]	PROGMEM;			// 2014-07-15 -WFC- Listener ID.
extern const char gcStr_Output[]	PROGMEM;			// 2014-07-15 -WFC- Output
extern const char gcStr_Port_0[]	PROGMEM;			// 2014-07-15 -WFC- Output
extern const char gcStr_Port_2[]	PROGMEM;			// 2014-07-15 -WFC- Output


#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )		// 2011-06-10, 2011-09-27 -WFC-
extern const char gcStr_unCal[]		PROGMEM;
//2012-09-20 -WFC-  extern const char gcStr_DL_2[]		PROGMEM;
extern const char gcStr_diSabL[]	PROGMEM;
extern const char gcStr_UndrLd[]	PROGMEM;
extern const char gcStr_d_tESt[]	PROGMEM;
extern const char gcStr_tArE[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_StrnG[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_Cntrl[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_USEr[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_Cont[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_5Unit[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_2Unit[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_rAtE[]		PROGMEM;			// 2011-07-14 -WFC-
extern const char gcStr_No_rF[]		PROGMEM;			// 2011-10-21 -WFC-
#endif

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-
#if (  CONFIG_PRODUCT_AS != CONFIG_AS_REMOTE_METER )
extern const char gcStr_unCal[]		PROGMEM;
extern const char gcStr_diSabL[]	PROGMEM;
extern const char gcStr_UndrLd[]	PROGMEM;
#endif

extern const char gcStr_AdCbsy[]	PROGMEM;

extern const char gcStr_no_dSc[]	PROGMEM;
extern const char gcStr_HLI_Sn[]	PROGMEM;
extern const char gcStr_Hd_Sn[]		PROGMEM;
extern const char gcStr_dSC_Sn[]	PROGMEM;

extern const char gcStr_r_CAL1[]	PROGMEM;
extern const char gcStr_r_CAL2[]	PROGMEM;

extern const char gcStr_d_tst1[]	PROGMEM;
extern const char gcStr_d_tst2[]	PROGMEM;
extern const char gcStr_d_tst3[]	PROGMEM;
extern const char gcStr_d_tst4[]	PROGMEM;

extern const char gcStr_A2d_dc[]	PROGMEM;
extern const char gcStr_A2d_Ac[]	PROGMEM;
extern const char gcStr_A2d_Or[]	PROGMEM;
extern const char gcStr_A2d_Ur[]	PROGMEM;

extern const char gcStr_P_inLo[]	PROGMEM;


// 2011-09-26 -WFC- v
extern const char gcStr_LO_1[]		PROGMEM;
extern const char gcStr_LO_2[]		PROGMEM;
extern const char gcStr_1hour[]		PROGMEM;
extern const char gcStr_rCAL_Q[]	PROGMEM;
// 2011-09-26 -WFC- ^

#endif

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-
extern const char gcStr_unCal[]		PROGMEM;
extern const char gcStr_PW2_Sn[]	PROGMEM;
extern const char gcStr_diSabL[]	PROGMEM;
extern const char gcStr_UndrLd[]	PROGMEM;
extern const char gcStr_d_tESt[]	PROGMEM;
extern const char gcStr_LO_1[]		PROGMEM;
extern const char gcStr_LO_2[]		PROGMEM;
extern const char gcStr_tArE[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_StrnG[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_Cntrl[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_USEr[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_Cont[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_5Unit[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_2Unit[]		PROGMEM;			// 2011-06-29 -WFC-
extern const char gcStr_rAtE[]		PROGMEM;			// 2011-07-14 -WFC-
extern const char gcStr_No_rF[]		PROGMEM;			// 2011-10-21 -WFC-
extern const char gcStr_Ethnt[]		PROGMEM;			// 2012-07-06 -DLM-
#endif


//extern const char gcStr_A2d_1[]	PROGMEM;
//extern const char gcStr_A2d_2[]	PROGMEM;

//extern const char gcStr_000000[]	PROGMEM;
//extern const char gcStr_111111[]	PROGMEM;
//extern const char gcStr_222222[]	PROGMEM;
//extern const char gcStr_333333[]	PROGMEM;
//extern const char gcStr_444444[]	PROGMEM;
//extern const char gcStr_555555[]	PROGMEM;
//extern const char gcStr_666666[]	PROGMEM;
//extern const char gcStr_777777[]	PROGMEM;
//extern const char gcStr_888888[]	PROGMEM;
//extern const char gcStr_999999[]	PROGMEM;


#endif  // MSI_LED_LCD_MSG_DEF_H

//@}
