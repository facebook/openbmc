/**
 *	@brief		Declares the TPM_FieldUpgradeStart command.
 *	@details	The module receives the input parameters, marshals these parameters to a byte array and sends the command to the TPM.
 *	@file		TPM_FieldUpgradeStart.h
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

#include "StdInclude.h"
#include "DeviceManagement.h"
#include "../Crypt/Crypt.h"
#include "../Platform/Platform.h"
#include "TPM_Types.h"
#include "TPM_Marshal.h"
#include "TPM2_Types.h"
#include "TPM2_FieldUpgradeTypes.h"
#include "TPM2_Marshal.h"
#include "TPM2_FieldUpgradeMarshal.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	_In_opt_									TSS_TPM_NONCE*		PpsLastNonceEven);

#ifdef __cplusplus
}
#endif