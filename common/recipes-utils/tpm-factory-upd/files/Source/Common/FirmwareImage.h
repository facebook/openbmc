/**
 *	@brief		Declares methods to access a firmware image
 *	@details	This module provides helper functions to access a firmware image.
 *	@file		FirmwareImage.h
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
#include <StdInclude.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Unique identifier for the IfxFirmwareImage structure
static const GUID EFI_IFXTPM_FIRMWARE_IMAGE_GUID =
{ 0x1a53667a, 0xfb12, 0x479e, { 0xac, 0x58, 0xec, 0x99, 0x58, 0x86, 0x10, 0x93 } };

/// Unique identifier for newer version of the IfxFirmwareImage structure not compatible with this tool.
static const GUID EFI_IFXTPM_FIRMWARE_IMAGE_2_GUID =
{ 0x1a53667a, 0xfb12, 0x479e, { 0xac, 0x58, 0xec, 0x99, 0x58, 0x86, 0x10, 0x94 } };

/// The ID of the signing key used to create the firmware image signature
#define SIG_KEY_ID_1 0x0001

/// Indicates TPM1.2 firmware update.
#define DEVICE_TYPE_TPM_12 0x01

/// Indicates TPM2.0 firmware update.
#define DEVICE_TYPE_TPM_20 0x02

/// The maximum supported number of source TPM firmware versions in an image file
#define MAX_SOURCE_VERSIONS_COUNT 8

/**
 *	@brief		TPM Target State bit field
 *	@details	This structure contains bit flags indicating the target state of the TPM after the firmware update.
 */
typedef struct tdBITFIELD_TPM_TARGET_STATE
{
	/// This bit indicates if firmware update with this image will cause the TPM to be reset to factory defaults.
	unsigned int factoryDefaults		: 1;
	/// This bit indicates if TPM Firmware Update will update the TPM to Common Criteria certified firmware.
	unsigned int commonCriteria		: 1;
	/// This bit indicates if TPM Firmware Update will update the TPM to FIPS certified firmware.
	unsigned int fips					: 1;
	/// Bits reserved for future use.
	unsigned int reserved3			: 29;
} BITFIELD_TPM_TARGET_STATE;

/**
 *	@brief		TPM firmware image capabilities bit field
 *	@details	This structure contains bit flags indicating capabilities of the TPM firmware image.
 */
typedef struct tdBITFIELD_FIRMWARE_IMAGE_CAPABILITIES
{
	/// This bit indicates if the firmware image supports matching a unique ID in invalid firmware mode.
	unsigned int invalidFirmwareMode_matchUniqueID	: 1;
	/// Bits reserved for future use.
	unsigned int reserved1							: 31;
} BITFIELD_FIRMWARE_IMAGE_CAPABILITIES;

/**
 *	@brief		Firmware image structure
 *	@details	This structure describes the firmware image that is passed to the driver in EFI_FIRMWARE_MANAGEMENT_PROTOCOL.SetImage and
 *				EFI_FIRMWARE_MANAGEMENT_PROTOCOL.CheckImage. The structure parses all fields of the firmware image and copies them into the
 *				structure, except for the parameter block and the firmware. Parameter block and the firmware are referenced by pointer and
 *				length only.
 */
typedef struct tdIfxFirmwareImage
{
	/// Unique identifier: Must be EFI_IFXTPM_FIRMWARE_IMAGE_GUID, a different version indicates a breaking change in firmware image file structure.
	/// If the unique identifier is EFI_IFXTPM_FIRMWARE_IMAGE_2_GUID then the firmware image is not supported by this version of the tool
	/// and a newer version of the tool is required to process the firmware image.
	GUID unique;
	/// Allowed TPM family where the firmware image can be installed on. Either DEVICE_TYPE_TPM_12 or DEVICE_TYPE_TPM_20.
	unsigned char bSourceTpmFamily;
	/// Count of the allowed source versions
	unsigned short usSourceVersionsCount;
	/// Allowed version names (e.g. "5.12.3456.0") where the firmware update can be applied to.
	wchar_t rgwszSourceVersions[MAX_SOURCE_VERSIONS_COUNT][64];
	/// Target TPM family. Either DEVICE_TYPE_TPM_12 or DEVICE_TYPE_TPM_20.
	unsigned char bTargetTpmFamily;
	/// Size in bytes of wszTargetVersion. Uses big-endian format.
	unsigned short  usTargetVersionSize;
	/// Target version name (e.g. "5.12.3456.0") wszTargetVersion includes a terminating NULL character.
	/// Size of wszTargetVersion is usTargetVersionSize.
	wchar_t wszTargetVersion[64];
	/// Size in bytes of the policy parameter block in rgbPolicyParameterBlock. Uses big-endian format.
	unsigned short  usPolicyParameterBlockSize;
	/// Policy parameter block. Size is usPolicyParameterBlockSize.
	unsigned char* rgbPolicyParameterBlock;
	/// Size in bytes of the firmware block in rgbFirmware. Uses big-endian format.
	unsigned int unFirmwareSize;
	/// Firmware block. Size is unFirmwareSize.
	unsigned char* rgbFirmware;
	/// Version number indicating the structure version of the firmware image. Will be increased with every non-breaking firmware image file structure change.
	unsigned short  usImageStructureVersion;
	/// Target state
	BITFIELD_TPM_TARGET_STATE bfTargetState;
	/// Firmware image capabilities
	BITFIELD_FIRMWARE_IMAGE_CAPABILITIES bfCapabilities;
	/// Count of the allowed source versions (unsigned integer representation)
	unsigned short  usIntSourceVersionCount;
	/// Allowed source versions (unsigned integer representation)
	unsigned int rgunIntSourceVersions[MAX_SOURCE_VERSIONS_COUNT];
	/// Checksum over the IfxFirmwareImage structure excluding the unChecksum field. Uses big-endian format.
	unsigned int unChecksum;
	/// Key identifier for signature
	unsigned short  usSignatureKeyId;
	/// Byte array for signature
	BYTE rgbSignature[256];
} IfxFirmwareImage;

/**
 *	@brief		Function to unmarshal a IfxFirmwareImage from a byte stream
 *	@details	This function unmarshals the structures parameters. For all fields with variable
 *				size the output structure contains a pointer to the buffers address where the data can be found.
 *
 *	@param		PpTarget				Pointer to the target structure; must be allocated by the caller
 *	@param		PprgbBuffer				Pointer to a byte stream containing the firmware image data; will be increased during execution by the amount of unmarshalled bytes
 *	@param		PpnBufferSize			Size of elements readable from the byte stream; will be decreased during execution by the amount of unmarshalled bytes
 *	@retval		RC_SUCCESS				In case the firmware is updatable with the given firmware
 *	@retval		RC_E_BAD_PARAMETER		An invalid parameter was passed to the function or an error occurred at unmarshal.
 *	@retval		RC_E_BUFFER_TOO_SMALL	In case an output buffer is too small for an input byte array
 *
 */
_Check_return_
unsigned int
FirmwareImage_Unmarshal(
	_Out_	IfxFirmwareImage*	PpTarget,
	_Inout_	unsigned char**		PprgbBuffer,
	_Inout_	int*				PpnBufferSize);

#ifdef __cplusplus
}
#endif
