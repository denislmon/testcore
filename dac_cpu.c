/*! \file dac_cpu.c \brief hardware cpu's DAC functions. */
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: serial.h
// Hardware: ATMEL ATXMEGA128A1
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2009/11/17 by Wai Fai Chin
//
//  Hardware cpu's DAC functions.
//
// ****************************************************************************


#include "dac_cpu.h"

#include  "cmdaction.h"  // for default dac settings.
#include  "scalecore_sys.h"
#if (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 )
//MERGE_TASK: #include  "self_test.h"
#endif

/*!
  \brief CPU DAC data structure.

* /

typedef struct  DAC_CPU_CAL_TAG
{
							/// calibrated offset
	BYTE	offset;
							/// calibrated gain
	BYTE	gain;
							/// DAC count at min span point; e.g. loadcell sensor at 0 LB = 0.5V,
	UINT16	wDacCountAtMinSpan;
							/// DAC count at max span point; e.g. loadcell sensor at Capacity = 5V,
	UINT16	wDacCountAtMaxSpan;
							/// sensor value at in span point of DAC output.
	float	fSenorValueAtMinSpan;
							/// sensor value at max span point of DAC output.
	float	fSensorValueAtMaxSpan;
							/// source sensor UNIT
	BYTE	sensorUnit;
							/// source sensor ID
	BYTE	sensorID;
							/// DAC config status
	BYTE	DacStatus;
}DAC_CPU_CAL_T;
*/

/// DAC calibrated offset
INT8	gabDacOffsetFNV[ MAX_DAC_CHANNEL ];

/// DAC calibrated Gain
INT8	gabDacGainFNV[ MAX_DAC_CHANNEL ];

/// DAC count of min span point; e.g. 0 LB = 0.5V,
UINT16	gawDacCountMinSpanFNV[ MAX_DAC_CHANNEL ];

/// DAC count of max span point; e.g. at Capacity = 5V,
UINT16	gawDacCountMaxSpanFNV[ MAX_DAC_CHANNEL ];

/// DAC source Sensor Value at min span point.
float	gafDacSensorValueAtMinSpanFNV[ MAX_DAC_CHANNEL ];

/// DAC source Sensor Value at max span point.
float	gafDacSensorValueAtMaxSpanFNV[ MAX_DAC_CHANNEL ];

/// DAC source sensor value unit
BYTE	gabDacSensorUnitFNV[ MAX_DAC_CHANNEL ];

/// DAC source sensor ID
BYTE	gabDacSensorIdFNV[ MAX_DAC_CHANNEL ];

/// DAC source sensor value Type.  bit7: 1=no filtered; bit6:0 Value type, 0==gross; 1==net,,,6=ADC count etc..

BYTE	gabDacSensorValueTypeFNV[ MAX_DAC_CHANNEL ];

/// DAC configuration status, bit7 1 =calibrated, bit6 1 = enable
BYTE	gabDacCnfgStatusFNV[ MAX_DAC_CHANNEL ];

/*! DAC output resolution of a corresponding sensor value.
 * resultion = MAX_DAC_VALUE / ( SensorValueMax - SensorValueMin)
 * It will be compute during initializing or after calibration.
 */
float	gafDacOutputResolution[ MAX_DAC_CHANNEL ];

// {dacChNum; status; sensorID; sensorUnit; sensorVmin; sensorVmax}
// {dacChNum; calType; opCode} where calType: calType: 0 == Aborted; 1 == save cal and exit; 2==offset, 3==gain; 4==min span point; 5== max span point;
//                             opCode: 0==init; 1==decrease by 1; 2==increase by 1; 10==dec by 10; 20==inc by 10; 100==dec by 100; 200 == inc by 100
// {dacChNum; offset; gain; CntMinSpan; CntMaxSpan}

BYTE	dac_cpu_get_sensor_value( BYTE sn, float *pfV );

#if (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 )

/**
 * Initialized CPU's DAC hardware
 *
 *
 * @note
 *
 * History:  Created on 2009/11/17 by Wai Fai Chin
 */

void dac_cpu_init_hardware( void )
{
	DAC_SingleChannel_Enable( &DACA, DAC_REFSEL_INT1V_gc, false );
	DAC_SingleChannel_Enable( &DACB, DAC_REFSEL_INT1V_gc, false );

}// end dac_cpu_init()

/**
 * Initialized all channels of CPU's DAC hardware
 *
 *
 * @note
 *
 * History:  Created on 2010/01/27 by Wai Fai Chin
 */

void dac_cpu_1st_init_all_channel( void )
{
	cmd_act_set_defaults( 0x30 );					// default high level DAC settings
	cmd_act_set_defaults( 0x32 );					// default internal DAC settings.
	dac_cpu_init_all_channel();
}// end dac_cpu_1st_init_all_channel()


/**
 * Initialized all channels of CPU's DAC hardware
 *
 *
 * @note
 *
 * History:  Created on 2010/01/27 by Wai Fai Chin
 */

void dac_cpu_init_all_channel( void )
{
	BYTE i;
	for ( i=0; i < MAX_DAC_CHANNEL; i++ ) {
		dac_cpu_init_settings(i);
	}
}// end dac_cpu_init_all_channel()

/**
 * Initialized CPU's DAC
 *
 *
 * @note
 *
 * History:  Created on 2009/11/17 by Wai Fai Chin
 */

void dac_cpu_init_settings( BYTE channel )
{
	dac_cpu_set_gain( channel, gabDacGainFNV[ channel ]);
	dac_cpu_set_offset( channel, gabDacOffsetFNV[ channel ] );
	dac_cpu_compute_resolution( channel);
}// end dac_cpu_init_settings()


/**
 * Test CPU's DAC output.
 *
 *
 * @note
 *
 * History:  Created on 2009/11/19 by Wai Fai Chin
 */
UINT16 gTestDACA0_data = 0xFFF;
UINT16 gTestDACB0_data = 0x7FF;

void dac_cpu_test_output( void )
{
	while ( !(DACA.STATUS & DAC_CH0DRE_bm ));
	DACA.CH0DATA = gTestDACA0_data;

	while ( !(DACB.STATUS & DAC_CH0DRE_bm ));
	DACB.CH0DATA = gTestDACB0_data;

}// end dac_cpu_test()


/**
 * set data out to the specified DAC channel.
 *
 * @param	channel	-- channel of a DAC,
 * @param	data	-- data to be output to the specified DAC channel.
 *
 * @note
 *  channel 0 is DACA.ch0;
 *  channel 1 is DACB.ch0;
 *
 * History:  Created on 2010/01/25 by Wai Fai Chin
 */

void dac_cpu_output( BYTE channel, UINT16 data  )
{
	switch ( channel ){
		case 0:
			while ( !(DACA.STATUS & DAC_CH0DRE_bm ));
			DACA.CH0DATA = data;
			break;
		case 1:
			while ( !(DACB.STATUS & DAC_CH0DRE_bm ));
			DACB.CH0DATA = data;
		break;
	}
}// end dac_cpu_output()


/**
 * set gain value of the specified DAC module.
 *
 * @param	channel	-- channel of a DAC,
 * @param	value	-- gain value the specified DAC channel.
 *
 * @note
 *  channel 0 is DACA.ch0;
 *  channel 1 is DACB.ch0;
 *
 * History:  Created on 2010/01/25 by Wai Fai Chin
 */

void dac_cpu_set_gain( BYTE channel, BYTE value  )
{
	switch ( channel ){
		case 0:
			DACA.GAINCAL = value;
			break;
		case 1:
			DACB.GAINCAL = value;
		break;
	}
}// end dac_cpu_set_gain()


/**
 * set offset value of the specified DAC module.
 *
 * @param	channel	-- channel of a DAC,
 * @param	data	-- data to be output to the specified DAC channel.
 *
 * @note
 *  channel 0 is DACA.ch0;
 *  channel 1 is DACB.ch0;
 *
 * History:  Created on 2010/01/25 by Wai Fai Chin
 */

void dac_cpu_set_offset( BYTE channel, BYTE value  )
{
	switch ( channel ){
		case 0:
			DACA.OFFSETCAL = value;
			break;
		case 1:
			DACB.OFFSETCAL = value;
		break;
	}
}// end dac_cpu_set_offset()


/**
 * walk through all DAC channel configuration data structure and output it based on its settings.
 *
 * DAC will output max voltage if a loadcell is overloaded, any other error will output 0V except warning messages.
 *
 * @note
 *  channel 0 is DACA.ch0;
 *  channel 1 is DACB.ch0;
 *  resultion = MAX_DAC_VALUE / ( maxSensorValueSpan - minSensorValueSpan )
 *
 * History:  Created on 2010/01/25 by Wai Fai Chin
 * 2011-01-10	-WFC- DAC outputs max voltage if a loadcell is overloaded, any other error will output 0V except warning messages.
 */
//TODO: handle input sensor error condition and output DAC based on error condition.

void dac_cpu_update_all_sensors_value( void  )
{
	BYTE	status;
	BYTE	i;
	BYTE	state;
	UINT16	wDacData;
	float	fV;

	for ( i=0; i < MAX_DAC_CHANNEL; i++ ) {
		status = gabDacCnfgStatusFNV[ i ];
		if ( !(status & DAC_STATUS_MANUAL_MODE )) {					// if DAC is in normal mode.
			if ( !(status & DAC_STATUS_IN_CAL_MODE) ) {					// if DAC is not in cal mode
				wDacData = 0;
				if ( (status & DAC_STATUS_ENABLED) ) {						// if DAC is enabled.
					state = dac_cpu_get_sensor_value( i, &fV );					// get sensor value.
					if ( gbSysErrorCodeForUser > 0 &&  gbSysErrorCodeForUser < SYS_USER_ERROR_CODE_START_WARNING_CODE ) {
						if ( SYS_USER_ERROR_CODE_OVERLOAD == gbSysErrorCodeForUser )
							wDacData = MAX_DAC_VALUE;
					}
					else {
						if ( !(state & SENSOR_STATE_IN_CAL_bm)) {					// if input sensor is NOT in cal mode.
							if ( ( state > SENSOR_STATE_GOT_VALUE_IN_CAL ) && ( state <= SENSOR_STATE_MAX_FOR_DISPLAYABLE ) ) {
								wDacData = dac_cpu_value_to_dac_count( i, fV);
							}
						} // end if sensor is not in cal mode
					}
				} // end if DAC enabled.
				dac_cpu_output( i, wDacData );
			} // end if DAC is not in cal mode.
		}
	}
}// end dac_cpu_translate_all_sensors_value()

/*
void dac_cpu_update_all_sensors_value( void  )
{
	BYTE	status;
	BYTE	i;
	BYTE	state;
	UINT16	wDacData;
	float	fV;

	for ( i=0; i < MAX_DAC_CHANNEL; i++ ) {
		status = gabDacCnfgStatusFNV[ i ];
		if ( !(status & DAC_STATUS_MANUAL_MODE )) {					// if DAC is in normal mode.
			if ( !(status & DAC_STATUS_IN_CAL_MODE) ) {					// if DAC is not in cal mode
				wDacData = 0;
				if ( (status & DAC_STATUS_ENABLED) ) {						// if DAC is enabled.
					state = dac_cpu_get_sensor_value( i, &fV );					// get sensor value.
					if ( !(state & SENSOR_STATE_IN_CAL_bm)) {					// if input sensor is NOT in cal mode.
						if ( SENSOR_STATE_OVERLOAD == state )						// if sensor is overload, then set DAC count to max.
							// wDacData = MAX_DAC_VALUE;
							wDacData = gawDacCountMaxSpanFNV[i];
						// else if sensor has valid or under range value, then translate it to DAC output.
						else if ( ( state > SENSOR_STATE_GOT_VALUE_IN_CAL ) && ( state <= SENSOR_STATE_MAX_FOR_DISPLAYABLE ) ) {
							wDacData = dac_cpu_value_to_dac_count( i, fV);
						}
					} // end if sensor is not in cal mode
				} // end if DAC enabled.
				dac_cpu_output( i, wDacData );
			} // end if DAC is not in cal mode.
		}
	}
}// end dac_cpu_translate_all_sensors_value()
*/

/**
 * It fetches the value of a sensor that associated with the specified channel.
 *
 * @param	n	-- DAC channel.
 * @param	pfV -- point to floating variable that save the sensor value.
 * @return	state of this sensor such as valid value, uncal, in cal, overload etc...
 *
 * History:  Created on 2010/03/19 by Wai Fai Chin
 */

BYTE	dac_cpu_get_sensor_value( BYTE n, float *pfV )
{
	BYTE	sn;
	BYTE	unit;
	BYTE	state;
	BYTE	valueType;

	*pfV = 0.0;
	state = FAILED;												// assumed failed.

	if ( n < MAX_DAC_CHANNEL ) {
		sn = gabDacSensorIdFNV[ n ];
		if ( sn < MAX_NUM_SENSORS ) {									// ensured sensor number is valid.
			valueType = gabDacSensorValueTypeFNV[ n ];
			unit = gabDacSensorUnitFNV[ n ];
			switch ( gabSensorTypeFNV[ sn ] )	{
				case SENSOR_TYPE_LOADCELL:
						state = loadcell_get_value_of_type( sn, valueType, unit, pfV );
					break;
				case SENSOR_TYPE_MATH_LOADCELL:
						state = vs_math_get_this_value( sn, valueType, unit, pfV );
					break;
				// TODO: SENSOR_TYPE_REMOTE
			} // end switch() {}
		}
	}
	return state;
} // end dac_cpu_get_sensor_value()

/**
 * It computes the DAC output resolution of a specified DAC channel.
 * resultion = MAX_DAC_VALUE / ( SensorValueMax - SensorValueMin)
 * It will be compute during initializing or after calibration.
 *
 * @param	channel	-- channel of a DAC,
 *
 * History:  Created on 2010/01/27 by Wai Fai Chin
 */

void dac_cpu_compute_resolution( BYTE channel )
{
	INT16	i16DacCountSpan;
	float fValueSpan;

	if ( channel < MAX_DAC_CHANNEL ) {
		fValueSpan = gafDacSensorValueAtMaxSpanFNV[ channel ] - gafDacSensorValueAtMinSpanFNV[ channel];
		// prevent divide by zero and too small of range.
		if ( fValueSpan < 1.0 )
			fValueSpan = MAX_DAC_VALUE;

		i16DacCountSpan = (INT16) gawDacCountMaxSpanFNV[ channel ] - (INT16) gawDacCountMinSpanFNV[ channel ];
		if ( i16DacCountSpan < 0 )
			i16DacCountSpan = -i16DacCountSpan;
		if ( i16DacCountSpan > MAX_DAC_VALUE )
			i16DacCountSpan = MAX_DAC_VALUE;

		gafDacOutputResolution[ channel ] = ((float) i16DacCountSpan) / fValueSpan;
	}
} // end dac_cpu_compute_resolution


/**
 * It converts a value to DAC count based on DAC channel configuration.
 *
 * @param	n	-- DAC channel.
 * @param	fV	-- value to be convert to DAC count based on DAC channel configuration.
 * @return	DAC count.
 *
 * History:  Created on 2010/05/18 by Wai Fai Chin
 */

UINT16	dac_cpu_value_to_dac_count( BYTE n, float fV )
{
	UINT16	wDacData;
	float	fDacData;

	if ( n < MAX_DAC_CHANNEL ) {
		fV = fV - gafDacSensorValueAtMinSpanFNV[ n ];
		fDacData = fV * gafDacOutputResolution[ n ];
		fDacData = fDacData + (float) gawDacCountMinSpanFNV[ n ];
		if ( float_lt_zero(fDacData) )
			wDacData = 0;
		else if ( fDacData > MAX_DAC_VALUE)
			wDacData = MAX_DAC_VALUE;
		else
			wDacData = (UINT16) fDacData;
	}
	else
		wDacData = 0;

	return wDacData;
} // end dac_cpu_value_to_dac_count()

#else
// CONFIG_USE_CPU == CONFIG_USE_ATMEGA1281:
// ATMEGA1281 does not have DAC, these just empty functions to interface with other modules such as cmdaction.c, nvmem.c, nv_cnfg_mem.c etc..
/**
 * Initialized CPU's DAC hardware
 *
 *
 * @note
 *
 * History:  Created on 2009/11/17 by Wai Fai Chin
 */

void dac_cpu_init_hardware( void )
{
}// end dac_cpu_init()

/**
 * Initialized all channels of CPU's DAC hardware
 *
 *
 * @note
 *
 * History:  Created on 2010/01/27 by Wai Fai Chin
 */

void dac_cpu_1st_init_all_channel( void )
{
	cmd_act_set_defaults( 0x30 );					// default high level DAC settings
	cmd_act_set_defaults( 0x32 );					// default internal DAC settings.
	dac_cpu_init_all_channel();
}// end dac_cpu_1st_init_all_channel()


/**
 * Initialized all channels of CPU's DAC hardware
 *
 *
 * @note
 *
 * History:  Created on 2010/01/27 by Wai Fai Chin
 */

void dac_cpu_init_all_channel( void )
{
	BYTE i;
	for ( i=0; i < MAX_DAC_CHANNEL; i++ ) {
		dac_cpu_init_settings(i);
	}
}// end dac_cpu_init_all_channel()

/**
 * Initialized CPU's DAC
 *
 *
 * @note
 *
 * History:  Created on 2009/11/17 by Wai Fai Chin
 */

void dac_cpu_init_settings( BYTE channel )
{
	dac_cpu_set_gain( channel, gabDacGainFNV[ channel ]);
	dac_cpu_set_offset( channel, gabDacOffsetFNV[ channel ] );
	// dac_cpu_compute_resolution( channel);
}// end dac_cpu_init_settings()


/**
 * Test CPU's DAC output.
 *
 *
 * @note
 *
 * History:  Created on 2009/11/19 by Wai Fai Chin
 */
UINT16 gTestDACA0_data = 0xFFF;
UINT16 gTestDACB0_data = 0x7FF;

void dac_cpu_test_output( void )
{
}// end dac_cpu_test()


/**
 * set data out to the specified DAC channel.
 *
 * @param	channel	-- channel of a DAC,
 * @param	data	-- data to be output to the specified DAC channel.
 *
 * @note
 *  channel 0 is DACA.ch0;
 *  channel 1 is DACB.ch0;
 *
 * History:  Created on 2010/01/25 by Wai Fai Chin
 */

void dac_cpu_output( BYTE channel, UINT16 data  )
{
}// end dac_cpu_output()


/**
 * set gain value of the specified DAC module.
 *
 * @param	channel	-- channel of a DAC,
 * @param	value	-- gain value the specified DAC channel.
 *
 * @note
 *  channel 0 is DACA.ch0;
 *  channel 1 is DACB.ch0;
 *
 * History:  Created on 2010/01/25 by Wai Fai Chin
 */

void dac_cpu_set_gain( BYTE channel, BYTE value  )
{
}// end dac_cpu_set_gain()


/**
 * set offset value of the specified DAC module.
 *
 * @param	channel	-- channel of a DAC,
 * @param	data	-- data to be output to the specified DAC channel.
 *
 * @note
 *  channel 0 is DACA.ch0;
 *  channel 1 is DACB.ch0;
 *
 * History:  Created on 2010/01/25 by Wai Fai Chin
 */

void dac_cpu_set_offset( BYTE channel, BYTE value  )
{
}// end dac_cpu_set_offset()


/**
 * walk through all DAC channel configuration data structure and output it based on its settings.
 *
 * DAC will output max voltage if a loadcell is overloaded, any other error will output 0V except warning messages.
 *
 * @note
 *  channel 0 is DACA.ch0;
 *  channel 1 is DACB.ch0;
 *  resultion = MAX_DAC_VALUE / ( maxSensorValueSpan - minSensorValueSpan )
 *
 * History:  Created on 2010/01/25 by Wai Fai Chin
 * 2011-01-10	-WFC- DAC outputs max voltage if a loadcell is overloaded, any other error will output 0V except warning messages.
 */
//TODO: handle input sensor error condition and output DAC based on error condition.

void dac_cpu_update_all_sensors_value( void  )
{
}// end dac_cpu_update_all_sensors_value()


/**
 * It fetches the value of a sensor that associated with the specified channel.
 *
 * @param	n	-- DAC channel.
 * @param	pfV -- point to floating variable that save the sensor value.
 * @return	state of this sensor such as valid value, uncal, in cal, overload etc...
 *
 * History:  Created on 2010/03/19 by Wai Fai Chin
 */

BYTE	dac_cpu_get_sensor_value( BYTE n, float *pfV )
{
	return FAILED;
} // end dac_cpu_get_sensor_value()

/**
 * It computes the DAC output resolution of a specified DAC channel.
 * resultion = MAX_DAC_VALUE / ( SensorValueMax - SensorValueMin)
 * It will be compute during initializing or after calibration.
 *
 * @param	channel	-- channel of a DAC,
 *
 * History:  Created on 2010/01/27 by Wai Fai Chin
 */

void dac_cpu_compute_resolution( BYTE channel )
{
} // end dac_cpu_compute_resolution


/**
 * It converts a value to DAC count based on DAC channel configuration.
 *
 * @param	n	-- DAC channel.
 * @param	fV	-- value to be convert to DAC count based on DAC channel configuration.
 * @return	DAC count.
 *
 * History:  Created on 2010/05/18 by Wai Fai Chin
 */

UINT16	dac_cpu_value_to_dac_count( BYTE n, float fV )
{
	return 0;
} // end dac_cpu_value_to_dac_count()

#endif
