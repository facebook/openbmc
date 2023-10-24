/**
 *  @brief      Declares utility methods
 *  @details    This module provides helper functions for common and platform independent use
 *  @file       UiUtility.h
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

/// The maximum length of the manufacturer string returned by the TPM
#define MANU_LENGTH 5

/**
 *  @brief      Gets a line from a buffer.
 *  @details    This function searches for a line end ('\n' or '\0'), allocates and copies the line
 *
 *  @param      PwszBuffer              String buffer to get the line from.
 *  @param      PunSize                 Length of the string buffer in elements (incl. zero termination).
 *  @param      PpunIndex               In: Position to start searching in the string buffer\n
 *                                      Out: Position in the string for the next search
 *  @param      PpwszLine               Pointer to a string buffer. The function allocates memory for a
 *                                      line string buffer and sets this pointer to the address of the
 *                                      allocated memory. The caller must free the allocated line buffer!
 *  @param      PpunLineSize            The length of the line in elements including the zero termination.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *  @retval     RC_E_END_OF_STRING      In case the end of the string was reached.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
UiUtility_StringGetLine(
    _In_z_count_(PunSize)           const wchar_t*  PwszBuffer,
    _In_                            unsigned int    PunSize,
    _Inout_                         unsigned int*   PpunIndex,
    _Inout_                         wchar_t**       PpwszLine,
    _Out_                           unsigned int*   PpunLineSize);

/**
 *  @brief      Convert TPM ASCII value to string
 *  @details    Used to show string instead of HEX value
 *
 *  @param      PwszBuffer              Pointer to text string (destination).
 *  @param      PunBufferSize           Size of PwszBuffer in elements (incl. zero termination).
 *  @param      PpunValue               Pointer to value to convert (source).
 *  @param      ...                     Variable arguments list.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. PwszBuffer is NULL.
 *  @retval     RC_E_BUFFER_TOO_SMALL   PunBufferSize is too small.
 */
_Check_return_
unsigned int
IFXAPI
UiUtility_StringParseTpmAscii(
    _Out_z_cap_(PunBufferSize)  wchar_t*            PpBuffer,
    _In_                        unsigned int        PunBufferSize,
    _In_                        const unsigned int* PpunValue,
    ...);

/**
 *  @brief      Scans a hex string into a byte array
 *  @details
 *
 *  @param      PwszSource              Source string containing the hex representation (no white spaces)
 *                                      Function stops scanning when the first element cannot be converted to a byte value.
 *  @param      PrgbDestination         Pointer to a byte array to fill in the scanned bytes.
 *  @param      PpunDestinationSize     In:     Capacity of the destination buffer in bytes\n
 *                                      Out:    Count of written bytes
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     ...                     Error codes from Platform_StringGetLength.
 */
_Check_return_
unsigned int
UiUtility_StringScanHexToByte(
    _In_z_                              const wchar_t*  PwszSource,
    _Out_bytecap_(*PpunDestinationSize) BYTE*           PrgbDestination,
    _Inout_                             unsigned int*   PpunDestinationSize);

/**
 *  @brief      Scans a byte array into a hex string
 *  @details
 *
 *  @param      PrgbSource              Pointer to a byte array containing TPM response.
 *  @param      PunSourceSize           Size of the source byte array.
 *  @param      PwszDestination         Pointer to a string buffer for the hex representation.
 *  @param      PpunDestinationSize     In:     Capacity of the destination buffer\n
 *                                      Out:    Count of written bytes
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case the destination buffer is too small.
 */
_Check_return_
unsigned int
UiUtility_StringScanByteToHex(
    _In_bytecount_(PunSourceSize)       const BYTE*     PrgbSource,
    _In_                                unsigned int    PunSourceSize,
    _Out_z_cap_(*PpunDestinationSize)   wchar_t*        PwszDestination,
    _Inout_                             unsigned int*   PpunDestinationSize);

/**
 *  @brief      Removes white spaces, tabs, CR and LF from the string
 *  @details
 *
 *  @param      PwszBuffer              Pointer to a buffer where all white spaces and tabs should be removed.
 *  @param      PpunBufferCapacity      IN: Capacity in elements including the terminating zero
  *                                     OUT: String length without zero termination
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     ...                     Error codes from Platform_StringGetLength.
 */
_Check_return_
unsigned int
UiUtility_StringRemoveWhiteChars(
    _Inout_z_   wchar_t*        PwszBuffer,
    _In_        unsigned int*   PpunBufferCapacity);

/**
 *  @brief      Splits the given wide character Line
 *  @details    The function splits up large lines regarding the maximum line character count and
 *              adds to every line a given bunch of characters in front.
 *
 *  @param      PwszSource                  Wide character buffer holding the string to format.
 *  @param      PunSourceSize               Source wide character buffer size in elements including the zero termination.
 *  @param      PwszPreSet                  Wide character buffer which is placed in front of every line.
 *  @param      PunPreSetSize               Preset wide character buffer size in elements including the zero termination.
 *  @param      PunLineSize                 Maximum wide character count per line.
 *  @param      PwszDestination             Pointer to a wide character buffer to receive the result of the format.
 *  @param      PpunDestinationSize         In:     Capacity of the destination wide character buffer in elements including zero termination\n
 *                                          Out:    Count of written wide characters to the buffer without zero termination
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. It was either NULL or invalid.
 *  @retval     RC_E_BUFFER_TOO_SMALL       In case the destination buffer is too small.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
UiUtility_StringFormatOutput_HandleLineSplit(
    _In_z_count_(PunSourceSize)         const wchar_t*  PwszSource,
    _In_                                unsigned int    PunSourceSize,
    _In_z_count_(PunPreSetSize)         const wchar_t*  PwszPreSet,
    _In_                                unsigned int    PunPreSetSize,
    _In_                                unsigned int    PunLineSize,
    _Out_z_cap_(*PpunDestinationSize)   wchar_t*        PwszDestination,
    _Inout_                             unsigned int*   PpunDestinationSize);

/**
 *  @brief      Formats the given wide character source
 *  @details    The function splits up large lines regarding the maximum line character count and
 *              adds to every line a given bunch of characters in front. The function allocates a
 *              return wide character buffer. The caller must free this memory.
 *
 *  @param      PwszSource                  Wide character buffer holding the string to format.
 *  @param      PunSourceSize               Source wide character buffer size in elements including the zero termination.
 *  @param      PwszPreSet                  Wide character buffer which is placed in front of every line.
 *  @param      PunPreSetSize               Preset wide character buffer size in elements including the zero termination.
 *  @param      PunLineSize                 Maximum wide character count per line.
 *  @param      PpwszDestination            Pointer to a wide character buffer including the result of the format. Will be allocated by the callee, but must be freed by the caller with Platform_MemoryFree().
 *  @param      PpunDestinationLength       Count of written wide characters to the buffer without zero termination.
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. It was either NULL or invalid.
 *  @retval     RC_E_BUFFER_TOO_SMALL       In case the destination buffer is too small.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
UiUtility_StringFormatOutput(
    _In_z_count_(PunSourceSize) const wchar_t*  PwszSource,
    _In_                        unsigned int    PunSourceSize,
    _In_z_count_(PunPreSetSize) const wchar_t*  PwszPreSet,
    _In_                        unsigned int    PunPreSetSize,
    _In_                        unsigned int    PunLineSize,
    _Out_                       wchar_t**       PpwszDestination,
    _Out_                       unsigned int*   PpunDestinationLength);

/**
 *  @brief      Removes comments from a line.
 *  @details    This function takes a line from a TPM command file
 *              and removes all comments.
 *
 *  @param      PwszLineBuffer          Pointer to a buffer containing a line from command file.
 *  @param      PpfBlockComment         Flag, TRUE when a comment block was started, otherwise FALSE.
 *  @param      PpunLineBufferCapacity  IN: Capacity of the buffer in elements including zero termination
 *                                      OUT: Length of string without terminating zero
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was NULL.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
UiUtility_StringRemoveComment(
    _Inout_updates_z_(*PpunLineBufferCapacity)  wchar_t*        PwszLineBuffer,
    _Inout_                                     BOOL*           PpfBlockComment,
    _Inout_                                     unsigned int*   PpunLineBufferCapacity);

/**
 *  @brief      Gets a integer version values out of a string.
 *  @details    The output structure can contain version information or not.
 *
 *  @param      PwszVersionName         Pointer to wide character string representing the version number.
 *  @param      PunVersionNameSize      Pointer to size of the value buffer in elements including the zero termination.\n.
 *  @param      PppVersionData          Pointer to a structure of version data.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
UiUtility_StringParseIfxVersion(
    _In_z_count_(PunVersionNameSize)    wchar_t*        PwszVersionName,
    _In_                                unsigned int    PunVersionNameSize,
    _Inout_                             IfxVersion*     PppVersionData);

//----------------------------------------------------------------------------------------------
// INI File Functions
//----------------------------------------------------------------------------------------------

/**
 *  @brief      Checks if the string contains an INI section
 *  @details
 *
 *  @param      PwszBuffer              Wide character buffer to search through.
 *  @param      PunSize                 Length of the string buffer in elements (incl. zero termination).
 *  @param      PpfIsSection            Pointer to an BOOL to store if it is a section or not.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
UiUtility_IniIsSection(
    _In_z_count_(PunSize)           const wchar_t*  PwszBuffer,
    _In_                            unsigned int    PunSize,
    _Out_                           BOOL*           PpfIsSection);

/**
 *  @brief      Gets the name of an INI section
 *  @details
 *
 *  @param      PwszBuffer              Wide character buffer (line) to search through.
 *  @param      PunSize                 Length of the string buffer in elements (incl. zero termination).
 *  @param      PwszSectionName         Pointer to a wide char array to retrieve the section name.
 *                                      If the function fails and the pointer is not NULL the function will
 *                                      return an empty (zero terminated) buffer.
 *  @param      PpunSectionNameSize     In:     Size of the wide character buffer in elements including the zero termination\n
 *                                      Out:    The length of the section name without zero termination.
 *                                              If the function fails the length will be set to zero.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case the out buffer is too small.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
UiUtility_IniGetSectionName(
    _In_z_count_(PunSize)               const wchar_t*  PwszBuffer,
    _In_                                unsigned int    PunSize,
    _Out_z_cap_(*PpunSectionNameSize)   wchar_t*        PwszSectionName,
    _Inout_                             unsigned int*   PpunSectionNameSize);

/**
 *  @brief      Checks if the string contains an INI key/value pair
 *  @details
 *
 *  @param      PwszBuffer              Wide character buffer to search through.
 *  @param      PunSize                 Length of the string buffer in elements.
 *  @param      PpfIsKeyValue           Pointer to an BOOL to store if it is a key/value pair or not.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
UiUtility_IniIsKeyValue(
    _In_z_count_(PunSize)           const wchar_t*  PwszBuffer,
    _In_                            unsigned int    PunSize,
    _Out_                           BOOL*           PpfIsKeyValue);

/**
 *  @brief      Checks if the string contains an INI key/value pair
 *  @details
 *
 *  @param      PwszBuffer              Wide character buffer to search through.
 *  @param      PunSize                 Length of the string buffer in elements.
 *  @param      PwszKey                 Pointer to a wide character buffer to fill in the key.
 *  @param      PpunKeySize             In:     Size of the key buffer in elements including zero termination.\n
 *                                      Out:    Length of the filled in key in elements without zero termination.
 *  @param      PwszValue               Pointer to a wide character buffer to fill in the value.
 *  @param      PpunValueSize           In:     Size of the value buffer in elements including the zero termination.\n
 *                                      Out:    Length of the filled in value in elements without zero termination.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case of any output buffer size is too small.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
UiUtility_IniGetKeyValue(
    _In_z_count_(PunSize)       const wchar_t*  PwszBuffer,
    _In_                        unsigned int    PunSize,
    _Out_z_cap_(*PpunKeySize)   wchar_t*        PwszKey,
    _Inout_                     unsigned int*   PpunKeySize,
    _Out_z_cap_(*PpunValueSize) wchar_t*        PwszValue,
    _Inout_                     unsigned int*   PpunValueSize);

//----------------------------------------------------------------------------------------------
// File Functions
//----------------------------------------------------------------------------------------------

/**
 *  @brief      Check if the configured log file path is accessible and writable
 *  @details
 *
 *  @param      PfRemoveFileIfNotExistsBefore   TRUE: Remove file after check if it does not exist before
 *                                              FALSE: Do not remove the file anyway
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     RC_E_FILE_NOT_FOUND     In case of the log file cannot be created.
 *  @retval     RC_E_ACCESS_DENIED      In case of the file is not writable.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
UiUtility_CheckIfLogPathWritable(
    BOOL PfRemoveFileIfNotExistsBefore);

#ifdef __cplusplus
}
#endif
