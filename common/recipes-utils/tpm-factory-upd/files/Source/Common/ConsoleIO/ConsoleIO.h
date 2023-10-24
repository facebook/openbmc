/**
 *  @brief      Declares the ConsoleIO interface
 *  @details    Includes all console operations which need to be implemented in platform specific source modules.
 *  @file       ConsoleIO.h
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
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

extern unsigned int g_unPageBreakCount;
extern unsigned int g_unPageBreakMax;

/// Macro define which checks the return of ConsoleIO_Write.
/// Preconditions do WHILE_FALSE_END loop and defined unReturnValueWrite variable
#define CONSOLEIO_WRITE_BREAK_FMT(PAGEBREAK,FORMAT,...) \
    LOGGING_WRITE_LEVEL2_FMT(FORMAT, __VA_ARGS__); \
    unReturnValueWrite = ConsoleIO_Write(PAGEBREAK, TRUE, FORMAT, __VA_ARGS__); \
    if (RC_SUCCESS != unReturnValueWrite) \
        break; \

#define CONSOLEIO_WRITE_BREAK(PAGEBREAK,FORMAT) \
    LOGGING_WRITE_LEVEL2(FORMAT); \
    unReturnValueWrite = ConsoleIO_Write(PAGEBREAK, TRUE, FORMAT, NULL); \
    if (RC_SUCCESS != unReturnValueWrite) \
        break; \

#define CONSOLEIO_WRITE_BREAK_FMT_NO_NEWLINE(PAGEBREAK,FORMAT,...) \
    LOGGING_WRITE_LEVEL2_FMT(FORMAT, __VA_ARGS__); \
    unReturnValueWrite = ConsoleIO_Write(PAGEBREAK, FALSE, FORMAT, __VA_ARGS__); \
    if (RC_SUCCESS != unReturnValueWrite) \
        break; \

/// Macro define which checks the return of ConsoleIO_Write.
/// Preconditions defined unReturnValueWrite variable
#define CONSOLEIO_WRITE_FMT(PAGEBREAK,FORMAT,...) \
    LOGGING_WRITE_LEVEL2_FMT(FORMAT, __VA_ARGS__); \
    unReturnValueWrite = ConsoleIO_Write(PAGEBREAK, TRUE, FORMAT, __VA_ARGS__); \

#define CONSOLEIO_WRITE(PAGEBREAK,FORMAT) \
    LOGGING_WRITE_LEVEL2(FORMAT); \
    unReturnValueWrite = ConsoleIO_Write(PAGEBREAK, TRUE, FORMAT, NULL); \

/// "Defines"
/// Big screen buffer - horizontal size
#define SCREEN_BUFFER_SIZE_X    300
/// Big screen buffer - vertical size
#define SCREEN_BUFFER_SIZE_Y    1000
/// Print buffer size
#define PRINT_BUFFER_SIZE       4096

/**
 *  @brief      Configures the screen buffer dimensions.
 *  @details    Sets the screen dimensions depending on the given mode
 *
 *  @param      PunMode Screen resolution:
 *                  0 means standard buffer
 *                  1 (CONSOLE_BUFFER_BIG) means big screen buffer with more columns and lines
 *                  -1 (CONSOLE_BUFFER_NONE) means unchanged console buffer
 */
void
ConsoleIO_InitConsole(
    _In_ unsigned int PunMode);

/**
 *  @brief      Changes the console configuration to interactive mode.
 *  @details    In interactive mode any keys pressed shall not be displayed on the console.
 */
void
ConsoleIO_SetInteractiveMode();

/**
 *  @brief      Reset settings to initial values
 *  @details    The function resets the initial values changed for the tool.
 */
void
ConsoleIO_UnInitConsole();

/**
 *  @brief      Clears the screen.
 *  @details
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_FAIL           An unexpected error occurred.
 */
_Check_return_
unsigned int
ConsoleIO_ClearScreen();

/**
 *  @brief      Checks the console for keyboard input.
 *  @details
 *
 *  @returns    Returns a nonzero value if a key has been pressed. Otherwise, it returns 0.
 */
_Check_return_
int
ConsoleIO_KeyboardHit();

/**
 *  @brief      Gets a wide character from the console without echo.
 *  @details
 *
 *  @returns    Returns the wide character read. There is no error return.
 */
_Check_return_
wchar_t
ConsoleIO_ReadWChar();

/**
 *  @brief      Prints format output
 *  @details    Prints formatted output to the standard output stream.
 *              Handles the page break functionality.
 *
 *  @param      PfPageBreak             If TRUE page break is activated, if FALSE not.
 *  @param      PfNewLine               Set a new line if TRUE.
 *  @param      PwszFormat              Format control.
 *  @param      ...                     Optional arguments.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case of insufficient buffer.
 *  @retval     RC_E_INTERNAL           An internal error occurred.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
IFXAPI
ConsoleIO_Write(
    _In_    BOOL            PfPageBreak,
    _In_    BOOL            PfNewLine,
    _In_z_  const wchar_t*  PwszFormat,
    ...);

/**
 *  @brief      Prints formatted output
 *  @details    Prints formatted output to the standard output stream.
 *              No page break handling.
 *
 *  @param      PfNewLine   Set a new line if TRUE.
 *  @param      PwszFormat              Format control.
 *  @param      ...                     Optional arguments.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case of insufficient buffer.
 *  @retval     RC_E_INTERNAL           An internal error occurred.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
IFXAPI
ConsoleIO_WritePlatform(
    _In_    BOOL            PfNewLine,
    _In_z_  const wchar_t*  PwszFormat,
    ...);

/**
 *  @brief      Prints formatted output
 *  @details    Prints formatted output to the standard output stream.
 *              No page break handling.
 *
 *  @param      PfNewLine                   Set a new line if TRUE.
 *  @param      PwszFormat                  Format control.
 *  @param      PargList                    Optional argument list.
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function.
 *  @retval     RC_E_BUFFER_TOO_SMALL       In case of insufficient buffer.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 */
_Check_return_
unsigned int
ConsoleIO_WritePlatformV(
    _In_    BOOL            PfNewLine,
    _In_z_  const wchar_t*  PwszFormat,
    _In_    va_list         PargList);

/**
 *  @brief      Handles the page break functionality
 *  @details    Handles the page break functionality and prints one line to the console out.
 *
 *  @param      PfNewLine               Set a new line if TRUE.
 *  @param      PwszMessage             Message (line) to print.
 *  @param      PunMessagesize          Size of the line including zero termination.
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval     ...                     Error codes from called functions.
 */
unsigned int
ConsoleIO_HandlePageBreak(
    _In_                            BOOL            PfNewLine,
    _In_z_count_(PunMessagesize)    const wchar_t*  PwszMessage,
    _In_                            unsigned int    PunMessagesize);

#ifdef __cplusplus
}
#endif
