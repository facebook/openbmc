/**
 *  @brief      Declares the TPM2_GetCapability method
 *  @file       TPM2_GetCapability.h
 *  @details    This file was auto-generated based on TPM2.0 specification revision 116.
 *
 *              Copyright Licenses:
 *
 *              * Trusted Computing Group (TCG) grants to the user of the source code
 *              in this specification (the "Source Code") a worldwide, irrevocable,
 *              nonexclusive, royalty free, copyright license to reproduce, create
 *              derivative works, distribute, display and perform the Source Code and
 *              derivative works thereof, and to grant others the rights granted
 *              herein.
 *
 *              * The TCG grants to the user of the other parts of the specification
 *              (other than the Source Code) the rights to reproduce, distribute,
 *              display, and perform the specification solely for the purpose of
 *              developing products based on such documents.
 *
 *              Source Code Distribution Conditions:
 *
 *              * Redistributions of Source Code must retain the above copyright
 *              licenses, this list of conditions and the following disclaimers.
 *
 *              * Redistributions in binary form must reproduce the above copyright
 *              licenses, this list of conditions and the following disclaimers in the
 *              documentation and/or other materials provided with the distribution.
 *
 *              Disclaimers:
 *
 *              * THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF
 *              LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH
 *              RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)
 *              THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR
 *              OTHERWISE. Contact TCG Administration
 *              (admin@trustedcomputinggroup.org) for information on specification
 *              licensing rights available through TCG membership agreements.
 *
 *              * THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED
 *              WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR
 *              FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR
 *              NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY
 *              OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.
 *
 *              * Without limitation, TCG and its members and licensors disclaim all
 *              liability, including liability for infringement of any proprietary
 *              rights, relating to use of information in this specification and to
 *              the implementation of this specification, and TCG disclaims all
 *              liability for cost of procurement of substitute goods or services,
 *              lost profits, loss of use, loss of data or any incidental,
 *              consequential, direct, indirect, or special damages, whether under
 *              contract, tort, warranty or otherwise, arising in any way out of use
 *              or reliance upon this specification or any information herein.
 *
 *              Any marks and brands contained herein are the property of their
 *              respective owners.
 */
#pragma once
#include "TPM2_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @brief  Implementation of TPM2_GetCapability command.
 *
 *  @retval TPM_RC_HANDLE                   value of property is in an unsupported handle range for the TPM_CAP_HANDLES capability value.
 *  @retval TPM_RC_VALUE                    invalid capability; or property is not 0 for the TPM_CAP_PCRS capability value.
 */
_Check_return_
unsigned int
TSS_TPM2_GetCapability(
    _In_    TSS_TPM_CAP                     capability,
    _In_    TSS_UINT32                      property,
    _In_    TSS_UINT32                      propertyCount,
    _Out_   TSS_TPMI_YES_NO*                pMoreData,
    _Out_   TSS_TPMS_CAPABILITY_DATA*       pCapabilityData
);

#ifdef __cplusplus
}
#endif
