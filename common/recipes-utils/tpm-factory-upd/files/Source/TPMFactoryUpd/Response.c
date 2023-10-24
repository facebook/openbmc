/**
 *  @brief      Implements the responses of the user interface
 *  @details
 *  @file       Response.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Response.h"
#include "UiUtility.h"
#include "Utility.h"
#include "Resource.h"
#include "CommandLineParser.h"
#include "TpmResponse.h"
#include "FirmwareImage.h"
#include "FirmwareUpdate.h"
#include "ConfigSettings.h"

/// Static flag indicating that header has already been shown once since this application has been started
BOOL s_fHeaderShown = FALSE;

/**
 *  @brief      Gets display text for platformAuth
 *  @details
 *
 *  @param      PbfTpmAttributes    TPM attributes.
 *
 *  @retval     platformAuth text to be displayed on the console.
 */
const wchar_t*
Response_GetPlatformAuthText(
    _In_ BITFIELD_TPM_ATTRIBUTES PbfTpmAttributes)
{
    const wchar_t* pwszText = NULL;

    if (!PbfTpmAttributes.tpmInOperationalMode)
        pwszText = RES_TPM_INFO_N_A;
    else if (PbfTpmAttributes.tpm20restartRequired)
        pwszText = RES_TPM_INFO_N_A;
    else if (PbfTpmAttributes.tpm20phDisabled)
        pwszText = RES_TPM_INFO_PHDISABLED;
    else if (PbfTpmAttributes.tpm20InFailureMode)
        pwszText = RES_TPM_INFO_N_A;
    else if (PbfTpmAttributes.tpm20emptyPlatformAuth)
        pwszText = RES_TPM_INFO_EMPTY_BUFFER;
    else
        pwszText = RES_TPM_INFO_NOT_EMPTY_BUFFER;

    return pwszText;
}

/**
 *  @brief      Displays info text for TPM1.2 where self-test completed successfully (Fields: enabled, activated, owned, deferred physical presence)
 *  @details
 *
 *  @param      PbfTpmAttributes    TPM attributes.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 */
_Check_return_
unsigned int
Response_ShowTpm12Info(
    _In_ BITFIELD_TPM_ATTRIBUTES PbfTpmAttributes)
{
    unsigned int unReturnValueWrite = RC_SUCCESS;
    do
    {
        if (PbfTpmAttributes.tpm12enabled)
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_ENABLED, RES_TPM_INFO_YES);
        }
        else
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_ENABLED, RES_TPM_INFO_NO);
        }

        if (PbfTpmAttributes.tpm12activated)
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_ACTIVATED, RES_TPM_INFO_YES);
        }
        else
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_ACTIVATED, RES_TPM_INFO_NO);
        }

        if (PbfTpmAttributes.tpm12owner)
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_HAS_OWNER, RES_TPM_INFO_YES);
        }
        else
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_HAS_OWNER, RES_TPM_INFO_NO);
        }
        // Show whether the TPM can be updated through deferred physical presence with the tpm12-PP switch.
        if (PbfTpmAttributes.tpm12DeferredPhysicalPresence)
        {
            // Deferred physical presence is asserted.
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_DEFERREDPP, RES_TPM_INFO_YES);
        }
        else
        {
            if (PbfTpmAttributes.tpm12PhysicalPresence ||
                    (!PbfTpmAttributes.tpm12PhysicalPresenceLock &&
                     (!PbfTpmAttributes.tpm12PhysicalPresenceLifetimeLock || PbfTpmAttributes.tpm12PhysicalPresenceCMDEnable)))
            {
                // Preconditions:
                // - Physical presence is asserted
                // - Physical presence can be asserted with SW command because it is not locked yet in this boot cycle.
                // Hence, Deferred Physical Presence can also be asserted.
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_DEFERREDPP, RES_TPM_INFO_NO_SETTABLE);
            }
            else
            {
                // Physical presence is not asserted and cannot be asserted in this boot cycle.
                // Hence, Deferred Physical Presence cannot be asserted.
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_DEFERREDPP, RES_TPM_INFO_NO_NOT_SETTABLE);
            }
        }
    }
    WHILE_FALSE_END;

    return unReturnValueWrite;
}

/**
 *  @brief      Show output
 *  @details    Takes the response information, formats and displays the output
 *
 *  @param      PpHeader            Header to show in response.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. PpHeader is NULL.
 *  @retval     RC_E_INTERNAL       PpHeader->unType is invalid.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
Response_Show(
    _In_ const IfxToolHeader* PpHeader)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Parameter check
        if (NULL == PpHeader)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        switch (PpHeader->unType)
        {
            case STRUCT_TYPE_TpmInfo:
            {
                LOGGING_WRITE_LEVEL4(L"Showing TpmInfo command output.");
                // Show the info response
                unReturnValue = Response_ShowInfo((IfxInfo*)PpHeader);
                break;
            }
            case STRUCT_TYPE_TpmUpdate:
            {
                LOGGING_WRITE_LEVEL4(L"Showing TpmUpdate command output.");
                // Show the update response
                // Hint: Usage of additional '(void*)' cast due to clang warning "-Wcast-align": ... increases required alignment from 4 to 8
                // Since PpHeader (IfxToolHeader*) contains casted pointer to IfxUpdate structure (correctly allocated and aligned), casting the pointer back via '(void*)' is safe
                unReturnValue = Response_ShowUpdate((IfxUpdate*)(void*)PpHeader);
                break;
            }
            case STRUCT_TYPE_Tpm12ClearOwnership:
            {
                LOGGING_WRITE_LEVEL4(L"Showing Tpm12ClearOwnership command output.");
                // Show the clear ownership response
                unReturnValue = Response_ShowClearOwnership((IfxTpm12ClearOwnership*)PpHeader);
                break;
            }
            case STRUCT_TYPE_SetMode:
            {
                LOGGING_WRITE_LEVEL4(L"Showing SetMode command output.");
                // Show the set mode response
                unReturnValue = Response_ShowSetMode((IfxSetMode*)PpHeader);
                break;
            }
            default:
            {
                LOGGING_WRITE_LEVEL1(L"Skipped display of an unrecognized command.");
                unReturnValue = Response_ShowUnknownAction();
                if (RC_SUCCESS != unReturnValue)
                    break;

                // Set return value to RC_E_INTERNAL
                unReturnValue = RC_E_INTERNAL;
                break;
            }
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Show TPM Info output
 *  @details    Format TPM Info output and display
 *
 *  @param      PpTpmInfo       Pointer to a IfxInfo response structure.
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. PpTpmInfo was invalid.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
Response_ShowInfo(
    _In_    const IfxInfo* PpTpmInfo)
{
    unsigned int unReturnValue = RC_E_FAIL;
    unsigned int unReturnValueWrite = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check parameters
        if (NULL == PpTpmInfo || PpTpmInfo->hdr.unType != STRUCT_TYPE_TpmInfo)
        {
            LOGGING_WRITE_LEVEL1(L"Error while checking object PpTpmInfo: was invalid or NULL.");
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized (PpTpmInfo)");
            break;
        }

        if (RC_SUCCESS != unReturnValueWrite)
            break;

        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_INFO_INFORMATION);
        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_INFO_DASHED_LINE);

        if (!PpTpmInfo->sTpmState.attribs.tpm20 && !PpTpmInfo->sTpmState.attribs.tpmInOperationalMode)
        {
            // Display information for TPM with loader v1 in non-operational mode.
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_FAMILY, RES_TPM_INFO_N_A);
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_VERSION, RES_TPM_INFO_N_A);
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_FIRMWARE_VALID, RES_TPM_INFO_NO);
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_FWUPDATE);
        }
        else
        {
            // Display 'TPM family'
            if (PpTpmInfo->sTpmState.attribs.tpm20)
            {
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_FAMILY, RES_TPM_INFO_2_0);
            }
            else if (PpTpmInfo->sTpmState.attribs.tpm12)
            {
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_FAMILY, RES_TPM_INFO_1_2);
            }
            // Display 'TPM firmware version'
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_VERSION, PpTpmInfo->wszVersionName);
            // Display 'TPM recovery support'
            if (!PpTpmInfo->sTpmState.attribs.tpm20restartRequired)
            {
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_FIRMWARE_RECOVERY_SUPPORT, PpTpmInfo->sTpmState.attribs.tpmSupportsFwRecovery ? RES_TPM_INFO_YES : RES_TPM_INFO_NO);
            }
            else
            {
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_FIRMWARE_RECOVERY_SUPPORT, RES_TPM_INFO_N_A);
            }
            // Display 'TPM firmware valid'
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_FIRMWARE_VALID, PpTpmInfo->sTpmState.attribs.tpmFirmwareIsValid ? RES_TPM_INFO_YES : RES_TPM_INFO_NO);
            // Display 'TPM operation mode'
            if (PpTpmInfo->sTpmState.attribs.tpmInOperationalMode)
            {
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_OPERATIONAL);
            }
            else
            {
                if (PpTpmInfo->sTpmState.attribs.tpmHasFULoader20)
                {
                    if (PpTpmInfo->sTpmState.attribs.tpmInFwUpdateMode)
                    {
                        CONSOLEIO_WRITE_BREAK_FMT_NO_NEWLINE(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_FWUPDATE);
                    }
                    else if (PpTpmInfo->sTpmState.attribs.tpmInFwRecoveryMode)
                    {
                        CONSOLEIO_WRITE_BREAK_FMT_NO_NEWLINE(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_FWRECOVERY);
                    }
                    else
                    {
                        CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_N_A);
                    }
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_OM_MODE_HEX, PpTpmInfo->sTpmState.attribs.tpm20OperationMode);
                }
                else
                {
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_FWUPDATE);
                }
            }
            // Display TPM1.2 specific information
            if (PpTpmInfo->sTpmState.attribs.tpm12)
            {
                if (!PpTpmInfo->sTpmState.attribs.tpm12FailedSelfTest)
                {
                    unReturnValueWrite = Response_ShowTpm12Info(PpTpmInfo->sTpmState.attribs);
                    if (RC_SUCCESS != unReturnValueWrite)
                        break;
                }
            }
            // Display 'TPM platformAuth'
            if (PpTpmInfo->sTpmState.attribs.tpm20)
            {
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_PLATFORM_AUTH, Response_GetPlatformAuthText(PpTpmInfo->sTpmState.attribs));
            }
        }
        // Display 'Remaining updates'
        if (PpTpmInfo->unRemainingUpdates != REMAINING_UPDATES_UNAVAILABLE) // -1
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_REMAINING_UPDATES_NUMBER, PpTpmInfo->unRemainingUpdates);
        }
        else
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_REMAINING_UPDATES_STRING, RES_TPM_INFO_N_A);
        }
        // Display 'Remaining updates (same version)'
        if (PpTpmInfo->sTpmState.attribs.tpmHasFULoader20)
        {
            // Display number of remaining updates (same version)
            if (PpTpmInfo->unRemainingUpdatesSelf != REMAINING_UPDATES_UNAVAILABLE) // -1
            {
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_REMAINING_UPDATES_SELF_NUMBER, PpTpmInfo->unRemainingUpdatesSelf);
            }
            else
            {
                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_REMAINING_UPDATES_SELF_STRING, RES_TPM_INFO_N_A);
            }
        }

        // Display information if TPM2.0 is in failure mode
        if (PpTpmInfo->sTpmState.attribs.tpm20InFailureMode)
        {
            const wchar_t wszDefaultTestResult[] = L"Unknown";
            wchar_t wszTestResult[256];
            unsigned int unTestResultLen = RG_LEN(wszTestResult);
            unReturnValue = Platform_StringCopy(wszTestResult, &unTestResultLen, wszDefaultTestResult);
            if (RC_SUCCESS != unReturnValue)
                break;
            CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
            CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_INFO_IN_FAILURE_MODE_1);
            if (PpTpmInfo->sTpmState.unTestResultLen > 0)
            {
                unTestResultLen = RG_LEN(wszTestResult);
                unReturnValue = UiUtility_StringScanByteToHex(PpTpmInfo->sTpmState.testResult, PpTpmInfo->sTpmState.unTestResultLen, wszTestResult, &unTestResultLen);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_IN_FAILURE_MODE_2, wszTestResult);
            CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_INFO_IN_FAILURE_MODE_3);
        }

        // Display information if TPM1.2 is in self-test failed mode
        if (PpTpmInfo->sTpmState.attribs.tpm12FailedSelfTest)
        {
            wchar_t wszTestResult[256];
            unsigned int unTestResultLen = RG_LEN(wszTestResult);
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszTestResult, RG_LEN(wszTestResult)));
            CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
            CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_INFO_IN_SELFTEST_FAILED_MODE_1);
            unReturnValue = UiUtility_StringScanByteToHex(PpTpmInfo->sTpmState.testResult, PpTpmInfo->sTpmState.unTestResultLen, wszTestResult, &unTestResultLen);
            if (RC_SUCCESS != unReturnValue)
                break;
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_IN_SELFTEST_FAILED_MODE_2, wszTestResult);
        }

        // Display information if TPM2.0 requires a restart
        if (PpTpmInfo->sTpmState.attribs.tpm20restartRequired)
        {
            CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
            CONSOLEIO_WRITE_BREAK(FALSE, RES_SYSTEM_RESTART_REQUIRED);
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    // Check if a ConsoleIO_Write error occurred and no other error has occurred then store it
    if (RC_SUCCESS == unReturnValue && RC_SUCCESS != unReturnValueWrite)
    {
        ERROR_STORE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
        unReturnValue = unReturnValueWrite;
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Show TPM Update output
 *  @details    Format TPM Update output and display
 *
 *  @param      PpTpmUpdate             Pointer to IfxUpdate response structure.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PpTpmUpdate is NULL or invalid.
 *  @retval     ...                     Error codes from ConsoleIO_Write functions.
 */
_Check_return_
unsigned int
Response_ShowUpdate(
    _In_    const IfxUpdate * PpTpmUpdate)
{
    unsigned int unReturnValue = RC_E_FAIL;
    unsigned int unReturnValueWrite = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check parameters
        if (NULL == PpTpmUpdate || STRUCT_TYPE_TpmUpdate != PpTpmUpdate->info.hdr.unType ||
                (STRUCT_SUBTYPE_UPDATE != PpTpmUpdate->unSubType &&
                 STRUCT_SUBTYPE_IS_UPDATABLE != PpTpmUpdate->unSubType &&
                 STRUCT_SUBTYPE_PREPARE != PpTpmUpdate->unSubType))
        {
            LOGGING_WRITE_LEVEL1(L"Error while checking object PpTpmUpdate: was invalid or NULL.");
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized (PpTpmUpdate)");
            break;
        }

        switch (PpTpmUpdate->unSubType)
        {
            case STRUCT_SUBTYPE_IS_UPDATABLE:
            {
                CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_INFORMATION);
                CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_DASHED_LINE);

                if (!PpTpmUpdate->info.sTpmState.attribs.tpm20 && !PpTpmUpdate->info.sTpmState.attribs.tpmInOperationalMode)
                {
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_FAMILY, RES_TPM_INFO_N_A);
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_VERSION, RES_TPM_INFO_N_A);
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_FIRMWARE_VALID, RES_TPM_INFO_NO);
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_FWUPDATE);
                }
                else
                {
                    // Display 'TPM family'
                    if (PpTpmUpdate->info.sTpmState.attribs.tpm12)
                    {
                        CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_FAMILY, RES_TPM_INFO_1_2);
                    }
                    else if (PpTpmUpdate->info.sTpmState.attribs.tpm20)
                    {
                        CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_FAMILY, RES_TPM_INFO_2_0);
                    }
                    // Display 'TPM firmware version'
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_VERSION, PpTpmUpdate->info.wszVersionName);
                    // Display 'TPM firmware valid'
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_FIRMWARE_VALID, PpTpmUpdate->info.sTpmState.attribs.tpmFirmwareIsValid ? RES_TPM_INFO_YES : RES_TPM_INFO_NO);
                    // Display 'TPM operation mode'
                    if (PpTpmUpdate->info.sTpmState.attribs.tpmInOperationalMode)
                    {
                        CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_OPERATIONAL);
                    }
                    else
                    {
                        if (PpTpmUpdate->info.sTpmState.attribs.tpmHasFULoader20)
                        {
                            if (PpTpmUpdate->info.sTpmState.attribs.tpmInFwUpdateMode)
                            {
                                CONSOLEIO_WRITE_BREAK_FMT_NO_NEWLINE(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_FWUPDATE);
                            }
                            else if (PpTpmUpdate->info.sTpmState.attribs.tpmInFwRecoveryMode)
                            {
                                CONSOLEIO_WRITE_BREAK_FMT_NO_NEWLINE(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_FWRECOVERY);
                            }
                            else
                            {
                                CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_N_A);
                            }
                            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_OM_MODE_HEX, PpTpmUpdate->info.sTpmState.attribs.tpm20OperationMode);
                        }
                        else
                        {
                            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_TPM_OPERATION_MODE, RES_TPM_INFO_OM_FWUPDATE);
                        }
                    }
                    if (PpTpmUpdate->info.sTpmState.attribs.tpm12 && !PpTpmUpdate->info.sTpmState.attribs.tpm12FailedSelfTest)
                    {
                        // Show additional TPM1.2 data
                        unReturnValueWrite = Response_ShowTpm12Info(PpTpmUpdate->info.sTpmState.attribs);
                        if (RC_SUCCESS != unReturnValueWrite)
                            break;
                    }
                    // Display 'platformAuth'
                    if (PpTpmUpdate->info.sTpmState.attribs.tpm20)
                    {
                        CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_PLATFORM_AUTH, Response_GetPlatformAuthText(PpTpmUpdate->info.sTpmState.attribs));
                    }
                }
                // // Display 'Remaining updates'
                if (PpTpmUpdate->info.unRemainingUpdates != REMAINING_UPDATES_UNAVAILABLE) // -1
                {
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_REMAINING_UPDATES_NUMBER, PpTpmUpdate->info.unRemainingUpdates);
                }
                else
                {
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_REMAINING_UPDATES_STRING, RES_TPM_INFO_N_A);
                }
                // Display 'Remaining updates (same version)'
                if (PpTpmUpdate->info.sTpmState.attribs.tpmHasFULoader20)
                {
                    if (PpTpmUpdate->info.unRemainingUpdatesSelf != REMAINING_UPDATES_UNAVAILABLE) // -1
                    {
                        CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_REMAINING_UPDATES_SELF_NUMBER, PpTpmUpdate->info.unRemainingUpdatesSelf);
                    }
                    else
                    {
                        CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_INFO_REMAINING_UPDATES_SELF_STRING, RES_TPM_INFO_N_A);
                    }
                }
                // Display 'New firmware valid for TPM'
                if (GENERIC_TRISTATE_STATE_YES == PpTpmUpdate->unNewFirmwareValid)
                {
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_UPDATE_UPDATABLE, RES_TPM_INFO_YES);
                }
                else if (GENERIC_TRISTATE_STATE_NO == PpTpmUpdate->unNewFirmwareValid)
                {
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_UPDATE_UPDATABLE, RES_TPM_INFO_NO);
                }
                else
                {
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_UPDATE_UPDATABLE, RES_TPM_INFO_N_A);
                }

                if (RC_E_TPM20_FAILURE_MODE == PpTpmUpdate->info.hdr.unReturnCode)
                    break;

                if (RC_SUCCESS == PpTpmUpdate->info.hdr.unReturnCode)
                {
                    // Display 'TPM family after update'
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_UPDATE_TPM_FAMILY_AFTER, PpTpmUpdate->bTargetFamily == DEVICE_TYPE_TPM_12 ? RES_TPM_INFO_1_2 : RES_TPM_INFO_2_0);
                    // Display 'TPM firmware version after update'
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_UPDATE_TPM_VERSION_AFTER, PpTpmUpdate->wszNewFirmwareVersion);
                    if (PpTpmUpdate->bfNewTpmFirmwareInfo.factoryDefaults)
                    {
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_FACTORYDEFAULT);
                    }
                }
                else if (RC_E_ALREADY_UP_TO_DATE == PpTpmUpdate->info.hdr.unReturnCode)
                {
                    CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                    CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_ALREADY_UP_TO_DATE);
                }
                if (!PLATFORM_STRING_IS_NULL_OR_EMPTY(PpTpmUpdate->wszUsedFirmwareImage))
                {
                    CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                    CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM_USED_FIRMWARE_FILE, PpTpmUpdate->wszUsedFirmwareImage);
                }
                break;
            }
            case STRUCT_SUBTYPE_PREPARE:
            {
                CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE);

                // Chip in non-operational mode?
                if (!PpTpmUpdate->info.sTpmState.attribs.tpmInOperationalMode)
                {
                    if (RC_SUCCESS == PpTpmUpdate->info.hdr.unReturnCode)
                    {
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_SKIP);
                        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_DO_NOT_TURN_OFF);
                        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_UPDATE);
                    }
                    else
                    {
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_SKIP);
                    }
                }
                // TPM2.0 Preparation
                else if (PpTpmUpdate->info.sTpmState.attribs.tpm20)
                {
                    if (RC_SUCCESS == PpTpmUpdate->info.hdr.unReturnCode)
                    {
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_POLICY);
                        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_DO_NOT_TURN_OFF);
                        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_UPDATE);
                    }
                    else
                    {
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_POLICY_FAIL);
                    }
                }
                // TPM1.2 Preparation
                else if (PpTpmUpdate->info.sTpmState.attribs.tpm12)
                {
                    unsigned int unUpdateType = UPDATE_TYPE_NONE;
                    if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, &unUpdateType))
                    {
                        unReturnValue = RC_E_FAIL;
                        ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_UPDATE_TYPE);
                        break;
                    }

                    if (RC_SUCCESS == PpTpmUpdate->info.hdr.unReturnCode)
                    {
                        if (UPDATE_TYPE_TPM12_DEFERREDPP == unUpdateType)
                        {
                            if (1 == PpTpmUpdate->info.sTpmState.attribs.tpm12DeferredPhysicalPresence)
                            {
                                CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_PP_EXT);
                            }
                            else
                            {
                                CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_PP);
                            }
                        }
                        else if (UPDATE_TYPE_TPM12_TAKEOWNERSHIP == unUpdateType)
                        {
                            CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_TAKEOWNERSHIP);
                        }
                        else if (UPDATE_TYPE_TPM12_OWNERAUTH == unUpdateType)
                        {
                            CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_OWNERAUTH);
                        }

                        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_DO_NOT_TURN_OFF);
                        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_UPDATE);
                    }
                    else
                    {
                        if (UPDATE_TYPE_TPM12_DEFERREDPP == unUpdateType)
                        {
                            CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_PP_FAIL);
                        }
                        else if (UPDATE_TYPE_TPM12_TAKEOWNERSHIP == unUpdateType)
                        {
                            CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_TAKEOWNERSHIP_FAIL);
                        }
                        else if (UPDATE_TYPE_TPM12_OWNERAUTH == unUpdateType)
                        {
                            if (RC_E_TPM12_DISABLED_DEACTIVATED == PpTpmUpdate->info.hdr.unReturnCode)
                            {
                                CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_OWNERAUTH_FAIL_DISABLED);
                                CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                                CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_DO_NOT_TURN_OFF);
                                CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                                CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_UPDATE);
                            }
                            else
                            {
                                CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_PREPARE_OWNERAUTH_FAIL);
                            }
                        }
                    }
                }

                break;
            }
            case STRUCT_SUBTYPE_UPDATE:
            {
                CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                if (RC_SUCCESS == PpTpmUpdate->info.hdr.unReturnCode)
                {
                    CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_SUCCESS);
                    CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
                    CONSOLEIO_WRITE_BREAK(FALSE, RES_SYSTEM_RESTART_REQUIRED);
                }
                else
                {
                    CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM_UPDATE_FAIL);
                }

                break;
            }
            default:
                break;
        }
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    // Check if a ConsoleIO_Write error occurred and no other error has occurred then store it
    if (RC_SUCCESS == unReturnValue && RC_SUCCESS != unReturnValueWrite)
    {
        ERROR_STORE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
        unReturnValue = unReturnValueWrite;
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, RC_SUCCESS);

    return unReturnValue;
}

/**
 *  @brief      Show TPM1.2 ClearOwnership output
 *  @details    Format TPM1.2 ClearOwnership output and display
 *
 *  @param      PpTpm12ClearOwnership   Pointer to a IfxTpm12ClearOwnership response structure.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PpTpm12ClearOwnership was invalid.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
Response_ShowClearOwnership(
    _In_    const IfxTpm12ClearOwnership* PpTpm12ClearOwnership)
{
    unsigned int unReturnValue = RC_E_FAIL;
    unsigned int unReturnValueWrite = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check parameters
        if (NULL == PpTpm12ClearOwnership || PpTpm12ClearOwnership->unType != STRUCT_TYPE_Tpm12ClearOwnership)
        {
            LOGGING_WRITE_LEVEL1(L"Error while checking object PpTpm12ClearOwnership: was invalid or NULL.");
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized (PpTpm12ClearOwnership)");
            break;
        }

        if (RC_SUCCESS != unReturnValueWrite)
            break;

        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM12_CLEAR_OWNER_INFORMATION);
        CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM12_CLEAR_OWNER_DASHED_LINE);

        if (RC_SUCCESS == PpTpm12ClearOwnership->unReturnCode)
        {
            CONSOLEIO_WRITE_BREAK(FALSE, RES_TPM12_CLEAR_OWNER_SUCCESS);
        }
        else
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_TPM12_CLEAR_OWNER_FAILED, PpTpm12ClearOwnership->unReturnCode);
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    // Check if a ConsoleIO_Write error occurred and no other error has occurred then store it
    if (RC_SUCCESS == unReturnValue && RC_SUCCESS != unReturnValueWrite)
    {
        ERROR_STORE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
        unReturnValue = unReturnValueWrite;
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Show SetMode output
 *  @details    Format SetMode output and display
 *
 *  @param      PpSetMode               Pointer to a IfxTpm12ClearOwnership response structure.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PpTpm12ClearOwnership was invalid.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
Response_ShowSetMode(
    _In_    const IfxSetMode* PpSetMode)
{
    unsigned int unReturnValue = RC_E_FAIL;
    unsigned int unReturnValueWrite = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check parameters
        if (NULL == PpSetMode || PpSetMode->hdr.unType != STRUCT_TYPE_SetMode)
        {
            LOGGING_WRITE_LEVEL1(L"Error while checking object PpSetMode: was invalid or NULL.");
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized (PpSetMode)");
            break;
        }

        if (RC_SUCCESS != unReturnValueWrite)
            break;

        if (RC_SUCCESS == PpSetMode->hdr.unReturnCode)
        {
            CONSOLEIO_WRITE_BREAK(FALSE, RES_SET_MODE_SUCCESS);
        }
        else
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_SET_MODE_FAILED, PpSetMode->hdr.unReturnCode);
        }

        // Get setmode type
        ENUM_SETMODE_TYPES unSetModeType = SETMODE_TYPE_NONE;
        if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_SETMODE_TYPE, (unsigned int*)&unSetModeType))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"Unexpected error calling PropertyStorage_GetUIntegerValueByKey.");
            break;
        }

        // Check if system restart message is required
        if (SETMODE_TYPE_TPM20_OPERATIONAL == unSetModeType)
        {
            CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
            CONSOLEIO_WRITE_BREAK(FALSE, RES_SYSTEM_RESTART_REQUIRED);
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    // Check if a ConsoleIO_Write error occurred and no other error has occurred then store it
    if (RC_SUCCESS == unReturnValue && RC_SUCCESS != unReturnValueWrite)
    {
        ERROR_STORE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
        unReturnValue = unReturnValueWrite;
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Show Unknown Action info
 *  @details    Displays the output for an unknown action to the console
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     ...                     Error Codes from ConsoleIO_Write function.
 */
_Check_return_
unsigned int
Response_ShowUnknownAction()
{
    unsigned int unReturnValue = RC_E_FAIL;
    // Must be success because of breaking out before reached the first call
    unsigned int unReturnValueWrite = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Write error information to console
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_SEPARATOR_LINE);
        CONSOLEIO_WRITE_BREAK(FALSE, RES_UNKNOWN_ACTION);
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_SEPARATOR_LINE);
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
        CONSOLEIO_WRITE_BREAK(FALSE, L"Skipped display of an unrecognized command.");
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValueWrite)
    {
        if (RC_SUCCESS == unReturnValue)
        {
            ERROR_STORE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
            unReturnValue = unReturnValueWrite;
        }
        else
        {
            ERROR_LOGCODEANDMESSAGE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
        }
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Show Error output
 *  @details    Formats the error output and display it to the user
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from ConsoleIO_Write functions.
 */
_Check_return_
unsigned int
Response_ShowError()
{
    unsigned int unReturnValue = RC_E_FAIL;
    // Must be success because of breaking out before reached the first call
    unsigned int unReturnValueWrite = RC_SUCCESS;
    wchar_t* wszFormatedMessage = NULL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        BOOL fLogFileAvailable = TRUE;
        IfxErrorData* pErrorData = NULL;
        wchar_t wszBuffer[MAX_MESSAGE_SIZE];
        unsigned int unFormatedMessageSize = 0;
        unsigned int unSize = RG_LEN(wszBuffer);
        unsigned int unFinalCode = RC_E_FAIL;
        unsigned int unInternalCode = RC_E_FAIL;
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszBuffer, RG_LEN(wszBuffer)));

        // Check if an error exists
        pErrorData = Error_GetStack();
        if (NULL == pErrorData)
        {
            // If not do nothing
            unReturnValue = RC_SUCCESS;
            break;
        }

        // Check if log file path is available
        unReturnValue = UiUtility_CheckIfLogPathWritable(TRUE);
        if (RC_SUCCESS != unReturnValue && (
                    RC_E_FILE_NOT_FOUND == unReturnValue ||
                    RC_E_ACCESS_DENIED == unReturnValue))
        {
            fLogFileAvailable = FALSE;
        }

        // Get Final Error Message
        unReturnValue = Error_GetFinalMessage(wszBuffer, &unSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Format error message
        unReturnValue = UiUtility_StringFormatOutput(
                            wszBuffer, unSize + 1,
                            RES_TPM_ERROR_EXPLANATION_PRESET, RG_LEN(RES_TPM_ERROR_EXPLANATION_PRESET), RES_MAX_LINE_SIZE,
                            &wszFormatedMessage, &unFormatedMessageSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Add Message Header
        unReturnValue = Platform_MemoryCopy(
                            (void*)wszFormatedMessage, unFormatedMessageSize * sizeof(wszFormatedMessage[0]),
                            RES_ERROR_MESSAGE_PRESET, sizeof(RES_ERROR_MESSAGE_PRESET) - sizeof(wchar_t));
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an error");
            break;
        }

        // Show response header
        if (!Response_HeaderShown())
        {
            unReturnValue = Response_ShowHeader();
            if (RC_SUCCESS != unReturnValue)
                break;
        }

        // Get the error codes from store
        unFinalCode = Error_GetFinalCode();
        unInternalCode = Error_GetInternalCode();

        // Write error information to console
        CONSOLEIO_WRITE_BREAK(FALSE, L"");
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_SEPARATOR_LINE);
        CONSOLEIO_WRITE_BREAK(FALSE, RES_ERROR_INFO);
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_SEPARATOR_LINE);
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_ERROR_CODE, unFinalCode);
        CONSOLEIO_WRITE_BREAK(FALSE, wszFormatedMessage);

        // If log file is not available add detailed error message
        if (!fLogFileAvailable)
        {
            // Cleanup allocated buffer before reuse the variable
            Platform_MemoryFree((void**)&wszFormatedMessage);

            unReturnValue = Platform_StringGetLength(pErrorData->wszInternalErrorMessage, RG_LEN(pErrorData->wszInternalErrorMessage), &unSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Format error message
            unReturnValue = UiUtility_StringFormatOutput(
                                pErrorData->wszInternalErrorMessage, unSize + 1,
                                RES_TPM_ERROR_EXPLANATION_PRESET, RG_LEN(RES_TPM_ERROR_EXPLANATION_PRESET), RES_MAX_LINE_SIZE,
                                &wszFormatedMessage, &unFormatedMessageSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Add Message Header
            unReturnValue = Platform_MemoryCopy(
                                (void*)wszFormatedMessage, unFormatedMessageSize * sizeof(wszFormatedMessage[0]),
                                RES_TPM_ERROR_EXPLANATION, sizeof(RES_TPM_ERROR_EXPLANATION) - sizeof(wchar_t));
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an error");
                break;
            }
            CONSOLEIO_WRITE_BREAK(FALSE, wszFormatedMessage);
        }

        // Brackets must be set here, because of macro use!
        if (unInternalCode != unFinalCode)
        {
            CONSOLEIO_WRITE_BREAK_FMT(FALSE, RES_ERROR_DETAILS, unInternalCode);

            // In case of an TPM error -> get detailed error message
            if (RC_E_TPM_GENERAL == unFinalCode)
            {
                unReturnValue = Response_ShowTpmErrorExplanation(unInternalCode);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }
        }
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);

        // Cleanup allocated buffer before reuse the variable
        Platform_MemoryFree((void**)&wszFormatedMessage);

        if (TRUE == fLogFileAvailable)
        {
            wchar_t wszLoggingPath[PROPERTY_STORAGE_MAX_VALUE];
            unsigned int unLoggingPathSize = RG_LEN(wszLoggingPath);
            unSize = RG_LEN(wszBuffer);
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszLoggingPath, RG_LEN(wszLoggingPath)));

            // Get the logging path from property storage
            if (!(PropertyStorage_ExistsElement(PROPERTY_LOGGING_PATH) &&
                    PropertyStorage_GetValueByKey(PROPERTY_LOGGING_PATH, wszLoggingPath, &unLoggingPathSize)))
            {
                // Calculate default log file path
                wchar_t wszDefaultLogPath[MAX_PATH];
                unsigned int unCapacity = RG_LEN(wszDefaultLogPath);
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszDefaultLogPath, RG_LEN(wszDefaultLogPath)));

                unReturnValue = Platform_StringCopy(wszDefaultLogPath, &unCapacity, GwszToolName);
                if (RC_SUCCESS != unReturnValue)
                    break;
                unCapacity = RG_LEN(wszDefaultLogPath);
                unReturnValue = Platform_StringConcatenate(wszDefaultLogPath, &unCapacity, L".log");
                if (RC_SUCCESS != unReturnValue)
                    break;

                // Use default logging path on error
                unLoggingPathSize = RG_LEN(wszLoggingPath);
                unReturnValue = Platform_StringCopy(wszLoggingPath, &unLoggingPathSize, wszDefaultLogPath);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }

            unReturnValue = Platform_StringFormat(wszBuffer, &unSize, RES_ERROR_LOGFILE_HINT, wszLoggingPath);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Format error message
            unReturnValue = UiUtility_StringFormatOutput(
                                wszBuffer, unSize + 1,
                                L"  ", RG_LEN(L"  "), RES_MAX_LINE_SIZE,
                                &wszFormatedMessage, &unFormatedMessageSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            CONSOLEIO_WRITE_BREAK(FALSE, wszFormatedMessage);
        }
        else
        {
            // Show hint only for return codes other than the RC_E_INVALID_LOG_OPTION because this code already showed the hint.
            if (RC_E_INVALID_LOG_OPTION != unFinalCode)
            {
                // Format error message
                unReturnValue = UiUtility_StringFormatOutput(
                                    RES_ERROR_LOGFILE_UNAVAILABLE, RG_LEN(RES_ERROR_LOGFILE_UNAVAILABLE),
                                    L"  ", RG_LEN(L"  "), RES_MAX_LINE_SIZE,
                                    &wszFormatedMessage, &unFormatedMessageSize);
                if (RC_SUCCESS != unReturnValue)
                    break;

                CONSOLEIO_WRITE_BREAK(FALSE, wszFormatedMessage);
            }
        }

        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    // Cleanup allocated buffer
    Platform_MemoryFree((void**)&wszFormatedMessage);

    if (RC_SUCCESS != unReturnValueWrite)
    {
        if (RC_SUCCESS == unReturnValue)
        {
            ERROR_STORE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
            unReturnValue = unReturnValueWrite;
        }
        else
        {
            ERROR_LOGCODEANDMESSAGE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
        }
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Show TPM explanation to a TPM Error
 *  @details
 *
 *  @param      PunInternalCode         TPM masked error code.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 */
_Check_return_
unsigned int
Response_ShowTpmErrorExplanation(
    _In_    unsigned int PunInternalCode)
{
    unsigned int unReturnValue = RC_E_FAIL;
    // Must be success because of breaking out before reached the first call
    unsigned int unReturnValueWrite = RC_SUCCESS;
    wchar_t* wszFormatedOutput = NULL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        unsigned int unFormatedOutputSize = 0;
        wchar_t wszTpmResponseMessage[MAX_MESSAGE_SIZE];
        unsigned int unTpmResponseMessage = RG_LEN(wszTpmResponseMessage);
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszTpmResponseMessage, RG_LEN(wszTpmResponseMessage)));

        // Get TPM Response
        unReturnValue = TpmResponse_GetMessage(PunInternalCode, wszTpmResponseMessage, unTpmResponseMessage);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"TpmResponse_GetMessage returned an error");
            break;
        }
        // Calculate the length of the response
        unReturnValue = Platform_StringGetLength(wszTpmResponseMessage, unTpmResponseMessage, &unTpmResponseMessage);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_StringGetLength returned an error");
            break;
        }

        unReturnValue = UiUtility_StringFormatOutput(
                            wszTpmResponseMessage, unTpmResponseMessage,
                            RES_TPM_ERROR_EXPLANATION_PRESET, RG_LEN(RES_TPM_ERROR_EXPLANATION_PRESET), RES_MAX_LINE_SIZE,
                            &wszFormatedOutput, &unFormatedOutputSize);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Utility_StringFormatOutput returned an error");
            break;
        }

        unReturnValue = Platform_MemoryCopy(
                            (void*)wszFormatedOutput, unFormatedOutputSize * sizeof(wszFormatedOutput[0]),
                            RES_TPM_ERROR_EXPLANATION, sizeof(RES_TPM_ERROR_EXPLANATION) - sizeof(wchar_t));
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an error");
            break;
        }

        CONSOLEIO_WRITE_BREAK(FALSE, wszFormatedOutput);
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValueWrite)
    {
        if (RC_SUCCESS == unReturnValue)
        {
            ERROR_STORE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
            unReturnValue = unReturnValueWrite;
        }
        else
        {
            ERROR_LOGCODEANDMESSAGE(unReturnValueWrite, L"ConsoleIO_Write returned an error");
        }
    }

    // Cleanup allocated buffer
    Platform_MemoryFree((void**)&wszFormatedOutput);

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Print the help information
 *  @details
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       An unexpected error occurred.
 *  @retval     ...             Error codes from ConsoleIO_Write functions.
 */
_Check_return_
unsigned int
Response_ShowHelp()
{
    unsigned int unReturnValueWrite = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Show the help
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE01, GwszToolName);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE02);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE03);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE04, CMD_HELP_ALT, CMD_HELP);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE05, GwszToolName);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE06);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE07, CMD_INFO);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE08);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE09, CMD_UPDATE, CMD_FIRMWARE, CMD_CONFIG, CMD_TPM12_CLEAROWNERSHIP);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE10, CMD_SETMODE, CMD_FORCE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE11, CMD_UPDATE);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE12);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE13);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE14, CMD_UPDATE_OPTION_TPM12_DEFERREDPP);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE15, CMD_UPDATE_OPTION_TPM12_OWNERAUTH);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE16, CMD_OWNERAUTH);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE17, CMD_UPDATE_OPTION_TPM12_TAKEOWNERSHIP, GwszToolName);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE17A, CMD_OWNERAUTH);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE17B);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE18, CMD_UPDATE_OPTION_TPM20_EMPTYPLATFORMAUTH);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE19, CMD_UPDATE_OPTION_CONFIG_FILE);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE20);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE21, CMD_CONFIG);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE22, CMD_INFO, CMD_TPM12_CLEAROWNERSHIP, CMD_SETMODE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE23, CMD_FIRMWARE);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE24);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE25, CMD_UPDATE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE27, CMD_CONFIG);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE28, CMD_UPDATE, CMD_UPDATE_OPTION_CONFIG_FILE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE29, CMD_UPDATE, CMD_UPDATE_OPTION_CONFIG_FILE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE31, CMD_LOG);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE32, GwszToolName);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE33, GwszToolName);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE34);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE35);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE36, CMD_TPM12_CLEAROWNERSHIP);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE37, GwszToolName);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE37A, CMD_OWNERAUTH);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE38, CMD_INFO, CMD_UPDATE, CMD_FIRMWARE, CMD_CONFIG, CMD_SETMODE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE38A, CMD_FORCE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE39, CMD_OWNERAUTH);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE40);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE41, CMD_UPDATE_OPTION_TPM12_OWNERAUTH);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE42, CMD_SETMODE);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE43);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE43A);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE43B, CMD_SETMODE_OPTION_TPM20_FW_UPDATE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE43C, CMD_FIRMWARE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE43D, CMD_SETMODE_OPTION_TPM20_FW_RECOVERY);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE43E, CMD_SETMODE_OPTION_TPM20_OPERATIONAL);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE44, CMD_INFO, CMD_UPDATE, CMD_TPM12_CLEAROWNERSHIP, CMD_CONFIG);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE44A, CMD_FORCE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_FORCE1, CMD_FORCE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_FORCE2, CMD_FORCE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_FORCE3, CMD_UPDATE);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_FORCE4, CMD_INFO, CMD_TPM12_CLEAROWNERSHIP, CMD_SETMODE);
#ifdef LINUX
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, HELP_LINE45, CMD_ACCESS_MODE);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE46);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE47);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE48);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE49);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE50);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE51);
        CONSOLEIO_WRITE_BREAK(FALSE, HELP_LINE52);
#endif // LINUX
    }
    WHILE_FALSE_END;

    // Check if a ConsoleIO_Write error occurred and store it
    if (RC_SUCCESS != unReturnValueWrite)
        ERROR_STORE(unReturnValueWrite, L"ConsoleIO_Write returned an error");

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValueWrite);

    return unReturnValueWrite;
}

/**
 *  @brief      Print the TPMFactoryUpd tool header
 *  @details
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from ConsoleIO_Write function call.
 */
_Check_return_
unsigned int
Response_ShowHeader()
{
    unsigned int unReturnValueWrite = RC_E_FAIL;
    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Print Header to the screen
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_SEPARATOR);
        CONSOLEIO_WRITE_BREAK_FMT(FALSE, MENU_HEADLINE, IFX_BRAND, GwszToolName, MENU_VERSION, APP_VERSION);
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_SEPARATOR);
        CONSOLEIO_WRITE_BREAK(FALSE, MENU_NEWLINE);

        s_fHeaderShown = TRUE;
    }
    WHILE_FALSE_END;

    // Check if a ConsoleIO_Write error occurred and store it
    if (RC_SUCCESS != unReturnValueWrite)
        ERROR_STORE(unReturnValueWrite, L"ConsoleIO_Write returned an error");

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValueWrite);

    return unReturnValueWrite;
}

/**
 *  @brief      Returns whether the header has been shown
 *  @details
 *
 *  @retval     TRUE        Header has been shown.
 *  @retval     FALSE       Header has not been shown.
 */
_Check_return_
BOOL
Response_HeaderShown()
{
    return s_fHeaderShown;
}

/**
 *  @brief      Callback function for progress report of EFI_FIRMWARE_MANAGEMENT_PROTOCOL.SetImage().
 *  @details    The function is called by EFI_FIRMWARE_MANAGEMENT_PROTOCOL.SetImage() to update the progress (1 - 100). It prints the
 *              progress to the console.
 *
 *  @param      PullCompletion  Progress completion value between 1 and 100.
 *
 *  @retval     0               The callback executed successfully.
 */
_Check_return_
unsigned long long
IFXAPI
Response_ProgressCallback(
    _In_ unsigned long long PullCompletion)
{
    unsigned int unProgress = (unsigned int) PullCompletion;
    IGNORE_RETURN_VALUE(ConsoleIO_Write(FALSE, FALSE, RES_TPM_UPDATE_PROGRESS, unProgress));

    return 0;
}
