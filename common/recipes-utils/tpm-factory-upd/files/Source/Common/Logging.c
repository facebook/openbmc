/**
 *	@brief		Implements logging methods for the application.
 *	@details	This module provides logging for the application.
 *	@file		Logging.c
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

#include "Logging.h"
#include "FileIO.h"
#include "Config.h"
#include "Platform.h"
#include "Utility.h"

/// Flag indicating whether to write a header into the log file or not
BOOL g_fLogHeader = TRUE;

/// Flag indicating whether logging is already ongoing
BOOL s_fInLogging = FALSE;

/**
 *	@brief		This function writes the logging header to the log file, if it is the first call of the current instance.
 *	@details
 *
 *	@param		PfFileExists			Flag if the log file exists
 *	@param		PpFileHandle			Log file handle
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. The parameter is NULL
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
Logging_WriteHeader(
	_In_	BOOL	PfFileExists,
	_In_	void*	PpFileHandle)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		wchar_t wszTimeStamp[TIMESTAMP_LENGTH] = {0};
		unsigned int unTimeStampSize = RG_LEN(wszTimeStamp);

		// Check parameter
		if (NULL == PpFileHandle)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Write header in case this is the first log entry in the current application execution run
		if (TRUE == g_fLogHeader)
		{
			if (TRUE == PfFileExists)
			{
				// In case of appending add new lines to make the restart of the tool more visible
				// Needs \n\n as format string so it will be auto-converted to \r\n\r\n on UEFI.
				unReturnValue = FileIO_WriteStringf(PpFileHandle, L"\n\n");
				if (RC_SUCCESS != unReturnValue)
					break;
			}

			// Retrieve current wszTimeStamp including date here
			unReturnValue = Utility_GetTimestamp(TRUE, wszTimeStamp, &unTimeStampSize);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Write header to log file
			unReturnValue = FileIO_WriteStringf(PpFileHandle, L"%ls   %ls   Version %ls\n%ls\n\n", IFX_BRAND, GwszToolName, APP_VERSION, wszTimeStamp);
			if (RC_SUCCESS != unReturnValue)
				break;

			g_fLogHeader = FALSE;
		}

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		This function opens the logging file
 *	@details	This function opens the logging file, by append an existing or create a new one.
 *				It also checks the file size, if the property storage flag PROPERTY_LOGGING_CHECK_SIZE is set.
 *				In case of the maximum file size is reached the function reopens the file by using override flag.
 *
 *	@param		PppFileHandle			Pointer to store the file handle to. Must be closed by caller in any case.
 *	@param		PpfFileExists			Pointer to store the file exists flag
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. The parameter is NULL
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
Logging_OpenFile(
	_Inout_	void**	PppFileHandle,
	_Out_	BOOL*	PpfFileExists)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		wchar_t wszLoggingFilePath[MAX_STRING_1024] = {0};
		unsigned int unLoggingFilePathBufferSize = RG_LEN(wszLoggingFilePath);
		BOOL fFlag = FALSE;

		// Check out parameter
		if (NULL == PpfFileExists)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Set default
		*PpfFileExists = FALSE;

		// Check in/out parameter
		if (NULL == PppFileHandle || NULL != *PppFileHandle)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Get logging file path
		if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_LOGGING_PATH, wszLoggingFilePath, &unLoggingFilePathBufferSize))
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Check first if file exists and open it in the corresponding mode
		if (FileIO_Exists(wszLoggingFilePath))
		{
			*PpfFileExists = TRUE;
			unReturnValue = FileIO_Open(wszLoggingFilePath, PppFileHandle, FILE_APPEND);
		}
		else
			unReturnValue = FileIO_Open(wszLoggingFilePath, PppFileHandle, FILE_WRITE);

		if (RC_SUCCESS != unReturnValue || NULL == *PppFileHandle)
			break;

		// Check if PROPERTY_LOGGING_CHECK_SIZE flag is set in PropertyStorage
		if (TRUE == PropertyStorage_GetBooleanValueByKey(PROPERTY_LOGGING_CHECK_SIZE, &fFlag) &&
				TRUE == fFlag)
		{
			unsigned int unMaxFileSize = 0;
			unsigned long long ullFileSize = 0;

			// Get maximum log file size
			if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOGGING_MAXSIZE, &unMaxFileSize))
			{
				unReturnValue = RC_E_FAIL;
				break;
			}

			// Do actual size check only in case max log file size is not 0 (== unlimited)
			if (0 != unMaxFileSize)
			{
				// Get file size of actual logging file
				unReturnValue = FileIO_GetFileSize(*PppFileHandle, &ullFileSize);
				if (RC_SUCCESS != unReturnValue)
					break;

				// Check log file size limit and reopen file to overwrite it if necessary
				if (unMaxFileSize <= (unsigned int)(ullFileSize / DIV_KILOBYTE))
				{
					// Close log file (since it has been opened in append mode)
					unReturnValue = FileIO_Close(PppFileHandle);
					if (RC_SUCCESS != unReturnValue)
						break;

					// Reopen it (now in write mode)
					unReturnValue = FileIO_Open(wszLoggingFilePath, PppFileHandle, FILE_WRITE);
					if (RC_SUCCESS != unReturnValue || NULL == *PppFileHandle)
						break;

					*PpfFileExists = FALSE;
					g_fLogHeader = TRUE;
				}
			}

			// Reset property for enabling log file size check
			if (FALSE == PropertyStorage_ChangeBooleanValueByKey(PROPERTY_LOGGING_CHECK_SIZE, FALSE))
			{
				unReturnValue = RC_E_FAIL;
				break;
			}
		}
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		This writes a message to the log file
 *	@details	This function handles the logging work flow and writes a given message line by line to the
 *				logging file.
 *				It also checks the file size, if the property storage flag PROPERTY_LOGGING_CHECK_SIZE is set.
 *				In case of the maximum file size is reached the function reopens the file by using override flag.
 *
 *	@param		PszCurrentModule		Character string containing the current module name
 *	@param		PszCurrentFunction		Character string containing the current function name
 *	@param		PunLoggingLevel			Actual configured logging level
 *	@param		PwszMessage				Wide character string containing the message to log
 *	@param		PunMessageSize			Message size including the zero termination
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. The parameter is NULL
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
Logging_WriteMessage(
	_In_z_							const char*		PszCurrentModule,
	_In_z_							const char*		PszCurrentFunction,
	_In_							unsigned int	PunLoggingLevel,
	_In_z_count_(PunMessageSize)	wchar_t*		PwszMessage,
	_In_							unsigned int	PunMessageSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	// If logging is already ongoing avoid endless recursion
	if (FALSE == s_fInLogging)
	{
		void* pFile = NULL;
		wchar_t* wszLine = NULL;

		// Signal that logging has been started
		s_fInLogging = TRUE;

		do
		{
			BOOL fFileExists = FALSE;
			wchar_t wszTimeStamp[TIMESTAMP_LENGTH] = {0};
			unsigned int unTimeStampSize = RG_LEN(wszTimeStamp);
			unsigned int unIndex = 0;

			// Check main parameter
			if (NULL == PwszMessage)
			{
				unReturnValue = RC_E_BAD_PARAMETER;
				break;
			}

			// Retrieve current wszTimeStamp without date here
			unReturnValue = Utility_GetTimestamp(FALSE, wszTimeStamp, &unTimeStampSize);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Try to open the log file
			unReturnValue = Logging_OpenFile(&pFile, &fFileExists);
			if (RC_SUCCESS != unReturnValue || NULL == pFile)
				break;

			// Write header if necessary
			unReturnValue = Logging_WriteHeader(fFileExists, pFile);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Log the message line by line
			do
			{
				unsigned int unLineSize = 0;

				// Free allocated memory
				Platform_MemoryFree((void**)&wszLine);

				// Get the one line
				unReturnValue = Utility_StringGetLine(PwszMessage, PunMessageSize, &unIndex, &wszLine, &unLineSize);
				if (RC_SUCCESS != unReturnValue)
				{
					if (RC_E_END_OF_STRING != unReturnValue)
						ERROR_STORE(unReturnValue, L"Utility_StringGetLine returned an unexpected value.");
					else
						unReturnValue = RC_SUCCESS;

					break;
				}

				// For logging level 3 and 4, also write time-stamp to log file
				if (PunLoggingLevel >= LOGGING_LEVEL_3)
				{
					unReturnValue = FileIO_WriteStringf(pFile, L"%ls ", wszTimeStamp);
					if (RC_SUCCESS != unReturnValue)
						break;
				}

				// For logging level 4, also write module and function name to log file
				// If the module or function name is not set omit it
				if (PunLoggingLevel >= LOGGING_LEVEL_4)
				{
					// Convert module and function name from ANSI to Unicode
					wchar_t wszCurrentModule[MAX_NAME] = {0};
					wchar_t wszCurrentModuleNormalized[MAX_NAME] = {0};
					wchar_t wszCurrentFunction[MAX_NAME] = {0};

					unReturnValue = Platform_AnsiString2UnicodeString(wszCurrentModule, RG_LEN(wszCurrentModule), PszCurrentModule);
					if (RC_SUCCESS == unReturnValue)
					{
						unsigned int unCurrentModuleNormalizedLen = RG_LEN(wszCurrentModuleNormalized);
						wchar_t* pwszStart = NULL;
						// Find first occurrence of L"TPMToolsUEFIPkg\\" and remove everything before ...
						if (Platform_FindString(L"TPMToolsUEFIPkg\\", wszCurrentModule, &pwszStart) == RC_SUCCESS)
						{
							unReturnValue = Platform_StringCopy(wszCurrentModuleNormalized, &unCurrentModuleNormalizedLen, pwszStart);
							if (RC_SUCCESS != unReturnValue)
								break;
						}
						else
						{
							unReturnValue = Platform_StringCopy(wszCurrentModuleNormalized, &unCurrentModuleNormalizedLen, wszCurrentModule);
							if (RC_SUCCESS != unReturnValue)
								break;
						}
					}

					if (RC_SUCCESS == unReturnValue)
						unReturnValue = Platform_AnsiString2UnicodeString(wszCurrentFunction, RG_LEN(wszCurrentFunction), PszCurrentFunction);

					if ((RC_SUCCESS == unReturnValue) && !PLATFORM_STRING_IS_NULL_OR_EMPTY(wszCurrentModuleNormalized) && !PLATFORM_STRING_IS_NULL_OR_EMPTY(wszCurrentFunction))
					{
						unReturnValue = FileIO_WriteStringf(pFile, L"%ls - %ls - ", wszCurrentModuleNormalized, wszCurrentFunction);
						if (RC_SUCCESS != unReturnValue)
							break;
					}
				}

				// Skip empty lines
				if (wszLine[0] != L'\0')
				{
					unReturnValue = FileIO_WriteString(pFile, wszLine);
					if (RC_SUCCESS != unReturnValue)
						break;
				}

				// Add new line before logging the next line.
				// Needs \n as format string so it will be auto-converted to \r\n on UEFI.
				unReturnValue = FileIO_WriteStringf(pFile, L"\n");
				if (RC_SUCCESS != unReturnValue)
					break;
			}
			WHILE_TRUE_END;
		}
		WHILE_FALSE_END;

		// Close file if it is open
		if (NULL != pFile)
			unReturnValue = FileIO_Close(&pFile);

		// Free allocated memory
		Platform_MemoryFree((void**)&wszLine);

		// Signal that logging is finished
		s_fInLogging = FALSE;
	}

	return unReturnValue;
}

/**
 *	@brief		Logging function
 *	@details	Writes the given text into the configured log
 *
 *	@param		PszCurrentModule		Pointer to a char array holding the module name (optional, can be NULL)
 *	@param		PszCurrentFunction		Pointer to a char array holding the function name (optional, can be NULL)
 *	@param		PunLoggingLevel			Logging level
 *	@param		PwszLoggingMessage		Format string used to format the message
 *	@param		...						Parameters needed to format the message
 */
void
Logging_WriteLog(
	_In_z_	const char*		PszCurrentModule,
	_In_z_	const char*		PszCurrentFunction,
	_In_	unsigned int	PunLoggingLevel,
	_In_z_	const wchar_t*	PwszLoggingMessage,
	...)
{
	unsigned int unReturnValue = RC_E_FAIL;
	unsigned int unConfiguredLoggingLevel = 0;

	do
	{
		// Get Logging level
		if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOGGING_LEVEL, &unConfiguredLoggingLevel))
			break;

		if (PunLoggingLevel <= unConfiguredLoggingLevel)
		{
			va_list argptr;

			// Write log message with variable arguments to log file
			// Get pointer to variable arguments
			if (NULL != PwszLoggingMessage)
			{
				wchar_t wszMessage[MAX_MESSAGE_SIZE] = {0};
				unsigned int unMessageSize = RG_LEN(wszMessage);

				// Skip formating if a empty line should be written
				if (PwszLoggingMessage[0] != L'\0')
				{
					// Format message
					va_start(argptr, PwszLoggingMessage);
					unReturnValue = Platform_StringFormatV(wszMessage, &unMessageSize, PwszLoggingMessage, argptr);
					va_end(argptr);
					if (RC_SUCCESS != unReturnValue)
						break;
				}
				else
					unMessageSize = 0;

				// Write actual message line by line
				unReturnValue = Logging_WriteMessage(
									PszCurrentModule,
									PszCurrentFunction,
									unConfiguredLoggingLevel,
									wszMessage, unMessageSize + 1);
				if (RC_SUCCESS != unReturnValue)
					break;
			}
		}
	}
	WHILE_FALSE_END;
}

/**
 *	@brief		Log hex dump function
 *	@details	Writes the given data as hex dump to the file
 *
 *	@param		PszCurrentModule		Pointer to a char array holding the module name (optional, can be NULL)
 *	@param		PszCurrentFunction		Pointer to a char array holding the function name (optional, can be NULL)
 *	@param		PunLoggingLevel			Logging level
 *	@param		PrgbHexData				Format string used to format the message
 *	@param		PunSize					Size of hex data buffer
 */
void
Logging_WriteHex(
	_In_z_					const char*		PszCurrentModule,
	_In_z_					const char*		PszCurrentFunction,
	_In_					unsigned int	PunLoggingLevel,
	_In_bytecount_(PunSize)	const BYTE*		PrgbHexData,
	_In_					unsigned int	PunSize)
{
	unsigned int unReturnValue = RC_E_FAIL;
	unsigned int unConfiguredLoggingLevel = 0;

	do
	{
		// Check parameters
		if (NULL == PrgbHexData || 0 == PunSize)
			break;

		// Get Logging level
		if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOGGING_LEVEL, &unConfiguredLoggingLevel))
			break;

		if (PunLoggingLevel <= unConfiguredLoggingLevel)
		{
			wchar_t wszFormatedHexData[MAX_MESSAGE_SIZE] = {0};
			unsigned int unFormatedHexDataSize = RG_LEN(wszFormatedHexData);

			unReturnValue = Utility_StringWriteHex(
								PrgbHexData, PunSize,
								wszFormatedHexData, &unFormatedHexDataSize);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Write actual message line by line
			unReturnValue = Logging_WriteMessage(
								PszCurrentModule,
								PszCurrentFunction,
								unConfiguredLoggingLevel,
								wszFormatedHexData, unFormatedHexDataSize + 1);
			if (RC_SUCCESS != unReturnValue)
				break;
		}
	}
	WHILE_FALSE_END;
}
