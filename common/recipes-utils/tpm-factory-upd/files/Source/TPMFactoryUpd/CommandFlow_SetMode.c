/**
 *  @brief      Implements the command flow to switch between specific TPM2.0 operation modes.
 *  @details    This module validates if all preconditions are met to switch to the requested operation mode and executes the task.
 *  @file       CommandFlow_SetMode.c
 *
 *  Copyright 2020 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CommandFlow_SetMode.h"
#include "CommandFlow_TpmInfo.h"
#include "CommandFlow_TpmUpdate.h"
#include "DeviceManagement.h"
#include "FirmwareUpdate.h"
#include "UiUtility.h"
#include "TPM2_Marshal.h"
#include "TPM2_FieldUpgradeStartVendor.h"
#include "TPM2_FieldUpgradeAbandonVendor.h"
#include "Resource.h"

/**
 *  @brief      Processes a sequence of TPM set mode related commands.
 *  @details    This function sets the TPM in firmware update mode, recovery mode or operational mode.
 *
 *  @param      PpTpmSetMode            Pointer to an initialized IfxSetMode structure to be filled in.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PpTpmSetMode was invalid.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_SetMode_Execute(
    _Inout_ IfxSetMode* PpTpmSetMode)
{

    unsigned int unReturnValue = RC_E_FAIL;
    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check parameters
        if (NULL == PpTpmSetMode ||
                PpTpmSetMode->hdr.unType != STRUCT_TYPE_SetMode ||
                PpTpmSetMode->hdr.unSize != sizeof(IfxSetMode))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected. IfxSetMode structure is not in the correct state.");
            break;
        }

        // Get setmode type
        ENUM_SETMODE_TYPES unSetModeType = SETMODE_TYPE_NONE;
        if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_SETMODE_TYPE, (unsigned int*)&unSetModeType))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"Unexpected error calling PropertyStorage_GetUIntegerValueByKey.");
            break;
        }

        // Check for TPM2.0 based firmware update
        if (1 != PpTpmSetMode->sTpmState.attribs.tpmHasFULoader20) {
            ERROR_STORE(unReturnValue, L"Unexpected error calling CommandFlow_SetMode_Execute. TPM attributes are not in the correct state.");
            break;
        }

        // Call TPM2_FieldUpgradeStartVendor or TPM2_FieldUpgradeAbandonVendor?
        if (SETMODE_TYPE_TPM20_FW_UPDATE == unSetModeType || SETMODE_TYPE_TPM20_FW_RECOVERY == unSetModeType)
        {
            // Policy parameter block data
            TSS_TPM2B_MAX_BUFFER sData;

            // Authorization session area
            TSS_AuthorizationCommandData sAuthSessionData;
            TSS_AcknowledgmentResponseData sAckAuthSessionData;

            Platform_MemorySet(&sAuthSessionData, 0, sizeof(sAuthSessionData));
            Platform_MemorySet(&sAckAuthSessionData, 0, sizeof(sAckAuthSessionData));
            Platform_MemorySet(&sData, 0, sizeof(sData));

            TSS_TPMI_SH_AUTH_SESSION hPolicySession = 0;
            unReturnValue = FirmwareUpdate_PrepareTPM20Policy(&hPolicySession);
            if (RC_SUCCESS != unReturnValue)
                break;

            TSS_TPMT_HA sHash;
            TSS_INT32 nMarshal = sizeof(sData.buffer);
            TSS_BYTE* pMarshal = sData.buffer;

            // Get hash algorithm
            sHash.hashAlg = (TSS_TPMI_ALG_HASH)PpTpmSetMode->algorithmId;

            // Copy digest value from buffer
            unReturnValue = Platform_MemoryCopy(&sHash.digest, sizeof(sHash.digest), PpTpmSetMode->manifestHash, sizeof(PpTpmSetMode->manifestHash));
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected error calling Platform_MemoryCopy.");
                break;
            }

            // Marshal the hash as TPMT_HA structure into TSS_TPM2B_MAX_BUFFER.
            unReturnValue = TSS_TPMT_HA_Marshal(&sHash, &pMarshal, &nMarshal);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected error calling TSS_TPMT_HA_Marshal.");
                break;
            }

            // Update the size of the TSS_TPM2B_MAX_BUFFER.
            sData.size = (TSS_UINT16)(sizeof(sData.buffer) - nMarshal);

            sAuthSessionData.authHandle = hPolicySession;
            sAuthSessionData.sessionAttributes.continueSession = 1;
            unsigned short usStartSize = 0;

            // Call TPM2_FieldUpgradeStartVendor command with manifest hash
            TSS_UINT8 bSubCommand = TPM20_FieldUpgradeStartManifestHash;
            unReturnValue = TSS_TPM2_FieldUpgradeStartVendor(TSS_TPM_RH_PLATFORM, &sAuthSessionData, bSubCommand, &sData, &usStartSize, &sAckAuthSessionData);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected error calling TSS_TPM2_FieldUpgradeStartVendor.");
                break;
            }
        }
        else if (SETMODE_TYPE_TPM20_OPERATIONAL == unSetModeType)
        {
            unReturnValue = FirmwareUpdate_AbandonUpdate();
            if (RC_SUCCESS != unReturnValue)
                break;
        }
        else
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"Unsupported ENUM_SETMODE_TYPES given.");
            break;
        }

        // Try to reconnect to TPM after mode switch
        for (unsigned int unRetryCounter = 1; unRetryCounter <= TPM20_FU_RETRY_COUNT; unRetryCounter++)
        {
            // Wait for TPM to switch to boot loader mode.
            Platform_Sleep(TPM20_FU_WAIT_TIME);

            // Disconnect from TPM.
            unReturnValue = DeviceManagement_Disconnect();
            if (RC_SUCCESS != unReturnValue) {
                LOGGING_WRITE_LEVEL2_FMT(L"DeviceManagement_Disconnect failed (0x%.8X). Continue to reconnect.", unReturnValue);
            }

            // Reconnect to the TPM.
            unReturnValue = DeviceManagement_Connect();
            if (RC_SUCCESS != unReturnValue) {
                LOGGING_WRITE_LEVEL2_FMT(L"DeviceManagement_Connect failed (0x%.8X). Continue to reconnect.", unReturnValue);
                continue;
            }
        }

        // Check if reconnect succeeded
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"No connection to the TPM can be established after mode switch.");
            break;
        }

        // Set success
        PpTpmSetMode->hdr.unReturnCode = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);
    return unReturnValue;
}

/**
 *  @brief      Processes the given command line parameter for the -setmode option.
 *  @details    This function sets all relevant properties like the algorithm ID and manifest hash.
 *
 *  @param      PpTpmSetMode            Pointer to an initialized IfxSetMode structure to be filled in.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PpTpmSetMode was invalid.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_SetMode_ValidateParameter(
    _Inout_ IfxSetMode* PpTpmSetMode)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    wchar_t wszValue[PROPERTY_STORAGE_MAX_VALUE];
    unsigned int unValueSize = RG_LEN(wszValue);

    do
    {
        // Check parameters
        if (NULL == PpTpmSetMode ||
                PpTpmSetMode->hdr.unType != STRUCT_TYPE_SetMode ||
                PpTpmSetMode->hdr.unSize != sizeof(IfxSetMode))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected. IfxSetMode structure is not in the correct state.");
            break;
        }

        // Get setmode type
        ENUM_SETMODE_TYPES unSetModeType = SETMODE_TYPE_NONE;
        if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_SETMODE_TYPE, (unsigned int*)&unSetModeType))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"Unexpected error calling PropertyStorage_GetUIntegerValueByKey.");
            break;
        }

        // Get TPM information
        IfxInfo sTpmInfo;
        Platform_MemorySet(&sTpmInfo, 0, sizeof(sTpmInfo));
        sTpmInfo.hdr.unType = STRUCT_TYPE_TpmInfo;
        sTpmInfo.hdr.unSize = sizeof(sTpmInfo);
        unReturnValue = CommandFlow_TpmInfo_Execute(&sTpmInfo);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"CommandFlow_TpmInfo_Execute failed.");
            break;
        }

        // Set TPM information for later usage
        PpTpmSetMode->sTpmState = sTpmInfo.sTpmState;

        // Check for TPM2.0 based firmware update
        if (0 == PpTpmSetMode->sTpmState.attribs.tpmHasFULoader20) {
            unReturnValue = RC_E_TPM_NOT_SUPPORTED_FEATURE;
            ERROR_STORE(unReturnValue, L"No TPM2.0 based firmware update loader.");
            break;
        }

        // Check if a restart is required
        if (PpTpmSetMode->sTpmState.attribs.tpm20restartRequired)
        {
            unReturnValue = RC_E_RESTART_REQUIRED;
            ERROR_STORE(unReturnValue, L"The TPM2.0 requires a restart to perform this action.");
            break;
        }

        // Perform checks based on mode type
        if (SETMODE_TYPE_TPM20_FW_UPDATE == unSetModeType)
        {
            // Check operation mode
            if (!PpTpmSetMode->sTpmState.attribs.tpmInOperationalMode)
            {
                unReturnValue = RC_E_TPM_WRONG_STATE;
                ERROR_STORE(unReturnValue, L"The TPM2.0 is not in operational mode.");
                break;
            }

            // Get path to firmware file
            if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_FIRMWARE_PATH, wszValue, &unValueSize))
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE(unReturnValue, L"Unexpected error calling PropertyStorage_GetValueByKey.");
                break;
            }

            // Set structure members
            IfxUpdate sTpmUpdate;
            Platform_MemorySet(&sTpmUpdate, 0, sizeof(sTpmUpdate));
            sTpmUpdate.info.hdr.unSize = sizeof(IfxUpdate);
            sTpmUpdate.info.hdr.unType = STRUCT_TYPE_TpmUpdate;
            sTpmUpdate.info.sTpmState = sTpmInfo.sTpmState;
            sTpmUpdate.info.unRemainingUpdates = sTpmInfo.unRemainingUpdates;
            sTpmUpdate.info.unRemainingUpdatesSelf = sTpmInfo.unRemainingUpdatesSelf;

            // Set update type property that CommandFlow_TpmUpdate_IsFirmwareUpdatable can be used
            if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, UPDATE_TYPE_TPM20_EMPTYPLATFORMAUTH))
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE(unReturnValue, L"PropertyStorage_SetUIntegerValueByKey failed.");
                break;
            }

            // Check image
            unReturnValue = CommandFlow_TpmUpdate_IsFirmwareUpdatable(&sTpmUpdate);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"CommandFlow_TpmUpdate_IsFirmwareUpdatable failed.");
                break;
            }

            // Is firmware file valid?
            if (sTpmUpdate.unNewFirmwareValid != GENERIC_TRISTATE_STATE_YES)
            {
                unReturnValue = RC_E_WRONG_FW_IMAGE;
                ERROR_STORE(unReturnValue, L"Firmware image is not valid.");
                break;
            }

            // Get firmware image information
            IfxFirmwareImage firmwareImage = { { 0 } };
            int nBufferSize = sTpmUpdate.unFirmwareImageSize;
            BYTE* pbBuffer = sTpmUpdate.rgbFirmwareImage;
            unReturnValue = FirmwareImage_Unmarshal(&firmwareImage, &pbBuffer, &nBufferSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Failed to unmarshal the firmware image.");
                if (RC_E_NEWER_TOOL_REQUIRED != unReturnValue)
                    unReturnValue = RC_E_CORRUPT_FW_IMAGE;
                break;
            }

            // Select correct manifest
            unReturnValue = FirmwareUpdate_SelectManifest(&firmwareImage);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"FirmwareUpdate_SelectManifest returned an error");
                break;
            }

            // Use SHA512 as hash algorithm
            PpTpmSetMode->algorithmId = TSS_TPM_ALG_SHA512;

            // Calculate SHA-512 hash over the manifest.
            unReturnValue = Crypt_SHA512(firmwareImage.rgbPolicyParameterBlock, firmwareImage.usPolicyParameterBlockSize, PpTpmSetMode->manifestHash);
            if (RC_SUCCESS != unReturnValue)
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE(unReturnValue, L"Hashing of firmware image failed.");
                break;
            }
        }
        else if (SETMODE_TYPE_TPM20_FW_RECOVERY == unSetModeType)
        {
            if (0 == PpTpmSetMode->sTpmState.attribs.tpmSupportsFwRecovery)
            {
                unReturnValue = RC_E_TPM_NOT_SUPPORTED_FEATURE;
                ERROR_STORE(unReturnValue, L"TPM2.0 recovery mode not supported.");
                break;
            }

            // Check operation mode
            if (!PpTpmSetMode->sTpmState.attribs.tpmInOperationalMode)
            {
                unReturnValue = RC_E_TPM_WRONG_STATE;
                ERROR_STORE(unReturnValue, L"The TPM2.0 is not in operational mode.");
                break;
            }

            // Use SHA512 as hash algorithm
            PpTpmSetMode->algorithmId = TSS_TPM_ALG_SHA512;

            // Use 64 Byte 0x3C as manifest hash to enter recovery mode
            Platform_MemorySet(PpTpmSetMode->manifestHash, 0x3C, TSS_SHA512_DIGEST_SIZE);
            unReturnValue = RC_SUCCESS;
        }
        else if (SETMODE_TYPE_TPM20_OPERATIONAL == unSetModeType)
        {
            // Check if firmware is valid in non-operational mode without pending restart
            if (!(PpTpmSetMode->sTpmState.attribs.tpmFirmwareIsValid &&
                    !PpTpmSetMode->sTpmState.attribs.tpmInOperationalMode &&
                    !PpTpmSetMode->sTpmState.attribs.tpm20restartRequired))
            {
                unReturnValue = RC_E_TPM_WRONG_STATE;
                ERROR_STORE(unReturnValue, L"The TPM2.0 is not in an state that can be abandoned.");
                break;
            }

            unReturnValue = RC_SUCCESS;
        }
        else
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"Unsupported ENUM_SETMODE_TYPES given.");
            break;
        }

        // Validation succeeded
        PpTpmSetMode->hdr.unReturnCode = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);
    return unReturnValue;
}
