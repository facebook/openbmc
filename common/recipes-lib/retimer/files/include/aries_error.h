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
 * @file aries_error.h
 * @brief Definition of error types for the SDK.
 */

#ifndef ASTERA_ARIES_SDK_ERROR_H_
#define ASTERA_ARIES_SDK_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Used to avoid warnings in case of unused parameters in function pointers */
#define ARIES_UNUSED(x) (void)(x)

/*
 * @brief Aries Error enum
 *
 * Enumeration of return values from the aries* functions within the SDK.
 * Value of 0 is a generic success response
 * Values less than 1 are specific error return codes
 */
typedef enum
{
    /* General purpose errors */
    /** Success return value, no error occurred */
    ARIES_SUCCESS = 0,
    /** Generic failure code */
    ARIES_FAILURE = -1,
    /** Invalid function argument */
    ARIES_INVALID_ARGUMENT = -2,
    /** Failed to open connection to I2C slave */
    ARIES_I2C_OPEN_FAILURE = -3,

    /* I2C Access failures*/
    /** I2C read transaction to Aries failure */
    ARIES_I2C_BLOCK_READ_FAILURE = -4,
    /** I2C write transaction to Aries failure */
    ARIES_I2C_BLOCK_WRITE_FAILURE = -5,
    /** Indirect SRAM access mechanism did not provide expected status */
    ARIES_FAILURE_SRAM_IND_ACCESS_TIMEOUT = -6,

    /* EEPROM Write and Read thru access failures */
    /** Main Micro status update to program EEPROM failed */
    ARIES_EEPROM_MM_STATUS_BUSY = -7,
    /** Data read back from EEPROM did not match expectations */
    ARIES_EEPROM_VERIFY_FAILURE = -8,

    /* Main Micro access failures */
    /** Failure in PMA access via Main Micro */
    ARIES_PMA_MM_ACCESS_FAILURE = -9,

    /* Link and logger configuration failures */
    /** Invalid link configuration */
    ARIES_LINK_CONFIG_INVALID = -10,
    /** Invalid LTSSM Logger entry */
    ARIES_LTSSM_INVALID_ENTRY = -11,

    /** SDK Function not yet implemented */
    ARIES_FUNCTION_NOT_IMPLEMENTED = -12,

    /** EEPROM write error */
    ARIES_EEPROM_WRITE_ERROR = -13,

    /** EEPROM CRC block fail */
    ARIES_EEPROM_CRC_BLOCK_NUM_FAIL = -14,
    /** EEPROM CRC block byte fail */
    ARIES_EEPROM_CRC_BYTE_FAIL = -15,

    /** Temperature read value not ready */
    ARIES_TEMP_READ_NOT_READY = -16,

    /** OUT OF MEMORY */
    ARIES_OUT_OF_MEMORY = -17
} AriesErrorType;

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_ARIES_SDK_ERROR_H_ */
