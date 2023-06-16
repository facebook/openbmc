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
 * @file aries_link.h
 * @brief Definition of Link level functions for the SDK.
 */

#ifndef ASTERA_ARIES_SDK_LINK_H_
#define ASTERA_ARIES_SDK_LINK_H_

#include "aries_globals.h"
#include "aries_error.h"
#include "aries_api_types.h"
#include "astera_log.h"
#include "aries_api.h"
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Set the PCIe Protocol Reset.
 *
 * PCIe Protocol (fundamental) Reset may be triggered from the PERST_N
 * hardware pin or from a register. This function sets the register to assert
 * PCIe reset (1) or de-assert PCIe reset (0). User must wait 1 ms between
 * an assertion and de-assertion.
 *
 * @param[in]  link  Link object (containing link id)
 * @param[in]  reset      Reset assert (1) or de-assert (0)
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesSetPcieReset(AriesLinkType* link, uint8_t reset);

/**
 * @brief Perform an all-in-one health check on a given Link.
 *
 * Check critical parameters like junction temperature, Link LTSSM state,
 * and per-lane eye height/width against certain alert thresholds. Update
 * link.linkOkay member (bool).
 *
 * @param[in]  link    Pointer to Aries Link struct object
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesCheckLinkHealth(AriesLinkType* link);

// Calculate the current width of the link
AriesErrorType ariesLinkGetCurrentWidth(AriesLinkType* link, int* currentWidth);

/**
 * @brief Get link recovery counter value
 *
 * @param[in]  link   Link struct created by user
 * @param[out] recoveryCount   Recovery count returned
 * @return     AriesErrorType - Aries error code
 *
 */
AriesErrorType ariesGetLinkRecoveryCount(AriesLinkType* link,
                                         int* recoveryCount);

/**
 * @brief Clear link recovery counter value
 *
 * @param[in]  link   Link struct created by user
 * @return     AriesErrorType - Aries error code
 *
 */
AriesErrorType ariesClearLinkRecoveryCount(AriesLinkType* link);

/**
 * @brief Get the current detailed Link state, including electrical parameters.
 *
 * Read the current Link state and return the parameters for the current links
 * Returns a negative error code, else zero on success.
 *
 * @param[in,out]  link   Pointer to Aries Link struct object
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLinkState(AriesLinkType* link);

/**
 * @brief Get the current detailed Link state, including electrical parameters.
 *
 * Read the current Link state and return the parameters for the current links
 * Returns a negative error code, else zero on success.
 *
 * @param[in,out]  link   Pointer to Aries Link struct object
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesGetLinkStateDetailed(AriesLinkType* link);

/**
 * @brief Initialize LTSSM logger.
 *
 * Configure the LTSSM logger for one-batch or continuous mode and set the
 * print classes (and per-Link enables for main microcontroller log).
 * Returns a negative error code, else zero on success.
 *
 * @param[in]  link          Pointer to Aries Link struct object
 * @param[in]  oneBatchModeEn  Enable one-batch mode (1) or continuous mode (0)
 * @param[in]  verbosity     Logger verbosity control
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesLTSSMLoggerInit(AriesLinkType* link, uint8_t oneBatchModeEn,
                                    AriesLTSSMVerbosityType verbosity);

/**
 * @brief Enable or disable LTSSM logger.
 *
 * @param[in]  link     Pointer to Aries Link struct object
 * @param[in]  printEn  Enable (1) or disable (0) printing for this Link
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesLTSSMLoggerPrintEn(AriesLinkType* link, uint8_t printEn);

/**
 * @brief Read an entry from the LTSSM logger.
 *
 * The LTSSM log entry starting at offset is returned, and offset
 * is updated to point to the next entry. Returns a negative error code,
 * including ARIES_LTSSM_INVALID_ENTRY if the the end of the log is reached,
 * else zero on success.
 *
 * @param[in]      link    Pointer to Aries Link struct object
 * @param[in]      logType     The specific log to read from
 * @param[in,out]  offset  Pointer to the log offset value
 * @param[out]     entry   Pointer to Aries LTSSM Logger Entry struct returned
 *                         by this function
 * @return     AriesErrorType - Aries error code
 */
AriesErrorType ariesLTSSMLoggerReadEntry(AriesLinkType* link,
                                         AriesLTSSMLoggerEnumType logType,
                                         int* offset,
                                         AriesLTSSMEntryType* entry);

// Dump all the debug info from the retimer
AriesErrorType ariesLinkDumpDebugInfo(AriesLinkType* link, const char*fru, int id);

// Function to iterate over logger and print entry
AriesErrorType ariesLinkPrintMicroLogs(AriesLinkType* link,
                                       const char* basepath,
                                       const char* filename);

// Capture the detailed link state and print it to file
AriesErrorType ariesLinkPrintDetailedState(AriesLinkType* link,
                                           const char* basepath,
                                           const char* filename);

// Print the micro logger entries
AriesErrorType ariesPrintLog(AriesLinkType* link, AriesLTSSMLoggerEnumType log,
                             FILE* fp);

#ifdef __cplusplus
}
#endif

#endif /* ASTERA_ARIES_SDK_LINK_H_ */
