/**
 *	@brief		Implements the FileIO interface for Linux
 *	@details	All FileIO functions which are platform specific
 *	@file		Linux\FileIO.c
 *	@copyright	Copyright 2016 - 2018 Infineon Technologies AG ( www.infineon.com )
 *
 *	@copyright	All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "FileIO.h"
#include "Platform.h"

/**
 *	@brief		Enum for the different types of BOM
 *	@details
 */
typedef enum td_ENUM_BOM_TYPES
{
	/// No BOM
	BOM_TYPES_NO,
	/// BOM UTF8
	BOM_TYPES_UTF8,
	/// BOM UTF16LE
	BOM_TYPES_UTF16LE,
	/// BOM UTF16BE
	BOM_TYPES_UTF16BE,
	/// BOM UTF32LE
	BOM_TYPES_UTF32LE,
	/// BOM UTF32BE
	BOM_TYPES_UTF32BE
} ENUM_BOM_TYPES;

/**
 *	@brief		Get the BOM type of the current file handle.
 *	@details	The function stores the current file position, jumps to the beginning of the file, reads out the BOM information
 *				and restores the original file position. If non of the current implemented BOMs is found the return value will be
 *				BOM_TYPES_NO.
 *
 *	@param		PpvFile				File pointer to the readable binary opened file
 *	@param		PpBOMType			Pointer to retrieve the type of BOM
 *	@param		PpunSizeOfBOM		Pointer to retrieve the size of the found BOM
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function. One of the parameters are NULL or empty, *PppvFileHandle is not NULL or PunFileAccessMode is invalid
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
FileIO_GetBOMType(
	_In_ void* PpvFile,
	_Out_ ENUM_BOM_TYPES* PpBOMType,
	_Out_ unsigned int* PpunSizeOfBOM)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		unsigned long long ullFilePosition = 0;
		unsigned int unSizeRead = 0;
		unsigned int unBom = 0;

		// Check parameters
		if (NULL == PpvFile || NULL == PpBOMType || NULL == PpunSizeOfBOM)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}
		// Initialize it to no BOM and zero size
		*PpBOMType = BOM_TYPES_NO;
		*PpunSizeOfBOM = 0;

		// Save actual file position
		unReturnValue = FileIO_GetPosition(PpvFile, &ullFilePosition);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Set file pointer to beginning
		unReturnValue = FileIO_SetPosition(PpvFile, 0);
		if (RC_SUCCESS != unReturnValue)
			break;

		unSizeRead = (unsigned int)fread(&unBom, 1, sizeof (unBom), PpvFile);

		switch(unSizeRead)
		{
			case 4:
			{
				if(0x0000FEFF == unBom)
				{
					*PpBOMType = BOM_TYPES_UTF32LE;
					*PpunSizeOfBOM = 4;
					break;
				}
				if (0xFFFE0000 == unBom)
				{
					*PpBOMType = BOM_TYPES_UTF32BE;
					*PpunSizeOfBOM = 4;
					break;
				}
			}
#if __GNUC__ >= 7
			__attribute__ ((fallthrough));
#endif
			// 3 or 4
			case 3:
			{
				if(0x00BFBBEF == (unBom & 0x00FFFFFF))
				{
					*PpBOMType = BOM_TYPES_UTF8;
					*PpunSizeOfBOM = 3;
					break;
				}
			}
#if __GNUC__ >= 7
			__attribute__ ((fallthrough));
#endif
			// 2, 3 or 4
			case 2:
			{
				if (0x0000FFFE == (unBom & 0x0000FFFF))
				{
					*PpBOMType = BOM_TYPES_UTF16BE;
					*PpunSizeOfBOM = 2;
					break;
				}
				if (0x0000FEFF == (unBom & 0x0000FFFF))
				{
					*PpBOMType = BOM_TYPES_UTF16LE;
					*PpunSizeOfBOM = 2;
					break;
				}
			}
			default:
				break;
		}

		// Restore file pointer position
		unReturnValue = FileIO_SetPosition(PpvFile, ullFilePosition);
		if (RC_SUCCESS != unReturnValue)
			break;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

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
	_In_						unsigned int	PunFileAccessMode)
{
	unsigned int unReturnValue = RC_E_FAIL;
	char* szFileName = NULL;

	do
	{
		FILE* pFile = NULL;
		size_t sizeFileName = 0;

		// Check parameters
		if (NULL == PppvFileHandle || PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszFileName) || NULL != *PppvFileHandle)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// For fopen operation the file name (wide character string) needs to be converted to multibyte string
		// Get the required size for the multibyte string.
		sizeFileName = wcsrtombs(NULL, &PwszFileName, 0, NULL);
		if ((size_t) - 1 == sizeFileName)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Allocate memory for the multibyte string
		szFileName = (char*)calloc(sizeFileName + 1, sizeof(char));
		if (NULL == szFileName)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Convert the wide character string to a multibyte string.
		sizeFileName = wcsrtombs(szFileName, &PwszFileName, wcslen(PwszFileName), NULL);
		if ((size_t) - 1 == sizeFileName)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Open the file
		switch (PunFileAccessMode)
		{
			case FILE_READ:
				pFile = fopen(szFileName, "rb");
				break;
			case FILE_WRITE:
				pFile = fopen(szFileName, "w");
				break;
			case FILE_APPEND:
				pFile = fopen(szFileName, "a");
				break;
			case FILE_WRITE_BINARY:
				pFile = fopen(szFileName, "wb");
				break;
			case FILE_READ_BINARY:
				pFile = fopen(szFileName, "rb");
				break;
			default:
				unReturnValue = RC_E_BAD_PARAMETER;
				break;
		}
		if (unReturnValue == RC_E_BAD_PARAMETER)
			break;

		if (NULL == pFile)
		{
			switch(errno)
			{
				case ENOENT:
					unReturnValue = RC_E_FILE_NOT_FOUND;
					break;
				case EACCES:
					unReturnValue = RC_E_ACCESS_DENIED;
					break;
				case EINVAL:
					unReturnValue = RC_E_BAD_PARAMETER;
					break;
				default:
					unReturnValue = RC_E_FAIL;
					break;
			}
			break;
		}

		// In case of success, copy the file handle to the output parameter
		*PppvFileHandle = (void*)pFile;
		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	// Cleanup memory
	if (szFileName != NULL)
	{
		free(szFileName);
		szFileName = NULL;
	}

	return unReturnValue;
}

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
	_Inout_ void** PppvFileHandle)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// Check file handle
		if (NULL == PppvFileHandle || NULL == *PppvFileHandle)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Close file
		unReturnValue = fclose((FILE*)*PppvFileHandle);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Set file handle to NULL in case it has been successfully closed
		*PppvFileHandle = NULL;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

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
	_Inout_					unsigned int*	PpunSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		FILE* hFile = NULL;
		unsigned long long ullOriginalPosition = 0;
		ENUM_BOM_TYPES eBOMType = BOM_TYPES_NO;
		unsigned int unSizeOfBOM = 0;
		void* pBuffer = NULL;

		// Check parameters
		if (NULL == PwszBuffer || NULL == PpvFileHandle || NULL == PpunSize || 0 == *PpunSize || INT_MAX < *PpunSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Cast and store pointer to avoid casting twice later
		hFile = (FILE*)PpvFileHandle;

		// Get current file pointer position
		unReturnValue = FileIO_GetPosition(PpvFileHandle, &ullOriginalPosition);
		if (RC_SUCCESS != unReturnValue)
			break;

		unReturnValue = FileIO_GetBOMType(hFile, &eBOMType, &unSizeOfBOM);
		if (RC_SUCCESS != unReturnValue)
			break;

		if (0 == ullOriginalPosition)
		{
			unReturnValue = FileIO_SetPosition(PpvFileHandle, (unsigned long long) unSizeOfBOM);
			if (RC_SUCCESS != unReturnValue)
				break;
		}

		switch (eBOMType)
		{
			case BOM_TYPES_NO:
			case BOM_TYPES_UTF8:
			{
				char szBuffer[MAX_MESSAGE_SIZE] = {0};
				unsigned int unBufferSize = MAX_MESSAGE_SIZE;
				size_t sizeCoverted = 0;
				pBuffer = (void*) fgets(szBuffer, unBufferSize, hFile);
				sizeCoverted = mbstowcs(PwszBuffer, szBuffer, *PpunSize);
				if ((size_t) - 1 == sizeCoverted)
				{
					unReturnValue = RC_E_FAIL;
					break;
				}

				unReturnValue = RC_SUCCESS;
				break;
			}
			default:
			{
				unReturnValue = RC_E_WRONG_ENCODING;
				break;
			}
		}

		if (RC_SUCCESS != unReturnValue)
			break;

		if (NULL == pBuffer)
		{
			// Get error code, if reading has not been successful
			unReturnValue = ferror(hFile);

			// If no error has occurred, we have reached end of file
			if (RC_SUCCESS == unReturnValue)
				unReturnValue = RC_E_END_OF_FILE;

			break;
		}

		// Return number of bytes read
		unReturnValue = Platform_StringGetLength(PwszBuffer, *PpunSize, PpunSize);
	}
	WHILE_FALSE_END;

	if (RC_SUCCESS != unReturnValue)
	{
		// Reset out parameters
		if (NULL != PwszBuffer)
			PwszBuffer[0] = L'\0';
		if (NULL != PpunSize)
			*PpunSize = 0;
	}

	return unReturnValue;
}

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
	_In_	unsigned long long	PullFilePosition)
{
	unsigned int unReturnValue = RC_E_FAIL;

	// Check file handle parameter
	if (NULL == PpvFileHandle)
		unReturnValue = RC_E_BAD_PARAMETER;

	// Set file pointer position
	else if (0 == fseek((FILE*)PpvFileHandle, PullFilePosition, SEEK_SET))
		unReturnValue = RC_SUCCESS;

	return unReturnValue;
}

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
	_Out_	unsigned long long*	PpullFilePosition)
{
	unsigned int unReturnValue = RC_E_FAIL;

	// Check parameters
	if (NULL == PpvFileHandle || NULL == PpullFilePosition)
		unReturnValue = RC_E_BAD_PARAMETER;
	else
	{
		// Get file pointer position
		*PpullFilePosition = ftell((FILE*)PpvFileHandle);

		// Check if operation was successful
		if (*PpullFilePosition != ((unsigned long long) - 1))
			unReturnValue = RC_SUCCESS;
	}

	if (RC_SUCCESS != unReturnValue)
		// Reset out parameter
		if (NULL != PpullFilePosition)
			*PpullFilePosition = 0;

	return unReturnValue;
}

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
	_In_z_	const wchar_t*	PwszString)
{
	unsigned int unReturnValue = RC_E_FAIL;

	if (NULL == PwszString)
		unReturnValue = RC_E_BAD_PARAMETER;
	else
		unReturnValue = FileIO_WriteStringf(PpvFileHandle, L"%ls", PwszString);

	return unReturnValue;
}

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
	...)
{
	unsigned int unReturnValue = RC_E_FAIL;
	va_list argptr;

	// Get pointer to variable arguments
	va_start(argptr, PwszFormat);

	// Call actual write method with variable arguments
	unReturnValue = FileIO_WriteStringvf(PpvFileHandle, PwszFormat, argptr);

	// Reset pointer
	va_end(argptr);

	return unReturnValue;
}

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
	_In_	va_list			PpArguments)
{
	unsigned int unReturnValue = RC_E_FAIL;
	int nResult = 0;

	// Check parameters
	if (NULL == PpvFileHandle || PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszFormat))
		unReturnValue = RC_E_BAD_PARAMETER;

	else
	{
		// Write data
		nResult = vfwprintf((FILE*)PpvFileHandle, PwszFormat, PpArguments);

		// Check if operation was successful
		if (0 < nResult)
			unReturnValue = RC_SUCCESS;
	}

	return unReturnValue;
}

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
	_Out_	BOOL*		PpfEofReached)
{
	unsigned int unReturnValue = RC_E_FAIL;
	int nFeofReturnValue = 0;

	// Initialize out parameter
	if (NULL != PpfEofReached)
		*PpfEofReached = FALSE;

	// Check parameters
	if (NULL == PpvFileHandle || NULL == PpfEofReached)
		unReturnValue = RC_E_BAD_PARAMETER;
	else
	{
		// Check if EOF has been reached
		nFeofReturnValue = feof((FILE*)PpvFileHandle);
		if (0 < nFeofReturnValue)
			*PpfEofReached = TRUE;

		unReturnValue = RC_SUCCESS;
	}

	return unReturnValue;
}

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
	_In_z_ const wchar_t* PwszFileName)
{
	BOOL fReturn = FALSE;
	char* szFileName = NULL;
	size_t sizeFileName = 0;

	do
	{
		// Check input parameter
		if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszFileName))
			break;

		// For fopen operation the file name (wide character string) needs to be converted to multibyte string
		// Get the required size for the multibyte string.
		sizeFileName = wcsrtombs(NULL, &PwszFileName, 0, NULL);
		if ((size_t) - 1 == sizeFileName)
			break;

		// Allocate memory for the multibyte string
		szFileName = (char*)calloc(sizeFileName + 1, sizeof(char));
		if (NULL == szFileName)
			break;

		// Convert the wide character string to a multibyte string.
		sizeFileName = wcsrtombs(szFileName, &PwszFileName, wcslen(PwszFileName), NULL);
		if ((size_t) - 1 == sizeFileName)
			break;

		// Check if the file exists
		if (access(szFileName, F_OK) != -1)
		{
			fReturn = TRUE;
			break;
		}
	}
	WHILE_FALSE_END;

	// Cleanup memory
	if (szFileName != NULL)
	{
		free(szFileName);
		szFileName = NULL;
	}

	return fReturn;
}

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
	_Out_	unsigned long long*	PpullFileSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		unsigned long long ullFileSize = 0;
		unsigned long long ullFilePosition = 0;

		// Initialize output parameter
		if (NULL != PpullFileSize)
			*PpullFileSize = 0;

		// Check parameters
		if (NULL == PpvFileHandle || NULL == PpullFileSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Get current file pointer position
		unReturnValue = FileIO_GetPosition(PpvFileHandle, &ullFilePosition);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Seek to the end of the file
		if (0 != fseek((FILE*)PpvFileHandle, 0LL, SEEK_END))
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Get the new file pointer position. This is the size.
		unReturnValue = FileIO_GetPosition(PpvFileHandle, &ullFileSize);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Restore original file pointer position
		rewind((FILE*)PpvFileHandle);

		// Assign out parameter
		*PpullFileSize = ullFileSize;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

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
	_Out_						unsigned int*	PpunBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;
	FILE* pFile = NULL;

	do
	{
		unsigned long long ullFileSize = 0;

		// Check parameters
		if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszFileName) || NULL == PpunBufferSize || NULL == PprgbBuffer || NULL != *PprgbBuffer)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// First open the file
		unReturnValue = FileIO_Open(PwszFileName, (void**)(&pFile), FILE_READ_BINARY);
		if (RC_SUCCESS != unReturnValue)
			break;
		if (NULL == pFile)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Second get the file size
		unReturnValue = FileIO_GetFileSize(pFile, &ullFileSize);
		if (RC_SUCCESS != unReturnValue)
			break;
		if (UINT_MAX < ullFileSize) // Memory allocation can only handle limited size
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Third allocate memory
		*PprgbBuffer = (BYTE*)Platform_MemoryAllocateZero((unsigned int)ullFileSize);
		if (NULL == *PprgbBuffer)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Fourth read the whole file
		*PpunBufferSize = (unsigned int)fread(*PprgbBuffer, (size_t)1, (size_t)ullFileSize, pFile);
		if (0 == *PpunBufferSize && 0 != ferror(pFile))
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Check that the whole file was read.
		if (*PpunBufferSize != ullFileSize)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	// Check if the file pointer is not NULL and close the file, but do not overwrite previous error code if existing
	if (NULL != pFile)
	{
		unsigned int unReturnValueClose = RC_E_FAIL;
		unReturnValueClose = FileIO_Close((void**)&pFile);
		if (RC_SUCCESS != unReturnValueClose)
		{
			if (RC_SUCCESS == unReturnValue)
				unReturnValue = unReturnValueClose;
		}
	}

	// If an error occurs free the buffer if allocated before
	if (RC_SUCCESS != unReturnValue)
	{
		Platform_MemoryFree((void**)PprgbBuffer);

		// Reset out parameter
		if (NULL != PpunBufferSize)
			*PpunBufferSize = 0;
	}

	return unReturnValue;
}

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
	_Out_						unsigned int*	PpunBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;
	FILE* pFile = NULL;
	unsigned char* pbTemp = NULL;

	do
	{
		unsigned long long ullFileSize = 0;
		unsigned int unSizeOfBOM = 0;
		ENUM_BOM_TYPES eBOMType = BOM_TYPES_NO;

		// Check parameters
		if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszFileName) || NULL == PpunBufferSize || NULL == PpwszBuffer || NULL != *PpwszBuffer)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// First open the file
		unReturnValue = FileIO_Open(PwszFileName, (void**)(&pFile), FILE_READ);
		if (RC_SUCCESS != unReturnValue)
			break;

		if (NULL == pFile)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Second get the file size
		unReturnValue = FileIO_GetFileSize(pFile, &ullFileSize);
		if (RC_SUCCESS != unReturnValue)
			break;

		if (UINT_MAX < ullFileSize + 2) // Memory allocation can only handle limited size (add 2 byte for final zero termination)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		unReturnValue = FileIO_GetBOMType(pFile, &eBOMType, &unSizeOfBOM);
		if (RC_SUCCESS != unReturnValue)
			break;

		unReturnValue = FileIO_SetPosition(pFile, (unsigned long long) unSizeOfBOM);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Remove size of BOM
		ullFileSize -= unSizeOfBOM;

		{
			// Third allocate temporary process memory (including 2 byte for zero termination)
			unsigned int unTempSize = 0;
			unsigned int unNumWcharsInFile = 0;
			pbTemp = (unsigned char*)Platform_MemoryAllocateZero((unsigned int)ullFileSize + 2);

			if (NULL == pbTemp)
			{
				unReturnValue = RC_E_FAIL;
				break;
			}

			// Fourth read the whole file to process memory
			unTempSize = (unsigned int)fread(pbTemp, 1, (size_t)ullFileSize, pFile);
			if (ullFileSize != unTempSize)
			{
				unReturnValue = RC_E_FAIL;
				break;
			}

			switch(eBOMType)
			{
				case BOM_TYPES_UTF16LE:
				{
					// Fifth allocate memory for the resulting wchar_t string. This buffer has twice the file size.
					// A character consumes 2 bytes in the file but 4 bytes in wchar_t field.
					unNumWcharsInFile = (ullFileSize / 2 + 1);
					*PpwszBuffer = (wchar_t*)Platform_MemoryAllocateZero(unNumWcharsInFile * sizeof(wchar_t));
					if (NULL == *PpwszBuffer)
					{
						unReturnValue = RC_E_FAIL;
						break;
					}

					// Sixth unmarshal process memory to resulting wchar_t string. Do not add the termination zero size of 2
					// to ullFileSize because the Platform_UnmarshalString function handles the buffer as byte array and not as
					// wide string array. The +2 is only needed in case of UTF-8 where mbstowcs casts the byte array to a const char*.
					unReturnValue = Platform_UnmarshalString(pbTemp, ullFileSize, *PpwszBuffer, &unNumWcharsInFile);
					if (RC_SUCCESS != unReturnValue)
						break;

					*PpunBufferSize = unNumWcharsInFile;
					unReturnValue = RC_SUCCESS;
					break;
				}
				case BOM_TYPES_UTF8:
				case BOM_TYPES_NO:
				{
					size_t sizeNumOfReadBytes = 0;
					unNumWcharsInFile = mbstowcs(NULL, (const char*) pbTemp, 0) + 1;
					*PpwszBuffer = (wchar_t*)Platform_MemoryAllocateZero(unNumWcharsInFile * sizeof(wchar_t));
					if (NULL == *PpwszBuffer)
					{
						unReturnValue = RC_E_FAIL;
						break;
					}

					sizeNumOfReadBytes = mbstowcs(*PpwszBuffer, (const char*)pbTemp, unNumWcharsInFile);

					if (sizeNumOfReadBytes + 1 != unNumWcharsInFile)
					{
						unReturnValue = RC_E_FAIL;
						break;
					}

					*PpunBufferSize = unNumWcharsInFile;
					unReturnValue = RC_SUCCESS;
					break;
				}
				default:
				{
					unReturnValue = RC_E_WRONG_ENCODING;
					break;
				}
			}
		}
	}
	WHILE_FALSE_END;

	// Check if pbTemp is allocated
	if (NULL != pbTemp)
		Platform_MemoryFree((void**)&pbTemp);

	// Check if the file pointer is not NULL and close the file, but do not overwrite previous error code if existing
	if (NULL != pFile)
	{
		unsigned int unReturnValueClose = RC_E_FAIL;
		unReturnValueClose = FileIO_Close((void**)&pFile);
		if (RC_SUCCESS != unReturnValueClose)
		{
			if (RC_SUCCESS == unReturnValue)
				unReturnValue = unReturnValueClose;
		}
	}

	// If an error occurs free the buffer if allocated before
	if (RC_SUCCESS != unReturnValue)
	{
		Platform_MemoryFree((void**)PpwszBuffer);

		// Reset out parameter
		if (NULL != PpunBufferSize)
			*PpunBufferSize = 0;
	}

	return unReturnValue;
}

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
	_In_	unsigned int	PunBufferSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		unsigned int unWrittenBytes = 0;

		// Check parameters
		if ( NULL == PpvFileHandle || NULL == PrgbBuffer)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Write bytes to file
		unWrittenBytes = (unsigned int)fwrite((void*)PrgbBuffer, sizeof(BYTE), PunBufferSize, (FILE*)PpvFileHandle);

		// Check if the buffer size is identical to the written bytes count or an error has occurred
		if (unWrittenBytes != PunBufferSize || 0 != ferror((FILE*)PpvFileHandle))
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

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
	_In_z_ const wchar_t* PwszFileName)
{
	unsigned int unReturnValue = RC_E_FAIL;
	char* szFileName = NULL;

	do
	{
		size_t sizeFileName = 0;

		// Check parameter
		if (PLATFORM_STRING_IS_NULL_OR_EMPTY(PwszFileName))
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// For remove operation the file name (wide character string) needs to be converted to multibyte string
		// Get the required size for the multibyte string.
		sizeFileName = wcsrtombs(NULL, &PwszFileName, 0, NULL);
		if ((size_t) - 1 == sizeFileName)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Allocate memory for the multibyte string
		szFileName = (char*) Platform_MemoryAllocateZero(sizeFileName + 1);
		if (NULL == szFileName)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Convert the wide character string to a multibyte string.
		sizeFileName = wcsrtombs(szFileName, &PwszFileName, wcslen(PwszFileName), NULL);
		if ((size_t) - 1 == sizeFileName)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		if (0 != unlink(szFileName))
		{
			switch(errno)
			{
				case EACCES:
					unReturnValue = RC_E_ACCESS_DENIED;
					break;
				case ENOENT:
					unReturnValue = RC_E_FILE_NOT_FOUND;
					break;
				default:
					unReturnValue = RC_E_INTERNAL;
					break;
			}
		}
		else
			unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	// Cleanup memory
	if (szFileName != NULL)
	{
		Platform_MemoryFree((void**)&szFileName);
		szFileName = NULL;
	}

	return unReturnValue;
}
