/*! \file scalecore_sys.h \brief system relate stuff..*/
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
// Software layer:  application, concrete layer
//
//  History:  Created on 2009-04-28 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup system_run_mode system relate stuff.
/// \code #include "scalecore_sys.h" \endcode
/// \par Overview
//
// ****************************************************************************
//@{


#ifndef MSI_SCALECORE_SYS_H
#define MSI_SCALECORE_SYS_H

#include	"nvmem.h"


#define  SYS_RUN_MODE_NORMAL					0
#define  SYS_RUN_MODE_IN_CNFG					1
#define  SYS_RUN_MODE_SELF_TEST					2
#define  SYS_RUN_MODE_ONE_SHOT_SELF_TEST		3
#define  SYS_RUN_MODE_AUTO_SECONDARY_TEST		4
#define  SYS_RUN_MODE_UI_SECONDARY_TEST			5
#define  SYS_RUN_MODE_TEST_EXTERNAL_LED_BOARD	6
#define	 SYS_RUN_MODE_RF_REMOTE_LEARN			7					//	PHJ
#define  SYS_RUN_MODE_EXIT_CNFG_SAVE_FRAM		20
#define  SYS_RUN_MODE_EXIT_CNFG_SAVE_ALL		21
#define  SYS_RUN_MODE_EXIT_CNFG_NO_SAVE			22
#define  SYS_RUN_MODE_PEND_POWER_OFF			170					// Pending power off in 1 second. 2011-08-23 -WFC-

// 2011-08-23 -WFC- #define	MAX_NUM_SYS_RUN_MODE	SYS_RUN_MODE_EXIT_CNFG_NO_SAVE
#define	MAX_NUM_SYS_RUN_MODE	SYS_RUN_MODE_PEND_POWER_OFF			// 2011-08-23 -WFC-

// The following are designed for synchronize remote device to display test steps.
#define  SYS_STATUS_SELF_TEST_STEP_START			0
#define  SYS_STATUS_SELF_TEST_STEP_MODEL_NAME		1
#define  SYS_STATUS_SELF_TEST_STEP_RCAL_VALUE_1		2
#define  SYS_STATUS_SELF_TEST_STEP_RCAL_VALUE_2		3
#define  SYS_STATUS_SELF_TEST_STEP_END_TEST			100

// Temperature, voltage status
#define  SYS_HW_STATUS_ANNC_UNDER_VOLTAGE			0x80
#define  SYS_HW_STATUS_ANNC_OVER_TEMPERATURE		0x40
#define  SYS_HW_STATUS_ANNC_UNDER_TEMPERATURE		0x20

#define  SYS_STATUS_ANNC_HIRES_MODE					0x10		// 2012-02-23 -WFC-

#define  SYS_OVER_TEMPERATURE_KELVIN		343
#define  SYS_UNDER_TEMPERATURE_KELVIN		228

// High level error code for user. This is mainly use for EFB and LED display output.
// Loadcell related status was based on focus loadcell selected by bargraph SensorID of command {39}.
#define  SYS_USER_ERROR_CODE_NO_ERROR			0
#define  SYS_USER_ERROR_CODE_UNDEF_MODEL		1
#define  SYS_USER_ERROR_CODE_LC_ERROR			2
#define  SYS_USER_ERROR_CODE_LC_DISABLED		3
#define  SYS_USER_ERROR_CODE_IN_CAL				4
#define  SYS_USER_ERROR_CODE_UN_CAL				5
#define  SYS_USER_ERROR_CODE_WRONG_MATH_EXPRS	6
#define  SYS_USER_ERROR_CODE_ADC_CHIP_BUSY		7
#define  SYS_USER_ERROR_CODE_OVERLOAD			8
#define  SYS_USER_ERROR_CODE_OVERRANGE			9
#define  SYS_USER_ERROR_CODE_UNDERLOAD			10
#define  SYS_USER_ERROR_CODE_UNDERRANGE			11
#define  SYS_USER_ERROR_CODE_UNDER_VOLTAGE		12
#define  SYS_USER_ERROR_CODE_OVER_TEMPERATURE	13
#define  SYS_USER_ERROR_CODE_UNDER_TEMPERATURE	14
#define  SYS_USER_ERROR_CODE_MAX_ITEM			15
#define	 SYS_USER_ERROR_CODE_START_WARNING_CODE		SYS_USER_ERROR_CODE_UNDER_VOLTAGE
// 2014-10-31 -WFC- v
#define  SYS_USER_ERROR_CODE_AUTO_TOTAL_OFF		200
#define  SYS_USER_ERROR_CODE_AUTO_TOTAL_ON		201
// 2014-10-31 -WFC- ^

#define  SYS_POWER_SAVE_STATE_INACTIVE		0
#define  SYS_POWER_SAVE_STATE_ACTIVE		1

// 2015-05-07 -WFC- V
#define SYS_STATUS_DURING_POWER_UP			0x01
#define SYS_STATUS_CAL_SWITCH_TRIPPED		0x02
/// System status 2015-05-07 -WFC-
extern BYTE	gbSysStatus;
// 2015-05-07 -WFC- ^

/// System Running Mode use by main() to run system tasks.
extern BYTE		gbSysRunMode;
extern UINT16	gwSysStatus;			// system status;

/// System Power Saving Running State	2014-09-09 -WFC-
extern BYTE	gbSysPowerSaveState;

// for remote smart meter
#if ( CONFIG_PRODUCT_AS == CONFIG_AS_HD )

/// A remote device System Running Mode use by main() to run system tasks.
extern BYTE		gbAckCmdSysRunMode;
extern UINT16	gwAckSysStatus;			// system status of a remote device;

///	Acknowledgment from a remote device ID, model code, model name, serial numbers.
extern PRODUCT_INFO_OF_REMOTE_DEVICE_T		gtAckProductInfoRemoteDev;
#endif

#endif	// MSI_SCALECORE_SYS_H
//@}
