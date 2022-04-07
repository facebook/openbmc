/**
 *	@brief		Implements all common parts of the ConsoleIO
 *	@details	All ConsoleIO functions which are partly or fully platform independent
 *	@file		ConsoleIO\Common.c
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

#include "ConsoleIO.h"
#include "Platform.h"
#include "Utility.h"

#define MESSAGE_PAGE_BREAK	L"\n   *** Press Any Key ***"

/**
 *	@brief		Handles the page break functionality
 *	@details	Handles the page break functionality and prints one line to the console out.
 *
 *	@param		PfNewLine				Set a new line if TRUE.
 *	@param		PwszMessage				Message (line) to print
 *	@param		PunMessagesize			Size of the line including zero termination
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from called functions.
 */
unsigned int
ConsoleIO_HandlePageBreak(
	_In_							BOOL			PfNewLine,
	_In_z_count_(PunMessagesize)	const wchar_t*	PwszMessage,
	_In_							unsigned int	PunMessagesize)
{
	unsigned int unReturnValue = RC_E_FAIL;
	wchar_t* wszLine = NULL;
	do
	{
		unsigned int unIndex = 0;

		// Check Parameter
		if (NULL == PwszMessage ||
				0 == PunMessagesize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}
		// Loop over all Lines in the message
		do
		{
			unsigned int unLineSize = 0;

			// Free memory before read the next line
			Platform_MemoryFree((void**)&wszLine);

			// Get line from buffer
			unReturnValue = Utility_StringGetLine(
								PwszMessage,
								PunMessagesize,
								&unIndex,
								&wszLine,
								&unLineSize);
			if (RC_SUCCESS != unReturnValue)
			{
				if (RC_E_END_OF_STRING == unReturnValue)
					unReturnValue = RC_SUCCESS;
				break;
			}
			if (NULL == wszLine)
				continue;

			// Check if Page break reached
			if (g_unPageBreakCount + 3 >= g_unPageBreakMax)
			{
				// Show Page Break
				unReturnValue = ConsoleIO_WritePlatform(PfNewLine, MESSAGE_PAGE_BREAK);
				if (RC_SUCCESS != unReturnValue)
					break;

				while (!ConsoleIO_KeyboardHit());
				IGNORE_RETURN_VALUE(ConsoleIO_ReadWChar());

				unReturnValue = ConsoleIO_ClearScreen();
				if (RC_SUCCESS != unReturnValue)
					break;

				g_unPageBreakCount = 0;
			}

			// Print out the line
			unReturnValue = ConsoleIO_WritePlatform(PfNewLine, wszLine);
			if (RC_SUCCESS != unReturnValue)
				break;

			g_unPageBreakCount++;
		}
		while(RC_SUCCESS == unReturnValue);
	}
	WHILE_FALSE_END;

	// Free allocated memory
	Platform_MemoryFree((void**)&wszLine);

	return unReturnValue;
}

/**
 *	@brief		Prints format output
 *	@details	Prints formatted output to the standard output stream.
 *				Handles the page break functionality.
 *
 *	@param		PfPageBreak				If TRUE page break is activated, if FALSE not
 *	@param		PfNewLine				Set a new line if TRUE.
 *	@param		PwszFormat				Format control.
 *	@param		...						Optional arguments.
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case of insufficient buffer
 *	@retval		RC_E_INTERNAL			An internal error occurred.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
ConsoleIO_Write(
	_In_	BOOL			PfPageBreak,
	_In_	BOOL			PfNewLine,
	_In_z_	const wchar_t*	PwszFormat,
	...)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// Check Parameter
		if (NULL == PwszFormat)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Print without Page break or prepare for page break output
		{
			va_list vaArgPtr;

			// Check PageBreak functionality
			if (FALSE == PfPageBreak)
			{
				va_start(vaArgPtr, PwszFormat);
				unReturnValue = ConsoleIO_WritePlatformV(PfNewLine, PwszFormat, vaArgPtr);
				va_end(vaArgPtr);
				break;
			}
			else
			{
				wchar_t wszDestination[MAX_MESSAGE_SIZE] = {0};
				unsigned int unDestinationCapacity = RG_LEN(wszDestination);

				if (PwszFormat[0] != L'\0')
				{
					va_start(vaArgPtr, PwszFormat);
					unReturnValue = Platform_StringFormatV(wszDestination, &unDestinationCapacity, PwszFormat, vaArgPtr);
					va_end(vaArgPtr);
					if (RC_SUCCESS != unReturnValue)
						break;
				}
				else
					unDestinationCapacity = 0;

				unReturnValue = ConsoleIO_HandlePageBreak(PfNewLine, wszDestination, unDestinationCapacity + 1);
			}
		}
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Prints formatted output
 *	@details	Prints formatted output to the standard output stream.
 *				No page break handling.
 *
 *	@param		PfNewLine				Set a new line if TRUE.
 *	@param		PwszFormat				Format control.
 *	@param		...						Optional arguments.
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case of insufficient buffer
 *	@retval		RC_E_INTERNAL			An internal error occurred.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
ConsoleIO_WritePlatform(
	_In_	BOOL			PfNewLine,
	_In_z_	const wchar_t*	PwszFormat,
	...)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		va_list vaArgPtr;

		// Check Parameter
		if (NULL == PwszFormat)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Prepare string to write in buffer szBuf
		va_start(vaArgPtr, PwszFormat);
		unReturnValue = ConsoleIO_WritePlatformV(PfNewLine, PwszFormat, vaArgPtr);
		va_end(vaArgPtr);
	}
	WHILE_FALSE_END;

	return unReturnValue;
}