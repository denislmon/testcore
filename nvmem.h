/*! \file nvmem.h \brief none volatile memory related functions.*/
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
// Software layer:  Application and Middle layer
//
//  History:  Created on 2007/08/06 by Wai Fai Chin
// 
/// \ingroup Application
/// \defgroup nvmem Nvmem manages none volatile memory such as EEPROM, FLASH etc... (nvram.c)
/// \code #include "nvmem.h" \endcode
/// \par Overview
///   It allocated global none volatile memory variables.
/// The none volatile memory could be EEPROM, flash or other none volatile memory device.
/// This module hides all of the complexity of using none volatile memory devices.
/// It uses macro define to hide the hardware specific non volatile memory functions.
///
/// It saves system configuration like calibration, output mode, etc...
/// Sensors cal tables are saved in EEMEM only. The rest of settings are save in FRAM
/// because it allow user keeps change and save as often as they want without
/// degrade the EEMEM.
//
// ****************************************************************************
//@{


#ifndef MSI_NVRAM_H
#define MSI_NVRAM_H

#include "config.h"

// current avr_lib do not support eeprom for the Xmega128A1, once the future avr_lib support eeprom, we may removed the EEMEM macro define.
#if (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 )
	#define  EEMEM __attribute__((section(".eeprom")))
#endif

#define  FLASH_DATA_MEM		1
#define  FLASH_PROG_MEM		0
#define  FLASH_PAGE_SIZE	128
#define  NVRAM_READ_PASS	0
#define  NVRAM_WRITE_PASS	0
#define	 NVRAM_NV_MEMORY_FAIL	0xED	//ALL Non volatile memory failed included EEMEM and FRAM.
#define  NVRAM_EEMEM_FAIL	0XEE
#define  NVRAM_FRAM_FAIL	0XEF
#define  NVRAM_WRITE_FAIL	255

#define  NVRAM_CNFG_SAVED	0x55

#if ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )

#include  "calibrate.h"
#include  "sensor.h"
#include  "dataoutputs.h"
#include  "setpoint.h"
#include  "eeprm.h"
#include  "nv_cnfg_mem.h"
#include  "dac_cpu.h"

#if (  CONFIG_USE_CPU == CONFIG_USE_ATXMEGA128A1 )
#define  nvmem_is_ready()   (!(NVM.STATUS & NVM_NVMBUSY_bm))

/** \def nvmem_write_byte
 * write a byte to none volatile memory device.
 * 
 * @param  nvmemAddr -- none volatile memory address to be write to.
 * @param      value -- value of a byte to be write to none volatile memory.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define  nvmem_write_byte( nvmemAddr, value)	eeprm_write_byte( (UINT16) (nvmemAddr), (BYTE)(value) )

/** \def nvmem_read_byte
 * read a byte from none volatile memory.
 * 
 * @param nvmemAddr -- none volatile memory address of a content to be read.
 * @return a byte of data.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define  nvmem_read_byte( nvmemAddr )		eeprm_read_byte ((UINT16)(nvmemAddr))

/** \def nvmem_read_byte_timeout
 * read a byte from none volatile memory device with timeout detection.
 * 
 * @param nvmemAddr -- none volatile memory address of a content to be read.
 * @param ch -- points at the destination byte.
 * @param status return false if none volatile memory device failed.
 *         true if it successes.
 * @note
 *  This is intent for use in beginning of the main function to check if 
 *  the none volatile memory device is working.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define	nvmem_read_byte_timeout( nvmemAddr, ch)		eeprm_read_byte_timeout ( (BYTE *)(nvmemAddr), (BYTE *)(ch))

/** \def nvmem_read_block
 * read a block of data from none volatile memory device.
 * 
 * @param pDest -- points to destination in RAM.
 * @param pSrc  -- points to source of data in none volatile memory.
 * @param n     -- number of bytes to read from none volatile memory.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define nvmem_read_block( pDest, pSrc, n)   eeprm_read_block ( (BYTE *)(pDest), (UINT16)(pSrc), (UINT16)(n) )

/** \def nvmem_write_block
 * write a block of data to none volatile memory device.
 * 
 * @param pDest -- points to destination in none volatile memory where data to be write to.
 * @param pSrc  -- points to source of data from RAM
 * @param n     -- number of bytes to write to none volatile memory.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define  nvmem_write_block(pDest, pSrc, n)  eeprm_write_block ( (BYTE *)(pSrc), (UINT16)(pDest), (UINT16)(n) )

#else

#define  nvmem_is_ready() eeprom_is_ready()

/** \def nvmem_write_byte
 * write a byte to nonevolatile memory device.
 *
 * @param  nvmemAddr -- nonevolatile memory address to be write to.
 * @param      value -- value of a byte to be write to nonevolatile memory.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define  nvmem_write_byte( nvmemAddr, value)   eeprom_write_byte ((uint8_t *)(nvmemAddr), (uint8_t)(value))

/** \def nvmem_read_byte
 * read a byte from nonevolatile memory.
 *
 * @param nvmemAddr -- nonevolatile memory address of a content to be read.
 * @return a byte of data.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define  nvmem_read_byte( nvmemAddr )   eeprom_read_byte ((uint8_t *)(nvmemAddr))

/** \def nvmem_write_word
 * write a word to nonevolatile memory.
 *
 * @param nvmemAddr -- nonevolatile memory address to be write to.
 * @param    wValue  -- value of a word to be write to nonevolatile memory.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define  nvmem_write_word( nvmemAddr, wValue ) eeprom_write_word ((uint16_t *) (nvmemAddr), (uint16_t) (wValue))


/** \def nvmem_read_word
 * read a word from nonevolatile memory.
 *
 * @param nvmemAddr -- nonevolatile memory address of a content to be read.
 * @return  a word of data.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define  nvmem_read_word( nvmemAddr )	eeprom_read_word ((uint16_t *)(nvmemAddr))


/** \def nvmem_read_byte_timeout
 * read a byte from nonevolatile memory device with timeout detection.
 *
 * @param nvmemAddr -- nonevolatile memory address of a content to be read.
 * @param ch -- points at the destination byte.
 * @param status return false if nonevolatile memory device failed.
 *         true if it successed.
 * @note
 *  This is intent for use in beginning of the main function to check if
 *  the nonevolatile memory device is working.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */

#define	nvmem_read_byte_timeout( nvmemAddr, ch)		eeprm_read_byte ( (BYTE *)(nvmemAddr), (BYTE *)(ch))


/** \def nvmem_read_block
 * read a block of data from nonevolatile memory device.
 *
 * @param pDest -- points to destination in RAM.
 * @param pSrc  -- points to source of data in nonevolatile memory.
 * @param n     -- number of bytes to read from nonevolatile memory.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define nvmem_read_block( pDest, pSrc, n)  eeprom_read_block ( (void *)( pDest),(const void *) (pSrc), (size_t)(n))


/** \def nvmem_write_block
 * write a block of data to nonevolatile memory device.
 *
 * @param pDest -- points to destination in nonevolatile memory where data to be write to.
 * @param pSrc  -- points to source of data from RAM
 * @param n     -- number of bytes to write to nonevolatile memory.
 *
 * History:  Created on 2007/08/21 by Wai Fai Chin
 */
#define  nvmem_write_block(pDest, pSrc, n)  eeprom_write_block ((const void *)(pSrc), (void *) (pDest), (size_t)(n))

#endif

#endif //  ( CONFIG_COMPILER == CONFIG_USE_AVR_GCC )

BYTE	nvmem_save_all_essential_config_eemem( void );
BYTE	nvmem_recall_all_essential_config_eemem( void );

BYTE	nvmem_save_all_essential_config_fram( void );
BYTE	nvmem_recall_all_essential_config_fram( void );

BYTE	nvmem_save_all_config( void );
BYTE	nvmem_recall_all_config( void );

BYTE	nvmem_save_all_loadcell_statistics_fram( void );
BYTE	nvmem_recall_all_loadcell_statistics_fram( void );

BYTE	nvmem_save_all_loadcell_statistics_fram( void );
BYTE	nvmem_recall_all_loadcell_statistics_fram( void );
void	nvmem_default_all_loadcell_statistics_fram( void );

void	nvmem_default_all_dac_config_fram( void );
BYTE	nvmem_recall_all_dac_config_fram( void );
BYTE	nvmem_save_all_dac_config_fram( void );

//void	nvmem_default_all_vs_math_config_fram( void );
BYTE	nvmem_recall_all_vs_math_config_fram( void );
BYTE	nvmem_save_all_vs_math_config_fram( void );
BYTE	nvmem_check_memory( void );


BYTE 	nvmem_save_all_lc_total_motion_opmode_config_fram( void );
BYTE	nvmem_recall_all_lc_total_motion_opmode_config_fram( void );

BYTE	nvmem_save_all_lc_standard_mode_azm_config_fram( void );
BYTE	nvmem_recall_all_lc_standard_mode_azm_config_fram( void );

BYTE	nvmem_recall_a_loadcell_statistics_fram( BYTE lc );
BYTE	nvmem_save_a_loadcell_statistics_fram( BYTE lc );

void 	nvmem_default_all_sensor_viewing_cb_fram( void );  // 2010-09-10 -WFC-

#define CONFIG_TEST_NV_MEM_MODULE FALSE
#if  ( CONFIG_TEST_NV_MEM_MODULE == TRUE)
void nvmem_test_eeprom( void );
#endif // ( CONFIG_TEST_NV_MEM_MODULE == TRUE)



#endif	// end MSI_NVRAM_H

//@}
