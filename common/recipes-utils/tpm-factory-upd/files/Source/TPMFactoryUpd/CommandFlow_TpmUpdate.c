/**
 *  @brief      Implements the command flow to update the TPM firmware.
 *  @details    This module processes the firmware update. Afterwards the result is returned to the calling module.
 *  @file       CommandFlow_TpmUpdate.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CommandFlow_TpmUpdate.h"
#include "CommandFlow_Tpm12ClearOwnership.h"
#include "CommandFlow_TpmInfo.h"
#include "FirmwareImage.h"
#include "FirmwareUpdate.h"
#include "Response.h"
#include "Crypt.h"
#include "Config.h"
#include "ConfigSettings.h"
#include "TPMFactoryUpdStruct.h"
#include "Resource.h"
#include "FileIO.h"
#include "Platform.h"
#include "UiUtility.h"
#include "Utility.h"

#include <TPM2_FlushContext.h>
#include <TPM2_StartAuthSession.h>
#include <TPM2_FieldUpgradeTypes.h>
#include <TPM_OIAP.h>
#include <TPM_ReadPubEK.h>
#include <TPM_SetCapability.h>
#include <TPM_TakeOwnership.h>
#include <TSC_PhysicalPresence.h>
#include <TPM_FieldUpgradeInfoRequest.h>

// Storage Root Key well known authentication value
#define SRK_WELL_KNOWN_AUTH { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }

// TPM family defines
#define TPM12_FAMILY_STRING L"TPM12"
#define TPM20_FAMILY_STRING L"TPM20"

// TPM firmware image file name patterns
#define TPM_FIRMWARE_FILE_NAME_PATTERN L"%ls_%ls_to_%ls_%ls.BIN"
#define TPM_FIRMWARE_FILE_NAME_PATTERN2 L"TPM20_%ls_%ls.BIN"
#define TPM_FIRMWARE_FILE_NAME_EXT L".BIN"
#define TPM_FIRMWARE_FILE_NAME_EXT_LEN 4

#define TPM_FACTORY_UPD_RUNDATA_FILE L"TPMFactoryUpd_RunData.txt"

// Flag to remember that firmware update is done through configuration file option
BOOL s_fUpdateThroughConfigFile = FALSE;

static const BYTE s_rgbOAEPPad[] = { 'T', 'C', 'P', 'A' };
const wchar_t CwszErrorMsgFormatAddKeyUIntegerValuePair[] = L"PropertyStorage_AddKeyUIntegerValuePair failed while updating the property '%ls'.";

/**
 *  @brief      Callback function to save the used firmware image path to TPM_FACTORY_UPD_RUNDATA_FILE (once an update has been started successfully)
 *  @details    The function is called by FirmwareUpdate_UpdateImage() to create the TPM_FACTORY_UPD_RUNDATA_FILE.
 */
void
IFXAPI
CommandFlow_TpmUpdate_UpdateStartedCallback()
{
    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);
    // Save the firmware image path in TPM_FACTORY_UPD_RUNDATA_FILE if firmware update was initiated through "-update config-file" option.
    // If the firmware update should fail unexpectedly and leave TPM in invalid firmware mode, the user can restart the system and run TPMFactoryUpd
    // to resume the firmware update with the saved firmware image path.
    // Continue on any errors if for example the saving of the file fails, etc.
    if (s_fUpdateThroughConfigFile)
    {
        void* pFile = NULL;
        if (0 == FileIO_Open(TPM_FACTORY_UPD_RUNDATA_FILE, &pFile, FILE_WRITE) && NULL != pFile)
        {
            wchar_t wszFirmwareImagePath[MAX_PATH];
            unsigned int unFirmwareImagePathSize = RG_LEN(wszFirmwareImagePath);
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszFirmwareImagePath, RG_LEN(wszFirmwareImagePath)));
            if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_FIRMWARE_PATH, wszFirmwareImagePath, &unFirmwareImagePathSize))
            {
                ERROR_STORE_FMT(RC_E_FAIL, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_FIRMWARE_PATH);
            }
            else
            {
                IGNORE_RETURN_VALUE(FileIO_WriteString(pFile, wszFirmwareImagePath));
            }
            IGNORE_RETURN_VALUE(FileIO_Close(&pFile));
        }
    }
    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);
    return;
}

/**
 *  @brief      Checks if the given firmware package can be used to update the TPM.
 *  @details    The function calls FirmwareUpdate_CheckImage() to check whether the TPM can be updated with the given firmware package.
 *
 *  @param      PpTpmUpdate             Pointer to a IfxUpdate structure where only firmware image is accessed.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function.
 *  @retval     RC_E_CORRUPT_FW_IMAGE       In case of a corrupt firmware image.
 *  @retval     RC_E_FW_UPDATE_BLOCKED      The TPM does not allow further updates .
 *  @retval     RC_E_NEWER_TOOL_REQUIRED    The firmware image provided requires a newer version of this tool.
 *  @retval     RC_E_WRONG_FW_IMAGE         In case of a wrong firmware image.
 *  @retval     RC_E_WRONG_DECRYPT_KEYS     In case the TPM2.0 does not have decrypt keys matching to the firmware image.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_IsTpmUpdatableWithFirmware(
    _Inout_ IfxUpdate* PpTpmUpdate)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check input parameters
        if (NULL == PpTpmUpdate ||
                STRUCT_TYPE_TpmUpdate != PpTpmUpdate->info.hdr.unType ||
                sizeof(IfxUpdate) != PpTpmUpdate->info.hdr.unSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
            break;
        }

        // Set new firmware valid state to "NO"
        PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NO;

        // Call CheckImage
        unReturnValue = FirmwareUpdate_CheckImage(PpTpmUpdate->rgbFirmwareImage, PpTpmUpdate->unFirmwareImageSize, &PpTpmUpdate->fValid, &PpTpmUpdate->bfNewTpmFirmwareInfo, &PpTpmUpdate->unErrorDetails);
        if (RC_SUCCESS != unReturnValue)
            break;

        if (!PpTpmUpdate->fValid)
        {
            switch (PpTpmUpdate->unErrorDetails)
            {
                case RC_E_CORRUPT_FW_IMAGE:
                    unReturnValue = PpTpmUpdate->unErrorDetails;
                    ERROR_STORE_FMT(unReturnValue, L"The provided firmware image is corrupt. (0x%.8X)", PpTpmUpdate->unErrorDetails);
                    break;
                case RC_E_WRONG_FW_IMAGE:
                    unReturnValue = PpTpmUpdate->unErrorDetails;
                    ERROR_STORE_FMT(unReturnValue, L"The provided firmware image is not valid for the TPM. (0x%.8X)", PpTpmUpdate->unErrorDetails);
                    break;
                case RC_E_NEWER_TOOL_REQUIRED:
                    PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NA;
                    unReturnValue = PpTpmUpdate->unErrorDetails;
                    ERROR_STORE_FMT(unReturnValue, L"A newer version of the tool is required to process the provided firmware image. (0x%.8X)", PpTpmUpdate->unErrorDetails);
                    break;
                case RC_E_WRONG_DECRYPT_KEYS:
                    unReturnValue = PpTpmUpdate->unErrorDetails;
                    ERROR_STORE_FMT(unReturnValue, L"The provided firmware image is not valid for the TPM. (0x%.8X)", PpTpmUpdate->unErrorDetails);
                    break;
                case RC_E_NEWER_FW_IMAGE_REQUIRED:
                    unReturnValue = PpTpmUpdate->unErrorDetails;
                    ERROR_STORE_FMT(unReturnValue, L"A newer revision of the firmware image is required. (0x%.8X)", PpTpmUpdate->unErrorDetails);
                    break;
                case RC_E_FW_UPDATE_BLOCKED:
                    PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NA;
                    unReturnValue = PpTpmUpdate->unErrorDetails;
                    ERROR_STORE_FMT(unReturnValue, L"The TPM does not allow further updates. (0x%.8X)", PpTpmUpdate->unErrorDetails);
                    break;
                default:
                    unReturnValue = RC_E_TPM_FIRMWARE_UPDATE;
                    ERROR_STORE_FMT(unReturnValue, L"The firmware image check returned an unexpected value. (0x%.8X)", PpTpmUpdate->unErrorDetails);
                    break;
            }
            break;
        }

        // Parse the new image and get the target version and the target family
        {
            BYTE* rgbIfxFirmwareImageStream = PpTpmUpdate->rgbFirmwareImage;
            IfxFirmwareImage sIfxFirmwareImage;
            int nIfxFirmwareImageSize = (int)PpTpmUpdate->unFirmwareImageSize;
            unsigned int unNewFirmwareVersionSize = RG_LEN(PpTpmUpdate->wszNewFirmwareVersion);
            Platform_MemorySet(&sIfxFirmwareImage, 0, sizeof(sIfxFirmwareImage));

            unReturnValue = FirmwareImage_Unmarshal(&sIfxFirmwareImage, &rgbIfxFirmwareImageStream, &nIfxFirmwareImageSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Firmware image cannot be parsed.");
                break;
            }

            unReturnValue = Platform_StringCopy(PpTpmUpdate->wszNewFirmwareVersion, &unNewFirmwareVersionSize, sIfxFirmwareImage.wszTargetVersion);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value while copying the target firmware version.");
                break;
            }

            PpTpmUpdate->bTargetFamily = sIfxFirmwareImage.bTargetTpmFamily;

            // Set new firmware valid state to "YES"
            PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_YES;
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Prepare a firmware update for a TPM1.2 with (Deferred) Physical Presence.
 *  @details    This function will prepare the TPM1.2 to do a firmware update.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_TPM12_DEFERREDPP_REQUIRED  Physical Presence is locked and Deferred Physical Presence is not set.
 *  @retval     RC_E_FAIL                       An unexpected error occurred.
 *  @retval     ...                             Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_PrepareTPM12PhysicalPresence()
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Deferred Physical Presence is not set so try to enable Physical Presence and set Deferred Physical Presence with following command sequence.
        // First try to enable the Physical Presence command, it may already be enabled
        unReturnValue = TSS_TSC_PhysicalPresence(TSS_TPM_PHYSICAL_PRESENCE_CMD_ENABLE);
        // In case this has already been done and lifetime was locked in TPM factory, the command above will fail with TPM_BAD_PARAMETER.
        // But in case Physical Presence is not locked yet, we can still perform all required actions, therefore this is not necessarily an error and we
        // should continue.
        if (RC_SUCCESS != unReturnValue && TSS_TPM_BAD_PARAMETER != (unReturnValue ^ RC_TPM_MASK))
        {
            ERROR_STORE(unReturnValue, L"Error calling TSS_TSC_PhysicalPresence(TPM_PHYSICAL_PRESENCE_CMD_ENABLE)");
            break;
        }

        // Try to set Physical Presence, may be locked
        unReturnValue = TSS_TSC_PhysicalPresence(TSS_TPM_PHYSICAL_PRESENCE_PRESENT);
        // In case Physical Presence is locked, the command above will fail with TPM_BAD_PARAMETER.
        // Since Deferred Physical Presence is also not set we must stop the update execution and return to the caller
        if (RC_SUCCESS != unReturnValue)
        {
            if (TSS_TPM_BAD_PARAMETER == (unReturnValue ^ RC_TPM_MASK))
                unReturnValue = RC_E_TPM12_DEFERREDPP_REQUIRED;

            ERROR_STORE(unReturnValue, L"Error calling TSS_TSC_PhysicalPresence(TPM_PHYSICAL_PRESENCE_PRESENT)");
            break;
        }

        {
            // Set Deferred Physical Presence bit
            unsigned int unSubCapSwapped = Platform_SwapBytes32(TSS_TPM_SD_DEFERREDPHYSICALPRESENCE);
            BYTE setValue[] = { 0x00, 0x00, 0x00, 0x01}; // TRUE
            unReturnValue = TSS_TPM_SetCapability(TSS_TPM_SET_STCLEAR_DATA, sizeof(unSubCapSwapped), (BYTE*)&unSubCapSwapped, sizeof(setValue), setValue);
            // If we manage to come to this call, the command should succeed. Therefore any error is really an error and should be logged and handled properly.
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Error calling TSS_TPM_SetCapability(TPM_SET_STCLEAR_DATA)");
                break;
            }
        }
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Processes a sequence of TPM update related commands to update the firmware.
 *  @details    This module processes the firmware update. Afterwards the result is returned to the calling module.
 *              The function utilizes the MicroTSS library.
 *
 *  @param      PpTpmUpdate         Pointer to an initialized IfxUpdate structure to be filled in.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. PpTpmUpdate is NULL or invalid.
 *  @retval     RC_E_FAIL           An unexpected error occurred.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_UpdateFirmware(
    _Inout_ IfxUpdate* PpTpmUpdate)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        ENUM_UPDATE_TYPES unUpdateType = UPDATE_TYPE_NONE;
        IfxFirmwareUpdateData sFirmwareUpdateData;
        Platform_MemorySet(&sFirmwareUpdateData, 0, sizeof(sFirmwareUpdateData));

        // Check parameters
        if (NULL == PpTpmUpdate ||
                STRUCT_TYPE_TpmUpdate != PpTpmUpdate->info.hdr.unType ||
                PpTpmUpdate->info.hdr.unSize != sizeof(IfxUpdate) ||
                STRUCT_SUBTYPE_PREPARE != PpTpmUpdate->unSubType)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
            break;
        }

        // Set TpmUpdate structure sub type and return value
        PpTpmUpdate->unSubType = STRUCT_SUBTYPE_UPDATE;
        PpTpmUpdate->info.hdr.unReturnCode = RC_E_FAIL;

        // Set the session handle for a TPM2.0 update flow if necessary
        if (PpTpmUpdate->info.sTpmState.attribs.tpm20)
            sFirmwareUpdateData.unSessionHandle = PpTpmUpdate->hPolicySession;

        // Try to set TPM Owner authentication hash for update with taking ownership if necessary
        if (PpTpmUpdate->info.sTpmState.attribs.tpm12)
        {
            // Get update type
            if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, (unsigned int*)&unUpdateType))
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_UPDATE_TYPE);
                break;
            }

            // Set TPM Owner authentication hash only in case of corresponding update type
            if (UPDATE_TYPE_TPM12_TAKEOWNERSHIP == unUpdateType)
            {
                if (!PropertyStorage_ExistsElement(PROPERTY_OWNERAUTH_FILE_PATH))
                {
                    // By default use the default owner authorization value (Hash of 1..8 in ASCII format)
                    unReturnValue = Platform_MemoryCopy(sFirmwareUpdateData.rgbOwnerAuthHash, sizeof(sFirmwareUpdateData.rgbOwnerAuthHash), C_defaultOwnerAuthData.authdata, sizeof(C_defaultOwnerAuthData.authdata));
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an unexpected value while copying TPM Owner authentication hash.");
                        break;
                    }
                }
                else
                {
                    // If option <ownerauth> was provided, read owner authorization from file specified in PROPERTY_OWNERAUTH_FILE_PATH
                    wchar_t wszOwnerAuthFilePath[MAX_PATH];
                    unsigned int unOwnerAuthFilePathSize = RG_LEN(wszOwnerAuthFilePath);
                    BYTE* prgbOwnerAuth = NULL;
                    unsigned int unOwnerAuthSize = 0;
                    IGNORE_RETURN_VALUE(Platform_StringSetZero(wszOwnerAuthFilePath, RG_LEN(wszOwnerAuthFilePath)));

                    if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_OWNERAUTH_FILE_PATH, wszOwnerAuthFilePath, &unOwnerAuthFilePathSize))
                    {
                        unReturnValue = RC_E_FAIL;
                        ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_OWNERAUTH_FILE_PATH);
                        break;
                    }

                    unReturnValue = FileIO_ReadFileToBuffer(wszOwnerAuthFilePath, &prgbOwnerAuth, &unOwnerAuthSize);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE_FMT(RC_E_INVALID_OWNERAUTH_OPTION, L"Failed to load the owner authorization file (%ls). (0x%.8X)", wszOwnerAuthFilePath, unReturnValue);
                        unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
                        break;
                    }

                    // It should be 20 bytes, a SHA-1 hash
                    if (unOwnerAuthSize != 20)
                    {
                        Platform_MemoryFree((void**)&prgbOwnerAuth);
                        unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
                        ERROR_STORE_FMT(unReturnValue, L"Owner authorization should be 20 bytes. It is %d bytes.", unOwnerAuthSize);
                        break;
                    }

                    unReturnValue = Platform_MemoryCopy(sFirmwareUpdateData.rgbOwnerAuthHash, sizeof(sFirmwareUpdateData.rgbOwnerAuthHash), prgbOwnerAuth, unOwnerAuthSize);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        Platform_MemoryFree((void**)&prgbOwnerAuth);
                        ERROR_STORE(unReturnValue, L"Unexpected return value from function call Platform_MemoryCopy");
                        break;
                    }

                    Platform_MemoryFree((void**)&prgbOwnerAuth);
                }
                sFirmwareUpdateData.fOwnerAuthProvided = TRUE;
            }
            else if (UPDATE_TYPE_TPM12_OWNERAUTH == unUpdateType)
            {
                BYTE* prgbOwnerAuth = NULL;
                unsigned int unOwnerAuthSize = 0;
                wchar_t wszOwnerAuthFilePath[MAX_PATH];
                unsigned int unOwnerAuthFilePathSize = RG_LEN(wszOwnerAuthFilePath);
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszOwnerAuthFilePath, RG_LEN(wszOwnerAuthFilePath)));

                // Read owner authorization from file specified in PROPERTY_OWNERAUTH_FILE_PATH
                if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_OWNERAUTH_FILE_PATH, wszOwnerAuthFilePath, &unOwnerAuthFilePathSize))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_OWNERAUTH_FILE_PATH);
                    break;
                }

                unReturnValue = FileIO_ReadFileToBuffer(wszOwnerAuthFilePath, &prgbOwnerAuth, &unOwnerAuthSize);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_INVALID_OWNERAUTH_OPTION, L"Failed to load the owner authorization file (%ls). (0x%.8X)", wszOwnerAuthFilePath, unReturnValue);
                    unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
                    break;
                }

                // It should be 20 bytes, a SHA-1 hash
                if (unOwnerAuthSize != 20)
                {
                    Platform_MemoryFree((void**)&prgbOwnerAuth);
                    unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
                    ERROR_STORE_FMT(unReturnValue, L"Owner authorization should be 20 bytes. It is %d bytes.", unOwnerAuthSize);
                    break;
                }

                // Store in sFirmwareUpdateData.rgbOwnerAuthHash
                unReturnValue = Platform_MemoryCopy(sFirmwareUpdateData.rgbOwnerAuthHash, sizeof(sFirmwareUpdateData.rgbOwnerAuthHash), prgbOwnerAuth, unOwnerAuthSize);
                Platform_MemoryFree((void**)&prgbOwnerAuth);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an unexpected value while copying TPM Owner authentication hash.");
                    break;
                }
                sFirmwareUpdateData.fOwnerAuthProvided = TRUE;
            }
        }

        // Update firmware
        sFirmwareUpdateData.fnProgressCallback = &Response_ProgressCallback;
        sFirmwareUpdateData.fnUpdateStartedCallback = &CommandFlow_TpmUpdate_UpdateStartedCallback;
        sFirmwareUpdateData.rgbFirmwareImage = PpTpmUpdate->rgbFirmwareImage;
        sFirmwareUpdateData.unFirmwareImageSize = PpTpmUpdate->unFirmwareImageSize;
        PpTpmUpdate->info.hdr.unReturnCode = FirmwareUpdate_UpdateImage(&sFirmwareUpdateData);
        unReturnValue = RC_SUCCESS;
        if (RC_SUCCESS != PpTpmUpdate->info.hdr.unReturnCode)
            break;

        // The firmware update completed successfully. Remove run data. Ignore errors: For example the tool might not have
        // access rights to remove the file.
        if (FileIO_Exists(TPM_FACTORY_UPD_RUNDATA_FILE))
        {
            IGNORE_RETURN_VALUE(FileIO_Remove(TPM_FACTORY_UPD_RUNDATA_FILE));
        }
    }
    WHILE_FALSE_END;

    // Try to close policy session in case of errors (only if session has already been started)
    if ((RC_SUCCESS != unReturnValue || (NULL != PpTpmUpdate && RC_SUCCESS != PpTpmUpdate->info.hdr.unReturnCode)) && (NULL != PpTpmUpdate && 0 != PpTpmUpdate->hPolicySession))
    {
        IGNORE_RETURN_VALUE(TSS_TPM2_FlushContext(PpTpmUpdate->hPolicySession));
        PpTpmUpdate->hPolicySession = 0;
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Prepare a firmware update.
 *  @details    This function will prepare the TPM to do a firmware update.
 *
 *  @param      PpTpmUpdate         Pointer to an initialized IfxUpdate structure to be filled in.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL           An unexpected error occurred.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_PrepareFirmwareUpdate(
    _Inout_ IfxUpdate* PpTpmUpdate)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check parameters
        if (NULL == PpTpmUpdate ||
                STRUCT_TYPE_TpmUpdate != PpTpmUpdate->info.hdr.unType ||
                PpTpmUpdate->info.hdr.unSize != sizeof(IfxUpdate) ||
                STRUCT_SUBTYPE_IS_UPDATABLE != PpTpmUpdate->unSubType)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
            break;
        }

        // Set TpmUpdate structure sub type and return value
        PpTpmUpdate->unSubType = STRUCT_SUBTYPE_PREPARE;
        PpTpmUpdate->info.hdr.unReturnCode = RC_E_FAIL;

        // Check which type of TPM is present in which state
        if (!PpTpmUpdate->info.sTpmState.attribs.tpmInOperationalMode)
        {
            // No preparation needed
            PpTpmUpdate->info.hdr.unReturnCode = RC_SUCCESS;
            unReturnValue = RC_SUCCESS;
        }
        else if (PpTpmUpdate->info.sTpmState.attribs.tpm20)
        {
            // Prepare TPM2.0 update
            PpTpmUpdate->info.hdr.unReturnCode = FirmwareUpdate_PrepareTPM20Policy(&PpTpmUpdate->hPolicySession);
            unReturnValue = RC_SUCCESS;
        }
        else if (PpTpmUpdate->info.sTpmState.attribs.tpm12)
        {
            unsigned int unUpdateType = UPDATE_TYPE_NONE;

            // Check which type is given
            if (TRUE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, &unUpdateType))
            {
                if (UPDATE_TYPE_TPM12_DEFERREDPP == unUpdateType)
                {
                    // Check if Deferred Physical Presence is set. If so we do not need to set it and can jump out.
                    if (PpTpmUpdate->info.sTpmState.attribs.tpm12DeferredPhysicalPresence)
                    {
                        PpTpmUpdate->info.hdr.unReturnCode = RC_SUCCESS;
                        unReturnValue = RC_SUCCESS;
                    }
                    else
                    {
                        // Prepare (deferred) physical presence based TPM1.2 update
                        PpTpmUpdate->info.hdr.unReturnCode = CommandFlow_TpmUpdate_PrepareTPM12PhysicalPresence();
                        unReturnValue = RC_SUCCESS;
                    }
                }
                else if (UPDATE_TYPE_TPM12_TAKEOWNERSHIP == unUpdateType)
                {
                    // Prepare owner based TPM1.2 update
                    PpTpmUpdate->info.hdr.unReturnCode = CommandFlow_TpmUpdate_PrepareTPM12Ownership();
                    unReturnValue = RC_SUCCESS;
                }
                else if (UPDATE_TYPE_TPM12_OWNERAUTH == unUpdateType)
                {
                    // Verify owner authorization
                    PpTpmUpdate->info.hdr.unReturnCode = CommandFlow_TpmUpdate_VerifyTPM12Ownerauth(PpTpmUpdate->info.sTpmState);
                    unReturnValue = RC_SUCCESS;
                }
                else
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE(unReturnValue, L"Unsupported Update type detected");
                }
            }
            else
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_UPDATE_TYPE);
            }
        }
        else
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"Unsupported TPM type detected");
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Check if a firmware update is possible.
 *  @details    This function will check if the firmware is updatable with the given firmware image
 *
 *  @param      PpTpmUpdate             Pointer to an initialized IfxUpdate structure to be filled in.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_INVALID_FW_OPTION  In case of an invalid firmware option argument.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from Micro TSS functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_IsFirmwareUpdatable(
    _Inout_ IfxUpdate* PpTpmUpdate)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check parameters
        if (NULL == PpTpmUpdate || PpTpmUpdate->info.hdr.unType != STRUCT_TYPE_TpmUpdate || PpTpmUpdate->info.hdr.unSize != sizeof(IfxUpdate))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
            break;
        }

        // Set TpmUpdate structure sub type and return value
        PpTpmUpdate->unSubType = STRUCT_SUBTYPE_IS_UPDATABLE;
        PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NA;
        PpTpmUpdate->info.hdr.unReturnCode = RC_E_FAIL;

        // Check if TPM is updatable regarding the count
        {
            ENUM_UPDATE_TYPES unUpdateType = UPDATE_TYPE_NONE;

            // Get update type
            if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, (unsigned int*)&unUpdateType))
            {
                unReturnValue = RC_E_FAIL;
                break;
            }

            // Check if TPM1.2 is detected
            if (PpTpmUpdate->info.sTpmState.attribs.tpm12 && PpTpmUpdate->info.sTpmState.attribs.tpmInOperationalMode)
            {
                // Check if the correct update type is set
                if (UPDATE_TYPE_TPM12_DEFERREDPP != unUpdateType && UPDATE_TYPE_TPM12_TAKEOWNERSHIP != unUpdateType && UPDATE_TYPE_TPM12_OWNERAUTH != unUpdateType)
                {
                    PpTpmUpdate->info.hdr.unReturnCode = RC_E_INVALID_UPDATE_OPTION;
                    ERROR_STORE(PpTpmUpdate->info.hdr.unReturnCode, L"Wrong update type detected. The underlying TPM is a TPM1.2.");
                    unReturnValue = RC_SUCCESS;
                    break;
                }

                // Check if TPM already has an owner
                if (PpTpmUpdate->info.sTpmState.attribs.tpm12owner && (UPDATE_TYPE_TPM12_DEFERREDPP == unUpdateType || UPDATE_TYPE_TPM12_TAKEOWNERSHIP == unUpdateType))
                {
                    PpTpmUpdate->info.hdr.unReturnCode = RC_E_TPM12_OWNED;
                    ERROR_STORE(PpTpmUpdate->info.hdr.unReturnCode, L"TPM1.2 Owner detected. Update cannot be done.");
                    unReturnValue = RC_SUCCESS;
                    break;
                }
            }

            // Check if TPM2.0 is detected and correct update type is set
            if (PpTpmUpdate->info.sTpmState.attribs.tpm20 && UPDATE_TYPE_TPM20_EMPTYPLATFORMAUTH != unUpdateType)
            {
                // For TPM2.0 based firmware update loader exclude config file usage in non-operational mode
                if (!(PpTpmUpdate->info.sTpmState.attribs.tpmHasFULoader20 &&
                        UPDATE_TYPE_CONFIG_FILE == unUpdateType &&
                        !PpTpmUpdate->info.sTpmState.attribs.tpmInOperationalMode))
                {
                    PpTpmUpdate->info.hdr.unReturnCode = RC_E_INVALID_UPDATE_OPTION;
                    ERROR_STORE(PpTpmUpdate->info.hdr.unReturnCode, L"Wrong update type detected. The underlying TPM is a TPM2.0.");
                    unReturnValue = RC_SUCCESS;
                    break;
                }
            }

            // Check if restart is required
            if (PpTpmUpdate->info.sTpmState.attribs.tpm20restartRequired)
            {
                PpTpmUpdate->info.hdr.unReturnCode = RC_E_RESTART_REQUIRED;
                ERROR_STORE_FMT(PpTpmUpdate->info.hdr.unReturnCode, L"System must be restarted. (0x%.8lX)", PpTpmUpdate->info.sTpmState.attribs);
                unReturnValue = RC_SUCCESS;
                break;
            }

            // Check if TPM is in failure mode
            if (PpTpmUpdate->info.sTpmState.attribs.tpm20InFailureMode)
            {
                PpTpmUpdate->info.hdr.unReturnCode = RC_E_TPM20_FAILURE_MODE;
                ERROR_STORE_FMT(PpTpmUpdate->info.hdr.unReturnCode, L"TPM2.0 is in failure mode. (0x%.8lX)", PpTpmUpdate->info.sTpmState.attribs);
                unReturnValue = RC_SUCCESS;
                break;
            }

            // Check if updatable
            if (PpTpmUpdate->info.unRemainingUpdates == 0)
            {
                PpTpmUpdate->info.hdr.unReturnCode = RC_E_FW_UPDATE_BLOCKED;
                ERROR_STORE_FMT(PpTpmUpdate->info.hdr.unReturnCode, L"Update counter is zero. (0x%.8lX)", PpTpmUpdate->info.sTpmState.attribs);
                unReturnValue = RC_SUCCESS;
                break;
            }
        }

        // Get firmware path from property storage and load file
        {
            wchar_t wszFirmwareImagePath[MAX_PATH];
            unsigned int unFirmwareImagePathSize = RG_LEN(wszFirmwareImagePath);
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszFirmwareImagePath, RG_LEN(wszFirmwareImagePath)));

            if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_FIRMWARE_PATH, wszFirmwareImagePath, &unFirmwareImagePathSize))
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_FIRMWARE_PATH);
                break;
            }

            unReturnValue = FileIO_ReadFileToBuffer(wszFirmwareImagePath, &PpTpmUpdate->rgbFirmwareImage, &PpTpmUpdate->unFirmwareImageSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE_FMT(RC_E_INVALID_FW_OPTION, L"Failed to load the firmware image (%ls). (0x%.8X)", wszFirmwareImagePath, unReturnValue);
                unReturnValue = RC_E_INVALID_FW_OPTION;
                break;
            }
        }

        unReturnValue = CommandFlow_TpmUpdate_IsTpmUpdatableWithFirmware(PpTpmUpdate);
        if (RC_SUCCESS != unReturnValue)
        {
            PpTpmUpdate->info.hdr.unReturnCode = unReturnValue;
            unReturnValue = RC_SUCCESS;
            break;
        }

        PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_YES;
        PpTpmUpdate->info.hdr.unReturnCode = RC_SUCCESS;
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Take TPM Ownership with hard coded hash value.
 *  @details    The corresponding TPM Owner authentication is described in the user manual.
 *
 *  @retval     RC_SUCCESS                      TPM Ownership was taken successfully.
 *  @retval     RC_E_FAIL                       An unexpected error occurred.
 *  @retval     RC_E_TPM12_DISABLED_DEACTIVATED In case the TPM is disabled and deactivated.
 *  @retval     ...                             Error codes from Micro TSS functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_PrepareTPM12Ownership()
{
    unsigned int unReturnValue = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        TSS_TPM_PUBKEY sTpmPubKey;
        BYTE rgbPublicExponent[] = {0x01, 0x00, 0x01};
        // Encrypted TPM Owner authentication hash buffer
        BYTE rgbEncryptedOwnerAuthHash[TSS_MAX_RSA_KEY_BYTES];
        unsigned int unEncryptedOwnerHashSize = RG_LEN(rgbEncryptedOwnerAuthHash);
        // Storage Root Key hash value = SRK authentication value
        BYTE rgbSrkWellKnownAuth[] = SRK_WELL_KNOWN_AUTH;
        // Encrypted SRK hash buffer
        BYTE rgbEncryptedSrkHash[TSS_MAX_RSA_KEY_BYTES];
        unsigned int unEncryptedSrkHashSize = RG_LEN(rgbEncryptedSrkHash);
        // OIAP Session parameters
        // Authorization session handle
        TSS_TPM_AUTHHANDLE unAuthHandle = 0;
        // Even nonce to be generated by TPM prior TPM_TakeOwnership
        TSS_TPM_NONCE sAuthLastNonceEven;
        // Take ownership parameters
        TSS_TPM_KEY sSrkParams;
        TSS_TPM_KEY sSrkKey;

        Platform_MemorySet(&sTpmPubKey, 0, sizeof(sTpmPubKey));
        Platform_MemorySet(rgbEncryptedOwnerAuthHash, 0, sizeof(rgbEncryptedOwnerAuthHash));
        Platform_MemorySet(rgbEncryptedSrkHash, 0, sizeof(rgbEncryptedSrkHash));
        Platform_MemorySet(&sAuthLastNonceEven, 0, sizeof(sAuthLastNonceEven));
        Platform_MemorySet(&sSrkParams, 0, sizeof(sSrkParams));
        Platform_MemorySet(&sSrkKey, 0, sizeof(sSrkKey));

        // Get Public Endorsement Key
        unReturnValue = TSS_TPM_ReadPubEK(&sTpmPubKey);
        if (unReturnValue != RC_SUCCESS)
        {
            ERROR_STORE(unReturnValue, L"Read Public Endorsement Key failed!");
            break;
        }

        {
            TSS_TPM_AUTHDATA sOwnerAuth;
            Platform_MemorySet(&sOwnerAuth, 0, sizeof(sOwnerAuth));

            if (!PropertyStorage_ExistsElement(PROPERTY_OWNERAUTH_FILE_PATH))
            {
                // By default use the default owner authorization value (Hash of 1..8 in ASCII format)
                unReturnValue = Platform_MemoryCopy(sOwnerAuth.authdata, sizeof(sOwnerAuth.authdata), C_defaultOwnerAuthData.authdata, sizeof(C_defaultOwnerAuthData.authdata));
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Unexpected return value from function call Platform_MemoryCopy");
                    break;
                }
            }
            else
            {
                // If option <ownerauth> was provided, read owner authorization from file specified in PROPERTY_OWNERAUTH_FILE_PATH
                wchar_t wszOwnerAuthFilePath[MAX_PATH];
                unsigned int unOwnerAuthFilePathSize = RG_LEN(wszOwnerAuthFilePath);
                BYTE* prgbOwnerAuth = NULL;
                unsigned int unOwnerAuthSize = 0;
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszOwnerAuthFilePath, RG_LEN(wszOwnerAuthFilePath)));

                if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_OWNERAUTH_FILE_PATH, wszOwnerAuthFilePath, &unOwnerAuthFilePathSize))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_OWNERAUTH_FILE_PATH);
                    break;
                }

                unReturnValue = FileIO_ReadFileToBuffer(wszOwnerAuthFilePath, &prgbOwnerAuth, &unOwnerAuthSize);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_INVALID_OWNERAUTH_OPTION, L"Failed to load the owner authorization file (%ls). (0x%.8X)", wszOwnerAuthFilePath, unReturnValue);
                    unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
                    break;
                }

                // It should be 20 bytes, a SHA-1 hash
                if (unOwnerAuthSize != 20)
                {
                    Platform_MemoryFree((void**)&prgbOwnerAuth);
                    unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
                    ERROR_STORE_FMT(unReturnValue, L"Owner authorization should be 20 bytes. It is %d bytes.", unOwnerAuthSize);
                    break;
                }

                unReturnValue = Platform_MemoryCopy(sOwnerAuth.authdata, sizeof(sOwnerAuth.authdata), prgbOwnerAuth, unOwnerAuthSize);
                if (RC_SUCCESS != unReturnValue)
                {
                    Platform_MemoryFree((void**)&prgbOwnerAuth);
                    ERROR_STORE(unReturnValue, L"Unexpected return value from function call Platform_MemoryCopy");
                    break;
                }

                Platform_MemoryFree((void**)&prgbOwnerAuth);
            }

            // Encrypt TPM Owner authentication hash
            unReturnValue = Crypt_EncryptRSA(
                                CRYPT_ES_RSAESOAEP_SHA1_MGF1,
                                TSS_SHA1_DIGEST_SIZE, sOwnerAuth.authdata, // Decrypted buffer
                                sTpmPubKey.pubKey.keyLength, sTpmPubKey.pubKey.key, // Public modulus
                                sizeof(rgbPublicExponent), rgbPublicExponent, // Public exponent
                                sizeof(s_rgbOAEPPad), s_rgbOAEPPad,
                                &unEncryptedOwnerHashSize, rgbEncryptedOwnerAuthHash);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"TPM Owner authentication hash encryption failed!");
                break;
            }

            // Encrypt TPM SRK authentication hash (well-known authentication)
            unReturnValue = Crypt_EncryptRSA(
                                CRYPT_ES_RSAESOAEP_SHA1_MGF1,
                                TSS_SHA1_DIGEST_SIZE, rgbSrkWellKnownAuth, // Decrypted buffer
                                sTpmPubKey.pubKey.keyLength, sTpmPubKey.pubKey.key, // Public modulus
                                sizeof(rgbPublicExponent), rgbPublicExponent, // Public exponent
                                sizeof(s_rgbOAEPPad), s_rgbOAEPPad,
                                &unEncryptedSrkHashSize, rgbEncryptedSrkHash);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"SRK authentication hash encryption failed!");
                break;
            }

            // Get Authorization Session handle and even nonce
            unReturnValue = TSS_TPM_OIAP(&unAuthHandle, &sAuthLastNonceEven);
            if (unReturnValue != RC_SUCCESS)
            {
                ERROR_STORE(unReturnValue, L"Get Authorization Session handle failed!");
                break;
            }

            // Initialize SRK parameters
            sSrkParams.ver.major = 1;
            sSrkParams.ver.minor = 1;
            sSrkParams.ver.revMajor = 0;
            sSrkParams.ver.revMinor = 0;
            sSrkParams.keyUsage = TSS_TPM_KEY_STORAGE;
            sSrkParams.keyFlags = 0;
            sSrkParams.authDataUsage = TSS_TPM_AUTH_ALWAYS;
            sSrkParams.algorithmParms.algorithmID = 0x00000001;
            sSrkParams.algorithmParms.encScheme = CRYPT_ES_RSAESOAEP_SHA1_MGF1;
            sSrkParams.algorithmParms.sigScheme = TSS_TPM_SS_NONE;
            sSrkParams.algorithmParms.parmSize = sizeof(TSS_TPM_RSA_KEY_PARMS);
            sSrkParams.algorithmParms.parms.keyLength = 0x800;
            sSrkParams.algorithmParms.parms.numPrimes = 2;
            sSrkParams.algorithmParms.parms.exponentSize = 0;
            sSrkParams.PCRInfoSize = 0;
            sSrkParams.pubKey.keyLength = 0;
            sSrkParams.encSize = 0;

            unReturnValue = TSS_TPM_TakeOwnership(
                                rgbEncryptedOwnerAuthHash, unEncryptedSrkHashSize, // Encrypted TPM Owner authentication hash
                                rgbEncryptedSrkHash, unEncryptedSrkHashSize, // Encrypted SRK authentication hash
                                &sSrkParams, unAuthHandle, &sOwnerAuth,
                                &sAuthLastNonceEven, &sSrkKey);

            if (RC_SUCCESS != unReturnValue || 0 == sSrkKey.pubKey.keyLength)
            {
                ERROR_STORE(unReturnValue, L"Take Ownership failed!");
                break;
            }
        }
    }
    WHILE_FALSE_END;

    // Map return value in case TPM is disabled or deactivated to corresponding tool exit code
    if ((TSS_TPM_DEACTIVATED == (unReturnValue ^ RC_TPM_MASK)) ||
            (TSS_TPM_DISABLED == (unReturnValue ^ RC_TPM_MASK)))
    {
        unReturnValue = RC_E_TPM12_DISABLED_DEACTIVATED;
        ERROR_STORE(unReturnValue, L"Take Ownership failed!");
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Verify TPM1.2 owner authorization.
 *  @details    Verify TPM1.2 owner authorization.
 *
 *  @param      PtpmState                       The current TPM state.
 *  @retval     RC_SUCCESS                      TPM owner authorization was verified successfully.
 *  @retval     RC_E_FAIL                       An unexpected error occurred.
 *  @retval     RC_E_INVALID_OWNERAUTH_OPTION   Either an invalid file path was given in --ownerauth parameter or file was not 20 bytes in size.
 *  @retval     RC_E_TPM12_DISABLED_DEACTIVATED In case the TPM is disabled and deactivated.
 *  @retval     RC_E_TPM12_INVALID_OWNERAUTH    The owner authorization is invalid.
 *  @retval     ...                             Error codes from Micro TSS functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_VerifyTPM12Ownerauth(
    _In_ TPM_STATE PtpmState)
{
    unsigned int unReturnValue = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        BYTE* prgbOwnerAuth = NULL;
        unsigned int unOwnerAuthSize = 0;
        wchar_t wszOwnerAuthFilePath[MAX_PATH];
        unsigned int unOwnerAuthFilePathSize = RG_LEN(wszOwnerAuthFilePath);
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszOwnerAuthFilePath, RG_LEN(wszOwnerAuthFilePath)));

        // Verify that the TPM has an owner
        if (!PtpmState.attribs.tpm12owner)
        {
            unReturnValue = RC_E_TPM12_NO_OWNER;
            break;
        }

        // Read owner authorization from file specified in PROPERTY_OWNERAUTH_FILE_PATH
        if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_OWNERAUTH_FILE_PATH, wszOwnerAuthFilePath, &unOwnerAuthFilePathSize))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_OWNERAUTH_FILE_PATH);
            break;
        }

        unReturnValue = FileIO_ReadFileToBuffer(wszOwnerAuthFilePath, &prgbOwnerAuth, &unOwnerAuthSize);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE_FMT(RC_E_INVALID_OWNERAUTH_OPTION, L"Failed to load the owner authorization file (%ls). (0x%.8X)", wszOwnerAuthFilePath, unReturnValue);
            unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
            break;
        }

        // It should be 20 bytes, a SHA-1 hash
        if (unOwnerAuthSize != 20)
        {
            Platform_MemoryFree((void**)&prgbOwnerAuth);
            unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
            ERROR_STORE_FMT(unReturnValue, L"Owner authorization should be 20 bytes. It is %d bytes.", unOwnerAuthSize);
            break;
        }

        // Check TPM Owner authorization (except for disabled/deactivated TPM)
        unReturnValue = FirmwareUpdate_CheckOwnerAuthorization(prgbOwnerAuth);
        if (RC_SUCCESS != unReturnValue)
        {
            if ((TSS_TPM_DEACTIVATED == (unReturnValue ^ RC_TPM_MASK)) || (TSS_TPM_DISABLED == (unReturnValue ^ RC_TPM_MASK)))
            {
                // In disabled and/or deactivated state TPM Owner authorization may not be checkable prior to first TPM_FieldUpgrade request
                unReturnValue = RC_E_TPM12_DISABLED_DEACTIVATED;
            }
            else
            {
                if (TSS_TPM_AUTHFAIL == (unReturnValue ^ RC_TPM_MASK))
                {
                    unReturnValue = RC_E_TPM12_INVALID_OWNERAUTH;
                    ERROR_STORE(unReturnValue, L"TPM1.2 Owner authorization is incorrect.");
                }
                else
                {
                    ERROR_STORE(unReturnValue, L"TPM1.2 Owner authorization failed.");
                    unReturnValue = RC_E_FAIL;
                }
                break;
            }
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Parses the update configuration settings
 *  @details    Parses the update configuration settings for a settings file based update flow
 *
 *  @param      PwszSection         Pointer to a wide character array containing the current section.
 *  @param      PunSectionSize      Size of the section buffer in elements including the zero termination.
 *  @param      PwszKey             Pointer to a wide character array containing the current key.
 *  @param      PunKeySize          Size of the key buffer in elements including the zero termination.
 *  @param      PwszValue           Pointer to a wide character array containing the current value.
 *  @param      PunValueSize        Size of the value buffer in elements including the zero termination.
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. It is NULL or empty.
 *  @retval     RC_E_FAIL           An unexpected error occurred.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_Parse(
    _In_z_count_(PunSectionSize)    const wchar_t*  PwszSection,
    _In_                            unsigned int    PunSectionSize,
    _In_z_count_(PunKeySize)        const wchar_t*  PwszKey,
    _In_                            unsigned int    PunKeySize,
    _In_z_count_(PunValueSize)      const wchar_t*  PwszValue,
    _In_                            unsigned int    PunValueSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    UNREFERENCED_PARAMETER(PunValueSize);

    do
    {
        // Check parameters
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszSection) ||
                0 == PunSectionSize ||
                PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszKey) ||
                0 == PunKeySize ||
                PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszValue) ||
                0 == PunValueSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more input parameters are NULL or empty.");
            break;
        }

        // Section Update Type
        if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_UPDATE_TYPE, PunSectionSize, TRUE))
        {
            // Check setting tpm12
            if (0 == Platform_StringCompare(PwszKey, CONFIG_UPDATE_TYPE_TPM12, PunKeySize, TRUE))
            {
                if (0 == Platform_StringCompare(PwszValue, CMD_UPDATE_OPTION_TPM12_DEFERREDPP, PunValueSize, TRUE))
                {
                    if (!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_CONFIG_FILE_UPDATE_TYPE12, UPDATE_TYPE_TPM12_DEFERREDPP))
                    {
                        ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatAddKeyUIntegerValuePair, PROPERTY_CONFIG_FILE_UPDATE_TYPE12);
                        break;
                    }
                }
                else if(0 == Platform_StringCompare(PwszValue, CMD_UPDATE_OPTION_TPM12_TAKEOWNERSHIP, PunValueSize, TRUE))
                {
                    if (!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_CONFIG_FILE_UPDATE_TYPE12, UPDATE_TYPE_TPM12_TAKEOWNERSHIP))
                    {
                        ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatAddKeyUIntegerValuePair, PROPERTY_CONFIG_FILE_UPDATE_TYPE12);
                        break;
                    }
                }
                else if(0 == Platform_StringCompare(PwszValue, CMD_UPDATE_OPTION_TPM12_OWNERAUTH, PunValueSize, TRUE))
                {
                    if (!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_CONFIG_FILE_UPDATE_TYPE12, UPDATE_TYPE_TPM12_OWNERAUTH))
                    {
                        ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatAddKeyUIntegerValuePair, PROPERTY_CONFIG_FILE_UPDATE_TYPE12);
                        break;
                    }
                }
                else
                {
                    unReturnValue = RC_E_INVALID_SETTING;
                    ERROR_STORE(unReturnValue, L"Invalid update config-file value for setting CONFIG_UPDATE_TYPE_TPM12 found");
                    break;
                }
            }
            // Check setting tpm20
            if (0 == Platform_StringCompare(PwszKey, CONFIG_UPDATE_TYPE_TPM20, PunKeySize, TRUE))
            {
                if(0 == Platform_StringCompare(PwszValue, CMD_UPDATE_OPTION_TPM20_EMPTYPLATFORMAUTH, PunValueSize, TRUE))
                {
                    if (!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_CONFIG_FILE_UPDATE_TYPE20, UPDATE_TYPE_TPM20_EMPTYPLATFORMAUTH))
                    {
                        ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatAddKeyUIntegerValuePair, PROPERTY_CONFIG_FILE_UPDATE_TYPE20);
                        break;
                    }
                }
                else
                {
                    unReturnValue = RC_E_INVALID_SETTING;
                    ERROR_STORE(unReturnValue, L"Invalid update config-file value for setting CONFIG_UPDATE_TYPE_TPM12 found");
                    break;
                }
            }

            // Unknown setting in current section ignore it
            unReturnValue = RC_SUCCESS;
            break;
        }

        // Section Target Firmware
        if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_TARGET_FIRMWARE, PunSectionSize, TRUE))
        {
            wchar_t* pwszConfigPropertyKey = NULL;
            wchar_t wszPropertyKey[MAX_NAME];

            // Check setting version_FW* (generic setting related to major version)
            if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_FWxx, RG_LEN(CONFIG_TARGET_FIRMWARE_VERSION_FWxx) - 1, TRUE))
            {
                // Get base property storage key
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszPropertyKey, RG_LEN(wszPropertyKey)));
                unsigned int unPropertyKeyLen = RG_LEN(wszPropertyKey);
                unReturnValue = Platform_StringCopy(wszPropertyKey, &unPropertyKeyLen, PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_FWxx);
                if (RC_SUCCESS != unReturnValue)
                    break;

                // Get storage key by concatenating base storage key and major version extracted from config key
                unPropertyKeyLen = RG_LEN(wszPropertyKey);
                unReturnValue = Platform_StringConcatenate(wszPropertyKey, &unPropertyKeyLen, &PwszKey[RG_LEN(CONFIG_TARGET_FIRMWARE_VERSION_FWxx) - 1]);
                if (RC_SUCCESS != unReturnValue)
                    break;

                pwszConfigPropertyKey = wszPropertyKey;
            }

            // Check setting version
            if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_GENERIC, PunKeySize, TRUE))
                pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_GENERIC;

            // Check setting version_SLB965x (LPC)
            if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_SLB965x, PunKeySize, TRUE))
                pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB965x;

            // Check setting version_SLB966x (LPC)
            if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_SLB966x, PunKeySize, TRUE))
                pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB966x;

            // Check setting version_SLB9670 (SPI)
            if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_SLB9670, PunKeySize, TRUE))
                pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB9670;

            // Check setting version_SLI9670 (SPI)
            if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_SLI9670, PunKeySize, TRUE))
                pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLI9670;

            // Check setting version_SLB9615 (I2C)
            if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_SLB9615, PunKeySize, TRUE))
                pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB9615;

            // Check setting version_SLB9645 (I2C)
            if (0 == Platform_StringCompare(PwszKey, CONFIG_TARGET_FIRMWARE_VERSION_SLB9645, PunKeySize, TRUE))
                pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB9645;

            if (NULL != pwszConfigPropertyKey)
            {
                if (!PropertyStorage_AddKeyValuePair(pwszConfigPropertyKey, PwszValue))
                {
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatAddKeyUIntegerValuePair, pwszConfigPropertyKey);
                    break;
                }
            }
            // Unknown setting in current section ignore it
            unReturnValue = RC_SUCCESS;
        }

        // Section Firmware Folder
        if (0 == Platform_StringCompare(PwszSection, CONFIG_SECTION_FIRMWARE_FOLDER, PunSectionSize, TRUE))
        {
            // Check setting version
            if (0 == Platform_StringCompare(PwszKey, CONFIG_FIRMWARE_FOLDER_PATH, PunKeySize, TRUE))
            {
                if (!PropertyStorage_AddKeyValuePair(PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH, PwszValue))
                {
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatAddKeyUIntegerValuePair, PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH);
                    break;
                }
            }
        }

        // Unknown section ignore it
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Initialize configuration settings parsing
 *  @details
 *
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     RC_E_FAIL   An unexpected error occurred.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_InitializeParsing()
{
    unsigned int unReturnValue = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);
    // Nothing to initialize here
    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Finalize configuration settings parsing
 *  @details
 *
 *  @param      PunReturnValue  Current return code which can be overwritten here.
 *  @retval     PunReturnValue  In case PunReturnValue is not equal to RC_SUCCESS.
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       An unexpected error occurred.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_FinalizeParsing(
    _In_ unsigned int PunReturnValue)
{
    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        unsigned int unIndex = 0;
        wchar_t* prgwszMandatoryProperties[] = {
            PROPERTY_CONFIG_FILE_UPDATE_TYPE12,
            PROPERTY_CONFIG_FILE_UPDATE_TYPE20,
            PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH,
            L""
        };

        if (RC_SUCCESS != PunReturnValue)
            break;

        while (0 != Platform_StringCompare(prgwszMandatoryProperties[unIndex], L"", RG_LEN(L""), FALSE))
        {
            // Check if all mandatory settings were parsed
            if (!PropertyStorage_ExistsElement(prgwszMandatoryProperties[unIndex]))
            {
                PunReturnValue = RC_E_INVALID_SETTING;
                ERROR_STORE_FMT(PunReturnValue, L"TPM update config file: %ls is mandatory", prgwszMandatoryProperties[unIndex]);
                break;
            }
            unIndex++;
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, PunReturnValue);

    return PunReturnValue;
}

/**
 *  @brief      Parse the update configuration settings file
 *  @details
 *
 *  @param      PpTpmUpdate                         Contains information about the current TPM and can be filled up with information for the
 *                                                  corresponding Current return code which can be overwritten here.
 *  @retval     PunReturnValue                      In case PunReturnValue is not equal to RC_SUCCESS.
 *  @retval     RC_SUCCESS                          The operation completed successfully.
 *  @retval     RC_E_FAIL                           An unexpected error occurred.
 *  @retval     RC_E_INVALID_CONFIG_OPTION          A configuration file was given that cannot be opened.
 *  @retval     RC_E_FIRMWARE_UPDATE_NOT_FOUND      A firmware update for the current TPM version cannot be found.
 */
_Check_return_
unsigned int
CommandFlow_TpmUpdate_ProceedUpdateConfig(
    _Inout_ IfxUpdate* PpTpmUpdate)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        wchar_t wszConfigFilePath[MAX_STRING_1024];
        unsigned int unConfigFileNamePathSize = RG_LEN(wszConfigFilePath);
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszConfigFilePath, RG_LEN(wszConfigFilePath)));

        // Check parameters
        if (NULL == PpTpmUpdate || PpTpmUpdate->info.hdr.unType != STRUCT_TYPE_TpmUpdate || PpTpmUpdate->info.hdr.unSize != sizeof(IfxUpdate))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected. TpmUpdate structure is not in the correct state.");
            break;
        }

        // Set TpmUpdate structure sub type and return value
        PpTpmUpdate->unSubType = STRUCT_SUBTYPE_IS_UPDATABLE;
        PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NA;
        PpTpmUpdate->info.hdr.unReturnCode = RC_E_FAIL;

        // Get configuration file path from property storage
        if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_CONFIG_FILE_PATH, wszConfigFilePath, &unConfigFileNamePathSize))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_FILE_PATH);
            break;
        }

        // Check if file exists
        if (!FileIO_Exists(wszConfigFilePath))
        {
            unReturnValue = RC_E_INVALID_CONFIG_OPTION;
            ERROR_STORE_FMT(unReturnValue, L"The config file '%ls' does not exist", wszConfigFilePath);
            break;
        }

        // Parse configuration file using the configuration module
        unReturnValue = Config_ParseCustom(
                            wszConfigFilePath,
                            &CommandFlow_TpmUpdate_InitializeParsing,
                            &CommandFlow_TpmUpdate_FinalizeParsing,
                            &CommandFlow_TpmUpdate_Parse);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Error while parsing the config file of the config option.");
            break;
        }

        {
            IfxVersion sVersionName;
            IfxVersion sTargetVersion;
            wchar_t* pwszConfigPropertyKey = NULL;
            wchar_t* pwszConfigTargetVersion = NULL;
            unsigned int unUsedFirmwareImageSize = RG_LEN (PpTpmUpdate->wszUsedFirmwareImage);
            wchar_t wszConfigSettingFirmwarePath[MAX_PATH];
            unsigned int unConfigSettingFirmwarePathSize = RG_LEN (wszConfigSettingFirmwarePath);
            wchar_t wszConfigPropertyExpectedFwKey[MAX_NAME];
            wchar_t wszConfigTargetExpectedFwKey[MAX_NAME];
            wchar_t wszTargetVersion[MAX_NAME];
            unsigned int unTargetVersionSize = RG_LEN(wszTargetVersion);
            wchar_t wszSourceFamily[MAX_NAME];
            unsigned int unSourceFamilySize = RG_LEN(wszSourceFamily);
            wchar_t wszTargetFamily[MAX_NAME];
            unsigned int unTargetFamilySize = RG_LEN(wszTargetFamily);
            unsigned int unSize;
            ENUM_UPDATE_TYPES unUpdateType = UPDATE_TYPE_NONE;
            unsigned int unVersionNameSize = RG_LEN(PpTpmUpdate->info.wszVersionName);
            Platform_MemorySet(&sVersionName, 0, sizeof(sVersionName));
            Platform_MemorySet(&sTargetVersion, 0, sizeof(sTargetVersion));
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszConfigSettingFirmwarePath, RG_LEN(wszConfigSettingFirmwarePath)));
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszTargetVersion, RG_LEN(wszTargetVersion)));
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszSourceFamily, RG_LEN(wszSourceFamily)));
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszTargetFamily, RG_LEN(wszTargetFamily)));

            if (PpTpmUpdate->info.sTpmState.attribs.tpmInOperationalMode)
            {
                // Convert version string to integer values
                unReturnValue = UiUtility_StringParseIfxVersion(PpTpmUpdate->info.wszVersionName, unVersionNameSize, &sVersionName);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Error while parsing the firmware version string.");
                    break;
                }

                BOOL fGenericEntryExists = PropertyStorage_ExistsElement(PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_GENERIC);

                wchar_t wszKeyName[MAX_NAME];
                unSize = MAX_NAME;
                BOOL fNameEntryExists = PropertyStorage_FindKey(PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_Sxx, NULL, wszKeyName, &unSize);
                unSize = MAX_NAME;
                BOOL fFwEntryExists = PropertyStorage_FindKey(PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_FWxx, NULL, wszKeyName, &unSize);

                // Unique entries?
                if (fGenericEntryExists && (fNameEntryExists || fFwEntryExists))
                {
                    unReturnValue = RC_E_INVALID_SETTING;
                    ERROR_STORE_FMT(unReturnValue, L"Option in configuration file must be uniquely '%ls'.", CONFIG_TARGET_FIRMWARE_VERSION_GENERIC);
                    break;
                }

                if (fGenericEntryExists)
                {
                    pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_GENERIC;
                    pwszConfigTargetVersion = CONFIG_TARGET_FIRMWARE_VERSION_GENERIC;
                }

                if(fNameEntryExists)
                {
                    // Check if TPM is SPI, LPC or I2C
                    if ((sVersionName.unMajor == 6) || (sVersionName.unMajor == 7))
                    {
                        pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB9670;
                        pwszConfigTargetVersion = CONFIG_TARGET_FIRMWARE_VERSION_SLB9670;
                    }
                    else if ((sVersionName.unMajor == 4 && sVersionName.unMinor >= 40) || (sVersionName.unMajor == 5))
                    {
                        pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB966x;
                        pwszConfigTargetVersion = CONFIG_TARGET_FIRMWARE_VERSION_SLB966x;
                    }
                    else if ((sVersionName.unMajor == 4 && sVersionName.unMinor < 40))
                    {
                        pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB965x;
                        pwszConfigTargetVersion = CONFIG_TARGET_FIRMWARE_VERSION_SLB965x;
                    }
                    else if ((sVersionName.unMajor == 133) || (sVersionName.unMajor == 149))
                    {
                        pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLB9645;
                        pwszConfigTargetVersion = CONFIG_TARGET_FIRMWARE_VERSION_SLB9645;
                    }
                    else if (sVersionName.unMajor == 13)
                    {
                        pwszConfigPropertyKey = PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_SLI9670;
                        pwszConfigTargetVersion = CONFIG_TARGET_FIRMWARE_VERSION_SLI9670;
                    }
                }

                // Get firmware entry
                if (fFwEntryExists)
                {
                    // Create config property key related to actual firmware version
                    unSize = RG_LEN(wszConfigPropertyExpectedFwKey);
                    unReturnValue = Platform_StringFormat(wszConfigPropertyExpectedFwKey, &unSize, L"%ls%d", PROPERTY_CONFIG_TARGET_FIRMWARE_VERSION_FWxx, sVersionName.unMajor);
                    if (RC_SUCCESS != unReturnValue)
                        break;
                    unSize = RG_LEN(wszConfigTargetExpectedFwKey);
                    unReturnValue = Platform_StringFormat(wszConfigTargetExpectedFwKey, &unSize, L"%ls%d", CONFIG_TARGET_FIRMWARE_VERSION_FWxx, sVersionName.unMajor);
                    if (RC_SUCCESS != unReturnValue)
                        break;

                    if (PropertyStorage_ExistsElement(wszConfigPropertyExpectedFwKey))
                    {
                        if (NULL == pwszConfigPropertyKey)
                        {
                            pwszConfigPropertyKey = wszConfigPropertyExpectedFwKey;
                            pwszConfigTargetVersion = wszConfigTargetExpectedFwKey;
                        }
                        else
                        {
                            unReturnValue = RC_E_INVALID_SETTING;
                            ERROR_STORE_FMT(unReturnValue, L"Option in configuration file must be uniquely '%ls' or '%ls'.", pwszConfigTargetVersion, wszConfigTargetExpectedFwKey);
                            break;
                        }
                    }
                }

                // Property key found?
                if ((NULL == pwszConfigPropertyKey) || (NULL == pwszConfigTargetVersion))
                {
                    unReturnValue = RC_E_INVALID_SETTING;
                    ERROR_STORE(unReturnValue, L"Option in configuration file is wrong or missing.");
                    break;
                }

                // Get target version or target image
                if (!PropertyStorage_GetValueByKey(pwszConfigPropertyKey, wszTargetVersion, &unTargetVersionSize))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", pwszConfigPropertyKey);
                    break;
                }

                // Check if firmware is already up to date
                wchar_t wszTargetVersionUpperCase[MAX_PATH] = {0};
                // Convert characters to upper case
                for (unsigned int i = 0; i < unTargetVersionSize; i++)
                    wszTargetVersionUpperCase[i] = Platform_WCharToUpper(wszTargetVersion[i]);
                wchar_t* pwszTargetVersionSubString = NULL;
                // Check if file name contains source to target version
                if (RC_SUCCESS != Platform_FindString(L"_TO_", wszTargetVersionUpperCase, &pwszTargetVersionSubString))
                    pwszTargetVersionSubString = wszTargetVersion;
                wchar_t* pwszTemp = NULL;
                // Block update to the same firmware version only for TPMs without TPM2.0 based update loader
                if (RC_SUCCESS == Platform_FindString(PpTpmUpdate->info.wszVersionName, pwszTargetVersionSubString, &pwszTemp) && !PpTpmUpdate->info.sTpmState.attribs.tpmHasFULoader20)
                {
                    PpTpmUpdate->unNewFirmwareValid = GENERIC_TRISTATE_STATE_NO;
                    PpTpmUpdate->info.hdr.unReturnCode = RC_E_ALREADY_UP_TO_DATE;
                    unReturnValue = RC_SUCCESS;
                    break;
                }

                // Get firmware folder path
                if (!PropertyStorage_GetValueByKey(PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH, wszConfigSettingFirmwarePath, &unConfigSettingFirmwarePathSize))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_FIRMWARE_FOLDER_PATH);
                    break;
                }

                // Construct firmware binary file path regarding the naming convention of update images
                if ((unTargetVersionSize >= TPM_FIRMWARE_FILE_NAME_EXT_LEN) &&
                        (TPM_FIRMWARE_FILE_NAME_EXT[0] == wszTargetVersion[unTargetVersionSize - TPM_FIRMWARE_FILE_NAME_EXT_LEN]) &&
                        (0 == Platform_StringCompare(&wszTargetVersion[unTargetVersionSize - TPM_FIRMWARE_FILE_NAME_EXT_LEN], TPM_FIRMWARE_FILE_NAME_EXT, TPM_FIRMWARE_FILE_NAME_EXT_LEN, TRUE)))
                {
                    // Fill in the firmware file
                    unReturnValue = Platform_StringCopy(PpTpmUpdate->wszUsedFirmwareImage, &unUsedFirmwareImageSize, wszTargetVersion);
                }
                else if (PpTpmUpdate->info.sTpmState.attribs.tpmHasFULoader20)
                {
                    // Search for comma separated revision string e.g. "a.b.c.d,R1"
                    wchar_t* pwszTargetRevision = NULL;
                    unReturnValue = Platform_FindString(L",", wszTargetVersion, &pwszTargetRevision);
                    if (RC_SUCCESS != unReturnValue || NULL == pwszTargetRevision)
                    {
                        unReturnValue = RC_E_INVALID_SETTING;
                        ERROR_STORE_FMT(unReturnValue, L"Splitting of '%s' into target version and revision failed.", wszTargetVersion);
                        break;
                    }

                    // Set null termination for comma and forward pointer to next character
                    *(pwszTargetRevision++) = 0;

                    // Create firmware image name
                    unReturnValue = Platform_StringFormat(
                                        PpTpmUpdate->wszUsedFirmwareImage,
                                        &unUsedFirmwareImageSize,
                                        TPM_FIRMWARE_FILE_NAME_PATTERN2,
                                        wszTargetVersion,
                                        pwszTargetRevision);
                }
                else
                {
                    // Detect TPM source family
                    if (PpTpmUpdate->info.sTpmState.attribs.tpm12)
                        unReturnValue = Platform_StringCopy(wszSourceFamily, &unSourceFamilySize, TPM12_FAMILY_STRING);
                    else if (PpTpmUpdate->info.sTpmState.attribs.tpm20)
                        unReturnValue = Platform_StringCopy(wszSourceFamily, &unSourceFamilySize, TPM20_FAMILY_STRING);
                    else
                        unReturnValue = RC_E_FAIL;
                    if (RC_SUCCESS != unReturnValue)
                    {
                        unReturnValue = RC_E_FAIL;
                        ERROR_STORE(unReturnValue, L"CommandFlow_TpmUpdate_ProceedUpdateConfig failed while detecting the TPM source family.");
                        break;
                    }

                    // Convert target version string to integer values
                    if (RC_SUCCESS != UiUtility_StringParseIfxVersion(wszTargetVersion, unTargetVersionSize, &sTargetVersion))
                    {
                        unReturnValue = RC_E_INVALID_SETTING;
                        ERROR_STORE(unReturnValue, L"Error while parsing the firmware version string of the config file.");
                        break;
                    }

                    // Get TPM target family from supported target version
                    if ((sTargetVersion.unMajor == 4) || (sTargetVersion.unMajor == 6) ||
                            (sTargetVersion.unMajor == 133) || (sTargetVersion.unMajor == 149))
                        unReturnValue = Platform_StringCopy(wszTargetFamily, &unTargetFamilySize, TPM12_FAMILY_STRING);
                    else if ((sTargetVersion.unMajor == 5) || (sTargetVersion.unMajor == 7) || (sTargetVersion.unMajor == 13))
                        unReturnValue = Platform_StringCopy(wszTargetFamily, &unTargetFamilySize, TPM20_FAMILY_STRING);
                    else
                    {
                        unReturnValue = RC_E_INVALID_SETTING;
                        ERROR_STORE_FMT(unReturnValue, L"The configuration file contains an unsupported value (%ls) in either the TargetFirmware/version_SLB966x or TargetFirmware/version_SLB9670 field.",
                                        PpTpmUpdate->info.wszVersionName);
                        break;
                    }

                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"Platform_StringCopy failed while detecting the TPM target family");
                        break;
                    }

                    // Create firmware image name
                    unReturnValue = Platform_StringFormat(
                                        PpTpmUpdate->wszUsedFirmwareImage,
                                        &unUsedFirmwareImageSize,
                                        TPM_FIRMWARE_FILE_NAME_PATTERN,
                                        wszSourceFamily,
                                        PpTpmUpdate->info.wszVersionName,
                                        wszTargetFamily,
                                        wszTargetVersion);
                }

                if (RC_SUCCESS != unReturnValue)
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE(unReturnValue, L"Platform_StringFormat/Copy returned an unexpected value while composing the firmware image file path.");
                    break;
                }

                // Compose firmware folder
                {
                    wchar_t wszFirmwareFilePath[MAX_STRING_1024];
                    unsigned int unFirmwareFilePathSize = RG_LEN(wszFirmwareFilePath);
                    unsigned int unIndex = 0, unLastFolderIndex = 0;
                    IGNORE_RETURN_VALUE(Platform_StringSetZero(wszFirmwareFilePath, RG_LEN(wszFirmwareFilePath)));

                    // Copy configuration file path to destination buffer
                    unReturnValue = Platform_StringCopy(wszFirmwareFilePath, &unFirmwareFilePathSize, wszConfigFilePath);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                        break;
                    }

                    // Cut off file part
                    for (unIndex = 0; unIndex < unFirmwareFilePathSize; unIndex++)
                    {
                        if (wszFirmwareFilePath[unIndex] == L'\\' ||
                                wszFirmwareFilePath[unIndex] == L'/')
                            unLastFolderIndex = unIndex;
                    }

                    // Check if index is zero means only configuration file name given or configuration file name in Linux root
                    if (unLastFolderIndex == 0)
                    {
                        // If configuration file is placed in Linux root only remove the name not the root slash otherwise add a relative folder "." at the beginning
                        if (wszFirmwareFilePath[unLastFolderIndex] == L'/')
                            unLastFolderIndex++;
                        else
                            wszFirmwareFilePath[unLastFolderIndex++] = L'.';
                    }

                    // Set string zero termination
                    wszFirmwareFilePath[unLastFolderIndex] = L'\0';

                    // Check if configuration setting firmware path is not the actual folder
                    if (0 != Platform_StringCompare(wszConfigSettingFirmwarePath, L".", RG_LEN(L"."), FALSE) &&
                            0 != Platform_StringCompare(wszConfigSettingFirmwarePath, L"./", RG_LEN(L"./"), FALSE) &&
                            0 != Platform_StringCompare(wszConfigSettingFirmwarePath, L".\\", RG_LEN(L".\\"), FALSE))
                    {
                        // If so add the configuration setting firmware folder path to the configuration file folder part
                        unFirmwareFilePathSize = RG_LEN(wszFirmwareFilePath);
                        unReturnValue = Platform_StringConcatenatePaths(wszFirmwareFilePath, &unFirmwareFilePathSize, wszConfigSettingFirmwarePath);
                        if (RC_SUCCESS != unReturnValue)
                        {
                            ERROR_STORE(unReturnValue, L"Platform_StringConcatenate returned an unexpected value while composing the firmware image file path.");
                            break;
                        }
                    }

                    // Add the filled firmware file name template to the composed folder
                    unFirmwareFilePathSize = RG_LEN(wszFirmwareFilePath);
                    unReturnValue = Platform_StringConcatenatePaths(wszFirmwareFilePath, &unFirmwareFilePathSize, PpTpmUpdate->wszUsedFirmwareImage);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"Platform_StringConcatenate returned an unexpected value while composing the firmware image file path.");
                        break;
                    }

                    // Check if firmware image exists
                    if (!FileIO_Exists(wszFirmwareFilePath))
                    {
                        unReturnValue = RC_E_FIRMWARE_UPDATE_NOT_FOUND;
                        ERROR_STORE_FMT(unReturnValue, L"No firmware image found to update the current TPM firmware. (%ls)", wszFirmwareFilePath);
                        break;
                    }

                    // Set property storage attributes
                    // Get configuration file TPM update type depending on the source family version
                    if (PpTpmUpdate->info.sTpmState.attribs.tpm12)
                    {
                        // Get the update type for TPM1.2 from the configuration file
                        if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_CONFIG_FILE_UPDATE_TYPE12, (unsigned int*)&unUpdateType))
                        {
                            unReturnValue = RC_E_FAIL;
                            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_FILE_UPDATE_TYPE12);
                            break;
                        }
                    }
                    else
                    {
                        // Get the update type for TPM2.0 from the configuration file
                        if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_CONFIG_FILE_UPDATE_TYPE20, (unsigned int*)&unUpdateType))
                        {
                            unReturnValue = RC_E_FAIL;
                            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_CONFIG_FILE_UPDATE_TYPE20);
                            break;
                        }
                    }
                    // Set the update type
                    if (!PropertyStorage_ChangeUIntegerValueByKey(PROPERTY_UPDATE_TYPE, unUpdateType))
                    {
                        unReturnValue = RC_E_FAIL;
                        ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_ChangeUIntegerValueByKey failed to change property '%ls'.", PROPERTY_UPDATE_TYPE);
                        break;
                    }
                    // Set the firmware file path
                    if (!PropertyStorage_AddKeyValuePair(PROPERTY_FIRMWARE_PATH, wszFirmwareFilePath))
                    {
                        unReturnValue = RC_E_FAIL;
                        ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_AddKeyValuePair failed to add property '%ls'.", PROPERTY_FIRMWARE_PATH);
                        break;
                    }

                    // Remember that update is done through configuration file.
                    s_fUpdateThroughConfigFile = TRUE;
                }
            }
            else
            {
                // Configuration-file properties will not be evaluated if TPM is in boot loader mode
                // Instead the firmware image from TPM_FACTORY_UPD_RUNDATA_FILE will be used.
                if (FileIO_Exists(TPM_FACTORY_UPD_RUNDATA_FILE))
                {
                    wchar_t* pwszFirmwareImage = NULL;
                    unsigned int unFirmwareImageSize = 0;
                    unsigned int unIndex = 0;
                    wchar_t* pwszLine = NULL;
                    unsigned int unLineSize = 0;

                    if (RC_SUCCESS != FileIO_ReadFileToStringBuffer(TPM_FACTORY_UPD_RUNDATA_FILE, &pwszFirmwareImage, &unFirmwareImageSize))
                    {
                        unReturnValue = RC_E_FAIL;
                        Platform_MemoryFree((void**)&pwszFirmwareImage);
                        ERROR_STORE_FMT(unReturnValue, L"Unexpected error occurred while reading file '%ls'", TPM_FACTORY_UPD_RUNDATA_FILE);
                        break;
                    }

                    // Get line from buffer
                    unReturnValue = UiUtility_StringGetLine(pwszFirmwareImage, unFirmwareImageSize, &unIndex, &pwszLine, &unLineSize);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        if (RC_E_END_OF_STRING == unReturnValue)
                            unReturnValue = RC_SUCCESS;
                        else
                            ERROR_STORE(unReturnValue, L"Unexpected error occurred while parsing the rundata file content.");
                        Platform_MemoryFree((void**)&pwszFirmwareImage);
                        Platform_MemoryFree((void**)&pwszLine);
                        break;
                    }
                    Platform_MemoryFree((void**)&pwszFirmwareImage);

                    // Set the firmware file path
                    if (!PropertyStorage_AddKeyValuePair(PROPERTY_FIRMWARE_PATH, pwszLine))
                    {
                        unReturnValue = RC_E_FAIL;
                        Platform_MemoryFree((void**)&pwszLine);
                        ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_AddKeyValuePair failed to add property '%ls'.", PROPERTY_FIRMWARE_PATH);
                        break;
                    }
                    Platform_MemoryFree((void**)&pwszLine);
                }
                else
                {
                    // Cannot resume the firmware update without TPM_FACTORY_UPD_RUNDATA_FILE
                    unReturnValue = RC_E_RESUME_RUNDATA_NOT_FOUND;
                    ERROR_STORE_FMT(unReturnValue, L"File '%s' is missing. This file is required to resume firmware update in interrupted firmware mode.", TPM_FACTORY_UPD_RUNDATA_FILE);
                    break;
                }
            }

            PpTpmUpdate->info.hdr.unReturnCode = RC_SUCCESS;
            unReturnValue = RC_SUCCESS;
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}
