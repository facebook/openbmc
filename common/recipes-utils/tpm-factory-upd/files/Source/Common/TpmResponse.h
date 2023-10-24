/**
 *  @brief      Declares the TPM response message parser
 *  @details    This module provides function and constant declarations for parsing TPM1.2
 *              and TPM2.0 response codes and obtaining a corresponding error message.
 *  @file       TpmResponse.h
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
#include "TpmResponseMessages.h"

// --------------------------------- Bit masks ---------------------------------
/// Mask for the upper 20 bits of an unsigned integer
#define UINT32_RANGE_UPPER_20_BITS 0xFFFFF000 // Value equals bits 12 to 31 are set

/// Mask for the TPM2.0 response code F-Flag (format selector)
#define TPM_RC_FLAG_BIT_7_F 0x080 // Value equals bit 7 is set

/// Mask for the Format-Zero response code E-Range (error number)
#define F0_RC_RANGE_BITS_0_TO_6_E 0x07F // Value equals bits 0 to 6 are set

/// Mask for the Format-Zero response code V-Flag (version)
#define F0_RC_FLAG_BIT_8_V 0x100 // Value equals bit 8 is set

/// Mask for the Format-Zero response code Reserved-Flag (shall be zero)
#define F0_RC_FLAG_BIT_9_RESERVED 0x200 // Value equals bit 9 is set

/// Mask for the Format-Zero response code T-Flag (TCG/Vendor indicator); equals TPM_Vendor_Specific32 response code offset in TPM1.2
#define F0_RC_FLAG_BIT_10_T 0x400 // Value equals bit 10 is set

/// Mask for the Format-Zero response code S-Flag (severity)
#define F0_RC_FLAG_BIT_11_S 0x800 // Value equals bit 11 is set

/// Mask for the Format-One response code E-Range (error number)
#define F1_RC_RANGE_BITS_0_TO_5_E 0x03F // Value equals bits 0 to 5 are set

/// Mask for the Format-One response code P-Flag
#define F1_RC_FLAG_BIT_6_P 0x040 // Value equals bit 6 is set

/// Mask for the Format-One response code N-Range (number of handle, session or parameter in error)
#define F1_RC_RANGE_BITS_8_TO_11_N 0xF00 // Value equals bits 8 to 11 are set

// ------------------------- TPM1.2-specific constants ------------------------
/// TPM1.2 response code offset for a warning
#define TSS_TPM_NON_FATAL 0x800

// ------------------------- TPM2.0-specific constants ------------------------

/// If the value of the Format-One response code N-Range is higher than this value, the value represents the number of a session; otherwise it represents a handle
#define F1_RC_N_MIN_SESSION_VALUE 0x008 // Value equals 1000 (binary)

/// The value of RC_MAX_FM0 (without RC_VER1 offset)
#define RC_MAX_FM0_NO_OFFSET 0x07F

/// The value of TPM_RC_NOT_USED (without RC_WARN offset)
#define TPM_RC_NOT_USED_NO_OFFSET 0x07F

// ---------------------------- Method declarations ----------------------------

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief      Parses a TPM2.0 Format-One response code
 *  @details    Parses a given TPM2.0 Format-One response code and returns a corresponding message.
 *
 *  @param      PunErrorCode            The TPM2.0 Format-One response code to be parsed.
 *  @param      PwszResponseBuffer      In: A target buffer for the output message\n
 *                                      Out: A buffer containing the null-terminated message
 *  @param      PunResponseBufferSize   The size of the target buffer.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatOneGetMessage(
    _In_                                    unsigned int    PunErrorCode,
    _Inout_z_cap_(PunResponseBufferSize)    wchar_t*        PwszResponseBuffer,
    _In_                                    unsigned int    PunResponseBufferSize);

/**
 *  @brief      Parses a TPM2.0 Format-Zero response code
 *  @details    Parses a given TPM2.0 Format-Zero response code and returns a corresponding message.
 *
 *  @param      PunErrorCode            The TPM2.0 Format-Zero response code to be parsed.
 *  @param      PwszResponseBuffer      In: A target buffer for the output message\n
 *                                      Out: A buffer containing the null-terminated message
 *  @param      PunResponseBufferSize   The size of the target buffer.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatZeroGetMessage(
    _In_                                    unsigned int    PunErrorCode,
    _Inout_z_cap_(PunResponseBufferSize)    wchar_t*        PwszResponseBuffer,
    _In_                                    unsigned int    PunResponseBufferSize);

/**
 *  @brief      Gets the message for a TPM2.0 Format-Zero error number
 *  @details    Gets the error message for a given TPM2.0 Format-Zero error number (unmasked with RC_VER1).
 *
 *  @param      PunErrorCode            The RC_VER1 (0x100) unmasked TPM2.0 Format-Zero error number.
 *  @param      PwszResponseBuffer      In: A target buffer for the output message\n
 *                                      Out: A buffer containing the null-terminated message
 *  @param      PunResponseBufferSize   The size of the target buffer.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatZeroErrorGetMessage(
    _In_                                    unsigned int    PunErrorCode,
    _Inout_z_cap_(PunResponseBufferSize)    wchar_t*        PwszResponseBuffer,
    _In_                                    unsigned int    PunResponseBufferSize);

/**
 *  @brief      Gets the message for a TPM2.0 Format-One error number
 *  @details    Gets the error message for a given TPM2.0 Format-One error number (unmasked with RC_FMT1).
 *
 *  @param      PunErrorCode            The RC_FMT1 (0x080) unmasked TPM2.0 Format-One error number.
 *  @param      PwszResponseBuffer      In: A target buffer for the output message\n
 *                                      Out: A buffer containing the null-terminated message
 *  @param      PunResponseBufferSize   The size of the target buffer.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatOneErrorGetMessage(
    _In_                                    unsigned int    PunErrorCode,
    _Inout_z_cap_(PunResponseBufferSize)    wchar_t*        PwszResponseBuffer,
    _In_                                    unsigned int    PunResponseBufferSize);

/**
 *  @brief      Gets the message for a TPM2.0 Format-One warning number
 *  @details    Gets the message for a given TPM2.0 Format-One warning number (unmasked with RC_FMT1).
 *
 *  @param      PunErrorCode            The RC_WARN (0x900) unmasked TPM2.0 Format-One warning number.
 *  @param      PwszResponseBuffer      In: A target buffer for the output message\n
 *                                      Out: A buffer containing the null-terminated message
 *  @param      PunResponseBufferSize   The size of the target buffer.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatZeroWarningGetMessage(
    _In_                                    unsigned int    PunErrorCode,
    _Inout_z_cap_(PunResponseBufferSize)    wchar_t*        PwszResponseBuffer,
    _In_                                    unsigned int    PunResponseBufferSize);

/**
 *  @brief      Parses a TPM1.2 response code
 *  @details    Parses a given TPM1.2 response code and returns a corresponding message.
 *
 *  @param      PunErrorCode            The TPM1.2 response code to be parsed.
 *  @param      PwszResponseBuffer      In: A target buffer for the output message\n
 *                                      Out: A buffer containing the null-terminated message
 *  @param      PunResponseBufferSize   The size of the target buffer.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_TPM12GetMessage(
    _In_                                    unsigned int    PunErrorCode,
    _Inout_z_cap_(PunResponseBufferSize)    wchar_t*        PwszResponseBuffer,
    _In_                                    unsigned int    PunResponseBufferSize);

/**
 *  @brief      Topmost function for parsing a TPM2.0 response code
 *  @details    Parses a given TPM2.0 response code and returns a corresponding message.
 *
 *  @param      PunErrorCode            The TPM2.0 response code to be parsed.
 *  @param      PwszResponseBuffer      In: A target buffer for the output message\n
 *                                      Out: A buffer containing the null-terminated message
 *  @param      PunResponseBufferSize   The size of the target buffer.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_GetMessage(
    _In_                                    unsigned int    PunErrorCode,
    _Inout_z_cap_(PunResponseBufferSize)    wchar_t*        PwszResponseBuffer,
    _In_                                    unsigned int    PunResponseBufferSize);

#ifdef __cplusplus
}
#endif
