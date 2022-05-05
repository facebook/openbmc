/**
 *	@brief		Implements the error handling module.
 *	@details	This module stores information about the error occurred and maps
 *				errors to outgoing error codes and messages for logging and displaying.
 *	@file		Error.c
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

#include "Error.h"
#include "Logging.h"
#include "Platform.h"
#include "TpmResponse.h"

/// Pointer to a IfxErrorData structure to store error parameters
IfxErrorData* s_pErrorData = NULL;

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Private functions

/**
 *	@brief		Function to clear the error stack
 *	@details	This function clears all elements on the error stack.
 */
void
Error_ClearStackInternal(
	_In_ IfxErrorData* PpErrorData)
{
	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	// Check if parameter is not null
	if (NULL != PpErrorData)
	{
		// Check if a child structure is stored
		if (NULL != PpErrorData->pPreviousError)
		{
			// Call recursive to clear child structures first
			Error_ClearStackInternal((IfxErrorData*)PpErrorData->pPreviousError);
			PpErrorData->pPreviousError = NULL;
		}

		// Now clear the structure
		Platform_MemoryFree((void**)&PpErrorData);
	}

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);
}

/**
 *	@brief		Maps the ErrorCode to the final one
 *	@details	This function maps the internal error code to the final one displayed to the end user.
 *
 *	@returns	The mapped final error code
 */
_Check_return_
unsigned int
Error_MapToFinalCode(
	_In_ unsigned int PunErrorCode)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	switch (PunErrorCode)
	{
		// Error codes as they are
		case RC_E_FAIL:
		case RC_E_BAD_COMMANDLINE:
		case RC_E_TPM_GENERAL:
		case RC_E_INTERNAL:
		case RC_E_NO_TPM:
		case RC_E_NO_IFXTPM20:
		case RC_E_FLAGCHECK:
		case RC_E_EKCHECK:
		case RC_E_HASHCHECK:
		case RC_E_REGISTERTEST:
		case RC_E_WRONG_ENCODING:
		case RC_E_INTERRUPTED_FU:
		case RC_E_TPM_FIRMWARE_UPDATE:
		case RC_E_INVALID_FW_OPTION:
		case RC_E_WRONG_FW_IMAGE:
		case RC_E_INVALID_LOG_OPTION:
		case RC_E_NO_IFX_TPM:
		case RC_E_PLATFORM_AUTH_NOT_EMPTY:
		case RC_E_PLATFORM_HIERARCHY_DISABLED:
		case RC_E_FW_UPDATE_BLOCKED:
		case RC_E_FIRMWARE_UPDATE_FAILED:
		case RC_E_TPM12_OWNED:
		case RC_E_TPM12_PP_LOCKED:
		case RC_E_TPM12_NO_OWNER:
		case RC_E_TPM12_INVALID_OWNERAUTH:
		case RC_E_INVALID_UPDATE_OPTION:
		case RC_E_RESTART_REQUIRED:
		case RC_E_TPM12_DEFERREDPP_REQUIRED:
		case RC_E_TPM12_DISABLED_DEACTIVATED:
		case RC_E_TPM12_DA_ACTIVE:
		case RC_E_NEWER_TOOL_REQUIRED:
		case RC_E_UNSUPPORTED_CHIP:
		case RC_E_CORRUPT_FW_IMAGE:
		case RC_E_WRONG_DECRYPT_KEYS:
		case RC_E_NOT_SUPPORTED_FEATURE:
		case RC_E_DEVICE_ALREADY_IN_USE:
		case RC_E_TPM_ACCESS_DENIED:
		case RC_E_INVALID_ACCESS_MODE:
		case RC_E_INVALID_SETTING:
		case RC_E_TPM_NOT_SUPPORTED_FEATURE:
		case RC_E_TPM20_FAILURE_MODE:
		case RC_E_INVALID_CONFIG_OPTION:
		case RC_E_FIRMWARE_UPDATE_NOT_FOUND:
		case RC_E_RESUME_RUNDATA_NOT_FOUND:
		case RC_E_TPM12_FAILED_SELFTEST:
		case RC_E_INVALID_OWNERAUTH_OPTION:
			unReturnValue = PunErrorCode;
			break;

		// Error codes mapped to RC_E_INTERNAL
		case RC_E_NOT_INITIALIZED:
		case RC_E_NOT_CONNECTED:
		case RC_E_ALREADY_CONNECTED:
		case RC_E_BAD_PARAMETER:
		case RC_E_BUFFER_TOO_SMALL:
		case RC_E_END_OF_FILE:
		case RC_E_FILE_NOT_FOUND:
		case RC_E_ACCESS_DENIED:
		case RC_E_FILE_EXISTS:
		case RC_E_END_OF_STRING:
			unReturnValue = RC_E_INTERNAL;
			break;

		// Error codes mapped to RC_E_NO_TPM
		case RC_E_COMPONENT_NOT_FOUND:
		case RC_E_LOCALITY_NOT_ACTIVE:
		case RC_E_LOCALITY_NOT_SUPPORTED:
		case RC_E_TPM_NO_DATA_AVAILABLE:
		case RC_E_INSUFFICIENT_BUFFER:
		case RC_E_TPM_RECEIVE_DATA:
		case RC_E_TPM_TRANSMIT_DATA:
		case RC_E_NOT_READY:
			unReturnValue = RC_E_NO_TPM;
			break;

		// Error codes mapped to RC_E_TPM_FIRMWARE_UPDATE
		case RC_E_TPM20_INVALID_POLICY_SESSION:
		case RC_E_TPM20_POLICY_HANDLE_OUT_OF_RANGE:
		case RC_E_TPM20_POLICY_SESSION_NOT_LOADED:
		case RC_E_TPM12_MISSING_OWNERAUTH:
		case RC_E_TPM_NO_BOOT_LOADER_MODE:
			unReturnValue = RC_E_TPM_FIRMWARE_UPDATE;
			break;

		// All not handled codes are mapped to RC_E_FAIL
		default:
			// Check if it is a TPM error code
			if ((PunErrorCode & 0xFFFF0000) == RC_TPM_MASK)
				unReturnValue = RC_E_TPM_GENERAL;
			else
				unReturnValue = RC_E_FAIL;
			break;
	}

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Maps the final error code to a message
 *	@details	This function maps the final error code to the corresponding message.
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Error_MapFinalCodeToMessage(
	_In_							unsigned int	PunErrorCode,
	_Out_z_cap_(*PpunBufferSize)	wchar_t*		PwszErrorMessage,
	_Inout_							unsigned int*	PpunBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check out parameter
		if (NULL == PwszErrorMessage)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			LOGGING_WRITE_LEVEL4(L"PwszErrorMessage not initialized correctly");
			break;
		}

		PwszErrorMessage[0] = L'\0';

		// Check in out parameter
		if (NULL == PpunBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			LOGGING_WRITE_LEVEL4(L"PpunBufferSize not initialized correctly");
			break;
		}

		switch (PunErrorCode)
		{
			case RC_E_BAD_COMMANDLINE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_BAD_COMMANDLINE);
				break;
			case RC_E_TPM_GENERAL:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM_GENERAL);
				break;
			case RC_E_INTERNAL:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_INTERNAL);
				break;
			case RC_E_NO_TPM:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_NO_TPM);
				break;
			case RC_E_NO_IFXTPM20:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_NO_IFXTPM20);
				break;
			case RC_E_FLAGCHECK:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_FLAGCHECK);
				break;
			case RC_E_EKCHECK:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_EKCHECK);
				break;
			case RC_E_HASHCHECK:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_HASHCHECK);
				break;
			case RC_E_REGISTERTEST:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_REGISTERTEST);
				break;
			case RC_E_WRONG_ENCODING:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_WRONG_ENCODING);
				break;
			case RC_E_INTERRUPTED_FU:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_INTERRUPTED_FU);
				break;
			case RC_E_TPM_FIRMWARE_UPDATE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM_FIRMWARE_UPDATE);
				break;
			case RC_E_INVALID_FW_OPTION:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_INVALID_FW_OPTION);
				break;
			case RC_E_WRONG_FW_IMAGE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_WRONG_FW_IMAGE);
				break;
			case RC_E_INVALID_LOG_OPTION:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_INVALID_LOG_OPTION);
				break;
			case RC_E_NO_IFX_TPM:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_NO_IFX_TPM);
				break;
			case RC_E_PLATFORM_AUTH_NOT_EMPTY:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_PLATFORM_AUTH_NOT_EMPTY);
				break;
			case RC_E_PLATFORM_HIERARCHY_DISABLED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_PLATFORM_HIERARCHY_DISABLED);
				break;
			case RC_E_FW_UPDATE_BLOCKED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_FW_UPDATE_BLOCKED);
				break;
			case RC_E_FIRMWARE_UPDATE_FAILED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_FIRMWARE_UPDATE_FAILED);
				break;
			case RC_E_TPM12_OWNED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM12_OWNED);
				break;
			case RC_E_TPM12_NO_OWNER:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM12_NO_OWNER);
				break;
			case RC_E_TPM12_INVALID_OWNERAUTH:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM12_INVALID_OWNERAUTH);
				break;
			case RC_E_TPM12_PP_LOCKED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM12_PP_LOCKED);
				break;
			case RC_E_INVALID_UPDATE_OPTION:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_INVALID_UPDATE_OPTION);
				break;
			case RC_E_RESTART_REQUIRED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_RESTART_REQUIRED);
				break;
			case RC_E_TPM12_DEFERREDPP_REQUIRED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM12_DEFERREDPP_REQUIRED);
				break;
			case RC_E_TPM12_DISABLED_DEACTIVATED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM12_DISABLED_DEACTIVATED);
				break;
			case RC_E_TPM12_DA_ACTIVE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM12_DA_ACTIVE);
				break;
			case RC_E_NEWER_TOOL_REQUIRED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_NEWER_TOOL_REQUIRED);
				break;
			case RC_E_UNSUPPORTED_CHIP:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_UNSUPPORTED_CHIP);
				break;
			case RC_E_CORRUPT_FW_IMAGE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_CORRUPT_FW_IMAGE);
				break;
			case RC_E_WRONG_DECRYPT_KEYS:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_WRONG_DECRYPT_KEYS);
				break;
			case RC_E_NOT_SUPPORTED_FEATURE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_NOT_SUPPORTED_FEATURE);
				break;
			case RC_E_DEVICE_ALREADY_IN_USE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_DEVICE_ALREADY_IN_USE);
				break;
			case RC_E_TPM_ACCESS_DENIED:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM_ACCESS_DENIED);
				break;
			case RC_E_INVALID_ACCESS_MODE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_INVALID_ACCESS_MODE);
				break;
			case RC_E_INVALID_SETTING:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_INVALID_SETTING);
				break;
			case RC_E_TPM_NOT_SUPPORTED_FEATURE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM_NOT_SUPPORTED_FEATURE);
				break;
			case RC_E_TPM20_FAILURE_MODE:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM20_FAILURE_MODE);
				break;
			case RC_E_INVALID_CONFIG_OPTION:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_INVALID_CONFIG_OPTION);
				break;
			case RC_E_FIRMWARE_UPDATE_NOT_FOUND:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_FIRMWARE_UPDATE_NOT_FOUND);
				break;
			case RC_E_RESUME_RUNDATA_NOT_FOUND:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_RESUME_RUNDATA_NOT_FOUND);
				break;
			case RC_E_TPM12_FAILED_SELFTEST:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_TPM12_FAILED_SELFTEST);
				break;
			case RC_E_INVALID_OWNERAUTH_OPTION:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_INVALID_OWNERAUTH_OPTION);
				break;
			default:
				unReturnValue = Platform_StringCopy(PwszErrorMessage, PpunBufferSize, MSG_RC_E_FAIL);
				break;
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Public functions

/**
 *	@brief		Function to return the error stack
 *	@details	This function returns the first element in the error stack.
 *
 *	@returns	Returns the pointer to the first element in the error stack
 */
_Check_return_
IfxErrorData*
Error_GetStack()
{
	return s_pErrorData;
}

/**
 *	@brief		Function to clear the error stack
 *	@details	This function clears all elements on the error stack.
 */
void
Error_ClearStack()
{
	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	Error_ClearStackInternal(s_pErrorData);
	s_pErrorData = NULL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);
}

/**
 *	@brief		Function to clear the first item in the error stack
 *	@details	This function removes the first item of the error stack.
 */
void
Error_ClearFirstItem()
{
	IfxErrorData* pErrorData = NULL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	// Get error Stack
	pErrorData = Error_GetStack();

	// Check if element is not null
	if (NULL != pErrorData)
	{
		// Remove first item from the list
		s_pErrorData = (IfxErrorData*)pErrorData->pPreviousError;

		// Free memory of the first removed item
		Platform_MemoryFree((void**)&pErrorData);
	}

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);
}

/**
 *	@brief		Function to store all parameters to an IfxErrorData structure
 *	@details	This function stores an error and its specific parameters for later use.
 *
 *	@param		PszOccurredInModule			Pointer to a char array holding the module name where the error occurred
 *	@param		PszOccurredInFunction		Pointer to a char array holding the function name where the error occurred
 *	@param		PnOccurredInLine			The line where the error occurred
 *	@param		PunInternalErrorCode		Internal error code
 *	@param		PwszInternalErrorMessage	Format string used to format the internal error message
 *	@param		PvaArgumentList				Parameters needed to format the error message
 */
_Check_return_
IfxErrorData*
Error_GetErrorData(
	_In_z_	const char*		PszOccurredInModule,
	_In_z_	const char*		PszOccurredInFunction,
	_In_	int				PnOccurredInLine,
	_In_	unsigned int	PunInternalErrorCode,
	_In_z_	const wchar_t*	PwszInternalErrorMessage,
	_In_	va_list			PvaArgumentList)
{
	IfxErrorData* pErrorData = NULL;
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		unsigned int unSize = 0;

		// Allocate new error structure
		pErrorData = (IfxErrorData*)Platform_MemoryAllocateZero(sizeof(IfxErrorData));
		if (NULL == pErrorData)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Store Internal error code
		pErrorData->unInternalErrorCode = PunInternalErrorCode;

		// Store Line number
		pErrorData->nOccurredInLine = PnOccurredInLine;

		// Store Internal error message
		if (!PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszInternalErrorMessage))
		{
			unSize = RG_LEN(pErrorData->wszInternalErrorMessage);
			unReturnValue = Platform_StringFormatV(pErrorData->wszInternalErrorMessage, &unSize, PwszInternalErrorMessage, PvaArgumentList);
			if (RC_SUCCESS != unReturnValue)
				break;
		}

		{
			wchar_t wszOccurredInModule[256] = {0};
			wchar_t wszOccurredInFunction[256] = {0};
			if (RC_E_BUFFER_TOO_SMALL == Platform_AnsiString2UnicodeString(wszOccurredInModule, RG_LEN(wszOccurredInModule), PszOccurredInModule))
			{
				unReturnValue = RC_E_BUFFER_TOO_SMALL;
				break;
			}

			// Store Internal error message
			if (!PLATFORM_STRING_IS_NULL_OR_EMPTY(wszOccurredInModule))
			{
				// Store Module name
				unSize = RG_LEN(pErrorData->wszOccurredInModule);
				unReturnValue = Platform_StringCopy(pErrorData->wszOccurredInModule, &unSize, wszOccurredInModule);
				if (RC_SUCCESS != unReturnValue)
					break;
			}

			if (RC_E_BUFFER_TOO_SMALL == Platform_AnsiString2UnicodeString(wszOccurredInFunction, RG_LEN(wszOccurredInFunction), PszOccurredInFunction))
			{
				unReturnValue = RC_E_BUFFER_TOO_SMALL;
				break;
			}

			// Store Function name
			if (!PLATFORM_STRING_IS_NULL_OR_EMPTY(wszOccurredInFunction))
			{
				unSize = RG_LEN(pErrorData->wszOccurredInFunction);
				unReturnValue = Platform_StringCopy(pErrorData->wszOccurredInFunction, &unSize, wszOccurredInFunction);
				if (RC_SUCCESS != unReturnValue)
					break;
			}
		}
	}
	WHILE_FALSE_END;

	if (RC_SUCCESS != unReturnValue)
	{
		ERROR_STORE_FMT(unReturnValue, L"Unexpected error while storing an error. (0x%.8X)", PunInternalErrorCode);
		// Free allocated memory
		Platform_MemoryFree((void**)&pErrorData);
	}

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);

	return pErrorData;
}

/**
 *	@brief		Function to store an error
 *	@details	This function stores an error and its specific parameters for later use.
 *				The error element is stored as the first element of the error list (stack).
 *
 *	@param		PszOccurredInModule			Pointer to a char array holding the module name where the error occurred
 *	@param		PszOccurredInFunction		Pointer to a char array holding the function name where the error occurred
 *	@param		PnOccurredInLine			The line where the error occurred
 *	@param		PunInternalErrorCode		Internal error code
 *	@param		PwszInternalErrorMessage	Format string used to format the internal error message
 *	@param		...							Parameters needed to format the error message
 */
void
Error_Store(
	_In_z_	const char*		PszOccurredInModule,
	_In_z_	const char*		PszOccurredInFunction,
	_In_	int				PnOccurredInLine,
	_In_	unsigned int	PunInternalErrorCode,
	_In_z_	const wchar_t*	PwszInternalErrorMessage,
	...)
{
	IfxErrorData* pErrorData = NULL;
	va_list vaArgumentList;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	va_start(vaArgumentList, PwszInternalErrorMessage);
	pErrorData = Error_GetErrorData(
					 PszOccurredInModule, PszOccurredInFunction, PnOccurredInLine,
					 PunInternalErrorCode, PwszInternalErrorMessage, vaArgumentList);
	va_end(vaArgumentList);

	// Put the error in front of the list
	if (pErrorData != NULL)
	{
		if (NULL != s_pErrorData)
			pErrorData->pPreviousError = s_pErrorData;
		s_pErrorData = pErrorData;
	}

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);
}

/**
 *	@brief		Return the final error code
 *	@details	This function maps the given error code to the final one.
 *
 *	@retval		The final mapped error code
 */
_Check_return_
unsigned int
Error_GetFinalCodeFromError(
	_In_ unsigned int PunErrorCode)
{
	unsigned int unFinalErrorCode = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	unFinalErrorCode = Error_MapToFinalCode(PunErrorCode);

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);

	return unFinalErrorCode;
}

/**
 *	@brief		Return the final error code
 *	@details	This function maps the internal error code to the final one displayed to the end user.
 *
 *	@retval		The final mapped error code
 */
_Check_return_
unsigned int
Error_GetFinalCode()
{
	unsigned int unFinalErrorCode = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	if (NULL == s_pErrorData)
		unFinalErrorCode = RC_SUCCESS;
	else
		unFinalErrorCode = Error_GetFinalCodeFromError(s_pErrorData->unInternalErrorCode);

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);

	return unFinalErrorCode;
}

/**
 *	@brief		Return the final error code
 *	@details	This function maps the internal error code to the final one displayed to the user.
 *
 *	@param		PunErrorCode		Error Code to map to the final message
 *	@param		PwszErrorMessage	Pointer to a char buffer to copy the error message to
 *	@param		PpunBufferSize		Size of the error message buffer
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. E.g. PwszErrorMessage == NULL
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Error_GetFinalMessageFromErrorCode(
	_In_							unsigned int	PunErrorCode,
	_Out_z_cap_(*PpunBufferSize)	wchar_t*		PwszErrorMessage,
	_Inout_							unsigned int*	PpunBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		unsigned int unFinalErrorCode = RC_E_FAIL;

		// Check out parameter
		if (NULL == PwszErrorMessage)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			LOGGING_WRITE_LEVEL4(L"PwszErrorMessage not initialized correctly");
			break;
		}

		PwszErrorMessage[0] = L'\0';

		// Check in/out parameter
		if (NULL == PpunBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			LOGGING_WRITE_LEVEL4(L"PpunBufferSize not initialized correctly");
			break;
		}

		// Map to final error code
		unFinalErrorCode = Error_MapToFinalCode(PunErrorCode);

		// Get final error message to final error code
		unReturnValue = Error_MapFinalCodeToMessage(unFinalErrorCode, PwszErrorMessage, PpunBufferSize);
		if (RC_SUCCESS != unReturnValue)
		{
			*PpunBufferSize = 0;
			PwszErrorMessage[0] = L'\0';
			break;
		}

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Return the final error code
 *	@details	This function maps the internal error code to the final one displayed to the user.
 *
 *	@param		PwszErrorMessage	Pointer to a char buffer to copy the error message to
 *	@param		PpunBufferSize		Size of the error message buffer
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. E.g. PwszErrorMessage == NULL
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Error_GetFinalMessage(
	_Out_z_cap_(*PpunBufferSize)	wchar_t*		PwszErrorMessage,
	_Inout_							unsigned int*	PpunBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check out parameter
		if (NULL == PwszErrorMessage)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			LOGGING_WRITE_LEVEL4(L"PwszErrorMessage not initialized correctly");
			break;
		}

		PwszErrorMessage[0] = L'\0';

		// Check in/out parameter
		if (NULL == PpunBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			LOGGING_WRITE_LEVEL4(L"PpunBufferSize not initialized correctly");
			break;
		}

		// Check if an error is stored
		if (NULL == s_pErrorData)
		{
			*PpunBufferSize = 0;
			PwszErrorMessage[0] = L'\0';
			unReturnValue = RC_SUCCESS;
			break;
		}

		// Get final error message to final error code
		unReturnValue = Error_GetFinalMessageFromErrorCode(s_pErrorData->unInternalErrorCode, PwszErrorMessage, PpunBufferSize);
		if (RC_SUCCESS != unReturnValue)
		{
			*PpunBufferSize = 0;
			PwszErrorMessage[0] = L'\0';
			break;
		}

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Return the internal error code
 *	@details	This function returns the internal error code.
 *
 *	@retval		The internal error code
 */
_Check_return_
unsigned int
Error_GetInternalCode()
{
	unsigned int unInternalErrorCode = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	if (NULL == s_pErrorData)
		unInternalErrorCode = RC_SUCCESS;
	else
		unInternalErrorCode = s_pErrorData->unInternalErrorCode;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);

	return unInternalErrorCode;
}

/**
 *	@brief		Log the error stack
 *	@details	This function logs the whole error stack.
 */
void
Error_LogStack()
{
	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	Error_LogErrorData(s_pErrorData);

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);
}

/**
 *	@brief		Log error code and message
 *	@details	This function logs an error code and message.
 *
 *	@param		PszOccurredInModule			Pointer to a char array holding the module name where the error occurred
 *	@param		PszOccurredInFunction		Pointer to a char array holding the function name where the error occurred
 *	@param		PnOccurredInLine			The line where the error occurred
 *	@param		PunInternalErrorCode		Internal error code
 *	@param		PwszInternalErrorMessage	Format string used to format the internal error message
 *	@param		...							Parameters needed to format the error message
 */
void
Error_LogCodeAndMessage(
	_In_z_	const char*		PszOccurredInModule,
	_In_z_	const char*		PszOccurredInFunction,
	_In_	int				PnOccurredInLine,
	_In_	unsigned int	PunInternalErrorCode,
	_In_z_	const wchar_t*	PwszInternalErrorMessage,
	...)
{
	va_list vaArgumentList;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	va_start(vaArgumentList, PwszInternalErrorMessage);
	Error_LogErrorData(Error_GetErrorData(
						   PszOccurredInModule, PszOccurredInFunction, PnOccurredInLine,
						   PunInternalErrorCode, PwszInternalErrorMessage, vaArgumentList));
	va_end(vaArgumentList);

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);
}

/**
 *	@brief		Log IfxErrorData and linked IfxErrorData
 *	@details	This function logs an IfxErrorData object and all linked IfxErrorData objects.
 */
void
Error_LogErrorData(
	_In_	const IfxErrorData*	PpErrorData)
{
	unsigned int	unReturnValue = RC_E_FAIL;
	wchar_t			wszMessage[MAX_MESSAGE_SIZE] = {0};
	unsigned int	unMessageSize = RG_LEN(wszMessage);

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	if (PpErrorData != NULL)
	{
		unReturnValue = Error_GetFinalMessageFromErrorCode(PpErrorData->unInternalErrorCode, wszMessage, &unMessageSize);
		if (RC_SUCCESS != unReturnValue)
			LOGGING_WRITE_LEVEL1_FMT(L"Error occurred during Error_GetFinalMessageFromErrorCode. (0x%.8X)", unReturnValue);
		LOGGING_WRITE_LEVEL1(L"Error detected:");
		LOGGING_WRITE_LEVEL1_FMT(L"Final code: 0x%.8X", Error_GetFinalCodeFromError(PpErrorData->unInternalErrorCode));
		LOGGING_WRITE_LEVEL1_FMT(L"Final message: %ls", wszMessage);
		do
		{
			LOGGING_WRITE_LEVEL1_FMT(L"    Module: %ls; Function: %ls; Line: %d", PpErrorData->wszOccurredInModule, PpErrorData->wszOccurredInFunction, PpErrorData->nOccurredInLine);
			LOGGING_WRITE_LEVEL1_FMT(L"    Code: 0x%.8X", PpErrorData->unInternalErrorCode);
			LOGGING_WRITE_LEVEL1_FMT(L"    Message: %ls", PpErrorData->wszInternalErrorMessage);
			if ((PpErrorData->unInternalErrorCode & 0xFFFF0000) == RC_TPM_MASK)
			{
				unReturnValue = Platform_StringSetZero(wszMessage, MAX_MESSAGE_SIZE);
				if (RC_SUCCESS != unReturnValue)
					LOGGING_WRITE_LEVEL1_FMT(L"Initialization of a buffer failed. (0x%.8X)", unReturnValue);
				unReturnValue = TpmResponse_GetMessage(PpErrorData->unInternalErrorCode, wszMessage, MAX_MESSAGE_SIZE);
				if (RC_SUCCESS != unReturnValue)
					LOGGING_WRITE_LEVEL1_FMT(L"Error occurred during TpmResponse_GetMessage. (0x%.8X)", unReturnValue);
				if (0 != Platform_StringCompare(wszMessage, L"", 1, FALSE))
					LOGGING_WRITE_LEVEL1_FMT(L"    Reason: %ls", wszMessage);
			}
			PpErrorData = (IfxErrorData*)PpErrorData->pPreviousError;
		}
		while (PpErrorData != NULL);
	}

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_EXIT_STRING);
}

//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
