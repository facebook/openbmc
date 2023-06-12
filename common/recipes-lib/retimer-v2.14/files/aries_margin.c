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
 * @file aries_margin.c
 * @brief Implementation of receiver margining functions for the SDK.
 */

#include "include/aries_margin.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * noCommand does nothing
 */
AriesErrorType ariesMarginNoCommand(AriesRxMarginType* marginDevice)
{
    (void)marginDevice;
    ASTERA_WARN("Sending this out of band doesn't do anything");
    return ARIES_SUCCESS;
}

/*
 * Not implemented
 */
AriesErrorType ariesMarginAccessRetimerRegister(AriesRxMarginType* marginDevice)
{
    (void)marginDevice;
    ASTERA_WARN("Register access is not implemented yet");
    return ARIES_SUCCESS;
}

/*
 * Report margin control capabilities
 */
AriesErrorType
    ariesMarginReportMarginControlCapabilities(AriesRxMarginType* marginDevice,
                                               int* capabilities)
{
    (void)marginDevice;
    ASTERA_INFO("Reporting Margin Control Capabilities:");
    ASTERA_INFO("  M_VOLTAGE_SUPPORTED = %d", VOLTAGESUPPORTED);
    ASTERA_INFO("  M_IND_UP_DOWN_VOLTAGE = %d", INDUPDOWNVOLTAGE);
    ASTERA_INFO("  M_IND_LEFT_RIGHT_TIMING = %d", INDLEFTRIGHTTIMING);
    ASTERA_INFO("  M_SAMPLE_REPORTING_METHOD = %d", SAMPLEREPORTINGMETHOD);
    ASTERA_INFO("  M_IND_ERROR_SAMPLER = %d", INDERRORSAMPLER);
    capabilities[0] = VOLTAGESUPPORTED;
    capabilities[1] = INDUPDOWNVOLTAGE;
    capabilities[2] = INDLEFTRIGHTTIMING;
    capabilities[3] = SAMPLEREPORTINGMETHOD;
    capabilities[4] = INDERRORSAMPLER;

    return ARIES_SUCCESS;
}

/*
 * Report number of voltage steps
 */
AriesErrorType ariesMarginReportNumVoltageSteps(AriesRxMarginType* marginDevice,
                                                int* numVoltageSteps)
{
    (void)marginDevice;
    *numVoltageSteps = NUMVOLTAGESTEPS;
    return ARIES_SUCCESS;
}

/*
 * Report number of timing steps
 */
AriesErrorType ariesMarginReportNumTimingSteps(AriesRxMarginType* marginDevice,
                                               int* numTimingSteps)
{
    (void)marginDevice;
    *numTimingSteps = NUMTIMINGSTEPS;
    return ARIES_SUCCESS;
}

/*
 * Report max timing offset
 */
AriesErrorType ariesMarginReportMaxTimingOffset(AriesRxMarginType* marginDevice,
                                                int* maxTimingOffset)
{
    (void)marginDevice;
    *maxTimingOffset = MAXTIMINGOFFSET;
    return ARIES_SUCCESS;
}

/*
 * Report max voltage offset
 */
AriesErrorType
    ariesMarginReportMaxVoltageOffset(AriesRxMarginType* marginDevice,
                                      int* maxVoltageOffset)
{
    (void)marginDevice;
    *maxVoltageOffset = MAXVOLTAGEOFFSET;
    return ARIES_SUCCESS;
}

/*
 * Report sampling rate voltage
 */
AriesErrorType
    ariesMarginReportSamplingRateVoltage(AriesRxMarginType* marginDevice,
                                         int* samplingRateVoltage)
{
    (void)marginDevice;
    *samplingRateVoltage = SAMPLINGRATEVOLTAGE;
    return ARIES_SUCCESS;
}

/*
 * Report sampling rate timing
 */
AriesErrorType
    ariesMarginReportSamplingRateTiming(AriesRxMarginType* marginDevice,
                                        int* samplingRateTiming)
{
    (void)marginDevice;
    *samplingRateTiming = NUMTIMINGSTEPS;
    return ARIES_SUCCESS;
}

/*
 * We do not use sample count
 */
AriesErrorType ariesMarginReportSampleCount(AriesRxMarginType* marginDevice)
{
    (void)marginDevice;
    ASTERA_WARN("We support sampling rates instead of sample count");
    return ARIES_SUCCESS;
}

/*
 * Report max lanes
 */
AriesErrorType ariesMarginReportMaxLanes(AriesRxMarginType* marginDevice,
                                         int* maxLanes)
{
    (void)marginDevice;
    *maxLanes = MAXLANES;
    return ARIES_SUCCESS;
}

/*
 * Set the Error Count Limit
 */
AriesErrorType ariesMarginSetErrorCountLimit(AriesRxMarginType* marginDevice,
                                             int limit)
{
    if (limit > 63)
    {
        ASTERA_WARN(
            "Error Count Limit cannot be greater than 63. Setting it to 63");
        marginDevice->errorCountLimit = 63;
    }
    else
    {
        ASTERA_DEBUG("Error Count Limit is %d", limit);
        marginDevice->errorCountLimit = limit;
    }
    return ARIES_SUCCESS;
}

/*
 * Reset the phase and voltage offset and disable margining
 */
AriesErrorType ariesMarginGoToNormalSettings(AriesRxMarginType* marginDevice,
                                             AriesPseudoPortType port,
                                             int* lane, int laneCount)
{
    AriesErrorType rc;

    rc = ariesMarginPmaRxMarginStop(marginDevice, port, lane, laneCount);
    CHECK_SUCCESS(rc);

    // rc = ariesMarginClearErrorLog(marginDevice, port, lane, laneCount);
    // CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Clears error count of a given port and lane
 */
AriesErrorType ariesMarginClearErrorLog(AriesRxMarginType* marginDevice,
                                        AriesPseudoPortType port, int* lane,
                                        int laneCount)
{
    int ln;
    for (ln = 0; ln < laneCount; ln++)
    {
        marginDevice->errorCount[port][lane[ln]] = 0;
    }
    return ARIES_SUCCESS;
}

/*
 * Margin the device at a specified timing offset
 */
AriesErrorType ariesMarginStepMarginToTimingOffset(
    AriesRxMarginType* marginDevice, AriesPseudoPortType port, int* lane,
    int* direction, int* steps, int laneCount, double dwell, int* eCount)
{
    AriesErrorType rc;
    int ln;

    for (ln = 0; ln < laneCount; ln++)
    {
        if (direction[ln] != 0 && direction[ln] != 1)
        {
            ASTERA_ERROR("Unsupported direction argument, must be 1 or 0");
            return ARIES_INVALID_ARGUMENT;
        }
        if (steps[ln] > NUMTIMINGSTEPS)
        {
            ASTERA_ERROR(
                "Unsupported Lane Margining command: Exceeded NumTimingSteps");
            return ARIES_INVALID_ARGUMENT;
        }
    }

    rc = ariesMarginPmaRxMarginTiming(marginDevice, port, lane, direction,
                                      steps, laneCount);
    CHECK_SUCCESS(rc);
    usleep((int)(dwell * 1000000));
    rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane, laneCount);
    CHECK_SUCCESS(rc);
    if (marginDevice->do1XAnd0XCapture)
    {
        rc = ariesMarginPmaRxMarginTiming(marginDevice, port, lane, direction,
                                          steps, laneCount);
        CHECK_SUCCESS(rc);
        usleep((int)(dwell * 100000));
        rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane,
                                             laneCount);
        CHECK_SUCCESS(rc);
    }
    for (ln = 0; ln < laneCount; ln++)
    {
        if (marginDevice->errorCount[port][lane[ln]] > 63)
        {
            marginDevice->errorCount[port][lane[ln]] = 63;
        }
        if (marginDevice->errorCount[port][lane[ln]] >
            marginDevice->errorCountLimit)
        {
            ASTERA_DEBUG(
                "Error count on port %d lane %d exceeded error count limit: %d > %d",
                port, lane[ln], marginDevice->errorCount[port][lane[ln]],
                marginDevice->errorCountLimit);
            eCount[ln] = marginDevice->errorCount[port][lane[ln]];
            ASTERA_DEBUG("Port %d lane %d is going back to default settings",
                         port, lane[ln]);
            rc = ariesMarginGoToNormalSettings(marginDevice, port, lane + ln,
                                               1);
            CHECK_SUCCESS(rc);
        }
        else
        {
            ASTERA_DEBUG(
                "Margining time is in progress on port %d lane %d. Current error count is: %d",
                port, lane[ln], marginDevice->errorCount[port][lane[ln]]);
            eCount[ln] = marginDevice->errorCount[port][lane[ln]];
        }
    }
    return ARIES_SUCCESS;
}

/*
 * Margin the device at a specified voltage offset
 */
AriesErrorType ariesMarginStepMarginToVoltageOffset(
    AriesRxMarginType* marginDevice, AriesPseudoPortType port, int* lane,
    int* direction, int* steps, int laneCount, double dwell, int* eCount)
{
    AriesErrorType rc;
    int ln;

    for (ln = 0; ln < laneCount; ln++)
    {
        if (direction[ln] != 0 && direction[ln] != 1)
        {
            ASTERA_ERROR("Unsupported direction argument, must be 0 or 1");
            return ARIES_INVALID_ARGUMENT;
        }
        if (steps[ln] > NUMVOLTAGESTEPS)
        {
            ASTERA_ERROR(
                "Unsupported Lane Margining command: Exceeded NumTimingSteps");
            return ARIES_INVALID_ARGUMENT;
        }
    }

    rc = ariesMarginPmaRxMarginVoltage(marginDevice, port, lane, direction,
                                       steps, laneCount);
    CHECK_SUCCESS(rc);
    usleep((int)(dwell * 100000));
    rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane, laneCount);
    CHECK_SUCCESS(rc);
    if (marginDevice->do1XAnd0XCapture)
    {
        for (ln = 0; ln < laneCount; ln++)
        {
            direction[ln] = 1 - direction[ln];
        }
        rc = ariesMarginPmaRxMarginVoltage(marginDevice, port, lane, direction,
                                           steps, laneCount);
        CHECK_SUCCESS(rc);
        usleep((int)(dwell * 100000));
        rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane,
                                             laneCount);
        CHECK_SUCCESS(rc);
    }
    for (ln = 0; ln < laneCount; ln++)
    {
        if (marginDevice->errorCount[port][lane[ln]] > 63)
        {
            marginDevice->errorCount[port][lane[ln]] = 63;
        }
        if (marginDevice->errorCount[port][lane[ln]] >
            marginDevice->errorCountLimit)
        {
            ASTERA_DEBUG(
                "Error count on port %d lane %d exceeded error count limit: %d > %d",
                port, lane[ln], marginDevice->errorCount[port][lane[ln]],
                marginDevice->errorCountLimit);
            eCount[ln] = marginDevice->errorCount[port][lane[ln]];
            ASTERA_DEBUG("Port %d lane %d is going back to default settings",
                         port, lane[ln]);
            rc = ariesMarginGoToNormalSettings(marginDevice, port, lane + ln,
                                               1);
            CHECK_SUCCESS(rc);
        }
        else
        {
            ASTERA_DEBUG(
                "Margining voltage is in progress on port %d lane %d. Current error count is: %d",
                port, lane[ln], marginDevice->errorCount[port][lane[ln]]);
            eCount[ln] = marginDevice->errorCount[port][lane[ln]];
        }
    }
    return ARIES_SUCCESS;
}

/*
 * Not implemented
 */
AriesErrorType ariesMarginVendorDefined(AriesRxMarginType* marginDevice)
{
    (void)marginDevice;
    ASTERA_WARN("Vendor defined function is not implemented yet");
    return ARIES_SUCCESS;
}

/*
 * Tell the retimer we are finished margining
 */
AriesErrorType ariesMarginPmaRxMarginStop(AriesRxMarginType* marginDevice,
                                          AriesPseudoPortType port, int* lane,
                                          int laneCount)
{
    AriesErrorType rc;
    int side, quadSlice, quadSliceLane;
    int ln;

    for (ln = 0; ln < laneCount; ln++)
    {
        rc = ariesMarginDeterminePmaSideAndQs(
            marginDevice, port, lane[ln], &side, &quadSlice, &quadSliceLane);
        CHECK_SUCCESS(rc);

        uint8_t dataWord[2];
        rc = ariesReadWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, dataWord);
        CHECK_SUCCESS(rc);
        uint8_t en = (dataWord[0] >> 1) & 0x1; // enable has offset of 1

        if (en == 1)
        {
            // Setting OVRD enable to 0 for rx IQ
            // Offset of 12, width of 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7, 12, 0, 1);
            CHECK_SUCCESS(rc);
            // Setting OVRD enable to 0 for margin vdac
            // Offset of 11, width of 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 11, 0, 1);
            CHECK_SUCCESS(rc);
            // Setting OVRD enable to 0 for margin in progress
            // Offset of 13, width of 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 13, 0, 1);
            CHECK_SUCCESS(rc);

            rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane + ln,
                                                 1);
            CHECK_SUCCESS(rc);

            // Setting OVRD enable to 0 for margin error clear
            // Offset of 1, width of 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 1, 0, 1);
            CHECK_SUCCESS(rc);
        }
    }
    return ARIES_SUCCESS;
}

/*
 * Set margin timing sampler to a specified time value
 */
AriesErrorType ariesMarginPmaRxMarginTiming(AriesRxMarginType* marginDevice,
                                            AriesPseudoPortType port, int* lane,
                                            int* direction, int* steps,
                                            int laneCount)
{
    AriesErrorType rc;
    int ln;

    // Determine pma side and qs from device port and lane
    int side, quadSlice, quadSliceLane;
    for (ln = 0; ln < laneCount; ln++)
    {
        rc = ariesMarginDeterminePmaSideAndQs(
            marginDevice, port, lane[ln], &side, &quadSlice, &quadSliceLane);
        CHECK_SUCCESS(rc);

        // positive deltaValues move the slicer to the left
        // assume the direction is left and if it is right we will overwrite
        // deltaValue later
        uint8_t deltaValue = steps[ln];

        // negative deltaValues move the slicer to the right
        // if the direction is right, and we want to move at least 1 step then
        // take the negation of our steps, so we move the opposite direction
        if (direction[ln] == 1 && steps[ln] > 0) // right and not 0 steps
        {
            deltaValue = ((~steps[ln] + 1) & 0x7f);
        }

        // Setting IQ OVRD to 1 and setting the IQ value
        // offset 12, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7, 12, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 5, width 7
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7, 5, deltaValue, 7);
        CHECK_SUCCESS(rc);

        // set Rx margin error clear ovrd en to 1 and Rx margin error clear ovrd
        // to 1 ALS-105: Stop a dummy req being generated by keeping Margin
        // error clear high in case iq value is unchanged form last command
        // offset 1, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 1, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 0, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 0, 1, 1);
        CHECK_SUCCESS(rc);

        // set rxX margin in prog
        // offset 13, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 13, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 12, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 12, 1, 1);
        CHECK_SUCCESS(rc);
    }
    rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane, laneCount);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Set margin voltage sampler to a specified voltage
 */
AriesErrorType ariesMarginPmaRxMarginVoltage(AriesRxMarginType* marginDevice,
                                             AriesPseudoPortType port,
                                             int* lane, int* direction,
                                             int* steps, int laneCount)
{
    AriesErrorType rc;
    int side, quadSlice, quadSliceLane;
    int ln;

    for (ln = 0; ln < laneCount; ln++)
    {
        // Determine pma side and qs from device port and lane
        rc = ariesMarginDeterminePmaSideAndQs(
            marginDevice, port, lane[ln], &side, &quadSlice, &quadSliceLane);
        CHECK_SUCCESS(rc);

        // positive vdacValues move the slicer up
        // assume the direction is up and if it is down we will overwrite
        // deltaValue later

        uint16_t vdacValue = steps[ln];

        // negative vdacValues move the slicer down
        // if the direction is down, and we want to move at least 1 step then
        // take the negation of our steps, so we move the opposite direction
        // (aka down)
        if (direction[ln] == 1 && steps[ln] > 0) // down or not 0 steps
        {
            vdacValue = ((~steps[ln] + 1) & 0x1ff);
        }

        // set VDAC override to 1 and set the VDAC value
        // offset 11, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 11, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 2, width 9
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 2, vdacValue, 9);
        CHECK_SUCCESS(rc);

        // set Rx margin error clear ovrd en to 1 and Rx margin error clear ovrd
        // to 1 ALS-105: Stop a dummy req being generated by keeping Margin
        // error clear high in case iq value is unchanged form last command
        // offset 1, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 1, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 0, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 0, 1, 1);
        CHECK_SUCCESS(rc);

        // set rxX margin in prog
        // offset 13, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 13, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 12, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 12, 1, 1);
        CHECK_SUCCESS(rc);
    }

    rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane, laneCount);
    CHECK_SUCCESS(rc);

    return ARIES_SUCCESS;
}

/*
 * Set margin sampler to specified timing and voltage
 */
AriesErrorType ariesMarginPmaTimingVoltageOffset(
    AriesRxMarginType* marginDevice, AriesPseudoPortType port, int* lane,
    int* timeDirection, int* timeSteps, int* voltageDirection,
    int* voltageSteps, int laneCount, double dwell, int* eCount)
{
    AriesErrorType rc;
    int side, quadSlice, quadSliceLane;
    uint16_t deltaValue = 0, vdacValue = 0;
    int ln;

    for (ln = 0; ln < laneCount; ln++)
    {
        // Determine pma side and qs from device port and lane
        rc = ariesMarginDeterminePmaSideAndQs(
            marginDevice, port, lane[ln], &side, &quadSlice, &quadSliceLane);
        CHECK_SUCCESS(rc);

        // get the 1X eye capture
        deltaValue = timeSteps[ln];
        vdacValue = voltageSteps[ln];

        if (timeDirection[ln] == 1 && timeSteps[ln] != 0)
        {
            deltaValue = ((~timeSteps[ln] + 1) & 0x7f);
        }

        if (voltageDirection[ln] == 1 && timeSteps[ln] != 0)
        {
            vdacValue = ((~voltageSteps[ln] + 1) & 0x1ff);
        }

        // set IQ override to 1 and set the IQ value
        // offset 12, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7, 12, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 5, width 7
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7, 5, deltaValue, 7);
        CHECK_SUCCESS(rc);
        // set VDAC override to 1 and set the VDAC value
        // offset 11, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 11, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 2, width 9
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 2, vdacValue, 9);
        CHECK_SUCCESS(rc);

        // set Rx margin error clear ovrd en to 1 and Rx margin error clear ovrd
        // to 1 ALS-105: Stop a dummy req being generated by keeping Margin
        // error clear high in case iq value is unchanged form last command
        // offset 1, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 1, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 0, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 0, 1, 1);
        CHECK_SUCCESS(rc);

        // set rxX margin in prog
        // offset 13, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 13, 1, 1);
        CHECK_SUCCESS(rc);
        // offset 12, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 12, 1, 1);
        CHECK_SUCCESS(rc);
    }

    rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane, laneCount);
    CHECK_SUCCESS(rc);

    usleep((int)(dwell * 100000));

    rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane, laneCount);
    CHECK_SUCCESS(rc);

    if (marginDevice->do1XAnd0XCapture)
    {
        for (ln = 0; ln < laneCount; ln++)
        {
            voltageDirection[ln] = 1 - voltageDirection[ln];

            if (timeDirection[ln] == 1 && timeSteps[ln] != 0)
            {
                deltaValue = ((~timeSteps[ln] + 1) & 0x7f);
            }

            if (voltageDirection[ln] == 1 && timeSteps[ln] != 0)
            {
                vdacValue = ((~voltageSteps[ln] + 1) & 0x1ff);
            }

            // set IQ override to 1 and set the IQ value
            // offset 12, width 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7, 12, 1, 1);
            CHECK_SUCCESS(rc);
            // offset 5, with 7
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7, 5, deltaValue, 7);
            CHECK_SUCCESS(rc);
            // set VDAC override to 1 and set the VDAC value
            // offset 11, with 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 11, 1, 1);
            CHECK_SUCCESS(rc);
            // offset 2, width 9
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 2, vdacValue, 9);
            CHECK_SUCCESS(rc);

            // set Rx margin error clear ovrd en to 1 and Rx margin error clear
            // ovrd to 1 ALS-105: Stop a dummy req being generated by keeping
            // Margin error clear high in case iq value is unchanged form last
            // command offset 1, width 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 1, 1, 1);
            CHECK_SUCCESS(rc);
            // offset 0, width 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 0, 1, 1);
            CHECK_SUCCESS(rc);

            // set rxX margin in prog
            // offset 13, width 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 13, 1, 1);
            CHECK_SUCCESS(rc);
            // offset 12, width 1
            rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9, 12, 1, 1);
            CHECK_SUCCESS(rc);
        }

        rc = ariesMarginPmaRxReqAckHandshake(marginDevice, port, lane,
                                             laneCount);
        CHECK_SUCCESS(rc);

        usleep((int)(dwell * 100000));

        rc = ariesMarginPmaRxMarginGetECount(marginDevice, port, lane,
                                             laneCount);
        CHECK_SUCCESS(rc);
    }

    for (ln = 0; ln < laneCount; ln++)
    {
        if (marginDevice->errorCount[port][lane[ln]] > 63)
            marginDevice->errorCount[port][lane[ln]] = 63;
        eCount[ln] = marginDevice->errorCount[port][lane[ln]];
    }

    return ARIES_SUCCESS;
}

/*
 * Updates the errorCount array with the number of errors that occurred in a
 * given port and lane
 */
AriesErrorType ariesMarginPmaRxMarginGetECount(AriesRxMarginType* marginDevice,
                                               AriesPseudoPortType port,
                                               int* lane, int laneCount)
{
    // Determine pma side and quad slice
    AriesErrorType rc;
    int side, quadSlice, quadSliceLane;
    int ln;

    for (ln = 0; ln < laneCount; ln++)
    {
        rc = ariesMarginDeterminePmaSideAndQs(
            marginDevice, port, lane[ln], &side, &quadSlice, &quadSliceLane);
        CHECK_SUCCESS(rc);

        // Get error count from error count register
        uint8_t dataWord[2];
        rc = ariesReadWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_RX_CTL_RX_MARGIN_ERROR, dataWord);
        CHECK_SUCCESS(rc);

        uint8_t eCount =
            dataWord[0] &
            0x3f; // eCount is only 6 bits wide. We want the first 6 bits.

        // update errorCount array
        marginDevice->errorCount[port][lane[ln]] += eCount;
    }

    return ARIES_SUCCESS;
}

/*
 * Send handshake for a port and lane
 */
AriesErrorType ariesMarginPmaRxReqAckHandshake(AriesRxMarginType* marginDevice,
                                               AriesPseudoPortType port,
                                               int* lane, int laneCount)
{
    AriesErrorType rc;
    int side, quadSlice, quadSliceLane;
    int ln;

    for (ln = 0; ln < laneCount; ln++)
    {
        rc = ariesMarginDeterminePmaSideAndQs(
            marginDevice, port, lane[ln], &side, &quadSlice, &quadSliceLane);
        CHECK_SUCCESS(rc);

        // Assert rxX_req
        // Force 0
        // Offset 5, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN, 5, 1, 1);
        CHECK_SUCCESS(rc);
        // Offset 4, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN, 4, 0, 1);
        CHECK_SUCCESS(rc);

        // Force 1
        // Offset 5, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN, 5, 1, 1);
        CHECK_SUCCESS(rc);
        // Offset 4, width 1
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN, 4, 1, 1);
        CHECK_SUCCESS(rc);
    }

    // Check for ack
    uint8_t dataWord[2];
    uint8_t ack = 0x0;
    int count = 0;
    while (!ack && count <= 0x3fff)
    {
        ack = 1;
        for (ln = 0; ln < laneCount; ln++)
        {
            rc = ariesReadWordPmaLaneMainMicroIndirect(
                marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
                ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_PCS_OUT, dataWord);
            CHECK_SUCCESS(rc);
            if (!(dataWord[0] & 0x1))
            {
                ack = 0;
            }
        }
        count += 1;
    }
    if (!ack)
    {
        ASTERA_ERROR("During Rx req handshake, ACK timed out");
    }
    // Set rxX_req override to 0
    // Offset 5, width 1
    for (ln = 0; ln < laneCount; ln++)
    {
        rc = ariesMarginDeterminePmaSideAndQs(
            marginDevice, port, lane[ln], &side, &quadSlice, &quadSliceLane);
        CHECK_SUCCESS(rc);
        rc = ariesReadWriteWordPmaLaneMainMicroIndirect(
            marginDevice->device->i2cDriver, side, quadSlice, quadSliceLane,
            ARIES_PMA_RAWLANE_DIG_PCS_XF_ATE_OVRD_IN, 5, 0, 1);
        CHECK_SUCCESS(rc);
    }
    return ARIES_SUCCESS;
}

/*
 * Determine the PMA side and quadslice of a port and lane
 */
AriesErrorType ariesMarginDeterminePmaSideAndQs(AriesRxMarginType* marginDevice,
                                                AriesPseudoPortType port,
                                                int lane, int* side,
                                                int* quadSlice,
                                                int* quadSliceLane)
{
    if (marginDevice->partNumber == ARIES_PTX16)
    {
        if (marginDevice->orientation ==
            ARIES_ORIENTATION_NORMAL) // Normal Orientation
        {
            if (port == ARIES_UP_STREAM_PSEUDO_PORT) // USPP
            {
                *side = 1; // Side is a
            }
            else if (port == ARIES_DOWN_STREAM_PSEUDO_PORT) // DSPP
            {
                *side = 0; // Side is b
            }
            else
            {
                ASTERA_ERROR("Invalid port %d", port);
                return ARIES_INVALID_ARGUMENT;
            }
        }
        else if (marginDevice->orientation ==
                 ARIES_ORIENTATION_REVERSED) // Reversed Orientation
        {
            if (port == ARIES_UP_STREAM_PSEUDO_PORT) // USPP
            {
                *side = 0; // Side is b
            }
            else if (port == ARIES_DOWN_STREAM_PSEUDO_PORT) // DSPP
            {
                *side = 1; // Side is a
            }
            else
            {
                ASTERA_ERROR("Invalid port: %d", port);
                return ARIES_INVALID_ARGUMENT;
            }
        }
        else
        {
            ASTERA_ERROR("Invalid Orientation: %d", marginDevice->orientation);
            return ARIES_INVALID_ARGUMENT;
        }
        *quadSlice = lane / 4;
    }
    else if (marginDevice->partNumber == ARIES_PTX08)
    {
        if (marginDevice->orientation ==
            ARIES_ORIENTATION_NORMAL) // Normal Orientation
        {
            if (port == ARIES_UP_STREAM_PSEUDO_PORT) // USPP
            {
                *side = 0; // Side is b
            }
            else if (port == ARIES_DOWN_STREAM_PSEUDO_PORT) // DSPP
            {
                *side = 1; // Side is a
            }
            else
            {
                ASTERA_ERROR("Invalid port %d", port);
                return ARIES_INVALID_ARGUMENT;
            }
        }
        else if (marginDevice->orientation ==
                 ARIES_ORIENTATION_REVERSED) // Reversed Orientation
        {
            if (port == ARIES_UP_STREAM_PSEUDO_PORT) // USPP
            {
                *side = 1; // Side is a
            }
            else if (port == ARIES_DOWN_STREAM_PSEUDO_PORT) // DSPP
            {
                *side = 0; // Side is b
            }
            else
            {
                ASTERA_ERROR("Invalid port %d", port);
                return ARIES_INVALID_ARGUMENT;
            }
        }
        else
        {
            ASTERA_ERROR("Invalid Orientation: %d", marginDevice->orientation);
            return ARIES_INVALID_ARGUMENT;
        }
        *quadSlice = lane / 4 + 1;
    }
    else
    {
        ASTERA_ERROR("Failed to match board Part Number: %d",
                     marginDevice->partNumber);
        return ARIES_INVALID_ARGUMENT;
    }

    *quadSliceLane = lane % 4;

    return ARIES_SUCCESS;
}

AriesErrorType ariesGetLaneRecoveryCount(AriesRxMarginType* marginDevice,
                                         int* lane, int laneCount,
                                         int* recoveryCount)
{
    AriesErrorType rc;
    AriesBifurcationType bifMode;
    uint8_t byteVal[1];
    int linkId;
    int address;
    int ln;

    // Get current bifurcation settings
    rc = ariesGetBifurcationMode(marginDevice->device, &bifMode);
    CHECK_SUCCESS(rc);

    // Use bifurcation settings and lane to determine linkId
    for (ln = 0; ln < laneCount; ln++)
    {
        ariesGetLinkId(bifMode, lane[ln], &linkId);
        address = marginDevice->device->mm_print_info_struct_addr;
        address += ARIES_PRINT_INFO_STRUCT_LNK_RECOV_ENTRIES_PTR_OFFSET +
                   linkId;

        // Read recovery count
        rc = ariesReadByteDataMainMicroIndirect(marginDevice->device->i2cDriver,
                                                address, byteVal);
        CHECK_SUCCESS(rc);
        recoveryCount[ln] = byteVal[0];
    }

    return ARIES_SUCCESS;
}

AriesErrorType ariesClearLaneRecoveryCount(AriesRxMarginType* marginDevice,
                                           int* lane, int laneCount)
{
    AriesErrorType rc;
    AriesBifurcationType bifMode;
    uint8_t byteVal[1];
    int linkId;
    int address;
    int ln;

    // Get current bifurcation settings
    rc = ariesGetBifurcationMode(marginDevice->device, &bifMode);
    CHECK_SUCCESS(rc);

    // Use bifurcation settings and lane to determine linkId
    for (ln = 0; ln < laneCount; ln++)
    {
        ariesGetLinkId(bifMode, lane[ln], &linkId);
        address = marginDevice->device->mm_print_info_struct_addr;
        address += ARIES_PRINT_INFO_STRUCT_LNK_RECOV_ENTRIES_PTR_OFFSET +
                   linkId;

        // Reset recovery count
        byteVal[0] = 0;
        rc = ariesWriteByteDataMainMicroIndirect(
            marginDevice->device->i2cDriver, address, byteVal);
        CHECK_SUCCESS(rc);
    }

    return ARIES_SUCCESS;
}

AriesErrorType ariesGetLaneLastEqNumPresetReq(AriesRxMarginType* marginDevice,
                                              AriesPseudoPortType port,
                                              int* lane, int laneCount,
                                              int* numReq)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    int ln, qs, qs_lane, abs_lane;

    for (ln = 0; ln < laneCount; ln++)
    {
        int direction;
        ariesMarginDeterminePmaSideAndQs(marginDevice, port, lane[ln],
                                         &direction, &qs, &qs_lane);
        abs_lane = qs * 4 + qs_lane;
        int pathID = ((abs_lane / 2) * 2) + direction;
        int pathLane = abs_lane % 2;
        int address = marginDevice->device->pm_gp_ctrl_sts_struct_addr;
        if (pathLane == 0)
        {
            address += ARIES_CTRL_STS_STRUCT_LAST_EQ_NUM_PRESET_REQS_LN0;
        }
        else
        {
            address += ARIES_CTRL_STS_STRUCT_LAST_EQ_NUM_PRESET_REQS_LN1;
        }

        // Read recovery count
        rc = ariesReadBlockDataPathMicroIndirect(
            marginDevice->device->i2cDriver, pathID, address, 1, dataByte);
        CHECK_SUCCESS(rc);
        numReq[ln] = dataByte[0];
    }

    return ARIES_SUCCESS;
}

AriesErrorType ariesGetLaneLastEqPresetReq(AriesRxMarginType* marginDevice,
                                           AriesPseudoPortType port, int* lane,
                                           int laneCount, int* presets)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    int ln, direction, qs, qs_lane, abs_lane, pathID, address;

    for (ln = 0; ln < laneCount; ln++)
    {
        rc = ariesMarginDeterminePmaSideAndQs(marginDevice, port, lane[ln],
                                              &direction, &qs, &qs_lane);
        CHECK_SUCCESS(rc);

        abs_lane = qs * 4 + qs_lane;
        rc = ariesGetLaneLastEqNumPresetReq(marginDevice, port, &lane[ln], 1,
                                            &address);
        CHECK_SUCCESS(rc);
        address -= 1;

        address += marginDevice->device->pm_gp_ctrl_sts_struct_addr;
        if (abs_lane % 2 == 0)
        {
            address += ARIES_CTRL_STS_STRUCT_LAST_EQ_PRESET_REQS_LN0;
        }
        else
        {
            address += ARIES_CTRL_STS_STRUCT_LAST_EQ_PRESET_REQS_LN1;
        }

        // Read recovery count
        pathID = ((abs_lane / 2) * 2) + direction;
        rc = ariesReadByteDataPathMicroIndirect(marginDevice->device->i2cDriver,
                                                pathID, address, dataByte);
        CHECK_SUCCESS(rc);
        presets[ln] = dataByte[0];
    }

    return ARIES_SUCCESS;
}

AriesErrorType ariesGetLaneLastEqPresetReqFOM(AriesRxMarginType* marginDevice,
                                              AriesPseudoPortType port,
                                              int* lane, int laneCount,
                                              int* foms)
{
    AriesErrorType rc;
    uint8_t dataByte[1];
    int ln, direction, qs, qs_lane, abs_lane, pathID, address;

    for (ln = 0; ln < laneCount; ln++)
    {
        rc = ariesMarginDeterminePmaSideAndQs(marginDevice, port, lane[ln],
                                              &direction, &qs, &qs_lane);
        CHECK_SUCCESS(rc);

        abs_lane = qs * 4 + qs_lane;
        rc = ariesGetLaneLastEqNumPresetReq(marginDevice, port, &lane[ln], 1,
                                            &address);
        CHECK_SUCCESS(rc);
        address -= 1;

        address += marginDevice->device->pm_gp_ctrl_sts_struct_addr;
        if (abs_lane % 2 == 0)
        {
            address += ARIES_CTRL_STS_STRUCT_LAST_EQ_FOMS_LN0;
        }
        else
        {
            address += ARIES_CTRL_STS_STRUCT_LAST_EQ_FOMS_LN1;
        }

        // Read recovery count
        pathID = ((abs_lane / 2) * 2) + direction;
        rc = ariesReadByteDataPathMicroIndirect(marginDevice->device->i2cDriver,
                                                pathID, address, dataByte);
        CHECK_SUCCESS(rc);
        foms[ln] = dataByte[0];
    }

    return ARIES_SUCCESS;
}

/*
 * determines eye height using binary search
 */
AriesErrorType ariesCheckEye(AriesRxMarginType* marginDevice,
                             AriesPseudoPortType port, int* lane, int laneCount,
                             double dwell, double*** eyeResults)
{
    AriesErrorType rc;
    int* recovCount = (int*)malloc(sizeof(int) * laneCount);
    int* low = (int*)malloc(sizeof(int) * laneCount);
    int* high = (int*)malloc(sizeof(int) * laneCount);
    int* steps = (int*)malloc(sizeof(int) * laneCount);
    int* direction = (int*)malloc(sizeof(int) * laneCount);
    int* errorCount = (int*)malloc(sizeof(int) * laneCount);
    int* presets = (int*)malloc(sizeof(int) * laneCount);
    int* foms = (int*)malloc(sizeof(int) * laneCount);
    int* presets2 = (int*)malloc(sizeof(int) * laneCount);
    int* foms2 = (int*)malloc(sizeof(int) * laneCount);
    int left_right, down_up, ln;

    if (recovCount == NULL || low == NULL || high == NULL || steps == NULL ||
        direction == NULL || errorCount == NULL)
    {
        free(recovCount);
        free(low);
        free(high);
        free(steps);
        free(direction);
        free(errorCount);
        free(presets);
        free(foms);
        free(presets2);
        free(foms2);
        ASTERA_ERROR("Failed to allocate memory for ariesCheckEye");
        return ARIES_FAILURE;
    }
    rc = ariesClearLaneRecoveryCount(marginDevice, lane, laneCount);
    rc |= ariesGetLaneLastEqPresetReq(marginDevice, port, lane, laneCount,
                                      presets);
    rc |= ariesGetLaneLastEqPresetReqFOM(marginDevice, port, lane, laneCount,
                                         foms);
    if (rc != ARIES_SUCCESS)
    {
        free(recovCount);
        free(low);
        free(high);
        free(steps);
        free(direction);
        free(errorCount);
        free(presets);
        free(foms);
        free(presets2);
        free(foms2);
        return rc;
    }

    for (left_right = 0; left_right < 2; left_right++)
    { // 0:left, 1:right
        rc = ariesMarginGoToNormalSettings(marginDevice, port, lane, laneCount);
        if (rc != ARIES_SUCCESS)
        {
            free(recovCount);
            free(low);
            free(high);
            free(steps);
            free(direction);
            free(errorCount);
            free(presets);
            free(foms);
            free(presets2);
            free(foms2);
            return rc;
        }
        for (ln = 0; ln < laneCount; ln++)
        {
            low[ln] = 0;
            high[ln] = NUMTIMINGSTEPS;
            direction[ln] = left_right;
        }
        while (memcmp(low, high, sizeof(int) * laneCount))
        {
            for (ln = 0; ln < laneCount; ln++)
            {
                steps[ln] = (low[ln] + high[ln] + 1) / 2;
                ASTERA_DEBUG(
                    "Checking timing offset lane %d direction %d steps %d",
                    lane[ln], left_right, steps[ln]);
            }
            rc = ariesMarginClearErrorLog(marginDevice, port, lane, laneCount);
            if (rc != ARIES_SUCCESS)
            {
                free(recovCount);
                free(low);
                free(high);
                free(steps);
                free(direction);
                free(errorCount);
                free(presets);
                free(foms);
                free(presets2);
                free(foms2);
                return rc;
            }

            memset(errorCount, 0, sizeof(int) * laneCount);

            rc = ariesMarginStepMarginToTimingOffset(
                marginDevice, port, lane, direction, steps, laneCount, dwell,
                errorCount);
            if (rc != ARIES_SUCCESS)
            {
                free(recovCount);
                free(low);
                free(high);
                free(steps);
                free(direction);
                free(errorCount);
                free(presets);
                free(foms);
                free(presets2);
                free(foms2);
                return rc;
            }
            for (ln = 0; ln < laneCount; ln++)
            {
                if (low[ln] != high[ln])
                {
                    if (errorCount[ln] > marginDevice->errorCountLimit)
                    {
                        // can't be here anymore. We saw too many errors here
                        high[ln] = steps[ln] - 1;
                        if (high[ln] < low[ln])
                        {
                            low[ln] = high[ln];
                        }
                    }
                    else
                    {
                        low[ln] = steps[ln];
                        if (steps[ln] == NUMTIMINGSTEPS)
                        {
                            ASTERA_DEBUG(
                                "We reached maximum timing offset, exiting margining");
                        }
                    }
                    eyeResults[port][ln][left_right] = low[ln];
                }
            }
        }
    }

    // voltage
    for (down_up = 0; down_up < 2; down_up++)
    { // 0:up, 1:down
        rc = ariesMarginGoToNormalSettings(marginDevice, port, lane, laneCount);
        if (rc != ARIES_SUCCESS)
        {
            free(recovCount);
            free(low);
            free(high);
            free(steps);
            free(direction);
            free(errorCount);
            free(presets);
            free(foms);
            free(presets2);
            free(foms2);
            return rc;
        }
        for (ln = 0; ln < laneCount; ln++)
        {
            low[ln] = 0;
            high[ln] = NUMVOLTAGESTEPS;
            direction[ln] = down_up;
        }
        while (memcmp(low, high, sizeof(int) * laneCount))
        {
            for (ln = 0; ln < laneCount; ln++)
            {
                steps[ln] = (low[ln] + high[ln] + 1) / 2;
                ASTERA_DEBUG(
                    "Checking voltage offset lane %d direction %d steps %d",
                    lane[ln], down_up, steps[ln]);
            }
            rc = ariesMarginClearErrorLog(marginDevice, port, lane, laneCount);
            if (rc != ARIES_SUCCESS)
            {
                free(recovCount);
                free(low);
                free(high);
                free(steps);
                free(direction);
                free(errorCount);
                free(presets);
                free(foms);
                free(presets2);
                free(foms2);
                return rc;
            }
            memset(errorCount, 0, sizeof(int) * laneCount);

            rc = ariesMarginStepMarginToVoltageOffset(
                marginDevice, port, lane, direction, steps, laneCount, dwell,
                errorCount);
            if (rc != ARIES_SUCCESS)
            {
                free(recovCount);
                free(low);
                free(high);
                free(steps);
                free(direction);
                free(errorCount);
                free(presets);
                free(foms);
                free(presets2);
                free(foms2);
                return rc;
            }
            for (ln = 0; ln < laneCount; ln++)
            {
                if (low[ln] != high[ln])
                {
                    if (errorCount[ln] > marginDevice->errorCountLimit)
                    {
                        // can't be here anymore. We saw too many errors here
                        high[ln] = steps[ln] - 1;
                        if (high[ln] < low[ln])
                        {
                            low[ln] = high[ln];
                        }
                    }
                    else
                    {
                        low[ln] = steps[ln];
                        if (steps[ln] == NUMVOLTAGESTEPS)
                        {
                            ASTERA_DEBUG(
                                "We reached maximum timing offset, exiting margining");
                        }
                    }
                    eyeResults[port][ln][down_up + 2] = low[ln];
                }
            }
        }
    }

    rc = ariesMarginGoToNormalSettings(marginDevice, port, lane, laneCount);
    if (rc != ARIES_SUCCESS)
    {
        free(recovCount);
        free(low);
        free(high);
        free(steps);
        free(direction);
        free(errorCount);
        free(presets);
        free(foms);
        free(presets2);
        free(foms2);
        return rc;
    }
    rc = ariesMarginClearErrorLog(marginDevice, port, lane, laneCount);
    if (rc != ARIES_SUCCESS)
    {
        free(recovCount);
        free(low);
        free(high);
        free(steps);
        free(direction);
        free(errorCount);
        free(presets);
        free(foms);
        free(presets2);
        free(foms2);
        return rc;
    }

    // Check recovery count and preset values.
    rc = ariesGetLaneRecoveryCount(marginDevice, lane, laneCount, recovCount);
    rc |= ariesGetLaneLastEqPresetReq(marginDevice, port, lane, laneCount,
                                      presets2);
    rc |= ariesGetLaneLastEqPresetReqFOM(marginDevice, port, lane, laneCount,
                                         foms2);
    if (rc != ARIES_SUCCESS)
    {
        free(recovCount);
        free(low);
        free(high);
        free(steps);
        free(direction);
        free(errorCount);
        free(presets);
        free(foms);
        free(presets2);
        free(foms2);
        return rc;
    }

    for (ln = 0; ln < laneCount; ln++)
    {
        if (recovCount[ln] != 0)
        {
            ASTERA_WARN("%d recoveries occured while margining lane %d",
                        recovCount[ln] / 2, lane[ln]);
        }
    }
    AriesLinkType tmp;
    tmp.device = marginDevice->device;
    int dfe1;
    int dfe2;
    for (ln = 0; ln < laneCount; ln++)
    {
        eyeResults[port][ln][4] = recovCount[ln] / 2;
        eyeResults[port][ln][5] = presets[ln];
        eyeResults[port][ln][6] = foms[ln];
        eyeResults[port][ln][7] = presets2[ln];
        eyeResults[port][ln][8] = foms2[ln];
        ariesGetRxDfeCode(&tmp, port, lane[ln], 1, &dfe1);
        ariesGetRxDfeCode(&tmp, port, lane[ln], 2, &dfe2);
        eyeResults[port][ln][9] = dfe1 * 1.85;
        eyeResults[port][ln][10] = dfe2 * 0.35;
        double eyeWidthLeft = eyeResults[port][ln][0] / (double)NUMTIMINGSTEPS *
                              (double)MAXTIMINGOFFSET;
        double eyeWidthRight = eyeResults[port][ln][1] /
                               (double)NUMTIMINGSTEPS * (double)MAXTIMINGOFFSET;
        double eyeHeightUp = eyeResults[port][ln][2] / (double)NUMVOLTAGESTEPS *
                             (double)MAXVOLTAGEOFFSET;
        double eyeHeightDown = eyeResults[port][ln][3] /
                               (double)NUMVOLTAGESTEPS *
                               (double)MAXVOLTAGEOFFSET;
        ASTERA_INFO("Eye stats for port %d lane %d", port, lane[ln]);
        ASTERA_INFO("  Width = -%.2fUI to %.2fUI", eyeWidthLeft / 100.0,
                    eyeWidthRight / 100.0);
        ASTERA_INFO("  Height = -%.0fmV to %.0fmV", eyeHeightDown * 10.0,
                    eyeHeightUp * 10.0);
    }
    free(recovCount);
    free(low);
    free(high);
    free(steps);
    free(direction);
    free(errorCount);
    free(presets);
    free(foms);
    free(presets2);
    free(foms2);

    // Print results
    return ARIES_SUCCESS;
}

/*
 * logs eye results to a file
 */
AriesErrorType ariesLogEye(AriesRxMarginType* marginDevice,
                           AriesPseudoPortType port, int width,
                           const char* filename, int startLane, double dwell,
                           double*** eyeResults)
{
    AriesErrorType rc;

    if (width == 0)
    {
        if (marginDevice->partNumber == ARIES_PTX16)
        {
            width = 16;
        }
        else if (marginDevice->partNumber == ARIES_PTX08)
        {
            width = 8;
        }
    }

    char filepath[ARIES_PATH_MAX];
    snprintf(filepath, ARIES_PATH_MAX, "%s_%d.csv", filename, port);

    int* lanes = (int*)malloc(sizeof(int) * width);
    if (lanes == NULL)
    {
        free(lanes);
        ASTERA_ERROR("Failed to allocate memory for ariesLogEye");
        return ARIES_FAILURE;
    }
    int ln;
    for (ln = 0; ln < width; ln++)
    {
        lanes[ln] = startLane + ln;
    }
    rc = ariesCheckEye(marginDevice, port, lanes, width, dwell, eyeResults);
    if (rc != ARIES_SUCCESS)
    {
        free(lanes);
        return rc;
    }
    FILE* fp;
    fp = fopen(filepath, "w");
    // Adding header
    fprintf(
        fp,
        "Lane,Timing_neg_UI%%,Timing_pos_UI%%,Timing_tot_UI%%,Voltage_neg_mV,Voltage_pos_mV,Voltage_tot_mV,");
    fprintf(
        fp,
        "Recoveries,Preset_before,FoM_before,Preset_after,FoM_after,DFE1,DFE2\n");
    for (ln = 0; ln < width; ln++)
    {
        double eyeWidthLeft = eyeResults[port][ln][0] / (double)NUMTIMINGSTEPS *
                              (double)MAXTIMINGOFFSET;
        double eyeWidthRight = eyeResults[port][ln][1] /
                               (double)NUMTIMINGSTEPS * (double)MAXTIMINGOFFSET;
        double eyeHeightUp = eyeResults[port][ln][2] / (double)NUMVOLTAGESTEPS *
                             (double)MAXVOLTAGEOFFSET;
        double eyeHeightDown = eyeResults[port][ln][3] /
                               (double)NUMVOLTAGESTEPS *
                               (double)MAXVOLTAGEOFFSET;
        int recoveries = (int)eyeResults[port][ln][4];
        int preset = (int)eyeResults[port][ln][5];
        int fom = (int)eyeResults[port][ln][6];
        int preset2 = (int)eyeResults[port][ln][7];
        int fom2 = (int)eyeResults[port][ln][8];
        int dfe1 = (int)eyeResults[port][ln][9];
        int dfe2 = (int)eyeResults[port][ln][10];
        fprintf(fp, "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", lanes[ln],
                eyeWidthLeft, eyeWidthRight, eyeWidthLeft + eyeWidthRight,
                eyeHeightDown * 10, eyeHeightUp * 10,
                (eyeHeightUp + eyeHeightDown) * 10);

        fprintf(fp, "%d,P%d,%2xh,P%d,%2xh,%dmV,%dmV\n", recoveries, preset, fom,
                preset2, fom2, dfe1, dfe2);
    }

    fclose(fp);
    free(lanes);
    return ARIES_SUCCESS;
}

AriesErrorType ariesLogEyeSerial(AriesRxMarginType* marginDevice,
                                 AriesPseudoPortType port, int width,
                                 const char* filename, int startLane,
                                 double dwell, double*** eyeResults)
{
    AriesErrorType rc;

    if (width == 0)
    {
        if (marginDevice->partNumber == ARIES_PTX16)
        {
            width = 16;
        }
        else if (marginDevice->partNumber == ARIES_PTX08)
        {
            width = 8;
        }
    }

    char filepath[ARIES_PATH_MAX];
    snprintf(filepath, ARIES_PATH_MAX, "%s_%d.csv", filename, port);

    int ln;
    double** startLaneResults = eyeResults[port];
    for (ln = startLane; ln < startLane + width; ln++)
    {
        rc = ariesCheckEye(marginDevice, port, &ln, 1, dwell, eyeResults);
        if (rc != ARIES_SUCCESS)
        {
            eyeResults[port] = startLaneResults;
            return rc;
        }
        ++eyeResults[port];
    }
    eyeResults[port] = startLaneResults;

    FILE* fp;
    fp = fopen(filepath, "w");
    // Adding header
    fprintf(
        fp,
        "Lane,Timing_neg_UI%%,Timing_pos_UI%%,Timing_tot_UI%%,Voltage_neg_mV,Voltage_pos_mV,Voltage_tot_mV,");
    fprintf(fp, "Recoveries,Preset_before,FoM_before,Preset_after,FoM_after\n");
    for (ln = 0; ln < width; ln++)
    {
        double eyeWidthLeft = eyeResults[port][ln][0] / (double)NUMTIMINGSTEPS *
                              (double)MAXTIMINGOFFSET;
        double eyeWidthRight = eyeResults[port][ln][1] /
                               (double)NUMTIMINGSTEPS * (double)MAXTIMINGOFFSET;
        double eyeHeightUp = eyeResults[port][ln][2] / (double)NUMVOLTAGESTEPS *
                             (double)MAXVOLTAGEOFFSET;
        double eyeHeightDown = eyeResults[port][ln][3] /
                               (double)NUMVOLTAGESTEPS *
                               (double)MAXVOLTAGEOFFSET;
        int recoveries = (int)eyeResults[port][ln][4];
        int preset = (int)eyeResults[port][ln][5];
        int fom = (int)eyeResults[port][ln][6];
        int preset2 = (int)eyeResults[port][ln][7];
        int fom2 = (int)eyeResults[port][ln][8];
        fprintf(fp, "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,", startLane + ln,
                eyeWidthLeft, eyeWidthRight, eyeWidthLeft + eyeWidthRight,
                eyeHeightDown * 10, eyeHeightUp * 10,
                (eyeHeightUp + eyeHeightDown) * 10);

        fprintf(fp, "%d,P%d,%2xh,P%d,%2xh\n", recoveries, preset, fom, preset2,
                fom2);
    }

    fclose(fp);
    return ARIES_SUCCESS;
}

/*
 * Create an eyeDiagram of the device on a specific port and lane
 */
AriesErrorType ariesEyeDiagram(AriesRxMarginType* marginDevice,
                               AriesPseudoPortType port, int lane, int rate,
                               double dwell, int**** eyeResults)
{
    AriesErrorType rc;

    int* timingOffsets = (int*)malloc(sizeof(int) * NUMTIMINGSTEPS * 2 + 1);
    int voltageOffsets[] = {70,  60,  50,  40,  30,  20,  10, 0,
                            -10, -20, -30, -40, -50, -60, -70};
    int timingOffset;
    if (rate == 3)
    {
        for (timingOffset = 0; timingOffset < NUMTIMINGSTEPS + 1;
             timingOffset++)
        {
            timingOffsets[timingOffset] = (-NUMTIMINGSTEPS * 2) +
                                          (4 * timingOffset);
        }
    }
    else if (rate >= 4 && rate < 6)
    {
        for (timingOffset = 0; timingOffset < NUMTIMINGSTEPS + 1;
             timingOffset++)
        {
            timingOffsets[timingOffset] = (-NUMTIMINGSTEPS) +
                                          (2 * timingOffset);
        }
    }
    else
    {
        ASTERA_ERROR("%d is not a valid rate", rate);
        free(timingOffsets);
        return ARIES_INVALID_ARGUMENT;
    }

    char filepath[ARIES_PATH_MAX];
    snprintf(filepath, ARIES_PATH_MAX, "eye_diagram_%d_lane%d.csv", port, lane);

    // Clear error count
    rc = ariesClearLaneRecoveryCount(marginDevice, &lane, 1);
    CHECK_SUCCESS(rc);

    FILE* fp;
    fp = fopen(filepath, "w");
    int voltageOffset;
    for (voltageOffset = 0; voltageOffset < 15; voltageOffset++)
    {
        fprintf(fp, "%3d,,", voltageOffsets[voltageOffset]);
        for (timingOffset = 0; timingOffset < NUMTIMINGSTEPS + 1;
             timingOffset++)
        {
            rc = ariesMarginClearErrorLog(marginDevice, port, &lane, 1);
            CHECK_SUCCESS(rc);

            int errorCount = 0;
            int timeDirection = 0;
            if (timingOffsets[timingOffset] < 0)
            {
                timeDirection = 0; // left
            }
            else
            {
                timeDirection = 1; // right
            }
            int timeSteps = abs(timingOffsets[timingOffset]);

            int voltageDirection = 0;
            if (voltageOffsets[voltageOffset] < 0)
            {
                voltageDirection = 0;
            }
            else
            {
                voltageDirection = 1;
            }
            int voltageSteps = abs(voltageOffsets[voltageOffset]);

            ASTERA_DEBUG("Checking offset x=%d,y=%d",
                         timingOffsets[timingOffset],
                         voltageOffsets[voltageOffset]);
            rc = ariesMarginPmaTimingVoltageOffset(
                marginDevice, port, &lane, &timeDirection, &timeSteps,
                &voltageDirection, &voltageSteps, 1, dwell, &errorCount);
            CHECK_SUCCESS(rc);
            eyeResults[port][lane][timingOffset][voltageOffset] = errorCount;
            fprintf(fp, "%3d,", errorCount);
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
    fprintf(fp, "   ,,");
    for (timingOffset = 0; timingOffset < NUMTIMINGSTEPS + 1; timingOffset++)
    {
        fprintf(fp, "%3d,", timingOffsets[timingOffset]);
    }
    fprintf(fp, "\n");
    fclose(fp);

    int recovCount;
    rc = ariesGetLaneRecoveryCount(marginDevice, &lane, 1, &recovCount);
    CHECK_SUCCESS(rc);

    // Check recovery count again. if they are different then a recovery
    // happened
    if (recovCount != 0)
    {
        ASTERA_WARN("%d recoveries occured while margining this lane",
                    recovCount / 2);
    }

    return ARIES_SUCCESS;
}

#ifdef __cplusplus
}
#endif
