/**
 *	@brief		Implements the TPM2_FieldUpgradeStartVendor command
 *	@details	The module receives the input parameters marshals these parameters
 *				to a byte array sends the command to the TPM and unmarshals the response
 *				back to the out parameters
 *	@file		TPM2_FieldUpgradeStartVendor.c
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

#include "TPM2_FieldUpgradeStartVendor.h"
#include "TPM2_Marshal.h"
#include "TPM2_FieldUpgradeMarshal.h"
#include "DeviceManagement.h"
#include "Platform.h"
#include "StdInclude.h"

/**
 *	@brief		This function handles the TPM2_FieldUpgradeStartVendor command
 *	@details	The function receives the input parameters marshals these parameters
 *				to a byte array sends the command to the TPM and unmarshals the response
 *				back to the out parameters
 *
 *	@param		PhAuthorization						The authorization handle (Platform Authorization)
 *	@param		PsAuthorizationSessionRequestData	The request authorization data
 *	@param		PsSignedData						The signed data structure (Policy parameter block)
 *	@param		PpusStartSize						The out start size
 *	@param		PpsAuthorizationSessionResponseData	The response authorization data
 *
 *	@retval		RC_SUCCESS							The operation completed successfully.
 *	@retval		...									Error codes from Micro TSS functions
 */
_Check_return_
unsigned int
TSS_TPM2_FieldUpgradeStartVendor(
	_In_	TSS_TPMI_RH_PLATFORM				PhAuthorization,
	_In_	TSS_AuthorizationCommandData		PsAuthorizationSessionRequestData,
	_In_	sSignedData_d						PsSignedData,
	_Out_	TSS_UINT16*							PpusStartSize,
	_Out_	TSS_AcknowledgmentResponseData*		PpsAuthorizationSessionResponseData)
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
		TSS_TPM_CC commandCode = TPM2_CC_FieldUpgradeStartVendor;
		SubCmd_d subCommand = TPM_FieldUpgradeStart;

		// Response parameters
		TSS_UINT32 unResponseSize = 0;
		TSS_TPM_RC responseCode = TSS_TPM_RC_SUCCESS;

		// Initialize _Out_ parameters
		unReturnValue |= Platform_MemorySet(PpusStartSize, 0x00, sizeof(UINT16));
		unReturnValue |= Platform_MemorySet(PpsAuthorizationSessionResponseData, 0x00, sizeof(TSS_AcknowledgmentResponseData));
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
		unReturnValue = TSS_TPMI_RH_PLATFORM_Marshal(&PhAuthorization, &pbBuffer, &nSizeRemaining);
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
			unReturnValue = TSS_AuthorizationCommandData_Marshal(&PsAuthorizationSessionRequestData, &pbBuffer, &nSizeRemaining);
			if (RC_SUCCESS != unReturnValue)
				break;
			// Overwrite authorization data size
			unSizeSessionData = (TSS_UINT32)(pbBuffer - (prgbPositionOfSize + nSizeRemain));
			unReturnValue = TSS_UINT32_Marshal(&unSizeSessionData, &prgbPositionOfSize, &nSizeRemain);
			if (RC_SUCCESS != unReturnValue)
				break;
		}

		unReturnValue = TSS_SubCmd_d_Marshal(&subCommand, &pbBuffer, &nSizeRemaining);
		if (RC_SUCCESS != unReturnValue)
			break;

		{
			TSS_BYTE* pbPositionSize = NULL;
			TSS_UINT16 usSize = 0;
			TSS_INT32 nSizeOfSize = 2;

			pbPositionSize = pbBuffer;
			// Marshal size of sSignedInfoData
			unReturnValue = TSS_UINT16_Marshal(&usSize, &pbBuffer, &nSizeRemaining);
			if (RC_SUCCESS != unReturnValue)
				break;
			// Marshal sSignedData
			unReturnValue = TSS_sSignedData_d_Marshal(&PsSignedData, &pbBuffer, &nSizeRemaining);
			if (RC_SUCCESS != unReturnValue)
				break;
			// Calculate correct size of marshaled structure
			usSize = (TSS_UINT16)(pbBuffer - pbPositionSize - sizeof(uint16_t));
			// Marshal size of sSignedInfoData
			unReturnValue = TSS_UINT16_Marshal(&usSize, &pbPositionSize, &nSizeOfSize);
			if (RC_SUCCESS != unReturnValue)
				break;
		}
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

		unReturnValue = TSS_UINT16_Unmarshal(PpusStartSize, &pbBuffer, &nSizeRemaining);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;
		// Unmarshal Session Context
		{
			unReturnValue = TSS_AcknowledgmentResponseData_Unmarshal(PpsAuthorizationSessionResponseData, &pbBuffer, &nSizeRemaining);
			if (RC_SUCCESS != unReturnValue)
				break;
		}
	}
	WHILE_FALSE_END;

	return unReturnValue;
}
