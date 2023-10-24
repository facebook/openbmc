/**
 *  @brief      Declares the configuration settings parser interface
 *  @details    This interface declares some functions for a configuration parser implementation
 *  @file       IConfigSettings.h
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

/// A type definition for function signature for initialize parsing function
typedef unsigned int
(*IConfigSettings_InitializeParsing) ();

/// A type definition for function signature for finalize parsing function
typedef unsigned int
(*IConfigSettings_FinalizeParsing)(
    _In_ unsigned int PunReturnValue);
/// A type definition for function signature for parse function
typedef unsigned int
(*IConfigSettings_Parse) (
    _In_z_count_(PunSectionSize)    const wchar_t*  PwszSection,
    _In_                            unsigned int    PunSectionSize,
    _In_z_count_(PunKeySize)        const wchar_t*  PwszKey,
    _In_                            unsigned int    PunKeySize,
    _In_z_count_(PunValueSize)      const wchar_t*  PwszValue,
    _In_                            unsigned int    PunValueSize);

/**
 *  @brief      Initialize configuration settings parsing
 *  @details
 *
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     RC_E_FAIL   An unexpected error occurred.
 */
_Check_return_
unsigned int
ConfigSettings_InitializeParsing();

/**
 *  @brief      Parses the configuration setting
 *  @details    Parses the configuration setting identified by the common configuration module code
 *
 *  @param      PwszSection         Pointer to a wide character array containing the current section.
 *  @param      PunSectionSize      Size of the section buffer in elements including the zero termination.
 *  @param      PwszKey             Pointer to a wide character array containing the current key.
 *  @param      PunKeySize          Size of the key buffer in elements including the zero termination.
 *  @param      PwszValue           Pointer to a wide character array containing the current value.
 *  @param      PunValueSize        Size of the value buffer in elements including the zero termination.
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function. It is NULL or empty.
 *  @retval     RC_E_FAIL           An unexpected error occurred.
 */
_Check_return_
unsigned int
ConfigSettings_Parse(
    _In_z_count_(PunSectionSize)    const wchar_t*  PwszSection,
    _In_                            unsigned int    PunSectionSize,
    _In_z_count_(PunKeySize)        const wchar_t*  PwszKey,
    _In_                            unsigned int    PunKeySize,
    _In_z_count_(PunValueSize)      const wchar_t*  PwszValue,
    _In_                            unsigned int    PunValueSize);

/**
 *  @brief      Finalize configuration settings parsing
 *  @details
 *
 *  @param      PunReturnValue  Current return code which can be overwritten here.
 *  @retval     PunReturnValue  In case PunReturnValue is not equal to RC_SUCCESS.
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       An unexpected error occurred.
 */
_Check_return_
unsigned int
ConfigSettings_FinalizeParsing(
    _In_ const unsigned int PunReturnValue);

#ifdef __cplusplus
}
#endif
