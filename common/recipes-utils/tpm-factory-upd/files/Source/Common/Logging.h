/**
 *	@brief		Declares logging methods for the application
 *	@details	This module provides logging for the application.
 *	@file		Logging.h
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

#ifdef __cplusplus
extern "C" {
#endif

/// Flag indicating whether to write a header into the log file or not
extern BOOL g_fLogHeader;

/**
 *	Value definitions for logging
 */

/// Log level value definition for no logging
#define LOGGING_DISABLED	0

/// Log level value definition for error logging
#define LOGGING_LEVEL_1		1

/// Log level value definition for additionally copy of screen output
#define LOGGING_LEVEL_2		2

/// Log level value definition for additionally byte streams
#define LOGGING_LEVEL_3		3

/// Log level value definition for additionally debugging information
#define LOGGING_LEVEL_4		4

/// String definition for tracing method entry in log
#define LOGGING_METHOD_ENTRY_STRING			L"-> Method entry"

/// String definition for tracing method exit in log
#define LOGGING_METHOD_EXIT_STRING			L"<- Method exit"

/// String definition for tracing method exit incl. the return value in log
#define LOGGING_METHOD_EXIT_STRING_RET_VAL	L"<- Method exit: 0x%.8X"

/// The maximum HEX byte characters written per line in log file
#define LOGGING_HEX_CHARS_PER_LINE 16

/// Divisor for megabyte
#define DIV_KILOBYTE 1024

/**
 *	Macro definitions for logging
 */

/// Macro for writing a message into the log file
#define LOGGING_WRITE(LOGLEVEL, LOGMESSAGE, ...)	Logging_WriteLog(__FILE__, __func__, LOGLEVEL, LOGMESSAGE, ##__VA_ARGS__);

/// Macro for writing a message into the log file only in case current log level is level 1 or higher
#define LOGGING_WRITE_LEVEL1_FMT(LOGMESSAGE, ...)	Logging_WriteLog(__FILE__, __func__, LOGGING_LEVEL_1, LOGMESSAGE, ##__VA_ARGS__);
#define LOGGING_WRITE_LEVEL1(LOGMESSAGE)			Logging_WriteLog(__FILE__, __func__, LOGGING_LEVEL_1, LOGMESSAGE, NULL);

/// Macro for writing a message into the log file only in case current log level is level 2 or higher
#define LOGGING_WRITE_LEVEL2_FMT(LOGMESSAGE, ...)	Logging_WriteLog(__FILE__, __func__, LOGGING_LEVEL_2, LOGMESSAGE, ##__VA_ARGS__);
#define LOGGING_WRITE_LEVEL2(LOGMESSAGE)			Logging_WriteLog(__FILE__, __func__, LOGGING_LEVEL_2, LOGMESSAGE, NULL);

/// Macro for writing a message into the log file only in case current log level is level 3 or higher
#define LOGGING_WRITE_LEVEL3_FMT(LOGMESSAGE, ...)	Logging_WriteLog(__FILE__, __func__, LOGGING_LEVEL_3, LOGMESSAGE, ##__VA_ARGS__);
#define LOGGING_WRITE_LEVEL3(LOGMESSAGE)			Logging_WriteLog(__FILE__, __func__, LOGGING_LEVEL_3, LOGMESSAGE, NULL);

/// Macro for writing a message into the log file only in case current log level is level 4 or higher
#define LOGGING_WRITE_LEVEL4_FMT(LOGMESSAGE, ...)	Logging_WriteLog(__FILE__, __func__, LOGGING_LEVEL_4, LOGMESSAGE, ##__VA_ARGS__);
#define LOGGING_WRITE_LEVEL4(LOGMESSAGE)			Logging_WriteLog(__FILE__, __func__, LOGGING_LEVEL_4, LOGMESSAGE, NULL);

/// Macro for writing a buffer's contents in hex bytes into the log file
#define LOGGING_WRITEHEX(LOGLEVEL, BUFFER, SIZE)	Logging_WriteHex(__FILE__, __func__, LOGLEVEL, BUFFER, SIZE);

/// Macro for writing a buffer's contents in hex bytes into the log file only in case current log level is level 1 or higher
#define LOGGING_WRITEHEX_LEVEL1(BUFFER, SIZE)		Logging_WriteHex(__FILE__, __func__, LOGGING_LEVEL_1, BUFFER, SIZE);

/// Macro for writing a buffer's contents in hex bytes into the log file only in case current log level is level 3 or higher
#define LOGGING_WRITEHEX_LEVEL3(BUFFER, SIZE)		Logging_WriteHex(__FILE__, __func__, LOGGING_LEVEL_3, BUFFER, SIZE);

/**
 *	Method declarations for logging
 */

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
	...);

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
	_In_					unsigned int	PunSize);

#ifdef __cplusplus
}
#endif
