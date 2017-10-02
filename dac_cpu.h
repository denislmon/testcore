/*! \file dac_cpu.h \brief hardware cpu's DAC functions. */
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
/// \ingroup driver_avr_cpu
/// \defgroup dac CPU DAC related function Library (dac_cpu.c)
/// \code #include "dac_cpu.h" \endcode
/// \par Overview
///  Hardware cpu's DAC  functions.
//
// ****************************************************************************
//@{

#ifndef MSI_DAC_CPU_H
#define MSI_DAC_CPU_H

#include "config.h"
#if (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 )
#include "dac_driver.h"
#endif

#include "sensor.h"

/// Maximum number of DAC
#define MAX_DAC_CHANNEL		2

/// Maximum offset value of the DAC
#define MAX_DAC_VALUE		0xFFF

// DAC count per volt on the 5V scale.
//#define DAC_COUNT_PER_VOLT	819.2

/// Maximum offset value of the DAC
#define MAX_DAC_OFFSET		63
#define MIN_DAC_OFFSET		-63

/// Maximum gain value of the DAC
#define MAX_DAC_GAIN		63
#define MIN_DAC_GAIN		-63

/// defined for gbDacCnfgStatusFNV
#define DAC_STATU_CALIBRATED	0X80
#define DAC_STATUS_ENABLED		0x40
#define DAC_STATUS_IN_CAL_MODE	0x20	// in calibration mode. This will never save to non volatile memory.
#define DAC_STATUS_MANUAL_MODE	0X10

#define DAC_CAL_TYPE_MAX_TYPE	5
#define DAC_CAL_TYPE_ABORT		0
#define DAC_CAL_TYPE_SAVE_EXIT	1
#define DAC_CAL_TYPE_OFFSET		2
#define DAC_CAL_TYPE_GAIN		3
#define DAC_CAL_TYPE_MIN_SPAN	4
#define DAC_CAL_TYPE_MAX_SPAN	5

#define	DAC_SOURCE_VALUE_TYPE_MASK		0x7F

/// DAC calibrated offset
extern	INT8	gabDacOffsetFNV[ MAX_DAC_CHANNEL ];

/// DAC calibrated Gain
extern	INT8	gabDacGainFNV[ MAX_DAC_CHANNEL ];

/// DAC count of min span point; e.g. 0 LB = 0.5V,
extern	UINT16	gawDacCountMinSpanFNV[ MAX_DAC_CHANNEL ];

/// DAC count of max span point; e.g. at Capacity = 5V,
extern	UINT16	gawDacCountMaxSpanFNV[ MAX_DAC_CHANNEL ];

/// DAC source Sensor Value at min span point.
extern	float	gafDacSensorValueAtMinSpanFNV[ MAX_DAC_CHANNEL ];

/// DAC source Sensor Value at max span point.
extern	float	gafDacSensorValueAtMaxSpanFNV[ MAX_DAC_CHANNEL ];

/// DAC source sensor value unit
extern	BYTE	gabDacSensorUnitFNV[ MAX_DAC_CHANNEL ];

/// DAC source sensor ID
extern	BYTE	gabDacSensorIdFNV[ MAX_DAC_CHANNEL ];

/// DAC source sensor value Type.  bit7 1==no filter, bit6:0 value type such as normal, total, peak.
extern	BYTE	gabDacSensorValueTypeFNV[ MAX_DAC_CHANNEL ];

/// DAC configuration status, bit7 1 =calibrated, bit6 1 = enable,
extern	BYTE	gabDacCnfgStatusFNV[ MAX_DAC_CHANNEL ];


/*!
  \brief DAC data structure.

* /

typedef struct  DAC_CAL_TAG
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
}DAC_CAL_T;
*/


/*!
  \brief CPU DAC data structure.

* /

typedef struct  DAC_CPU_TAG {
									/// azm threshold weight.
	float	azmThresholdWt;
									/// zero band high limit weight above cal zero that can be zero off if below this limit.
	float	zeroBandHiWt;
									/// zero band high limit weight below cal zero that can be zero off if above this limit.
	float	zeroBandLoWt;
									/// quarter countby weight, it is mainly for speedup execution in the expensive of memory space.
	float	quarterCBWt;
									/// azm Interval time.
	BYTE	azmIntervalTime;
}DAC_CPU_T;
*/


void 	dac_cpu_1st_init_all_channel( void );
void	dac_cpu_init_hardware( void );
void	dac_cpu_init_all_channel( void );
void	dac_cpu_init_settings( BYTE channel );
void	dac_cpu_output( BYTE channel, UINT16 data  );
void	dac_cpu_set_gain( BYTE channel, BYTE value  );
void	dac_cpu_set_offset( BYTE channel, BYTE value  );
void	dac_cpu_compute_resolution( BYTE channel );
void 	dac_cpu_update_all_sensors_value( void );

void	dac_cpu_test_output( void );
UINT16	dac_cpu_value_to_dac_count( BYTE n, float fV );

#endif
//@}
