/**
 *	@brief		Implements the TPM_FieldUpgradeStart command.
 *	@details	The module receives the input parameters, marshals these parameters to a byte array and sends the command to the TPM.
 *	@file		TPM_FieldUpgradeStart.c
 *	@copyright	Copyright 2014 - 2018 Infineon Technologies AG ( www.infineon.com )
 *
 *	@copyright	All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TPM_FieldUpgradeStart.h"

/**
 *	@brief		Calls TPM_FieldUpgradeStart.
 *	@details	Transmits the TPM1.2 command TPM_FieldUpgradeStart with the given policy parameter block.
 *
 *	@param		PpbPolicyParameterBlock			Pointer to policy parameter block to be sent
 *	@param		PunPolicyParameterBlockSize		Size of policy parameter block to be sent in bytes
 *	@param		PpbOwnerAuth					TPM Owner authorization data (optional, can be NULL).
 *	@param		PunAuthHandle					TPM Owner authorization handle (optional).
 *	@param		PpsLastNonceEven				Nonce needed for generating OIAP-authenticated TPM command (optional, can be NULL).
 *
 *	@retval		RC_SUCCESS						The operation completed successfully.
 *	@retval		RC_E_FAIL						An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER				An invalid parameter was passed to the function. PpbOwnerAuth is not NULL and PpsLastNonceEven is NULL
 *	@retval		...								Error codes from Micro TSS functions
 */
_Check_return_
unsigned int
TSS_TPM_FieldUpgradeStart(
	_In_bytecount_(PunPolicyParameterBlockSize)	const TSS_BYTE*		PpbPolicyParameterBlock,
	_In_										TSS_UINT16			PunPolicyParameterBlockSize,
	_In_opt_bytecount_(TSS_SHA1_DIGEST_SIZE)	const TSS_BYTE*		PpbOwnerAuth,
	_In_										TSS_TPM_AUTHHANDLE	PunAuthHandle,
	_In_opt_									TSS_TPM_NONCE*		PpsLastNonceEven)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		TSS_BYTE rgbRequest[TSS_MAX_COMMAND_SIZE] = {0};
		TSS_BYTE rgbResponse[TSS_MAX_RESPONSE_SIZE] = {0};
		TSS_BYTE* pbBuffer = NULL;
		TSS_INT32 nSizeRemaining = sizeof(rgbRequest);
		TSS_UINT32 unSizeResponse = sizeof(rgbResponse);
		// Request parameters
		TSS_TPM_ST tag = TSS_TPM_TAG_RQU_COMMAND;
		TSS_UINT32 unCommandSize = 0;
		TSS_TPM_CC commandCode = TPM_CC_FieldUpgradeCommand;
		TSS_BYTE* pbLRCStart = NULL;
		SubCmd_d subCommandCode = TPM_FieldUpgradeStart;
		TSS_BYTE bLRC = 0;
		// Response parameters
		TSS_UINT32 unResponseSize = 0;
		TSS_TPM_RC responseCode = TSS_TPM_RC_SUCCESS;
		// Authorization session parameters
		TSS_UINT16 usInParamDigestSize = 0;
		TSS_BYTE *pbInParamDigest = NULL;
		TSS_UINT16 usHmacInputSize = 0;
		TSS_BYTE *pbHmacInput = NULL;

		if (PpbOwnerAuth != NULL)
		{
			// Initialize temporary data buffers for HMAC calculation according to the OIAP
			usInParamDigestSize = sizeof(TSS_TPM_CC) + PunPolicyParameterBlockSize + 4;	// The SHA-1 input uses command code and policy parameter block.
			usHmacInputSize = TSS_SHA1_DIGEST_SIZE + 2 * sizeof(TSS_TPM_NONCE) + 1;	// The HMAC input uses (SHA1_DIGEST_SIZE + 2 * sizeof(TSS_TPM_NONCE) + 1) Bytes
			pbInParamDigest = (TSS_BYTE*)Platform_MemoryAllocateZero(usInParamDigestSize);
			pbHmacInput = (TSS_BYTE*)Platform_MemoryAllocateZero(usHmacInputSize);
			if (NULL == pbInParamDigest || NULL == pbHmacInput)
			{
				unReturnValue = RC_E_FAIL;
				break;
			}
			tag = TSS_TPM_TAG_RQU_AUTH1_COMMAND;
		}

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

		pbLRCStart = pbBuffer; // Store current position for later LRC calculation
		unReturnValue = TSS_SubCmd_d_Marshal(&subCommandCode, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_UINT16_Marshal(&PunPolicyParameterBlockSize, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;
		unReturnValue = TSS_BYTE_Array_Marshal(PpbPolicyParameterBlock, &pbBuffer, &nSizeRemaining, PunPolicyParameterBlockSize);
		if (RC_SUCCESS != unReturnValue)
			break;
		bLRC = TSS_CalcLRC(pbLRCStart, (TSS_UINT32)(pbBuffer - pbLRCStart)); // Calculate LRC beginning from stored location to current pointer position
		unReturnValue = TSS_UINT8_Marshal(&bLRC, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;

		if (PpbOwnerAuth != NULL)
		{
			TSS_TPM_AUTH_IN sAuthIn = {0};
			if (NULL == PpsLastNonceEven)
			{
				unReturnValue = RC_E_BAD_PARAMETER;
				break;
			}

			// Set data
			sAuthIn.dwAuthHandle = PunAuthHandle;
			unReturnValue = Crypt_GetRandom(sizeof(sAuthIn.sNonceOdd.nonce), sAuthIn.sNonceOdd.nonce);
			if (RC_SUCCESS != unReturnValue)
				break;
			sAuthIn.bContinueAuthSession = 0;

			// Fill pbInParamDigest buffer according to OIAP
			{
				TSS_BYTE* pbBuffer2 = pbInParamDigest;
				TSS_INT32 nSizeRemaining2 = usInParamDigestSize;
				unReturnValue = TSS_UINT32_Marshal(&commandCode, &pbBuffer2, &nSizeRemaining2);
				if (RC_SUCCESS != unReturnValue)
					break;

				unReturnValue = TSS_SubCmd_d_Marshal(&subCommandCode, &pbBuffer2, &nSizeRemaining2);
				if (RC_SUCCESS != unReturnValue)
					break;

				unReturnValue = TSS_UINT16_Marshal(&PunPolicyParameterBlockSize, &pbBuffer2, &nSizeRemaining2);
				if (RC_SUCCESS != unReturnValue)
					break;

				unReturnValue = TSS_BYTE_Array_Marshal(PpbPolicyParameterBlock, &pbBuffer2, &nSizeRemaining2, PunPolicyParameterBlockSize);
				if (RC_SUCCESS != unReturnValue)
					break;

				unReturnValue = TSS_UINT8_Marshal(&bLRC, &pbBuffer2, &nSizeRemaining2);
				if (RC_SUCCESS != unReturnValue)
					break;
			}
			// Create SHA-1 hash from the input parameters according to OIAP
			unReturnValue = Crypt_SHA1(pbInParamDigest, usInParamDigestSize, pbHmacInput);
			if (unReturnValue != RC_SUCCESS)
				break;

			// Fill HMAC input buffer according to OIAP
			{
				TSS_BYTE* pbBuffer2 = &pbHmacInput[TSS_SHA1_DIGEST_SIZE];
				TSS_INT32 nSizeRemaining2 = usHmacInputSize - TSS_SHA1_DIGEST_SIZE;
				unReturnValue = TSS_BYTE_Array_Marshal((TSS_BYTE*)PpsLastNonceEven, &pbBuffer2, &nSizeRemaining2, sizeof(TSS_TPM_NONCE));
				if (RC_SUCCESS != unReturnValue)
					break;
				unReturnValue = TSS_BYTE_Array_Marshal((TSS_BYTE*)&sAuthIn.sNonceOdd, &pbBuffer2, &nSizeRemaining2, sizeof(TSS_TPM_NONCE));
				if (RC_SUCCESS != unReturnValue)
					break;
				unReturnValue = TSS_BYTE_Marshal(&sAuthIn.bContinueAuthSession, &pbBuffer2, &nSizeRemaining2);
				if (RC_SUCCESS != unReturnValue)
					break;
			}

			// Create HMAC for authenticated TPM command according to OIAP
			unReturnValue = Crypt_HMAC(pbHmacInput, usHmacInputSize, PpbOwnerAuth, sAuthIn.sOwnerAuth.authdata);
			if (unReturnValue != RC_SUCCESS)
				break;

			unReturnValue = TSS_TPM_AUTH_IN_Marshal(&sAuthIn, &pbBuffer, &nSizeRemaining);
			if (TSS_TPM_RC_SUCCESS != unReturnValue)
				break;
		}

		// Update command size
		unCommandSize = sizeof(rgbRequest) - nSizeRemaining;
		pbBuffer = rgbRequest + sizeof(tag);
		unReturnValue = TSS_UINT32_Marshal(&unCommandSize, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Transmit the command over TDDL
		unReturnValue = DeviceManagement_Transmit(rgbRequest, unCommandSize, rgbResponse, &unSizeResponse);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;

		// Unmarshal the response
		pbBuffer = rgbResponse;
		nSizeRemaining = unSizeResponse;
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
	}
	WHILE_FALSE_END;

	return unReturnValue;
}