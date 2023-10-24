/**
 *  @brief      Declares methods to access a firmware image
 *  @details    This module provides helper functions to access a firmware image.
 *  @file       FirmwareImage.h
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once
#include <StdInclude.h>
#include "Crypt.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Unique identifier for the IfxFirmwareImage structure V1
static const GUID EFI_IFXTPM_FIRMWARE_IMAGE_GUID =
{ 0x1a53667a, 0xfb12, 0x479e, { 0xac, 0x58, 0xec, 0x99, 0x58, 0x86, 0x10, 0x93 } };

// Unique identifier for the IfxFirmwareImage structure V2, V3
// { 0x1a53667a, 0xfb12, 0x479e, { 0xac, 0x58, 0xec, 0x99, 0x58, 0x86, 0x10, 0x94 } };
// { 0x1a53667a, 0xfb12, 0x479e, { 0xac, 0x58, 0xec, 0x99, 0x58, 0x86, 0x10, 0x95 } };
// ...
/// Definitions for firmware image version
#define FIRMWARE_IMAGE_V1 0x01
#define FIRMWARE_IMAGE_V2 0x02
#define FIRMWARE_IMAGE_V3 0x03

/// The ID of the signing key used to create the firmware image signature
#define SIG_KEY_ID_1 0x0001

/**
 *  @brief      Key record.
 */
typedef struct tdIfxKeyRecord
{
    /// Key identifier
    unsigned short usIdentifier;
    /// Public key (256 bytes)
    const BYTE rgbPublicKey[RSA2048_MODULUS_SIZE];
} IfxKeyRecord;

/**
 *  @brief      List of supported public key records.
 */
static const IfxKeyRecord s_rgPublicKeys[] = {
    {
        SIG_KEY_ID_1,
        {
            0xc9, 0x70, 0xa4, 0x12, 0x73, 0x0d, 0x84, 0xe8, 0x79, 0x3e, 0x5d, 0x64, 0x8a, 0xb3, 0x3a, 0x77,
            0xb8, 0xd4, 0x18, 0x9c, 0x0b, 0xe4, 0xfb, 0xf5, 0x60, 0x09, 0xe7, 0x5e, 0xc4, 0x65, 0x42, 0xa8,
            0x87, 0x62, 0x3f, 0x9d, 0x20, 0x50, 0x09, 0xcf, 0x0a, 0xbf, 0xfd, 0x14, 0x22, 0x9a, 0xf1, 0x29,
            0x33, 0x70, 0xd9, 0xec, 0xde, 0x94, 0xfb, 0xf9, 0xb9, 0x45, 0xdf, 0x56, 0xea, 0x44, 0x0a, 0xa0,
            0xf5, 0x8c, 0x84, 0x55, 0xfd, 0x2f, 0xa8, 0x40, 0xbd, 0x71, 0xae, 0x94, 0x5d, 0xfc, 0x54, 0xdb,
            0x4f, 0x08, 0x71, 0x08, 0xd7, 0x7f, 0xd6, 0x8c, 0x7e, 0xe0, 0xab, 0x63, 0xfb, 0x7c, 0x4a, 0xab,
            0x6e, 0xdc, 0xd0, 0x7a, 0x41, 0x7b, 0x9c, 0xed, 0x55, 0x8f, 0x81, 0xc4, 0x22, 0x1d, 0xbc, 0x80,
            0x64, 0xa0, 0x10, 0x2f, 0xcb, 0x15, 0xcb, 0x04, 0x35, 0x37, 0x15, 0xc9, 0x7d, 0x7b, 0x13, 0x06,
            0x8b, 0xe1, 0x2a, 0x87, 0x40, 0xb3, 0xe0, 0xf1, 0xe6, 0x65, 0xc8, 0x9a, 0x20, 0x9d, 0x85, 0x74,
            0x0b, 0xd8, 0xc4, 0xd4, 0x1e, 0x04, 0x5e, 0x05, 0x4d, 0x8f, 0xb3, 0xc7, 0x64, 0x46, 0x87, 0x4a,
            0x9d, 0x6e, 0xc9, 0xd1, 0xd4, 0xcf, 0x61, 0xd8, 0x83, 0x2f, 0x80, 0x36, 0xbd, 0x98, 0x11, 0xae,
            0xa5, 0x93, 0x18, 0x99, 0xae, 0x9d, 0x13, 0x77, 0x2d, 0x4c, 0x53, 0xda, 0xb5, 0xe4, 0xdd, 0xdc,
            0xe8, 0x60, 0x1e, 0x73, 0x22, 0x02, 0xe6, 0x4b, 0xa1, 0xce, 0x6f, 0x27, 0x12, 0x2d, 0x3d, 0x98,
            0x27, 0x0a, 0x4b, 0x47, 0x53, 0xa3, 0x92, 0x19, 0x69, 0x7c, 0x29, 0x9e, 0xda, 0x83, 0x69, 0x22,
            0xa8, 0x1c, 0x5f, 0x4b, 0x82, 0xce, 0x98, 0x7b, 0x8e, 0x69, 0x2c, 0xc6, 0x1f, 0xe4, 0xc2, 0x0c,
            0x7a, 0x1f, 0x03, 0xd4, 0x15, 0x10, 0xa8, 0xd6, 0xf9, 0x68, 0xc1, 0x75, 0x29, 0xa8, 0x85, 0xb1
        }
    },
};

/// Indicates TPM1.2 firmware update.
#define DEVICE_TYPE_TPM_12 0x01

/// Indicates TPM2.0 firmware update.
#define DEVICE_TYPE_TPM_20 0x02

/// The maximum supported number of source TPM firmware versions in an V1 and V2 image file
#define MAX_SOURCE_VERSIONS_COUNT 8
/// The maximum supported number of source TPM firmware versions in V3 image file
#define MAX_SOURCE_VERSIONS_COUNT_V3 32

/**
 *  @brief      TPM Target State bit field
 *  @details    This structure contains bit flags indicating the target state of the TPM after the firmware update.
 */
typedef struct tdBITFIELD_TPM_TARGET_STATE
{
    /// This bit indicates if firmware update with this image will cause the TPM to be reset to factory defaults.
    unsigned int factoryDefaults : 1;
    /// This bit indicates if TPM Firmware Update will update the TPM to Common Criteria certified firmware.
    unsigned int commonCriteria : 1;
    /// This bit indicates if TPM Firmware Update will update the TPM to FIPS certified firmware.
    unsigned int fips : 1;
    /// Bits reserved for future use.
    unsigned int reserved3 : 29;
} BITFIELD_TPM_TARGET_STATE;

/**
 *  @brief      TPM firmware image capabilities bit field
 *  @details    This structure contains bit flags indicating capabilities of the TPM firmware image.
 */
typedef struct tdBITFIELD_FIRMWARE_IMAGE_CAPABILITIES
{
    /// This bit indicates if the firmware image supports matching a unique ID in invalid firmware mode.
    unsigned int invalidFirmwareMode_matchUniqueID : 1;
    /// This bit indicates if the firmware image allows updates onto the same version.
    unsigned int firmwareImage_allowUpdateSameVersion : 1;
    /// This bit indicates if the firmware image allows firmware recovery.
    unsigned int firmwareImage_allowFirmwareRecovery : 1;
    /// Bits reserved for future use.
    unsigned int reserved3 : 29;
} BITFIELD_FIRMWARE_IMAGE_CAPABILITIES;

/**
 *  @brief      Firmware image structure
 *  @details    This structure describes the firmware image that is passed to the driver in EFI_FIRMWARE_MANAGEMENT_PROTOCOL.SetImage and
 *              EFI_FIRMWARE_MANAGEMENT_PROTOCOL.CheckImage. The structure parses all fields of the firmware image and copies them into the
 *              structure, except for the parameter block and the firmware. Parameter block and the firmware are referenced by pointer and
 *              length only.
 */
typedef struct tdIfxFirmwareImage
{
    /// Unique identifier: Must be EFI_IFXTPM_FIRMWARE_IMAGE_GUID or higher.
    /// A different GUID indicates a breaking change the in firmware image file structure and a newer version of the tool is required
    /// to process the firmware image.
    GUID unique;
    /// Firmware image version (e.g. V1...V3) derived from GUID
    BYTE bImageVersion;
    /// Allowed TPM family where the firmware image can be installed on. Either DEVICE_TYPE_TPM_12 or DEVICE_TYPE_TPM_20.
    unsigned char bSourceTpmFamily;
    /// Count of the allowed source versions
    unsigned short usSourceVersionsCount;
    /// Size in bytes of rgwszSourceVersions. Uses big-endian format.
    unsigned short usSourceVersionsSize;
    /// Allowed versions where the firmware update can be applied to. Array of version names (e.g. "5.12.3456.0") with additional null termination (multi string).
    ///  Size of multi string array is usSourceVersionsSize.
    BYTE* prgwszSourceVersions;
    /// Target TPM family. Either DEVICE_TYPE_TPM_12 or DEVICE_TYPE_TPM_20.
    unsigned char bTargetTpmFamily;
    /// Size in bytes of wszTargetVersion. Uses big-endian format.
    unsigned short  usTargetVersionSize;
    /// Target version name (e.g. "5.12.3456.0") wszTargetVersion includes a terminating NULL character.
    /// Size of wszTargetVersion is usTargetVersionSize.
    wchar_t wszTargetVersion[64];
    /// Size in bytes of the policy parameter block in rgbPolicyParameterBlock. Uses big-endian format.
    unsigned int unManifestDataSize;
    /// Manifest data. Includes one or more manifest data elements.
    // Size is unManifestDataSize.
    unsigned char* rgbManifestData;
    /// Number of manifests included within rgbManifestData. Uses big-endian format.
    unsigned short usNumManifests;
    /// Key group id. Uses big-endian format.
    unsigned int unKeyGroupId;
    /// Size in bytes of the parameter block in rgbPolicyParameterBlock. Uses big-endian format.
    unsigned short  usPolicyParameterBlockSize;
    /// Parameter block. Either points to a manifest block or to a legacy policy parameter block.
    // Size is usParameterBlockSize.
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
    /// Allowed source versions (unsigned integer representation) only used for V1 and V2
    unsigned int rgunIntSourceVersions[MAX_SOURCE_VERSIONS_COUNT];
    /// Checksum over the IfxFirmwareImage structure excluding the unChecksum field. Uses big-endian format.
    unsigned int unChecksum;
    /// Key identifier for signature
    unsigned short  usSignatureKeyId;
    /// Byte array for signature
    BYTE rgbSignature[256];
} IfxFirmwareImage;

/**
 *  @brief      Function to unmarshal a IfxFirmwareImage from a byte stream
 *  @details    This function unmarshals the structures parameters. For all fields with variable
 *              size the output structure contains a pointer to the buffers address where the data can be found.
 *
 *  @param      PpTarget                    Pointer to the target structure; must be allocated by the caller.
 *  @param      PprgbBuffer                 Pointer to a byte stream containing the firmware image data; will be increased during execution by the amount of unmarshalled bytes.
 *  @param      PpnBufferSize               Size of elements readable from the byte stream; will be decreased during execution by the amount of unmarshalled bytes.
 *  @retval     RC_SUCCESS                  In case the firmware is updatable with the given firmware.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function or an error occurred at unmarshal.
 *  @retval     RC_E_BUFFER_TOO_SMALL       In case an output buffer is too small for an input byte array.
 *  @retval     RC_E_NEWER_TOOL_REQUIRED    The firmware image provided requires a newer version of this tool.
 */
_Check_return_
unsigned int
FirmwareImage_Unmarshal(
    _Out_   IfxFirmwareImage*   PpTarget,
    _Inout_ unsigned char**     PprgbBuffer,
    _Inout_ int*                PpnBufferSize);

#ifdef __cplusplus
}
#endif
