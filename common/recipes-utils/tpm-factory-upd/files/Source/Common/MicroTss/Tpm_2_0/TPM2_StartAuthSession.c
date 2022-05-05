/**
 *	@brief		Implements the TPM2_StartAuthSession method
 *	@file		TPM2_StartAuthSession.c
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
#include "TPM2_StartAuthSession.h"
#include "TPM2_Marshal.h"
#include "DeviceManagement.h"
#include "Platform.h"
#include "StdInclude.h"

/**
 *	@brief	Implementation of TPM2_StartAuthSession command.
 *
 *	@retval TPM_RC_ATTRIBUTES				tpmKey does not reference a decrypt key
 *	@retval TPM_RC_CONTEXT_GAP				the difference between the most recently created active context and the oldest active context is at the limits of the TPM
 *	@retval TPM_RC_HANDLE					input decrypt key handle only has public portion loaded
 *	@retval TPM_RC_MODE						symmetric specifies a block cipher but the mode is not TPM_ALG_CFB.
 *	@retval TPM_RC_SESSION_HANDLES			no session handle is available
 *	@retval TPM_RC_SESSION_MEMORY			no more slots for loading a session
 *	@retval TPM_RC_SIZE						nonce less than 16 octets or greater than the size of the digest produced by authHash
 *	@retval TPM_RC_VALUE					secret size does not match decrypt key type; or the recovered secret is larger than the digest size of the nameAlg of tpmKey; or, for an RSA decrypt key, if encryptedSecret is greater than the public exponent of tpmKey.
 */
_Check_return_
unsigned int
TSS_TPM2_StartAuthSession(
	_In_	TSS_TPMI_DH_OBJECT				tpmKey,
	_In_	TSS_TPMI_DH_ENTITY				bind,
	_In_	TSS_TPM2B_NONCE					nonceCaller,
	_In_	TSS_TPM2B_ENCRYPTED_SECRET		encryptedSalt,
	_In_	TSS_TPM_SE						sessionType,
	_In_	TSS_TPMT_SYM_DEF				symmetric,
	_In_	TSS_TPMI_ALG_HASH				authHash,
	_Out_	TSS_TPMI_SH_AUTH_SESSION*		pSessionHandle,
	_Out_	TSS_TPM2B_NONCE*				pNonceTPM
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
		// Request parameters
		TSS_TPM_ST tag = TSS_TPM_ST_NO_SESSIONS;
		TSS_UINT32 unCommandSize = 0;
		TSS_TPM_CC commandCode = TSS_TPM_CC_StartAuthSession;
		// Response parameters
		TSS_UINT32 unResponseSize = 0;
		TSS_TPM_RC responseCode = TSS_TPM_RC_SUCCESS;

		// Initialize _Out_ parameters
		unReturnValue |= Platform_MemorySet(pSessionHandle, 0x00, sizeof(TSS_TPMI_SH_AUTH_SESSION));
		unReturnValue |= Platform_MemorySet(pNonceTPM, 0x00, sizeof(TSS_TPM2B_NONCE));
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
		unReturnValue = TSS_TPMI_DH_OBJECT_Marshal(&tpmKey, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMI_DH_ENTITY_Marshal(&bind, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM2B_NONCE_Marshal(&nonceCaller, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM2B_ENCRYPTED_SECRET_Marshal(&encryptedSalt, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM_SE_Marshal(&sessionType, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMT_SYM_DEF_Marshal(&symmetric, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPMI_ALG_HASH_Marshal(&authHash, &pbBuffer, &nSizeRemaining);
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
		unReturnValue = TSS_TPMI_SH_AUTH_SESSION_Unmarshal(pSessionHandle, &pbBuffer, &nSizeRemaining);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_TPM2B_NONCE_Unmarshal(pNonceTPM, &pbBuffer, &nSizeRemaining);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;
	}
	WHILE_FALSE_END;
	return unReturnValue;
}