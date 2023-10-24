/**
 *  @brief      Declares the Error Codes for all projects
 *  @details    This file contains definitions for all error and return codes.
 *  @file       ErrorCodes.h
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

/// Return code for a successful execution
#define RC_SUCCESS                              0x00000000U // General success return code

/// Error code mask for TPM errors
#define RC_TPM_MASK                             0xE0280000U

/// Error code mask for tool errors
#define RC_APP_MASK                             0xE0295000U

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Tool errors
/// General error code (0xE0295001)
#define RC_E_FAIL                               RC_APP_MASK + 0x001
#define MSG_RC_E_FAIL                           L"An unexpected error occurred."
/// Bad command line error code (0xE0295002)
#define RC_E_BAD_COMMANDLINE                    RC_APP_MASK + 0x002
#define MSG_RC_E_BAD_COMMANDLINE                L"Invalid command line parameter(s)."
/// Chip version or vendor error (0xE0295003)
#define RC_E_NO_IFXTPM20                        RC_APP_MASK + 0x003
#define MSG_RC_E_NO_IFXTPM20                    L"This tool is compatible with Infineon TPM2.0.\nThe TPM chip detected is not a supported device."
/// File does not use correct encoding (0xE0295004)
#define RC_E_WRONG_ENCODING                     RC_APP_MASK + 0x004
#define MSG_RC_E_WRONG_ENCODING                 L"Check if file exists and is using correct encoding."
/// Interrupted Firmware Update mode (0xE0295005)
#define RC_E_INTERRUPTED_FU                     RC_APP_MASK + 0x005
#define MSG_RC_E_INTERRUPTED_FU                 L"The TPM is in invalid firmware state. Use TPM Firmware Update Tools to recover the TPM."
/// Not supported feature when using a TPM driver (0xE0295006)
#define RC_E_NOT_SUPPORTED_FEATURE              RC_APP_MASK + 0x006
#define MSG_RC_E_NOT_SUPPORTED_FEATURE          L"This feature is not supported in the currently configured tool mode."
/// TPM driver already in use (0xE0295007)
#define RC_E_DEVICE_ALREADY_IN_USE              RC_APP_MASK + 0x007
#define MSG_RC_E_DEVICE_ALREADY_IN_USE          L"The TPM device is in use by another process."
/// No access to the TPM driver (0xE0295008)
#define RC_E_TPM_ACCESS_DENIED                  RC_APP_MASK + 0x008
#define MSG_RC_E_TPM_ACCESS_DENIED              L"The application does not have the appropriate rights to access the TPM device."
/// Invalid setting in configuration file (0xE0295009)
#define RC_E_INVALID_SETTING                    RC_APP_MASK + 0x009
#define MSG_RC_E_INVALID_SETTING                L"A setting in the configuration file is missing or invalid."
/// Not supported feature in case of a TPM2.0 (0xE029500A)
#define RC_E_TPM_NOT_SUPPORTED_FEATURE          RC_APP_MASK + 0x00A
#define MSG_RC_E_TPM_NOT_SUPPORTED_FEATURE      L"The selected command line option cannot be used with the TPM family or device."
/// Not supported state in case of a TPM2.0 (0xE029500B)
#define RC_E_TPM_WRONG_STATE                    RC_APP_MASK + 0x00B
#define MSG_RC_E_TPM_WRONG_STATE                L"The selected command line option cannot be used in the current TPM state."

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
/// General internal error (0xE0295100)
#define RC_E_INTERNAL                           RC_APP_MASK + 0x100
#define MSG_RC_E_INTERNAL                       L"An internal error occurred."
// Following error codes will be mapped to RC_E_INTERNAL
/// Error code for not initialized modules (0xE0295101)
#define RC_E_NOT_INITIALIZED                    RC_E_INTERNAL + 0x01
/// Error code for not connected module e.g. TpmIO (0xE0295102)
#define RC_E_NOT_CONNECTED                      RC_E_INTERNAL + 0x02
/// Error code for already connected module e.g. TpmIO (0xE0295103)
#define RC_E_ALREADY_CONNECTED                  RC_E_INTERNAL + 0x03
/// Error code for Bad Parameters (0xE0295104)
#define RC_E_BAD_PARAMETER                      RC_E_INTERNAL + 0x04
/// Buffer too small (0xE0295105)
#define RC_E_BUFFER_TOO_SMALL                   RC_E_INTERNAL + 0x05
/// End of file (0xE0295106)
#define RC_E_END_OF_FILE                        RC_E_INTERNAL + 0x06
/// File or Directory not found (0xE0295107)
#define RC_E_FILE_NOT_FOUND                     RC_E_INTERNAL + 0x07
/// Access denied (e.g. file) (0xE0295108)
#define RC_E_ACCESS_DENIED                      RC_E_INTERNAL + 0x08
/// File already exists (0xE0295109)
#define RC_E_FILE_EXISTS                        RC_E_INTERNAL + 0x09
/// End of String (0xE029510A)
#define RC_E_END_OF_STRING                      RC_E_INTERNAL + 0x0A
/// String not found (0xE029510B)
#define RC_E_NOT_FOUND                          RC_E_INTERNAL + 0x0B
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

/// General TPM device/connection error code (0xE0295200)
#define RC_E_NO_TPM                             RC_APP_MASK + 0x200
#define MSG_RC_E_NO_TPM                         L"No connection to the TPM or TPM not found."
// Following error codes will be mapped to RC_E_NO_TPM
/// Component not found. Used by TIS. (0xE0295201)
#define RC_E_COMPONENT_NOT_FOUND                RC_E_NO_TPM + 0x01
/// Not an active locality. Used by TIS. (0xE0295202)
#define RC_E_LOCALITY_NOT_ACTIVE                RC_E_NO_TPM + 0x02
/// Not a supported locality. Used by TIS. (0xE0295203)
#define RC_E_LOCALITY_NOT_SUPPORTED             RC_E_NO_TPM + 0x03
/// No data available. Used by TIS. (0xE0295204)
#define RC_E_TPM_NO_DATA_AVAILABLE              RC_E_NO_TPM + 0x04
/// Buffer insufficient. Used by TIS. (0xE0295205)
#define RC_E_INSUFFICIENT_BUFFER                RC_E_NO_TPM + 0x05
/// Error receiving data. Used by TIS. (0xE0295206)
#define RC_E_TPM_RECEIVE_DATA                   RC_E_NO_TPM + 0x06
/// Error transmit data. Used by TIS. (0xE0295207)
#define RC_E_TPM_TRANSMIT_DATA                  RC_E_NO_TPM + 0x07
/// TPM not ready. Used by TIS. (0xE0295208)
#define RC_E_NOT_READY                          RC_E_NO_TPM + 0x08
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// TPM errors
/// General TPM error code (0xE0295300)
#define RC_E_TPM_GENERAL                        RC_APP_MASK + 0x300
#define MSG_RC_E_TPM_GENERAL                    L"A TPM error occurred."
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Test failures
/// Error code mask for tool errors (0xE0295400)
#define RC_TEST_MASK                            RC_APP_MASK + 0x400
/// Flag check error (0xE0295401)
#define RC_E_FLAGCHECK                          RC_TEST_MASK + 0x01
#define MSG_RC_E_FLAGCHECK                      L"Flag check failed."
/// Public EK Compare failed (0xE0295402)
#define RC_E_EKCHECK                            RC_TEST_MASK + 0x02
#define MSG_RC_E_EKCHECK                        L"Public EK comparison failed."
/// Hash Compare failed (0xE0295403)
#define RC_E_HASHCHECK                          RC_TEST_MASK + 0x03
#define MSG_RC_E_HASHCHECK                      L"Hash comparison failed."
/// Register test failed (0xE0295404)
#define RC_E_REGISTERTEST                       RC_TEST_MASK + 0x04
#define MSG_RC_E_REGISTERTEST                   L"Register test failed."
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// TPM Firmware Update failures
/// Error code mask for TPM Firmware Update codes (0xE0295500)
#define RC_E_TPM_FIRMWARE_UPDATE                RC_APP_MASK + 0x500
#define MSG_RC_E_TPM_FIRMWARE_UPDATE            L"The firmware update process returned an unexpected value."
/// Error code for invalid firmware option (0xE0295503)
#define RC_E_INVALID_FW_OPTION                  RC_E_TPM_FIRMWARE_UPDATE + 0x03
#define MSG_RC_E_INVALID_FW_OPTION              L"An invalid value was passed in the <firmware> command line option."
/// Error code for a wrong firmware image (0xE0295504)
#define RC_E_WRONG_FW_IMAGE                     RC_E_TPM_FIRMWARE_UPDATE + 0x04
#define MSG_RC_E_WRONG_FW_IMAGE                 L"The firmware image cannot be used to update the TPM."
/// Error code for invalid log file option (0xE0295505)
#define RC_E_INVALID_LOG_OPTION                 RC_E_TPM_FIRMWARE_UPDATE + 0x05
#define MSG_RC_E_INVALID_LOG_OPTION             L"An invalid value was passed in the <log> command line option."
/// Error code for no Infineon TPM (0xE0295506)
#define RC_E_NO_IFX_TPM                         RC_E_TPM_FIRMWARE_UPDATE + 0x06
#define MSG_RC_E_NO_IFX_TPM                     L"The TPM is not an Infineon TPM."
/// Error code for not empty platform authentication (0xE0295507)
#define RC_E_PLATFORM_AUTH_NOT_EMPTY            RC_E_TPM_FIRMWARE_UPDATE + 0x07
#define MSG_RC_E_PLATFORM_AUTH_NOT_EMPTY        L"TPM2.0: PlatformAuth is not the Empty Buffer. The firmware cannot be updated."
/// Error code for disabled platform hierarchy (0xE0295508)
#define RC_E_PLATFORM_HIERARCHY_DISABLED        RC_E_TPM_FIRMWARE_UPDATE + 0x08
#define MSG_RC_E_PLATFORM_HIERARCHY_DISABLED    L"TPM2.0: The platform hierarchy is disabled. The firmware cannot be updated."
/// Error code for blocked firmware update (0xE0295509)
#define RC_E_FW_UPDATE_BLOCKED                  RC_E_TPM_FIRMWARE_UPDATE + 0x09
#define MSG_RC_E_FW_UPDATE_BLOCKED              L"The TPM does not allow further updates because the update counter is zero."
/// Error code for failed update (0xE029550A)
#define RC_E_FIRMWARE_UPDATE_FAILED             RC_E_TPM_FIRMWARE_UPDATE + 0x0A
#define MSG_RC_E_FIRMWARE_UPDATE_FAILED         L"The firmware update started but failed."
/// Error code for owned TPM1.2 (0xE029550B)
#define RC_E_TPM12_OWNED                        RC_E_TPM_FIRMWARE_UPDATE + 0x0B
#define MSG_RC_E_TPM12_OWNED                    L"TPM1.2: The TPM has an owner. The firmware cannot be updated."
/// Error code for locked PP TPM1.2 (0xE029550C)
#define RC_E_TPM12_PP_LOCKED                    RC_E_TPM_FIRMWARE_UPDATE + 0x0C
#define MSG_RC_E_TPM12_PP_LOCKED                L"TPM1.2: Physical Presence is locked. The firmware cannot be updated."
/// Error code for invalid update option (0xE029550E)
#define RC_E_INVALID_UPDATE_OPTION              RC_E_TPM_FIRMWARE_UPDATE + 0x0E
#define MSG_RC_E_INVALID_UPDATE_OPTION          L"The selected <update> command line option cannot be used with the TPM family."
/// Error code for restart required (0xE029550F)
#define RC_E_RESTART_REQUIRED                   RC_E_TPM_FIRMWARE_UPDATE + 0x0F
#define MSG_RC_E_RESTART_REQUIRED               L"The system must be restarted before the TPM can be updated or can function properly again."
/// Error code for missing Deferred PP TPM1.2 (0xE0295510)
#define RC_E_TPM12_DEFERREDPP_REQUIRED          RC_E_TPM_FIRMWARE_UPDATE + 0x10
#define MSG_RC_E_TPM12_DEFERREDPP_REQUIRED      L"TPM1.2: Deferred Physical Presence is not set. The firmware cannot be updated."
/// Error code for disabled / deactivated TPM1.2 (0xE0295511)
#define RC_E_TPM12_DISABLED_DEACTIVATED         RC_E_TPM_FIRMWARE_UPDATE + 0x11
#define MSG_RC_E_TPM12_DISABLED_DEACTIVATED     L"TPM1.2: The TPM is disabled or deactivated. The firmware cannot be updated."
/// Error code for TPM1.2 dictionary attack mode (0xE0295512)
#define RC_E_TPM12_DA_ACTIVE                    RC_E_TPM_FIRMWARE_UPDATE + 0x12
#define MSG_RC_E_TPM12_DA_ACTIVE                L"TPM1.2: The TPM is locked out due to dictionary attack."
/// Error code for newer firmware image (0xE0295513)
#define RC_E_NEWER_TOOL_REQUIRED                RC_E_TPM_FIRMWARE_UPDATE + 0x13
#define MSG_RC_E_NEWER_TOOL_REQUIRED            L"The firmware image provided requires a newer version of this tool."
/// Error code for unsupported Infineon TPMs (0xE0295514)
#define RC_E_UNSUPPORTED_CHIP                   RC_E_TPM_FIRMWARE_UPDATE + 0x14
#define MSG_RC_E_UNSUPPORTED_CHIP               L"The Infineon TPM chip detected is not supported by this tool."
/// Error code for a corrupt firmware image (0xE0295515)
#define RC_E_CORRUPT_FW_IMAGE                   RC_E_TPM_FIRMWARE_UPDATE + 0x15
#define MSG_RC_E_CORRUPT_FW_IMAGE               L"The firmware image is corrupt."
/// Error code for not matching decrypt keys between TPM and firmware image (0xE0295516)
#define RC_E_WRONG_DECRYPT_KEYS                 RC_E_TPM_FIRMWARE_UPDATE + 0x16
#define MSG_RC_E_WRONG_DECRYPT_KEYS             L"The firmware image cannot be used to update this TPM (decrypt key mismatch)."
/// Error code for an invalid configuration option (0xE0295517)
#define RC_E_INVALID_CONFIG_OPTION              RC_E_TPM_FIRMWARE_UPDATE + 0x17
#define MSG_RC_E_INVALID_CONFIG_OPTION          L"An invalid value was passed in the <config> command line option."
/// Error code if an update firmware file could not be found (0xE0295518)
#define RC_E_FIRMWARE_UPDATE_NOT_FOUND          RC_E_TPM_FIRMWARE_UPDATE + 0x18
#define MSG_RC_E_FIRMWARE_UPDATE_NOT_FOUND      L"Could not find a firmware image to update the configured target firmware version."

// Error code Ox19 is for tool internal use

/// Error code if the rundata file to resume interrupted firmware update is not available (0xE029551A)
#define RC_E_RESUME_RUNDATA_NOT_FOUND           RC_E_TPM_FIRMWARE_UPDATE + 0x1A
#define MSG_RC_E_RESUME_RUNDATA_NOT_FOUND       L"Cannot resume interrupted firmware update or recover existing firmware with option '-update config-file' because file 'TPMFactoryUpd_RunData.txt' is missing."
/// Error code if a newer revision of the firmware is required (0xE029551B)
#define RC_E_NEWER_FW_IMAGE_REQUIRED            RC_E_TPM_FIRMWARE_UPDATE + 0x1B
#define MSG_RC_E_NEWER_FW_IMAGE_REQUIRED        L"A newer revision of the firmware image is required."

// Range from 0x1C to 0x1F can be used for new error codes.

// Error codes 0x20 and 0x21 is for tool internal use

/// Error code for an invalid owner authorization (0xE0295522)
#define RC_E_TPM12_INVALID_OWNERAUTH            RC_E_TPM_FIRMWARE_UPDATE + 0x22
#define MSG_RC_E_TPM12_INVALID_OWNERAUTH        L"TPM1.2: The owner authorization is invalid."
/// Error code for an unowned TPM1.2 (0xE0295523)
#define RC_E_TPM12_NO_OWNER                     RC_E_TPM_FIRMWARE_UPDATE + 0x23
#define MSG_RC_E_TPM12_NO_OWNER                 L"TPM1.2: The TPM has no owner."

// Error codes 0x24 to 0x27 are for tool internal use

/// Error code for an invalid access-mode configuration (0xE0295528)
#define RC_E_INVALID_ACCESS_MODE                RC_E_TPM_FIRMWARE_UPDATE + 0x28
#define MSG_RC_E_INVALID_ACCESS_MODE            L"An invalid value was passed in the <access-mode> command line option."
/// Error code for TPM2.0 in Failure Mode (0xE0295529)
#define RC_E_TPM20_FAILURE_MODE                 RC_E_TPM_FIRMWARE_UPDATE + 0x29
#define MSG_RC_E_TPM20_FAILURE_MODE             L"The TPM2.0 is in failure mode. TPM firmware update is not possible. Restart the system and try again."
/// Error code for TPM in self-test failed mode (0xE029552A)
#define RC_E_TPM12_FAILED_SELFTEST              RC_E_TPM_FIRMWARE_UPDATE + 0x2A
#define MSG_RC_E_TPM12_FAILED_SELFTEST          L"The TPM1.2 failed the self-test. TPM firmware update is not possible. Restart the system and try again."
/// Error code for invalid ownerauth parameter
#define RC_E_INVALID_OWNERAUTH_OPTION           RC_E_TPM_FIRMWARE_UPDATE + 0x2B
#define MSG_RC_E_INVALID_OWNERAUTH_OPTION       L"An invalid value was passed in the <ownerauth> command line option."

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// TPM Firmware Update error codes for internal mapping
/// Internal error code for a configuration option triggered update that detects the
/// state "already up to date" (0xE0295528)
#define RC_E_ALREADY_UP_TO_DATE                 RC_E_TPM_FIRMWARE_UPDATE + 0x19
/// Internal error code for invalid policy session (0xE0295520)
#define RC_E_TPM20_INVALID_POLICY_SESSION       RC_E_TPM_FIRMWARE_UPDATE + 0x20
/// Internal error code for missing owner authorization (0xE0295521)
#define RC_E_TPM12_MISSING_OWNERAUTH            RC_E_TPM_FIRMWARE_UPDATE + 0x21
/// Internal error code for TPM not in Boot Loader Mode (0xE0295524)
#define RC_E_TPM_NO_BOOT_LOADER_MODE            RC_E_TPM_FIRMWARE_UPDATE + 0x24
// 0x22 and 0x23 were moved from internal to external mapping
/// Internal error code for not loaded policy session (0xE0295525)
#define RC_E_TPM20_POLICY_SESSION_NOT_LOADED    RC_E_TPM_FIRMWARE_UPDATE + 0x25
/// Internal error code for policy handle out of range (0xE0295526)
#define RC_E_TPM20_POLICY_HANDLE_OUT_OF_RANGE   RC_E_TPM_FIRMWARE_UPDATE + 0x26
/// Internal error code for failed signature verification (0xE0295527)
#define RC_E_VERIFY_SIGNATURE                   RC_E_TPM_FIRMWARE_UPDATE + 0x27
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
