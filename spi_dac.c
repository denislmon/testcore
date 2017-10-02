/*! \file spi_dac.c \brief SPI Analog Output Module implementation.*/
// ****************************************************************************
//
//                      MSI ScaleCore
//
//                  Copyright (c) 2009 by
//            Measurement Systems International
//                   Seattle, Washington
//                   All Rights Reserved
//
// Filename: spi_dac.c
// Hardware: ATMEL ATXMEGA192D3
// OS:       independent
// Compiler: avr-gcc
// Software layer:  Hardware
//
//  History:  Created on 2013-09-10 by Tim Alberts
//
// This module is designed to provide functions necessary to control the SPI
// Analog Output Module PCA#503530 and compatible hardware designed initially
// to be used in the MSI-7000/7001 hardware. The board is equipped with EEPROM
// and DAC devices that are individually selected.
//
// The primary interface is to select either the EEPROM, DAC, or NOP
//
// The EEPROM is intended for storing board identification and calibration or
// configuration constants.  This module is to be implemented at a later time.
//
// The DAC serves as both a standard analog output for weight measurements
// with functions that match what was done for the DSC analog output and the
// CHI-ALWS project using ScaleCore2.  It also serves as the mV/V module
// driver PCA#503546 when operating as a load cell cable replacement.
//
// ****************************************************************************


#include "spi_dac.h"


WORD16 gwSPIDacCtrlRegister;

///////////////////////////////////////////////////////////////////////////////
// PCA level interface functions
///////////////////////////////////////////////////////////////////////////////

/**
 * Set the board interface to talk to the EEPROM, DAC, or NOP.
 * The BDSEL line is always low on a 7000.
 *
 * @param ifc -- the desired SPI_DAC_INTERFACE_*
 *
 * History:  Created on 2013-09-10 by Tim Alberts
 */
void spidac_select_interface( const BYTE ifc ){
	if( ifc == SPI_DAC_INTERFACE_NOP ){
		// Set ID(A1) High, CS(A0) High
		HW7000_SPIDAC_A0CS_PORT |= ( 1 << HW7000_SPIDAC_A0CS_BIT);
		// TODO make sure PB6 is configured correctly and this is how to write it
		HW7000_SPIDAC_A1ID_PORT |= ( 1 << HW7000_SPIDAC_A1ID_BIT);
		return;
	}
#ifdef USING_SPIDAC_EEPROM
	if( ifc == SPI_DAC_INTERFACE_EEPROM ){
		// Set ID(A1) High, CS(A0) Low
		HW7000_SPIDAC_A0CS_PORT &= ~( 1 << HW7000_SPIDAC_A0CS_BIT);
		HW7000_SPIDAC_A1ID_PORT |= ( 1 << HW7000_SPIDAC_A1ID_BIT);
		return;
	}
#endif // USING_SPIDAC_EEPROM
	if( ifc == SPI_DAC_INTERFACE_DAC ){
		// Set ID(A1) Low, CS(A0) Low
		HW7000_SPIDAC_A0CS_PORT &= ~( 1 << HW7000_SPIDAC_A0CS_BIT);
		HW7000_SPIDAC_A1ID_PORT &= ~( 1 << HW7000_SPIDAC_A1ID_BIT);
		return;
	}
} // end spidac_select_interface()



///////////////////////////////////////////////////////////////////////////////
// EEPROM interface functions
///////////////////////////////////////////////////////////////////////////////
#ifdef USING_SPIDAC_EEPROM

/**
 *
 * History:  Created on 2013-09-10 by Tim Alberts
 */
void spidac_init_eeprom()
{
	//TODO
}

#endif // USING_SPIDAC_EEPROM

///////////////////////////////////////////////////////////////////////////////
// DAC interface functions
///////////////////////////////////////////////////////////////////////////////

/**
 * Typically called from cold start.
 *
 * Initialization method that writes the DAC configuration for our circuit.
 * At the end, the DAC is ready to operate, but the output is disabled until
 * the application sets the output moderange.
 *
 * Should allow 40us after a power-on reset for the chip to settle.
 *
 * History:  Created on 2013-09-10 by Tim Alberts
 */
void spidac_init_dac()
{
	// The PCA is designed to use an external RSET resistor for higher
	// 		precision. The chip has to be set to use this with the control
	//		register. Recommended set this when enabling the output.

	gwSPIDacCtrlRegister = SPI_ADC_REGISTER_CONTROL_DEFAULT;
	spidac_write_controlregister();

} // end spidac_init_dac()

/**
 * This is a private function used to write the AD5422 control register
 * with the current value of gwSPIDacCtrlRegister
 *
 * History:  Created on 2013-09-10 by Tim Alberts
 */
void spidac_write_controlregister()
{
	BYTE bdat;

	// configure SPI to talk to the DAC
	spidac_select_interface(SPI_DAC_INTERFACE_DAC);

	// select DAC control register
	display_spi_transceive(SPI_ADC_REGISTER_CONTROL);

	// write DAC global control register value
	bdat = gbSPIDacCtrlRegister >> 8; // MSByte
	display_spi_transceive(bdat);
	bdat = gbSPIDacCtrlRegister && 0xFF; // LSByte
	display_spi_transceive(bdat);

	// configure SPI not to talk to this board
	spidac_select_interface(SPI_DAC_INTERFACE_NOP);

} // spidac_write_controlregister()

/**
 * This method tells the chip (AD5422) to perform it's own internal reset.
 *
 * History:  Created on 2013-09-10 by Tim Alberts
 */
void spidac_reset()
{
	//TODO

} // end spidac_init_dac()

/**
 * Enable/Disable the DAC output for whatever the global mode value is currently
 * Uses the DAC control register...
 * Recommend using method spidac_set_output_moderange() to enable the DAC output
 * so the proper mode/range is selected when enabled.
 *
 * Note: Setting this bit selects the external current setting resistor (see
 * 		the AD5412/AD5422 Features section). When using an external current
 * 		setting resistor, it is recommended to only set REXT when also setting
 * 		the OUTEN bit.
 * Note: Output enable. This bit must be set to enable the outputs. The range
 * 		bits select which output is functional.
 *
 * @param enable -- ON/OFF
 *
 * History:  Created on 2013-09-10 by Tim Alberts
 */
void spidac_enable( const BYTE enable )
{
	enable ? gwSPIDacCtrlRegister |= SPI_ADC_REGISTER_CONTROL_ENABLE_MASK : gwSPIDacCtrlRegister &= ~SPI_ADC_REGISTER_CONTROL_ENABLE_MASK;
	spidac_write_controlregister();

} // spidac_enable()

/**
 *
 * @param ifc -- the desired SPI_DAC_INTERFACE_*
 *
 * History:  Created on 2013-09-10 by Tim Alberts
 */
#ifdef SPIDAC_SUPPORTS_TESTOUTPUT
void spidac_set_output_test( const BYTE enable )
{
	//TODO

} // end spidac_set_output_test()
#endif // SPIDAC_SUPPORTS_TESTOUTPUT

/**
 * Set the output mode (current/voltage) and range that the DAC
 * will use.
 * It is required that the output be disabled, then re-enabled
 * when setting the DAC mode according to the data sheet to
 * prevent output glitches.
 * This function takes care of disabling the output and re-enabling
 * it.
 * Recommend using this function to enable the DAC output as well.
 *
 * @param dacmoderange -- the desired SPI_DAC_MODERANGE_*
 *
 * History:  Created on 2013-09-10 by Tim Alberts
 */
void spidac_set_output_moderange( const BYTE dacmoderange )
{
	// disable output with OUTEN bit of the control register to prevent glitches
	spidac_enable(OFF);

	// clear the current moderange R2,R1,R0
	gwSPIDacCtrlRegister &= ~0x07; // same as SPI_DAC_MODERANGE_VOLTAGE_0to5

	//if( dacmoderange == SPI_DAC_MODERANGE_VOLTAGE_0to5 ){
	//}
	if( dacmoderange == SPI_DAC_MODERANGE_VOLTAGE_0to10 ){
		gwSPIDacCtrlRegister != SPI_DAC_MODERANGE_VOLTAGE_0to10;
	}
	if( dacmoderange == SPI_DAC_MODERANGE_VOLTAGE_pm5 ){
		gwSPIDacCtrlRegister != SPI_DAC_MODERANGE_VOLTAGE_pm5;
	}
	if( dacmoderange == SPI_DAC_MODERANGE_VOLTAGE_pm10 ){
		gwSPIDacCtrlRegister != SPI_DAC_MODERANGE_VOLTAGE_pm10;
	}
	if( dacmoderange == SPI_DAC_MODERANGE_CURRENT_4to20 ){
		gwSPIDacCtrlRegister != SPI_DAC_MODERANGE_CURRENT_4to20;
	}
	if( dacmoderange == SPI_DAC_MODERANGE_CURRENT_0to20 ){
		gwSPIDacCtrlRegister != SPI_DAC_MODERANGE_CURRENT_0to20;
	}
	if( dacmoderange == SPI_DAC_MODERANGE_CURRENT_0to24 ){
		gwSPIDacCtrlRegister != SPI_DAC_MODERANGE_CURRENT_0to24;
	}

	// enable output with OUTEN bit of the control register again
	spidac_enable(ON);

} // end spidac_set_output_moderange()

/**
 * Write the DAC (AD5422) data register with the specified value.
 *
 * @param data -- the 16 bit data value to be written to the DAC output.
 *
 * History:  Created on 2013-09-10 by Tim Alberts
 */
void spidac_write_dataregister( const WORD16 data )
{
	BYTE bdat;

	// configure SPI to talk to the DAC
	spidac_select_interface(SPI_DAC_INTERFACE_DAC);

	// select DAC data register
	display_spi_transceive(SPI_ADC_REGISTER_DATA);

	// write DAC data
	bdat = data >> 8; // MSByte
	display_spi_transceive(bdat);
	bdat = data && 0xFF; // LSByte
	display_spi_transceive(bdat);

	// configure SPI not to talk to this board
	spidac_select_interface(SPI_DAC_INTERFACE_NOP);

} // end spidac_write_dataregister()

