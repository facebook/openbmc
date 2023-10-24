/**
 *  @brief      Declares global headers for TPMFactoryUpd.
 *  @details
 *  @file       TPMFactoryUpdStruct.h
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
#include "FirmwareImage.h"
#include "FirmwareUpdate.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Default configuration file name
#define CONFIG_FILE L"TPMFactoryUpd.cfg"

/**
 *  @brief      Enum for generic structure types used by Infineon TPM2 tools
 *  @details
 */
typedef enum td_ENUM_STRUCT_TYPES
{
    /// Structure type 'none'
    STRUCT_TYPE_NULL,
    /// Structure tdTpmInfo
    STRUCT_TYPE_TpmInfo,
    /// Structure tdTpmUpdate
    STRUCT_TYPE_TpmUpdate,
    /// Structure tdTpm12ClearOwnership
    STRUCT_TYPE_Tpm12ClearOwnership,
    /// Structure tdSetMode
    STRUCT_TYPE_SetMode
} ENUM_STRUCT_TYPES;

/**
 *  @brief      Enum for TpmUpdate structure sub type
 *  @details
 */
typedef enum td_ENUM_STRUCT_SUBTYPES
{
    /// Structure sub type none
    STRUCT_SUBTYPE_NULL,
    /// Structure sub type IsUpdatable
    STRUCT_SUBTYPE_IS_UPDATABLE,
    /// Structure sub type Prepare
    STRUCT_SUBTYPE_PREPARE,
    /// Structure sub type Update
    STRUCT_SUBTYPE_UPDATE
} ENUM_STRUCT_SUBTYPES;

/**
 *  @brief      Enumeration of update types
 *  @details
 */
typedef enum td_ENUM_UPDATE_TYPES
{
    /// No type
    UPDATE_TYPE_NONE = 0,
    /// Update type for TPM1.2 using Deferred Physical Presence
    UPDATE_TYPE_TPM12_DEFERREDPP = 1,
    /// Update type for TPM1.2 taking TPM Ownership
    UPDATE_TYPE_TPM12_TAKEOWNERSHIP = 2,
    /// Update type for TPM2.0 using empty platformAuth
    UPDATE_TYPE_TPM20_EMPTYPLATFORMAUTH = 3,
    /// Update type for using settings from configuration file
    UPDATE_TYPE_CONFIG_FILE = 4,
    /// Update type for TPM1.2 with owner authorization provided
    UPDATE_TYPE_TPM12_OWNERAUTH = 5
} ENUM_UPDATE_TYPES;

/**
 *  @brief      Enumeration of setmode parameter types
 *  @details
 */
typedef enum td_ENUM_SETMODE_TYPES
{
    /// No type
    SETMODE_TYPE_NONE = 0,
    /// Set mode type for firmware update
    SETMODE_TYPE_TPM20_FW_UPDATE = 1,
    /// Set mode type for firmware recovery
    SETMODE_TYPE_TPM20_FW_RECOVERY = 2,
    /// Set mode type for TPM operational
    SETMODE_TYPE_TPM20_OPERATIONAL = 3,
} ENUM_SETMODE_TYPES;

/**
 *  @brief      Generic enum for tristate results like "yes", "no" and "not applicable".
 *  @details
 */
typedef enum td_ENUM_GENERIC_TRISTATE
{
    /// Current state is not applicable or unknown
    GENERIC_TRISTATE_STATE_NA,
    /// Current state is false, no or negative
    GENERIC_TRISTATE_STATE_NO,
    /// Current state is true, yes or positive
    GENERIC_TRISTATE_STATE_YES
} ENUM_GENERIC_TRISTATE;

/**
 *  @brief      Header for generic structure used by Infineon tools
 *  @details
 */
typedef struct tdIfxToolHeader
{
    /// Type of structure according to ENUM_STRUCT_TYPES
    ENUM_STRUCT_TYPES   unType;
    /// Size of complete structure
    unsigned int        unSize;
    /// TPM return code
    unsigned int        unReturnCode;
} IfxToolHeader;

#define IfxTpm12ClearOwnership IfxToolHeader

/**
 *  @brief      Structure for TPM Info utilizing generic structure IfxToolHeader
 *  @details
 */
typedef struct tdIfxInfo
{
    /// Shared content with IfxToolHeader structure
    IfxToolHeader           hdr;
    /// TPM wszVersion Name
    wchar_t                 wszVersionName[MAX_NAME];
    /// TPM state
    TPM_STATE               sTpmState;
    /// Number of remaining updates
    unsigned int            unRemainingUpdates;
    /// Number of remaining updates on same version
    unsigned int            unRemainingUpdatesSelf;
} IfxInfo;

/**
 *  @brief      Structure for TPM Update utilizing generic structure IfxToolHeader
 *  @details
 */
typedef struct tdIfxUpdate
{
    /// Shared content with IfxInfo structure
    IfxInfo                         info;
    /// SubType of the structure
    ENUM_STRUCT_SUBTYPES            unSubType;
    /// Whether the new firmware image is valid
    BOOL                            fValid;
    /// Info about new firmware image
    BITFIELD_NEW_TPM_FIRMWARE_INFO  bfNewTpmFirmwareInfo;
    /// Error details why TPM is not updatable with the firmware
    unsigned int                    unErrorDetails;
    /// New firmware image version
    wchar_t                         wszNewFirmwareVersion[MAX_NAME];
    /// New firmware image TPM family
    BYTE                            bTargetFamily;
    /// FirmwareImage size
    unsigned int                    unFirmwareImageSize;
    /// FirmwareImage pointer. The allocated memory must be freed after usage.
    BYTE*                           rgbFirmwareImage;
    /// TPM2.0 Policy session handle
    TSS_TPMI_SH_AUTH_SESSION        hPolicySession;
    /// New firmware valid state
    ENUM_GENERIC_TRISTATE           unNewFirmwareValid;
    /// Used firmware image
    wchar_t                         wszUsedFirmwareImage[MAX_NAME];
} IfxUpdate;

/**
 *  @brief      Structure for TPM SetMode utilizing generic structure IfxToolHeader
 *  @details
 */
typedef struct tdIfxSetMode
{
    /// Shared content with IfxToolHeader structure
    IfxToolHeader                   hdr;
    /// TPM state
    TPM_STATE                       sTpmState;
    /// Algorithm ID the manifest hash is based on
    unsigned int                    algorithmId;
    /// SHA512 manifest (64 Byte), SHA384 manifest (48 Byte)
    BYTE                            manifestHash[TSS_SHA512_DIGEST_SIZE];
} IfxSetMode;

#ifdef __cplusplus
}
#endif
