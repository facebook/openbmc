/**
 *	@brief		Declares the command flow to retrieve information about the TPM1.2 or TPM2.0.
 *	@details	This module collects TPM Firmware Update related information via specific TPM commands.
 *				Afterwards the TPM related information is returned to the calling module.
 *	@file		CommandFlow_TpmInfo.h
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

#ifdef __cplusplus
extern "C" {
#endif

/**
 *	@brief		Processes a sequence of TPM info related commands.
 *	@details	This function collects TPM Firmware Update related information via specific TPM commands.
 *				Afterwards the TPM related information is returned to the calling module.
 *				The function utilizes the MicroTss library.
 *
 *	@param		PpTpmInfo				Pointer to an initialized IfxInfo structure to be filled in
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PpTpmInfo was invalid.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
CommandFlow_TpmInfo_Execute(
	_Inout_ IfxInfo* PpTpmInfo);

#ifdef __cplusplus
}
#endif
