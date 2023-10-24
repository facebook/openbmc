/**
 *  @brief      Declares property keys for TPMFactoryUpd
 *  @details
 *  @file       TPMFactoryUpd\PropertyDefines.h
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "StdInclude.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Define for tool path configuration setting property
#define PROPERTY_TOOL_PATH                  L"ToolPath"
/// Define for command line option count
#define PROPERTY_CMDLINE_COUNT              L"CmdLineCount"
/// Define for log command line parameter
#define PROPERTY_LOGGING                    L"Log"
/// Define for logging level configuration setting property
#define PROPERTY_LOGGING_LEVEL              L"LoggingLevel"
/// Define for logging path configuration setting property
#define PROPERTY_LOGGING_PATH               L"LoggingPath"
/// Define for logging max file size configuration setting property
#define PROPERTY_LOGGING_MAXSIZE            L"LoggingMaxSize"
/// Define for console mode configuration setting property
#define PROPERTY_CONSOLE_MODE               L"ConsoleMode"
/// Define for locality configuration setting property
#define PROPERTY_LOCALITY                   L"Locality"
/// Define for locality request.
/// If TRUE, locality is requested for use once before first TPM command and released after last TPM response.
/// If FALSE, locality is requested before each TPM command and released after each TPM response.
#define PROPERTY_KEEP_LOCALITY_ACTIVE       L"KeepLocalityActive"
/// Define for help property
#define PROPERTY_HELP                       L"Help"
/// Define for alternative help command line parameter
#define PROPERTY_HELP_ALT                   L"?"
/// Define for info property
#define PROPERTY_INFO                       L"Info"
/// Define for update property
#define PROPERTY_UPDATE                     L"Update"
/// Define for update type property
#define PROPERTY_UPDATE_TYPE                L"UpdateType"
/// Define for firmware path configuration setting property
#define PROPERTY_FIRMWARE_PATH              L"Firmware"
/// Define for configuration file path configuration setting property
#define PROPERTY_CONFIG_FILE_PATH           L"Config"
// Define for shutdown type configuration setting property
#define PROPERTY_SHUTDOWN_TYPE              L"ShutdownType"
/// Define for Logging size check flag
#define PROPERTY_LOGGING_CHECK_SIZE         L"LoggingCheckFileSize"
/// Define for TPM12-ClearOwnership property
#define PROPERTY_TPM12_CLEAROWNERSHIP       L"TPM12-ClearOwnership"
// PROPERTY_CALL_SHUTDOWN_ON_EXIT, PROPERTY_TPM_DEVICE_ACCESS_MODE and PROPERTY_TPM_DEVICE_ACCESS_PATH are defined in Globals.h
/// Define for access-mode property
#define PROPERTY_ACCESS_MODE                L"AccessMode"
/// Define for configuration file update type12
#define PROPERTY_CONFIG_FILE_UPDATE_TYPE12  L"UpdateType12"
/// Define for configuration file update type20
#define PROPERTY_CONFIG_FILE_UPDATE_TYPE20  L"UpdateType20"
/// Define for Target Firmware Version
#define PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_GENERIC L"TargetFirmwareVersion"
/// Define for Target Firmware Version for major part of actual firmware version
#define PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_FWxx L"TargetFirmwareVersionFW"
/// Define for Target Firmware Version for TPM type (SLB, SLI, etc.)
#define PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_Sxx L"TargetFirmwareVersionS"
/// Define for Target Firmware Version for SLB 9670 TPM
#define PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB9670 L"TargetFirmwareVersionSLB9670"
/// Define for Target Firmware Version for SLI 9670 TPM
#define PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLI9670 L"TargetFirmwareVersionSLI9670"
/// Define for Target Firmware Version for SLB 9655 and SLB 9656 TPM
#define PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB965x L"TargetFirmwareVersionSLB965x"
/// Define for Target Firmware Version for SLB 9660 and SLB 9665 TPM
#define PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB966x L"TargetFirmwareVersionSLB966x"
/// Define for Target Firmware Version for SLB 9615 TPM
#define PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB9615 L"TargetFirmwareVersionSLB9615"
/// Define for Target Firmware Version for SLB 9645 TPM
#define PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB9645 L"TargetFirmwareVersionSLB9645"
/// Define for Firmware Folder Path
#define PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH    L"FirmwareFolderPath"
/// Define for Owner Authorization
#define PROPERTY_OWNERAUTH_FILE_PATH            L"OwnerAuthFile"
/// Define for setmode property
#define PROPERTY_SETMODE                        L"SetMode"
/// Define for setmode type property
#define PROPERTY_SETMODE_TYPE                   L"SetModeType"
/// Define for firmware update abandon mode property
#define PROPERTY_ABANDON_UPDATE_MODE            L"AbandonUpdateMode"
/// Define for force property
#define PROPERTY_FORCE                          L"Force"
/// Definitions for PROPERTY_ABANDON_UPDATE_MODE (bit field)
/// Define for no action if firmware update fails
#define ABANDON_UPDATE_NO_ACTION                (UINT32)0x00000000
/// Define for calling abandon update if FieldUpgradeManifestVendor command fails
#define ABANDON_UPDATE_IF_MANIFEST_CALL_FAIL    (UINT32)0x00000001

#ifdef __cplusplus
}
#endif
