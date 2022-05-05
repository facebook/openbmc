/**
 *	@brief		Declares the common command line module
 *	@details	This module contains common methods for the command line module
 *	@file		CommandLine.h
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
#include "ICommandLineParser.h"

/**
 *	@brief		Parses the command line
 *	@details	Puts all parameters from the command line into a Command Line Parameters structure.
 *
 *	@param		PnArgc		Number of command line arguments
 *	@param		PrgwszArgv	Array of pointers to each command line argument
 *	@retval		RC_SUCCESS	The operation completed successfully.
 *	@retval		...			Error codes from called functions.
 */
_Check_return_
unsigned int
CommandLine_Parse(
	_In_					int						PnArgc,
	_In_reads_z_(PnArgc)	const wchar_t* const	PrgwszArgv[]);

/**
 *	@brief		Check that the command is an option (i.e. that it starts with '-')
 *	@details
 *
 *	@param		PwszCommand				The input buffer
 *	@param		PpIsCommandLine			Flag if it is a valid command line option or not
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PwszCommand, PwszOutCommand or PpunOutBufferSize is NULL
 *	@retval		...						Error codes from called functions
 */
_Check_return_
BOOL
CommandLine_IsCommandLineOption(
	_In_z_	const wchar_t*	PwszCommand,
	_Inout_	BOOL*			PpIsCommandLine);
/**
 *	@brief		Removes the option character from the string.
 *	@details
 *
 *	@param		PwszCommand				The input buffer
 *	@param		PwszOutCommand			The output buffer
 *	@param		PpunOutBufferSize		The output buffer size
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PwszCommand, PwszOutCommand or PpunOutBufferSize is NULL
 *	@retval		RC_E_BUFFER_TOO_SMALL	*PpunOutBufferSize is 0
 */
_Check_return_
unsigned int
CommandLine_GetTrimmedCommand(
	_In_z_								const wchar_t*	PwszCommand,
	_Out_z_cap_(*PpunOutBufferSize)		wchar_t*		PwszOutCommand,
	_Inout_								unsigned int*	PpunOutBufferSize);

#ifdef __cplusplus
}
#endif