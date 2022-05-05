/**
 *	@brief		Declares utility methods
 *	@details	This module provides helper functions for common and platform independent use
 *	@file		Utility.h
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
#include "Platform.h"

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------------------->
// String Functions
//----------------------------------------------------------------------------------------------

/// The maximum length of the manufacturer string returned by the TPM
#define MANU_LENGTH 5

/// The maximum length of the date string returned by the TPM (length must fit also for output in case of parsing errors)
#define DATE_LENGTH 16

/// The maximum length of the time string returned by the TPM (length must fit also for output in case of parsing errors)
#define TIME_LENGTH 16

/// The maximum length of the TimeStamp string
#define TIMESTAMP_LENGTH 30

/**
 *	@brief		Gets a line from a buffer.
 *	@details	This function searches for a line end ('\n' or '\0'), allocates and copies the line
 *
 *	@param		PwszBuffer				String buffer to get the line from
 *	@param		PunSize					Length of the string buffer in elements (incl. zero termination)
 *	@param		PpunIndex				In: Position to start searching in the string buffer\n
 *										Out: Position in the string for the next search
 *	@param		PpwszLine				Pointer to a string buffer. The function allocates memory for a
 *										line string buffer and sets this pointer to the address of the
 *										allocated memory. The caller must free the allocated line buffer!
 *	@param		PpunLineSize			The length of the line in elements including the zero termination.
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_END_OF_STRING		In case the end of the string was reached
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_StringGetLine(
	_In_z_count_(PunSize)			const wchar_t*	PwszBuffer,
	_In_							unsigned int	PunSize,
	_Inout_							unsigned int*	PpunIndex,
	_Inout_							wchar_t**		PpwszLine,
	_Out_							unsigned int*	PpunLineSize);

/**
 *	@brief		Returns the index of the first occurrence of the searched character
 *	@details
 *
 *	@param		PwszBuffer				Wide character buffer to search through
 *	@param		PunSize					Length of the string buffer in elements (incl. zero termination)
 *	@param		PwchSearch				Wide character to search for
 *	@param		PpnIndex				-1 if the searched character cannot be found,
 *										otherwise the index of the first occurrence in the string buffer
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_StringContainsWChar(
	_In_z_count_(PunSize)			const wchar_t*	PwszBuffer,
	_In_							unsigned int	PunSize,
	_In_							wchar_t			PwchSearch,
	_Out_							int*			PpnIndex);

/**
 *	@brief		Convert TPM ASCII value to string
 *	@details	Used to show string instead of HEX value
 *
 *	@param		PwszBuffer				Pointer to text string (destination)
 *	@param		PunBufferSize			Size of PwszBuffer in elements (incl. zero termination)
 *	@param		PpunValue				Pointer to value to convert (source)
 *	@param		...						Variable arguments list
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. PwszBuffer is NULL.
 *	@retval		RC_E_BUFFER_TOO_SMALL	PunBufferSize is too small
 */
_Check_return_
unsigned int
Utility_StringParseTpmAscii(
	_Out_z_cap_(PunBufferSize)	wchar_t*			PpBuffer,
	_In_						unsigned int		PunBufferSize,
	_In_						const unsigned int*	PpunValue,
	...);

/**
 *	@brief		Converts an unsigned long long to a string
 *	@details	This function converts the unsigned long long to a wide character, zero terminated string.
 *
 *	@param		PullValue				Unsigned long long value to convert
 *	@param		PwszBuffer				Pointer to a wide character buffer
 *	@param		PpunBufferSize			Pointer to a unsigned long long
 *										IN: Size of buffer in elements
 *										OUT: Length of written characters without zero termination
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_ULongLong2String(
	_In_							unsigned long long	PullValue,
	_Out_z_cap_(*PpunBufferSize)	wchar_t*			PwszBuffer,
	_Inout_							unsigned int*		PpunBufferSize);

/**
 *	@brief		Converts an unsigned integer to a string
 *	@details	This function converts the unsigned integer to a wide character, zero terminated string.
 *
 *	@param		PunValue				Unsigned integer value to convert
 *	@param		PwszBuffer				Pointer to a wide character buffer
 *	@param		PpunBufferSize			Pointer to a unsigned integer
 *										IN: Size of buffer in elements
 *										OUT: Length of written characters without zero termination
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		...						Error codes from Utility_ULongLong2String
 */
_Check_return_
unsigned int
Utility_UInteger2String(
	_In_							unsigned int	PunValue,
	_Out_z_cap_(*PpunBufferSize)	wchar_t*		PwszBuffer,
	_Inout_							unsigned int*	PpunBufferSize);

/**
 *	@brief		Converts a hex string with a leading '0x' or '0X' header to an unsigned integer
 *	@details	This function converts a hex string with a leading '0x' or '0X' header to an unsigned integer
 *
 *	@param		PwszBuffer				Pointer to a wide character buffer containing the hex value
 *	@param		PpunValue				Pointer to an unsigned integer
 *										OUT: The value of the input string
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		In case of an invalid character in the input string or if the hex value
 *										overflows the unsigned integer range
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_UIntegerParseHexString(
	_In_z_	const wchar_t*	PwszBuffer,
	_Out_	unsigned int*	PpunValue);

/**
 *	@brief		Converts a string to an unsigned integer value
 *	@details	This function converts the wide characters from a string to an unsigned decimal number, if possible
 *
 *	@param		PwszBuffer				Pointer to the wide character string buffer
 *	@param		PunMaximumCapacity		Maximum capacity of the string buffer in elements (including terminating zero)
 *	@param		PpunNumber				Pointer to an integer variable
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		In case of an invalid character in the input string or if the hex value
 *										overflows the unsigned integer range
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_StringParseUInteger(
	_In_z_count_(PunMaximumCapacity)	const wchar_t*	PwszBuffer,
	_In_								unsigned int	PunMaximumCapacity,
	_Out_								unsigned int*	PpunNumber);

/**
 *	@brief		Scans a hex string into a byte array
 *	@details
 *
 *	@param		PwszSource				Source string containing the hex representation (no white spaces)
 *										Function stops scanning when the first element cannot be converted to a byte value.
 *	@param		PrgbDestination			Pointer to a byte array to fill in the scanned bytes
 *	@param		PpunDestinationSize		In:		Capacity of the destination buffer in bytes\n
 *										Out:	Count of written bytes
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		...						Error codes from Platform_StringGetLength
 */
_Check_return_
unsigned int
Utility_StringScanHexToByte(
	_In_z_								const wchar_t*	PwszSource,
	_Out_bytecap_(*PpunDestinationSize)	BYTE*			PrgbDestination,
	_Inout_								unsigned int*	PpunDestinationSize);

/**
 *	@brief		Scans a byte array into a hex string
 *	@details
 *
 *	@param		PrgbSource				Pointer to a byte array containing TPM response
 *	@param		PunSourceSize			Size of the source byte array
 *	@param		PwszDestination			Pointer to a string buffer for the hex representation
 *	@param		PpunDestinationSize		In:		Capacity of the destination buffer\n
 *										Out:	Count of written bytes
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case the destination buffer is too small
 */
_Check_return_
unsigned int
Utility_StringScanByteToHex(
	_In_bytecount_(PunSourceSize)		const BYTE*		PrgbSource,
	_In_								unsigned int	PunSourceSize,
	_Out_z_cap_(*PpunDestinationSize)	wchar_t*		PwszDestination,
	_Inout_								unsigned int*	PpunDestinationSize);

/**
 *	@brief		Removes white spaces, tabs, CR and LF from the string
 *	@details
 *
 *	@param		PwszBuffer				Pointer to a buffer where all white spaces and tabs should be removed
 *	@param		PpunBufferCapacity		IN: Capacity in elements including the terminating zero
  *										OUT: String length without zero termination
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		...						Error codes from Platform_StringGetLength
 */
_Check_return_
unsigned int
Utility_StringRemoveWhiteChars(
	_Inout_z_	wchar_t*		PwszBuffer,
	_In_		unsigned int*	PpunBufferCapacity);

/**
 *	@brief		Prints data from a byte array to a string in formatted HEX style
 *	@details
 *
 *	@param		PrgbHexData					Pointer to a buffer with the data
 *	@param		PunSize						Count of byte in the input buffer in elements
 *	@param		PwszFormattedHexData		Pointer to the buffer containing the formatted hex data
 *	@param		PpunFormattedHexDataSize	In:		Capacity of the destination buffer\n
 *											Out:	Count of written bytes
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function. It was either NULL or invalid.
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 *	@retval		...							Error codes from Platform_StringFormat
 */
_Check_return_
unsigned int
Utility_StringWriteHex(
	_In_bytecount_(PunSize)						const BYTE*		PrgbHexData,
	_In_										unsigned int	PunSize,
	_Out_z_cap_(*PpunFormattedHexDataSize)		wchar_t*		PwszFormattedHexData,
	_Inout_										unsigned int*	PpunFormattedHexDataSize);

/**
 *	@brief		Splits the given wide character Line
 *	@details	The function splits up large lines regarding the maximum line character count and
 *				adds to every line a given bunch of characters in front.
 *
 *	@param		PwszSource					Wide character buffer holding the string to format
 *	@param		PunSourceSize				Source wide character buffer size in elements including the zero termination
 *	@param		PwszPreSet					Wide character buffer which is placed in front of every line
 *	@param		PunPreSetSize				Preset wide character buffer size in elements including the zero termination
 *	@param		PunLineSize					Maximum wide character count per line
 *	@param		PwszDestination				Pointer to a wide character buffer to receive the result of the format
 *	@param		PpunDestinationSize			In:		Capacity of the destination wide character buffer in elements including zero termination\n
 *											Out:	Count of written wide characters to the buffer without zero termination
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function. It was either NULL or invalid.
 *	@retval		RC_E_BUFFER_TOO_SMALL		In case the destination buffer is too small
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
Utility_StringFormatOutput_HandleLineSplit(
	_In_z_count_(PunSourceSize)			const wchar_t*	PwszSource,
	_In_								unsigned int	PunSourceSize,
	_In_z_count_(PunPreSetSize)			const wchar_t*	PwszPreSet,
	_In_								unsigned int	PunPreSetSize,
	_In_								unsigned int	PunLineSize,
	_Out_z_cap_(*PpunDestinationSize)	wchar_t*		PwszDestination,
	_Inout_								unsigned int*	PpunDestinationSize);

/**
 *	@brief		Formats the given wide character source
 *	@details	The function splits up large lines regarding the maximum line character count and
 *				adds to every line a given bunch of characters in front. The function allocates a
 *				return wide character buffer. The caller must free this memory.
 *
 *	@param		PwszSource					Wide character buffer holding the string to format
 *	@param		PunSourceSize				Source wide character buffer size in elements including the zero termination
 *	@param		PwszPreSet					Wide character buffer which is placed in front of every line
 *	@param		PunPreSetSize				Preset wide character buffer size in elements including the zero termination
 *	@param		PunLineSize					Maximum wide character count per line
 *	@param		PpwszDestination			Pointer to a wide character buffer including the result of the format. Will be allocated by the callee, but must be freed by the caller with Platform_MemoryFree().
 *	@param		PpunDestinationLength		Count of written wide characters to the buffer without zero termination
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function. It was either NULL or invalid.
 *	@retval		RC_E_BUFFER_TOO_SMALL		In case the destination buffer is too small
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
Utility_StringFormatOutput(
	_In_z_count_(PunSourceSize)	const wchar_t*	PwszSource,
	_In_						unsigned int	PunSourceSize,
	_In_z_count_(PunPreSetSize)	const wchar_t*	PwszPreSet,
	_In_						unsigned int	PunPreSetSize,
	_In_						unsigned int	PunLineSize,
	_Out_						wchar_t**		PpwszDestination,
	_Out_						unsigned int*	PpunDestinationLength);

/**
 *	@brief		Removes comments from a line.
 *	@details	This function takes a line from a TPM command file
 *				and removes all comments.
 *
 *	@param		PwszLineBuffer			Pointer to a buffer containing a line from command file
 *	@param		PfBlockComment			Flag, TRUE when a comment block was started, otherwise FALSE
 *	@param		PpunLineBufferCapacity	IN: Capacity of the buffer in elements including zero termination
 *										OUT: Length of string without terminating zero
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was NULL.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
Utility_StringRemoveComment(
	_Inout_updates_z_(*PpunLineBufferCapacity)	wchar_t*		PwszLineBuffer,
	_Inout_										BOOL*			PfBlockComment,
	_Inout_										unsigned int*	PpunLineBufferCapacity);

/**
 *	@brief		Formats a time-stamp structure to an output string.
 *	@details	The output string can contain date information or not.
 *
 *	@param		PpTime					Pointer to a IfxTime structure, containing time-stamp info.
 *	@param		PfDate					If '1' the output contains a date, if '0' not.
 *	@param		PwszValue				Pointer to a wide character buffer to fill in the value.
 *	@param		PpunValueSize			In:		Size of the value buffer in elements including the zero termination.\n
 *										Out:	Length of the filled in value in elements without zero termination.
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case of any output buffer size is too small
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_Timestamp2String(
	_In_						const IfxTime*	PpTime,
	_In_						BOOL			PfDate,
	_Out_z_cap_(*PpunValueSize)	wchar_t*		PwszValue,
	_Inout_						unsigned int*	PpunValueSize);

/**
 *	@brief		Gets a time-stamp with or without date as string.
 *	@details	The output string can contain date information or not.
 *
 *	@param		PfDate					If '1' the output contains a date, if '0' not.
 *	@param		PwszValue				Pointer to a wide character buffer to fill in the value.
 *	@param		PpunValueSize			In:		Size of the value buffer in elements including the zero termination.\n
 *										Out:	Length of the filled in value in elements without zero termination.
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case of any output buffer size is too small
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_GetTimestamp(
	_In_						BOOL			PfDate,
	_Out_z_cap_(*PpunValueSize)	wchar_t*		PwszValue,
	_Inout_						unsigned int*	PpunValueSize);

/**
 *	@brief		Gets a integer version values out of a string.
 *	@details	The output structure can contain version information or not.
 *
 *	@param		PwszVersionName			Pointer to wide character string representing the version number.
 *	@param		PunVersionNameSize		Pointer to size of the value buffer in elements including the zero termination.\n
 *	@param		PppVersionData			Pointer to a structure of version data
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_StringParseIfxVersion(
	_In_z_count_(PunVersionNameSize)	wchar_t*		PwszVersionName,
	_In_								unsigned int	PunVersionNameSize,
	_Inout_								IfxVersion*		PppVersionData);
#ifndef NO_INI_FILES

//<---------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------->
// INI File Functions
//----------------------------------------------------------------------------------------------

/**
 *	@brief		Checks if the string contains an INI section
 *	@details
 *
 *	@param		PwszBuffer				Wide character buffer to search through
 *	@param		PunSize					Length of the string buffer in elements (incl. zero termination)
 *	@param		PpfIsSection			Pointer to an BOOL to store if it is a section or not
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_IniIsSection(
	_In_z_count_(PunSize)			const wchar_t*	PwszBuffer,
	_In_							unsigned int	PunSize,
	_Out_							BOOL*			PpfIsSection);

/**
 *	@brief		Gets the name of an INI section
 *	@details
 *
 *	@param		PwszBuffer				Wide character buffer (line) to search through
 *	@param		PunSize					Length of the string buffer in elements (incl. zero termination)
 *	@param		PwszSectionName			Pointer to a wide char array to retrieve the section name.
 *										If the function fails and the pointer is not NULL the function will
 *										return an empty (zero terminated) buffer.
 *	@param		PpunSectionNameSize		In:		Size of the wide character buffer in elements including the zero termination\n
 *										Out:	The length of the section name without zero termination.
 *												If the function fails the length will be set to zero.
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case the out buffer is too small
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_IniGetSectionName(
	_In_z_count_(PunSize)				const wchar_t*	PwszBuffer,
	_In_								unsigned int	PunSize,
	_Out_z_cap_(*PpunSectionNameSize)	wchar_t*		PwszSectionName,
	_Inout_								unsigned int*	PpunSectionNameSize);

/**
 *	@brief		Checks if the string contains an INI key/value pair
 *	@details
 *
 *	@param		PwszBuffer				Wide character buffer to search through
 *	@param		PunSize					Length of the string buffer in elements
 *	@param		PpfIsKeyValue			Pointer to an BOOL to store if it is a key/value pair or not
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_IniIsKeyValue(
	_In_z_count_(PunSize)			const wchar_t*	PwszBuffer,
	_In_							unsigned int	PunSize,
	_Out_							BOOL*			PpfIsKeyValue);

/**
 *	@brief		Checks if the string contains an INI key/value pair
 *	@details
 *
 *	@param		PwszBuffer				Wide character buffer to search through
 *	@param		PunSize					Length of the string buffer in elements
 *	@param		PwszKey					Pointer to a wide character buffer to fill in the key.
 *	@param		PpunKeySize				In:		Size of the key buffer in elements including zero termination.\n
 *										Out:	Length of the filled in key in elements without zero termination.
 *	@param		PwszValue				Pointer to a wide character buffer to fill in the value.
 *	@param		PpunValueSize			In:		Size of the value buffer in elements including the zero termination.\n
 *										Out:	Length of the filled in value in elements without zero termination.
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case of any output buffer size is too small
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_IniGetKeyValue(
	_In_z_count_(PunSize)		const wchar_t*	PwszBuffer,
	_In_						unsigned int	PunSize,
	_Out_z_cap_(*PpunKeySize)	wchar_t*		PwszKey,
	_Inout_						unsigned int*	PpunKeySize,
	_Out_z_cap_(*PpunValueSize)	wchar_t*		PwszValue,
	_Inout_						unsigned int*	PpunValueSize);
#endif // NO_INI_FILES

/**
 *	@brief		Set the unsigned long long value of an element identified by a key.
 *	@details
 *
 *	@param		PwszKey			Key identifier for the PropertyElement to be set\n
 *								null-terminated wide char array; max length MAX_NAME
 *	@param		PullValue		Unsigned long long value to be set
 *
 *	@retval		TRUE		If the element value has been set
 *	@retval		FALSE		Internal error
 */
_Check_return_
BOOL
Utility_PropertyStorage_SetULongLongValueByKey(
	_In_z_	const wchar_t*		PwszKey,
	_In_	unsigned long long	PullValue);

/**
 *	@brief		Converts a string to an unsigned long long value
 *	@details	This function converts the wide characters from a string to an unsigned long long number, if possible
 *
 *	@param		PwszBuffer				Pointer to the wide character string buffer
 *	@param		PunMaximumCapacity		Maximum capacity of the string buffer in elements (including terminating zero)
 *	@param		PpullNumber				Pointer to an unsigned long long variable
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function. Or the parameter overflows UINT64_MAX.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 */
_Check_return_
unsigned int
Utility_StringParseULongLong(
	_In_z_count_(PunMaximumCapacity)	const wchar_t*		PwszBuffer,
	_In_								unsigned int		PunMaximumCapacity,
	_Out_								unsigned long long*	PpullNumber);

//<---------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------->
// File Functions
//----------------------------------------------------------------------------------------------

#ifndef IFXTPMUpdate

/**
 *	@brief		Check if the configured log file path is accessible and writable
 *	@details
 *
 *	@param		PfRemoveFileIfNotExistsBefore	TRUE: Remove file after check if it does not exist before
 *												FALSE: Do not remove the file anyway
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_FAIL				An unexpected error occurred.
 *	@retval		RC_E_FILE_NOT_FOUND		In case of the log file cannot be created
 *	@retval		RC_E_ACCESS_DENIED		In case of the file is not writable
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
Utility_CheckIfLogPathWritable(
	BOOL PfRemoveFileIfNotExistsBefore);
#endif

//<---------------------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
