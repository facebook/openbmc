/**
 *	@brief		Declares the command flow to clear the TPM1.2 ownership.
 *	@details	This module removes the TPM owner that was temporarily created during an update from TPM1.2 to TPM1.2.
 *	@file		CommandFlow_Tpm12ClearOwnership.h
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
#include "TPMFactoryUpdStruct.h"
#include "TPM_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

// SHA1 Hash of an owner password 1...8
extern const TSS_TPM_AUTHDATA C_defaultOwnerAuthData;

/**
 *	@brief		Processes a sequence of TPM commands to clear the TPM1.2 ownership.
 *	@details	This function removes the TPM owner that was temporarily created during an update from TPM1.2 to TPM1.2.
 *				The function utilizes the MicroTss library.
 *
 *	@param		PpTpmClearOwnership		Pointer to an initialized IfxTpm12ClearOwnership structure to be filled in
 *
 *	@retval		RC_SUCCESS						The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER				An invalid parameter was passed to the function. PpTpmClearOwnership was invalid.
 *	@retval		RC_E_NOT_SUPPORTED_FEATURE		In case of the TPM is a TPM2.0
 *	@retval		RC_E_TPM12_NO_OWNER				The TPM1.2 does not have an owner.
 *	@retval		RC_E_NO_IFX_TPM					The underlying TPM is not an Infineon TPM.
 *	@retval		RC_E_UNSUPPORTED_CHIP			In case of the underlying TPM does not support that functionality
 *	@retval		RC_E_TPM12_INVALID_OWNERAUTH	In case of the expected owner authorization can not be verified
 *	@retval		RC_E_FAIL						An unexpected error occurred.
 *	@retval		...								Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_Tpm12ClearOwnership_Execute(
	_Inout_ IfxTpm12ClearOwnership* PpTpmClearOwnership);

#ifdef __cplusplus
}
#endif
