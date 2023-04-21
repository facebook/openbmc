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
 * @file aries_i2c.h
 * @brief Definition of I2C/SMBus types for the SDK.
 */

#ifndef ASTERA_ARIES_SDK_I2C_H_
#define ASTERA_ARIES_SDK_I2C_H_

#include "aries_globals.h"
#include "aries_error.h"
#include "aries_api_types.h"
#include "astera_log.h"
#include "aries_misc.h"

#ifdef ARIES_MPW
#include "aries_mpw_reg_defines.h"
#else
#include "aries_a0_reg_defines.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CHECK_SUCCESS(rc)                                                      \
    {                                                                          \
        if (rc != ARIES_SUCCESS)                                               \
        {                                                                      \
            ASTERA_ERROR("Unexpected return code: %d", rc);                    \
            return rc;                                                         \
        }                                                                      \
    }

/**
 * @brief Low-level I2C method to open a connection.
 *
 * @warning THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 *
 * @param[in]  i2cBus           I2C bus number
 * @param[in]  slaveAddress     I2C (7-bit) address of Retimer
 * @return     int - Error code
 */
int asteraI2COpenConnection(int i2cBus, int slaveAddress);

/**
 * @brief Low-level I2C write method.
 *
 * @warning THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 *
 * For example, if using linux/i2c.h, then the
 * implementation would be:
 *   int asteraI2CWriteBlockData(
 *       int handle,
 *       uint8_t cmdCode,
 *       uint8_t numBytes,
 *       uint8_t* buf)
 *   {
 *       return i2c_smbus_write_i2c_block_data(handle, cmdCode, numBytes, buf);
 *   }
 *
 * @param[in]  handle     Handle to I2C driver
 * @param[in]  cmdCode    8-bit command code
 * @param[in]  numBytes     Data buffer length, in bytes
 * @param[in]  buf        Pointer to data buffer (byte array)
 * @return     int - Error code
 */
int asteraI2CWriteBlockData(int handle, uint8_t cmdCode, uint8_t numBytes,
                            uint8_t* buf);

/**
 * @brief Low-level I2C read method.
 *
 * @warning THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 *
 * For example, if using linux/i2c.h, then the
 * implementation would be:
 *   int asteraI2CReadBlockData(
 *       int handle,
 *       uint8_t cmdCode,
 *       uint8_t numBytes,
 *       uint8_t* buf)
 *   {
 *       return i2c_smbus_read_i2c_block_data(handle, cmdCode, numBytes, buf);
 *   }
 *
 * @param[in]  handle     Handle to I2C driver
 * @param[in]  cmdCode    8-bit command code
 * @param[in]  numBytes     Number of bytes to read
 * @param[out] buf        Pointer to data buffer into which the read data will
 *                        be placed
 * @return     int - Error code
 */
int asteraI2CReadBlockData(int handle, uint8_t cmdCode, uint8_t numBytes,
                           uint8_t* buf);

/**
 * @brief Low-level I2C method to implement a lock around I2C transactions such
 * that a set of transactions can be atomic.
 *
 * @warning THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 *
 * @param[in]  handle Handle to I2C driver
 * @return     int - Error code
 */
int asteraI2CBlock(int handle);

/**
 * @brief Low-level I2C method to unlock a previous lock around I2C
 * transactions such that a set of transactions can be atomic.
 *
 * @warning THIS FUNCTION MUST BE IMPLEMENTED IN THE USER'S APPLICATION.
 *
 * @param[in]  handle Handle to I2C driver
 * @return     int - Error code
 */
int asteraI2CUnblock(int handle);

/**
 * @brief Set Slave address to user-specified value: new7bitSmbusAddr
 *
 * @param[in]  handle            Handle to I2C driver
 * @param[in]  new7bitSmbusAddr  DesiredI2C (7-bit) address of Retimer
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesRunArp(int handle, uint8_t new7bitSmbusAddr);

/**
 * @brief Write multiple data bytes to Aries over I2C  This function returns a
 * negative error code on error, and zero on success.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  address      17-bit address to write
 * @param[in]  numBytes  Number of bytes to write (maximum 16 bytes)
 * @param[in]  values       Pointer to array of data (bytes) you wish to write
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteBlockData(AriesI2CDriverType* i2cDriver,
                                   uint32_t address, uint8_t numBytes,
                                   uint8_t* values);

/**
 * @brief Write a data byte at specified address to Aries over I2C. Returns a
 * negative error code if unsuccessful, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    17-bit address to write
 * @param[in]  value      Pointer to single element array of data you wish
 *                        to write
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteByteData(AriesI2CDriverType* i2cDriver,
                                  uint32_t address, uint8_t* value);

/**
 * @brief Read multiple bytes of data at specified address from Aries over I2C.
 * If unsuccessful, return a negative error code, else zero.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  address      17-bit address from which to read
 * @param[in]  numBytes  Number of bytes to read (maximum 16 bytes)
 * @param[out] values       Pointer to array of read data (bytes)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadBlockData(AriesI2CDriverType* i2cDriver,
                                  uint32_t address, uint8_t numBytes,
                                  uint8_t* values);

/**
 * @brief Read a data byte from Aries over I2C
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    17-bit address from which to read
 * @param[out] value      Pointer to single element array of read data (byte)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadByteData(AriesI2CDriverType* i2cDriver,
                                 uint32_t address, uint8_t* value);

/**
 * @brief Read multiple (up to eight) data bytes from micro SRAM over I2C for
 * A0. Returns a negative error code, or zero on success
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  microIndStructOffset   Micro Indirect Struct Offset
 * @param[in]  address    16-bit address from which to read
 * @param[in]  numBytes  Number of bytes to read
 * @param[out] values     Pointer to single element array of read data
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadBlockDataMainMicroIndirectA0(
    AriesI2CDriverType* i2cDriver, uint32_t microIndStructOffset,
    uint32_t address, uint8_t numBytes, uint8_t* values);

/**
 * @brief Read multiple (up to eight) data bytes from micro SRAM over I2C.
 * Returns a negative error code, or zero on success
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  microIndStructOffset Micro Indirect Struct Offset
 * @param[in]  address    16-bit address from which to read
 * @param[in]  numBytes  Number of bytes to read
 * @param[out] values     Pointer to single element array of read data
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadBlockDataMainMicroIndirectMPW(
    AriesI2CDriverType* i2cDriver, uint32_t microIndStructOffset,
    uint32_t address, uint8_t numBytes, uint8_t* values);

/**
 * @brief Write multiple data bytes to specified address from micro SRAM over
 * I2C for A0. Returns a negative error code, or zero on success
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  microIndStructOffset Micro Indirect Struct Offset
 * @param[in]  address    16-bit address from which to write
 * @param[in]  numBytes  Number of bytes to write
 * @param[out] values      Pointer to array of data (bytes) you wish to write
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteBlockDataMainMicroIndirectA0(
    AriesI2CDriverType* i2cDriver, uint32_t microIndStructOffset,
    uint32_t address, uint8_t numBytes, uint8_t* values);

/**
 * @brief Write multiple (up to eight) data bytes to specified address from
 * micro SRAM over I2C. Returns a negative error code, or zero on success
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  microIndStructOffset Micro Indirect Struct Offset
 * @param[in]  address    16-bit address from which to write
 * @param[in]  numBytes  Number of bytes to write
 * @param[out] values      Pointer to array of data (bytes) you wish to write
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteBlockDataMainMicroIndirectMPW(
    AriesI2CDriverType* i2cDriver, uint32_t microIndStructOffset,
    uint32_t address, uint8_t numBytes, uint8_t* values);

/**
 * @brief Read one byte of data from specified address from Main micro SRAM
 * over I2C. Returns a negative error code, else the number byte data read.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    16-bit address from which to read
 * @param[out] values      Pointer to single element array of read data
 *                        (one byte)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadByteDataMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                                  uint32_t address,
                                                  uint8_t* values);

/**
 * @brief Read multiple (up to eight) data bytes from specified address from
 * Main micro SRAM over I2C. Returns a negative error code, else zero on
 * success
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  address      16-bit address from which to read
 * @param[in]  numBytes  Number of bytes to read (maximum 8 bytes)
 * @param[out] values       Pointer to array storing up to 8 bytes of data
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType
    ariesReadBlockDataMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                        uint32_t address, uint8_t numBytes,
                                        uint8_t* values);

/**
 * @brief Write a data byte at specified address to Main micro SRAM Aries
 * over I2C. Returns a negative error code, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  address    16-bit address to write
 * @param[in]  value      Byte data to write
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType
    ariesWriteByteDataMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                        uint32_t address, uint8_t* value);

/**
 * @brief Write multiple (up to eight) data bytes at specified address to
 * Main micro SRAM over I2C. Returns a negative error code, else zero on
 * success.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  address      16-bit address to write
 * @param[in]  numBytes  Number of bytes to write (maximum 16 bytes)
 * @param[in]  values       Byte array which will be written
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType
    ariesWriteBlockDataMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                         uint32_t address, uint8_t numBytes,
                                         uint8_t* values);

/**
 * @brief Read a data byte at specified address from Path micro SRAM over I2C.
 * Returns a negative error code, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  pathID     Path micro ID (e.g. 0, 1, ..., 15)
 * @param[in]  address    16-bit address from which to read
 * @param[out] value      Pointer to array of one byte of read data
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadByteDataPathMicroIndirect(AriesI2CDriverType* i2cDriver,
                                                  uint8_t pathID,
                                                  uint32_t address,
                                                  uint8_t* value);

/**
 * @brief Read multiple (up to eight) data bytes at specified address from
 * Path micro SRAM over I2C. Returns a negative error code, else the number
 * of bytes read.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  pathID       Path micro ID (e.g. 0, 1, ..., 15)
 * @param[in]  address      16-bit address from which to read
 * @param[in]  numBytes  Number of bytes to read (maximum 16 bytes)
 * @param[out] values       Pointer to array storing up to 16 bytes of data
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType
    ariesReadBlockDataPathMicroIndirect(AriesI2CDriverType* i2cDriver,
                                        uint8_t pathID, uint32_t address,
                                        uint8_t numBytes, uint8_t* values);

/**
 * @brief Write one byte of data byte at specified address to Path micro SRAM
 * Aries over I2C. Returns a negative error code, else zero on success.
 *
 * @param[in]  i2cDriver  I2C driver responsible for the transaction(s)
 * @param[in]  pathID     Path micro ID (e.g. 0, 1, ..., 15)
 * @param[in]  address    16-bit address to write
 * @param[in]  value      Byte data to write
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType
    ariesWriteByteDataPathMicroIndirect(AriesI2CDriverType* i2cDriver,
                                        uint8_t pathID, uint32_t address,
                                        uint8_t* value);

/**
 * @brief Write multiple (up to eight) data bytes at specified address to
 * Path micro SRAM over I2C. Returns a negative error code, else zero on
 * success.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  pathID       Path micro ID (e.g. 0, 1, ..., 15)
 * @param[in]  address      16-bit address to write
 * @param[in]  numBytes  Number of bytes to write (maximum 16 bytes)
 * @param[in]  values       Byte array which will be written
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType
    ariesWriteBlockDataPathMicroIndirect(AriesI2CDriverType* i2cDriver,
                                         uint8_t pathID, uint32_t address,
                                         uint8_t numBytes, uint8_t* values);

/**
 * @brief Read 2 bytes of data from PMA register over I2C
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0) or A (1)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  address      16-bit address from which to read
 * @param[in]  values       Byte array which will be written
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadWordPmaIndirect(AriesI2CDriverType* i2cDriver, int side,
                                        int quadSlice, uint16_t address,
                                        uint8_t* values);

/**
 * @brief Write 2 bytes of data from PMA register over I2C
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1), or broadcast to both (2)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  address      16-bit address to write
 * @param[in]  values       Byte array which will be written
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteWordPmaIndirect(AriesI2CDriverType* i2cDriver,
                                         int side, int quadSlice,
                                         uint16_t address, uint8_t* values);

/**
 * @brief Read 2 bytes of data from PMA lane register over I2C
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0) or A (1)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  lane         PMA lane number
 * @param[in]  regOffset    16-bit ref offset from which to read
 * @param[in]  values       Byte array to store read data
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadWordPmaLaneIndirect(AriesI2CDriverType* i2cDriver,
                                            int side, int quadSlice, int lane,
                                            uint16_t regOffset,
                                            uint8_t* values);

/**
 * @brief Write 2 bytes of data to PMA lane register over I2C
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1), or broadcast to both (2)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  lane         PMA lane number
 * @param[in]  regOffset    16-bit ref offset from which to read
 * @param[in]  values       Byte array which will be written
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteWordPmaLaneIndirect(AriesI2CDriverType* i2cDriver,
                                             int side, int quadSlice, int lane,
                                             uint16_t regOffset,
                                             uint8_t* values);

/**
 * @brief Read 2 bytes of data from PMA register over I2C using the
 * 'main-micro-assisted' indirect method. This method is necessary during
 * mission-mode (non-PRBS-test-mode) operation.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0) or A (1)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  pmaAddr      16-bit PMA reg offset from which to read
 * @param[in,out]  data       Byte array which will be contain data read
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadWordPmaMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                                 int side, int quadSlice,
                                                 uint16_t pmaAddr,
                                                 uint8_t* data);

/**
 * @brief Write 2 bytes of data to PMA register over I2C using the
 * 'main-micro-assisted' indirect method. This method is necessary during
 * mission-mode (non-PRBS-test-mode) operation.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1), or broadcast to both (2)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  pmaAddr      16-bit PMA reg offset from which to read
 * @param[in,out]  data       Byte array which will be written
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteWordPmaMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                                  int side, int quadSlice,
                                                  uint16_t pmaAddr,
                                                  uint8_t* data);

/**
 * @brief Read 2 bytes of data from PMA lane register over I2C using the
 * 'main-micro-assisted' indirect method. This method is necessary during
 * mission-mode (non-PRBS-test-mode) operation.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0) or A (1)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  lane         PMA lane number
 * @param[in]  regOffset      16-bit PMA reg offset from which to read
 * @param[in,out]  values       Byte array which will be contain data read
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType
    ariesReadWordPmaLaneMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                          int side, int quadSlice, int lane,
                                          uint16_t regOffset, uint8_t* values);

/**
 * @brief Write 2 bytes of data to PMA lane register over I2C using the
 * 'main-micro-assisted' indirect method. This method is necessary during
 * mission-mode (non-PRBS-test-mode) operation.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1), or broadcast to both (2)
 * @param[in]  quadSlice    PMA num: 0, 1, 2, or 3
 * @param[in]  lane         PMA lane number
 * @param[in]  regOffset    16-bit ref offset from which to read
 * @param[in]  values       Byte array which will be written
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType
    ariesWriteWordPmaLaneMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                           int side, int quadSlice, int lane,
                                           uint16_t regOffset, uint8_t* values);

/**
 * @brief Read Modify Write 2 bytes of data to PMA lane register over I2C using
 * the 'main-micro-assisted' indirect method. This method is necessary during
 * mission-mode (non-PRBS-test-mode) operation.
 *
 * @param[in] i2cDriver     I2C driver responsible for the transaction(s)
 * @param[in] side          PMA Side B (0), A (1), or broadcast to both (2)
 * @param[in] quadSlice     PMA num: 0, 1, 2, or 3
 * @param[in] quadSliceLane PMA lane number
 * @param[in] pmaAddr       Address you wish to write to
 * @param[in] offset        Offset from the start of the word you want to write
 * in
 * @param[in] value         The value to write
 * @param[in] width         The width of the field you are writing to
 * @return      AriesErrorType - Aries error code
 */
AriesErrorType ariesReadWriteWordPmaLaneMainMicroIndirect(
    AriesI2CDriverType* i2cDriver, int side, int quadSlice, int quadSliceLane,
    uint16_t pmaAddr, int offset, uint16_t value, int width);

/**
 * @brief Read N bytes of data from a Retimer (gbl, ln0, or ln1) CSR.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1)
 * @param[in]  lane         Absolute lane number (15:0)
 * @param[in]  baseAddr     16-bit base address
 * @param[in]  numBytes     Number of bytes to read
 * @param[in]  data         Byte array which will be contain data read
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesReadRetimerRegister(AriesI2CDriverType* i2cDriver, int side,
                                        int lane, uint16_t baseAddr,
                                        uint8_t numBytes, uint8_t* data);

/**
 * @brief Write N bytes of data to a Retimer (gbl, ln0, or ln1) CSR.
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @param[in]  side         PMA Side B (0), A (1)
 * @param[in]  lane         Absolute lane number (15:0)
 * @param[in]  baseAddr     16-bit base address
 * @param[in]  numBytes     Number of bytes to write
 * @param[in]  data         Byte array containing data to be written
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesWriteRetimerRegister(AriesI2CDriverType* i2cDriver,
                                         int side, int lane, uint16_t baseAddr,
                                         uint8_t numBytes, uint8_t* data);

/**
 * @brief Set lock on bus (Aries transaction)
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesLock(AriesI2CDriverType* i2cDriver);

/**
 * @brief Unlock bus after lock (Aries transaction)
 *
 * @param[in]  i2cDriver    I2C driver responsible for the transaction(s)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesUnlock(AriesI2CDriverType* i2cDriver);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_ARIES_SDK_I2C_H_ */
