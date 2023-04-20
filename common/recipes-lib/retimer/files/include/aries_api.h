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
 * @file aries_api.h
 * @brief Definition of public functions for the SDK.
 */

#ifndef ASTERA_ARIES_SDK_API_H_
#define ASTERA_ARIES_SDK_API_H_

#include "aries_globals.h"
#include "aries_error.h"
#include "aries_api_types.h"
#include "astera_log.h"
#include "aries_i2c.h"
#include "aries_link.h"
#include "aries_misc.h"

#ifdef ARIES_MPW
#include "aries_mpw_reg_defines.h"
#else
#include "aries_a0_reg_defines.h"
#endif

#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARIES_SDK_VERSION "2.13.1"

/**
 * @brief Return the SDK version
 *
 * @return     const char* - SDK version number
 */
const char* ariesGetSDKVersion(void);

AriesErrorType ariesInitDevice(AriesDeviceType* device, uint8_t recoveryAddr);

/**
 * @brief Check status of FW loaded
 *
 * Check the code load register and the Main Micro heartbeat. If all
 * is good, then read the firmware version
 *
 * @param[in,out]  device  Aries device struct
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesFWStatusCheck(AriesDeviceType* device);

/**
 * @brief Checks if Aries device is running compliance FW
 *
 * Checks the ate_customer_board and self_test compile options to determine
 * if compliance FW is running on the device, rather than mission-mode FW.
 *
 * @param[in,out]  device  Aries device struct
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesComplianceFWGet(AriesDeviceType* device);

/**
 * @brief Update the EEPROM page variables
 *
 * This is only necessariy if a 512 kbit EEPROM is being used
 *
 * @param[in, out]  device  Aries device struct
 * @param[in]  pageSize     Page size of EEPROMs in bytes
 * @return      AriesErrorType - Aries error code
 */
AriesErrorType ariesSetEEPROMPageSize(AriesDeviceType* device, int pageSize);

/**
 * @brief Set the bifurcation mode
 *
 * Bifurcation mode is written to global parameter register 0. Returns a
 * negative error code, else zero on success.
 *
 * @param[in]  device   Struct containing device information
 * @param[in]  bifur      Bifurcation mode
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesSetBifurcationMode(AriesDeviceType* device,
                                       AriesBifurcationType bifur);

/**
 * @brief Get the bifurcation mode
 *
 * Bifurcation mode is read from the global parameter register 0. Returns a
 * negative error code, else zero on success.
 *
 * @param[in]  device   Struct containing device information
 * @param[in]  bifur      Pointer to bifurcation mode variable
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetBifurcationMode(AriesDeviceType* device,
                                       AriesBifurcationType* bifur);

/**
 * @brief Set the Retimer HW Reset.
 *
 * This function sets the register to assert reset (1) or de-assert reset (0).
 * Asserting reset would set the retimer in reset, and de-asserting it will
 * bring the retimer out of reset and cause a firmware reload. User must wait
 * 1ms between assertion and de-assertion
 *
 * @param[in]  device   Struct containing device information
 * @param[in]  reset      Reset assert (1) or de-assert (0)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesSetHwReset(AriesDeviceType* device, uint8_t reset);

/**
 * @brief Set the Main Micro and I2C Master Reset.
 *
 * This function sets the register to assert reset (1) or de-assert reset (0).
 * Asserting the MM reset will halt firmware execution and de-asserting will
 * cause firmware to reload
 * Asserting the I2C master reset can be used to keep the Retimer from sending
 * transactions on the I2C bus connected to the EEPROM
 *
 * @param[in]  device   Struct containing device information
 * @param[in]  reset      Reset assert (1) or de-assert (0)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesSetMMI2CMasterReset(AriesDeviceType* device, uint8_t reset);

/**
 * @brief Set the I2C Master Reset.
 *
 * This function sets the register to assert reset (1) or de-assert reset (0).
 * Asserting the I2C master reset can be used to keep the Retimer from sending
 * tranactions on the I2C bus connected to the EEPROM
 *
 * @param[in]  device   Struct containing device information
 * @param[in]  reset      Reset assert (1) or de-assert (0)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesSetI2CMasterReset(AriesDeviceType* device, uint8_t reset);

/**
 * @brief Update the FW image in the EEPROM connected to the Retimer.
 *
 * The firmware image specififed by filename will be written to the EEPROM
 * attached to the Retimer then verified using the optimal method.
 *
 * @param[in]  device    Struct containing device information
 * @param[in]  filename  Filename of the file containing the firmware
 * @param[in]  fileType  Enum specifying firmware image file type (IHX or BIN)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesUpdateFirmware(AriesDeviceType* device,
                                   const char* filename,
                                   AriesFWImageFormatType fileType);

/**
 * @brief Update the FW image in the EEPROM connected to the Retimer via buffer
 *
 * The firmware image preloaded into a buffer will be written to the EEPROM
 * attached to the Retimer then verified using the optimal method.
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  image   Pointer to buffer of bytes containing image
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesUpdateFirmwareViaBuffer(AriesDeviceType* device,
                                            uint8_t* image);

/**
 * @brief Get the progress of the FW update in percent complete.
 *
 * @param[in]  device   Struct containing device information
 * @param[in]  percentComplete   Pointer to percent complete variable
 *
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesFirmwareUpdateProgress(AriesDeviceType* device,
                                           uint8_t* percentComplete);

/**
 * @brief Load a FW image into the EEPROM connected to the Retimer.
 *
 * numBytes from the values[] buffer will be written to the EEPROM attached
 * to the Retimer.
 *
 * @param[in]  device   Struct containing device information
 * @param[in]  values   Pointer to byte array containing the data to be
 *                      written to the EEPROM
 * @param[in]  legacyMode   If true, write EEPROM in slower legacy mode
                            (set this flag in error scenarios, when you wish to
 write EEPROM without using faster Main Micro assisted writes). Please set to
 false otherwise.
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteEEPROMImage(AriesDeviceType* device, uint8_t* values,
                                     bool legacyMode);

/**
 * @brief Verify the FW image in the EEPROM connected to the Retimer.
 *
 * numBytes from the values[] buffer will be compared against the contents
 * of the EEPROM attached to the Retimer. Returns a negative error code, else
 * zero on success.
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  values     Pointer to byte array containing the expected
 *                        firmware image. The actual contents of the EEPROM
 *                        will be compared against this.
 * @param[in]  legacyMode  If true, write EEPROM in slower legacy mode
                            (set this flag in error scenarios, when you wish to
 read EEPROM without using faster Main Micro assisted read). Please set to false
 otherwise.
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesVerifyEEPROMImage(AriesDeviceType* device, uint8_t* values,
                                      bool legacyMode);

/**
 * @brief Verify the FW image in the EEPROM connected to the Retimer via
 * checksum.
 *
 * In this case, no re-writes happen in case of a failure.
 * It is recommended to attempt the rewrite the FW into the EEPROM again
 * upon failure
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  image     Pointer to byte array containing the expected
 *                        firmware image. The actual contents of the EEPROM
 *                        will be compared against this.
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesVerifyEEPROMImageViaChecksum(AriesDeviceType* device,
                                                 uint8_t* image);

/**
 * @brief Calculate block CRCs from data in EEPROM
 *
 * @param[in]  device  Struct containing device information
 * @param[in, out]  image  Array containing FW info (in bytes)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesCheckEEPROMCrc(AriesDeviceType* device, uint8_t* image);

/**
 * @brief Calculate block CRCs from data in EEPROM
 *
 * @param[in]  device  Struct containing device information
 * @param[in, out]  crcBytes  Array containing block crc bytes
 * @param[in, out]  numCrcBytes size of crcBytes (in bytes)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesCheckEEPROMImageCrcBytes(AriesDeviceType* device,
                                             uint8_t* crcBytes,
                                             uint8_t* numCrcBytes);

/**
 * @brief Read a byte from the EEPROM.
 *
 * Executes the sequence of SMBus transactions to read a single byte from the
 * EEPROM connected to the retimer.
 *
 * @param[in]  device   Struct containing the device information
 * @param[in]  addr     EEPROM address to read a byte from
 * @param[in,out]  value    Data read from EEPROM (1 byte)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadEEPROMByte(AriesDeviceType* device, int addr,
                                   uint8_t* value);

/**
 * @brief Write a byte to the EEPROM.
 *
 * Executes the sequence of SMBus transactions to write a single byte to the
 * EEPROM connected to the retimer.
 *
 * @param[in]  device   Struct containing the device information
 * @param[in]  addr     EEPROM address to write a byte to
 * @param[in]  value    Data to write to EEPROM (1 byte)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteEEPROMByte(AriesDeviceType* device, int addr,
                                    uint8_t* value);

/**
 * @brief Check connection health
 *
 * @param[in] device   Pointer to Aries Device struct object
 * @param[in] slaveAddress   Desired Retimer I2C (7-bit) address in case ARP
 * needs to be run
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesCheckConnectionHealth(AriesDeviceType* device,
                                          uint8_t slaveAddress);

/**
 * @brief Perform an all-in-one health check on the device.
 *
 * Check if regular accesses to the EEPROM are working, and if the EEPROM has
 * loaded correctly (i.e. Main Mircocode, Path Microcode and PMA code have
 * loaded correctly).
 *
 * @param[in]  device    Pointer to Aries Device struct object
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesCheckDeviceHealth(AriesDeviceType* device);

/**
 * @brief Get the max recorded junction temperature.
 *
 * Read the maximum junction temperature and return in units of degrees
 * Celsius. This value represents the maximum value from all temperature
 * sensors on the Retimer. Returns a negative error code, else zero on
 * success.
 *
 * @param[in]  device  Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetMaxTemp(AriesDeviceType* device);

/**
 * @brief Get the current junction temperature.
 *
 * Read the current junction temperature and return in units of degrees
 * Celsius.
 *
 * @param[in]  device  Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetCurrentTemp(AriesDeviceType* device);

/**
 * @brief Set max data rate
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  rate  Max data rate to set
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesSetMaxDataRate(AriesDeviceType* device,
                                   AriesMaxDataRateType rate);

/**
 * @brief Set the GPIO value
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  gpioNum  GPIO number [0:3]
 * @param[in]  value  GPIO value (0) or (1)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesSetGPIO(AriesDeviceType* device, int gpioNum, bool value);

/**
 * @brief Get the GPIO value
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  gpioNum  GPIO number [0:3]
 * @param[in, out]  value  Pointer to GPIO value (0) or (1) variable
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetGPIO(AriesDeviceType* device, int gpioNum, bool* value);

/**
 * @brief Toggle the GPIO value
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  gpioNum  GPIO number [0:3]
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesToggleGPIO(AriesDeviceType* device, int gpioNum);

/**
 * @brief Set the GPIO direction
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  gpioNum  GPIO number [0:3]
 * @param[in]  value  GPIO direction (0 = output) or (1 = input)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesSetGPIODirection(AriesDeviceType* device, int gpioNum,
                                     bool value);

/**
 * @brief Get the GPIO direction
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  gpioNum  GPIO number [0:3]
 * @param[in, out]  value  Pointer to GPIO direction (0 = output) or (1 = input)
 * variable
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetGPIODirection(AriesDeviceType* device, int gpioNum,
                                     bool* value);

/**
 * @brief Enable Aries Test Mode for PRBS generation and checking
 *
 * @param[in]  device  Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeEnable(AriesDeviceType* device);

/**
 * @brief Disable Aries Test Mode for PRBS generation and checking
 *
 * @param[in]  device  Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeDisable(AriesDeviceType* device);

/**
 * @brief Set the desired Aries Test Mode data rate
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  rate  Desired rate (1, 2, ... 5)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeRateChange(AriesDeviceType* device,
                                       AriesMaxDataRateType rate);

/**
 * @brief Aries Test Mode transmitter configuration
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  pattern  Desired PRBS data pattern
 * @param[in]  preset  Desired Tx preset setting
 * @param[in]  enable  Enable (1) or disable (0) flag
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeTxConfig(AriesDeviceType* device,
                                     AriesPRBSPatternType pattern, int preset,
                                     bool enable);

/**
 * @brief Aries Test Mode transmitter single lane configuration
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  pattern  Desired PRBS data pattern
 * @param[in]  preset  Desired Tx preset setting
 * @param[in]  enable  Enable (1) or disable (0) flag
 * @param[in]  side    Side of chip
 * @param[in]  lane    Lane to configure
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeTxConfigLane(AriesDeviceType* device,
                                         AriesPRBSPatternType pattern,
                                         int preset, int side, int lane,
                                         bool enable);

/**
 * @brief Aries Test Mode receiver configuration
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  pattern  Desired PRBS data pattern
 * @param[in]  enable  Enable (1) or disable (0) flag
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeRxConfig(AriesDeviceType* device,
                                     AriesPRBSPatternType pattern, bool enable);

/**
 * @brief Aries Test Mode read error count
 *
 * @param[in]  device  Struct containing device information
 * @param[in, out]  ecount  Array containing error count data for each side and
 * lane
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeRxEcountRead(AriesDeviceType* device, int* ecount);

/**
 * @brief Aries Test Mode clear error count
 *
 * Reads ecount values for each side and lane and populates an array
 *
 * @param[in]  device  Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeRxEcountClear(AriesDeviceType* device);

/**
 * @brief Aries Test Mode clear single lane error count
 *
 * Reads ecount values for a single lane on one side and populates an array
 *
 * @param[in]  device  Struct containing device information
 * @param[in]  side    Side of chip
 * @param[in]  lane    Lane to clear error count for
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeRxEcountClearLane(AriesDeviceType* device, int side,
                                              int lane);

/**
 * @brief Aries Test Mode read FoM
 *
 * Reads FoM values for each side and lane and populates an array
 *
 * @param[in]  device  Struct containing device information
 * @param[in, out]  fom  Array containing FoM data for each side and lane
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeRxFomRead(AriesDeviceType* device, int* fom);

/**
 * @brief Aries Test Mode read Rx valid
 *
 * @param[in]  device  Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeRxValidRead(AriesDeviceType* device);

/**
 * @brief Aries Test Mode inject error
 *
 * @param[in]  device  Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesTestModeTxErrorInject(AriesDeviceType* device);

/**
 * @brief Aries trigger FW reload on next secondary bus reset (SBR)
 *
 * This only works on Firmware version 1.28.x or later.
 *
 * @param[in]  device Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesFWReloadOnNextSBR(AriesDeviceType* device);

/**
 * @brief Aries get over temperature status
 *
 * This only works on Firmware version 1.28.x or later.
 *
 * @param[in]  device Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesOvertempStatusGet(AriesDeviceType* device, bool* value);

/**
 * @brief Aries get status of I2C bus self-clear event log
 *
 * When an errant I2C transaction (e.g. a transaction intervening between the
 * Write and Read portions of a Aries Read transaction, or a transaction which
 * does not follow the specified transaction format) causes the Aries I2C slave
 * to get into a stuck state, the Aries FW will detect and clear this state.
 * This API reads back status to show if such an event has occured (1) or
 * not (0). This is a sticky status.
 *
 * @param[in]  device  Struct containing device information
 * @param[out] status  Pointer to return value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetI2CBusClearEventStatus(AriesDeviceType* device,
                                              int* status);

/**
 * @brief Aries clear I2C bus self-clear event log status
 *
 * @param[in]  device  Struct containing device information
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesClearI2CBusClearEventStatus(AriesDeviceType* device);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_ARIES_SDK_API_H_ */
