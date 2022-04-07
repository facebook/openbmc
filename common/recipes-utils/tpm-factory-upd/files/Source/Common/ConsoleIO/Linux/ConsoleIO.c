/**
 *	@brief		Implements the ConsoleIO interface for Linux
 *	@details	All ConsoleIO functions which are platform specific
 *	@file		Linux\ConsoleIO.c
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
#include <stdarg.h>
#include <sys/ioctl.h>
#include "ConsoleIO.h"

unsigned int g_unPageBreakCount = 0;
unsigned int g_unPageBreakMax = 25;

int g_nKeyboardInitialized = 0;
struct termios g_initial_settings;

/**
 *	@brief		Configures the screen buffer dimensions.
 *	@details	Sets the screen dimensions depending on the given mode
 *
 *	@param		PunMode	Screen resolution:
 *					0 means standard buffer
 *					1 (CONSOLE_BUFFER_BIG) means big screen buffer with more columns and lines
 *					-1 (CONSOLE_BUFFER_NONE) means unchanged console buffer
 *	@returns	No return value
 */
void
ConsoleIO_InitConsole(
	_In_ unsigned int PunMode)
{
	UNREFERENCED_PARAMETER(PunMode);
	// Get the window size
	struct winsize windowSize;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &windowSize);
	g_unPageBreakMax = windowSize.ws_row;

	setlocale(LC_ALL, "");
}

/**
 *	@brief		Changes the console configuration to interactive mode.
 *	@details	In interactive mode any keys pressed shall not be displayed on the console.
 */
void
ConsoleIO_SetInteractiveMode()
{
	// Initialize keyboard
	if (0 == g_nKeyboardInitialized)
	{
		struct termios new_settings;
		tcgetattr(STDIN_FILENO, &g_initial_settings);
		new_settings = g_initial_settings;
		new_settings.c_lflag &= ~ICANON;
		new_settings.c_lflag &= ~ECHO;
		tcsetattr(STDIN_FILENO, TCSANOW, &new_settings);
		setbuf(stdin, NULL);
		g_nKeyboardInitialized = 1;
	}
}

/**
 *	@brief		Reset settings to initial values
 *	@details	The function resets the initial values changed for the tool.
 */
void
ConsoleIO_UnInitConsole()
{
	// Uninitialize keyboard
	if (1 == g_nKeyboardInitialized)
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &g_initial_settings);
		g_nKeyboardInitialized = 0;
	}
}

/**
 *	@brief		Clears the screen.
 *	@details
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
ConsoleIO_ClearScreen()
{
	unsigned int unIndex = 0;
	// Move actual position out of screen scope
	for(unIndex = 0; unIndex < g_unPageBreakMax; unIndex++)
	{
		unsigned int unReturnValue = ConsoleIO_WritePlatform(TRUE, L"");
		if (RC_SUCCESS != unReturnValue)
			break;
	}
	// Set cursor position to (0;0)
	wprintf(L"\033[0;0H");
	g_unPageBreakCount = 0;
	return RC_SUCCESS;
}

/**
 *	@brief		Checks the console for keyboard input.
 *	@details
 *
 *	@returns	Returns a nonzero value if a key has been pressed. Otherwise, it returns 0.
 */
_Check_return_
int
ConsoleIO_KeyboardHit()
{
	int nBytesWaiting = 0;
	ioctl(STDIN_FILENO, FIONREAD, &nBytesWaiting);

	return nBytesWaiting;
}

/**
 *	@brief		Gets a wide character from the console without echo.
 *	@details
 *
 *	@returns	Returns the wide character read. There is no error return.
 */
_Check_return_
wchar_t
ConsoleIO_ReadWChar()
{
	wchar_t wchRead = getwchar();
	// Check if a key stroke sequence is following
	if (wchRead == 0x001b)
	{
		int nBytesWaiting = ConsoleIO_KeyboardHit();
		// If sequence is detected read out all chars from stdin and ignore them
		if (nBytesWaiting != 0)
		{
			int nIndex = 0;
			for (nIndex = 0; nIndex < nBytesWaiting; nIndex++)
			{
				getwchar();
			}
			wchRead = EOF;
		}
	}
	return wchRead;
}

/**
 *	@brief		Prints formatted output
 *	@details	Prints formatted output to the standard output stream.
 *				No page break handling.
 *
 *	@param		PfNewLine					Set a new line if TRUE.
 *	@param		PwszFormat					Format control.
 *	@param		PargList					Optional argument list.
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function.
 *	@retval		RC_E_BUFFER_TOO_SMALL		In case of insufficient buffer
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 */
_Check_return_
unsigned int
ConsoleIO_WritePlatformV(
	_In_	BOOL			PfNewLine,
	_In_z_	const wchar_t*	PwszFormat,
	_In_	va_list			PargList)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		int	nWritten = 0;
		wchar_t wszBuf[PRINT_BUFFER_SIZE] = {0};

		// Check Parameter
		if (NULL == PwszFormat)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Prepare string to write in buffer szBuf
		nWritten = vswprintf(wszBuf, RG_LEN(wszBuf), PwszFormat, PargList);

		switch (nWritten)
		{
			case -1:
				// Buffer is too small write what fits to the buffer and return
				// insufficient buffer error
				wprintf(wszBuf);
				if (PfNewLine)
					wprintf(L"\n");

				unReturnValue = RC_E_BUFFER_TOO_SMALL;
				break;

			default:
				// Message was formatted correctly
				// print it to the stdout
				wprintf(wszBuf);
				if (PfNewLine)
					wprintf(L"\n");

				unReturnValue = RC_SUCCESS;
				break;
		}
		fflush(stdout);
	}
	WHILE_FALSE_END;

	return unReturnValue;
}
