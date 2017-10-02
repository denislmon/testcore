/*! \file vs_sensor.c \brief sensors related functions.*/
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
// Software layer:  Application
//
//  History:  Created on 2007/08/01 by Wai Fai Chin
// 
// Description: 
//   It is a high level module manages all aspect of different type of sensors.
//   It knows the high level work flow and delegates the detail tasks to a specific sensor type module.
//   It has an abstract sensor descriptors prescribed what type of ADC chip, input selection,
//   conversion speed and sensor filter algorithm.  Each specific sensor ADC value is filled by sensor filter algorithm
//   which will be call by its ADC module. The sensor value is also supplied by the specified sensor module.
//   The descriptor contains an abstract device pointer pDev to a specific sensor type data structure, e.g. LOADCELL_T,
//   VOLTAGEMON_T etc... This makes easy for sensor module to compute the value and format output data.
//   It has method to initialize all sensor types based on gabSensorTypeFNV[], gabSensorFeaturesFNV[] and
//   gabSensorSpeedFNV[].
//
// Note
//  The Scalecore can have up to 4 loadcells. Only the first two sensors
//  (sensor0 and sensor1) are for loadcells with software temperature compensation.
//  Sensors 0 to 3 are loadcells. Sensors 4 to 6 are battery monitor.
//  Sensor 7 is a temperature sensor. Sensor 8 is a light sensor.
//  Note that sensor 7 and 8 do not require calibration.
//
//  Sensor 9 to 15 are virtual sensors such as remote sensors, math sensors, etc.. 
//  For remote sensor, it will not allow to calibrate with remote sensor ID;
//  instead, user must uses device ID and local sensor ID with cal commands and
//  talk to remote device directly ( may be indirectly by router).
//  
//  The first sensor loadcell will get temperature data from sensor 7 by default.
//  Each software temperature compensated loadcell will have 3 calibration tables corresponding
//  to 3 different temperature level.
//
// Threshold weight values will be saved in the same unit as the cal unit.
// If the user enter the value in unit different than cal unit, it will do
// converted entered value to cal unit and then saved it; these values have suffix of NV.
// The tare, zero, total, Sum square total etc... weight values are saved in display unit;
// these values have the suffix of FNV.
//
// ****************************************************************************
 
#include  "config.h"
#include  "vs_sensor.h"


VS_SENSOR_INFO_T	gVsSensorInfo;

