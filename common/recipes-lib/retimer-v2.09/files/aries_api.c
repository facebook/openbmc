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
 * @file aries_api.c
 * @brief Implementation of public functions for the SDK.
 */

#include "include/aries_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Return the SDK version
 */
const char* ariesGetSDKVersion(void)
{
    return ARIES_SDK_VERSION;
}

/*
 * Initialize the device data structure
 */
AriesErrorType ariesInitDevice(AriesDeviceType* device, uint8_t recoveryAddr)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint8_t dataWord[2];
    uint8_t dataBytes[4];

    rc = ariesCheckConnectionHealth(device, recoveryAddr);
    CHECK_SUCCESS(rc);

    // Set lock = 0 (if it hasnt been set before)
    if (device->i2cDriver->lockInit == 0)
    {
        device->i2cDriver->lock = 0;
        device->i2cDriver->lockInit = 1;
    }

    rc = ariesFWStatusCheck(device);
    CHECK_SUCCESS(rc);

    // Initialize eeprom struct by setting all values to default values
    device->eeprom.pageSize = ARIES_EEPROM_PAGE_SIZE;
    device->eeprom.pageCount = ARIES_EEPROM_PAGE_COUNT;
    device->eeprom.maxBurstSize = ARIES_MAX_BURST_SIZE;

    // Initialize MM-assist FW update parameters (optimizations made starting
    // with FW 1.24.0 and later)
    if ((device->fwVersion.major < 1) ||
        ((device->fwVersion.major == 1) && (device->fwVersion.minor < 24)))
    {
        // Use non-optimized FW update parameters
        device->eeprom.blockBaseAddr = ARIES_EEPROM_BLOCK_BASE_ADDR_WIDE;
        device->eeprom.blockCmdModifier = ARIES_EEPROM_BLOCK_CMD_MODIFIER_WIDE;
        device->eeprom.blockWriteSize = ARIES_EEPROM_BLOCK_WRITE_SIZE_WIDE;
    }
    else
    {
        // Use optimized FW update parameters
        device->eeprom.blockBaseAddr = ARIES_EEPROM_BLOCK_BASE_ADDR_NOWIDE;
        device->eeprom.blockCmdModifier =
            ARIES_EEPROM_BLOCK_CMD_MODIFIER_NOWIDE;
        device->eeprom.blockWriteSize = ARIES_EEPROM_BLOCK_WRITE_SIZE_NOWIDE;
    }

    // Capture vendor id, device id and rev number
    rc = ariesReadBlockData(device->i2cDriver, 0x4, 4, dataBytes);
    CHECK_SUCCESS(rc);
    device->vendorId = ((dataBytes[3] << 8) + dataBytes[2]);
    device->deviceId = dataBytes[1];
    device->revNumber = dataBytes[0];

    // Get link_path_struct size
    // Prior to FW 1.1.52, this size is 38
    device->linkPathStructSize = ARIES_LINK_PATH_STRUCT_SIZE;
    if ((device->fwVersion.major >= 1) && (device->fwVersion.minor >= 1) &&
        (device->fwVersion.build >= 52))
    {
        rc = ariesReadBlockDataMainMicroIndirect(
            device->i2cDriver, ARIES_LINK_PATH_STRUCT_SIZE_ADDR, 1, dataByte);
        CHECK_SUCCESS(rc);
        device->linkPathStructSize = dataByte[0];
    }
    else if ((device->fwVersion.major >= 1) && (device->fwVersion.minor >= 2))
    {
        rc = ariesReadBlockDataMainMicroIndirect(
            device->i2cDriver, ARIES_LINK_PATH_STRUCT_SIZE_ADDR, 1, dataByte);
        CHECK_SUCCESS(rc);
        device->linkPathStructSize = dataByte[0];
    }

    // Get the al print info struct offset for Main Micro
    rc = ariesReadBlockDataMainMicroIndirect(
        device->i2cDriver,
        (ARIES_MAIN_MICRO_FW_INFO + ARIES_MM_AL_PRINT_INFO_STRUCT_ADDR), 2,
        dataWord);
    CHECK_SUCCESS(rc);
    device->mm_print_info_struct_addr = AL_MAIN_SRAM_DMEM_OFFSET +
                                        (dataWord[1] << 8) + dataWord[0];

    // Get the gp ctrl status struct offset for Main Micro
    rc = ariesReadBlockDataMainMicroIndirect(
        device->i2cDriver,
        (ARIES_MAIN_MICRO_FW_INFO + ARIES_MM_GP_CTRL_STS_STRUCT_ADDR), 2,
        dataWord);
    CHECK_SUCCESS(rc);
    device->mm_gp_ctrl_sts_struct_addr = AL_MAIN_SRAM_DMEM_OFFSET +
                                         (dataWord[1] << 8) + dataWord[0];

    // Get AL print info struct address for path micros
    // All Path Micros will have same address, so get for PM 4 (present on both
    // x16 and x8 devices)
    rc = ariesReadBlockDataPathMicroIndirect(
        device->i2cDriver, 4,
        (ARIES_PATH_MICRO_FW_INFO_ADDRESS + ARIES_PM_AL_PRINT_INFO_STRUCT_ADDR),
        2, dataWord);
    CHECK_SUCCESS(rc);
    device->pm_print_info_struct_addr = AL_PATH_SRAM_DMEM_OFFSET +
                                        (dataWord[1] << 8) + dataWord[0];

    // Get GP ctrl status struct address for path micros
    // All Path Micros will have same address, so get for PM 4 (present on both
    // x16 and x8 devices)
    rc = ariesReadBlockDataPathMicroIndirect(
        device->i2cDriver, 4,
        (ARIES_PATH_MICRO_FW_INFO_ADDRESS + ARIES_PM_GP_CTRL_STS_STRUCT_ADDR),
        2, dataWord);
    CHECK_SUCCESS(rc);
    device->pm_gp_ctrl_sts_struct_addr = AL_PATH_SRAM_DMEM_OFFSET +
                                         (dataWord[1] << 8) + dataWord[0];

    rc = ariesGetTempCalibrationCodes(device);
    CHECK_SUCCESS(rc);

    rc = ariesGetPinMap(device);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Check the status of firmware
 */
AriesErrorType ariesFWStatusCheck(AriesDeviceType* device)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint8_t dataWord[2];
    uint8_t dataBytes[4];

    // Read Code Load reg
    rc = ariesReadBlockData(device->i2cDriver, ARIES_CODE_LOAD_REG, 1,
                            dataBytes);
    CHECK_SUCCESS(rc);

    if (dataBytes[0] < 0xe)
    {
        ASTERA_WARN("Code Load reg unexpected. Not all modules are loaded");
        device->codeLoadOkay = false;
    }
    else
    {
        device->codeLoadOkay = true;
    }

    // Check Main Micro heartbeat
    // If heartbeat value does not change for 100 tries, no MM heartbeat
    // Else heartbeat present even if one value changes
    uint8_t numTries = 100;
    uint8_t tryIndex = 0;
    uint8_t heartbeatVal;
    bool heartbeatSet = false;
    rc = ariesReadByteData(device->i2cDriver, ARIES_MM_HEARTBEAT_ADDR,
                           dataByte);
    CHECK_SUCCESS(rc);
    heartbeatVal = dataByte[0];
    while (tryIndex < numTries)
    {
        rc = ariesReadByteData(device->i2cDriver, ARIES_MM_HEARTBEAT_ADDR,
                               dataByte);
        CHECK_SUCCESS(rc);
        if (dataByte[0] != heartbeatVal)
        {
            heartbeatSet = true;
            device->mmHeartbeatOkay = true;
            break;
        }
        tryIndex++;
    }

    // Read FW version
    // If heartbeat not there, set default FW values to 0.0.0
    // and return ARIES_SUCCESS
    device->fwVersion.major = 0;
    device->fwVersion.minor = 0;
    device->fwVersion.build = 0;
    if (heartbeatSet)
    {
        // Get FW version (major)
        rc = ariesReadBlockDataMainMicroIndirect(
            device->i2cDriver,
            (ARIES_MAIN_MICRO_FW_INFO + ARIES_MM_FW_VERSION_MAJOR), 1,
            dataByte);
        CHECK_SUCCESS(rc);
        device->fwVersion.major = dataByte[0];

        // Get FW version (minor)
        rc = ariesReadBlockDataMainMicroIndirect(
            device->i2cDriver,
            (ARIES_MAIN_MICRO_FW_INFO + ARIES_MM_FW_VERSION_MINOR), 1,
            dataByte);
        CHECK_SUCCESS(rc);
        device->fwVersion.minor = dataByte[0];

        // Get FW version (build)
        rc = ariesReadBlockDataMainMicroIndirect(
            device->i2cDriver,
            (ARIES_MAIN_MICRO_FW_INFO + ARIES_MM_FW_VERSION_BUILD), 2,
            dataWord);
        CHECK_SUCCESS(rc);
        device->fwVersion.build = (dataWord[1] << 8) + dataWord[0];
    }
    else
    {
        ASTERA_WARN("No Main Micro Heartbeat");
        device->mmHeartbeatOkay = false;
    }

    return ARIES_SUCCESS;
}

/*
 * Set the EEPROM page size and max burst size
 */
AriesErrorType ariesSetEEPROMPageSize(AriesDeviceType* device, int pageSize)
{
    // pageSize and maxBurstSize are the same
    device->eeprom.pageSize = pageSize;
    device->eeprom.maxBurstSize = pageSize;
    device->eeprom.pageCount = ARIES_EEPROM_MAX_NUM_BYTES / pageSize;

    return ARIES_SUCCESS;
}

/*
 * Set the bifurcation mode
 */
AriesErrorType ariesSetBifurcationMode(AriesDeviceType* device,
                                       AriesBifurcationType bifur)
{
    AriesErrorType rc;
    uint8_t dataBytes[4];

    rc = ariesReadBlockData(device->i2cDriver, 0x0, 4, dataBytes);
    CHECK_SUCCESS(rc);

    // Bifurcation setting is in bits 12:7
    dataBytes[0] = ((bifur & 0x01) << 7) | (dataBytes[0] & 0x7f);
    dataBytes[1] = ((bifur & 0x3e) >> 1) | (dataBytes[1] & 0xe0);
    rc = ariesWriteBlockData(device->i2cDriver, 0x0, 4, dataBytes);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Get the bifurcation mode
 */
AriesErrorType ariesGetBifurcationMode(AriesDeviceType* device,
                                       AriesBifurcationType* bifur)
{
    AriesErrorType rc;
    uint8_t dataBytes[4];

    rc = ariesReadBlockData(device->i2cDriver, 0x0, 4, dataBytes);
    CHECK_SUCCESS(rc);
    *bifur = ((dataBytes[1] & 0x1f) << 1) + ((dataBytes[0] & 0x80) >> 7);

    return ARIES_SUCCESS;
}

/*
 * Set the Retimer HW Reset.
 */
AriesErrorType ariesSetHwReset(AriesDeviceType* device, uint8_t reset)
{
    AriesErrorType rc;
    uint8_t dataWord[2];

    if (reset == 1) // Put retimer into reset
    {
        dataWord[0] = 0xff;
        dataWord[1] = 0x06;
        rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, dataWord);
        CHECK_SUCCESS(rc);
    }
    else if (reset == 0) // Take retimer out of reset (FW will reload)
    {
        dataWord[0] = 0x0;
        dataWord[1] = 0x0;
        rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, dataWord);
        CHECK_SUCCESS(rc);
    }
    else
    {
        return ARIES_INVALID_ARGUMENT;
    }

    return ARIES_SUCCESS;
}

/*
 * Set the I2C Master Reset.
 */
AriesErrorType ariesSetI2CMasterReset(AriesDeviceType* device, uint8_t reset)
{
    AriesErrorType rc;
    uint8_t dataWord[2];

    if (reset == 1) // Put I2C Master into reset
    {
        // Assert MM reset
        dataWord[0] = 0x0;
        dataWord[1] = 0x4;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, dataWord);
        CHECK_SUCCESS(rc);
        // Assert MM reset and I2C reset
        dataWord[0] = 0x0;
        dataWord[1] = 0x6;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, dataWord);
        CHECK_SUCCESS(rc);
        // Assert MM reset (de-assert I2C reset)
        dataWord[0] = 0x0;
        dataWord[1] = 0x4;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, dataWord);
        CHECK_SUCCESS(rc);
    }
    else if (reset == 0) // Take I2C Master out of reset
    {
        // De-assert MM reset
        dataWord[0] = 0x0;
        dataWord[1] = 0x0;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, dataWord);
        CHECK_SUCCESS(rc);
    }
    else
    {
        return ARIES_INVALID_ARGUMENT;
    }

    return ARIES_SUCCESS;
}

/*
 * Update the FW image in the EEPROM connected to the Retimer.
 */
AriesErrorType ariesUpdateFirmware(AriesDeviceType* device,
                                   const char* filename,
                                   AriesFWImageFormatType fileType)
{
    AriesErrorType rc;
    bool legacyMode = false;
    bool checksumVerifyFailed = false;
    uint8_t image[ARIES_EEPROM_MAX_NUM_BYTES];

    if (fileType == ARIES_FW_IMAGE_FORMAT_IHX)
    {
        // Load ihx file
        rc = ariesLoadIhxFile(filename, image);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Failed to load the .ihx file. RC = %d", rc);
            return ARIES_FAILURE;
        }
    }
    else if (fileType == ARIES_FW_IMAGE_FORMAT_BIN)
    {
        // Load bin file
        rc = ariesLoadBinFile(filename, image);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Failed to load the .bin file. RC = %d", rc);
            return ARIES_FAILURE;
        }
    }
    else
    {
        ASTERA_ERROR("Invalid Aries FW image format type");
        return ARIES_INVALID_ARGUMENT;
    }

    // Enable legacy mode if ARP is enabled or not running valid FW
    if (device->arpEnable || !device->mmHeartbeatOkay)
    {
        legacyMode = true;
    }

    // Program EEPROM image
    rc = ariesWriteEEPROMImage(device, image, legacyMode);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Failed to program the EEPROM. RC = %d", rc);
    }

    if (!legacyMode)
    {
        // Verify EEPROM programming by reading EEPROM and computing a checksum
        rc = ariesVerifyEEPROMImageViaChecksum(device, image);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Failed to verify the EEPROM using checksum. RC = %d",
                         rc);
            checksumVerifyFailed = true;
        }
    }

    // If the EEPROM verify via checksum failed, attempt the byte by byte verify
    // Optionally, it can be manually enabled by sending a 1 as the 4th argument
    if (legacyMode || checksumVerifyFailed)
    {
        // Verify EEPROM programming by reading EEPROM and comparing data with
        // expected image. In case there is a failure, the API will attempt a
        // rewrite once
        rc = ariesVerifyEEPROMImage(device, image, legacyMode);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Failed to read and verify the EEPROM. RC = %d", rc);
        }
    }

    device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_COMPLETE;

    return ARIES_SUCCESS;
}

/*
 * Update the FW image in the EEPROM connected to the Retimer.
 */
AriesErrorType ariesFirmwareUpdateProgress(AriesDeviceType* device,
                                           uint8_t* percentComplete)
{
    *percentComplete = 100 * device->fwUpdateProg /
                       (ARIES_FW_UPDATE_PROGRESS_COMPLETE -
                        ARIES_FW_UPDATE_PROGRESS_START + 1);

    return ARIES_SUCCESS;
}

/*
 * Load a FW image into the EEPROM connected to the Retimer.
 */
AriesErrorType ariesWriteEEPROMImage(AriesDeviceType* device, uint8_t* values,
                                     bool legacyMode)
{
    int currentPage = 0;
    AriesErrorType rc;

    // Deassert HW and SW resets
    uint8_t tmpData[2];
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    // Update device FW update progress state
    device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_WRITE_0;

    // If operating in legacy mode, put MM in reset
    if (legacyMode)
    {
        tmpData[0] = 0;
        tmpData[1] = 4;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);
        tmpData[0] = 0;
        tmpData[1] = 6;
        /*rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); //
         * hw_rst*/
        /*CHECK_SUCCESS(rc);*/
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);
        tmpData[0] = 0;
        tmpData[1] = 4;
        /*rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); //
         * hw_rst*/
        /*CHECK_SUCCESS(rc);*/
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);
    }
    else
    {
        tmpData[0] = 0;
        tmpData[1] = 2;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);
        tmpData[0] = 0;
        tmpData[1] = 0;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);
    }

    rc = ariesI2cMasterSoftReset(device->i2cDriver);
    CHECK_SUCCESS(rc);
    usleep(2000);

    int addr = 0;
    int burst;
    int eepromWriteDelta = 0;
    int addrFlag = -1;
    int addrDiff = 0;
    int addrDiffDelta = 0;

    time_t start_t, end_t;

    uint8_t data[device->eeprom.maxBurstSize];
    int addrMSB = 0;
    int addrI2C = 0;

    int eepromEnd;
    int eepromEndLoc = ariesGetEEPROMImageEnd(values);
    eepromEnd = eepromEndLoc;

    // Update EEPROM end index to match end of valid portion of EEPROM
    if (eepromEndLoc == -1)
    {
        eepromEnd = ARIES_EEPROM_MAX_NUM_BYTES;
    }
    else
    {
        eepromEnd += 8;
        eepromWriteDelta = eepromEnd % device->eeprom.blockWriteSize;
        if (eepromWriteDelta)
        {
            eepromEnd += device->eeprom.blockWriteSize - eepromWriteDelta;
        }
        // Calculate the location of the last 256 bytes
        addrDiff = (eepromEnd % device->eeprom.pageSize);
        addrFlag = eepromEnd - addrDiff;
        // The last write needs to be 16 bytes, so set location accordingly
        addrDiffDelta = addrDiff % device->eeprom.blockWriteSize;
        if (addrDiffDelta)
        {
            addrDiff += device->eeprom.blockWriteSize - addrDiffDelta;
        }
    }

    // Start timer
    time(&start_t);

    // Init I2C Master
    rc = ariesI2CMasterInit(device->i2cDriver);
    CHECK_SUCCESS(rc);

    // Set Page address
    rc = ariesI2CMasterSetPage(device->i2cDriver, currentPage);
    CHECK_SUCCESS(rc);

    bool mainMicroWriteAssist = false;
    if (!legacyMode)
    {
        if ((device->fwVersion.major >= 1) && (device->fwVersion.minor >= 1))
        {
            mainMicroWriteAssist = true;
        }
        else if ((device->fwVersion.major >= 1) &&
                 (device->fwVersion.minor >= 0) &&
                 (device->fwVersion.build >= 48))
        {
            mainMicroWriteAssist = true;
        }
    }

    if ((!legacyMode) && mainMicroWriteAssist)
    {
        ASTERA_INFO("Starting Main Micro assisted EEPROM write");

        while (addr < eepromEnd)
        {
            // Set MSB and local addresses for this page
            addrMSB = addr / 65536;
            addrI2C = addr % 65536;

            if (addrMSB != currentPage)
            {
                // Increment page num when you increment MSB
                rc = ariesI2CMasterSetPage(device->i2cDriver, addrMSB);
                CHECK_SUCCESS(rc);
                currentPage = addrMSB;
            }

            if (!(addrI2C % 8192))
            {
                ASTERA_INFO("Slv: 0x%02x, Reg: 0x%04x", (0x50 + addrMSB),
                            addrI2C);
            }

            // Send blocks of data (defined by burst size) to speed up process
            burst = 0;
            while (burst < device->eeprom.pageSize)
            {
                int addrBurst = addrI2C + burst;
                int indx;
                // In last iteration, no need to write all 256 bytes
                if (addr == addrFlag)
                {
                    for (indx = 0; indx < addrDiff; indx++)
                    {
                        data[indx] = values[(addr + burst + indx)];
                    }
                    // Send a block of bytes to the EEPROM starting at address
                    // addrBurst
                    rc = ariesI2CMasterMultiBlockWrite(device, addrBurst,
                                                       addrDiff, data);
                    CHECK_SUCCESS(rc);
                }
                else
                {
                    for (indx = 0; indx < device->eeprom.maxBurstSize; indx++)
                    {
                        data[indx] = values[(addr + burst + indx)];
                    }
                    // Send a block of bytes to the EEPROM starting at address
                    // addrBurst
                    rc = ariesI2CMasterMultiBlockWrite(
                        device, addrBurst, device->eeprom.maxBurstSize, data);
                    CHECK_SUCCESS(rc);
                }
                usleep(ARIES_DATA_BLOCK_PROGRAM_TIME_USEC);
                burst += device->eeprom.maxBurstSize;
            }

            // Update address
            addr += device->eeprom.pageSize;

            // Calcualte and update progress
            uint8_t prog = 10 * addr / eepromEnd;
            device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_WRITE_0 + prog;
        }
    }
    else // Block writes not supported here. Must write one byte a a time
    {
        ASTERA_INFO("Starting legacy mode EEPROM write");

        while (addr < eepromEnd)
        {
            // Set MSB and local addresses for this page
            addrMSB = addr / 65536;
            addrI2C = addr % 65536;

            if (addrMSB != currentPage)
            {
                // Increment page num when you increment MSB
                rc = ariesI2CMasterSetPage(device->i2cDriver, addrMSB);
                CHECK_SUCCESS(rc);
                currentPage = addrMSB;
            }

            if (!(addrI2C % 8192))
            {
                ASTERA_INFO("Slv: 0x%02x, Reg: 0x%04x", (0x50 + addrMSB),
                            addrI2C);
            }

            // Send blocks of data (defined by burst size) to speed up process
            burst = 0;
            while (burst < device->eeprom.pageSize)
            {
                int addrBurst = addrI2C + burst;
                int indx;
                // In last iteration, no need to write all 256 bytes
                if (addr == addrFlag)
                {
                    for (indx = 0; indx < addrDiff; indx++)
                    {
                        data[indx] = values[(addr + burst + indx)];
                    }
                    // Send a block of bytes to the EEPROM starting at address
                    // addrBurst
                    rc = ariesI2CMasterSendByteBlockData(
                        device->i2cDriver, addrBurst, addrDiff, data);
                    CHECK_SUCCESS(rc);
                }
                else
                {
                    for (indx = 0; indx < device->eeprom.maxBurstSize; indx++)
                    {
                        data[indx] = values[(addr + burst + indx)];
                    }
                    // Send bytes to the EEPROM starting at address
                    // addrBust
                    rc = ariesI2CMasterSendByteBlockData(
                        device->i2cDriver, addrBurst,
                        device->eeprom.maxBurstSize, data);
                    CHECK_SUCCESS(rc);
                }
                usleep(ARIES_DATA_BLOCK_PROGRAM_TIME_USEC);
                burst += device->eeprom.maxBurstSize;
            }

            // Update address
            addr += device->eeprom.pageSize;

            // Calcualte and update progress
            uint8_t prog = 10 * addr / eepromEnd;
            device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_WRITE_0 + prog;
        }
    }
    ASTERA_INFO("Ending write");

    // Stop timer
    time(&end_t);
    ASTERA_INFO("EEPROM load time: %.2f seconds", difftime(end_t, start_t));

    // Update device FW update progress state
    device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_WRITE_DONE;

    // Assert HW resets for I2C master interface
    tmpData[0] = 0x00;
    tmpData[1] = 0x02;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    usleep(1000);

    return ARIES_SUCCESS;
}

/*
 * Verify the FW image in the EEPROM connected to the Retimer.
 */
AriesErrorType ariesVerifyEEPROMImage(AriesDeviceType* device, uint8_t* values,
                                      bool legacyMode)
{
    int currentPage;
    bool firstByte;
    int addr;
    uint8_t dataByte[1];
    uint8_t reWriteByte[1];
    uint8_t expectedByte;
    AriesErrorType rc;
    AriesErrorType matchError;

    // Set current page to 0
    currentPage = 0;
    matchError = ARIES_SUCCESS;
    firstByte = true;

    // Deassert HW and SW resets
    uint8_t tmpData[2];
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    // Update device FW update progress state
    device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_VERIFY_0;

    if (legacyMode)
    {
        tmpData[0] = 0;
        tmpData[1] = 4;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);

        tmpData[0] = 0;
        tmpData[1] = 6;
        /*rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); //
         * hw_rst*/
        /*CHECK_SUCCESS(rc);*/
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);

        tmpData[0] = 0;
        tmpData[1] = 4;
        /*rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); //
         * hw_rst*/
        /*CHECK_SUCCESS(rc);*/
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);
    }
    else
    {
        tmpData[0] = 0;
        tmpData[1] = 2;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);
        tmpData[0] = 0;
        tmpData[1] = 0;
        rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2,
                                 tmpData); // sw_rst
        CHECK_SUCCESS(rc);
    }

    rc = ariesI2cMasterSoftReset(device->i2cDriver);
    CHECK_SUCCESS(rc);
    usleep(2000);

    // Set page address
    rc = ariesI2CMasterSetPage(device->i2cDriver, 0);
    CHECK_SUCCESS(rc);

    // Send EEPROM address 0
    dataByte[0] = 0;
    rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 2);
    CHECK_SUCCESS(rc);
    dataByte[0] = 0;
    rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 1);
    CHECK_SUCCESS(rc);

    time_t start_t, end_t;

    uint8_t dataBytes[ARIES_EEPROM_BLOCK_WRITE_SIZE_MAX];
    int addrMSB = 0;
    int addrI2C = 0;
    int mismatchCount = 0;

    int eepromEnd;
    int eepromWriteDelta;
    int eepromEndLoc = ariesGetEEPROMImageEnd(values);
    eepromEnd = eepromEndLoc;

    // Update EEPROM end index to match end of valid portion of EEPROM
    if (eepromEndLoc == -1)
    {
        eepromEnd = ARIES_EEPROM_MAX_NUM_BYTES;
    }
    else
    {
        eepromEnd += 8;
        eepromWriteDelta = eepromEnd % device->eeprom.blockWriteSize;
        if (eepromWriteDelta)
        {
            eepromEnd += device->eeprom.blockWriteSize - eepromWriteDelta;
        }
    }

    // Start timer
    time(&start_t);

    bool mainMicroAssist = false;

    if (!legacyMode)
    {
        if ((device->fwVersion.major >= 1) && (device->fwVersion.minor >= 1))
        {
            mainMicroAssist = true;
        }
        else if ((device->fwVersion.major >= 1) &&
                 (device->fwVersion.minor >= 0) &&
                 (device->fwVersion.build >= 50))
        {
            mainMicroAssist = true;
        }
    }

    if ((!legacyMode) && mainMicroAssist)
    {
        bool rewriteFlag;
        ASTERA_INFO("Starting Main Micro assisted EEPROM verify");
        for (addr = 0; addr < eepromEnd; addr += device->eeprom.blockWriteSize)
        {
            addrMSB = addr / 65536;
            addrI2C = addr % 65536;

            if (addrMSB != currentPage)
            {
                // Set updated page address
                rc = ariesI2CMasterSetPage(device->i2cDriver, addrMSB);
                CHECK_SUCCESS(rc);
                currentPage = addrMSB;
                // Send EEPROM address 0 after page update
                dataByte[0] = 0;
                rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 2);
                CHECK_SUCCESS(rc);
                dataByte[0] = 0;
                rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 1);
                CHECK_SUCCESS(rc);
                firstByte = true;
            }

            if (!(addrI2C % 8192))
            {
                ASTERA_INFO("Slv: 0x%02x, Reg: 0x%04x, Mismatch count: %d",
                            (0x50 + addrMSB), addrI2C, mismatchCount);
            }

            // Receive byte(s)
            // Address decided starting at what was set after setting
            // page address
            // Read entire page as a continuous set of 16 block bytes.
            rc = ariesI2CMasterReceiveByteBlock(device, dataBytes);
            CHECK_SUCCESS(rc);

            int byteIdx;
            rewriteFlag = false;
            for (byteIdx = 0; byteIdx < device->eeprom.blockWriteSize;
                 byteIdx++)
            {
                expectedByte = values[(addr + byteIdx)];
                if (expectedByte != dataBytes[byteIdx])
                {
                    mismatchCount += 1;
                    reWriteByte[0] = expectedByte;
                    ASTERA_ERROR("Data mismatch");
                    ASTERA_ERROR(
                        "    (Addr: %d) Expected: 0x%02x, Received: 0x%02x",
                        (addr + byteIdx), expectedByte, dataBytes[byteIdx]);
                    ASTERA_INFO("    Re-trying ...");
                    rc = ariesI2CMasterRewriteAndVerifyByte(
                        device->i2cDriver, (addr + byteIdx), reWriteByte);
                    // If re-verify step failed, mark error as verify failure
                    // and dont return error. Else, return error if not success
                    if (rc == ARIES_EEPROM_VERIFY_FAILURE)
                    {
                        matchError = ARIES_EEPROM_VERIFY_FAILURE;
                    }
                    else if (rc != ARIES_SUCCESS)
                    {
                        return rc;
                    }
                    rewriteFlag = true;
                }
            }

            // Rewrite address to start of next block, since it was reset in
            // rewrite and verify step
            if (rewriteFlag)
            {
                rc = ariesI2CMasterSendAddress(
                    device->i2cDriver, (addr + device->eeprom.blockWriteSize));
                CHECK_SUCCESS(rc);
            }

            // Calcualte and update progress
            uint8_t prog = 10 * addr / eepromEnd;
            device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_VERIFY_0 + prog;
        }
    }
    else
    {
        ASTERA_INFO("Starting legacy mode EEPROM verify");
        uint8_t value[1];
        for (addr = 0; addr < eepromEnd; addr++)
        {
            addrMSB = addr / 65536;
            addrI2C = addr % 65536;

            if (addrMSB != currentPage)
            {
                // Set updated page address
                rc = ariesI2CMasterSetPage(device->i2cDriver, addrMSB);
                CHECK_SUCCESS(rc);
                currentPage = addrMSB;
                // Send EEPROM address 0 after page update
                dataByte[0] = 0;
                rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 2);
                CHECK_SUCCESS(rc);
                dataByte[0] = 0;
                rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 1);
                CHECK_SUCCESS(rc);
                firstByte = true;
            }

            if (!(addrI2C % 8192))
            {
                ASTERA_INFO("Slv: 0x%02x, Reg: 0x%04x, Mismatch count: %d",
                            (0x50 + addrMSB), addrI2C, mismatchCount);
            }

            if (firstByte == true)
            {
                // Receive byte
                // Address decided starting at what was set after setting
                // page address
                rc = ariesI2CMasterReceiveByte(device->i2cDriver, value);
                CHECK_SUCCESS(rc);
            }
            else
            {
                // Receive continuous stream of bytes to speed up process
                rc = ariesI2CMasterReceiveContinuousByte(device->i2cDriver,
                                                         value);
                CHECK_SUCCESS(rc);
            }

            expectedByte = values[addr];
            if (expectedByte != value[0])
            {
                mismatchCount += 1;
                reWriteByte[0] = expectedByte;
                ASTERA_ERROR("Data mismatch");
                ASTERA_ERROR(
                    "    (Addr: %d) Expected: 0x%02x, Received: 0x%02x", addr,
                    expectedByte, value[0]);
                ASTERA_INFO("    Re-trying ...");
                rc = ariesI2CMasterRewriteAndVerifyByte(device->i2cDriver, addr,
                                                        reWriteByte);

                // If re-verify step failed, mark error as verify failure
                // and dont return error. Else, return error if not success
                if (rc == ARIES_EEPROM_VERIFY_FAILURE)
                {
                    matchError = ARIES_EEPROM_VERIFY_FAILURE;
                }
                else if (rc != ARIES_SUCCESS)
                {
                    return rc;
                }
            }

            // Calcualte and update progress
            uint8_t prog = 10 * addr / eepromEnd;
            device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_VERIFY_0 + prog;
        }
    }
    ASTERA_INFO("Ending verify");

    // Stop timer
    time(&end_t);
    ASTERA_INFO("EEPROM verify time: %.2f seconds", difftime(end_t, start_t));

    // Update device FW update progress state
    device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_VERIFY_DONE;

    // Assert HW resets for I2C master interface
    tmpData[0] = 0x00;
    tmpData[1] = 0x02;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);
    usleep(2000);

    return matchError;
}

/*
 * Verify EEPROM via checksum
 */
AriesErrorType ariesVerifyEEPROMImageViaChecksum(AriesDeviceType* device,
                                                 uint8_t* image)
{
    int currentPage;
    int addr;
    uint8_t dataByte[1];
    AriesErrorType rc;

    // Set current page to 0
    currentPage = 0;

    ASTERA_INFO("Starting Main Micro assisted EEPROM verify via checksum");

    // Deassert HW and SW resets
    uint8_t tmpData[2];
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    // Update device FW update progress state
    device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_VERIFY_0;

    // Reset I2C Master
    tmpData[0] = 0;
    tmpData[1] = 2;
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    // Set page address
    rc = ariesI2CMasterSetPage(device->i2cDriver, 0);
    CHECK_SUCCESS(rc);

    // Send EEPROM address 0
    dataByte[0] = 0;
    rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 2);
    CHECK_SUCCESS(rc);
    dataByte[0] = 0;
    rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 1);
    CHECK_SUCCESS(rc);

    time_t start_t, end_t;

    // Calculate EEPROM end address
    int eepromEnd;
    int eepromWriteDelta;
    int eepromEndLoc = ariesGetEEPROMImageEnd(image);
    eepromEnd = eepromEndLoc;

    // Update EEPROM end index to match end of valid portion of EEPROM
    if (eepromEndLoc == -1)
    {
        eepromEnd = ARIES_EEPROM_MAX_NUM_BYTES;
    }
    else
    {
        eepromEnd += 8;
        eepromWriteDelta = eepromEnd % device->eeprom.blockWriteSize;
        if (eepromWriteDelta)
        {
            eepromEnd += device->eeprom.blockWriteSize - eepromWriteDelta;
        }
    }

    // Start timer
    time(&start_t);

    // Calculate expected checksum values for each block
    uint8_t eepromBlockEnd = floor(eepromEnd / ARIES_EEPROM_BANK_SIZE);
    uint16_t eepromBlockEndDelta = eepromEnd -
                                   (eepromBlockEnd * ARIES_EEPROM_BANK_SIZE);

    uint32_t eepromBlockChecksum[ARIES_EEPROM_NUM_BANKS];
    uint16_t blockIdx;
    uint32_t blockSum;
    uint32_t byteIdx;
    bool isPass = true;
    for (blockIdx = 0; blockIdx < ARIES_EEPROM_NUM_BANKS; blockIdx++)
    {
        blockSum = 0;
        if (blockIdx == eepromBlockEnd)
        {
            for (byteIdx = 0; byteIdx < eepromBlockEndDelta; byteIdx++)
            {
                blockSum +=
                    image[(ARIES_EEPROM_BANK_SIZE * blockIdx) + byteIdx];
            }
        }
        else
        {
            for (byteIdx = 0; byteIdx < ARIES_EEPROM_BANK_SIZE; byteIdx++)
            {
                blockSum +=
                    image[(ARIES_EEPROM_BANK_SIZE * blockIdx) + byteIdx];
            }
        }
        eepromBlockChecksum[blockIdx] = blockSum;
    }

    uint8_t addrMSB;
    bool eepromBlockEndFlag = false;
    uint32_t checksum;

    for (addr = 0; addr < eepromEnd; addr += ARIES_EEPROM_BANK_SIZE)
    {
        addrMSB = addr / 65536;
        eepromBlockEndFlag = false;
        if (addrMSB != currentPage)
        {
            // Set updated page address
            rc = ariesI2CMasterSetPage(device->i2cDriver, addrMSB);
            CHECK_SUCCESS(rc);
            currentPage = addrMSB;
            // Send EEPROM address 0 after page update
            dataByte[0] = 0;
            rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 2);
            CHECK_SUCCESS(rc);
            dataByte[0] = 0;
            rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 1);
            CHECK_SUCCESS(rc);

            if (currentPage == eepromBlockEnd)
            {
                eepromBlockEndFlag = true;
            }
        }

        checksum = 0;

        if (eepromBlockEndFlag)
        {
            // partial-block checksum calc.
            rc = ariesI2CMasterGetChecksum(device, eepromBlockEndDelta,
                                           &checksum);
            CHECK_SUCCESS(rc);
        }
        else
        {
            // full-block checksum calc.
            rc = ariesI2CMasterGetChecksum(device, 0, &checksum);
            CHECK_SUCCESS(rc);
        }

        if (checksum != eepromBlockChecksum[currentPage])
        {
            ASTERA_ERROR("Page %d: checksum did not match expected value",
                         currentPage);
            ASTERA_ERROR("    Expected: %d", eepromBlockChecksum[currentPage]);
            ASTERA_ERROR("    Received: %d", checksum);
            isPass = false;
        }
        else
        {
            ASTERA_INFO("Page %d: checksums matched", currentPage);
        }

        if (eepromBlockEndFlag)
        {
            break;
        }

        // Calcualte and update progress
        uint8_t prog = 10 * addr / eepromEnd;
        device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_VERIFY_0 + prog;
    }
    ASTERA_INFO("Ending verify");

    // Stop timer
    time(&end_t);
    ASTERA_INFO("EEPROM verify time: %.2f seconds", difftime(end_t, start_t));

    // Update device FW update progress state
    device->fwUpdateProg = ARIES_FW_UPDATE_PROGRESS_VERIFY_DONE;

    if (!isPass)
    {
        return ARIES_EEPROM_VERIFY_FAILURE;
    }

    return ARIES_SUCCESS;
}

/*
 * Calculate block CRCs from data in EEPROM
 */
AriesErrorType ariesCheckEEPROMCrc(AriesDeviceType* device, uint8_t* image)
{
    AriesErrorType rc;

    uint8_t crcBytesEEPROM[ARIES_EEPROM_MAX_NUM_CRC_BLOCKS];
    uint8_t crcBytesImg[ARIES_EEPROM_MAX_NUM_CRC_BLOCKS];

    uint8_t numCrcBytesEEPROM;
    uint8_t numCrcBytesImg;

    rc = ariesCheckEEPROMImageCrcBytes(device, crcBytesEEPROM,
                                       &numCrcBytesEEPROM);
    CHECK_SUCCESS(rc);
    ariesGetCrcBytesImage(image, crcBytesImg, &numCrcBytesImg);

    if (numCrcBytesImg != numCrcBytesEEPROM)
    {
        // FAIL
        ASTERA_ERROR("CRC block size mismatch. Please check FW version");
        return ARIES_EEPROM_CRC_BLOCK_NUM_FAIL;
    }

    uint8_t byteIdx;
    for (byteIdx = 0; byteIdx < numCrcBytesEEPROM; byteIdx++)
    {
        if (crcBytesEEPROM[byteIdx] != crcBytesImg[byteIdx])
        {
            // Mismatch
            ASTERA_ERROR("CRC byte mismatch. Please check FW version");
            ASTERA_ERROR("    EEPROM CRC: %x, FILE CRC: %x",
                         crcBytesEEPROM[byteIdx], crcBytesImg[byteIdx]);
            return ARIES_EEPROM_CRC_BYTE_FAIL;
        }
    }

    ASTERA_INFO("EEPROM Block CRCs match with expected FW image");

    return ARIES_SUCCESS;
}

/*
 * Calculate block CRCs from data in EEPROM
 */
AriesErrorType ariesCheckEEPROMImageCrcBytes(AriesDeviceType* device,
                                             uint8_t* crcBytes,
                                             uint8_t* numCrcBytes)
{
    AriesErrorType rc;
    uint8_t dataByte[1];

    // Deassert HW and SW resets
    uint8_t tmpData[2];
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    // Init I2C Master
    rc = ariesI2CMasterInit(device->i2cDriver);
    CHECK_SUCCESS(rc);

    // Set page address
    rc = ariesI2CMasterSetPage(device->i2cDriver, 0);
    CHECK_SUCCESS(rc);

    // Send EEPROM address 0
    dataByte[0] = 0;
    rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 2);
    CHECK_SUCCESS(rc);
    dataByte[0] = 0;
    rc = ariesI2CMasterSendByte(device->i2cDriver, dataByte, 1);
    CHECK_SUCCESS(rc);

    // Get block start address
    int numBlocks = 0;
    int blockStartAddr = 0;
    rc = ariesGetEEPROMFirstBlock(device->i2cDriver, &blockStartAddr);
    CHECK_SUCCESS(rc);

    uint8_t blockType;
    int blockLength;
    uint8_t crcByte;

    while (numBlocks < ARIES_EEPROM_MAX_NUM_CRC_BLOCKS)
    {
        rc = ariesGetEEPROMBlockType(device->i2cDriver, blockStartAddr,
                                     &blockType);
        CHECK_SUCCESS(rc);

        if (blockType != 0xff)
        {
            rc = ariesEEPROMGetBlockLength(device->i2cDriver, blockStartAddr,
                                           &blockLength);
            CHECK_SUCCESS(rc);
            rc = ariesGetEEPROMBlockCrcByte(device->i2cDriver, blockStartAddr,
                                            blockLength, &crcByte);
            CHECK_SUCCESS(rc);

            crcBytes[numBlocks] = crcByte;

            blockStartAddr += blockLength + 13;
            numBlocks++;
        }
        else
        {
            // Last Page
            break;
        }
    }

    *numCrcBytes = numBlocks;

    // Assert HW resets for I2C master interface
    tmpData[0] = 0x00;
    tmpData[1] = 0x02;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);
    usleep(2000);

    return ARIES_SUCCESS;
}

/*
 * Program EEPROM, but only write bytes which are different from current
 * step
 */
AriesErrorType ariesWriteEEPROMImageDelta(AriesDeviceType* device,
                                          uint8_t* imageCurrent,
                                          int sizeCurrent, uint8_t* imageNew,
                                          int sizeNew)
{
    AriesErrorType rc;
    AriesEEPROMDeltaType differences[ARIES_EEPROM_MAX_NUM_BYTES];

    if (sizeCurrent != sizeNew)
    {
        ASTERA_WARN("Image sizes need to be equal");
        return ARIES_EEPROM_WRITE_ERROR;
    }

    // Iterate over array and check differences
    int addrIdx;
    int diffIdx = 0;
    for (addrIdx = 0; addrIdx < sizeNew; addrIdx++)
    {
        if (imageCurrent[addrIdx] != imageNew[addrIdx])
        {
            differences[diffIdx].address = addrIdx;
            differences[diffIdx].data = imageNew[addrIdx];
            diffIdx++;
        }
    }

    // If less than 25% of image is different, we can use this mode
    // Else recommend MM-assist mode
    if (diffIdx > ARIES_EEPROM_MAX_NUM_BYTES / 4)
    {
        ASTERA_INFO("Image difference large");
        ASTERA_INFO("Please use MM-assist write mode to program EEPROM");
        return ARIES_EEPROM_WRITE_ERROR;
    }

    // De-assert HW and SW resets and reset I2C Master
    uint8_t tmpData[2];
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    tmpData[0] = 0;
    tmpData[1] = 4;
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    tmpData[0] = 0;
    tmpData[1] = 6;
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    tmpData[0] = 0;
    tmpData[1] = 4;
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    rc = ariesI2cMasterSoftReset(device->i2cDriver);
    CHECK_SUCCESS(rc);
    usleep(2000);

    // Init I2C Master
    rc = ariesI2CMasterInit(device->i2cDriver);
    CHECK_SUCCESS(rc);

    // Set Page address to 0
    uint8_t currentPage = 0;
    rc = ariesI2CMasterSetPage(device->i2cDriver, currentPage);
    CHECK_SUCCESS(rc);

    int wrIndex;
    uint8_t dataByte[1];
    for (wrIndex = 0; wrIndex < diffIdx; wrIndex++)
    {
        // Check if page needs to be updated
        int addr = differences[diffIdx].address;
        uint8_t pageNum = floor(addr / ARIES_EEPROM_BANK_SIZE);
        if (pageNum != currentPage)
        {
            rc = ariesI2CMasterSetPage(device->i2cDriver, pageNum);
            CHECK_SUCCESS(rc);
            currentPage = pageNum;
        }

        dataByte[0] = differences[diffIdx].data;

        rc = ariesI2CMasterRewriteAndVerifyByte(device->i2cDriver, addr,
                                                dataByte);
        CHECK_SUCCESS(rc);
    }

    return ARIES_SUCCESS;
}

/*
 * Read a byte from EEPROM.
 */
AriesErrorType ariesReadEEPROMByte(AriesDeviceType* device, int addr,
                                   uint8_t* value)
{
    AriesErrorType rc;

    // Deassert HW reset for I2C master interface
    uint8_t tmpData[2];
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);

    // Toggle SW reset for I2C master interface
    tmpData[0] = 0;
    tmpData[1] = 2;
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    // Set page address
    uint8_t pageNum = floor(addr / ARIES_EEPROM_BANK_SIZE);
    rc = ariesI2CMasterSetPage(device->i2cDriver, pageNum);
    CHECK_SUCCESS(rc);

    // The EEPROM access
    rc = ariesEEPROMGetRandomByte(device->i2cDriver, addr, value);
    CHECK_SUCCESS(rc);

    // Assert HW/SW resets for I2C master interface
    tmpData[0] = 0x00;
    tmpData[1] = 0x02;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Write a byte to EEPROM.
 */
AriesErrorType ariesWriteEEPROMByte(AriesDeviceType* device, int addr,
                                    uint8_t* value)
{
    AriesErrorType rc;

    // Deassert HW reset for I2C master interface
    uint8_t tmpData[2];
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);

    // Toggle SW reset for I2C master interface
    tmpData[0] = 0;
    tmpData[1] = 2;
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);
    tmpData[0] = 0;
    tmpData[1] = 0;
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    // Set page address
    uint8_t pageNum = floor(addr / ARIES_EEPROM_BANK_SIZE);
    rc = ariesI2CMasterSetPage(device->i2cDriver, pageNum);
    CHECK_SUCCESS(rc);

    // The EEPROM access
    rc = ariesI2CMasterSendByteBlockData(device->i2cDriver, addr, 1, value);
    CHECK_SUCCESS(rc);

    // Assert HW/SW resets for I2C master interface
    tmpData[0] = 0x00;
    tmpData[1] = 0x02;
    rc = ariesWriteBlockData(device->i2cDriver, 0x600, 2, tmpData); // hw_rst
    CHECK_SUCCESS(rc);
    rc = ariesWriteBlockData(device->i2cDriver, 0x602, 2, tmpData); // sw_rst
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Check connection health
 */
AriesErrorType ariesCheckConnectionHealth(AriesDeviceType* device,
                                          uint8_t slaveAddress)
{
    AriesErrorType rc;
    uint8_t dataByte[1];

    device->arpEnable = false;

    rc = ariesReadByteData(device->i2cDriver, ARIES_CODE_LOAD_REG, dataByte);
    if (rc != ARIES_SUCCESS)
    {
        // Failure to read code, run ARP
        ASTERA_WARN("Failed to read code_load, Run ARP");
        // Perform Address Resolution Protocol (ARP) to set the Aries slave
        // address. Aries slave will respond to 0x61 during ARP.
        // NOTE: In most cases, Aries firmware will disable ARP and the Retimer
        // will take on a fixed SMBus address: 0x20, 0x21, ..., 0x27.
        int arpHandle = asteraI2COpenConnection(device->i2cBus, 0x61);
        rc = ariesRunArp(arpHandle, slaveAddress); // Run ARP, user addr
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("ARP connection unsuccessful");
            return ARIES_I2C_OPEN_FAILURE;
        }

        // Update Aries SMBus driver
        device->i2cDriver->handle = asteraI2COpenConnection(device->i2cBus,
                                                            slaveAddress);
        device->i2cDriver->slaveAddr = slaveAddress;
        device->arpEnable = true;
        rc = ariesReadByteData(device->i2cDriver, ARIES_CODE_LOAD_REG,
                               dataByte);
        if (rc != ARIES_SUCCESS)
        {
            ASTERA_ERROR("Failed to read code_load after ARP");
            return ARIES_I2C_OPEN_FAILURE;
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Check if device has loaded firmware properly, and if retimer slave address
 * is correct
 */
AriesErrorType ariesCheckDeviceHealth(AriesDeviceType* device)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint8_t dataBytes[4];

    device->deviceOkay = true;

    // Check if retimer slave address is correct
    // This can be done by perfroming a simple read
    rc = ariesReadBlockData(device->i2cDriver, 0x0, 4, dataBytes);
    if (rc != ARIES_SUCCESS)
    {
        ASTERA_ERROR("Reads to retimer aren't working");
        ASTERA_ERROR("Check slave address and/or connections to retimer");
        device->deviceOkay = false;
        return rc;
    }

    // Check if EEPROM has loaded successfully
    rc = ariesReadByteData(device->i2cDriver, ARIES_CODE_LOAD_REG, dataByte);
    CHECK_SUCCESS(rc);
    if (dataByte[0] < ARIES_LOAD_CODE)
    {
        ASTERA_ERROR("Device firmware load unsuccessful");
        ASTERA_ERROR("Must attempt firmware rewrite to EEPROM");
        device->deviceOkay = false;
    }

    return ARIES_SUCCESS;
}

/*
 * Get the maximum junction temperature.
 */
AriesErrorType ariesGetMaxTemp(AriesDeviceType* device)
{
    return ariesReadPmaTempMax(device);
}

/*
 * Get the current average temperature across all sensors
 */
AriesErrorType ariesGetCurrentTemp(AriesDeviceType* device)
{
    return ariesReadPmaAvgTemp(device);
}

/*
 * Set max data rate
 */
AriesErrorType ariesSetMaxDataRate(AriesDeviceType* device,
                                   AriesMaxDataRateType rate)
{
    AriesErrorType rc;
    uint8_t dataBytes[4];
    uint32_t val;
    uint32_t mask;

    rc = ariesReadBlockData(device->i2cDriver, 0, 4, dataBytes);
    CHECK_SUCCESS(rc);
    val = dataBytes[0] + (dataBytes[1] << 8) + (dataBytes[2] << 16) +
          (dataBytes[3] << 24);

    mask = ~(7 << 24) & 0xffffffff;
    val &= mask;
    val |= (rate << 24);

    dataBytes[0] = val & 0xff;
    dataBytes[1] = (val >> 8) & 0xff;
    dataBytes[2] = (val >> 16) & 0xff;
    dataBytes[3] = (val >> 24) & 0xff;

    rc = ariesWriteBlockData(device->i2cDriver, 0, 4, dataBytes);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Set the GPIO value
 */
AriesErrorType ariesSetGPIO(AriesDeviceType* device, int gpioNum, bool value)
{
    AriesErrorType rc;
    uint8_t dataByte[1];

    rc = ariesReadByteData(device->i2cDriver, 0x916, dataByte);
    CHECK_SUCCESS(rc);
    if (value == 1)
    {
        // Set the corresponding GPIO bit to 1
        dataByte[0] |= (1 << gpioNum);
    }
    else
    {
        // Clear the corresponding GPIO bit to 0
        dataByte[0] &= ~(1 << gpioNum);
    }

    rc = ariesWriteByteData(device->i2cDriver, 0x916, dataByte);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Get the GPIO value
 */
AriesErrorType ariesGetGPIO(AriesDeviceType* device, int gpioNum, bool* value)
{
    AriesErrorType rc;
    uint8_t dataByte[1];

    rc = ariesReadByteData(device->i2cDriver, 0x915, dataByte);
    CHECK_SUCCESS(rc);
    *value = (dataByte[0] >> gpioNum) & 1;

    return ARIES_SUCCESS;
}

/**
 * Toggle the GPIO value
 */
AriesErrorType ariesToggleGPIO(AriesDeviceType* device, int gpioNum)
{
    AriesErrorType rc;
    uint8_t dataByte[1];

    rc = ariesReadByteData(device->i2cDriver, 0x916, dataByte);
    CHECK_SUCCESS(rc);

    dataByte[0] ^= (1 << gpioNum);

    rc = ariesWriteByteData(device->i2cDriver, 0x916, dataByte);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Set the GPIO direction
 */
AriesErrorType ariesSetGPIODirection(AriesDeviceType* device, int gpioNum,
                                     bool value)
{
    AriesErrorType rc;
    uint8_t dataByte[1];

    rc = ariesReadByteData(device->i2cDriver, 0x917, dataByte);
    CHECK_SUCCESS(rc);
    if (value == 1)
    {
        // Set the corresponding GPIO bit to 1
        dataByte[0] |= (1 << gpioNum);
    }
    else
    {
        // Clear the corresponding GPIO bit to 0
        dataByte[0] &= ~(1 << gpioNum);
    }

    rc = ariesWriteByteData(device->i2cDriver, 0x917, dataByte);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Get the GPIO direction
 */
AriesErrorType ariesGetGPIODirection(AriesDeviceType* device, int gpioNum,
                                     bool* value)
{
    AriesErrorType rc;
    uint8_t dataByte[1];

    rc = ariesReadByteData(device->i2cDriver, 0x917, dataByte);
    CHECK_SUCCESS(rc);
    *value = (dataByte[0] >> gpioNum) & 1;

    return ARIES_SUCCESS;
}

/*
 * Enable Aries Test Mode for PRBS generation and checking
 */
AriesErrorType ariesTestModeEnable(AriesDeviceType* device)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint8_t dataWord[2];
    int side;
    int lane;
    int qs;
    int qsLane;

    // Assert register PERST
    ASTERA_INFO("Assert internal PERST");
    dataByte[0] = 0x00;
    rc = ariesWriteByteData(device->i2cDriver, 0x604,
                            dataByte); // Assert for Links[7:0]
    CHECK_SUCCESS(rc);
    usleep(100000); // 100 ms

    // Put MM in reset
    ASTERA_INFO("Put MM into reset");
    rc = ariesSetMMReset(device, true);
    CHECK_SUCCESS(rc);
    usleep(100000); // 100 ms

    // Verify
    // rc = ariesReadBlockData(device->i2cDriver, 0x602, 2, dataWord);
    // CHECK_SUCCESS(rc);
    // uint8_t mm_reset = dataWord[1] >> 2;
    // ASTERA_INFO("Read back MM reset: %d", mm_reset);

    // MM could have been halted in the middle of a temp sensor reading. Undo
    // those overrides.
    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            qs = lane / 4;
            qsLane = lane % 4;
            // Disable the temp sensor (one control bit per PMA instance)
            if (qsLane == 0)
            {
                rc = ariesReadWordPmaIndirect(device->i2cDriver, side, qs, 0xed,
                                              dataWord);
                CHECK_SUCCESS(rc);
                dataWord[0] &= ~(1 << 3);
                rc = ariesWriteWordPmaIndirect(device->i2cDriver, side, qs,
                                               0xed, dataWord);
                CHECK_SUCCESS(rc);
                rc = ariesReadWordPmaIndirect(device->i2cDriver, side, qs, 0xea,
                                              dataWord);
                CHECK_SUCCESS(rc);
                dataWord[0] &= ~(1 << 5);
                dataWord[0] &= ~(1 << 6);
                rc = ariesWriteWordPmaIndirect(device->i2cDriver, side, qs,
                                               0xea, dataWord);
                CHECK_SUCCESS(rc);
            }
            // Un-freeze PMA FW
            rc = ariesReadWordPmaLaneIndirect(device->i2cDriver, side, qs,
                                              qsLane, 0x2060, dataWord);
            CHECK_SUCCESS(rc);
            dataWord[1] &= ~(1 << 6);
            rc = ariesWriteWordPmaLaneIndirect(device->i2cDriver, side, qs,
                                               qsLane, 0x2060, dataWord);
            CHECK_SUCCESS(rc);
        }
    }
    // In some versions of FW, MPLLB output divider is overriden to /2. Undo
    // this change.
    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            qs = lane / 4;
            qsLane = lane % 4;
            // one control bit per PMA instance
            if (qsLane == 0)
            {
                dataWord[0] = 0x20;
                dataWord[1] = 0x00;
                rc = ariesWriteWordPmaIndirect(
                    device->i2cDriver, side, qs,
                    ARIES_PMA_SUP_DIG_MPLLB_OVRD_IN_0, dataWord);
                CHECK_SUCCESS(rc);
            }
        }
    }
    // Put Receivers into standby
    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            rc = ariesPipeRxStandbySet(device, side, lane, true);
            CHECK_SUCCESS(rc);
        }
    }
    usleep(10000); // 10 ms

    // Transition to Powerdown=0 (P0)
    for (side = 0; side < 2; side++)
    {
        // One powerdown control for every grouping of two lanes
        for (lane = 0; lane < 16; lane += 2)
        {
            rc = ariesPipePowerdownSet(device, side, lane,
                                       ARIES_PIPE_POWERDOWN_P0);
            CHECK_SUCCESS(rc);
            usleep(10000); // 10 ms
            rc = ariesPipePowerdownCheck(device, side, lane,
                                         ARIES_PIPE_POWERDOWN_P0);
            CHECK_SUCCESS(rc);
        }
    }
    usleep(10000); // 10 ms

    // Enable RX terminations
    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            rc = ariesPipeRxTermSet(device, side, lane, true);
            CHECK_SUCCESS(rc);
        }
    }
    usleep(10000); // 10 ms

    // Disable PCS block align control
    for (side = 0; side < 2; side++)
    {
        // One blockaligncontrol control for every grouping of two lanes
        for (lane = 0; lane < 16; lane += 2)
        {
            rc = ariesPipeBlkAlgnCtrlSet(device, side, lane, false);
            CHECK_SUCCESS(rc);
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Disable Aries Test Mode for PRBS generation and checking
 */
AriesErrorType ariesTestModeDisable(AriesDeviceType* device)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint8_t dataWord[2];
    int side;
    int lane;
    int qs;
    int qsLane;

    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            qs = lane / 4;
            qsLane = lane % 4;
            // Undo RxReq override
            rc = ariesReadWordPmaLaneIndirect(
                device->i2cDriver, side, qs, qsLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_1, dataWord);
            CHECK_SUCCESS(rc);
            dataWord[0] &= ~(1 << 3);
            rc = ariesWriteWordPmaLaneIndirect(
                device->i2cDriver, side, qs, qsLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_1, dataWord);
            CHECK_SUCCESS(rc);
            // Disable Rx terminations
            rc = ariesPipeRxTermSet(device, side, lane, false);
            CHECK_SUCCESS(rc);
            // Undo Rx inversion overrides
            rc = ariesPMARxInvertSet(device, side, lane, false, false);
            CHECK_SUCCESS(rc);
            // Set de-emphasis back to -3.5 dB
            rc = ariesPipeDeepmhasisSet(device, side, lane, 1,
                                        ARIES_PIPE_DEEMPHASIS_PRESET_NONE, 0,
                                        44, 0);
            // Turn off PRBS generators
            rc = ariesPMABertPatGenConfig(device, side, lane, 0);
            CHECK_SUCCESS(rc);
            // Turn off PRBS checkers
            rc = ariesPMABertPatChkConfig(device, side, lane, 0);
            CHECK_SUCCESS(rc);
            // Undo block align control override
            rc = ariesPipeBlkAlgnCtrlSet(device, side, lane, false);
            CHECK_SUCCESS(rc);
            // Undo Tx/Rx data enable override
            rc = ariesPMATxDataEnSet(device, side, lane, false);
            CHECK_SUCCESS(rc);
            rc = ariesPMARxDataEnSet(device, side, lane, false);
            CHECK_SUCCESS(rc);
            // Rate change to Gen1
            if ((lane % 2) == 0)
            {
                // Rate is controlled for each group of two lanes, so only do
                // this once per pair
                rc = ariesPipeRateChange(device, side, lane,
                                         1); // rate=1 for Gen1
                CHECK_SUCCESS(rc);
            }
            // Powerdown to P1 (P1 is value 2)
            rc = ariesPipePowerdownSet(device, side, lane,
                                       ARIES_PIPE_POWERDOWN_P1);
            CHECK_SUCCESS(rc);
            // Undo Rxstandby override
            rc = ariesPipeRxStandbySet(device, side, lane, false);
            CHECK_SUCCESS(rc);
        }
    }

    // Take MM out of reset
    rc = ariesSetMMReset(device, false);
    CHECK_SUCCESS(rc);
    // De-assert register PERST
    dataByte[0] = 0xff;
    rc = ariesWriteByteData(device->i2cDriver, 0x604, dataByte);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Set the desired Aries Test Mode data rate
 */
AriesErrorType ariesTestModeRateChange(AriesDeviceType* device,
                                       AriesMaxDataRateType rate)
{
    AriesErrorType rc;
    int side;
    int lane;

    for (side = 0; side < 2; side++)
    {
        // Rate is controlled for each grouping of two lanes, so only do this
        // once per pair
        for (lane = 0; lane < 16; lane += 2)
        {
            rc = ariesPipeRateChange(device, side, lane, rate);
            CHECK_SUCCESS(rc);
            usleep(50000); // 50 ms
        }
        // Confirm that every lane changed rate successfully
        for (lane = 0; lane < 16; lane++)
        {
            // Confirm rate change by reading PMA registers
            rc = ariesPipeRateCheck(device, side, lane, rate);
            CHECK_SUCCESS(rc);
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Aries Test Mode transmitter configuration
 */
AriesErrorType ariesTestModeTxConfig(AriesDeviceType* device,
                                     AriesPRBSPatternType pattern, int preset,
                                     bool enable)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    int side;
    int lane;
    int rate;
    int de;
    AriesPRBSPatternType mode = DISABLED;

    // Decode the pattern argument
    if (enable)
    {
        // Set the generator mode (pattern)
        mode = pattern;

        // Check any Path's rate (assumption is they're all the same)
        // qs_2, pth_wrap_0 is absolute lane 8
        rc = ariesReadRetimerRegister(
            device->i2cDriver, 0, 8,
            ARIES_RET_PTH_GBL_MAC_PHY_RATE_AND_PCLK_RATE_ADDR, 1, dataByte);
        CHECK_SUCCESS(rc);
        rate = dataByte[0] & 0x7;
        if (rate >= 2) // rate==2 is Gen3
        {
            // For Gen3/4/5, use presets
            de = ARIES_PIPE_DEEMPHASIS_DE_NONE;
            if (preset < 0)
            {
                preset = 0;
            }
            else if (preset > 10)
            {
                preset = 10;
            }
        }
        else
        {
            // For Gen1/2, use de-emphasis -3.5dB (default)
            de = 1;
            preset = ARIES_PIPE_DEEMPHASIS_PRESET_NONE;
        }
        for (side = 0; side < 2; side++)
        {
            for (lane = 0; lane < 16; lane++)
            {
                rc = ariesPipeDeepmhasisSet(device, side, lane, de, preset, 0,
                                            44, 0);
                CHECK_SUCCESS(rc);
            }
        }
    }
    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            rc = ariesPMABertPatGenConfig(device, side, lane, mode);
            CHECK_SUCCESS(rc);
            usleep(10000); // 10 ms
            rc = ariesPMATxDataEnSet(device, side, lane, enable);
            CHECK_SUCCESS(rc);
            usleep(10000); // 10 ms
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Aries Test Mode receiver configuration
 */
AriesErrorType ariesTestModeRxConfig(AriesDeviceType* device,
                                     AriesPRBSPatternType pattern, bool enable)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint8_t dataWord[2];
    int side;
    int lane;
    int qs;
    int qsLane;
    int rate;

    if (enable)
    {
        for (side = 0; side < 2; side++)
        {
            for (lane = 0; lane < 16; lane++)
            {
                rc = ariesPMAPCSRxReqBlock(device, side, lane);
                CHECK_SUCCESS(rc);
                rc = ariesPMARxDataEnSet(device, side, lane, true);
                CHECK_SUCCESS(rc);
                usleep(10000); // 10 ms
            }
        }
        usleep(500000); // 500 ms
        // Adapt the receivers for Gen3/4/5
        // Check any Path's rate (assumption is they're all the same)
        // qs_2, pth_wrap_0 is absolute lane 8
        rc = ariesReadRetimerRegister(
            device->i2cDriver, 0, 8,
            ARIES_RET_PTH_GBL_MAC_PHY_RATE_AND_PCLK_RATE_ADDR, 1, dataByte);
        CHECK_SUCCESS(rc);
        rate = dataByte[0] & 0x7;
        if (rate >= 2) // rate==2 is Gen3
        {
            ASTERA_INFO("Run Rx adaptation....");
            for (side = 0; side < 2; side++)
            {
                for (lane = 0; lane < 16; lane++)
                {
                    rc = ariesPipeRxAdapt(device, side, lane);
                    CHECK_SUCCESS(rc);
                    usleep(10000); // 10 ms
                }
            }
        }
        usleep(500000); // 500 ms
        // Configure pattern checker
        for (side = 0; side < 2; side++)
        {
            for (lane = 0; lane < 16; lane++)
            {
                rc = ariesPMABertPatChkConfig(device, side, lane, pattern);
                CHECK_SUCCESS(rc);
                usleep(10000); // 10 ms
            }
        }
        // Clear patter checkers
        // rc = ariesTestModeRxEcountClear(device);
        // CHECK_SUCCESS(rc);
        // Detect/correct polarity
        for (side = 0; side < 2; side++)
        {
            for (lane = 0; lane < 16; lane++)
            {
                rc = ariesPMABertPatChkDetectCorrectPolarity(device, side,
                                                             lane);
                CHECK_SUCCESS(rc);
                usleep(10000); // 10 ms
            }
        }
    }
    else
    {
        for (side = 0; side < 2; side++)
        {
            for (lane = 0; lane < 16; lane++)
            {
                qs = lane / 4;
                qsLane = lane % 4;
                rc = ariesReadWordPmaLaneIndirect(
                    device->i2cDriver, side, qs, qsLane,
                    ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_1, dataWord);
                CHECK_SUCCESS(rc);
                dataWord[0] &= ~(1 << 3);
                rc = ariesWriteWordPmaLaneIndirect(
                    device->i2cDriver, side, qs, qsLane,
                    ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_1, dataWord);
                CHECK_SUCCESS(rc);
                rc = ariesPMARxDataEnSet(device, side, lane, false);
                CHECK_SUCCESS(rc);
                rc = ariesPMABertPatChkConfig(device, side, lane, DISABLED);
                CHECK_SUCCESS(rc);
            }
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Aries Test Mode read error count
 */
AriesErrorType ariesTestModeRxEcountRead(AriesDeviceType* device, int* ecount)
{
    AriesErrorType rc;
    int dataByte[1];
    int side;
    int lane;

    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            rc = ariesPMABertPatChkSts(device, side, lane, dataByte);
            CHECK_SUCCESS(rc);
            // ASTERA_INFO("Side:%d, Lane:%2d, ECOUNT = %d", side, lane,
            // dataByte[0]);
            int index = side * 16 + lane;
            ecount[index] = dataByte[0];
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Aries Test Mode clear error count
 */
AriesErrorType ariesTestModeRxEcountClear(AriesDeviceType* device)
{
    AriesErrorType rc;
    int side;
    int lane;

    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            rc = ariesPMABertPatChkToggleSync(device, side, lane);
            CHECK_SUCCESS(rc);
            rc = ariesPMABertPatChkToggleSync(device, side, lane);
            CHECK_SUCCESS(rc);
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Aries Test Mode read FoM
 */
AriesErrorType ariesTestModeRxFomRead(AriesDeviceType* device, int* fom)
{
    AriesErrorType rc;
    int dataByte[1];
    int side;
    int lane;

    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            rc = ariesPipeFomGet(device, side, lane, dataByte);
            CHECK_SUCCESS(rc);
            // ASTERA_INFO("Side:%d, Lane:%2d, FOM = 0x%x", side, lane,
            // dataByte[0]);
            int index = side * 16 + lane;
            fom[index] = dataByte[0];
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Aries Test Mode read Rx valid
 */
AriesErrorType ariesTestModeRxValidRead(AriesDeviceType* device)
{
    AriesErrorType rc;
    uint8_t dataWord[2];
    int side;
    int lane;
    int qs;
    int qsLane;
    bool rxvalid;

    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            qs = lane / 4;
            qsLane = lane % 4;
            rc = ariesReadWordPmaLaneIndirect(device->i2cDriver, side, qs,
                                              qsLane, 0x1033, dataWord);
            CHECK_SUCCESS(rc);
            rxvalid = (dataWord[0] >> 1) & 0x1;
            ASTERA_INFO("Side:%d, Lane:%2d, PHY rxvalid = %d", side, lane,
                        rxvalid);
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Aries Test Mode inject error
 */
AriesErrorType ariesTestModeTxErrorInject(AriesDeviceType* device)
{
    AriesErrorType rc;
    uint8_t dataWord[2];
    int side;
    int lane;
    int qs;
    int qsLane;

    for (side = 0; side < 2; side++)
    {
        for (lane = 0; lane < 16; lane++)
        {
            qs = lane / 4;
            qsLane = lane % 4;
            rc = ariesReadWordPmaLaneIndirect(device->i2cDriver, side, qs,
                                              qsLane, 0x1072, dataWord);
            CHECK_SUCCESS(rc);
            dataWord[0] |= (1 << 4);
            rc = ariesWriteWordPmaLaneIndirect(device->i2cDriver, side, qs,
                                               qsLane, 0x1072, dataWord);
            CHECK_SUCCESS(rc);
            dataWord[0] &= ~(1 << 4);
            rc = ariesWriteWordPmaLaneIndirect(device->i2cDriver, side, qs,
                                               qsLane, 0x1072, dataWord);
            CHECK_SUCCESS(rc);
        }
    }

    return ARIES_SUCCESS;
}

/**
 * Aries trigger FW reload on next secondary bus reset (SBR).
 * Supported on FW v1.28.x onwards
 */
AriesErrorType ariesFWReloadOnNextSBR(AriesDeviceType* device)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    // Set glb_err_msk_reg2[1]=1 to trigger FW reload on next SBR.
    // Aries FW will self-clear this bit once SBR-triggered FW reload starts.
    rc = ariesReadByteData(device->i2cDriver, 0xe, dataByte);
    CHECK_SUCCESS(rc);
    dataByte[0] |= 0x02;
    rc = ariesWriteByteData(device->i2cDriver, 0xe, dataByte);
    CHECK_SUCCESS(rc);
    return ARIES_SUCCESS;
}

/**
 * Aries get over temperature status
 * Supported on FW v1.28.x onwards
 */
AriesErrorType ariesOvertempStatusGet(AriesDeviceType* device, bool* value)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    // OVERTEMP_STATUS is glb_err_msk_reg2[2].
    rc = ariesReadByteData(device->i2cDriver, 0xe, dataByte);
    CHECK_SUCCESS(rc);
    *value = (dataByte[0] >> 2) & 1;
    return ARIES_SUCCESS;
}

/**
 * Aries get status of I2C bus self-clear event log.
 */
AriesErrorType ariesGetI2CBusClearEventStatus(AriesDeviceType* device,
                                              int* status)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    dataByte[0] = ARIES_GENERIC_STATUS_MASK_I2C_CLEAR_EVENT;
    rc = ariesReadByteData(device->i2cDriver, ARIES_GENERIC_STATUS_ADDR,
                           dataByte);
    CHECK_SUCCESS(rc);
    if (dataByte[0] & ARIES_GENERIC_STATUS_MASK_I2C_CLEAR_EVENT)
        *status = 1;
    else
        *status = 0;
    return ARIES_SUCCESS;
}

/**
 * Aries clear I2C bus self-clear event log status.
 */
AriesErrorType ariesClearI2CBusClearEventStatus(AriesDeviceType* device)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    dataByte[0] = ARIES_GENERIC_STATUS_MASK_I2C_CLEAR_EVENT;
    rc = ariesWriteByteData(device->i2cDriver, ARIES_GENERIC_STATUS_CLR_ADDR,
                            dataByte);
    CHECK_SUCCESS(rc);
    return ARIES_SUCCESS;
}

#ifdef __cplusplus
}
#endif
