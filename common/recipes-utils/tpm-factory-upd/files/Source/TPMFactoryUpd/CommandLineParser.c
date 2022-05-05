/**
 *	@brief		Implements the command line parser
 *	@details	This module parses the command line and provides the command line properties
 *	@file		CommandLineParser.c
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
#include "ICommandLineParser.h"
#include "CommandLineParser.h"
#include "TPMFactoryUpdStruct.h"
#include "Resource.h"

wchar_t GwszToolName[MAX_PATH] = L"TPMFactoryUpd";

/**
 *	@brief		Parses the command line option. Implements interface ICommandLineParser
 *	@details	Parses the command line option identified by the common command line module code
 *
 *	@param		PwszCommandLineOption	Pointer to a wide character array containing the current option to work on
 *	@param		PpunCurrentArgIndex		Pointer to an index for the current position in the argument list
 *	@param		PnMaxArg				Maximum number of argument is the arguments list
 *	@param		PrgwszArgv				Pointer to an array of wide character arrays representing the command line
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_BAD_COMMANDLINE	In case of an invalid command line
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_Parse(
	_In_z_					const wchar_t*			PwszCommandLineOption,
	_Inout_					unsigned int*			PpunCurrentArgIndex,
	_In_					int						PnMaxArg,
	_In_reads_z_(PnMaxArg)	const wchar_t* const	PrgwszArgv[])
{
	// Initialize to RC_SUCCESS because of while loop condition
	unsigned int unReturnValue = RC_E_FAIL;

	wchar_t wszValue[MAX_STRING_1024] = {0};
	unsigned int unValueSize = RG_LEN(wszValue);

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		// Check parameters
		if (NULL == PwszCommandLineOption ||
				NULL == PpunCurrentArgIndex ||
				NULL == PrgwszArgv)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Invalid NULL parameter.");
			break;
		}

		// **** -help
		if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_HELP, RG_LEN(CMD_HELP), TRUE) ||
				0 == Platform_StringCompare(PwszCommandLineOption, CMD_HELP_ALT, RG_LEN(CMD_HELP_ALT), FALSE))
		{
			unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Add help property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
			IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_HELP, TRUE));

			unReturnValue = CommandLineParser_IncrementOptionCount();
			break;
		}

		// **** -info
		if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_INFO, RG_LEN(CMD_INFO), TRUE))
		{
			unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Add info property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
			IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_INFO, TRUE));

			unReturnValue = CommandLineParser_IncrementOptionCount();
			break;
		}

		// **** -update
		if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_UPDATE, RG_LEN(CMD_UPDATE), TRUE))
		{
			unsigned int unUpdateType = UPDATE_TYPE_NONE;

			unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Add update property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
			IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_UPDATE, TRUE));

			// Read parameter Update type
			unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
			if (RC_SUCCESS == unReturnValue)
			{
				// Set Update type
				if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_TPM12_DEFERREDPP, RG_LEN(CMD_UPDATE_OPTION_TPM12_DEFERREDPP), TRUE))
				{
					unUpdateType = UPDATE_TYPE_TPM12_DEFERREDPP;
				}
				else if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_TPM12_TAKEOWNERSHIP, RG_LEN(CMD_UPDATE_OPTION_TPM12_TAKEOWNERSHIP), TRUE))
				{
					unUpdateType = UPDATE_TYPE_TPM12_TAKEOWNERSHIP;
				}
				else if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_TPM12_OWNERAUTH, RG_LEN(CMD_UPDATE_OPTION_TPM12_OWNERAUTH), TRUE))
				{
					unUpdateType = UPDATE_TYPE_TPM12_OWNERAUTH;
				}
				else if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_TPM20_EMPTYPLATFORMAUTH, RG_LEN(CMD_UPDATE_OPTION_TPM20_EMPTYPLATFORMAUTH), TRUE))
				{
					unUpdateType = UPDATE_TYPE_TPM20_EMPTYPLATFORMAUTH;
				}
				else if (0 == Platform_StringCompare(wszValue, CMD_UPDATE_OPTION_CONFIG_FILE, RG_LEN(CMD_UPDATE_OPTION_CONFIG_FILE), TRUE))
				{
					unUpdateType = UPDATE_TYPE_CONFIG_FILE;
				}
				else
				{
					unReturnValue = RC_E_BAD_COMMANDLINE;
					ERROR_STORE_FMT(unReturnValue, L"Unknown option for command line parameter <update> (%ls).", wszValue);
					break;
				}

				// Change / add update type property (if it does not exist)
				if (!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_UPDATE_TYPE, unUpdateType) &&
						!PropertyStorage_ChangeUIntegerValueByKey(PROPERTY_UPDATE_TYPE, unUpdateType))
				{
					unReturnValue = RC_E_FAIL;
					ERROR_STORE(unReturnValue, L"Setting PROPERTY_UPDATE_TYPE failed.");
					break;
				}
			}
			else
			{
				ERROR_STORE(unReturnValue, L"Missing type for command line parameter <update>.");
				break;
			}

			unReturnValue = CommandLineParser_IncrementOptionCount();
			break;
		}

		// **** -log
		if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_LOG, RG_LEN(CMD_LOG), TRUE))
		{
			unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Add logging property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
			IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_LOGGING, TRUE));

			// Change / add logging level property (if it does not exist)
			if (!PropertyStorage_ChangeUIntegerValueByKey(PROPERTY_LOGGING_LEVEL, LOGGING_LEVEL_3) &&
					!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_LOGGING_LEVEL, LOGGING_LEVEL_3))
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE(unReturnValue, L"Setting PROPERTY_LOGGING_LEVEL failed.");
				break;
			}

			// Read parameter Logging path (optional)
			unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
			if (RC_SUCCESS == unReturnValue)
			{
				// Check if path fits into property storage
				if (PROPERTY_STORAGE_MAX_VALUE <= unValueSize)
				{
					unReturnValue = RC_E_INVALID_LOG_OPTION;
					ERROR_STORE_FMT(unReturnValue, L"Log file (%ls) path is too long. (0x%.8X)", wszValue, unReturnValue);
					break;
				}
				// Change / add logging path property (if it does not exist)
				if (!PropertyStorage_AddKeyValuePair(PROPERTY_LOGGING_PATH, wszValue) &&
						!PropertyStorage_ChangeValueByKey(PROPERTY_LOGGING_PATH, wszValue))
				{
					unReturnValue = RC_E_FAIL;
					ERROR_STORE(unReturnValue, L"Setting PROPERTY_LOGGING_PATH failed.");
					break;
				}
			}

			unReturnValue = CommandLineParser_IncrementOptionCount();
			break;
		}

		// **** -firmware
		if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_FIRMWARE, RG_LEN(CMD_FIRMWARE), TRUE))
		{
			unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Read parameter Firmware path
			unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Missing firmware file path for command line parameter <firmware>.");
				break;
			}

			// Set Firmware path
			if (!PropertyStorage_AddKeyValuePair(PROPERTY_FIRMWARE_PATH, wszValue) &&
					!PropertyStorage_ChangeValueByKey(PROPERTY_FIRMWARE_PATH, wszValue))
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_AddKeyValuePair failed to add property '%ls'.", PROPERTY_FIRMWARE_PATH);
				break;
			}

			unReturnValue = CommandLineParser_IncrementOptionCount();
			break;
		}

		// **** -tpm12-clearownership
		if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_TPM12_CLEAROWNERSHIP, RG_LEN(CMD_TPM12_CLEAROWNERSHIP), TRUE))
		{
			unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Add ClearOwnership property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
			IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_TPM12_CLEAROWNERSHIP, TRUE));

			unReturnValue = CommandLineParser_IncrementOptionCount();
			break;
		}

		// **** -access-mode
		if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_ACCESS_MODE, RG_LEN(CMD_ACCESS_MODE), TRUE))
		{
			unsigned int unAccessMode = 0;
			unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Add ClearOwnership property, ignore return value because CommandLineParser_CheckCommandLineOptions takes care of doubled given options
			IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_ACCESS_MODE, TRUE));

			// Read parameter mode
			unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Missing mode for command line parameter <access-mode>.");
				break;
			}

			// Set access-mode
			if (!PropertyStorage_AddKeyValuePair(PROPERTY_TPM_DEVICE_ACCESS_MODE, wszValue) &&
					!PropertyStorage_ChangeValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, wszValue))
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE(unReturnValue, L"Unexpected error occurred while adding or changing the PROPERTY_TPM_DEVICE_ACCESS_MODE property.");
				break;
			}

			// Check if value is 1 or 3 for the TPM device access mode
			if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, &unAccessMode) ||
					(TPM_DEVICE_ACCESS_DRIVER != unAccessMode && TPM_DEVICE_ACCESS_MEMORY_BASED != unAccessMode))
			{
				unReturnValue = RC_E_INVALID_ACCESS_MODE;
				ERROR_STORE_FMT(unReturnValue, L"An invalid value (%ls) was passed in the <access-mode> command line option.", wszValue);
				break;
			}

			// Read optional device path
			unValueSize = RG_LEN(wszValue);
			unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
			if (RC_SUCCESS == unReturnValue)
			{
				// Store setting value
				if (!PropertyStorage_AddKeyValuePair(PROPERTY_TPM_DEVICE_ACCESS_PATH, wszValue) &&
						!PropertyStorage_ChangeValueByKey(PROPERTY_TPM_DEVICE_ACCESS_PATH, wszValue))
				{
					unReturnValue = RC_E_FAIL;
					ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_ChangeValueByKey failed to change property '%ls'.", PROPERTY_TPM_DEVICE_ACCESS_PATH);
					break;
				}
			}

			unReturnValue = CommandLineParser_IncrementOptionCount();
			break;
		}

		// **** -config
		if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_CONFIG, RG_LEN(CMD_CONFIG), TRUE))
		{
			unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Read parameter configuration file path
			unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Missing configuration file path for command line parameter <config>.");
				break;
			}

			// Set configuration file path
			if (!PropertyStorage_AddKeyValuePair(PROPERTY_CONFIG_FILE_PATH, wszValue) &&
					!PropertyStorage_ChangeValueByKey(PROPERTY_CONFIG_FILE_PATH, wszValue))
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_AddKeyValuePair failed to add property '%ls'.", PROPERTY_CONFIG_FILE_PATH);
				break;
			}

			unReturnValue = CommandLineParser_IncrementOptionCount();
			break;
		}

		// **** -ownerauth
		if (0 == Platform_StringCompare(PwszCommandLineOption, CMD_OWNERAUTH, RG_LEN(CMD_OWNERAUTH), TRUE))
		{
			unReturnValue = CommandLineParser_CheckCommandLineOptions(PwszCommandLineOption);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Read parameter owner authorization file
			unReturnValue = CommandLineParser_ReadParameter(PrgwszArgv, PnMaxArg, PpunCurrentArgIndex, wszValue, &unValueSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Missing owner authorization file path for command line parameter <ownerauth>.");
				break;
			}

			// Set owner authorization file path
			if (!PropertyStorage_AddKeyValuePair(PROPERTY_OWNERAUTH_FILE_PATH, wszValue) &&
				!PropertyStorage_ChangeValueByKey(PROPERTY_OWNERAUTH_FILE_PATH, wszValue))
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_AddKeyValuePair failed to add property '%ls'.", PROPERTY_OWNERAUTH_FILE_PATH);
				break;
			}

			unReturnValue = CommandLineParser_IncrementOptionCount();
			break;
		}

		unReturnValue = RC_E_BAD_COMMANDLINE;
		ERROR_STORE_FMT(unReturnValue, L"Unknown command line parameter (%ls).", PwszCommandLineOption);
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Initialize command line parsing
 *	@details
 *
 *	@retval		RC_SUCCESS	The operation completed successfully.
 *	@retval		...			Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_InitializeParsing()
{
	return RC_SUCCESS;
}

/**
 *	@brief		Finalize command line parsing
 *	@details
 *
 *	@param		PunReturnValue		Current return code which can be overwritten here.
 *	@retval		RC_SUCCESS	The operation completed successfully.
 *	@retval		...			Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_FinalizeParsing(
	_In_ unsigned int PunReturnValue)
{
	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		BOOL fValue = FALSE;

		if (RC_SUCCESS != PunReturnValue)
		{
			if (RC_E_BAD_COMMANDLINE != PunReturnValue &&
					RC_E_INVALID_ACCESS_MODE != PunReturnValue &&
					RC_E_INVALID_LOG_OPTION != PunReturnValue)
			{
				// Map error code to RC_E_BAD_PARAMETER
				unsigned int unReturnValue = PunReturnValue;
				PunReturnValue = RC_E_BAD_COMMANDLINE;
				ERROR_STORE_FMT(PunReturnValue, L"Mapped error code: 0x%.8x", unReturnValue);
			}
			// In case of an error skip final checks
			break;
		}

		// Check if log option is given...
		if (PropertyStorage_GetBooleanValueByKey(PROPERTY_LOGGING, &fValue) &&
				fValue == TRUE)
		{
			PunReturnValue = Utility_CheckIfLogPathWritable(TRUE);
			if (RC_SUCCESS != PunReturnValue)
			{
				// Calculate default log file path
				wchar_t wszDefaultLogPath[PROPERTY_STORAGE_MAX_VALUE] = {0};
				unsigned int unCapacity = RG_LEN(wszDefaultLogPath);
				PunReturnValue = Platform_StringCopy(wszDefaultLogPath, &unCapacity, GwszToolName);
				if (RC_SUCCESS != PunReturnValue)
					break;
				unCapacity = RG_LEN(wszDefaultLogPath);
				PunReturnValue = Platform_StringConcatenate(wszDefaultLogPath, &unCapacity, L".log");
				if (RC_SUCCESS != PunReturnValue)
					break;

				// Set default path
				// Ignore return value we are already in error handling
				IGNORE_RETURN_VALUE(PropertyStorage_ChangeValueByKey(PROPERTY_LOGGING_PATH, wszDefaultLogPath));

				ERROR_STORE_FMT(RC_E_INVALID_LOG_OPTION, L"The log file could not be opened or created. The path may be invalid, too long or not accessible. (0x%.8X)", PunReturnValue);
				PunReturnValue = RC_E_INVALID_LOG_OPTION;
				break;
			}
		}

		// Check if at least one mandatory option is set
		if ((FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_UPDATE, &fValue) || FALSE == fValue) &&
				(FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_INFO, &fValue) || FALSE == fValue) &&
				(FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_HELP, &fValue) || FALSE == fValue) &&
				(FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_TPM12_CLEAROWNERSHIP, &fValue) || FALSE == fValue))
		{
			PunReturnValue = RC_E_BAD_COMMANDLINE;
			ERROR_STORE(PunReturnValue, L"No mandatory command line option found.");
			break;
		}

		// Check that when update option is set ...
		if (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_UPDATE, &fValue) && TRUE == fValue)
		{
			unsigned int unUpdateType = UPDATE_TYPE_NONE;
			if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_UPDATE_TYPE, &unUpdateType))
			{
				PunReturnValue = RC_E_FAIL;
				ERROR_STORE(PunReturnValue, L"An internal error occurred while parsing the -update option.");
				break;
			}
			if (UPDATE_TYPE_CONFIG_FILE == unUpdateType)
			{
				// The configuration option is set for -update config-file
				if (FALSE == PropertyStorage_ExistsElement(PROPERTY_CONFIG_FILE_PATH))
				{
					PunReturnValue = RC_E_BAD_COMMANDLINE;
					ERROR_STORE(PunReturnValue, L"Mandatory command line option config is missing.");
					break;
				}
			}
			else if (UPDATE_TYPE_TPM12_OWNERAUTH == unUpdateType)
			{
				// The firmware option is set for -update tpm12-ownerauth
				if (FALSE == PropertyStorage_ExistsElement(PROPERTY_FIRMWARE_PATH))
				{
					PunReturnValue = RC_E_BAD_COMMANDLINE;
					ERROR_STORE(PunReturnValue, L"Mandatory command line option firmware is missing.");
					break;
				}
				// The ownerauth option is set for -update tpm12-ownerauth
				if (FALSE == PropertyStorage_ExistsElement(PROPERTY_OWNERAUTH_FILE_PATH))
				{
					PunReturnValue = RC_E_BAD_COMMANDLINE;
					ERROR_STORE(PunReturnValue, L"Mandatory command line option ownerauth is missing.");
					break;
				}
			}
			else
			{
				// The firmware option is set for all other -update <options>
				if (FALSE == PropertyStorage_ExistsElement(PROPERTY_FIRMWARE_PATH))
				{
					PunReturnValue = RC_E_BAD_COMMANDLINE;
					ERROR_STORE(PunReturnValue, L"Mandatory command line option firmware is missing.");
					break;
				}
			}
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, PunReturnValue);

	return PunReturnValue;
}

/**
 *	@brief		This function increments the command line option count
 *	@details
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_FAIL			An unexpected error occurred using the property storage.
 */
_Check_return_
unsigned int
CommandLineParser_IncrementOptionCount()
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		unsigned int unOptionCount = 0;
		BOOL fReturnValue = FALSE;
		// Check if option count exists
		if (PropertyStorage_GetUIntegerValueByKey(PROPERTY_CMDLINE_COUNT, &unOptionCount))
			// Increment the option count property
			fReturnValue = PropertyStorage_ChangeUIntegerValueByKey(PROPERTY_CMDLINE_COUNT, ++unOptionCount);
		else
			// Add option count property with value 1
			fReturnValue = PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_CMDLINE_COUNT, 1);

		if (!fReturnValue)
		{
			ERROR_STORE_FMT(unReturnValue, L"PropertyStorage returned an unexpected value while adding a key value pair. (%ls|%d)", PROPERTY_CMDLINE_COUNT, unOptionCount);
			break;
		}

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Read command line parameter to a string
 *	@details	Checks, if parameter is available and read it to buffer
 *
 *	@param		PrgwszArgv				Command line parameters
 *	@param		PnTotalParams			Number of all parameters
 *	@param		PpunPosition			Current position in parsing
 *	@param		PwszValue				Output buffer for command line parameter
 *	@param		PpunValueSize			Buffer length
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_COMMANDLINE	In case a mandatory parameter is missing for RegisterTest option
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLineParser_ReadParameter(
	_In_reads_z_(PnTotalParams)		const wchar_t* const	PrgwszArgv[],
	_In_							int						PnTotalParams,
	_Inout_							unsigned int*			PpunPosition,
	_Out_z_cap_(*PpunValueSize)		wchar_t*				PwszValue,
	_Inout_							unsigned int*			PpunValueSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		if ((NULL == PpunPosition) ||
				(NULL == PrgwszArgv) ||
				(NULL == PwszValue) ||
				(NULL == PpunValueSize))
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Increment the argument position and check if more arguments are available
		(*PpunPosition)++;

		// Check if parameter is present
		if (((int)*PpunPosition >= PnTotalParams) || (FALSE == CommandLineParser_IsValue(PrgwszArgv[*PpunPosition])))
		{
			(*PpunPosition)--;
			unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		// Read parameter: register
		unReturnValue = Platform_StringCopy(
							PwszValue,
							PpunValueSize,
							PrgwszArgv[*PpunPosition]
						);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"Unexpected error executing Platform_StringCopy.");
			break;
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Checks if the given string is a command line parameter value (no option)
 *	@details	If the given string does not start with '--' or '-' it is a command line parameter value
 *
 *	@param		PwszCommand		The command line parameter to check (max length MAX_STRING_1024)
 *	@retval		TRUE			PwszCommand does not start with '--', '-' or '/'
 *	@retval		FALSE			Otherwise
 */
_Check_return_
BOOL
CommandLineParser_IsValue(
	_In_z_		const wchar_t*	PwszCommand)
{
	BOOL			fIsValue = TRUE;
	unsigned int	unReturnValue = RC_SUCCESS;
	unsigned int	unSize = 0;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);
	// LOGGING_WRITE_LEVEL4_FMT(L"parameter: '%x'",PwszCommand);

	do
	{
		// Check buffer pointer and capacity
		if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszCommand))
		{
			// Parameter is empty
			fIsValue = FALSE;
			break;
		}

		// Determine string length
		unReturnValue = Platform_StringGetLength(PwszCommand, MAX_STRING_1024, &unSize);
		if (RC_SUCCESS != unReturnValue)
		{
			fIsValue = FALSE;
			ERROR_STORE_FMT(unReturnValue, L"Parameter PwszCommand is larger than %d or is not zero terminated.", MAX_STRING_1024);
			break;
		}

		// Check first character(s) in string
		if (unSize >= 2)
		{
			// Check for "--"
			if (PwszCommand[0] == L'-' && PwszCommand[1] == L'-')
			{
				fIsValue = FALSE;
			}
			else
			{
				// Check for "-"
				if (PwszCommand[0] == L'-')
					fIsValue = FALSE;
			}
		}
	}
	WHILE_FALSE_END;

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return fIsValue;
}

/**
 *	@brief		Checks whether the parameter can be combined with any previously set
 *	@details
 *
 *	@param		PwszCommand				The command line parameter to check
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_COMMANDLINE	In case of a option is not combinable with an already parsed one
 */
_Check_return_
unsigned int
CommandLineParser_CheckCommandLineOptions(
	_In_z_		const wchar_t*	PwszCommand)
{
	// Initialize return value to success because of negative checking and storing the error code
	unsigned int unReturnValue = RC_SUCCESS;

	LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

	do
	{
		BOOL fHelpOption = FALSE;
		BOOL fInfoOption = FALSE;
		BOOL fUpdateOption = FALSE;
		BOOL fLogOption = FALSE;
		BOOL fFwPathUpdateOption = FALSE;
		BOOL fClearOwnership = FALSE;
		BOOL fAccessMode = FALSE;
		BOOL fConfigFileOption = FALSE;
		BOOL fOwnerAuth = FALSE;

		// Read Property storage
		if (TRUE == PropertyStorage_ExistsElement(PROPERTY_HELP))
			fHelpOption = TRUE;
		if (TRUE == PropertyStorage_ExistsElement(PROPERTY_INFO))
			fInfoOption = TRUE;
		if (TRUE == PropertyStorage_ExistsElement(PROPERTY_UPDATE))
			fUpdateOption = TRUE;
		if (TRUE == PropertyStorage_ExistsElement(PROPERTY_LOGGING))
			fLogOption = TRUE;
		if (TRUE == PropertyStorage_ExistsElement(PROPERTY_FIRMWARE_PATH))
			fFwPathUpdateOption = TRUE;
		if (TRUE == PropertyStorage_ExistsElement(PROPERTY_TPM12_CLEAROWNERSHIP))
			fClearOwnership = TRUE;
		if (TRUE == PropertyStorage_ExistsElement(PROPERTY_ACCESS_MODE))
			fAccessMode = TRUE;
		if (TRUE == PropertyStorage_ExistsElement(PROPERTY_CONFIG_FILE_PATH))
			fConfigFileOption = TRUE;
		if (TRUE == PropertyStorage_ExistsElement(PROPERTY_OWNERAUTH_FILE_PATH))
			fOwnerAuth = TRUE;

		// **** -help [Help]
		if (0 == Platform_StringCompare(PwszCommand, CMD_HELP, RG_LEN(CMD_HELP), TRUE) ||
				0 == Platform_StringCompare(PwszCommand, CMD_HELP_ALT, RG_LEN(CMD_HELP_ALT), FALSE))
		{
			// Command line parameter 'help' combined with parameters 'info', 'update', 'firmware', 'log', 'tpm12-clearownership', 'access-mode' or 'config' is a bad command line
			if (TRUE == fHelpOption || // Parameter should not be given twice
					TRUE == fInfoOption ||
					TRUE == fUpdateOption ||
					TRUE == fFwPathUpdateOption ||
					TRUE == fLogOption ||
					TRUE == fClearOwnership ||
					TRUE == fAccessMode ||
					TRUE == fConfigFileOption ||
					TRUE == fOwnerAuth)
				unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		// **** -info [Info]
		if (0 == Platform_StringCompare(PwszCommand, CMD_INFO, RG_LEN(CMD_INFO), TRUE))
		{
			// Command line parameter 'info' combined with parameters 'help', 'update', 'firmware', 'tpm12-clearownership' or 'config' is a bad command line
			if (TRUE == fInfoOption || // And parameter 'info' should not be given twice
					TRUE == fHelpOption ||
					TRUE == fUpdateOption ||
					TRUE == fFwPathUpdateOption ||
					TRUE == fClearOwnership ||
					TRUE == fConfigFileOption ||
					TRUE == fOwnerAuth)
				unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		// **** -update [Update]
		if (0 == Platform_StringCompare(PwszCommand, CMD_UPDATE, RG_LEN(CMD_UPDATE), TRUE))
		{
			// Command line parameter 'update' combined with parameters 'help', 'info', 'tpm12-clearownership', or 'config' is a bad command line
			if (TRUE == fUpdateOption || // And parameter 'update' should not be given twice
					TRUE == fHelpOption ||
					TRUE == fInfoOption ||
					TRUE == fClearOwnership ||
					TRUE == fConfigFileOption)
				unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		// **** -log [Log]
		if (0 == Platform_StringCompare(PwszCommand, CMD_LOG, RG_LEN(CMD_LOG), TRUE))
		{
			// Command line parameter 'log' combined with parameter 'help' is a bad command line
			if (TRUE == fLogOption || // And parameter 'log' should not be given twice
					TRUE == fHelpOption)
				unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		// **** -firmware [Firmware]
		if (0 == Platform_StringCompare(PwszCommand, CMD_FIRMWARE, RG_LEN(CMD_FIRMWARE), TRUE))
		{
			// Command line parameter 'firmware' combined with parameters 'help', 'info', 'tpm12-clearownership' or 'config' is a bad command line
			if (TRUE == fFwPathUpdateOption || // And parameter 'firmware' should not be given twice
					TRUE == fHelpOption ||
					TRUE == fInfoOption ||
					TRUE == fClearOwnership ||
					TRUE == fConfigFileOption)
				unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		// **** -tpm12-clearownership [TPM12-ClearOwnership]
		if (0 == Platform_StringCompare(PwszCommand, CMD_TPM12_CLEAROWNERSHIP, RG_LEN(CMD_TPM12_CLEAROWNERSHIP), TRUE))
		{
			// Command line parameter 'tpm12-clearownership' combined with parameters 'help', 'info', 'update', 'firmware' or 'config' is a bad command line
			if (TRUE == fClearOwnership || // And parameter 'tpm12-clearownership' should not be given twice
					TRUE == fHelpOption ||
					TRUE == fInfoOption ||
					TRUE == fUpdateOption ||
					TRUE == fFwPathUpdateOption ||
					TRUE == fConfigFileOption)
				unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		// **** -access-mode [Access-mode]
		if (0 == Platform_StringCompare(PwszCommand, CMD_ACCESS_MODE, RG_LEN(CMD_ACCESS_MODE), TRUE))
		{
			// Command line parameter 'access-mode' combined with parameters 'help' is a bad command line
			if (TRUE == fAccessMode || // And parameter 'access-mode' should not be given twice
					TRUE == fHelpOption)
				unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		// **** -config [Configuration File]
		if (0 == Platform_StringCompare(PwszCommand, CMD_CONFIG, RG_LEN(CMD_CONFIG), TRUE))
		{
			// Command line parameter 'config' combined with parameters 'help', 'info', 'tpm12-clearownership' or 'firmware' is a bad command line
			if (TRUE == fConfigFileOption || // And parameter 'config' should not be given twice
					TRUE == fHelpOption ||
					TRUE == fInfoOption ||
					TRUE == fClearOwnership ||
					TRUE == fFwPathUpdateOption)
				unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		// **** -ownerauth [Owner Authorization File]
		if (0 == Platform_StringCompare(PwszCommand, CMD_OWNERAUTH, RG_LEN(CMD_OWNERAUTH), TRUE))
		{
			// Command line parameter 'ownerauth' combined with parameters 'help' or 'info' is a bad command line
			if (TRUE == fOwnerAuth || // And parameter 'ownerauth' should not be given twice
					TRUE == fHelpOption ||
					TRUE == fInfoOption)
				unReturnValue = RC_E_BAD_COMMANDLINE;
			break;
		}

		unReturnValue = RC_E_BAD_COMMANDLINE;
	}
	WHILE_FALSE_END;

	if (RC_SUCCESS != unReturnValue)
		ERROR_STORE(unReturnValue, L"Incompatible command line options found.");

	LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

	return unReturnValue;
}
