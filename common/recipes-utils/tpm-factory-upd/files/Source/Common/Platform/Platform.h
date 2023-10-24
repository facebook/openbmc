/**
 *  @brief      Declares the Memory interface
 *  @details    This module provides platform related functions (memory allocation, string manipulation, time, etc.).
 *  @file       Platform.h
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

#ifdef __cplusplus
extern "C" {
#endif

/// Checks if a string is NULL or empty
#define PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszString) (NULL == PwszString || (0 == Platform_StringCompare(PwszString, L"", 1, FALSE)))

/// This is the string length of a byte when printed as HEX with a trailing whitespace and a null-termination
#define HEX_STRING_OUT_LENGTH 4

/// This is an invalid value for milliseconds and will be used on systems with an RTC not supporting millisecond readout.
#define MIL_SEC_INVAL -1

/**
 *  @brief      This structure contains platform independent time and date information
 *  @details
 */
/// Contains date and time information. The last valid values are '03:14:07, January 19, 2038' on 32 bit machines and '23:59:59, December 31, 3000' on 64 bit machines.
typedef struct tdIfxTime
{
    /// Flag indicating if millisecond accuracy is available; if flag is FALSE, the value of nMillisecond is set to MIL_SEC_INVAL
    BOOL fMillisecondAvailable;

    /// The millisecond of the second (semantically valid value range: 0-999; MIL_SEC_INVAL in case fMillisecondAvailable is FALSE)
    int nMillisecond;

    /// The second of the minute (semantically valid value range: 0 - 59)
    unsigned int unSecond;

    /// The minute of the hour (semantically valid value range: 0 - 59)
    unsigned int unMinute;

    /// The hour of the day (semantically valid value range: 0 - 23)
    unsigned int unHour;

    /// The day of the month (semantically valid value range: 1 - 31)
    unsigned int unDay;

    /// The month of the year (semantically valid value range: 1 - 12)
    unsigned int unMonth;

    /// The year (semantically valid value range: 1900 - 2038 or 3000; depends on CPU architecture)
    unsigned int unYear;
} IfxTime;

/**
 *  @brief      Structure for TPM Firmware Version
 *  @details
 */
typedef struct tdIfxVersion
{
    /// Number of Major Version
    unsigned int            unMajor;
    /// Number of Minor Version
    unsigned int            unMinor;
    /// Number of Build Version
    unsigned int            unBuild;
    /// Number of Version
    unsigned int            unExtension;
} IfxVersion;

/**
 *  @brief      Memory allocation initialized with zeros
 *  @details    This function returns a pointer to a zero initialized memory
 *
 *  @param      PunSize     Memory allocation size in bytes.
 *  @retval     != NULL     Pointer to the zero initialized memory.
 *  @retval     NULL        If the allocation fails.
 */
_Check_return_
void*
Platform_MemoryAllocateZero(
    _In_ unsigned int PunSize);

/**
 *  @brief      Memory deallocation
 *  @details    This function releases the allocated memory and sets its pointer to NULL
 *
 *  @param      PppvMemory  Pointer to the pointer to the memory which should be released.
 */
void
Platform_MemoryFree(
    _Inout_opt_ void** PppvMemory);

/**
 *  @brief      Memory compare
 *  @details    This function compares 2 memory buffers
 *
 *  @param      PpvBuffer1      Pointer to the first buffer.
 *  @param      PpvBuffer2      Pointer to the second buffer.
 *  @param      PunSize         Size to compare.
 *  @retval     0               If buffers are equal.
 *  @retval     < 0             If first buffer is less than second one.
 *  @retval     > 0             If first buffer is greater than the second one.
 */
_Check_return_
int
Platform_MemoryCompare(
    _In_reads_bytes_(PunSize)   const void*     PpvBuffer1,
    _In_reads_bytes_(PunSize)   const void*     PpvBuffer2,
    _In_                        unsigned int    PunSize);

/**
 *  @brief      Memory Set
 *  @details    This function sets the memory to a value
 *
 *  @param      PpvDestination      Pointer to the buffer.
 *  @param      PnValue             Value to set.
 *  @param      PunSize             Size of the buffer in bytes.
 */
void
Platform_MemorySet(
    _Out_writes_bytes_all_(PunSize) void*           PpvDestination,
    _In_                            int             PnValue,
    _In_                            unsigned int    PunSize);

/**
 *  @brief      Memory copy
 *  @details    This function copies one memory buffer to another
 *
 *  @param      PpvDestination          Pointer to the destination buffer.
 *  @param      PunDestinationCapacity  Capacity of the destination buffer.
 *  @param      PpvSource               Pointer to the source buffer.
 *  @param      PunSize                 Size of the buffer in bytes.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. One parameter is NULL.
 *  @retval     RC_E_BUFFER_TOO_SMALL   Destination parameter too small.
 */
_Check_return_
unsigned int
Platform_MemoryCopy(
    _Inout_bytecap_(PunDestinationCapacity) void*           PpvDestination,
    _In_                                    unsigned int    PunDestinationCapacity,
    _In_reads_bytes_opt_(PunSize)           const void*     PpvSource,
    _In_                                    unsigned int    PunSize);

/**
 *  @brief      Copy Unicode strings
 *  @details    This function copies the source Unicode string to the destination.
 *
 *  @param      PwszDestination         Pointer to the destination Unicode buffer.
 *  @param      PpunDestinationCapacity In: Capacity of the destination buffer in elements (include additional space for terminating 0)\n
 *                                      Out: Number of elements copied to the destination buffer (without terminating 0)
 *  @param      PwszSource              Pointer to the source Unicode buffer.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. One parameter is NULL.
 *  @retval     RC_E_BUFFER_TOO_SMALL   Destination parameter too small.
 */
_Check_return_
unsigned int
Platform_StringCopy(
    _Out_z_cap_(*PpunDestinationCapacity)   wchar_t*        PwszDestination,
    _Inout_                                 unsigned int*   PpunDestinationCapacity,
    _In_z_                                  const wchar_t*  PwszSource);

/**
 *  @brief      Compare strings
 *  @details    This function compares two strings
 *
 *  @param      PwszString1         First string to compare.
 *  @param      PwszString2         Second string to compare.
 *  @param      PunCount            The maximum number of elements to compare.
 *  @param      PfCaseInsensitive   If set to TRUE, do a case insensitive compare; FALSE for case sensitive compare.
 *  @retval     0                   If strings match.
 *  @retval     <0                  string 1 less than string 2.
 *  @retval     >0                  string 1 greater than string 2.
 */
_Check_return_
int
Platform_StringCompare(
    _In_z_  const wchar_t*  PwszString1,
    _In_z_  const wchar_t*  PwszString2,
    _In_    unsigned int    PunCount,
    _In_    BOOL            PfCaseInsensitive);

/**
 *  @brief      Format Unicode strings
 *  @details    This function formats a Unicode string
 *
 *  @param      PwszDestination         Pointer to the destination Unicode buffer.
 *  @param      PpunDestinationCapacity In:     Capacity of the destination buffer in elements\n
 *                                      Out:    Count of written elements
 *  @param      PwszSource              Pointer to the source Unicode buffer.
 *  @param      ...                     Additional parameters to format the string.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. One parameter is NULL.
 *  @retval     ...                     Error codes from Platform_StringFormatV.
 */
_Check_return_
unsigned int
IFXAPI
Platform_StringFormat(
    _Out_z_cap_(*PpunDestinationCapacity)   wchar_t*        PwszDestination,
    _Inout_                                 unsigned int*   PpunDestinationCapacity,
    _In_z_                                  const wchar_t*  PwszSource,
    ...);

/**
 *  @brief      Format Unicode strings using a va_list
 *  @details    This function formats a Unicode string using a va_list
 *
 *  @param      PwszDestination         Pointer to the destination Unicode buffer.
 *  @param      PpunDestinationCapacity In:     Capacity of the destination buffer in elements\n
 *                                      Out:    Count of written elements
 *  @param      PwszSource              Pointer to the source Unicode buffer.
 *  @param      PargList                Additional parameters to format the string.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. One parameter is NULL.
 *  @retval     RC_E_BUFFER_TOO_SMALL   Destination parameter too small.
 *  @retval     RC_E_INTERNAL           An internal error occurs (only on windows and linux).
 */
_Check_return_
unsigned int
Platform_StringFormatV(
    _Out_z_cap_(*PpunDestinationCapacity)   wchar_t*        PwszDestination,
    _Inout_                                 unsigned int*   PpunDestinationCapacity,
    _In_z_                                  const wchar_t*  PwszSource,
    _In_                                    va_list         PargList);

/**
 *  @brief      Get Unicode string length
 *  @details    This function returns the length of a Unicode string in elements without the terminating 0
 *
 *  @param      PwszBuffer              Pointer to the Unicode string buffer.
 *  @param      PunMaximumCapacity      Maximum capacity of the string buffer in elements (including terminating 0).
 *  @param      PpunStrLen              Length of the Unicode string in elements (without terminating 0)
 *  @returns    RC_SUCCESS  The operation completed successfully. In case of an error, *PpunStrLen is set to 0 if possible and one of the following error codes is being returned:
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PwszBuffer or PpunStrLen is NULL, or PunMaximumCapacity is 0.
 *  @retval     RC_E_BUFFER_TOO_SMALL   String is not null-terminated within given maximum capacity.
 */
_Check_return_
unsigned int
Platform_StringGetLength(
    _In_z_  const wchar_t*  PwszBuffer,
    _In_    unsigned int    PunMaximumCapacity,
    _Out_   unsigned int*   PpunStrLen);

/**
 *  @brief      Concatenates Unicode strings
 *  @details    This function concatenates destination and source Unicode strings.
 *
 *  @param      PwszDestination             In: Pointer to the destination Unicode buffer
 *                                          Out: Pointer to the concatenated Unicode buffer
 *  @param      PpunDestinationCapacity     In: Capacity of the destination buffer in elements (include additional space for terminating 0)\n
 *                                          Out: Number of elements in the destination buffer (without terminating 0)
 *  @param      PwszSource                  Pointer to the source Unicode buffer.
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. One parameter is NULL or strings overlap.
 *  @retval     RC_E_BUFFER_TOO_SMALL       Destination buffer is too small.
 */
_Check_return_
unsigned int
Platform_StringConcatenate(
    _Inout_updates_z_(*PpunDestinationCapacity) wchar_t*        PwszDestination,
    _Inout_                                     unsigned int*   PpunDestinationCapacity,
    _In_z_                                      const wchar_t*  PwszSource);

/**
 *  @brief      Concatenates Unicode strings representing two paths
 *  @details    This function concatenates destination and source Unicode strings representing two paths. The function adds if necessary
 *              a '\' or '/' depending on the operating system.
 *
 *  @param      PwszDestination             In: Pointer to the destination Unicode buffer
 *                                          Out: Pointer to the concatenated Unicode buffer
 *  @param      PpunDestinationCapacity     In: Capacity of the destination buffer in elements (include additional space for terminating 0)\n
 *                                          Out: Number of elements in the destination buffer (without terminating 0)
 *  @param      PwszSource                  Pointer to the source Unicode buffer.
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. One parameter is NULL or strings overlap.
 *  @retval     RC_E_BUFFER_TOO_SMALL       Destination buffer is too small.
 */
_Check_return_
unsigned int
Platform_StringConcatenatePaths(
    _Inout_updates_z_(*PpunDestinationCapacity) wchar_t*        PwszDestination,
    _Inout_                                     unsigned int*   PpunDestinationCapacity,
    _In_z_                                      const wchar_t*  PwszSource);

/**
 *  @brief      Convert an ANSI string to a Unicode string
 *  @details    This function converts an ANSI string to a Unicode string
 *
 *  @param      PwszDestination             Pointer to the destination Unicode string buffer.
 *  @param      PunDestinationCapacity      Size of the destination Unicode string buffer in wide characters.
 *  @param      PszSource                   Pointer to the ANSI string buffer.
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. One parameter is NULL.
 *  @retval     RC_E_BUFFER_TOO_SMALL       Destination buffer is too small.
 */
_Check_return_
unsigned int
Platform_AnsiString2UnicodeString(
    _Inout_updates_z_(PunDestinationCapacity)   wchar_t*        PwszDestination,
    _In_                                        unsigned int    PunDestinationCapacity,
    _In_z_                                      const char*     PszSource);

/**
 *  @brief      Convert a string to integer
 *  @details    This function returns the int value produced by interpreting the input characters as a decimal number
 *
 *  @param      PwszBuffer              Pointer to the Unicode string buffer
 *  @returns    Decimal numerical value of the interpreted string as integer
 */
_Check_return_
int
Platform_String2Int(
    _In_z_ const wchar_t* PwszBuffer);

/**
 *  @brief      Set a String to zero wide characters
 *  @details    This function sets all wide characters of the buffer to zero
 *
 *  @param      PwszBuffer          Pointer to the Unicode string buffer.
 *  @param      PunBufferSize       Size of wchar_t buffer in elements.
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. One parameter is NULL.
 */
_Check_return_
unsigned int
Platform_StringSetZero(
    _Out_writes_z_(PunBufferSize)   wchar_t*        PwszBuffer,
    _In_                            unsigned int    PunBufferSize);

/**
 *  @brief      Converts a wide character to upper case
 *  @details
 *
 *  @param      PwchToUpper     Wide character to be converted
 *  @returns    Converted upper case wide character
 */
_Check_return_
wchar_t
Platform_WCharToUpper(
    _In_ wchar_t PwchToUpper);

/**
 *  @brief      Gets the current time
 *  @details    Retrieves the current local date and time. The accuracy is platform and OS dependent.
 *
 *  @param      PpTime                  The current local time; accuracy is given in this structure and is platform and OS dependent.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL               An unexpected error occurred. Returned from the system call.
 */
_Check_return_
unsigned int
Platform_GetTime(
    _Inout_ IfxTime* PpTime);

/**
 *  @brief      Sleeps the given time in milliseconds
 *  @details
 *
 *  @param      PunSleepTime            Time to sleep in milliseconds.
 */
void
Platform_Sleep(
    _In_ unsigned int PunSleepTime);

/**
 *  @brief      Sleeps the given time in microseconds.
 *  @details
 *
 *  @param      PunSleepTime    Time to sleep in microseconds.
 */
void
Platform_SleepMicroSeconds(
    _In_ unsigned int PunSleepTime);

/**
 *  @brief      Swaps a UINT16
 *  @details
 *
 *  @param      PusValue
 *  @returns    Swapped UINT16 value
 */
unsigned short
Platform_SwapBytes16(
    _In_ unsigned short PusValue);

/**
 *  @brief      Swaps a UINT32
 *  @details
 *
 *  @param      PunValue
 *  @returns    Swapped UINT32 value
 */
unsigned int
Platform_SwapBytes32(
    _In_ unsigned int PunValue);

/**
 *  @brief      Unmarshal a Unicode string (16bit per character) to the target platform
 *  @details
 *
 *  @param      PrgbBuffer              Binary buffer.
 *  @param      PunBufferLen            Length of binary buffer.
 *  @param      PwszTargetString        Receives the unmarshalled string including null-termination.
 *  @param      PpunTargetStringLen     On input the capacity of the wchar_t buffer in elements.
 *                                      On output the length of the wchar_t string in elements without terminating NULL character.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_BUFFER_TOO_SMALL   The buffer in PwszTargetString is too small to hold the contents of the binary buffer.
 *  @retval     RC_E_FAIL               An internal error occurred. Either there was not enough memory to create a temporary copy of PrgbBuffer or StrnCpyS failed.
 */
_Check_return_
unsigned int
Platform_UnmarshalString(
    _In_bytecount_(PunBufferLen)            const void*     PrgbBuffer,
    _In_                                    unsigned int    PunBufferLen,
    _Out_writes_z_(*PpunTargetStringLen)    wchar_t*        PwszTargetString,
    _Inout_                                 unsigned int*   PpunTargetStringLen);

/**
 *  @brief      Finds a string in another string.
 *  @details
 *
 *  @param      PwszSearch              String to search for.
 *  @param      PwszString              String to search in.
 *  @param      PpwszStart              If return code is RC_SUCCESS, returns the start position of the PwszSearch in PwszString.
 *
 *  @retval     RC_SUCCESS              Search string was found in the actual string.
 *  @retval     RC_E_NOT_FOUND          Search string was not found in the actual string.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 */
_Check_return_
unsigned int
Platform_FindString(
    _In_z_  const wchar_t*  PwszSearch,
    _In_z_  const wchar_t*  PwszString,
    _Out_   wchar_t**       PpwszStart);

#ifdef __cplusplus
}
#endif
