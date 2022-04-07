/**
 *	@brief		Declares the TIS related functions
 *	@details
 *	@file		TpmDeviceAccess/TPM_TIS.h
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

#ifndef __TPM_TIS_H__
#define __TPM_TIS_H__

#include "StdInclude.h"

// TIS Definitions
/// Base address for TIS
#define TIS_BASE_ADDRESS 0xFED40000
/// Offset for locality 0
#define TIS_LOCALITY0OFFSET (TIS_BASE_ADDRESS + 0x0000)
/// Offset for locality 1
#define TIS_LOCALITY1OFFSET (TIS_BASE_ADDRESS + 0x1000)
/// Offset for locality 2
#define TIS_LOCALITY2OFFSET (TIS_BASE_ADDRESS + 0x2000)
/// Offset for locality 3
#define TIS_LOCALITY3OFFSET (TIS_BASE_ADDRESS + 0x3000)
/// Offset for locality 4
#define TIS_LOCALITY4OFFSET (TIS_BASE_ADDRESS + 0x4000)

/// Locality 0
#define TIS_LOCALITY_0				0x00
/// Locality 1
#define TIS_LOCALITY_1				0x01
/// Locality 2
#define TIS_LOCALITY_2				0x02
/// Locality 3
#define TIS_LOCALITY_3				0x03
/// Locality 4
#define TIS_LOCALITY_4				0x04

// TPM Interface Registers
/// Register offset for TPM Access register
#define TIS_TPM_ACCESS 0x00000000
/// Register offset for TPM Status register
#define TIS_TPM_STS 0x00000018
/// Register offset for TPM Burst Count register
#define TIS_TPM_BURSTCOUNT 0x00000019
/// Register offset for TPM Data FIFO register
#define TIS_TPM_DATA_FIFO 0x00000024

/// TPM Access register bit for access valid
#define TIS_TPM_ACCESS_VALID 0x80
/// TPM Access register bit for active locality
#define TIS_TPM_ACCESS_ACTIVELOCALITY 0x20
/// TPM Access register bit for access been seized
#define TIS_TPM_ACCESS_BEENSEIZED 0x10
/// TPM Access register bit for access seize
#define TIS_TPM_ACCESS_SEIZE 0x08
/// TPM Access register bit for pending request
#define TIS_TPM_ACCESS_PENDINGREQUEST 0x04
/// TPM Access register bit for request use
#define TIS_TPM_ACCESS_REQUESTUSE 0x02
/// TPM Access register bit for access establishment
#define TIS_TPM_ACCESS_ESTABLISHMENT 0x01

/// TPM Status register bit for status valid
#define TIS_TPM_STS_VALID 0x80
/// TPM Status register bit for status command ready
#define TIS_TPM_STS_CMDRDY 0x40
/// TPM Status register bit for status go
#define TIS_TPM_STS_GO 0x20
/// TPM Status register bit for status available
#define TIS_TPM_STS_AVAIL 0x10
/// TPM Status register bit for status expect
#define TIS_TPM_STS_EXPECT 0x08
/// TPM Status register bit for status retry
#define TIS_TPM_STS_RETRY 0x02

// TIS timeouts, use default values only
/// TIS Timeout A
#define TIMEOUT_A 750
/// TIS Timeout B
#define TIMEOUT_B 2000
/// TIS Timeout C
#define TIMEOUT_C 750
/// TIS Timeout D
#define TIMEOUT_D 750

/**
 *	@brief		Keep the locality active between TPM commands. If not set, the locality would be released after a TPM response
 *				and requested again before the next TPM command.
 *	@details
 */
void
TIS_KeepLocalityActive();

/**
 *	@brief		Read the value of a TIS register
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PusRegOffset	Register offset
 *	@param		PbRegSize		Register size
 *	@param		PpValue			Pointer to the value
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_LOCALITY_NOT_SUPPORTED	Given locality is not supported
 *	@retval		RC_E_BAD_PARAMETER			Invalid register size requested
 */
_Check_return_
UINT32
TIS_ReadRegister(
	_In_	BYTE	PbLocality,
	_In_	UINT16	PusRegOffset,
	_In_	BYTE	PbRegSize,
	_Out_	void*	PpValue);

/**
 *	@brief		Write the value into a TIS register
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PusRegOffset	Register offset
 *	@param		PbRegSize		Register size
 *	@param		PunValue		Value to write
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_LOCALITY_NOT_SUPPORTED	Given locality is not supported
 *	@retval		RC_E_BAD_PARAMETER			Invalid register size requested
 */
_Check_return_
UINT32
TIS_WriteRegister(
	_In_	BYTE	PbLocality,
	_In_	UINT16	PusRegOffset,
	_In_	BYTE	PbRegSize,
	_In_	UINT32	PunValue);

/**
 *	@brief		Read the value from the access register
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PpbValue		Pointer to a byte value
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_COMPONENT_NOT_FOUND	Component not found
 */
_Check_return_
UINT32
TIS_ReadAccessRegister(
	_In_	BYTE	PbLocality,
	_Out_	BYTE*	PpbValue);

/**
 *	@brief		Read the value from the status register.
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PpbValue		Pointer to a byte value
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_LOCALITY_NOT_ACTIVE	The locality is not active.
 *	@retval		...							Error codes from TIS_ReadRegister function
 */
_Check_return_
UINT32
TIS_ReadStsRegister(
	_In_	BYTE	PbLocality,
	_Out_	BYTE*	PpbValue);

/**
 *	@brief		Writes the value into the status register
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PbValue			Byte value to write
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_LOCALITY_NOT_ACTIVE	The locality is not active.
 *	@retval		...							Error codes from TIS_WriteRegister function
 */
_Check_return_
UINT32
TIS_WriteStsRegister(
	_In_	BYTE	PbLocality,
	_In_	BYTE	PbValue);

/**
 *	@brief		Returns the value of TPM.ACCESS.VALID
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PpbFlag			Pointer to a BOOL flag
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from TIS_ReadAccessRegister function
 */
_Check_return_
UINT32
TIS_IsAccessValid(
	_In_	BYTE	PbLocality,
	_Out_	BOOL	*PpbFlag);

/**
 *	@brief		Returns the value of TPM.ACCESS.ACTIVE.LOCALITY
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PpbFlag			Pointer to a BOOL flag
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from TIS_ReadAccessRegister function
 */
_Check_return_
UINT32
TIS_IsActiveLocality(
	_In_	BYTE	PbLocality,
	_Out_	BOOL	*PpbFlag);

/**
 *	@brief		Release the currently active locality
 *	@details
 *
 *	@param		PbLocality		Locality value
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from TIS_WriteRegister function
 */
_Check_return_
UINT32
TIS_ReleaseActiveLocality(
	_In_	BYTE	PbLocality);

/**
 *	@brief		Requests ownership of the TPM
 *	@details
 *
 *	@param		PbLocality		Locality value
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from TIS_WriteRegister function
 */
_Check_return_
UINT32
TIS_RequestUse(
	_In_	BYTE	PbLocality);

/**
 *	@brief		Returns the value of TPM.STS.BURSTCOUNT
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PpusBurstCount	Pointer to the Burst Count variable
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_LOCALITY_NOT_ACTIVE	Not an active locality
 *	@retval		...							Error codes from:
 *												TIS_IsActiveLocality
 *												TIS_ReadRegister function
 */
_Check_return_
UINT32
TIS_GetBurstCount(
	_In_	BYTE	PbLocality,
	_Out_	UINT16*	PpusBurstCount);

/**
 *	@brief		Returns the value of TPM.STS.COMMAND.READY
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PpbFlag			Pointer to a BOOL flag
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from TIS_ReadStsRegister function
 */
_Check_return_
UINT32
TIS_IsCommandReady(
	_In_	BYTE	PbLocality,
	_Out_	BOOL	*PpbFlag);

/**
 *	@brief		Aborts the currently running action by writing 1 to Command Ready
 *	@details
 *
 *	@param		PbLocality		Locality value
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from TIS_WriteStsRegister function
 */
_Check_return_
UINT32
TIS_Abort(
	_In_	BYTE	PbLocality);

/**
 *	@brief		Lets the TPM execute a command
 *	@details
 *
 *	@param		PbLocality		Locality value
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from TIS_WriteStsRegister function
 */
_Check_return_
UINT32
TIS_Go(
	_In_	BYTE	PbLocality);

/**
 *	@brief		Returns the value of TPM.STS.DATA.AVAIL
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PpbFlag			Pointer to a BOOL flag
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from TIS_ReadStsRegister function
 */
_Check_return_
UINT32
TIS_IsDataAvailable(
	_In_	BYTE	PbLocality,
	_Out_	BOOL	*PpbFlag);

/**
 *	@brief		Lets the TPM repeat the last response
 *	@details
 *
 *	@param		PbLocality		Locality value
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from TIS_ReadStsRegister function
 */
_Check_return_
UINT32
TIS_Retry(
	_In_	BYTE	PbLocality);

/**
 *	@brief		Send data block to the TPM
 *	@details	Send a data block to the TPM TIS data FIFO under consideration of the
 *				TIS communication protocol
 *
 *	@param		PbLocality		Locality value
 *	@param		PrgbByteBuf		Bytes to send
 *	@param		PusLen			Length of bytes
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function. PrgbByteBuf is NULL.
 *	@retval		RC_E_LOCALITY_NOT_ACTIVE	Locality not active
 *	@retval		RC_E_TPM_NO_DATA_AVAILABLE	TPM no data available
 *	@retval		RC_E_NOT_READY				Not ready
 *	@retval		RC_E_TPM_TRANSMIT_DATA		Error during transmit data
 *	@retval		...							Error codes from:
 *												TIS_RequestUse,
 *												TIS_IsActiveLocality,
 *												TIS_IsCommandReady,
 *												TIS_Abort,
 *												TIS_GetBurstCount,
 *												TIS_WriteRegister,
 *												TIS_ReadStsRegister function
 */
_Check_return_
UINT32
TIS_SendLPC(
	_In_					BYTE		PbLocality,
	_In_bytecount_(PusLen)	const BYTE*	PrgbByteBuf,
	_In_					UINT16		PusLen);

/**
 *	@brief		Read a data block from the TPM TIS port
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PrgbByteBuf		Pointer to buffer to store bytes
 *	@param		PpusLen			Pointer to the length
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function. PrgbByteBuf or PpusLen is NULL.
 *	@retval		RC_E_INSUFFICIENT_BUFFER	Insufficient buffer size
 *	@retval		RC_E_LOCALITY_NOT_ACTIVE	Locality not active
 *	@retval		RC_E_TPM_NO_DATA_AVAILABLE	TPM no data available
 *	@retval		RC_E_TPM_RECEIVE_DATA		Error TPM Receive data
 *	@retval		RC_E_NOT_READY				Not ready
 *	@retval		RC_E_TPM_TRANSMIT_DATA		Error during transmit data
 *	@retval		...							Error codes from:
 *												TIS_IsActiveLocality,
 *												TIS_ReadStsRegister,
 *												TIS_GetBurstCount,
 *												TIS_ReadRegister,
 *												TIS_ReadStsRegister,
 *												TIS_IsCommandReady,
 *												TIS_Abort,
 *												TIS_Retry function
 */
_Check_return_
UINT32
TIS_ReadLPC(
	_In_					BYTE	PbLocality,
	_Out_bytecap_(*PpusLen)	BYTE*	PrgbByteBuf,
	_Inout_					UINT16*	PpusLen);

/**
 *	@brief		Sends the Transceive Buffer to the TPM and returns the response
 *	@details
 *
 *	@param		PbLocality		Locality value
 *	@param		PrgbTxBuffer	Pointer to Transceive buffer
 *	@param		PusTxLen		Length of the Transceive buffer
 *	@param		PrgbRxBuffer	Pointer to a Receive buffer
 *	@param		PpusRxLen		Pointer to the length of the Receive buffer
 *	@param		PunMaxDuration	The maximum duration of the command in microseconds
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_TPM_NO_DATA_AVAILABLE	TPM no data available
 *	@retval		...							Error codes from:
 *												TIS_SendLPC,
 *												TIS_IsDataAvailable,
 *												TIS_ReadLPC,
 *												TIS_ReleaseActiveLocality function
 */
_Check_return_
UINT32
TIS_TransceiveLPC(
	_In_						BYTE		PbLocality,
	_In_bytecount_(PusTxLen)	const BYTE*	PrgbTxBuffer,
	_In_						UINT16		PusTxLen,
	_Out_bytecap_(*PpusRxLen)	BYTE*		PrgbRxBuffer,
	_Inout_						UINT16*		PpusRxLen,
	_In_						UINT32		PunMaxDuration);

#endif //__TPM_TIS_H__
