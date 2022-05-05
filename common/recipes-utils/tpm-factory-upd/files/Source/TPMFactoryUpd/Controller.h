/**
 *	@brief		Declares the controller module for TPMFactoryUpd.
 *	@details	This module controls the TPMFactoryUpd view and business layers.
 *	@file		Controller.h
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
 *	@brief		This function initializes the applications's view and business layers.
 *	@details
 *
 *	@param		PnArgc			Parameter as provided in main()
 *	@param		PrgwszArgv		Parameter as provided in main()
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		RC_E_FAIL		An unexpected error occurred. E.g. PropertyStorage not initialized correctly
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_Initialize(
	_In_					int						PnArgc,
	_In_reads_z_(PnArgc)	const wchar_t* const	PrgwszArgv[]);

/**
 *	@brief		This function shows the response output
 *	@details
 *
 *	@param		PpTpm2ToolHeader	Response structure
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_ShowResponse(
	_In_	IfxToolHeader*	PpTpm2ToolHeader);

/**
 *	@brief		This function controls the TPMFactoryUpd view and business layers.
 *	@details	This function handles the program flow between UI and business modules.
 *
 *	@param		PnArgc			Parameter as provided in main()
 *	@param		PrgwszArgv		Parameter as provided in main()
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_Proceed(
	_In_					int						PnArgc,
	_In_reads_z_(PnArgc)	const wchar_t* const	PrgwszArgv[]);

/**
 *	@brief		This function uninitializes the TPMFactoryUpd view and business layers.
 *	@details	This function uninitializes the DeviceManagement.
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_Uninitialize();

/**
 *	@brief		This function controls the TPMFactoryUpd view and business layers regarding the provided command line.
 *	@details	This function handles the program flow between UI and business modules.
 *
 *	@param		PppResponseData		Pointer to a IfxToolHeader structure; inner pointer
 *									must be initialized with NULL or allocated on the heap!
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Controller_ProceedWork(
	_Inout_ IfxToolHeader** PppResponseData);

#ifdef __cplusplus
}
#endif
