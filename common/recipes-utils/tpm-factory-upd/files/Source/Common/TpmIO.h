/**
 *	@brief		Declares the TPM I/O interface
 *	@details	This header defines the interface implemented by the underlying TPM access module
 *	@file		TpmIO.h
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

/// Function pointer to method for connecting to the TPM
typedef
unsigned int
(*PFN_TPMIO_Connect)();
/// Function pointer to method for disconnecting from the TPM
typedef
unsigned int
(*PFN_TPMIO_Disconnect)();
/// Function pointer to method for transmitting data to the TPM
typedef
unsigned int
(*PFN_TPMIO_Transmit)(
	const BYTE*		PrgbRequestBuffer,
	unsigned int	PunRequestBufferSize,
	BYTE*			PrgbResponseBuffer,
	unsigned int*	PpunResponseBufferSize,
	unsigned int	PunMaxDuration);
/// Function pointer to read a byte from a register of the TPM
typedef
unsigned int
(*PFN_TPMIO_ReadRegister)(
	unsigned int	PunRegisterAddress,
	BYTE*			PpbRegisterValue);
/// Function pointer to write a byte to a register of the TPM
typedef
unsigned int
(*PFN_TPMIO_WriteRegister)(
	unsigned int	PunRegisterAddress,
	BYTE			PbRegisterValue);

/**
 *	@brief		TPM connect function
 *	@details	This function handles the connect to the underlying TPM.
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_ALREADY_CONNECTED		If TPM I/O is already connected
 *	@retval		RC_E_COMPONENT_NOT_FOUND	No IFX TPM found
 *	@retval		...							Error codes from DeviceAccess_Initialize and TIS
 */
_Check_return_
unsigned int
TPMIO_Connect();

/**
 *	@brief		TPM disconnect function
 *	@details	This function handles the disconnect to the underlying TPM.
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_NOT_CONNECTED		If TPM I/O is not connected to the TPM
 *	@retval		...						Error codes from DeviceAccess_Uninitialize function
 */
_Check_return_
unsigned int
TPMIO_Disconnect();

/**
 *	@brief		TPM transmit function
 *	@details	This function submits the TPM command to the underlying TPM.
 *
 *	@param		PrgbRequestBuffer		Pointer to a byte array containing the TPM command request bytes
 *	@param		PunRequestBufferSize	Size of command request in bytes
 *	@param		PrgbResponseBuffer		Pointer to a byte array receiving the TPM command response bytes
 *	@param		PpunResponseBufferSize	Input size of response buffer, output size of TPM command response in bytes
 *	@param		PunMaxDuration			The maximum duration of the command in microseconds (relevant for memory based access / TIS protocol only)
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		RC_E_NOT_CONNECTED		If the TPM I/O is not connected to the TPM
 *	@retval		...						Error codes from TDDL_TransmitData function
 */
_Check_return_
unsigned int
TPMIO_Transmit(
	_In_bytecount_(PunRequestBufferSize)		const BYTE*		PrgbRequestBuffer,
	_In_										unsigned int	PunRequestBufferSize,
	_Out_bytecap_(*PpunResponseBufferSize)		BYTE*			PrgbResponseBuffer,
	_Inout_										unsigned int*	PpunResponseBufferSize,
	_In_										unsigned int	PunMaxDuration);

/**
 *	@brief		Read a byte from a specific address (register)
 *	@details	This function reads a byte from the specified address
 *
 *	@param		PunRegisterAddress		Register address
 *	@param		PpbRegisterValue		Pointer to a byte to store the register value
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function.
 *	@retval		...						Error codes from called functions
 */
_Check_return_
unsigned int
TPMIO_ReadRegister(
	_In_		unsigned int		PunRegisterAddress,
	_Inout_		BYTE*				PpbRegisterValue);

/**
 *	@brief		Write a byte to a specific address (register)
 *	@details	This function writes a byte to the specified address
 *
 *	@param		PunRegisterAddress		Register address
 *	@param		PbRegisterValue			Byte to write to the register address
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		...						Error codes from called functions
 */
_Check_return_
unsigned int
TPMIO_WriteRegister(
	_In_		unsigned int		PunRegisterAddress,
	_In_		BYTE				PbRegisterValue);

#ifdef __cplusplus
}
#endif
