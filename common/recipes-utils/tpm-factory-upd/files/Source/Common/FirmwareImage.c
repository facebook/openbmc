/**
 *  @brief      Implements methods to access a firmware image
 *  @details    Implements helper functions to access a firmware image.
 *  @file       FirmwareImage.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FirmwareImage.h"
#include "Platform.h"
#include "TPM2_FieldUpgradeMarshal.h"
#include "TPM2_FieldUpgradeTypes.h"
#include "TPM2_Marshal.h"
#include "TPM2_Types.h"

/**
 *  @brief      Verifies that a given string consists of alphanumeric characters and [._] character only.
 *  @details    This function verifies that a given string consists of alphanumeric characters and [._] character only.
 *
 *  @param      PwszBuffer              String buffer.
 *  @param      PunBufferLength         Length of the buffer.
 *  @retval     RC_SUCCESS              In case the string consists of alphanumeric characters and [._] character only.
 *  @retval     RC_E_BAD_PARAMETER      Otherwise.
 */
_Check_return_
unsigned int
FirmwareImage_VerifyString(
    _In_z_  const wchar_t*  PwszBuffer,
    _In_    unsigned int    PunBufferLength)
{
    unsigned int unReturnValue = RC_SUCCESS;
    do
    {
        unsigned int unPos = 0;
        for (unPos = 0; unPos < PunBufferLength; unPos++)
        {
            const wchar_t wchChar = PwszBuffer[unPos];
            if (!(
                        (wchChar >= L'0' && wchChar <= L'9') || (wchChar >= L'A' && wchChar <= L'Z') ||
                        (wchChar >= L'a' && wchChar <= L'z') || (wchChar == L'.') || (wchChar == L'_')))
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                break;
            }
        }
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;
    return unReturnValue;
}

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
    _Out_   IfxFirmwareImage* PpTarget,
    _Inout_ unsigned char**   PprgbBuffer,
    _Inout_ int*              PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check out parameter
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Initialize out parameter
        Platform_MemorySet(PpTarget, 0, sizeof(IfxFirmwareImage));

        // Check remaining inout parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Unmarshal EFI_GUID
        unReturnValue = TSS_UINT32_Unmarshal((TSS_UINT32*)&PpTarget->unique.Data1, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->unique.Data2, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->unique.Data3, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        unReturnValue = TSS_UINT8_Array_Unmarshal((BYTE*)&PpTarget->unique.Data4, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->unique.Data4));
        if (RC_SUCCESS != unReturnValue)
            break;

        // Check unique value if GUID is in the supported range (by ignoring last byte of GUID)
        GUID guidTemp = PpTarget->unique;
        guidTemp.Data4[7] = EFI_IFXTPM_FIRMWARE_IMAGE_GUID.Data4[7];
        if (0 != Platform_MemoryCompare(&guidTemp, &EFI_IFXTPM_FIRMWARE_IMAGE_GUID, sizeof(EFI_IFXTPM_FIRMWARE_IMAGE_GUID)))
        {
            unReturnValue = RC_E_CORRUPT_FW_IMAGE;
            break;
        }

        // Get image version from last byte of GUID (V1 to V3 currently defined)
        PpTarget->bImageVersion = PpTarget->unique.Data4[7] - EFI_IFXTPM_FIRMWARE_IMAGE_GUID.Data4[7] + 1;
        if (PpTarget->bImageVersion < FIRMWARE_IMAGE_V1 || PpTarget->bImageVersion > FIRMWARE_IMAGE_V3)
        {
            // The tool cannot process the image. A newer version of the driver is required to process the firmware image.
            unReturnValue = RC_E_NEWER_TOOL_REQUIRED;
            ERROR_STORE_FMT(RC_E_NEWER_TOOL_REQUIRED, L"Not supported image version detected: %d", PpTarget->bImageVersion);
            break;
        }

        // Check for preliminary V2 and correct image version if found
        if ((FIRMWARE_IMAGE_V2 == PpTarget->bImageVersion) && 0 == *PprgbBuffer[0])
            PpTarget->bImageVersion = FIRMWARE_IMAGE_V3;

        // Get image struct version from V3
        if (FIRMWARE_IMAGE_V3 == PpTarget->bImageVersion)
        {
            // Unmarshal usImageStructureVersion
            unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->usImageStructureVersion, PprgbBuffer, PpnBufferSize);
            if (RC_SUCCESS != unReturnValue)
                break;
        }

        // Unmarshal bSourceTpmFamily
        unReturnValue = TSS_BYTE_Unmarshal(&PpTarget->bSourceTpmFamily, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal / set Pointers to the single source version strings
        {
            UINT16 usSourceVersionsSize = 0;
            int nBufferSize = 0;
            BYTE* rgbBuffer = NULL;

            // Unmarshal usSourceVersionsSize
            unReturnValue = TSS_UINT16_Unmarshal(&usSourceVersionsSize, PprgbBuffer, PpnBufferSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            if (usSourceVersionsSize > *PpnBufferSize)
            {
                unReturnValue = RC_E_BUFFER_TOO_SMALL;
                break;
            }

            // Get maximum allowed source versions
            UINT16 usSourceVersionsCountMax = MAX_SOURCE_VERSIONS_COUNT;
            if (PpTarget->bImageVersion > FIRMWARE_IMAGE_V2)
                usSourceVersionsCountMax = MAX_SOURCE_VERSIONS_COUNT_V3;

            // Store source version size and pointer
            PpTarget->usSourceVersionsSize = usSourceVersionsSize;
            PpTarget->prgwszSourceVersions = *PprgbBuffer;

            nBufferSize = (int)usSourceVersionsSize;
            rgbBuffer = *PprgbBuffer;
            PpTarget->usSourceVersionsCount = 0;
            do
            {
                wchar_t wszBuffer[256];
                unsigned int unLength = RG_LEN(wszBuffer);
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszBuffer, RG_LEN(wszBuffer)));

                // Unmarshal the binary source version blob to a string
                unReturnValue = Platform_UnmarshalString(rgbBuffer, (unsigned int)nBufferSize, &wszBuffer[0], &unLength);
                if (RC_SUCCESS != unReturnValue)
                    break;

                if (0 != unLength)
                {
                    // Maximum size for allowed source version is 64 characters
                    if (unLength > 64)
                    {
                        unReturnValue = RC_E_BAD_PARAMETER;
                        break;
                    }

                    // Allowed source version value must consist of alphanumeric characters and [._] character only
                    unReturnValue = FirmwareImage_VerifyString(wszBuffer, unLength);
                    if (RC_SUCCESS != unReturnValue)
                        break;

                    PpTarget->usSourceVersionsCount++;
                    // Reduce the remaining buffer size by unmarshalled string length (each character in the firmware image spans across 2 bytes).
                    nBufferSize -= (unLength + 1) * (unsigned int)sizeof(uint16_t);
                }

                // Advance the pointer to the next source version string.
                rgbBuffer = (BYTE*)(rgbBuffer + (((size_t)unLength + 1) * sizeof(uint16_t)));
            }
            while (((rgbBuffer)[0] != 0x00 || (rgbBuffer)[1] != 0x00) && // Check for terminating NULL (L"\0" as uint16_t)
                    nBufferSize > 0 &&
                    PpTarget->usSourceVersionsCount < usSourceVersionsCountMax);

            if (RC_SUCCESS != unReturnValue)
                break;

            // After unmarshalling of source version strings usSourceVersionSize shall be exhausted.
            // Only the remaining 2 bytes containing the double null-termination shall be present.
            if (nBufferSize != 2)
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                break;
            }
            if ((*rgbBuffer != 0x0) || (*(rgbBuffer + 1) != 0x0))
            {
                unReturnValue = RC_E_BAD_PARAMETER;
                break;
            }

            *PprgbBuffer = (BYTE*)(*PprgbBuffer + usSourceVersionsSize);
            *PpnBufferSize -= usSourceVersionsSize;
        }

        // Unmarshal bTargetTpmFamily
        unReturnValue = TSS_BYTE_Unmarshal(&PpTarget->bTargetTpmFamily, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal usTargetVersionSize
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->usTargetVersionSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal / set Pointer to the data for wszTargetVersion
        if (PpTarget->usTargetVersionSize > *PpnBufferSize)
        {
            unReturnValue = RC_E_BUFFER_TOO_SMALL;
            break;
        }

        // Maximum size for target version is 64 characters
        if (PpTarget->usTargetVersionSize > 64)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        // Target version value must consist of alphanumeric characters and [._] character only
        {
            wchar_t wszBuffer[256];
            unsigned int unLength = RG_LEN(wszBuffer);
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszBuffer, RG_LEN(wszBuffer)));

            // Unmarshal the target version blob to a string
            unReturnValue = Platform_UnmarshalString(*PprgbBuffer, PpTarget->usTargetVersionSize, &wszBuffer[0], &unLength);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Target version value must consist of alphanumeric characters and [._] character only
            unReturnValue = FirmwareImage_VerifyString(wszBuffer, unLength);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Copy the target version to the target structure.
            {
                unsigned int unTargetVersionLen = RG_LEN(PpTarget->wszTargetVersion);
                unReturnValue = Platform_StringCopy(PpTarget->wszTargetVersion, &unTargetVersionLen, wszBuffer);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }
        }

        *PprgbBuffer = (BYTE*)(*PprgbBuffer + PpTarget->usTargetVersionSize);
        *PpnBufferSize -= PpTarget->usTargetVersionSize;

        // Unmarshal ParameterBlockSize (using UINT32 or UINT16 field)
        UINT32 unParameterBlock = 0;
        if (PpTarget->bImageVersion >= FIRMWARE_IMAGE_V3)
        {
            unReturnValue = TSS_UINT32_Unmarshal(&unParameterBlock, PprgbBuffer, PpnBufferSize);
            if (RC_SUCCESS != unReturnValue)
                break;
        }
        else
        {
            // UINT16
            unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->usPolicyParameterBlockSize, PprgbBuffer, PpnBufferSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            unParameterBlock = PpTarget->usPolicyParameterBlockSize;
        }

        // Check length and set pointer to the data for rgbParameterBlock
        if (unParameterBlock > (unsigned int)*PpnBufferSize)
        {
            unReturnValue = RC_E_BUFFER_TOO_SMALL;
            break;
        }
        PpTarget->rgbPolicyParameterBlock = (UINT8*)*PprgbBuffer;
        *PprgbBuffer = (BYTE*)(*PprgbBuffer + unParameterBlock);
        *PpnBufferSize -= unParameterBlock;

        // Get manifest information (not V1)
        if (PpTarget->bImageVersion >= FIRMWARE_IMAGE_V2)
        {
            // Reassign pointer to manifest information
            PpTarget->unManifestDataSize = unParameterBlock;
            PpTarget->rgbManifestData = PpTarget->rgbPolicyParameterBlock;

            // Skip padding bytes handling (due to backwards compatibility)?
            if (PpTarget->bImageVersion == FIRMWARE_IMAGE_V2)
            {
                // Get number of padding bytes (added for legacy code)
                UINT16 usPaddingBytes = 0;
                unReturnValue = TSS_UINT16_Unmarshal(&usPaddingBytes, &PpTarget->rgbManifestData, (TSS_INT32*)&PpTarget->unManifestDataSize);
                if (RC_SUCCESS != unReturnValue)
                    break;
                if (usPaddingBytes >= PpTarget->unManifestDataSize)
                {
                    unReturnValue = RC_E_BAD_PARAMETER;
                    break;
                }

                // Skip padding bytes
                PpTarget->unManifestDataSize -= usPaddingBytes;
                PpTarget->rgbManifestData += usPaddingBytes;
            }

            // Reset fields
            PpTarget->usPolicyParameterBlockSize = 0;
            PpTarget->rgbPolicyParameterBlock = NULL;
        }
        else
        {
            // Get allowed versions from parameter block and store them in rgunIntSourceVersions
            // For >= VERSION_3 images the data will be overwritten with section retrieved from metadata.
            sSignedData_d sSignedData;
            BYTE* rgbPolicyParameterBlock = PpTarget->rgbPolicyParameterBlock;
            TSS_INT32 nPolicyParameterBlockSize = PpTarget->usPolicyParameterBlockSize;
            Platform_MemorySet(&sSignedData, 0, sizeof(sSignedData));
            // Unmarshal the block to sSignedData structure
            unReturnValue = TSS_sSignedData_d_Unmarshal(&sSignedData, &rgbPolicyParameterBlock, &nPolicyParameterBlockSize);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE_FMT(RC_E_CORRUPT_FW_IMAGE, L"TSS_sSignedData_d_Unmarshal returned an unexpected value. (0x%.8x)", unReturnValue);
                unReturnValue = RC_E_CORRUPT_FW_IMAGE;
                break;
            }

            // Get allowed versions from parameter block
            {
                unsigned int unIndex = 0;
                PpTarget->usIntSourceVersionCount = sSignedData.sSignerInfo.sSignedAttributes.sVersions.wEntries;
                for (unIndex = 0; unIndex < MAX_SOURCE_VERSIONS_COUNT; unIndex++)
                {
                    if (unIndex < PpTarget->usIntSourceVersionCount)
                        PpTarget->rgunIntSourceVersions[unIndex] = sSignedData.sSignerInfo.sSignedAttributes.sVersions.Version[unIndex];
                    else
                        PpTarget->rgunIntSourceVersions[unIndex] = 0;
                }
            }
        }

        // Unmarshal unFirmwareSize
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->unFirmwareSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal / set pointer to the data for rgbFirmware
        if (PpTarget->unFirmwareSize > (unsigned int)*PpnBufferSize)
        {
            unReturnValue = RC_E_BUFFER_TOO_SMALL;
            break;
        }
        PpTarget->rgbFirmware = (UINT8*)*PprgbBuffer;
        *PprgbBuffer = (BYTE*)(*PprgbBuffer + PpTarget->unFirmwareSize);
        *PpnBufferSize -= PpTarget->unFirmwareSize;

        // Check for Additional Data section
        // V1 and V2 size 1024 bytes, of which 266 bytes are mandatory for size, version, target state, signature key ID and signature
        // V3: no limit
        if ((unsigned int)*PpnBufferSize > sizeof(PpTarget->unChecksum))
        {
            // Unmarshal additional data size
            UINT16 usAdditionalDataSize = 0;
            unReturnValue = TSS_UINT16_Unmarshal(&usAdditionalDataSize, PprgbBuffer, PpnBufferSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Check max size
            if (usAdditionalDataSize > 1024 && PpTarget->bImageVersion <= FIRMWARE_IMAGE_V2)
            {
                unReturnValue = RC_E_CORRUPT_FW_IMAGE;
                break;
            }

            // Unmarshal firmware image file structure version (not V3)
            if (PpTarget->bImageVersion <= FIRMWARE_IMAGE_V2)
            {
                unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->usImageStructureVersion, PprgbBuffer, PpnBufferSize);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }

            // Unmarshal TPM target state
            if (PpTarget->usImageStructureVersion >= 1)
            {
                unReturnValue = TSS_UINT32_Unmarshal((TSS_UINT32*)&PpTarget->bfTargetState, PprgbBuffer, PpnBufferSize);
                if (RC_SUCCESS != unReturnValue)
                    break;

                if (PpTarget->usImageStructureVersion >= 3)
                {
                    // Unmarshal firmware image capabilities
                    unReturnValue = TSS_UINT32_Unmarshal((TSS_UINT32*)&PpTarget->bfCapabilities, PprgbBuffer, PpnBufferSize);
                    if (RC_SUCCESS != unReturnValue)
                        break;

                    // Handle firmware image allowed versions (not V3)?
                    if (PpTarget->bImageVersion <= FIRMWARE_IMAGE_V2)
                    {
                        // Unmarshal firmware image allowed versions count
                        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->usIntSourceVersionCount, PprgbBuffer, PpnBufferSize);
                        if (RC_SUCCESS != unReturnValue)
                            break;

                        // Check maximum allowed value
                        if (PpTarget->usIntSourceVersionCount > MAX_SOURCE_VERSIONS_COUNT)
                        {
                            unReturnValue = RC_E_CORRUPT_FW_IMAGE;
                            break;
                        }

                        // Unmarshal firmware image allowed versions list
                        unReturnValue = TSS_UINT32_Array_Unmarshal((TSS_UINT32*)&PpTarget->rgunIntSourceVersions, PprgbBuffer, PpnBufferSize, MAX_SOURCE_VERSIONS_COUNT);
                        if (RC_SUCCESS != unReturnValue)
                            break;
                    }
                }

                // Get remaining size
                int nRemaining = *PpnBufferSize - sizeof(PpTarget->usSignatureKeyId) - sizeof(PpTarget->rgbSignature) - sizeof(PpTarget->unChecksum);
                if (nRemaining < 0)
                {
                    unReturnValue = RC_E_BUFFER_TOO_SMALL;
                    break;
                }

                // Skip possible remaining unknown parts of the Additional Data section
                if (nRemaining > 0)
                {
                    // Forward pointer and adjust remaining size
                    *PprgbBuffer += nRemaining;
                    *PpnBufferSize -= nRemaining;
                }

                // Unmarshal image signature
                if (PpTarget->usImageStructureVersion >= 2)
                {
                    // Unmarshal signature key identifier
                    unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->usSignatureKeyId, PprgbBuffer, PpnBufferSize);
                    if (RC_SUCCESS != unReturnValue)
                        break;

                    // Unmarshal signature
                    unReturnValue = TSS_BYTE_Array_Unmarshal((BYTE*)&PpTarget->rgbSignature, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->rgbSignature));
                    if (RC_SUCCESS != unReturnValue)
                        break;
                }
            }
        }

        // Unmarshal unChecksum
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->unChecksum, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Check that no more data is available
        if (*PpnBufferSize != 0)
        {
            unReturnValue = RC_E_CORRUPT_FW_IMAGE;
            break;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}
