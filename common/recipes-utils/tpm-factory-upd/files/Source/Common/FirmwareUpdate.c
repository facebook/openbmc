/**
 *  @brief      Implements common firmware update functionality used by different tools
 *  @details    Implements an internal firmware update interface used by IFXTPMUpdate and TPMFactoryUpd.
 *  @file       FirmwareUpdate.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
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

// SLB 9672 specific
#include "TPM2_FieldUpgradeManifestVendor.h"
#include "TPM2_FieldUpgradeDataVendor.h"
#include "TPM2_FieldUpgradeFinalizeVendor.h"
#include "TPM2_FieldUpgradeAbandonVendor.h"

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

/**
 *  @brief      Function to read Security Module Logic Info from TPM2.0.
 *  @details    This function obtains the Security Module Logic Info from TPM2.0.
 *
 *  @param      PpSecurityModuleLogicInfo2  Pointer to the Security Module Logic Info.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
_Success_(return == 0)
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
            TSS_TPMS_VENDOR_CAPABILITY_DATA vendorCapabilityData;
            int nBufferSize = 0;
            BYTE* rgbBuffer = NULL;
            BYTE bMoreData = 0;

            Platform_MemorySet(&vendorCapabilityData, 0, sizeof(vendorCapabilityData));
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
 *  @brief      Function to read the field upgrade counter from the TPM
 *  @details    This function obtains the field upgrade counter value from the TPM.
 *
 *  @param      PbfTpmAttributes            TPM state attributes.
 *  @param      PpunUpgradeCounter          Pointer to the field upgrade counter parameter.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     RC_E_UNSUPPORTED_CHIP       In case of the underlying TPM does not support FieldUpdateCounter acquiring commands.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetTpmFieldUpgradeCounter(
    _In_    BITFIELD_TPM_ATTRIBUTES PbfTpmAttributes,
    _Out_   unsigned int*           PpunUpgradeCounter)
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
            // Check for TPM2.0 based firmware update loader
            if (PbfTpmAttributes.tpmHasFULoader20)
            {
                // Read update counter
                BYTE bMoreData = 0;
                TSS_TPMS_VENDOR_CAPABILITY_DATA vendorCapabilityData;
                Platform_MemorySet(&vendorCapabilityData, 0, sizeof(vendorCapabilityData));

                // Get TPM_PT_VENDOR_FIX_FU_COUNTER
                unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_COUNTER, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
                if (TSS_TPM_RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_COUNTER)");
                    break;
                }

                // Check count of capabilities returned
                if (1 != vendorCapabilityData.data.vendorData.count)
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned more than one capability");
                    break;
                }

                // Get pointer to first element in returned max buffer array (TPM_PT_VENDOR_FIX_FU_COUNTER)
                TSS_TPM2B_MAX_BUFFER* pMaxBuffer = &vendorCapabilityData.data.vendorData.buffer[0];

                // Check buffer size
                if (sizeof(TSS_UINT16) != pMaxBuffer->size)
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned wrong buffer size of capability");
                    break;
                }

                // Get value
                BYTE* pbBuffer = pMaxBuffer->buffer;
                TSS_INT32 nSizeRemaining = pMaxBuffer->size;
                TSS_UINT16 upgradeCounter = 0;
                unReturnValue = TSS_UINT16_Unmarshal(&upgradeCounter, &pbBuffer, &nSizeRemaining);
                if (RC_SUCCESS != unReturnValue)
                    break;

                // Return upgrade counter
                *PpunUpgradeCounter = upgradeCounter;
            }
            else
            {
                // TPM2.0
                sSecurityModuleLogicInfo2_d securityModuleLogicInfo2;
                Platform_MemorySet(&securityModuleLogicInfo2, 0, sizeof(securityModuleLogicInfo2));
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
        }
        else if (PbfTpmAttributes.tpm12 || !PbfTpmAttributes.tpmInOperationalMode)
        {
            // TPM1.2
            sSecurityModuleLogicInfo_d securityModuleLogicInfo;
            Platform_MemorySet(&securityModuleLogicInfo, 0, sizeof(securityModuleLogicInfo));
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
 *  @brief      Function to read the field upgrade counter for an update to the same version
 *  @details    This function obtains the field upgrade counter value for an update to the same version from the TPM (only TPM2.0 based firmware update).
 *
 *  @param      PpunUpgradeCounterSelf      Pointer to the field upgrade counter (same version) parameter.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetTpm20FieldUpgradeCounterSelf(
    _Out_   unsigned int*           PpunUpgradeCounterSelf)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check parameter
        if (NULL == PpunUpgradeCounterSelf)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpunUpgradeCounterSelf is NULL)");
            break;
        }

        // Initialize output parameter
        *PpunUpgradeCounterSelf = 0;

        // Read update counter (same version)
        BYTE bMoreData = 0;
        TSS_TPMS_VENDOR_CAPABILITY_DATA vendorCapabilityData;
        Platform_MemorySet(&vendorCapabilityData, 0, sizeof(vendorCapabilityData));

        // Get TPM_PT_VENDOR_FIX_FU_COUNTER_SAME
        unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_COUNTER_SAME, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
        if (TSS_TPM_RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_COUNTER_SAME)");
            break;
        }

        // Check count of capabilities returned
        if (1 != vendorCapabilityData.data.vendorData.count)
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned more than one capability");
            break;
        }

        // Get pointer to first element in returned max buffer array (TPM_PT_VENDOR_FIX_FU_COUNTER_SAME)
        TSS_TPM2B_MAX_BUFFER* pMaxBuffer = &vendorCapabilityData.data.vendorData.buffer[0];

        // Check buffer size
        if (sizeof(TSS_UINT16) != pMaxBuffer->size)
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned wrong buffer size of capability");
            break;
        }

        // Get value
        BYTE* pbBuffer = pMaxBuffer->buffer;
        TSS_INT32 nSizeRemaining = pMaxBuffer->size;
        TSS_UINT16 upgradeCounterSelf = 0;
        unReturnValue = TSS_UINT16_Unmarshal(&upgradeCounterSelf, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Return upgrade counter (same version)
        *PpunUpgradeCounterSelf = upgradeCounterSelf;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Function to read the firmware version by using the according vendor capability.
 *  @details    This function obtains the firmware version by using the according vendor capability (only TPM2.0 based firmware update).
 *
 *  @param      PunCapProperty         GetCapability property value used for TSS_TPM_CAP_VENDOR_PROPERTY.
 *  @param      PpFirmwareVersion      Pointer to store TPM firmware version.

 *  @retval     RC_SUCCESS             The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER     An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     RC_E_FAIL              An unexpected error occurred.
 *  @retval     ...                    Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetTpmFirmwareVersionByVendorCap(
    _In_    TSS_UINT32            PunCapProperty,
    _Inout_ TPM_FIRMWARE_VERSION* PpFirmwareVersion)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check parameters
        if (NULL == PpFirmwareVersion)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpFirmwareVersion is NULL)");
            break;
        }

        BYTE bMoreData = 0;
        TSS_TPMS_VENDOR_CAPABILITY_DATA vendorCapabilityData;
        Platform_MemorySet(&vendorCapabilityData, 0, sizeof(vendorCapabilityData));

        // Get TPM2.0 current firmware version
        unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, PunCapProperty, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
        if (RC_SUCCESS != unReturnValue)
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE_FMT(unReturnValue, L"TSS_TPM2_GetCapability returned an unexpected value. (TSS_TPM_CAP_VENDOR_PROPERTY / Property=%d)", PunCapProperty);
            break;
        }

        // Check count of capabilities returned
        if (1 != vendorCapabilityData.data.vendorData.count)
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned more than one capability");
            break;
        }

        // Get pointer to first element in returned max buffer array (TPM_PT_VENDOR_FIX_OPERATION_MODE)
        TSS_TPM2B_MAX_BUFFER* pMaxBuffer = &vendorCapabilityData.data.vendorData.buffer[0];

        // Check buffer size
        if (9 != pMaxBuffer->size)
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned wrong buffer size of capability");
            break;
        }

        // Unmarshal FU firmware version
        TSS_INT32 nSizeRemaining = pMaxBuffer->size;
        TSS_BYTE* pbBuffer = pMaxBuffer->buffer;
        unReturnValue = TSS_UINT16_Unmarshal(&PpFirmwareVersion->usMajor, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;

        unReturnValue = TSS_UINT16_Unmarshal(&PpFirmwareVersion->usMinor, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;

        TSS_UINT32 unBuild;
        unReturnValue = TSS_UINT32_Unmarshal(&unBuild, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Cast build number
        PpFirmwareVersion->usBuild = (TSS_UINT16)unBuild;
        // Set Revision: if no bit is set return not certified (2), if one bit is set return certified (0)
        PpFirmwareVersion->usRevision = (0 == pMaxBuffer->buffer[8]) ? 2 : 0;
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Function to read the firmware version from the TPM
 *  @details    This function obtains the firmware version from the TPM.
 *
 *  @param      PbfTpmAttributes            TPM state attributes.
 *  @param      PpFirmwareVersion           TPM firmware version.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. The parameter is NULL or the output buffer is too small.
 *  @retval     RC_E_FAIL                   An unexpected error occurred. E.g. more than one property returned from TPM2_GetCapability call
 *                                          or unknown BITFIELD_TPM_ATTRIBUTES
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetTpmFirmwareVersion(
    _In_    BITFIELD_TPM_ATTRIBUTES PbfTpmAttributes,
    _Out_   TPM_FIRMWARE_VERSION*   PpFirmwareVersion)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check parameters
        if (NULL == PpFirmwareVersion)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpFirmwareVersion is NULL)");
            break;
        }

        Platform_MemorySet(PpFirmwareVersion, 0, sizeof(TPM_FIRMWARE_VERSION));

        if (PbfTpmAttributes.tpm20)
        {
            // Read version from TPM2.0
            BYTE bMoreData = 0;
            TSS_TPMS_CAPABILITY_DATA capabilityData;
            unsigned int unFirmwareVersion1 = 0;
            unsigned int unFirmwareVersion2 = 0;
            Platform_MemorySet(&capabilityData, 0, sizeof(capabilityData));

            if (PbfTpmAttributes.tpmHasFULoader20)
            {
                // Current firmware version can be retrieved via vendor capability
                unReturnValue = FirmwareUpdate_GetTpmFirmwareVersionByVendorCap(TPM_PT_VENDOR_FIX_FU_CURRENT_TPM_FW_VERSION, PpFirmwareVersion);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"GetTpmFirmwareVersionByVendorCap returned an unexpected value.");
                    break;
                }
            }
            else {
                if (PbfTpmAttributes.tpm20InFailureMode || PbfTpmAttributes.tpm20restartRequired)
                {
                    // In tpm20InFailureMode and tpm20restartRequired mode the capabilities must be read individually.
                    // Get TSS_TPM_PT_FIRMWARE_VERSION_1.
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

                    // Get TSS_TPM_PT_FIRMWARE_VERSION_2.
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
                }
                else
                {
                    // In other TPM2.0 states the TPM allows to get both capabilities in one TPM command. This is faster.
                    // Get TSS_TPM_PT_FIRMWARE_VERSION_1 and TSS_TPM_PT_FIRMWARE_VERSION_2.
                    unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_TPM_PROPERTIES, TSS_TPM_PT_FIRMWARE_VERSION_1, 2, &bMoreData, &capabilityData);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned an unexpected value.(TPM_CAP_TPM_PROPERTIES,TPM_PT_FIRMWARE_VERSION_1)");
                        break;
                    }

                    // Check count of capabilities returned
                    if (2 != capabilityData.data.tpmProperties.count)
                    {
                        unReturnValue = RC_E_FAIL;
                        ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability did not return two capabilities");
                        break;
                    }
                    unFirmwareVersion1 = capabilityData.data.tpmProperties.tpmProperty[0].value;
                    unFirmwareVersion2 = capabilityData.data.tpmProperties.tpmProperty[1].value;
                }

                PpFirmwareVersion->usMajor = unFirmwareVersion1 >> 16;
                PpFirmwareVersion->usMinor = unFirmwareVersion1 & 0xFFFF;
                PpFirmwareVersion->usBuild = (UINT16)((unFirmwareVersion2 & 0xFFFF00) >> 8);
                PpFirmwareVersion->usRevision = unFirmwareVersion2 & 0xFF;
            }
        }
        else if (PbfTpmAttributes.tpm12 || !PbfTpmAttributes.tpmInOperationalMode)
        {
            // TPM1.2
            TSS_TPM_CAP_VERSION_INFO sTpmVersionInfo;
            unsigned int unTpmVersionInfoSize = sizeof(sTpmVersionInfo);
            Platform_MemorySet(&sTpmVersionInfo, 0, sizeof(sTpmVersionInfo));

            unReturnValue = TSS_TPM_GetCapability(TSS_TPM_CAP_VERSION_VAL, 0, NULL, &unTpmVersionInfoSize, (BYTE*)&sTpmVersionInfo);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned an unexpected value.(TSS_TPM_CAP_VERSION_VAL,0)");
                break;
            }

            PpFirmwareVersion->usMajor = sTpmVersionInfo.version.revMajor;
            PpFirmwareVersion->usMinor = sTpmVersionInfo.version.revMinor;
            if (PbfTpmAttributes.tpmInOperationalMode)
            {
                PpFirmwareVersion->usBuild = (sTpmVersionInfo.vendorSpecific[2] << 8) + sTpmVersionInfo.vendorSpecific[3];
                PpFirmwareVersion->usRevision = sTpmVersionInfo.vendorSpecific[4];
            }
        }
        else
        {
            // Unknown TPM mode
            unReturnValue = RC_E_FAIL;
            ERROR_STORE_FMT(unReturnValue, L"Unknown TPM state attributes detected. 0x%.8X", PbfTpmAttributes);
            break;
        }
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Function to read the firmware version string from the TPM
 *  @details    This function obtains the firmware version string from the TPM.
 *
 *  @param      PbfTpmAttributes                TPM state attributes.
 *  @param      PwszFirmwareVersion             Wide character string output buffer for the firmware version (must be allocated by the caller).
 *  @param      PpunFirmwareVersionSize         In: Capacity of the wide character string including the zero termination
 *                                              Out: String length without the zero termination
 *  @param      PwszFirmwareVersionShort        Wide character string output buffer for the firmware version without subversion.minor (must be allocated by the caller).
 *  @param      PpunFirmwareVersionShortSize    In: Capacity of the wide character string including the zero termination
 *                                              Out: String length without the zero termination
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. The parameter is NULL or the output buffer is too small.
 *  @retval     RC_E_FAIL                   An unexpected error occurred. E.g. more than one property returned from TPM2_GetCapability call
 *                                          or unknown BITFIELD_TPM_ATTRIBUTES
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetTpmFirmwareVersionString(
    _In_                                            BITFIELD_TPM_ATTRIBUTES PbfTpmAttributes,
    _Out_bytecap_(*PpunFirmwareVersionSize)         wchar_t*                PwszFirmwareVersion,
    _Inout_                                         unsigned int*           PpunFirmwareVersionSize,
    _Out_bytecap_(*PpunFirmwareVersionShortSize)    wchar_t*                PwszFirmwareVersionShort,
    _Inout_                                         unsigned int*           PpunFirmwareVersionShortSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        TPM_FIRMWARE_VERSION sFirmwareVersion;

        Platform_MemorySet(&sFirmwareVersion, 0, sizeof(sFirmwareVersion));

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

        unReturnValue = FirmwareUpdate_GetTpmFirmwareVersion(PbfTpmAttributes, &sFirmwareVersion);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"FirmwareUpdate_GetTpmFirmwareVersion returned an unexpected value.");
            break;
        }

        // Firmware version without subversion.minor
        unReturnValue = Platform_StringFormat(PwszFirmwareVersionShort, PpunFirmwareVersionShortSize, L"%d.%d.%d", sFirmwareVersion.usMajor, sFirmwareVersion.usMinor, sFirmwareVersion.usBuild);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
            break;
        }

        // Firmware version with subversion.minor
        unReturnValue = Platform_StringFormat(PwszFirmwareVersion, PpunFirmwareVersionSize, L"%d.%d.%d.%d", sFirmwareVersion.usMajor, sFirmwareVersion.usMinor, sFirmwareVersion.usBuild, sFirmwareVersion.usRevision);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_StringFormat returned an unexpected value.");
            break;
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Function to read the key group ID
 *  @details    This function obtains the key group ID value from the TPM (only TPM2.0 based firmware update).
 *
 *  @param      PpunKeyGroupId              Variable to store key group ID.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetTpmKeyGroupId(
    _Out_   unsigned int* PpunKeyGroupId)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check parameter
        if (NULL == PpunKeyGroupId)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpunUpgradeCounterSelf is NULL)");
            break;
        }

        // Initialize output parameter
        *PpunKeyGroupId = 0;

        // Read Keygroup ID
        BYTE bMoreData = 0;
        TSS_TPMS_VENDOR_CAPABILITY_DATA vendorCapabilityData;
        Platform_MemorySet(&vendorCapabilityData, 0, sizeof(vendorCapabilityData));

        // Get TPM_PT_VENDOR_FIX_FU_KEYGROUP_ID
        unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_KEYGROUP_ID, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
        if (TSS_TPM_RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_KEYGROUP_ID)");
            break;
        }

        // Check count of capabilities returned
        if (1 != vendorCapabilityData.data.vendorData.count)
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned more than one capability");
            break;
        }

        // Get pointer to first element in returned max buffer array (TPM_PT_VENDOR_FIX_FU_KEYGROUP_ID)
        TSS_TPM2B_MAX_BUFFER* pMaxBuffer = &vendorCapabilityData.data.vendorData.buffer[0];

        // Check buffer size
        if (sizeof(TSS_UINT32) != pMaxBuffer->size)
        {
            unReturnValue = RC_E_FAIL;
            ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned wrong buffer size of capability");
            break;
        }

        // Get value
        BYTE* pbBuffer = pMaxBuffer->buffer;
        TSS_INT32 nSizeRemaining = pMaxBuffer->size;
        TSS_UINT32 keygroupId = 0;
        unReturnValue = TSS_UINT32_Unmarshal(&keygroupId, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Return Keygroup ID
        *PpunKeyGroupId = keygroupId;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Function to select the correct manifest
 *  @details    This function selects the correct manifest utilizing the key group ID value from the TPM (only TPM2.0 based firmware update).
 *
 *  @param      PpsFirmwareImage              Pointer with firmware image structure. On success will set rgbPolicyParameterBlock and usPolicyParameterBlockSize.
 *
 *  @retval     RC_SUCCESS                    The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER            An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     RC_E_CORRUPT_FW_IMAGE         The firmware image does not have a valid manifest count.
 *  @retval     RC_E_BUFFER_TOO_SMALL         The firmware image does not have valid manifest information (not enough information).
 *  @retval     RC_E_NEWER_FW_IMAGE_REQUIRED  The firmware image does not have a matching manifest.
 *  @retval     RC_E_FAIL                     An unexpected error occurred.
 *  @retval     ...                           Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_SelectManifest(
    _In_  IfxFirmwareImage* PpsFirmwareImage)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check parameter
        if (NULL == PpsFirmwareImage)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpsFirmwareImage is NULL)");
            break;
        }

        // Get key group id
        unReturnValue = FirmwareUpdate_GetTpmKeyGroupId(&PpsFirmwareImage->unKeyGroupId);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Get pointer and size of manifest data
        BYTE* rgbBuffer = PpsFirmwareImage->rgbManifestData;
        TSS_INT32 nBufferSize = PpsFirmwareImage->unManifestDataSize;

        // Unmarshal number of included manifests
        unsigned short usManifestCount = 0;
        unReturnValue = TSS_UINT16_Unmarshal(&usManifestCount, &rgbBuffer, &nBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Check if at least one manifest is included in firmware image
        if (usManifestCount < 1)
        {
            unReturnValue = RC_E_CORRUPT_FW_IMAGE;
            break;
        }

        // Find the correct manifest with the corresponding key group id
        {
            TSS_UINT32 unKeyGroupId = 0;
            TSS_UINT16 usParameterBlockSize = 0;

            // Loop through all manifests to find the manifest for the correct key group
            for (unsigned short usCount = 1; usCount <= usManifestCount; usCount++)
            {
                // Unmarshal key group ID
                unReturnValue = TSS_UINT32_Unmarshal(&unKeyGroupId, &rgbBuffer, &nBufferSize);
                if (RC_SUCCESS != unReturnValue)
                    break;

                // Unmarshal usParameterBlockSize
                unReturnValue = TSS_UINT16_Unmarshal(&usParameterBlockSize, &rgbBuffer, &nBufferSize);
                if (RC_SUCCESS != unReturnValue)
                    break;

                // Check length and set pointer for rgbParameterBlock
                if (usParameterBlockSize > nBufferSize)
                {
                    unReturnValue = RC_E_BUFFER_TOO_SMALL;
                    break;
                }

                // Manifest with correct key group id found?
                if (PpsFirmwareImage->unKeyGroupId == unKeyGroupId)
                {
                    PpsFirmwareImage->usPolicyParameterBlockSize = usParameterBlockSize;
                    PpsFirmwareImage->rgbPolicyParameterBlock = rgbBuffer;
                    break;
                }

                rgbBuffer += usParameterBlockSize;
                nBufferSize -= usParameterBlockSize;
            }

            // Newer revision of firmware image required?
            if (PpsFirmwareImage->unKeyGroupId != unKeyGroupId)
            {
                unReturnValue = RC_E_NEWER_FW_IMAGE_REQUIRED;
                break;
            }
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Function to check the firmware image manifest
 *  @details    This function checks the manifest utilizing the key group ID value from the TPM and validates
                in non-operational mode if the start hash is equal (only TPM2.0 based firmware update).
 *
 *  @param      PbfTpmAttributes        TPM state attributes.
 *  @param      PpsFirmwareImage        Pointer to the unmarshalled firmware image structure (Unmarshalled PrgbFirmwareImage).
 *
 *  @retval     RC_SUCCESS                      The operation completed successfully.
 *  @retval     RC_E_FAIL                       An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER              In case of a NULL input parameter.
 *  @retval     RC_E_WRONG_FW_IMAGE             In case the TPM is not updatable with the given image.
 *  @retval     RC_E_NEWER_FW_IMAGE_REQUIRED    In case the TPM2.0 does not have a key group id matching to the firmware image.
 *  @retval     ...                             Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_CheckManifest(
    _In_ BITFIELD_TPM_ATTRIBUTES    PbfTpmAttributes,
    _In_ IfxFirmwareImage*          PpsFirmwareImage)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check parameter
        if (NULL == PpsFirmwareImage)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpsFirmwareImage is NULL)");
            break;
        }

        unReturnValue = FirmwareUpdate_SelectManifest(PpsFirmwareImage);
        if (RC_SUCCESS != unReturnValue)
        {
            if (RC_E_NEWER_FW_IMAGE_REQUIRED == unReturnValue)
            {
                ERROR_STORE(RC_E_NEWER_FW_IMAGE_REQUIRED, L"The firmware image does not have a suitable manifest");
                unReturnValue = RC_E_NEWER_FW_IMAGE_REQUIRED;
            }
            else
                ERROR_STORE(unReturnValue, L"FirmwareUpdate_SelectManifest returned an unexpected value");

            break;
        }

        if (!PbfTpmAttributes.tpmInOperationalMode)
        {
            // Get manifest hash from TPM
            BYTE bMoreData = 0;
            TSS_TPMS_VENDOR_CAPABILITY_DATA vendorCapabilityData;
            Platform_MemorySet(&vendorCapabilityData, 0, sizeof(vendorCapabilityData));

            // Get TPM_PT_VENDOR_FIX_FU_START_HASH_DIGEST
            unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_START_HASH_DIGEST, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
            if (TSS_TPM_RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_START_HASH_DIGEST)");
                break;
            }

            TSS_TPM2B_MAX_BUFFER* pMaxBuffer = &vendorCapabilityData.data.vendorData.buffer[0];
            BYTE* pbBuffer = pMaxBuffer->buffer;
            TSS_INT32 nSize = pMaxBuffer->size;

            // Unmarshal TPMT_HA structure
            TSS_TPMT_HA manifestHashInfo;
            unReturnValue = TSS_TPMT_HA_Unmarshal(&manifestHashInfo, &pbBuffer, &nSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Calculate manifest hash based on return hash algorithm
            BYTE rgbManifestHash[TSS_MAX_DIGEST_BUFFER];
            Platform_MemorySet(rgbManifestHash, 0, sizeof(rgbManifestHash));

            UINT32 unDigestSize = 0;
            BYTE* pbTpmFuStartHash = NULL;
            switch (manifestHashInfo.hashAlg)
            {
                case TSS_TPM_ALG_SHA384:
                    pbTpmFuStartHash = manifestHashInfo.digest.sha384;
                    unDigestSize = TSS_SHA384_DIGEST_SIZE;
                    unReturnValue = Crypt_SHA384(PpsFirmwareImage->rgbPolicyParameterBlock, PpsFirmwareImage->usPolicyParameterBlockSize, rgbManifestHash);
                    break;

                case TSS_TPM_ALG_SHA512:
                    pbTpmFuStartHash = manifestHashInfo.digest.sha512;
                    unDigestSize = TSS_SHA512_DIGEST_SIZE;
                    unReturnValue = Crypt_SHA512(PpsFirmwareImage->rgbPolicyParameterBlock, PpsFirmwareImage->usPolicyParameterBlockSize, rgbManifestHash);
                    break;

                default:
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, L"Unsupported hash algorithm set (%d)", manifestHashInfo.hashAlg);
                    break;
            }

            // Check if hashing of manifest succeeded
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE_FMT(RC_E_FAIL, L"Manifest hash creation failed (%x)", unReturnValue);
                unReturnValue = RC_E_FAIL;
                break;
            }

            // Check if manifest hash is equal to start hash of the TPM
            if (0 != Platform_MemoryCompare(pbTpmFuStartHash, rgbManifestHash, unDigestSize))
            {
                ERROR_STORE(RC_E_WRONG_FW_IMAGE, L"Manifest hash of image does not match to start hash of TPM");
                unReturnValue = RC_E_WRONG_FW_IMAGE;
                break;
            }
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Function to check if the TPM is updatable with the given firmware image
 *  @details    Some parameters like GUID, file content signature, TPM firmware major minor version or file content CRC
 *              are checked to get a decision if the firmware is updatable with the current image.
 *
 *  @param      PbfTpmAttributes            TPM state attributes.
 *  @param      PrgbFirmwareImage           Pointer to the firmware image byte stream.
 *  @param      PnFirmwareImageSize         Size of the firmware image byte stream.
 *  @param      PpsFirmwareImage            Pointer to the unmarshalled firmware image structure (Unmarshalled PrgbFirmwareImage).
 *  @param      PpfValid                    TRUE in case the image is valid, FALSE otherwise.
 *  @param      PpbfNewTpmFirmwareInfo      Pointer to a bit field to return info data for the new firmware image.
 *  @param      PpunErrorDetails            Pointer to an unsigned int to return error details. Possible values are:\n
 *                                              RC_E_FW_UPDATE_BLOCKED in case the field upgrade counter value has been exceeded.\n
 *                                              RC_E_WRONG_FW_IMAGE in case the TPM is not updatable with the given image.\n
 *                                              RC_E_CORRUPT_FW_IMAGE in case the firmware image is corrupt.\n
 *                                              RC_E_NEWER_TOOL_REQUIRED in case a newer version of the tool is required to parse the firmware image.\n
 *                                              RC_E_WRONG_DECRYPT_KEYS in case the TPM2.0 does not have decrypt keys matching to the firmware image.\n
 *                                              RC_E_NEWER_FW_IMAGE_REQUIRED in case the TPM2.0 does not have a key group id matching to the firmware image.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER          In case of a NULL input parameter.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_IsFirmwareUpdatable(
    _In_                                BITFIELD_TPM_ATTRIBUTES         PbfTpmAttributes,
    _In_bytecount_(PnFirmwareImageSize) BYTE*                           PrgbFirmwareImage,
    _In_                                int                             PnFirmwareImageSize,
    _In_                                IfxFirmwareImage*               PpsFirmwareImage,
    _Out_                               BOOL*                           PpfValid,
    _Out_                               BITFIELD_NEW_TPM_FIRMWARE_INFO* PpbfNewTpmFirmwareInfo,
    _Out_                               unsigned int*                   PpunErrorDetails)
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
        Platform_MemorySet(PpbfNewTpmFirmwareInfo, 0, sizeof(BITFIELD_NEW_TPM_FIRMWARE_INFO));

        // Check _In_ parameters.
        if (NULL == PrgbFirmwareImage ||
                0 >= PnFirmwareImageSize ||
                NULL == PpsFirmwareImage)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PrgbFirmwareImage or PpsFirmwareImage is NULL or PnFirmwareImageSize <= 0)");
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
            BYTE rgbHash[TSS_SHA256_DIGEST_SIZE];
            // The signature is 256 bytes long and is located before the CRC
            int nSizeOfDataForHash = PnFirmwareImageSize - sizeof(PpsFirmwareImage->unChecksum) - sizeof(s_rgPublicKeys[0].rgbPublicKey);
            Platform_MemorySet(rgbHash, 0, sizeof(rgbHash));

            // Check structure version of the firmware image file for V1 images.
            if (FIRMWARE_IMAGE_V1 == PpsFirmwareImage->bImageVersion && PpsFirmwareImage->usImageStructureVersion < 2)
            {
                ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"The structure of the firmware image file is too old and therefore does not meet the minimum requirements");
                unReturnValue = RC_SUCCESS;
                *PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
                break;
            }

            // Check if signature key ID is known
            {
                unsigned int unIndexKey = 0;
                unsigned int unIndex = 0;
                BOOL fKeyIdKnown = FALSE;
                for (unIndex = 0; unIndex < RG_LEN(s_rgPublicKeys); unIndex++)
                {
                    if (s_rgPublicKeys[unIndex].usIdentifier == PpsFirmwareImage->usSignatureKeyId)
                    {
                        fKeyIdKnown = TRUE;
                        unIndexKey = unIndex;
                    }
                }

                if (!fKeyIdKnown)
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
                unReturnValue = Crypt_VerifySignature(rgbHash, sizeof(rgbHash), PpsFirmwareImage->rgbSignature, sizeof(PpsFirmwareImage->rgbSignature), s_rgPublicKeys[unIndexKey].rgbPublicKey, 256);
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

        // Verify manifest information and set policy parameter block (TPM2.0 firmware loader)
        if (NULL != PpsFirmwareImage->rgbManifestData)
        {
            // Check that TPM2.0 firmware loader is running
            if (!PbfTpmAttributes.tpmHasFULoader20)
            {
                ERROR_STORE(RC_E_WRONG_FW_IMAGE, L"The TPM is not updatable with the given image");
                unReturnValue = RC_SUCCESS;
                *PpunErrorDetails = RC_E_WRONG_FW_IMAGE;
                break;
            }
        }
        // Run checks on policy parameter block
        else
        {
            // Check that correct firmware loader is running
            if (PbfTpmAttributes.tpmHasFULoader20)
            {
                ERROR_STORE(RC_E_WRONG_FW_IMAGE, L"The TPM is not updatable with the given image");
                unReturnValue = RC_SUCCESS;
                *PpunErrorDetails = RC_E_WRONG_FW_IMAGE;
                break;
            }

            BYTE* rgbPolicyParameterBlock = NULL;
            TSS_INT32 nPolicyParameterBlockSize = 0;
            sSignedData_d sSignedData;
            Platform_MemorySet(&sSignedData, 0, sizeof(sSignedData));

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
                BYTE rgbMessageDigest[TSS_SHA256_DIGEST_SIZE];
                Platform_MemorySet(rgbMessageDigest, 0, sizeof(rgbMessageDigest));
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
                sSecurityModuleLogicInfo2_d sSecurityModuleLogicInfo2;
                unsigned int unIndex = 0;
                BOOL fDecryptKeyValid = FALSE;
                Platform_MemorySet(&sSecurityModuleLogicInfo2, 0, sizeof(sSecurityModuleLogicInfo2));

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

        // Check if the field upgrade counters allow an upgrade
        {
            unsigned int unFieldUpgradeCounter = 0;
            unReturnValue = FirmwareUpdate_GetTpmFieldUpgradeCounter(PbfTpmAttributes, &unFieldUpgradeCounter);
            if (RC_SUCCESS != unReturnValue)
                break;

            if (0 == unFieldUpgradeCounter)
            {
                *PpunErrorDetails = RC_E_FW_UPDATE_BLOCKED;
                ERROR_STORE(*PpunErrorDetails, L"Update counter is zero.");
                unReturnValue = RC_SUCCESS;
                break;
            }

            // Update counter self available?
            if (PbfTpmAttributes.tpmHasFULoader20)
            {
                // Check if the field upgrade counter on the same version allows an upgrade
                unsigned int unFieldUpgradeCounterSelf = 0;
                unReturnValue = FirmwareUpdate_GetTpm20FieldUpgradeCounterSelf(&unFieldUpgradeCounterSelf);
                if (RC_SUCCESS != unReturnValue)
                    break;

                if (0 == unFieldUpgradeCounterSelf)
                {
                    *PpunErrorDetails = RC_E_FW_UPDATE_BLOCKED;
                    ERROR_STORE(*PpunErrorDetails, L"Update counter self is zero.");
                    unReturnValue = RC_SUCCESS;
                    break;
                }
            }
        }

        // If TPM is in operational mode or has a TPM2.0 firmware loader, check the allowed source versions
        if (PbfTpmAttributes.tpmInOperationalMode || PbfTpmAttributes.tpmHasFULoader20)
        {
            wchar_t wszFirmwareVersion[MAX_NAME];
            wchar_t wszFirmwareVersionShort[MAX_NAME];
            unsigned int unFirmwareVersionSize = RG_LEN(wszFirmwareVersion);
            unsigned int unFirmwareVersionShortSize = RG_LEN(wszFirmwareVersionShort);
            unsigned int unIndex = 0;
            BOOL fImageAllowed = FALSE;
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszFirmwareVersion, RG_LEN(wszFirmwareVersion)));
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszFirmwareVersionShort, RG_LEN(wszFirmwareVersionShort)));

            // Get the firmware version from the TPM
            unReturnValue = FirmwareUpdate_GetTpmFirmwareVersionString(PbfTpmAttributes, wszFirmwareVersion, &unFirmwareVersionSize, wszFirmwareVersionShort, &unFirmwareVersionShortSize);
            if (RC_SUCCESS != unReturnValue)
                break;

            BYTE* prgwszSourceVersion = PpsFirmwareImage->prgwszSourceVersions;
            UINT32 unSourceVersionsSize = PpsFirmwareImage->usSourceVersionsSize;
            // Check if the firmware version is listed in the allowed source versions
            for (; unIndex < PpsFirmwareImage->usSourceVersionsCount; unIndex++)
            {
                wchar_t wszSourceVersion[MAX_NAME];
                unsigned int unStrLen = RG_LEN(wszSourceVersion);
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszSourceVersion, RG_LEN(wszSourceVersion)));

                // Unmarshal the binary source version blob to a string
                unReturnValue = Platform_UnmarshalString(prgwszSourceVersion, unSourceVersionsSize, &wszSourceVersion[0], &unStrLen);
                if (RC_SUCCESS != unReturnValue)
                    break;

                // Try the firmware version with subversion.minor first (e.g. 4.40.119.0)
                if (0 == Platform_StringCompare(wszFirmwareVersion, wszSourceVersion, unFirmwareVersionSize + 1, FALSE))
                {
                    fImageAllowed = TRUE;
                    break;
                }

                // If this does not match, try the firmware version without subversion.minor (e.g. 4.40.119)
                if (0 == Platform_StringCompare(wszFirmwareVersionShort, wszSourceVersion, unFirmwareVersionShortSize + 1, FALSE))
                {
                    fImageAllowed = TRUE;
                    break;
                }

                // Calculate string length with null termination (2 bytes per character)
                UINT32 unStrLen2 = (unStrLen + 1) * 2;
                // Reduce size in bytes accordingly
                unSourceVersionsSize -= unStrLen2;
                // Set pointer to next position in double null terminated string
                prgwszSourceVersion += unStrLen2;
                // Check pointer
                if (prgwszSourceVersion > (PpsFirmwareImage->prgwszSourceVersions + PpsFirmwareImage->usSourceVersionsSize))
                {
                    unReturnValue = RC_E_BUFFER_TOO_SMALL;
                    break;
                }
            }

            if (RC_SUCCESS != unReturnValue)
            {
                *PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
                ERROR_STORE_FMT(RC_E_CORRUPT_FW_IMAGE, L"Parsing source version information returned an unexpected value. (0x%.8X)", unReturnValue);
                unReturnValue = RC_SUCCESS;
                break;
            }

            if (FALSE == fImageAllowed)
            {
                *PpunErrorDetails = RC_E_WRONG_FW_IMAGE;
                unReturnValue = RC_SUCCESS;
                break;
            }

            // Perform additional checks if running on a TPM2.0 firmware loader
            if (PbfTpmAttributes.tpmHasFULoader20)
            {
                // Check if the correct manifest is included
                unReturnValue = FirmwareUpdate_CheckManifest(PbfTpmAttributes, PpsFirmwareImage);
                if (RC_SUCCESS != unReturnValue)
                {
                    if (RC_E_NEWER_FW_IMAGE_REQUIRED == unReturnValue || RC_E_WRONG_FW_IMAGE == unReturnValue)
                    {
                        *PpunErrorDetails = unReturnValue;
                        unReturnValue = RC_SUCCESS;
                    }
                    break;
                }

                // Check if an update on the same version is performed
                if (0 == Platform_StringCompare(wszFirmwareVersion, PpsFirmwareImage->wszTargetVersion, unFirmwareVersionSize + 1, FALSE))
                {
                    // Set flag to indicate a firmware update to the same version
                    PpbfNewTpmFirmwareInfo->fwUpdateSameVersion = 1;
                }

                // Check if firmware recovery is possible or ongoing
                if (PbfTpmAttributes.tpmInFwRecoveryMode)
                {
                    PpbfNewTpmFirmwareInfo->fwUpdateSameVersion = 0;
                    PpbfNewTpmFirmwareInfo->fwRecovery = 1;
                }
            }
        }
        // If the TPM is in boot loader mode and not a TPM2.0 firmware loader, a previous firmware update was interrupted.
        // Check if the currently running build number matches the allowed build number in the policy parameter block.
        // If this check fails verify if the build number matches the number in the IfxFirmwareImage structure.
        else
        {
            sSecurityModuleLogicInfo_d securityModuleLogicInfo;

            // Policy parameter block data
            unsigned int unActiveVersion = 0;
            unsigned int unIndex = 0;
            BOOL fActiveVersionVerified = FALSE;

            Platform_MemorySet(&securityModuleLogicInfo, 0, sizeof(securityModuleLogicInfo));
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

                if (unActiveVersion == 0)
                {
                    // Skip verification check if TPM does not report active firmware version.
                    fActiveVersionVerified = TRUE;
                }
                else
                {
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
                TSS_INT32 nPolicyParameterBlockSize = 0;
                sSignedData_d sSignedData;
                Platform_MemorySet(&sSignedData, 0, sizeof(sSignedData));
                // Copy pointer and size for unmarshalling
                rgbPolicyParameterBlock = PpsFirmwareImage->rgbPolicyParameterBlock;
                nPolicyParameterBlockSize = PpsFirmwareImage->usPolicyParameterBlockSize;
                // Unmarshal the block to sSignedData structure
                unReturnValue = TSS_sSignedData_d_Unmarshal(&sSignedData, &rgbPolicyParameterBlock, &nPolicyParameterBlockSize);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_CORRUPT_FW_IMAGE, L"TSS_sSignedData_d_Unmarshal returned an unexpected value. (0x%.8X)", unReturnValue);
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
 *  @brief      Returns the TPM state attributes
 *  @details
 *
 *  @param      PfCheckPlatformHierarchy    Whether to check the state of platform hierarchy. This operation can be skipped if .tpm20phDisabled and .tpm20emptyPlatformAuth are
 *                                          not needed and tool runtime should be optimized for performance.
 *  @param      PpsTpmState                 Pointer to a variable representing the TPM state.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. PpsTpmState is NULL.
 *  @retval     RC_E_FAIL                   An unexpected error occurred. E.g. more than one property returned from TPM2_GetCapability call.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_CalculateState(
    _In_    BOOL        PfCheckPlatformHierarchy,
    _Out_   TPM_STATE*  PpsTpmState)
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

        Platform_MemorySet(PpsTpmState, 0, sizeof(*PpsTpmState));

        // Try to call a TPM2_Startup command
        unReturnValue = TSS_TPM2_Startup(TSS_TPM_SU_CLEAR);
        // Remember to orderly shutdown the TPM2.0 if TPM2_Startup completed successfully.
        if (TSS_TPM_RC_SUCCESS == unReturnValue)
        {
            IGNORE_RETURN_VALUE(PropertyStorage_AddKeyBooleanValuePair(PROPERTY_CALL_SHUTDOWN_ON_EXIT, TRUE));
        }

        if (TSS_TPM_RC_SUCCESS == unReturnValue ||
                TSS_TPM_RC_INITIALIZE == (unReturnValue ^ RC_TPM_MASK) ||
                TSS_TPM_RC_FAILURE == (unReturnValue ^ RC_TPM_MASK) ||
                TSS_TPM_RC_REBOOT == (unReturnValue ^ RC_TPM_MASK))
        {
            // The TPM is a TPM2.0
            BYTE bMoreData = 0;
            TSS_TPMS_CAPABILITY_DATA capabilityData;
            Platform_MemorySet(&capabilityData, 0, sizeof(capabilityData));
            PpsTpmState->attribs.tpm20 = 1;
            PpsTpmState->attribs.tpmFirmwareIsValid = 1; // Set to initial value
            PpsTpmState->attribs.tpmInOperationalMode = 1; // Set to initial value

            // Set the failure mode flag in case the TPM2.0 is in failure mode.
            if (TSS_TPM_RC_FAILURE == (unReturnValue ^ RC_TPM_MASK))
            {
                PpsTpmState->attribs.tpm20InFailureMode = 1;
                // And get the test result
                {
                    TSS_TPM2B_MAX_BUFFER sOutData;
                    TSS_TPM_RC rcTestResult = 0;
                    Platform_MemorySet(&sOutData, 0, sizeof(sOutData));
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
            else if(TSS_TPM_RC_REBOOT == (unReturnValue ^ RC_TPM_MASK))
            {
                // The TPM has been updated to TPM2.0 but has not yet been restarted
                PpsTpmState->attribs.tpm20restartRequired = 1;
                PpsTpmState->attribs.tpmInOperationalMode = 0;
                PpsTpmState->attribs.infineon = 1; // This may not be true
            }

            unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_TPM_PROPERTIES, TSS_TPM_PT_MANUFACTURER, 1, &bMoreData, &capabilityData);
            if (TSS_TPM_RC_SUCCESS == unReturnValue && 1 == capabilityData.data.tpmProperties.count)
            {
                if (capabilityData.data.tpmProperties.tpmProperty[0].value == 0x49465800 /* IFX\0 */)
                    PpsTpmState->attribs.infineon = 1;
                else
                    PpsTpmState->attribs.infineon = 0;
            }
            else
            {
                ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned an unexpected value. (TPM_CAP_TPM_PROPERTIES,TPM_PT_MANUFACTURER)");
            }

            // Check if TPM2.0 based firmware update is supported
            if (TSS_TPM_RC_SUCCESS == unReturnValue && 1 == PpsTpmState->attribs.infineon)
            {
                bMoreData = 0;
                TSS_TPMS_VENDOR_CAPABILITY_DATA vendorCapabilityData;
                Platform_MemorySet(&vendorCapabilityData, 0, sizeof(vendorCapabilityData));

                // Get TPM2.0 operation mode property
                unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_OPERATION_MODE, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
                if (TSS_TPM_RC_SUCCESS == unReturnValue)
                {
                    // Check count of capabilities returned
                    if (1 != vendorCapabilityData.data.vendorData.count)
                    {
                        unReturnValue = RC_E_FAIL;
                        ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned more than one capability");
                        break;
                    }

                    // Get pointer to first element in returned max buffer array (TPM_PT_VENDOR_FIX_OPERATION_MODE)
                    TSS_TPM2B_MAX_BUFFER* pMaxBuffer = &vendorCapabilityData.data.vendorData.buffer[0];

                    // Check buffer size
                    if (sizeof(TSS_UINT8) != pMaxBuffer->size)
                    {
                        unReturnValue = RC_E_FAIL;
                        ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned wrong buffer size of capability");
                        break;
                    }

                    // Set TPM2.0 based firmware update
                    PpsTpmState->attribs.tpmHasFULoader20 = 1;

                    // Set TPM2.0 operation mode value
                    UINT8 bOperationMode = pMaxBuffer->buffer[0];
                    PpsTpmState->attribs.tpm20OperationMode = bOperationMode;

                    // Is TPM in non-operational mode?
                    if(OM_TPM != bOperationMode)
                    {
                        // Clear operational mode flag
                        PpsTpmState->attribs.tpmInOperationalMode = 0;

                        // Set firmware update or recovery mode
                        PpsTpmState->attribs.tpmInFwUpdateMode = (bOperationMode & 0x80) == 0x00 ? 1 : 0;
                        PpsTpmState->attribs.tpmInFwRecoveryMode = !PpsTpmState->attribs.tpmInFwUpdateMode;

                        // Is firmware valid (0x01 or 0x81)?
                        PpsTpmState->attribs.tpmFirmwareIsValid = (bOperationMode & 0x7) == 0x1 ? 1 : 0;

                        // Is firmware valid and TPM restart pending (0x04 or 0x84)?
                        if ((bOperationMode & 0x7) == 0x4)
                        {
                            PpsTpmState->attribs.tpmFirmwareIsValid = 1;
                            PpsTpmState->attribs.tpm20restartRequired = 1;
                        }

                        // Set firmware recovery supported flag if firmware recovery mode is active (info may not available if TPM restart is pending)
                        if (PpsTpmState->attribs.tpmInFwRecoveryMode && PpsTpmState->attribs.tpm20restartRequired)
                            PpsTpmState->attribs.tpmSupportsFwRecovery = 1;
                    }

                    // Get TPM_PT_VENDOR_FIX_FU_PROPERTIES
                    unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_PROPERTIES, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
                    if (TSS_TPM_RC_SUCCESS == unReturnValue)
                    {
                        // Check count of capabilities returned
                        if (1 != vendorCapabilityData.data.vendorData.count)
                        {
                            unReturnValue = RC_E_FAIL;
                            ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned more than one capability");
                            break;
                        }

                        // Get pointer to first element in returned max buffer array (TPM_PT_VENDOR_FIX_FU_PROPERTIES)
                        pMaxBuffer = &vendorCapabilityData.data.vendorData.buffer[0];

                        // Check buffer size
                        if (sizeof(TSS_UINT32) != pMaxBuffer->size)
                        {
                            unReturnValue = RC_E_FAIL;
                            ERROR_STORE(unReturnValue, L"TSS_TPM2_GetCapability returned wrong buffer size of capability");
                            break;
                        }

                        // Is firmware recovery support bit set?
                        PpsTpmState->attribs.tpmSupportsFwRecovery = pMaxBuffer->buffer[3] & TPM_FU_PROPERTIES_FW_RECOVERY_SUPPORTED;
                    }
                    else
                    {
                        // If TPM restart is pending capability may not be available
                        if (PpsTpmState->attribs.tpm20restartRequired)
                            unReturnValue = RC_SUCCESS;
                        else
                            ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_GetCapability returned an unexpected value. (TSS_TPM_CAP_VENDOR_PROPERTY,TPM_PT_VENDOR_FIX_FU_PROPERTIES)");
                    }
                }
                else
                {
                    if ((unReturnValue ^ RC_TPM_MASK) == (TSS_TPM_RC_VALUE | TSS_TPM_RC_P | TSS_TPM_RC_2) ||
                            (unReturnValue ^ RC_TPM_MASK) == TSS_TPM_RC_VALUE)
                        unReturnValue = RC_SUCCESS;
                    else
                        ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_GetCapability returned an unexpected value. (TSS_TPM_CAP_VENDOR_PROPERTY,TPM_PT_VENDOR_FIX_OPERATION_MODE)");
                }
            }

            if (TSS_TPM_RC_SUCCESS == unReturnValue &&
                    PpsTpmState->attribs.infineon == 1 && !PpsTpmState->attribs.tpm20InFailureMode &&
                    !PpsTpmState->attribs.tpm20restartRequired && PpsTpmState->attribs.tpmInOperationalMode &&
                    PfCheckPlatformHierarchy)
            {
                // Check whether platformAuth is the Empty Buffer and platform hierarchy is enabled.
                // (-> Preconditions for TPMFactoryUpd to update a TPM2.0)
                TSS_AuthorizationCommandData sAuthSessionData;
                TSS_AcknowledgmentResponseData sAckAuthSessionData;
                TSS_TPM2B_AUTH sNewAuth;
                Platform_MemorySet(&sAuthSessionData, 0, sizeof(sAuthSessionData));
                Platform_MemorySet(&sAckAuthSessionData, 0, sizeof(sAckAuthSessionData));
                Platform_MemorySet(&sNewAuth, 0, sizeof(sNewAuth));
                // Initialize authorization command data structure
                sAuthSessionData.authHandle = TSS_TPM_RS_PW;    // Use password based authorization session
                sAuthSessionData.sessionAttributes.continueSession = 1;
                unReturnValue = TSS_TPM2_HierarchyChangeAuth(TSS_TPM_RH_PLATFORM, &sAuthSessionData, &sNewAuth, &sAckAuthSessionData);
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
        else
        {
            TSS_TPM_CAP_VERSION_INFO tpmVersionInfo;
            unsigned int unTpmVersionSizeInfo = sizeof(TSS_TPM_CAP_VERSION_INFO);
            BYTE rgbIFX[] = { 'I', 'F', 'X', 0x00};
            Platform_MemorySet(&tpmVersionInfo, 0, sizeof(tpmVersionInfo));
            unReturnValue = TSS_TPM_Startup(TSS_TPM_ST_CLEAR);
            if (RC_SUCCESS == unReturnValue || TSS_TPM_INVALID_POSTINIT == (unReturnValue ^ RC_TPM_MASK))
            {
                // The TPM is a TPM1.2
                PpsTpmState->attribs.tpm12 = 1;
                PpsTpmState->attribs.tpmFirmwareIsValid = 1;
                PpsTpmState->attribs.tpmInOperationalMode = 1;

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
                            TSS_IFX_FIELDUPGRADEINFO sFieldUpgradeInfo;
                            Platform_MemorySet(&sFieldUpgradeInfo, 0, sizeof(sFieldUpgradeInfo));
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
                    TSS_TPM_PERMANENT_FLAGS sPermanentFlags;
                    TSS_TPM_STCLEAR_FLAGS sStclearFlags;
                    unsigned int unResponseSize = sizeof(sPermanentFlags);
                    Platform_MemorySet(&sPermanentFlags, 0, sizeof(sPermanentFlags));
                    Platform_MemorySet(&sStclearFlags, 0, sizeof(sStclearFlags));
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
                    // The TPM is a TPM1.2
                    PpsTpmState->attribs.tpm12 = 1;

                    if (0 == Platform_MemoryCompare(tpmVersionInfo.tpmVendorID, rgbIFX, sizeof(rgbIFX)))
                    {
                        sSecurityModuleLogicInfo_d securityModuleLogicInfo;
                        Platform_MemorySet(&securityModuleLogicInfo, 0, sizeof(securityModuleLogicInfo));
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
                            PpsTpmState->attribs.tpmInOperationalMode = 1;
                            PpsTpmState->attribs.tpmFirmwareIsValid = 1;
                            PpsTpmState->attribs.tpm12FailedSelfTest = 1;
                            PpsTpmState->attribs.tpm12FieldUpgradeInfo2Failed = 1;
                        }
                        else if (RC_SUCCESS == unReturnValue)
                        {
                            if (securityModuleLogicInfo.SecurityModuleStatus == SMS_BTLDR_ACTIVE)
                            {
                                // TPM is in firmware update mode
                                PpsTpmState->attribs.tpmInFwUpdateMode = 1;
                                PpsTpmState->attribs.tpmFirmwareIsValid = 0;
                                PpsTpmState->attribs.tpmInOperationalMode = 0;

                                // Clear TPM1.2 flag since unknown
                                PpsTpmState->attribs.tpm12 = 0;

                                // Check hardware identifier and set according flag
                                if (securityModuleLogicInfo.sSecurityModuleLogic.unTargetHardwareIdentifier == 0x00000002)
                                {
                                    PpsTpmState->attribs.tpmSlb966x = 1;
                                }
                                else if (securityModuleLogicInfo.sSecurityModuleLogic.unTargetHardwareIdentifier == 0x00000102)
                                {
                                    PpsTpmState->attribs.tpmSlb9670 = 1;
                                }
                            }
                            else
                            {
                                PpsTpmState->attribs.tpmInOperationalMode = 1;
                                PpsTpmState->attribs.tpmFirmwareIsValid = 1;
                                PpsTpmState->attribs.tpm12FailedSelfTest = 1;
                            }
                        }

                        if (PpsTpmState->attribs.tpm12FailedSelfTest)
                        {
                            BYTE rgbSelftestResult[MAX_NAME];
                            unsigned int unSelftestResultSize = RG_LEN(rgbSelftestResult);
                            Platform_MemorySet(rgbSelftestResult, 0, sizeof(rgbSelftestResult));
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
 *  @brief      FirmwareUpdate start for TPM2.0.
 *  @details    The function takes the firmware update policy parameter block and the stored policy session and starts the
 *              firmware update process.
 *
 *  @param      PbfTpmAttributes                The operation mode of the TPM.
 *  @param      PusPolicyParameterBlockSize     Size of the policy parameter block.
 *  @param      PrgbPolicyParameterBlock        Pointer to the policy parameter block byte stream.
 *  @param      PunSessionHandle                Session handle.
 *  @param      PfnProgress                     Callback function to indicate the progress.
 *
 *  @retval     RC_SUCCESS                              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER                      An invalid parameter was passed to the function. Policy parameter block stream is NULL.
 *  @retval     RC_E_TPM20_INVALID_POLICY_SESSION       The policy session handle is 0 or invalid or policy authorization failed.
 *  @retval     RC_E_TPM20_POLICY_HANDLE_OUT_OF_RANGE   The policy handle value is out of range.
 *  @retval     RC_E_TPM20_POLICY_SESSION_NOT_LOADED    The policy session is not loaded to the TPM.
 *  @retval     RC_E_FIRMWARE_UPDATE_FAILED             Firmware update started but failed.
 *  @retval     RC_E_FAIL                               An unexpected error occurred.
 */
_Check_return_
unsigned int
FirmwareUpdate_Start_Tpm20(
    _In_                                        BITFIELD_TPM_ATTRIBUTES             PbfTpmAttributes,
    _In_bytecount_(PusPolicyParameterBlockSize) BYTE*                               PrgbPolicyParameterBlock,
    _In_                                        UINT16                              PusPolicyParameterBlockSize,
    _In_                                        unsigned int                        PunSessionHandle,
    _In_                                        PFN_FIRMWAREUPDATE_PROGRESSCALLBACK PfnProgress)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Authorization session area
        TSS_AuthorizationCommandData sAuthSessionData;
        TSS_AcknowledgmentResponseData sAckAuthSessionData;

        // Policy parameter block data
        TSS_TPM2B_MAX_BUFFER sData;
        TSS_UINT8 bSubCommand;

        // Out parameter
        unsigned short usStartSize = 0;

        Platform_MemorySet(&sAuthSessionData, 0, sizeof(sAuthSessionData));
        Platform_MemorySet(&sAckAuthSessionData, 0, sizeof(sAckAuthSessionData));
        Platform_MemorySet(&sData, 0, sizeof(sData));

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

        if (PbfTpmAttributes.tpmHasFULoader20)
        {
            TSS_TPMT_HA sHash;
            TSS_INT32 nMarshal = sizeof(sData.buffer);
            TSS_BYTE* pMarshal = sData.buffer;

            // Set the sub-command.
            bSubCommand = TPM20_FieldUpgradeStartManifestHash;

            // Calculate SHA-512 hash over the manifest.
            unReturnValue = Crypt_SHA512(PrgbPolicyParameterBlock, PusPolicyParameterBlockSize, sHash.digest.sha512);
            if (RC_SUCCESS != unReturnValue)
            {
                unReturnValue = RC_E_FAIL;
                break;
            }
            sHash.hashAlg = TSS_TPM_ALG_SHA512;

            // Marshal the hash as TPMT_HA structure into TSS_TPM2B_MAX_BUFFER.
            unReturnValue = TSS_TPMT_HA_Marshal(&sHash, &pMarshal, &nMarshal);
            if (RC_SUCCESS != unReturnValue)
            {
                unReturnValue = RC_E_FAIL;
                break;
            }

            // Update the size of the TSS_TPM2B_MAX_BUFFER.
            sData.size = (TSS_UINT16)(sizeof(sData.buffer) - nMarshal);
        }
        else
        {
            // Set the sub-command.
            bSubCommand = TPM20_FieldUpgradeStart;

            // Copy the block to TSS_TPM2B_MAX_BUFFER.
            unReturnValue = Platform_MemoryCopy(sData.buffer, sizeof(sData.buffer), PrgbPolicyParameterBlock, PusPolicyParameterBlockSize);
            if (RC_SUCCESS != unReturnValue)
            {
                unReturnValue = RC_E_FAIL;
                break;
            }
            sData.size = PusPolicyParameterBlockSize;
        }

        // Initialize authorization command data structure
        sAuthSessionData.authHandle = PunSessionHandle;
        sAuthSessionData.sessionAttributes.continueSession = 1;

        // Call TPM2_FieldUpgradeStartVendor command
        unReturnValue = TSS_TPM2_FieldUpgradeStartVendor(TSS_TPM_RH_PLATFORM, &sAuthSessionData, bSubCommand, &sData, &usStartSize, &sAckAuthSessionData);
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
            ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM2_FieldUpgradeStartVendor returned an unexpected value.(0x%.8X)", unReturnValue);
            unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
            break;
        }

        // The TPM switches to boot loader mode, so there is no more need to call TPM2_Shutdown to prevent unorderly shutdowns.
        if (PropertyStorage_ExistsElement(PROPERTY_CALL_SHUTDOWN_ON_EXIT))
        {
            IGNORE_RETURN_VALUE_BOOL(PropertyStorage_RemoveElement(PROPERTY_CALL_SHUTDOWN_ON_EXIT));
        }

        // Set Progress to 0% after start vendor
        PfnProgress(0);

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      FirmwareUpdate start for TPM1.2.
 *  @details    The function takes the firmware update policy parameter block and starts the firmware update process.
 *
 *  @param      PbfTpmAttributes                The operation mode of the TPM.
 *  @param      PrgbPolicyParameterBlock        Pointer to the policy parameter block byte stream.
 *  @param      PusPolicyParameterBlockSize     Size of the policy parameter block.
 *  @param      PrgbOwnerAuthHash               TPM Owner authentication hash (sha1).
 *  @param      PfnProgress                     Callback function to indicate the progress.
 *
 *  @retval     RC_SUCCESS                      The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER              An invalid parameter was passed to the function. Policy parameter block stream is NULL or policy session handle is not 0.
 *  @retval     RC_E_CORRUPT_FW_IMAGE           Policy parameter block is corrupt and/or cannot be unmarshalled.
 *  @retval     RC_E_FAIL                       TPM connection or command error.
 *  @retval     RC_E_TPM12_DEFERREDPP_REQUIRED  Deferred Physical Presence has not been set (TPM1.2 only).
 *  @retval     RC_E_FIRMWARE_UPDATE_FAILED     The update operation was started but failed.
 *  @retval     RC_E_TPM12_INVALID_OWNERAUTH    TPM Owner authentication is incorrect (TPM1.2 only).
 *  @retval     RC_E_TPM12_DA_ACTIVE            The TPM Owner is locked out due to dictionary attack (TPM1.2 only).
 */
_Check_return_
unsigned int
FirmwareUpdate_Start_Tpm12(
    _In_                                        BITFIELD_TPM_ATTRIBUTES             PbfTpmAttributes,
    _In_bytecount_(PusPolicyParameterBlockSize) BYTE*                               PrgbPolicyParameterBlock,
    _In_                                        UINT16                              PusPolicyParameterBlockSize,
    _In_bytecount_(TSS_SHA1_DIGEST_SIZE)        const BYTE                          PrgbOwnerAuthHash[TSS_SHA1_DIGEST_SIZE],
    _In_                                        PFN_FIRMWAREUPDATE_PROGRESSCALLBACK PfnProgress)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        TSS_TPM_NONCE sNonceEven;
        TSS_TPM_AUTHHANDLE unAuthHandle = 0;
        BYTE* pbOwnerAuth = NULL;
        BYTE* rgbPolicyParameterBlock = NULL;
        int nPolicyParameterBlockSize = 0;

        // Policy parameter block data
        sSignedData_d sSignedData;

        Platform_MemorySet(&sNonceEven, 0, sizeof(sNonceEven));
        Platform_MemorySet(&sSignedData, 0, sizeof(sSignedData));

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
            ERROR_STORE_FMT(RC_E_CORRUPT_FW_IMAGE, L"TSS_sSignedData_d_Unmarshal returned an unexpected value. (0x%.8X)", unReturnValue);
            unReturnValue = RC_E_CORRUPT_FW_IMAGE;
            break;
        }

        if (PbfTpmAttributes.tpm12 && PbfTpmAttributes.tpm12owner)
        {
            // Get dictionary attack state for TPM_ET_OWNER and return RC_E_TPM12_DA_ACTIVE if TPM Owner is locked out.
            UINT16 usSubCap = TSS_TPM_ET_OWNER;
            TSS_TPM_DA_INFO sDaInfo;
            unsigned int unDaInfoSize = sizeof(TSS_TPM_DA_INFO);
            Platform_MemorySet(&sDaInfo, 0, sizeof(sDaInfo));
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
                ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"Error while sending the policy parameter block. (0x%.8X)", unReturnValue);
                unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
            }

            if (PbfTpmAttributes.tpm12owner)
            {
                IGNORE_RETURN_VALUE(TSS_TPM_FlushSpecific(unAuthHandle, TSS_TPM_RT_AUTH));
            }

            break;
        }

        // Set Progress to 0% after start vendor
        PfnProgress(0);

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      FirmwareUpdateProcess Update
 *  @details    The function determines the maximum data size for a firmware block and sends the firmware to the TPM
 *              in chunks of maximum data size.
 *
 *  @param      PunFirmwareBlockSize    Size of the firmware block.
 *  @param      PrgbFirmwareBlock       Pointer to the firmware block byte stream.
 *  @param      PfnProgress             Callback function to indicate the progress.
 *
 *  @retval     RC_SUCCESS                      The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER              An invalid parameter was passed to the function. The firmware image block is NULL.
 *  @retval     RC_E_FAIL                       In case of a TPM connection or command error.
 *  @retval     RC_E_FIRMWARE_UPDATE_FAILED     The update operation was started but failed.
 *  @retval     RC_E_TPM_NO_BOOT_LOADER_MODE    The TPM is not in boot loader mode.
 */
_Check_return_
unsigned int
FirmwareUpdate_Update(
    _In_                                    unsigned int                        PunFirmwareBlockSize,
    _In_bytecount_(PunFirmwareBlockSize)    BYTE*                               PrgbFirmwareBlock,
    _In_                                    PFN_FIRMWAREUPDATE_PROGRESSCALLBACK PfnProgress)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        BYTE* rgbFirmwareBlock = NULL;
        unsigned int unRemainingBytes = 0;
        unsigned int unBlockNumber = 0;
        unsigned int unCurrentProgress = 0;
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
                sSecurityModuleLogicInfo_d securityModuleLogicInfo;
                Platform_MemorySet(&securityModuleLogicInfo, 0, sizeof(securityModuleLogicInfo));
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
                ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM_FieldUpgradeUpdate returned an unexpected value while processing block %d. (0x%.8X)", unBlockNumber, unReturnValue);
                unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                break;
            }

            // Increase data pointer by block size
            rgbFirmwareBlock += usBlockSize;

            // Decrease size of remaining data by block size
            unRemainingBytes -= usBlockSize;

            // Set Progress (0 and 100% are set for _Start and _Complete so 98 steps are left)
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
 *  @brief      FirmwareUpdate Complete
 *  @details    The function finishes the firmware update process.
 *
 *  @param      PfnProgress             Callback function to indicate the progress.
 *  @retval     RC_SUCCESS                      The operation completed successfully.
 *  @retval     RC_E_FAIL                       In case of a TPM connection or command error.
 *  @retval     RC_E_FIRMWARE_UPDATE_FAILED     The update operation was started but failed.
 */
_Check_return_
unsigned int
FirmwareUpdate_Complete(
    _In_    PFN_FIRMWAREUPDATE_PROGRESSCALLBACK         PfnProgress)
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
            ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM_FieldUpgradeComplete returned an unexpected value. (0x%.8X)", unReturnValue);
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
 *  @brief      Processes the firmware update for the SLB 9672 TPM.
 *  @details    The function sends the firmware to the TPM in chunks of maximum data size. It sends
 *              the manifest first, then the firmware, and finally it calls the finalize function.
 *
 *  @param      PpsIfxFirmwareImage     Pointer to structure containing the firmware image.
 *  @param      PpsFirmwareUpdateData   Pointer to structure containing all relevant data for a firmware update.
 *
 *  @retval     RC_SUCCESS                      The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER              An invalid parameter was passed to the function. The firmware image block is NULL.
 *  @retval     RC_E_FIRMWARE_UPDATE_FAILED     The update operation was started but failed.
 *  @retval     RC_E_TPM_NO_BOOT_LOADER_MODE    The TPM is not in boot loader mode.
 */
_Check_return_
unsigned int
FirmwareUpdate_UpdateTpm20(
    _In_    const IfxFirmwareImage* const       PpsIfxFirmwareImage,
    _In_    const IfxFirmwareUpdateData* const  PpsFirmwareUpdateData)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        TSS_TPM2B_MAX_BUFFER sData;
        TSS_TPMS_VENDOR_CAPABILITY_DATA vendorCapabilityData;
        TSS_TPMI_YES_NO bMoreData = 0;
        BYTE bOperationMode = 0;
        unsigned int unRemainingBytes = 0;
        unsigned int unBlockNumber = 0;
        unsigned int unCurrentProgress = 1;

        // Check input parameters.
        if (NULL == PpsIfxFirmwareImage || NULL == PpsFirmwareUpdateData)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        BYTE* rgbFirmwareBlock = PpsIfxFirmwareImage->rgbFirmware;
        BYTE* rgbPolicyParameterBlock = PpsIfxFirmwareImage->rgbPolicyParameterBlock;

        {
            unsigned int unRetryCounter = 1;
            for (; unRetryCounter <= TPM20_FU_RETRY_COUNT; unRetryCounter++)
            {
                // Wait for TPM to switch to boot loader mode.
                Platform_Sleep(TPM20_FU_WAIT_TIME);

                // Disconnect from TPM.
                unReturnValue = DeviceManagement_Disconnect();
                if (RC_SUCCESS != unReturnValue) {
                    LOGGING_WRITE_LEVEL2_FMT(L"DeviceManagement_Disconnect failed (0x%.8X). Continue checking for boot loader mode.", unReturnValue);
                }

                // Reconnect to the TPM.
                unReturnValue = DeviceManagement_Connect();
                if (RC_SUCCESS != unReturnValue) {
                    LOGGING_WRITE_LEVEL2_FMT(L"DeviceManagement_Connect failed (0x%.8X). Continue checking for boot loader mode.", unReturnValue);
                    continue;
                }

                // Verify that TPM switched to boot loader mode.
                Platform_MemorySet(&vendorCapabilityData, 0, sizeof(vendorCapabilityData));
                unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_OPERATION_MODE, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM2_GetCapability returned an unexpected value. (0x%.8X)", unReturnValue);
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                    break;
                }

                // Check returned vendor capability data
                if (TSS_TPM_CAP_VENDOR_PROPERTY != vendorCapabilityData.capability || 1 != vendorCapabilityData.data.vendorData.count)
                {
                    ERROR_STORE(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM2_GetCapability(TPM_PT_VENDOR_FIX_FU_OPERATION_MODE) returned unexpected data.");
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                    break;
                }

                // Verify that TPM switched to boot loader mode.
                bOperationMode = vendorCapabilityData.data.vendorData.buffer[0].buffer[0];
                if (OM_TPM == bOperationMode)
                {
                    unReturnValue = RC_E_TPM_NO_BOOT_LOADER_MODE;
                    LOGGING_WRITE_LEVEL1(L"TPM is not in boot loader mode as expected.");
                    break;
                }
                else
                {
                    unReturnValue = RC_SUCCESS;
                    break;
                }
            }

            if (RC_SUCCESS != unReturnValue)
            {
                if (TPM20_FU_RETRY_COUNT == unRetryCounter)
                {
                    ERROR_STORE(RC_E_FIRMWARE_UPDATE_FAILED, L"No connection to the TPM can be established.");
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                }
                break;
            }
        }

        // Check if manifest must be send
        if (OM_FU_BEFORE_FINALIZE != bOperationMode && OM_RE_BEFORE_FINALIZE != bOperationMode)
        {
            // Send the manifest to the TPM block-by-block in chunks of <= TSS_MAX_DIGEST_BUFFER bytes (1024)
            unRemainingBytes = PpsIfxFirmwareImage->usPolicyParameterBlockSize;
            for (unBlockNumber = 1; unRemainingBytes > 0; unBlockNumber++)
            {
                UINT16 usBlockSize = unRemainingBytes < TSS_MAX_DIGEST_BUFFER ? (UINT16)unRemainingBytes : TSS_MAX_DIGEST_BUFFER;
                // Copy the firmware block to TSS_TPM2B_MAX_BUFFER.
                unReturnValue = Platform_MemoryCopy(sData.buffer, sizeof(sData.buffer), rgbPolicyParameterBlock, usBlockSize);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"Platform_MemoryCopy returned an unexpected value. (0x%.8X)", unReturnValue);
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                    break;
                }
                sData.size = usBlockSize;

                // Set intial processing info
                TSS_UINT8 processingInfo = PROCESSING_INFO_FIRST_BLOCK;
                // Consecutive block?
                if (unBlockNumber > 1)
                    processingInfo = PROCESSING_INFO_CONSECUTIVE_BLOCK;
                // Last block reached?
                if (unRemainingBytes <= TSS_MAX_DIGEST_BUFFER)
                    processingInfo = PROCESSING_INFO_LAST_BLOCK_OR_NO_CHAINING;

                // Transmit manifest block
                unReturnValue = TSS_TPM2_FieldUpgradeManifestVendor(processingInfo, &sData);
                if (RC_SUCCESS != unReturnValue)
                {
                    // Check if manifest was already send
                    if (TSS_TPM_RC_DISABLED == unReturnValue )
                    {
                        // Continue update
                        unReturnValue = RC_SUCCESS;
                        break;
                    }

                    ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM2_FieldUpgradeManifestVendor returned an unexpected value while processing block %d. (0x%.8X)", unBlockNumber, unReturnValue);
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;

                    // Get abandon mode
                    UINT32 unAbandonUpdateMode = 0;
                    if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_ABANDON_UPDATE_MODE, &unAbandonUpdateMode))
                    {
                        unReturnValue = RC_E_INTERNAL;
                        break;
                    }

                    // Check if firmware update shall be abandoned
                    if ((unAbandonUpdateMode & ABANDON_UPDATE_IF_MANIFEST_CALL_FAIL) == ABANDON_UPDATE_IF_MANIFEST_CALL_FAIL)
                    {
                        UINT32 unResultAbandonUpdate = FirmwareUpdate_AbandonUpdate();
                        if (RC_SUCCESS != unResultAbandonUpdate)
                            LOGGING_WRITE_LEVEL1_FMT(L"Unexpected error calling FirmwareUpdate_AbandonUpdate: (0x%.8lX)", unResultAbandonUpdate);
                    }

                    // Break FirmwareUpdate_UpdateTpm20 due to error within TSS_TPM2_FieldUpgradeManifestVendor
                    break;
                }

                // Increase data pointer by block size
                rgbPolicyParameterBlock += usBlockSize;

                // Decrease size of remaining data by block size
                unRemainingBytes -= usBlockSize;
            }
        }

        // Check for errors
        if (RC_SUCCESS != unReturnValue)
            break;

        // Set Progress to 1% after manifest vendor
        PpsFirmwareUpdateData->fnProgressCallback(1);

        // Check if firmware blocks must be send
        if (OM_FU_BEFORE_FINALIZE != bOperationMode && OM_RE_BEFORE_FINALIZE != bOperationMode)
        {
            // Send the firmware image to the TPM block-by-block in chunks of <= TSS_MAX_DIGEST_BUFFER bytes (1024)
            unRemainingBytes = PpsIfxFirmwareImage->unFirmwareSize;
            for (unBlockNumber = 1; unRemainingBytes > 0; unBlockNumber++)
            {
                UINT16 usBlockSize = unRemainingBytes < TSS_MAX_DIGEST_BUFFER ? (UINT16)unRemainingBytes : TSS_MAX_DIGEST_BUFFER;

                // Copy the firmware block to TSS_TPM2B_MAX_BUFFER.
                unReturnValue = Platform_MemoryCopy(sData.buffer, sizeof(sData.buffer), rgbFirmwareBlock, usBlockSize);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"Platform_MemoryCopy returned an unexpected value. (0x%.8X)", unReturnValue);
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                    break;
                }
                sData.size = usBlockSize;

                // Transmit firmware block
                unReturnValue = TSS_TPM2_FieldUpgradeDataVendor(&sData);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM2_FieldUpgradeDataVendor returned an unexpected value while processing block %d. (0x%.8X)", unBlockNumber, unReturnValue);
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                    break;
                }

                // Increase data pointer by block size
                rgbFirmwareBlock += usBlockSize;

                // Decrease size of remaining data by block size
                unRemainingBytes -= usBlockSize;

                // Set Progress (0% after _StartVendor, 1% after _ManifestVendor, 99% before _FinalizeVendor, 100% after _FinalizeVendor)
                {
                    unsigned int unProgress = (PpsIfxFirmwareImage->unFirmwareSize - unRemainingBytes) * 96 / PpsIfxFirmwareImage->unFirmwareSize + 2;
                    if (unCurrentProgress != unProgress)
                    {
                        unCurrentProgress = unProgress;
                        PpsFirmwareUpdateData->fnProgressCallback(unCurrentProgress);
                    }
                }
            }
        }

        // Check for errors
        if (RC_SUCCESS != unReturnValue)
            break;

        // Set Progress to 99%
        PpsFirmwareUpdateData->fnProgressCallback(99);

        // Finalize firmware upgrade.
        {
            unsigned int unRetryCounter = 1;
            for (; unRetryCounter <= TPM20_FU_RETRY_COUNT; unRetryCounter++)
            {
                // Wait for TPM to switch back to firmware mode.
                Platform_Sleep(TPM20_FU_WAIT_TIME);

                // Disconnect from TPM.
                unReturnValue = DeviceManagement_Disconnect();
                if (RC_SUCCESS != unReturnValue)
                {
                    LOGGING_WRITE_LEVEL2_FMT(L"DeviceManagement_Disconnect failed (0x%.8X). Continue finalizing update.", unReturnValue);
                }

                // Reconnect to the TPM.
                unReturnValue = DeviceManagement_Connect();
                if (RC_SUCCESS != unReturnValue)
                {
                    LOGGING_WRITE_LEVEL2_FMT(L"DeviceManagement_Connect failed (0x%.8X). Continue finalizing update.", unReturnValue);
                    continue;
                }

                // Verify that TPM switched to boot loader mode.
                Platform_MemorySet(&vendorCapabilityData, 0, sizeof(vendorCapabilityData));
                unReturnValue = TSS_TPM2_GetCapability(TSS_TPM_CAP_VENDOR_PROPERTY, TPM_PT_VENDOR_FIX_FU_OPERATION_MODE, 1, &bMoreData, (TSS_TPMS_CAPABILITY_DATA*)&vendorCapabilityData);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM2_GetCapability returned an unexpected value. (0x%.8X)", unReturnValue);
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                    break;
                }

                // Check returned vendor capability data
                if (TSS_TPM_CAP_VENDOR_PROPERTY != vendorCapabilityData.capability || 1 != vendorCapabilityData.data.vendorData.count)
                {
                    ERROR_STORE(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM2_GetCapability(TPM_PT_VENDOR_FIX_FU_OPERATION_MODE) returned unexpected data.");
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                    break;
                }

                bOperationMode = vendorCapabilityData.data.vendorData.buffer[0].buffer[0];
                if (OM_FU_BEFORE_FINALIZE != bOperationMode && OM_RE_BEFORE_FINALIZE != bOperationMode)
                {
                    ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TPM is in an unexpected mode (%d).", bOperationMode);
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                    break;
                }

                // Call finalize command
                sData.size = 0;
                unReturnValue = TSS_TPM2_FieldUpgradeFinalizeVendor(&sData);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_FIRMWARE_UPDATE_FAILED, L"TSS_TPM2_FieldUpgradeFinalizeVendor returned an unexpected value. (0x%.8X)", unReturnValue);
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                    break;
                }
                else {
                    LOGGING_WRITE_LEVEL3(L"TSS_TPM2_FieldUpgradeFinalizeVendor succeeded.");
                    break;
                }
            }

            if (RC_SUCCESS != unReturnValue)
            {
                if (TPM20_FU_RETRY_COUNT == unRetryCounter)
                {
                    ERROR_STORE(RC_E_FIRMWARE_UPDATE_FAILED, L"No connection to the TPM can be established.");
                    unReturnValue = RC_E_FIRMWARE_UPDATE_FAILED;
                }
                break;
            }
        }

        // Set Progress to 100%
        PpsFirmwareUpdateData->fnProgressCallback(100);
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Abandon update and switch back to TPM operational mode.
 *  @details    This function switch back to TPM operational mode by calling TPM2_FieldUpgradeAbandonVendor() command.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Return codes from TSS_TPM2_FieldUpgradeAbandonVendor().
 */
_Check_return_
unsigned int
FirmwareUpdate_AbandonUpdate()
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Initialize size with zero since parameter is not used
        TSS_TPM2B_MAX_BUFFER sData;
        sData.size = 0;

        // Call abandon command (switching back to TPM operational mode)
        unReturnValue = TSS_TPM2_FieldUpgradeAbandonVendor(&sData);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"TSS_TPM2_FieldUpgradeAbandonVendor returned an unexpected value.");
            break;
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Returns information about the current state of TPM
 *  @details
 *
 *  @param      PwszVersionName             A pointer to a null-terminated string representing the firmware image version name.
 *  @param      PpunVersionNameSize         IN: Capacity of the PwszVersionName parameter in wide characters including zero termination
 *                                          OUT: Length of written wide character string (excluding zero termination)
 *  @param      PpsTpmState                 Receives TPM state.
 *  @param      PpunRemainingUpdates        Receives the number of remaining updates or -1 in case the value could not be detected.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function.
 *  @retval     RC_E_NO_IFX_TPM             The TPM is not manufactured by Infineon.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_GetImageInfo(
    _Inout_bytecap_(*PpunVersionNameSize)   wchar_t*                    PwszVersionName,
    _Inout_                                 unsigned int*               PpunVersionNameSize,
    _Inout_                                 TPM_STATE *                 PpsTpmState,
    _Inout_                                 unsigned int*               PpunRemainingUpdates)
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
        unReturnValue = FirmwareUpdate_CalculateState(TRUE, PpsTpmState);
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
            wchar_t wszDummy[MAX_NAME];
            unsigned int unDummySize = RG_LEN(wszDummy);
            IGNORE_RETURN_VALUE(Platform_StringSetZero(wszDummy, RG_LEN(wszDummy)));
            unReturnValue = FirmwareUpdate_GetTpmFirmwareVersionString(PpsTpmState->attribs, PwszVersionName, PpunVersionNameSize, wszDummy, &unDummySize);
            if (RC_SUCCESS != unReturnValue)
                break;
        }

        // Read update counter from TPM
        // Cannot be done after firmware update to TPM 2.0 when reboot is required
        // Can be done for TPM2.0 firmware update loader when reboot is required (not in failure mode)
        if ((!(PpsTpmState->attribs.tpm20 && PpsTpmState->attribs.tpm20restartRequired) &&
                !PpsTpmState->attribs.tpm20InFailureMode &&
                !(PpsTpmState->attribs.tpm12 && PpsTpmState->attribs.tpm12FieldUpgradeInfo2Failed)) ||
                (PpsTpmState->attribs.tpmHasFULoader20 && !PpsTpmState->attribs.tpm20InFailureMode))
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
 *  @brief      Checks if the firmware image is valid for the TPM
 *  @details    Performs integrity, consistency and content checks to determine if the given firmware image can be applied to the installed TPM.
 *
 *  @param      PrgbImage                   Firmware image byte stream.
 *  @param      PullImageSize               Size of firmware image byte stream.
 *  @param      PpfValid                    TRUE in case the image is valid, FALSE otherwise.
 *  @param      PpbfNewTpmFirmwareInfo      Pointer to a bit field to return info data for the new firmware image.
 *  @param      PpunErrorDetails            Pointer to an unsigned int to return error details. Possible values are:\n
 *                                              RC_E_FW_UPDATE_BLOCKED in case the field upgrade counter value has been exceeded.\n
 *                                              RC_E_WRONG_FW_IMAGE in case the TPM is not updatable with the given image.\n
 *                                              RC_E_CORRUPT_FW_IMAGE in case the firmware image is corrupt.\n
 *                                              RC_E_NEWER_TOOL_REQUIRED in case a newer version of the tool is required to parse the firmware image.\n
 *                                              RC_E_WRONG_DECRYPT_KEYS in case the TPM2.0 does not have decrypt keys matching to the firmware image.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function.
 *  @retval     RC_E_TPM20_FAILURE_MODE     The TPM2.0 is in failure mode.
 *  @retval     RC_E_RESTART_REQUIRED       In case a restart is required to get back to a functional TPM.
 *  @retval     RC_E_NO_IFX_TPM             In case TPM vendor is not IFX.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
_Success_(return == 0)
unsigned int
FirmwareUpdate_CheckImage(
    _In_bytecount_(PullImageSize)   BYTE *                           PrgbImage,
    _In_                            unsigned long long               PullImageSize,
    _Out_                           BOOL *                           PpfValid,
    _Out_                           BITFIELD_NEW_TPM_FIRMWARE_INFO * PpbfNewTpmFirmwareInfo,
    _Out_                           unsigned int*                    PpunErrorDetails)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        TPM_STATE sTpmState;
        IfxFirmwareImage sIfxFirmwareImage;
        int nBufferSize = (int)PullImageSize;
        BYTE* pbBuffer = PrgbImage;
        Platform_MemorySet(&sTpmState, 0, sizeof(sTpmState));
        Platform_MemorySet(&sIfxFirmwareImage, 0, sizeof(sIfxFirmwareImage));

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

        Platform_MemorySet(PpbfNewTpmFirmwareInfo, 0, sizeof(BITFIELD_NEW_TPM_FIRMWARE_INFO));

        // Get TPM operation mode
        unReturnValue = FirmwareUpdate_CalculateState(TRUE, &sTpmState);
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
            if (RC_E_NEWER_TOOL_REQUIRED == unReturnValue)
            {
                unReturnValue = RC_SUCCESS;
                *PpunErrorDetails = RC_E_NEWER_TOOL_REQUIRED;
            }
            else
            {
                unReturnValue = RC_SUCCESS;
                *PpunErrorDetails = RC_E_CORRUPT_FW_IMAGE;
            }
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
 *  @brief      Prepares a policy session for TPM firmware.
 *  @details    The function prepares a policy session for TPM Firmware Update.
 *
 *  @param      PphPolicySession                    Pointer to session handle that will be filled in by this method.
 *
 *  @retval     RC_SUCCESS                          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER                  An invalid parameter was passed to the function. It is invalid or NULL.
 *  @retval     RC_E_FAIL                           An unexpected error occurred.
 *  @retval     RC_E_PLATFORM_AUTH_NOT_EMPTY        In case PlatformAuth is not the Empty Buffer.
 *  @retval     RC_E_PLATFORM_HIERARCHY_DISABLED    In case platform hierarchy has been disabled.
 *  @retval     ...                                 Error codes from called functions. like TPM error codes.
 */
_Check_return_
unsigned int
FirmwareUpdate_PrepareTPM20Policy(
    _Out_ TSS_TPMI_SH_AUTH_SESSION * PphPolicySession)
{
    unsigned int unReturnValue = RC_E_FAIL;
    TSS_TPMI_SH_AUTH_SESSION hPolicySession = 0;

    do
    {
        // Check parameters
        if (NULL == PphPolicySession)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PphPolicySession is NULL)");
            break;
        }
        *PphPolicySession = 0;

        TSS_TPM2B_NONCE sNonceTpm;
        TSS_TPM2B_NONCE sNonceCaller;

        TSS_TPM2B_DIGEST sPolicyDigest;

        // Authorization session area
        TSS_AuthorizationCommandData sAuthSessionData;
        TSS_AcknowledgmentResponseData sAckAuthSessionData;

        Platform_MemorySet(&sNonceTpm, 0, sizeof(sNonceTpm));
        Platform_MemorySet(&sNonceCaller, 0, sizeof(sNonceCaller));
        Platform_MemorySet(&sPolicyDigest, 0, sizeof(sPolicyDigest));
        Platform_MemorySet(&sAuthSessionData, 0, sizeof(sAuthSessionData));
        Platform_MemorySet(&sAckAuthSessionData, 0, sizeof(sAckAuthSessionData));

        sPolicyDigest.size = TSS_SHA256_DIGEST_SIZE;
        unReturnValue = Platform_MemoryCopy(sPolicyDigest.buffer, sPolicyDigest.size, rgbTpm20FirmwareUpdatePolicyDigest, sPolicyDigest.size);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"Platform_MemoryCopy returned an unexpected value while copying rgbTpm20FirmwareUpdatePolicyDigest.");
            break;
        }

        // Call TPM2_SetPrimaryPolicy command
        sAuthSessionData.authHandle = TSS_TPM_RS_PW;    // Use password based authorization session
        sAuthSessionData.sessionAttributes.continueSession = 1;
        // sAuthSessionData.hmac.size = 0;          // Due to current Empty Buffer platform AuthSecret
        // sAuthSessionData.nonceCaller.size = 0;   // Due to password based authorization session

        // PolicyDigest.buffer: return value of TPM2_PolicyDigest
        unReturnValue = TSS_TPM2_SetPrimaryPolicy(TSS_TPM_RH_PLATFORM, &sAuthSessionData, &sPolicyDigest, TSS_TPM_ALG_SHA256, &sAckAuthSessionData);
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
            TSS_TPM2B_ENCRYPTED_SECRET sEncSecretEmpty;
            TSS_TPMT_SYM_DEF sSymDefEmpty;
            Platform_MemorySet(&sEncSecretEmpty, 0, sizeof(sEncSecretEmpty));
            Platform_MemorySet(&sSymDefEmpty, 0, sizeof(sSymDefEmpty));

            sSymDefEmpty.algorithm = TSS_TPM_ALG_NULL;
            unReturnValue = TSS_TPM2_StartAuthSession(TSS_TPM_RH_NULL,
                            // Bind must be NULL, otherwise we'd have
                            // to calculate AuthSessionData.hmac !!
                            TSS_TPM_RH_NULL,
                            &sNonceCaller, &sEncSecretEmpty,
                            TSS_TPM_SE_POLICY, &sSymDefEmpty, TSS_TPM_ALG_SHA256,
                            &hPolicySession, &sNonceTpm);
            if (TSS_TPM_RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Error calling TSS_TPM2_StartAuthSession");
                break;
            }
        }

        {
            // Update policy session to include platformAuth
            TSS_TPM2B_DIGEST sDigestEmpty;
            TSS_TPM2B_NONCE sPolicyRef;
            TSS_TPMT_TK_AUTH sPolicyTicket;
            TSS_TPM2B_TIMEOUT sTimeout;
            unsigned int unPlatformValue = Platform_SwapBytes32(TSS_TPM_RH_PLATFORM);

            Platform_MemorySet(&sDigestEmpty, 0, sizeof(sDigestEmpty));
            Platform_MemorySet(&sPolicyRef, 0, sizeof(sPolicyRef));
            Platform_MemorySet(&sPolicyTicket, 0, sizeof(sPolicyTicket));
            Platform_MemorySet(&sTimeout, 0, sizeof(sTimeout));

            sPolicyRef.size = sizeof(unsigned int);
            unReturnValue = Platform_MemoryCopy(sPolicyRef.buffer, sPolicyRef.size, (const void*) &unPlatformValue, sizeof(unPlatformValue));
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"Unexpected return value from function call Platform_MemoryCopy");
                break;
            }

            unReturnValue = TSS_TPM2_PolicySecret(TSS_TPM_RH_PLATFORM, &sAuthSessionData, hPolicySession, &sNonceTpm, &sDigestEmpty, &sPolicyRef, 0, &sTimeout, &sPolicyTicket, &sAckAuthSessionData);
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
 *  @brief      Function to issue TPM_FieldUpgrade_Start
 *  @details    This function initiates TPM Firmware Update via the TPM_FieldUpgrade_Start command depending on the current TPM Operation mode.
 *
 *  @param      PbfTpmAttributes        The current TPM operation mode.
 *  @param      PpsIfxFirmwareImage     Pointer to structure containing the firmware image.
 *  @param      PpsFirmwareUpdateData   Pointer to structure containing all relevant data for a firmware update.
 *
 *  @retval     RC_SUCCESS                      The operation completed successfully.
 *  @retval     RC_E_TPM12_MISSING_OWNERAUTH    The TPM has an owner but TPM Owner authorization was not provided (TPM1.2 only).
 *  @retval     RC_E_TPM12_NO_OWNER             The TPM does not have an owner but TPM Owner authorization was provided (TPM1.2 only).
 *  @retval     RC_E_FAIL                       TPM connection or command error.
 *  @retval     ...                             Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_Start(
    _In_    BITFIELD_TPM_ATTRIBUTES             PbfTpmAttributes,
    _In_    const IfxFirmwareImage * const      PpsIfxFirmwareImage,
    _In_    const IfxFirmwareUpdateData * const PpsFirmwareUpdateData)
{
    unsigned int unReturnValue = RC_E_FAIL;
    BOOL fUpdateStarted = FALSE;

    // Check if firmware update was already started
    if (!PbfTpmAttributes.tpmInOperationalMode)
    {
        unReturnValue = RC_SUCCESS; // Continue firmware update
        PpsFirmwareUpdateData->fnProgressCallback(0);
    }
    else if (PbfTpmAttributes.tpm20 && PbfTpmAttributes.infineon && !PbfTpmAttributes.tpm20restartRequired)
    {
        unReturnValue = FirmwareUpdate_Start_Tpm20(
                            PbfTpmAttributes,
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
 *  @brief      Function to update the firmware with the given firmware image
 *  @details    This function updates the TPM firmware with the image given in the parameters.
 *              A check if the firmware can be updated with the image is done before.
 *
 *  @param      PpsFirmwareUpdateData   Pointer to structure containing all relevant data for a firmware update.
 *
 *  @retval     RC_SUCCESS                      The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER              An invalid parameter was passed to the function. The parameter is NULL.
 *  @retval     RC_E_CORRUPT_FW_IMAGE           Policy parameter block is corrupt and/or cannot be unmarshalled.
 *  @retval     RC_E_NO_IFX_TPM                 The TPM is not manufactured by Infineon.
 *  @retval     RC_E_TPM12_MISSING_OWNERAUTH    The TPM has an owner but TPM Owner authorization was not provided (TPM1.2 only).
 *  @retval     RC_E_TPM12_NO_OWNER             The TPM does not have an owner but TPM Owner authorization was provided (TPM1.2 only).
 *  @retval     RC_E_FAIL                       TPM connection or command error.
 *  @retval     ...                             Error codes from called functions.
 *  @retval     RC_E_FIRMWARE_UPDATE_FAILED     The update operation was started but failed.
 */
_Check_return_
unsigned int
FirmwareUpdate_UpdateImage(
    _In_    const IfxFirmwareUpdateData * const  PpsFirmwareUpdateData)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Check parameters
        if (NULL == PpsFirmwareUpdateData)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Parameter not initialized correctly (PpsFirmwareUpdateData is NULL)");
            break;
        }

        TPM_STATE sTpmState;
        IfxFirmwareImage sIfxFirmwareImage;
        int nBufferSize = (int)PpsFirmwareUpdateData->unFirmwareImageSize;
        BYTE* pbBuffer = PpsFirmwareUpdateData->rgbFirmwareImage;
        Platform_MemorySet(&sTpmState, 0, sizeof(sTpmState));
        Platform_MemorySet(&sIfxFirmwareImage, 0, sizeof(sIfxFirmwareImage));

        // Get TPM operation mode
        unReturnValue = FirmwareUpdate_CalculateState(TRUE, &sTpmState);
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
            ERROR_STORE(RC_E_CORRUPT_FW_IMAGE, L"Firmware image cannot be parsed. (0x%.8X)");
            unReturnValue = RC_E_CORRUPT_FW_IMAGE;
            break;
        }

        if (sTpmState.attribs.tpmHasFULoader20)
        {
            // Select correct manifest
            unReturnValue = FirmwareUpdate_SelectManifest(&sIfxFirmwareImage);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"FirmwareUpdate_SelectManifest returned an error");
                break;
            }
        }

        // Perform the firmware update
        // Start the firmware update
        unReturnValue = FirmwareUpdate_Start(sTpmState.attribs, &sIfxFirmwareImage, PpsFirmwareUpdateData);
        if (RC_SUCCESS != unReturnValue)
            break;

        if (sTpmState.attribs.tpmHasFULoader20)
        {
            // Transfer manifest and new firmware data to TPM
            unReturnValue = FirmwareUpdate_UpdateTpm20(&sIfxFirmwareImage, PpsFirmwareUpdateData);
            if (RC_SUCCESS != unReturnValue)
                break;
        }
        else
        {
            // Transfer new firmware data to TPM
            unReturnValue = FirmwareUpdate_Update(sIfxFirmwareImage.unFirmwareSize, sIfxFirmwareImage.rgbFirmware, PpsFirmwareUpdateData->fnProgressCallback);
            if (RC_SUCCESS != unReturnValue)
                break;

            // Finalize the firmware update
            unReturnValue = FirmwareUpdate_Complete(PpsFirmwareUpdateData->fnProgressCallback);
            if (RC_SUCCESS != unReturnValue)
                break;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Checks TPM Owner authorization on TPM1.2.
 *  @details    The function checks TPM Owner authorization on TPM1.2 by calling TPM_ChangeAuthOwner with old and new TPM Owner authorization value set to the same value.
 *
 *  @param      PrgbOwnerAuthHash               TPM Owner Authentication hash (sha1).
 *
 *  @retval     RC_SUCCESS                      The operation completed successfully.
 *  @retval     RC_E_FAIL                       An internal error occurred.
 *  @retval     ...                             Error codes from called functions.
 */
_Check_return_
unsigned int
FirmwareUpdate_CheckOwnerAuthorization(
    _In_bytecount_(TSS_SHA1_DIGEST_SIZE)    const BYTE  PrgbOwnerAuthHash[TSS_SHA1_DIGEST_SIZE])
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        TSS_TPM_NONCE sNonceOddOSAP;
        TSS_TPM_AUTHHANDLE unAuthHandle = 0;
        TSS_TPM_NONCE sNonceEven;
        TSS_TPM_NONCE sNonceEvenOSAP;
        TSS_TPM_AUTHDATA sOsapSharedSecret;
        TSS_TPM_ENTITY_TYPE usEntity = (TSS_TPM_ET_XOR << 8) | TSS_TPM_ET_OWNER;
        TSS_TPM_PUBKEY sPubKey;

        Platform_MemorySet(&sNonceOddOSAP, 0, sizeof(sNonceOddOSAP));
        Platform_MemorySet(&sNonceEven, 0, sizeof(sNonceEven));
        Platform_MemorySet(&sNonceEvenOSAP, 0, sizeof(sNonceEvenOSAP));
        Platform_MemorySet(&sOsapSharedSecret, 0, sizeof(sOsapSharedSecret));
        Platform_MemorySet(&sPubKey, 0, sizeof(sPubKey));

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
            BYTE rgbInput[2 * sizeof(TSS_TPM_NONCE)];
            Platform_MemorySet(rgbInput, 0, sizeof(rgbInput));
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
            TSS_TPM_AUTHDATA sResAuth;
            Platform_MemorySet(&sResAuth, 0, sizeof(sResAuth));
            // Read public portion of EK over OSAP session
            unReturnValue = TSS_TPM_OwnerReadInternalPub(TSS_TPM_KH_EK, unAuthHandle, &sNonceOddOSAP, &sNonceEven, &sOsapSharedSecret, &sPubKey, &sResAuth);
            {
                IGNORE_RETURN_VALUE(TSS_TPM_FlushSpecific(unAuthHandle, TSS_TPM_RT_AUTH));
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
