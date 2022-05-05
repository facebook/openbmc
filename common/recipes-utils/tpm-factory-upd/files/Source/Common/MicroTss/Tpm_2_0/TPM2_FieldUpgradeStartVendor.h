/**
 *	@brief		Declares the TPM2_FieldUpgradeStartVendor command
 *	@details	The module receives the input parameters marshals these parameters
 *				to a byte array sends the command to the TPM and unmarshals the response
 *				back to the out parameters
 *	@file		TPM2_FieldUpgradeStartVendor.h
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
#pragma once

#include "TPM2_Types.h"
#include "TPM2_FieldUpgradeTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	_Out_	TSS_AcknowledgmentResponseData*		PpsAuthorizationSessionResponseData);

#ifdef __cplusplus
}
#endif
