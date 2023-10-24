/**
 *  @brief      Implements logging methods for the application.
 *  @details    This module provides logging for the application.
 *  @file       Logging.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Logging.h"
#include "FileIO.h"
#include "Config.h"
#include "Platform.h"
#include "UiUtility.h"
#include "Utility.h"
#include "ConsoleIO/ConsoleIO.h"

/// Flag indicating whether to write a header into the log file or not
BOOL g_fLogHeader = TRUE;

/// Flag indicating whether caching of log entries is enabled or not
BOOL s_fCacheLogEntries = FALSE;

/// Flag indicating whether logging is already ongoing
BOOL s_fInLogging = FALSE;

/// The handle to the log file.
void* s_pFile = NULL;

/// Pointer for cached log messages
IfxLogEntry* s_pFirstLogEntry = NULL;
IfxLogEntry* s_pLastLogEntry = NULL;

/**
 *  @brief      This function builds the final logging file path
 *  @details    This function builds the final logging file path by concatenating the tool path if required.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       An unexpected error occurred. E.g. PropertyStorage not initialized correctly.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
Logging_BuildLoggingFilePath()
{
    unsigned int unReturnValue = RC_E_FAIL;
    wchar_t wszLoggingFilePath[MAX_STRING_1024];

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        unsigned int unLoggingFilePathBufferSize = RG_LEN(wszLoggingFilePath);
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszLoggingFilePath, RG_LEN(wszLoggingFilePath)));

        // Get logging file path
        if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_LOGGING_PATH, wszLoggingFilePath, &unLoggingFilePathBufferSize))
        {
            unReturnValue = RC_E_FAIL;
            break;
        }

        // Check if tool path shall be used in addition for correct logging file path
        if (PropertyStorage_ExistsElement(PROPERTY_TOOL_PATH) && !PropertyStorage_ExistsElement(PROPERTY_LOGGING))
        {
            // Check if path can be concatenated to tool path
            BOOL fConcatenatePath = TRUE;

            // Check for root path
            if (unLoggingFilePathBufferSize > 0 && (L'\\' == wszLoggingFilePath[0] || L'/' == wszLoggingFilePath[0]))
                fConcatenatePath = FALSE;

#ifdef WINDOWS
            // Check for drive letter
            if (unLoggingFilePathBufferSize > 1 && L':' == wszLoggingFilePath[1])
                fConcatenatePath = FALSE;
#endif
#ifdef UEFI
            // Check for absolute path
            for (unsigned int unCount = 1; unCount <= unLoggingFilePathBufferSize; unCount++)
            {
                if (L':' == wszLoggingFilePath[unCount])
                {
                    fConcatenatePath = FALSE;
                    break;
                }
            }
#endif

            if (fConcatenatePath)
            {
                // Get tool path
                wchar_t wszTempPath[PROPERTY_STORAGE_MAX_VALUE];
                unsigned int unCapacity = RG_LEN(wszTempPath);
                if (!PropertyStorage_GetValueByKey(PROPERTY_TOOL_PATH, wszTempPath, &unCapacity))
                {
                    unReturnValue = RC_E_FAIL;
                    break;
                }

                if (L'\0' != wszTempPath[0])
                {
                    unCapacity = RG_LEN(wszTempPath);
                    unReturnValue = Platform_StringConcatenatePaths(wszTempPath, &unCapacity, wszLoggingFilePath);
                    if (RC_SUCCESS != unReturnValue)
                        break;

                    // Copy path
                    unCapacity = RG_LEN(wszLoggingFilePath);
                    unReturnValue = Platform_StringCopy(wszLoggingFilePath, &unCapacity, wszTempPath);
                    if (RC_SUCCESS != unReturnValue)
                        break;

                    // Update property store value
                    if (FALSE == PropertyStorage_ChangeValueByKey(PROPERTY_LOGGING_PATH, wszLoggingFilePath))
                    {
                        unReturnValue = RC_E_FAIL;
                        break;
                    }
                }
            }
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      This function enables caching of log messages
 *  @details    This function enables caching of log messages.
 */
void
Logging_EnableCaching()
{
    s_fCacheLogEntries = TRUE;
}

/**
 *  @brief      This function writes cached log entries
 *  @details    This function writes cached log entries if caching is enabled and log entries exist
 *              that fits to the log level. Caching is disabled.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       An unexpected error occurred. E.g. PropertyStorage not initialized correctly.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
Logging_WriteCachedEntries()
{
    unsigned int unReturnValue = RC_E_FAIL;
    unsigned int unLogLevel = 0;
    BOOL fWriteCacheInfo = TRUE;

    do {

        // Caching enabled? If not, return immediately with success.
        if (FALSE == s_fCacheLogEntries)
        {
            unReturnValue = RC_SUCCESS;
            break;
        }

        // Disable caching
        s_fCacheLogEntries = FALSE;

        // Get Logging level
        if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOGGING_LEVEL, &unLogLevel))
        {
            unReturnValue = RC_E_FAIL;
            break;
        }

        // Log entries available?
        if (s_pFirstLogEntry)
        {
            // Loop through all log entries (starting from first)
            IfxLogEntry* pCurrentEntry = s_pFirstLogEntry;
            while (pCurrentEntry)
            {
                // Write message to log if log level fits
                if (pCurrentEntry->unLogLevel <= unLogLevel)
                {
                    if (fWriteCacheInfo)
                    {
                        fWriteCacheInfo = FALSE;
                        // Important: Next log message call will create/prepare log file. Don't remove.
                        LOGGING_WRITE_LEVEL1(L"Start processing cached log entries ...");
                    }

                    unReturnValue = FileIO_WriteString(s_pFile, pCurrentEntry->pwszLogMessage);
                    if (RC_SUCCESS != unReturnValue)
                        break;
                }
                else
                    unReturnValue = RC_SUCCESS;

                // Next entry is now first one
                s_pFirstLogEntry = (IfxLogEntry*)pCurrentEntry->pNextLogEntry;

                // Free memory for cached message
                Platform_MemoryFree((void**)&pCurrentEntry->pwszLogMessage);
                // Free memory for log entry
                Platform_MemoryFree((void**)&pCurrentEntry);
                pCurrentEntry = s_pFirstLogEntry;
            }

            // Error occurred within processing?
            if (RC_SUCCESS != unReturnValue)
                break;

            if (FALSE == fWriteCacheInfo)
                LOGGING_WRITE_LEVEL1(L"... finished processing cached log entries!");

            // Reset pointer
            s_pFirstLogEntry = NULL;
            s_pLastLogEntry = NULL;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      This internal function writes or stores a message
 *  @details    This internal function writes or stores a message depending on if a file handle is provided
 *              or caching is enabled.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       An unexpected error occurred. E.g. PropertyStorage not initialized correctly.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
IFXAPI
Logging_WriteStringf(
    _In_        unsigned int    PunLoggingLevel,
    _In_opt_    const void*     PpvFileHandle,
    _In_z_      const wchar_t*  PwszFormat,
    ...)
{
    unsigned int unReturnValue = RC_E_FAIL;

    // Get pointer to variable arguments
    va_list argptr;
    va_start(argptr, PwszFormat);

    do
    {
        // Check if log messages shall be stored or written into file
        if (TRUE == s_fCacheLogEntries)
        {
            // Format log message
            wchar_t wszMessage[MAX_STRING_1024];
            unsigned int unLength = RG_LEN(wszMessage);
            unReturnValue = Platform_StringFormatV(wszMessage, &unLength, PwszFormat, argptr);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Allocate memory for new log entry
            IfxLogEntry* pNewLogEntry = (IfxLogEntry*) Platform_MemoryAllocateZero(sizeof(IfxLogEntry));

            // First entry?
            if (NULL == s_pFirstLogEntry)
                s_pFirstLogEntry = pNewLogEntry;
            else
                s_pLastLogEntry->pNextLogEntry = pNewLogEntry;

            // Allocate memory and copy message
            unsigned int unSize = RG_LEN(wszMessage);
            unReturnValue = Platform_StringGetLength(wszMessage, unSize, &unSize);
            if (RC_SUCCESS != unReturnValue)
                break;
            pNewLogEntry->pwszLogMessage = (wchar_t*) Platform_MemoryAllocateZero(++unSize * sizeof(wchar_t));
            unReturnValue = Platform_StringCopy(pNewLogEntry->pwszLogMessage, &unSize, wszMessage);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Store log level
            pNewLogEntry->unLogLevel = PunLoggingLevel;

            // Make the new entry to last one
            s_pLastLogEntry = pNewLogEntry;

            // Set success
            unReturnValue = RC_SUCCESS;
        }
        else
        {
            if (NULL == PpvFileHandle)
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                break;
            }

            // Write message to log file
            unReturnValue = FileIO_WriteStringvf(PpvFileHandle, PwszFormat, argptr);
        }
    }
    WHILE_FALSE_END;

    va_end(argptr);

    return unReturnValue;
}

/**
 *  @brief      This function writes the logging header to the log file, if it is the first call of the current instance
 *  @details
 *
 *  @param      PfFileExists            Flag if the log file exists.
 *  @param      PpFileHandle            Log file handle.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
Logging_WriteHeader(
    _In_    BOOL    PfFileExists,
    _In_    void*   PpFileHandle)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        wchar_t wszTimeStamp[TIMESTAMP_LENGTH];
        unsigned int unTimeStampSize = RG_LEN(wszTimeStamp);
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszTimeStamp, RG_LEN(wszTimeStamp)));

        // Check parameter
        if (NULL == PpFileHandle)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Write header in case this is the first log entry in the current application execution run
        if (TRUE == g_fLogHeader)
        {
            if (TRUE == PfFileExists)
            {
                // In case of appending add new lines to make the restart of the tool more visible
                // Needs \n\n as format string so it will be auto-converted to \r\n\r\n on UEFI.
                unReturnValue = Logging_WriteStringf(LOGGING_LEVEL_1, PpFileHandle, L"\n\n");
                if (RC_SUCCESS != unReturnValue)
                    break;
            }

            // Retrieve current wszTimeStamp including date here
            unReturnValue = Utility_GetTimestamp(TRUE, wszTimeStamp, &unTimeStampSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Write header to log file
            unReturnValue = Logging_WriteStringf(LOGGING_LEVEL_1, PpFileHandle, L"%ls   %ls   Version %ls\n%ls\n\n", IFX_BRAND, GwszToolName, APP_VERSION, wszTimeStamp);
            if (RC_SUCCESS != unReturnValue)
                break;

            g_fLogHeader = FALSE;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      This function opens the logging file
 *  @details    This function opens the logging file, by append an existing or create a new one.
 *              It also checks the file size, if the property storage flag PROPERTY_LOGGING_CHECK_SIZE is set.
 *              In case of the maximum file size is reached the function reopens the file by using override flag.
 *
 *  @param      PppFileHandle           Pointer to store the file handle to. Must be closed by caller in any case.
 *  @param      PpfFileExists           Pointer to store the file exists flag.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
Logging_OpenFile(
    _Inout_ void**  PppFileHandle,
    _Out_   BOOL*   PpfFileExists)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        wchar_t wszLoggingFilePath[MAX_STRING_1024];
        unsigned int unLoggingFilePathBufferSize = RG_LEN(wszLoggingFilePath);
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszLoggingFilePath, RG_LEN(wszLoggingFilePath)));

        // Check out parameter
        if (NULL == PpfFileExists)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Set default
        *PpfFileExists = FALSE;

        // Check in/out parameter
        if (NULL == PppFileHandle || NULL != *PppFileHandle)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Get logging file path
        if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_LOGGING_PATH, wszLoggingFilePath, &unLoggingFilePathBufferSize))
        {
            unReturnValue = RC_E_FAIL;
            break;
        }

        // Check first if file exists and open it in the corresponding mode
        if (FileIO_Exists(wszLoggingFilePath))
        {
            *PpfFileExists = TRUE;
            unReturnValue = FileIO_Open(wszLoggingFilePath, PppFileHandle, FILE_APPEND);
        }
        else
            unReturnValue = FileIO_Open(wszLoggingFilePath, PppFileHandle, FILE_WRITE);

        if (RC_SUCCESS != unReturnValue || NULL == *PppFileHandle)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      This internal function checks the log file size and truncate it
 *  @details    This internal function checks the log file size and truncate it if the property PROPERTY_LOGGING_CHECK_SIZE
 *              is set. On success the property is reset. File handle to the log file must be set.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       An unexpected error occurred. E.g. PropertyStorage not initialized correctly.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
Logging_CheckAndTruncateSize()
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        BOOL fFlag = FALSE;
        // Check if PROPERTY_LOGGING_CHECK_SIZE flag is set in PropertyStorage
        if (PropertyStorage_GetBooleanValueByKey(PROPERTY_LOGGING_CHECK_SIZE, &fFlag) && fFlag)
        {
            unsigned int unMaxFileSize = 0;
            unsigned long long ullFileSize = 0;

            // Get maximum log file size
            if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOGGING_MAXSIZE, &unMaxFileSize))
            {
                unReturnValue = RC_E_FAIL;
                break;
            }

            // Do actual size check only in case max log file size is not 0 (== unlimited)
            if (0 != unMaxFileSize)
            {
                // Get file size of actual logging file
                unReturnValue = FileIO_GetFileSize(s_pFile, &ullFileSize);
                if (RC_SUCCESS != unReturnValue)
                    break;

                // Check log file size limit and reset the file if necessary
                if (unMaxFileSize <= (unsigned int)(ullFileSize / DIV_KILOBYTE))
                {
                    wchar_t wszLoggingPath[260];
                    unsigned int unLoggingPathLen = RG_LEN(wszLoggingPath);
                    unReturnValue = FileIO_Close(&s_pFile);
                    if (RC_SUCCESS != unReturnValue)
                        break;

                    if (!PropertyStorage_GetValueByKey(PROPERTY_LOGGING_PATH, wszLoggingPath, &unLoggingPathLen))
                    {
                        unReturnValue = RC_E_FAIL;
                        break;
                    }

                    unReturnValue = FileIO_Open(wszLoggingPath, &s_pFile, FILE_WRITE);
                    if (RC_SUCCESS != unReturnValue)
                        break;

                    // Write tool header
                    g_fLogHeader = TRUE;
                    unReturnValue = Logging_WriteHeader(FALSE, s_pFile);
                    if (RC_SUCCESS != unReturnValue)
                        break;
                }
            }

            // Reset property for enabling log file size check
            if (!PropertyStorage_ChangeBooleanValueByKey(PROPERTY_LOGGING_CHECK_SIZE, FALSE))
            {
                unReturnValue = RC_E_FAIL;
                break;
            }
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      This writes a message to the log file
 *  @details    This function handles the logging work flow and writes a given message line by line to the
 *              logging file.
 *              It also checks the file size, if the property storage flag PROPERTY_LOGGING_CHECK_SIZE is set.
 *              In case of the maximum file size is reached the function reopens the file by using override flag.
 *
 *  @param      PszCurrentModule            Character string containing the current module name.
 *  @param      PszCurrentFunction          Character string containing the current function name.
 *  @param      PunConfiguredLoggingLevel   Configured logging level.
 *  @param      PunLoggingLevel             Actual logging level.
 *  @param      PwszMessage                 Wide character string containing the message to log.
 *  @param      PunMessageSize              Message size including the zero termination.
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
Logging_WriteMessage(
    _In_z_                          const char*     PszCurrentModule,
    _In_z_                          const char*     PszCurrentFunction,
    _In_                            unsigned int    PunConfiguredLoggingLevel,
    _In_                            unsigned int    PunLoggingLevel,
    _In_z_count_(PunMessageSize)    wchar_t*        PwszMessage,
    _In_                            unsigned int    PunMessageSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    // If logging is already ongoing avoid endless recursion
    if (FALSE == s_fInLogging)
    {
        wchar_t* wszLine = NULL;

        // Signal that logging has been started
        s_fInLogging = TRUE;

        do
        {
            unsigned int unIndex = 0;

            // Check main parameter
            if (NULL == PwszMessage)
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                break;
            }

            // Try to open the log file if caching is disabled
            if (NULL == s_pFile && FALSE == s_fCacheLogEntries)
            {
                BOOL fFileExists = FALSE;
                unReturnValue = Logging_OpenFile(&s_pFile, &fFileExists);
                if (RC_SUCCESS != unReturnValue || NULL == s_pFile)
                    break;

                // Write header if necessary
                unReturnValue = Logging_WriteHeader(fFileExists, s_pFile);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }

            if (s_pFile)
            {
                // Check size of log file and truncate it if required
                unReturnValue = Logging_CheckAndTruncateSize();
                if (RC_SUCCESS != unReturnValue)
                    break;
            }

            // Log the message line by line
            do
            {
                unsigned int unLineSize = 0;
                wchar_t wszTimeStamp[TIMESTAMP_LENGTH];
                unsigned int unTimeStampSize = RG_LEN(wszTimeStamp);

                // Free allocated memory
                Platform_MemoryFree((void**)&wszLine);

                // Get the one line
                unReturnValue = UiUtility_StringGetLine(PwszMessage, PunMessageSize, &unIndex, &wszLine, &unLineSize);
                if (RC_SUCCESS != unReturnValue)
                {
                    if (RC_E_END_OF_STRING != unReturnValue)
                        ERROR_STORE(unReturnValue, L"Utility_StringGetLine returned an unexpected value.");
                    else
                        unReturnValue = RC_SUCCESS;

                    break;
                }

                // For logging level 3 and 4, also write time-stamp to log file
                if (PunConfiguredLoggingLevel >= LOGGING_LEVEL_3)
                {
                    IGNORE_RETURN_VALUE(Platform_StringSetZero(wszTimeStamp, RG_LEN(wszTimeStamp)));
                    // Retrieve current wszTimeStamp without date here
                    IGNORE_RETURN_VALUE(Utility_GetTimestamp(FALSE, wszTimeStamp, &unTimeStampSize));
                }

                // For logging level 4, also write module and function name to log file
                // If the module or function name is not set omit it
                if (PunConfiguredLoggingLevel >= LOGGING_LEVEL_4)
                {
                    // Convert module and function name from ANSI to Unicode
                    wchar_t wszCurrentModule[MAX_NAME];
                    wchar_t wszCurrentFunction[MAX_NAME];

                    IGNORE_RETURN_VALUE(Platform_StringSetZero(wszCurrentModule, RG_LEN(wszCurrentModule)));
                    IGNORE_RETURN_VALUE(Platform_StringSetZero(wszCurrentFunction, RG_LEN(wszCurrentFunction)));
                    IGNORE_RETURN_VALUE(Platform_AnsiString2UnicodeString(wszCurrentModule, RG_LEN(wszCurrentModule), PszCurrentModule));
                    IGNORE_RETURN_VALUE(Platform_AnsiString2UnicodeString(wszCurrentFunction, RG_LEN(wszCurrentFunction), PszCurrentFunction));

                    if (wszCurrentModule[0] != L'\0')
                        unReturnValue = Logging_WriteStringf(PunLoggingLevel, s_pFile, L"%ls %ls - %ls - %ls\n", wszTimeStamp, wszCurrentModule, wszCurrentFunction, wszLine);
                    else
                        unReturnValue = Logging_WriteStringf(PunLoggingLevel, s_pFile, L"%ls %ls - %ls\n", wszTimeStamp, wszCurrentFunction, wszLine);
                }
                else if (PunConfiguredLoggingLevel >= LOGGING_LEVEL_3)
                    unReturnValue = Logging_WriteStringf(PunLoggingLevel, s_pFile, L"%ls %ls\n", wszTimeStamp, wszLine);
                else
                    unReturnValue = Logging_WriteStringf(PunLoggingLevel, s_pFile, L"%ls\n", wszLine);

                if (RC_SUCCESS != unReturnValue)
                    break;

                // Flush file changes if logging level of message is less or equal 3 or configured level is above 4
                if ((PunLoggingLevel <= LOGGING_LEVEL_3 || PunConfiguredLoggingLevel > LOGGING_LEVEL_4) && s_pFile)
                    IGNORE_RETURN_VALUE(FileIO_Flush(s_pFile));
            }
            WHILE_TRUE_END;
        }
        WHILE_FALSE_END;

        // Free allocated memory
        Platform_MemoryFree((void**)&wszLine);

        // Signal that logging is finished
        s_fInLogging = FALSE;
    }

    return unReturnValue;
}

/**
 *  @brief      Logging function
 *  @details    Writes the given text into the configured log.
 *
 *  @param      PszCurrentModule        Pointer to a char array holding the module name (optional, can be NULL).
 *  @param      PszCurrentFunction      Pointer to a char array holding the function name (optional, can be NULL).
 *  @param      PunLoggingLevel         Logging level.
 *  @param      PwszLoggingMessage      Format string used to format the message.
 *  @param      ...                     Parameters needed to format the message.
 */
void
IFXAPI
Logging_WriteLog(
    _In_z_  const char*     PszCurrentModule,
    _In_z_  const char*     PszCurrentFunction,
    _In_    unsigned int    PunLoggingLevel,
    _In_z_  const wchar_t*  PwszLoggingMessage,
    ...)
{
    unsigned int unReturnValue = RC_E_FAIL;
    unsigned int unConfiguredLoggingLevel = 0;

    do
    {
        // Check if caching is enabled to accept all messages up to level 4
        if (s_fCacheLogEntries)
            unConfiguredLoggingLevel = LOGGING_LEVEL_4;
        else
        {
            // Get Logging level
            if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOGGING_LEVEL, &unConfiguredLoggingLevel))
                break;
        }

        if (PunLoggingLevel <= unConfiguredLoggingLevel)
        {
            va_list argptr;

            // Write log message with variable arguments to log file
            // Get pointer to variable arguments
            if (NULL != PwszLoggingMessage)
            {
                wchar_t wszMessage[MAX_MESSAGE_SIZE];
                unsigned int unMessageSize = RG_LEN(wszMessage);
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszMessage, RG_LEN(wszMessage)));

                // Skip formating if a empty line should be written
                if (PwszLoggingMessage[0] != L'\0')
                {
                    // Format message
                    va_start(argptr, PwszLoggingMessage);
                    unReturnValue = Platform_StringFormatV(wszMessage, &unMessageSize, PwszLoggingMessage, argptr);
                    va_end(argptr);
                    if (RC_SUCCESS != unReturnValue)
                        break;
                }
                else
                    unMessageSize = 0;

                // Write actual message line by line
                unReturnValue = Logging_WriteMessage(
                                    PszCurrentModule,
                                    PszCurrentFunction,
                                    unConfiguredLoggingLevel,
                                    PunLoggingLevel,
                                    wszMessage, unMessageSize + 1);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }
        }
    }
    WHILE_FALSE_END;
}

/**
 *  @brief      Log hex dump function
 *  @details    Writes the given data as hex dump to the file.
 *
 *  @param      PszCurrentModule        Pointer to a char array holding the module name (optional, can be NULL).
 *  @param      PszCurrentFunction      Pointer to a char array holding the function name (optional, can be NULL).
 *  @param      PunLoggingLevel         Logging level.
 *  @param      PrgbHexData             Format string used to format the message.
 *  @param      PunSize                 Size of hex data buffer.
 */
void
Logging_WriteHex(
    _In_z_                  const char*     PszCurrentModule,
    _In_z_                  const char*     PszCurrentFunction,
    _In_                    unsigned int    PunLoggingLevel,
    _In_bytecount_(PunSize) const BYTE*     PrgbHexData,
    _In_                    unsigned int    PunSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    unsigned int unConfiguredLoggingLevel = 0;

    do
    {
        // Check parameters
        if (NULL == PrgbHexData || 0 == PunSize)
            break;

        // Get Logging level
        if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOGGING_LEVEL, &unConfiguredLoggingLevel))
            break;

        if (PunLoggingLevel <= unConfiguredLoggingLevel)
        {
            wchar_t wszFormatedHexData[MAX_MESSAGE_SIZE];
            unsigned int unFormatedHexDataSize = RG_LEN(wszFormatedHexData);
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszFormatedHexData, RG_LEN(wszFormatedHexData)));

            unReturnValue = Utility_StringWriteHex(
                                PrgbHexData, PunSize,
                                wszFormatedHexData, &unFormatedHexDataSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Write actual message line by line
            unReturnValue = Logging_WriteMessage(
                                PszCurrentModule,
                                PszCurrentFunction,
                                unConfiguredLoggingLevel,
                                PunLoggingLevel,
                                wszFormatedHexData, unFormatedHexDataSize + 1);
            if (RC_SUCCESS != unReturnValue)
                break;
        }
    }
    WHILE_FALSE_END;
}

/**
 *  @brief      Check if log file is open for writing
 *  @details    Checks if log file is open for writing.
 *
 *  @retval     TRUE                Log file is open.
 *  @retval     FALSE               Log file is not open.
 */
_Check_return_
BOOL
Logging_IsFileOpen()
{
    BOOL fReturnValue = FALSE;
    if (NULL != s_pFile)
    {
        fReturnValue = TRUE;
    }
    return fReturnValue;
}

/**
 *  @brief      Finalize logging
 *  @details    Finalizes the logging by closing the file.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully or file handle was NULL.
 *  @retval     RC_E_FAIL           An unexpected error occurred.
 */
_Check_return_
unsigned int
Logging_Finalize()
{
    unsigned int unReturnValue = RC_E_FAIL;

    // Disable caching
    s_fCacheLogEntries = FALSE;

    // Close file if it is open
    if (NULL != s_pFile)
    {
        unReturnValue = FileIO_Close(&s_pFile);
        s_pFile = NULL;
    }
    else
        unReturnValue = RC_SUCCESS;

    return unReturnValue;
}
