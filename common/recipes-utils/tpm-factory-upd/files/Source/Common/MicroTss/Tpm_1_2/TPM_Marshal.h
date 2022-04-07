/**
 *	@brief		Declares methods to marshal and unmarshal TPM1.2 structures and types.
 *	@details	This module provides marshalling and unmarshalling function for
 *				structures and types needed at the moment.
 *	@file		TPM_Marshal.h
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

#include "TPM_Types.h"

#ifdef __cplusplus
extern "C" {
#endif

//********************************************************************************************************
//
// Marshal and unmarshal for types
//
//********************************************************************************************************

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TSS_TPM_TAG
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TSS_TPM_TAG
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_TAG_Marshal(
	_In_	TSS_TPM_TAG*	PpSource,
	_Inout_ TSS_BYTE**		PprgbBuffer,
	_Inout_	TSS_INT32*		PpnBufferSize);

/**
 *	@brief		Unmarshal type TSS_TPM_TAG
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_TAG_Unmarshal(
	_Out_	TSS_TPM_TAG*	PpTarget,
	_Inout_ TSS_BYTE**		PprgbBuffer,
	_Inout_	TSS_INT32*		PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TSS_TPM_COMMAND_CODE
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TSS_TPM_COMMAND_CODE
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_COMMAND_CODE_Marshal(
	_In_	TSS_TPM_COMMAND_CODE*	PpSource,
	_Inout_ TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TPM_STARTUP_TYPE
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TPM_STARTUP_TYPE
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STARTUP_TYPE_Marshal(
	_In_	TSS_TPM_STARTUP_TYPE*	PpSource,
	_Inout_ TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TSS_TPM_RESULT
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Unmarshal type TSS_TPM_RESULT
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_RESULT_Unmarshal(
	_Out_	TSS_TPM_RESULT*	PpTarget,
	_Inout_	TSS_BYTE**		PprgbBuffer,
	_Inout_	TSS_INT32*		PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TPM_STRUCTURE_TAG
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Unmarshal type TPM_STRUCTURE_TAG
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STRUCTURE_TAG_Unmarshal(
	_Out_	TSS_TPM_STRUCTURE_TAG*	PpTarget,
	_Inout_	TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TPM_VERSION_BYTE
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Unmarshal type TPM_VERSION_BYTE
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_VERSION_BYTE_Unmarshal(
	_Out_	TSS_TPM_VERSION_BYTE*	PpTarget,
	_Inout_	TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TSS_TPM_CAPABILITY_AREA
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TSS_TPM_CAPABILITY_AREA
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_CAPABILITY_AREA_Marshal(
	_In_	TSS_TPM_CAPABILITY_AREA*	PpSource,
	_Inout_	TSS_BYTE**					PprgbBuffer,
	_Inout_	TSS_INT32*					PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TPM_ENC_SCHEME
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TPM_ENC_SCHEME
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_ENC_SCHEME_Marshal(
	_In_	TSS_TPM_ENC_SCHEME*	PpSource,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal type TPM_ENC_SCHEME
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_ENC_SCHEME_Unmarshal(
	_Out_	TSS_TPM_ENC_SCHEME*	PpTarget,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TPM_SIG_SCHEME
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TPM_SIG_SCHEME
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_SIG_SCHEME_Marshal(
	_In_	TSS_TPM_SIG_SCHEME*	PpSource,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal type TPM_SIG_SCHEME
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_SIG_SCHEME_Unmarshal(
	_Out_	TSS_TPM_SIG_SCHEME*	PpTarget,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TPM_PROTOCOL_ID
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TPM_PROTOCOL_ID
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_PROTOCOL_ID_Marshal(
	_In_	TSS_TPM_PROTOCOL_ID*	PpSource,
	_Inout_	TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TPM_KEY_USAGE
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TPM_KEY_USAGE
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_USAGE_Marshal(
	_In_	TSS_TPM_KEY_USAGE*	PpSource,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal type TPM_KEY_USAGE
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_USAGE_Unmarshal(
	_Out_	TSS_TPM_KEY_USAGE*	PpTarget,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TPM_KEY_FLAGS
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TPM_KEY_FLAGS
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_FLAGS_Marshal(
	_In_	TSS_TPM_KEY_FLAGS*	PpSource,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal type TPM_KEY_FLAGS
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_FLAGS_Unmarshal(
	_Out_	TSS_TPM_KEY_FLAGS*	PpTarget,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TPM_AUTH_DATA_USAGE
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TPM_AUTH_DATA_USAGE
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTH_DATA_USAGE_Marshal(
	_In_	TSS_TPM_AUTH_DATA_USAGE*	PpSource,
	_Inout_	TSS_BYTE**					PprgbBuffer,
	_Inout_	TSS_INT32*					PpnBufferSize);

/**
 *	@brief		Unmarshal type TPM_KEY_FLAGS
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTH_DATA_USAGE_Unmarshal(
	_Out_	TSS_TPM_AUTH_DATA_USAGE*	PpTarget,
	_Inout_	TSS_BYTE**					PprgbBuffer,
	_Inout_	TSS_INT32*					PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal type TSS_TPM_AUTHHANDLE
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal type TSS_TPM_AUTHHANDLE
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTHHANDLE_Marshal(
	_In_	TSS_TPM_AUTHHANDLE*	PpSource,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//********************************************************************************************************
//
// Marshal and unmarshal for structures
//
//********************************************************************************************************

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure TPM_VERSION
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Unmarshal type TPM_VERSION
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_VERSION_Unmarshal(
	_Out_	TSS_TPM_VERSION*	PpTarget,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure TSS_TPM_CAP_VERSION_INFO
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Unmarshal type TSS_TPM_CAP_VERSION_INFO
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case of vendor specific data buffer is too small
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_CAP_VERSION_INFO_Unmarshal(
	_Out_	TSS_TPM_CAP_VERSION_INFO*	PpTarget,
	_Inout_	TSS_BYTE**					PprgbBuffer,
	_Inout_	TSS_INT32*					PpnBufferSize);

/**
 *	@brief		Unmarshal type TPM_PERMANENT_FLAGS
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_PERMANENT_FLAGS_Unmarshal(
	_Out_	TSS_TPM_PERMANENT_FLAGS*		PpTarget,
	_Inout_	TSS_BYTE**						PprgbBuffer,
	_Inout_	TSS_INT32*						PpnBufferSize);

/**
 *	@brief		Unmarshal type TPM_STCLEAR_FLAGS
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STCLEAR_FLAGS_Unmarshal(
	_Out_	TSS_TPM_STCLEAR_FLAGS*			PpTarget,
	_Inout_	TSS_BYTE**						PprgbBuffer,
	_Inout_	TSS_INT32*						PpnBufferSize);

/**
 *	@brief		Unmarshal TCPA_VERSION structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TCPA_VERSION_Unmarshal(
	_Out_	TSS_TCPA_VERSION*	PpTarget,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Marshal type TSS_TPM_AUTH_IN
 *	@details
 *
 *	@param		PpSource		Source type to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS	The operation completed successfully.
 *	@retval		...			Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTH_IN_Marshal(
	_In_	TSS_TPM_AUTH_IN*	PpSource,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure TSS_TPM_AUTHDATA
//--------------------------------------------------------------------------------------------------------
/**
 *	@brief		Marshal TSS_TPM_AUTHDATA structure
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTHDATA_Marshal(
	_In_	TSS_TPM_AUTHDATA*	PpSource,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal TSS_TPM_AUTHDATA structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTHDATA_Unmarshal(
	_Out_	TSS_TPM_AUTHDATA*	PpTarget,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure TSS_TPM_NONCE
//--------------------------------------------------------------------------------------------------------
/**
 *	@brief		Marshal TSS_TPM_NONCE structure
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_NONCE_Marshal(
	_In_	TSS_TPM_NONCE*	PpSource,
	_Inout_ TSS_BYTE**		PprgbBuffer,
	_Inout_	TSS_INT32*		PpnBufferSize);

/**
 *	@brief		Unmarshal TSS_TPM_NONCE structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_NONCE_Unmarshal(
	_Out_	TSS_TPM_NONCE*	PpTarget,
	_Inout_ TSS_BYTE**		PprgbBuffer,
	_Inout_	TSS_INT32*		PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure TPM_RSA_KEY_PARMS
//--------------------------------------------------------------------------------------------------------
/**
 *	@brief		Marshal TPM_RSA_KEY_PARMS structure
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_RSA_KEY_PARMS_Marshal(
	_In_	TSS_TPM_RSA_KEY_PARMS*	PpSource,
	_Inout_ TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

/**
 *	@brief		Unmarshal TPM_RSA_KEY_PARMS structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_RSA_KEY_PARMS_Unmarshal(
	_Out_	TSS_TPM_RSA_KEY_PARMS*	PpTarget,
	_Inout_ TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure TPM_KEY_PARMS
//--------------------------------------------------------------------------------------------------------
/**
 *	@brief		Marshal TPM_KEY_PARMS structure
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_PARMS_Marshal(
	_In_	TSS_TPM_KEY_PARMS*	PpSource,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal TPM_KEY_PARMS structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_PARMS_Unmarshal(
	_Out_	TSS_TPM_KEY_PARMS*	PpTarget,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure TPM_STORE_PUBKEY
//--------------------------------------------------------------------------------------------------------
/**
 *	@brief		Marshal TPM_STORE_PUBKEY structure
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STORE_PUBKEY_Marshal(
	_In_	TSS_TPM_STORE_PUBKEY*	PpSource,
	_Inout_ TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

/**
 *	@brief		Unmarshal TPM_STORE_PUBKEY structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STORE_PUBKEY_Unmarshal(
	_Out_	TSS_TPM_STORE_PUBKEY*	PpTarget,
	_Inout_ TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

/**
 *	@brief		Unmarshal TPM_PUBKEY structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_PUBKEY_Unmarshal(
	_Out_	TSS_TPM_PUBKEY*		PpTarget,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure TPM_STRUCT_VER
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal TPM_STRUCT_VER structure
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STRUCT_VER_Marshal(
	_In_	TSS_TPM_STRUCT_VER*	PpSource,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal TPM_STRUCT_VER structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal to
 *	@param		PprgbBuffer		Pointer to a buffer to read from
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STRUCT_VER_Unmarshal(
	_Out_	TSS_TPM_STRUCT_VER*	PpTarget,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure TPM_KEY
//--------------------------------------------------------------------------------------------------------
/**
 *	@brief		Marshal TPM_KEY structure
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_Marshal(
	_In_	TSS_TPM_KEY*		PpSource,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal TPM_KEY structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal to
 *	@param		PprgbBuffer		Pointer to a buffer to read from
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_Unmarshal(
	_Out_	TSS_TPM_KEY*		PpTarget,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal a TPM_DA_INFO or TPM_DA_INFO_LIMITED structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal to
 *	@param		PprgbBuffer		Pointer to a buffer to read from
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS		The operation completed successfully.
 *	@retval		RC_E_FAIL		Invalid tag
 *	@retval		...				Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_DA_INFO_Unmarshal(
	_Out_	TSS_TPM_DA_INFO*	PpTarget,
	_Inout_ TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal IFX_FIELDUPGRADEINFO structure
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS				The operation completed successfully.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case of vendor specific data buffer is too small
 *	@retval		...						Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_IFX_FIELDUPGRADEINFO_Unmarshal(
	_Out_	TSS_IFX_FIELDUPGRADEINFO*	PpTarget,
	_Inout_ TSS_BYTE**					PprgbBuffer,
	_Inout_	TSS_INT32*					PpnBufferSize);

#ifdef __cplusplus
}
#endif
