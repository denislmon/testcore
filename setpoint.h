/*! \file setpoint.h \brief setpoint related functions.*/
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
/// \ingroup Application
/// \defgroup setpoint setpoint module (setpoint.c)
/// \code #include "setpoint.h" \endcode
/// \par Overview
///	\code
///
///  NOTE:
///		raw_weight   = scale_factor * filtered_Adc_count;
///		zero_weight  = user selected pure raw_weight;
///		gross_weight = raw_weight - zero_weight;
///		net_weight   = gross_weight - tare_weight;
///		gross_wieght, net_weight and tare_weight are rounded based on countby.
///	\endcode
// ****************************************************************************
//@{
 

#ifndef MSI_SETPOINT_H
#define MSI_SETPOINT_H
#include  "config.h"


/// maxium number of setpoints.
#define SETPOINT_MAX_NUM		8		// changed from 3 to 8; 2015-09-02 -WFC-

#define	SP_CMP_ENABLED_BIT			0X80
#define	SP_CMP_LOGIC_MASK			0X70
#define	SP_CMP_LOGIC_LESS_THAN		0X20
#define	SP_CMP_LOGIC_GREATER_THAN	0X10
//#define	SP_CMP_VALUE_MODE_MASK		0X0F
#define	SP_CMP_VALUE_MODE_MASK		0X07	// 2015-09-09 -WFC-
#define	SP_CMP_VALUE_MODE_NET_GROSS	0X00
#define	SP_CMP_VALUE_MODE_GROSS		0X01
#define	SP_CMP_VALUE_MODE_TOTAL		0X02
#define	SP_CMP_VALUE_MODE_TOTAL_CNT	0X03		// 2011-06-29 -WFC-
#define	SP_CMP_VALUE_MODE_LIFT_CNT	0X04		// 2014-10-16 -WFC-


/// bit field for gSP_Registry
#define	SP_LATCH_STATE_1			0x01
#define	SP_LATCH_STATE_2			0x02
#define	SP_LATCH_STATE_3			0x04

											/// setpoint trigger state registry, bit2 is setpoint 2, 0==untrip, 1== tripped. bit0 is setpoint 0, bit15 to bit3 are reserved for future expansion.
extern	UINT16	gSP_Registry;


/* !
  \brief setpoint data structure.
* /
typedef struct  SETPOINT_TAG {
							/// sensor ID.
	BYTE	sensorID;			
							/// cmp logic and value mode. bit 7== 0=disabled, 1== enabled, bit 6 to 4 compare logic, 2== '<', 1== '>'; bit 3 to 0; value mode; 0 == net_gross, 1== gross, 2==total.
	BYTE	cmp_logic_valu_mode;			
							/// hysteresis in countby.
	BYTE	hysteresisCB;			
							/// value to be compare with.
	float	fVcmp;			
}SETPOINT_T;
*/

// make this into arrays instead of structure is for easy host command interface. It also use less code memory too.
											/// setpoint source data from a given sensor ID.
extern	BYTE	gaSP_sensorID_FNV[ SETPOINT_MAX_NUM ];
											/// cmp logic and value mode. bit 7== 0=disabled, 1== enabled, bit 6 to 4 compare logic, 2== '<', 1== '>'; bit 3 to 0; value mode; 0 == net_gross, 1== gross, 2==total.
extern	BYTE	gaSP_cmp_logic_value_modeFNV[ SETPOINT_MAX_NUM ];
											/// setpoint hysteresis in countby.
extern	BYTE	gaSP_hysteresisCB_FNV[ SETPOINT_MAX_NUM ];
											/// setpoint compare value.
extern	float	gaSP_fCmpValueFNV[ SETPOINT_MAX_NUM ];



void	setpoint_init_all( void );
void	setpoint_process_all( void );
BYTE	setpoint_value_validation( BYTE spNum );

void	setpoint_unit_conversion( BYTE spNum );
void	setpoint_unit_conversion_all( void );


/** \def setpoint_1st_init
 * Initialize setpoint configuration the very first when the application run.
 *
 * @param	none.
 *
 * History:  Created on 2009/06/23 by Wai Fai Chin
 */
#define setpoint_1st_init()		cmd_act_set_defaults( 0x2C )


#endif		// end MSI_SETPOINT_H
//@}

