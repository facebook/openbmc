/**
 *  @brief      Implements the command line parser
 *  @details    This module parses the command line and provides the command line properties
 *  @file       CommandLineParser.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "ICommandLineParser.h"
#include "CommandLineParser.h"
#include "TPMFactoryUpdStruct.h"
#include "Resource.h"
#include "Platform.h"
#include "UiUtility.h"
#include "Utility.h"

wchar_t GwszToolName[MAX_PATH] = L"TPMFactoryUpd";

const wchar_t CwszErrorMsgFormatPropertyStorage_Set[] = L"PropertyStorage_Set* failed to set property '%ls'.";

/**
 *  @brief      Parses the command line option
 *  @details    Parses the command line option identified by the common command line module code
 *
 *  @param      PwszCommandLineOption   Pointer to a wide character array containing the current option to work on.
 *  @param      PpunCurrentArgIndex     Pointer to an index for the current position in the argument list.
 *  @param      PnMaxArg                Maximum number of argument is the arguments list.
 *  @param      PrgwszArgv              Pointer to an array of wide character arrays representing the command line.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_BAD_COMMANDLINE    In case of an invalid command line.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_Parse(
    _In_z_                  const wchar_t*          PwszCommandLineOption,
    _Inout_                 unsigned int*           PpunCurrentArgIndex,
    _In_                    int                     PnMaxArg,
    _In_reads_z_(PnMaxArg)  const wchar_t* const    PrgwszArgv[])
{
    // Initialize to RC_SUCCESS because of while loop condition
    unsigned int unReturnValue = RC_E_FAIL;

    wchar_t wszValue[MAX_STRING_1024];
    unsigned int unValueSize = RG_LEN(wszValue);

    IGNORE_RETURN_VALUE(Platform_StringSetZero(wszValue, RG_LEN(wszValue)));

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check parameters
        if (NULL == PwszCommandLineOption ||
                NULL == PpunCurrentArgIndex ||
                NULL == PrgwszArgv)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Invalid NULL parameter.");
            break;
        }

        // **** -help
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_HELP, RG_LEN(CMD_HELP), TRUE) ||
                0 == Platform_StringCompare(PwszCommandLineOption, CMD_HELP_ALT, RG_LEN(CMD_HELP_ALT), FALSE))
        {
            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Add help property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
            IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_HELP, TRUE));

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -info
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_INFO, RG_LEN(CMD_INFO), TRUE))
        {
            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Add info property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
            IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_INFO, TRUE));

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -update
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_UPDATE, RG_LEN(CMD_UPDATE), TRUE))
        {
            unsigned int unUpdateType = UPDATE_TYPE_NONE;

            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Add update property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
            IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_UPDATE, TRUE));

            // Read parameter Update type
            unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
            if (RC_SUCCESS == unReturnValue)
            {
                // Set Update type
                if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_TPM12_DEFERREDPP, RG_LEN(CMD_UPDATE_OPTION_TPM12_DEFERREDPP), TRUE))
                {
                    unUpdateType = UPDATE_TYPE_TPM12_DEFERREDPP;
                }
                else if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_TPM12_TAKEOWNERSHIP, RG_LEN(CMD_UPDATE_OPTION_TPM12_TAKEOWNERSHIP), TRUE))
                {
                    unUpdateType = UPDATE_TYPE_TPM12_TAKEOWNERSHIP;
                }
                else if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_TPM12_OWNERAUTH, RG_LEN(CMD_UPDATE_OPTION_TPM12_OWNERAUTH), TRUE))
                {
                    unUpdateType = UPDATE_TYPE_TPM12_OWNERAUTH;
                }
                else if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_TPM20_EMPTYPLATFORMAUTH, RG_LEN(CMD_UPDATE_OPTION_TPM20_EMPTYPLATFORMAUTH), TRUE))
                {
                    unUpdateType = UPDATE_TYPE_TPM20_EMPTYPLATFORMAUTH;
                }
                else if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_CONFIG_FILE, RG_LEN(CMD_UPDATE_OPTION_CONFIG_FILE), TRUE))
                {
                    unUpdateType = UPDATE_TYPE_CONFIG_FILE;
                }
                else
                {
                    unReturnValue = RC_E_BAD_COMMANDLINE;
                    ERROR_STORE_FMT(unReturnValue, L"Unknown option for command line parameter <update> (%ls).", wszValue);
                    break;
                }

                // Set update type property
                if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, unUpdateType))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_UPDATE_TYPE);
                    break;
                }
            }
            else
            {
                ERROR_STORE(unReturnValue, L"Missing type for command line parameter <update>.");
                break;
            }

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -log
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_LOG, RG_LEN(CMD_LOG), TRUE))
        {
            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Add logging property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
            IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_LOGGING, TRUE));

            // Get logging level property
            unsigned int unLoggingLevel = 0;
            if (PropertyStorage_ExistsElement(PROPERTY_LOGGING_LEVEL))
            {
                if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOGGING_LEVEL, &unLoggingLevel))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, L"Failed", PROPERTY_LOGGING_LEVEL);
                    break;
                }
            }

            if (unLoggingLevel < LOGGING_LEVEL_3)
            {
                // Set logging level property
                if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_LOGGING_LEVEL, LOGGING_LEVEL_3))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_LOGGING_LEVEL);
                    break;
                }
            }

            // Read parameter Logging path (optional)
            unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
            if (RC_SUCCESS == unReturnValue)
            {
                LOGGING_WRITE_LEVEL4_FMT(L"New logging path: %s", wszValue);

                // Check if path fits into property storage
                if (PROPERTY_STORAGE_MAX_VALUE <= unValueSize)
                {
                    unReturnValue = RC_E_INVALID_LOG_OPTION;
                    ERROR_STORE_FMT(unReturnValue, L"Log file (%ls) path is too long. (0x%.8X)", wszValue, unReturnValue);
                    break;
                }
                // Set logging path property
                if (!PropertyStorage_SetValueByKey(PROPERTY_LOGGING_PATH, wszValue))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_LOGGING_PATH);
                    break;
                }
            }
            else if (RC_E_BAD_COMMANDLINE == unReturnValue)
            {
                // Create default log file path
                unsigned int unCapacity = RG_LEN(wszValue);
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszValue, RG_LEN(wszValue)));
                unReturnValue = Platform_StringCopy(wszValue, &unCapacity, GwszToolName);
                if (RC_SUCCESS != unReturnValue)
                    break;

                unCapacity = RG_LEN(wszValue);
                unReturnValue = Platform_StringConcatenate(wszValue, &unCapacity, L".log");
                if (RC_SUCCESS != unReturnValue)
                    break;

                LOGGING_WRITE_LEVEL4_FMT(L"Use default logging path: %s", wszValue);

                // Set log file path without path (relative to current working dir)
                if (!PropertyStorage_SetValueByKey(PROPERTY_LOGGING_PATH, wszValue))
                {
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_LOGGING_PATH);
                    break;
                }
            }

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -firmware
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_FIRMWARE, RG_LEN(CMD_FIRMWARE), TRUE))
        {
            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Read parameter Firmware path
            unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Missing firmware file path for command line parameter <firmware>.");
                break;
            }

            // Set Firmware path
            if (!PropertyStorage_SetValueByKey(PROPERTY_FIRMWARE_PATH, wszValue))
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_FIRMWARE_PATH);
                break;
            }

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -tpm12-clearownership
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_TPM12_CLEAROWNERSHIP, RG_LEN(CMD_TPM12_CLEAROWNERSHIP), TRUE))
        {
            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Add ClearOwnership property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
            IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_TPM12_CLEAROWNERSHIP, TRUE));

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -access-mode
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_ACCESS_MODE, RG_LEN(CMD_ACCESS_MODE), TRUE))
        {
            unsigned int unAccessMode = 0;
            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Add ClearOwnership property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
            IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_ACCESS_MODE, TRUE));

            // Read parameter mode
            unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Missing mode for command line parameter <access-mode>.");
                break;
            }

            // Set access-mode
            if (!PropertyStorage_SetValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, wszValue))
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_TPM_DEVICE_ACCESS_MODE);
                break;
            }

            // Check if value is 1 or 3 for the TPM device access mode
            if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, &unAccessMode) ||
                    (TPM_DEVICE_ACCESS_DRIVER != unAccessMode && TPM_DEVICE_ACCESS_MEMORY_BASED != unAccessMode))
            {
                unReturnValue = RC_E_INVALID_ACCESS_MODE;
                ERROR_STORE_FMT(unReturnValue, L"An invalid value (%ls) was passed in the <access-mode> command line option.", wszValue);
                break;
            }

            // Read optional device path
            unValueSize = RG_LEN(wszValue);
            unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
            if (RC_SUCCESS == unReturnValue)
            {
                // Store setting value
                if (!PropertyStorage_SetValueByKey(PROPERTY_TPM_DEVICE_ACCESS_PATH, wszValue))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_TPM_DEVICE_ACCESS_PATH);
                    break;
                }
            }

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -config
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_CONFIG, RG_LEN(CMD_CONFIG), TRUE))
        {
            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Read parameter configuration file path
            unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Missing configuration file path for command line parameter <config>.");
                break;
            }

            // Set configuration file path
            if (!PropertyStorage_SetValueByKey(PROPERTY_CONFIG_FILE_PATH, wszValue))
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_CONFIG_FILE_PATH);
                break;
            }

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -ownerauth
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_OWNERAUTH, RG_LEN(CMD_OWNERAUTH), TRUE))
        {
            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Read parameter owner authorization file
            unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Missing owner authorization file path for command line parameter <ownerauth>.");
                break;
            }

            // Set owner authorization file path
            if (!PropertyStorage_SetValueByKey(PROPERTY_OWNERAUTH_FILE_PATH, wszValue))
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_OWNERAUTH_FILE_PATH);
                break;
            }

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -setmode
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_SETMODE, RG_LEN(CMD_SETMODE), TRUE))
        {
            unsigned int unSetModeType = SETMODE_TYPE_NONE;

            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Add setmode property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
            IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_SETMODE, TRUE));

            // Read setmode parameter
            unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
            if (RC_SUCCESS == unReturnValue)
            {
                // Check firmware update option
                if (0 == Platform_StringCompare(wszValue, CMD_SETMODE_OPTION_TPM20_FW_UPDATE, RG_LEN(CMD_SETMODE_OPTION_TPM20_FW_UPDATE), TRUE))
                {
                    unSetModeType = SETMODE_TYPE_TPM20_FW_UPDATE;
                }
                // Check firmware recovery option
                else if (0 == Platform_StringCompare(wszValue, CMD_SETMODE_OPTION_TPM20_FW_RECOVERY, RG_LEN(CMD_SETMODE_OPTION_TPM20_FW_RECOVERY), TRUE))
                {
                    unSetModeType = SETMODE_TYPE_TPM20_FW_RECOVERY;
                }
                // Check operational option
                else if (0 == Platform_StringCompare(wszValue, CMD_SETMODE_OPTION_TPM20_OPERATIONAL, RG_LEN(CMD_SETMODE_OPTION_TPM20_OPERATIONAL), TRUE))
                {
                    unSetModeType = SETMODE_TYPE_TPM20_OPERATIONAL;
                }
                else
                {
                    unReturnValue = RC_E_BAD_COMMANDLINE;
                    ERROR_STORE_FMT(unReturnValue, L"Unknown option for command line parameter <setmode> (%ls).", wszValue);
                    break;
                }

                // Set setmode type
                if (!PropertyStorage_SetUIntegerValueByKey(PROPERTY_SETMODE_TYPE, unSetModeType))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, CwszErrorMsgFormatPropertyStorage_Set, PROPERTY_SETMODE_TYPE);
                    break;
                }
            }
            else
            {
                ERROR_STORE(unReturnValue, L"Missing manifest hash value for command line parameter <setmode>.");
                break;
            }

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        // **** -force
        if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_FORCE, RG_LEN(CMD_FORCE), TRUE))
        {
            unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Add info property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
            IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_FORCE, TRUE));

            unReturnValue = CommandLineParser_IncrementOptionCount();
            break;
        }

        unReturnValue = RC_E_BAD_COMMANDLINE;
        ERROR_STORE_FMT(unReturnValue, L"Unknown command line parameter (%ls).", PwszCommandLineOption);
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Initialize command line parsing
 *  @details
 *
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     ...         Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_InitializeParsing()
{
    return RC_SUCCESS;
}

/**
 *  @brief      Finalize command line parsing
 *  @details
 *
 *  @param      PunReturnValue      Current return code which can be overwritten here.
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     ...         Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_FinalizeParsing(
    _In_ unsigned int PunReturnValue)
{
    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        BOOL fValue = FALSE;

        if (RC_SUCCESS != PunReturnValue)
        {
            if (RC_E_BAD_COMMANDLINE != PunReturnValue &&
                    RC_E_INVALID_ACCESS_MODE != PunReturnValue &&
                    RC_E_INVALID_LOG_OPTION != PunReturnValue)
            {
                // Map error code to RC_E_BAD_PARAMETER
                unsigned int unReturnValue = PunReturnValue;
                PunReturnValue = RC_E_BAD_COMMANDLINE;
                ERROR_STORE_FMT(PunReturnValue, L"Mapped error code: 0x%.8x", unReturnValue);
            }
            // In case of an error skip final checks
            break;
        }

        // Check if log option is given...
        if (PropertyStorage_GetBooleanValueByKey(PROPERTY_LOGGING, &fValue) &&
                fValue == TRUE)
        {
            PunReturnValue = UiUtility_CheckIfLogPathWritable(TRUE);
            if (RC_SUCCESS != PunReturnValue)
            {
                // Calculate default log file path
                wchar_t wszDefaultLogPath[PROPERTY_STORAGE_MAX_VALUE];
                unsigned int unCapacity = RG_LEN(wszDefaultLogPath);
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszDefaultLogPath, RG_LEN(wszDefaultLogPath)));
                PunReturnValue = Platform_StringCopy(wszDefaultLogPath, &unCapacity, GwszToolName);
                if (RC_SUCCESS != PunReturnValue)
                    break;
                unCapacity = RG_LEN(wszDefaultLogPath);
                PunReturnValue = Platform_StringConcatenate(wszDefaultLogPath, &unCapacity, L".log");
                if (RC_SUCCESS != PunReturnValue)
                    break;

                // Set default path
                // Ignore return value we are already in error handling
                IGNORE_RETURN_VALUE(PropertyStorage_ChangeValueByKey(PROPERTY_LOGGING_PATH, wszDefaultLogPath));

                ERROR_STORE_FMT(RC_E_INVALID_LOG_OPTION, L"The log file could not be opened or created. The path may be invalid, too long or not accessible. (0x%.8X)", PunReturnValue);
                PunReturnValue = RC_E_INVALID_LOG_OPTION;
                break;
            }
        }

        // Check if at least one mandatory option is set
        if ((FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_UPDATE, &fValue) || FALSE == fValue) &&
                (FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_INFO, &fValue) || FALSE == fValue) &&
                (FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_HELP, &fValue) || FALSE == fValue) &&
                (FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_TPM12_CLEAROWNERSHIP, &fValue) || FALSE == fValue) &&
                (FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_SETMODE, &fValue) || FALSE == fValue))
        {
            PunReturnValue = RC_E_BAD_COMMANDLINE;
            ERROR_STORE(PunReturnValue, L"No mandatory command line option found.");
            break;
        }

        // Check that when update option is set ...
        if (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_UPDATE, &fValue) && TRUE == fValue)
        {
            unsigned int unUpdateType = UPDATE_TYPE_NONE;
            if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, &unUpdateType))
            {
                PunReturnValue = RC_E_FAIL;
                ERROR_STORE(PunReturnValue, L"An internal error occurred while parsing the -update option.");
                break;
            }
            if (UPDATE_TYPE_CONFIG_FILE == unUpdateType)
            {
                // The configuration option is set for -update config-file
                if (FALSE == PropertyStorage_ExistsElement(PROPERTY_CONFIG_FILE_PATH))
                {
                    PunReturnValue = RC_E_BAD_COMMANDLINE;
                    ERROR_STORE(PunReturnValue, L"Mandatory command line option config is missing.");
                    break;
                }
            }
            else if (UPDATE_TYPE_TPM12_OWNERAUTH == unUpdateType)
            {
                // The firmware option is set for -update tpm12-ownerauth
                if (FALSE == PropertyStorage_ExistsElement(PROPERTY_FIRMWARE_PATH))
                {
                    PunReturnValue = RC_E_BAD_COMMANDLINE;
                    ERROR_STORE(PunReturnValue, L"Mandatory command line option firmware is missing.");
                    break;
                }
                // The ownerauth option is set for -update tpm12-ownerauth
                if (FALSE == PropertyStorage_ExistsElement(PROPERTY_OWNERAUTH_FILE_PATH))
                {
                    PunReturnValue = RC_E_BAD_COMMANDLINE;
                    ERROR_STORE(PunReturnValue, L"Mandatory command line option ownerauth is missing.");
                    break;
                }
            }
            else
            {
                // The firmware option is set for all other -update <options>
                if (FALSE == PropertyStorage_ExistsElement(PROPERTY_FIRMWARE_PATH))
                {
                    PunReturnValue = RC_E_BAD_COMMANDLINE;
                    ERROR_STORE(PunReturnValue, L"Mandatory command line option firmware is missing.");
                    break;
                }
            }
        }

        // Check that when setmode option is set ...
        if (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_SETMODE, &fValue) && TRUE == fValue)
        {
            unsigned int unSetmodeType = SETMODE_TYPE_NONE;
            if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_SETMODE_TYPE, &unSetmodeType))
            {
                PunReturnValue = RC_E_FAIL;
                ERROR_STORE(PunReturnValue, L"An internal error occurred while parsing the -setmode option.");
                break;
            }
            if (SETMODE_TYPE_TPM20_FW_UPDATE == unSetmodeType)
            {
                // The firmware option is mandatory
                if (FALSE == PropertyStorage_ExistsElement(PROPERTY_FIRMWARE_PATH))
                {
                    PunReturnValue = RC_E_BAD_COMMANDLINE;
                    ERROR_STORE(PunReturnValue, L"Mandatory command line option firmware is missing.");
                    break;
                }
            }
            else if (SETMODE_TYPE_TPM20_FW_RECOVERY == unSetmodeType || SETMODE_TYPE_TPM20_OPERATIONAL == unSetmodeType)
            {
                // The firmware option is not supported
                if (TRUE == PropertyStorage_ExistsElement(PROPERTY_FIRMWARE_PATH))
                {
                    PunReturnValue = RC_E_BAD_COMMANDLINE;
                    ERROR_STORE(PunReturnValue, L"Command line option firmware is not supported.");
                    break;
                }
            }
            else
            {
                PunReturnValue = RC_E_FAIL;
                ERROR_STORE(PunReturnValue, L"An internal error occurred while parsing -setmode parameter.");
                break;
            }
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, PunReturnValue);

    return PunReturnValue;
}

/**
 *  @brief      This function increments the command line option count
 *  @details
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_FAIL           An unexpected error occurred using the property storage.
 */
_Check_return_
unsigned int
CommandLineParser_IncrementOptionCount()
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        unsigned int unOptionCount = 0;
        BOOL fReturnValue = FALSE;
        // Check if option count exists
        if (PropertyStorage_GetUIntegerValueByKey(PROPERTY_CMDLINE_COUNT, &unOptionCount))
            // Increment the option count property
            fReturnValue = PropertyStorage_ChangeUIntegerValueByKey(PROPERTY_CMDLINE_COUNT, ++unOptionCount);
        else
            // Add option count property with value 1
            fReturnValue = PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_CMDLINE_COUNT, 1);

        if (!fReturnValue)
        {
            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage returned an unexpected value while adding a key value pair. (%ls|%d)", PROPERTY_CMDLINE_COUNT, unOptionCount);
            break;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Read command line parameter to a string
 *  @details    Checks, if parameter is available and read it to buffer
 *
 *  @param      PrgwszArgv              Command line parameters.
 *  @param      PnTotalParams           Number of all parameters.
 *  @param      PpunPosition            Current position in parsing.
 *  @param      PwszValue               Output buffer for command line parameter.
 *  @param      PpunValueSize           Buffer length.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_COMMANDLINE    In case a mandatory parameter is missing for RegisterTest option.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
_Success_(return == 0)
CommandLineParser_ReadParameter(
    _In_reads_z_(PnTotalParams)     const wchar_t* const    PrgwszArgv[],
    _In_                            int                     PnTotalParams,
    _Inout_                         unsigned int*           PpunPosition,
    _Out_z_cap_(*PpunValueSize)     wchar_t*                PwszValue,
    _Inout_                         unsigned int*           PpunValueSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        if ((NULL == PpunPosition) ||
                (NULL == PrgwszArgv) ||
                (NULL == PwszValue) ||
                (NULL == PpunValueSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Increment the argument position and check if more arguments are available
        (*PpunPosition)++;

        // Check if parameter is present
        if (((int)*PpunPosition >= PnTotalParams) || (FALSE == CommandLineParser_IsValue(PrgwszArgv[*PpunPosition])))
        {
            (*PpunPosition)--;
            unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // Read parameter: register
        unReturnValue = Platform_StringCopy(
                            PwszValue,
                            PpunValueSize,
                            PrgwszArgv[*PpunPosition]
                        );
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Unexpected error executing Platform_StringCopy.");
            break;
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Checks if the given string is a command line parameter value (no option)
 *  @details    If the given string does not start with '--' or '-' it is a command line parameter value
 *
 *  @param      PwszCommand     The command line parameter to check (max length MAX_STRING_1024).
 *  @retval     TRUE            PwszCommand does not start with '--', '-' or '/'.
 *  @retval     FALSE           Otherwise.
 */
_Check_return_
BOOL
CommandLineParser_IsValue(
    _In_z_      const wchar_t* const    PwszCommand)
{
    BOOL            fIsValue = TRUE;
    unsigned int    unReturnValue = RC_SUCCESS;
    unsigned int    unSize = 0;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);
    // LOGGING_WRITE_LEVEL4_FMT(L"parameter: '%x'",PwszCommand);

    do
    {
        // Check buffer pointer and capacity
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszCommand))
        {
            // Parameter is empty
            fIsValue = FALSE;
            break;
        }

        // Determine string length
        unReturnValue = Platform_StringGetLength(PwszCommand, MAX_STRING_1024, &unSize);
        if (RC_SUCCESS != unReturnValue)
        {
            fIsValue = FALSE;
            ERROR_STORE_FMT(unReturnValue, L"Parameter PwszCommand is larger than %d or is not zero terminated.", MAX_STRING_1024);
            break;
        }

        // Check first character(s) in string
        if (unSize >= 2)
        {
            // Check for "--"
            if (PwszCommand[0] == L'-' && PwszCommand[1] == L'-')
            {
                fIsValue = FALSE;
            }
            else
            {
                // Check for "-"
                if (PwszCommand[0] == L'-')
                    fIsValue = FALSE;
            }
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return fIsValue;
}

/**
 *  @brief      Checks whether the parameter can be combined with any previously set
 *  @details
 *
 *  @param      PwszCommand             The command line parameter to check.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_COMMANDLINE    In case of a option is not combinable with an already parsed one.
 */
_Check_return_
unsigned int
CommandLineParser_CheckCommandLineOptions(
    _In_z_      const wchar_t*  PwszCommand)
{
    // Initialize return value to success because of negative checking and storing the error code
    unsigned int unReturnValue = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        BOOL fHelpOption = FALSE;
        BOOL fInfoOption = FALSE;
        BOOL fUpdateOption = FALSE;
        BOOL fLogOption = FALSE;
        BOOL fFwPathUpdateOption = FALSE;
        BOOL fClearOwnership = FALSE;
        BOOL fAccessMode = FALSE;
        BOOL fConfigFileOption = FALSE;
        BOOL fOwnerAuth = FALSE;
        BOOL fSetMode = FALSE;
        BOOL fForce = FALSE;

        // Read Property storage
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_HELP))
            fHelpOption = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_INFO))
            fInfoOption = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_UPDATE))
            fUpdateOption = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_LOGGING))
            fLogOption = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_FIRMWARE_PATH))
            fFwPathUpdateOption = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_TPM12_CLEAROWNERSHIP))
            fClearOwnership = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_ACCESS_MODE))
            fAccessMode = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_CONFIG_FILE_PATH))
            fConfigFileOption = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_OWNERAUTH_FILE_PATH))
            fOwnerAuth = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_SETMODE))
            fSetMode = TRUE;
        if (TRUE == PropertyStorage_ExistsElement(PROPERTY_FORCE))
            fForce = TRUE;

        // **** -help [Help]
        if (0 == Platform_StringCompare(PwszCommand, CMD_HELP, RG_LEN(CMD_HELP), TRUE) ||
                0 == Platform_StringCompare(PwszCommand, CMD_HELP_ALT, RG_LEN(CMD_HELP_ALT), FALSE))
        {
            // Command line parameter 'help' combined with the following parameters is invalid
            if (TRUE == fHelpOption || // Parameter 'help' should not be given twice
                    TRUE == fInfoOption ||
                    TRUE == fUpdateOption ||
                    TRUE == fFwPathUpdateOption ||
                    TRUE == fLogOption ||
                    TRUE == fClearOwnership ||
                    TRUE == fAccessMode ||
                    TRUE == fConfigFileOption ||
                    TRUE == fOwnerAuth ||
                    TRUE == fSetMode ||
                    TRUE == fForce)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -info [Info]
        if (0 == Platform_StringCompare(PwszCommand, CMD_INFO, RG_LEN(CMD_INFO), TRUE))
        {
            // Command line parameter 'info' combined with the following parameters is invalid
            if (TRUE == fInfoOption || // Parameter 'info' should not be given twice
                    TRUE == fHelpOption ||
                    TRUE == fUpdateOption ||
                    TRUE == fFwPathUpdateOption ||
                    TRUE == fClearOwnership ||
                    TRUE == fConfigFileOption ||
                    TRUE == fOwnerAuth ||
                    TRUE == fSetMode ||
                    TRUE == fForce)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -update [Update]
        if (0 == Platform_StringCompare(PwszCommand, CMD_UPDATE, RG_LEN(CMD_UPDATE), TRUE))
        {
            // Command line parameter 'update' combined with the following parameters is invalid
            if (TRUE == fUpdateOption || // Parameter 'update' should not be given twice
                    TRUE == fHelpOption ||
                    TRUE == fInfoOption ||
                    TRUE == fClearOwnership ||
                    TRUE == fConfigFileOption ||
                    TRUE == fSetMode)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -log [Log]
        if (0 == Platform_StringCompare(PwszCommand, CMD_LOG, RG_LEN(CMD_LOG), TRUE))
        {
            // Command line parameter 'log' combined with the following parameters is invalid
            if (TRUE == fLogOption || // Parameter 'log' should not be given twice
                    TRUE == fHelpOption)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -firmware [Firmware]
        if (0 == Platform_StringCompare(PwszCommand, CMD_FIRMWARE, RG_LEN(CMD_FIRMWARE), TRUE))
        {
            // Command line parameter 'firmware' combined with the following parameters is invalid
            if (TRUE == fFwPathUpdateOption || // Parameter 'firmware' should not be given twice
                    TRUE == fHelpOption ||
                    TRUE == fInfoOption ||
                    TRUE == fClearOwnership ||
                    TRUE == fConfigFileOption)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -tpm12-clearownership [TPM12-ClearOwnership]
        if (0 == Platform_StringCompare(PwszCommand, CMD_TPM12_CLEAROWNERSHIP, RG_LEN(CMD_TPM12_CLEAROWNERSHIP), TRUE))
        {
            // Command line parameter 'tpm12-clearownership' combined with the following parameters is invalid
            if (TRUE == fClearOwnership || // Parameter 'tpm12-clearownership' should not be given twice
                    TRUE == fHelpOption ||
                    TRUE == fInfoOption ||
                    TRUE == fUpdateOption ||
                    TRUE == fFwPathUpdateOption ||
                    TRUE == fConfigFileOption ||
                    TRUE == fSetMode ||
                    TRUE == fForce)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -access-mode [Access-mode]
        if (0 == Platform_StringCompare(PwszCommand, CMD_ACCESS_MODE, RG_LEN(CMD_ACCESS_MODE), TRUE))
        {
            // Command line parameter 'access-mode' combined with the following parameters is invalid
            if (TRUE == fAccessMode || // Parameter 'access-mode' should not be given twice
                    TRUE == fHelpOption)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -config [Configuration File]
        if (0 == Platform_StringCompare(PwszCommand, CMD_CONFIG, RG_LEN(CMD_CONFIG), TRUE))
        {
            // Command line parameter 'config' combined with the following parameters is invalid
            if (TRUE == fConfigFileOption || // Parameter 'config' should not be given twice
                    TRUE == fHelpOption ||
                    TRUE == fInfoOption ||
                    TRUE == fClearOwnership ||
                    TRUE == fFwPathUpdateOption ||
                    TRUE == fSetMode)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -ownerauth [Owner Authorization File]
        if (0 == Platform_StringCompare(PwszCommand, CMD_OWNERAUTH, RG_LEN(CMD_OWNERAUTH), TRUE))
        {
            // Command line parameter 'ownerauth' combined with the following parameters is invalid
            if (TRUE == fOwnerAuth || // Parameter 'ownerauth' should not be given twice
                    TRUE == fHelpOption ||
                    TRUE == fInfoOption ||
                    TRUE == fSetMode ||
                    TRUE == fForce)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -setmode [Manifest Hash]
        if (0 == Platform_StringCompare(PwszCommand, CMD_SETMODE, RG_LEN(CMD_SETMODE), TRUE))
        {
            // Command line parameter 'setmode' combined with the following parameters is invalid
            if (TRUE == fSetMode || // Parameter 'setmode' should not be given twice
                    TRUE == fHelpOption ||
                    TRUE == fInfoOption ||
                    TRUE == fUpdateOption ||
                    TRUE == fClearOwnership ||
                    TRUE == fConfigFileOption ||
                    TRUE == fOwnerAuth ||
                    TRUE == fForce)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        // **** -force [Force]
        if (0 == Platform_StringCompare(PwszCommand, CMD_FORCE, RG_LEN(CMD_FORCE), TRUE))
        {
            // Command line parameter 'force' combined with the following parameters is invalid
            if (TRUE == fForce || // Parameter 'force' should not be given twice
                    TRUE == fHelpOption ||
                    TRUE == fInfoOption ||
                    TRUE == fClearOwnership ||
                    TRUE == fOwnerAuth ||
                    TRUE == fSetMode)
                unReturnValue = RC_E_BAD_COMMANDLINE;
            break;
        }

        unReturnValue = RC_E_BAD_COMMANDLINE;
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
        ERROR_STORE(unReturnValue, L"Incompatible command line options found.");

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}
