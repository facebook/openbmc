/**
 *	@brief		Implements the Device memory access routines
 *	@details
 *	@file		Linux/DeviceAccess.c
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

#include "StdInclude.h"
#include "DeviceAccess.h"
#include "Logging.h"
#include "Platform.h"

static UINT32 s_unFileHandle = 0;
static BYTE *s_bMemPtr = NULL;

#define DEV_TPM_MEM "/dev/mem"

/**
 *	@brief		Initialize the device access
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_INTERNAL			The operation failed.
 *	@retval		RC_E_TPM_ACCESS_DENIED	Read-write access to /dev/mem was denied.
 */
_Check_return_
unsigned int
DeviceAccess_Initialize(
	_In_	BYTE	PbLocality)
{
	unsigned int unReturnValue = RC_E_FAIL;
	UNREFERENCED_PARAMETER(PbLocality);

	do
	{
		s_unFileHandle = open(DEV_TPM_MEM, O_RDWR);
		if (s_unFileHandle == (UINT32) - 1)
		{
			int nErrorNumber = errno;
			if (EACCES == nErrorNumber)
				unReturnValue = RC_E_TPM_ACCESS_DENIED;
			else
				unReturnValue = RC_E_INTERNAL;

			LOGGING_WRITE_LEVEL1_FMT(L"Error: Open device pseudo file %s failed with errno %d (%s).", DEV_TPM_MEM, errno, strerror(errno));
			break;
		}

		s_bMemPtr = (BYTE *) mmap(0, TPM_DEFAULT_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, s_unFileHandle, TPM_DEFAULT_MEM_BASE);
		if (s_bMemPtr == MAP_FAILED)
		{
			LOGGING_WRITE_LEVEL1_FMT(L"Error: Memory mapping failed with errno %d (%s).", errno, strerror(errno));
			unReturnValue = RC_E_INTERNAL;
			break;
		}

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		UnInitialize the device access
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		RC_E_INTERNAL	The operation failed.
 */
_Check_return_
unsigned int
DeviceAccess_Uninitialize(
	_In_	BYTE	PbLocality)
{
	unsigned int unReturnValue = RC_E_FAIL;
	UNREFERENCED_PARAMETER(PbLocality);

	munmap(s_bMemPtr, TPM_DEFAULT_MEM_SIZE);

	if (close(s_unFileHandle) == -1)
	{
		LOGGING_WRITE_LEVEL1_FMT(L"Error: Close device pseudo file %s failed with errno %d (%s).", DEV_TPM_MEM, errno, strerror(errno));
		unReturnValue = RC_E_INTERNAL;
	}
	else
	{
		unReturnValue = RC_SUCCESS;
	}
	return unReturnValue;
}

/**
 *	@brief		Read a Byte from the specified memory address
 *	@details
 *
 *	@param		PunMemoryAddress	Memory address
 *	@returns	Data value read from specified memory
 */
_Check_return_
BYTE
DeviceAccess_ReadByte(
	_In_	unsigned int PunMemoryAddress)
{
	BYTE bPortValue = 0;

	if (PunMemoryAddress < TPM_DEFAULT_MEM_BASE || PunMemoryAddress >= (TPM_DEFAULT_MEM_BASE + TPM_DEFAULT_MEM_SIZE))
	{
		LOGGING_WRITE_LEVEL4_FMT(L"Error: DeviceAccess_ReadByte: Memory address %0.4X is invalid!", PunMemoryAddress);
	}
	else
	{
		bPortValue = s_bMemPtr[PunMemoryAddress - TPM_DEFAULT_MEM_BASE];
	}

	LOGGING_WRITE_LEVEL4_FMT(L"DeviceAccess_ReadByte: Address: %0.4X: %0.2X", PunMemoryAddress, bPortValue);
	return bPortValue;
}

/**
 *	@brief		Write a Byte to the specified memory address
 *	@details
 *
 *	@param		PunMemoryAddress	Memory address
 *	@param		PbData				Byte to write
 */
void
DeviceAccess_WriteByte(
	_In_	unsigned int	PunMemoryAddress,
	_In_	BYTE			PbData)
{
	LOGGING_WRITE_LEVEL4_FMT(L"DeviceAccess_WriteByte: Address: %0.4X = %0.2X", PunMemoryAddress, PbData);

	if (PunMemoryAddress < TPM_DEFAULT_MEM_BASE || PunMemoryAddress >= (TPM_DEFAULT_MEM_BASE + TPM_DEFAULT_MEM_SIZE))
	{
		LOGGING_WRITE_LEVEL4_FMT(L"Error: DeviceAccess_WriteByte: Memory address %0.4X is invalid!", PunMemoryAddress);
	}
	else
	{
		s_bMemPtr[PunMemoryAddress - TPM_DEFAULT_MEM_BASE] = PbData;
	}
}

/**
 *	@brief		Read a Word from the specified memory address
 *	@details
 *
 *	@param		PunMemoryAddress	Memory address
 *	@returns	Data value read from specified memory
 */
_Check_return_
unsigned short
DeviceAccess_ReadWord(
	_In_	unsigned int	PunMemoryAddress)
{
	UINT16 usPortValue = 0;
	unsigned int unReturnValue = RC_E_FAIL;
	if (PunMemoryAddress < TPM_DEFAULT_MEM_BASE || PunMemoryAddress >= (TPM_DEFAULT_MEM_BASE + TPM_DEFAULT_MEM_SIZE))
	{
		LOGGING_WRITE_LEVEL4_FMT(L"Error: DeviceAccess_ReadWord: Memory address %0.4X is invalid!", PunMemoryAddress);
	}
	else
	{
		unReturnValue = Platform_MemoryCopy(&usPortValue, sizeof(UINT16), (const void*) & s_bMemPtr[PunMemoryAddress - TPM_DEFAULT_MEM_BASE], sizeof(UINT16));
		if (RC_SUCCESS != unReturnValue)
		{
			LOGGING_WRITE_LEVEL1_FMT(L"Unexpected returnvalue from function call Platform_MemoryCopy. Return Code: %0.4X", unReturnValue);
		}
	}

	LOGGING_WRITE_LEVEL4_FMT(L"DeviceAccess_ReadWord: Address: %0.4X: %0.4X", PunMemoryAddress, usPortValue);
	return usPortValue;
}

/**
 *	@brief		Write a Word to the specified memory address
 *	@details
 *
 *	@param		PunMemoryAddress	Memory address
 *	@param		PusData				Data to be written
 */
void
DeviceAccess_WriteWord(
	_In_	unsigned int	PunMemoryAddress,
	_In_	unsigned short	PusData)
{
	unsigned int unReturnValue = RC_E_FAIL;
	LOGGING_WRITE_LEVEL4_FMT(L"DeviceAccess_WriteWord:  Address: %0.4X = %0.4X", PunMemoryAddress, PusData);

	if (PunMemoryAddress < TPM_DEFAULT_MEM_BASE || PunMemoryAddress >= (TPM_DEFAULT_MEM_BASE + TPM_DEFAULT_MEM_SIZE))
	{
		LOGGING_WRITE_LEVEL4_FMT(L"Error: DeviceAccess_WriteWord: Memory address %0.4X is invalid!", PunMemoryAddress);
	}
	else
	{
		unReturnValue = Platform_MemoryCopy(& s_bMemPtr[PunMemoryAddress - TPM_DEFAULT_MEM_BASE], sizeof(UINT16), (const void*) & PusData, sizeof(unsigned short));
		if (RC_SUCCESS != unReturnValue)
		{
			LOGGING_WRITE_LEVEL1_FMT(L"Unexpected returnvalue from function call Platform_MemoryCopy. Return Code: %0.4X", unReturnValue);
		}
	}
}
