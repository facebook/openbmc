/**
 *	@brief		Declares the command line parser
 *	@details	This module parses the command line and provides the command line properties
 *	@file		CommandLineParser.h
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

/**
 *	@brief		This function increments the command line option count
 *	@details
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_FAIL			An unexpected error occurred using the property storage.
 */
_Check_return_
unsigned int
CommandLineParser_IncrementOptionCount();

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
	_Inout_							unsigned int*			PpunValueSize);

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
	_In_z_		const wchar_t*	PwszCommand);

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
	_In_z_		const wchar_t*	PwszCommand);

#ifdef __cplusplus
}
#endif
