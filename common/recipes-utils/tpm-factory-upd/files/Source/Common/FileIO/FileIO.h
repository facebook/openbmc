/**
 *	@brief		Declares the FileIO interface
 *	@details	File IO library platform independent
 *	@file		FileIO.h
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
 *	@brief		Open a file
 *	@details	Opens a file with the given name and access rights and returns the handle to it in *PppvFileHandle.
 *
 *	@param		PwszFileName		Filename to open
 *	@param		PppvFileHandle		Pointer where to store the handle to the opened file
 *	@param		PunFileAccessMode	FILE_READ, FILE_WRITE, FILE_APPEND, FILE_READ_BINARY or FILE_WRITE_BINARY
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_FILE_NOT_FOUND	In case of file or directory is not found
 *	@retval		RC_E_BAD_PARAMETER	In case one of the parameters are NULL or empty, *PppvFileHandle is not NULL or PunFileAccessMode is invalid
 *	@retval		RC_E_ACCESS_DENIED	In case the access to the file is denied
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
FileIO_Open(
	_In_z_						const wchar_t*	PwszFileName,
	_Outptr_result_maybenull_	void**			PppvFileHandle,
	_In_						unsigned int	PunFileAccessMode);

/**
 *	@brief		Close a file
 *	@details	Closes the given file handle and sets it to NULL in case of success (leaves it unchanged in case of error).
 *
 *	@param		PppvFileHandle		Handle to an opened file
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		EOF					In case PpvFileHandle is invalid (returned by fclose() from C stdio)
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PpvFileHandle is NULL
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
FileIO_Close(
	_Inout_ void** PppvFileHandle);
/**
 *	@brief		Reads a line from a text file
 *	@details
 *	@param		PpvFileHandle		Handle to an open file
 *	@param		PwszBuffer			Buffer to the read data
 *	@param		PpunSize			In:	Capacity of the destination buffer in wchar_t elements (include additional space for terminating 0)\n
 *									Out: Number of elements copied to the destination buffer (without terminating 0)
 *	@returns	RC_SUCCESS, one of the return values listed here (http://msdn.microsoft.com/en-us/library/vstudio/t3ayayh1%28v=vs.110%29.aspx) or one of the following error codes:
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. One of the parameters is NULL
 *	@retval		RC_E_END_OF_FILE	In case method has reached the end of file
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 *	@retval		RC_E_WRONG_ENCODING	In case file is not in UCS-2 LE encoding
 */
_Check_return_
unsigned int
FileIO_ReadLine(
	_In_					const void*		PpvFileHandle,
	_Out_z_cap_(*PpunSize)	wchar_t*		PwszBuffer,
	_Inout_					unsigned int*	PpunSize);

/**
 *	@brief		Sets the specified pointer position in a file
 *	@details
 *	@param		PpvFileHandle		Handle to an open file
 *	@param		PullFilePosition	File position to set
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PpvFileHandle is NULL
 *	@retval		RC_E_FAIL			An unexpected error occurred. E.g. PullFilePosition is invalid
 */
_Check_return_
unsigned int
FileIO_SetPosition(
	_In_	const void*			PpvFileHandle,
	_In_	unsigned long long	PullFilePosition);

/**
 *	@brief		Gets the current position in a file
 *	@details
 *	@param		PpvFileHandle		Handle to an open file
 *	@param		PpullFilePosition	Pointer to variable for file position
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PpvFileHandle or PpullFilePosition is NULL
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
FileIO_GetPosition(
	_In_	const void*			PpvFileHandle,
	_Out_	unsigned long long*	PpullFilePosition);

/**
 *	@brief		Writes a string
 *	@details	Writes a string to the current position in a file
 *	@param		PpvFileHandle	Handle to an open file
 *	@param		PwszString		String to write
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PwszString is NULL
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 *	@retval		...					Error codes from FileIO_WriteStringf
 */
_Check_return_
unsigned int
FileIO_WriteString(
	_In_	const void*		PpvFileHandle,
	_In_z_	const wchar_t*	PwszString);

/**
 *	@brief		Writes a formatted string
 *	@details	Writes a formatted string to the current position in a file
 *	@param		PpvFileHandle	Handle to an open file
 *	@param		PwszFormat		Format string to write
 *	@param		...				one or more values to write
 *	@retval		...				Error codes from FileIO_WriteStringvf
 */
_Check_return_
unsigned int
FileIO_WriteStringf(
	_In_	const void*		PpvFileHandle,
	_In_z_	const wchar_t*	PwszFormat,
	...);

/**
 *	@brief		Writes a formatted string using a va_list
 *	@details	Writes a formatted string to the current position in a file, using a va_list
 *	@param		PpvFileHandle		Handle to an open file
 *	@param		PwszFormat			Format string to write
 *	@param		PpArguments			One or more values to write
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PpvFileHandle or PwszFormat is NULL
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
FileIO_WriteStringvf(
	_In_	const void*		PpvFileHandle,
	_In_z_	const wchar_t*	PwszFormat,
	_In_	va_list			PpArguments);

/**
 *	@brief		Checks if the current position in a file points to the end of the file
 *	@details
 *	@param		PpvFileHandle		Handle to an open file
 *	@param		PpfEofReached		Out: TRUE if a read operation has attempted to read past the end of the file; FALSE otherwise
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PpvFileHandle or PpfEofReached is NULL
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
FileIO_EOF(
	_In_	const void*	PpvFileHandle,
	_Out_	BOOL*		PpfEofReached);

/**
 *	@brief		Checks if file exists
 *	@details
 *
 *	@param		PwszFileName	File name
 *	@retval		TRUE			If file exists
 *	@retval		FALSE			If file does not exist
 */
_Check_return_
BOOL
FileIO_Exists(
	_In_z_ const wchar_t* PwszFileName);

/**
 *	@brief		Get the file size
 *	@details	Returns the size in bytes of a given file
 *	@param		PpvFileHandle		Handle to an open file
 *	@param		PpullFileSize		The size of the file in bytes
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. PpvFileHandle or PpullFileSize is NULL
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
FileIO_GetFileSize(
	_In_	const void*			PpvFileHandle,
	_Out_	unsigned long long*	PpullFileSize);

/**
 *	@brief		Read the whole content of a file into a byte array
 *	@details	The function opens, reads and closes the file.
 *				Allocates a buffer with the required size.
 *				and returns the pointer to that buffer.
 *
 *	@param		PwszFileName		String containing the file to be read
 *	@param		PprgbBuffer			Pointer to a byte array which receives
 *									the allocated memory for the buffer.
 *	@param		PpunBufferSize		Number of bytes read to the buffer.
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. It was either NULL or not initialized correctly. Or the file was too large.
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
FileIO_ReadFileToBuffer(
	_In_z_						const wchar_t*	PwszFileName,
	_Outptr_result_maybenull_	BYTE**			PprgbBuffer,
	_Out_						unsigned int*	PpunBufferSize);

/**
 *	@brief		Read the whole content of a file into a wide char array
 *	@details	The function opens, reads and closes the file.
 *				Allocates a buffer with the required size.
 *				and returns the pointer to that buffer.
 *
 *	@param		PwszFileName		String containing the file to be read
 *	@param		PpwszBuffer			Pointer to a wide char array which receives
 *									the allocated memory for the buffer. The caller must free the allocated buffer!
 *									The value behind the pointer must be initialized to zero.
 *	@param		PpunBufferSize		Number of wide chars read to the buffer. Without zero termination.
 *									Value behind the pointer must be initialized to zero.
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. It was either NULL or not initialized correctly. Or the file was too large.
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 *	@retval		RC_E_WRONG_ENCODING	In case file is not in UCS-2 LE encoding
 */
_Check_return_
unsigned int
FileIO_ReadFileToStringBuffer(
	_In_z_						const wchar_t*	PwszFileName,
	_Outptr_result_maybenull_z_	wchar_t**		PpwszBuffer,
	_Out_						unsigned int*	PpunBufferSize);

/**
 *	@brief		Write a byte array to the a file
 *	@details	The function writes the bytes from a buffer to the file.
 *
 *	@param		PpvFileHandle		Handle to a file
 *	@param		PrgbBuffer			Pointer to a byte buffer
 *	@param		PunBufferSize		Number of bytes to be written to the file
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. It was either NULL or not initialized correctly.
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
FileIO_WriteBuffer(
	_In_	const void*		PpvFileHandle,
	_In_	const BYTE*		PrgbBuffer,
	_In_	unsigned int	PunBufferSize);

/**
 *	@brief		Remove a file
 *	@details
 *
 *	@param		PwszFileName		File name
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_ACCESS_DENIED	In case of a read-only or opened file
 *	@retval		RC_E_FILE_NOT_FOUND	The file does not exist.
 *	@retval		RC_E_INTERNAL		In all other cases where the file could not be removed
 *	@retval		RC_E_FAIL			An unexpected error occurred.
 */
_Check_return_
unsigned int
FileIO_Remove(
	_In_z_ const wchar_t* PwszFileName);

#ifdef __cplusplus
}
#endif
