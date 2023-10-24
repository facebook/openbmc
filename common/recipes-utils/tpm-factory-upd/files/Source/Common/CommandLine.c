/**
 *  @brief      Implements the command line module
 *  @details    This module contains common methods for the command line module
 *  @file       CommandLine.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "CommandLine.h"
#include "Platform.h"

/**
 *  @brief      Parses the command line
 *  @details    Puts all parameters from the command line into a Command Line Parameters structure.
 *
 *  @param      PnArgc      Number of command line arguments.
 *  @param      PrgwszArgv  Array of pointers to each command line argument.
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     ...         Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLine_Parse(
    _In_                    int                     PnArgc,
    _In_reads_z_(PnArgc)    const wchar_t* const    PrgwszArgv[])
{
    // Initialize to RC_SUCCESS because of while loop condition
    unsigned int unReturnValue = RC_SUCCESS;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        unsigned int unCounter = 1;
        wchar_t wszCommandBuffer[MAX_STRING_1024];
        unsigned int unCommandBufferSize = RG_LEN(wszCommandBuffer);
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszCommandBuffer, RG_LEN(wszCommandBuffer)));

        // Check if parameters are set
        if (1 == PnArgc)
        {
            // If not stop the parsing
            unReturnValue = RC_SUCCESS;
            break;
        }

        // Initialize dedicated CommandLine implementation
        unReturnValue = CommandLineParser_InitializeParsing();
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Unexpected error occurred during initialization.");
            break;
        }

        // Walk over the arguments
        while (((int)unCounter) < PnArgc)
        {
            BOOL fIsOption = FALSE;

            // Check if argument is null or empty or " "
            if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PrgwszArgv[unCounter]) ||
                    0 == Platform_StringCompare(PrgwszArgv[unCounter], L" ", RG_LEN(L" "), FALSE))
            {
                // Next parameter
                unCounter++;
                unCommandBufferSize = RG_LEN(wszCommandBuffer);
                continue;
            }

            // Check if starts with - or --
            unReturnValue = CommandLine_IsCommandLineOption(PrgwszArgv[unCounter], &fIsOption);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE_FMT(unReturnValue, L"Command line option does not start with - or -- (%ls)", PrgwszArgv[unCounter]);
                break;
            }
            if (!fIsOption)
            {
                unReturnValue = RC_E_BAD_COMMANDLINE;
                ERROR_STORE_FMT(unReturnValue, L"Command line option (Number: %d, Text: %ls) was not expected.", unCounter, PrgwszArgv[unCounter]);
                break;
            }

            // Trim command line argument remove - or --
            unReturnValue = CommandLine_GetTrimmedCommand(PrgwszArgv[unCounter], wszCommandBuffer, &unCommandBufferSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE_FMT(unReturnValue, L"Error while trimming command line parameter. (%ls)", PrgwszArgv[unCounter]);
                break;
            }

            // Parse command line option
            unReturnValue = CommandLineParser_Parse(wszCommandBuffer, &unCounter, PnArgc, PrgwszArgv);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE_FMT(unReturnValue, L"Error while parsing command line parameter option. (%ls)", wszCommandBuffer);
                break;
            }

            // Next parameter
            unCounter++;
            unCommandBufferSize = RG_LEN(wszCommandBuffer);
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    unReturnValue = CommandLineParser_FinalizeParsing(unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Check that the command is an option (i.e. that it starts with '-')
 *  @details
 *
 *  @param      PwszCommand             The input buffer.
 *  @param      PpIsCommandLine         Flag if it is a valid command line option or not.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PwszCommand, PwszOutCommand or PpunOutBufferSize is NULL.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
BOOL
CommandLine_IsCommandLineOption(
    _In_z_  const wchar_t*  PwszCommand,
    _Inout_ BOOL*           PpIsCommandLine)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        unsigned int unSize = 0;

        // Check buffer pointers and capacity
        if (NULL == PwszCommand ||
                NULL == PpIsCommandLine)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more parameters in function CommandLine_IsCommandLineOption are incorrect");
            break;
        }

        *PpIsCommandLine = FALSE;

        // Get string length
        unReturnValue = Platform_StringGetLength(PwszCommand, MAX_STRING_1024, &unSize);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE_FMT(unReturnValue, L"Platform_StringGetLength returned an unexpected value while parsing a command line argument (%ls)", PwszCommand);
            break;
        }

        if (unSize >= 2)
        {
            if ((PwszCommand[0] == L'-' && PwszCommand[1] == L'-') ||
                    (PwszCommand[0] == L'-'))
            {
                *PpIsCommandLine = TRUE;
            }
            else
                *PpIsCommandLine = FALSE;
        }
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Removes the option character from the string
 *  @details
 *
 *  @param      PwszCommand             The input buffer.
 *  @param      PwszOutCommand          The output buffer.
 *  @param      PpunOutBufferSize       The output buffer size.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PwszCommand, PwszOutCommand or PpunOutBufferSize is NULL.
 *  @retval     RC_E_BUFFER_TOO_SMALL   *PpunOutBufferSize is 0.
 */
_Check_return_
unsigned int
CommandLine_GetTrimmedCommand(
    _In_z_                              const wchar_t*  PwszCommand,
    _Out_z_cap_(*PpunOutBufferSize)     wchar_t*        PwszOutCommand,
    _Inout_                             unsigned int*   PpunOutBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4_FMT(L"-> Method entry (%ls, %d)", PwszCommand, PwszOutCommand);

    do
    {
        int nTrimSize = 0;
        unsigned int unSize = 0;

        // Check buffer pointers and capacity
        if (NULL == PwszCommand ||
                NULL == PwszOutCommand ||
                NULL == PpunOutBufferSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more parameters in function CommandLine_GetTrimmedCommand are incorrect");
            break;
        }
        if (1 > *PpunOutBufferSize)
        {
            unReturnValue = RC_E_BUFFER_TOO_SMALL;
            ERROR_STORE(unReturnValue, L"Output buffer size in function CommandLine_GetTrimmedCommand is too small");
            break;
        }

        // Set zero termination if an unexpected error occurs or nothing to do
        PwszOutCommand[0] = L'\0';

        unReturnValue = Platform_StringGetLength(PwszCommand, *PpunOutBufferSize, &unSize);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Unexpected error executing Platform_StringGetLength");
            break;
        }

        if (unSize >= 2)
        {
            if (PwszCommand[0] == L'-' && PwszCommand[1] == L'-')
            {
                if (unSize > 2)
                    // Trim "--"
                    nTrimSize = 2;
            }
            else if (PwszCommand[0] == L'-')
            {
                // Trim "-"
                nTrimSize = 1;
            }

            unReturnValue = Platform_StringCopy(PwszOutCommand, PpunOutBufferSize, &PwszCommand[nTrimSize]);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected error executing Platform_StringCopy");
                break;
            }
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Gets the tool path from argv0 parameter and stores it
 *  @details    Gets the tool path from argv0 parameter and stores it into property PROPERTY_TOOL_PATH.
 *              If the tool is started in the current working directory, the parameter may not contain
 *              any path information which results in an empty tool path string (meaning current working directory).
 *
 *  @param      PpwszArgv0  Command line argument (argv0).
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. PpwszArgv0 is NULL.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLine_GetSetToolPath(
    _In_z_  const wchar_t*  PpwszArgv0)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        // Check parameters
        if (NULL == PpwszArgv0)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Copy tool name from arg0 (without path, with full path or relative path)
        wchar_t wszToolPath[PROPERTY_STORAGE_MAX_VALUE];
        unsigned int unToolPathLength = RG_LEN(wszToolPath);
        unReturnValue = Platform_StringCopy(wszToolPath, &unToolPathLength, PpwszArgv0);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_StringCopy failed");
            break;
        }

        // Get the tool path from the full file path if set
        if (unToolPathLength > 0)
        {
            wchar_t* pArgModuleName = wszToolPath;
            // Search for path delimiters from the end of the string to the beginning
            unsigned int unCount = unToolPathLength - 1;
            for (; unCount > 0; unCount--)
            {
                if (L'\\' == pArgModuleName[unCount] || L'/' == pArgModuleName[unCount])
                    break;
            }

            // Set string termination after last path delimiter or at first character if no path or delimiter is available
            wszToolPath[unCount] = L'\0';
        }

        // Store the tool path for later usage.
        // Can be empty, a full path (e.g.: C:\... or /media/...) or a relative path (e.g.: .\ or ../).
        // If empty, the current working directory is used.
        if (!PropertyStorage_SetValueByKey(PROPERTY_TOOL_PATH, wszToolPath))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_SetValueByKey failed to set property '%ls'.", PROPERTY_TOOL_PATH);
            break;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}
