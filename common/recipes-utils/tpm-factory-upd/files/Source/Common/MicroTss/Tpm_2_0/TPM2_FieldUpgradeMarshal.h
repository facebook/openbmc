/**
 *	@brief		Declares methods to marshal and unmarshal TPM1.2 and TPM2.0 commands, structures and types.
 *	@details	This module provides marshalling and unmarshalling function for all TPM1.2 and TPM2.0 commands,
 *				structures and types defined in the Infineon Secure Boot Loader Specification.
 *	@file		TPM2_FieldUpgradeMarshal.h
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
#include "TPM2_FieldUpgradeTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

//********************************************************************************************************
//
// Marshaling helper functions
//
//********************************************************************************************************

/**
 *	@brief		Marshal helper to calculate the LRC
 *	@details
 *
 *	@param		PrgbData		Pointer to a buffer to calculate the LRC from
 *	@param		PunDataSize		Size of the buffer
 *
 *	@retval		RC_SUCCESS	The operation completed successfully.
 *	@retval		...			Error codes from called functions.
 */
TSS_BYTE
TSS_CalcLRC(
	_In_bytecount_(PunDataSize) uint8_t*	PrgbData,
	_In_						uint32_t	PunDataSize);

//********************************************************************************************************
//
// Marshal and unmarshal for types
//
//********************************************************************************************************
#define TSS_DecryptKeyId_d_Marshal TSS_UINT32_Marshal ///< Marshal type DecryptKeyId_d
#define TSS_DecryptKeyId_d_Unmarshal TSS_UINT32_Unmarshal ///< Unmarshal type DecryptKeyId_d
#define TSS_VersionNumber_d_Marshal TSS_UINT32_Marshal ///< Marshal type VersionNumber_d
#define TSS_VersionNumber_d_Unmarshal TSS_UINT32_Unmarshal ///< Unmarshal type VersionNumber_d
#define TSS_VersionNumber_d_Array_Marshal TSS_UINT32_Array_Marshal ///< Marshal array of type VersionNumber_d
#define TSS_VersionNumber_d_Array_Unmarshal TSS_UINT32_Array_Unmarshal ///< Unmarshal array of type VersionNumber_d
#define TSS_SubCmd_d_Marshal TSS_UINT8_Marshal ///< Marshal type SubCmd_d
#define TSS_SecurityModuleStatus_d_Unmarshal TSS_UINT16_Unmarshal ///< Unmarshal type SecurityModuleStatus_d

//********************************************************************************************************
//
// Marshal and unmarshal for structures
//
//********************************************************************************************************

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sMessageDigest_d
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal structure of type sMessageDigest_d
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sMessageDigest_d_Marshal(
	_In_	sMessageDigest_d*		PpSource,
	_Inout_	TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sMessageDigest_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sMessageDigest_d_Unmarshal(
	_Out_	sMessageDigest_d*		PpTarget,
	_Inout_	TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sFirmwarePackage_d
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal structure of type sFirmwarePackage_d
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sFirmwarePackage_d_Marshal(
	_In_	sFirmwarePackage_d*		PpSource,
	_Inout_	TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sFirmwarePackage_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sFirmwarePackage_d_Unmarshal(
	_Out_	sFirmwarePackage_d*		PpTarget,
	_Inout_	TSS_BYTE**				PprgbBuffer,
	_Inout_	TSS_INT32*				PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sVersions_d
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal structure of type sVersions_d
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sVersions_d_Marshal(
	_In_	sVersions_d*		PpSource,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sVersions_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sVersions_d_Unmarshal(
	_Out_	sVersions_d*		PpTarget,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sSignedAttributes_d
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal structure of type sSignedAttributes_d
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSignedAttributes_d_Marshal(
	_In_	sSignedAttributes_d*		PpSource,
	_Inout_	TSS_BYTE**					PprgbBuffer,
	_Inout_	TSS_INT32*					PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sSignedAttributes_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSignedAttributes_d_Unmarshal(
	_Out_	sSignedAttributes_d*		PpTarget,
	_Inout_	TSS_BYTE**					PprgbBuffer,
	_Inout_	TSS_INT32*					PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sSignerInfo_d
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal structure of type sSignerInfo_d
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSignerInfo_d_Marshal(
	_In_	sSignerInfo_d*		PpSource,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sSignerInfo_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSignerInfo_d_Unmarshal(
	_Out_	sSignerInfo_d*		PpTarget,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sSignedData_d
//--------------------------------------------------------------------------------------------------------

/**
 *	@brief		Marshal structure of type sSignedData_d
 *	@details
 *
 *	@param		PpSource		Pointer to the source to marshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSignedData_d_Marshal(
	_In_	sSignedData_d*		PpSource,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sSignedData_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSignedData_d_Unmarshal(
	_Out_	sSignedData_d*		PpTarget,
	_Inout_	TSS_BYTE**			PprgbBuffer,
	_Inout_	TSS_INT32*			PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sSecurityModuleLogicInfo_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSecurityModuleLogicInfo_d_Unmarshal(
	_Out_	sSecurityModuleLogicInfo_d*			PpTarget,
	_Inout_	TSS_BYTE**							PprgbBuffer,
	_Inout_	TSS_INT32*							PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sSecurityModuleLogicInfo2_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSecurityModuleLogicInfo2_d_Unmarshal(
	_Out_	sSecurityModuleLogicInfo2_d*		PpTarget,
	_Inout_	TSS_BYTE**							PprgbBuffer,
	_Inout_	TSS_INT32*							PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sKeyList_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sKeyList_d_Unmarshal(
	_Out_	sKeyList_d*		PpTarget,
	_Inout_	TSS_BYTE**		PprgbBuffer,
	_Inout_	TSS_INT32*		PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sSecurityModuleLogic_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSecurityModuleLogic_d_Unmarshal(
	_Out_	sSecurityModuleLogic_d*		PpTarget,
	_Inout_	TSS_BYTE**					PprgbBuffer,
	_Inout_	TSS_INT32*					PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type sFirmwarePackages_d
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sFirmwarePackages_d_Unmarshal(
	_Out_	sFirmwarePackages_d*		PpTarget,
	_Inout_	TSS_BYTE**					PprgbBuffer,
	_Inout_	TSS_INT32*					PpnBufferSize);

/**
 *	@brief		Unmarshal structure of type TPML_MAX_BUFFER
 *	@details
 *
 *	@param		PpTarget		Pointer to the target to unmarshal
 *	@param		PprgbBuffer		Pointer to a buffer to store the source
 *	@param		PpnBufferSize	Size of the buffer
 *
 *	@retval		RC_SUCCESS			The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER	An invalid parameter was passed to the function.
 *	@retval		...					Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPML_MAX_BUFFER_Unmarshal(
	_Out_ TSS_TPML_MAX_BUFFER* PpTarget,
	_Inout_	TSS_BYTE **PprgbBuffer,
	_Inout_	TSS_INT32 *PpnBufferSize);

/**
 *	@brief		Unmarshal union of type TPMU_VENDOR_CAPABILITY
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
TSS_TPMU_VENDOR_CAPABILITY_Unmarshal(
	_Inout_	TSS_TPMU_CAPABILITIES* PpTarget,
	_Inout_	TSS_BYTE **PprgbBuffer,
	_Inout_	TSS_INT32 *PpnBufferSize);

#ifdef __cplusplus
}
#endif
