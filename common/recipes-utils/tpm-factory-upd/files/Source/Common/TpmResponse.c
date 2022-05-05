/**
 *	@brief		Implements the TPM response message parser
 *	@details	This module provides function and constant declarations for parsing TPM1.2
 *				and TPM2.0 response codes and obtaining a corresponding error message.
 *	@file		TpmResponse.c
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

#include "StdInclude.h"
#include "Platform.h"
#include "TpmResponse.h"

/// A list of all TPM2.0 Format-Zero error messages with their response code offset as index
const wchar_t* s_rgwszFormatZeroErrorMessages[] =	// RC_VER1 (0x100) + offset
{
	TPM_RC_INITIALIZE_MESSAGE,			// 0x000
	TPM_RC_FAILURE_MESSAGE,				// 0x001
	NULL,								// 0x002
	TPM_RC_SEQUENCE_MESSAGE,			// 0x003
	NULL,								// 0x004
	NULL,								// 0x005
	NULL,								// 0x006
	NULL,								// 0x007
	NULL,								// 0x008
	NULL,								// 0x009
	NULL,								// 0x00A
	TPM_RC_PRIVATE_MESSAGE,				// 0x00B
	NULL,								// 0x00C
	NULL,								// 0x00D
	NULL,								// 0x00E
	NULL,								// 0x00F
	NULL,								// 0x010
	NULL,								// 0x011
	NULL,								// 0x012
	NULL,								// 0x013
	NULL,								// 0x014
	NULL,								// 0x015
	NULL,								// 0x016
	NULL,								// 0x017
	NULL,								// 0x018
	TPM_RC_HMAC_MESSAGE,				// 0x019
	NULL,								// 0x01A
	NULL,								// 0x01B
	NULL,								// 0x01C
	NULL,								// 0x01D
	NULL,								// 0x01E
	NULL,								// 0x01F
	TPM_RC_DISABLED_MESSAGE,			// 0x020
	TPM_RC_EXCLUSIVE_MESSAGE,			// 0x021
	NULL,								// 0x022
	NULL,								// 0x023
	TPM_RC_AUTH_TYPE_MESSAGE,			// 0x024
	TPM_RC_AUTH_MISSING_MESSAGE,		// 0x025
	TPM_RC_POLICY_MESSAGE,				// 0x026
	TPM_RC_PCR_MESSAGE,					// 0x027
	TPM_RC_PCR_CHANGED_MESSAGE,			// 0x028
	NULL,								// 0x029
	NULL,								// 0x02A
	NULL,								// 0x02B
	NULL,								// 0x02C
	TPM_RC_UPGRADE_MESSAGE,				// 0x02D
	TPM_RC_TOO_MANY_CONTEXTS_MESSAGE,	// 0x02E
	TPM_RC_AUTH_UNAVAILABLE_MESSAGE,	// 0x02F
	TPM_RC_REBOOT_MESSAGE,				// 0x030
	TPM_RC_UNBALANCED_MESSAGE,			// 0x031
	NULL,								// 0x032
	NULL,								// 0x033
	NULL,								// 0x034
	NULL,								// 0x035
	NULL,								// 0x036
	NULL,								// 0x037
	NULL,								// 0x038
	NULL,								// 0x039
	NULL,								// 0x03A
	NULL,								// 0x03B
	NULL,								// 0x03C
	NULL,								// 0x03D
	NULL,								// 0x03E
	NULL,								// 0x03F
	NULL,								// 0x040
	NULL,								// 0x041
	TPM_RC_COMMAND_SIZE_MESSAGE,		// 0x042
	TPM_RC_COMMAND_CODE_MESSAGE,		// 0x043
	TPM_RC_AUTHSIZE_MESSAGE,			// 0x044
	TPM_RC_AUTH_CONTEXT_MESSAGE,		// 0x045
	TPM_RC_NV_RANGE_MESSAGE,			// 0x046
	TPM_RC_NV_SIZE_MESSAGE,				// 0x047
	TPM_RC_NV_LOCKED_MESSAGE,			// 0x048
	TPM_RC_NV_AUTHORIZATION_MESSAGE,	// 0x049
	TPM_RC_NV_UNINITIALIZED_MESSAGE,	// 0x04A
	TPM_RC_NV_SPACE_MESSAGE,			// 0x04B
	TPM_RC_NV_DEFINED_MESSAGE,			// 0x04C
	NULL,								// 0x04D
	NULL,								// 0x04E
	NULL,								// 0x04F
	TPM_RC_BAD_CONTEXT_MESSAGE,			// 0x050
	TPM_RC_CPHASH_MESSAGE,				// 0x051
	TPM_RC_PARENT_MESSAGE,				// 0x052
	TPM_RC_NEEDS_TEST_MESSAGE,			// 0x053
	TPM_RC_NO_RESULT_MESSAGE,			// 0x054
	TPM_RC_SENSITIVE_MESSAGE			// 0x055
};

/// A list of all TPM2.0 Format-One error messages with their response code offset as index
const wchar_t* s_rgwszFormatOneErrorMessages[] =	// RC_FMT1 (0x80) + offset
{
	NULL,								// 0x000
	TPM_RC_ASYMMETRIC_MESSAGE,			// 0x001
	TPM_RC_ATTRIBUTES_MESSAGE,			// 0x002
	TPM_RC_HASH_MESSAGE,				// 0x003
	TPM_RC_VALUE_MESSAGE,				// 0x004
	TPM_RC_HIERARCHY_MESSAGE,			// 0x005
	NULL,								// 0x006
	TPM_RC_KEY_SIZE_MESSAGE,			// 0x007
	TPM_RC_MGF_MESSAGE,					// 0x008
	TPM_RC_MODE_MESSAGE,				// 0x009
	TPM_RC_TYPE_MESSAGE,				// 0x00A
	TPM_RC_HANDLE_MESSAGE,				// 0x00B
	TPM_RC_KDF_MESSAGE,					// 0x00C
	TPM_RC_RANGE_MESSAGE,				// 0x00D
	TPM_RC_AUTH_FAIL_MESSAGE,			// 0x00E
	TPM_RC_NONCE_MESSAGE,				// 0x00F
	TPM_RC_PP_MESSAGE,					// 0x010
	NULL,								// 0x011
	TPM_RC_SCHEME_MESSAGE,				// 0x012
	NULL,								// 0x013
	NULL,								// 0x014
	TPM_RC_SIZE_MESSAGE,				// 0x015
	TPM_RC_SYMMETRIC_MESSAGE,			// 0x016
	TPM_RC_TAG_MESSAGE,					// 0x017
	TPM_RC_SELECTOR_MESSAGE,			// 0x018
	NULL,								// 0x019
	TPM_RC_INSUFFICIENT_MESSAGE,		// 0x01A
	TPM_RC_SIGNATURE_MESSAGE,			// 0x01B
	TPM_RC_KEY_MESSAGE,					// 0x01C
	TPM_RC_POLICY_FAIL_MESSAGE,			// 0x01D
	NULL,								// 0x01E
	TPM_RC_INTEGRITY_MESSAGE,			// 0x01F
	TPM_RC_TICKET_MESSAGE,				// 0x020
	TPM_RC_RESERVED_BITS_MESSAGE,		// 0x021
	TPM_RC_BAD_AUTH_MESSAGE,			// 0x022
	TPM_RC_EXPIRED_MESSAGE,				// 0x023
	TPM_RC_POLICY_CC_MESSAGE,			// 0x024
	TPM_RC_BINDING_MESSAGE,				// 0x025
	TPM_RC_CURVE_MESSAGE,				// 0x026
	TPM_RC_ECC_POINT_MESSAGE			// 0x027
};

/// A list of all TPM2.0 Format-Zero warning messages with their response code offset as index
const wchar_t* s_rgwszFormatZeroWarningMessages[] =	// RC_WARN (0x900) + offset
{
	NULL,								// 0x000
	TPM_RC_CONTEXT_GAP_MESSAGE,			// 0x001
	TPM_RC_OBJECT_MEMORY_MESSAGE,		// 0x002
	TPM_RC_SESSION_MEMORY_MESSAGE,		// 0x003
	TPM_RC_MEMORY_MESSAGE,				// 0x004
	TPM_RC_SESSION_HANDLES_MESSAGE,		// 0x005
	TPM_RC_OBJECT_HANDLES_MESSAGE,		// 0x006
	TPM_RC_LOCALITY_MESSAGE,			// 0x007
	TPM_RC_YIELDED_MESSAGE,				// 0x008
	TPM_RC_CANCELED_MESSAGE,			// 0x009
	TPM_RC_TESTING_MESSAGE,				// 0x00A
	NULL,								// 0x00B
	NULL,								// 0x00C
	NULL,								// 0x00D
	NULL,								// 0x00E
	NULL,								// 0x00F
	TPM_RC_REFERENCE_H0_MESSAGE,		// 0x010
	TPM_RC_REFERENCE_H1_MESSAGE,		// 0x011
	TPM_RC_REFERENCE_H2_MESSAGE,		// 0x012
	TPM_RC_REFERENCE_H3_MESSAGE,		// 0x013
	TPM_RC_REFERENCE_H4_MESSAGE,		// 0x014
	TPM_RC_REFERENCE_H5_MESSAGE,		// 0x015
	TPM_RC_REFERENCE_H6_MESSAGE,		// 0x016
	NULL,								// 0x017
	TPM_RC_REFERENCE_S0_MESSAGE,		// 0x018
	TPM_RC_REFERENCE_S1_MESSAGE,		// 0x019
	TPM_RC_REFERENCE_S2_MESSAGE,		// 0x01A
	TPM_RC_REFERENCE_S3_MESSAGE,		// 0x01B
	TPM_RC_REFERENCE_S4_MESSAGE,		// 0x01C
	TPM_RC_REFERENCE_S5_MESSAGE,		// 0x01D
	TPM_RC_REFERENCE_S6_MESSAGE,		// 0x01E
	NULL,								// 0x01F
	TPM_RC_NV_RATE_MESSAGE,				// 0x020
	TPM_RC_LOCKOUT_MESSAGE,				// 0x021
	TPM_RC_RETRY_MESSAGE,				// 0x022
	TPM_RC_NV_UNAVAILABLE_MESSAGE		// 0x023
};

/// A list of all TPM1.2 error messages with their response code offset as index
const wchar_t* s_rgwszTpm12ErrorMessages[] =	// TPM_BASE (0x0) + offset
{
	TPM_SUCCESS_MESSAGE,				// 0x000
	TPM_AUTHFAIL_MESSAGE,				// 0x001
	TPM_BADINDEX_MESSAGE,				// 0x002
	TPM_BAD_PARAMETER_MESSAGE,			// 0x003
	TPM_AUDITFAILURE_MESSAGE,			// 0x004
	TPM_CLEAR_DISABLED_MESSAGE,			// 0x005
	TPM_DEACTIVATED_MESSAGE,			// 0x006
	TPM_DISABLED_MESSAGE,				// 0x007
	TPM_DISABLED_CMD_MESSAGE,			// 0x008
	TPM_FAIL_MESSAGE,					// 0x009
	TPM_BAD_ORDINAL_MESSAGE,			// 0x00A
	TPM_INSTALL_DISABLED_MESSAGE,		// 0x00B
	TPM_INVALID_KEYHANDLE_MESSAGE,		// 0x00C
	TPM_KEYNOTFOUND_MESSAGE,			// 0x00D
	TPM_INAPPROPRIATE_ENC_MESSAGE,		// 0x00E
	TPM_MIGRATEFAIL_MESSAGE,			// 0x00F
	TPM_INVALID_PCR_INFO_MESSAGE,		// 0x010
	TPM_NOSPACE_MESSAGE,				// 0x011
	TPM_NOSRK_MESSAGE,					// 0x012
	TPM_NOTSEALED_BLOB_MESSAGE,			// 0x013
	TPM_OWNER_SET_MESSAGE,				// 0x014
	TPM_RESOURCES_MESSAGE,				// 0x015
	TPM_SHORTRANDOM_MESSAGE,			// 0x016
	TPM_SIZE_MESSAGE,					// 0x017
	TPM_WRONGPCRVAL_MESSAGE,			// 0x018
	TPM_BAD_PARAM_SIZE_MESSAGE,			// 0x019
	TPM_SHA_THREAD_MESSAGE,				// 0x01A
	TPM_SHA_ERROR_MESSAGE,				// 0x01B
	TPM_FAILEDSELFTEST_MESSAGE,			// 0x01C
	TPM_AUTH2FAIL_MESSAGE,				// 0x01D
	TPM_BADTAG_MESSAGE,					// 0x01E
	TPM_IOERROR_MESSAGE,				// 0x01F
	TPM_ENCRYPT_ERROR_MESSAGE,			// 0x020
	TPM_DECRYPT_ERROR_MESSAGE,			// 0x021
	TPM_INVALID_AUTHHANDLE_MESSAGE,		// 0x022
	TPM_NO_ENDORSEMENT_MESSAGE,			// 0x023
	TPM_INVALID_KEYUSAGE_MESSAGE,		// 0x024
	TPM_WRONG_ENTITYTYPE_MESSAGE,		// 0x025
	TPM_INVALID_POSTINIT_MESSAGE,		// 0x026
	TPM_INAPPROPRIATE_SIG_MESSAGE,		// 0x027
	TPM_BAD_KEY_PROPERTY_MESSAGE,		// 0x028
	TPM_BAD_MIGRATION_MESSAGE,			// 0x029
	TPM_BAD_SCHEME_MESSAGE,				// 0x02A
	TPM_BAD_DATASIZE_MESSAGE,			// 0x02B
	TPM_BAD_MODE_MESSAGE,				// 0x02C
	TPM_BAD_PRESENCE_MESSAGE,			// 0x02D
	TPM_BAD_VERSION_MESSAGE,			// 0x02E
	TPM_NO_WRAP_TRANSPORT_MESSAGE,		// 0x02F
	TPM_AUDITFAIL_UNSUCCESSFUL_MESSAGE,	// 0x030
	TPM_AUDITFAIL_SUCCESSFUL_MESSAGE,	// 0x031
	TPM_NOTRESETABLE_MESSAGE,			// 0x032
	TPM_NOTLOCAL_MESSAGE,				// 0x033
	TPM_BAD_TYPE_MESSAGE,				// 0x034
	TPM_INVALID_RESOURCE_MESSAGE,		// 0x035
	TPM_NOTFIPS_MESSAGE,				// 0x036
	TPM_INVALID_FAMILY_MESSAGE,			// 0x037
	TPM_NO_NV_PERMISSION_MESSAGE,		// 0x038
	TPM_REQUIRES_SIGN_MESSAGE,			// 0x039
	TPM_KEY_NOTSUPPORTED_MESSAGE,		// 0x03A
	TPM_AUTH_CONFLICT_MESSAGE,			// 0x03B
	TPM_AREA_LOCKED_MESSAGE,			// 0x03C
	TPM_BAD_LOCALITY_MESSAGE,			// 0x03D
	TPM_READ_ONLY_MESSAGE,				// 0x03E
	TPM_PER_NOWRITE_MESSAGE,			// 0x03F
	TPM_FAMILYCOUNT_MESSAGE,			// 0x040
	TPM_WRITE_LOCKED_MESSAGE,			// 0x041
	TPM_BAD_ATTRIBUTES_MESSAGE,			// 0x042
	TPM_INVALID_STRUCTURE_MESSAGE,		// 0x043
	TPM_KEY_OWNER_CONTROL_MESSAGE,		// 0x044
	TPM_BAD_COUNTER_MESSAGE,			// 0x045
	TPM_NOT_FULLWRITE_MESSAGE,			// 0x046
	TPM_CONTEXT_GAP_MESSAGE,			// 0x047
	TPM_MAXNVWRITES_MESSAGE,			// 0x048
	TPM_NOOPERATOR_MESSAGE,				// 0x049
	TPM_RESOURCEMISSING_MESSAGE,		// 0x04A
	TPM_DELEGATE_LOCK_MESSAGE,			// 0x04B
	TPM_DELEGATE_FAMILY_MESSAGE,		// 0x04C
	TPM_DELEGATE_ADMIN_MESSAGE,			// 0x04D
	TPM_TRANSPORT_NOTEXCLUSIVE_MESSAGE,	// 0x04E
	TPM_OWNER_CONTROL_MESSAGE,			// 0x04F
	TPM_DAA_RESOURCES_MESSAGE,			// 0x050
	TPM_DAA_INPUT_DATA0_MESSAGE,		// 0x051
	TPM_DAA_INPUT_DATA1_MESSAGE,		// 0x052
	TPM_DAA_ISSUER_SETTINGS_MESSAGE,	// 0x053
	TPM_DAA_TPM_SETTINGS_MESSAGE,		// 0x054
	TPM_DAA_STAGE_MESSAGE,				// 0x055
	TPM_DAA_ISSUER_VALIDITY_MESSAGE,	// 0x056
	TPM_DAA_WRONG_W_MESSAGE,			// 0x057
	TPM_BAD_HANDLE_MESSAGE,				// 0x058
	TPM_BAD_DELEGATE_MESSAGE,			// 0x059
	TPM_BADCONTEXT_MESSAGE,				// 0x05A
	TPM_TOOMANYCONTEXTS_MESSAGE,		// 0x05B
	TPM_MA_TICKET_SIGNATURE_MESSAGE,	// 0x05C
	TPM_MA_DESTINATION_MESSAGE,			// 0x05D
	TPM_MA_SOURCE_MESSAGE,				// 0x05E
	TPM_MA_AUTHORITY_MESSAGE,			// 0x05F
	NULL,								// 0x060
	TPM_PERMANENTEK_MESSAGE,			// 0x061
	TPM_BAD_SIGNATURE_MESSAGE,			// 0x062
	TPM_NOCONTEXTSPACE_MESSAGE			// 0x063
};

/// A list of all TPM1.2 warning messages with their response code offset as index
const wchar_t* s_rgwszTpm12WarningMessages[] =	// TPM_BASE (0x0) + TPM_NON_FATAL (0x800) + offset
{
	TPM_RETRY_MESSAGE,					// 0x000
	TPM_NEEDS_SELFTEST_MESSAGE,			// 0x001
	TPM_DOING_SELFTEST_MESSAGE,			// 0x002
	TPM_DEFEND_LOCK_RUNNING_MESSAGE		// 0x003
};

/**
 *	@brief		Parses a TPM2.0 Format-One response code
 *	@details	Parses a given TPM2.0 Format-One response code and returns a corresponding message.
 *
 *	@param		PunErrorCode			The TPM2.0 Format-One response code to be parsed
 *	@param		PwszResponseBuffer		In: A target buffer for the output message\n
 *										Out: A buffer containing the null-terminated message
 *	@param		PunResponseBufferSize	The size of the target buffer
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatOneGetMessage(
	_In_									unsigned int	PunErrorCode,
	_Inout_z_cap_(PunResponseBufferSize)	wchar_t*		PwszResponseBuffer,
	_In_									unsigned int	PunResponseBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;
	wchar_t wszTemp[MAX_MESSAGE_SIZE] = {0};
	unsigned int unTempSize = RG_LEN(wszTemp);

	// In Format-One response codes, the bits 8 to 11 indicate the number of the element in error
	unsigned int unErrorElementNumber = (PunErrorCode & F1_RC_RANGE_BITS_8_TO_11_N) >> 8;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check parameter
		if (NULL == PwszResponseBuffer || 0 == PunResponseBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Get message for actual error number; in Format-One, the bits 0 to 5 represent the actual error number
		unReturnValue = TpmResponse_FormatOneErrorGetMessage(PunErrorCode & F1_RC_RANGE_BITS_0_TO_5_E, PwszResponseBuffer, PunResponseBufferSize);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Check for valid element number
		if ((0 == unErrorElementNumber) || (F1_RC_N_MIN_SESSION_VALUE == unErrorElementNumber && !(F1_RC_FLAG_BIT_6_P & PunErrorCode)))
		{
			unReturnValue = Platform_StringFormat(wszTemp, &unTempSize, L" Unable to designate the handle, session, or parameter in error.");
			if (RC_SUCCESS != unReturnValue)
				break;
		}
		// Check for error associated with a parameter or a session / handle
		else if (F1_RC_FLAG_BIT_6_P & PunErrorCode)
		{
			unReturnValue = Platform_StringFormat(wszTemp, &unTempSize, L" Parameter %d is invalid.", unErrorElementNumber);
			if (RC_SUCCESS != unReturnValue)
				break;
		}
		else // Bit 6 == 0 => The error is associated with a handle or a session or none
		{
			// Check if element number is a session or a handle number
			if (F1_RC_N_MIN_SESSION_VALUE <= unErrorElementNumber)
			{
				// Value is equal or larger than F1_RC_N_MIN_SESSION_VALUE => value indicates session number
				// F1_RC_N_MIN_SESSION_VALUE is not part of actual session number
				unReturnValue = Platform_StringFormat(wszTemp, &unTempSize, L" Session %d is invalid.", unErrorElementNumber - F1_RC_N_MIN_SESSION_VALUE);
			}
			else
			{
				// Value is smaller than F1_RC_N_MIN_SESSION_VALUE => value indicates handle number
				unReturnValue = Platform_StringFormat(wszTemp, &unTempSize, L" Handle %d is invalid.", unErrorElementNumber);
			}
			if (RC_SUCCESS != unReturnValue)
				break;
		}

		// Add information to error message
		unReturnValue = Platform_StringConcatenate(PwszResponseBuffer, &PunResponseBufferSize, wszTemp);
		if (RC_SUCCESS != unReturnValue)
			break;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Parses a TPM2.0 Format-Zero response code
 *	@details	Parses a given TPM2.0 Format-Zero response code and returns a corresponding message.
 *
 *	@param		PunErrorCode			The TPM2.0 Format-Zero response code to be parsed
 *	@param		PwszResponseBuffer		In: A target buffer for the output message\n
 *										Out: A buffer containing the null-terminated message
 *	@param		PunResponseBufferSize	The size of the target buffer
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatZeroGetMessage(
	_In_									unsigned int	PunErrorCode,
	_Inout_z_cap_(PunResponseBufferSize)	wchar_t*		PwszResponseBuffer,
	_In_									unsigned int	PunResponseBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;
	wchar_t wszTemp[MAX_MESSAGE_SIZE] = {0};
	unsigned int unTempSize = RG_LEN(wszTemp);
	unsigned int unOriginalSize = PunResponseBufferSize;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check parameter
		if (NULL == PwszResponseBuffer || 0 == PunResponseBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Check for valid Format-Zero response code
		if (F0_RC_FLAG_BIT_9_RESERVED & PunErrorCode)
		{
			unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, L"Invalid response code (Bit 9 is not zero).");
			if (RC_SUCCESS != unReturnValue)
				break;
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Check for vendor specific response code
		if (F0_RC_FLAG_BIT_10_T & PunErrorCode)
		{
			unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, L"Vendor specific TPM return code: ");
			if (RC_SUCCESS != unReturnValue)
				break;
		}

		// Check for TPM2.0 or 1.2 response code
		if (F0_RC_FLAG_BIT_8_V & PunErrorCode)
		{
			// Value is a TPM2.0 Format-Zero response code; here the bits 0 to 6 represent the actual error number
			unsigned int unErrorNumber = PunErrorCode & F0_RC_RANGE_BITS_0_TO_6_E;

			// Check for warning or error response code (aka severity) and get the corresponding error message
			if (F0_RC_FLAG_BIT_11_S & PunErrorCode)
				unReturnValue = TpmResponse_FormatZeroWarningGetMessage(unErrorNumber, wszTemp, unTempSize);
			else
				unReturnValue = TpmResponse_FormatZeroErrorGetMessage(unErrorNumber, wszTemp, unTempSize);
		}
		else
			// Code is a TPM1.2 response code
			unReturnValue = TpmResponse_TPM12GetMessage(PunErrorCode, wszTemp, unTempSize);

		if (RC_SUCCESS != unReturnValue)
			break;

		// Add response message to response buffer
		PunResponseBufferSize = unOriginalSize;
		unReturnValue = Platform_StringConcatenate(PwszResponseBuffer, &PunResponseBufferSize, wszTemp);
		if (RC_SUCCESS != unReturnValue)
			break;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Gets the message for a TPM2.0 Format-Zero error number
 *	@details	Gets the error message for a given TPM2.0 Format-Zero error number (unmasked with RC_VER1).
 *
 *	@param		PunErrorCode			The RC_VER1 (0x100) unmasked TPM2.0 Format-Zero error number.
 *	@param		PwszResponseBuffer		In: A target buffer for the output message\n
 *										Out: A buffer containing the null-terminated message
 *	@param		PunResponseBufferSize	The size of the target buffer
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatZeroErrorGetMessage(
	_In_									unsigned int	PunErrorCode,
	_Inout_z_cap_(PunResponseBufferSize)	wchar_t*		PwszResponseBuffer,
	_In_									unsigned int	PunResponseBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		const wchar_t* wszMessage = NULL;
		unsigned int unMessagesListSize = sizeof(s_rgwszFormatZeroErrorMessages) / sizeof(wchar_t*);

		// Check parameter
		if (NULL == PwszResponseBuffer || 0 == PunResponseBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Check for array boundaries and get corresponding message from list
		if (PunErrorCode < unMessagesListSize)
			wszMessage = s_rgwszFormatZeroErrorMessages[PunErrorCode];
		if (NULL == wszMessage)
		{
			unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, L"Unknown error code: 0x%.08X.", PunErrorCode);
			if (RC_SUCCESS != unReturnValue)
				break;
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Write message to output buffer
		unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, wszMessage);
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Gets the message for a TPM2.0 Format-One error number
 *	@details	Gets the error message for a given TPM2.0 Format-One error number (unmasked with RC_FMT1).
 *
 *	@param		PunErrorCode			The RC_FMT1 (0x080) unmasked TPM2.0 Format-One error number.
 *	@param		PwszResponseBuffer		In: A target buffer for the output message\n
 *										Out: A buffer containing the null-terminated message
 *	@param		PunResponseBufferSize	The size of the target buffer
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatOneErrorGetMessage(
	_In_									unsigned int	PunErrorCode,
	_Inout_z_cap_(PunResponseBufferSize)	wchar_t*		PwszResponseBuffer,
	_In_									unsigned int	PunResponseBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		const wchar_t* wszMessage = NULL;
		unsigned int unMessagesListSize = sizeof(s_rgwszFormatOneErrorMessages) / sizeof(wchar_t*);

		// Check parameter
		if (NULL == PwszResponseBuffer || 0 == PunResponseBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Check for array boundaries and get corresponding message from list
		if (PunErrorCode < unMessagesListSize)
			wszMessage = s_rgwszFormatOneErrorMessages[PunErrorCode];
		if (NULL == wszMessage)
		{
			unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, L"Unknown error code: 0x%.08X.", PunErrorCode);
			if (RC_SUCCESS != unReturnValue)
				break;
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Write message to output buffer
		unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, wszMessage);
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Gets the message for a TPM2.0 Format-One warning number
 *	@details	Gets the message for a given TPM2.0 Format-One warning number (unmasked with RC_FMT1).
 *
 *	@param		PunErrorCode			The RC_WARN (0x900) unmasked TPM2.0 Format-One warning number.
 *	@param		PwszResponseBuffer		In: A target buffer for the output message\n
 *										Out: A buffer containing the null-terminated message
 *	@param		PunResponseBufferSize	The size of the target buffer
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_FormatZeroWarningGetMessage(
	_In_									unsigned int	PunErrorCode,
	_Inout_z_cap_(PunResponseBufferSize)	wchar_t*		PwszResponseBuffer,
	_In_									unsigned int	PunResponseBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		const wchar_t* wszMessage = NULL;
		unsigned int unMessagesListSize = sizeof(s_rgwszFormatZeroWarningMessages) / sizeof(wchar_t*);

		// Check parameter
		if (NULL == PwszResponseBuffer || 0 == PunResponseBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Check for array boundaries and get corresponding message from list
		if (PunErrorCode < unMessagesListSize)
			wszMessage = s_rgwszFormatZeroWarningMessages[PunErrorCode];
		if (NULL == wszMessage)
		{
			unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, L"Unknown warning code: 0x%.08X.", PunErrorCode);
			if (RC_SUCCESS != unReturnValue)
				break;
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Write message to output buffer
		unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, wszMessage);
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Parses a TPM1.2 response code
 *	@details	Parses a given TPM1.2 response code and returns a corresponding message.
 *
 *	@param		PunErrorCode			The TPM1.2 response code to be parsed
 *	@param		PwszResponseBuffer		In: A target buffer for the output message\n
 *										Out: A buffer containing the null-terminated message
 *	@param		PunResponseBufferSize	The size of the target buffer
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_TPM12GetMessage(
	_In_									unsigned int	PunErrorCode,
	_Inout_z_cap_(PunResponseBufferSize)	wchar_t*		PwszResponseBuffer,
	_In_									unsigned int	PunResponseBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		const wchar_t* wszMessage = NULL;
		unsigned int unErrorMessagesListSize = sizeof(s_rgwszTpm12ErrorMessages) / sizeof(wchar_t*);
		unsigned int unUnMaskedErrorCode = PunErrorCode & ~RC_TPM_MASK;

		// Check parameter
		if (NULL == PwszResponseBuffer || 0 == PunResponseBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Check if TPM1.2 response code is a warning
		if (TPM_NON_FATAL & unUnMaskedErrorCode)
		{
			unsigned int unWarningMessagesListSize = sizeof(s_rgwszTpm12WarningMessages) / sizeof(wchar_t*);

			// Remove TPM_NON_FATAL to get the actual message index
			unsigned int unIndex = unUnMaskedErrorCode - TPM_NON_FATAL;

			// Check for array boundaries and get corresponding warning message from list
			if (unIndex < unWarningMessagesListSize)
				wszMessage = s_rgwszTpm12WarningMessages[unIndex];
		}
		// Check for array boundaries and get corresponding error message from list
		else if (unUnMaskedErrorCode < unErrorMessagesListSize)
		{
			wszMessage = s_rgwszTpm12ErrorMessages[unUnMaskedErrorCode];
		}
		if (NULL == wszMessage)
		{
			unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, L"Unknown TPM1.2 response code: 0x%.08X.", PunErrorCode);
			if (RC_SUCCESS != unReturnValue)
				break;
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Write message to output buffer
		unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, wszMessage);
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Topmost function for parsing a TPM2.0 response code
 *	@details	Parses a given TPM2.0 response code and returns a corresponding message.
 *
 *	@param		PunErrorCode			The TPM2.0 response code to be parsed
 *	@param		PwszResponseBuffer		In: A target buffer for the output message\n
 *										Out: A buffer containing the null-terminated message
 *	@param		PunResponseBufferSize	The size of the target buffer
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TpmResponse_GetMessage(
	_In_									unsigned int	PunErrorCode,
	_Inout_z_cap_(PunResponseBufferSize)	wchar_t*		PwszResponseBuffer,
	_In_									unsigned int	PunResponseBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check parameter
		if (NULL == PwszResponseBuffer || 0 == PunResponseBufferSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Make sure response buffer is empty
		unReturnValue = Platform_StringSetZero(PwszResponseBuffer, PunResponseBufferSize);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
			break;

		// Check for TSS_TPM_RC_SUCCESS
		if (TSS_TPM_RC_SUCCESS == PunErrorCode)
		{
			unReturnValue = Platform_StringFormat(PwszResponseBuffer, &PunResponseBufferSize, TPM_RC_SUCCESS_MESSAGE);
			break;
		}

		// Check for error format and call corresponding parser method
		if (TPM_RC_FLAG_BIT_7_F & PunErrorCode)
			unReturnValue = TpmResponse_FormatOneGetMessage(PunErrorCode, PwszResponseBuffer, PunResponseBufferSize);
		else
			unReturnValue = TpmResponse_FormatZeroGetMessage(PunErrorCode, PwszResponseBuffer, PunResponseBufferSize);
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}
