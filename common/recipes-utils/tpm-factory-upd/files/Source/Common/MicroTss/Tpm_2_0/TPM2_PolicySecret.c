/**
 *	@brief		Implements the TPM2_PolicySecret method
 *	@file		TPM2_PolicySecret.c
 *	@details	This file was auto-generated based on TPM2.0 specification revision 116.
 *	@copyright
 *				Copyright Licenses:
 *
 *				* Trusted Computing Group (TCG) grants to the user of the source code
 *				in this specification (the "Source Code") a worldwide, irrevocable,
 *				nonexclusive, royalty free, copyright license to reproduce, create
 *				derivative works, distribute, display and perform the Source Code and
 *				derivative works thereof, and to grant others the rights granted
 *				herein.
 *
 *				* The TCG grants to the user of the other parts of the specification
 *				(other than the Source Code) the rights to reproduce, distribute,
 *				display, and perform the specification solely for the purpose of
 *				developing products based on such documents.
 *
 *				Source Code Distribution Conditions:
 *
 *				* Redistributions of Source Code must retain the above copyright
 *				licenses, this list of conditions and the following disclaimers.
 *
 *				* Redistributions in binary form must reproduce the above copyright
 *				licenses, this list of conditions and the following disclaimers in the
 *				documentation and/or other materials provided with the distribution.
 *
 *				Disclaimers:
 *
 *				* THE COPYRIGHT LICENSES SET FORTH ABOVE DO NOT REPRESENT ANY FORM OF
 *				LICENSE OR WAIVER, EXPRESS OR IMPLIED, BY ESTOPPEL OR OTHERWISE, WITH
 *				RESPECT TO PATENT RIGHTS HELD BY TCG MEMBERS (OR OTHER THIRD PARTIES)
 *				THAT MAY BE NECESSARY TO IMPLEMENT THIS SPECIFICATION OR
 *				OTHERWISE. Contact TCG Administration
 *				(admin@trustedcomputinggroup.org) for information on specification
 *				licensing rights available through TCG membership agreements.
 *
 *				* THIS SPECIFICATION IS PROVIDED "AS IS" WITH NO EXPRESS OR IMPLIED
 *				WARRANTIES WHATSOEVER, INCLUDING ANY WARRANTY OF MERCHANTABILITY OR
 *				FITNESS FOR A PARTICULAR PURPOSE, ACCURACY, COMPLETENESS, OR
 *				NONINFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS, OR ANY WARRANTY
 *				OTHERWISE ARISING OUT OF ANY PROPOSAL, SPECIFICATION OR SAMPLE.
 *
 *				* Without limitation, TCG and its members and licensors disclaim all
 *				liability, including liability for infringement of any proprietary
 *				rights, relating to use of information in this specification and to
 *				the implementation of this specification, and TCG disclaims all
 *				liability for cost of procurement of substitute goods or services,
 *				lost profits, loss of use, loss of data or any incidental,
 *				consequential, direct, indirect, or special damages, whether under
 *				contract, tort, warranty or otherwise, arising in any way out of use
 *				or reliance upon this specification or any information herein.
 *
 *				Any marks and brands contained herein are the property of their
 *				respective owners.
 */
#include "TPM2_PolicySecret.h"
#include "TPM2_Marshal.h"
#include "DeviceManagement.h"
#include "Platform.h"
#include "StdInclude.h"

/**
 *	@brief	Implementation of TPM2_PolicySecret command.
 *
 *	@retval TPM_RC_CPHASH					cpHash for policy was previously set to a value that is not the same as cpHashA
 *	@retval TPM_RC_EXPIRED					expiration indicates a time in the past
 *	@retval TPM_RC_NONCE					nonceTPM does not match the nonce associated with policySession
 *	@retval TPM_RC_SIZE						cpHashA is not the size of a digest for the hash associated with policySession
 *	@retval TPM_RC_VALUE					input policyID or expiration does not match the internal data in policy session
 */
_Check_return_
unsigned int
TSS_TPM2_PolicySecret(
	_In_	TSS_TPMI_DH_ENTITY					authHandle,
	_In_	TSS_AuthorizationCommandData		authHandleSessionRequestData,
	_In_	TSS_TPMI_SH_POLICY					policySession,
	_In_	TSS_TPM2B_NONCE						nonceTPM,
	_In_	TSS_TPM2B_DIGEST					cpHashA,
	_In_	TSS_TPM2B_NONCE						policyRef,
	_In_	TSS_INT32							expiration,
	_Out_	TSS_TPM2B_TIMEOUT*					pTimeout,
	_Out_	TSS_TPMT_TK_AUTH*					pPolicyTicket,
	_Out_	TSS_AcknowledgmentResponseData*		pAuthHandleSessionResponseData
)
{
	unsigned int unReturnValue = RC_SUCCESS;
	do
	{
		TSS_BYTE rgbRequest[TSS_MAX_COMMAND_SIZE] = {0};
		TSS_BYTE rgbResponse[TSS_MAX_RESPONSE_SIZE] = {0};
		TSS_BYTE* pbBuffer = NULL;
		TSS_INT32 nSizeRemaining = sizeof(rgbRequest);
		TSS_INT32 nSizeResponse = sizeof(rgbResponse);
		TSS_UINT32 unParameterSize = 0;
		// Request parameters
		TSS_TPM_ST tag = TSS_TPM_ST_SESSIONS;
		TSS_UINT32 unCommandSize = 0;
		TSS_TPM_CC commandCode = TSS_TPM_CC_PolicySecret;
		// Response parameters
		TSS_UINT32 unResponseSize = 0;
		TSS_TPM_RC responseCode = TSS_TPM_RC_SUCCESS;

		// Initialize _Out_ parameters
		unReturnValue |= Platform_MemorySet(pTimeout, 0x00, sizeof(TSS_TPM2B_TIMEOUT));
		unReturnValue |= Platform_MemorySet(pPolicyTicket, 0x00, sizeof(TSS_TPMT_TK_AUTH));
		unReturnValue |= Platform_MemorySet(pAuthHandleSessionResponseData, 0x00, sizeof(TSS_AcknowledgmentResponseData));
		if (RC_SUCCESS != unReturnValue)
			break;
		// Marshal the request
		pbBuffer = rgbRequest;
		unReturnValue = TSS_TPMI_ST_COMMAND_TAG_Marshal(&tag, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_UINT32_Marshal(&unCommandSize, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM_CC_Marshal(&commandCode, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMI_DH_ENTITY_Marshal(&authHandle, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMI_SH_POLICY_Marshal(&policySession, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		// Marshal Session Context
		{
			TSS_UINT32 unSizeSessionData = 0;
			TSS_INT32 nSizeRemain = sizeof(TSS_UINT32);
			TSS_BYTE* prgbPositionOfSize = pbBuffer;
			unReturnValue = TSS_UINT32_Marshal(&unSizeSessionData, &pbBuffer, &nSizeRemaining);
			if (RC_SUCCESS != unReturnValue)
				break;
			unReturnValue = TSS_AuthorizationCommandData_Marshal(&authHandleSessionRequestData, &pbBuffer, &nSizeRemaining);
			if (RC_SUCCESS != unReturnValue)
				break;
			// Overwrite authorization data size
			unSizeSessionData = (TSS_UINT32) (pbBuffer - (prgbPositionOfSize + nSizeRemain));
			unReturnValue = TSS_UINT32_Marshal(&unSizeSessionData, &prgbPositionOfSize, &nSizeRemain);
			if (RC_SUCCESS != unReturnValue)
				break;
		}
		unReturnValue = TSS_TPM2B_NONCE_Marshal(&nonceTPM, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM2B_DIGEST_Marshal(&cpHashA, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM2B_NONCE_Marshal(&policyRef, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_INT32_Marshal(&expiration, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Overwrite unCommandSize
		unCommandSize = sizeof(rgbRequest) - nSizeRemaining;
		pbBuffer = rgbRequest + 2;
		nSizeRemaining = 4;
		unReturnValue = TSS_UINT32_Marshal(&unCommandSize, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Transmit the command over TDDL
		unReturnValue = DeviceManagement_Transmit(rgbRequest, unCommandSize, rgbResponse, (unsigned int*)&nSizeResponse);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;

		// Unmarshal the response
		pbBuffer = rgbResponse;
		nSizeRemaining = nSizeResponse;
		unReturnValue = TSS_TPM_ST_Unmarshal(&tag, &pbBuffer, &nSizeRemaining);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_UINT32_Unmarshal(&unResponseSize, &pbBuffer, &nSizeRemaining);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM_RC_Unmarshal(&responseCode, &pbBuffer, &nSizeRemaining);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;
		if (responseCode != TSS_TPM_RC_SUCCESS)
		{
			unReturnValue = RC_TPM_MASK | responseCode;
			break;
		}
		// Unmarshal the parameter size
		unReturnValue = TSS_UINT32_Unmarshal(&unParameterSize, &pbBuffer, &nSizeRemaining);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM2B_TIMEOUT_Unmarshal(pTimeout, &pbBuffer, &nSizeRemaining);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMT_TK_AUTH_Unmarshal(pPolicyTicket, &pbBuffer, &nSizeRemaining);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;
		// Unmarshal Session Context
		{
			unReturnValue = TSS_AcknowledgmentResponseData_Unmarshal(pAuthHandleSessionResponseData, &pbBuffer, &nSizeRemaining);
			if (RC_SUCCESS != unReturnValue)
				break;
		}
	}
	WHILE_FALSE_END;
	return unReturnValue;
}