/**
 *  @brief      Declares types and definitions for the TPM2 vendor specific capabilities
 *  @file       TPM2_VendorTypes.h
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
#include "TPM2_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************************************************************
 * Vendor specific property definitions for TPM_CAP_VENDOR_PROPERTY (0x00000100)
 ********************************************************************************/

#define     PT_VENDOR_FIX                                   0x80000000
/// SLB9665 and SLB9670 properties
#define     TPM_PT_VENDOR_FIX_SMLI2                         (PT_VENDOR_FIX + 1)
/// SLB966X properties
#define     TPM_PT_VENDOR_FIX_TIME_TO_SLEEP                 (PT_VENDOR_FIX + 2)
/// Firmware update properties (new with SLB9672)
#define     TPM_PT_VENDOR_FIX_FU_COUNTER                    (PT_VENDOR_FIX + 3)
#define     TPM_PT_VENDOR_FIX_FU_COUNTER_SAME               (PT_VENDOR_FIX + 4)
#define     TPM_PT_VENDOR_FIX_FU_START_HASH_DIGEST          (PT_VENDOR_FIX + 5)
#define     TPM_PT_VENDOR_FIX_FU_LOADER_FW_VERSION          (PT_VENDOR_FIX + 6)
#define     TPM_PT_VENDOR_FIX_FU_OPERATION_MODE             (PT_VENDOR_FIX + 7)
#define     TPM_PT_VENDOR_FIX_FU_KEYGROUP_ID                (PT_VENDOR_FIX + 8)
#define     TPM_PT_VENDOR_FIX_FU_CHIP_IDENT                 (PT_VENDOR_FIX + 9)
#define     TPM_PT_VENDOR_FIX_FU_CURRENT_TPM_FW_VERSION     (PT_VENDOR_FIX + 10)
#define     TPM_PT_VENDOR_FIX_FU_NEW_TPM_FW_VERSION         (PT_VENDOR_FIX + 11)
#define     TPM_PT_VENDOR_FIX_FU_PROPERTIES                 (PT_VENDOR_FIX + 12)

/**
 *  @brief      TPML_MAX_BUFFER structure
 *  @details    TPML_MAX_BUFFER structure definition
 */
typedef struct _TSS_TPML_MAX_BUFFER
{
    /// Number of properties
    /// A value of zero is allowed.
    uint32_t count;
    /// An array of bytes
    TSS_TPM2B_MAX_BUFFER buffer[1];
} TSS_TPML_MAX_BUFFER;

/**
 *  @brief      TPMU_VENDOR_CAPABILITY union
 *  @details    TPMU_VENDOR_CAPABILITY union definition
 */
typedef union _TSS_TPMU_VENDOR_CAPABILITY {
    TSS_TPML_MAX_BUFFER vendorData;
} TSS_TPMU_VENDOR_CAPABILITY;

/**
 *  @brief      TPMS_VENDOR_CAPABILITY structure
 *  @details    TPMS_VENDOR_CAPABILITY structure definition\n
 *              Table 105 - Definition of TPMS_CAPABILITY_DATA Structure [OUT]
 */
typedef struct _TSS_TPMS_VENDOR_CAPABILITY_DATA {
    /// The capability
    TSS_TPM_CAP capability;
    /// The capability data
    TSS_TPMU_VENDOR_CAPABILITY data;
} TSS_TPMS_VENDOR_CAPABILITY_DATA;

#ifdef __cplusplus
}
#endif
