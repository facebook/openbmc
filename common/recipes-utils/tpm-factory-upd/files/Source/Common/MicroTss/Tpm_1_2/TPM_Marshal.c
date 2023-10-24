/**
 *  @brief      Implements marshal and unmarshal operations for TPM1.2 structures and types.
 *  @details    This module provides marshalling and unmarshalling function for structures and types.
 *  @file       TPM_Marshal.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TPM_Marshal.h"
#include "TPM2_Marshal.h"

//********************************************************************************************************
//
// Marshal and unmarshal for types
//
//********************************************************************************************************

/**
 *  @brief      Marshal type TSS_TPM_TAG
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_TAG_Marshal(
    _In_    TSS_TPM_TAG*    PpSource,
    _Inout_ TSS_BYTE**      PprgbBuffer,
    _Inout_ TSS_INT32*      PpnBufferSize)
{
    return TSS_UINT16_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Unmarshal type TSS_TPM_TAG
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_TAG_Unmarshal(
    _Out_   TSS_TPM_TAG*    PpTarget,
    _Inout_ TSS_BYTE**      PprgbBuffer,
    _Inout_ TSS_INT32*      PpnBufferSize)
{
    return TSS_UINT16_Unmarshal(PpTarget, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TSS_TPM_COMMAND_CODE
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_COMMAND_CODE_Marshal(
    _In_    TSS_TPM_COMMAND_CODE*   PpSource,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    return TSS_UINT32_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TPM_STARTUP_TYPE
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STARTUP_TYPE_Marshal(
    _In_    TSS_TPM_STARTUP_TYPE*   PpSource,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    return TSS_UINT16_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Unmarshal type TSS_TPM_RESULT
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_RESULT_Unmarshal(
    _Out_   TSS_TPM_RESULT* PpTarget,
    _Inout_ TSS_BYTE**      PprgbBuffer,
    _Inout_ TSS_INT32*      PpnBufferSize)
{
    return TSS_UINT32_Unmarshal(PpTarget, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Unmarshal type TPM_STRUCTURE_TAG
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STRUCTURE_TAG_Unmarshal(
    _Out_   TSS_TPM_STRUCTURE_TAG*  PpTarget,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    return TSS_UINT16_Unmarshal(PpTarget, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Unmarshal type TPM_VERSION_BYTE
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_VERSION_BYTE_Unmarshal(
    _Out_   TSS_TPM_VERSION_BYTE*   PpTarget,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    return TSS_UINT8_Unmarshal(PpTarget, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TSS_TPM_CAPABILITY_AREA
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_CAPABILITY_AREA_Marshal(
    _In_    TSS_TPM_CAPABILITY_AREA*    PpSource,
    _Inout_ TSS_BYTE**                  PprgbBuffer,
    _Inout_ TSS_INT32*                  PpnBufferSize)
{
    return TSS_UINT32_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TPM_ENC_SCHEME
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_ENC_SCHEME_Marshal(
    _In_    TSS_TPM_ENC_SCHEME* PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    return TSS_UINT16_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Unmarshal type TPM_ENC_SCHEME
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_ENC_SCHEME_Unmarshal(
    _Out_   TSS_TPM_ENC_SCHEME* PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    return TSS_UINT16_Unmarshal(PpTarget, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TPM_SIG_SCHEME
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_SIG_SCHEME_Marshal(
    _In_    TSS_TPM_SIG_SCHEME* PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    return TSS_UINT16_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Unmarshal type TPM_SIG_SCHEME
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_SIG_SCHEME_Unmarshal(
    _Out_   TSS_TPM_SIG_SCHEME* PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    return TSS_UINT16_Unmarshal(PpTarget, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TPM_PROTOCOL_ID
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_PROTOCOL_ID_Marshal(
    _In_    TSS_TPM_PROTOCOL_ID*    PpSource,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    return TSS_UINT16_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TPM_KEY_USAGE
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_USAGE_Marshal(
    _In_    TSS_TPM_KEY_USAGE*  PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    return TSS_UINT16_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Unmarshal type TPM_KEY_USAGE
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_USAGE_Unmarshal(
    _Out_   TSS_TPM_KEY_USAGE*  PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    return TSS_UINT16_Unmarshal(PpTarget, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TPM_KEY_FLAGS
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_FLAGS_Marshal(
    _In_    TSS_TPM_KEY_FLAGS*  PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    return TSS_UINT32_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Unmarshal type TPM_KEY_FLAGS
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_FLAGS_Unmarshal(
    _Out_   TSS_TPM_KEY_FLAGS*  PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    return TSS_UINT32_Unmarshal(PpTarget, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TPM_AUTH_DATA_USAGE
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTH_DATA_USAGE_Marshal(
    _In_    TSS_TPM_AUTH_DATA_USAGE*    PpSource,
    _Inout_ TSS_BYTE**                  PprgbBuffer,
    _Inout_ TSS_INT32*                  PpnBufferSize)
{
    return TSS_BYTE_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Unmarshal type TPM_KEY_FLAGS
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTH_DATA_USAGE_Unmarshal(
    _Out_   TSS_TPM_AUTH_DATA_USAGE*    PpTarget,
    _Inout_ TSS_BYTE**                  PprgbBuffer,
    _Inout_ TSS_INT32*                  PpnBufferSize)
{
    return TSS_BYTE_Unmarshal(PpTarget, PprgbBuffer, PpnBufferSize);
}

/**
 *  @brief      Marshal type TSS_TPM_AUTHHANDLE
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTHHANDLE_Marshal(
    _In_    TSS_TPM_AUTHHANDLE* PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    return TSS_UINT32_Marshal(PpSource, PprgbBuffer, PpnBufferSize);
}

//********************************************************************************************************
//
// Marshal and unmarshal for structures
//
//********************************************************************************************************

/**
 *  @brief      Unmarshal type TPM_VERSION
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_VERSION_Unmarshal(
    _Out_   TSS_TPM_VERSION*    PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal Version byte major
        unReturnValue = TSS_TPM_VERSION_BYTE_Unmarshal(&PpTarget->major, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        /// Unmarshal Version byte minor
        unReturnValue = TSS_TPM_VERSION_BYTE_Unmarshal(&PpTarget->minor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal revision major
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->revMajor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal revision minor
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->revMinor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal type TSS_TPM_CAP_VERSION_INFO
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case of vendor specific data buffer is too small.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_CAP_VERSION_INFO_Unmarshal(
    _Out_   TSS_TPM_CAP_VERSION_INFO*   PpTarget,
    _Inout_ TSS_BYTE**                  PprgbBuffer,
    _Inout_ TSS_INT32*                  PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal structure tag
        unReturnValue = TSS_TPM_STRUCTURE_TAG_Unmarshal(&PpTarget->tag, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal TPM version
        unReturnValue = TSS_TPM_VERSION_Unmarshal(&PpTarget->version, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal spec Level
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->specLevel, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal spec letter
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->specLetter, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal spec letter
        unReturnValue = TSS_UINT8_Array_Unmarshal(&PpTarget->tpmVendorID[0], PprgbBuffer, PpnBufferSize, sizeof(PpTarget->tpmVendorID));
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal vendor specific size
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->vendorSpecificSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Check maximal buffer size
        if (sizeof(PpTarget->vendorSpecific) < PpTarget->vendorSpecificSize)
        {
            unReturnValue = RC_E_BUFFER_TOO_SMALL;
            break;
        }

        // Unmarshal spec letter
        unReturnValue = TSS_UINT8_Array_Unmarshal(&PpTarget->vendorSpecific[0], PprgbBuffer, PpnBufferSize, PpTarget->vendorSpecificSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal type TPM_PERMANENT_FLAGS
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_PERMANENT_FLAGS_Unmarshal(
    _Out_   TSS_TPM_PERMANENT_FLAGS*        PpTarget,
    _Inout_ TSS_BYTE**                      PprgbBuffer,
    _Inout_ TSS_INT32*                      PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Set Permanent Flag
        PpTarget->tag = TSS_TPM_TAG_PERMANENT_FLAGS;

        // Unmarshal disable
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->disable, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal ownership
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->ownership, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal deactivated
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->deactivated, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal readPubek
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->readPubek, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal disableOwnerClear
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->disableOwnerClear, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal allowMaintenance
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->allowMaintenance, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal physicalPresenceLifetimeLock
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->physicalPresenceLifetimeLock, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal physicalPresenceHWEnable
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->physicalPresenceHWEnable, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal physicalPresenceCMDEnable
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->physicalPresenceCMDEnable, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal CEKPUsed
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->CEKPUsed, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal TPMpost
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->TPMpost, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal TPMpostLock
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->TPMpostLock, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal FIPS
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->FIPS, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal bOperator
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->bOperator, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal enableRevokeEK
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->enableRevokeEK, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal nvLocked
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->nvLocked, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal readSRKPub
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->readSRKPub, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal tpmEstablished
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->tpmEstablished, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal maintenanceDone
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->maintenanceDone, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal disableFullDALogicInfo
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->disableFullDALogicInfo, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal type TPM_STCLEAR_FLAGS
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STCLEAR_FLAGS_Unmarshal(
    _Out_   TSS_TPM_STCLEAR_FLAGS*  PpTarget,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Set Permanent Flag
        PpTarget->tag = TSS_TPM_TAG_STCLEAR_FLAGS;

        // Unmarshal deactivated
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->deactivated, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal disableForceClear
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->disableForceClear, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal physicalPresence
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->physicalPresence, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal physicalPresenceLock
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->physicalPresenceLock, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal bGlobalLock
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->bGlobalLock, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal TCPA_VERSION structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TCPA_VERSION_Unmarshal(
    _Out_   TSS_TCPA_VERSION*   PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal major
        unReturnValue = TSS_TPM_VERSION_BYTE_Unmarshal(&PpTarget->major, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal minor
        unReturnValue = TSS_TPM_VERSION_BYTE_Unmarshal(&PpTarget->minor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal revMajor
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->revMajor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal revMinor
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->revMinor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Marshal type TSS_TPM_AUTH_IN
 *  @details
 *
 *  @param      PpSource        Source type to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS  The operation completed successfully.
 *  @retval     ...         Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTH_IN_Marshal(
    _In_    TSS_TPM_AUTH_IN*    PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal dwAuthHandle
        unReturnValue = TSS_UINT32_Marshal(&PpSource->dwAuthHandle, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal sNonceOdd
        unReturnValue = TSS_UINT8_Array_Marshal(&(PpSource->sNonceOdd.nonce[0]), PprgbBuffer, PpnBufferSize, sizeof(PpSource->sNonceOdd.nonce));
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal bContinueAuthSession
        unReturnValue = TSS_UINT8_Marshal(&PpSource->bContinueAuthSession, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal sOwnerAuth
        unReturnValue = TSS_UINT8_Array_Marshal(&(PpSource->sOwnerAuth.authdata[0]), PprgbBuffer, PpnBufferSize, sizeof(PpSource->sOwnerAuth.authdata));
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Marshal TPM_RSA_KEY_PARMS structure
 *  @details
 *
 *  @param      PpSource        Pointer to the source to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_RSA_KEY_PARMS_Marshal(
    _In_    TSS_TPM_RSA_KEY_PARMS*  PpSource,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Marshal algorithmParms
        unReturnValue = TSS_UINT32_Marshal(&PpSource->keyLength, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Marshal algorithmParms
        unReturnValue = TSS_UINT32_Marshal(&PpSource->numPrimes, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Marshal algorithmParms
        unReturnValue = TSS_UINT32_Marshal(&PpSource->exponentSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal TPM_RSA_KEY_PARMS structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_RSA_KEY_PARMS_Unmarshal(
    _Out_   TSS_TPM_RSA_KEY_PARMS*  PpTarget,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal algorithmParms
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->keyLength, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal algorithmParms
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->numPrimes, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal algorithmParms
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->exponentSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Marshal TPM_KEY_PARMS structure
 *  @details
 *
 *  @param      PpSource        Pointer to the source to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_PARMS_Marshal(
    _In_    TSS_TPM_KEY_PARMS*  PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Marshal algorithmParms
        unReturnValue = TSS_TPM_ALGORITHM_ID_Marshal(&PpSource->algorithmID, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Marshal algorithmParms
        unReturnValue = TSS_TPM_ENC_SCHEME_Marshal(&PpSource->encScheme, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Marshal algorithmParms
        unReturnValue = TSS_TPM_SIG_SCHEME_Marshal(&PpSource->sigScheme, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Marshal algorithmParms
        unReturnValue = TSS_UINT32_Marshal(&PpSource->parmSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Marshal algorithmParms
        unReturnValue = TSS_TPM_RSA_KEY_PARMS_Marshal(&PpSource->parms, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal TPM_KEY_PARMS structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_PARMS_Unmarshal(
    _Out_   TSS_TPM_KEY_PARMS*  PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal algorithmParms
        unReturnValue = TSS_TPM_ALGORITHM_ID_Unmarshal(&PpTarget->algorithmID, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal algorithmParms
        unReturnValue = TSS_TPM_ENC_SCHEME_Unmarshal(&PpTarget->encScheme, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal algorithmParms
        unReturnValue = TSS_TPM_SIG_SCHEME_Unmarshal(&PpTarget->sigScheme, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal algorithmParms
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->parmSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal algorithmParms
        unReturnValue = TSS_TPM_RSA_KEY_PARMS_Unmarshal(&PpTarget->parms, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Marshal TPM_STORE_PUBKEY structure
 *  @details
 *
 *  @param      PpSource        Pointer to the source to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STORE_PUBKEY_Marshal(
    _In_    TSS_TPM_STORE_PUBKEY*   PpSource,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Marshal algorithmParms
        unReturnValue = TSS_UINT32_Marshal(&PpSource->keyLength, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Marshal pubKey
        unReturnValue = TSS_BYTE_Array_Marshal(&PpSource->key[0], PprgbBuffer, PpnBufferSize, KEY_LEN);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal TPM_STORE_PUBKEY structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STORE_PUBKEY_Unmarshal(
    _Out_   TSS_TPM_STORE_PUBKEY*   PpTarget,
    _Inout_ TSS_BYTE**              PprgbBuffer,
    _Inout_ TSS_INT32*              PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal algorithmParms
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->keyLength, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal pubKey
        unReturnValue = TSS_BYTE_Array_Unmarshal(&PpTarget->key[0], PprgbBuffer, PpnBufferSize, KEY_LEN);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal TPM_PUBKEY structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to read from.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_PUBKEY_Unmarshal(
    _Out_   TSS_TPM_PUBKEY*     PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal algorithmParms
        unReturnValue = TSS_TPM_KEY_PARMS_Unmarshal(&PpTarget->algorithmParms, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal pubKey
        unReturnValue = TSS_TPM_STORE_PUBKEY_Unmarshal(&PpTarget->pubKey, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Marshal TSS_TPM_AUTHDATA structure
 *  @details
 *
 *  @param      PpSource        Pointer to the source to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTHDATA_Marshal(
    _In_    TSS_TPM_AUTHDATA*   PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Marshal nonce buffer
        unReturnValue = TSS_BYTE_Array_Marshal(&PpSource->authdata[0], PprgbBuffer, PpnBufferSize, TPM_AUTHDATA_SIZE);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal TSS_TPM_AUTHDATA structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_AUTHDATA_Unmarshal(
    _Out_   TSS_TPM_AUTHDATA*   PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal nonce buffer
        unReturnValue = TSS_BYTE_Array_Unmarshal(&PpTarget->authdata[0], PprgbBuffer, PpnBufferSize, TPM_AUTHDATA_SIZE);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Marshal TSS_TPM_NONCE structure
 *  @details
 *
 *  @param      PpSource        Pointer to the source to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_NONCE_Marshal(
    _In_    TSS_TPM_NONCE*  PpSource,
    _Inout_ TSS_BYTE**      PprgbBuffer,
    _Inout_ TSS_INT32*      PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Marshal nonce buffer
        unReturnValue = TSS_BYTE_Array_Marshal(&PpSource->nonce[0], PprgbBuffer, PpnBufferSize, TPM_NONCE_SIZE);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal TSS_TPM_NONCE structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_NONCE_Unmarshal(
    _Out_   TSS_TPM_NONCE*  PpTarget,
    _Inout_ TSS_BYTE**      PprgbBuffer,
    _Inout_ TSS_INT32*      PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal nonce buffer
        unReturnValue = TSS_BYTE_Array_Unmarshal(&PpTarget->nonce[0], PprgbBuffer, PpnBufferSize, TPM_NONCE_SIZE);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Marshal TPM_STRUCT_VER structure
 *  @details
 *
 *  @param      PpSource        Pointer to the source to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STRUCT_VER_Marshal(
    _In_    TSS_TPM_STRUCT_VER* PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Marshal major version
        unReturnValue = TSS_BYTE_Marshal(&PpSource->major, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal minor version
        unReturnValue = TSS_BYTE_Marshal(&PpSource->minor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal major revision
        unReturnValue = TSS_BYTE_Marshal(&PpSource->revMajor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal minor revision
        unReturnValue = TSS_BYTE_Marshal(&PpSource->revMinor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal TPM_STRUCT_VER structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal to.
 *  @param      PprgbBuffer     Pointer to a buffer to read from.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_STRUCT_VER_Unmarshal(
    _Out_   TSS_TPM_STRUCT_VER* PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal major version
        unReturnValue = TSS_BYTE_Unmarshal(&PpTarget->major, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal minor version
        unReturnValue = TSS_BYTE_Unmarshal(&PpTarget->minor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal major revision
        unReturnValue = TSS_BYTE_Unmarshal(&PpTarget->revMajor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal minor revision
        unReturnValue = TSS_BYTE_Unmarshal(&PpTarget->revMinor, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Marshal TPM_KEY structure
 *  @details
 *
 *  @param      PpSource        Pointer to the source to marshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_Marshal(
    _In_    TSS_TPM_KEY*        PpSource,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Marshal Structure version
        unReturnValue = TSS_TPM_STRUCT_VER_Marshal(&PpSource->ver, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal Key usage
        unReturnValue = TSS_TPM_KEY_USAGE_Marshal(&PpSource->keyUsage, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal Key flags
        unReturnValue = TSS_TPM_KEY_FLAGS_Marshal(&PpSource->keyFlags, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal Authentication data usage
        unReturnValue = TSS_TPM_AUTH_DATA_USAGE_Marshal(&PpSource->authDataUsage, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal Algorithm parameters
        unReturnValue = TSS_TPM_KEY_PARMS_Marshal(&PpSource->algorithmParms, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal PCR Info Size
        unReturnValue = TSS_UINT32_Marshal(&PpSource->PCRInfoSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal PCR Info
        // PCRInfo is not marshaled since unbounded keys don't use it
        // Marshal PCR Public key
        unReturnValue = TSS_TPM_STORE_PUBKEY_Marshal(&PpSource->pubKey, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal encryption size
        unReturnValue = TSS_UINT32_Marshal(&PpSource->encSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Marshal encryption data
        unReturnValue = TSS_BYTE_Array_Marshal(&PpSource->encData[0], PprgbBuffer, PpnBufferSize, PpSource->encSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal TPM_KEY structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal to.
 *  @param      PprgbBuffer     Pointer to a buffer to read from.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_KEY_Unmarshal(
    _Out_   TSS_TPM_KEY*        PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // Unmarshal Structure version
        unReturnValue = TSS_TPM_STRUCT_VER_Unmarshal(&PpTarget->ver, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal Key usage
        unReturnValue = TSS_TPM_KEY_USAGE_Unmarshal(&PpTarget->keyUsage, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal Key flags
        unReturnValue = TSS_TPM_KEY_FLAGS_Unmarshal(&PpTarget->keyFlags, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal Authentication data usage
        unReturnValue = TSS_TPM_AUTH_DATA_USAGE_Unmarshal(&PpTarget->authDataUsage, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal Algorithm parameters
        unReturnValue = TSS_TPM_KEY_PARMS_Unmarshal(&PpTarget->algorithmParms, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal PCR Info Size
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->PCRInfoSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal PCR Info
        // PCRInfo is not unmarshalled since unbounded keys don't use it
        // Unmarshal Public key
        unReturnValue = TSS_TPM_STORE_PUBKEY_Unmarshal(&PpTarget->pubKey, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal encryption size
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->encSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        // Unmarshal encryption data
        unReturnValue = TSS_BYTE_Array_Unmarshal(&PpTarget->encData[0], PprgbBuffer, PpnBufferSize, PpTarget->encSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      Unmarshal IFX_FIELDUPGRADEINFO structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal.
 *  @param      PprgbBuffer     Pointer to a buffer to store the source.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BUFFER_TOO_SMALL   In case of vendor specific data buffer is too small.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_IFX_FIELDUPGRADEINFO_Unmarshal(
    _Out_   TSS_IFX_FIELDUPGRADEINFO*   PpTarget,
    _Inout_ TSS_BYTE**                  PprgbBuffer,
    _Inout_ TSS_INT32*                  PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->internal1, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT8_Array_Unmarshal(&(PpTarget->internal2[0]), PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal2));
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_TCPA_VERSION_Unmarshal(&PpTarget->internal3, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT8_Array_Unmarshal(&(PpTarget->internal4[0]), PprgbBuffer, PpnBufferSize, sizeof(PpTarget->internal4));
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal wMaxDataSize
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wMaxDataSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->internal6, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->internal7, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT8_Unmarshal(&PpTarget->internal8, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal wFlagsFieldUpgrade
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->wFlagsFieldUpgrade, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}
/**
 *  @brief      Unmarshal a TPM_DA_INFO or TPM_DA_INFO_LIMITED structure
 *  @details
 *
 *  @param      PpTarget        Pointer to the target to unmarshal to.
 *  @param      PprgbBuffer     Pointer to a buffer to read from.
 *  @param      PpnBufferSize   Size of the buffer.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_FAIL       Invalid tag.
 *  @retval     ...             Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_DA_INFO_Unmarshal(
    _Out_   TSS_TPM_DA_INFO*    PpTarget,
    _Inout_ TSS_BYTE**          PprgbBuffer,
    _Inout_ TSS_INT32*          PpnBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        unReturnValue = TSS_TPM_STRUCTURE_TAG_Unmarshal(&PpTarget->tag, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        if (PpTarget->tag != TSS_TPM_TAG_DA_INFO && PpTarget->tag != TSS_TPM_TAG_DA_INFO_LIMITED)
        {
            unReturnValue = RC_E_FAIL;
            break;
        }
        unReturnValue = TSS_BYTE_Unmarshal(&PpTarget->state, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        if (PpTarget->tag == TSS_TPM_TAG_DA_INFO)
        {
            unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->currentCount, PprgbBuffer, PpnBufferSize);
            if (RC_SUCCESS != unReturnValue)
                break;
            unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->thresholdCount, PprgbBuffer, PpnBufferSize);
            if (RC_SUCCESS != unReturnValue)
                break;
        }
        unReturnValue = TSS_UINT16_Unmarshal(&PpTarget->actionAtThreshold.tag, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->actionAtThreshold.actions, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        if (PpTarget->tag == TSS_TPM_TAG_DA_INFO)
        {
            unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->actionDependValue, PprgbBuffer, PpnBufferSize);
            if (RC_SUCCESS != unReturnValue)
                break;
        }
        unReturnValue = TSS_UINT32_Unmarshal(&PpTarget->vendorDataSize, PprgbBuffer, PpnBufferSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Unmarshal((TSS_BYTE*)PpTarget->vendorData, PprgbBuffer, PpnBufferSize, PpTarget->vendorDataSize);
        if (RC_SUCCESS != unReturnValue)
            break;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}
