/**
 *	@brief		Declares application configuration methods
 *	@details	Module to handle the application's configuration
 *	@file		Config.h
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
#include "IConfigSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

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
	_In_z_	const wchar_t*						PwszConfigFileName);

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
	_In_opt_	IConfigSettings_Parse				PpfParse);

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
	_In_opt_						IConfigSettings_Parse	PpfParse);

#ifdef __cplusplus
}
#endif