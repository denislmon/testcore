/*! \file setpoint.c \brief setpoint related functions.*/
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
//  History:  Created on 2009/06/22 by Wai Fai Chin
//
//  NOTE:
//		raw_weight   = scale_factor * filtered_Adc_count;
//		zero_weight  = user selected pure raw_weight;
//		gross_weight = raw_weight - zero_weight;
//		net_weight   = gross_weight - tare_weight;
//		gross_wieght, net_weight and tare_weight are rounded based on countby.
//
// ****************************************************************************
 
#include  "setpoint.h"
#include  "sensor.h"
#include  "loadcell.h"
#include  "cmdparser.h"

											/// setpoint trigger state registry, bit2 is setpoint 2, 0==untrip, 1== tripped. bit0 is setpoint 0, bit15 to bit3 are reserved for future expansion.
UINT16	gSP_Registry;
											/// setpoint source data from a given sensor ID.
BYTE	gaSP_sensorID_FNV[ SETPOINT_MAX_NUM ];
											/// cmp logic and value mode. bit 7== 0=disabled, 1== enabled, bit 6 to 4 compare logic, 1== '<', 2== '>'; bit 3 to 0; value mode; 0 == net_gross, 1== gross, 2==total. 
BYTE	gaSP_cmp_logic_value_modeFNV[ SETPOINT_MAX_NUM ];
											/// setpoint hysteresis in countby.
BYTE	gaSP_hysteresisCB_FNV[ SETPOINT_MAX_NUM ];
											/// setpoint compare value. The unit is implied the same as gabSensorShowCBunitsFNV. It is a referenct value. It cannot changed when changed gabSensorViewUnitsFNV.
float	gaSP_fCmpValueFNV[ SETPOINT_MAX_NUM ];

				// 2010-10-07 -WFC-			/// setpoint compare value. The unit is the same as gabSensorViewUnitsFNV. It is converted gaSP_fCmpValueFNV base on the gabSensorViewUnitsFNV unit.
float	gaSP_fCmpValue[ SETPOINT_MAX_NUM ];

/**
 * It initialized all setpoints.
 *
 * @return none
 *
 * History:  Created on 2009/06/23 by Wai Fai Chin
 */

void  setpoint_init_all( void )
{
	gSP_Registry = 0;
	setpoint_unit_conversion_all();
} // end setpoint_init_all()


/**
 * It converted all setpoint value based on its sensor viewing unit.
 *
 * @return none
 *
 * History:  Created on 2010-10-07 by Wai Fai Chin
 */

void  setpoint_unit_conversion_all( void )
{
	BYTE	spNum;

	for ( spNum = 0; spNum < SETPOINT_MAX_NUM; spNum++) {
		setpoint_unit_conversion( spNum );
	}
} // end setpoint_unit_conversion_all()


/**
 * It converted a setpoint value based on its sensor viewing unit.
 *
 * @param  spNum	-- setpoint number.
 * @return none
 *
 * History:  Created on 2010-10-07 by Wai Fai Chin
 */

void  setpoint_unit_conversion( BYTE spNum )
{

	BYTE		status;
	BYTE		lc;
	float		unitCnv;

	status = CMD_STATUS_ERROR_INDEX;			// assumed bad.
	if ( spNum < SETPOINT_MAX_NUM ) {
		lc = gaSP_sensorID_FNV[ spNum ];
		if ( lc < MAX_NUM_LSENSORS ) {
			status = CMD_STATUS_NO_ERROR;
			if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[lc] ||
				 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[lc] 	) {
				// convert reference setpoint values from reference unit to current viewing unit.
				memcpy_P ( &unitCnv,  &gafLoadcellUnitsTbl[ gabSensorShowCBunitsFNV[lc] ][ gabSensorViewUnitsFNV[lc] ], sizeof(float));
				gaSP_fCmpValue[ spNum ] = gaSP_fCmpValueFNV[ spNum ] * unitCnv;
			}
		}
	}
	return status;

} // end setpoint_unit_conversion()



/**
 *
 * This function ensure that the setpoint comparing value is less than the capacity
 * if the sensor is loadcell type and has a completed calibrated table.
 * 
 * @param  spNum	-- setpoint number.
 *
 * @return 0 error if it have valid comparing value, else CMD_STATUS_ERROR_INDEX or CMD_STATUS_OUT_RANGE_INPUT.
 *
 * History:  Created on 2009-06-23 by Wai Fai Chin
 * 2010-10-07 -WFC- validation based on reference viewing capacity instead of dynamic viewing capacity.
 *
 */
BYTE setpoint_value_validation( BYTE spNum )
{
	BYTE		status;
	BYTE		lc;
	LOADCELL_T	*pLc;
	
	status = CMD_STATUS_ERROR_INDEX;			// assumed bad.
	if ( spNum < SETPOINT_MAX_NUM ) {
		lc = gaSP_sensorID_FNV[ spNum ];
		if ( lc < MAX_NUM_LSENSORS ) {
			status = CMD_STATUS_NO_ERROR;
			if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[lc] ||
				 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[lc] 	) {
				pLc = &gaLoadcell[ lc ];
				if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[lc]) {
					// 2010-10-07 -WFC- if ( CAL_STATUS_COMPLETED == pLc-> pCal-> status ){			// if this loadcell has a completed calibration table.
					if ( CAL_STATUS_COMPLETED == gaLoadcell[ lc ].pCal-> status ){		// if this loadcell has a completed calibration table. // 2010-10-07 -WFC-
						// 2010-10-07 -WFC- if ( float_a_gt_b( gaSP_fCmpValueFNV[ spNum ], pLc-> viewCapacity) )
						if ( float_a_gt_b( gaSP_fCmpValueFNV[ spNum ], gafSensorShowCapacityFNV[lc]) ) { // 2010-10-07 -WFC-
							status = CMD_STATUS_OUT_RANGE_INPUT;
						}
					}
				}
				else if (SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[lc] 	) {
					// 2010-10-07 -WFC-  if ( float_a_gt_b( gaSP_fCmpValueFNV[ spNum ], pLc-> viewCapacity) )
					if ( float_a_gt_b( gaSP_fCmpValueFNV[ spNum ], gafSensorShowCapacityFNV[lc]) ) { // 2010-10-07 -WFC-
						status = CMD_STATUS_OUT_RANGE_INPUT;
					}
				}
			}
		}
	}
	return status;
}// end setpoint_value_validation()


/**
 *
 * This function process all setpoints.
 * 
 * @param  none.
 *
 * @return none.
 *
 * History:  Created on 2009-06-23 by Wai Fai Chin
 * 2010-10-07 -WFC- comparing setpoint value of current viewing unit.
 * 2011-07-13 -WFC- Added codes to handle comparison with Total Count set point value.
 * 2014-10-16 -WFC- Added codes to handle comparison with lift count set point value.
 */

void setpoint_process_all( void )
{
	BYTE					status;
	BYTE					spNum;
	BYTE					cmpLogicVMode;
	BYTE					mode;
	BYTE					bV;
	BYTE					sn;				// sensor number
	BYTE					trigger;
	UINT16					bitMask;
	LOADCELL_T				*pLc;
	LSENSOR_DESCRIPTOR_T	*pSnDesc;		// points to local sensor descriptor
	float					fCmpV;			// setpoint comparison value
	float					fHystV;			// setpoint hysteresis value
	UINT16					wSetPointV;		// 16bit unsigned integer setpoint value.
	UINT32					uI32SetPointV;	// 2014-10-20 -WFC-

	
	bitMask = 1;
	for ( spNum = 0; spNum < SETPOINT_MAX_NUM; spNum++) {
		cmpLogicVMode = gaSP_cmp_logic_value_modeFNV[ spNum ];
		trigger = FALSE;
		if ( SP_CMP_ENABLED_BIT & cmpLogicVMode )	{			// if enabled.
			sn = gaSP_sensorID_FNV[ spNum ];
			if ( sn < MAX_NUM_LSENSORS )	{					// ensured sn within bound.
				pSnDesc = &gaLSensorDescriptor[ sn ];
				mode = cmpLogicVMode & SP_CMP_VALUE_MODE_MASK;			// 2011-07-13 -WFC-
				if ( SENSOR_TYPE_LOADCELL == gabSensorTypeFNV[sn] ||
					 SENSOR_TYPE_MATH_LOADCELL == gabSensorTypeFNV[sn] ) {		// if sensor is a loadcell, comparison value is based on the valueMode.
					if ( sn < MAX_NUM_PV_LOADCELL )
						pLc = &gaLoadcell[ sn ];
					// 2011-07-13 -WFC- mode = cmpLogicVMode & SP_CMP_VALUE_MODE_MASK;
					switch ( mode ) {
						case SP_CMP_VALUE_MODE_NET_GROSS:
								if ( LC_OP_MODE_NET_GROSS & *(pLc-> pOpModes) )		// if loadcell is in NET mode
									fCmpV =	pLc-> netWt;
								else
									fCmpV =	pLc-> grossWt;
							break;
						case SP_CMP_VALUE_MODE_GROSS:
									fCmpV =	pLc-> grossWt;
							break;
						case SP_CMP_VALUE_MODE_TOTAL:
									fCmpV =	*(pLc-> pTotalWt);
							break;
					}

					// 2011-07-13 -WFC- v
					if ( SP_CMP_VALUE_MODE_TOTAL_CNT == mode ) {		// 2011-07-13 -WFC-
						wSetPointV = (UINT16) gaSP_fCmpValue[ spNum ];		//  setpoint value is Total Count.
						bV = cmpLogicVMode & SP_CMP_LOGIC_MASK;
						if ( ((SP_CMP_LOGIC_GREATER_THAN == bV) && ( *(pLc->pNumTotal) > wSetPointV )) ||
							 ((SP_CMP_LOGIC_LESS_THAN == bV) && ( *(pLc->pNumTotal) < wSetPointV ) ) ) {
							trigger = TRUE;
						}
					}
					// 2011-07-13 -WFC- ^
					// 2014-10-16 -WFC- v
					else if ( SP_CMP_VALUE_MODE_LIFT_CNT == mode ) {
						uI32SetPointV = (UINT32) gaSP_fCmpValue[ spNum ];		//  setpoint value is Total Count.
						bV = cmpLogicVMode & SP_CMP_LOGIC_MASK;
						if ( ((SP_CMP_LOGIC_GREATER_THAN == bV) && ( gaulUserLiftCntFNV[ sn ] > uI32SetPointV )) ||
							 ((SP_CMP_LOGIC_LESS_THAN == bV) && ( gaulUserLiftCntFNV[ sn ] < uI32SetPointV ) ) ) {
							trigger = TRUE;
						}
					}
					// 2014-10-16 -WFC- ^

					else {
						fCmpV = float_round( fCmpV, pLc-> viewCB.fValue);
						mode = cmpLogicVMode & SP_CMP_LOGIC_MASK;
						bV = gaSP_hysteresisCB_FNV[ spNum ];
						//if ( bV ) {									// if this setpoint has a specified hysteresis
						 if ( bV  && gSP_Registry & bitMask  ) {		// Use hysteresis if triggered // PHJ
							fHystV = (float) bV * pLc-> viewCB.fValue;
							if ( SP_CMP_LOGIC_GREATER_THAN == mode )
								fCmpV += fHystV;
							else
								fCmpV -= fHystV;
						}
					}
				}
				else {
					fCmpV = pSnDesc-> value;
				}

				// if ( SP_CMP_VALUE_MODE_TOTAL_CNT != mode ) {		// 2011-07-13 -WFC-
				if ( !(SP_CMP_VALUE_MODE_TOTAL_CNT == mode || SP_CMP_VALUE_MODE_LIFT_CNT == mode) ) {		// 2014-10-16 -WFC-
					// 2010-10-07 -WFC- fHystV = gaSP_fCmpValueFNV[ spNum ];	// now the fHystV is the user specified setpoint comparision value.
					fHystV = gaSP_fCmpValue[ spNum ];							// now the fHystV is the user specified setpoint comparision value. 2010-10-07 -WFC-
					mode = cmpLogicVMode & SP_CMP_LOGIC_MASK;
					if ( ((SP_CMP_LOGIC_GREATER_THAN == mode) && ( fCmpV > fHystV )) ||
						 ((SP_CMP_LOGIC_LESS_THAN == mode) && ( fCmpV < fHystV ) ) ) {
						trigger = TRUE;
					}
				}
			} // end if ( sn < MAX_NUM_LSENSORS )	{}
		}
		
		if ( trigger ) {
			gSP_Registry |= bitMask;
		}
		else {
			gSP_Registry &= ~bitMask;
		}
		
		bitMask <<=1;	// next setpoint bit.
	}
	return status;
}// end setpoint_value_validation()

