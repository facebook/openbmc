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
 * @file aries_misc.h
 * @brief Definition of helper functions used by aries SDK.
 */

#ifndef ASTERA_ARIES_SDK_MISC_H_
#define ASTERA_ARIES_SDK_MISC_H_

#include "aries_globals.h"
#include "aries_error.h"
#include "aries_api_types.h"
#include "astera_log.h"
#include "aries_api.h"
#include "aries_i2c.h"

#ifdef ARIES_MPW
#include "aries_mpw_reg_defines.h"
#else
#include "aries_a0_reg_defines.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARIES_MAIN_MICRO_EXT_CSR_I2C_MST_INIT_CTRL_BIT_BANG_MODE_EN_GET(x)     \
    (((x)&0x04) >> 2)
#define ARIES_MAIN_MICRO_EXT_CSR_I2C_MST_INIT_CTRL_BIT_BANG_MODE_EN_SET(x)     \
    (((x) << 2) & 0x04)
#define ARIES_MAIN_MICRO_EXT_CSR_I2C_MST_INIT_CTRL_BIT_BANG_MODE_EN_MODIFY(r,  \
                                                                           x)  \
    ((((x) << 2) & 0x04) | ((r)&0xfb))

/**
 * @brief Read FW version info from the Main Micro space
 *
 * @param[in]  i2cDriver  Aries i2c driver
 * @param[in]  offset  Offset inside FW struct for member
 * @param[in,out]  dataVal - data captured from registeres
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadFwVersion(AriesI2CDriverType* i2cDriver, int offset,
                                  uint8_t* dataVal);

int ariesFirmwareIsAtLeast(AriesDeviceType* device, uint8_t major,
                           uint8_t minor, uint16_t build);

AriesErrorType ariesI2CMasterSetPage(AriesI2CDriverType* i2cDriver, int page);

AriesErrorType ariesI2CMasterSendByteBlockData(AriesI2CDriverType* i2cDriver,
                                               int address, int numBytes,
                                               uint8_t* value);

AriesErrorType ariesI2CMasterWriteCtrlReg(AriesI2CDriverType* i2cDriver,
                                          uint32_t address,
                                          uint8_t lengthDataBytes,
                                          uint8_t* values);

AriesErrorType ariesI2CMasterInit(AriesI2CDriverType* i2cDriver);

AriesErrorType ariesI2CMasterSendByte(AriesI2CDriverType* i2cDriver,
                                      uint8_t* value, int flag);

/**
 * @brief Get the location where the EEPROM image ends, which is after a
 * specific sequence - a55aa555aff. This can be used to help reduce the overall
 * fw load time
 *
 * @param[in]  data  Full EEPROM image as a byte array
 * @param[in]  numBytes  Number of bytes to write (<= 256k)
 * @return     location - Index until which we have to write
 */
int ariesGetEEPROMImageEnd(uint8_t* data);

/**
 * @brief Write multiple blocks of data to the EEPROM with help from
 * Main Micro
 *
 * @param[in]  device      Aries Device struct
 * @param[in]  address     EEPROM address
 * @param[in]  numBytes    Number of bytes to write (<= 256k)
 * @param[in]  values      EEPROM data to be written as a byte array
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesI2CMasterMultiBlockWrite(AriesDeviceType* device,
                                             uint16_t address, int numBytes,
                                             uint8_t* values);

/**
 * @brief Write multiple blocks of data to the EEPROM with help from
 * Main Micro
 *
 * @param[in]  i2cDriver  Aries i2c driver
 * @param[in]  address    EEPROM address to rewrite and verify
 * @param[in]  value      Data to write to EEPROM (1 byte)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesI2CMasterRewriteAndVerifyByte(AriesI2CDriverType* i2cDriver,
                                                  int address, uint8_t* value);

/**
 * @brief Write multiple blocks of data to the EEPROM with help from
 * Main Micro
 *
 * @param[in]  i2cDriver  Aries i2c driver
 * @param[in]  address    EEPROM address to send
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesI2CMasterSendAddress(AriesI2CDriverType* i2cDriver,
                                         int address);

/**
 * @brief Write multiple blocks of data to the EEPROM with help from
 * Main Micro. At end of instruction, send a terminate flag
 *
 * @param[in]  device      Aries Device struct
 * @param[in]  value      EEPROM data read out in a byte array
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesI2CMasterReceiveByteBlock(AriesDeviceType* device,
                                              uint8_t* value);

/**
 * @brief Calculate checksum of this EEPROM block
 *
 * @param[in]  i2cDriver  Aries i2c driver
 * @param[in]  checksum   Checksum value returned by function
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesI2CMasterGetChecksum(AriesDeviceType* device,
                                         uint16_t blockEnd, uint32_t* checksum);

/**
 * @brief Set I2C Master frequency
 *
 * @param[in]  i2cDriver  Aries i2c driver
 * @param[in]  frequencyHz      I2C Master frequency in Hz
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesI2CMasterSetFrequency(AriesI2CDriverType* i2cDriver,
                                          int frequencyHz);

AriesErrorType ariesI2CMasterReceiveByte(AriesI2CDriverType* i2cDriver,
                                         uint8_t* value);

AriesErrorType
    ariesI2CMasterReceiveContinuousByte(AriesI2CDriverType* i2cDriver,
                                        uint8_t* value);

/**
 * @brief Get temp calibration codes, lot ID, and chip ID from eFuse.
 *
 * @param[in]  device   Aries Device struct
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetTempCalibrationCodes(AriesDeviceType* device);

/**
 * @brief Get max. temp reading across all PMAs
 *
 * @param[in]  device   Aries Device struct
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadPmaTempMax(AriesDeviceType* device);

/**
 * @brief Get current avg. temp reading across all PMAs
 *
 * @param[in]  device   Aries Device struct
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadPmaAvgTemp(AriesDeviceType* device);

AriesErrorType ariesReadPmaAvgTempDirect(AriesDeviceType* device);

/**
 * @brief Enable thermal shutdown in Aries
 *
 * @param[in]  device   Aries Device struct
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesEnableThermalShutdown(AriesDeviceType* device);

/**
 * @brief Disable thermal shutdown in Aries
 *
 * @param[in]  device   Aries Device struct
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesDisableThermalShutdown(AriesDeviceType* device);

/**
 * @brief Get PMA Temp reading
 *
 * @param[in]  device   Aries Device struct
 * @param[in]  side   PMA side
 * @param[in]  qs     PMA Quad Slice
 * @param[in/out] temp     PMA Temperature reading
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadPmaTemp(AriesDeviceType* device, int side, int qs,
                                float* temperature_C);

/**
 * @brief Get port orientation
 *
 * @param[in]  i2cDriver   i2c Driver struct
 * @param[out] orientation   port orientation
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetPortOrientation(AriesDeviceType* device,
                                       int* orientation);

/**
 * @brief Get PMA ID
 *
 * @param[in]  absLane   Absolute lane number
 * @return     int - pma id.
 */
int ariesGetPmaNumber(int absLane);

/**
 * @brief Get PMA lane ID
 *
 * @param[in]  absLane   Absolute lane number
 * @return     int - pma lane id.
 */
int ariesGetPmaLane(int absLane);

/**
 * @brief Get absolute Path ID
 *
 * @param[in]  lane   start lane of link
 * @param[in]  direction   upstream/downsteram direction
 * @return     int - path id.
 */
int ariesGetPathID(int lane, int direction);

/**
 * @brief Get Path lane ID
 *
 * @param[in]  lane   start lane of link
 * @return     int - path lane id.
 */
int ariesGetPathLaneID(int lane);

void ariesGetQSPathInfo(int lane, int direction, int* qs, int* qsPath,
                        int* qsPathLane);

int ariesGetStartLane(AriesLinkType* link);

/**
 * @brief Get link termination status
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  side    pma side - a (0) or b (1)
 * @param[in]  lane    link lane number
 * @param[out] term    term value returned
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLinkRxTerm(AriesLinkType* link, int side, int lane,
                                  int* term);

AriesErrorType ariesGetLinkId(AriesBifurcationType bifMode, int lane,
                              int* linkNum);

AriesErrorType ariesGetLinkCurrentSpeed(AriesLinkType* link, int lane,
                                        int direction, float* speed);

AriesErrorType ariesGetLaneNum(AriesLinkType* link, int lane, int* laneNum);

/**
 * @brief Get Logical Lane Num
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    link lane number
 * @param[in]  direction  link direction
 * @param[in/out] laneNum   logical lane number
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLogicalLaneNum(AriesLinkType* link, int lane,
                                      int direction, int* laneNum);

/**
 * @brief Get TX pre cursor value
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    link lane number
 * @param[in]  direction  link direction
 * @param[in/out] txPre   tx pre cursor value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetTxPre(AriesLinkType* link, int lane, int direction,
                             int* txPre);

/**
 * @brief Get TX current cursor value
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    link lane number
 * @param[in]  direction  link direction
 * @param[in/out] txCur   tx current cursor value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetTxCur(AriesLinkType* link, int lane, int direction,
                             int* txCur);

/**
 * @brief Get TX port cursor value
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    link lane number
 * @param[in]  direction  link direction
 * @param[in/out] txPst   tx pst value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetTxPst(AriesLinkType* link, int lane, int direction,
                             int* txPst);

/**
 * @brief Get RX polarity code
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    link lane number
 * @param[in]  direction  link direction
 * @param[in]  pinSet    location in pins array
 * @param[in/out] polarity    polarity code
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetRxPolarityCode(AriesLinkType* link, int lane,
                                      int direction, int pinSet, int* polarity);

/**
 * @brief Get current RX att code
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  side    pma side - a (0) or b (1)
 * @param[in]  absLane    Absolute lane number
 * @param[in/out] code    rx att code
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetRxAttCode(AriesLinkType* link, int side, int absLane,
                                 int* code);

/**
 * @brief Get current RX VGA Code
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  side    pma side - a (0) or b (1)
 * @param[in]  absLane    Absolute lane number
 * @param[in/out] boostCode    RX boost code
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetRxCtleBoostCode(AriesLinkType* link, int side,
                                       int absLane, int* boostCode);

/**
 * @brief Get current RX VGA Code
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  side    pma side - a (0) or b (1)
 * @param[in]  absLane    Absolute lane number
 * @param[in/out] vgaCode    RX VGA code
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetRxVgaCode(AriesLinkType* link, int side, int absLane,
                                 int* vgaCode);

/**
 * @brief Get current RX Boost Value (in dB)
 *
 * @param[in]  boostCode   boostCode
 * @param[in]  attValDb    att val (in db)
 * @param[in]  vgaCode    vga code
 * @return     float - rx Boost value
 */
float ariesGetRxBoostValueDb(int boostCode, float attValDb, int vgaCode);

/**
 * @brief: Get the current RX CTLE POLE code
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  side   PMA side
 * @param[in]  absLane    Absolute lane number
 * @param[in/out]  poleCode    POLE code captured
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetRxCtlePoleCode(AriesLinkType* link, int side,
                                      int absLane, int* poleCode);

/**
 * @brief: Get the current RX Adapt IQ Value
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  side   PMA side
 * @param[in]  absLane    Absolute lane number
 * @param[in/out]  iqValue    Adapt IQ value captured
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetRxAdaptIq(AriesLinkType* link, int side, int absLane,
                                 int* iqValue);

/**
 * @brief: Get the current RX Adapt IQ Value
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  side   PMA side
 * @param[in]  absLane    Absolute lane number
 * @param[in]  bank   Bank number
 * @param[in/out]  iqValue    Adapt IQ value captured
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetRxAdaptIqBank(AriesLinkType* link, int side, int absLane,
                                     int bank, int* iqValue);

/**
 * @brief: Get the current RX Adapt IQ Value
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  side   PMA side
 * @param[in]  absLane    Absolute lane number
 * @param[in]  bank   Bank number
 * @param[in/out]  doneValue    Adapt done value captured
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetRxAdaptDoneBank(AriesLinkType* link, int side,
                                       int absLane, int bank, int* doneValue);

/**
 * @brief: Get the current RX DFE code
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  side   PMA side
 * @param[in]  absLane    Absolute lane number
 * @param[in]  tapNum    DFE tap code
 * @param[in/out]  dfeCode    dfe code captured
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetRxDfeCode(AriesLinkType* link, int side, int absLane,
                                 int tapNum, int* dfeCode);

/**
 * @brief: Get the last speed at which EQ was run (e.g. 3 for Gen-3)
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  direction   PMA side
 * @param[in/out]  speed    last captures speed
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLastEqSpeed(AriesLinkType* link, int lane, int direction,
                                   int* speed);

/**
 * @brief: Get the deskew status string for the given lane
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  direction   PMA side
 * @param[in/out]  val    deskew status for lane
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetDeskewStatus(AriesLinkType* link, int lane,
                                    int direction, int* status);

/**
 * @brief: Get the number of clocks of deskew applied for the given lane
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  direction   PMA side
 * @param[in/out]  val    deskew clocks for lane
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetDeskewClks(AriesLinkType* link, int lane, int direction,
                                  int* val);

/**
 * @brief: For the last round of Equalization, get the final pre-cursor request
 * the link partner made. If the last request was a Perest request, this value
 * will be 0.
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  direction   PMA side
 * @param[in/out]  val    Pre-cursor request value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLastEqReqPre(AriesLinkType* link, int lane,
                                    int direction, int* val);

/**
 * @brief: For the last round of Equalization, get the final cursor request
 * the link partner made. If the last request was a Perest request, this value
 * will be 0.
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  direction   PMA side
 * @param[in/out]  val    cursor request value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLastEqReqCur(AriesLinkType* link, int lane,
                                    int direction, int* val);

/**
 * @brief: For the last round of Equalization, get the final post-cursor request
 * the link partner made. If the last request was a Perest request, this value
 * will be 0.
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  direction   PMA side
 * @param[in/out]  val    Post-cursor request value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLastEqReqPst(AriesLinkType* link, int lane,
                                    int direction, int* val);

/**
 * @brief: For the last round of Equalization, get the final preset request
 * the link partner made. A value of 0xf will indicate the final request was a
 * coefficient request.
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  direction   PMA side
 * @param[in/out]  val    Preset request value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLastEqReqPreset(AriesLinkType* link, int lane,
                                       int direction, int* val);

/**
 * @brief For the last round of Equalization, get the Preset value for the
 * specified request.
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  reqNum    Request num
 * @param[in/out]  val    Preset value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLastEqPresetReq(AriesLinkType* link, int lane,
                                       int direction, int reqNum, int* val);

/**
 * @brief For the last round of Equalization, get the Preset value for the
 * specified request.
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  reqNum    Request num
 * @param[in/out]  val    Preset value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLastEqPresetReq(AriesLinkType* link, int lane,
                                       int direction, int reqNum, int* val);

/**
 * @brief For the last round of Equalization, get the FOM value for the
 * specified request.
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  reqNum    Request num
 * @param[in/out]  val    FOM value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLastEqPresetReqFOM(AriesLinkType* link, int lane,
                                          int direction, int reqNum, int* val);

/**
 * @brief For the last round of Equalization, get the number of
 * reset requests issued.
 *
 * @param[in]  link   Link struct created by user
 * @param[in]  lane    Absolute lane number
 * @param[in]  reqNum    Request num
 * @param[in/out]  val    Number of reset requests
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLastEqNumPresetReq(AriesLinkType* link, int lane,
                                          int direction, int* val);

/**
 * @brief Get the format ID at offset int he print buffer
 *
 * @param[in]  link         Link struct created by user
 * @param[in]  loggerType   Logger type (main or which path)
 * @param[in]  offset       Print buffer offset
 * @param[in/out]  fmtID    Format ID from logger
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLoggerFmtID(AriesLinkType* link,
                                   AriesLTSSMLoggerEnumType loggerType,
                                   int offset, int* fmtID);

/**
 * @brief Get the write pointer location in the LTSSM logger
 *
 * @param[in]  link               Link struct created by user
 * @param[in]  loggerType         Logger type (main or which path)
 * @param[in/out]  writeOffset    Current write offset
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLoggerWriteOffset(AriesLinkType* link,
                                         AriesLTSSMLoggerEnumType loggerType,
                                         int* writeOffset);

/**
 * @brief Check if one batch mode in the LTSSM logger is enabled or not
 *
 * @param[in]  link               Link struct created by user
 * @param[in]  loggerType         Logger type (main or which path)
 * @param[in/out]  oneBatchModeEn    One batch mode enabled or not
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLoggerOneBatchModeEn(AriesLinkType* link,
                                            AriesLTSSMLoggerEnumType loggerType,
                                            int* oneBatchModeEn);

/**
 * @brief Check if one batch write in the LTSSM logger is enabled or not
 *
 * @param[in]  link               Link struct created by user
 * @param[in]  loggerType         Logger type (main or which path)
 * @param[in/out]  oneBatchWrEn    One batch write enabled or not
 * @return     AriesErrorType - Aries error code
 *
 */
AriesErrorType ariesGetLoggerOneBatchWrEn(AriesLinkType* link,
                                          AriesLTSSMLoggerEnumType loggerType,
                                          int* oneBatchWrEn);

/**
 * @brief Calculate CRC-8 byte from polynomial
 *
 * @param[in] msg  Polynomial to calculate PEC
 * @param[in] length Length of polynomial
 * @return     uint8_t - crc-8 byte
 */
uint8_t ariesGetPecByte(uint8_t* polynomial, uint8_t length);

/**
 * @brief Capture the min FoM value seen for a given lane.
 *
 * @param[in] device  Device struct containing i2c driver
 * @param[in] side    PMA side
 * @param[in] pathID  Path ID
 * @param[in] lane    lane inside path
 * @param[in] regOffset  address offset for FoM reg
 * @param[in,out] data   2-byte data returned, containing FoM value
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetMinFoMVal(AriesDeviceType* device, int side, int pathID,
                                 int lane, int regOffset, uint8_t* data);

AriesErrorType ariesGetPinMap(AriesDeviceType* device);

// Read numBytes bytes of data starting at startAddr
AriesErrorType ariesEepromReadBlockData(AriesDeviceType* device,
                                        uint8_t* values, int startAddr,
                                        uint8_t numBytes);

// Read numBytes bytes from the EEPROM starting at startAddr and
// calculate a running checksum (e.g. add the bytes as you read them):
// uint8_t checksum = (checksum + new_byte) % 256
AriesErrorType ariesEepromCalcChecksum(AriesDeviceType* device, int startAddr,
                                       int numBytes, uint8_t* checksum);

/**
 * @brief Sort Array in ascending order
 *
 * @param[in] arr Array to be sorted
 * @param[in] size Num elements in array
 * @return None
 */
void ariesSortArray(uint16_t* arr, int size);

/**
 * @brief Get median of all elements in the array
 *
 * @param[in] arr Array to be sorted
 * @param[in] size Num elements in array
 * @return None
 */
uint16_t ariesGetMedian(uint16_t* arr, int size);

/**
 * @brief This loads an binary file into the mem[] array
 */
AriesErrorType ariesLoadBinFile(const char* filename, uint8_t* mem);

/**
 * @brief This loads an intel hex file into the mem[] array
 */
AriesErrorType ariesLoadIhxFile(const char* filename, uint8_t* mem);

/**
 * @brief This is used by loadFile to get each line of intex hex
 */
AriesErrorType ariesParseIhxLine(char* line, uint8_t* bytes, int* addr,
                                 int* num, int* code);

AriesErrorType ariesI2cMasterSoftReset(AriesI2CDriverType* i2cDriver);

void ariesGetCrcBytesImage(uint8_t* image, uint8_t* crcBytes,
                           uint8_t* numCrcBytes);

AriesErrorType ariesEEPROMGetBlockLength(AriesI2CDriverType* i2cDriver,
                                         int blockStartAddr, int* blockLength);

AriesErrorType ariesEEPROMGetRandomByte(AriesI2CDriverType* i2cDriver, int addr,
                                        uint8_t* value);

AriesErrorType ariesGetEEPROMBlockCrcByte(AriesI2CDriverType* i2cDriver,
                                          int blockStartAddr, int blockLength,
                                          uint8_t* crcByte);

AriesErrorType ariesGetEEPROMBlockType(AriesI2CDriverType* i2cDriver,
                                       int blockStartAddr, uint8_t* blockType);

AriesErrorType ariesGetEEPROMFirstBlock(AriesI2CDriverType* i2cDriver,
                                        int* blockStartAddr);

AriesErrorType ariesGetPathFWState(AriesLinkType* link, int lane, int direction,
                                   int* state);

AriesErrorType ariesGetPathHWState(AriesLinkType* link, int lane, int direction,
                                   int* state);

AriesErrorType ariesSetPortOrientation(AriesDeviceType* device,
                                       uint8_t orientation);

AriesErrorType ariesPipeRxAdapt(AriesDeviceType* device, int side, int lane);

AriesErrorType ariesPipeFomGet(AriesDeviceType* device, int side, int lane,
                               int* fom);

AriesErrorType ariesPipeRxStandbySet(AriesDeviceType* device, int side,
                                     int lane, bool value);

AriesErrorType ariesPipeTxElecIdleSet(AriesDeviceType* device, int side,
                                      int lane, bool value);

AriesErrorType ariesPipeRxEqEval(AriesDeviceType* device, int side, int lane,
                                 bool value);

AriesErrorType ariesPipePhyStatusClear(AriesDeviceType* device, int side,
                                       int lane);

AriesErrorType ariesPipePhyStatusGet(AriesDeviceType* device, int side,
                                     int lane, bool* value);

AriesErrorType ariesPipePhyStatusToggle(AriesDeviceType* device, int side,
                                        int lane);

AriesErrorType ariesPipePowerdownSet(AriesDeviceType* device, int side,
                                     int lane, int value);

AriesErrorType ariesPipePowerdownCheck(AriesDeviceType* device, int side,
                                       int lane, int value);

AriesErrorType ariesPipeRateChange(AriesDeviceType* device, int side, int lane,
                                   int rate);

AriesErrorType ariesPipeRateCheck(AriesDeviceType* device, int side, int lane,
                                  int rate);

AriesErrorType ariesPipeDeepmhasisSet(AriesDeviceType* device, int side,
                                      int lane, int de, int preset, int pre,
                                      int main, int pst);

AriesErrorType ariesPipeRxPolaritySet(AriesDeviceType* device, int side,
                                      int lane, int value);

AriesErrorType ariesPipeRxTermSet(AriesDeviceType* device, int side, int lane,
                                  bool value);

AriesErrorType ariesPipeBlkAlgnCtrlSet(AriesDeviceType* device, int side,
                                       int lane, bool value, bool enable);

AriesErrorType ariesPMABertPatChkSts(AriesDeviceType* device, int side,
                                     int lane, int* ecount);

AriesErrorType ariesPMABertPatChkToggleSync(AriesDeviceType* device, int side,
                                            int lane);

AriesErrorType ariesPMABertPatChkDetectCorrectPolarity(AriesDeviceType* device,
                                                       int side, int lane);

AriesErrorType ariesPMARxInvertSet(AriesDeviceType* device, int side, int lane,
                                   bool invert, bool override);

AriesErrorType ariesPMABertPatChkConfig(AriesDeviceType* device, int side,
                                        int lane, AriesPRBSPatternType mode);

AriesErrorType ariesPMABertPatGenConfig(AriesDeviceType* device, int side,
                                        int lane, AriesPRBSPatternType mode);

AriesErrorType ariesPMARxDataEnSet(AriesDeviceType* device, int side, int lane,
                                   bool value);

AriesErrorType ariesPMATxDataEnSet(AriesDeviceType* device, int side, int lane,
                                   bool value);

AriesErrorType ariesPMAPCSRxReqBlock(AriesDeviceType* device, int side,
                                     int lane);

AriesErrorType ariesPMAVregVrefSet(AriesDeviceType* device, int side, int lane,
                                   int rate);

/**
 * @brief Read multiple data bytes from Aries over I2C. Purposely force an error
 * in the transaction by injecting an intervening I2C transaction in the middle
 * between the Write and Read.
 */
AriesErrorType ariesReadBlockDataForceError(AriesI2CDriverType* i2cDriver,
                                            uint32_t address, uint8_t numBytes,
                                            uint8_t* values);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_ARIES_SDK_MISC_H_ */
