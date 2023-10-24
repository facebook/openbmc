/**
 *  @brief      Implements common controller methods for TPM applications.
 *  @details    This module has common control methods for a TPM application's view and business layers.
 *  @file       ControllerCommon.c
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
#include "CommandLine.h"
#include "CommandLineParser.h"
#include "Response.h"
#include "DeviceManagement.h"
#include "CommandFlow_Init.h"
#include "Platform.h"
#include "Config.h"
#include "ConfigSettings.h"
#include "TPM2_Shutdown.h"

/**
 *  @brief      This function initializes the applications's view and business layers.
 *  @details    This function utilizes the UI modules like CmdLineParser and initializes the DeviceManagement.
 *
 *  @param      PnArgc          Parameter as provided in main().
 *  @param      PrgwszArgv      Parameter as provided in main().
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       An unexpected error occurred. E.g. PropertyStorage not initialized correctly.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_Initialize(
    _In_                    int                     PnArgc,
    _In_reads_z_(PnArgc)    const wchar_t* const    PrgwszArgv[])
{
    unsigned int unReturnValue = RC_E_FAIL;
    unsigned int unReturnValue2 = RC_E_FAIL;
    unsigned int unConsoleMode = CONSOLE_BUFFER_BIG;
    BOOL fIsHelpSet = FALSE;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Get tool path and store it for later use
        unReturnValue = CommandLine_GetSetToolPath(PrgwszArgv[0]);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"CommandLine_GetSetToolPath failed.");
            break;
        }

        // Get tool path
        wchar_t wszConfigFilePath[MAX_STRING_1024];
        unsigned int unConfigFilePathSize = RG_LEN(wszConfigFilePath);
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszConfigFilePath, unConfigFilePathSize));
        if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_TOOL_PATH, wszConfigFilePath, &unConfigFilePathSize))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_TOOL_PATH);
            break;
        }

        // Try to concatenate tool path and default config file (must be placed next to the tool)
        // If tool path is empty, the current working directory is used (where also the tool should be located then).
        wchar_t* pwszConfigFilePath = NULL;
        if (L'\0' != wszConfigFilePath[0])
        {
            unConfigFilePathSize = RG_LEN(wszConfigFilePath);
            unReturnValue = Platform_StringConcatenatePaths(wszConfigFilePath, &unConfigFilePathSize, CONFIG_FILE);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringConcatenate returned an unexpected value while composing the config file path.");
                break;
            }

            pwszConfigFilePath = wszConfigFilePath;
        }
        else
            pwszConfigFilePath = CONFIG_FILE;

        // Initialize the configuration module
        unReturnValue = Config_Parse(pwszConfigFilePath);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE_FMT(unReturnValue, L"Parsing the configuration file failed (%ls).", pwszConfigFilePath);
            break;
        }

        // Initialize and set file size check flag before first logging
        if (!PropertyStorage_SetBooleanValueByKey(PROPERTY_LOGGING_CHECK_SIZE, TRUE))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"PropertyStorage error while setting the file size check flag");
            break;
        }

        // Get Console buffer mode
        if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_CONSOLE_MODE, &unConsoleMode))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetUIntegerValueByKey failed to get property '%ls'.", PROPERTY_CONSOLE_MODE);
            break;
        }

        // Initialize ConsoleIO
        ConsoleIO_InitConsole(unConsoleMode);

        // Call command line parser
        unReturnValue = CommandLine_Parse(PnArgc, PrgwszArgv);

        // Write cached log entries if enabled and entries exists
        unReturnValue2 = Logging_WriteCachedEntries();
        if (RC_SUCCESS != unReturnValue2)
        {
            // Write message to console and continue
            IGNORE_RETURN_VALUE(ConsoleIO_Write(TRUE, TRUE, L"Logging_WriteCachedEntries failed (0%x). Continue execution.", unReturnValue2));
        }

        // Break if an error is occurred or help option is selected
        if (RC_SUCCESS != unReturnValue ||
                (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_HELP, &fIsHelpSet) && TRUE == fIsHelpSet))
            break;

        // Call the device management initialization
        DeviceManagement_Initialize();

        // Connect to the TPM device
        unReturnValue = DeviceManagement_Connect();
        if (RC_SUCCESS != unReturnValue)
            break;

        // Execute product specific initialization tasks (for example the product may need to seed the RNG or initialize TPM I/O)
        unReturnValue = CommandFlow_Init_Execute();
    }
    WHILE_FALSE_END;

    // In case of an error, try to write cached log entries and disable caching
    if (RC_SUCCESS != unReturnValue)
    {
        IGNORE_RETURN_VALUE(Logging_WriteCachedEntries());
    }

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      This function uninitializes the applications view and business layers.
 *  @details    This function uninitializes the DeviceManagement.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_Uninitialize()
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Uninitialize
        ConsoleIO_UnInitConsole();

        // Check if Connected
        if (TRUE == DeviceManagement_IsConnected())
        {
            if (PropertyStorage_ExistsElement(PROPERTY_CALL_SHUTDOWN_ON_EXIT))
            {
                // In case this tool started up the TPM successfully with TPM2_Startup, call TPM2_Shutdown to
                // prevent unorderly shutdown of the TPM.
                IGNORE_RETURN_VALUE(TSS_TPM2_Shutdown(TSS_TPM_SU_CLEAR));
            }

            unReturnValue = DeviceManagement_Disconnect();
            if (RC_SUCCESS != unReturnValue)
                break;
        }

        // Check if initialized
        if (TRUE == DeviceManagement_IsInitialized())
        {
            DeviceManagement_Uninitialize();
        }
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}
