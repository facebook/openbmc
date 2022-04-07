/**
 *	@brief		Implements common firmware update functionality used by different tools
 *	@details	Implements an internal firmware update interface used by IFXTPMUpdate and TPMFactoryUpd.
 *	@file		FirmwareUpdate.c
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

#include "FirmwareUpdate.h"
#include "FirmwareImage.h"
#include "Crypt.h"

#include "TPM2_Marshal.h"
#include "TPM2_FlushContext.h"
#include "TPM2_GetCapability.h"
#include "TPM2_GetTestResult.h"
#include "TPM2_HierarchyChangeAuth.h"
#include "TPM2_PolicyCommandCode.h"
#include "TPM2_PolicySecret.h"
#include "TPM2_SetPrimaryPolicy.h"
#include "TPM2_StartAuthSession.h"
#include "TPM2_Startup.h"

#include "TPM2_FieldUpgradeTypes.h"
#include "TPM2_FieldUpgradeMarshal.h"
#include "TPM2_FieldUpgradeStartVendor.h"

#include "TPM_Types.h"
#include "TPM_Startup.h"
#include "TPM_GetCapability.h"
#include "TPM_GetTestResult.h"
#include "TPM_OwnerReadInternalPub.h"
#include "TPM_FlushSpecific.h"
#include "TPM_OIAP.h"
#include "TPM_OSAP.h"

#include "TPM_FieldUpgradeInfoRequest.h"
#include "TPM_FieldUpgradeInfoRequest2.h"
#include "TPM_FieldUpgradeStart.h"
#include "TPM_FieldUpgradeUpdate.h"
#include "TPM_FieldUpgradeComplete.h"

/// Maximum amount of times to check if TPM switched to boot loader mode after TPM_FieldUpgrade_Start command.
#define TPM_FU_START_MAX_RETRIES 16U
/// Default wait time in milliseconds in between two of the above checks.
#define TPM_FU_START_RETRY_WAIT_TIME 1000
/// Default wait time in milliseconds after sending TPM_FieldUpgrade_Complete before continuing to give TPM time to finish.
#define TPM_FU_COMPLETE_WAIT_TIME 2000

/**
 *	@brief		Function to read Security Module Logic Info from TPM2.0.
 *	@details	This function obtains the Security Module Logic Info from TPM2.0.
 *
 *	@param		PpSecurityModuleLogicInfo2	Pointer to the Security Module Logic Info
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function. The parameter is NULL
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_Tpm20_GetSecurityModuleLogicInfo2(
	_Out_ sSecurityModuleLogicInfo2_d* PpSecurityModuleLogicInfo2)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// Check parameter
		if (NULL == PpSecurityModuleLogicInfo2)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpSecurityModuleLogicInfo2 is NULL)");
			break;
		}

		{
			// Read out the vendor specific TPM_PT_VENDOR_FIX_SMLI2 property from the TPM.
			TSS_TPMS_VENDOR_CAPABILITY_DATA vendorCapabilityData = {0};
			int nBufferSize = 0;
			BYTE* rgbBuffer = NULL;
			BYTE bMoreData = 0;

			unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_SMLI2, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned an unexpected value. (TPM_CAP_VENDOR_PROPERTY,TPM_PT_VENDOR_FIX_SMLI2)");
				break;
			}

			if (1 != vendorCapabilityData.data.vendorData.count)
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned more than one capability");
				break;
			}

			// Unmarshal the property to a sSecurityModuleLogicInfo2_d structure
			nBufferSize = vendorCapabilityData.data.vendorData.buffer[0].size;
			rgbBuffer = (BYTE*)&vendorCapabilityData.data.vendorData.buffer[0].buffer;

			unReturnValue = TSS_sSecurityModuleLogicInfo2_d_Unmarshal(PpSecurityModuleLogicInfo2, &rgbBuffer, &nBufferSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TSS_sSecurityModuleLogicInfo2_d_Unmarshal returned an unexpected value.");
				break;
			}
		}
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Function to read the field upgrade counter from the TPM
 *	@details	This function obtains the field upgrade counter value from the TPM.
 *
 *	@param		PbfTpmAttributes			TPM state attributes
 *	@param		PpunUpgradeCounter			Pointer to the field upgrade counter parameter
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function. The parameter is NULL
 *	@retval		RC_E_UNSUPPORTED_CHIP		In case of the underlying TPM does not support FieldUpdateCounter acquiring commands
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetTpmFieldUpgradeCounter(
	_In_	BITFIELD_TPM_ATTRIBUTES	PbfTpmAttributes,
	_Out_	unsigned int*			PpunUpgradeCounter)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// Check parameter
		if (NULL == PpunUpgradeCounter)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpunUpgradeCounter is NULL)");
			break;
		}

		// Initialize output parameter
		*PpunUpgradeCounter = 0;

		// Read update counter from TPM
		if (PbfTpmAttributes.tpm20)
		{
			// TPM2.0
			sSecurityModuleLogicInfo2_d securityModuleLogicInfo2 = {0};
			unReturnValue = FirmwareUpdate_Tpm20_GetSecurityModuleLogicInfo2(&securityModuleLogicInfo2);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"FirmwareUpdate_Tpm20_GetSecurityModuleLogicInfo2 returned an unexpected value.");
				// Check if the TPM2_GetCapability call returns an unknown CAP area code (0xE02802c4)
				if ((TSS_TPM_RC_VALUE | TSS_TPM_RC_P | TSS_TPM_RC_2 | RC_TPM_MASK) == unReturnValue)
					unReturnValue = RC_E_UNSUPPORTED_CHIP;
				break;
			}

			*PpunUpgradeCounter = securityModuleLogicInfo2.wFieldUpgradeCounter;
		}
		else if ((PbfTpmAttributes.tpm12 && PbfTpmAttributes.infineon) || PbfTpmAttributes.bootLoader)
		{
			// TPM1.2
			sSecurityModuleLogicInfo_d securityModuleLogicInfo = {0};
			unReturnValue = TSS_TPM_FieldUpgradeInfoRequest2(&securityModuleLogicInfo);
			if (TSS_TPM_RESOURCES == (unReturnValue ^ RC_TPM_MASK))
			{
				// Retry once on TPM_RESOURCES
				unReturnValue = TSS_TPM_FieldUpgradeInfoRequest2(&securityModuleLogicInfo);
			}
			if (RC_SUCCESS != unReturnValue)
			{
				if (TSS_TPM_BAD_PARAM_SIZE == (unReturnValue ^ RC_TPM_MASK) ||
						TSS_TPM_BAD_PARAMETER == (unReturnValue ^ RC_TPM_MASK))
				{
					ERROR_STORE(unReturnValue, L"TSS_TPM_FieldUpgradeInfoRequest2 returned an unexpected value.");
					unReturnValue = RC_E_UNSUPPORTED_CHIP;
				}
				ERROR_STORE(unReturnValue, L"TSS_TPM_FieldUpgradeInfoRequest2 returned an unexpected value.");
				break;
			}

			*PpunUpgradeCounter = securityModuleLogicInfo.wFieldUpgradeCounter;
		}
		else
		{
			unReturnValue = RC_E_FAIL;
			break;
		}
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Function to read the firmware version string from the TPM
 *	@details	This function obtains the firmware version string from the TPM.
 *
 *	@param		PbfTpmAttributes				TPM state attributes
 *	@param		PwszFirmwareVersion				Wide character string output buffer for the firmware version (must be allocated by the caller)
 *	@param		PpunFirmwareVersionSize			In: Capacity of the wide character string including the zero termination
 *												Out: String length without the zero termination
 *	@param		PwszFirmwareVersionShort		Wide character string output buffer for the firmware version without subversion.minor (must be allocated by the caller)
 *	@param		PpunFirmwareVersionShortSize	In: Capacity of the wide character string including the zero termination
 *												Out: String length without the zero termination
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function. The parameter is NULL or the output buffer is too small
 *	@retval		RC_E_FAIL					An unexpected error occurred. E.g. more than one property returned from TPM2_GetCapability call
 *											or unknown BITFIELD_TPM_ATTRIBUTES
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetTpmFirmwareVersionString(
	_In_											BITFIELD_TPM_ATTRIBUTES	PbfTpmAttributes,
	_Out_bytecap_(*PpunFirmwareVersionSize)			wchar_t*				PwszFirmwareVersion,
	_Inout_											unsigned int*			PpunFirmwareVersionSize,
	_Out_bytecap_(*PpunFirmwareVersionShortSize)	wchar_t*				PwszFirmwareVersionShort,
	_Inout_											unsigned int*			PpunFirmwareVersionShortSize)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// Check parameters
		if (NULL == PpunFirmwareVersionSize ||
				NULL == PwszFirmwareVersion ||
				NULL == PpunFirmwareVersionShortSize ||
				NULL == PwszFirmwareVersionShort)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpunFirmwareVersionSize or PwszFirmwareVersion or PpunFirmwareVersionShortSize or PwszFirmwareVersionShort is NULL)");
			break;
		}

		if (PbfTpmAttributes.tpm20)
		{
			// Read version from TPM2.0
			BYTE bMoreData = 0;
			TSS_TPMS_CAPABILITY_DATA capabilityData = {0};
			unsigned int unFirmwareVersion1 = 0;
			unsigned int unFirmwareVersion2 = 0;

			// Get actual firmware version
			unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_TPM_PROPERTIES, TSS_TPM_PT_FIRMWARE_VERSION_1, 1, &bMoreData, &capabilityData);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned an unexpected value.(TPM_CAP_TPM_PROPERTIES,TPM_PT_FIRMWARE_VERSION_1)");
				break;
			}

			// Check count of capabilities returned
			if (1 != capabilityData.data.tpmProperties.count)
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned more than one capability");
				break;
			}
			unFirmwareVersion1 = capabilityData.data.tpmProperties.tpmProperty[0].value;

			unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_TPM_PROPERTIES, TSS_TPM_PT_FIRMWARE_VERSION_2, 1, &bMoreData, &capabilityData);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned an unexpected value.(TPM_CAP_TPM_PROPERTIES,TPM_PT_FIRMWARE_VERSION_2)");
				break;
			}

			// Check count of capabilities returned
			if (1 != capabilityData.data.tpmProperties.count)
			{
				unReturnValue = RC_E_FAIL;
				ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned more than one capability");
				break;
			}
			unFirmwareVersion2 = capabilityData.data.tpmProperties.tpmProperty[0].value;

			// Firmware version without subversion.minor for TPM2.0
			unReturnValue = Platform_StringFormat(PwszFirmwareVersionShort, PpunFirmwareVersionShortSize, L"%d.%d.%d", unFirmwareVersion1 >> 16, unFirmwareVersion1 & 0xFFFF, unFirmwareVersion2 >> 8);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
				break;
			}

			// Firmware version with subversion.minor for TPM2.0
			unReturnValue = Platform_StringFormat(PwszFirmwareVersion, PpunFirmwareVersionSize, L"%d.%d.%d.%d", unFirmwareVersion1 >> 16, unFirmwareVersion1 & 0xFFFF, unFirmwareVersion2 >> 8, unFirmwareVersion2 & 0xFF);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
				break;
			}
		}
		else if (PbfTpmAttributes.tpm12 || PbfTpmAttributes.bootLoader)
		{
			// TPM1.2
			UINT16 usBuildNumber = 0;
			TSS_TPM_CAP_VERSION_INFO sTpmVersionInfo = {0};
			unsigned int unTpmVersionInfoSize = sizeof(sTpmVersionInfo);
			UINT8 bCertState = 0;

			unReturnValue = TSS_TPM_GetCapability(TSS_TPM_CAP_VERSION_VAL, 0, NULL, &unTpmVersionInfoSize, (BYTE*)&sTpmVersionInfo);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned an unexpected value.(TSS_TPM_CAP_VERSION_VAL,0)");
				break;
			}

			if (!PbfTpmAttributes.bootLoader)
			{
				usBuildNumber = (sTpmVersionInfo.vendorSpecific[2] << 8) + sTpmVersionInfo.vendorSpecific[3];
				bCertState = sTpmVersionInfo.vendorSpecific[4];
			}

			// Firmware version without subversion.minor for TPM1.2
			unReturnValue = Platform_StringFormat(PwszFirmwareVersionShort, PpunFirmwareVersionShortSize, L"%d.%d.%d", sTpmVersionInfo.version.revMajor, sTpmVersionInfo.version.revMinor, usBuildNumber);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
				break;
			}
			// Firmware version with subversion.minor for TPM1.2
			unReturnValue = Platform_StringFormat(PwszFirmwareVersion, PpunFirmwareVersionSize, L"%d.%d.%d.%d", sTpmVersionInfo.version.revMajor, sTpmVersionInfo.version.revMinor, usBuildNumber, bCertState);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
				break;
			}
		}
		else
		{
			// Unknown TPM mode
			unReturnValue = RC_E_FAIL;
			ERROR_STORE_FMT(unReturnValue, L"Unknown TPM state attributes detected. 0x%.8x", PbfTpmAttributes);
			break;
		}
		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Function to check if the TPM is updatable with the given firmware image
 *	@details	Some parameters like GUID, file content signature, TPM firmware major minor version or file content CRC
 *				are checked to get a decision if the firmware is updatable with the current image.
 *
 *	@param		PbfTpmAttributes			TPM state attributes
 *	@param		PrgbFirmwareImage			Pointer to the firmware image byte stream
 *	@param		PnFirmwareImageSize			Size of the firmware image byte stream
 *	@param		PpsFirmwareImage			Pointer to the unmarshalled firmware image structure (Unmarshalled PrgbFirmwareImage)
 *	@param		PpfValid					TRUE in case the image is valid, FALSE otherwise.
 *	@param		PpbfNewTpmFirmwareInfo		Pointer to a bit field to return info data for the new firmware image.
 *	@param		PpunErrorDetails			Pointer to an unsigned int to return error details. Possible values are:\n
 *												RC_E_FW_UPDATE_BLOCKED in case the field upgrade counter value has been exceeded.\n
 *												RC_E_WRONG_FW_IMAGE in case the TPM is not updatable with the given image.\n
 *												RC_E_CORRUPT_FW_IMAGE in case the firmware image is corrupt.\n
 *												RC_E_NEWER_TOOL_REQUIRED in case a newer version of the tool is required to parse the firmware image.\n
 *												RC_E_WRONG_DECRYPT_KEYS in case the TPM2.0 does not have decrypt keys matching to the firmware image.
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER			In case of a NULL input parameter
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_IsFirmwareUpdatable(
	_In_								BITFIELD_TPM_ATTRIBUTES			PbfTpmAttributes,
	_In_bytecount_(PnFirmwareImageSize)	BYTE*							PrgbFirmwareImage,
	_In_								int								PnFirmwareImageSize,
	_In_								IfxFirmwareImage*				PpsFirmwareImage,
	_Out_								BOOL*							PpfValid,
	_Out_								BITFIELD_NEW_TPM_FIRMWARE_INFO*	PpbfNewTpmFirmwareInfo,
	_Out_								unsigned int*					PpunErrorDetails)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// Check _Out_ parameters.
		if (NULL == PpfValid || NULL == PpbfNewTpmFirmwareInfo || NULL == PpunErrorDetails)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpfValid or PpbfNewTpmFirmwareInfo or PpunErrorDetails is NULL)");
			break;
		}

		// Set default value for _Out_ parameters.
		*PpfValid = FALSE;
		*PpunErrorDetails = RC_E_WRONG_FW_IMAGE;
		unReturnValue = Platform_MemorySet(PpbfNewTpmFirmwareInfo, 0, sizeof(BITFIELD_NEW_TPM_FIRMWARE_INFO));
		if (RC_SUCCESS != unReturnValue)
			break;

		// Check _In_ parameters.
		if (NULL == PrgbFirmwareImage ||
				0 >= PnFirmwareImageSize ||
				NULL == PpsFirmwareImage)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PrgbFirmwareImage or PpsFirmwareImage is NULL or PnFirmwareImageSize <= 0)");
			break;
		}

		// Compare GUID to the expected one.
		if (0 != Platform_MemoryCompare(&PpsFirmwareImage->unique, &EFI_IFXTPM_FIRMWARE_IMAGE_GUID, sizeof(EFI_IFXTPM_FIRMWARE_IMAGE_GUID)))
		{
			// The tool cannot process the image. Detect whether a newer version of the tool could parse the image.
			// IMPORTANT: As soon as EFI_IFXTPM_FIRMWARE_IMAGE_2_GUID is supported, update firmware image file format by introducing
			// a schema version for all future breaking changes. Furthermore, leave GUID the same as long as possible! This enables
			// all (i.e. older) application versions to return the correct return code RC_E_NEWER_TOOL_REQUIRED in case of an
			// unsupported firmware image file format even for schema versions introduced after EFI_IFXTPM_FIRMWARE_IMAGE_2_GUID.
			if (0 == Platform_MemoryCompare(&PpsFirmwareImage->unique, &EFI_IFXTPM_FIRMWARE_IMAGE_2_GUID, sizeof(EFI_IFXTPM_FIRMWARE_IMAGE_2_GUID)))
				// A newer version of the driver is required to process the firmware image
				*PpunErrorDetails = RC_E_NEWER_TOOL_REQUIRED;
			else
			{
				ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The firmware image file is corrupt or not a firmware image file at all");
				*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
			}

			unReturnValue = RC_SUCCESS;
			break;
		}

		// Check the CRC at the end of the firmware image
		{
			unsigned int unCRC = 0;
			unReturnValue = Crypt_CRC((void*)PrgbFirmwareImage, PnFirmwareImageSize - sizeof(unCRC), &unCRC);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Crypt_CRC returned an unexpected value");
				break;
			}
			if (PpsFirmwareImage->unChecksum != unCRC)
			{
				ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The CRC value in the firmware image file is incorrect");
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
				break;
			}
		}

		// Check signature on the firmware image file with Infineon code signing public key
		{
			BYTE rgbHash[TSS_SHA256_DIGEST_SIZE] = {0};
			// The signature is 256 bytes long and is located before the CRC
			int nSizeOfDataForHash = PnFirmwareImageSize - sizeof(PpsFirmwareImage->unChecksum) - sizeof(RSA_PUB_MODULUS_KEY_ID_0);

			// Check structure version of the firmware image file
			if (PpsFirmwareImage->usImageStructureVersion < 2)
			{
				ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The structure of the firmware image file is too old and therefore does not meet the minimum requirements");
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
				break;
			}

			// Check if signature key ID is known
			if (SIG_KEY_ID_1 != PpsFirmwareImage->usSignatureKeyId)
			{
				ERROR_STORE(RC_E_NEWER_TOOL_REQUIRED, L"The signature key ID of the firmware image file is not supported");
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_NEWER_TOOL_REQUIRED;
				break;
			}

			// Check that the firmware image file is large enough to contain a signature
			if (nSizeOfDataForHash <= 0)
			{
				ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The size of the firmware image file signature is too small");
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
				break;
			}

			// Recalculate the SHA-256 digest on which the signature is based
			unReturnValue = Crypt_SHA256(PrgbFirmwareImage, nSizeOfDataForHash, rgbHash);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Crypt_SHA256 returned an unexpected value");
				break;
			}

			// Verify the signature of the firmware image file
			unReturnValue = Crypt_VerifySignature(rgbHash, sizeof(rgbHash), PpsFirmwareImage->rgbSignature, sizeof(PpsFirmwareImage->rgbSignature), RSA_PUB_MODULUS_KEY_ID_0, sizeof(RSA_PUB_MODULUS_KEY_ID_0));
			if (RC_SUCCESS != unReturnValue && RC_E_VERIFY_SIGNATURE != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Crypt_VerifySignature returned an unexpected value");
				break;
			}
			else if (RC_E_VERIFY_SIGNATURE == unReturnValue)
			{
				ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The signature in the firmware image file is invalid");
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
				break;
			}
		}

		// Check consistency of firmware
		{
			// Source and target TPM family flags in the firmware image must indicate either TPM1.2 or TPM2.0
			if (PpsFirmwareImage->bSourceTpmFamily != DEVICE_TYPE_TPM_12 && PpsFirmwareImage->bSourceTpmFamily != DEVICE_TYPE_TPM_20)
			{
				ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The content of the firmware image file is inconsistent");
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
				break;
			}
			if (PpsFirmwareImage->bTargetTpmFamily != DEVICE_TYPE_TPM_12 && PpsFirmwareImage->bTargetTpmFamily != DEVICE_TYPE_TPM_20)
			{
				ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The content of the firmware image file is inconsistent");
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
				break;
			}
		}

		// Run checks on policy parameter block
		{
			BYTE* rgbPolicyParameterBlock = NULL;
			int nPolicyParameterBlockSize = 0;
			sSignedData_d sSignedData = {0};

			// Unmarshal the policy parameter block
			rgbPolicyParameterBlock = PpsFirmwareImage->rgbPolicyParameterBlock;
			nPolicyParameterBlockSize = PpsFirmwareImage->usPolicyParameterBlockSize;
			unReturnValue = TSS_sSignedData_d_Unmarshal(&sSignedData, &rgbPolicyParameterBlock, &nPolicyParameterBlockSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The content of the firmware image file is not parsable");
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
				break;
			}

			// Calculate the SHA256 digest of the firmware image and verify if it matches the digest given in the policy parameter block
			{
				// Recalculate the messageDigest of the firmware block
				BYTE rgbMessageDigest[TSS_SHA256_DIGEST_SIZE] = {0};
				unReturnValue = Crypt_SHA256(PpsFirmwareImage->rgbFirmware, PpsFirmwareImage->unFirmwareSize, rgbMessageDigest);
				if (RC_SUCCESS != unReturnValue)
				{
					ERROR_STORE(unReturnValue, L"Crypt_SHA256 returned an unexpected value");
					break;
				}

				// Compare the messageDigest to the value stored in the policy parameter block
				if (0 != Platform_MemoryCompare(sSignedData.sSignerInfo.sSignedAttributes.sMessageDigest.rgbMessageDigest, rgbMessageDigest, TSS_SHA256_DIGEST_SIZE))
				{
					ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The firmware digest in the firmware image file is incorrect");
					unReturnValue = RC_SUCCESS;
					*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
					break;
				}
			}

			// On TPM2.0 check that the TPM and the firmware image use the same key material
			if (PbfTpmAttributes.tpm20)
			{
				sSecurityModuleLogicInfo2_d sSecurityModuleLogicInfo2 = {0};
				unsigned int unIndex = 0;
				BOOL fDecryptKeyValid = FALSE;

				// Read the decrypt key ID from the TPM
				unReturnValue = FirmwareUpdate_Tpm20_GetSecurityModuleLogicInfo2(&sSecurityModuleLogicInfo2);
				if (RC_SUCCESS != unReturnValue)
				{
					ERROR_STORE(unReturnValue, L"FirmwareUpdate_Tpm20_GetInfo returned an unexpected value");
					break;
				}

				// Consistency check: the TPM may report at maximum 4 decrypt key ID entries
				if (sSecurityModuleLogicInfo2.sKeyList.wEntries > RG_LEN(sSecurityModuleLogicInfo2.sKeyList.DecryptKeyId))
				{
					ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The content of the firmware image file is inconsistent");
					unReturnValue = RC_SUCCESS;
					*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
					break;
				}

				// Compare the decrypt key IDs reported by the TPM to the key ID used to encrypt the firmware image.
				// There should be at least one match
				for (unIndex = 0; unIndex < sSecurityModuleLogicInfo2.sKeyList.wEntries; unIndex++)
				{
					if (sSecurityModuleLogicInfo2.sKeyList.DecryptKeyId[unIndex] == sSignedData.sSignerInfo.sSignedAttributes.DecryptKeyId)
					{
						fDecryptKeyValid = TRUE;
						break;
					}
				}

				if (!fDecryptKeyValid)
				{
					// TPM and firmware image do not use the same key material. Hence, the firmware image is not applicable to this TPM.
					unReturnValue = RC_SUCCESS;
					*PpunErrorDetails = RC_E_WRONG_DECRYPT_KEYS;
					break;
				}
			}
		}

		// Check if the field upgrade counter allows an upgrade
		{
			unsigned int unFieldUpgradeCounter = 0;
			unReturnValue = FirmwareUpdate_GetTpmFieldUpgradeCounter(PbfTpmAttributes, &unFieldUpgradeCounter);
			if (RC_SUCCESS != unReturnValue)
				break;

			if (0 == unFieldUpgradeCounter)
			{
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_FW_UPDATE_BLOCKED;
				break;
			}
		}

		// If the TPM is not in boot loader mode, check the allowed source versions
		if (!PbfTpmAttributes.bootLoader)
		{
			wchar_t wszFirmwareVersion[MAX_NAME] = {0};
			wchar_t wszFirmwareVersionShort[MAX_NAME] = {0};
			unsigned int unFirmwareVersionSize = RG_LEN(wszFirmwareVersion);
			unsigned int unFirmwareVersionShortSize = RG_LEN(wszFirmwareVersionShort);
			unsigned int unIndex = 0;
			BOOL fImageAllowed = FALSE;

			// Get the firmware version from the TPM
			unReturnValue = FirmwareUpdate_GetTpmFirmwareVersionString(PbfTpmAttributes, wszFirmwareVersion, &unFirmwareVersionSize, wszFirmwareVersionShort, &unFirmwareVersionShortSize);
			if (RC_SUCCESS != unReturnValue)
				break;

			// Check if the firmware version is listed in the allowed source versions
			for (; unIndex < PpsFirmwareImage->usSourceVersionsCount; unIndex++)
			{
				// Try the firmware version with subversion.minor first (e.g. 4.40.119.0)
				if (0 == Platform_StringCompare(wszFirmwareVersion, PpsFirmwareImage->rgwszSourceVersions[unIndex], unFirmwareVersionSize + 1, FALSE))
				{
					fImageAllowed = TRUE;
					break;
				}

				// If this does not match, try the firmware version without subversion.minor (e.g. 4.40.119)
				if (0 == Platform_StringCompare(wszFirmwareVersionShort, PpsFirmwareImage->rgwszSourceVersions[unIndex], unFirmwareVersionShortSize + 1, FALSE))
				{
					fImageAllowed = TRUE;
					break;
				}
			}

			if (FALSE == fImageAllowed)
			{
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_WRONG_FW_IMAGE;
				break;
			}
		}
		// If the TPM is in boot loader mode, a previous firmware update was interrupted.
		// Check if the currently running build number matches the allowed build number in the policy parameter block.
		// If this check fails verify if the build number matches the number in the IfxFirmwareImage structure.
		else
		{
			sSecurityModuleLogicInfo_d securityModuleLogicInfo = {0};

			// Policy parameter block data
			unsigned int unActiveVersion = 0;
			unsigned int unIndex = 0;
			BOOL fActiveVersionVerified = FALSE;

			// First get the current running build number from FieldUpgradeInfoRequest2
			unReturnValue = TSS_TPM_FieldUpgradeInfoRequest2(&securityModuleLogicInfo);
			if (TSS_TPM_RESOURCES == (unReturnValue ^ RC_TPM_MASK))
			{
				// Retry once on TPM_RESOURCES
				unReturnValue = TSS_TPM_FieldUpgradeInfoRequest2(&securityModuleLogicInfo);
			}
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TSS_TPM_FieldUpgradeInfoRequest2 returned an unexpected value.");
				if (TSS_TPM_BAD_PARAM_SIZE == (unReturnValue ^ RC_TPM_MASK) ||
						TSS_TPM_BAD_PARAMETER == (unReturnValue ^ RC_TPM_MASK))
				{
					unReturnValue = RC_E_UNSUPPORTED_CHIP;
				}
				break;
			}

			// Get the active firmware package.
			// Models SLB966x and SLB9670 have INACTIVE_STALE_VERSION tag. The firmware package without INACTIVE_STALE_VERSION tag is active.
			// Model SLB9655 does not have INACTIVE_STALE_VERSION tag. The first firmware package is active.
			{
				unsigned int unField = securityModuleLogicInfo.sSecurityModuleLogic.sFirmwareConfiguration.FirmwarePackage[0].StaleVersion != INACTIVE_STALE_VERSION ? 0 : 1;
				unActiveVersion = securityModuleLogicInfo.sSecurityModuleLogic.sFirmwareConfiguration.FirmwarePackage[unField].Version;

				// Verify against allowed source versions
				// VERSION_3 or greater: uses data from meta data section
				// Others: uses data from parameter block
				for (unIndex = 0; unIndex < PpsFirmwareImage->usIntSourceVersionCount; unIndex++)
				{
					if (PpsFirmwareImage->rgunIntSourceVersions[unIndex] == unActiveVersion)
					{
						fActiveVersionVerified = TRUE;
						break;
					}
				}
			}

			// If the comparison did not match the actual version, the firmware image is invalid for this TPM.
			if (FALSE == fActiveVersionVerified)
			{
				unReturnValue = RC_SUCCESS;
				*PpunErrorDetails = RC_E_WRONG_FW_IMAGE;
				break;
			}

			// For VERSION_3 firmware images match unique ID between policy parameter block and TPM.
			if (PpsFirmwareImage->bfCapabilities.invalidFirmwareMode_matchUniqueID)
			{
				BYTE* rgbPolicyParameterBlock = NULL;
				int nPolicyParameterBlockSize = 0;
				sSignedData_d sSignedData = {0};
				// Copy pointer and size for unmarshalling
				rgbPolicyParameterBlock = PpsFirmwareImage->rgbPolicyParameterBlock;
				nPolicyParameterBlockSize = PpsFirmwareImage->usPolicyParameterBlockSize;
				// Unmarshal the block to sSignedData structure
				unReturnValue = TSS_sSignedData_d_Unmarshal(&sSignedData, &rgbPolicyParameterBlock, &nPolicyParameterBlockSize);
				if (RC_SUCCESS != unReturnValue)
				{
					ERROR_STORE_FMT(RC_E_CORRUPT_FW_IMAGE, L"TSS_sSignedData_d_Unmarshal returned an unexpected value. (0x%.8x)", unReturnValue);
					unReturnValue = RC_E_CORRUPT_FW_IMAGE;
					break;
				}

				{
					// Get the unique ID from policy parameter block.
					unsigned int unUniqueTPM = 0;
					unsigned int unUniqueFirmwareImage = sSignedData.sSignerInfo.sSignedAttributes.sFirmwarePackage.StaleVersion & 0x0000000f;

					// Get the unique ID from the TPM
					unUniqueTPM = securityModuleLogicInfo.sProcessFirmwarePackage.StaleVersion & 0x0000000f;

					// Match unique ID of policy parameter block and TPM.
					if (unUniqueFirmwareImage != unUniqueTPM)
					{
						unReturnValue = RC_SUCCESS;
						*PpunErrorDetails = RC_E_WRONG_FW_IMAGE;
						break;
					}
				}
			}
		}

		*PpunErrorDetails = RC_SUCCESS;
		*PpfValid = TRUE;

		// Check if device type changes with an update
		if (PpsFirmwareImage->bSourceTpmFamily != PpsFirmwareImage->bTargetTpmFamily)
		{
			PpbfNewTpmFirmwareInfo->deviceTypeChange = 1;
			PpbfNewTpmFirmwareInfo->factoryDefaults = 1;
		}
		else if (PpsFirmwareImage->bfTargetState.factoryDefaults)
		{
			PpbfNewTpmFirmwareInfo->factoryDefaults = 1;
		}

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Returns the TPM state attributes
 *	@details
 *
 *	@param		PpsTpmState					Pointer to a variable representing the TPM state
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function. PpsTpmState is NULL.
 *	@retval		RC_E_FAIL					An unexpected error occurred. E.g. more than one property returned from TPM2_GetCapability call
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_CalculateState(
	_Out_ TPM_STATE* PpsTpmState)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// Check parameters
		if (NULL == PpsTpmState)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpsTpmState is NULL)");
			break;
		}

		unReturnValue = Platform_MemorySet(PpsTpmState, 0, sizeof(*PpsTpmState));
		if (RC_SUCCESS != unReturnValue)
			break;

		// Try to call a TPM2_Startup command
		unReturnValue = TSS_TPM2_Startup(TSS_TPM_SU_CLEAR);
		// Remember to orderly shutdown the TPM2.0 if TPM2_Startup completed successfully.
		if (TSS_TPM_RC_SUCCESS == unReturnValue)
		{
			IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_CALL_SHUTDOWN_ON_EXIT, TRUE));
		}

		if (TSS_TPM_RC_SUCCESS == unReturnValue ||
				TSS_TPM_RC_INITIALIZE == (unReturnValue ^ RC_TPM_MASK) ||
				TSS_TPM_RC_FAILURE == (unReturnValue ^ RC_TPM_MASK))
		{
			// The TPM is a TPM2.0
			BYTE bMoreData = 0;
			TSS_TPMS_CAPABILITY_DATA capabilityData = {0};
			PpsTpmState->attribs.tpm20 = 1;

			// Set the failure mode flag in case the TPM2.0 is in failure mode.
			if (TSS_TPM_RC_FAILURE == (unReturnValue ^ RC_TPM_MASK))
			{
				PpsTpmState->attribs.tpm20InFailureMode = 1;
				// And get the test result
				{
					TSS_TPM2B_MAX_BUFFER sOutData = {0};
					TSS_TPM_RC rcTestResult = 0;
					unReturnValue = TSS_TPM2_GetTestResult(&sOutData, &rcTestResult);
					if (TSS_TPM_RC_SUCCESS == unReturnValue)
					{
						unReturnValue = Platform_MemoryCopy(PpsTpmState->testResult, sizeof(PpsTpmState->testResult), sOutData.buffer, sOutData.size);
						if (TSS_TPM_RC_SUCCESS == unReturnValue)
						{
							PpsTpmState->unTestResultLen = sOutData.size;
						}
					}
				}
			}

			unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_TPM_PROPERTIES, TSS_TPM_PT_MANUFACTURER, 1, &bMoreData, &capabilityData);
			if (TSS_TPM_RC_SUCCESS == unReturnValue && 1 == capabilityData.data.tpmProperties.count)
			{
				if (capabilityData.data.tpmProperties.tpmProperty[0].value == 0x49465800 /* IFX\0 */)
				{
					PpsTpmState->attribs.infineon = 1;
					if (!PpsTpmState->attribs.tpm20InFailureMode)
					{
						// Check whether platformAuth is the Empty Buffer and platform hierarchy is enabled.
						// (-> Preconditions for TPMFactoryUpd to update a TPM2.0)
						TSS_AuthorizationCommandData sAuthSessionData = {0};
						TSS_AcknowledgmentResponseData sAckAuthSessionData = {{0}};
						TSS_TPM2B_AUTH sNewAuth = {0};
						// Initialize authorization command data structure
						sAuthSessionData.authHandle = TSS_TPM_RS_PW;	// Use password based authorization session
						sAuthSessionData.sessionAttributes.continueSession = 1;
						unReturnValue = TSS_TPM2_HierarchyChangeAuth(TSS_TPM_RH_PLATFORM, sAuthSessionData, sNewAuth, &sAckAuthSessionData);
						if (TSS_TPM_RC_SUCCESS == unReturnValue)
						{
							// The platformAuth is the Empty Buffer and platform hierarchy is enabled. The TPM can be updated with TPMFactoryUpd.
							PpsTpmState->attribs.tpm20emptyPlatformAuth = 1;
						}
						else if ((unReturnValue ^ RC_TPM_MASK) == (TSS_TPM_RC_HIERARCHY | TSS_TPM_RC_1))
						{
							// The platform hierarchy is disabled. The TPM cannot be updated with TPMFactoryUpd.
							PpsTpmState->attribs.tpm20phDisabled = 1;
							unReturnValue = RC_SUCCESS;
						}
						else if ((unReturnValue ^ RC_TPM_MASK) == (TSS_TPM_RC_BAD_AUTH | TSS_TPM_RC_S | TSS_TPM_RC_1))
						{
							// The platformAuth is not the Empty Buffer. The TPM cannot be updated with TPMFactoryUpd.
							// (Hint: platformAuth is not protected by DA, so a failed TPM2_HierarchyChangeAuth does not have DA implications)
							unReturnValue = RC_SUCCESS;
						}
						else
						{
							ERROR_STORE(unReturnValue, L"TSS_TPM2_HierarchyChangeAuth returned an unexpected value.");
						}
					}
				}
			}
			else
			{
				ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned an unexpected value. (TPM_CAP_TPM_PROPERTIES,TPM_PT_MANUFACTURER)");
			}
		}
		else if ((unReturnValue ^ RC_TPM_MASK) == TSS_TPM_RC_REBOOT)
		{
			// The TPM has been updated to TPM2.0 but has not yet been restarted
			PpsTpmState->attribs.tpm20 = 1;
			PpsTpmState->attribs.infineon = 1; // This may not be true
			PpsTpmState->attribs.tpm20restartRequired = 1;
			unReturnValue = RC_SUCCESS;
		}
		else
		{
			TSS_TPM_CAP_VERSION_INFO tpmVersionInfo = {0};
			unsigned int unTpmVersionSizeInfo = sizeof(TSS_TPM_CAP_VERSION_INFO);
			BYTE rgbIFX[] = { 'I', 'F', 'X' , 0x00};
			unReturnValue = TSS_TPM_Startup(TSS_TPM_ST_CLEAR);
			if (RC_SUCCESS == unReturnValue || TSS_TPM_INVALID_POSTINIT == (unReturnValue ^ RC_TPM_MASK))
			{
				// The TPM is a TPM1.2
				PpsTpmState->attribs.tpm12 = 1;
				unReturnValue = TSS_TPM_GetCapability(TSS_TPM_CAP_VERSION_VAL, 0, NULL, &unTpmVersionSizeInfo, (BYTE*)&tpmVersionInfo);
				if ((RC_SUCCESS == unReturnValue) && 0 == Platform_MemoryCompare(tpmVersionInfo.tpmVendorID, rgbIFX, sizeof(rgbIFX)))
				{
					unsigned int unSubCap = Platform_SwapBytes32(TSS_TPM_CAP_PROP_OWNER);
					BYTE bOwner = 0x0;
					unsigned int unResponseSize = sizeof(bOwner);
					PpsTpmState->attribs.infineon = 1;
					unReturnValue = TSS_TPM_GetCapability(TSS_TPM_CAP_PROPERTY, sizeof(unSubCap), (BYTE*)&unSubCap, &unResponseSize, (BYTE*)&bOwner);
					if (RC_SUCCESS == unReturnValue)
					{
						if (bOwner != 0)
							// The TPM1.2 has an owner, therefore (deferred) physical presence is not relevant for firmware update.
							PpsTpmState->attribs.tpm12owner = 1;
						else
						{
							// The TPM1.2 does not have an owner, therefore the TPM could be updated with Deferred Physical Presence.
							// Check if the Deferred Physical Presence bit is set.
							TSS_IFX_FIELDUPGRADEINFO sFieldUpgradeInfo = {0};
							unReturnValue = TSS_TPM_FieldUpgradeInfoRequest(&sFieldUpgradeInfo);
							if (RC_SUCCESS != unReturnValue)
							{
								ERROR_STORE(unReturnValue, L"TSS_TPM_FieldUpgradeInfoRequest returned an unexpected value.");
								break;
							}
							// Bit 0x8 of wFlagsFieldUpgrade stores the Deferred Physical Presence setting.
							if ((sFieldUpgradeInfo.wFlagsFieldUpgrade & 0x0008) == 0x0008)
							{
								PpsTpmState->attribs.tpm12DeferredPhysicalPresence = 1;
							}
						}
					}
				}
				{
					// Get activated/enabled state.
					// First get permanent flags.
					unsigned int unFlag = Platform_SwapBytes32(TSS_TPM_CAP_FLAG_PERMANENT);
					TSS_TPM_PERMANENT_FLAGS sPermanentFlags = {0};
					TSS_TPM_STCLEAR_FLAGS sStclearFlags = {0};
					unsigned int unResponseSize = sizeof(sPermanentFlags);
					unReturnValue = TSS_TPM_GetCapability(TSS_TPM_CAP_FLAG, sizeof(unFlag), (BYTE*)&unFlag, &unResponseSize, (BYTE*)&sPermanentFlags);
					if (RC_SUCCESS == unReturnValue)
					{
						PpsTpmState->attribs.tpm12enabled = !sPermanentFlags.disable;
						PpsTpmState->attribs.tpm12activated = !sPermanentFlags.deactivated;
						PpsTpmState->attribs.tpm12PhysicalPresenceCMDEnable = sPermanentFlags.physicalPresenceCMDEnable;
						PpsTpmState->attribs.tpm12PhysicalPresenceHWEnable = sPermanentFlags.physicalPresenceHWEnable;
						PpsTpmState->attribs.tpm12PhysicalPresenceLifetimeLock = sPermanentFlags.physicalPresenceLifetimeLock;
					}

					// Then get volatile flags
					unFlag = Platform_SwapBytes32(TSS_TPM_CAP_FLAG_VOLATILE);
					unResponseSize = sizeof(sStclearFlags);
					unReturnValue = TSS_TPM_GetCapability(TSS_TPM_CAP_FLAG, sizeof(unFlag), (BYTE*)&unFlag, &unResponseSize, (BYTE*)&sStclearFlags);
					if (RC_SUCCESS == unReturnValue)
					{
						// Volatile activated flag may overwrite permanent activated flag to false.
						PpsTpmState->attribs.tpm12activated &= !sStclearFlags.deactivated;

						// Extract the physical presence flags
						PpsTpmState->attribs.tpm12PhysicalPresence = sStclearFlags.physicalPresence;
						PpsTpmState->attribs.tpm12PhysicalPresenceLock = sStclearFlags.physicalPresenceLock;
					}
				}
				unReturnValue = RC_SUCCESS;
			}
			else if (TSS_TPM_FAILEDSELFTEST == (unReturnValue ^ RC_TPM_MASK))
			{
				// A TPM1.2 either failed the self test or the TPM is in boot loader mode
				unReturnValue = TSS_TPM_GetCapability(TSS_TPM_CAP_VERSION_VAL, 0, NULL, &unTpmVersionSizeInfo, (BYTE*)&tpmVersionInfo);
				if (RC_SUCCESS == unReturnValue)
				{
					if (0 == Platform_MemoryCompare(tpmVersionInfo.tpmVendorID, rgbIFX, sizeof(rgbIFX)))
					{
						sSecurityModuleLogicInfo_d securityModuleLogicInfo = {0};
						PpsTpmState->attribs.infineon = 1;
						unReturnValue = TSS_TPM_FieldUpgradeInfoRequest2(&securityModuleLogicInfo);
						if (TSS_TPM_RESOURCES == (unReturnValue ^ RC_TPM_MASK))
						{
							// Retry once on TPM_RESOURCES
							unReturnValue = TSS_TPM_FieldUpgradeInfoRequest2(&securityModuleLogicInfo);
						}

						if (TSS_TPM_BAD_PARAM_SIZE == (unReturnValue ^ RC_TPM_MASK) || TSS_TPM_BAD_PARAMETER == (unReturnValue ^ RC_TPM_MASK))
						{
							PpsTpmState->attribs.unsupportedChip = 1;
						}
						else if (TSS_TPM_FAILEDSELFTEST == (unReturnValue ^ RC_TPM_MASK))
						{
							// May occur in FIPS TPM1.2
							PpsTpmState->attribs.tpm12 = 1;
							PpsTpmState->attribs.tpm12FailedSelfTest = 1;
							PpsTpmState->attribs.tpm12FieldUpgradeInfo2Failed = 1;
						}
						else if (RC_SUCCESS == unReturnValue)
						{
							if (securityModuleLogicInfo.SecurityModuleStatus == SMS_BTLDR_ACTIVE)
								PpsTpmState->attribs.bootLoader = 1;
							else
							{
								PpsTpmState->attribs.tpm12 = 1;
								PpsTpmState->attribs.tpm12FailedSelfTest = 1;
							}
						}

						if (PpsTpmState->attribs.tpm12FailedSelfTest)
						{
							BYTE rgbSelftestResult[MAX_NAME] = {0};
							unsigned int unSelftestResultSize = RG_LEN(rgbSelftestResult);
							unReturnValue = TSS_TPM_GetTestResult(&unSelftestResultSize, rgbSelftestResult);
							if (RC_SUCCESS == unReturnValue)
							{
								unReturnValue = Platform_MemoryCopy(PpsTpmState->testResult, sizeof(PpsTpmState->testResult), rgbSelftestResult, unSelftestResultSize);
								if (RC_SUCCESS != unReturnValue)
									break;
								PpsTpmState->unTestResultLen = unSelftestResultSize;
							}
						}
						unReturnValue = RC_SUCCESS;
					}
				}
				else
				{
					ERROR_STORE(unReturnValue, L"TSS_TPM_GetCapability returned an unexpected value. (TSS_TPM_CAP_VERSION_VAL,0)");
				}
			}
			else
			{
				ERROR_STORE(unReturnValue, L"TSS_TPM_Startup returned an unexpected value.");
			}
		}
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		FirmwareUpdate start for TPM2.0.
 *	@details	The function takes the firmware update policy parameter block and the stored policy session and starts the
 *				firmware update process.
 *
 *	@param		PusPolicyParameterBlockSize		Size of the policy parameter block
 *	@param		PrgbPolicyParameterBlock		Pointer to the policy parameter block byte stream
 *	@param		PunSessionHandle				Session handle
 *	@param		PfnProgress						Callback function to indicate the progress
 *
 *	@retval		RC_SUCCESS								The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER						An invalid parameter was passed to the function. Policy parameter block stream is NULL
 *	@retval		RC_E_TPM20_INVALID_POLICY_SESSION		The policy session handle is 0 or invalid or policy authorization failed
 *	@retval		RC_E_TPM20_POLICY_HANDLE_OUT_OF_RANGE	The policy handle value is out of range
 *	@retval		RC_E_TPM20_POLICY_SESSION_NOT_LOADED	The policy session is not loaded to the TPM
 *	@retval		RC_E_CORRUPT_FW_IMAGE					Firmware image is corrupt
 *	@retval		RC_E_FIRMWARE_UPDATE_FAILED				Firmware update started but failed
 *	@retval		RC_E_FAIL								An unexpected error occurred.
 */
_Check_return_
unsigned int
FirmwareUpdate_Start_Tpm20(
	_In_bytecount_(PusPolicyParameterBlockSize)	BYTE*								PrgbPolicyParameterBlock,
	_In_										UINT16								PusPolicyParameterBlockSize,
	_In_										unsigned int						PunSessionHandle,
	_In_										PFN_FIRMWAREUPDATE_PROGRESSCALLBACK	PfnProgress)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// Authorization session area
		TSS_AuthorizationCommandData sAuthSessionData = {0};
		TSS_AcknowledgmentResponseData sAckAuthSessionData = {{0}};

		// Policy parameter block data
		sSignedData_d sSignedData = {0};

		BYTE* rgbPolicyParameterBlock = NULL;
		int nPolicyParameterBlockSize = 0;

		// Out parameter
		unsigned short usStartSize = 0;

		// Check parameters
		if (NULL == PrgbPolicyParameterBlock ||
				0 == PusPolicyParameterBlockSize)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PrgbPolicyParameterBlock is NULL or PusPolicyParameterBlockSize is zero)");
			break;
		}

		if (0 == PunSessionHandle)
		{
			unReturnValue = RC_E_TPM20_INVALID_POLICY_SESSION;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PunSessionHandle is zero)");
			break;
		}

		// Copy pointer and size for unmarshalling
		rgbPolicyParameterBlock = PrgbPolicyParameterBlock;
		nPolicyParameterBlockSize = PusPolicyParameterBlockSize;

		// Unmarshal the block to sSignedData structure
		unReturnValue = TSS_sSignedData_d_Unmarshal(&sSignedData, &rgbPolicyParameterBlock, &nPolicyParameterBlockSize);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE_FMT(RC_E_CORRUPT_FW_IMAGE, L"TSS_sSignedData_d_Unmarshal returned an unexpected value. (0x%.8x)", unReturnValue);
			unReturnValue = RC_E_CORRUPT_FW_IMAGE;
			break;
		}

		// Initialize authorization command data structure
		sAuthSessionData.authHandle = PunSessionHandle;
		sAuthSessionData.sessionAttributes.continueSession = 1;

		// Call TPM2_FieldUpgradeStartVendor command
		unReturnValue = TSS_TPM2_FieldUpgradeStartVendor(TSS_TPM_RH_PLATFORM, sAuthSessionData, sSignedData, &usStartSize, &sAckAuthSessionData);
		if ((RC_TPM_MASK | TSS_TPM_RC_REFERENCE_S0) == unReturnValue)
		{
			// Policy session handle is not loaded to the TPM
			unReturnValue = RC_E_TPM20_POLICY_SESSION_NOT_LOADED;
			ERROR_STORE(unReturnValue, L"A \"policy session handle is not loaded\" error was returned by TSS_TPM2_FieldUpgradeStartVendor function.");
			break;
		}
		else if ((RC_TPM_MASK | TSS_TPM_RC_VALUE | TSS_TPM_RC_S | TSS_TPM_RC_1) == unReturnValue)
		{
			// Policy session handle value is out of range
			unReturnValue = RC_E_TPM20_POLICY_HANDLE_OUT_OF_RANGE;
			ERROR_STORE(unReturnValue, L"A \"policy handle is out of range\" error was returned by TSS_TPM2_FieldUpgradeStartVendor function");
			break;
		}
		else if ((RC_TPM_MASK | TSS_TPM_RC_POLICY_FAIL | TSS_TPM_RC_S | TSS_TPM_RC_1) == unReturnValue)
		{
			// Policy session is invalid
			unReturnValue = RC_E_TPM20_INVALID_POLICY_SESSION;
			ERROR_STORE(unReturnValue, L"A \"policy is invalid\" error was returned by TSS_TPM2_FieldUpgradeStartVendor function.");
			break;
		}
		else if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM2_FieldUpgradeStartVendor returned an unexpected value.(0x%.8x)", unReturnValue);
			unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
			break;
		}

		// The TPM switches to boot loader mode, so there is no more need to call TPM2_Shutdown to prevent unorderly shutdowns.
		if (PropertyStorage_ExistsElement(PROPERTY_CALL_SHUTDOWN_ON_EXIT))
		{
			IGNORE_RETURN_VALUE_BOOL(PropertyStorage_RemoveElement(PROPERTY_CALL_SHUTDOWN_ON_EXIT));
		}

		// Set Progress to 1%
		PfnProgress(1);

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		FirmwareUpdate start for TPM1.2.
 *	@details	The function takes the firmware update policy parameter block and starts the firmware update process.
 *
 *	@param		PbfTpmAttributes				The operation mode of the TPM.
 *	@param		PrgbPolicyParameterBlock		Pointer to the policy parameter block byte stream
 *	@param		PusPolicyParameterBlockSize		Size of the policy parameter block
 *	@param		PrgbOwnerAuthHash				TPM Owner authentication hash (sha1)
 *	@param		PfnProgress						Callback function to indicate the progress
 *
 *	@retval		RC_SUCCESS						The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER				An invalid parameter was passed to the function. Policy parameter block stream is NULL or policy session handle is not 0.
 *	@retval		RC_E_CORRUPT_FW_IMAGE			Policy parameter block is corrupt and/or cannot be unmarshalled.
 *	@retval		RC_E_FAIL						TPM connection or command error.
 *	@retval		RC_E_TPM12_DEFERREDPP_REQUIRED	Deferred Physical Presence has not been set (TPM1.2 only).
 *	@retval		RC_E_FIRMWARE_UPDATE_FAILED		The update operation was started but failed.
 *	@retval		RC_E_TPM12_INVALID_OWNERAUTH	TPM Owner authentication is incorrect (TPM1.2 only).
 *	@retval		RC_E_TPM12_DA_ACTIVE			The TPM Owner is locked out due to dictionary attack (TPM1.2 only).
 */
_Check_return_ unsigned int FirmwareUpdate_Start_Tpm12(
	_In_										BITFIELD_TPM_ATTRIBUTES				PbfTpmAttributes,
	_In_bytecount_(PusPolicyParameterBlockSize)	BYTE*								PrgbPolicyParameterBlock,
	_In_										UINT16								PusPolicyParameterBlockSize,
	_In_bytecount_(TSS_SHA1_DIGEST_SIZE)		const BYTE							PrgbOwnerAuthHash[TSS_SHA1_DIGEST_SIZE],
	_In_										PFN_FIRMWAREUPDATE_PROGRESSCALLBACK	PfnProgress)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		TSS_TPM_NONCE sNonceEven = {{0}};
		TSS_TPM_AUTHHANDLE unAuthHandle = 0;
		BYTE* pbOwnerAuth = NULL;
		BYTE* rgbPolicyParameterBlock = NULL;
		int nPolicyParameterBlockSize = 0;

		// Policy parameter block data
		sSignedData_d sSignedData = {0};

		// Check parameters
		if (NULL == PrgbPolicyParameterBlock ||
				0 == PusPolicyParameterBlockSize)
		{
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PrgbPolicyParameterBlock is NULL or PusPolicyParameterBlockSize is zero)");
			unReturnValue = RC_E_BAD_PARAMETER;
			break;
		}

		// Copy pointer and size for unmarshalling
		rgbPolicyParameterBlock = PrgbPolicyParameterBlock;
		nPolicyParameterBlockSize = PusPolicyParameterBlockSize;

		// Unmarshal the block to sSignedData structure
		unReturnValue = TSS_sSignedData_d_Unmarshal(&sSignedData, &rgbPolicyParameterBlock, &nPolicyParameterBlockSize);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE_FMT(RC_E_CORRUPT_FW_IMAGE, L"TSS_sSignedData_d_Unmarshal returned an unexpected value. (0x%.8x)", unReturnValue);
			unReturnValue = RC_E_CORRUPT_FW_IMAGE;
			break;
		}

		if (PbfTpmAttributes.tpm12 && PbfTpmAttributes.tpm12owner)
		{
			// Get dictionary attack state for TPM_ET_OWNER and return RC_E_TPM12_DA_ACTIVE if TPM Owner is locked out.
			UINT16 usSubCap = TSS_TPM_ET_OWNER;
			TSS_TPM_DA_INFO sDaInfo = {0};
			unsigned int unDaInfoSize = sizeof(TSS_TPM_DA_INFO);
			usSubCap = Platform_SwapBytes16(usSubCap);
			unReturnValue = TSS_TPM_GetCapability(TSS_TPM_CAP_DA_LOGIC, sizeof(usSubCap), (BYTE*)&usSubCap, &unDaInfoSize, (BYTE*)&sDaInfo);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TSS_TPM_GetCapability returned an unexpected value.");
				break;
			}

			if (TSS_TPM_DA_STATE_ACTIVE == sDaInfo.state)
			{
				unReturnValue = RC_E_TPM12_DA_ACTIVE;
				ERROR_STORE(unReturnValue, L"TPM1.2 is in a dictionary attack mode.");
				break;
			}

			// Check TPM Owner authorization (except for disabled/deactivated TPM)
			unReturnValue = FirmwareUpdate_CheckOwnerAuthorization(PrgbOwnerAuthHash);
			if (RC_SUCCESS != unReturnValue)
			{
				if ((TSS_TPM_DEACTIVATED == (unReturnValue ^ RC_TPM_MASK)) || (TSS_TPM_DISABLED == (unReturnValue ^ RC_TPM_MASK)))
				{
					// In disabled and/or deactivated state TPM Owner authorization may not be checkable prior to first TPM_FieldUpgrade request
				}
				else
				{
					if (TSS_TPM_AUTHFAIL == (unReturnValue ^ RC_TPM_MASK))
					{
						unReturnValue = RC_E_TPM12_INVALID_OWNERAUTH;
						ERROR_STORE(unReturnValue, L"TPM1.2 Owner authentication is incorrect.");
					}
					else
					{
						ERROR_STORE(unReturnValue, L"TPM1.2 Owner authentication failed.");
						unReturnValue = RC_E_FAIL;
					}
					break;
				}
			}

			// Start OIAP session for TPM Owner authorized firmware update.
			unReturnValue = TSS_TPM_OIAP(&unAuthHandle, &sNonceEven);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"TSS_TPM_OIAP returned an unexpected value.");
				break;
			}

			pbOwnerAuth = (BYTE*)PrgbOwnerAuthHash;
		}

		unReturnValue = TSS_TPM_FieldUpgradeStart(PrgbPolicyParameterBlock, PusPolicyParameterBlockSize, pbOwnerAuth, unAuthHandle, &sNonceEven);
		if (RC_SUCCESS != unReturnValue)
		{
			if (TSS_TPM_BAD_PRESENCE == (unReturnValue ^ RC_TPM_MASK))
			{
				unReturnValue = RC_E_TPM12_DEFERREDPP_REQUIRED;
				ERROR_STORE(unReturnValue, L"Deferred physical presence is required.");
			}
			else if (TSS_TPM_AUTHFAIL == (unReturnValue ^ RC_TPM_MASK))
			{
				unReturnValue = RC_E_TPM12_INVALID_OWNERAUTH;
				ERROR_STORE(unReturnValue, L"The signature of the policy parameter block is invalid.");
			}
			else
			{
				ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"Error while sending the policy parameter block. (0x%.8x)", unReturnValue);
				unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
			}

			if (PbfTpmAttributes.tpm12owner)
			{
				IGNORE_RETURN_VALUE(TSS_TPM_FlushSpecific(unAuthHandle, TPM_RT_AUTH));
			}

			break;
		}

		// Set Progress to 1%
		PfnProgress(1);

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		FirmwareUpdateProcess Update
 *	@details	The function determines the maximum data size for a firmware block and sends the firmware to the TPM
 *				in chunks of maximum data size.
 *
 *	@param		PunFirmwareBlockSize	Size of the firmware block
 *	@param		PrgbFirmwareBlock		Pointer to the firmware block byte stream
 *	@param		PfnProgress				Callback function to indicate the progress
 *
 *	@retval		RC_SUCCESS						The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER				An invalid parameter was passed to the function. The firmware image block is NULL
 *	@retval		RC_E_FAIL						In case of a TPM connection or command error
 *	@retval		RC_E_FIRMWARE_UPDATE_FAILED		The update operation was started but failed
 *	@retval		RC_E_TPM_NO_BOOT_LOADER_MODE	The TPM is not in boot loader mode
 */
_Check_return_ unsigned int FirmwareUpdate_Update(
	_In_									unsigned int						PunFirmwareBlockSize,
	_In_bytecount_(PunFirmwareBlockSize)	BYTE*								PrgbFirmwareBlock,
	_In_									PFN_FIRMWAREUPDATE_PROGRESSCALLBACK	PfnProgress)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		BYTE* rgbFirmwareBlock = NULL;
		unsigned int unRemainingBytes = 0;
		unsigned int unBlockNumber = 0;
		unsigned int unCurrentProgress = 1;
		UINT16 usMaxDataSize = 0;

		// Check parameters
		if (NULL == PrgbFirmwareBlock)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PrgbFirmwareBlock is NULL)");
			break;
		}

		rgbFirmwareBlock = PrgbFirmwareBlock;
		unRemainingBytes = PunFirmwareBlockSize;

		{
			unsigned int unRetryCounter = 0;

			for (unRetryCounter = 0; unRetryCounter < TPM_FU_START_MAX_RETRIES; unRetryCounter++)
			{
				// Get the max data size for a firmware update block.
				sSecurityModuleLogicInfo_d securityModuleLogicInfo = {0};
				unReturnValue = TSS_TPM_FieldUpgradeInfoRequest2(&securityModuleLogicInfo);
				if (TSS_TPM_RESOURCES == (unReturnValue ^ RC_TPM_MASK))
				{
					// Retry once on TPM_RESOURCES
					unReturnValue = TSS_TPM_FieldUpgradeInfoRequest2(&securityModuleLogicInfo);
				}
				if (RC_SUCCESS != unReturnValue)
				{
					if (TSS_TPM_BAD_PARAM_SIZE == (unReturnValue ^ RC_TPM_MASK) ||
							TSS_TPM_BAD_PARAMETER == (unReturnValue ^ RC_TPM_MASK))
					{
						ERROR_STORE(unReturnValue, L"TSS_TPM_FieldUpgradeInfoRequest2 returned an unexpected value.");
						unReturnValue = RC_E_UNSUPPORTED_CHIP;
					}
					ERROR_STORE(unReturnValue, L"TSS_TPM_FieldUpgradeInfoRequest2 returned an unexpected value.");
					break;
				}
				usMaxDataSize = securityModuleLogicInfo.wMaxDataSize;

				// Verify that TPM switched to boot loader mode. Wait some time if it did not.
				if (securityModuleLogicInfo.SecurityModuleStatus != SMS_BTLDR_ACTIVE)
				{
					unReturnValue = RC_E_TPM_NO_BOOT_LOADER_MODE;
					LOGGING_WRITE_LEVEL1_FMT(L"TPM is not in boot loader mode as expected (Count:%d)", unRetryCounter);
					Platform_Sleep(TPM_FU_START_RETRY_WAIT_TIME);
					continue;
				}
				else
				{
					unReturnValue = RC_SUCCESS;
					break;
				}
			}

			if (RC_SUCCESS != unReturnValue)
				break;
		}

		// Send the firmware image to the TPM block-by-block.
		for (unBlockNumber = 1; unRemainingBytes > 0; unBlockNumber++)
		{
			UINT16 usBlockSize = unRemainingBytes < usMaxDataSize ? (UINT16)unRemainingBytes : usMaxDataSize;

			// Transmit data block
			unReturnValue = TSS_TPM_FieldUpgradeUpdate(rgbFirmwareBlock, usBlockSize);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM_FieldUpgradeUpdate returned an unexpected value while processing block %d. (0x%.8x)", unBlockNumber, unReturnValue);
				unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
				break;
			}

			// Increase data pointer by block size
			rgbFirmwareBlock += usBlockSize;

			// Decrease size of remaining data by block size
			unRemainingBytes -= usBlockSize;

			// Set Progress (1 and 100% are set for _Start and _Complete so 98 steps are left)
			{
				unsigned int unProgress = (PunFirmwareBlockSize - unRemainingBytes) * 98 / PunFirmwareBlockSize + 1;
				if (unCurrentProgress != unProgress)
				{
					unCurrentProgress = unProgress;
					PfnProgress(unCurrentProgress);
				}
			}
		}
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		FirmwareUpdate Complete
 *	@details	The function finishes the firmware update process.
 *
 *	@param		PfnProgress				Callback function to indicate the progress
 *	@retval		RC_SUCCESS						The operation completed successfully.
 *	@retval		RC_E_FAIL						In case of a TPM connection or command error
 *	@retval		RC_E_FIRMWARE_UPDATE_FAILED		The update operation was started but failed
 */
_Check_return_ unsigned int FirmwareUpdate_Complete(
	_In_	PFN_FIRMWAREUPDATE_PROGRESSCALLBACK			PfnProgress)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		// Send TPM_FieldUpgradeComplete command.
		uint16_t usCompleteDataSize = 1;
		uint16_t usOutCompleteSize = 0;
		BYTE bCompleteData = 0;

		if (RC_SUCCESS != TSS_TPM_FieldUpgradeComplete(usCompleteDataSize, &bCompleteData, &usOutCompleteSize))
		{
			ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM_FieldUpgradeComplete returned an unexpected value. (0x%.8x)", unReturnValue);
			unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
			break;
		}

		// Check if out parameter is as expected (zero)
		if (0 != usOutCompleteSize)
		{
			unReturnValue = RC_E_FAIL;
			break;
		}

		// Wait for TPM to complete update sequence
		Platform_Sleep(TPM_FU_COMPLETE_WAIT_TIME);

		// Set Progress to 100%
		PfnProgress(100);

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Returns information about the current state of TPM
 *	@details
 *
 *	@param		PwszVersionName				A pointer to a null-terminated string representing the firmware image version name.
 *	@param		PpunVersionNameSize			IN: Capacity of the PwszVersionName parameter in wide characters including zero termination
 *											OUT: Length of written wide character string (excluding zero termination)
 *	@param		PpsTpmState					Receives TPM state.
 *	@param		PpunRemainingUpdates		Receives the number of remaining updates or -1 in case the value could not be detected.
 *
 *	@retval		RC_SUCCESS					The operation completed successfully.
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function.
 *	@retval		RC_E_NO_IFX_TPM				The TPM is not manufactured by Infineon.
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetImageInfo(
	_Inout_bytecap_(*PpunVersionNameSize)	wchar_t*					PwszVersionName,
	_Inout_									unsigned int*				PpunVersionNameSize,
	_Inout_									TPM_STATE*					PpsTpmState,
	_Inout_									unsigned int*				PpunRemainingUpdates)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		unsigned int unTpmFuCounter = REMAINING_UPDATES_UNAVAILABLE;

		// Check parameters
		if (NULL == PwszVersionName ||
				NULL == PpunVersionNameSize ||
				0 == *PpunVersionNameSize ||
				NULL == PpsTpmState ||
				NULL == PpunRemainingUpdates)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Bad parameter detected. PwszVersionName, PpunVersionNameSize, PpsTpmState or PpunRemainingUpdates is NULL or *PpunVersionNameSize is 0.");
			break;
		}

		// Get TPM operation mode
		unReturnValue = FirmwareUpdate_CalculateState(PpsTpmState);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Check if the recognized TPM is from an unsupported vendor and return the appropriate value.
		if (PpsTpmState->attribs.infineon == 0)
		{
			unReturnValue = RC_E_NO_IFX_TPM;
			ERROR_STORE(unReturnValue, L"Unsupported TPM vendor.");
			break;
		}

		// Get firmware version string
		{
			wchar_t wszDummy[MAX_NAME] = {0};
			unsigned int unDummySize = RG_LEN(wszDummy);
			unReturnValue = FirmwareUpdate_GetTpmFirmwareVersionString(PpsTpmState->attribs, PwszVersionName, PpunVersionNameSize, wszDummy, &unDummySize);
			if (RC_SUCCESS != unReturnValue)
				break;
		}

		// Read update counter from TPM (cannot be done after firmware update to TPM 2.0 when reboot is required)
		if (!(PpsTpmState->attribs.tpm20 && PpsTpmState->attribs.tpm20restartRequired) &&
				!PpsTpmState->attribs.tpm20InFailureMode &&
				!(PpsTpmState->attribs.tpm12 && PpsTpmState->attribs.tpm12FieldUpgradeInfo2Failed))
		{
			unReturnValue = FirmwareUpdate_GetTpmFieldUpgradeCounter(PpsTpmState->attribs, &unTpmFuCounter);
			if (RC_SUCCESS != unReturnValue)
				break;
		}

		// Fill in number of remaining updates
		*PpunRemainingUpdates = unTpmFuCounter;

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Checks if the firmware image is valid for the TPM
 *	@details	Performs integrity, consistency and content checks to determine if the given firmware image can be applied to the installed TPM.
 *
 *	@param		PrgbImage					Firmware image byte stream
 *	@param		PullImageSize				Size of firmware image byte stream
 *	@param		PpfValid					TRUE in case the image is valid, FALSE otherwise.
 *	@param		PpbfNewTpmFirmwareInfo		Pointer to a bit field to return info data for the new firmware image.
 *	@param		PpunErrorDetails			Pointer to an unsigned int to return error details. Possible values are:\n
 *												RC_E_FW_UPDATE_BLOCKED in case the field upgrade counter value has been exceeded.\n
 *												RC_E_WRONG_FW_IMAGE in case the TPM is not updatable with the given image.\n
 *												RC_E_CORRUPT_FW_IMAGE in case the firmware image is corrupt.\n
 *												RC_E_NEWER_TOOL_REQUIRED in case a newer version of the tool is required to parse the firmware image.\n
 *												RC_E_WRONG_DECRYPT_KEYS in case the TPM2.0 does not have decrypt keys matching to the firmware image.
 *
 *	@retval		RC_SUCCESS					The operation completed successfully
 *	@retval		RC_E_BAD_PARAMETER			An invalid parameter was passed to the function.
 *	@retval		RC_E_TPM20_FAILURE_MODE		The TPM2.0 is in failure mode.
 *	@retval		RC_E_RESTART_REQUIRED		In case a restart is required to get back to a functional TPM
 *	@retval		RC_E_NO_IFX_TPM				In case TPM vendor is not IFX
 *	@retval		RC_E_FAIL					An unexpected error occurred.
 *	@retval		...							Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_CheckImage(
	_In_bytecount_(PullImageSize)	BYTE*							PrgbImage,
	_In_							unsigned long long				PullImageSize,
	_Out_							BOOL*							PpfValid,
	_Out_							BITFIELD_NEW_TPM_FIRMWARE_INFO*	PpbfNewTpmFirmwareInfo,
	_Out_							unsigned int*					PpunErrorDetails)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		TPM_STATE sTpmState = {{0}};
		IfxFirmwareImage sIfxFirmwareImage = {{0}};
		int nBufferSize = (int)PullImageSize;
		BYTE* pbBuffer = PrgbImage;

		// Check parameters
		if (NULL == PrgbImage ||
				0 == PullImageSize ||
				NULL == PpfValid ||
				NULL == PpbfNewTpmFirmwareInfo ||
				NULL == PpunErrorDetails)
		{
			unReturnValue = RC_E_BAD_PARAMETER;
			ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PrgbImage or PpfValid or PpbfNewTpmFirmwareInfo is NULL or PullImageSize is zero)");
			break;
		}

		// Initialize out parameters
		*PpunErrorDetails = RC_E_FW_UPDATE_BLOCKED;
		*PpfValid = FALSE;

		unReturnValue = Platform_MemorySet(PpbfNewTpmFirmwareInfo, 0, sizeof(BITFIELD_NEW_TPM_FIRMWARE_INFO));
		if (RC_SUCCESS != unReturnValue)
			break;

		// Get TPM operation mode
		unReturnValue = FirmwareUpdate_CalculateState(&sTpmState);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Check if the recognized TPM is from an unsupported vendor and return the appropriate value.
		if (!sTpmState.attribs.infineon)
		{
			unReturnValue = RC_E_NO_IFX_TPM;
			ERROR_STORE(unReturnValue, L"Unsupported TPM vendor.");
			break;
		}

		// Cannot make a decision if restart is required.
		if (sTpmState.attribs.tpm20restartRequired)
		{
			unReturnValue = RC_E_RESTART_REQUIRED;
			ERROR_STORE(unReturnValue, L"A restart is required.");
			*PpunErrorDetails = RC_E_WRONG_FW_IMAGE;
			break;
		}

		// Cannot make a decision if TPM2.0 is in failure mode.
		if (sTpmState.attribs.tpm20InFailureMode)
		{
			unReturnValue = RC_E_TPM20_FAILURE_MODE;
			ERROR_STORE(unReturnValue, L"TPM2.0 is in failure mode.");
			break;
		}

		// Unmarshal the firmware image structure
		unReturnValue = FirmwareImage_Unmarshal(&sIfxFirmwareImage, &pbBuffer, &nBufferSize);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"Failed to unmarshal the firmware image.");
			unReturnValue = RC_SUCCESS;
			*PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
			break;
		}

		// Check if update is possible
		nBufferSize = (int)PullImageSize;
		unReturnValue = FirmwareUpdate_IsFirmwareUpdatable(sTpmState.attribs, PrgbImage, nBufferSize, &sIfxFirmwareImage, PpfValid, PpbfNewTpmFirmwareInfo, PpunErrorDetails);
		if (RC_SUCCESS != unReturnValue)
			break;

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Prepares a policy session for TPM firmware.
 *	@details	The function prepares a policy session for TPM Firmware Update.
 *
 *	@param		PphPolicySession					Pointer to session handle that will be filled in by this method.
 *
 *	@retval		RC_SUCCESS							The operation completed successfully.
 *	@retval		RC_E_BAD_PARAMETER					An invalid parameter was passed to the function. It is invalid or NULL.
 *	@retval		RC_E_FAIL							An unexpected error occurred.
 *	@retval		RC_E_PLATFORM_AUTH_NOT_EMPTY		In case PlatformAuth is not the Empty Buffer.
 *	@retval		RC_E_PLATFORM_HIERARCHY_DISABLED	In case platform hierarchy has been disabled.
 *	@retval		...									Error codes from called functions. like TPM error codes
 */
_Check_return_
unsigned int
FirmwareUpdate_PrepareTPM20Policy(
	_Out_ TSS_TPMI_SH_AUTH_SESSION* PphPolicySession)
{
	unsigned int unReturnValue = RC_E_FAIL;
	TSS_TPMI_SH_AUTH_SESSION hPolicySession = 0;
	*PphPolicySession = 0;

	do
	{
		TSS_TPM2B_NONCE sNonceTpm = {0};
		TSS_TPM2B_NONCE sNonceCaller = {0};

		TSS_TPM2B_DIGEST sPolicyDigest = {0};

		// Authorization session area
		TSS_AuthorizationCommandData sAuthSessionData = {0};
		TSS_AcknowledgmentResponseData sAckAuthSessionData = {{0}};

		sPolicyDigest.size = TSS_SHA256_DIGEST_SIZE;
		unReturnValue = Platform_MemoryCopy(sPolicyDigest.buffer, sPolicyDigest.size, rgbTpm20FirmwareUpdatePolicyDigest, sPolicyDigest.size);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an unexpected value while copying rgbTpm20FirmwareUpdatePolicyDigest.");
			break;
		}

		// Call TPM2_SetPrimaryPolicy command
		sAuthSessionData.authHandle = TSS_TPM_RS_PW;	// Use password based authorization session
		sAuthSessionData.sessionAttributes.continueSession = 1;
		// sAuthSessionData.hmac.size = 0;			// Due to current Empty Buffer platform AuthSecret
		// sAuthSessionData.nonceCaller.size = 0;	// Due to password based authorization session

		// PolicyDigest.buffer: return value of TPM2_PolicyDigest
		unReturnValue = TSS_TPM2_SetPrimaryPolicy(TSS_TPM_RH_PLATFORM, sAuthSessionData, sPolicyDigest, TSS_TPM_ALG_SHA256, &sAckAuthSessionData);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
		{
			if ((TSS_TPM_RC_HIERARCHY | TSS_TPM_RC_1) == (unReturnValue ^ RC_TPM_MASK))
			{
				ERROR_STORE_FMT(RC_E_PLATFORM_HIERARCHY_DISABLED, L"TSS_TPM2_SetPrimaryPolicy returned that platform hierarchy is disabled. (0x%.8X)", unReturnValue);
				unReturnValue = RC_E_PLATFORM_HIERARCHY_DISABLED;
				break;
			}
			else if ((TSS_TPM_RC_BAD_AUTH | TSS_TPM_RC_S | TSS_TPM_RC_1) == (unReturnValue ^ RC_TPM_MASK))
			{
				ERROR_STORE_FMT(RC_E_PLATFORM_AUTH_NOT_EMPTY, L"TSS_TPM2_SetPrimaryPolicy returned that platformAuth is not the EmptyBuffer. (0x%.8X)", unReturnValue);
				unReturnValue = RC_E_PLATFORM_AUTH_NOT_EMPTY;
				break;
			}
			ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_SetPrimaryPolicy");
			break;
		}

		sNonceCaller.size = TSS_SHA256_DIGEST_SIZE;
		unReturnValue = Crypt_GetRandom(sNonceCaller.size, sNonceCaller.buffer);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"Error calling Crypt_GetRandom");
			break;
		}

		{
			// Start policy session
			TSS_TPM2B_ENCRYPTED_SECRET sEncSecretEmpty = {0};
			TSS_TPMT_SYM_DEF sSymDefEmpty = {0};
			sSymDefEmpty.algorithm = TSS_TPM_ALG_NULL;
			unReturnValue = TSS_TPM2_StartAuthSession(TSS_TPM_RH_NULL,
							// Bind must be NULL, otherwise we'd have
							// to calculate AuthSessionData.hmac !!
							TSS_TPM_RH_NULL,
							sNonceCaller, sEncSecretEmpty,
							TSS_TPM_SE_POLICY, sSymDefEmpty, TSS_TPM_ALG_SHA256,
							&hPolicySession, &sNonceTpm);
			if (TSS_TPM_RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_StartAuthSession");
				break;
			}
		}

		{
			// Update policy session to include platformAuth
			TSS_TPM2B_DIGEST sDigestEmpty = {0};
			TSS_TPM2B_NONCE sPolicyRef = {0};
			TSS_TPMT_TK_AUTH sPolicyTicket = {0};
			TSS_TPM2B_TIMEOUT sTimeout = {0};
			unsigned int unPlatformValue = Platform_SwapBytes32(TSS_TPM_RH_PLATFORM);
			sPolicyRef.size = sizeof(unsigned int);
			unReturnValue = Platform_MemoryCopy(sPolicyRef.buffer, sPolicyRef.size, (const void*) &unPlatformValue, sizeof(unPlatformValue));
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Unexpected return value from function call Platform_MemoryCopy");
				break;
			}

			unReturnValue = TSS_TPM2_PolicySecret(TSS_TPM_RH_PLATFORM, sAuthSessionData, hPolicySession, sNonceTpm, sDigestEmpty, sPolicyRef, 0, &sTimeout, &sPolicyTicket, &sAckAuthSessionData);
			if (TSS_TPM_RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_PolicySecret");
				break;
			}
		}

		// Update policy session to include command TPM2_FieldUpgradeStartVendor
		unReturnValue = TSS_TPM2_PolicyCommandCode(hPolicySession, TPM2_CC_FieldUpgradeStartVendor);
		if (TSS_TPM_RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_PolicyCommandCode");
			break;
		}

		*PphPolicySession = hPolicySession;
	}
	WHILE_FALSE_END;

	// Try to close policy session in case of errors (only if session has already been started)
	if (RC_SUCCESS != unReturnValue && 0 != hPolicySession)
		IGNORE_RETURN_VALUE(TSS_TPM2_FlushContext(hPolicySession));

	return unReturnValue;
}

/**
 *	@brief		Function to issue TPM_FieldUpgrade_Start
 *	@details	This function initiates TPM Firmware Update via the TPM_FieldUpgrade_Start command depending on the current TPM Operation mode.
 *
 *	@param		PbfTpmAttributes		The current TPM operation mode
 *	@param		PpsIfxFirmwareImage		Pointer to structure containing the firmware image
 *	@param		PpsFirmwareUpdateData	Pointer to structure containing all relevant data for a firmware update
 *
 *	@retval		RC_SUCCESS						The operation completed successfully.
 *	@retval		RC_E_TPM12_MISSING_OWNERAUTH	The TPM has an owner but TPM Owner authorization was not provided (TPM1.2 only).
 *	@retval		RC_E_TPM12_NO_OWNER				The TPM does not have an owner but TPM Owner authorization was provided (TPM1.2 only).
 *	@retval		RC_E_FAIL						TPM connection or command error.
 *	@retval		...								Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_Start(
	_In_	BITFIELD_TPM_ATTRIBUTES				PbfTpmAttributes,
	_In_	const IfxFirmwareImage* const		PpsIfxFirmwareImage,
	_In_	const IfxFirmwareUpdateData* const	PpsFirmwareUpdateData)
{
	unsigned int unReturnValue = RC_E_FAIL;
	BOOL fUpdateStarted = FALSE;

	if (PbfTpmAttributes.tpm20 && PbfTpmAttributes.infineon && !PbfTpmAttributes.tpm20restartRequired)
	{
		unReturnValue = FirmwareUpdate_Start_Tpm20(
							PpsIfxFirmwareImage->rgbPolicyParameterBlock,
							PpsIfxFirmwareImage->usPolicyParameterBlockSize,
							PpsFirmwareUpdateData->unSessionHandle,
							PpsFirmwareUpdateData->fnProgressCallback);
		fUpdateStarted = RC_SUCCESS == unReturnValue ? TRUE : FALSE;
	}
	else if (PbfTpmAttributes.tpm12 && PbfTpmAttributes.infineon && !PbfTpmAttributes.unsupportedChip)
	{
		if (PbfTpmAttributes.tpm12owner)
		{
			if (!PpsFirmwareUpdateData->fOwnerAuthProvided)
			{
				unReturnValue = RC_E_TPM12_MISSING_OWNERAUTH;
				ERROR_STORE(unReturnValue, L"TPM1.2 has an owner but TPM Owner authorization was not provided.");
			}
			else
			{
				unReturnValue = FirmwareUpdate_Start_Tpm12(
									PbfTpmAttributes,
									PpsIfxFirmwareImage->rgbPolicyParameterBlock,
									PpsIfxFirmwareImage->usPolicyParameterBlockSize,
									PpsFirmwareUpdateData->rgbOwnerAuthHash,
									PpsFirmwareUpdateData->fnProgressCallback);
				fUpdateStarted = RC_SUCCESS == unReturnValue ? TRUE : FALSE;
			}
		}
		else
		{
			if (PpsFirmwareUpdateData->fOwnerAuthProvided)
			{
				unReturnValue = RC_E_TPM12_NO_OWNER;
				ERROR_STORE(unReturnValue, L"TPM1.2 does not have an owner but TPM Owner authorization was provided.");
			}
			else
			{
				unReturnValue = FirmwareUpdate_Start_Tpm12(
									PbfTpmAttributes,
									PpsIfxFirmwareImage->rgbPolicyParameterBlock,
									PpsIfxFirmwareImage->usPolicyParameterBlockSize,
									PpsFirmwareUpdateData->rgbOwnerAuthHash,
									PpsFirmwareUpdateData->fnProgressCallback);
				fUpdateStarted = RC_SUCCESS == unReturnValue ? TRUE : FALSE;
			}
		}
	}
	else if (PbfTpmAttributes.bootLoader)
	{
		unReturnValue = RC_SUCCESS; // Continue interrupted firmware update
		PpsFirmwareUpdateData->fnProgressCallback(1);
	}
	else
	{
		ERROR_STORE_FMT(unReturnValue, L"Error: Unexpected TPM state attributes were recognized. (Mode: 0x%.8X)", PbfTpmAttributes);
	}

	// Invoke the callback function if firmware update was started and TPM is about to switch to boot loader mode.
	if (fUpdateStarted && NULL != PpsFirmwareUpdateData->fnUpdateStartedCallback)
	{
		PpsFirmwareUpdateData->fnUpdateStartedCallback();
	}

	return unReturnValue;
}

/**
 *	@brief		Function to update the firmware with the given firmware image
 *	@details	This function updates the TPM firmware with the image given in the parameters.
 *				A check if the firmware can be updated with the image is done before.
 *
 *	@param		PpsFirmwareUpdateData	Pointer to structure containing all relevant data for a firmware update
 *
 *	@retval		RC_SUCCESS						The operation completed successfully.
 *	@retval		RC_E_CORRUPT_FW_IMAGE			Policy parameter block is corrupt and/or cannot be unmarshalled.
 *	@retval		RC_E_NO_IFX_TPM					The TPM is not manufactured by Infineon.
 *	@retval		RC_E_TPM12_MISSING_OWNERAUTH	The TPM has an owner but TPM Owner authorization was not provided (TPM1.2 only).
 *	@retval		RC_E_TPM12_NO_OWNER				The TPM does not have an owner but TPM Owner authorization was provided (TPM1.2 only).
 *	@retval		RC_E_FAIL						TPM connection or command error.
 *	@retval		...								Error codes from called functions.
 *	@retval		RC_E_FIRMWARE_UPDATE_FAILED		The update operation was started but failed.
 */
_Check_return_
unsigned int
FirmwareUpdate_UpdateImage(
	_In_	const IfxFirmwareUpdateData* const	PpsFirmwareUpdateData)
{
	unsigned int unReturnValue = RC_E_FAIL;

	do
	{
		TPM_STATE sTpmState = {{0}};
		IfxFirmwareImage sIfxFirmwareImage = {{0}};
		int nBufferSize = (int)PpsFirmwareUpdateData->unFirmwareImageSize;
		BYTE* pbBuffer = PpsFirmwareUpdateData->rgbFirmwareImage;

		// Get TPM operation mode
		unReturnValue = FirmwareUpdate_CalculateState(&sTpmState);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Check if the recognized TPM is from an unsupported vendor and return the appropriate value.
		if (!sTpmState.attribs.infineon)
		{
			unReturnValue = RC_E_NO_IFX_TPM;
			ERROR_STORE(unReturnValue, L"Unsupported TPM vendor.");
			break;
		}

		// Unmarshal the firmware image structure
		unReturnValue = FirmwareImage_Unmarshal(&sIfxFirmwareImage, &pbBuffer, &nBufferSize);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"Firmware image cannot be parsed. (0x%.8x)");
			unReturnValue = RC_E_CORRUPT_FW_IMAGE;
			break;
		}

		// Perform the firmware update
		// Start the firmware update in order to get TPM in Boot Loader Mode
		unReturnValue = FirmwareUpdate_Start(sTpmState.attribs, &sIfxFirmwareImage, PpsFirmwareUpdateData);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Transfer new firmware data to TPM
		unReturnValue = FirmwareUpdate_Update(sIfxFirmwareImage.unFirmwareSize, sIfxFirmwareImage.rgbFirmware, PpsFirmwareUpdateData->fnProgressCallback);
		if (RC_SUCCESS != unReturnValue)
			break;

		// Finalize the firmware update
		unReturnValue = FirmwareUpdate_Complete(PpsFirmwareUpdateData->fnProgressCallback);
		if (RC_SUCCESS != unReturnValue)
			break;

		unReturnValue = RC_SUCCESS;
	}
	WHILE_FALSE_END;

	return unReturnValue;
}

/**
 *	@brief		Checks TPM Owner authorization on TPM1.2.
 *	@details	The function checks TPM Owner authorization on TPM1.2 by calling TPM_ChangeAuthOwner with old and new TPM Owner authorization value set to the same value.
 *
 *	@param		PrgbOwnerAuthHash				TPM Owner Authentication hash (sha1)
 *
 *	@retval		RC_SUCCESS						The operation completed successfully.
 *	@retval		RC_E_FAIL						An internal error occurred.
 *	@retval		...								Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_CheckOwnerAuthorization(
	_In_bytecount_(TSS_SHA1_DIGEST_SIZE)	const BYTE	PrgbOwnerAuthHash[TSS_SHA1_DIGEST_SIZE])
{
	unsigned int unReturnValue = RC_E_FAIL;
	do
	{
		TSS_TPM_NONCE sNonceOddOSAP = {{0}};
		TSS_TPM_AUTHHANDLE unAuthHandle = 0;
		TSS_TPM_NONCE sNonceEven = {{0}};
		TSS_TPM_NONCE sNonceEvenOSAP = {{0}};
		TSS_TPM_AUTHDATA sOsapSharedSecret = {{0}};
		TSS_TPM_ENTITY_TYPE usEntity = (TSS_TPM_ET_XOR << 8) | TSS_TPM_ET_OWNER;
		TSS_TPM_PUBKEY sPubKey = {{0}};

		unReturnValue = Crypt_GetRandom(sizeof(sNonceOddOSAP.nonce), sNonceOddOSAP.nonce);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"Crypt_GetRandom returned an unexpected value.");
			break;
		}

		// Start an OSAP session
		unReturnValue = TSS_TPM_OSAP(usEntity, TSS_TPM_KH_OWNER, sNonceOddOSAP, &unAuthHandle, &sNonceEven, &sNonceEvenOSAP);
		if (RC_SUCCESS != unReturnValue)
		{
			ERROR_STORE(unReturnValue, L"TSS_TPM_OSAP returned an unexpected value.");
			break;
		}

		// Calculate the OSAP shared secret
		{
			BYTE rgbInput[2 * sizeof(TSS_TPM_NONCE)] = {0};
			unReturnValue = Platform_MemoryCopy(rgbInput, sizeof(rgbInput), sNonceEvenOSAP.nonce, sizeof(TSS_TPM_NONCE));
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an unexpected value.");
				break;
			}
			unReturnValue = Platform_MemoryCopy(rgbInput + sizeof(TSS_TPM_NONCE), sizeof(rgbInput) - sizeof(TSS_TPM_NONCE), sNonceOddOSAP.nonce, sizeof(TSS_TPM_NONCE));
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an unexpected value.");
				break;
			}
			unReturnValue = Crypt_HMAC(rgbInput, sizeof(rgbInput), PrgbOwnerAuthHash, sOsapSharedSecret.authdata);
			if (RC_SUCCESS != unReturnValue)
			{
				ERROR_STORE(unReturnValue, L"Crypt_HMAC returned an unexpected value.");
				break;
			}
		}

		{
			TSS_TPM_AUTHDATA sResAuth = {{0}};
			// Read public portion of EK over OSAP session
			unReturnValue = TSS_TPM_OwnerReadInternalPub(TSS_TPM_KH_EK, unAuthHandle, sNonceOddOSAP, &sNonceEven, sOsapSharedSecret, &sPubKey, &sResAuth);
			{
				IGNORE_RETURN_VALUE(TSS_TPM_FlushSpecific(unAuthHandle, TPM_RT_AUTH));
			}
			if (RC_SUCCESS != unReturnValue)
			{
				// For firmware update in disabled / deactivated state TSS_TPM_OwnerReadInternalPub cannot be executed. This is ok, so don't log an error.
				if (!((TSS_TPM_DEACTIVATED == (unReturnValue ^ RC_TPM_MASK)) || (TSS_TPM_DISABLED == (unReturnValue ^ RC_TPM_MASK))))
				{
					ERROR_STORE(unReturnValue, L"TSS_TPM_OwnerReadInternalPub returned an unexpected value.");
				}
				break;
			}
		}
	}
	WHILE_FALSE_END;

	return unReturnValue;
}
