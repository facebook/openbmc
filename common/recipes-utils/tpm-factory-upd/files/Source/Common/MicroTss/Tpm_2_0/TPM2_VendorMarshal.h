/**
 *  @brief      Declares unmarshal operations for TPM2.0 commands, structures and types.
 *  @details    This module provides vendor specific unmarshalling function for TPM2.0 commands,
 *              structures and types.
 *  @file       TPM2_VendorMarshal.h
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
#include "TPM2_VendorTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief      Unmarshal structure of type TPML_MAX_BUFFER
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPML_MAX_BUFFER_Unmarshal(
    _Out_ TSS_TPML_MAX_BUFFER* PpTarget,
    _Inout_ TSS_BYTE **PprgbBuffer,
    _Inout_ TSS_INT32 *PpnBufferSize);

/**
 *  @brief      Unmarshal union of type TPMU_VENDOR_CAPABILITY
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPMU_VENDOR_CAPABILITY_Unmarshal(
    _Inout_ TSS_TPMU_CAPABILITIES* PpTarget,
    _Inout_ TSS_BYTE **PprgbBuffer,
    _Inout_ TSS_INT32 *PpnBufferSize);

#ifdef __cplusplus
}
#endif
