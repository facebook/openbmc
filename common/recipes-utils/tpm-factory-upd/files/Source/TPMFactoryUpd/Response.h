/**
 *	@brief		Declares the responses of the user interface
 *	@details
 *	@file		Response.h
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
#ifdef __cplusplus
extern "C" {
#endif

#include "StdInclude.h"
#include "TPMFactoryUpdStruct.h"
#include "ConsoleIO.h"

/**
 *	@brief		Show output
 *	@details	Takes the response information, formats and displays the output
 *
 *	@param		PpHeader			Header to show in response
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PpHeader is NULL.
 *	@retval		RC_E_INTERNAL		PpHeader->unType is invalid
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Response_Show(
	_In_ const IfxToolHeader* PpHeader);

/**
 *	@brief		Show TPM Info output
 *	@details	Format TPM Info output and display
 *
 *	@param		PpTpmInfo		Pointer to a IfxInfo response structure
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PpTpmInfo was invalid.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Response_ShowInfo(
	_In_ const IfxInfo* PpTpmInfo);

/**
 *	@brief		Show TPM Update output
 *	@details	Format TPM Update output and display
 *
 *	@param		PpTpmUpdate				Pointer to IfxUpdate response structure
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PpTpmUpdate is NULL or invalid
 *	@retval		...						Error codes from ConsoleIO_Write functions
 */
_Check_return_
unsigned int
Response_ShowUpdate(
	_In_ const IfxUpdate* PpTpmUpdate);

/**
 *	@brief		Show TPM1.2 ClearOwnership output
 *	@details	Format TPM1.2 ClearOwnership output and display
 *
 *	@param		PpTpm12ClearOwnership	Pointer to a IfxTpm12ClearOwnership response structure
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PpTpm12ClearOwnership was invalid.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
Response_ShowClearOwnership(
	_In_	const IfxTpm12ClearOwnership* PpTpm12ClearOwnership);

/**
 *	@brief		Show Unknown Action info
 *	@details	Displays the output for an unknown action to the console
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		...						Error Codes from ConsoleIO_Write function
 */
_Check_return_
unsigned int
Response_ShowUnknownAction();

/**
 *	@brief		Show Error output
 *	@details	Formats the error output and display it to the user
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from ConsoleIO_Write functions
 */
_Check_return_
unsigned int
Response_ShowError();

/**
 *	@brief		Show TPM explanation to a TPM Error
 *	@details
 *
 *	@param		PunInternalCode			TPM masked error code
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 */
_Check_return_
unsigned int
Response_ShowTpmErrorExplanation(
	_In_	unsigned int PunInternalCode);

/**
 *	@brief		Print the help information
 *	@details
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		RC_E_FAIL		An unexpected error occurred.
 *	@retval		...				Error codes from ConsoleIO_Write functions
 */
_Check_return_
unsigned int
Response_ShowHelp();

/**
 *	@brief		Print the TPMFactoryUpd tool header
 *	@details
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from ConsoleIO_Write function call
 */
_Check_return_
unsigned int
Response_ShowHeader();

/**
 *	@brief		Returns whether the header has been shown
 *	@details
 *
 *	@retval		TRUE		Header has been shown.
 *	@retval		FALSE		Header has not been shown.
 */
_Check_return_
BOOL
Response_HeaderShown();

/**
 *	@brief		Callback function for progress report of EFI_FIRMWARE_MANAGEMENT_PROTOCOL.SetImage().
 *	@details	The function is called by EFI_FIRMWARE_MANAGEMENT_PROTOCOL.SetImage() to update the progress (1 - 100). It prints the
 *				progress to the console.
 *
 *	@param		PullCompletion	Progress completion value between 1 and 100
 *
 *	@retval		0				The callback executed successfully.
 */
_Check_return_
extern unsigned long long
Response_ProgressCallback(
	_In_ unsigned long long PullCompletion);

// Function pointer type definition for Response_ProgressCallback
typedef unsigned long long (*PFN_RESPONSE_PROGRESSCALLBACK)(
	unsigned long long PullCompletion);

#ifdef __cplusplus
}
#endif
