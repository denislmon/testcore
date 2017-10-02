/*! \file temperature.c \brief temperature related functions.*/
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
//   This is a high level module to convert filtered ADC count into a temperature data
// base on the calibration table. It does not know how the sensor got the ADC data.
// It just know how to compute the termperature. It formats output termperature data string to
// a caller supplied string buffer but it does not know how to output the data.
//
//            Application          Abstract Object        Hardware driver
//         +---------------+      +---------------+     +----------------+
//         |  temperature  |      |               |     |                |
//    -----|     MODULE    |<-----| SENSOR MODULE |<----| CPU ADC value  |
//         |  ADC to C, F  |      |      ADC      |     |                |
//         |               |      |               |     |                |
//         +---------------+      +---------------+     +----------------+
//
//
//
//
// Threshold termperature values will be saved in the same unit as the cal unit.
// If the user enter the value in unit different than cal unit, it will do
// convered entered value to cal unit and then saved it; these values have suffix of NV.
// termperature value is saved in display unit; these values have the suffix of FNV.
//
// NOTE:
// NV suffix stands for nonvolatile memory. It is in RAM and recalled from nonvolatile memory during powerup.
// FNV suffix stands for ferri nonvolatile memory. It is in RAM and frequently write to ferri memory everytime the value has changed.
// ****************************************************************************


#if ( CONFIG_PCB_AS == 	CONFIG_PCB_AS_SCALECORE1 )

#include  "config.h"
#include  "cmdparser.h"		// for CMD_ITEM_DELIMITER
#include  "calibrate.h"
#include  "sensor.h"
#include  "temperature.h"
#include  "loadcell.h"
#include  "lc_tare.h"
#include  "lc_total.h"
#include  "lc_zero.h"
#include  "nvmem.h"
#include  "commonlib.h"
#include  <math.h>
#include  <stdio.h>		// for sprintf_P()

#include  "scalecore_sys.h"
#include  "bios.h"			// 2011-04-18 -WFC-

TEMP_SENSOR_T	gaTempSensor[ MAX_NUM_TEMP_SENSOR ];

BYTE temp_sensor_format_annunciator( TEMP_SENSOR_T *pTS, char *pOutStr );

/**
 * It computes temperature based on ADC counts of voltage from temperature sensor chip.
 *
 * @param  sn  -- Sensor channel number.
 *
 * @post updated contents in gaLSensorDescriptor[ sn ] and gaTempSensor[ sn ].
 *
 * @return none.
 *
 *   Vref == 1.0 V.     12 bits resultion.
 *   1 volt = 1/1 * 2^12 = 4096 ADC count.
 *   0.00107421875 volt = 1 ADC count.
 *   0.0195 V = 1 C degree = 0.0195 V * 4096 count/ V = 66.56 == 67 count / degree.
 *   0 C = 0.4 Volt = 0.4V * 3413 count/V = 1365 count at 0 C;
 *	 Vo = 0.0195V * T + Vzero = 0.0195 * T + 0.4 V;
 *   Temp out ADC_count = 67 count * T + 1365 count;
 *   Temp = (ADC_count - 1365) / 66.56 = ( ADC_count - 1365) * 0.015024 in C;
 *
 * History:  Created on 2009/01/08  by Wai Fai Chin
 * 2011-08-11 -WFC-  fixed a bug not to change value when changed unit and also fixed under temperature bug.
 * 2013-09-04 -WFC-  modified for on chip temperature sensor.
 */


void  temp_sensor_compute_temp( BYTE sn )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	TEMP_SENSOR_T			*pTS;			// points to a Temperature Sensor.
	BYTE					sensorStatus;

	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pSnDesc = &gaLSensorDescriptor[ sn ];
		pTS = (TEMP_SENSOR_T *) pSnDesc-> pDev;
		sensorStatus = pSnDesc-> status & ~SENSOR_STATUS_GOT_VALUE;		// Assume it has no valid value.
		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {						// if this sensor is enabled.
			if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {		// if got a new ADC count
				pTS-> status &= ~(TMPS_STATUS_GOT_VALID_VALUE | TMPS_STATUS_GOT_CAL_VALUE);	// assumed it has no valid temperature
				pTS-> value = (pSnDesc-> curADCcount - 200) * 0.133982f - 273.15f;		// in Celsius
				pSnDesc-> value = pTS-> value + 273.15f;						// pSnDesc->value is always in Kelvin
				if (SENSOR_UNIT_TMPF == pTS-> viewCB.unit ) {
					pTS-> value = (pSnDesc-> value * 1.8f) - 459.67f;			// in Fahrenheit
				}
				else if ( SENSOR_UNIT_TMPK == pTS-> viewCB.unit )
					pTS-> value = pSnDesc-> value;								// in Kelvin
				pTS-> status	|= TMPS_STATUS_GOT_VALID_VALUE;			// flag this loadcell has a valid weight
				sensorStatus	|= SENSOR_STATUS_GOT_VALUE;				// flag this sensor has a valid value.
			} // end if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc->status) ) {}
		} // end if ( SENSOR_STATUS_ENABLED...) if this sensor is enabled.
		pSnDesc-> status |= sensorStatus;					// update sensor status.
	} // end if valid sensor number.
} // end temp_sensor_compute_temp()

/*
void  temp_sensor_compute_temp( BYTE sn )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
//	SENSOR_CAL_T			*pCal;			// points to a cal table
	TEMP_SENSOR_T			*pTS;			// points to a Temperature Sensor.
//	float					scaleFactor;
	BYTE					sensorStatus;
//	BYTE					vN;
	
	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pSnDesc = &gaLSensorDescriptor[ sn ];
		pTS = (TEMP_SENSOR_T *) pSnDesc-> pDev;
		sensorStatus = pSnDesc-> status & ~SENSOR_STATUS_GOT_VALUE;		// Assume it has no valid value.
		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {						// if this sensor is enabled.
			if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {		// if got a new ADC count
				pTS-> status &= ~(TMPS_STATUS_GOT_VALID_VALUE | TMPS_STATUS_GOT_CAL_VALUE);	// assumed it has no valid weight
																			// convert ADC count to voltage

				// 2011-08-11 -WFC- v fixed a bug not to change value when changed unit and also fixed under temperature bug.
				pTS-> value = (pSnDesc-> curADCcount - 1365) * 0.015024f;		// in Celsius
				pSnDesc-> value = pTS-> value + 273.15f;						// pSnDesc->value is always in Kelvin
				if (SENSOR_UNIT_TMPF == pTS-> viewCB.unit ) {
					pTS-> value = (pSnDesc-> value * 1.8f) - 459.67f;			// in Fahrenheit
				}
				else if ( SENSOR_UNIT_TMPK == pTS-> viewCB.unit )
					pTS-> value = pSnDesc-> value;								// in Kelvin
//				pTS-> value =
//				pSnDesc-> value = (pSnDesc-> curADCcount - 372) * 0.055088f;
				// 2011-08-11 -WFC- ^
				pTS-> status	|= TMPS_STATUS_GOT_VALID_VALUE;			// flag this loadcell has a valid weight
				sensorStatus	|= SENSOR_STATUS_GOT_VALUE;				// flag this sensor has a valid value.
			} // end if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc->status) ) {}
		} // end if ( SENSOR_STATUS_ENABLED...) if this sensor is enabled.
		pSnDesc-> status |= sensorStatus;					// update sensor status.
	} // end if valid sensor number.
} // end temp_sensor_compute_temp()
*/


/**
 * It initialized temperature sensor when the application software runs the very 1st time of this device.
 * It initializes gaTempSensor[] data structure of sn based on
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaTempSensor[].
 *
 * @post   updated gaTempSensor[] data structure of this temperature sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 * 2011-04-21 -WFC- default viewing unit to C.
 */

void temp_sensor_1st_init( BYTE sn )
{
	TEMP_SENSOR_T	*pTS;	// points to a voltage monitor
	
	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pTS = &gaTempSensor[ 0 ];
		pTS-> viewCB.fValue	= gafSensorShowCBFNV[ sn ];
		pTS-> viewCB.iValue	= gawSensorShowCBFNV[ sn ];
		pTS-> viewCB.decPt	= gabSensorShowCBdecPtFNV[ sn ];
		pTS-> viewCB.unit	= gabSensorShowCBunitsFNV[ sn ] = gabSensorViewUnitsFNV[ sn ] =SENSOR_UNIT_TMPC;
	}
} // end temp_sensor_1st_init()


/**
 * It initializes gaTempSensor[] data structure of sn based on
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaTempSensor[].
 *
 * @post   updated gaTempSensor[] data structure of this temperature sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 */

void temp_sensor_init( BYTE sn )
{
	TEMP_SENSOR_T	*pTS;	// points to a temperature sensor
	
	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pTS = &gaTempSensor[ 0 ];
		pTS-> status   = 0;										// clear status
		pTS-> runModes = 0;										// disabled this loadcell.
		temp_sensor_update_param( sn );
	}
} // end temp_sensor_init()



/**
 * It performs ADC to temperature...
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaTempSensor[].
 *
 * @post   updated gaTempSensor[] data structure of this temperature sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 */

void temp_sensor_tasks( BYTE sn )
{
	TEMP_SENSOR_T	*pTS;	// points to a temperature sensor

	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pTS = &gaTempSensor[ 0 ];
		if ( TMPS_RUN_MODE_ENABLED & (pTS-> runModes)) {		// if this voltage monitor is enabled.
			temp_sensor_compute_temp( sn );
		}
	}
} // end temp_sensor_tasks()

#else
#include  "config.h"
#include  "cmdparser.h"		// for CMD_ITEM_DELIMITER
#include  "calibrate.h"
#include  "sensor.h"
#include  "temperature.h"
#include  "loadcell.h"
#include  "lc_tare.h"
#include  "lc_total.h"
#include  "lc_zero.h"
#include  "nvmem.h"
#include  "commonlib.h"
#include  <math.h>
#include  <stdio.h>		// for sprintf_P()

#include  "adc_driver.h"
#include  "adc_cpu.h"
#include  "scalecore_sys.h"

TEMP_SENSOR_T	gaTempSensor[ MAX_NUM_TEMP_SENSOR ];

BYTE temp_sensor_format_annunciator( TEMP_SENSOR_T *pTS, char *pOutStr );


/* *
 * It computes temperature based on ADC counts of voltage from temperature sensor chip.
 *
 * @param  sn  -- Sensor channel number.
 *
 * @post updated contents in gaLSensorDescriptor[ sn ] and gaTempSensor[ sn ].
 *
 * @return none.
 *   ScaleCore1: ATmega1281
 *   Vref == 1.1 V.     10 bits resultion.
 *   1 volt = 1/1.1 * 2^10 = 930.90909 = 931 ADC count.
 *   0.00107421875 volt = 1 ADC count.
 *   0.0195 V = 1 C degree = 0.0195 V * 930.90909 count/ V = 18.152727255 == 18 count / degree.
 *   0 C = 0.4 Volt = 0.4V * 930.90909 count/V = 372.363636 = 372 count at 0 C;
 *	 Vo = 0.0195V * T + Vzero = 0.0195 * T + 0.4 V;
 *   Temp out ADC_count = 18 count * T + 372 count;
 *   Temp = (ADC_count - 372) / 18.152727255 = ( ADC_count - 372) * 0.055088 in C;
 *
 * History:  Created on 2009/01/08  by Wai Fai Chin
 * /


void  temp_sensor_compute_temp( BYTE sn )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
//	SENSOR_CAL_T			*pCal;			// points to a cal table
	TEMP_SENSOR_T			*pTS;			// points to a Temperature Sensor.
//	float					scaleFactor;
	BYTE					sensorStatus;
//	BYTE					vN;

	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pSnDesc = &gaLSensorDescriptor[ sn ];
		pTS = (TEMP_SENSOR_T *) pSnDesc-> pDev;
		sensorStatus = pSnDesc-> status & ~SENSOR_STATUS_GOT_VALUE;		// Assume it has no valid value.
		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {						// if this sensor is enabled.
			if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {		// if got a new ADC count
				pTS-> status &= ~(TMPS_STATUS_GOT_VALID_VALUE | TMPS_STATUS_GOT_CAL_VALUE);	// assumed it has no valid weight
																			// convert ADC count to voltage
				pTS-> value =
				pSnDesc-> value = (pSnDesc-> curADCcount - 372) * 0.055088f;
				pTS-> status	|= TMPS_STATUS_GOT_VALID_VALUE;			// flag this loadcell has a valid weight
				sensorStatus	|= SENSOR_STATUS_GOT_VALUE;				// flag this sensor has a valid value.
			} // end if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc->status) ) {}
		} // end if ( SENSOR_STATUS_ENABLED...) if this sensor is enabled.
		pSnDesc-> status |= sensorStatus;					// update sensor status.
	} // end if valid sensor number.
} // end temp_sensor_compute_temp()
*/

/**
 * It computes temperature based on ADC counts of voltage from temperature sensor chip.
 *
 * @param  sn  -- Sensor channel number.
 *
 * @post updated contents in gaLSensorDescriptor[ sn ] and gaTempSensor[ sn ].
 *
 * @return none.
 *   ATXmega128A1 CPU:
 *   Atmel stored a calibrated ADC count reading at 85 C. 273.15 + 85 = 358.15 Kelvin.
 *   This calibrated ADC count value is stored at TEMPSENSE0 and TEMPSENSE1 to form 16bit value.
 *   scaleFactor = 358.15 / TEMPSENSE;
 *   Temp = ADC_count * ( 358.15 / TEMPSENSE) - 273.15 = (ADC_count * scaleFactor) - 273.15; in C
 *
 * History:  Created on 2009/01/08  by Wai Fai Chin
 */


void  temp_sensor_compute_temp( BYTE sn )
{
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
//	SENSOR_CAL_T			*pCal;			// points to a cal table
	TEMP_SENSOR_T			*pTS;			// points to a Temperature Sensor.
	BYTE					sensorStatus;

	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pSnDesc = &gaLSensorDescriptor[ sn ];
		pTS = (TEMP_SENSOR_T *) pSnDesc-> pDev;
		sensorStatus = pSnDesc-> status & ~SENSOR_STATUS_GOT_VALUE;		// Assume it has no valid value.
		if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {						// if this sensor is enabled.
			if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc-> status) ) {		// if got a new ADC count
				pTS-> status &= ~(TMPS_STATUS_GOT_VALID_VALUE | TMPS_STATUS_GOT_CAL_VALUE);	// assumed it has no valid weight
				// convert ADC count to voltage
				pTS-> value =
				pSnDesc-> value = ((float)(pSnDesc-> curADCcount) * pTS-> scaleFactor );		// pSnDesc-> value will in Kelvin always,
				if ( SENSOR_UNIT_TMPC == pTS-> viewCB.unit ) {
					pTS-> value = pSnDesc-> value - 273.15f;					// in Celsius
				}
				else if (SENSOR_UNIT_TMPF == pTS-> viewCB.unit ) {
					pTS-> value = (pSnDesc-> value * 1.8f) - 459.67f;			// in Fahrenheit
				}

				pTS-> status	|= TMPS_STATUS_GOT_VALID_VALUE;			// flag this loadcell has a valid weight
				sensorStatus	|= SENSOR_STATUS_GOT_VALUE;				// flag this sensor has a valid value.
			} // end if ( SENSOR_STATUS_GOT_ADC_CNT & (pSnDesc->status) ) {}
		} // end if ( SENSOR_STATUS_ENABLED...) if this sensor is enabled.
		pSnDesc-> status |= sensorStatus;					// update sensor status.
	} // end if valid sensor number.
} // end temp_sensor_compute_temp()


/**
 * It initialized temperature sensor when the application software runs the very 1st time of this device.
 * It initializes gaTempSensor[] data structure of sn based on
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaTempSensor[].
 *
 * @post   updated gaTempSensor[] data structure of this temperature sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 * 2010-08-30	-WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 *
 */

#define TEMPSENSE0_offset 0x2E
#define TEMPSENSE1_offset 0x2F

void temp_sensor_1st_init( BYTE sn )
{
	TEMP_SENSOR_T	*pTS;	// points to a voltage monitor
	NVM_PROD_SIGNATURES_t a;

	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pTS = &gaTempSensor[ 0 ];
		pTS-> viewCB.fValue	= gafSensorShowCBFNV[ sn ] = 0.1f;
		pTS-> viewCB.iValue	= gawSensorShowCBFNV[ sn ] = 1;
		pTS-> viewCB.decPt	= gabSensorShowCBdecPtFNV[ sn ] = 1;
		//2010-08-30	-WFC- 	pTS-> viewCB.unit	= gabSensorShowCBunitsFNV[ sn ] = SENSOR_UNIT_TMPC;
		pTS-> viewCB.unit	= gabSensorViewUnitsFNV[ sn ] = gabSensorShowCBunitsFNV[ sn ] = SENSOR_UNIT_TMPC;		//2010-08-30	-WFC-
	}
} // end temp_sensor_1st_init()


/**
 * It initializes gaTempSensor[] data structure of sn based on
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaTempSensor[].
 *
 * @post   updated gaTempSensor[] data structure of this temperature sensor.
 *
 * History:  Created on 2009-01-08 by Wai Fai Chin
 * 2010-11-17 -WFC- Adjusted temperature scale factor based on user define model.
 */

void temp_sensor_init( BYTE sn )
{
	TEMP_SENSOR_T	*pTS;	// points to a temperature sensor
	UINT16			tempCal;

	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pTS = &gaTempSensor[ 0 ];
		pTS-> status   = 0;										// clear status
		pTS-> runModes = 0;										// disabled this loadcell.
		// read calibarted ADC count at 85 C, 358.15 in Kelvin.
		tempCal  = SP_ReadCalibrationByte( PROD_SIGNATURES_START +  TEMPSENSE0_offset );
		tempCal |= SP_ReadCalibrationByte( PROD_SIGNATURES_START +  TEMPSENSE1_offset ) << 8;
		if ( USER_DEF_MODEL_CHI234 == gtProductInfoFNV.userDefModelCode ||
			 USER_DEF_MODEL_GENERIC_AC == gtProductInfoFNV.userDefModelCode ) {	// 5V AC dimmer, just use 1.0 V Ref V
			tempCal += (UINT16)( gbCPU_ADCB_offset>>1 );			// for internal 1.0V Ref.
			pTS-> scaleFactor = (358.15f / (float)tempCal);			// Vref 1.0 V.
		}
		else {
			 pTS-> scaleFactor	= (358.15f / (float)tempCal);
			 // adjusted cal ADC count with offset/2,
			 tempCal -= (UINT16)( gbCPU_ADCB_offset );				// for Vcc/1.6 Ref
			 // Multiply the scale factor by 2.1875 because the calibrated ADC count was used 1 V Ref. The new reference is Vcc/1.6 = 2.1875V
			 // 2.1875 * 358.15 = 783.453125, or Just 2 x 358.15 = 716.3 for Vcc ranged from 3.4 to 3.6 V
			 pTS-> scaleFactor = (716.3f / (float)tempCal);			// Vref 2.1875 V

		}
		temp_sensor_update_param( sn );
	}
} // end temp_sensor_init()


/**
 * It performs ADC to temperature...
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaTempSensor[].
 *
 * @post   updated gaTempSensor[] data structure of this temperature sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 */

void temp_sensor_tasks( BYTE sn )
{
	TEMP_SENSOR_T	*pTS;	// points to a temperature sensor

	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pTS = &gaTempSensor[ 0 ];
		if ( TMPS_RUN_MODE_ENABLED & (pTS-> runModes)) {		// if this voltage monitor is enabled.
			temp_sensor_compute_temp( sn );
		}
	}
} // end temp_sensor_tasks()


#endif


/**
 * It formats temperature data in a dispaly format output based on countby.
 *
 * @param  pTs		-- pointer to temperature data structure; it is an input parameter.
 * @param  pOutStr  -- points to an allocated output string buffer.
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 */

BYTE temp_sensor_format_output( TEMP_SENSOR_T *pTS, char *pOutStr )
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	//char *ptr_P; this is also work
	PGM_P	ptr_P;
	char	unitName[5];
	BYTE	n;
	BYTE 	formatBuf[6];

	len = 0;
	if ( TMPS_RUN_MODE_ENABLED & (pTS-> runModes)) {			// if this voltage monitor is enabled.
		if ( TMPS_STATUS_GOT_VALID_VALUE & (pTS-> status)) {		// if this voltage monitor has valid value.
			memcpy_P ( &ptr_P, &gcUnitNameTbl[ pTS-> viewCB.unit ], sizeof(PGM_P));
			strcpy_P( unitName, ptr_P );
			fRound = float_round( pTS-> value, pTS-> viewCB.fValue);
			if ( pTS-> viewCB.decPt > 0 )
				precision = pTS-> viewCB.decPt;
			else
				precision = 0;
			float_format( formatBuf, 8, precision);
			len = (BYTE) sprintf( pOutStr, formatBuf, fRound );
			n = sprintf_P( pOutStr + len, PSTR(" %4s\r\n"), unitName );
			len += n;
		}
		else
			len = (BYTE) sprintf_P( pOutStr, gcStrFmtDash10LfCr_P);		// print ---------- line.
	}
	return len;	// number of char in the output string buffer.
} // end temp_sensor_format_output()

/**
 * It formats temperature data in packet format:
 *  data; unit; annunciator
 *
 * @param  pSnDesc	 -- pointer to sensor descriptor structure; it is an input parameter.
 * @param  pOutStr	 -- points to an allocated output string buffer.
 * @param  valueType -- value type such as gross, net, tare, total, etc...
 *
 * @post   format output data is store in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/01/16 by Wai Fai Chin
 * 2012-02-23 -WFC- Changed input parameter from TEMP_SENSOR_T to LSENSOR_DESCRIPTOR_T, support new sensor_format_sys_annunciator() with extra parameters.
 * 				Added codes to support ADC count value type.
 */

// 2012-02-23 -WFC-  BYTE temp_sensor_format_packet_output( TEMP_SENSOR_T *pTS, char *pOutStr, BYTE valueType )
BYTE temp_sensor_format_packet_output( LSENSOR_DESCRIPTOR_T *pSnDesc, char *pOutStr, BYTE valueType ) //2012-02-23 -WFC-
{
	BYTE	len;
	BYTE    precision;
	float	fRound;
	BYTE 	formatBuf[6];
	TEMP_SENSOR_T *pTS;

	pTS = (TEMP_SENSOR_T *) pSnDesc-> pDev;
	len = 0;
	if ( TMPS_RUN_MODE_ENABLED & (pTS-> runModes)) {			// if this loadcell is enabled.
		if ( TMPS_STATUS_GOT_VALID_VALUE & (pTS-> status)) {	// if this loadcell has valid value

			if ( SENSOR_VALUE_TYPE_ADC_COUNT == valueType )
				fRound = (float)(pSnDesc-> curADCcount);
			else
				fRound = float_round( pTS-> value, pTS-> viewCB.fValue);

			precision = 0;
			if ( SENSOR_VALUE_TYPE_ADC_COUNT != valueType ) {
				if ( pTS-> viewCB.decPt > 0 )
					precision = pTS-> viewCB.decPt;
				else
					precision = 0;
			}
			float_format( formatBuf, 8, precision);
			len += (BYTE) sprintf( pOutStr + len, formatBuf, fRound );
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
			len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pTS-> viewCB.unit );
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
		}
	}

	if ( 0 == len ) {
		len = (BYTE) sprintf_P( pOutStr, gcStrDefaultSensorOutFormat );		// formated default 0;0;0; as weight;unit;numLitSeg;
	}
	else {
		if ( valueType & SENSOR_VALUE_TYPE_CAL_BARGRAPH_bm ) {				// if this sensor is selected for output bargraph, then format it for output.
			len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) gbBargraphNumLitSeg );
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
		}
		else {
			pOutStr[len]= '0';						// number of lit segment is 0
			len++;
			pOutStr[len]= CMD_ITEM_DELIMITER;
			len++;
		}
	}

	len += temp_sensor_format_annunciator( pTS, pOutStr + len );
	// 2012-02-23 -WFC- len += sensor_format_sys_annunciator( pOutStr + len );
	len += sensor_format_sys_annunciator( pOutStr + len, 0 );		// 2012-02-23 -WFC-
	return len;	// number of char in the output string buffer.
} // end temp_sensor_format_packet_output(,,)


/**
 * It formats annunciator of this loadcell.
 *
 *
 * @param  pTs		-- pointer to temperature data structure; it is an input parameter.
 * @param  pOutStr	 -- points to an allocated output string buffer.
 *
 * @post   format annuciators are stored in a buffer pointed by pOutStr.
 *
 * @return number bytes in the string buffer. 0 == failed.
 *
 * History:  Created on 2009/01/20 by Wai Fai Chin
 */
BYTE temp_sensor_format_annunciator( TEMP_SENSOR_T *pTS, char *pOutStr )
{
	BYTE len;

	len  = sprintf_P( pOutStr, gcStrFmt_pct_d, (int) pTS-> runModes );
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	len += sprintf_P( pOutStr + len, gcStrFmt_pct_d, (int) pTS-> status );
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;
	pOutStr[len]= '0';		// dummy annunciator.
	len++;
	pOutStr[len]= CMD_ITEM_DELIMITER;
	len++;

	return len;
} // end temp_sensor_format_annunciator(,)



/**
 * It updates parameter of gaTempSensor[] data structure of sn based on changed value of
 * gaSensorCalNV[], gafSensorShowCapacityFNV[], gafSensorShowCBFNV[],
 * gawSensorShowCBFNV[], gabSensorShowCBdecPtFNV[] and gabSensorShowCBunitsFNV[].
 *
 * @param  sn	-- sensor number of local sensor. Note that it is not the index of gaTempSensor[].
 *
 * @post   updated gaTempSensor[] data structure of this temperature sensor.
 *
 * History:  Created on 2009/01/08 by Wai Fai Chin
 * 2010-08-30	-WFC- added gabSensorViewUnitsFNV[] and logics to handle viewing unit.
 *
 */

void  temp_sensor_update_param( BYTE sn )
{
	TEMP_SENSOR_T			*pTS;		// points to a temperature sensor
	LSENSOR_DESCRIPTOR_T	*pSnDesc;	// points to local sensor descriptor

	if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
		pSnDesc = &gaLSensorDescriptor[ sn ];
		pTS = (TEMP_SENSOR_T *) pSnDesc-> pDev;
		//pTS-> viewCapacity	= gafSensorShowCapacityFNV[ sn ];		// NOTE that viewCapacity and viewCB could had already updated by cmd_current_unit_post_update()
		pTS-> viewCB.fValue	= gafSensorShowCBFNV[ sn ];
		pTS-> viewCB.iValue	= gawSensorShowCBFNV[ sn ];
		pTS-> viewCB.decPt	= gabSensorShowCBdecPtFNV[ sn ];
		//2010-08-30	-WFC- pTS-> viewCB.unit	= gabSensorShowCBunitsFNV[ sn ];
		pTS-> viewCB.unit	= gabSensorViewUnitsFNV[ sn ];				//2010-08-30	-WFC-

		if ( SENSOR_UNIT_TMPC == pTS-> viewCB.unit ) {
			pTS-> value = pSnDesc-> value - 273.15f;					// in Celsius
		}
		else if (SENSOR_UNIT_TMPF == pTS-> viewCB.unit ) {
			pTS-> value = (pSnDesc-> value * 1.8f) - 459.67f;			// in Fahrenheit
		}
		else
			pTS-> value = pSnDesc-> value;								// in Kelvin

		if (gabSensorFeaturesFNV[ sn ] & SENSOR_FEATURE_SENSOR_ENABLE)
			pTS-> runModes	   |= SENSOR_FEATURE_SENSOR_ENABLE;
		else
			pTS-> runModes	   &= ~SENSOR_FEATURE_SENSOR_ENABLE;
	}
} // end temp_sensor_update_param()


/**
 * See if the cpu temperature sensor's value is outside of specification.
 * The specification is defined as <-45 C as under temperature, > 70 C as over temperature.
 *
 * @param	sn		-- sensor number.
 *
 * @return SYS_HW_STATUS_ANNC_UNDER_TEMPERATURE if temperature < -45 C , 273 - 45 = 228 Kelvin
 * 		   SYS_HW_STATUS_ANNC_OVER_TEMPERATURE  if temperature > 70 C,   273 + 70 = 343 Kelvin
 *		   0 if within specification.
 *
 * History:  Created on 2010-08-02 by Wai Fai Chin
 * 2011-04-18 -WFC- set temperature flags in gbBiosSysStatus.
 */

BYTE temp_sensor_is_temp_outside_spec( BYTE sn )
{
	BYTE status;
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	INT16		kelvinTmp;

	status = 0;
	if (  SYS_RUN_MODE_AUTO_SECONDARY_TEST != gbSysRunMode)	{		// secondary test mode changed RefV, so don't check temperature because it may give false reading.
		if ( SENSOR_NUM_PCB_TEMPERATURE == sn ) { // ensured sn is a pcb temperature sensor index.
			pSnDesc = &gaLSensorDescriptor[ sn ];
			if ( SENSOR_CNFG_ENABLED & pSnDesc-> cnfg ) {						// if this sensor is enabled.
				kelvinTmp = (INT16) pSnDesc-> value;							// pSnDesc->value is always in Kelvin.
				if ( kelvinTmp > SYS_OVER_TEMPERATURE_KELVIN ) {
					status = SYS_HW_STATUS_ANNC_OVER_TEMPERATURE;
					gbBiosSysStatus |= BIOS_SYS_STATUS_ANNC_OVER_TEMPERATURE;	// set over temperature flag. 2011-04-18 -WFC-
				}
				else if (kelvinTmp < SYS_UNDER_TEMPERATURE_KELVIN ) {
					status = SYS_HW_STATUS_ANNC_UNDER_TEMPERATURE;
					gbBiosSysStatus |= BIOS_SYS_STATUS_ANNC_UNDER_TEMPERATURE;	// set under temperature flag. 2011-04-18 -WFC-
				}
				else {
					gbBiosSysStatus &= ~ ( BIOS_SYS_STATUS_ANNC_OVER_TEMPERATURE | BIOS_SYS_STATUS_ANNC_UNDER_TEMPERATURE ); // clear flags. 2011-04-18 -WFC-
				}
			}
		}
	}

	return status;
} // end temp_sensor_is_temp_outside_spec()

