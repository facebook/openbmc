/**
 *  @brief      Implements marshal and unmarshal operations for TPM1.2 and TPM2.0 commands, structures and types.
 *  @details    This module provides marshalling and unmarshalling function for all TPM1.2 and TPM2.0 commands,
 *              structures and types defined in the Infineon Secure Boot Loader Specification.
 *  @file       TPM2_FieldUpgradeMarshal.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TPM2_Marshal.h"
#include "TPM2_FieldUpgradeMarshal.h"
#include "Platform.h"

//********************************************************************************************************
//
// Marshaling helper functions
//
//********************************************************************************************************

/**
 *  @brief      Marshal helper to calculate the LRC
 *  @details
 *
 *  @param      PrgbData        Pointer to a buffer to calculate the LRC from.
 *  @param      PunDataSize     Size of the buffer.
 *
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     ...         Error codes from called functions.
 */
TSS_BYTE
TSS_CalcLRC(
    _In_bytecount_(PunDataSize) uint8_t*    PrgbData,
    _In_                        uint32_t    PunDataSize)
{
    uint8_t bLRC = 0;
    uint32_t unIndex;

    if (NULL != PrgbData)
    {
        for (unIndex = 0; unIndex < PunDataSize; unIndex++)
            bLRC ^= *(PrgbData++);
    }

    return bLRC;
}

//********************************************************************************************************
//
// Marshal and unmarshal for structures
//
//********************************************************************************************************

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sMessageDigest_d
//--------------------------------------------------------------------------------------------------------

/**
 *  @brief      Unmarshal structure of type sMessageDigest_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sMessageDigest_d_Unmarshal(
    _Out_   sMessageDigest_d*   PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sMessageDigest_d));
        // Check _Inout_ parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        // Unmarshal wSize
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal rgbMessageDigest
        unReturnValue = TSS_UINT8_Array_Unmarshal(PpTarget->rgbMessageDigest, PprgbBuffer, PpnBufferSize, PpTarget->wSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;
    return unReturnValue;
}

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sFirmwarePackage_d
//--------------------------------------------------------------------------------------------------------

/**
 *  @brief      Unmarshal structure of type sFirmwarePackage_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sFirmwarePackage_d_Unmarshal(
    _Out_   sFirmwarePackage_d* PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sFirmwarePackage_d));
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        // Unmarshal FwPackageIdentifier_d (FwPackageIdentifier)
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->FwPackageIdentifier, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal VersionNumber_d (Version)
        unReturnValue = TSS_VersionNumber_d_Unmarshal(&PpTarget->Version, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal VersionNumber_d (StaleVersion)
        unReturnValue = TSS_VersionNumber_d_Unmarshal(&PpTarget->StaleVersion, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;
    return unReturnValue;
}

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sVersions_d
//--------------------------------------------------------------------------------------------------------

/**
 *  @brief      Unmarshal structure of type sVersions_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sVersions_d_Unmarshal(
    _Out_   sVersions_d*    PpTarget,
    _Inout_ TSS_BYTE**      PprgbBuffer,
    _Inout_ TSS_INT32*      PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sVersions_d));
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->internal1, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal UINT16 (wEntries)
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wEntries, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal VersionNumber_d (Version)
        unReturnValue = TSS_VersionNumber_d_Array_Unmarshal(PpTarget->Version, PprgbBuffer, PpnBufferSize, 8);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sSignedAttributes_d
//--------------------------------------------------------------------------------------------------------

/**
 *  @brief      Unmarshal structure of type sSignedAttributes_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSignedAttributes_d_Unmarshal(
    _Out_   sSignedAttributes_d*    PpTarget,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sSignedAttributes_d));
        // Check _Inout_ parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal1, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal2, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal2));
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal3, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sMessageDigest_d (sMessageDigest)
        unReturnValue = TSS_sMessageDigest_d_Unmarshal(&PpTarget->sMessageDigest, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sFirmwarePackage_d (sFirmwarePackage)
        unReturnValue = TSS_sFirmwarePackage_d_Unmarshal(&PpTarget->sFirmwarePackage, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal6, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal6));
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal7, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal7));
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal8, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal8));
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal DecryptKeyId_d (DecryptKeyId)
        unReturnValue = TSS_DecryptKeyId_d_Unmarshal(&PpTarget->DecryptKeyId, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal10, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal10));
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sVersions_d (sVersions)
        unReturnValue = TSS_sVersions_d_Unmarshal(&PpTarget->sVersions, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal12, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal12));
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal13, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal13));
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sSignerInfo_d
//--------------------------------------------------------------------------------------------------------

/**
 *  @brief      Unmarshal structure of type sSignerInfo_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSignerInfo_d_Unmarshal(
    _Out_   sSignerInfo_d*  PpTarget,
    _Inout_ TSS_BYTE**      PprgbBuffer,
    _Inout_ TSS_INT32*      PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sSignerInfo_d));
        // Check _Inout_ parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal1, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->internal2, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal3, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal4, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal5, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sSignedAttributes_d (sSignedAttributes)
        unReturnValue = TSS_sSignedAttributes_d_Unmarshal(&PpTarget->sSignedAttributes, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal7, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT8_Array_Unmarshal(PpTarget->internal8, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal8));
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

//--------------------------------------------------------------------------------------------------------
// Marshal and unmarshal structure sSignedData_d
//--------------------------------------------------------------------------------------------------------

/**
 *  @brief      Unmarshal structure of type sSignedData_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSignedData_d_Unmarshal(
    _Out_   sSignedData_d*  PpTarget,
    _Inout_ TSS_BYTE**      PprgbBuffer,
    _Inout_ TSS_INT32*      PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sSignedData_d));
        // Check _Inout_ parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal1, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal2, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal UINT16 (wSignerInfoSize)
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wSignerInfoSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sSignerInfo_d (sSignerInfo)
        unReturnValue = TSS_sSignerInfo_d_Unmarshal(&PpTarget->sSignerInfo, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal structure of type sSecurityModuleLogicInfo_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSecurityModuleLogicInfo_d_Unmarshal(
    _Out_   sSecurityModuleLogicInfo_d*     PpTarget,
    _Inout_ TSS_BYTE**                      PprgbBuffer,
    _Inout_ TSS_INT32*                      PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sSecurityModuleLogicInfo_d));
        // Check _Inout_ parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal1, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal UINT16 (wMaxDataSize)
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wMaxDataSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sSecurityModuleLogic_d (sSecurityModuleLogic)
        unReturnValue = TSS_sSecurityModuleLogic_d_Unmarshal(&PpTarget->sSecurityModuleLogic, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal SecurityModuleStatus_d (SecurityModuleStatus)
        unReturnValue = TSS_SecurityModuleStatus_d_Unmarshal(&PpTarget->SecurityModuleStatus, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sFirmwarePackage_d (sProcessFirmwarePackage)
        unReturnValue = TSS_sFirmwarePackage_d_Unmarshal(&PpTarget->sProcessFirmwarePackage, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal6, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal7, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal7));
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal UINT16 (wFieldUpgradeCounter)
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wFieldUpgradeCounter, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal structure of type sSecurityModuleLogicInfo2_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSecurityModuleLogicInfo2_d_Unmarshal(
    _Out_   sSecurityModuleLogicInfo2_d*    PpTarget,
    _Inout_ TSS_BYTE**                      PprgbBuffer,
    _Inout_ TSS_INT32*                      PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sSecurityModuleLogicInfo2_d));
        // Check _Inout_ parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal1, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal UINT16 (wMaxDataSize)
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wMaxDataSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sSecurityModuleLogic_d (sSecurityModuleLogic)
        unReturnValue = TSS_sSecurityModuleLogic_d_Unmarshal(&PpTarget->sSecurityModuleLogic, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal SecurityModuleStatus_d (SecurityModuleStatus)
        unReturnValue = TSS_SecurityModuleStatus_d_Unmarshal(&PpTarget->SecurityModuleStatus, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sFirmwarePackage_d (sProcessFirmwarePackage)
        unReturnValue = TSS_sFirmwarePackage_d_Unmarshal(&PpTarget->sProcessFirmwarePackage, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal6, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal7, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal7));
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal UINT16 (wFieldUpgradeCounter)
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wFieldUpgradeCounter, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sKeyList_d (sKeyList)
        unReturnValue = TSS_sKeyList_d_Unmarshal(&PpTarget->sKeyList, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal structure of type sKeyList_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sKeyList_d_Unmarshal(
    _Out_   sKeyList_d* PpTarget,
    _Inout_ TSS_BYTE**  PprgbBuffer,
    _Inout_ TSS_INT32*  PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sKeyList_d));
        // Check _Inout_ parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        // Unmarshal UINT16 (wEntries)
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wEntries, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        if (PpTarget->wEntries != 4)
        {
            unReturnValue = RC_E_FAIL;
            break;
        }
        {
            unsigned int unCounter = 0;
            for (unCounter = 0; unCounter < PpTarget->wEntries; unCounter++)
            {
                unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->internal2[unCounter], PprgbBuffer, PpnBufferSize);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }
            if (RC_SUCCESS != unReturnValue)
                break;
            for (unCounter = 0; unCounter < PpTarget->wEntries; unCounter++)
            {
                // Unmarshal DecryptKeyId_d (DecryptKeyId)
                unReturnValue = TSS_DecryptKeyId_d_Unmarshal(&PpTarget->DecryptKeyId[unCounter], PprgbBuffer, PpnBufferSize);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }
            if (RC_SUCCESS != unReturnValue)
                break;
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal structure of type sSecurityModuleLogic_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sSecurityModuleLogic_d_Unmarshal(
    _Out_   sSecurityModuleLogic_d* PpTarget,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sSecurityModuleLogic_d));
        // Check _Inout_ parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal1, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->unTargetHardwareIdentifier, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal(PpTarget->internal3, PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal3));
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sFirmwarePackage_d (sBootloaderFirmwarePackage)
        unReturnValue = TSS_sFirmwarePackage_d_Unmarshal(&PpTarget->sBootloaderFirmwarePackage, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal sFirmwarePackages_d (sFirmwareConfiguration)
        unReturnValue = TSS_sFirmwarePackages_d_Unmarshal(&PpTarget->sFirmwareConfiguration, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal structure of type sFirmwarePackages_d
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER  An invalid parameter was passed to the function.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_sFirmwarePackages_d_Unmarshal(
    _Out_   sFirmwarePackages_d*    PpTarget,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        // Check and initialize _Out_ parameters
        if (NULL == PpTarget)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        Platform_MemorySet(PpTarget, 0x00, sizeof(sFirmwarePackages_d));
        // Check _Inout_ parameters
        if ((NULL == PprgbBuffer) || (NULL == *PprgbBuffer) || (NULL == PpnBufferSize))
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        // Unmarshal UINT16 (wEntries)
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wEntries, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        if (PpTarget->wEntries > 0)
        {
            TSS_INT32 nPos;
            for (nPos = 0; nPos < PpTarget->wEntries; nPos++)
            {
                // Unmarshal sFirmwarePackage_d (FirmwarePackage)
                unReturnValue = TSS_sFirmwarePackage_d_Unmarshal((sFirmwarePackage_d*)&PpTarget->FirmwarePackage[nPos], PprgbBuffer, PpnBufferSize);
                if (RC_SUCCESS != unReturnValue)
                    break;
            }
        }
    }
    WHILE_FALSE_END;
    return unReturnValue;
}
