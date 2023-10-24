/**
 *  @brief      Implements utility methods
 *  @details    This module provides helper functions for common and platform independent use
 *  @file       Utility.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Utility.h"

//----------------------------------------------------------------------------------------------
// String Functions
//----------------------------------------------------------------------------------------------
/**
 *  @brief      Returns the index of the first occurrence of the searched character
 *  @details
 *
 *  @param      PwszBuffer              Wide character buffer to search through.
 *  @param      PunSize                 Length of the string buffer in elements (incl. zero termination).
 *  @param      PwchSearch              Wide character to search for.
 *  @param      PpnIndex                -1 if the searched character cannot be found,
 *                                      otherwise the index of the first occurrence in the string buffer
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_StringContainsWChar(
    _In_z_count_(PunSize)           const wchar_t*  PwszBuffer,
    _In_                            unsigned int    PunSize,
    _In_                            wchar_t         PwchSearch,
    _Out_                           int*            PpnIndex)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check parameters
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszBuffer) ||
                0 == PunSize ||
                INT_MAX < PunSize || // Just to be sure, but this may never occur due to stack / heap size limits
                NULL == PpnIndex)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One of the parameters is NULL or not well initialized.");
            break;
        }

        // Walk over the string and try to find the character the caller searched for
        {
            unsigned int unIndex = 0;
            for (; unIndex < PunSize; unIndex++)
                if (PwchSearch == PwszBuffer[unIndex])
                    break;

            // In case we have reached the end of the string, then the searched character is not there
            if (unIndex < PunSize)
                *PpnIndex = unIndex;
            else
                *PpnIndex = -1;

            unReturnValue = RC_SUCCESS;
        }
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
        if (NULL != PpnIndex)
            // Reset out parameter
            *PpnIndex = -1;

    return unReturnValue;
}

/**
 *  @brief      Converts an unsigned long long to a string
 *  @details    This function converts the unsigned long long to a wide character, zero terminated string.
 *
 *  @param      PullValue               Unsigned long long value to convert.
 *  @param      PwszBuffer              Pointer to a wide character buffer.
 *  @param      PpunBufferSize          Pointer to a unsigned long long
 *                                      IN: Size of buffer in elements
 *                                      OUT: Length of written characters without zero termination
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     ...                     Error codes from Platform_StringFormat.
 */
_Check_return_
unsigned int
Utility_ULongLong2String(
    _In_                            unsigned long long  PullValue,
    _Out_z_cap_(*PpunBufferSize)    wchar_t*            PwszBuffer,
    _Inout_                         unsigned int*       PpunBufferSize)
{
    return Platform_StringFormat(PwszBuffer, PpunBufferSize, L"%llu", PullValue);
}

/**
 *  @brief      Converts an unsigned integer to a string
 *  @details    This function converts the unsigned integer to a wide character, zero terminated string.
 *
 *  @param      PunValue                Unsigned integer value to convert.
 *  @param      PwszBuffer              Pointer to a wide character buffer.
 *  @param      PpunBufferSize          Pointer to a unsigned integer
 *                                      IN: Size of buffer in elements
 *                                      OUT: Length of written characters without zero termination
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     ...                     Error codes from Platform_StringFormat.
 */
_Check_return_
unsigned int
Utility_UInteger2String(
    _In_                            unsigned int    PunValue,
    _Out_z_cap_(*PpunBufferSize)    wchar_t*        PwszBuffer,
    _Inout_                         unsigned int*   PpunBufferSize)
{
    return Platform_StringFormat(PwszBuffer, PpunBufferSize, L"%lu", PunValue);
}

/**
 *  @brief      Converts a hex string with a leading '0x' or '0X' header to an unsigned integer
 *  @details    This function converts a hex string with a leading '0x' or '0X' header to an unsigned integer
 *
 *  @param      PwszBuffer              Pointer to a wide character buffer containing the hex value.
 *  @param      PpunValue               Pointer to an unsigned integer
 *                                      OUT: The value of the input string
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      In case of an invalid character in the input string or if the hex value
 *                                      overflows the unsigned integer range
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_UIntegerParseHexString(
    _In_z_  const wchar_t*  PwszBuffer,
    _Out_   unsigned int*   PpunValue)
{
    unsigned int unIndex = 0, unCharValue = 0;
    unsigned int unReturnValue = RC_SUCCESS;
    unsigned long long ullValue = 0;
    *PpunValue = 0;

    // Remove "0x" or "0X"
    if (PwszBuffer[0] == '0' && (PwszBuffer[1] == 'x' || PwszBuffer[1] == 'X'))
        unIndex = 2;

    while (PwszBuffer[unIndex] != '\0')
    {
        // Get the next character and convert it to an integer
        if (PwszBuffer[unIndex] >= 'A' && PwszBuffer[unIndex] <= 'F')
            unCharValue = PwszBuffer[unIndex] - 'A' + 10;
        else if (PwszBuffer[unIndex] >= 'a' && PwszBuffer[unIndex] <= 'f')
            unCharValue = PwszBuffer[unIndex] - 'a' + 10;
        else if (PwszBuffer[unIndex] >= '0' && PwszBuffer[unIndex] <= '9')
            unCharValue = PwszBuffer[unIndex] - '0';
        else
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Invalid character in input string detected");
            break;
        }

        // Update the cumulative value
        ullValue = (16 * (ullValue)) + unCharValue;

        // Check for UINT_MAX overflow
        if (ullValue > UINT_MAX)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"The input string overflows UINT_MAX");
            break;
        }

        ++unIndex;
    }

    if (RC_SUCCESS == unReturnValue)
    {
        *PpunValue = (unsigned int)ullValue;
    }

    return unReturnValue;
}

/**
 *  @brief      Converts a string to an unsigned integer value
 *  @details    This function converts the wide characters from a string to an unsigned decimal number, if possible
 *
 *  @param      PwszBuffer              Pointer to the wide character string buffer.
 *  @param      PunMaximumCapacity      Maximum capacity of the string buffer in elements (including terminating zero).
 *  @param      PpunNumber              Pointer to an integer variable.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      In case of an invalid character in the input string or if the hex value
 *                                      overflows the unsigned integer range
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_StringParseUInteger(
    _In_z_count_(PunMaximumCapacity)    const wchar_t*  PwszBuffer,
    _In_                                unsigned int    PunMaximumCapacity,
    _Out_                               unsigned int*   PpunNumber)
{
    unsigned int unReturnValue = RC_SUCCESS;
    unsigned long long ullNumber = 0;

    do
    {
        unsigned int unIndex;
        wchar_t wchChar;
        unsigned int unStringLength = 0;

        // Check out parameter
        if (NULL == PpunNumber)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Output parameter is NULL.");
            break;
        }

        *PpunNumber = 0;

        // Check parameter
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszBuffer))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Input buffer is NULL or empty.");
            break;
        }

        // Get string length
        unReturnValue = Platform_StringGetLength(PwszBuffer, PunMaximumCapacity, &unStringLength);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Read characters and check if they are numbers
        for (unIndex = 0; unIndex < unStringLength; unIndex++)
        {
            // Update the cumulative value
            ullNumber = ullNumber * 10;
            wchChar = PwszBuffer[unIndex];
            if (wchChar >= L'0' && wchChar <= L'9')
                ullNumber += wchChar - '0';
            else
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                ERROR_STORE(unReturnValue, L"Input is not a decimal number.");
                *PpunNumber = 0;
                break;
            }

            // Check for UINT_MAX overflow
            if (ullNumber > UINT_MAX)
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                ERROR_STORE(unReturnValue, L"Input is larger than UINT_MAX.");
                *PpunNumber = 0;
                break;
            }
        }

        if (RC_SUCCESS != unReturnValue)
            break;

        *PpunNumber = (unsigned int)ullNumber;
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Prints data from a byte array to a string in formatted HEX style
 *  @details
 *
 *  @param      PrgbHexData                 Pointer to a buffer with the data.
 *  @param      PunSize                     Count of byte in the input buffer in elements.
 *  @param      PwszFormattedHexData        Pointer to the buffer containing the formatted hex data.
 *  @param      PpunFormattedHexDataSize    In:     Capacity of the destination buffer\n
 *                                          Out:    Count of written bytes
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. It was either NULL or invalid.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from Platform_StringFormat.
 */
_Check_return_
unsigned int
Utility_StringWriteHex(
    _In_bytecount_(PunSize)                     const BYTE*     PrgbHexData,
    _In_                                        unsigned int    PunSize,
    _Out_z_cap_(*PpunFormattedHexDataSize)      wchar_t*        PwszFormattedHexData,
    _Inout_                                     unsigned int*   PpunFormattedHexDataSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        unsigned int unDestinationSize = 0;
        unsigned int unIndex = 0;
        wchar_t wszHelper[HEX_STRING_OUT_LENGTH * 2];
        unsigned int unHelperSize = RG_LEN(wszHelper);
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszHelper, RG_LEN(wszHelper)));

        if (NULL == PrgbHexData || NULL == PwszFormattedHexData || NULL == PpunFormattedHexDataSize || *PpunFormattedHexDataSize == 0)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One of the input parameters is invalid.");
            break;
        }

        PwszFormattedHexData[0] = '\0';

        // Log buffer content
        for (unIndex = 0; unIndex < PunSize; unIndex++)
        {
            // Line header
            if (unIndex % LOGGING_HEX_CHARS_PER_LINE == 0)
            {
                // Add a new line to the beginning of each hex line written except the first one
                unDestinationSize = *PpunFormattedHexDataSize;
                if (unIndex != 0)
                {
                    unsigned int unLength = 0;
                    unsigned int unSize = 0;
                    unReturnValue = Platform_StringGetLength(PwszFormattedHexData, unDestinationSize, &unLength);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"Platform_StringGetLength returned an unexpected value.");
                        break;
                    }
                    unSize = *PpunFormattedHexDataSize - unLength;
                    unReturnValue = Platform_StringFormat(&PwszFormattedHexData[unLength], &unSize, L"\n");
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
                        break;
                    }
                }

                // Convert current byte to string
                unHelperSize = HEX_STRING_OUT_LENGTH * 2;
                unReturnValue = Platform_StringFormat(wszHelper, &unHelperSize, L"%.4X: ", unIndex);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
                    break;
                }

                // Add header to output
                unDestinationSize = *PpunFormattedHexDataSize;
                unReturnValue = Platform_StringConcatenate(PwszFormattedHexData, &unDestinationSize, wszHelper);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_StringConcatenate returned an unexpected value.");
                    break;
                }
            }

            // Hex characters
            unHelperSize = HEX_STRING_OUT_LENGTH;
            unReturnValue = Platform_StringFormat(wszHelper, &unHelperSize, L"%.2X ", PrgbHexData[unIndex]);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
                break;
            }

            // Concatenate string for current byte with string for previous bytes
            unDestinationSize = *PpunFormattedHexDataSize;
            unReturnValue = Platform_StringConcatenate(PwszFormattedHexData, &unDestinationSize, wszHelper);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringConcatenate returned an unexpected value.");
                break;
            }

            // Additional space after 8th character
            if ((int)unIndex % LOGGING_HEX_CHARS_PER_LINE == LOGGING_HEX_CHARS_PER_LINE / 2 - 1)
            {
                // Concatenate string for current byte with string for previous bytes
                unDestinationSize = *PpunFormattedHexDataSize;
                unReturnValue = Platform_StringConcatenate(PwszFormattedHexData, &unDestinationSize, L" ");
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_StringConcatenate returned an unexpected value.");
                    break;
                }
            }
        }

        // Update output size
        unReturnValue = Platform_StringGetLength(PwszFormattedHexData, *PpunFormattedHexDataSize, PpunFormattedHexDataSize);
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        // Reset out parameters
        if (NULL != PwszFormattedHexData)
            PwszFormattedHexData[0] = L'\0';
        if (NULL != PpunFormattedHexDataSize)
            *PpunFormattedHexDataSize = 0;
    }

    return unReturnValue;
}

/**
 *  @brief      Formats a time-stamp structure to an output string.
 *  @details    The output string can contain date information or not.
 *
 *  @param      PpTime                  Pointer to a IfxTime structure, containing time-stamp info.
 *  @param      PfDate                  If '1' the output contains a date, if '0' not.
 *  @param      PwszValue               Pointer to a wide character buffer to fill in the value.
 *  @param      PpunValueSize           In:     Size of the value buffer in elements including the zero termination.\n
 *                                      Out:    Length of the filled in value in elements without zero termination.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case of any output buffer size is too small.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
_Success_(return == 0)
unsigned int
Utility_Timestamp2String(
    _In_                        const IfxTime*  PpTime,
    _In_                        BOOL            PfDate,
    _Out_z_cap_(*PpunValueSize) wchar_t*        PwszValue,
    _Inout_                     unsigned int*   PpunValueSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    wchar_t wszDate[DATE_LENGTH];
    unsigned int unDateSize = RG_LEN(wszDate);
    IGNORE_RETURN_VALUE(Platform_StringSetZero(wszDate, RG_LEN(wszDate)));

    do
    {
        // Parameter check
        if (NULL == PpTime || NULL == PwszValue || NULL == PpunValueSize || 0 == *PpunValueSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more parameters are invalid");
            break;
        }

        // Build date
        if (TRUE == PfDate)
        {
            unReturnValue = Platform_StringFormat(wszDate, &unDateSize, L"[%04d-%02d-%02d ", PpTime->unYear, PpTime->unMonth, PpTime->unDay);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringFormat failed");
                break;
            }
        }
        else
        {
            unReturnValue = Platform_StringFormat(wszDate, &unDateSize, L"[");
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringFormat failed");
                break;
            }
        }

        // Build time
        if (TRUE == PpTime->fMillisecondAvailable)
        {
            unReturnValue = Platform_StringFormat(PwszValue, PpunValueSize, L"%ls%02d:%02d:%02d.%03d]", wszDate, PpTime->unHour, PpTime->unMinute, PpTime->unSecond, PpTime->nMillisecond);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringFormat failed");
                break;
            }
        }
        else
        {
            unReturnValue = Platform_StringFormat(PwszValue, PpunValueSize, L"%ls%02d:%02d:%02d]", wszDate, PpTime->unHour, PpTime->unMinute, PpTime->unSecond);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringFormat failed");
                break;
            }
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Gets a time-stamp with or without date as string.
 *  @details    The output string can contain date information or not.
 *
 *  @param      PfDate                  If '1' the output contains a date, if '0' not.
 *  @param      PwszValue               Pointer to a wide character buffer to fill in the value.
 *  @param      PpunValueSize           In:     Size of the value buffer in elements including the zero termination.\n
 *                                      Out:    Length of the filled in value in elements without zero termination.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case of any output buffer size is too small.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
_Success_(return == 0)
unsigned int
Utility_GetTimestamp(
    _In_                        BOOL            PfDate,
    _Out_z_cap_(*PpunValueSize) wchar_t*        PwszValue,
    _Inout_                     unsigned int*   PpunValueSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        IfxTime sTime;

        Platform_MemorySet(&sTime, 0, sizeof(sTime));

        // Parameter check
        if (NULL == PwszValue || NULL == PpunValueSize || 0 == *PpunValueSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more parameters are invalid");
            break;
        }

        // Get current date and time
        unReturnValue = Platform_GetTime(&sTime);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_GetTime failed");
            break;
        }

        // Convert current time to string
        unReturnValue = Utility_Timestamp2String(&sTime, PfDate, PwszValue, PpunValueSize);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Utility_Timestamp2String failed");
            break;
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Converts a string to an unsigned long long value
 *  @details    This function converts the wide characters from a string to an unsigned long long number, if possible
 *
 *  @param      PwszBuffer              Pointer to the wide character string buffer.
 *  @param      PunMaximumCapacity      Maximum capacity of the string buffer in elements (including terminating zero).
 *  @param      PpullNumber             Pointer to an unsigned long long variable.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. Or the parameter overflows UINT64_MAX.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_StringParseULongLong(
    _In_z_count_(PunMaximumCapacity)    const wchar_t*      PwszBuffer,
    _In_                                unsigned int        PunMaximumCapacity,
    _Out_                               unsigned long long* PpullNumber)
{
    unsigned int unReturnValue = RC_SUCCESS;
    unsigned long long ullNumber = 0;

    do
    {
        unsigned int unIndex = 0;
        wchar_t wchChar = L'\0';
        unsigned int unStringLength = 0;

        // Check out parameter
        if (NULL == PpullNumber)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Output parameter is NULL.");
            break;
        }

        *PpullNumber = 0;

        // Check parameter
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszBuffer))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Input buffer is NULL or empty.");
            break;
        }

        // Get string length
        unReturnValue = Platform_StringGetLength(PwszBuffer, PunMaximumCapacity, &unStringLength);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Read characters and check if they are numbers
        for (unIndex = 0; unIndex < unStringLength; unIndex++)
        {
            // Check for multiplication overflow
            if (UINT64_MAX / 10 < ullNumber)
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                ERROR_STORE(unReturnValue, L"Input overflows UINT64_MAX.");
                break;
            }
            // Multiply...
            ullNumber = ullNumber * 10;

            wchChar = PwszBuffer[unIndex];
            if (wchChar >= L'0' && wchChar <= L'9')
            {
                unsigned int unAdder = wchChar - '0';
                // Check for addition overflow
                if (UINT64_MAX - unAdder < ullNumber)
                {
                    unReturnValue = RC_E_BAD_PARAMETER;
                    ERROR_STORE(unReturnValue, L"Input overflows UINT64_MAX.");
                    break;
                }
                ullNumber += unAdder;
            }
            else
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                ERROR_STORE(unReturnValue, L"Input is no decimal number.");
                break;
            }
        }

        if (RC_SUCCESS != unReturnValue)
            break;

        *PpullNumber = ullNumber;
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}
