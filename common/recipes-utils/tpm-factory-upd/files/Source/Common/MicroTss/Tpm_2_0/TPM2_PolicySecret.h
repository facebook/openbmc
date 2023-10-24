/**
 *  @brief      Declares the TPM2_PolicySecret method
 *  @file       TPM2_PolicySecret.h
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
 *  @brief  Implementation of TPM2_PolicySecret command.
 *
 *  @retval TPM_RC_CPHASH                   cpHash for policy was previously set to a value that is not the same as cpHashA.
 *  @retval TPM_RC_EXPIRED                  expiration indicates a time in the past.
 *  @retval TPM_RC_NONCE                    nonceTPM does not match the nonce associated with policySession.
 *  @retval TPM_RC_SIZE                     cpHashA is not the size of a digest for the hash associated with policySession.
 *  @retval TPM_RC_VALUE                    input policyID or expiration does not match the internal data in policy session.
 */
_Check_return_
unsigned int
TSS_TPM2_PolicySecret(
    _In_    TSS_TPMI_DH_ENTITY                  authHandle,
    _In_    TSS_AuthorizationCommandData*       pAuthHandleSessionRequestData,
    _In_    TSS_TPMI_SH_POLICY                  policySession,
    _In_    TSS_TPM2B_NONCE*                    pNonceTPM,
    _In_    TSS_TPM2B_DIGEST*                   pCpHashA,
    _In_    TSS_TPM2B_NONCE*                    pPolicyRef,
    _In_    TSS_INT32                           expiration,
    _Out_   TSS_TPM2B_TIMEOUT*                  pTimeout,
    _Out_   TSS_TPMT_TK_AUTH*                   pPolicyTicket,
    _Out_   TSS_AcknowledgmentResponseData*     pAuthHandleSessionResponseData
);

#ifdef __cplusplus
}
#endif
