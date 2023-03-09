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
 * @file aries_i2c.c
 * @brief Implementation of public functions for the SDK I2C interface.
 */

#include "include/aries_i2c.h"
#include "include/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Set Slave address to user-specified value: new7bitSmbusAddr
 */
AriesErrorType ariesRunArp(int handle, uint8_t new7bitSmbusAddr)
{
    int rc;
    uint8_t dataByte[1];
    uint8_t rdataBytes[19];

    dataByte[0] = 0xc9;
    rc = asteraI2CWriteBlockData(handle, 0x2, 1, dataByte);
    CHECK_SUCCESS(rc);

    dataByte[0] = 0xc0;
    rc = asteraI2CWriteBlockData(handle, 0x1, 1, dataByte);
    CHECK_SUCCESS(rc);

    // This read will get first device repsonse in ARP mode
    rc = asteraI2CReadBlockData(handle, 0x3, 19, rdataBytes);
    if (rc != 19)
    {
        return ARIES_I2C_BLOCK_READ_FAILURE;
    }

    // The PEC byte is calculated from 20 bytes of data including the ARP slave
    // address byte (0x61<<1 = 0xc2), the command code (0x4), and the remainder
    // of the Assign Address command.
    uint8_t new8bitSmbusAddr = new7bitSmbusAddr << 1;
    uint8_t pec_data[21] = {
        0xc2, 4, 17, 1, 9, 29, 250, 0, 1, 0, 21,
        0,    0, 0,  0, 0, 0,  0,   1, 0, 0}; // last byte is new 8-bit SMBus
                                              // address
    pec_data[19] = new8bitSmbusAddr;
    uint8_t crc = ariesGetPecByte(pec_data, 21);

    // Program slave address of device to 0x55
    uint8_t dataBytes[19];
    uint8_t i;
    for (i = 0; i < 18; i++)
    {
        dataBytes[i] = pec_data[i + 2];
    }
    dataBytes[18] = crc; // PEC byte

    rc = asteraI2CWriteBlockData(handle, 0x4, 19, dataBytes);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Write multiple data bytes to Aries over I2C
 */
AriesErrorType ariesWriteBlockData(AriesI2CDriverType* i2cDriver,
                                   uint32_t address, uint8_t numBytes,
                                   uint8_t* values)
{
    AriesErrorType rc;
    uint8_t cmdCode;
    uint8_t pecEn;
    uint8_t rsvd;
    uint8_t funcCode;
    uint8_t start;
    uint8_t end;
    uint8_t addr15To8;
    uint8_t addr7To0;
    int pos;

    if (i2cDriver->i2cFormat == ARIES_I2C_FORMAT_INTEL)
    {
        // If Address more than 16 bit, return with an error code
        // Intel format only allows for 16 bit address
        if (address > 0xffff)
        {
            ASTERA_ERROR("Address cannot be more than 16 bits in Intel format");
            return ARIES_INVALID_ARGUMENT;
        }

        // If byte count is greater than 4, perform multiple iterations
        // Hence keep track of remaining bytes each time we perform a
        // transaction
        uint8_t currBytes = 0;
        uint8_t remainingBytes = numBytes;
        while (remainingBytes > 0)
        {
            if (remainingBytes > 4)
            {
                currBytes = 4;
                remainingBytes -= 4;
            }
            else
            {
                currBytes = remainingBytes;
                remainingBytes = 0;
            }

            pecEn = 0;
            rsvd = 0;
            funcCode = 1;
            start = 1;
            end = 1;
            pos = 0;

            // Check if PEC is enabled
            if (i2cDriver->pecEnable == ARIES_I2C_PEC_ENABLE)
            {
                pecEn = 1;
            }

            addr15To8 = (address & 0xff00) >> 8; // Get bits 16:8
            addr7To0 = address & 0xff;           // Get bits 7:0

            // Set buffer length based on number of bytes being written
            // Include 2 bytes of address in calculation
            // Include PEC in calculation
            // 1 extra byte for num bytes in buffer
            int writeNumBytes = 2 + 1 + currBytes + pecEn;
            uint8_t writeBuf[writeNumBytes];
            writeBuf[0] = writeNumBytes - 1 - pecEn;
            writeBuf[1] = addr7To0;
            writeBuf[2] = addr15To8;

            // Construct Command code
            cmdCode = (pecEn << 7) + (rsvd << 5) + (funcCode << 2) +
                      (start << 1) + (end << 0);

            ASTERA_TRACE("Write:");
            ASTERA_TRACE("    cmdCode = 0x%02x", cmdCode);
            ASTERA_TRACE("    writeBuf[0] = 0x%02x", writeBuf[0]);
            ASTERA_TRACE("    writeBuf[1] = 0x%02x", writeBuf[1]);
            ASTERA_TRACE("    writeBuf[2] = 0x%02x", writeBuf[2]);

            int byteIndex;
            for (byteIndex = 0; byteIndex < currBytes; byteIndex++)
            {
                writeBuf[(byteIndex + 3)] = values[byteIndex];
                ASTERA_TRACE("    writeBuf[%d] = 0x%02x", (byteIndex + 3),
                             (values[byteIndex]));
            }

            // Set PEC bits
            if (pecEn)
            {
                // Include slave address and write bit, and cmd Code
                // PEC byte accounted in writeNumBytes
                uint8_t wrStreamLen = writeNumBytes + 2;
                uint8_t wrStream[wrStreamLen];
                uint8_t wrPec;

                // Include slave address and write bit (0)
                wrStream[0] = (i2cDriver->slaveAddr << 1) + 0;
                wrStream[1] = cmdCode;
                ASTERA_TRACE("        pecBuf[0] = 0x%02x", wrStream[0]);
                ASTERA_TRACE("        pecBuf[1] = 0x%02x", wrStream[1]);

                uint8_t wrStreamIdx;
                for (wrStreamIdx = 2; wrStreamIdx < (wrStreamLen - 1);
                     wrStreamIdx++)
                {
                    wrStream[wrStreamIdx] = writeBuf[(wrStreamIdx - 2)];
                    ASTERA_TRACE("        pecBuf[%d] = 0x%02x", wrStreamIdx,
                                 wrStream[wrStreamIdx]);
                }
                wrStream[(wrStreamLen - 1)] = 0x0;
                ASTERA_TRACE("        pecBuf[last] = 0x0");

                // Calculate CRC8 byte from polynomial
                wrPec = ariesGetPecByte(wrStream, wrStreamLen);
                writeBuf[(writeNumBytes - 1)] = wrPec;
                ASTERA_TRACE("    writeBuf[last] = 0x%02x", wrPec);
            }

            rc = asteraI2CWriteBlockData(i2cDriver->handle, cmdCode,
                                         writeNumBytes, writeBuf);
            CHECK_SUCCESS(rc);

            // Increment iteration count
            values += currBytes;
            address += currBytes;
        }
    }
    else
    {
        uint8_t currBytes = 0;
        uint8_t remainingBytes = numBytes;
        while (remainingBytes > 0)
        {
            if (remainingBytes > 4)
            {
                currBytes = 4;
                remainingBytes -= 4;
            }
            else
            {
                currBytes = remainingBytes;
                remainingBytes = 0;
            }

            pecEn = 0;
            rsvd = 0;
            funcCode = 3;
            start = 1;
            end = 1;
            pos = 0;

            if (i2cDriver->pecEnable == ARIES_I2C_PEC_ENABLE)
            {
                pecEn = 1;
            }

            // Buffer Length must include extra bytes to add "numBytes" and addr
            // info 1 byte for "numBytes" 1 byte for cfg_type, burstLen,
            // addr[17] etc 2 bytes for 16 bit addr Rest is data bytes 1 byte
            // for pec
            uint8_t wrBufLen = currBytes + 4 + pecEn;
            uint8_t writeBuf[wrBufLen];

            uint8_t config;
            uint8_t cfg_type = 1;
            uint8_t bdcst = 0;
            uint8_t burstLen = (currBytes - 1);
            uint8_t addr17;

            ASTERA_TRACE("Writing to address: 0x%08x", address);

            // Construct Command code
            cmdCode = (pecEn << 7) + (rsvd << 5) + (funcCode << 2) +
                      (start << 1) + (end << 0);

            // Construct Config & Offset Upper byte
            addr17 = (address & 0x10000) >> 16;  // Get 17bit of addr
            addr15To8 = (address & 0xff00) >> 8; // Get bits 16:8
            addr7To0 = address & 0xff;           // Get bits 7:0

            config = (cfg_type << 6) + (bdcst << 4) + (burstLen << 1) +
                     (addr17 << 0);

            // Construct data buffer
            writeBuf[0] = wrBufLen - 1 - pecEn;
            writeBuf[1] = config;
            writeBuf[2] = addr15To8;
            writeBuf[3] = addr7To0;

            ASTERA_TRACE("Write:");
            ASTERA_TRACE("    cmdCode = 0x%02x", cmdCode);
            ASTERA_TRACE("    writeBuf[0] = 0x%02x", writeBuf[0]);
            ASTERA_TRACE("    writeBuf[1] = 0x%02x", writeBuf[1]);
            ASTERA_TRACE("    writeBuf[2] = 0x%02x", writeBuf[2]);
            ASTERA_TRACE("    writeBuf[3] = 0x%02x", writeBuf[3]);

            // Fill up data buffer
            for (pos = 0; pos < currBytes; pos++)
            {
                int bufPos = pos + 4;
                writeBuf[bufPos] = values[pos];
                ASTERA_TRACE("    writeBuf[%d] = 0x%02x", bufPos,
                             writeBuf[bufPos]);
            }

            // Set PEC bits
            if (pecEn)
            {
                // Include slave address and write bit, and cmd Code
                // PEC byte accounted in writeNumBytes
                uint8_t wrStreamLen = wrBufLen + 2;
                uint8_t wrStream[wrStreamLen];
                uint8_t wrPec;

                // Include slave address and write bit (0)
                wrStream[0] = (i2cDriver->slaveAddr << 1) + 0;
                wrStream[1] = cmdCode;
                ASTERA_TRACE("        pecBuf[0] = 0x%02x", wrStream[0]);
                ASTERA_TRACE("        pecBuf[1] = 0x%02x", wrStream[1]);

                uint8_t wrStreamIdx;
                for (wrStreamIdx = 2; wrStreamIdx < (wrStreamLen - 1);
                     wrStreamIdx++)
                {
                    wrStream[wrStreamIdx] = writeBuf[(wrStreamIdx - 2)];
                    ASTERA_TRACE("        pecBuf[%d] = 0x%02x", wrStreamIdx,
                                 wrStream[wrStreamIdx]);
                }
                wrStream[(wrStreamLen - 1)] = 0x0;

                // Calculate CRC8 byte from polynomial
                wrPec = ariesGetPecByte(wrStream, wrStreamLen);
                writeBuf[(wrBufLen - 1)] = wrPec;
                ASTERA_TRACE("    writeBuf[%d] = 0x%02x", (wrBufLen - 1),
                             writeBuf[(wrBufLen - 1)]);
            }

            // This translates to actual low level library call
            // Function definition part of user application
            rc = asteraI2CWriteBlockData(i2cDriver->handle, cmdCode, wrBufLen,
                                         writeBuf);
            CHECK_SUCCESS(rc);
            values += currBytes;
            address += currBytes;
        }
    }
    return ARIES_SUCCESS;
}

/*
 * Write a data byte to Aries over I2C
 */
AriesErrorType ariesWriteByteData(AriesI2CDriverType* i2cDriver,
                                  uint32_t address, uint8_t* value)
{
    return ariesWriteBlockData(i2cDriver, address, 1, value);
}

/*
 * Read multiple data bytes from Aries over I2C
 */
AriesErrorType ariesReadBlockData(AriesI2CDriverType* i2cDriver,
                                  uint32_t address, uint8_t numBytes,
                                  uint8_t* values)
{
    AriesErrorType rc;
    AriesErrorType lc;
    int readBytes;
    int pos;
    uint8_t wrCmdCode;
    uint8_t rdCmdCode;
    uint8_t pecEn;
    uint8_t rsvd;
    uint8_t funcCode;
    uint8_t start;
    uint8_t end;

    uint8_t addr15To8; // Get bits 16:8
    uint8_t addr7To0;  // Get bits 7:0

    uint8_t tryIndex;
    uint8_t readTryCount = 3;

    if (i2cDriver->i2cFormat == ARIES_I2C_FORMAT_INTEL)
    {
        // Addresses in this format can be 16 bit only
        if (address > 0xffff)
        {
            ASTERA_ERROR("Address greater than allowed 16 bit (0x%08x)",
                         address);
            return ARIES_INVALID_ARGUMENT;
        }

        uint8_t remainingBytes = numBytes;
        uint8_t currBytes = 0;
        while (remainingBytes > 0)
        {
            if (remainingBytes > 4)
            {
                currBytes = 4;
                remainingBytes -= 4;
            }
            else
            {
                currBytes = remainingBytes;
                remainingBytes = 0;
            }

            pecEn = 0;
            rsvd = 0;
            funcCode = 0;
            start = 1;
            end = 0;

            // Increment address when you update numIters
            if (i2cDriver->pecEnable == ARIES_I2C_PEC_ENABLE)
            {
                pecEn = 1;
            }

            wrCmdCode = (pecEn << 7) + (rsvd << 5) + (funcCode << 2) +
                        (start << 1) + (end << 0);

            addr15To8 = (address & 0xff00) >> 8;
            addr7To0 = address & 0xff;

            int writeNumBytes = 3 + pecEn;
            uint8_t writeBuf[writeNumBytes];

            writeBuf[0] = writeNumBytes - 1 - pecEn;
            writeBuf[1] = addr7To0;
            writeBuf[2] = addr15To8;

            ASTERA_TRACE("Write:");
            ASTERA_TRACE("    cmdCode = 0x%02x", wrCmdCode);
            ASTERA_TRACE("    writeBuf[0] = 0x%02x", writeBuf[0]);
            ASTERA_TRACE("    writeBuf[1] = 0x%02x", writeBuf[1]);
            ASTERA_TRACE("    writeBuf[2] = 0x%02x", writeBuf[2]);

            if (pecEn)
            {
                // Include slave addr and write bit (0) in wrStream
                // Include cmd code in wrStream
                // PEC bit is already accounted for
                uint8_t wrStreamLen = 2 + writeNumBytes;
                uint8_t wrStream[wrStreamLen];

                // Fill wrStream
                wrStream[0] = (i2cDriver->slaveAddr << 1) + 0;
                wrStream[1] = wrCmdCode;
                ASTERA_TRACE("        pecBuf[0] = 0x%02x", wrStream[0]);
                ASTERA_TRACE("        pecBuf[1] = 0x%02x", wrStream[1]);
                int byteIdx;
                for (byteIdx = 2; byteIdx < (wrStreamLen - 1); byteIdx++)
                {
                    wrStream[byteIdx] = writeBuf[(byteIdx - 2)];
                    ASTERA_TRACE("        pecBuf[%d] = 0x%02x", byteIdx,
                                 wrStream[byteIdx]);
                }
                // Append 0x0 as CRC Polynomial
                wrStream[(wrStreamLen - 1)] = 0x0;
                ASTERA_TRACE("        pecBuf[last] = 0x0");

                uint8_t wrPec = ariesGetPecByte(wrStream, wrStreamLen);
                writeBuf[3] = wrPec;
                ASTERA_TRACE("    writeBuf[3] = 0x%02x", writeBuf[3]);
            }

            // Set up I2C lock, to keep write and read atomic
            rc = ariesLock(i2cDriver);
            // Do not unblock here on return of error since this error code
            // would mean that the block call did not happen
            CHECK_SUCCESS(rc);

            // read buffer is 3 + data + PEC bytes always
            // First byte is num.bytes read
            // Second and third bytes are address
            // Bytes 4 onwards is data (4 bytes)
            // Last Byte is PEC
            uint8_t readBufLen = 3 + 4 + pecEn;
            uint8_t readBuf[readBufLen];

            for (tryIndex = 0; tryIndex < readTryCount; tryIndex++)
            {
                rc = asteraI2CWriteBlockData(i2cDriver->handle, wrCmdCode,
                                             writeNumBytes, writeBuf);
                if (rc != ARIES_SUCCESS)
                {
                    lc = ariesUnlock(i2cDriver);
                    CHECK_SUCCESS(lc);
                    return rc;
                }

                start = 0;
                end = 1;
                funcCode = 0;

                rdCmdCode = (pecEn << 7) + (rsvd << 5) + (funcCode << 2) +
                            (start << 1) + (end << 0);

                ASTERA_TRACE("Read:");
                ASTERA_TRACE("    cmdCode = 0x%02x", rdCmdCode);

                rc = asteraI2CReadBlockData(i2cDriver->handle, rdCmdCode,
                                            readBufLen, readBuf);

                if (rc != readBufLen)
                {
                    ASTERA_TRACE(
                        "ariesReadBlockData() interrupted by intervening transaction");
                    ASTERA_TRACE("    Performing read operation again ...");
                    rc = ARIES_I2C_BLOCK_READ_FAILURE;
                    if (tryIndex == (readTryCount - 1))
                    {
                        ASTERA_ERROR("Incorrect num. bytes returned by read");
                        lc = ariesUnlock(i2cDriver);
                        CHECK_SUCCESS(lc);
                        return ARIES_I2C_BLOCK_READ_FAILURE;
                    }
                }
                else
                {
                    rc = ARIES_SUCCESS;
                    break;
                }
            }

            // Unlock previous lock set before write
            rc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(rc);

            if (pecEn)
            {
                // In read PEC calculation, include write (2 bytes - addr,
                // w bit and command) and read portions (addr, r bit, data
                // returned) of smbus instruction
                uint8_t readStreamLen = readBufLen + 2 + 1;
                uint8_t readStream[readStreamLen];
                uint8_t rdPec;

                readStream[0] = (i2cDriver->slaveAddr << 1) + 0;
                readStream[1] = rdCmdCode;
                readStream[2] = (i2cDriver->slaveAddr << 1) + 1;

                uint8_t rdStreamId;
                for (rdStreamId = 3; rdStreamId < readStreamLen; rdStreamId++)
                {
                    readStream[rdStreamId] = readBuf[rdStreamId - 3];
                }
                rdPec = ariesGetPecByte(readStream, readStreamLen);
                if (rdPec != 0)
                {
                    ASTERA_ERROR("PEC value not as expected");
                    return ARIES_I2C_BLOCK_READ_FAILURE;
                }
            }

            // Fill up user given array
            int byteIndex;
            for (byteIndex = 0; byteIndex < currBytes; byteIndex++)
            {
                values[byteIndex] = readBuf[(3 + byteIndex)];
            }

            // Increment iteration count
            values += currBytes;
            address += currBytes;
        }
    }
    else
    {
        uint8_t currBytes = 0;
        uint8_t remainingBytes = numBytes;
        while (remainingBytes > 0)
        {
            if (remainingBytes > 16)
            {
                currBytes = 16;
                remainingBytes -= 16;
            }
            else
            {
                currBytes = remainingBytes;
                remainingBytes = 0;
            }

            pecEn = 0;
            rsvd = 0;
            funcCode = 2;
            start = 1;
            end = 0;

            if (i2cDriver->pecEnable == ARIES_I2C_PEC_ENABLE)
            {
                pecEn = 1;
            }

            int wrBufLength = 4 + pecEn;
            uint8_t writeBuf[wrBufLength];

            ASTERA_TRACE("Reading from address: 0x%08x", address);

            // Construct command code
            wrCmdCode = (pecEn << 7) + (rsvd << 5) + (funcCode << 2) +
                        (start << 1) + (end << 0);

            // Construct Config & Offset Upper byte
            uint8_t config;
            uint8_t addr17;                      // Get 17bit of addr
            addr17 = (address & 0x10000) >> 16;  // Get 17bit of addr
            addr15To8 = (address & 0xff00) >> 8; // Get bits 16:8
            addr7To0 = address & 0xff;           // Get bits 7:0

            uint8_t cfg_type = 0;
            uint8_t bdcst = 0;
            uint8_t burstLen = (currBytes - 1);

            config = (cfg_type << 6) + (bdcst << 4) + (burstLen << 1) +
                     (addr17 << 0);

            writeBuf[0] = wrBufLength - 1;
            writeBuf[1] = config;
            writeBuf[2] = addr15To8;
            writeBuf[3] = addr7To0;

            ASTERA_TRACE("Write:");
            ASTERA_TRACE("    cmdCode = 0x%02x", wrCmdCode);
            ASTERA_TRACE("    writeBuf[0] = 0x%02x", writeBuf[0]);
            ASTERA_TRACE("    writeBuf[1] = 0x%02x", writeBuf[1]);
            ASTERA_TRACE("    writeBuf[2] = 0x%02x", writeBuf[2]);
            ASTERA_TRACE("    writeBuf[3] = 0x%02x", writeBuf[3]);

            if (pecEn)
            {
                // Add address and w bit, and cmd code to wrStream
                // PEC byte is already accounted for
                uint8_t wrStreamLen = wrBufLength + 2;
                uint8_t wrStream[wrStreamLen];
                wrStream[0] = (i2cDriver->slaveAddr << 1) + 0;
                wrStream[1] = wrCmdCode;
                ASTERA_TRACE("        pecBuf[0] = 0x%02x", wrStream[0]);
                ASTERA_TRACE("        pecBuf[1] = 0x%02x", wrStream[1]);

                uint8_t wrStreamIdx;
                for (wrStreamIdx = 2; wrStreamIdx < (wrStreamLen - 1);
                     wrStreamIdx++)
                {
                    wrStream[wrStreamIdx] = writeBuf[(wrStreamIdx - 2)];
                    ASTERA_TRACE("        pecBuf[%d] = 0x%02x", wrStreamIdx,
                                 wrStream[wrStreamIdx]);
                }
                wrStream[(wrStreamLen - 1)] = 0x0; // Addition PEC byte
                ASTERA_TRACE("        pecBuf[last] = 0x0");

                uint8_t wrPec = ariesGetPecByte(wrStream, wrStreamLen);
                writeBuf[4] = wrPec;
                ASTERA_TRACE("    writeBuf[4] = 0x%02x", writeBuf[4]);
            }

            // Set up I2C lock, to keep write and read atomic
            rc = ariesLock(i2cDriver);
            // Do not unblock here on return of error since this error code
            // would mean that the block call did not happen
            CHECK_SUCCESS(rc);

            // Read buffer includes num bytes returned in stream
            uint8_t readBufLen = currBytes + pecEn + 1;
            uint8_t readBuf[readBufLen];

            // Perform read operation
            // Try the read operation upto 3 times before issuing a block read
            // failure
            for (tryIndex = 0; tryIndex < readTryCount; tryIndex++)
            {
                // First write address you wish to read from
                rc = asteraI2CWriteBlockData(i2cDriver->handle, wrCmdCode,
                                             wrBufLength, writeBuf);
                if (rc != ARIES_SUCCESS)
                {
                    lc = ariesUnlock(i2cDriver);
                    CHECK_SUCCESS(lc);
                    return rc;
                }

                funcCode = 2;
                start = 0;
                end = 1;
                rdCmdCode = (pecEn << 7) + (rsvd << 5) + (funcCode << 2) +
                            (start << 1) + (end << 0);

                ASTERA_TRACE("Read:");
                ASTERA_TRACE("    cmdCode = 0x%02x", rdCmdCode);

                // Perform read operation
                // First byte returned is length. Hence add length+1 as bytes to
                // read
                readBytes = asteraI2CReadBlockData(i2cDriver->handle, rdCmdCode,
                                                   readBufLen, readBuf);
                /*printf("Read (Rd): ");*/
                /*printf("0x%02x ", rdCmdCode);*/
                /*for (i = 0; i < readBufLen; i++)*/
                /*{*/
                /*printf("0x%02x ", readBuf[i]);*/
                /*}*/
                /*printf("\n");*/
                if (readBytes != readBufLen)
                {
                    ASTERA_TRACE(
                        "ariesReadBlockData() interrupted by intervening transaction");
                    ASTERA_TRACE("Perform read again ... ");
                    rc = ARIES_I2C_BLOCK_READ_FAILURE;

                    if (tryIndex == (readTryCount - 1))
                    {
                        ASTERA_ERROR("Incorrect num. bytes returned by read");
                        lc = ariesUnlock(i2cDriver);
                        CHECK_SUCCESS(lc);
                        return ARIES_I2C_BLOCK_READ_FAILURE;
                    }
                }
                else
                {
                    rc = ARIES_SUCCESS;
                    break;
                }
            }

            // Unlock previous lock set before write
            rc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(rc);

            // Verify PEC checksum
            if (pecEn)
            {
                // Include write portion of read (addr and wr bit, and cmd Code)
                // and extra read portion (addr and rd bit)
                uint8_t rdStreamLen = readBufLen + 2 + 1;
                uint8_t rdStream[rdStreamLen];
                uint8_t rdPec;

                rdStream[0] = (i2cDriver->slaveAddr << 1) + 0;
                rdStream[1] = rdCmdCode;
                rdStream[2] = (i2cDriver->slaveAddr << 1) + 1;

                uint8_t rdStreamIdx;
                for (rdStreamIdx = 3; rdStreamIdx < rdStreamLen; rdStreamIdx++)
                {
                    rdStream[rdStreamIdx] = readBuf[rdStreamIdx - 3];
                }

                rdPec = ariesGetPecByte(rdStream, rdStreamLen);
                if (rdPec != 0)
                {
                    ASTERA_ERROR("PEC value not as expected");
                    return ARIES_I2C_BLOCK_READ_FAILURE;
                }
            }

            // Print the values
            ASTERA_TRACE("Values read:");
            for (pos = 0; pos < currBytes; pos++)
            {
                values[pos] = readBuf[(pos + 1)] + 0;
                ASTERA_TRACE("    Read_Buf[%d] = 0x%02x", pos, values[pos]);
            }
            values += currBytes;
            address += currBytes;
        }
    }
    return ARIES_SUCCESS;
}

/*
 * Read a data byte from Aries over I2C
 */
AriesErrorType ariesReadByteData(AriesI2CDriverType* i2cDriver,
                                 uint32_t address, uint8_t* values)
{
    return ariesReadBlockData(i2cDriver, address, 1, values);
}

/*
 * Read multiple (up to eight) data bytes from micro SRAM over I2C
 */
AriesErrorType ariesReadBlockDataMainMicroIndirectA0(
    AriesI2CDriverType* i2cDriver, uint32_t microIndStructOffset,
    uint32_t address, uint8_t numBytes, uint8_t* values)
{
    AriesErrorType rc;
    AriesErrorType lc;
    int byteIndex;
    uint8_t dataByte[1];

    // Grab lock
    lc = ariesLock(i2cDriver);
    CHECK_SUCCESS(lc);

    // No multi-byte indirect support here. Hence read a byte at a time
    for (byteIndex = 0; byteIndex < numBytes; byteIndex++)
    {
        // Write eeprom addr
        uint8_t eepromAddrBytes[3];
        int eepromAccAddr = address - AL_MAIN_SRAM_DMEM_OFFSET + byteIndex;
        eepromAddrBytes[0] = eepromAccAddr & 0xff;
        eepromAddrBytes[1] = (eepromAccAddr >> 8) & 0xff;
        eepromAddrBytes[2] = (eepromAccAddr >> 16) & 0xff;
        rc = ariesWriteBlockData(i2cDriver, microIndStructOffset, 3,
                                 eepromAddrBytes);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Write eeprom cmd
        uint8_t eepromCmdByte[1];
        eepromCmdByte[0] = AL_TG_RD_LOC_IND_SRAM;
        rc = ariesWriteByteData(i2cDriver, (microIndStructOffset + 4),
                                eepromCmdByte);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Test successfull access
        uint8_t status = 0xff;
        uint8_t rdata[1];
        uint8_t count = 0;
        while ((status != 0) && (count < 0xff))
        {
            rc = ariesReadByteData(i2cDriver, (microIndStructOffset + 4),
                                   rdata);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }
            status = rdata[0];
            count += 1;
        }

        if (status != 0)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return ARIES_FAILURE_SRAM_IND_ACCESS_TIMEOUT;
        }
        else
        {
            rc = ariesReadByteData(i2cDriver, (microIndStructOffset + 3),
                                   dataByte);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }
            values[(byteIndex)] = dataByte[0];
        }
    }

    lc = ariesUnlock(i2cDriver);
    CHECK_SUCCESS(lc);

    return ARIES_SUCCESS;
}

/*
 * Read multiple (up to eight) data bytes from micro SRAM over I2C
 */
AriesErrorType ariesReadBlockDataMainMicroIndirectMPW(
    AriesI2CDriverType* i2cDriver, uint32_t microIndStructOffset,
    uint32_t address, uint8_t numBytes, uint8_t* values)
{
    AriesErrorType rc;
    AriesErrorType lc;

    if (numBytes > 8)
    {
        return ARIES_INVALID_ARGUMENT;
    }
    else
    {
        // Set aries transaction lock
        lc = ariesLock(i2cDriver);
        CHECK_SUCCESS(lc);

        uint8_t curOffset = 0;
        uint8_t remainingBytes = numBytes;
        uint8_t tmpValue[1];
        while (remainingBytes > 0)
        {
            uint8_t curRemainingBytes;
            if (remainingBytes > 4)
            {
                curRemainingBytes = 4;
                remainingBytes -= 4;
            }
            else
            {
                curRemainingBytes = remainingBytes;
                remainingBytes = 0;
            }

            // Set indirect access command
            tmpValue[0] = ((curRemainingBytes - 1) << 5) |
                          ((((address + curOffset) >> 16) & 0x1) << 1);
            rc = ariesWriteByteData(i2cDriver, microIndStructOffset, tmpValue);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }

            // Set address upper byte
            tmpValue[0] = ((address + curOffset) >> 8) & 0xff;
            rc = ariesWriteByteData(i2cDriver, (microIndStructOffset + 1),
                                    tmpValue);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }

            // Set address lower byte (this triggers indirect access)
            tmpValue[0] = (address + curOffset) & 0xff;
            rc = ariesWriteByteData(i2cDriver, (microIndStructOffset + 2),
                                    tmpValue);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }

            // Test successfull access
            uint8_t status = 0;
            uint8_t rdata[1];
            uint8_t count = 0;
            while ((status == 0) && (count < 0xff))
            {
                rc = ariesReadByteData(i2cDriver, (microIndStructOffset + 0xb),
                                       rdata);
                if (rc != ARIES_SUCCESS)
                {
                    lc = ariesUnlock(i2cDriver);
                    CHECK_SUCCESS(lc);
                    return rc;
                }
                status = rdata[0] & 0x1;
                count += 1;
            }
            if ((status == 0) || (count == 0xff))
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return ARIES_FAILURE_SRAM_IND_ACCESS_TIMEOUT;
            }
            else
            {
                uint8_t i;
                for (i = 0; i < curRemainingBytes; i++)
                {
                    rc = ariesReadByteData(
                        i2cDriver, (microIndStructOffset + 3 + i), tmpValue);
                    if (rc != ARIES_SUCCESS)
                    {
                        lc = ariesUnlock(i2cDriver);
                        CHECK_SUCCESS(lc);
                        return rc;
                    }
                    values[curOffset + i] = tmpValue[0];
                }
            }
            curOffset += curRemainingBytes;
        }

        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);

        return ARIES_SUCCESS;
    }
}

/*
 * Write multiple (up to eight) data bytes to micro SRAM over I2C
 */
AriesErrorType ariesWriteBlockDataMainMicroIndirectA0(
    AriesI2CDriverType* i2cDriver, uint32_t microIndStructOffset,
    uint32_t address, uint8_t numBytes, uint8_t* values)
{
    AriesErrorType rc;
    AriesErrorType lc;
    int byteIndex;
    uint8_t dataByte[1];

    lc = ariesLock(i2cDriver);
    CHECK_SUCCESS(lc);

    // No multi-byte indirect support here. Hence read a byte at a time
    for (byteIndex = 0; byteIndex < numBytes; byteIndex++)
    {
        // Write eeprom addr
        uint8_t eepromAddrBytes[3];
        int eepromAccAddr = address - AL_MAIN_SRAM_DMEM_OFFSET + byteIndex;
        eepromAddrBytes[0] = eepromAccAddr & 0xff;
        eepromAddrBytes[1] = (eepromAccAddr >> 8) & 0xff;
        eepromAddrBytes[2] = (eepromAccAddr >> 16) & 0xff;
        rc = ariesWriteBlockData(i2cDriver, microIndStructOffset, 3,
                                 eepromAddrBytes);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Write data
        dataByte[0] = values[byteIndex];
        rc = ariesWriteByteData(i2cDriver, (microIndStructOffset + 3),
                                dataByte);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Write eeprom cmd
        uint8_t eepromCmdByte[1];
        eepromCmdByte[0] = AL_TG_WR_LOC_IND_SRAM;
        rc = ariesWriteByteData(i2cDriver, (microIndStructOffset + 4),
                                eepromCmdByte);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Test successfull access
        uint8_t status = 0xff;
        uint8_t rdata[1];
        uint8_t count = 0;
        while ((status != 0) && (count < 0xff))
        {
            rc = ariesReadByteData(i2cDriver, (microIndStructOffset + 4),
                                   rdata);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }
            status = rdata[0];
            count += 1;
        }

        if (status != 0)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return ARIES_FAILURE_SRAM_IND_ACCESS_TIMEOUT;
        }
    }

    lc = ariesUnlock(i2cDriver);
    CHECK_SUCCESS(lc);

    return ARIES_SUCCESS;
}

/*
 * Write multiple (up to eight) data bytes to micro SRAM over I2C
 */
AriesErrorType ariesWriteBlockDataMainMicroIndirectMPW(
    AriesI2CDriverType* i2cDriver, uint32_t microIndStructOffset,
    uint32_t address, uint8_t numBytes, uint8_t* values)
{
    uint8_t tmpValue[1];
    uint8_t curOffset = 0;
    AriesErrorType rc;
    AriesErrorType lc;
    if (numBytes > 8)
    {
        return ARIES_INVALID_ARGUMENT;
    }
    else
    {
        lc = ariesLock(i2cDriver);
        CHECK_SUCCESS(lc);

        uint8_t remainingBytes = numBytes;
        while (remainingBytes > 0)
        {
            uint8_t curRemainingBytes;
            if (remainingBytes > 4)
            {
                curRemainingBytes = 4;
                remainingBytes -= 4;
            }
            else
            {
                curRemainingBytes = remainingBytes;
                remainingBytes = 0;
            }

            // Set indirect access command
            tmpValue[0] = ((curRemainingBytes - 1) << 5) |
                          ((((address + curOffset) >> 16) & 0x1) << 1) | 0x1;
            rc = ariesWriteByteData(i2cDriver, microIndStructOffset, tmpValue);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }

            // Set the data byte(s)
            uint8_t i;
            for (i = 0; i < curRemainingBytes; i++)
            {
                tmpValue[0] = values[curOffset + i];
                rc = ariesWriteByteData(i2cDriver, microIndStructOffset + 3 + i,
                                        tmpValue);
                if (rc != ARIES_SUCCESS)
                {
                    lc = ariesUnlock(i2cDriver);
                    CHECK_SUCCESS(lc);
                    return rc;
                }
            }

            // Set address upper byte
            tmpValue[0] = ((address + curOffset) >> 8) & 0xff;
            rc = ariesWriteByteData(i2cDriver, microIndStructOffset + 1,
                                    tmpValue);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }

            // Set address lower byte (this triggers indirect access)
            tmpValue[0] = (address + curOffset) & 0xff;
            rc = ariesWriteByteData(i2cDriver, microIndStructOffset + 2,
                                    tmpValue);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }

            // Test successfull access
            uint8_t status = 0;
            uint8_t rdata[1];
            uint8_t count = 0;
            while ((status == 0) && (count < 0xff))
            {
                rc = ariesReadByteData(i2cDriver, microIndStructOffset + 0xb,
                                       rdata);
                if (rc != ARIES_SUCCESS)
                {
                    lc = ariesUnlock(i2cDriver);
                    CHECK_SUCCESS(lc);
                    return rc;
                }
                status = rdata[0] & 0x1;
                count += 1;
            }
            if ((status == 0) || (count == 0xff))
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return ARIES_FAILURE_SRAM_IND_ACCESS_TIMEOUT;
            }
            curOffset += curRemainingBytes;
        }

        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);

        return ARIES_SUCCESS;
    }
}

/*
 * Read a data byte from Main micro SRAM over I2C
 */
AriesErrorType ariesReadByteDataMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                                  uint32_t address,
                                                  uint8_t* value)
{
    AriesErrorType rc;
#ifdef ARIES_MPW
    rc = ariesReadBlockDataMainMicroIndirectMPW(i2cDriver, 0xe00, address, 1,
                                                value);
#else
    rc = ariesReadBlockDataMainMicroIndirectA0(i2cDriver, 0xd99, address, 1,
                                               value);
#endif
    return rc;
}

/*
 * Read multiple (up to eight) data bytes from Main micro SRAM over I2C
 */
AriesErrorType
    ariesReadBlockDataMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                        uint32_t address, uint8_t numBytes,
                                        uint8_t* values)
{
    AriesErrorType rc;
#ifdef ARIES_MPW
    rc = ariesReadBlockDataMainMicroIndirectMPW(i2cDriver, 0xe00, address,
                                                numBytes, values);
#else
    rc = ariesReadBlockDataMainMicroIndirectA0(i2cDriver, 0xd99, address,
                                               numBytes, values);
#endif
    return rc;
}

/*
 * Write a data byte to Main micro SRAM over I2C
 */
AriesErrorType
    ariesWriteByteDataMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                        uint32_t address, uint8_t* value)
{
    AriesErrorType rc;
#ifdef ARIES_MPW
    rc = ariesWriteBlockDataMainMicroIndirectMPW(i2cDriver, 0xe00, address, 1,
                                                 value);
#else
    rc = ariesWriteBlockDataMainMicroIndirectA0(i2cDriver, 0xd99, address, 1,
                                                value);
#endif
    return rc;
}

/*
 * Write multiple (up to eight) data bytes to Main micro SRAM over I2C
 */
AriesErrorType
    ariesWriteBlockDataMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                         uint32_t address, uint8_t numBytes,
                                         uint8_t* values)
{
    AriesErrorType rc;
#ifdef ARIES_MPW
    rc = ariesWriteBlockDataMainMicroIndirectMPW(i2cDriver, 0xe00, address,
                                                 numBytes, values);
#else
    rc = ariesWriteBlockDataMainMicroIndirectA0(i2cDriver, 0xd99, address,
                                                numBytes, values);
#endif
    return rc;
}

/*
 * Read multiple (up to eight) data bytes from Path micro SRAM over I2C
 */
AriesErrorType
    ariesReadBlockDataPathMicroIndirect(AriesI2CDriverType* i2cDriver,
                                        uint8_t pathID, uint32_t address,
                                        uint8_t numBytes, uint8_t* values)
{
    if (pathID > 15)
    {
        return ARIES_INVALID_ARGUMENT;
    }

    AriesErrorType rc;
    uint32_t microIndStructOffset = 0x4200 + (pathID * ARIES_PATH_WRP_STRIDE);

    rc = ariesReadBlockDataMainMicroIndirectMPW(i2cDriver, microIndStructOffset,
                                                address, numBytes, values);

    return rc;
}

/*
 * Read a data byte from Path micro SRAM over I2C
 */
AriesErrorType ariesReadByteDataPathMicroIndirect(AriesI2CDriverType* i2cDriver,
                                                  uint8_t pathID,
                                                  uint32_t address,
                                                  uint8_t* value)
{
    AriesErrorType rc;
    rc = ariesReadBlockDataPathMicroIndirect(i2cDriver, pathID, address, 1,
                                             value);
    return rc;
}

/*
 * Write multiple (up to eight) data bytes to Path micro over I2C
 */
AriesErrorType
    ariesWriteBlockDataPathMicroIndirect(AriesI2CDriverType* i2cDriver,
                                         uint8_t pathID, uint32_t address,
                                         uint8_t numBytes, uint8_t* values)
{
    if (pathID > 15)
    {
        return ARIES_INVALID_ARGUMENT;
    }

    AriesErrorType rc;
    uint32_t microIndStructOffset = 0x4200 + (pathID * ARIES_PATH_WRP_STRIDE);

    rc = ariesWriteBlockDataMainMicroIndirectMPW(
        i2cDriver, microIndStructOffset, address, numBytes, values);

    return rc;
}

/*
 * Write a data byte to Path micro SRAM over I2C
 */
AriesErrorType
    ariesWriteByteDataPathMicroIndirect(AriesI2CDriverType* i2cDriver,
                                        uint8_t pathID, uint32_t address,
                                        uint8_t* value)
{
    AriesErrorType rc;
    rc = ariesWriteBlockDataPathMicroIndirect(i2cDriver, pathID, address, 1,
                                              value);
    return rc;
}

/*
 * Read 2 bytes of data from PMA register over I2C
 */
AriesErrorType ariesReadWordPmaIndirect(AriesI2CDriverType* i2cDriver, int side,
                                        int quadSlice, uint16_t address,
                                        uint8_t* values)
{
    uint8_t cmd;
    uint8_t addr15To8;
    uint8_t addr7To0;
    uint8_t dataByte[1];
    AriesErrorType rc;
    AriesErrorType lc;
    int regAddr;

    // Set value for command register based on PMA side
    // A = 1, B = 0
    cmd = 0;
    if (side == 0) // B
    {
        cmd |= (0x1 << 1);
    }
    else if (side == 1) // A
    {
        cmd |= (0x2 << 1);
    }
    else
    {
        return ARIES_INVALID_ARGUMENT;
    }

    // Split address into 2 bytes
    addr15To8 = (address >> 8) & 0xff;
    addr7To0 = address & 0xff;

    lc = ariesLock(i2cDriver);
    CHECK_SUCCESS(lc);

    // Write Cmd reg
    dataByte[0] = cmd;
    regAddr = ARIES_PMA_QS0_CMD_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesWriteByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Write upper bytes of address
    dataByte[0] = addr15To8;
    regAddr = ARIES_PMA_QS0_ADDR_1_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesWriteByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Write lower bytes of address
    dataByte[0] = addr7To0;
    regAddr = ARIES_PMA_QS0_ADDR_0_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesWriteByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }
    usleep(ARIES_PMA_REG_ACCESS_TIME_US);

    // Read data (lower and upper bits)
    // Lower bits at data0 and upper bits at data1
    regAddr = ARIES_PMA_QS0_DATA_0_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesReadByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }
    values[0] = dataByte[0];

    regAddr = ARIES_PMA_QS0_DATA_1_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesReadByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }
    values[1] = dataByte[0];

    lc = ariesUnlock(i2cDriver);
    CHECK_SUCCESS(lc);

    return ARIES_SUCCESS;
}

/*
 * Write 2 bytes of data to PMA register over I2C
 */
AriesErrorType ariesWriteWordPmaIndirect(AriesI2CDriverType* i2cDriver,
                                         int side, int quadSlice,
                                         uint16_t address, uint8_t* values)
{
    uint8_t cmd;
    uint8_t addr15To8;
    uint8_t addr7To0;
    uint8_t dataByte[1];
    AriesErrorType rc;
    AriesErrorType lc;
    int regAddr;

    // Set value for command register based on PMA side
    // A = 1, B = 0, Both = 2
    cmd = 1;
    if (side == 0)
    {
        cmd |= (0x1 << 1);
    }
    else if (side == 1)
    {
        cmd |= (0x2 << 1);
    }
    else if (side == 2)
    {
        cmd |= (0x3 << 1);
    }
    else
    {
        return ARIES_INVALID_ARGUMENT;
    }

    // Split address into 2 bytes
    addr15To8 = (address >> 8) & 0xff;
    addr7To0 = address & 0xff;

    lc = ariesLock(i2cDriver);
    CHECK_SUCCESS(lc);

    // Write command reg
    dataByte[0] = cmd;
    regAddr = ARIES_PMA_QS0_CMD_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesWriteByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Write data word, LSByte (data0) and MSByte (data1)
    dataByte[0] = values[0];
    regAddr = ARIES_PMA_QS0_DATA_0_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesWriteByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    dataByte[0] = values[1];
    regAddr = ARIES_PMA_QS0_DATA_1_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesWriteByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Write address (upper bits)
    dataByte[0] = addr15To8;
    regAddr = ARIES_PMA_QS0_ADDR_1_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesWriteByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Write address (lower bits)
    dataByte[0] = addr7To0;
    regAddr = ARIES_PMA_QS0_ADDR_0_ADDRESS + (quadSlice * ARIES_QS_STRIDE);
    rc = ariesWriteByteData(i2cDriver, regAddr, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    lc = ariesUnlock(i2cDriver);
    CHECK_SUCCESS(lc);

    usleep(ARIES_PMA_REG_ACCESS_TIME_US);

    return ARIES_SUCCESS;
}

/*
 * Read PMA lane registers
 */
AriesErrorType ariesReadWordPmaLaneIndirect(AriesI2CDriverType* i2cDriver,
                                            int side, int quadSlice, int lane,
                                            uint16_t regOffset, uint8_t* values)
{
    AriesErrorType rc;
    uint16_t address;
    if (lane < 4)
    {
        // 0x200 is the lane offset in a PMA
        address = regOffset + (lane * ARIES_PMA_LANE_STRIDE);
    }
    else
    {
        // This is not a lane type read
        // Treat this as a regular Pma read
        address = regOffset;
    }

    rc = ariesReadWordPmaIndirect(i2cDriver, side, quadSlice, address, values);
    CHECK_SUCCESS(rc);
    return ARIES_SUCCESS;
}

/*
 * Write PMA lane registers
 */
AriesErrorType ariesWriteWordPmaLaneIndirect(AriesI2CDriverType* i2cDriver,
                                             int side, int quadSlice, int lane,
                                             uint16_t regOffset,
                                             uint8_t* values)
{
    AriesErrorType rc;
    uint16_t address;
    if (lane < 4)
    {
        // 0x200 is the lane offset in a PMA
        address = regOffset + (lane * ARIES_PMA_LANE_STRIDE);
    }
    else
    {
        // This is not a lane type read
        // Treat this as a regular Pma read
        address = regOffset;
    }

    rc = ariesWriteWordPmaIndirect(i2cDriver, side, quadSlice, address, values);
    CHECK_SUCCESS(rc);
    return ARIES_SUCCESS;
}

/*
 * Read data from PMA reg via Main Micro
 */
AriesErrorType ariesReadWordPmaMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                                 int side, int quadSlice,
                                                 uint16_t pmaAddr,
                                                 uint8_t* data)
{
    AriesErrorType rc;
    AriesErrorType lc;

    lc = ariesLock(i2cDriver);
    CHECK_SUCCESS(lc);

    // Write address (3 bytes)
    uint8_t addr[3];
    uint32_t address = ((uint32_t)(quadSlice * 4) << 20) | (uint32_t)pmaAddr;
    addr[0] = address & 0xff;
    addr[1] = (address >> 8) & 0xff;
    addr[2] = (address >> 16) & 0xff;
    rc = ariesWriteBlockData(i2cDriver, ARIES_MM_ASSIST_REG_ADDR_OFFSET, 3,
                             addr);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Set command
    uint8_t dataByte[1];
    if (side == 0)
    {
        dataByte[0] = ARIES_MM_RD_PID_IND_PMA0;
    }
    else
    {
        dataByte[0] = ARIES_MM_RD_PID_IND_PMA1;
    }

    rc = ariesWriteBlockData(i2cDriver, ARIES_MM_ASSIST_CMD_OFFSET, 1,
                             dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Check access status
    int count = 0;
    int status = 1;
    while ((status != 0) && (count < 100))
    {
        rc = ariesReadBlockData(i2cDriver, ARIES_MM_ASSIST_CMD_OFFSET, 1,
                                dataByte);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }
        status = dataByte[0];

        usleep(ARIES_MM_STATUS_TIME);
        count += 1;
    }

    if (status != 0)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return ARIES_PMA_MM_ACCESS_FAILURE;
    }

    // Read 2 bytes of data
    rc = ariesReadBlockData(i2cDriver, ARIES_MM_ASSIST_DATA_OFFSET, 1,
                            dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }
    data[0] = dataByte[0];
    rc = ariesReadBlockData(i2cDriver, ARIES_MM_ASSIST_STS_OFFSET, 1, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }
    data[1] = dataByte[0];

    lc = ariesUnlock(i2cDriver);
    CHECK_SUCCESS(lc);

    return ARIES_SUCCESS;
}

/*
 * Write data to PMA reg via Main Micro
 */
AriesErrorType ariesWriteWordPmaMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                                  int side, int quadSlice,
                                                  uint16_t pmaAddr,
                                                  uint8_t* data)
{
    AriesErrorType rc;
    AriesErrorType lc;

    uint8_t dataByte[1];

    lc = ariesLock(i2cDriver);
    CHECK_SUCCESS(lc);

    // Write address (3 bytes)
    uint8_t addr[3];
    uint32_t address = ((uint32_t)(quadSlice * 4) << 20) | (uint32_t)pmaAddr;
    addr[0] = address & 0xff;
    addr[1] = (address >> 8) & 0xff;
    addr[2] = (address >> 16) & 0xff;
    rc = ariesWriteBlockData(i2cDriver, ARIES_MM_ASSIST_REG_ADDR_OFFSET, 3,
                             addr);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Write 2 bytes of data
    dataByte[0] = data[0];
    rc = ariesWriteBlockData(i2cDriver, ARIES_MM_ASSIST_DATA_OFFSET, 1,
                             dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }
    dataByte[0] = data[1];
    rc = ariesWriteBlockData(i2cDriver, ARIES_MM_ASSIST_STS_OFFSET, 1,
                             dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Set command
    if (side == 0)
    {
        dataByte[0] = ARIES_MM_WR_PID_IND_PMA0;
    }
    if (side == 1)
    {
        dataByte[0] = ARIES_MM_WR_PID_IND_PMA1;
    }
    else
    {
        dataByte[0] = ARIES_MM_WR_PID_IND_PMAX;
    }

    rc = ariesWriteBlockData(i2cDriver, ARIES_MM_ASSIST_CMD_OFFSET, 1,
                             dataByte);
    if (rc != ARIES_SUCCESS)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return rc;
    }

    // Check access status
    int count = 0;
    int status = 1;
    while ((status != 0) && (count < 100))
    {
        rc = ariesReadBlockData(i2cDriver, ARIES_MM_ASSIST_CMD_OFFSET, 1,
                                dataByte);
        CHECK_SUCCESS(rc);
        status = dataByte[0];

        usleep(ARIES_MM_STATUS_TIME);
        count += 1;
    }

    if (status != 0)
    {
        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
        return ARIES_PMA_MM_ACCESS_FAILURE;
    }

    lc = ariesUnlock(i2cDriver);
    CHECK_SUCCESS(lc);

    return ARIES_SUCCESS;
}

/*
 * Read PMA lane registers
 */
AriesErrorType
    ariesReadWordPmaLaneMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                          int side, int quadSlice, int lane,
                                          uint16_t regOffset, uint8_t* values)
{
    AriesErrorType rc;
    uint16_t address;
    if (lane < 4)
    {
        // 0x200 is the lane offset in a PMA
        address = regOffset + (lane * ARIES_PMA_LANE_STRIDE);
    }
    else
    {
        // This is not a lane type read
        // Treat this as a regular Pma read
        address = regOffset;
    }

    rc = ariesReadWordPmaMainMicroIndirect(i2cDriver, side, quadSlice, address,
                                           values);
    CHECK_SUCCESS(rc);
    return ARIES_SUCCESS;
}

/*
 * Write PMA lane registers
 */
AriesErrorType
    ariesWriteWordPmaLaneMainMicroIndirect(AriesI2CDriverType* i2cDriver,
                                           int side, int quadSlice, int lane,
                                           uint16_t regOffset, uint8_t* values)
{
    AriesErrorType rc;
    uint16_t address;
    if (lane < 4)
    {
        // 0x200 is the lane offset in a PMA
        address = regOffset + (lane * ARIES_PMA_LANE_STRIDE);
    }
    else
    {
        // This is not a lane type read
        // Treat this as a regular Pma read
        address = regOffset;
    }

    rc = ariesWriteWordPmaMainMicroIndirect(i2cDriver, side, quadSlice, address,
                                            values);
    CHECK_SUCCESS(rc);
    return ARIES_SUCCESS;
}

/*
 * Read Modify Write PMA lane registers
 */
AriesErrorType ariesReadWriteWordPmaLaneMainMicroIndirect(
    AriesI2CDriverType* i2cDriver, int side, int quadSlice, int quadSliceLane,
    uint16_t pmaAddr, int offset, uint16_t value, int width)
{
    AriesErrorType rc;
    uint8_t dataWord[2];

    rc = ariesReadWordPmaLaneMainMicroIndirect(
        i2cDriver, side, quadSlice, quadSliceLane, pmaAddr, dataWord);
    CHECK_SUCCESS(rc)

    uint16_t mask = 0xFFFF;
    mask <<= (width + offset);
    mask += (1 << offset) - 1;

    dataWord[0] &= mask;
    dataWord[1] &= mask >> 8;

    uint16_t orMask = 0;
    uint16_t valueMask = ((1 << width) - 1);
    orMask = (value & valueMask) << offset;

    if (value >= (1 << width))
    {
        ASTERA_WARN(
            "Trying to write a value greater than the width of the bitfield. Truncation occurred.");
    }

    dataWord[0] |= orMask;
    dataWord[1] |= orMask >> 8;

    rc = ariesWriteWordPmaLaneMainMicroIndirect(
        i2cDriver, side, quadSlice, quadSliceLane, pmaAddr, dataWord);
    CHECK_SUCCESS(rc)

    return ARIES_SUCCESS;
}

/**
 * Read N bytes of data from a Retimer (gbl, ln0, or ln1) CSR.
 */
AriesErrorType ariesReadRetimerRegister(AriesI2CDriverType* i2cDriver, int side,
                                        int lane, uint16_t baseAddr,
                                        uint8_t numBytes, uint8_t* data)
{
    AriesErrorType rc;
    uint8_t ret_pth_ln = lane % 2;
    uint8_t pth_wrap;
    uint8_t qs = floor(lane / 4);
    uint32_t addr;
    if (side == 1) // Side "A" - Even paths
    {
        pth_wrap = (2 * floor((lane % 4) / 2));
    }
    else if (side == 0) // Side "B" - Odd paths
    {
        pth_wrap = (2 * floor((lane % 4) / 2)) + 1;
    }
    else
    {
        return ARIES_INVALID_ARGUMENT;
    }
    addr = baseAddr + ret_pth_ln * ARIES_PATH_LANE_STRIDE +
           pth_wrap * ARIES_PATH_WRP_STRIDE + qs * ARIES_QS_STRIDE;
    rc = ariesReadBlockData(i2cDriver, addr, numBytes, data);
    CHECK_SUCCESS(rc);
    return ARIES_SUCCESS;
}

/**
 * Write N bytes of data to a Retimer (gbl, ln0, or ln1) CSR.
 */
AriesErrorType ariesWriteRetimerRegister(AriesI2CDriverType* i2cDriver,
                                         int side, int lane, uint16_t baseAddr,
                                         uint8_t numBytes, uint8_t* data)
{
    AriesErrorType rc;
    uint8_t ret_pth_ln = lane % 2;
    uint8_t pth_wrap;
    uint8_t qs = floor(lane / 4);
    uint32_t addr;
    if (side == 1) // Side "A" - Even paths
    {
        pth_wrap = (2 * floor((lane % 4) / 2));
    }
    else if (side == 0) // Side "B" - Odd paths
    {
        pth_wrap = (2 * floor((lane % 4) / 2)) + 1;
    }
    else
    {
        return ARIES_INVALID_ARGUMENT;
    }
    addr = baseAddr + ret_pth_ln * ARIES_PATH_LANE_STRIDE +
           pth_wrap * ARIES_PATH_WRP_STRIDE + qs * ARIES_QS_STRIDE;
    rc = ariesWriteBlockData(i2cDriver, addr, numBytes, data);
    CHECK_SUCCESS(rc);
    return ARIES_SUCCESS;
}

AriesErrorType ariesReadWideRegister(AriesI2CDriverType* i2cDriver,
                                     uint32_t address, uint8_t width,
                                     uint8_t* values)
{
    if (i2cDriver->mmWideRegisterValid)
    {
        AriesErrorType rc;
        AriesErrorType lc;
        uint8_t dataByte[1];
        uint8_t addr[3];

        lc = ariesLock(i2cDriver);
        CHECK_SUCCESS(lc);

        // Write address (3 bytes)
        addr[0] = address & 0xff;
        addr[1] = (address >> 8) & 0xff;
        addr[2] = (address >> 16) & 0x01;
        rc = ariesWriteBlockData(i2cDriver, ARIES_MM_ASSIST_SPARE_0_OFFSET, 3,
                                 addr);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Set command based on width
        if (width == 2)
        {
            dataByte[0] = ARIES_MM_RD_WIDE_REG_2B;
        }
        else if (width == 3)
        {
            dataByte[0] = ARIES_MM_RD_WIDE_REG_3B;
        }
        else if (width == 4)
        {
            dataByte[0] = ARIES_MM_RD_WIDE_REG_4B;
        }
        else if (width == 5)
        {
            dataByte[0] = ARIES_MM_RD_WIDE_REG_5B;
        }
        else
        {
            return ARIES_INVALID_ARGUMENT;
        }

        rc = ariesWriteByteData(i2cDriver, ARIES_MM_ASSIST_CMD_OFFSET,
                                dataByte);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Check access status
        int count = 0;
        int status = 0xff;
        while ((status != 0) && (count < 100))
        {
            rc = ariesReadByteData(i2cDriver, ARIES_MM_ASSIST_CMD_OFFSET,
                                   dataByte);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }
            status = dataByte[0];

            usleep(ARIES_MM_STATUS_TIME);
            count += 1;
        }

        if (status != 0)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return ARIES_PMA_MM_ACCESS_FAILURE;
        }

        // Read N bytes of data based on width
        rc = ariesReadBlockData(i2cDriver, ARIES_MM_ASSIST_SPARE_3_OFFSET,
                                width, values);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
    }
    else
    {
        return ariesReadBlockData(i2cDriver, address, width, values);
    }

    return ARIES_SUCCESS;
}

AriesErrorType ariesWriteWideRegister(AriesI2CDriverType* i2cDriver,
                                      uint32_t address, uint8_t width,
                                      uint8_t* values)
{
    if (i2cDriver->mmWideRegisterValid)
    {
        AriesErrorType rc;
        AriesErrorType lc;
        uint8_t dataByte[1];
        uint8_t addr[3];

        lc = ariesLock(i2cDriver);
        CHECK_SUCCESS(lc);

        // Write address (3 bytes)
        addr[0] = address & 0xff;
        addr[1] = (address >> 8) & 0xff;
        addr[2] = (address >> 16) & 0x01;
        rc = ariesWriteBlockData(i2cDriver, ARIES_MM_ASSIST_SPARE_0_OFFSET, 3,
                                 addr);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Write N bytes of data based on width
        rc = ariesWriteBlockData(i2cDriver, ARIES_MM_ASSIST_SPARE_3_OFFSET,
                                 width, values);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Set command based on width
        if (width == 2)
        {
            dataByte[0] = ARIES_MM_WR_WIDE_REG_2B;
        }
        else if (width == 3)
        {
            dataByte[0] = ARIES_MM_WR_WIDE_REG_3B;
        }
        else if (width == 4)
        {
            dataByte[0] = ARIES_MM_WR_WIDE_REG_4B;
        }
        else if (width == 5)
        {
            dataByte[0] = ARIES_MM_WR_WIDE_REG_5B;
        }
        else
        {
            return ARIES_INVALID_ARGUMENT;
        }

        rc = ariesWriteByteData(i2cDriver, ARIES_MM_ASSIST_CMD_OFFSET,
                                dataByte);
        if (rc != ARIES_SUCCESS)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return rc;
        }

        // Check access status
        int count = 0;
        int status = 0xff;
        while ((status != 0) && (count < 100))
        {
            rc = ariesReadByteData(i2cDriver, ARIES_MM_ASSIST_CMD_OFFSET,
                                   dataByte);
            if (rc != ARIES_SUCCESS)
            {
                lc = ariesUnlock(i2cDriver);
                CHECK_SUCCESS(lc);
                return rc;
            }
            status = dataByte[0];

            usleep(ARIES_MM_STATUS_TIME);
            count += 1;
        }

        if (status != 0)
        {
            lc = ariesUnlock(i2cDriver);
            CHECK_SUCCESS(lc);
            return ARIES_PMA_MM_ACCESS_FAILURE;
        }

        lc = ariesUnlock(i2cDriver);
        CHECK_SUCCESS(lc);
    }
    else
    {
        return ariesWriteBlockData(i2cDriver, address, width, values);
    }

    return ARIES_SUCCESS;
}

/*
 * Lock I2C driver
 */
AriesErrorType ariesLock(AriesI2CDriverType* i2cDriver)
{
    AriesErrorType rc;
    rc = 0;
    if (i2cDriver->lock >= 1)
    {
        i2cDriver->lock++;
    }
    else
    {
        rc = asteraI2CBlock(i2cDriver->handle);
        i2cDriver->lock = 1;
    }

    return rc;
}

/*
 * Unlock I2C driver
 */
AriesErrorType ariesUnlock(AriesI2CDriverType* i2cDriver)
{
    AriesErrorType rc;
    rc = 0;
    if (i2cDriver->lock > 1)
    {
        i2cDriver->lock--;
    }
    else
    {
        rc = asteraI2CUnblock(i2cDriver->handle);
        i2cDriver->lock = 0;
    }

    return rc;
}

#ifdef __cplusplus
}
#endif
