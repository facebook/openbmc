/**
 *	@brief		Implements the application configuration module
 *	@details	Module to handle the application's configuration
 *	@file		Config.c
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

#include "Config.h"
#include "Utility.h"
#include "Platform.h"
#include "FileIO.h"
#include "IConfigSettings.h"

/**
 *	@brief		Parse the configuration file
 *	@details	This function reads the configuration file, parses the key value pairs and
 *				stores these pairs in the PropertyStorage.
 *
 *	@param		PwszConfigFileName	Pointer to a wide character configuration file name
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. E.g. NULL or empty file name
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Config_Parse(
	_In_z_	const wchar_t*	PwszConfigFileName)
{
	return Config_ParseCustom(
			   PwszConfigFileName,
			   (IConfigSettings_InitializeParsing) NULL,
			   (IConfigSettings_FinalizeParsing) NULL,
			   (IConfigSettings_Parse) NULL);
}

/**
 *	@brief		Parse the configuration file
 *	@details	This function reads the configuration file, parses the key value pairs and
 *				stores these pairs in the PropertyStorage.
 *
 *	@param		PwszConfigFileName		Pointer to a wide character configuration file name
 *	@param		PpfInitializeParsing	Function pointer to a Initialize parsing function
 *	@param		PpfFinalizeParsing		Function pointer to a Finalize parsing function
 *	@param		PpfParse				Function pointer to a Parsing function
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. E.g. NULL or empty file name
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Config_ParseCustom(
	_In_z_		const wchar_t*						PwszConfigFileName,
	_In_opt_	IConfigSettings_InitializeParsing	PpfInitializeParsing,
	_In_opt_	IConfigSettings_FinalizeParsing		PpfFinalizeParsing,
	_In_opt_	IConfigSettings_Parse				PpfParse)
{
	unsigned int unReturnValue = RC_E_FAIL;
	wchar_t* wszConfigFileContent = NULL;
	unsigned int unConfigFileContentSize = 0;

	do
	{
		// Check parameter
		if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszConfigFileName))
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Configuration file name is NULL or empty.");
			break;
		}

		// Initialize defaults
		if (NULL == PpfInitializeParsing)
			unReturnValue = ConfigSettings_InitializeParsing();
		else
			unReturnValue = PpfInitializeParsing();

		if (RC_SUCCESS != unReturnValue)
			break;

		// Read whole file
		unReturnValue = FileIO_ReadFileToStringBuffer(PwszConfigFileName, &wszConfigFileContent, &unConfigFileContentSize);
		if (RC_SUCCESS != unReturnValue)
		{
			if (RC_E_FILE_NOT_FOUND == unReturnValue)
				unReturnValue = RC_SUCCESS;
			else
			{
				ERROR_STORE_FMT(unReturnValue, L"FileIO_ReadFileToStringBuffer failed to read the configuration file (%ls)", PwszConfigFileName);
				break;
			}
		}
		if (PLATFORM_STRING_IS_NULL_OR_EMPTY(wszConfigFileContent) ||
				0 == unConfigFileContentSize)
			break;

		// Increase content size by one due to null-termination
		unConfigFileContentSize++;

		// Now parse the file content
		unReturnValue = Config_ParseContent(wszConfigFileContent, unConfigFileContentSize, PpfParse);
	}
	WHILE_FALSE_END;

	// Free allocated memory
	Platform_MemoryFree((void**)&wszConfigFileContent);

	// Finalize parsing
	if (NULL == PpfFinalizeParsing)
		unReturnValue = ConfigSettings_FinalizeParsing(unReturnValue);
	else
		unReturnValue = PpfFinalizeParsing(unReturnValue);

	return unReturnValue;
}

/**
 *	@brief		Parse configuration file content for settings
 *	@details	This function parses given configuration file content for settings
 *
 *	@param		PwszContent		Pointer to a wide character configuration file content
 *	@param		PunContentSize	Size of the content buffer in elements including the zero termination
 *	@param		PpfParse		Function pointer to a Parsing function
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. E.g. NULL or empty content
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
Config_ParseContent(
	_In_z_count_(PunContentSize)	const wchar_t*			PwszContent,
	_In_							unsigned int			PunContentSize,
	_In_opt_						IConfigSettings_Parse	PpfParse)
{
	unsigned int unReturnValue = RC_E_FAIL;
	wchar_t* wszLine = NULL;

	do
	{
		unsigned int unIndex = 0;
		wchar_t wszSectionName[MAX_STRING_1024] = {0};
		unsigned int unSectionNameSize = RG_LEN(wszSectionName);
		BOOL fBlockComment = FALSE;

		// Check parameters
		if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszContent) ||
				0 == PunContentSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Content is NULL or empty");
			break;
		}

		// Loop over the lines
		do
		{
			unsigned int unLineSize = 0;
			BOOL fFlag = FALSE;

			// Free memory before read the next line
			Platform_MemoryFree((void**)&wszLine);

			// Get line from buffer
			unReturnValue = Utility_StringGetLine(
								PwszContent,
								PunContentSize,
								&unIndex,
								&wszLine,
								&unLineSize);
			if (RC_SUCCESS != unReturnValue)
			{
				if (RC_E_END_OF_STRING == unReturnValue)
					unReturnValue = RC_SUCCESS;
				else
					ERROR_STORE(unReturnValue, L"Unexpected error occurred while parsing the flagcheck file content.");
				break;
			}

			// Remove comments
			unReturnValue = Utility_StringRemoveComment(
								wszLine,
								&fBlockComment,
								&unLineSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Unexpected error occurred in Utility_StringRemoveComment.");
				break;
			}

			// Add count for zero termination
			unLineSize += 1;

			unReturnValue = Utility_StringRemoveWhiteChars(wszLine, &unLineSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Unexpected error executing Utility_StringRemoveWhiteChars");
				break;
			}

			// Add count for zero termination
			unLineSize += 1;

			if (PLATFORM_STRING_IS_NULL_OR_EMPTY(wszLine) ||
					0 == unLineSize)
				continue;

			// Check if line contains a section
			unReturnValue = Utility_IniIsSection(wszLine, unLineSize, &fFlag);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Unexpected error occurred in Utility_IniIsSection.");
				break;
			}
			if (TRUE == fFlag)
			{
				unSectionNameSize = RG_LEN(wszSectionName);
				unReturnValue = Utility_IniGetSectionName(wszLine, unLineSize, wszSectionName, &unSectionNameSize);
				if (RC_SUCCESS != unReturnValue)
				{
					ERROR_STORE(unReturnValue, L"Unexpected error occurred in Utility_IniGetSectionName.");
					break;
				}
				continue;
			}

			// Check if line contains a key value pair
			unReturnValue = Utility_IniIsKeyValue(wszLine, unLineSize, &fFlag);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Unexpected error occurred in Utility_IniIsKeyValue.");
				break;
			}
			if (TRUE == fFlag)
			{
				wchar_t wszKey[MAX_STRING_1024] = {0};
				wchar_t wszValue[MAX_STRING_1024] = {0};
				unsigned int unKeySize = RG_LEN(wszKey), unValueSize = RG_LEN(wszValue);
				unReturnValue = Utility_IniGetKeyValue(wszLine, unLineSize, wszKey, &unKeySize, wszValue, &unValueSize);
				if (RC_SUCCESS != unReturnValue)
				{
					ERROR_STORE(unReturnValue, L"Unexpected error occurred in Utility_IniGetKeyValue.");
					break;
				}

				// Increase sizes by one due to null-termination
				if (RG_LEN(wszSectionName) > unSectionNameSize)
					unSectionNameSize++;
				if (RG_LEN(wszKey) > unKeySize)
					unKeySize++;

				// Parse Key Value pair
				if (NULL == PpfParse)
					unReturnValue = ConfigSettings_Parse(wszSectionName, unSectionNameSize, wszKey, unKeySize, wszValue, unValueSize);
				else
					unReturnValue = PpfParse(wszSectionName, unSectionNameSize, wszKey, unKeySize, wszValue, unValueSize);

				if (RC_SUCCESS != unReturnValue)
					break;

				continue;
			}
		}
		while(RC_SUCCESS == unReturnValue);
	}
	WHILE_FALSE_END;

	// Free allocated memory
	Platform_MemoryFree((void**)&wszLine);

	return unReturnValue;
}
