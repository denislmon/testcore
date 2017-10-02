/*! \file adc_cpu.h \brief internal CPU ADC related functions.*/
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
//  History:  Created on 2008/12/17 by Wai Fai Chin
// 
/// \ingroup driver_avr_cpu
/// \defgroup adc_cpu manage internal CPU ADC driver.(adc_cpu.c)
/// \code #include "adc_cpu.h" \endcode
/// \par Overview
///   This is a drive for internal CPU ADC module. It configures its input selection,
/// conversion speed and sensor filter algorithm based on the sensor descriptor
/// from sensor module. It does not know what type of sensor hooked up to it.
/// It just know how to read and supply ADC data to the specified sensor object.
//
// ****************************************************************************
//@{
 

#ifndef MSI_ADC_CPU_H
#define MSI_ADC_CPU_H

#include  "config.h"
#include "adc_driver.h"
#include "sensor.h"

/*!
   We are use the internal CPU ADC module.
	
*/


#define	ADC_CPU_MAX_CHANNEL	5


/*!
  \brief CPU ADC operation descriptor.
 feature bit7,6 definition.
	0 0 AREF, Internal VREF turned off
	0 1 AVCC with external capacitor at AREF pin
	1 0 Internal 1.0V Voltage Reference
	1 1 Internal Vcc/1.6 Voltage Reference

  \note It must initialize based on sensor descriptor first before use it.
    If corresponding sensor descriptor changed, the corresponding channel operation record must changed also.
*/

typedef struct  ADC_CPU_OP_DESC_TAG {
							/// sensor ID number, range 0 to (MAX_NUMBER_SENSOR - 1). 0xFF ==  no such sensor. 
	BYTE    sensorN;
							/// sensor status, Bit7 1==enabled; Bit6 AC excitation enabled, Bit5, excitation state 1==+, 0==-; Bit4-0 reserved
	BYTE	status;
							/// feature, Bit7,6 voltage reference, Bit5-0 reserved
	BYTE	feature;
}ADC_CPU_OP_DESC_T;

#define	ADC_CPU_SENSOR_STATUS_REFV_1V			0
#define	ADC_CPU_SENSOR_STATUS_REFV_VCC_DIV_1p6	0x01
#define	ADC_CPU_SENSOR_STATUS_REFV_EXTERNAL		0x02		// 2013-06-14 -WFC-
#define	ADC_CPU_SENSOR_STATUS_REFV_MASK			0xFC

#define ADC_CPU_INTERVAL_READ		0
#define ADC_CPU_READ_NOW			1

//	gaLSensorDescriptor[sn].status &= ADC_CPU_SENSOR_STATUS_REFV_MASK;
//	gaLSensorDescriptor[sn].status |= ADC_CPU_SENSOR_STATUS_REFV_VCC_DIV_1p6;		// Vcc /1.6 V ==> ref V.

extern BYTE gbCPU_ADCA_offset;
extern BYTE gbCPU_ADCB_offset;

void	adc_cpu_1st_init( void );
void	adc_cpu_init( void );
// void	adc_cpu_update( void );
void	adc_cpu_update( BYTE readItNow );
void	adc_cpu_construct_op_desc( LSENSOR_DESCRIPTOR_T *pSD, BYTE sensorN );
void 	adc_cpu_setup_channel( BYTE sn );

#endif  // MSI_ADC_CPU_H

//@}
