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
 * @file aries_link.c
 * @brief Implementation of public Link level functions for the SDK.
 */

#include "include/aries_link.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Bifurcation modes lookup */
extern AriesBifurcationParamsType bifurcationModes[36];

/*
 * Set the PCIe Protocol Reset.
 */
AriesErrorType ariesSetPcieReset(AriesLinkType* link, uint8_t reset)
{
    AriesErrorType rc;
    uint8_t dataByte[1];

    if (reset == 1)
    {
        rc = ariesReadByteData(link->device->i2cDriver, 0x604, dataByte);
        CHECK_SUCCESS(rc);
        dataByte[0] &= ~(1 << link->config.linkId);
        rc = ariesWriteByteData(link->device->i2cDriver, 0x604, dataByte);
        CHECK_SUCCESS(rc);
    }
    else if (reset == 0)
    {
        rc = ariesReadByteData(link->device->i2cDriver, 0x604, dataByte);
        CHECK_SUCCESS(rc);
        dataByte[0] |= (1 << link->config.linkId);
        rc = ariesWriteByteData(link->device->i2cDriver, 0x604, dataByte);
        CHECK_SUCCESS(rc);
    }
    else
    {
        return ARIES_INVALID_ARGUMENT;
    }

    return ARIES_SUCCESS;
}

/*
 * Check link health.
 * Check for eye height, width against thresholds
 * Check for temperature thresholds
 */
AriesErrorType ariesCheckLinkHealth(AriesLinkType* link)
{
    AriesErrorType rc;

    link->state.linkOkay = true;

    rc = ariesGetLinkState(link);
    CHECK_SUCCESS(rc);

    // Get current temperature and check against threshold
    rc = ariesGetCurrentTemp(link->device);
    CHECK_SUCCESS(rc);
    if ((link->device->currentTempC + ARIES_TEMP_CALIBRATION_OFFSET) >=
        link->device->tempAlertThreshC)
    {
        ASTERA_ERROR(
            "Temperature alert! Current (average) temp observed is above threshold");
        ASTERA_ERROR(
            "  Cur Temp observed (+uncertainty) = %f",
            (link->device->currentTempC + ARIES_TEMP_CALIBRATION_OFFSET));
        ASTERA_ERROR("  Alert threshold = %f", link->device->tempAlertThreshC);
        link->state.linkOkay = false;
    }
    else if ((link->device->currentTempC + ARIES_TEMP_CALIBRATION_OFFSET) >=
             link->device->tempWarnThreshC)
    {
        ASTERA_WARN(
            "Temperature warn! Current (average) temp observed is above threshold");
        ASTERA_WARN(
            "  Cur Temp observed (+uncertainty) = %f",
            (link->device->currentTempC + ARIES_TEMP_CALIBRATION_OFFSET));
        ASTERA_WARN("  Warn threshold = %f", link->device->tempWarnThreshC);
    }

    // Get max temp stat and check against threshold
    rc = ariesGetMaxTemp(link->device);
    CHECK_SUCCESS(rc);
    if ((link->device->maxTempC + ARIES_TEMP_CALIBRATION_OFFSET) >=
        link->device->tempAlertThreshC)
    {
        ASTERA_ERROR(
            "Temperature alert! All-time max temp observed is above threshold");
        ASTERA_ERROR("  Max Temp observed (+uncertainty) = %f",
                     (link->device->maxTempC + ARIES_TEMP_CALIBRATION_OFFSET));
        ASTERA_ERROR("  Alert threshold = %f", link->device->tempAlertThreshC);
        link->state.linkOkay = false;
    }
    else if ((link->device->maxTempC + ARIES_TEMP_CALIBRATION_OFFSET) >=
             link->device->tempWarnThreshC)
    {
        ASTERA_WARN(
            "Temperature warn! All-time max temp observed is above threshold");
        ASTERA_WARN("  Max Temp observed (+uncertainty) = %f",
                    (link->device->maxTempC + ARIES_TEMP_CALIBRATION_OFFSET));
        ASTERA_WARN("  Warn threshold = %f", link->device->tempWarnThreshC);
    }

    // Check over-temp alert
    bool overtempFlag;
    rc = ariesOvertempStatusGet(link->device, &overtempFlag);
    CHECK_SUCCESS(rc);
    if (overtempFlag)
    {
        link->device->overtempAlert = true;
    }
    else
    {
        link->device->overtempAlert = false;
    }

    // Get orientation - normal or reversed
    // 0 is normal, 1 is reversed
    int orientation;
    rc = ariesGetPortOrientation(link->device, &orientation);
    CHECK_SUCCESS(rc);

    int upstreamSide;
    int downstreamSide;

    if (orientation == 0)
    {
        upstreamSide = 1;
        downstreamSide = 0;
    }
    else
    {
        upstreamSide = 0;
        downstreamSide = 1;
    }

    // Check Link State
    // Set linkOkay to false if link is not in FWd state
    if (link->state.state != ARIES_STATE_FWD)
    {
        link->state.linkOkay = false;
    }

    // Check link FoM values
    int laneIndex;
    int absLane;

    int pathID;
    int lane;

    int startLane = ariesGetStartLane(link);

    uint8_t dataWord[2];
    uint8_t minFoM = 0xff;
    uint8_t thisLaneFoM;
    char* minFoMRx = "A_PER0";
    for (laneIndex = 0; laneIndex < link->state.width; laneIndex++)
    {
        absLane = startLane + laneIndex;
        pathID = floor(absLane / 4) * 4;
        lane = absLane % 4;

        rc = ariesGetMinFoMVal(link->device, upstreamSide, pathID, lane,
                               ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_ADAPT_FOM,
                               dataWord);
        CHECK_SUCCESS(rc);

        // FoM value is 7:0 of word
        thisLaneFoM = dataWord[0];
        if (thisLaneFoM <= minFoM)
        {
            minFoM = thisLaneFoM;
            if (orientation == 0)
            {
                minFoMRx = link->device->pins[absLane].pinSet1.rxPin;
            }
            else
            {
                minFoMRx = link->device->pins[absLane].pinSet2.rxPin;
            }
        }

        rc = ariesGetMinFoMVal(link->device, downstreamSide, pathID, lane,
                               ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_ADAPT_FOM,
                               dataWord);
        CHECK_SUCCESS(rc);

        // FoM value is 7:0 of word
        thisLaneFoM = dataWord[0];
        if (thisLaneFoM <= minFoM)
        {
            minFoM = thisLaneFoM;
            if (orientation == 0)
            {
                minFoMRx = link->device->pins[absLane].pinSet2.rxPin;
            }
            else
            {
                minFoMRx = link->device->pins[absLane].pinSet1.rxPin;
            }
        }
    }

    // For Gen-2 and below, FoM values do not make sense. Hence we set it to
    // default values
    if (link->state.rate >= 3)
    {
        link->state.linkMinFoM = minFoM;
        link->state.linkMinFoMRx = minFoMRx;
    }
    else
    {
        link->state.linkMinFoM = 0xff;
        link->state.linkMinFoMRx = "";
    }

    // Trigger alerts for Gen 3 and above only
    if ((link->state.linkMinFoM <= link->device->minLinkFoMAlert) &&
        (link->state.rate >= 3))
    {
        link->state.linkOkay = false;
        ASTERA_ERROR("Lane FoM alert! %s FoM below threshold (Val: 0x%02x)",
                     link->state.linkMinFoMRx, link->state.linkMinFoM);
    }

    // Capture the recovery count
    int recoveryCount = 0;
    rc = ariesGetLinkRecoveryCount(link, &recoveryCount);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

AriesErrorType ariesLinkGetCurrentWidth(AriesLinkType* link, int* currentWidth)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint32_t addressOffset;
    // Check state of each path micro in the link. If it is in FWD we add to
    // current width
    *currentWidth = 0;

    int i;
    int startLane = ariesGetStartLane(link);
    for (i = startLane; i < link->config.maxWidth + startLane; i++)
    {
        addressOffset = ARIES_QS_0_CSR_OFFSET +
                        (ARIES_PATH_WRAPPER_1_CSR_OFFSET * i);

        rc = ariesReadByteData(link->device->i2cDriver, addressOffset + 0xb7,
                               dataByte);
        CHECK_SUCCESS(rc);

        if (dataByte[0] == 0x13) // FWD state
        {
            (*currentWidth)++;
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Get the Link recovery counter value.
 */
AriesErrorType ariesGetLinkRecoveryCount(AriesLinkType* link,
                                         int* recoveryCount)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint32_t address;

    // Get current recovery count value
    address = link->device->mm_print_info_struct_addr +
              ARIES_PRINT_INFO_STRUCT_LNK_RECOV_ENTRIES_PTR_OFFSET +
              link->config.linkId;
    rc = ariesReadByteDataMainMicroIndirect(link->device->i2cDriver, address,
                                            dataByte);
    CHECK_SUCCESS(rc);

    // Update the value in the struct
    link->state.recoveryCount = dataByte[0];

    *recoveryCount = dataByte[0];
    return ARIES_SUCCESS;
}

/*
 * Clear the Link recovery counter value.
 */
AriesErrorType ariesClearLinkRecoveryCount(AriesLinkType* link)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint32_t address;

    // Set current recovery count value
    dataByte[0] = 0;
    address = link->device->mm_print_info_struct_addr +
              ARIES_PRINT_INFO_STRUCT_LNK_RECOV_ENTRIES_PTR_OFFSET +
              link->config.linkId;
    rc = ariesWriteByteDataMainMicroIndirect(link->device->i2cDriver, address,
                                             dataByte);
    CHECK_SUCCESS(rc);

    // Update the value in the struct
    link->state.recoveryCount = 0;

    return ARIES_SUCCESS;
}

/*
 * Get the current Link state.
 */
AriesErrorType ariesGetLinkState(AriesLinkType* link)
{
    AriesErrorType rc;
    AriesBifurcationType bifMode;
    uint8_t dataByte[1];
    uint8_t dataWord[2];
    uint32_t baseAddress;
    uint32_t addressOffset;
    int linkNum;
    int linkIdx;

    // Get current bifurcation settings
    rc = ariesGetBifurcationMode(link->device, &bifMode);
    CHECK_SUCCESS(rc);

    // Get link number in bifurcation mode
    // Iterate over links in bifurcation lookup table and find start lane
    // Lane number is part of bifurcation link properties
    bool laneFound = false;
    for (linkIdx = 0; linkIdx < bifurcationModes[bifMode].numLinks; linkIdx++)
    {
        if (link->config.startLane ==
            (bifurcationModes[bifMode].links[linkIdx].startLane))
        {
            linkNum = bifurcationModes[bifMode].links[linkIdx].linkId;
            laneFound = true;
            break;
        }
    }

    if (laneFound == false)
    {
        return ARIES_LINK_CONFIG_INVALID;
    }

    int timeout = 0;
    bool ready = 0;

    ariesGetGPIO(link->device, 0, &ready);
    while ((++timeout < 10) && !ready)
    {
        if (timeout == 1)
        {
            ASTERA_INFO(
                "Waiting for FW load to complete before fetching link state...");
        }
        sleep(1);
        ariesGetGPIO(link->device, 0, &ready);
    }

    if (!ready)
    {
        ASTERA_ERROR(
            "ariesGetLinkState timed out while waiting for FW load to complete.");
        return ARIES_FAILURE;
    }

    // Get link path struct offsets
    // The link path struct sits on top of the MM link struct
    // Hence need to compute this offset first, before getting link struct
    // parameters
    addressOffset = ARIES_MAIN_MICRO_FW_INFO +
                    ARIES_MM_LINK_STRUCT_ADDR_OFFSET +
                    (linkNum * ARIES_LINK_ADDR_EL_SIZE);

    rc = ariesReadBlockDataMainMicroIndirect(link->device->i2cDriver,
                                             addressOffset, 2, dataWord);
    CHECK_SUCCESS(rc);
    int linkStructAddr = dataWord[0] + (dataWord[1] << 8);

    // Compute offset, at which link struct members are available
    baseAddress = AL_MAIN_SRAM_DMEM_OFFSET + linkStructAddr +
                  (link->device->linkPathStructSize * 2);

    // Read link width
    addressOffset = baseAddress + ARIES_LINK_STRUCT_WIDTH_OFFSET;
    rc = ariesReadByteDataMainMicroIndirect(link->device->i2cDriver,
                                            addressOffset, dataByte);
    CHECK_SUCCESS(rc);
    link->state.width = dataByte[0];

    // Read detected width
    addressOffset = baseAddress + ARIES_LINK_STRUCT_DETECTED_WIDTH_OFFSET;
    rc = ariesReadByteDataMainMicroIndirect(link->device->i2cDriver,
                                            addressOffset, dataByte);
    CHECK_SUCCESS(rc);
    link->state.detWidth = dataByte[0];

    // Read current width
    int currentWidth = 0;
    rc = ariesLinkGetCurrentWidth(link, &currentWidth);
    CHECK_SUCCESS(rc);
    link->state.curWidth = currentWidth;

    // Read current state
    // Current state is offset at 10 from base address
    addressOffset = baseAddress + ARIES_LINK_STRUCT_STATE_OFFSET;
    rc = ariesReadByteDataMainMicroIndirect(link->device->i2cDriver,
                                            addressOffset, dataByte);
    CHECK_SUCCESS(rc);
    link->state.state = dataByte[0];

    // Read current rate
    // Current rate is offset at 6 from base address
    // Rate value is offset by 1, i.e. 0: Gen1, 1: Gen2, ... 4: Gen5
    // Hence update rate value by 1
    addressOffset = baseAddress + ARIES_LINK_STRUCT_RATE_OFFSET;
    rc = ariesReadByteDataMainMicroIndirect(link->device->i2cDriver,
                                            addressOffset, dataByte);
    CHECK_SUCCESS(rc);
    link->state.rate = dataByte[0] + 1;

    return ARIES_SUCCESS;
}

/*
 * Get detailed link state
 */
AriesErrorType ariesGetLinkStateDetailed(AriesLinkType* link)
{
    AriesErrorType rc;
    uint8_t dataWord[2];
    float usppSpeed = 0.0;
    float dsppSpeed = 0.0;

    // Update link state parameters
    rc = ariesCheckLinkHealth(link);
    CHECK_SUCCESS(rc);

    int width = link->state.width;
    int startLane = ariesGetStartLane(link);
    int laneIndex;
    int upstreamSide;
    int downstreamSide;
    int upstreamDirection;
    int downstreamDirection;
    int upstreamPinSet;
    int downstreamPinSet;
    int usppTxDirection;
    int dsppTxDirection;
    int usppRxDirection;
    int dsppRxDirection;

    // Get orientation - normal or reversed
    // 0 is normal, 1 is reversed
    int orientation;
    rc = ariesGetPortOrientation(link->device, &orientation);
    CHECK_SUCCESS(rc);

    if (orientation == 0) // Normal Orientation
    {
        upstreamSide = 1;
        downstreamSide = 0;
        upstreamDirection = 1;
        downstreamDirection = 0;
        upstreamPinSet = 0;
        downstreamPinSet = 1;
        usppRxDirection = 0;
        usppTxDirection = 1;
        dsppRxDirection = 1;
        dsppTxDirection = 0;
    }
    else // Reversed orientation
    {
        upstreamSide = 0;
        downstreamSide = 1;
        upstreamDirection = 0;
        downstreamDirection = 1;
        upstreamPinSet = 1;
        downstreamPinSet = 0;
        usppRxDirection = 1;
        usppTxDirection = 0;
        dsppRxDirection = 0;
        dsppTxDirection = 1;
    }

    // Get Current Speeds
    // Upstream path speed
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int absLane = startLane + laneIndex;
        int rxTerm;
        rc = ariesGetLinkRxTerm(link, upstreamSide, absLane, &rxTerm);
        CHECK_SUCCESS(rc);
        if (rxTerm == 1)
        {
            rc = ariesGetLinkCurrentSpeed(link, absLane, upstreamDirection,
                                          &usppSpeed);
            CHECK_SUCCESS(rc);
            break;
        }
    }

    // Downstream path speed
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int absLane = startLane + laneIndex;
        int rxTerm;
        rc = ariesGetLinkRxTerm(link, downstreamSide, absLane, &rxTerm);
        CHECK_SUCCESS(rc);
        if (rxTerm == 1)
        {
            rc = ariesGetLinkCurrentSpeed(link, absLane, downstreamDirection,
                                          &dsppSpeed);
            CHECK_SUCCESS(rc);
            break;
        }
    }

    link->state.usppSpeed = usppSpeed;
    link->state.dsppSpeed = dsppSpeed;

    // Get Logical Lane for both upstream and downstream
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int laneNum;
        int absLane = startLane + laneIndex;

        // Upstream direction (RX and TX)
        rc = ariesGetLogicalLaneNum(link, absLane, usppRxDirection, &laneNum);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].logicalLaneNum = laneNum;
        rc = ariesGetLogicalLaneNum(link, absLane, usppTxDirection, &laneNum);
        CHECK_SUCCESS(rc);
        link->state.usppState.txState[laneIndex].logicalLaneNum = laneNum;

        // Downstream direction (RX and TX)
        rc = ariesGetLogicalLaneNum(link, absLane, dsppRxDirection, &laneNum);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].logicalLaneNum = laneNum;
        rc = ariesGetLogicalLaneNum(link, absLane, dsppTxDirection, &laneNum);
        CHECK_SUCCESS(rc);
        link->state.dsppState.txState[laneIndex].logicalLaneNum = laneNum;
    }

    //////////////////////// USPP & DSPP Parameters ////////////////////////

    // Physical Pin Info
    // This is from a fixed array defined in aries_globals.c
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int absLane = startLane + laneIndex;

        if (orientation == 0)
        {
            link->state.usppState.rxState[laneIndex].physicalPinName =
                link->device->pins[absLane].pinSet1.rxPin;
            link->state.usppState.txState[laneIndex].physicalPinName =
                link->device->pins[absLane].pinSet1.txPin;
            link->state.dsppState.rxState[laneIndex].physicalPinName =
                link->device->pins[absLane].pinSet2.rxPin;
            link->state.dsppState.txState[laneIndex].physicalPinName =
                link->device->pins[absLane].pinSet2.txPin;
        }
        else
        {
            link->state.dsppState.rxState[laneIndex].physicalPinName =
                link->device->pins[absLane].pinSet1.rxPin;
            link->state.dsppState.txState[laneIndex].physicalPinName =
                link->device->pins[absLane].pinSet1.txPin;
            link->state.usppState.rxState[laneIndex].physicalPinName =
                link->device->pins[absLane].pinSet2.rxPin;
            link->state.usppState.txState[laneIndex].physicalPinName =
                link->device->pins[absLane].pinSet2.txPin;
        }
    }

    // Current TX Pre-Cursor value (valid for Gen-3 and above)
    // else 0
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        if ((usppSpeed == 2.5) || (usppSpeed == 5))
        {
            link->state.usppState.txState[laneIndex].pre = 0;
            link->state.dsppState.txState[laneIndex].pre = 0;
        }
        else
        {
            // Physical Path N drives the TX de-emphasis values for Path 1-N
            // Hence upstream stats will be in downstream path
            int txPre;
            int absLane = startLane + laneIndex;
            // Upstream direction
            rc = ariesGetTxPre(link, absLane, downstreamDirection, &txPre);
            CHECK_SUCCESS(rc);
            link->state.usppState.txState[laneIndex].pre = txPre;

            // Downstream parameters
            rc = ariesGetTxPre(link, absLane, upstreamDirection, &txPre);
            CHECK_SUCCESS(rc);
            link->state.dsppState.txState[laneIndex].pre = txPre;
        }
    }

    // Get TX Current Cursor value (valid for Gen-3 and above)
    // else 0
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        if ((usppSpeed == 2.5) || (usppSpeed == 5))
        {
            link->state.usppState.txState[laneIndex].cur = 0;
            link->state.dsppState.txState[laneIndex].cur = 0;
        }
        else
        {
            // Physical Path N drives the TX de-emphasis values for Path 1-N
            // Hence upstream stats will be in downstream path
            int txCur;
            int absLane = startLane + laneIndex;
            // Upstream parameters
            rc = ariesGetTxCur(link, absLane, downstreamDirection, &txCur);
            CHECK_SUCCESS(rc);
            link->state.usppState.txState[laneIndex].cur = txCur;

            // Downstream parameters
            rc = ariesGetTxCur(link, absLane, upstreamDirection, &txCur);
            CHECK_SUCCESS(rc);
            link->state.dsppState.txState[laneIndex].cur = txCur;
        }
    }

    // Get RX Polarity
    // Get RX pseudoport param - opposite paths
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int rxPolarity;
        int absLane = startLane + laneIndex;
        // Upstream parameters
        rc = ariesGetRxPolarityCode(link, absLane, usppRxDirection,
                                    upstreamPinSet, &rxPolarity);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].polarity = rxPolarity;

        // Downstream parameters
        rc = ariesGetRxPolarityCode(link, absLane, dsppRxDirection,
                                    downstreamPinSet, &rxPolarity);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].polarity = rxPolarity;
    }

    // Get TX Post Cursor Value (valid for gen-3 and above)
    // else get post val from tx-pre val
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        // Physical Path N drives the TX de-emphasis values for Path 1-N
        // Hence upstream stats will be in downstream path
        if ((usppSpeed == 2.5) || (usppSpeed == 5))
        {
            // For gen 1 and 2, pre-cursor mode has de-emphasis value
            // Compute pst from this value
            int txPre;
            float txPst;
            int absLane = startLane + laneIndex;

            // Upstream parameters (USPP)
            rc = ariesGetTxPre(link, absLane, downstreamDirection, &txPre);
            CHECK_SUCCESS(rc);
            if (txPre == 0)
            {
                txPst = -6;
            }
            else if (txPre == 1)
            {
                txPst = -3.5;
            }
            else
            {
                txPst = 0;
            }
            link->state.usppState.txState[laneIndex].pst = txPst;
            // De-emphasis value is in tx pre cursor val
            link->state.usppState.txState[laneIndex].de = txPre;

            // Downstream parameters (DSPP)
            rc = ariesGetTxPre(link, absLane, upstreamDirection, &txPre);
            CHECK_SUCCESS(rc);
            if (txPre == 0)
            {
                txPst = -6;
            }
            else if (txPre == 1)
            {
                txPst = -3.5;
            }
            else
            {
                txPst = 0;
            }
            link->state.dsppState.txState[laneIndex].pst = txPst;
            // De-emphasis value is in tx pre cursor val
            link->state.dsppState.txState[laneIndex].de = txPre;
        }
        else
        {
            int txPst;
            int absLane = startLane + laneIndex;
            // Upstream parameters
            rc = ariesGetTxPst(link, absLane, downstreamDirection, &txPst);
            CHECK_SUCCESS(rc);
            link->state.usppState.txState[laneIndex].pst = txPst;
            // De-emphasis value does not apply here
            link->state.usppState.txState[laneIndex].de = 0;

            // Downstream parameters
            rc = ariesGetTxPst(link, absLane, upstreamDirection, &txPst);
            CHECK_SUCCESS(rc);
            link->state.dsppState.txState[laneIndex].pst = txPst;
            // De-emphasis value does not apply here
            link->state.dsppState.txState[laneIndex].de = 0;
        }
    }

    // Get RX TERM (Calculate based on pseudo-port path)
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int rxTerm;
        int absLane = startLane + laneIndex;

        // Upstream path
        rc = ariesGetLinkRxTerm(link, upstreamSide, absLane, &rxTerm);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].termination = rxTerm;
        // Downstream path
        rc = ariesGetLinkRxTerm(link, downstreamSide, absLane, &rxTerm);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].termination = rxTerm;
    }

    // Get RX ATT, VGA, CTLE Boost
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int boostCode;
        int attCode;
        float attValDb;
        int vgaCode;
        float boostValDb;
        int absLane = startLane + laneIndex;

        // Upstream parameters
        rc = ariesGetRxCtleBoostCode(link, upstreamSide, absLane, &boostCode);
        CHECK_SUCCESS(rc);
        rc = ariesGetRxAttCode(link, upstreamSide, absLane, &attCode);
        CHECK_SUCCESS(rc);
        attValDb = attCode * -1.5;
        rc = ariesGetRxVgaCode(link, upstreamSide, absLane, &vgaCode);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].attdB = attValDb;
        link->state.usppState.rxState[laneIndex].vgadB = vgaCode * 0.9;
        boostValDb = ariesGetRxBoostValueDb(boostCode, attValDb, vgaCode);
        link->state.usppState.rxState[laneIndex].ctleBoostdB = boostValDb;

        // Downstream parameters
        rc = ariesGetRxCtleBoostCode(link, downstreamSide, absLane, &boostCode);
        CHECK_SUCCESS(rc);
        rc = ariesGetRxAttCode(link, downstreamSide, absLane, &attCode);
        CHECK_SUCCESS(rc);
        attValDb = attCode * -1.5;
        rc = ariesGetRxVgaCode(link, downstreamSide, absLane, &vgaCode);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].attdB = attValDb;
        link->state.dsppState.rxState[laneIndex].vgadB = vgaCode * 0.9;
        boostValDb = ariesGetRxBoostValueDb(boostCode, attValDb, vgaCode);
        link->state.dsppState.rxState[laneIndex].ctleBoostdB = boostValDb;
    }

    // Get RX Pole Code
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int poleCode;
        int absLane = startLane + laneIndex;

        // Upstream parameters
        rc = ariesGetRxCtlePoleCode(link, upstreamSide, absLane, &poleCode);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].ctlePole = poleCode;

        // Downstream parameters
        rc = ariesGetRxCtlePoleCode(link, downstreamSide, absLane, &poleCode);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].ctlePole = poleCode;
    }

    // Get RX ADAPT IQ Value
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int iqValue;
        int doneValue;
        int absLane = startLane + laneIndex;

        // Upstream parameters
        rc = ariesGetRxAdaptIq(link, upstreamSide, absLane, &iqValue);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].adaptIq = iqValue;
        // Bank 0
        rc = ariesGetRxAdaptIqBank(link, upstreamSide, absLane, 0, &iqValue);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].adaptIqB0 = iqValue;
        rc = ariesGetRxAdaptDoneBank(link, upstreamSide, absLane, 0,
                                     &doneValue);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].adaptDoneB0 = doneValue;
        // Bank 1
        rc = ariesGetRxAdaptIqBank(link, upstreamSide, absLane, 1, &iqValue);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].adaptIqB1 = iqValue;
        rc = ariesGetRxAdaptDoneBank(link, upstreamSide, absLane, 1,
                                     &doneValue);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].adaptDoneB1 = doneValue;
        // Bank 2
        rc = ariesGetRxAdaptIqBank(link, upstreamSide, absLane, 2, &iqValue);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].adaptIqB2 = iqValue;
        rc = ariesGetRxAdaptDoneBank(link, upstreamSide, absLane, 2,
                                     &doneValue);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].adaptDoneB2 = doneValue;

        // Downstream parameters
        rc = ariesGetRxAdaptIq(link, downstreamSide, absLane, &iqValue);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].adaptIq = iqValue;
        // Bank 0
        rc = ariesGetRxAdaptIqBank(link, downstreamSide, absLane, 0, &iqValue);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].adaptIqB0 = iqValue;
        rc = ariesGetRxAdaptDoneBank(link, downstreamSide, absLane, 0,
                                     &doneValue);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].adaptDoneB0 = doneValue;
        // Bank 1
        rc = ariesGetRxAdaptIqBank(link, downstreamSide, absLane, 1, &iqValue);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].adaptIqB1 = iqValue;
        rc = ariesGetRxAdaptDoneBank(link, downstreamSide, absLane, 1,
                                     &doneValue);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].adaptDoneB1 = doneValue;
        // Bank 2
        rc = ariesGetRxAdaptIqBank(link, downstreamSide, absLane, 2, &iqValue);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].adaptIqB2 = iqValue;
        rc = ariesGetRxAdaptDoneBank(link, downstreamSide, absLane, 2,
                                     &doneValue);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].adaptDoneB2 = doneValue;
    }

    // Get RX DFE Values
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int tap = 1;
        int code;
        float dfe;
        int absLane = startLane + laneIndex;

        for (tap = 1; tap <= 8; tap++)
        {
            // Upstream parameters
            rc = ariesGetRxDfeCode(link, upstreamSide, absLane, tap, &code);
            CHECK_SUCCESS(rc);

            switch (tap)
            {
                case 1:
                    dfe = code * 1.85;
                    link->state.usppState.rxState[laneIndex].dfe1 = dfe;
                    break;
                case 2:
                    dfe = code * 0.35;
                    link->state.usppState.rxState[laneIndex].dfe2 = dfe;
                    break;
                case 3:
                    dfe = code * 0.7;
                    link->state.usppState.rxState[laneIndex].dfe3 = dfe;
                    break;
                case 4:
                    dfe = code * 0.35;
                    link->state.usppState.rxState[laneIndex].dfe4 = dfe;
                    break;
                case 5:
                    dfe = code * 0.35;
                    link->state.usppState.rxState[laneIndex].dfe5 = dfe;
                    break;
                case 6:
                    dfe = code * 0.35;
                    link->state.usppState.rxState[laneIndex].dfe6 = dfe;
                    break;
                case 7:
                    dfe = code * 0.35;
                    link->state.usppState.rxState[laneIndex].dfe7 = dfe;
                    break;
                case 8:
                    dfe = code * 0.35;
                    link->state.usppState.rxState[laneIndex].dfe8 = dfe;
                    break;
                default:
                    ASTERA_ERROR("Invalid DFE tap argument");
                    return ARIES_INVALID_ARGUMENT;
                    break;
            }

            // Downstream parameters
            rc = ariesGetRxDfeCode(link, downstreamSide, absLane, tap, &code);
            CHECK_SUCCESS(rc);

            switch (tap)
            {
                case 1:
                    dfe = code * 1.85;
                    link->state.dsppState.rxState[laneIndex].dfe1 = dfe;
                    break;
                case 2:
                    dfe = code * 0.35;
                    link->state.dsppState.rxState[laneIndex].dfe2 = dfe;
                    break;
                case 3:
                    dfe = code * 0.7;
                    link->state.dsppState.rxState[laneIndex].dfe3 = dfe;
                    break;
                case 4:
                    dfe = code * 0.35;
                    link->state.dsppState.rxState[laneIndex].dfe4 = dfe;
                    break;
                case 5:
                    dfe = code * 0.35;
                    link->state.dsppState.rxState[laneIndex].dfe5 = dfe;
                    break;
                case 6:
                    dfe = code * 0.35;
                    link->state.dsppState.rxState[laneIndex].dfe6 = dfe;
                    break;
                case 7:
                    dfe = code * 0.35;
                    link->state.dsppState.rxState[laneIndex].dfe7 = dfe;
                    break;
                case 8:
                    dfe = code * 0.35;
                    link->state.dsppState.rxState[laneIndex].dfe8 = dfe;
                    break;
                default:
                    ASTERA_ERROR("Invalid DFE tap argument");
                    return ARIES_INVALID_ARGUMENT;
                    break;
            }
        }
    }

    // Get Temperature Values
    // Each PMA has a temp sensor. Read that and store value accordingly in
    // lane indexed array
    // ariesReadPmaTemp() returns max temp in 16 readings
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int absLane = startLane + laneIndex;

        // startLane = link->config.startLane;
        int pmaNum = ariesGetPmaNumber(absLane);

        // Upstream values
        float utemp;
        rc = ariesReadPmaTemp(link->device, upstreamSide, pmaNum, &utemp);
        CHECK_SUCCESS(rc);

        link->state.coreState.usppTempC[laneIndex] = utemp;

        // Downstream values
        float dtemp;
        rc = ariesReadPmaTemp(link->device, downstreamSide, pmaNum, &dtemp);
        CHECK_SUCCESS(rc);
        link->state.coreState.dsppTempC[laneIndex] = dtemp;

        link->state.coreState.usppTempAlert[laneIndex] = false;
        link->state.coreState.dsppTempAlert[laneIndex] = false;

        float thresh = link->device->tempAlertThreshC;
        if (utemp >= thresh)
        {
            link->state.coreState.usppTempAlert[laneIndex] = true;
        }
        if (dtemp >= thresh)
        {
            link->state.coreState.dsppTempAlert[laneIndex] = true;
        }
    }

    // Get Path HW State
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int absLane = startLane + laneIndex;
        int state;
        rc = ariesGetPathHWState(link, absLane, upstreamDirection, &state);
        CHECK_SUCCESS(rc);
        link->state.coreState.usppPathHWState[laneIndex] = state;
        rc = ariesGetPathHWState(link, absLane, downstreamDirection, &state);
        CHECK_SUCCESS(rc);
        link->state.coreState.dsppPathHWState[laneIndex] = state;
    }

    // Get DESKEW status (in nanoseconds)
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int status;
        int absLane = startLane + laneIndex;

        // Upstream Path
        rc = ariesGetDeskewClks(link, absLane, upstreamDirection, &status);
        CHECK_SUCCESS(rc);

        int clkPeriod;
        if (usppSpeed == 32)
        {
            clkPeriod = 1; // ns
        }
        else if (usppSpeed == 16)
        {
            clkPeriod = 2; // ns
        }
        else if (usppSpeed == 8)
        {
            clkPeriod = 4; // ns
        }
        else if (usppSpeed == 5)
        {
            clkPeriod = 8; // ns
        }
        else
        {
            // GEN 1, speed = 2.5
            clkPeriod = 16; // ns
        }

        link->state.coreState.usDeskewNs[laneIndex] = status * clkPeriod;

        // Downstream Values
        rc = ariesGetDeskewClks(link, absLane, downstreamDirection, &status);
        CHECK_SUCCESS(rc);

        // Get Clock Speed (ns)
        if (dsppSpeed == 32)
        {
            clkPeriod = 1; // ns
        }
        else if (dsppSpeed == 16)
        {
            clkPeriod = 2; // ns
        }
        else if (dsppSpeed == 8)
        {
            clkPeriod = 4; // ns
        }
        else if (dsppSpeed == 5)
        {
            clkPeriod = 8; // ns
        }
        else
        {
            // GEN 1, speed = 2.5
            clkPeriod = 16; // ns
        }

        link->state.coreState.dsDeskewNs[laneIndex] = status * clkPeriod;
    }

    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int absLane = startLane + laneIndex;
        int pathID = floor(absLane / 4) * 4;
        int lane = absLane % 4;

        rc = ariesGetMinFoMVal(link->device, upstreamSide, pathID, lane,
                               ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_ADAPT_FOM,
                               dataWord);
        CHECK_SUCCESS(rc);
        // FoM value is 7:0 of word.
        link->state.usppState.rxState[laneIndex].FoM = dataWord[0];

        rc = ariesGetMinFoMVal(link->device, downstreamSide, pathID, lane,
                               ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_ADAPT_FOM,
                               dataWord);
        CHECK_SUCCESS(rc);
        // FoM value is 7:0 of word.
        link->state.dsppState.rxState[laneIndex].FoM = dataWord[0];
    }

    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int absLane = laneIndex + startLane;
        int state;
        // FW state
        rc = ariesGetPathFWState(link, absLane, usppRxDirection, &state);
        CHECK_SUCCESS(rc);
        link->state.coreState.usppPathFWState[laneIndex] = state;
        // FW state
        rc = ariesGetPathFWState(link, absLane, dsppRxDirection, &state);
        CHECK_SUCCESS(rc);
        link->state.coreState.dsppPathFWState[laneIndex] = state;
    }

    // Get RX EQ parameters
    for (laneIndex = 0; laneIndex < width; laneIndex++)
    {
        int absLane = startLane + laneIndex;
        int speed;
        int presetReq;
        int preReq;
        int curReq;
        int pstReq;
        int numReq;
        int i;
        int req;
        int fom;

        ///////////////////// Upstream parameters ///////////////////

        // EQ Speed RX Param
        rc = ariesGetLastEqSpeed(link, absLane, usppRxDirection, &speed);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].lastEqRate = speed;

        // EQ Speed TX Param
        rc = ariesGetLastEqSpeed(link, absLane, usppTxDirection, &speed);
        CHECK_SUCCESS(rc);
        link->state.usppState.txState[laneIndex].lastEqRate = speed;

        // Get EQ Preset values
        rc = ariesGetLastEqReqPreset(link, absLane, usppTxDirection,
                                     &presetReq);
        CHECK_SUCCESS(rc);
        link->state.usppState.txState[laneIndex].lastPresetReq = presetReq;

        // If final request was a preset, this is the pre value
        rc = ariesGetLastEqReqPre(link, absLane, usppTxDirection, &preReq);
        CHECK_SUCCESS(rc);
        link->state.usppState.txState[laneIndex].lastPreReq = preReq;

        // If final request was a preset, this is the current value
        rc = ariesGetLastEqReqCur(link, absLane, usppTxDirection, &curReq);
        CHECK_SUCCESS(rc);
        link->state.usppState.txState[laneIndex].lastCurReq = curReq;

        // If final request was a preset, this is the post value
        rc = ariesGetLastEqReqPst(link, absLane, usppTxDirection, &pstReq);
        CHECK_SUCCESS(rc);
        link->state.usppState.txState[laneIndex].lastPstReq = pstReq;

        // Preset requests
        rc = ariesGetLastEqNumPresetReq(link, absLane, usppRxDirection,
                                        &numReq);
        CHECK_SUCCESS(rc);
        link->state.usppState.rxState[laneIndex].lastPresetNumReq = numReq;
        for (i = 0; i < numReq; ++i)
        {
            rc = ariesGetLastEqPresetReq(link, absLane, usppRxDirection, i,
                                         &req);
            CHECK_SUCCESS(rc);
            link->state.usppState.rxState[laneIndex].lastPresetReq[i] = req;
        }

        // Preset request FOM values
        for (i = 0; i < numReq; ++i)
        {
            rc = ariesGetLastEqPresetReqFOM(link, absLane, usppRxDirection, i,
                                            &fom);
            CHECK_SUCCESS(rc);
            link->state.usppState.rxState[laneIndex].lastPresetReqFom[i] = fom;
        }

        ///////////////////// Downstream parameters ///////////////////

        // Downstream EQ speed param (RX)
        rc = ariesGetLastEqSpeed(link, absLane, dsppRxDirection, &speed);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].lastEqRate = speed;

        // Downstream EQ speed param (TX)
        rc = ariesGetLastEqSpeed(link, absLane, dsppTxDirection, &speed);
        CHECK_SUCCESS(rc);
        link->state.dsppState.txState[laneIndex].lastEqRate = speed;

        // Get EQ Preset values
        rc = ariesGetLastEqReqPreset(link, absLane, dsppTxDirection,
                                     &presetReq);
        CHECK_SUCCESS(rc);
        link->state.dsppState.txState[laneIndex].lastPresetReq = presetReq;

        // If final request was a preset, this is the pre value
        rc = ariesGetLastEqReqPre(link, absLane, dsppTxDirection, &preReq);
        CHECK_SUCCESS(rc);
        link->state.dsppState.txState[laneIndex].lastPreReq = preReq;

        // If final request was a preset, this is the cur value
        rc = ariesGetLastEqReqCur(link, absLane, dsppTxDirection, &curReq);
        CHECK_SUCCESS(rc);
        link->state.dsppState.txState[laneIndex].lastCurReq = curReq;

        // If final request was a preset, this is the post value
        rc = ariesGetLastEqReqPst(link, absLane, dsppTxDirection, &pstReq);
        CHECK_SUCCESS(rc);
        link->state.dsppState.txState[laneIndex].lastPstReq = pstReq;

        rc = ariesGetLastEqNumPresetReq(link, absLane, dsppRxDirection,
                                        &numReq);
        CHECK_SUCCESS(rc);
        link->state.dsppState.rxState[laneIndex].lastPresetNumReq = numReq;

        // Preset requests
        for (i = 0; i < numReq; ++i)
        {
            rc = ariesGetLastEqPresetReq(link, absLane, dsppRxDirection, i,
                                         &req);
            CHECK_SUCCESS(rc);
            link->state.dsppState.rxState[laneIndex].lastPresetReq[i] = req;
        }

        // Preset request FOM values
        for (i = 0; i < numReq; ++i)
        {
            rc = ariesGetLastEqPresetReqFOM(link, absLane, dsppRxDirection, i,
                                            &fom);
            CHECK_SUCCESS(rc);
            link->state.dsppState.rxState[laneIndex].lastPresetReqFom[i] = fom;
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Initialize LTSSM logger
 */
AriesErrorType ariesLTSSMLoggerInit(AriesLinkType* link, uint8_t oneBatchModeEn,
                                    AriesLTSSMVerbosityType verbosity)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint8_t dataBytes8[8];
    uint32_t baseAddress;
    uint32_t address;
    int absLane;

    int startLane = ariesGetStartLane(link);

    // Initialize Main Micro logger
    baseAddress = link->device->mm_print_info_struct_addr;

    // Set One Batch Mode
    dataByte[0] = oneBatchModeEn;
    address = baseAddress + ARIES_PRINT_INFO_STRUCT_ONE_BATCH_MODE_EN_OFFSET;
    rc = ariesWriteByteDataMainMicroIndirect(link->device->i2cDriver, address,
                                             dataByte);
    CHECK_SUCCESS(rc);

    // Set print enables
    int pcIndex;
    switch (verbosity)
    {
        case ARIES_LTSSM_VERBOSITY_HIGH:
            for (pcIndex = 0; pcIndex < ARIES_MM_PRINT_INFO_NUM_PRINT_CLASS_EN;
                 pcIndex++)
            {
                dataBytes8[pcIndex] = 0xff;
            }
            break;
        default:
            ASTERA_ERROR("Invalid LTSSM logger verbosity");
            return ARIES_INVALID_ARGUMENT;
            break;
    }
    address = baseAddress + ARIES_MM_PRINT_INFO_STRUCT_PRINT_CLASS_EN_OFFSET;
    rc = ariesWriteBlockDataMainMicroIndirect(link->device->i2cDriver, address,
                                              8, dataBytes8);
    CHECK_SUCCESS(rc);

    // Initialize Path Micro logger
    baseAddress = link->device->pm_print_info_struct_addr;

    // Use laneIndex as path since lane is split between 2 paths and min lanes
    // per link is two. For example - lanes 2 and 3 would be path 2 and 3.
    int laneIndex;
    for (laneIndex = 0; laneIndex < link->state.width; laneIndex++)
    {
        // absLane = link->config.startLane + laneIndex;
        absLane = startLane + laneIndex;

        // Downstream paths
        // Set One Batch Mode
        dataByte[0] = oneBatchModeEn;
        address = baseAddress +
                  ARIES_PRINT_INFO_STRUCT_ONE_BATCH_MODE_EN_OFFSET;
        rc = ariesWriteByteDataPathMicroIndirect(link->device->i2cDriver,
                                                 absLane, address, dataByte);
        CHECK_SUCCESS(rc);

        // Set Print Class enables
        // Can only write micros 8 bytes at a time. Hence we have to spilt the
        // writes. There are 16 entires, so split writes into 2
        int pcCount;
        for (pcCount = 0; pcCount < 2; pcCount++)
        {
            address = baseAddress +
                      ARIES_PM_PRINT_INFO_STRUCT_PRINT_CLASS_EN_OFFSET +
                      (pcCount * 8);
            for (pcIndex = 0; pcIndex < 8; pcIndex++)
            {
                dataBytes8[pcIndex] = 0xff;
            }
            rc = ariesWriteBlockDataPathMicroIndirect(
                link->device->i2cDriver, absLane, address, 8, dataBytes8);
            CHECK_SUCCESS(rc);
        }
    }

    return ARIES_SUCCESS;
}

/*
 * Set Print EN in logger
 */
AriesErrorType ariesLTSSMLoggerPrintEn(AriesLinkType* link, uint8_t printEn)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint32_t baseAddress;
    uint32_t address;
    int absLane;

    int startLane = ariesGetStartLane(link);

    // Do for Main Micro
    baseAddress = link->device->mm_print_info_struct_addr;
    address = baseAddress + ARIES_PRINT_INFO_STRUCT_PRINT_EN_OFFSET;
    dataByte[0] = printEn;
    rc = ariesWriteByteDataMainMicroIndirect(link->device->i2cDriver, address,
                                             dataByte);
    CHECK_SUCCESS(rc);

    // Do for Path Micros
    // Use laneIndex as path since lane is split between 2 paths and min lanes
    // per link is two. For example - lanes 2 and 3 would be path 2 and 3.
    baseAddress = link->device->pm_print_info_struct_addr;

    // Use laneIndex as path since lane is split between 2 paths and min lanes
    // per link is two. For example - lanes 2 and 3 would be path 2 and 3.
    int laneIndex;
    for (laneIndex = 0; laneIndex < link->state.width; laneIndex++)
    {
        // absLane = link->config.startLane + laneIndex;
        absLane = startLane + laneIndex;
        address = baseAddress + ARIES_PRINT_INFO_STRUCT_PRINT_EN_OFFSET;
        dataByte[0] = printEn;

        rc = ariesWriteByteDataPathMicroIndirect(link->device->i2cDriver,
                                                 absLane, address, dataByte);
        CHECK_SUCCESS(rc);
    }

    return ARIES_SUCCESS;
}

/*
 * Read an entry from the LTSSM logger
 */
AriesErrorType ariesLTSSMLoggerReadEntry(AriesLinkType* link,
                                         AriesLTSSMLoggerEnumType logType,
                                         int* offset,
                                         AriesLTSSMEntryType* entry)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    uint32_t baseAddress;
    uint32_t address;

    // Main Micro entry
    if (logType == ARIES_LTSSM_LINK_LOGGER)
    {
        baseAddress = link->device->mm_print_info_struct_addr +
                      ARIES_PRINT_INFO_STRUCT_PRINT_BUFFER_OFFSET;
        address = baseAddress + *offset;
        rc = ariesReadByteDataMainMicroIndirect(link->device->i2cDriver,
                                                address, dataByte);
        CHECK_SUCCESS(rc);
        entry->data = dataByte[0];
        entry->offset = *offset;
        (*offset)++;
    }
    else
    {
        baseAddress = link->device->pm_print_info_struct_addr +
                      ARIES_PRINT_INFO_STRUCT_PRINT_BUFFER_OFFSET;
        address = baseAddress + *offset;
        rc = ariesReadByteDataPathMicroIndirect(link->device->i2cDriver,
                                                logType, address, dataByte);
        CHECK_SUCCESS(rc);
        entry->data = dataByte[0];
        entry->offset = *offset;
        (*offset)++;
    }
    return ARIES_SUCCESS;
}

AriesErrorType ariesLinkDumpDebugInfo(AriesLinkType* link)
{
    AriesErrorType rc;

    // Capture detailed debug information
    // This function prints Aries chip ID, FW version, SDK version,
    // and the detailed link state information to a file
    rc = ariesLinkPrintDetailedState(link, ".", "link_state_detailed");
    CHECK_SUCCESS(rc);
    // This function prints Aries LTSSM logs to a file
    rc = ariesLinkPrintMicroLogs(link, ".", "ltssm_micro_log");
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

// Deprecated! Will be removed in future release
AriesErrorType ariesPrintMicroLogs(AriesLinkType* link, const char* basepath,
                                   const char* filename);
AriesErrorType ariesPrintMicroLogs(AriesLinkType* link, const char* basepath,
                                   const char* filename)
{
    ASTERA_WARN("ariesPrintMicroLogs() has been deprecated "
                "and will be removed in a future major version release "
                "of the Aries C SDK! "
                "Please use ariesLinkPrintMicroLogs() instead");
    return ariesLinkPrintMicroLogs(link, basepath, filename);
}

AriesErrorType ariesLinkPrintMicroLogs(AriesLinkType* link,
                                       const char* basepath,
                                       const char* filename)
{
    AriesErrorType rc;
    char filepath[ARIES_PATH_MAX];
    FILE* fp;

    if (!link || !basepath || !filename)
    {
        ASTERA_ERROR("Invalid link or basepath or filename passed");
        return ARIES_INVALID_ARGUMENT;
    }

    if (strlen(basepath) == 0 || strlen(filename) == 0)
    {
        ASTERA_ERROR("Can't load a file without the basepath or filename");
        return ARIES_INVALID_ARGUMENT;
    }

    int startLane = ariesGetStartLane(link);

    // Write micro logs output to a file
    // File is printed as a python dict so that we can post-process it to a
    // readable format as log messages
    snprintf(filepath, ARIES_PATH_MAX, "%s/%s_%d.py", basepath, filename,
             link->config.linkId);

    fp = fopen(filepath, "w");
    if (fp == NULL)
    {
        ASTERA_ERROR("Could not open the specified filepath");
        return ARIES_INVALID_ARGUMENT;
    }

    fprintf(fp, "# AUTOGENERATED FILE. DO NOT EDIT #\n");
    fprintf(fp, "# GENERATED WTIH C SDK VERSION %s #\n\n\n",
            ariesGetSDKVersion());

    fprintf(fp, "fw_version_major = %d\n", link->device->fwVersion.major);
    fprintf(fp, "fw_version_minor = %d\n", link->device->fwVersion.minor);
    fprintf(fp, "fw_version_build = %d\n", link->device->fwVersion.build);

    // Initialise logger
    rc = ariesLTSSMLoggerInit(link, 0, ARIES_LTSSM_VERBOSITY_HIGH);
    if (rc != ARIES_SUCCESS)
    {
        fprintf(
            fp,
            "# Encountered error during ariesLinkPrintMicroLogs->ariesLTSSMLoggerInit. Closing file\n");
        fclose(fp);
        return rc;
    }

    // Enable Macros to print
    rc = ariesLTSSMLoggerPrintEn(link, 1);
    if (rc != ARIES_SUCCESS)
    {
        fprintf(
            fp,
            "# Encountered error during ariesLinkPrintMicroLogs->ariesLTSSMLoggerPrintEn. Closing file\n");
        fclose(fp);
        return rc;
    }

    fprintf(fp, "aries_micro_logs = [\n");
    fprintf(fp, "    {\n");
    fprintf(fp, "        'log_type': %d,\n", ARIES_LTSSM_LINK_LOGGER);
    fprintf(fp, "        'log': [      # Open MM log\n");
    // Print Main micro log
    rc = ariesPrintLog(link, ARIES_LTSSM_LINK_LOGGER, fp);
    fprintf(fp, "        ],    # Close MM log\n");
    fprintf(fp, "    },\n");
    if (rc != ARIES_SUCCESS)
    {
        fprintf(
            fp,
            "# Encountered error during ariesLinkPrintMicroLogs->ariesPrintLog. Closing file\n");
        fclose(fp);
        return rc;
    }

    // Print path micro logs
    int laneNum;
    int laneIdx;
    for (laneIdx = 0; laneIdx < link->state.width; laneIdx++)
    {
        laneNum = laneIdx + startLane;
        fprintf(fp, "    {\n");
        fprintf(fp, "        'log_type': %d,\n", laneNum);
        fprintf(fp, "        'log': [      # Open PM %d log\n", laneNum);
        rc |= ariesPrintLog(link, laneNum, fp);
        fprintf(fp, "        ],    # Close PM %d log\n", laneNum);
        fprintf(fp, "    },\n");
    }
    fprintf(fp, "]\n");

    if (rc != ARIES_SUCCESS)
    {
        fprintf(
            fp,
            "# Encountered error during ariesLinkPrintMicroLogs->ariesPrintLog. Closing file\n");
        fclose(fp);
        return rc;
    }
    fclose(fp);

    return ARIES_SUCCESS;
}

// Deprecated! Will be removed in future release
AriesErrorType ariesPrintLinkDetailedState(AriesLinkType* link,
                                           const char* basepath,
                                           const char* filename);

AriesErrorType ariesPrintLinkDetailedState(AriesLinkType* link,
                                           const char* basepath,
                                           const char* filename)
{
    ASTERA_WARN("ariesPrintLinkDetailedState() has been deprecated "
                "and will be removed in a future major version release "
                "of the Aries C SDK! "
                "Please use ariesLinkPrintDetailedState() instead");
    return ariesLinkPrintDetailedState(link, basepath, filename);
}

// Capture the detailed link state and print it to file
AriesErrorType ariesLinkPrintDetailedState(AriesLinkType* link,
                                           const char* basepath,
                                           const char* filename)
{
    AriesErrorType rc;
    char filepath[ARIES_PATH_MAX];
    FILE* fp;

    if (!link || !basepath || !filename)
    {
        ASTERA_ERROR("Invalid link or basepath or filename passed");
        return ARIES_INVALID_ARGUMENT;
    }

    if (strlen(basepath) == 0 || strlen(filename) == 0)
    {
        ASTERA_ERROR("Can't load a file without the basepath or filename");
        return ARIES_INVALID_ARGUMENT;
    }

    // Check overall device health
    rc = ariesCheckDeviceHealth(link->device);
    CHECK_SUCCESS(rc);

    // Check firmware state
    rc = ariesFWStatusCheck(link->device);
    CHECK_SUCCESS(rc);

    // Get Link state
    rc = ariesGetLinkStateDetailed(&link[0]);
    CHECK_SUCCESS(rc);

    int startLane = ariesGetStartLane(link);

    // Write link state detailed output to a file
    // File is printed as a python dict so that we can post-process it to a
    // readable format as a table
    snprintf(filepath, ARIES_PATH_MAX, "%s/%s_%d.py", basepath, filename,
             link->config.linkId);

    fp = fopen(filepath, "w");
    if (fp == NULL)
    {
        ASTERA_ERROR("Could not open the specified filepath");
        return ARIES_INVALID_ARGUMENT;
    }

    fprintf(fp, "# AUTOGENERATED FILE. DO NOT EDIT #\n");
    fprintf(fp, "# GENERATED WTIH C SDK VERSION %s #\n\n\n",
            ariesGetSDKVersion());

    // Aries device struct parameters
    fprintf(fp, "aries_device = {}\n");
    fprintf(fp, "aries_device['c_sdk_version'] = \"%s\"\n",
            ariesGetSDKVersion());
    fprintf(fp, "aries_device['fw_version_major'] = %d\n",
            link->device->fwVersion.major);
    fprintf(fp, "aries_device['fw_version_minor'] = %d\n",
            link->device->fwVersion.minor);
    fprintf(fp, "aries_device['fw_version_build'] = %d\n",
            link->device->fwVersion.build);
    int b = 0;
    for (b = 0; b < 12; b += 1)
    {
        fprintf(fp, "aries_device['chipID_%d'] = %d\n", b,
                link->device->chipID[b]);
    }
    for (b = 0; b < 6; b += 1)
    {
        fprintf(fp, "aries_device['lotNumber_%d'] = %d\n", b,
                link->device->lotNumber[b]);
    }
    fprintf(fp, "aries_device['fullPartNumber'] = \"%s\"\n",
            link->device->fullPartNumber);
    fprintf(fp, "aries_device['revNumber'] = %d\n", link->device->revNumber);
    fprintf(fp, "aries_device['deviceOkay'] = %s\n",
            link->device->deviceOkay ? "True" : "False");
    fprintf(fp, "aries_device['allTimeMaxTempC'] = %f\n",
            link->device->maxTempC);
    fprintf(fp, "aries_device['currentTempC'] = %f\n",
            link->device->currentTempC);
    fprintf(fp, "aries_device['tempAlertThreshC'] = %f\n\n",
            link->device->tempAlertThreshC);

    // Aries link struct parameters
    fprintf(fp, "aries_link = {}\n");
    fprintf(fp, "aries_link['link_id'] = %d\n", link->config.linkId);
    fprintf(fp, "aries_link['linkOkay'] = %s\n",
            link->state.linkOkay ? "True" : "False");
    fprintf(fp, "aries_link['state'] = %d\n", link->state.state);
    fprintf(fp, "aries_link['width'] = %d\n", link->state.width);
    fprintf(fp, "aries_link['detWidth'] = %d\n", link->state.detWidth);
    fprintf(fp, "aries_link['curWidth'] = %d\n", link->state.curWidth);
    fprintf(fp, "aries_link['linkMinFoM'] = %d\n", link->state.linkMinFoM);
    fprintf(fp, "aries_link['linkMinFoMRx'] = '%s'\n",
            link->state.linkMinFoMRx);
    fprintf(fp, "aries_link['recoveryCount'] = %d\n",
            link->state.recoveryCount);
    fprintf(fp, "aries_link['uspp_speed'] = %2.1f\n", link->state.usppSpeed);
    fprintf(fp, "aries_link['dspp_speed'] = %2.1f\n", link->state.dsppSpeed);
    fprintf(fp, "aries_link['uspp'] = {}\n");
    fprintf(fp, "aries_link['uspp']['tx'] = {}\n");
    fprintf(fp, "aries_link['uspp']['rx'] = {}\n");
    fprintf(fp, "aries_link['dspp'] = {}\n");
    fprintf(fp, "aries_link['dspp']['tx'] = {}\n");
    fprintf(fp, "aries_link['dspp']['rx'] = {}\n");
    fprintf(fp, "aries_link['rt_core'] = {}\n");
    fprintf(fp, "aries_link['rt_core']['us'] = {}\n");
    fprintf(fp, "aries_link['rt_core']['ds'] = {}\n");

    int phyLaneNum;
    int laneIndex;
    int i;
    for (laneIndex = 0; laneIndex < link->state.width; laneIndex++)
    {
        phyLaneNum = laneIndex + startLane;

        fprintf(fp,
                "\n\n### Stats for logical lane %d (physical lane %d) ###\n",
                laneIndex, phyLaneNum);

        fprintf(fp, "aries_link['uspp']['tx'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['logical_lane'] = %d\n",
                laneIndex,
                link->state.usppState.txState[laneIndex].logicalLaneNum);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['physical_pin'] = '%s'\n",
                laneIndex,
                link->state.usppState.txState[laneIndex].physicalPinName);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['de'] = %d\n", laneIndex,
                link->state.usppState.txState[laneIndex].de);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['pre'] = %d\n", laneIndex,
                link->state.usppState.txState[laneIndex].pre);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['cur'] = %d\n", laneIndex,
                link->state.usppState.txState[laneIndex].cur);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['pst'] = %f\n", laneIndex,
                link->state.usppState.txState[laneIndex].pst);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_eq_rate'] = %d\n",
                laneIndex, link->state.usppState.txState[laneIndex].lastEqRate);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_preset_req'] = %d\n",
                laneIndex,
                link->state.usppState.txState[laneIndex].lastPresetReq);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_pre_req'] = %d\n",
                laneIndex, link->state.usppState.txState[laneIndex].lastPreReq);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_cur_req'] = %d\n",
                laneIndex, link->state.usppState.txState[laneIndex].lastCurReq);
        fprintf(fp, "aries_link['uspp']['tx'][%d]['last_pst_req'] = %d\n",
                laneIndex, link->state.usppState.txState[laneIndex].lastPstReq);
        fprintf(fp, "\n");

        fprintf(fp, "aries_link['uspp']['rx'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['logical_lane'] = %d\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].logicalLaneNum);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['physical_pin'] = '%s'\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].physicalPinName);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['termination'] = %d\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].termination);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['polarity'] = %d\n",
                laneIndex, link->state.usppState.rxState[laneIndex].polarity);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['att_db'] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].attdB);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['ctle_boost_db'] = %f\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].ctleBoostdB);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['ctle_pole'] = %d\n",
                laneIndex, link->state.usppState.rxState[laneIndex].ctlePole);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['vga_db'] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].vgadB);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['adapt_iq'] = %d\n",
                laneIndex, link->state.usppState.rxState[laneIndex].adaptIq);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['adapt_iq_b0'] = %d\n",
                laneIndex, link->state.usppState.rxState[laneIndex].adaptIqB0);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['adapt_done_b0'] = %d\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].adaptDoneB0);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['adapt_iq_b1'] = %d\n",
                laneIndex, link->state.usppState.rxState[laneIndex].adaptIqB1);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['adapt_done_b1'] = %d\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].adaptDoneB1);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['adapt_iq_b2'] = %d\n",
                laneIndex, link->state.usppState.rxState[laneIndex].adaptIqB2);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['adapt_done_b2'] = %d\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].adaptDoneB2);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'] = {}\n", laneIndex);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][1] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].dfe1);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][2] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].dfe2);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][3] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].dfe3);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][4] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].dfe4);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][5] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].dfe5);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][6] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].dfe6);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][7] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].dfe7);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['dfe'][8] = %f\n", laneIndex,
                link->state.usppState.rxState[laneIndex].dfe8);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_eq_rate'] = %d\n",
                laneIndex, link->state.usppState.rxState[laneIndex].lastEqRate);
        fprintf(fp,
                "aries_link['uspp']['rx'][%d]['last_preset_num_req'] = %d\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].lastPresetNumReq);
        fprintf(fp, "aries_link['uspp']['rx'][%d]['last_preset_req'] = %d\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].lastPresetReq[0]);
        fprintf(fp,
                "aries_link['uspp']['rx'][%d]['last_preset_req_fom'] = %d\n",
                laneIndex,
                link->state.usppState.rxState[laneIndex].lastPresetReqFom[0]);
        for (i = 1;
             i < link->state.usppState.rxState[laneIndex].lastPresetNumReq; ++i)
        {
            fprintf(
                fp,
                "aries_link['uspp']['rx'][%d]['last_preset_req_m%d'] = %d\n",
                laneIndex, i,
                link->state.usppState.rxState[laneIndex].lastPresetReq[i]);
            fprintf(
                fp,
                "aries_link['uspp']['rx'][%d]['last_preset_req_fom_m%d'] = %d\n",
                laneIndex, i,
                link->state.usppState.rxState[laneIndex].lastPresetReqFom[i]);
        }
        fprintf(fp, "\n");

        fprintf(fp, "aries_link['rt_core']['us'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['tj_c'] = %2.2f\n",
                laneIndex, link->state.coreState.usppTempC[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['skew_ns'] = %d\n",
                laneIndex, link->state.coreState.usDeskewNs[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['tj_c_alert'] = %d\n",
                laneIndex, link->state.coreState.usppTempAlert[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['pth_fw_state'] = %d\n",
                laneIndex, link->state.coreState.usppPathFWState[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['us'][%d]['pth_hw_state'] = %d\n",
                laneIndex, link->state.coreState.usppPathHWState[laneIndex]);

        fprintf(fp, "aries_link['rt_core']['ds'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['tj_c'] = %2.2f\n",
                laneIndex, link->state.coreState.dsppTempC[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['skew_ns'] = %d\n",
                laneIndex, link->state.coreState.dsDeskewNs[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['tj_c_alert'] = %d\n",
                laneIndex, link->state.coreState.dsppTempAlert[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['pth_fw_state'] = %d\n",
                laneIndex, link->state.coreState.dsppPathFWState[laneIndex]);
        fprintf(fp, "aries_link['rt_core']['ds'][%d]['pth_hw_state'] = %d\n",
                laneIndex, link->state.coreState.dsppPathHWState[laneIndex]);
        fprintf(fp, "\n");

        fprintf(fp, "aries_link['dspp']['tx'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['logical_lane'] = %d\n",
                laneIndex,
                link->state.dsppState.txState[laneIndex].logicalLaneNum);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['physical_pin'] = '%s'\n",
                laneIndex,
                link->state.dsppState.txState[laneIndex].physicalPinName);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['de'] = %d\n", laneIndex,
                link->state.dsppState.txState[laneIndex].de);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['pre'] = %d\n", laneIndex,
                link->state.dsppState.txState[laneIndex].pre);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['cur'] = %d\n", laneIndex,
                link->state.dsppState.txState[laneIndex].cur);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['pst'] = %f\n", laneIndex,
                link->state.dsppState.txState[laneIndex].pst);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_eq_rate'] = %d\n",
                laneIndex, link->state.dsppState.txState[laneIndex].lastEqRate);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_preset_req'] = %d\n",
                laneIndex,
                link->state.dsppState.txState[laneIndex].lastPresetReq);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_pre_req'] = %d\n",
                laneIndex, link->state.dsppState.txState[laneIndex].lastPreReq);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_cur_req'] = %d\n",
                laneIndex, link->state.dsppState.txState[laneIndex].lastCurReq);
        fprintf(fp, "aries_link['dspp']['tx'][%d]['last_pst_req'] = %d\n",
                laneIndex, link->state.dsppState.txState[laneIndex].lastPstReq);
        fprintf(fp, "\n");

        fprintf(fp, "aries_link['dspp']['rx'][%d] = {}\n", laneIndex);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['logical_lane'] = %d\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].logicalLaneNum);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['physical_pin'] = '%s'\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].physicalPinName);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['termination'] = %d\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].termination);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['polarity'] = %d\n",
                laneIndex, link->state.dsppState.rxState[laneIndex].polarity);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['att_db'] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].attdB);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['ctle_boost_db'] = %f\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].ctleBoostdB);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['ctle_pole'] = %d\n",
                laneIndex, link->state.dsppState.rxState[laneIndex].ctlePole);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['vga_db'] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].vgadB);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['adapt_iq'] = %d\n",
                laneIndex, link->state.dsppState.rxState[laneIndex].adaptIq);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['adapt_iq_b0'] = %d\n",
                laneIndex, link->state.dsppState.rxState[laneIndex].adaptIqB0);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['adapt_done_b0'] = %d\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].adaptDoneB0);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['adapt_iq_b1'] = %d\n",
                laneIndex, link->state.dsppState.rxState[laneIndex].adaptIqB1);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['adapt_done_b1'] = %d\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].adaptDoneB1);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['adapt_iq_b2'] = %d\n",
                laneIndex, link->state.dsppState.rxState[laneIndex].adaptIqB2);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['adapt_done_b2'] = %d\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].adaptDoneB2);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'] = {}\n", laneIndex);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][1] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].dfe1);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][2] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].dfe2);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][3] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].dfe3);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][4] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].dfe4);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][5] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].dfe5);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][6] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].dfe6);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][7] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].dfe7);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['dfe'][8] = %f\n", laneIndex,
                link->state.dsppState.rxState[laneIndex].dfe8);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_eq_rate'] = %d\n",
                laneIndex, link->state.dsppState.rxState[laneIndex].lastEqRate);
        fprintf(fp,
                "aries_link['dspp']['rx'][%d]['last_preset_num_req'] = %d\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].lastPresetNumReq);
        fprintf(fp, "aries_link['dspp']['rx'][%d]['last_preset_req'] = %d\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].lastPresetReq[0]);
        fprintf(fp,
                "aries_link['dspp']['rx'][%d]['last_preset_req_fom'] = %d\n",
                laneIndex,
                link->state.dsppState.rxState[laneIndex].lastPresetReqFom[0]);
        for (i = 1;
             i < link->state.dsppState.rxState[laneIndex].lastPresetNumReq; ++i)
        {
            fprintf(
                fp,
                "aries_link['dspp']['rx'][%d]['last_preset_req_m%d'] = %d\n",
                laneIndex, i,
                link->state.dsppState.rxState[laneIndex].lastPresetReq[i]);
            fprintf(
                fp,
                "aries_link['dspp']['rx'][%d]['last_preset_req_fom_m%d'] = %d\n",
                laneIndex, i,
                link->state.dsppState.rxState[laneIndex].lastPresetReqFom[i]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);

    return ARIES_SUCCESS;
}

// Print the micro logger entries
AriesErrorType ariesPrintLog(AriesLinkType* link, AriesLTSSMLoggerEnumType log,
                             FILE* fp)
{
    AriesLTSSMEntryType ltssmEntry;
    int offset = 0;
    int oneBatchModeEn;
    int oneBatchWrEn;
    int batchComplete;
    int currFmtID;
    int currWriteOffset;
    AriesErrorType rc;
    int full = 0;

    ltssmEntry.logType = log;

    // Buffer size different for main and path micros
    int bufferSize;
    if (log == ARIES_LTSSM_LINK_LOGGER)
    {
        bufferSize = ARIES_MM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE;
    }
    else
    {
        bufferSize = ARIES_PM_PRINT_INFO_NUM_PRINT_BUFFER_SIZE;
    }

    // Get One batch mode enable
    rc = ariesGetLoggerOneBatchModeEn(link, log, &oneBatchModeEn);
    CHECK_SUCCESS(rc);

    // Get One batch write enable
    rc = ariesGetLoggerOneBatchWrEn(link, log, &oneBatchWrEn);
    CHECK_SUCCESS(rc);

    if (oneBatchModeEn == 0)
    {
        // Stop Micros from printing
        rc = ariesLTSSMLoggerPrintEn(link, 0);
        CHECK_SUCCESS(rc);

        // In this mode we print the logger from current write offset
        // and reset the offset to zero once we reach the end of the buffer
        // Capture current write offset
        rc = ariesGetLoggerWriteOffset(link, log, &currWriteOffset);
        CHECK_SUCCESS(rc);
        // Start offset from the current (paused) offset
        offset = currWriteOffset;

        // Print logger from current offset
        while (offset < bufferSize)
        {
            rc = ariesLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
            CHECK_SUCCESS(rc);
            fprintf(fp, "            {\n");
            fprintf(fp, "                'data': %d,\n", ltssmEntry.data);
            fprintf(fp, "                'offset': %d\n", ltssmEntry.offset);
            fprintf(fp, "            },\n");
        }

        // Wrap around and continue reading the log entries
        if (currWriteOffset != 0)
        {
            // Reset offset to start from beginning
            offset = 0;

            // Print logger from start of print buffer
            while (offset < currWriteOffset)
            {
                // Read logger entry
                rc = ariesLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
                CHECK_SUCCESS(rc);
                fprintf(fp, "            {\n");
                fprintf(fp, "                'data': %d,\n", ltssmEntry.data);
                fprintf(fp, "                'offset': %d\n",
                        ltssmEntry.offset);
                fprintf(fp, "            },\n");
            }
        }

        // Enable Macros to print
        rc = ariesLTSSMLoggerPrintEn(link, 1);
        CHECK_SUCCESS(rc);
    }
    else
    {
        // Check if batch is complete
        batchComplete = (oneBatchModeEn == 1) && (oneBatchWrEn == 0);

        // Read format ID at current offset
        rc = ariesGetLoggerFmtID(link, log, offset, &currFmtID);
        CHECK_SUCCESS(rc);

        if (batchComplete == 1)
        {
            full = 1;
            while ((currFmtID != 0) && (offset < bufferSize))
            {
                // Get logger entry
                rc = ariesLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
                CHECK_SUCCESS(rc);
                fprintf(fp, "            {\n");
                fprintf(fp, "                'data': %d,\n", ltssmEntry.data);
                fprintf(fp, "                'offset': %d\n",
                        ltssmEntry.offset);
                fprintf(fp, "            },\n");
                // Read Fmt ID
                rc = ariesGetLoggerFmtID(link, log, offset, &currFmtID);
                CHECK_SUCCESS(rc);
            }
        }
        else
        {
            // Print from start of the buffer until the end
            while ((currFmtID != 0) && (offset < bufferSize) &&
                   (offset < currWriteOffset))
            {
                // Get logger entry
                rc = ariesLTSSMLoggerReadEntry(link, log, &offset, &ltssmEntry);
                CHECK_SUCCESS(rc);
                fprintf(fp, "            {\n");
                fprintf(fp, "                'data': %d,\n", ltssmEntry.data);
                fprintf(fp, "                'offset': %d\n",
                        ltssmEntry.offset);
                fprintf(fp, "            },\n");
                // Read Fmt ID
                rc = ariesGetLoggerFmtID(link, log, offset, &currFmtID);
                CHECK_SUCCESS(rc);
                // Read current write offset
                rc = ariesGetLoggerWriteOffset(link, log, &currWriteOffset);
                CHECK_SUCCESS(rc);
            }
        }
    }

    if (full == 0)
    {
        ASTERA_INFO("There is more to print ...");
    }

    return ARIES_SUCCESS;
}

#ifdef __cplusplus
}
#endif
