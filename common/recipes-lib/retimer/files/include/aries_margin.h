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
 * @file aries_margin.h
 * @brief Definition of receiver margining types for the SDK.
 */

#ifndef ASTERA_ARIES_SDK_MARGIN_H_
#define ASTERA_ARIES_SDK_MARGIN_H_

#include "aries_globals.h"
#include "aries_error.h"
#include "aries_api_types.h"
#include "astera_log.h"
#include "aries_api.h"
#include "aries_i2c.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants describing margining actions
#define NUMTIMINGSTEPS 14
#define MAXTIMINGOFFSET 40
#define NUMVOLTAGESTEPS 108
#define MAXVOLTAGEOFFSET 20
#define SAMPLINGRATEVOLTAGE 31
#define SAMPLINGRATETIMING 31
#define MAXLANES 15
#define VOLTAGESUPPORTED true
#define INDUPDOWNVOLTAGE true
#define INDLEFTRIGHTTIMING true
#define SAMPLEREPORTINGMETHOD true
#define INDERRORSAMPLER true

/**
 * @brief Margin Command for No Command
 *
 * Sending this out of band doesn't do anything
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginNoCommand(AriesRxMarginType* marginDevice);

/**
 * @brief Margin Command to access Retimer register
 *
 * Register access is not implemented yet
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @return AriesErrorType - Aries error code
 */
AriesErrorType
    ariesMarginAccessRetimerRegister(AriesRxMarginType* marginDevice);

/**
 * @brief Margin Command to print and store margin control capabilities
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[out] capabilities  Array to store margin device capabilities
 * @return AriesErrorType - Aries error code
 */
AriesErrorType
    ariesMarginReportMarginControlCapabilities(AriesRxMarginType* marginDevice,
                                               int* capabilities);

/**
 * @brief Margin Command to report the number of voltage steps
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[out] numVoltageSteps  variable to store number of voltage steps in
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginReportNumVoltageSteps(AriesRxMarginType* marginDevice,
                                                int* numVoltageSteps);

/**
 * @brief Margin Command to report the number of timing steps
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[out] numTimingSteps  variable to store number of timing steps in
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginReportNumTimingSteps(AriesRxMarginType* marginDevice,
                                               int* numTimingSteps);

/**
 * @brief Margin Command to report the max timing offset
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[out] maxTimingOffset  variable to store max timing offset in
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginReportMaxTimingOffset(AriesRxMarginType* marginDevice,
                                                int* maxTimingOffset);

/**
 * @brief Margin Command to report the max voltage offset
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[out] maxVoltageOffset  variable to store max voltage offset in
 * @return AriesErrorType - Aries error code
 */
AriesErrorType
    ariesMarginReportMaxVoltageOffset(AriesRxMarginType* marginDevice,
                                      int* maxVoltageOffset);

/**
 * @brief Margin Command to report sampling rate voltage
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[out] samplingRateVoltage  variable to store samplingRateVoltage in
 * @return AriesErrorType - Aries error code
 */
AriesErrorType
    ariesMarginReportSamplingRateVoltage(AriesRxMarginType* marginDevice,
                                         int* samplingRateVoltage);

/**
 * @brief Margin Command to report sampling rate timing
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[out] samplingRateTiming  variable to store samplingRateTiming in;
 * @return AriesErrorType - Aries error code
 */
AriesErrorType
    ariesMarginReportSamplingRateTiming(AriesRxMarginType* marginDevice,
                                        int* samplingRateTiming);

/**
 * @brief Margin Command to report sample count
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginReportSampleCount(AriesRxMarginType* marginDevice);

/**
 * @brief Margin Command to report the max lanes
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[out] maxLanes  variable to store maxLanes in
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginReportMaxLanes(AriesRxMarginType* marginDevice,
                                         int* maxLanes);

/**
 * @brief Margin Command to set the ErrorCountLimit
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  limit  New ErrorCountLimit
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginSetErrorCountLimit(AriesRxMarginType* marginDevice,
                                             int limit);

/**
 * @brief Margin Command to go to normal settings
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @return  AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginGoToNormalSettings(AriesRxMarginType* marginDevice,
                                             AriesPseudoPortType port,
                                             int* lane, int laneCount);

/**
 * @brief Margin Command to clears error count of a given port and lane
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginClearErrorLog(AriesRxMarginType* marginDevice,
                                        AriesPseudoPortType port, int* lane,
                                        int laneCount);

/**
 * @brief Margin Command to Sets the margin sampler to a specific time offset
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @param[in]  direction  Direction to move the sampler (0: left, 1: right)
 * @param[in]  steps  Number of steps to move the sampler
 * @param[in]  dwell  Time between starting sampling and reading sampling
 * @param[out] eCount  Error count of sampling at this location
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginStepMarginToTimingOffset(
    AriesRxMarginType* marginDevice, AriesPseudoPortType port, int* lane,
    int* direction, int* steps, int laneCount, double dwell, int* eCount);

/**
 * @brief Margin Command to Sets the margin sampler to a specific voltage offset
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @param[in]  direction  Direction to move the sampler (0: up, 1: down)
 * @param[in]  steps  Number of steps to move the sampler
 * @param[in]  dwell  Time between starting sampling and reading sampling
 * @param[out] eCount  Error count of sampling at this location
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginStepMarginToVoltageOffset(
    AriesRxMarginType* marginDevice, AriesPseudoPortType port, int* lane,
    int* direction, int* steps, int laneCount, double dwell, int* eCount);

/**
 * @brief Margin Command for vendor defined
 *
 * Vendor defined function is not implemented yet
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginVendorDefined(AriesRxMarginType* marginDevice);

/**
 * @brief Stops the margining
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginPmaRxMarginStop(AriesRxMarginType* marginDevice,
                                          AriesPseudoPortType port, int* lane,
                                          int laneCount);

/**
 * @brief Sets the PMA registers to margin a specified time value
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @param[in]  direction  Direction to move (0:left, 1:right)
 * @param[in]  steps  Number of steps to move
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginPmaRxMarginTiming(AriesRxMarginType* marginDevice,
                                            AriesPseudoPortType port, int* lane,
                                            int* direction, int* steps,
                                            int laneCount);

/**
 * @brief Sets the PMA registers to margin a specified voltage value
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @param[in]  direction  Direction to move (0:up, 1:down)
 * @param[in]  steps  Number of steps to move
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginPmaRxMarginVoltage(AriesRxMarginType* marginDevice,
                                             AriesPseudoPortType port,
                                             int* lane, int* direction,
                                             int* steps, int laneCount);

/**
 * @brief Sets the PMA registers to margin a specified voltage and time value
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @param[in]  timeDirection  Direction to move on time axis (0: left, 1: right)
 * @param[in]  timeSteps  Number of steps to move on time axis
 * @param[in]  voltageDirection  Direction to move on voltage axis (0: up, 1:
 * down)
 * @param[in]  voltageSteps  Number of steps to move on the voltage axis
 * @param[in]  dwell  Time to wait between starting margining and reading error
 * count
 * @param[out] eCount  Error count of sampling at this location
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginPmaTimingVoltageOffset(
    AriesRxMarginType* marginDevice, AriesPseudoPortType port, int* lane,
    int* timeDirection, int* timeSteps, int* voltageDirection,
    int* voltageSteps, int laneCount, double dwell, int* eCount);

/**
 * @brief Gets the number of errors that have occurred on a specific port and
 * lane
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @return AriesErrorType - Aries Error Code
 */
AriesErrorType ariesMarginPmaRxMarginGetECount(AriesRxMarginType* marginDevice,
                                               AriesPseudoPortType port,
                                               int* lane, int laneCount);

/**
 * @brief Perform Rx Request/Ack Handshake
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginPmaRxReqAckHandshake(AriesRxMarginType* marginDevice,
                                               AriesPseudoPortType port,
                                               int* lane, int laneCount);

/**
 * @brief Determines the pma side and quadslice of a port and lane
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @param[out] side  Side the port and lane are in
 * @param[out] quadSlice  Quad slice the port and lane are in
 * @param[out] quadSliceLane  Quad slice lane the port and lane are in
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesMarginDeterminePmaSideAndQs(AriesRxMarginType* marginDevice,
                                                AriesPseudoPortType port,
                                                int lane, int* side,
                                                int* quadSlice,
                                                int* quadSliceLane);

/**
 * @brief Gets the recovery count of a lane
 *
 * @param[in] marginDevice Struct containing Margin Device information
 * @param[in] lane Lane we want the recovery count of
 * @param[out] recoveryCount Variable to store the recovery count in
 */
AriesErrorType ariesGetLaneRecoveryCount(AriesRxMarginType* marginDevice,
                                         int* lane, int laneCount,
                                         int* recoveryCount);

/**
 * @brief Determines eye stats for a given port and lane using binary search
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @param[in]  dwell  Time to wait before checking error count in a lane
 * @param[out] eyeResults  Array to store the results from the tests
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesCheckEye(AriesRxMarginType* marginDevice,
                             AriesPseudoPortType port, int* lane, int laneCount,
                             double dwell, double*** eyeResults);

/**
 * @brief Calculates the eye for each lane on the port on the device and outputs
 * it to a file
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  width  Width of the device (x16 or x8)
 * @param[in]  filename  Name of the file for the data to be stored in
 * @param[in]  startLane  Lane to start at on the Retimer
 * @param[in]  dwell  Time to wait before checking error count
 * @param[out] eyeResults  Array to store the results from the tests
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesLogEye(AriesRxMarginType* marginDevice,
                           AriesPseudoPortType port, int width,
                           const char* filename, int startLane, double dwell,
                           double*** eyeResults);

/**
 * @brief Determines eye stats for a given port and lane
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer (USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @param[in]  dwell  Time to wait before checking error count
 * @param[out] eyeResults  Array to store the results from the tests
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesSweepEye(AriesRxMarginType* marginDevice,
                             AriesPseudoPortType port, int lane, double dwell,
                             double**** eyeResults);

/**
 * @brief Creates a full 2D eye diagram for a given port and lane
 *
 * @param[in]  marginDevice  Struct containing Margin Device information
 * @param[in]  port  Port to margin on the Retimer(USPP or DSPP)
 * @param[in]  lane  Physical device lane on the Retimer
 * @param[in]  rate  data rate of the Retimer (Gen3: 3, Gen4: 4, Gen5: 5)
 * @param[in]  dwell  Time to wait before checking error count
 * @param[out] eyeResults  Array to store the results from the tests
 * @return AriesErrorType - Aries error code
 */
AriesErrorType ariesEyeDiagram(AriesRxMarginType* marginDevice,
                               AriesPseudoPortType port, int lane, int rate,
                               double dwell, int**** eyeResults);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_ARIES_SDK_MARGIN_H_ */
