﻿/**
 *  @brief      Implements utility methods
 *  @details    This module provides helper functions for common and platform independent use
 *  @file       UiUtility.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "UiUtility.h"
#include "Utility.h"

//----------------------------------------------------------------------------------------------
// String Functions
//----------------------------------------------------------------------------------------------
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
    _Out_                           unsigned int*   PpunLineSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check out parameters
        if (NULL == PpunLineSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One of the out parameters is NULL.");
            break;
        }
        // Initialize out parameter
        *PpunLineSize = 0;

        // Check in parameters
        if (NULL == PwszBuffer ||
                NULL == PpwszLine ||
                NULL != *PpwszLine ||
                NULL == PpunIndex ||
                0 == PunSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One of the input parameters is NULL or empty.");
            break;
        }

        // Check if end of string has been reached
        if (*PpunIndex >= PunSize)
        {
            unReturnValue = RC_E_END_OF_STRING;
            break;
        }

        // Get the line size
        {
            // Find index of line break or string end
            unsigned int unIndex = *PpunIndex;
            for (; unIndex < PunSize; unIndex++)
            {
                // Check if end of string or \n reached
                if (PwszBuffer[unIndex] == L'\0' ||
                        PwszBuffer[unIndex] == L'\n')
                    break;

                // Check if \r \n is reached
                if (unIndex + 1 < PunSize &&
                        PwszBuffer[unIndex] == L'\r' &&
                        PwszBuffer[unIndex + 1] == L'\n')
                {
                    unIndex++;
                    break;
                }

                // Increment output size
                (*PpunLineSize)++;
            }

            // Add terminating zero
            (*PpunLineSize)++;

            // Allocate memory for a line buffer with that size
            *PpwszLine = (wchar_t*)Platform_MemoryAllocateZero((*PpunLineSize) * sizeof(wchar_t));
            if (NULL == *PpwszLine)
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE(unReturnValue, L"Unexpected error occurred during memory allocation.");
                break;
            }
            // Copy line to buffer
            {
                unsigned int unByteCapacity = (*PpunLineSize) * sizeof((*PpwszLine)[0]);
                unsigned int unByteCount = ((*PpunLineSize) - 1) * sizeof((*PpwszLine)[0]);
                if (0 < unByteCount)
                {
                    unReturnValue = Platform_MemoryCopy((void*)*PpwszLine, unByteCapacity, (void*)&PwszBuffer[*PpunIndex], unByteCount);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"Unexpected error occurred during memory copy.");
                        break;
                    }
                }
                else
                    unReturnValue = RC_SUCCESS;
            }

            // Set index
            *PpunIndex = unIndex + 1;
        }
    }
    WHILE_FALSE_END;

    // Cleanup in case of error
    if (RC_SUCCESS != unReturnValue)
    {
        Platform_MemoryFree((void**)PpwszLine);

        // Reset out parameters
        if (NULL != PpunLineSize)
            *PpunLineSize = 0;
    }

    return unReturnValue;
}

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
    _Out_z_cap_(PunBufferSize)  wchar_t*            PwszBuffer,
    _In_                        unsigned int        PunBufferSize,
    _In_                        const unsigned int* PpunValue,
    ...)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check output buffer
        if (NULL == PwszBuffer || NULL == PpunValue)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"PwszBuffer or PpunValue parameter is NULL.");
            break;
        }

        {
            va_list argptr;
            unsigned int unOffset = 0;
            unsigned int unReturnValueInnerLoop = RC_SUCCESS;
            va_start(argptr, PpunValue);
            do
            {
                // Check output buffer length
                if (PunBufferSize < (unOffset + sizeof(*PpunValue) + 1))
                {
                    if (0 < PunBufferSize)
                        PwszBuffer[0] = L'\0';
                    unReturnValueInnerLoop = RC_E_BUFFER_TOO_SMALL;
                    ERROR_STORE(unReturnValueInnerLoop, L"Output buffer is too small.");
                    break;
                }

                // Convert value
                PwszBuffer[unOffset + 0] = ((*PpunValue & 0xFF000000) >> 24);
                PwszBuffer[unOffset + 1] = ((*PpunValue & 0x00FF0000) >> 16);
                PwszBuffer[unOffset + 2] = ((*PpunValue & 0x0000FF00) >> 8);
                PwszBuffer[unOffset + 3] = (*PpunValue & 0x000000FF);
                PwszBuffer[unOffset + 4] = L'\0';

                // Get next token
                unOffset += sizeof(*PpunValue);
                PpunValue = va_arg(argptr, unsigned int*);
            }
            while(PpunValue != NULL);
            va_end(argptr);

            // On error exit the WHILE_FALSE loop
            if (unReturnValueInnerLoop != RC_SUCCESS)
            {
                unReturnValue = unReturnValueInnerLoop;
                break;
            }
        }
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

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
    _Inout_                             unsigned int*   PpunDestinationSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        unsigned int unLength = 0;
        unsigned int unIndex;
        const BYTE rgbMatrix[128] =
        {
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
        };

        // Check parameters
        if (NULL == PwszSource || NULL == PrgbDestination || NULL == PpunDestinationSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One of the input parameters is invalid.");
            break;
        }

        // Check capacity: Maximum is the size of output buffer * 2 + 1
        // (every 2 wchars are one byte + 1 (Terminating zero which is not required here because of conversion to a byte))
        unReturnValue = Platform_StringGetLength(PwszSource, (*PpunDestinationSize * 2) + 1, &unLength);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_StringGetLength returned an unexpected value.");
            break;
        }

        // Parse bytes
        for (unIndex = 0; unIndex < unLength; unIndex += 2)
        {
            BYTE bHigh = 0;
            BYTE bLow = 0;

            // Check if the input contains not two but only one more character
            if (unIndex + 1 >= unLength)
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                ERROR_STORE(unReturnValue, L"Source string is not given in hex pairs.");
                break;
            }

            // Check if both characters are inside of the range of the matrix
            if (((unsigned short)PwszSource[unIndex]) >= sizeof(rgbMatrix) ||
                    ((unsigned short)PwszSource[unIndex + 1]) >= sizeof(rgbMatrix))
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                ERROR_STORE(unReturnValue, L"Source contains unexpected values out of the hex parsable range.");
                break;
            }

            // Get the two Hex characters and convert them to an integer
            bHigh = rgbMatrix[PwszSource[unIndex]];
            bLow = rgbMatrix[PwszSource[unIndex + 1]];

            // Check if one of the values is out of HEX character range
            if (0xFF == bHigh || 0xFF == bLow)
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                ERROR_STORE(unReturnValue, L"Source contains unexpected values out of the hex parsable range.");
                break;
            }

            // Check if there is enough space in the destination buffer to store one more byte
            // Check is present to ensure that CA does not recognize here a problem but cant reached normally
            if (*PpunDestinationSize <= unIndex / 2)
                break;

            // Shift the high part to the left by 4, add the low part to the right and store result in the destination buffer
            PrgbDestination[unIndex / 2] = (bHigh << 4) | bLow;
        }

        if (RC_SUCCESS != unReturnValue)
            break;

        // Update the destination size to the written size
        *PpunDestinationSize = unIndex / 2;

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        // Reset out parameters
        if (NULL != PpunDestinationSize)
            *PpunDestinationSize = 0;
    }

    return unReturnValue;
}

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
    _Inout_                             unsigned int*   PpunDestinationSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        unsigned int unIndex = 0;
        wchar_t wszHelper[HEX_STRING_OUT_LENGTH];
        unsigned int unHelperSize = RG_LEN(wszHelper);
        unsigned int unDestinationSize = 0;
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszHelper, RG_LEN(wszHelper)));

        // Check parameters
        if (NULL == PrgbSource || NULL == PwszDestination || NULL == PpunDestinationSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One of the input parameter is invalid.");
            break;
        }

        // Check destination buffer capacity; we need at least a size of 1 for null-termination
        if (0 == *PpunDestinationSize)
        {
            unReturnValue = RC_E_BUFFER_TOO_SMALL;
            ERROR_STORE(unReturnValue, L"Output buffer capacity is zero.");
            break;
        }

        // Initialize string for loop start
        PwszDestination[0] = L'\0';

        // Process input buffer
        for (unIndex = 0; unIndex < PunSourceSize; unIndex++)
        {
            // Convert current byte to string
            unHelperSize = HEX_STRING_OUT_LENGTH;
            unReturnValue = Platform_StringFormat(wszHelper, &unHelperSize, L"%.2X ", PrgbSource[unIndex]);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
                break;
            }
            // Concatenate string for current byte with string for previous bytes
            unDestinationSize = *PpunDestinationSize;
            unReturnValue = Platform_StringConcatenate(PwszDestination, &unDestinationSize, wszHelper);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringConcatenate returned an unexpected value.");
                break;
            }
        }

        // Update destination size
        *PpunDestinationSize = unDestinationSize;
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        // Reset out parameters
        if (NULL != PwszDestination)
            PwszDestination[0] = L'\0';
        if (NULL != PpunDestinationSize)
            *PpunDestinationSize = 0;
    }

    return unReturnValue;
}

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
    _In_        unsigned int*   PpunBufferCapacity)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        unsigned int unIndex, unLength = 0, unWriteIndex = 0;
        // Parameter check
        if (NULL == PwszBuffer ||
                NULL == PpunBufferCapacity ||
                0 == *PpunBufferCapacity)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected.");
            break;
        }

        unReturnValue = Platform_StringGetLength(PwszBuffer, *PpunBufferCapacity, &unLength);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_StringGetLength returned an unexpected value");
            break;
        }

        for (unIndex = 0; unIndex < unLength; unIndex++)
        {
            // Check current character for tab or whitespace or \r \n
            if (PwszBuffer[unIndex] != L' ' &&  PwszBuffer[unIndex] != L'\t' &&
                    PwszBuffer[unIndex] != L'\r' && PwszBuffer[unIndex] != L'\n')
            {
                PwszBuffer[unWriteIndex++] = PwszBuffer[unIndex];
            }
        }

        PwszBuffer[unWriteIndex] = L'\0';
        *PpunBufferCapacity = unWriteIndex;

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        // Reset out parameters
        if (NULL != PwszBuffer)
            PwszBuffer[0] = L'\0';
    }

    return unReturnValue;
}

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
    _Inout_                             unsigned int*   PpunDestinationSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Line does not fit into the screen output width so the it must be split
        unsigned int unSourceIndex = 0, unLastSpace = 0, unLastLine = 0;
        unsigned int unDestinationSize = 0, unDestinationCapacity = 0, unDestinationIndex = 0;

        // Check parameters
        if (NULL == PwszSource || NULL == PwszPreSet || NULL == PwszDestination ||
                0 == PunLineSize || NULL == PpunDestinationSize || 0 == *PpunDestinationSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        PwszDestination[0] = L'\0';

        unDestinationCapacity = *PpunDestinationSize;

        // Walk over the Line and search for spaces
        for (unSourceIndex = 0; unSourceIndex < PunSourceSize; unSourceIndex++)
        {
            if (PwszSource[unSourceIndex] == L' ' || PwszSource[unSourceIndex] == L'\0')
            {
                // If a space is reached check if the string so far matches into the line
                if (PunLineSize < (unSourceIndex - unLastLine + (PunPreSetSize - 1)))
                {
                    // If not add a new line and preset to the destination
                    unDestinationSize = unDestinationCapacity;
                    unReturnValue = Platform_StringCopy(&PwszDestination[unDestinationIndex], &unDestinationSize, L"\n");
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                        break;
                    }

                    unDestinationCapacity -= unDestinationSize;
                    unDestinationIndex += unDestinationSize;

                    // Add preset
                    unDestinationSize = unDestinationCapacity;
                    unReturnValue = Platform_StringCopy(&PwszDestination[unDestinationIndex], &unDestinationSize, PwszPreSet);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                        break;
                    }
                    unDestinationCapacity -= unDestinationSize;
                    unDestinationIndex += unDestinationSize;
                    unLastLine = ++unLastSpace;
                }

                // Copy Word
                unDestinationSize = unDestinationCapacity * sizeof(PwszDestination[0]);
                unReturnValue = Platform_MemoryCopy((void*)&PwszDestination[unDestinationIndex], unDestinationSize,
                                                    (void*) &PwszSource[unLastSpace], (unSourceIndex - unLastSpace) * sizeof(PwszSource[0]));

                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                    break;
                }

                unDestinationCapacity -= unSourceIndex - unLastSpace;
                unDestinationIndex += unSourceIndex - unLastSpace;
                if (unDestinationIndex < *PpunDestinationSize)
                    PwszDestination[unDestinationIndex] = L'\0';
                else
                {
                    unReturnValue = RC_E_BUFFER_TOO_SMALL;
                    ERROR_STORE(unReturnValue, L"Destination buffer is too small.");
                    break;
                }
                unLastSpace = unSourceIndex;
                continue;
            }

            // Check if it is one block
            if (unLastSpace == unLastLine && PunLineSize <= (unSourceIndex - unLastLine + (PunPreSetSize - 1)))
            {
                // Copy Block
                unDestinationSize = unDestinationCapacity * sizeof(PwszDestination[0]);
                unReturnValue = Platform_MemoryCopy((void*)&PwszDestination[unDestinationIndex], unDestinationSize,
                                                    (void*) &PwszSource[unLastSpace], (unSourceIndex - unLastSpace) * sizeof(PwszSource[0]));
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                    break;
                }

                unDestinationCapacity -= unSourceIndex - unLastSpace;
                unDestinationIndex += unSourceIndex - unLastSpace;

                // Add new line and preset
                unDestinationSize = unDestinationCapacity;
                unReturnValue = Platform_StringCopy(&PwszDestination[unDestinationIndex], &unDestinationSize, L"\n");
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                    break;
                }

                unDestinationCapacity -= unDestinationSize;
                unDestinationIndex += unDestinationSize;
                // Add preset
                unDestinationSize = unDestinationCapacity;
                unReturnValue = Platform_StringCopy(&PwszDestination[unDestinationIndex], &unDestinationSize, PwszPreSet);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                    break;
                }
                unDestinationCapacity -= unDestinationSize;
                unDestinationIndex += unDestinationSize;

                unLastLine = unSourceIndex;
                unLastSpace = unSourceIndex;
                continue;
            }
        }
        *PpunDestinationSize = *PpunDestinationSize - unDestinationCapacity;
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        if (NULL != PwszDestination)
            PwszDestination[0] = L'\n';
        if (NULL != PpunDestinationSize)
            *PpunDestinationSize = 0;
    }

    return unReturnValue;
}

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
    _Out_                       unsigned int*   PpunDestinationLength)
{
    unsigned int unReturnValue = RC_E_FAIL;
    wchar_t* wszDestination = NULL;

    do
    {
        unsigned int unSourceIndex = 0, unDestinationIndex = 0;
        unsigned int unLineSize = 0;
        unsigned int unDestinationCapacity;
        wchar_t* wszLine = NULL;

        // Check parameters
        if (NULL == PwszSource || NULL == PwszPreSet || NULL == PpwszDestination ||
                0 == PunLineSize || NULL == PpunDestinationLength)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more parameters are invalid");
            break;
        }

        wszDestination = (wchar_t*)Platform_MemoryAllocateZero(MAX_MESSAGE_SIZE * sizeof(wchar_t));
        if (NULL == wszDestination)
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"Memory allocation failed");
            break;
        }

        unDestinationCapacity = MAX_MESSAGE_SIZE;

        do
        {
            unsigned int unDestinationSize;

            // Clear allocated line if there is one
            Platform_MemoryFree((void**)&wszLine);

            // Loop over Lines because one line should be handled as one Block
            unReturnValue = UiUtility_StringGetLine(PwszSource, PunSourceSize, &unSourceIndex, &wszLine, &unLineSize);
            if (RC_SUCCESS != unReturnValue)
            {
                if (RC_E_END_OF_STRING == unReturnValue)
                {
                    *PpunDestinationLength = MAX_MESSAGE_SIZE - unDestinationCapacity;
                    *PpwszDestination = wszDestination;
                    unReturnValue = RC_SUCCESS;
                }
                else
                {
                    ERROR_STORE(unReturnValue, L"Utility_StringGetLine returned an unexpected value.");
                }
                break;
            }
            // Check if the pointer to the line is valid
            if (NULL == wszLine)
            {
                unReturnValue = RC_E_FAIL;
                ERROR_STORE(unReturnValue, L"Utility_StringGetLine wszLine parameter is returned with NULL.");
                break;
            }

            // Check if a whole line was written before.
            // If that is the case and another line starts now
            if (unDestinationCapacity < MAX_MESSAGE_SIZE)
            {
                unDestinationSize = unDestinationCapacity;
                unReturnValue = Platform_StringCopy(&wszDestination[unDestinationIndex], &unDestinationSize, L"\n");
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                    break;
                }
                unDestinationCapacity -= unDestinationSize;
                unDestinationIndex += unDestinationSize;
            }

            // Add preset to every new line
            unDestinationSize = unDestinationCapacity;
            unReturnValue = Platform_StringCopy(&wszDestination[unDestinationIndex], &unDestinationSize, PwszPreSet);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                break;
            }
            unDestinationCapacity -= unDestinationSize;
            unDestinationIndex += unDestinationSize;

            // Check if current line must be split or not
            // Both sizes must be calculated without zero termination
            if (PunLineSize >= (unLineSize - 1) + (PunPreSetSize - 1))
            {
                // No split necessary
                // Copy line to the destination and go on with the next one
                unDestinationSize = unDestinationCapacity;
                unReturnValue = Platform_StringCopy(&wszDestination[unDestinationIndex], &unDestinationSize, wszLine);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Platform_StringCopy returned an unexpected value.");
                    break;
                }
                unDestinationCapacity -= unDestinationSize;
                unDestinationIndex += unDestinationSize;
            }
            else
            {
                // Split necessary
                unDestinationSize = unDestinationCapacity;
                unReturnValue = UiUtility_StringFormatOutput_HandleLineSplit(
                                    wszLine, unLineSize, PwszPreSet, PunPreSetSize, PunLineSize,
                                    &wszDestination[unDestinationIndex], &unDestinationSize);
                if (RC_SUCCESS != unReturnValue)
                    break;
                unDestinationCapacity -= unDestinationSize;
                unDestinationIndex += unDestinationSize;
            }
        }
        WHILE_TRUE_END;

        // Clear allocated line if there is one
        Platform_MemoryFree((void**)&wszLine);
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        Platform_MemoryFree((void**)&wszDestination);

        if (NULL != PpwszDestination)
            *PpwszDestination = NULL;
        if (NULL != PpunDestinationLength)
            *PpunDestinationLength = 0;
    }

    return unReturnValue;
}

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
    _Inout_                                     unsigned int*   PpunLineBufferCapacity)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        unsigned int unIndex, unLength = 0, unWriteIndex = 0;
        // Parameter check
        if ((NULL == PwszLineBuffer) || (NULL == PpfBlockComment) ||
                (NULL == PpunLineBufferCapacity) || (0 == *PpunLineBufferCapacity))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected.");
            break;
        }

        unReturnValue = Platform_StringGetLength(PwszLineBuffer, *PpunLineBufferCapacity, &unLength);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_StringGetLength returned an unexpected value");
            break;
        }

        for (unIndex = 0; unIndex < unLength; unIndex++)
        {
            if ((unIndex + 1) >= *PpunLineBufferCapacity)
                break;

            // If Block comment is active
            if (TRUE == *PpfBlockComment)
            {
                // Try to find a closing block comment and nothing else
                if ((L'*' == PwszLineBuffer[unIndex]) &&
                        (L'/' == PwszLineBuffer[unIndex + 1]))
                {
                    unIndex++;
                    *PpfBlockComment = FALSE;
                    continue;
                }
            }
            else if ((L'/' == PwszLineBuffer[unIndex]) &&
                     (L'/' == PwszLineBuffer[unIndex + 1]))
            {
                unIndex++;
                break;
            }
            else if ((L'/' == PwszLineBuffer[unIndex]) &&
                     (L'*' == PwszLineBuffer[unIndex + 1]))
            {
                unIndex++;
                *PpfBlockComment = TRUE;
                continue;
            }
            else if (L';' == PwszLineBuffer[unIndex])
            {
                unIndex++;
                break;
            }
            else
                PwszLineBuffer[unWriteIndex++] = PwszLineBuffer[unIndex];
        }

        if (*PpunLineBufferCapacity <= unWriteIndex)
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"The line buffer is too small.");
            break;
        }

        PwszLineBuffer[unWriteIndex] = L'\0';
        *PpunLineBufferCapacity = unWriteIndex;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

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
    _Inout_                             IfxVersion*     PppVersionData)
{
    unsigned int unReturnValue = RC_E_FAIL;
    unsigned int unTemp = 0;

    wchar_t wszVersionNamePart[256];            // Temp buffer of version parts like major, minor etc.

    unsigned int unIndexVersionName = 0;        // Index to count characters of VersionName
    unsigned int unDigitIndex = 0;              // Index of digit of each version part
    unsigned int unIndexVersionPart = 0;        // Index of the part of the version e.g. major, minor etc.
    IGNORE_RETURN_VALUE(Platform_StringSetZero(wszVersionNamePart, RG_LEN(wszVersionNamePart)));

    // Loop through VersionName
    do
    {
        // Parameter check
        if ((NULL == PwszVersionName) ||
                (NULL == PppVersionData) ||
                (0 == PunVersionNameSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more parameters are invalid");
            break;
        }

        for (unIndexVersionName = 0; unIndexVersionName <= PunVersionNameSize; unIndexVersionName++)    // Increment index for version name
        {
            if ((L'.' == PwszVersionName[unIndexVersionName]) || (L'\0' == PwszVersionName[unIndexVersionName]))
            {
                // Delimiter in string detected
                wszVersionNamePart[unDigitIndex] = L'\0';
                if (RC_SUCCESS != Utility_StringParseUInteger(wszVersionNamePart, unDigitIndex + 1, &unTemp))
                {
                    unReturnValue = RC_E_BAD_PARAMETER;
                    ERROR_STORE(unReturnValue, L"VersionName has not the correct format.");
                    break;
                }
                switch (unIndexVersionPart)
                {
                    case 0:     // At first dot in version string
                        PppVersionData->unMajor = unTemp;
                        break;
                    case 1:     // At second dot in version string
                        PppVersionData->unMinor = unTemp;
                        break;
                    case 2:     // At third dot in version string
                        PppVersionData->unBuild = unTemp;
                        break;
                    case 3:     // At EOL in version string
                        PppVersionData->unBuild = unTemp;
                        unReturnValue = RC_SUCCESS;
                        break;
                    default:    // Should never go here
                        break;
                }
                unDigitIndex = 0;       // Reset index for next part of version
                unIndexVersionPart++;   // Increment counter for version parts (e.g. major minor etc.)
            }
            else
            {   // Go on and collect digits of version part
                wszVersionNamePart[unDigitIndex++] = PwszVersionName[unIndexVersionName];
            }
            if (L'\0' == PwszVersionName[unIndexVersionName])   // Reached EOL
            {
                if (4 != unIndexVersionPart)    // Check if string ends after third dot
                {
                    unReturnValue = RC_E_BAD_PARAMETER;
                    ERROR_STORE(unReturnValue, L"VersionName has not the correct format.");
                }
                break;
            }
            if (4 <= unIndexVersionPart)    // Version string contains more dots as expected
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                ERROR_STORE(unReturnValue, L"VersionName has not the correct format.");
                break;
            }
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

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
    _Out_                           BOOL*           PpfIsSection)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check out parameter
        if (NULL == PpfIsSection)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"PpfIsSection parameter is NULL.");
            break;
        }

        // Initialize out parameter
        *PpfIsSection = FALSE;

        // Check in parameter
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszBuffer))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"PwszBuffer parameter is NULL or empty.");
            break;
        }

        // Get and check for section brackets
        {
            int nIndexOpenSquareBracket = -1, nIndexCloseSquareBracker = -1;
            int nIndexSemiColon = -1;

            // Get index for '['
            unReturnValue = Utility_StringContainsWChar(PwszBuffer, PunSize, L'[', &nIndexOpenSquareBracket);
            if (RC_SUCCESS != unReturnValue)
                break;
            // Check if '[' index is == -1
            if (-1 == nIndexOpenSquareBracket)
                break;

            // Get index for ']'
            unReturnValue = Utility_StringContainsWChar(PwszBuffer, PunSize, L']', &nIndexCloseSquareBracker);
            if (RC_SUCCESS != unReturnValue)
                break;
            // Check if '[' index is == -1
            if (-1 == nIndexCloseSquareBracker)
                break;

            // Check order
            if (nIndexCloseSquareBracker < nIndexOpenSquareBracket)
                break;

            // Get index for ';'
            unReturnValue = Utility_StringContainsWChar(PwszBuffer, PunSize, L';', &nIndexSemiColon);
            if (RC_SUCCESS != unReturnValue)
                break;
            // Check if ';' index is != -1
            if (-1 != nIndexSemiColon)
            {
                // Check if both brackets are before the semicolon (otherwise it's a comment)
                if (nIndexOpenSquareBracket < nIndexSemiColon &&
                        nIndexCloseSquareBracker < nIndexSemiColon)
                    *PpfIsSection = TRUE;
            }
            else
                *PpfIsSection = TRUE;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        // Reset out parameters
        if (NULL != PpfIsSection)
            *PpfIsSection = FALSE;
    }

    return unReturnValue;
}

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
    _Inout_                             unsigned int*   PpunSectionNameSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        BOOL fIsSection = FALSE;

        // Check parameters
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszBuffer) ||
                NULL == PwszSectionName ||
                NULL == PpunSectionNameSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more parameters are NULL or empty.");
            break;
        }
        PwszSectionName[0] = L'\0';
        if (0 == *PpunSectionNameSize)
        {
            unReturnValue = RC_E_BUFFER_TOO_SMALL;
            ERROR_STORE(unReturnValue, L"One or more buffers are too small.");
            break;
        }

        // Check if there is a section
        unReturnValue = UiUtility_IniIsSection(PwszBuffer, PunSize, &fIsSection);
        if (RC_SUCCESS != unReturnValue)
            break;
        if (FALSE == fIsSection)
        {
            PwszSectionName[0] = L'\0';
            *PpunSectionNameSize = 0;
            unReturnValue = RC_SUCCESS;
            break;
        }

        // Parse section name now
        {
            int nIndexOpenSquareBracket = -1, nIndexCloseSquareBracker = -1;
            unsigned int unCount = 0;
            // Get index for '['
            unReturnValue = Utility_StringContainsWChar(PwszBuffer, PunSize, L'[', &nIndexOpenSquareBracket);
            if (RC_SUCCESS != unReturnValue)
                break;
            // Get index for ']'
            unReturnValue = Utility_StringContainsWChar(PwszBuffer, PunSize, L']', &nIndexCloseSquareBracker);
            if (RC_SUCCESS != unReturnValue)
                break;
            unCount = (unsigned int)(nIndexCloseSquareBracker - nIndexOpenSquareBracket - 1);

            // Check size of the out buffer (smaller or equal because of zero termination)
            if (*PpunSectionNameSize <= unCount)
            {
                unReturnValue = RC_E_BUFFER_TOO_SMALL;
                ERROR_STORE(unReturnValue, L"Output buffer is too small.");
                break;
            }

            // Copy the name (use memcpy because string copy method has no parameter for element count)
            unReturnValue = Platform_MemoryCopy((void*)PwszSectionName, *PpunSectionNameSize * sizeof(PwszSectionName[0]), (void*)&PwszBuffer[nIndexOpenSquareBracket + 1], unCount * sizeof(PwszBuffer[0]));
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected error during Platform_MemoryCopy.");
                break;
            }

            PwszSectionName[unCount] = L'\0';
            *PpunSectionNameSize = unCount;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        // Reset out parameters
        if (NULL != PpunSectionNameSize)
            *PpunSectionNameSize = 0;
        if (NULL != PwszSectionName)
            PwszSectionName[0] = L'\0';
    }

    return unReturnValue;
}

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
    _Out_                           BOOL*           PpfIsKeyValue)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check out parameter
        if (NULL == PpfIsKeyValue)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"PpfIsKeyValue parameter is NULL.");
            break;
        }

        // Initialize out parameter
        *PpfIsKeyValue = FALSE;

        // Check in parameter
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszBuffer))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"PwszBuffer parameter is NULL or empty.");
            break;
        }

        // Get and check for a key value pair
        {
            int nIndexKeyValueSeperator = -1;
            int nIndexSemiColon = -1;

            // Get index for '['
            unReturnValue = Utility_StringContainsWChar(PwszBuffer, PunSize, L'=', &nIndexKeyValueSeperator);
            if (RC_SUCCESS != unReturnValue)
                break;
            // Check if '=' index is == -1
            if (-1 == nIndexKeyValueSeperator)
                break;

            // Get index for ';'
            unReturnValue = Utility_StringContainsWChar(PwszBuffer, PunSize, L';', &nIndexSemiColon);
            if (RC_SUCCESS != unReturnValue)
                break;
            // Check if ';' index is != -1
            if (-1 != nIndexSemiColon)
            {
                // Check if both brackets are before the semicolon (otherwise it's a comment)
                if (nIndexKeyValueSeperator < nIndexSemiColon)
                    *PpfIsKeyValue = TRUE;
            }
            else
                *PpfIsKeyValue = TRUE;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        // Reset out parameters
        if (NULL != PpfIsKeyValue)
            *PpfIsKeyValue = FALSE;
    }

    return unReturnValue;
}

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
    _Inout_                     unsigned int*   PpunValueSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        BOOL fIsKeyValue = FALSE;

        // Check out parameter
        if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszBuffer) ||
                NULL == PwszKey ||
                NULL == PpunKeySize ||
                NULL == PwszValue ||
                NULL == PpunValueSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"One or more parameters are NULL or empty.");
            break;
        }
        if (0 == *PpunKeySize ||
                0 == *PpunValueSize)
        {
            unReturnValue = RC_E_BUFFER_TOO_SMALL;
            ERROR_STORE(unReturnValue, L"One or more buffers are too small.");
            break;
        }

        // Initialize out parameter
        PwszKey[0] = L'\0';
        PwszValue[0] = L'\0';

        // Check if there is a key/value pair
        unReturnValue = UiUtility_IniIsKeyValue(PwszBuffer, PunSize, &fIsKeyValue);
        if (RC_SUCCESS != unReturnValue)
            break;
        // If not, set all to NULL or empty and return
        if (FALSE == fIsKeyValue)
        {
            PwszKey[0] = L'\0';
            *PpunKeySize = 0;
            PwszValue[0] = L'\0';
            *PpunValueSize = 0;
            unReturnValue = RC_SUCCESS;
            break;
        }

        // Parse key value pair now
        {
            int nIndexKeyValue = -1;
            unsigned int unCountKey = 0;
            unsigned int unCountValue = 0;

            // Get index for '='
            unReturnValue = Utility_StringContainsWChar(PwszBuffer, PunSize, L'=', &nIndexKeyValue);
            if (RC_SUCCESS != unReturnValue)
                break;
            // Calculate the size for the key and value including white characters
            unCountKey = (unsigned int)nIndexKeyValue;
            // -2 because of "=" and zero termination
            unCountValue = PunSize - unCountKey - 2;

            // Check if the buffers are big enough
            if (*PpunKeySize <= unCountKey ||
                    *PpunValueSize <= unCountValue)
            {
                unReturnValue = RC_E_BUFFER_TOO_SMALL;
                ERROR_STORE(unReturnValue, L"Output buffer is too small.");
                break;
            }

            // Copy the name (use memcpy because string copy method has no parameter for element count)
            unReturnValue = Platform_MemoryCopy((void*)PwszKey, *PpunKeySize * sizeof(PwszKey[0]), (void*)PwszBuffer, unCountKey * sizeof(PwszBuffer[0]));
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected error during Platform_MemoryCopy.");
                break;
            }
            // Update zero termination
            PwszKey[unCountKey] = L'\0';

            // Eliminate white characters
            unReturnValue = UiUtility_StringRemoveWhiteChars(PwszKey, PpunKeySize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected error during Utility_StringRemoveWhiteChars.");
                break;
            }
            // Copy the name (use memcpy because string copy method has no parameter for element count)
            unReturnValue = Platform_MemoryCopy((void*)PwszValue, *PpunValueSize * sizeof(PwszValue[0]), (void*)&PwszBuffer[nIndexKeyValue + 1], unCountValue * sizeof(PwszBuffer[0]));
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected error during Platform_MemoryCopy.");
                break;
            }
            // Update zero termination
            PwszValue[unCountValue] = L'\0';

            // Eliminate white characters
            unReturnValue = UiUtility_StringRemoveWhiteChars(PwszValue, PpunValueSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected error during Utility_StringRemoveWhiteChars.");
                break;
            }
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    if (RC_SUCCESS != unReturnValue)
    {
        // Reset out parameters
        if (NULL != PpunKeySize)
            *PpunKeySize = 0;
        if (NULL != PwszKey)
            PwszKey[0] = L'\0';
        if (NULL != PpunValueSize)
            *PpunValueSize = 0;
        if (NULL != PwszValue)
            PwszValue[0] = L'\0';
    }

    return unReturnValue;
}

//----------------------------------------------------------------------------------------------
// File Functions
//----------------------------------------------------------------------------------------------

#include "FileIO.h"

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
    BOOL PfRemoveFileIfNotExistsBefore)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check if we can open the log file for writing...
        wchar_t wszLogFilePath[MAX_STRING_1024];
        unsigned int unLogFilePathSize = RG_LEN(wszLogFilePath);
        unsigned int unFileAccessMode = 0;
        void* pvFileHandle = NULL;
        IGNORE_RETURN_VALUE(Platform_StringSetZero(wszLogFilePath, RG_LEN(wszLogFilePath)));

        // If the log file is already open, then we do not need to run checks anymore.
        if (Logging_IsFileOpen())
        {
            unReturnValue = RC_SUCCESS;
            break;
        }

        if (!PropertyStorage_GetValueByKey(PROPERTY_LOGGING_PATH, wszLogFilePath, &unLogFilePathSize))
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_LOGGING_PATH);
            break;
        }

        unFileAccessMode = FileIO_Exists(wszLogFilePath) ? FILE_APPEND : FILE_WRITE;
        // Verify that logging to given path is possible
        unReturnValue = FileIO_Open(wszLogFilePath, &pvFileHandle, unFileAccessMode);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"FileIO_Open returned an unexpected value.");
            break;
        }

        // Close file handle
        if (NULL != pvFileHandle)
        {
            unReturnValue = FileIO_Close(&pvFileHandle);
            if (RC_SUCCESS != unReturnValue)
                ERROR_STORE(unReturnValue, L"Closing log file failed.");

            // No break because removing of file should be tried
        }

        if (TRUE == PfRemoveFileIfNotExistsBefore &&
                FILE_WRITE == unFileAccessMode)
        {
            unsigned int unRemoveReturnValue = RC_E_FAIL;
            unRemoveReturnValue = FileIO_Remove(wszLogFilePath);
            if (RC_SUCCESS != unRemoveReturnValue &&
                    RC_SUCCESS == unReturnValue)
            {
                unReturnValue = unRemoveReturnValue;
                ERROR_STORE(unReturnValue, L"Removing created log file while path testing failed.");
            }
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}
