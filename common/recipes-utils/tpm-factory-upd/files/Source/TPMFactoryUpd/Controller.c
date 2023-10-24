/**
 *  @brief      Implements the controller module for TPMFactoryUpd.
 *  @details    This module controls the TPMFactoryUpd view and business layers.
 *  @file       Controller.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Controller.h"
#include "CommandLineParser.h"
#include "Response.h"
#include "CommandFlow_SetMode.h"
#include "CommandFlow_TpmInfo.h"
#include "CommandFlow_TpmUpdate.h"
#include "CommandFlow_Tpm12ClearOwnership.h"
#include "Platform.h"

/**
 *  @brief      This function shows the response output
 *  @details
 *
 *  @param      PpTpm2ToolHeader    Response structure.
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_ShowResponse(
    _In_    IfxToolHeader*  PpTpm2ToolHeader)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Show Response
        unReturnValue = Response_Show(PpTpm2ToolHeader);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      This function controls the TPMFactoryUpd view and business layers.
 *  @details    This function handles the program flow between UI and business modules.
 *
 *  @param      PnArgc          Parameter as provided in main().
 *  @param      PrgwszArgv      Parameter as provided in main().
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_Proceed(
    _In_                    int                     PnArgc,
    _In_reads_z_(PnArgc)    const wchar_t* const    PrgwszArgv[])
{
    unsigned int    unReturnValue = RC_E_FAIL;
    unsigned int    unReturnValueError = RC_E_FAIL;
    IfxToolHeader*  pResponseData = NULL;

    // First enable caching of log messages
    Logging_EnableCaching();

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Store the tool name in global variable. It will be used for command line output and logging.
        unsigned int unCapacity = RG_LEN(GwszToolName);
        unReturnValue = Platform_StringCopy(GwszToolName, &unCapacity, L"TPMFactoryUpd");
        if (RC_SUCCESS != unReturnValue)
            break;

#if LINUX
        // Allow overriding the tool name for Linux
        if (PnArgc > 0)
        {
            // Get the file name from the full path
            const wchar_t* pwszStart = PrgwszArgv[0];
            wchar_t* pwszPos = NULL;
            while(RC_SUCCESS == Platform_FindString(L"/", pwszStart, &pwszPos))
            {
                pwszStart += 1;
            }

            // Store as tool name
            unCapacity = RG_LEN(GwszToolName);
            unReturnValue = Platform_StringCopy(GwszToolName, &unCapacity, pwszStart);
            if (RC_SUCCESS != unReturnValue)
                break;
        }
#endif // LINUX

        // Initialize controller and device
        unReturnValue = Controller_Initialize(PnArgc, PrgwszArgv);
        if (RC_SUCCESS != unReturnValue)
            break;

        unReturnValue = Controller_ProceedWork(&pResponseData);
    }
    WHILE_FALSE_END;

    // Call uninitialize also in an error case because some parts of the
    // Initialize method could have been executed successfully e.g. DeviceManagement_Connect
    unReturnValueError = Controller_Uninitialize();
    if (RC_SUCCESS != unReturnValueError)
    {
        // Check if we would cover an error case with leaving the error on the stack
        if (RC_SUCCESS != unReturnValue)
        {
            // We will cover one detailed error so remove the last one
            Error_ClearFirstItem();
        }
        else
        {
            unReturnValue = unReturnValueError;
        }
    }

    // Check if structure type is TpmUpdate to free allocated file buffer memory
    if (NULL != pResponseData && STRUCT_TYPE_TpmUpdate == pResponseData->unType)
    {
        // Hint: Usage of additional (void*) cast due to clang warning "-Wcast-align": ... increases required alignment from 4 to 8
        // Since pResponseData (IfxToolHeader*) contains casted pointer to IfxUpdate structure (correctly allocated and aligned), casting the pointer back via '(void*)' is safe
        Platform_MemoryFree((void**) & (((IfxUpdate*)(void*)pResponseData)->rgbFirmwareImage));
    }

    // Free allocated memory
    Platform_MemoryFree((void**)&pResponseData);

    // Do error handling now
    if (NULL != Error_GetStack())
    {
        // Get Final Error Code
        unReturnValue = Error_GetFinalCode();
        if (RC_E_BAD_COMMANDLINE == unReturnValue)
        {
            // Show header
            unReturnValueError = Response_ShowHeader();

            // Show help in case of bad command line error instead of error output
            unReturnValueError |= Response_ShowHelp();
        }
        else
        {
            // Show error output
            unReturnValueError = Response_ShowError();
            if (RC_SUCCESS != unReturnValueError)
                LOGGING_WRITE_LEVEL1_FMT(L"An error occurred during error handling. Response_ShowError() failed. (0x%.8X)", unReturnValueError);
        }

        // Check response and cleanup
        if (RC_SUCCESS != unReturnValueError)
            LOGGING_WRITE_LEVEL1_FMT(L"An error occurred during error handling. (0x%.8X)", unReturnValueError);

        // Log Error Stack
        Error_LogStack();

        // Clear Error stack
        Error_ClearStack();
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    // Finally close log file
    IGNORE_RETURN_VALUE(Logging_Finalize());

    return unReturnValue;
}

/**
 *  @brief      This function controls the TPMFactoryUpd view and business layers regarding the provided command line.
 *  @details    This function handles the program flow between UI and business modules.
 *
 *  @param      PppResponseData     Pointer to a IfxToolHeader structure; inner pointer
 *                                  must be initialized with NULL or allocated on the heap!
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_ProceedWork(
    _Inout_ IfxToolHeader** PppResponseData)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        unsigned int unCmdLineOptionCount = 0;
        BOOL fValue = FALSE;

        // Check parameter
        if (NULL == PppResponseData)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter PppResponseData is invalid.");
            break;
        }

        // Get command line option count
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_CMDLINE_COUNT) &&
                FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_CMDLINE_COUNT, &unCmdLineOptionCount))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_CMDLINE_COUNT);
            break;
        }

        unReturnValue = Response_ShowHeader();
        if (RC_SUCCESS != unReturnValue)
            break;

        // Check if Show help is set
        if ((TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_HELP, &fValue) && TRUE == fValue) ||
                0 == unCmdLineOptionCount)
        {
            unReturnValue = Response_ShowHelp();
            break;
        }

        // Check if Info is set
        if (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_INFO, &fValue) && TRUE == fValue)
        {
            // Allocate memory
            Platform_MemoryFree((void**)PppResponseData);
            *PppResponseData = (IfxToolHeader*)Platform_MemoryAllocateZero(sizeof(IfxInfo));
            if (NULL == *PppResponseData)
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE(unReturnValue, L"Error detected in Controller_ProceedWork: Memory allocation failed.");
                break;
            }
            // Execute command
            (*PppResponseData)->unSize = sizeof(IfxInfo);
            (*PppResponseData)->unType = STRUCT_TYPE_TpmInfo;

            unReturnValue = CommandFlow_TpmInfo_Execute((IfxInfo*)*PppResponseData);
            if (RC_SUCCESS != unReturnValue &&
                    RC_E_UNSUPPORTED_CHIP != unReturnValue)
                break;

            // Show command response
            unReturnValue = Controller_ShowResponse(*PppResponseData);
            break;
        }

        // Check if Update is set
        if (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_UPDATE, &fValue) && TRUE == fValue)
        {
            unsigned int unUpdateType = UPDATE_TYPE_NONE;

            // Allocate memory
            Platform_MemoryFree((void**)PppResponseData);
            IfxUpdate* pIfxUpdate = (IfxUpdate*)Platform_MemoryAllocateZero(sizeof(IfxUpdate));
            if (NULL == pIfxUpdate)
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE(unReturnValue, L"Error detected in Controller_ProceedWork: Memory allocation failed.");
                break;
            }
            *PppResponseData = (IfxToolHeader*)pIfxUpdate;

            // Get the TPM information and use the update structure to store the data
            (*PppResponseData)->unType = STRUCT_TYPE_TpmInfo;
            (*PppResponseData)->unSize = sizeof(IfxInfo);
            unReturnValue = CommandFlow_TpmInfo_Execute(&pIfxUpdate->info);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Execute command
            (*PppResponseData)->unSize = sizeof(IfxUpdate);
            (*PppResponseData)->unType = STRUCT_TYPE_TpmUpdate;

            // Get property "update type"
            if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, &unUpdateType))
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_UPDATE_TYPE);
                break;
            }

            // If update type is configuration file parse configuration file
            if (UPDATE_TYPE_CONFIG_FILE == unUpdateType)
            {
                unReturnValue = CommandFlow_TpmUpdate_ProceedUpdateConfig(pIfxUpdate);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }

            // Check if firmware is updatable with the given image
            if (RC_E_ALREADY_UP_TO_DATE != (*PppResponseData)->unReturnCode)
            {
                unReturnValue = CommandFlow_TpmUpdate_IsFirmwareUpdatable(pIfxUpdate);
                if (RC_SUCCESS != unReturnValue)
                    break;

                // Check if firmware update to the same version is allowed
                if (0 == Platform_StringCompare(pIfxUpdate->wszNewFirmwareVersion, pIfxUpdate->info.wszVersionName, MAX_NAME, FALSE))
                {
                    // Get force parameter value
                    if (FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_FORCE, &fValue))
                    {
                        fValue = FALSE;
                    }

                    // Reject update in case of missing force parameter (only in operational mode)
                    if ((FALSE == fValue && pIfxUpdate->info.sTpmState.attribs.tpmInOperationalMode))
                        (*PppResponseData)->unReturnCode = RC_E_ALREADY_UP_TO_DATE;
                }
            }

            unReturnValue = Controller_ShowResponse(*PppResponseData);
            if (RC_SUCCESS != unReturnValue)
                break;

            if (RC_SUCCESS != (*PppResponseData)->unReturnCode)
            {
                if (RC_E_ALREADY_UP_TO_DATE == (*PppResponseData)->unReturnCode)
                    unReturnValue = RC_SUCCESS;
                else
                    unReturnValue = (*PppResponseData)->unReturnCode;
                break;
            }

            // Do preparation steps
            unReturnValue = CommandFlow_TpmUpdate_PrepareFirmwareUpdate(pIfxUpdate);
            if (RC_SUCCESS != unReturnValue)
                break;
            unReturnValue = Controller_ShowResponse(*PppResponseData);
            if (RC_SUCCESS != unReturnValue)
                break;

            if (RC_SUCCESS != (*PppResponseData)->unReturnCode)
            {
                // Abort on error, except if owner password could not be verified due to disabled/deactivated TPM.
                if (!((UPDATE_TYPE_TPM12_OWNERAUTH == unUpdateType) && (RC_E_TPM12_DISABLED_DEACTIVATED == (*PppResponseData)->unReturnCode)))
                {
                    unReturnValue = (*PppResponseData)->unReturnCode;
                    break;
                }
            }

            // Do a firmware update
            unReturnValue = CommandFlow_TpmUpdate_UpdateFirmware(pIfxUpdate);
            if (RC_SUCCESS != unReturnValue)
                break;

            unReturnValue = Controller_ShowResponse(*PppResponseData);
            if (RC_SUCCESS != unReturnValue)
                break;

            if (RC_SUCCESS != (*PppResponseData)->unReturnCode)
            {
                unReturnValue = (*PppResponseData)->unReturnCode;
                break;
            }

            break;
        }

        // Check if TPM12-ClearOwnership is set
        if (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_TPM12_CLEAROWNERSHIP, &fValue) && TRUE == fValue)
        {
            // Allocate memory
            Platform_MemoryFree((void**)PppResponseData);
            *PppResponseData = (IfxToolHeader*)Platform_MemoryAllocateZero(sizeof(IfxTpm12ClearOwnership));
            if (NULL == *PppResponseData)
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE(unReturnValue, L"Error detected in Controller_ProceedWork: Memory allocation failed.");
                break;
            }
            // Execute command
            (*PppResponseData)->unSize = sizeof(IfxTpm12ClearOwnership);
            (*PppResponseData)->unType = STRUCT_TYPE_Tpm12ClearOwnership;

            unReturnValue = CommandFlow_Tpm12ClearOwnership_Execute((IfxToolHeader*)*PppResponseData);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Show command response
            unReturnValue = Controller_ShowResponse(*PppResponseData);
            if (RC_SUCCESS != unReturnValue)
                break;

            if (RC_SUCCESS != (*PppResponseData)->unReturnCode)
            {
                unReturnValue = (*PppResponseData)->unReturnCode;
                break;
            }

            break;
        }

        // Check if SetModeHash is set
        if (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_SETMODE, &fValue) && TRUE == fValue)
        {
            // Allocate memory
            Platform_MemoryFree((void**)PppResponseData);
            *PppResponseData = (IfxToolHeader*)Platform_MemoryAllocateZero(sizeof(IfxSetMode));
            if (NULL == *PppResponseData)
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE(unReturnValue, L"Error detected in Controller_ProceedWork: Memory allocation failed.");
                break;
            }
            // Execute command .....
            (*PppResponseData)->unSize = sizeof(IfxSetMode);
            (*PppResponseData)->unType = STRUCT_TYPE_SetMode;

            unReturnValue = CommandFlow_SetMode_ValidateParameter((IfxSetMode*)*PppResponseData);
            if (RC_SUCCESS != unReturnValue)
                break;

            unReturnValue = CommandFlow_SetMode_Execute((IfxSetMode*)*PppResponseData);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Show command response
            unReturnValue = Controller_ShowResponse(*PppResponseData);
            break;
        }

        // Unknown command line option -> return bad command line
        unReturnValue = RC_E_BAD_COMMANDLINE;
        ERROR_STORE(unReturnValue, L"Unknown command line option.");
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    IGNORE_RETURN_VALUE(Logging_Finalize());

    return unReturnValue;
}
