/*! \file adc_cpu.c \brief internal CPU ADC related functions.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//               Copyright (c) 2008 to 2013 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Hardware: ATMEL ATXMEGA192D3
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2008/12/17 by Wai Fai Chin
// 
//   This is a drive for internal CPU ADC module. It configures its input selection,
// conversion speed and sensor filter algorithm based on the sensor descriptor
// from sensor module. It does not know what type of sensor hooked up to it.
// It just know how to read and supply ADC data to the specified sensor object.
//
// ****************************************************************************
// 2013-06-19 -WFC- ported to ATXMEGA192D3 for ScaleCore3.
//

/*!
   We are use the internal CPU ADC module.
*/

/*!
	Before every read of ADC, it checks the next sensor enable status.
	If it enabled, then switch to the specified channel,
    type ( differential or single-end), conversion speed, and reference selection.

    ScaleCore2 CPU ADC input assignments:
    ADCA module:
    ADC7A pin on channel 0 for VF input voltage.

    ADCB module:
    ADC0B pin on channel 0 for VBAT1
    ADC1B pin on channel 1 for VBAT2
    internal temperature sensor input on channel 2.
*/

#include  "adc_cpu.h"
#include  <math.h>
#include  "sensor.h"
#include  "timer.h"
#include  "dataoutputs.h"
#include  "nv_cnfg_mem.h"		// 2010-11-17 -WFC- for gtProductInfoFNV

#define	ADC_CPU_ADCA_MAX_NUM_CHANNEL	1
#define	ADC_CPU_ADCB_MAX_NUM_CHANNEL	3

/// timer for read ADC
TIMER_T gAdc_cpu_readTimer;

BYTE gbCPU_ADCA_offset;
// BYTE gbCPU_ADCB_offset;
BYTE gbCPU_ADCA_CurChannel;		// 2013-06-20 -WFC- current scanning channel of CPU ADC
BYTE gbCPU_ADCA_numSample;		// 2013-06-20 -WFC- number of sample of current scanning channel of CPU ADC.

/// An array of ADC operation descriptor; This is private for this module.
ADC_CPU_OP_DESC_T	gaAdc_cpu_op_desc_a[ ADC_CPU_MAX_CHANNEL ];
ADC_CPU_OP_DESC_T	gaAdc_cpu_op_desc_b[ ADC_CPU_MAX_CHANNEL ];

// private function
// 2013-06-26 -WFC- void  adc_cpu_update_registered_sensor( BYTE	sensorN, UINT16 wADC );
void  adc_cpu_update_registered_sensor( BYTE	sensorN, UINT16 wADC, BYTE isNeg ); // 2013-06-26 -WFC-

/**
 * initialize CPU ADC and its filter adapter supplied by other sensor
 * module such as battery, temperature etc...
 *
 * @return none
 * @note must initialized sensor module and has valid initialized gaLSensorDescriptor[]
 *       before call this procedure.
 *
 * History:  Created on 2008/12/17 by Wai Fai Chin
 */

void  adc_cpu_init( void )
{
	BYTE i;
	
	timer_mSec_set( &gAdc_cpu_readTimer, TT_500mSEC);		// read the CPU ADC every half seconds.

	for ( i=0; i< ADC_CPU_MAX_CHANNEL; i++) {
		gaAdc_cpu_op_desc_b[i].sensorN = gaAdc_cpu_op_desc_a[i].sensorN = SENSOR_NO_SUCH_ID;	// assumed no sensor attached to this ADC.
		gaAdc_cpu_op_desc_b[i].status  = gaAdc_cpu_op_desc_a[i].status  = 0;					// assumed all channels are disabled.
	}
	
	for ( i= 0; i < MAX_NUM_LSENSORS; i++) {
		adc_cpu_construct_op_desc( &gaLSensorDescriptor[i], i );
		// adc_cpu_setup_channel( i );
	}

    //gbAdc_cpu_cur_channel = 0;
    //adc_cpu_start_conversion( gbAdc_cpu_cur_channel );
} // adc_cpu_init()

/**
 * initialize internal CPU ADC module A and B.
 * It is pre-configured according to the ScaleCore2 schematic.

    ScaleCore2 CPU ADC input assignments:
    ADCA module:
    ADC7A pin on channel 0 for VF input voltage.

    ADCB module:
    ADC0B pin on channel 0 for VBAT1
    ADC1B pin on channel 1 for VBAT2
    internal temperature sensor input on channel 2.
 *
 * @return none
 *
 * History:  Created on 2009/10/23 by Wai Fai Chin
 */


#if (CONFIG_PRODUCT_AS == CONFIG_AS_HD )
void  adc_cpu_1st_init( void )
{
	//-------------------------------------------------------------------------
	// Configure ADCB module
	//-------------------------------------------------------------------------
	// Move stored calibration values to ADC A.
	ADC_CalibrationValues_Load(&ADCA);

	// Set up ADC A to have signed conversion mode and 12 bit resolution.
	// ADC_ConvMode_and_Resolution_Config(&ADCA, ADC_ConvMode_Signed, ADC_RESOLUTION_12BIT_gc);
	// Set up ADC A to have unsigned conversion mode and 12 bit resolution.
	ADC_ConvMode_and_Resolution_Config(&ADCA, ADC_ConvMode_Unsigned, ADC_RESOLUTION_12BIT_gc);

	// Set sample rate to the slowest to have a clean signal.
	ADC_Prescaler_Config(&ADCA, ADC_PRESCALER_DIV512_gc);

	// Set reference voltage on ADC A to the internal 1.0 V.
	ADC_Reference_Config(&ADCA, ADC_REFSEL_INT1V_gc);

	// Setup channel 0 as single end external input.
	ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH0, ADC_CH_INPUTMODE_SINGLEENDED_gc, ADC_DRIVER_CH_GAIN_NONE);

	// Get offset value for ADC A.
	ADC_Ch_InputMux_Config(&ADCA.CH0, ADC_CH_MUXPOS_PIN7_gc, ADC_CH_MUXNEG_PIN7_gc);
	ADC_Enable(&ADCA);
	// Wait until common mode voltage is stable. Default clk is 2MHz and
	// therefore below the maximum frequency to use this function.
	ADC_Wait_32MHz(&ADCA);
	// 2010-09-28 -WFC- gbCPU_ADCA_offset = ADC_Offset_Get_Unsigned(&ADCA, &ADCA.CH0, TRUE);
	gbCPU_ADCA_offset = 200;			// 2010-09-28 -WFC- input voltage influenced the offset value, just hardcode a value.

	ADC_Disable(&ADCA);
	// assinged pin7 to channel 0 in ADCA module.
	ADC_Ch_InputMux_Config(&ADCA.CH0, ADC_CH_MUXPOS_PIN7_gc, 0);
	// Setup sweep just on channel 0 only.
	ADC_SweepChannels_Config(&ADCA, ADC_SWEEP_0_gc);
	// Enable ADC A.
	ADC_Enable(&ADCA);

	//  Wait until common mode voltage is stable. Default clk is 2MHz and
	// therefore below the maximum frequency to use this function.
	ADC_Wait_32MHz(&ADCA);
	// Enable free running mode.
	ADC_FreeRunning_Enable(&ADCA);

	//-------------------------------------------------------------------------
	// Configure ADCB module
	//-------------------------------------------------------------------------
	// Move stored calibration values to ADC B.
	ADC_CalibrationValues_Load(&ADCB);

	// Set up ADC B to have signed conversion mode and 12 bit resolution.
	// ADC_ConvMode_and_Resolution_Config(&ADCB, ADC_ConvMode_Signed, ADC_RESOLUTION_12BIT_gc);
	// Set up ADC B to have unsigned conversion mode and 12 bit resolution.
	ADC_ConvMode_and_Resolution_Config(&ADCB, ADC_ConvMode_Unsigned, ADC_RESOLUTION_12BIT_gc);

	// Set sample rate.
	ADC_Prescaler_Config(&ADCB, ADC_PRESCALER_DIV512_gc);

	// Set reference voltage on ADC B to be VCC/1.6 V. 2.18V refV; assumed ATP and 107 model chopters.
	ADC_Reference_Config(&ADCB, ADC_REFSEL_VCC_gc);

	// Setup channel 0, 1 with single ended external inputs.
	ADC_Ch_InputMode_and_Gain_Config(&ADCB.CH0, ADC_CH_INPUTMODE_SINGLEENDED_gc,  ADC_DRIVER_CH_GAIN_NONE);
	ADC_Ch_InputMode_and_Gain_Config(&ADCB.CH1, ADC_CH_INPUTMODE_SINGLEENDED_gc,  ADC_DRIVER_CH_GAIN_NONE);
	// channel2 set to internal mode for internal temperature sensor.
	ADC_Ch_InputMode_and_Gain_Config(&ADCB.CH2, ADC_CH_INPUTMODE_INTERNAL_gc,  ADC_DRIVER_CH_GAIN_NONE);

	// Get offset value for ADC B.
	ADC_Ch_InputMux_Config(&ADCB.CH0, ADC_CH_MUXPOS_PIN0_gc, ADC_CH_MUXNEG_PIN0_gc);

	ADC_Enable(&ADCB);
	// Wait until common mode voltage is stable. Default clk is 2MHz and
	// therefore below the maximum frequency to use this function.
	ADC_Wait_32MHz(&ADCB);
	// 2010-09-28 -WFC- gbCPU_ADCB_offset = ADC_Offset_Get_Unsigned(&ADCB, &ADCB.CH0, TRUE);
	gbCPU_ADCB_offset = 200;	// 2010-09-28 -WFC- input voltage influenced the offset value, just hardcode a value.
	ADC_Disable(&ADCB);

	// configure inputs to the channels in ADCB module
	ADC_Ch_InputMux_Config(&ADCB.CH0, ADC_CH_MUXPOS_PIN0_gc, 0);
	ADC_Ch_InputMux_Config(&ADCB.CH1, ADC_CH_MUXPOS_PIN1_gc, 0);
	// channel2 seted to internal mode for internal temperature sensor.
	ADC_Ch_InputMux_Config(&ADCB.CH2, ADC_CH_MUXINT_TEMP_gc, 0);

	// Setup sweep of virtual channels from 0 to 2.
	ADC_SweepChannels_Config(&ADCB, ADC_SWEEP_012_gc);

	// enabled internal temperature sensor of ADC B module.
	ADC_TempReference_Enable( &ADCB );

	// Enable ADC B.
	ADC_Enable(&ADCB);
	//  Wait until common mode voltage is stable. Default clk is 2MHz and
	// therefore below the maximum frequency to use this function.
	ADC_Wait_32MHz(&ADCB);

	// Enable free running mode.
	ADC_FreeRunning_Enable(&ADCB);

	// Move this to main_system_normal_init() instead because we have called adc_cpu_1st_init() before sensor_init_all(). The tempearature init needs gbCPU_ADCB_offset.
	// -WFC 2010-07-29 adc_cpu_init();
} // end adc_cpu_1st_init()

#else

// 2010-11-17 -WFC- Set voltage reference of ADCB module based on user defined model.
void  adc_cpu_1st_init( void )
{
	//-------------------------------------------------------------------------
	// Configure ADCA module
	//-------------------------------------------------------------------------

	// Move stored calibration values to ADC A.
	ADC_CalibrationValues_Load(&ADCA);

	// Conversion mode: Signed
	ADCA.CTRLB=(ADCA.CTRLB & (~(ADC_CONMODE_bm | ADC_FREERUN_bm | ADC_RESOLUTION_gm))) |
		ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;

	// Clock frequency: 1.953 kHz
	ADCA.PRESCALER=(ADCA.PRESCALER & (~ADC_PRESCALER_gm)) | ADC_PRESCALER_DIV512_gc;

	// Reference: AREF pin on PORTA
	// Temperature reference: On
	ADCA.REFCTRL=(ADCA.REFCTRL & ((~(ADC_REFSEL_gm | ADC_TEMPREF_bm)) | ADC_BANDGAP_bm)) |
		ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm;

	ADC_Disable(&ADCA);

	// Read and save the ADC offset using channel 0
	// ADC_Ch_InputMode_and_Gain_Config(&ADCA.CH0, ADC_CH_INPUTMODE_DIFF_gc, ADC_CH_GAIN_1X_gc);
//	ADCA.CH0.CTRL=(ADCA.CH0.CTRL & (~(ADC_CH_START_bm | ADC_CH_GAINFAC_gm | ADC_CH_INPUTMODE_gm))) |
//		ADC_CH_GAIN_4X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;  don't work because the definition file has wrong structure offset of MUXCTRL.

	ADCA_CH0_CTRL = ADC_CH_GAIN_4X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
	//ADC_Ch_InputMux_Config(&ADCA.CH0, ADC_CH_MUXPOS_PIN1_gc, ADC_CH_MUXNEG_PIN7_gc);
	// ADCA.CH0.MUXCTRL= ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN7_gc;	don't work because the definition file has wrong structure offset of MUXCTRL.
	ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN7_gc;

	// Enable the ADC in order to read the offset
	ADC_Enable(&ADCA);
	// Insert a delay to allow the ADC common mode voltage to stabilize
	delay_us(2);
	// Perform several offset measurements and store the mean value

	// gbCPU_ADCA_offset = ADC_Offset_Get_Unsigned(&ADCA, &ADCA.CH0, TRUE);
	// gbCPU_ADCA_offset = 200;	 // 2010-09-28 -WFC- input voltage influenced the offset value, just hardcode a value.

	// Initialize the ADC Compare register
	ADCA.CMPL=0x00;
	ADCA.CMPH=0x00;
	ADC_Disable(&ADCA);
	// ADC channel 0 input mode: Differential input signal with gain: 4
	ADCA_CH0_CTRL = ADC_CH_GAIN_4X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
	//ADC_Ch_InputMux_Config(&ADCA.CH0, ADC_CH_MUXPOS_PIN1_gc, ADC_CH_MUXNEG_PIN7_gc);
	// ADCA.CH0.MUXCTRL= ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN7_gc;	don't work because the definition file has wrong structure offset of MUXCTRL.
	ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN7_gc;

	gbCPU_ADCA_CurChannel = 2;
	ADC_SweepChannels_Config(&ADCA, ADC_SWEEP_0_gc);

	// ADC is in Free Running mode
	// Conversions are continuously performed on channel 0
	ADCA_EVCTRL=(ADCA_EVCTRL & (~(ADC_EVSEL_gm | ADC_EVACT_gm))) |
		ADC_EVACT_NONE_gc;

	// Channel 0 interrupt: Low Level
	// Channel 0 interrupt mode: Conversion Complete
	//	ADCA.CH0.INTCTRL=(ADCA.CH0.INTCTRL & (~(ADC_CH_INTMODE_gm | ADC_CH_INTLVL_gm))) |
	//		ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_LO_gc;

	// Channel 0 interrupt: Disabled
	ADCA_CH0_INTCTRL =(ADCA_CH0_INTCTRL  & (~(ADC_CH_INTMODE_gm | ADC_CH_INTLVL_gm))) |
		ADC_CH_INTMODE_COMPLETE_gc | ADC_CH_INTLVL_OFF_gc;

	// Free Running mode: On
	ADC_FreeRunning_Enable(&ADCA);

	// Enable the ADC
	ADC_Enable(&ADCA);
	// Insert a delay to allow the ADC common mode voltage to stabilize
	delay_us(2);

	// Move this to main_system_normal_init() instead because we have called adc_cpu_1st_init() before sensor_init_all(). The temperature init needs gbCPU_ADCB_offset.
	// -WFC 2010-07-29 adc_cpu_init();
} // end adc_cpu_1st_init()

#endif


/**
 * Construct low level CPU ADC commands based on sensor descriptor
 * and initialized the status for track AC excitation state.
 * This function encapsulated all the complexity of low level logics.
 *
 * @param     pSD -- pointer to local sensor descriptor.
 * @param sensorN -- sensor number, range 0 to MAX_NUM_SENSORS-1
 *
 * @return none
 * @post   initialized gaAdcLT_op_desc[ channel ] 
 * 
 * @note  pSD must points to a valid sensor descriptor. This sensor descriptor was initialize by
 *        sensor_init() or host commands. 
 *  
 * History:  Created on 2008/12/17 by Wai Fai Chin
 */

void  adc_cpu_construct_op_desc( LSENSOR_DESCRIPTOR_T *pSD, BYTE sensorN )
{
	BYTE channel;
	BYTE cnfg;
	
	// if this sensor assigned to this chip
	if ( SENSOR_CHIP_CPU_ADC_A == pSD-> chipID ) {
		channel = pSD-> hookup_cnfg & SENSOR_HOOKUP_CHANNEL_MASK;
		cnfg = pSD-> conversion_cnfg;
		if ( channel < ADC_CPU_MAX_CHANNEL ) {
			gaAdc_cpu_op_desc_a[ channel ].sensorN = sensorN;
			gaAdc_cpu_op_desc_a[ channel ].status = (cnfg & ( ADC_STATUS_ENABLED | ADC_STATUS_AC_EXCITE)) | ADC_STATUS_AC_EXCITE_STATE;
			gaAdc_cpu_op_desc_a[ channel ].feature = ( (pSD->cnfg) & SENSOR_CNFG_REF_V_MASK ) << 3;
		}
	} // end if ( SENSOR_CHIP_CPU_ADC == pSD-> chipID ) {}
} // end adc_cpu_construct_op_desc(,)


/**
 * Read CPU'S ADC at a schedule time interval.
 * While it reads the current channel ADC value, it also programs the ADC switching to the new
 * channel or configuration. This function works with none of channel enabled or any one of channel
 * is enabled. The for loop will loop back the current channel if it cannot find a new active
 * channel. It only calls adc_cpu_read(,) if there is an active channel.
 *
 * @param	readItNow	-- read it now if is true, otherwise will be ADC_CPU_INTERVAL_READ dictate by timer.
 * @return none
 * @post  if ADC data ready, it post ADC count in gaLSensorDescriptor[].curADCcount and updated gaLSensorDescriptor[].status.
 *
 * @note
 *
 * History:  Created on 2013-03-19 by Wai Fai Chin
 * 2013-06-19 -WFC- ported to ScaleCore3
 */

#if ( CONFIG_PRODUCT_AS	==  CONFIG_AS_CHALLENGER3 )	|| (CONFIG_PRODUCT_AS == CONFIG_AS_PORTAWEIGH2 )	// 2013-09-27 -WFC- 2014-02-06 -DLM-

void  adc_cpu_update( BYTE readItNow )
{
	BYTE isNeg;
	BYTE sn;					// sensor number
	INT16 wADC;
	ADC_CH_t *pADC_channel;

	pADC_channel = &ADCA.CH0;								// default channel.
	if ( timer_mSec_expired( &gAdc_cpu_readTimer ) || readItNow ) {
		if (  ADC_STATUS_ENABLED & gaAdc_cpu_op_desc_a[ gbCPU_ADCA_CurChannel ].status ) {
			sn = gaAdc_cpu_op_desc_a[ gbCPU_ADCA_CurChannel ].sensorN;
			if ( sn < MAX_NUM_LSENSORS ) {
				if ( (ADCA_CH0_INTFLAGS & ADC_CH_CHIF_bm ))			// if conversion is completed.
					// Clear interrupt flag.
					ADCA_CH0_INTFLAGS = ADC_CH_CHIF_bm;
					wADC = ADCA_CH0_RES;
					gbCPU_ADCA_numSample++;
					if ( gbCPU_ADCA_numSample > 3 ) {
						isNeg = FALSE;
						if ( gbCPU_ADCA_CurChannel < 4 ) {
							if ( wADC >= 0xF800 ) {
								wADC = ~wADC + 1;
								isNeg = TRUE;
							}
						}
						if ( wADC >= gbCPU_ADCA_offset )
							wADC -= gbCPU_ADCA_offset;
						adc_cpu_update_registered_sensor( sn, wADC, isNeg );
					}
				}
		}
		else {
			gbCPU_ADCA_numSample = 255;
		}

		if ( gbCPU_ADCA_numSample > 3) {
			gbCPU_ADCA_CurChannel++;
			if ( gbCPU_ADCA_CurChannel > (ADC_CPU_MAX_CHANNEL - 1))
				gbCPU_ADCA_CurChannel = 0;
			ADCA.CMPL=0x00;
			ADCA.CMPH=0x00;
			ADC_Disable(&ADCA);
			switch (gbCPU_ADCA_CurChannel) {
			case 0:
				ADCA.CTRLB = ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN7_gc;
				break;
			case 1:
				ADCA.CTRLB = ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN2_gc | ADC_CH_MUXNEG_PIN7_gc;
				break;
			case 2:
				ADCA.CTRLB = ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN3_gc | ADC_CH_MUXNEG_PIN7_gc;
				break;
			case 3:		// light sensor in the Challenger3
				ADCA.CTRLB = ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN4_gc | ADC_CH_MUXNEG_PIN7_gc;
				break;
			case 4:		// internal temperature sensor
				ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_INT1V_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_INTERNAL_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXINT_TEMP_gc;
				break;
			}
			ADC_FreeRunning_Enable(&ADCA);
			ADC_Enable(&ADCA);
			ADCA_CH0_CTRL |= ADC_CH_START_bm;
			gbCPU_ADCA_numSample = 0;
		}
		timer_mSec_set( &gAdc_cpu_readTimer, TT_50mSEC);
	}// end

} // end adc_cpu_update()
#elif ( CONFIG_PRODUCT_AS	==  CONFIG_AS_DYNA_LINK_II || CONFIG_PRODUCT_AS	==  CONFIG_AS_REMOTE_METER  )		// 2013-09-27 -WFC-
void  adc_cpu_update( BYTE readItNow )
{
	BYTE isNeg;
	BYTE sn;					// sensor number
	INT16 wADC;
	ADC_CH_t *pADC_channel;

	pADC_channel = &ADCA.CH0;								// default channel.
	if ( timer_mSec_expired( &gAdc_cpu_readTimer ) || readItNow ) {
		if (  ADC_STATUS_ENABLED & gaAdc_cpu_op_desc_a[ gbCPU_ADCA_CurChannel ].status ) {
			sn = gaAdc_cpu_op_desc_a[ gbCPU_ADCA_CurChannel ].sensorN;
			if ( sn < MAX_NUM_LSENSORS ) {
				if ( (ADCA_CH0_INTFLAGS & ADC_CH_CHIF_bm ))			// if conversion is completed.
					// Clear interrupt flag.
					ADCA_CH0_INTFLAGS = ADC_CH_CHIF_bm;
					wADC = ADCA_CH0_RES;
					gbCPU_ADCA_numSample++;
					if ( gbCPU_ADCA_numSample > 3 ) {
						isNeg = FALSE;
						if ( gbCPU_ADCA_CurChannel < 4 ) {
							if ( wADC >= 0xF800 ) {
								wADC = ~wADC + 1;
								isNeg = TRUE;
							}
						}
						if ( wADC >= gbCPU_ADCA_offset )
							wADC -= gbCPU_ADCA_offset;
						adc_cpu_update_registered_sensor( sn, wADC, isNeg );
					}
				}
		}
		else {
			gbCPU_ADCA_numSample = 255;
		}

		if ( gbCPU_ADCA_numSample > 3) {
			gbCPU_ADCA_CurChannel++;
			if ( gbCPU_ADCA_CurChannel > (ADC_CPU_MAX_CHANNEL - 1))
				gbCPU_ADCA_CurChannel = 0;
			ADCA.CMPL=0x00;
			ADCA.CMPH=0x00;
			ADC_Disable(&ADCA);
			switch (gbCPU_ADCA_CurChannel) {
			case 0:
				ADCA.CTRLB = ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN1_gc | ADC_CH_MUXNEG_PIN7_gc;
				break;
			case 1:
				ADCA.CTRLB = ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN2_gc | ADC_CH_MUXNEG_PIN7_gc;
				break;
			case 2:
				ADCA.CTRLB = ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN3_gc | ADC_CH_MUXNEG_PIN7_gc;
				break;
			case 3:		// light sensor in the Challenger3
				ADCA.CTRLB = ADC_CONMODE_bm | ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_AREFA_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAIN_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXPOS_PIN4_gc | ADC_CH_MUXNEG_PIN7_gc;
				break;
			case 4:		// internal temperature sensor
				ADCA.CTRLB = ADC_RESOLUTION_12BIT_gc;
				ADCA.REFCTRL = ADC_REFSEL_INT1V_gc | ADC_TEMPREF_bm;
				ADCA_CH0_CTRL = ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_INTERNAL_gc;
				ADCA_CH0_MUXCTRL = ADC_CH_MUXINT_TEMP_gc;
				break;
			}
			ADC_FreeRunning_Enable(&ADCA);
			ADC_Enable(&ADCA);
			ADCA_CH0_CTRL |= ADC_CH_START_bm;
			gbCPU_ADCA_numSample = 0;
		}
		timer_mSec_set( &gAdc_cpu_readTimer, TT_50mSEC);
	}// end

} // end adc_cpu_update()

#endif

/*
void  adc_cpu_update( BYTE readItNow )
{
	BYTE n;
	BYTE sn;					// sensor number
	UINT16 wADC;
	ADC_CH_t *pADC_channel;

	pADC_channel = &ADCA.CH0;								// default channel.
	if ( timer_mSec_expired( &gAdc_cpu_readTimer ) || readItNow ) {
		for ( n =0; n < ADC_CPU_ADCA_MAX_NUM_CHANNEL; n++) {
			switch ( n ) {
				case 0:	pADC_channel = &ADCA.CH0;
						break;
				case 1:	pADC_channel = &ADCA.CH1;
						break;
				case 2:	pADC_channel = &ADCA.CH2;
						break;
			} // end switch(n)

			if (  ADC_STATUS_ENABLED & gaAdc_cpu_op_desc_a[ n ].status ) {
				sn = gaAdc_cpu_op_desc_a[ n ].sensorN;
				if ( sn < MAX_NUM_LSENSORS ) {
					if (ADC_Ch_Conversion_Complete(pADC_channel)) {
						wADC = ADC_ResultCh_GetWord_Unsigned(pADC_channel, gbCPU_ADCA_offset);
						adc_cpu_update_registered_sensor( sn, wADC );
					}
				}
			}
		} // end for;
		timer_mSec_set( &gAdc_cpu_readTimer, TT_500mSEC);
	}// end

} // end adc_cpu_update()
 */

/* For HD display board.
void  adc_cpu_update( void )
{
	BYTE n;
	BYTE sn;					// sensor number
	UINT16 wADC;
	ADC_CH_t *pADC_A_channel;
	ADC_CH_t *pADC_B_channel;

	if ( timer_mSec_expired( &gAdc_cpu_readTimer ) ) {
		for ( n =0; n < ADC_CPU_MAX_CHANNEL; n++) {
			switch ( n ) {
				case 0:	pADC_A_channel = &ADCA.CH0;
						pADC_B_channel = &ADCB.CH0;
						break;
				case 1:	pADC_A_channel = &ADCA.CH1;
						pADC_B_channel = &ADCB.CH1;
						break;
				case 2:	pADC_A_channel = &ADCA.CH2;
						pADC_B_channel = &ADCB.CH2;
						break;
				case 3:	pADC_A_channel = &ADCA.CH3;
						pADC_B_channel = &ADCB.CH3;
						break;
			} // end switch(n)

			if (  ADC_STATUS_ENABLED & gaAdc_cpu_op_desc_a[ n ].status ) {
				sn = gaAdc_cpu_op_desc_a[ n ].sensorN;
				if ( sn < MAX_NUM_LSENSORS ) {
					if (ADC_Ch_Conversion_Complete(pADC_A_channel)) {
						wADC = ADC_ResultCh_GetWord_Unsigned(pADC_A_channel, gbCPU_ADCA_offset);
						adc_cpu_update_registered_sensor( sn, wADC );
					}
				}
			}

			if (  ADC_STATUS_ENABLED & gaAdc_cpu_op_desc_b[ n ].status ) {
				sn = gaAdc_cpu_op_desc_b[ n ].sensorN;
				if ( sn < MAX_NUM_LSENSORS ) {
					if (ADC_Ch_Conversion_Complete(pADC_B_channel)) {
						wADC = ADC_ResultCh_GetWord_Unsigned(pADC_B_channel, gbCPU_ADCB_offset);
						adc_cpu_update_registered_sensor( sn, wADC );
					}
				}
			}

		} // end for;
		timer_mSec_set( &gAdc_cpu_readTimer, TT_500mSec);
	}// end

} // end adc_cpu_update()

 */

/**
 * read and process raw ADC count and configure ADC chip while reading ADC chip.
 * 
 * @param  sensorN  -- sensor number
 * @param  wADC		-- ADC counts.
 * @param  isNeg	-- true if wADC is negative.
 *
 * @return none
 * @post   gaLSensorDescriptor[].curADCcount and updated gaLSensorDescriptor[].status.
 * @note caller ensured that the ADC had already completed a conversion.
 * 
 * History:  Created on 2008/12/23 by Wai Fai Chin
 *  2013-06-26 -WFC- modified to handle both signed and unsigned ADC value.
 */
void  adc_cpu_update_registered_sensor( BYTE	sensorN, UINT16 wADC, BYTE isNeg )
{
	LSENSOR_DESCRIPTOR_T		*pSnDesc;	// points to local sensor descriptor
	GENERIC_UNION u;

	u.i32v = 0;							// clear variable
	u.ui16v = wADC;						// read the current channel ADC value.
	if ( isNeg )
		u.i32v = -u.i32v;

	if ( sensorN < MAX_NUM_LSENSORS ) {
		pSnDesc = &gaLSensorDescriptor[ sensorN ];
		if ( sensorN < MAX_NUM_LSENSORS ) {
			pSnDesc->rawAdcData = u.i32v;		// raw ADC data included status of the chip
		}

		pSnDesc->prvRawADCcount = pSnDesc->curRawADCcount;
		pSnDesc->curRawADCcount = u.i32v;

		pSnDesc->prvADCcount = pSnDesc->curADCcount;
		pSnDesc->curADCcount = u.i32v;
		pSnDesc->status |= SENSOR_STATUS_GOT_ADC_CNT;	// flag that this sensor has new value.
	} // end if ( sensorN < MAX_NUM_LSENSORS ) {}

}// end adc_cpu_update_registered_sensor();
/*
void  adc_cpu_update_registered_sensor( BYTE	sensorN, UINT16 wADC )
{
	LSENSOR_DESCRIPTOR_T		*pSnDesc;	// points to local sensor descriptor
	GENERIC_UNION u;

	u.i32v = 0;							// clear variable
	u.ui16v = wADC;						// read the current channel ADC value.

	if ( sensorN < MAX_NUM_LSENSORS ) {
		pSnDesc = &gaLSensorDescriptor[ sensorN ];
		if ( sensorN < MAX_NUM_LSENSORS ) {
			pSnDesc->rawAdcData = u.i32v;		// raw ADC data included status of the chip
		}

		pSnDesc->prvRawADCcount = pSnDesc->curRawADCcount;
		pSnDesc->curRawADCcount = u.i32v;

		pSnDesc->prvADCcount = pSnDesc->curADCcount;
		pSnDesc->curADCcount = u.i32v;
		pSnDesc->status |= SENSOR_STATUS_GOT_ADC_CNT;	// flag that this sensor has new value.
	} // end if ( sensorN < MAX_NUM_LSENSORS ) {}

}// end adc_cpu_update_registered_sensor();
*/

/**
 * It setup ADC's refernce voltage, sampling speed etc..
 * Current version only configure reference voltage.
 *
 * @param  sn  -- sensor number
 *
 * @return none
 *
 * @note
 * Usage:
 *    gaLSensorDescriptor[sn].status &= 0xFC;		// 1.0 V ref V.
 *    gaLSensorDescriptor[sn].status &= 0xFD;		// Vcc /1.6 V ==> ref V.
 *    adc_cpu_setup_channel( sn );
 *
 * History:  Created on 2010-08-23 by Wai Fai Chin
 * 2013-06-14 -WFC- modified for ScaleCore3.
 */

void  adc_cpu_setup_channel( BYTE sn )
{
	ADC_Reference_Config( &ADCA, ADC_REFSEL_AREFA_gc );
} // end adc_cpu_setup_channel()


/**
 * It setup ADC's refernce voltage, sampling speed etc..
 * Current version only configure reference voltage.
 *
 * @param  sn  -- sensor number
 *
 * @return none
 *
 * @note
 * Usage:
 *    gaLSensorDescriptor[sn].status &= 0xFC;		// 1.0 V ref V.
 *    gaLSensorDescriptor[sn].status &= 0xFD;		// Vcc /1.6 V ==> ref V.
 *    adc_cpu_setup_channel( sn );
 *
 * History:  Created on 2010-08-23 by Wai Fai Chin
 */
/*
void  adc_cpu_setup_channel( BYTE sn )
{
	BYTE refV;
	ADC_t *pADC;

	// Only allow VBAT1, to be auto range because CPU ADC only allow has global reference for all channels. Thus, once VBAT1 auto range, so does VBAT2
	if ( SENSOR_NUM_1ST_VOLTAGEMON == sn ) {
		refV = gaLSensorDescriptor[sn].status & 0x03;
		// pADC = &ADCB;
		pADC = &ADCA;

		if ( SENSOR_CHIP_CPU_ADC_B == gaLSensorDescriptor[sn].chipID ) {
			pADC = &ADCB;
		}

//		if ( SENSOR_CHIP_CPU_ADC_A == gaLSensorDescriptor[sn].chipID ) {
//					pADC = &ADCA;
//		}

		if ( ADC_CPU_SENSOR_STATUS_REFV_1V == refV )
			ADC_Reference_Config(	pADC, ADC_REFSEL_INT1V_gc );
		else
			// Set reference voltage on ADC module to be VCC/1.6 V.
			ADC_Reference_Config(	pADC, ADC_REFSEL_VCC_gc );
	}
} // end adc_cpu_setup_channel()
*/
