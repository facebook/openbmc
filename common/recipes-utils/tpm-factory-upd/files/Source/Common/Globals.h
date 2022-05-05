/**
 *	@brief		Declares global definitions
 *	@details
 *	@file		Globals.h
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

// ------------------- Constant definitions --------------------
/// IFX branding name
#define IFX_BRAND						L"Infineon Technologies AG"
/// String buffer length 1024
#define MAX_STRING_1024			1024
/// The maximum supported path length
#define MAX_PATH				260
/// The maximum supported name length
#define MAX_NAME				256
/// The maximum supported command size
#define MAX_TPM_COMMAND_SIZE	4096
/// The maximum supported message size
#define MAX_MESSAGE_SIZE		6144
/// Application version
#define APP_VERSION L"01.04.2811.00" // Will be patched by build process
/// Define for usage of big console buffer size for more columns and lines
#define	CONSOLE_BUFFER_BIG		1
/// Define for untouched console buffer
#define CONSOLE_BUFFER_NONE		0xFFFFFFFF
/// Byte Order Mask (BOM) for Unicode UCS-2 Little Endian string encoding
#define BOM_UCS2LE				0xFEFF
/// Size of TPM2.0 generated random number
#define RANDOMSIZE				20
/// TPM device access through Memory based access (only supported on x86 based systems with PCH TPM support)
#define TPM_DEVICE_ACCESS_MEMORY_BASED 1
/// TPM device access through device driver (for example /dev/tpm0, etc.)
#define TPM_DEVICE_ACCESS_DRIVER 3
/// TPM DEVICE_ACCESS_PATH
#define TPM_DEVICE_ACCESS_PATH L"/dev/tpm0"
/// Define for TPM device access mode property string
#define PROPERTY_TPM_DEVICE_ACCESS_MODE		L"TpmDeviceAccessMode"
/// Define for TPM device driver path property string
#define PROPERTY_TPM_DEVICE_ACCESS_PATH		L"TpmDeviceAccessPath"
/// Define for CallTpm2ShutdownOnExit property
#define PROPERTY_CALL_SHUTDOWN_ON_EXIT		L"CallTpm2ShutdownOnExit"

// ------------------ Global type definitions ------------------
#ifndef BYTE
/// Definition of type BYTE
typedef unsigned char	BYTE;
#endif // BYTE
#ifndef BOOL
/// Definition of type BOOL
typedef int				BOOL;
#endif // BOOL
#ifndef DWORD
/// Definition of type DWORD
typedef unsigned long	DWORD;
#endif // DWORD
#ifndef TRUE
/// Definition of BOOL value TRUE
#define TRUE	1
#endif // TRUE
#ifndef FALSE
/// Definition of BOOL value FALSE
#define FALSE	0
#endif // FALSE

// --------------------- Macro definitions ---------------------
#if defined(_MSC_VER)
/// Macro definition to suppress CA warning C4127: "conditional expression is constant" in do .. while loop at Code Analysis
#define WHILE_FALSE_END __pragma(warning(push)) \
		__pragma(warning(disable:4127)) \
		while(FALSE); \
		__pragma(warning(pop))
#define WHILE_TRUE_END __pragma(warning(push)) \
		__pragma(warning(disable:4127)) \
		while(TRUE); \
		__pragma(warning(pop))

/// Define for unreferenced parameters
#define UNREFERENCED_PARAMETER(P) (P)

/// Define for unused "unsigned int" return values.
#define IGNORE_RETURN_VALUE(P) { unsigned int unTemp = RC_E_FAIL; unTemp = P;}

/// Define for unused "BOOL" return values.
#define IGNORE_RETURN_VALUE_BOOL(P) { BOOL fTemp = FALSE; fTemp = P;}

// __func__ macro does not exist on Windows
#define __func__ __FUNCTION__

#else // _GNU_C_
/// Macro definition for do .. while loop for GCC
#define WHILE_FALSE_END while(FALSE);
#define WHILE_TRUE_END while(TRUE);

/// Define for unreferenced parameters
#define UNREFERENCED_PARAMETER(P) do { (void)(P); } while (0)

/// Define for unused "unsigned int" return values.
#define IGNORE_RETURN_VALUE(P) (P)

/// Define for unused "BOOL" return values.
#define IGNORE_RETURN_VALUE_BOOL(P) (P)
#endif

/// Size of a constant array in elements, e.g. length (not size!) of a null-terminated wide character string (incl. null-termination)
#define RG_LEN(x) (sizeof(x) / sizeof(x[0]))

// ------------ Defines for TIS communication ---------------
/// Maximum number of retries in case of reading errors
#define MAX_TPM_READ_RETRIES		3
/// This is the generic time value to sleep between retries in micro seconds
#define SLEEP_TIME_US				100
/// This is the time value to sleep between TIS_GetBurstCount() retries in micro seconds
#define SLEEP_TIME_US_BURSTCOUNT	10
/// This is the time value to sleep between TIS_IsActiveLocality() retries in micro seconds
#define SLEEP_TIME_US_CR			10
/// Default memory address base for TPM device
#define TPM_DEFAULT_MEM_BASE		0xFED40000U
/// Default memory address size for TPM device
#define TPM_DEFAULT_MEM_SIZE		0x5000U

// --------------------- Enum definitions ----------------------
/**
 *	@brief		File Access Mode enumeration
 *	@details	Enumerates file access types.
 */
enum eFileAccessMode
{
	/// Read file
	FILE_READ	= 0,
	/// Create / overwrite file
	FILE_WRITE	= 1,
	/// Create / append to file
	FILE_APPEND = 2,
	/// Write binary
	FILE_WRITE_BINARY = 3,
	/// Read binary
	FILE_READ_BINARY = 4
};
