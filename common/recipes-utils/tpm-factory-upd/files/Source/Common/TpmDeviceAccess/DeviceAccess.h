/**
 *	@brief		Declares the Device memory access routines
 *	@details
 *	@file		DeviceAccess.h
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

#include "StdInclude.h"

/**
 *	@brief		Initialize the device access
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@retval		RC_SUCCESS		The operation completed successfully.
 */
_Check_return_
unsigned int
DeviceAccess_Initialize(
	_In_	BYTE	PbLocality);

/**
 *	@brief		UnInitialize the device access
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@retval		RC_SUCCESS		The operation completed successfully.
 */
_Check_return_
unsigned int
DeviceAccess_Uninitialize(
	_In_	BYTE	PbLocality);

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
	_In_	unsigned int PunMemoryAddress);

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
	_In_	BYTE			PbData);

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
	_In_	unsigned int	PunMemoryAddress);

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
	_In_	unsigned short	PusData);
