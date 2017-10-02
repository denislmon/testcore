/*! \file adc_lt.c \brief external ADC related functions.*/
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
// Software layer:  Hardware
//
//  History:  Created on 2007/09/10 by Wai Fai Chin
// 
//   This is a drive for external ADC chip. It configures its input selection,
// conversion speed and sensor filter algorithm based on the sensor descriptor
// from sensor module. It does not know what type of sensor hooked up to it.
// It just know how to read and supply ADC data to the specified sensor object.
//
// ****************************************************************************
// -WFC- 2011-03-28 Added logic to keep track of Peak hold ADC count.

/*!
   We use the Linear Tech LTC2447 delta sigma ADC chip. Its SPI sck can run at maximum of 20MHz.
   I found that polling on ADBUSY working in conjunction with a none delay main task loop is the best approached and it
   is the most reliable method without corrupt the final ADC result during running the filter function.
   It only took 50~60uSec to read four bytes of data with SPI at 1,843,200 Hz. Thus, I decided to just use
   polling mode for the UART_SPI on both xmitting and receiving. SPI interrupt mode would be more complex and
   overhead execution and memory size.


    input command format:
	
	ctrl2 ctrl1 ctrl0 SGL SIGN GLBL A1 A0       OSR3 OSR2 OSR1 OSR0  TwoX x x x
	
	If control bits = 101, then following 5 bits select the input channel/reference
    for the following conversion. The next 5 bits select the speed/resolution and
    mode 1x (no Latency) 2x (double output rate with one conversion latency).
	
	If control bits = 100 or 000, the following input data is ignored (don't care)
    and the previously selected speed/resolution, channel and reference remain
	valid for the next conversion.
	
	If SGL=0, input selection is differential.
	If SGL=1, input selection is single-ended.

    If GLBL=0 selects of channels has a corresponding differential input reference.
	If GLBL = 1, a global reference VREFG+/VREFG� is selected.
    The global reference input may be used for any input channel selected.
	
	(SIGN, A1, A0) determine which channel is selected.
	
	(OSR3, OSR2, OSR1, OSR0), (0000) Keep Previous conversion Speed/Resolution.
	(0001) is 3.52KHz with 23uV RMS noise. (1111) is 6.875Hz with 200nV RMS noise.
	
	When TwoX=1, it will double the above output rate with one conversion latency
    and with the same noise performance.
	
	Output format:
	Bit 31 (first output bit) is the end of conversion (/EOC) indicator.
		This bit is HIGH during the conversion and goes LOW when the conversion is complete.

	Bit 30 (second output bit) is a dummy bit (DMY) and is always LOW.
	Bit 29 (third output bit) is the conversion result sign indicator (SIG).
			If VIN is >0, this bit is HIGH. If VIN is <0, this bit is LOW.
	Bit 28 (fourth output bit) is the most significant bit (MSB) of the result.
		This bit in conjunction with Bit 29 also provides the underrange or overrange indication.
		If both Bit 29 and Bit 28 are HIGH, the differential input voltage is above +FS.
		If both Bit 29 and Bit 28 are LOW, the differential input voltage is below �FS.
	
	Bits 28-5 are the 24-bit conversion result MSB first.
	Bit 5 is the least significant bit (LSB).
	Bits 4-0 are sub LSBs below the 24-bit level.
    Bits 4-0 may be included in averaging or discarded without loss of resolution.
	
*/

/*!
	Before every read of ADC, it checks the next sensor enable status.
	If it enabled, then switch to the specified channel,
    type ( differential or single-end), conversion speed, and reference selection.
*/

#include  "spi.h"
#include  "adc_lt.h"
#include  <math.h>
#include  "sensor.h"
#include  "timer.h"
#include  "dataoutputs.h"
#include  "loadcellfilter.h"
#include  "bios.h"
#include  "nv_cnfg_mem.h"			// 2011-03-29 -WFC-

// private functions:
BYTE  adc_lt_get_next_enabled_channel( BYTE curChannel );	// -WFC- 2011-03-19

#if ( TRUE == CONFIG_ADC_SCHEDULE_MODE )
							/// timer for read ADC 
	TIMER_T gAdcReadTimer;
#else 
	#if ( CONFIG_ADC_POLL_MODE == FALSE )
		BYTE	gIsLTADC_data_ready;
	#endif
#endif

							/// current active channel of Linear Tech ADC.
BYTE	gbAdcLT_cur_channel;


/// An array of ADC operation descriptor; This is private for this module.
ADC_LT_OP_DESC_T	gaAdcLT_op_desc[ ADC_LT_MAX_CHANNEL ];

/**
 * It initialized ADC.
 *
 * @return none
 *
 * History:  Created on 2007/02/09 by Wai Fai Chin
 */

#if ( CONFIG_EMULATE_ADC == TRUE )

void	adc_lt_1st_init( void )	{}

void	adc_lt_init( void )	{}

void  	adc_lt_read( BYTE curChannel, BYTE nextChannel ) {}

INT32	adc_lt_read_spi( BYTE cmd0, BYTE cmd1 ) {}

void	adc_lt_update( void ) {}


#else
// Real hardware ADC:
void  adc_lt_1st_init( void )
{
	adc_lt_init();
} // adc_lt_1st_init()


/**
 * initialize Linear Tech ADC chip and its filter adapter supplied by other sensor
 * module such as loadcell, temperature etc...
 *
 * @return none
 * @note must initialized sensor module and has valid initialized gaLSensorDescriptor[]
 *       before call this procedure.
 *
 * History:  Created on 2007/09/12 by Wai Fai Chin
 * 2011-03-19 -WFC- activated first found enabled channel first instead of default channel 0.
 * 2013-10-25 -WFC- ported to ScaleCore3.
 */

void  adc_lt_init( void )
{
	BYTE i;
	
	#if ( CONFIG_ADC_SCHEDULE_MODE == TRUE )
		timer_mSec_set( &gAdcReadTimer, gabListenerIntervalFNV[ 0 ]);
	#else 
		#if ( CONFIG_ADC_POLL_MODE == FALSE )
			gIsLTADC_data_ready = FALSE;
		#endif
	#endif

	for ( i=0; i< ADC_LT_MAX_CHANNEL; i++) {
		gaAdcLT_op_desc[i].sensorN = SENSOR_NO_SUCH_ID;	// assumed no sensor attached to this ADC.
		gaAdcLT_op_desc[i].status  = 0;					// assumed all channels are disabled.
	}
	
	for ( i= 0; i < MAX_NUM_LSENSORS; i++) {
		adc_lt_construct_op_desc( &gaLSensorDescriptor[i], i );
	}

	ADC_LT_ENABLE_AC_EXCITATION;
	#if(  CONFIG_USE_CPU != CONFIG_USE_ATXMEGA192D3 )		// 2013-10-25 -WFC-
	ADC_LT_AC_EXCITATION_POS;
	#endif
	
	gbAdcLT_cur_channel = i = adc_lt_get_next_enabled_channel( 0 );
	if ( i < ADC_LT_MAX_CHANNEL )
		adc_lt_read_spi( gaAdcLT_op_desc[i].cmd0, gaAdcLT_op_desc[i].cmd1);

} // adc_lt_init()

/**
 * Construct low level Linear Tech ADC commands based on sensor descriptor
 * and initialized the status for track AC excitation state.
 * This function encapsulated all the complexity of low level logics.
 *
 * @param     pSD -- pointer to sensor descriptor.
 * @param sensorN -- sensor number, range 0 to MAX_NUM_SENSORS-1
 *
 * @return none
 * @post   initialized gaAdcLT_op_desc[ channel ] 
 * 
 * @note  pSD must points to a valid sensor descriptor. This sensor descriptor was initialize by
 *        sensor_init() or host commands. 
 *  
 * History:  Created on 2007-09-12 by Wai Fai Chin
 * 2011-03-29 -WFC- set sample speed for peak hold mode.
 * 2011-05-04 -WFC- Added filterTimer setting based on software filter level.
 */

void  adc_lt_construct_op_desc( LSENSOR_DESCRIPTOR_T *pSD, BYTE sensorN )
{
	BYTE cmd;
	BYTE channel;
	BYTE cnfg;
	LOADCELL_T 				*pLc;			// points to a loadcell
	
	// if this sensor assinged to this chip
	if ( SENSOR_CHIP_ID_ADC_LTC2447 == pSD-> chipID ) {
		cnfg = pSD-> hookup_cnfg;
		cmd = ADC_LT_CMD_ENABLED;
		
		channel = cnfg & SENSOR_HOOKUP_CHANNEL_MASK;
		if ( cnfg & SENSOR_HOOKUP_SINGLE_END ) {		// if it is single-end input
			cnfg = channel;								// cnfg is now use as channel.
			cnfg >>=1;									// Linear Tech ADC channel format
			if ( channel & 1 ) {						// if channel number is odd.
				cnfg |= ADC_LT_ODD_CHANNEL_BIT;
			}
			cmd |= (ADC_LT_SINGLE_END | cnfg);
		}
		else { // differential inputs
			channel &= 3;								// ensured that it will not greater than 3
			cmd |= channel;
		}

		if ( pSD-> cnfg & SENSOR_FEATURE_GLOBAL_REF_V )
			cmd |= ADC_LT_GLOBAL_REF_V;

		cnfg = pSD-> conversion_cnfg;
		
		gaAdcLT_op_desc[ channel ].sensorN = sensorN;
		gaAdcLT_op_desc[ channel ].cmd0 = cmd;
		// 2011-03-29 -WFC- v
		if ( SENSOR_TYPE_LOADCELL == pSD->type ) {
			pLc = (LOADCELL_T *) pSD-> pDev;
			if ( pLc-> runModes & LC_RUN_MODE_PEAK_HOLD_ENABLED )
				gaAdcLT_op_desc[ channel ].cmd1 = gtSystemFeatureFNV.peakholdSampleSpeed<<4;	// conversion speed is in the upper 4bits of cmd1 of the ADC.
			else
				gaAdcLT_op_desc[ channel ].cmd1 = cnfg<<4;	// conversion speed is in the upper 4bits of cmd1 of the ADC.
		}
		else
			gaAdcLT_op_desc[ channel ].cmd1 = cnfg<<4;		// conversion speed is in the upper 4bits of cmd1 of the ADC.
		// 2011-03-29 -WFC- ^
														// assumed initial excitation state as postive cycle		
		gaAdcLT_op_desc[ channel ].status = (cnfg & ( ADC_STATUS_ENABLED | ADC_STATUS_AC_EXCITE)) | ADC_STATUS_AC_EXCITE_STATE;
		sensor_set_filter_timer( pSD );		// 2011-05-04 -WFC-
	} // end if ( SENSOR_CHIP_ID_ADC_LTC2447 == pSD-> chipID ) {}
} // end adc_lt_construct_op_desc(,)

/**
 * It returns next enabled channel based on a given current channel.
 *
 * @param curChannel -- current channel
 *
 * @return next enabled channel. 0-254. 255 == ALL channels are disabled.
 *
 * History:  Created on 2011-03-19 by Wai Fai Chin
 */

BYTE  adc_lt_get_next_enabled_channel( BYTE curChannel )
{
	BYTE c;
	BYTE nextChannel;

	nextChannel = curChannel;
	// The for loop will loop back the current channel if it cannot find a new active channel.
	for ( c =0; c < ADC_LT_MAX_CHANNEL; c++) {
		nextChannel = ++nextChannel & ( ADC_LT_MAX_CHANNEL - 1);
		if (  ADC_STATUS_ENABLED & gaAdcLT_op_desc[ nextChannel ].status )
			break;
	}
	if ( c >= ADC_LT_MAX_CHANNEL ) 	{						// if it cannot found any enabled channel.
		nextChannel = 0xFF;
	}
	return nextChannel;
} // adc_lt_get_next_enabled_channel()


#if ( CONFIG_ADC_SCHEDULE_MODE == TRUE )
/**
 * Read Linear Tech ADC at a schedule time interval.
 * While it reads the current channel ADC value, it also programs the ADC switching to the new
 * channel or configuration. This function works with none of channel enabled or any one of channel
 * is enabled. The for loop will loop back the current channel if it cannot find a new active
 * channel. It only calls adc_lt_read(,) if there is an active channel.
 *
 * @return none
 * @post  if ADC data ready, it post ADC count in gaLSensorDescriptor[].curADCcount and updated gaLSensorDescriptor[].status.
 *
 * @note
 * I found that polling on ADBUSY working in conjunction with a none delay main task loop is the best approached and it
 * is the most reliabled method without corrupt the final ADC result during running the filter function.
 * It only took 50~60uSec to read four bytes of data with SPI at 1,843,200 Hz. Thus, I decided to just use
 * polling mode for the UART_SPI on both xmitting and receiving. SPI interrupt mode would be more complex and
 * overhead excuction and memory size.
 *
 * History:  Created on 2007/09/12 by Wai Fai Chin
 */

void  adc_lt_update( void )
{
	BYTE p;
	BYTE nextChannel;

	if ( timer_mSec_expired( &gAdcReadTimer ) ) {
		nextChannel = gbAdcLT_cur_channel;
		// The for loop will loop back the current channel if it cannot find a new active channel.
		for ( p =0; p < ADC_LT_MAX_CHANNEL; p++) {
			nextChannel = ++nextChannel & ( ADC_LT_MAX_CHANNEL - 1);
			if (  ADC_STATUS_ENABLED & gaAdcLT_op_desc[ nextChannel ].status )
				break;
		}
		if ( p < ADC_LT_MAX_CHANNEL ) 							// if it found an active channel.
			adc_lt_read( gbAdcLT_cur_channel, nextChannel);	// read ADC count.
		gbAdcLT_cur_channel = nextChannel;
		timer_mSec_set( &gAdcReadTimer, gabListenerIntervalFNV[ 0 ]);
	}
} // end adc_lt_update()

#else // quasi-interrupt-poll mode

#if ( CONFIG_ADC_POLL_MODE == TRUE )
/**
 * Update and read Linear Tech ADC value when an ADBUSY signal is low ( indicated data ready).
 * While it reads the current channel ADC value, it also programs the ADC switching to the new
 * channel or configuration. This function works with none of channel enabled or any one of channel
 * is enabled. The for loop will loop back the current channel if it cannot find a new active
 * channel. It only calls adc_lt_read(,) if there is an active channel.
 *
 * @return none
 * @post  if ADC data ready, it post ADC count in gaLSensorDescriptor[].curADCcount and updated gaLSensorDescriptor[].status.
 *
 * History:  Created on 2007/09/21 by Wai Fai Chin
 * 2011-03-19 -WFC- simplify next channel logic with new function adc_lt_get_next_enabled_channel();
 * 2011-04-22 -WFC- removed BIOS_SET_RCAL_ON and BIOS_SET_RCAL_OFF because I use ADC count difference between 0 weight and 1000 (LB/KG) as Rcal value, unit does not matter.
 */

#if ( CONFIG_PCB_AS == CONFIG_PCB_AS_SCALECORE2)
void  adc_lt_update( void )
{
	BYTE p;
	BYTE nextChannel;
	
	p = ADC_LT_CTRL_PORT_IN;
	if ( !(p & ADC_LT_CTRL_ADBUSY) ) { 									// if ADBUSY is low, data is ready.
		nextChannel = adc_lt_get_next_enabled_channel( gbAdcLT_cur_channel );
		if ( nextChannel < ADC_LT_MAX_CHANNEL ) {								// if found a next enabled channel.
			adc_lt_read( gbAdcLT_cur_channel, nextChannel);						// read current channel ADC count and activated next channel.
			gbAdcLT_cur_channel = nextChannel;									// nextChannel data will be available on the next pass.
		}
	}
} // adc_lt_update()
#else
void  adc_lt_update( void )
{
	BYTE p;
	BYTE sensorN;
	BYTE nextChannel;

	p = ADC_LT_CTRL_PORT_IN;
	if ( !(p & ADC_LT_CTRL_ADBUSY) ) { 									// if ADBUSY is low, data is ready.
		nextChannel = adc_lt_get_next_enabled_channel( gbAdcLT_cur_channel );
		if ( nextChannel < ADC_LT_MAX_CHANNEL ) {								// if found a next enabled channel.
			adc_lt_read( gbAdcLT_cur_channel, nextChannel);						// read current channel ADC count and activated next channel.
			sensorN = gaAdcLT_op_desc[ nextChannel ].sensorN;
			// 2011-04-22 -WFC- No more old Rcal stuff. v
//			if ( sensorN < 	MAX_NUM_RCAL_LOADCELL ) {
//				if ( gabLoadcellRcalEnabled[ sensorN ] )
//					BIOS_SET_RCAL_ON;
//				else
//					BIOS_SET_RCAL_OFF;
//			}
			// 2011-04-22 -WFC- ^
			gbAdcLT_cur_channel = nextChannel;									// nextChannel data will be available on the next pass.
		}
	}
} // adc_lt_update()
#endif

#else  // quasi-interrupt-mode 
/**
 * Update and read Linear Tech ADC value when an
 * ADBUSY signal falling edge triggered an interrupt.
 *
 * ADBUSY interrupt routine flagged that the ADC had completed conversion and ready for reading.
 * The main task loop call adc_lt_update() checks for ADC data ready flag.
 * If the ADC data is ready, it will update ADC configuration and read it value.
 * It calls the attached sensor filter algorithm if any of each channel.
 *
 * While it reads the current channel ADC value, it also programs the ADC switching to the new
 * channel or configuration. This function works with none of channel enabled or any one of channel
 * is enabled. The for loop will loop back the current channel if it cannot find a new active
 * channel. It only calls adc_lt_read(,) if there is an active channel.
 *
 * @return none
 * @post  if ADC data ready, it post ADC count in gaLSensorDescriptor[].curADCcount and updated gaLSensorDescriptor[].status.
 *
 * History:  Created on 2007/09/21 by Wai Fai Chin
 */

void  adc_lt_update( void )
{
	BYTE p;
	BYTE nextChannel;

	if ( gIsLTADC_data_ready ) {
		gIsLTADC_data_ready = FALSE;			// clear it for next interrupt

		nextChannel = gbAdcLT_cur_channel;
		// The for loop will loop back the current channel if it cannot find a new active channel.
		for ( p =0; p < ADC_LT_MAX_CHANNEL; p++) {
			nextChannel = ++nextChannel & ( ADC_LT_MAX_CHANNEL - 1);
			if (  ADC_STATUS_ENABLED & gaAdcLT_op_desc[ nextChannel ].status )
				break;
		}
		if ( p < ADC_LT_MAX_CHANNEL ) 							// if it found an active channel.
			adc_lt_read( gbAdcLT_cur_channel, nextChannel);	// read ADC count.
		gbAdcLT_cur_channel = nextChannel;

		EIFR =0x10;					// clear any residue pending interrupt from INT4.
		EIMSK |= 0x10;				// re-enable INT4 of ADBUSY line.
	}
} // adc_lt_update()


/**
 * Interrupt routine to handle ADBUSY low level of Linear Tech ADC data ready.
 *
 * @param  none
 * 
 * @return none
 * @post
 *   The following global variable had been updated.
 *	gIsLTADC_data_ready
 *
 * Dependencies:
 *      vector by external interrupt 4.
 *   bios_extern_interrupts_init(); adc_lt_update();
 *
 * Created on 2007/09/21 by Wai Fai Chin
 */

//ISR(INT4_vect)  // Doxygen cannot handle ISR() macro.
void INT4_vect (void) __attribute__ ((signal,__INTR_ATTRS));
void INT4_vect (void)
{
	gIsLTADC_data_ready = TRUE;
	EIMSK &= 0xEF;				// disabled INT4, it will re-enable in adc_lt_update();
} // end INT4_vect()

#endif  // ( CONFIG_ADC_POLL_MODE == TRUE )

#endif	// ( CONFIG_ADC_SCHEDULE_MODE == TRUE )

/**
 * read and process raw ADC count and configure ADC chip while reading ADC chip.
 * 
 * @param  curChannel  -- current active channel of the ADC chip.
 * @param  nextChannel -- next channel for this ADC chip.
 * @return none
 * @post   gaLSensorDescriptor[].curADCcount and updated gaLSensorDescriptor[].status.
 *
 * History:  Created on 2007/09/12 by Wai Fai Chin
 * 2011-05-04 -WFC- When software filter enabled, it requires spent at least filterTimer amount of time before declared as got a good ADC count.
 * 2011-05-09 -WFC- Bypass software filter if sensor is in peak hold mode.
 * 2011-05-09 -WFC- when sensor is in cal mode, it uses lc_stepwise_triangle_filter() to filter ADC data regardless of software filter setting.
 */

#define ADC_LT_SENOSR_IN_CAL_MODE	255		// 2011-05-09 -WFC-

void  adc_lt_read( BYTE curChannel, BYTE nextChannel )
{
	LSENSOR_DESCRIPTOR_T		*pSnDesc;	// points to local sensor descriptor
	GENERIC_UNION u;
	INT32	filteredData;
	BYTE	isPositive;
	BYTE	sensorN;
	LOADCELL_T 				*pLc;			// points to a loadcell 2011-05-09 -WFC-

	
	u.i32v = adc_lt_read_spi(  gaAdcLT_op_desc[ nextChannel ].cmd0,
								gaAdcLT_op_desc[ nextChannel ].cmd1 );

	sensorN = gaAdcLT_op_desc[ curChannel ].sensorN;
	if ( sensorN < MAX_NUM_LSENSORS ) {
		pSnDesc = &gaLSensorDescriptor[ sensorN ];
		if ( sensorN < MAX_NUM_LSENSORS ) {
			pSnDesc->rawAdcData = u.i32v;		// raw ADC data included status of the chip
		}
		
		#if ( CONFIG_DEBUG_ADC == TRUE )
			pSnDesc->numSamples++;		// 2011-08-05
		#endif
		isPositive = u.b.b3 & ADC_LT_SIGN_BIT;
		u.b.b3 &= ADC_LT_STRIP_3MSBITS_CNT;
		u.i32v >>=5;					// strip out the 5 LSB bits
		if (!isPositive) {				// if it is a negative value
			// u.i32v = -u.i32v;
			u.b.b3 = 0xFF;		// the ADC value is already in two's complement negative in 24bits, just expands it to 32bits as negative value.
		}
		
		// saved un-filtered raw ADC count.
		pSnDesc->prvRawADCcount = pSnDesc->curRawADCcount;
		pSnDesc->curRawADCcount = u.i32v;

		// 2011-05-09 -WFC- v
		if ( SENSOR_TYPE_LOADCELL == pSnDesc->type ) {
			pLc = (LOADCELL_T *) pSnDesc-> pDev;
			if ( (pLc-> runModes) & LC_RUN_MODE_IN_CAL )
				isPositive = ADC_LT_SENOSR_IN_CAL_MODE;
			else if ( pLc-> runModes & LC_RUN_MODE_PEAK_HOLD_ENABLED )
				isPositive = NO;	// set filter disabled.
			else
				isPositive = pSnDesc->cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK;
		}
		else
			isPositive = pSnDesc->cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK;
		// 2011-05-09 -WFC- ^

		// 2011-05-09 -WFC- isPositive = pSnDesc->cnfg & SENSOR_CNFG_FILTER_LEVEL_MASK;
		// 2011-05-09 -WFC- v
		if ( ADC_LT_SENOSR_IN_CAL_MODE == isPositive ) {
			if ( LC_FILTER_EVENT_DISPLAYABLE ==
				pSnDesc->gapfSensorFilterMethod( sensorN, isPositive, u.i32v, &filteredData) ) {
				pSnDesc->prvADCcount = pSnDesc->curADCcount;
				pSnDesc->curADCcount = filteredData;
				pSnDesc->status |= SENSOR_STATUS_GOT_ADC_CNT;	// flag that this sensor has new ADC count.
			}
		}// 2011-05-09 -WFC- ^
		else if ( isPositive ) { // if filter is enabled
			if ( LC_FILTER_EVENT_DISPLAYABLE ==
				pSnDesc->gapfSensorFilterMethod( sensorN, isPositive, u.i32v, &filteredData) ) {
				pSnDesc->prvADCcount = pSnDesc->curADCcount;
				pSnDesc->curADCcount = filteredData;
				//pSnDesc->status |= SENSOR_STATUS_GOT_ADC_CNT;	// flag that this sensor has new ADC count.
				// 2011-05-04 -WFC- v
				//pSnDesc->status |= SENSOR_STATUS_GOT_ADC_CNT;	// flag that this sensor has new ADC count.
				if ( timer_mSec_expired( &pSnDesc->filterTimer ) )		{		// perform time sensitive or schedule tasks here.
					sensor_set_filter_timer( pSnDesc );
					pSnDesc->status |= SENSOR_STATUS_GOT_ADC_CNT;	// flag that this sensor has new ADC count.
				}
				// 2011-05-04 -WFC- ^
			}
		}
		else { // filter is disabled, just use the raw value.
			pSnDesc->prvADCcount = pSnDesc->curADCcount;
			pSnDesc->curADCcount = u.i32v;
			pSnDesc->status |= SENSOR_STATUS_GOT_ADC_CNT;	// flag that this sensor has new ADC count.
		}
		// -WFC- 2011-04-12 v
		if ( pSnDesc->curRawADCcount > pSnDesc->maxRawADCcount ) {
			pSnDesc->maxRawADCcount = pSnDesc->curRawADCcount;
			pSnDesc->status |= SENSOR_STATUS_GOT_NEW_ADC_PEAK;
		}
		// -WFC- 2011-04-12 ^
		pSnDesc->status |= SENSOR_STATUS_GOT_UNFILTER_ADC_CNT;	// flag that this sensor has new unfiltered ADC count.

	} // end if ( sensorN < MAX_NUM_LSENSORS ) {}
	
} // end adc_lt_read();

/**
 * read raw ADC data.
 *
 * @param  cmd0 -- command0 of ADC chip which contains input selection such as channel and reference.
 * @param  cmd1 -- command1 of ADC chip which contains conversion speed.
 *
 * @return raw ADC data.
 *
 * @note It only takes about 50uSec~60uSec to read 4 bytes of data at 1,843,200 Hz.
 *       The raw data needs to further process to get the true ADC count.
 *
 * History:  Created on 2007/09/12 by Wai Fai Chin
 */

INT32  adc_lt_read_spi( BYTE cmd0, BYTE cmd1 )
{
	GENERIC_UNION u;
	
	ADC_LT_CS_SELECT;
	u.b.b3 = adc_lt_spi_transceive( cmd0 );
	u.b.b2 = adc_lt_spi_transceive( cmd1 );
	u.b.b1 = adc_lt_spi_transceive( cmd1 );
	u.b.b0 = adc_lt_spi_transceive( cmd1 );
	ADC_LT_CS_DESELECT;
	return u.i32v;
	
} // end adc_lt_read_spi(,)


/**
 * read and process raw ADC count and configure ADC chip while reading ADC chip.
 *
 * @param  curChannel  -- current active channel of the ADC chip.
 * @param  nextChannel -- next channel for this ADC chip.
 * @return adc count.
 *
 * History:  Created on 2007/09/12 by Wai Fai Chin
 */

INT32  adc_lt_read_count( BYTE nextChannelCmd, BYTE sampleSpeedCmd )
{
	GENERIC_UNION u;
	BYTE	isPositive;

	u.i32v = adc_lt_read_spi(  nextChannelCmd, sampleSpeedCmd );
	isPositive = u.b.b3 & ADC_LT_SIGN_BIT;
	u.b.b3 &= ADC_LT_STRIP_3MSBITS_CNT;
	u.i32v >>=5;					// strip out the 5 LSB bits
	if (!isPositive) {				// if it is a negative value
		u.b.b3 = 0xFF;				// the ADC value is already in two's complement negative in 24bits, just expands it to 32bits as negative value.
	}
	return u.i32v;
} // end adc_lt_read_count();

#endif  // ( CONFIG_EMULATE_ADC == TRUE )
