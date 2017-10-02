/*! \file led_lcd_msg_def.c \brief LED and LCD message string definition.*/
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
//   It defines message string for LED and LCD.
//
// ****************************************************************************
 

#include	"led_lcd_msg_def.h"

#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )
const char gcStr_Error[]	PROGMEM = "Error";
const char gcStr_Load[]		PROGMEM = " Load";
// 2011-04-20 -WFC- v
const char gcStr_Load1[]	PROGMEM = "Load1";
const char gcStr_Load2[]	PROGMEM = "Load2";
const char gcStr_Load3[]	PROGMEM = "Load3";
const char gcStr_Load4[]	PROGMEM = "Load4";
// 2011-04-20 -WFC- ^
const char gcStr_UnLd[]		PROGMEM = " UnLd";
const char gcStr_buttn[]	PROGMEM = "buttn";		// PHJ
#endif

const char gcStr_Func1[]	PROGMEM = "Func1";		// PHJ
const char gcStr_Func2[]	PROGMEM = "Func2";		// PHJ
const char gcStr_Func[]		PROGMEM = "Func";
const char gcStr_A_OFF[]	PROGMEM = "A-OFF";
const char gcStr_Filtr[]	PROGMEM = "Filtr";
const char gcStr_LEdS[]		PROGMEM = "DispL";		// PHJ mod from LEds
const char gcStr_SLEEP[]	PROGMEM = "SLEEP";		// PHJ
const char gcStr_Unit[]		PROGMEM = "Unit";
const char gcStr_StPt1[]	PROGMEM = "StPt1";
const char gcStr_StPt2[]	PROGMEM = "StPt2";
const char gcStr_StPt3[]	PROGMEM = "StPt3";
const char gcStr_StPt4[]	PROGMEM = "StPt4";		// 2015-09-09 -WFC- added setpoint 4 to 8.
const char gcStr_StPt5[]	PROGMEM = "StPt5";
const char gcStr_StPt6[]	PROGMEM = "StPt6";
const char gcStr_StPt7[]	PROGMEM = "StPt7";
const char gcStr_StPt8[]	PROGMEM = "StPt8";
const char gcStr_totAL[]	PROGMEM = "totAl";
const char gcStr_v_ttL[]	PROGMEM = "v-ttl";		// PHJ
const char gcStr_HIrES[]	PROGMEM = "HirES";		// PHJ

//const char gcStr_Port[]		PROGMEM = "Port";	// 2011-06-30 -WFC-
const char gcStr_Print[]	PROGMEM = "Print";		// 2011-06-29 -WFC-
const char gcStr_rF[]		PROGMEM = "rF";			// 2012-04-27 -WFC-
const char gcStr_On_OFF[]	PROGMEM = "On.OFF";		// 2012-04-27 -WFC-
const char gcStr_ScId[]		PROGMEM = "ScId";		// 2012-04-27 -WFC-
const char gcStr_ChnL[]		PROGMEM = "ChnL";		// 2012-04-27 -WFC-
const char gcStr_nEtId[]	PROGMEM = "nEtId";		// 2012-04-27 -WFC-
const char gcStr_StrEn[]	PROGMEM = "StrEn";		// 2013-04-02 -DLM-
const char gcStr_Hold[]		PROGMEM = "Hold";		// 2016-04-01 -WFC-
const char gcStr_ZBEE[]		PROGMEM = "2BEE";		// 2016-04-01 -WFC-
const char gcStr_OthEr[]	PROGMEM = "OthEr";		// 2016-04-01 -WFC-
const char gcStr_tyPE[]		PROGMEM = "tyPE";		// 2016-04-01 -WFC-

const char gcStr_tESt[]		PROGMEM = "tESt";
const char gcStr_OFF[]		PROGMEM = "OFF";
const char gcStr_On[]		PROGMEM = "On";
const char gcStr_LO[]		PROGMEM = "LO";
const char gcStr_HI_1[]		PROGMEM = "HI-1";
const char gcStr_HI_2[]		PROGMEM = "HI-2";
const char gcStr_Auto[]		PROGMEM = "Auto";
const char gcStr_GrEAt[]	PROGMEM = "GrEAt";
const char gcStr_LESS[]		PROGMEM = "LESS";
// 2011-05-03 -WFC- const char gcStr_Phold[]	PROGMEM = "P-hld";		// PHJ mod from PHold
const char gcStr_Phold[]	PROGMEM = "P-HLd";		// 2011-05-03 -WFC-
const char gcStr_nEtGr[]	PROGMEM = "nEtGr";
const char gcStr_GROSS[]	PROGMEM = "GroSS";		// PHJ
const char gcStr_t_Cnt[]	PROGMEM = "t-cnt";		// PHJ
const char gcStr_Learn[]	PROGMEM = "LEArn";		// PHJ
//const char gcStr_p2hr[]		PROGMEM = ".2hr";
const char gcStr_0[]		PROGMEM = "0";			// 2013-04-02 -DLM-
const char gcStr_1[]		PROGMEM = "1";			// 2013-04-02 -DLM-
const char gcStr_2[]		PROGMEM = "2";			// 2013-04-02 -DLM-
const char gcStr_3[]		PROGMEM = "3";			// 2013-04-02 -DLM-
const char gcStr_4[]		PROGMEM = "4";			// 2013-04-02 -DLM-
const char gcStr_5[]		PROGMEM = "5";			// PHJ
const char gcStr_15[]		PROGMEM = "15";
const char gcStr_30[]		PROGMEM = "30";
const char gcStr_45[]		PROGMEM = "45";
const char gcStr_60[]		PROGMEM = "60";

const char gcStr_CAL[]		PROGMEM = "CAL";
const char gcStr_F_CAL[]	PROGMEM = "F-CAL";
const char gcStr_CAL_r[]	PROGMEM = "CAL-r";
const char gcStr_CAL_d[]	PROGMEM = "CAL'd";
const char gcStr_r_CAL[]	PROGMEM = "r-CAL";
// 2011-04-28 -WFC- const char gcStr_rCAL_Q[]	PROGMEM = "rCAL.?";
const char gcStr_StAnd[]	PROGMEM = "StAnd";
const char gcStr_Azt[]		PROGMEM = "Auto0";		// PHJ mod from Azt
const char gcStr_Zop[]		PROGMEM = "0.P-UP";		// 2016-03-23 -WFC- Zero on Power Up
const char gcStr_IndUS[]	PROGMEM = "IndUS";
const char gcStr_nISt[]		PROGMEM = "HB-44";		// PHJ mod from nISt
const char gcStr_EuroP[]	PROGMEM = " r-76";		// PHJ mod from EuroP
const char gcStr_1Unit[]	PROGMEM = "1Unit";

const char gcStr_C_CAL[]	PROGMEM = "C-CAL";		// 2011-04-28 -WFC-

const char gcStr_C_SEt[]	PROGMEM = "C-SEt";
const char gcStr_CAP[]		PROGMEM = "CAP";
//const char gcStr_Unld[]		PROGMEM = "UnLd";
const char gcStr_CErr[]		PROGMEM = "CErr";
const char gcStr_r_Err[]	PROGMEM = "r-Err";
const char gcStr_StorE[]	PROGMEM = "StorE";
const char gcStr_d[]		PROGMEM = "d";
const char gcStr_LFCnt[]	PROGMEM = "LFCnt";
const char gcStr_OLCnt[]	PROGMEM = "OLCnt";

const char gcStr_A_LoAd[]	PROGMEM = "A.LoAd";
const char gcStr_A_nor[]	PROGMEM = "A.LASt";		//	PHJ mod from A.nor
const char gcStr_A_PEA[]	PROGMEM = "A.HiGH";		//	PHJ mod from A.PEA
const char gcStr_A_droP[]	PROGMEM = "L.droP";
const char gcStr_A_AcP[]	PROGMEM = "On.AcP";
const char gcStr_PrESS[]	PROGMEM = "ttlOn";		//	PHJ mod from PrESS
const char gcStr_T_Off[]	PROGMEM = "t-OFF";		//	PHJ

const char gcStr_O[]		PROGMEM = "O";
const char gcStr_No[]		PROGMEM = "no";
const char gcStr_PASS[]		PROGMEM = "PASS";
const char gcStr_FAIL[]		PROGMEM = "FAIL";
const char gcStr_Cancl[]	PROGMEM = "Cancl";
const char gcStr_Good[]		PROGMEM = "9ood";
const char gcStr_Littl[]	PROGMEM = "Littl";

const char gcStr_rESEt[]	PROGMEM = "rESEt";
const char gcStr_SurE_Q[]	PROGMEM = "SurE?";

const char gcStr_SEtuP[]	PROGMEM = "SEtUP";

const char gcStr_Cn_2[]		PROGMEM = "Cn-2";
const char gcStr_Ut[]		PROGMEM = "Ut";
const char gcStr_Ot[]		PROGMEM = "Ot";
const char gcStr_H_nd[]		PROGMEM = "H-nd";
const char gcStr_LcErr[]	PROGMEM = "LcErr";
const char gcStr_Eq_Eprs[]	PROGMEM = "=Eprs";
const char gcStr_OLoad[]	PROGMEM = "OLoad";

const char gcStr_bLoad[] 	PROGMEM = "bLOAD";

const char gcStr_batt[] 	PROGMEM = "batt";

const char gcStr_Lc_OFF[]	PROGMEM = "Lc.OFF";			// 2011-08-24 -WFC- loadcell disabled

const char gcStr_SoFt[]	PROGMEM = " SoFt";				//2012-09-20 -WFC- Soft version

const char gcStr_bLiFE[]	PROGMEM = "b.LiFE";			//2014-09-30 -WFC-
const char gcStr_Long[]		PROGMEM = "Long";			//2014-09-30 -WFC-

const char gcStr_LFcnt[]	PROGMEM = "LFcnt";			//2014-10-22 -WFC-

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 )
const char gcStr_unCal[]	PROGMEM = "unCal";

//2012-09-20 -WFC- const char gcStr_CH3_Sn[]	PROGMEM = "CH3.Sn";

const char gcStr_diSabL[]	PROGMEM = "diSab";
const char gcStr_UndrLd[]	PROGMEM = "UndLd";
const char gcStr_d_tESt[]	PROGMEM = "d.tESt";
const char gcStr_LO_1[]		PROGMEM = "LO-1";
const char gcStr_LO_2[]		PROGMEM = "LO-2";

const char gcStr_tArE[]		PROGMEM = "tArE";			// 2011-06-29 -WFC-
const char gcStr_StrnG[]	PROGMEM = "StrnG";			// 2011-06-29 -WFC-
const char gcStr_Cntrl[]	PROGMEM = "Cntrl";			// 2011-06-29 -WFC-
const char gcStr_USEr[]		PROGMEM = "USEr";			// 2011-06-29 -WFC-
const char gcStr_Cont[]		PROGMEM = "Cont";			// 2011-06-29 -WFC-
const char gcStr_5Unit[]	PROGMEM = "5Unit";			// 2011-06-29 -WFC-
const char gcStr_2Unit[]	PROGMEM = "2Unit";			// 2011-06-29 -WFC-
const char gcStr_rAtE[]		PROGMEM = "rAtE";			// 2011-07-14 -WFC-
const char gcStr_No_rF[]	PROGMEM = "No rF";			// 2011-10-21 -WFC-

#endif

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )		// 2011-06-10, 2011-09-27 -WFC-
const char gcStr_unCal[]	PROGMEM = "unCal";

//2012-09-20 -WFC-  const char gcStr_DL_2[]	PROGMEM = "DL-2 ";

const char gcStr_diSabL[]	PROGMEM = "diSab";
const char gcStr_UndrLd[]	PROGMEM = "UndLd";
const char gcStr_d_tESt[]	PROGMEM = "d.tESt";
const char gcStr_tArE[]		PROGMEM = "tArE";			// 2011-06-29 -WFC-
const char gcStr_StrnG[]	PROGMEM = "StrnG";			// 2011-06-29 -WFC-
const char gcStr_Cntrl[]	PROGMEM = "Cntrl";			// 2011-06-29 -WFC-
const char gcStr_USEr[]		PROGMEM = "USEr";			// 2011-06-29 -WFC-
const char gcStr_Cont[]		PROGMEM = "Cont";			// 2011-06-29 -WFC-
const char gcStr_5Unit[]	PROGMEM = "5Unit";			// 2011-06-29 -WFC-
const char gcStr_2Unit[]	PROGMEM = "2Unit";			// 2011-06-29 -WFC-
const char gcStr_rAtE[]		PROGMEM = "rAtE";			// 2011-07-14 -WFC-
const char gcStr_No_rF[]	PROGMEM = "No rF";			// 2011-10-21 -WFC-
#endif

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_HD || CONFIG_PRODUCT_AS == CONFIG_AS_HLI || CONFIG_PRODUCT_AS == CONFIG_AS_REMOTE_METER )	// 2011-10-14 -WFC-

#if (  CONFIG_PRODUCT_AS != CONFIG_AS_REMOTE_METER )
const char gcStr_unCal[]	PROGMEM = "un Cal";
const char gcStr_diSabL[]	PROGMEM = "diSabL";
const char gcStr_UndrLd[]	PROGMEM = "UndrLd";
#endif

const char gcStr_AdCbsy[]	PROGMEM = "AdCbsy";

const char gcStr_no_dSc[]	PROGMEM = "no-dSc";

const char gcStr_HLI_Sn[]	PROGMEM = "HLI-Sn";
const char gcStr_Hd_Sn[]	PROGMEM = "Hd-Sn";
const char gcStr_dSC_Sn[]	PROGMEM = "dSC-Sn";
const char gcStr_r_CAL1[]	PROGMEM = "r-CAL1";
const char gcStr_r_CAL2[]	PROGMEM = "r-CAL2";

const char gcStr_d_tst1[]	PROGMEM = "d-tSt1";
const char gcStr_d_tst2[]	PROGMEM = "d-tSt2";
const char gcStr_d_tst3[]	PROGMEM = "d-tSt3";
const char gcStr_d_tst4[]	PROGMEM = "d-tSt4";

const char gcStr_A2d_dc[]	PROGMEM = "A2d-dc";
const char gcStr_A2d_Ac[]	PROGMEM = "A2d-Ac";
const char gcStr_A2d_Or[]	PROGMEM = "A2d-Or";
const char gcStr_A2d_Ur[]	PROGMEM = "A2d-Ur";

const char gcStr_P_inLo[]	PROGMEM = "P-inLo";

// 2011-09-26 -WFC- v
const char gcStr_LO_1[]		PROGMEM = "LO-1";
const char gcStr_LO_2[]		PROGMEM = "LO-2";
const char gcStr_1hour[]	PROGMEM = "1hour";
const char gcStr_rCAL_Q[]	PROGMEM = "rCAL.?";
// 2011-09-26 -WFC- ^

#endif

#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_PORTAWEIGH2 )		// 2011-12-15 -DLM-
const char gcStr_unCal[]	PROGMEM = "unCal";

const char gcStr_PW2_Sn[]	PROGMEM = "PW2.Sn";

const char gcStr_diSabL[]	PROGMEM = "diSab";
const char gcStr_UndrLd[]	PROGMEM = "UndLd";
const char gcStr_d_tESt[]	PROGMEM = "d.tESt";
const char gcStr_LO_1[]		PROGMEM = "LO-1";
const char gcStr_LO_2[]		PROGMEM = "LO-2";

const char gcStr_tArE[]		PROGMEM = "tArE";			// 2011-06-29 -WFC-
const char gcStr_StrnG[]	PROGMEM = "StrnG";			// 2011-06-29 -WFC-
const char gcStr_Cntrl[]	PROGMEM = "Cntrl";			// 2011-06-29 -WFC-
const char gcStr_USEr[]		PROGMEM = "USEr";			// 2011-06-29 -WFC-
const char gcStr_Cont[]		PROGMEM = "Cont";			// 2011-06-29 -WFC-
const char gcStr_5Unit[]	PROGMEM = "5Unit";			// 2011-06-29 -WFC-
const char gcStr_2Unit[]	PROGMEM = "2Unit";			// 2011-06-29 -WFC-
const char gcStr_rAtE[]		PROGMEM = "rAtE";			// 2011-07-14 -WFC-
const char gcStr_No_rF[]	PROGMEM = "No rF";			// 2011-10-21 -WFC-

const char gcStr_Ethnt[]	PROGMEM = "Ethnt";			// 2012-04-27 -DLM-

const char gcStr_Listnr[]	PROGMEM = "Listn";			// 2014-07-15 -WFC- Listener ID.
const char gcStr_Output[]	PROGMEM = "Out-p";			// 2014-07-15 -WFC- Output
const char gcStr_Port_0[]	PROGMEM = "Port0";			// 2014-07-15 -WFC- Output
const char gcStr_Port_2[]	PROGMEM = "Port2";			// 2014-07-15 -WFC- Output


#endif


/*
Cal-n Info  changed to  "Cal"
UCal-n Warning changed to "un Cal"
OL-n Warning changed to "Error" then "LoAd"
UL-n Warning changed to "UnLd"
OR-n Warning changed to "Adc-Or"
UR-n Warning changed to "Adc-Ur"
UV Warning changed to "P-inLo"
UT Warning changed to "Ut"
OT Warning changed to "Ot"
no dSc Severe
Cn-2 Info
H-nd Info
*/
