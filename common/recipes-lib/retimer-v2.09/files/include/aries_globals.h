/*
 * Copyright 2020 Astera Labs, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file aries_globals.h
 * @brief Definition of enums and structs globally used by the SDK.
 */

#ifndef ASTERA_ARIES_SDK_GLOBALS_H
#define ASTERA_ARIES_SDK_GLOBALS_H

///////////////////////////////////////
////////// EEPROM parameters //////////
///////////////////////////////////////

/** EEPROM Page Count */
#define ARIES_EEPROM_PAGE_COUNT 1024
/** EEPROM Page Size */
#define ARIES_EEPROM_PAGE_SIZE 256
/** Max Burst Size */
#define ARIES_MAX_BURST_SIZE 256
/** EEPROM MM block write size */
#define ARIES_EEPROM_BLOCK_WRITE_SIZE_WIDE 16
#define ARIES_EEPROM_BLOCK_WRITE_SIZE_NOWIDE 32
#define ARIES_EEPROM_BLOCK_WRITE_SIZE_MAX 32           // 32 bytes max
#define ARIES_EEPROM_BLOCK_WRITE_SIZE_NUM_BLOCKS_MAX 8 // eight 4-byte blocks
/** EEPROM MM data block base address */
#define ARIES_EEPROM_BLOCK_BASE_ADDR_WIDE 0x410
#define ARIES_EEPROM_BLOCK_BASE_ADDR_NOWIDE 0x88e7
/** EEPROM MM command modifier code */
#define ARIES_EEPROM_BLOCK_CMD_MODIFIER_WIDE 0x0
#define ARIES_EEPROM_BLOCK_CMD_MODIFIER_NOWIDE 0x80
/** EEPROM MM block checksum write size */
#define ARIES_EEPROM_MM_BLOCK_CHECKSUM_WRITE_SIZE 8

/** Total EEPROM size (in bytes) */
#define ARIES_EEPROM_MAX_NUM_BYTES 262144

/** Value indicating FW was loaded properly */
#define ARIES_LOAD_CODE 0xe

/** Time delay between checking MM status of EEPROM write (microseconds) */
// #define ARIES_MM_STATUS_TIME 1000
#define ARIES_MM_STATUS_TIME 5000
/** Time between resetting command byte in EEPROM read */
#define ARIES_I2C_MASTER_CMD_RST 2000
// #define ARIES_I2C_MASTER_CMD_RST 1000
/** Delay after writing data in rewrite and re-verify step */
#define ARIES_I2C_MASTER_WRITE_DELAY 50000
// #define ARIES_I2C_MASTER_WRITE_DELAY 5000
/** Time delay after sending EEPROM read command to Main Micro */
#define ARIES_MM_READ_CMD_WAIT 10000
/** Time allocated to calculate checksum (secs) */
#define ARIES_MM_CALC_CHECKSUM_WAIT 6
/** Time allocated to calculate checksum (micro secs) */
#define ARIES_MM_CALC_CHECKSUM_TRY_TIME 10000
/** Time allocated for Rx adaptation (microseconds) */
#define ARIES_PIPE_RXEQEVAL_TIME_US 100000
/** Time allocated for PMA register access to complete (microseconds) */
#define ARIES_PMA_REG_ACCESS_TIME_US 100

/** Num banks per EEPROM (num slaves) */
#define ARIES_EEPROM_NUM_BANKS 4
/** EEPROM Bank Size (num slaves) */
#define ARIES_EEPROM_BANK_SIZE 0x10000

/** Main Micro codes for writing and reading EEPROM via MM-assist */
#define ARIES_MM_EEPROM_WRITE_REG_CODE 1
#define ARIES_MM_EEPROM_WRITE_END_CODE 2
#define ARIES_MM_EEPROM_READ_END_CODE 3
#define ARIES_MM_EEPROM_READ_REG_CODE 4

/** Main Micro code for checksum computation */
#define ARIES_MM_EEPROM_CHECKSUM_CODE 16

/** Main Micro code for checksum computation (partial) */
#define ARIES_MM_EEPROM_CHECKSUM_PARTIAL_CODE 17

/** Num EEPROM CRC blocks */
#define ARIES_EEPROM_MAX_NUM_CRC_BLOCKS 10

//////////////////////////////////////
////////// Delay parameters //////////
//////////////////////////////////////

/** Wait time after completing an EEPROM block write-thru */
#define ARIES_DATA_BLOCK_PROGRAM_TIME_USEC 10000
/** Wait time after setting manual training */
#define ARIES_MAN_TRAIN_WAIT_SEC 0.01

//////////////////////////////////////
////////// Memory parameters //////////
//////////////////////////////////////

/** Main Micro num of print class enables in logger module */
#define ARIES_MM_PRINT_INFO_NUM_PRINT_CLASS_EN 8

/** Main Micro print buffer size */
#define ARIES_MM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE 512

/** Path Micro num of print class enables in logger module */
#define ARIES_PM_PRINT_INFO_NUM_PRINT_CLASS_EN 16

/** Path Micro print buffer size */
#define ARIES_PM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE 256

///////////////////////////////////////////
////////// Sides and quad slices //////////
///////////////////////////////////////////

/** Num. sides for PMA */
#define ARIES_NUM_SIDES 2

/** Num. quad slices per slice */
#define ARIES_NUM_QUAD_SLICES 4

//////////////////////////////////////////////////////////////
////////// Main-micro-assisted access command codes //////////
//////////////////////////////////////////////////////////////

/** PMA 0, 1 read */
#define ARIES_RD_PID_IND_PMA0 6
#define ARIES_RD_PID_IND_PMA1 7

/** PMA 0, 1, and broadcast write */
#define ARIES_WR_PID_IND_PMA0 0xf
#define ARIES_WR_PID_IND_PMA1 0x10
#define ARIES_WR_PID_IND_PMAX 0x11

///////////////////////////////////////////////////////////
//////////// Temperature measurement constants ////////////
///////////////////////////////////////////////////////////

/** Num of readings to take at each PMA */
#define ARIES_TEMP_NUM_READINGS 16

/** Temperature to offset result by */
#define ARIES_TEMPERATURE_OFFSET 25

/** Aries Pma temperature measurement slope val */
#define ARIES_TEMP_SLOPE ((310 - 560) / (105 - 25))

/** Aries Pma temperature offset (min) */
#define ARIES_TEMP_OFFSET_MIN 560

/** Aries Pma temperature offset (max) */
#define ARIES_TEMP_OFFSET_MAX 625

/** Aries max temp ADC code CSR */
#define ARIES_MAX_TEMP_ADC_CSR 0x424
#define ARIES_ALL_TIME_MAX_TEMP_ADC_CSR 0x424

/** Aries current max temp ADC code CSR */
#define ARIES_CURRENT_MAX_TEMP_ADC_CSR 0x428

/** Aries current average temp ADC code CSR */
#define ARIES_CURRENT_TEMP_ADC_CSR 0x42c
#define ARIES_CURRENT_AVG_TEMP_ADC_CSR 0x42c

/** Aries temp calibration uncertainty offset */
#define ARIES_TEMP_CALIBRATION_OFFSET 3

//////////////////////////////////////////////////////////
///////////////////// Miscellaneous //////////////////////
//////////////////////////////////////////////////////////

/** Maximum path length */
#define ARIES_PATH_MAX 4096

/** CRC8 polynomial used in PEC computation */
#define ARIES_CRC8_POLYNOMIAL 0x107

///////////////////////////////////////////////////////////
///////////////////// PIPE Interface //////////////////////
///////////////////////////////////////////////////////////
#define ARIES_PIPE_POWERDOWN_P0 0
#define ARIES_PIPE_POWERDOWN_P1 2

#define ARIES_PIPE_DEEMPHASIS_DE_NONE -1
#define ARIES_PIPE_DEEMPHASIS_PRESET_NONE -1

#endif /* ASTERA_ARIES_SDK_GLOBALS_H */
