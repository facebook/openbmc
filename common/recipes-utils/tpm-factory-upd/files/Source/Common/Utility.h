/**
 *  @brief      Declares utility methods
 *  @details    This module provides helper functions for common and platform independent use
 *  @file       Utility.h
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
#include "Platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//----------------------------------------------------------------------------------------------
// String Functions
//----------------------------------------------------------------------------------------------

/// The maximum length of the date string returned by the TPM (length must fit also for output in case of parsing errors)
#define DATE_LENGTH 16

/// The maximum length of the TimeStamp string
#define TIMESTAMP_LENGTH 30

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
    _Out_                           int*            PpnIndex);

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
    _Inout_                         unsigned int*       PpunBufferSize);

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
    _Inout_                         unsigned int*   PpunBufferSize);

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
    _Out_   unsigned int*   PpunValue);

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
    _Out_                               unsigned int*   PpunNumber);

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
    _Inout_                                     unsigned int*   PpunFormattedHexDataSize);

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
    _Inout_                     unsigned int*   PpunValueSize);

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
    _Inout_                     unsigned int*   PpunValueSize);

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
    _Out_                               unsigned long long* PpullNumber);

#ifdef __cplusplus
}
#endif
