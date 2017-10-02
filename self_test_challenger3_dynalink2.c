/*! \file self_test_challenger3_dyanlink2.c \brief self test of a product related functions.*/
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
//  History:  Created on 2011/06/17 by Wai Fai Chin
//
//   It is a high level module test all require ios, functions and features.
// This is product specific module. Special custom software should modify this file and module.
//
// ****************************************************************************
// History:
// 2011-06-17 -WFC- derived from self_test.c into Challenger3 or DyanLink2 specific self test.

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

#include	"panelmain.h"

#include	"voltmon.h"
#include  "loadcellfilter.h"	// 2014-09-16 -WFC-

const char gcStrEFB_CalMode_CM_s[]		PROGMEM = "88888,CM,%s\n\r";
const char gcStrEFB_ErrorCode_d_ER_s[]	PROGMEM = "%d,ER,%s\n\r";
const char gcStrEFB_RCal_RC_s[]			PROGMEM = ",RC,%s\n\r";
const char gcStrEFB_Model_SW_s[]		PROGMEM = ",SW,%s\n\r";
const char gcStrEFB_TestMode_TM_s[]		PROGMEM = "88888,TM,%s\n\r";
const char gcStrEFB_Normal_s_s[]		PROGMEM = ",%s,%s\n\r";
//
//const char gcStr_107[]				PROGMEM = "107";
//const char gcStr_234[]				PROGMEM = "234";
//const char gcStr_model_undef[]		PROGMEM = "???";
//
//// gtProductInfoFNV.userDefModelCode 2== chi 234, 1== Kawasaki 107; 0 == ATP.
//// const char *gcStr_List_Model[]		PROGMEM = {gcStr_234, gcStr_107, gcStr_model_undef };
//
//#define MODEL_ATP_DAC_CNT	0.0f
//#define MODEL_107_DAC_CNT	10700.0f
//#define MODEL_234_DAC_CNT	23400.0f


// const float gcafModelNameValue[]	PROGMEM	= { MODEL_ATP_DAC_CNT, MODEL_107_DAC_CNT, MODEL_234_DAC_CNT, MODEL_ATP_DAC_CNT };

char	self_test_loadcell_inputs_thread( struct pt *pt );
BYTE	self_test_verify_loadcell_inputs( INT32 vA, INT32 vB );
void	self_test_data_out_to_all_listeners( char *pStr, BYTE n );
void	self_test_format_switch_status( char *pStr );
char 	self_test_one_shot_main_thread( struct pt *pt );

BYTE	self_test_loadcell_customer_special_format_output( BYTE sn, char *pOutStr );
// BYTE	self_test_vs_math_customer_special_format_output( BYTE sn, char *pOutStr );

struct	pt		gSelfTestMain_thread_pt;

/// scratch thread variable, assume these threads are not run simultaneously.
struct	pt		gSelfTest_scratch_thread_pt;


TIMER_T		gSelfTestScratchTimer;		// This timer use by many self_test_xxxxx_thread() because it runs one thread at a time.

TIMER_T		gTimerInMinute_LED_Dim_Sleep;	// This timer use for set LED dimmed and in sleep mode.
TIMER_T		gTimerInMinute_Power_Off;		// This timer use for turn off the system.

/// System Running Mode use by main() to run system tasks.
extern	BYTE	gbSysRunMode;

#define	SELFTEST_THREAD_STATE_TEST_LOADCELL_INPUTS		0
#define	SELFTEST_THREAD_STATE_TEST_OTHER				1

BYTE	gbSelfTestThreadState;

BYTE	gbSysErrorCodeForUser;			// 2010-12-29 -WFC-

BYTE	gbSysPowerSaveState;		// 2014-09-09 -WFC-
BYTE	gbTmpDisabledPowerSaving;	// 2014-11-05 -WFC- temporary disabled power saving state.
BYTE	gbLoadcellWeightChanged;	// 2014-09-16 -WFC-
BYTE	gbSleepCycleCnt;			// 2014-09-29 -WFC-

TIMER_T	gSysPowerSaveTimer;		// 2014-09-09 -WFC- system power saving timer for use to detect delta weight.

BYTE	gabLoadcellBridgeInputsStatus[ MAX_NUM_LOADCELL ];

/**
 * Self test init routine.
 *
 * History:  Created on 2010/04/12 by Wai Fai Chin
 */

void self_test_init( void )
{
	BYTE i;

	for (i=0; i < MAX_NUM_LOADCELL; i++) {
		gabLoadcellBridgeInputsStatus[i] = LOADCELL_INPUT_STATUS_IS_GOOD;
	}

} // end self_test_main_tasks()


/**
 * Self test init and start a thread.
 *
 * @note: This is the place to add more logic before run the thread.
 *
 * History:  Created on 2010/04/09 by Wai Fai Chin
 */

void self_test_init_and_start_threads( void )
{
	self_test_init();
	PT_INIT( &gSelfTestMain_thread_pt );				// init self test thread.

} // end self_test_main_tasks()


/**
 * Self test stop all related threads and init all sensors again.
 *
 * @note: This is the place to add more logic before run the thread.
 *
 * History:  Created on 2010/04/09 by Wai Fai Chin
 */

void self_test_stop_all_threads( void )
{
	sensor_init_all();
} // end self_test_stop_all_threads()


/**
 * Self test main.
 *
 * History:  Created on 2010/04/09 by Wai Fai Chin
 */

void self_test_main_tasks( void )
{
	self_test_main_thread( &gSelfTestMain_thread_pt );
} // end self_test_main_tasks()


/**
 * ScaleCore PCB self test thread.
 *
 * @param  pt	-- pointer to Proto thread structure.
 *
 * History:  Created on 2010/04/07 by Wai Fai Chin
 */

//PT_THREAD( self_test_main_thread( struct pt *pt )) // Doxygen cannot handle this macro
char self_test_main_thread( struct pt *pt )
{
  PT_BEGIN( pt );
	for(;;) {
		PT_SPAWN( pt, &gSelfTest_scratch_thread_pt, self_test_loadcell_inputs_thread( &gSelfTest_scratch_thread_pt ));
	}
  PT_END(pt);
} // end self_test_main_thread()


/**
 * ScaleCore2 specific self test loadcell inputs.
 *
 * @param  pt			-- pointer to Proto thread structure.
 *
 * @note: To save RAM memories, it re-uses RCAL sensor descriptor to hold the individual loadcell input ADC reading for further analysis.
 *
 * Loadcell 1:
 * For SIG2:
 * If -Excitation(Black) is disconnected, both +SIG2 and -SIG2 reads  8388608.
 * IF +Excitation(Black) is disconnected, both +SIG2 and -SIG2 reads -8388609.
 * If -SIG2 (WHITE) is disconnected, -SIG3 reading was started read as -1187390 and reached stable value -8388609.
 * If +SIG2 (GREEN) is disconnected, +SIG2 reading was started read as -1759775 and slowly reached stable value -8388609.
 * If shorted both +SIG2 and -SIG2 together, both input readings are -8388609.
 *
 * Loadcell 0:
 * For SIG3:
 * If -Excitation(Black) is disconnected, both +SIG3 and -SIG3 reads  8388608.
 * IF +Excitation(Black) is disconnected, both +SIG3 and -SIG3 reads -8388609.
 * If -SIG3 (WHITE) is disconnected, -SIG3 reading was started read as -3534066 and reached stable value -8388609.
 * If +SIG3 (GREEN) is disconnected, +SIG3 reading was started read as -42924   and reached stable value -8388609.
 * If shorted both +SIG3 and -SIG3 together, both input readings are -8388609.
 *
 * History:  Created on 2010/04/07 by Wai Fai Chin
 */

//PT_THREAD( self_test_loadcell_inputs_thread( struct pt *pt )) // Doxygen cannot handle this macro
char self_test_loadcell_inputs_thread( struct pt *pt )
{
  PT_BEGIN( pt );
	gbSelfTestThreadState = SELFTEST_THREAD_STATE_TEST_OTHER;
  PT_END(pt);
} // end self_test_loadcell_inputs_thread()

/**
 * It analyzes loadcell inputs based on the given ADC counts.
 *
 * @param  vA	  -- ADC count of +SIG loadcell input.
 * @param  vB	  -- ADC count of -SIG loadcell input.
 *
 * @return status code of loadcell inputs.
 *
 * History:  Created on 2010/04/12 by Wai Fai Chin
 */

BYTE self_test_verify_loadcell_inputs( INT32 vA, INT32 vB )
{
	BYTE status;
	status = LOADCELL_INPUT_STATUS_IS_GOOD;		// assumed no error.
	return status;
} // end self_test_verify_loadcell_inputs(,)


/**
 * It is a self contained self test main module.
 *
 * History:  Created on 2010/04/09 by Wai Fai Chin
 */

void self_test_one_shot_main_tasks( void )
{
	self_test_one_shot_main_thread( &gSelfTestMain_thread_pt );
	if ( SELFTEST_THREAD_STATE_TEST_OTHER == gbSelfTestThreadState )
		adc_lt_update();				// update Linear Tech ADC configuration based on sensor descriptor and read ADC count of the sensor channel.
} // end self_test_one_shot_main_tasks()


/**
 * ScaleCore PCB self test thread.
 *
 * @param  pt	-- pointer to Proto thread structure.
 *
 * History:  Created on 2010/04/07 by Wai Fai Chin
 * 2010-10-25 -WFC- Instead of use user defined model, we use bargraph source sensor ID dictated focus of sensor for a remote device.
 * 2010-11-17 -WFC- Fixed not transmit Rcal value bug by set gabLoadcellRcalEnabled[n] to enable.
 * 2011-01-13 -WFC- HLI model performs 3 passes to test loadcell inputs to prevent false error.
 */

#define SELF_TEST_MAX_STRING_LEN	80

//PT_THREAD( self_test_one_shot_main_thread( struct pt *pt )) // Doxygen cannot handle this macro
char self_test_one_shot_main_thread( struct pt *pt )
{

  PT_BEGIN( pt );
	gbCmdSysRunMode = gbSysRunMode = SYS_RUN_MODE_NORMAL;
	self_test_stop_all_threads();
  PT_END(pt);
} // end self_test_one_shot_main_thread()


/**
 * It outputs test result based on each IO_STREAM object configuration settings.
 * An IO_STREAM ojbect could be a UART serail, SPI, RF modem, ethernet, usb etc...
 *
 * @param  pStr	-- points to a formated output string.
 * @param  n	-- length of pStr string.
 *
 * @return none
 *
 * History:  Created on 2007/10/08 by Wai Fai Chin
 */

void	self_test_data_out_to_all_listeners( char *pStr, BYTE n )
{
	BYTE i;					// loop variable
	IO_STREAM_CLASS			*pOstream;	// pointer to a dynamic out stream object.

	//for ( i=0; i < MAX_NUM_STREAM_LISTENER;	i++) {					// walk through all IO_STREAM object.
	for ( i=0; i < 1;	i++) {					// walk through all IO_STREAM object.
		if (stream_router_get_a_new_stream( &pOstream ) ) {			// get an output stream to this listener.
			pOstream-> type		= gabListenerStreamTypeFNV[i];		// stream type for the listener.
			pOstream-> status	= IO_STREAM_STATUS_ACTIVE;			// output dir, active
			pOstream-> sourceID	= gtProductInfoFNV.devID;
			pOstream-> destID	= gabListenerDevIdFNV[ i ];

			//if ( LISTENER_MODE_CUSTOMER_SPECIAL == gabListenerModesFNV[i]  ) { no need for this because this method only call by single shot self test task. This will force output self test data regardless of listener mode.
				stream_router_routes_a_ostream_now( pOstream, pStr, n );
			//}// end if ( LISTENER_MODE_CUSTOMER_SPECIAL == gabListenerModesFNV[i]  )
			stream_router_free_stream( pOstream );	// put resource back to stream pool.
		} // end if (stream_router_get_a_new_stream( &pOstream ) ) {}
	} // end for (i=0; i < MAX_NUM_STREAM_LISTENER;	i++) {}
} // end self_test_data_out_to_all_listeners(,)

/**
 * It formated a ASCII string based on the binary switches status.
 * s,s,s,s where s could be '0' or '1', 1== high, 0 == low. first one is D1, last one is DF4.
 *
 * @param  pStr	-- points to a string buffer to be holding formated switch status.
 *
 * @return none
 *
 * History:  Created on 2010/04/20 by Wai Fai Chin
 */

void	self_test_format_switch_status( char *pStr )
{
} // end self_test_format_switch_status()


/**
 * It formats sensor data to be output by comport objects.
 *
 * @param  sensorID -- sensor ID or channel number.
 * @param  pOutStr  -- points to an allocated output string buffer.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2007/08/03 by Wai Fai Chin
 */

BYTE self_test_customer_special_format_value( BYTE sensorID, char *pOutStr )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to local sensor descriptor
	BYTE n;

	n = 0;
	if ( sensorID < MAX_NUM_SENSORS ) {					// ensured sensor number is valid.
		pSnDesc = &gaLSensorDescriptor[ sensorID ];
		switch ( gabSensorTypeFNV[ sensorID ] )	{
			case SENSOR_TYPE_LOADCELL:
			case SENSOR_TYPE_MATH_LOADCELL:
					n = self_test_loadcell_customer_special_format_output( sensorID, pOutStr );
				break;
//			#if ( CONFIG_INCLUDED_VS_MATH_MODULE ==	TRUE )
//			case SENSOR_TYPE_MATH_LOADCELL:
//				n = self_test_vs_math_customer_special_format_output( sensorID, pOutStr );
//				break;
//			#endif
			// TODO: SENSOR_TYPE_REMOTE
		} // end switch() {}
	} // end if ( sensorID < MAX_NUM_SENSORS ) {}
	return n;	// number of char in the output string buffer.
} // end self_test_customer_special_format_value(,)

/**
 * It formats sensor data based on specific product.
 * Its format is hard coded at compiled time.
 *
 * @param  sn		-- sensor ID or channel number.
 * @param  pOutStr  -- points to an allocated output string buffer.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2010-04-23 by Wai Fai Chin
 * 2010-10-14 -WFC- critical orders are matched with HD LED output.
 * 2010-10-14 -WFC- undefined model is the highest critical error.
 * 2010-10-14 -WFC- Treat wrong math expression as sensor disabled, because there is no corresponding LED error message. Also we don't have to modify cmd {05}.
 * 2010-11-17 -WFC- Fixed copied and pasted bug of under temperature message code
 */

BYTE self_test_loadcell_customer_special_format_output( BYTE sn, char *pOutStr )
{
	BYTE	len;
	BYTE	n;
	BYTE	errorCode;
	BYTE    precision;
	BYTE	state;
	float	fRound;
	PGM_P	ptr_P;
	char	unitName[5];
	char 	formatBuf[14];
	LOADCELL_T *pLc;

	errorCode = len = n = 0;
	if ( sn < MAX_NUM_SENSORS ) {
		errorCode = self_test_check_sys_error_code_for_user( sn, &state, &fRound );

		self_test_format_switch_status( formatBuf );

		if ( gbSysErrorCodeForUser ) {	// handle the above error conditions.
			if ( SYS_USER_ERROR_CODE_IN_CAL == gbSysErrorCodeForUser )
				len = (BYTE) sprintf_P( pOutStr, gcStrEFB_CalMode_CM_s, formatBuf );
			else
				len = (BYTE) sprintf_P( pOutStr, gcStrEFB_ErrorCode_d_ER_s, errorCode, formatBuf);
		}
		else if ( SENSOR_STATE_GOT_VALID_VALUE == state ) {
				pLc = gaLSensorDescriptor[ sn ].pDev;
				// memcpy_P ( unitName, gcUnitNameTbl[ pLc-> viewCB.unit ], 5); bug because gcUnitNameTbl[] is a two level pointers in program memory space. Fix this bug by get the pointer to string first, then copy the string from program memory space.
				memcpy_P ( &ptr_P, &gcUnitNameTbl[ pLc-> viewCB.unit ], sizeof(PGM_P));
				strcpy_P(unitName, ptr_P);
				if ( pLc-> viewCB.decPt > 0 )
					precision = pLc-> viewCB.decPt;
				else
					precision = 0;
				float_format( formatBuf, 8, precision);							// Configured printf format for floating point print out.

				len = (BYTE) sprintf( pOutStr, formatBuf, fRound );
				self_test_format_switch_status( formatBuf );
				unitName [2] = 0; // only use the first two characters.
				n = (BYTE) sprintf_P( pOutStr + len, gcStrEFB_Normal_s_s, unitName, formatBuf );
				len += n;
		}
	}

	return len;	// number of char in the output string buffer.
} // end self_test_loadcell_customer_special_format_output(,)

/**
 * It check for system error and reported it based on priority.
 *
 * @param  sn		-- sensor ID or channel number.
 * @param	pState	-- state of sensor sn.
 * @param	pfRound	-- sensor rounded value.
 *
 * @return sys error code for user.
 *
 * History:  Created on 2010-12-29 by Wai Fai Chin
 * 2011-04-18 -WFC- If there is small motion event, reset power off timer.
 */

BYTE self_test_check_sys_error_code_for_user( BYTE sn, BYTE *pState, float *pfRound )
{
	BYTE	errorCode;
	BYTE	state;
	BYTE	statebm;
	LOADCELL_T *pLc;

	errorCode = SYS_USER_ERROR_CODE_NO_ERROR;
	*pState = SENSOR_STATE_WRONG_INDEX;
	if ( sn < MAX_NUM_SENSORS ) {
		if ( sn < MAX_NUM_PV_LOADCELL ) {
			pLc = &gaLoadcell[ sn ];
			if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[sn] )
				statebm = loadcell_get_value_of_type( sn, SENSOR_VALUE_TYPE_FILTERED | SENSOR_VALUE_TYPE_CUR_MODE, SENSOR_VALUE_UNIT_CUR_MODE, pfRound );
			else if ( SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[sn] )
				statebm = vs_math_get_this_value( sn, SENSOR_VALUE_TYPE_FILTERED | SENSOR_VALUE_TYPE_CUR_MODE, SENSOR_VALUE_UNIT_CUR_MODE, pfRound );

			*pState = state = ~SENSOR_STATE_IN_CAL_bm & statebm;			// stripped out In Cal flag
			if ( gtProductInfoFNV.userDefModelCode > USER_DEF_MODEL_MAX ){
				errorCode = SYS_USER_ERROR_CODE_UNDEF_MODEL;
			}
			else if ( gabLoadcellBridgeInputsStatus[ 0 ] || gabLoadcellBridgeInputsStatus[ 1 ] ) {
				errorCode = SYS_USER_ERROR_CODE_LC_ERROR;
			}
			else if ( SENSOR_STATE_DISABLED == state ) {
				errorCode = SYS_USER_ERROR_CODE_LC_DISABLED;
			}
			else if (SENSOR_STATE_IN_CAL_bm & statebm || ( LC_RUN_MODE_IN_CAL & pLc-> runModes )) {
				errorCode = SYS_USER_ERROR_CODE_IN_CAL;
			}
			else if ( SENSOR_STATE_UNCAL == state || (LC_STATUS2_UN_CAL & pLc-> status2 )) {
				errorCode = SYS_USER_ERROR_CODE_UN_CAL;
			}
			else if ( LC_STATUS2_WRONG_MATH_EXPRESSION & pLc-> status2 ) {
				errorCode = SYS_USER_ERROR_CODE_WRONG_MATH_EXPRS;
			}
			else if ( SENSOR_STATE_ADC_BUSY == state ) {
				errorCode = SYS_USER_ERROR_CODE_ADC_CHIP_BUSY;
			}
			else if ( SENSOR_STATE_OVERLOAD == state ) {
				errorCode = SYS_USER_ERROR_CODE_OVERLOAD;
			}
			else if ( SENSOR_STATE_OVER_RANGE == state ) {
				errorCode = SYS_USER_ERROR_CODE_OVERRANGE;
			}
			else if ( SENSOR_STATE_UNDERLOAD == state ) {
				errorCode = SYS_USER_ERROR_CODE_UNDERLOAD;
			}
			else if ( SENSOR_STATE_UNDER_RANGE == state ) {
				errorCode = SYS_USER_ERROR_CODE_UNDERRANGE;
			}

			// 2011-04-18 -WFC- If there is small motion event, reset power off timer. v
			if (  LC_STATUS3_SMALL_MOTION & pLc->status3 )	// if there is a small motion in a loadcell, then
				self_test_timer_reset_event_timers();	// reset event timers so it would not trigger an expiration event that would execute LED sleep or power off etc..
			// 2011-04-18 -WFC- ^
		}
	}

	return errorCode;
} // end self_test_check_sys_error_code_for_user()


/**
 * It updates system error code for user based on SensorID, voltage and temperature.
 * Warning message only output during error message output mode.
 *
 * @param  sn			-- sensor ID or channel number.
 * @param  cycleMode	-- output mode  4 to 7 ==normal; 0 to 3 == warning msg mode.
 *
 *
 * History:  Created on 2011-01-05 by Wai Fai Chin
 * 2011-04-18 -WFC- added code to reset power off timer if there is active communication event.
 */

void self_test_update_sys_error_code_for_user( BYTE sn, BYTE cycleMode)
{
	BYTE	errorCode;
	BYTE    precision;
	BYTE	state;
	float	fRound;

	errorCode = 0;
	// 2011-04-18 -WFC- added code to reset power off timer if there is active communication event. v
	if ( BIOS_SYS_STATUS_ACTIVE_COMMUNICATION & gbBiosSysStatus ) {
		self_test_timer_reset_event_timers();	// reset event timers so it would not trigger an expiration event that would execute LED sleep or power off etc..
		gbBiosSysStatus &= ~BIOS_SYS_STATUS_ACTIVE_COMMUNICATION;		// clear active communication flag.
	}
	// 2011-04-18 -WFC- ^

	if ( sn < MAX_NUM_SENSORS ) {
		gbSysErrorCodeForUser = errorCode = self_test_check_sys_error_code_for_user( sn, &state, &fRound );
		voltmon_is_under_voltage( VOLTMON_UNDER_VOLTAGE_SN );		// need to check for battery voltage level.
		if ( !errorCode ) {	// if no loadcell error or other critical error
			errorCode = 0;
			if ( BIOS_SYS_STATUS_UNDER_VOLTAGE & gbBiosSysStatus )
				errorCode = SELF_TEST_ERROR_UNDER_VOLTAGE;
			state = temp_sensor_is_temp_outside_spec( SENSOR_NUM_PCB_TEMPERATURE );

			if ( cycleMode < 4 ) { // alternately output error codes
				if ( !errorCode ) {// if no under voltage error, then check operation temperature
					if ( SYS_HW_STATUS_ANNC_OVER_TEMPERATURE == state )
						gbSysErrorCodeForUser = SYS_USER_ERROR_CODE_OVER_TEMPERATURE;
					else if ( SYS_HW_STATUS_ANNC_UNDER_TEMPERATURE == state )
						gbSysErrorCodeForUser = SYS_USER_ERROR_CODE_UNDER_TEMPERATURE;
				}
				else {
					gbSysErrorCodeForUser = errorCode;
				}
			}
		}
	}

} // end self_test_update_sys_error_code_for_user()


/**
 * It update sys operation behavior such as dim LED, auto off ect...
 *
 * History:  Created on 2011-05-12 by Wai Fai Chin
 * 2011-06-15 -WFC- check auto power off and LED sleep timers and execute action once a timer had been expired.
 * 2011-08-23 -WFC- handle pending power off.
 * 2012-09-27 -WFC- call led_display_turn_all_annunciators(), led_display_turn_all_led_digits().
 * 2013-05-07 -WFC- replaced bios_power_off_timer() with bios_power_off_by_shutdown_event();
 * 2014-09-10 -WFC- handle power saving mode.
 * 2015-05-07 -WFC- check first power up expiration time.
 */

void self_test_time_critical_sys_behavior_update( void )
{

	// 2015-05-07 -WFC- v
	extern TIMER_T 	powerUpTimer;

	if ( SYS_STATUS_DURING_POWER_UP & gbSysStatus)	{ 		// if system is in power up state.
		if ( timer_mSec_expired( &powerUpTimer ) ) {		// if first power up time expired
			gbSysStatus &= ~SYS_STATUS_DURING_POWER_UP;		// clear first power status.
		}
	}
	// 2015-05-07 -WFC- ^

	if ( (SCALE_STD_MODE_POWER_SAVING_ENABLED == ( gbScaleStandardModeNV & SCALE_STD_MODE_POWER_SAVING_MASK )) &&
			!(gaLoadcell[0].runModes &  LC_RUN_MODE_PEAK_HOLD_ENABLED) &&
			!gbTmpDisabledPowerSaving )
		self_test_time_critical_sys_behavior_update_power_save_mode();
	else {
		gbSysPowerSaveState = SYS_POWER_SAVE_STATE_INACTIVE;
		// If there is small motion event, reset power off timer to change LED intensity etc..
		if (  LC_STATUS3_SMALL_MOTION & gaLoadcell[0].status3 )	// if there is a small motion in a loadcell, then
			self_test_timer_reset_event_timers();	// reset event timers so it would not trigger an expiration event that would execute LED sleep or power off etc..

		if ( gbPanelMainRunMode == PANEL_RUN_MODE_NORMAL )	//	sleep and auto off only work when normal
		{
			if ( gtSystemFeatureFNV.autoOffMode ) {			// if enabled auto power off, then...
				if ( timer_minute_expired( &gTimerInMinute_Power_Off ) )		{
					// 2013-05-07 -WFC- bios_power_off_timer();
					bios_power_off_by_shutdown_event();		// 2013-05-07 -WFC-
				}
			}

			#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-//
			if ( gtSystemFeatureFNV.ledSleep ) {			// if LED auto sleep mode is enabled.
				if ( ! led_display_is_led_dimmed_sleep()) {		// if LED has not been dimmed,
					if ( timer_minute_expired( &gTimerInMinute_LED_Dim_Sleep ) )		{	// if it is time to dim the LED
						led_display_set_intensity( 0 );		// dim LED by set it to the lowest intensity.
						led_display_set_led_dimmed_sleep_flag();	// flag LED is dimmed.
						led_display_turn_all_annunciators( OFF );	// 2012-09-27 -WFC-
						led_display_turn_all_led_digits( OFF );		// 2012-09-27 -WFC-
					}
				}
			}
			#endif
		}
	}
} // end self_test_time_critical_sys_behavior_update()

/* 2014-09-10 -WFC-
void self_test_time_critical_sys_behavior_update( void )
{
	// If there is small motion event, reset power off timer to change LED intensity etc..
	if (  LC_STATUS3_SMALL_MOTION & gaLoadcell[0].status3 )	// if there is a small motion in a loadcell, then
		self_test_timer_reset_event_timers();	// reset event timers so it would not trigger an expiration event that would execute LED sleep or power off etc..

	// 2011-08-23 -WFC- v
//	if ( BIOS_RUN_STATUS_PEND_POWER_OFF & gbBiosRunStatus ) {
//		// test only if ( timer_mSec_expired( &gSelfTestScratchTimer ))  // if timeout, it means there is NO ADC chip or a bad ADC chip.
//			// 2013-05-07 -WFC- bios_power_off_timer();
//			// test only bios_power_off_by_shutdown_event();		// 2013-05-07 -WFC-
//	}
	// 2011-08-23 -WFC- ^

	if ( gbPanelMainRunMode == PANEL_RUN_MODE_NORMAL )	//	sleep and auto off only work when normal
	{
		if ( gtSystemFeatureFNV.autoOffMode ) {			// if enabled auto power off, then...
			if ( timer_minute_expired( &gTimerInMinute_Power_Off ) )		{
				// 2013-05-07 -WFC- bios_power_off_timer();
				bios_power_off_by_shutdown_event();		// 2013-05-07 -WFC-
			}
		}

		#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-//
		if ( gtSystemFeatureFNV.ledSleep ) {			// if LED auto sleep mode is enabled.
			if ( ! led_display_is_led_dimmed_sleep()) {		// if LED has not been dimmed,
				if ( timer_minute_expired( &gTimerInMinute_LED_Dim_Sleep ) )		{	// if it is time to dim the LED
					led_display_set_intensity( 0 );		// dim LED by set it to the lowest intensity.
					led_display_set_led_dimmed_sleep_flag();	// flag LED is dimmed.
					led_display_turn_all_annunciators( OFF );	// 2012-09-27 -WFC-
					led_display_turn_all_led_digits( OFF );		// 2012-09-27 -WFC-
				}
			}
		}
		#endif
	}

} // end self_test_time_critical_sys_behavior_update()
*/

void self_test_time_critical_sys_behavior_update_power_save_mode( void )
{
	if ( SYS_POWER_SAVE_STATE_INACTIVE == gbSysPowerSaveState ) {
		if (  LC_STATUS3_SMALL_MOTION & gaLoadcell[0].status3 )	{// if there is a small motion in a loadcell, then
			self_test_timer_reset_event_timers();	// reset event timers so it would not trigger an expiration event that would execute LED sleep or power off etc..
			timer_mSec_set( &gSysPowerSaveTimer, TT_5SEC);
		}
	}
	else { // it is in power saving state
		if ( gbLoadcellWeightChanged || ( gbSleepCycleCnt > 5 )) {
//			gbLoadcellWeightChanged = FALSE;
//			gbSleepCycleCnt = 0;
//			gbSysPowerSaveState = SYS_POWER_SAVE_STATE_INACTIVE;
//			bios_clock_normal();			// 2015-01-14 -WFC-
//			BIOS_ENABLE_AC_EXCITATION;		// turn on excitation
//
//			#if ( CONFIG_RF_MODULE_AS	==	CONFIG_RF_MODULE_AS_XBEE )
//			BIOS_TURN_ON_XBEE;
//			#endif
//TODO:MERGE Make it for Challenger3:
//			serial0_port_init( SR_BAUD_9600_V2MHZ );		// reconfigure serial port based on new system clock speed.
//			serial1_port_init( SR_BAUD_9600_V2MHZ );		// reconfigure and enabled serial port

			bios_enabled_comports_in_power_save_state();	// 2015-01-14 -WFC-
			self_test_timer_reset_event_timers();
			timer_mSec_set( &gSysPowerSaveTimer, TT_5SEC);
		}
	}

	if ( (gbPanelMainRunMode == PANEL_RUN_MODE_NORMAL) && (SYS_RUN_MODE_IN_CNFG != gbSysRunMode) )	//	sleep, auto off and power saving mode only work during normal weighting mode. This prevent shut off during configuration etc..
	{
		if ( gtSystemFeatureFNV.autoOffMode ) {			// if enabled auto power off, then...
			if ( timer_minute_expired( &gTimerInMinute_Power_Off ) )		{
				bios_power_off_by_shutdown_event();
			}
		}
		else {
			if ( timer_mSec_expired( &gSysPowerSaveTimer ) ) {		// if no changed in weight in the pass 5 seconds, then:
				gaLSensorDescriptor[0].prvSleepCycleADCcount = gaLSensorDescriptor[0].curADCcount;  // test only, to be done by a function to save all loadcell curADCcount.
				gbSysPowerSaveState = SYS_POWER_SAVE_STATE_ACTIVE;
				bios_power_off_and_auto_wakeup();		// turn off excitation, turn off Rf radio, etc...
			}
		}

		#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-//
		if ( gtSystemFeatureFNV.ledSleep ) {			// if LED auto sleep mode is enabled.
			if ( ! led_display_is_led_dimmed_sleep()) {		// if LED has not been dimmed,
				if ( timer_minute_expired( &gTimerInMinute_LED_Dim_Sleep ) )		{	// if it is time to dim the LED
					led_display_set_intensity( 0 );		// dim LED by set it to the lowest intensity.
					led_display_set_led_dimmed_sleep_flag();	// flag LED is dimmed.
					led_display_turn_all_annunciators( OFF );	// 2012-09-27 -WFC-
					led_display_turn_all_led_digits( OFF );		// 2012-09-27 -WFC-
				}
			}
		}
		#endif
	}

} // end self_test_time_critical_sys_behavior_update()



/**
 * It resets event timers for compute next expiration event.
 *
 * History:  Created on 2011-06-14 by Wai Fai Chin
 * 2012-09-28 -WFC- turn off all annunciators after LED woke up.
 * 2015-01-15 -WFC- ensured LED intensity value not over limit.
 * 2015-01-16 -WFC- Set intensity to lowest if in power saving state.
 */

void self_test_timer_reset_event_timers( void )
{
	extern UINT8	gTTimer_minute;			// task timer update value in minute.  2011-06-14 -WFC-
	UINT16 w;		// 2015-01-15 -WFC-

	#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-//
	if ( led_display_is_led_dimmed_sleep()) {						// if LED segments were dimmed and in sleep mode, then
		if ( SYS_POWER_SAVE_STATE_ACTIVE == gbSysPowerSaveState)	// 2015-01-16 -WFC-
			led_display_set_intensity( 0 );							// 2015-01-16 -WFC-
		else if ( gtSystemFeatureFNV.ledIntensity )						// if LED is in user configured intensity mode, then
			#if CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2
			led_display_set_intensity( ((1 << gtSystemFeatureFNV.ledIntensity ) - 1 ) ); // dimming LED based on user led intensity setting.
			#else
			led_display_set_intensity( ((1 << gtSystemFeatureFNV.ledIntensity ) - 2 ) ); // dimming LED based on user led intensity setting.
			#endif
		else 	{													// else it is in auto intensity, use light sensor ADC to set LED intensity.
			// 2013-11-08 -WFC- led_display_set_intensity(((BYTE)(gaLSensorDescriptor[ SENSOR_NUM_LIGHT_SENSOR ].curADCcount >> 6)) );	//dimming LED based on light sensor ADC count.
			// 2015-01-15 -WFC- v
			// led_display_set_intensity(((BYTE)( (gaLSensorDescriptor[ SENSOR_NUM_LIGHT_SENSOR ].curADCcount + 2047 )>> 8)) );	//dimming LED based on light sensor ADC count. 2013-11-08 -WFC-
			w = (UINT16)( (gaLSensorDescriptor[ SENSOR_NUM_LIGHT_SENSOR ].curADCcount + 2048 )>> 6);
			if ( w > 0x0F ) w = 0xF;
			led_display_set_intensity( (BYTE) w );
			// 2015-01-15 -WFC- ^
		}
		led_display_clear_led_dimmed_sleep_flag();					// flag that LED is no longer in sleep mode, its intensity is based on user configuration.
		led_display_turn_all_annunciators( OFF );	// 2012-09-28 -WFC-
	}
	//	timer_minute_set( &gTimerInMinute_LED_Dim_Sleep, gTimerInMinute_LED_Dim_Sleep.interval );
	gTimerInMinute_LED_Dim_Sleep.start	= gTTimer_minute;
	#endif

//	timer_minute_set( &gTimerInMinute_Power_Off, gTimerInMinute_Power_Off.interval );
	// for the sake of save code memory space, I used direct settings instead of calling timer_minute_set();
	gTimerInMinute_Power_Off.start		= gTTimer_minute;
}

/**
 * It sets event timers interval value based on user configuration.
 * Timers in this function are gTimerInMinute_Power_Off and gTimerInMinute_LED_Dim_Sleep.
 * History:  Created on 2011-06-14 by Wai Fai Chin
 */

void self_test_set_event_timers_interval( void )
{
	switch (gtSystemFeatureFNV.autoOffMode) 	{
		case 0:
				gTimerInMinute_Power_Off.interval = 255;
			break;
		case 1:
				gTimerInMinute_Power_Off.interval = 15;
			break;
		case 2:
				gTimerInMinute_Power_Off.interval = 30;
			break;
		case 3:
				gTimerInMinute_Power_Off.interval = 45;
			break;
		case 4:
				gTimerInMinute_Power_Off.interval = 60;
			break;
		default:
			gTimerInMinute_Power_Off.interval = 255;
			break;
	}

	#if ( CONFIG_PRODUCT_AS	== CONFIG_AS_CHALLENGER3 || CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 ) // 2011-12-15 -DLM-//
	switch ( gtSystemFeatureFNV.ledSleep ) 	{
		case 0:
			gTimerInMinute_LED_Dim_Sleep.interval = 255;
			break;
		case 1:
			gTimerInMinute_LED_Dim_Sleep.interval = 5;
			break;
		case 2:
			gTimerInMinute_LED_Dim_Sleep.interval = 15;
			break;
		case 3:
			gTimerInMinute_LED_Dim_Sleep.interval = 30;
			break;
		default:
			gTimerInMinute_LED_Dim_Sleep.interval = 255;
			break;
	}
	#endif
} // end self_test_set_event_timers_interval()

/**
 * It init power saving related stuff.
 *
 * History:  Created on 2014-09-12 by Wai Fai Chin
 *
 */

void self_test_power_saving_init( void )
{
	gbSleepCycleCnt = 0;
	gbSysPowerSaveState = SYS_POWER_SAVE_STATE_INACTIVE;
	gbTmpDisabledPowerSaving =
	gbLoadcellWeightChanged = FALSE;
	timer_mSec_set( &gSysPowerSaveTimer, TT_12SEC + TT_700mSEC);
}

/**
 * It detects loadcell motion based on ADC counts
 * @param  lc	-- loadcell number
 *
 * History:  Created on 2014-09-15 by Wai Fai Chin
 *
 */
extern LC_FILTER_MANAGER_T gFilterManager[ MAX_NUM_LOADCELL ];

void self_test_detect_loadcell_motion( BYTE lc )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	SENSOR_CAL_T			*pCal;			// points to a cal table
	LOADCELL_T 				*pLc;			// points to a loadcell
	INT32 			deltaADCcount;

	if ( lc < MAX_NUM_LOADCELL )	{
		pSnDesc = &gaLSensorDescriptor[ lc ];
		pLc = (LOADCELL_T *) pSnDesc-> pDev;
		pCal = &(pLc-> pCal[0]);
		//sensorStatus = pSnDesc-> status & ~SENSOR_STATUS_GOT_VALUE;		// Assume it has no valid value.

		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {						// if this sensor is enabled.
			if ( (SENSOR_STATUS_GOT_ADC_CNT | SENSOR_STATUS_GOT_UNFILTER_ADC_CNT) & (pSnDesc-> status) ) {	// if got a new ADC count or unfiltered ADC count
				if ( CAL_STATUS_COMPLETED == pCal-> status ){				// if this loadcell has a calibration table.
					if ( SENSOR_STATUS_GOT_ADC_CNT  & (pSnDesc-> status) ) {		// if it has new ADC count
						// compute weight data with filtered ADC count.
						//pSnDesc-> value = adc_to_value( pSnDesc-> curADCcount,	&(pCal-> adcCnt[0]), &(pCal-> value[0]));
						deltaADCcount = pSnDesc-> curADCcount - pSnDesc->prvSleepCycleADCcount;
						if ( deltaADCcount < 0 )
							deltaADCcount = -deltaADCcount;
						if ( deltaADCcount > ((gFilterManager[lc].adcCountPerD )<<4) ) {
							gbLoadcellWeightChanged = TRUE;
						}
					}
				}
			} // end if got a new ADC count.
		}
	} // end if ( lc < MAX_NUM_LOADCELL )
} // end self_test_detect_loadcell_motion();
