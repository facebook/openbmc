/**
 *  @brief      Implements the command flow to clear the TPM1.2 ownership.
 *  @details    This module removes the TPM owner that was temporarily created during an update from TPM1.2 to TPM1.2.
 *  @file       CommandFlow_Tpm12ClearOwnership.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CommandFlow_Tpm12ClearOwnership.h"
#include "FirmwareUpdate.h"
#include "Crypt.h"
#include "TPM_OIAP.h"
#include "TPM_OSAP.h"
#include "TPM_OwnerClear.h"
#include "Platform.h"

// SHA1 Hash of an owner password 1...8
const TSS_TPM_AUTHDATA C_defaultOwnerAuthData = {
    {
        0x7c, 0x22, 0x2f, 0xb2, 0x92, 0x7d, 0x82, 0x8a,
        0xf2, 0x2f, 0x59, 0x21, 0x34, 0xe8, 0x93, 0x24,
        0x80, 0x63, 0x7c, 0x0d
    }
};

/**
 *  @brief      Processes a sequence of TPM commands to clear the TPM1.2 ownership.
 *  @details    This function removes the TPM owner that was temporarily created during an update from TPM1.2 to TPM1.2.
 *              The function utilizes the MicroTss library.
 *
 *  @param      PpTpmClearOwnership     Pointer to an initialized IfxTpm12ClearOwnership structure to be filled in.
 *                                      Error details are returned by the unReturnCode member:\n
 *                                      RC_E_NOT_SUPPORTED_FEATURE      In case of the TPM is a TPM2.0.\n
 *                                      RC_E_TPM12_NO_OWNER             The TPM1.2 does not have an owner.\n
 *                                      RC_E_NO_IFX_TPM                 The underlying TPM is not an Infineon TPM.\n
 *                                      RC_E_UNSUPPORTED_CHIP           In case of the underlying TPM does not support that functionality.\n
 *                                      RC_E_TPM12_INVALID_OWNERAUTH    In case of the expected owner authorization can not be verified.\n
 *                                      RC_E_FAIL                       An unexpected error occurred.\n
 *                                      ...                             Error codes from called functions.
 *
 *  @retval     RC_SUCCESS                      The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER              An invalid parameter was passed to the function. PpTpmClearOwnership was invalid.
 */
_Check_return_
unsigned int
CommandFlow_Tpm12ClearOwnership_Execute(
    _Inout_ IfxTpm12ClearOwnership* PpTpmClearOwnership)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        // TPM operation mode
        TPM_STATE sTpmState;
        Platform_MemorySet(&sTpmState, 0, sizeof(sTpmState));

        // Parameter check
        if (NULL == PpTpmClearOwnership)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            ERROR_STORE(unReturnValue, L"Bad parameter detected.");
            break;
        }

        // Calculate TPM operational mode
        unReturnValue = FirmwareUpdate_CalculateState(TRUE, &sTpmState);
        if (RC_SUCCESS != unReturnValue)
        {
            ERROR_STORE(unReturnValue, L"FirmwareUpdate_CalculateState returned an unexpected value.");
            break;
        }

        // Check TPM operation mode
        if (sTpmState.attribs.tpm12 && sTpmState.attribs.tpm12owner)
        {
            // Continue...
        }
        else if (sTpmState.attribs.tpm20)
        {
            // TPM2.0
            unReturnValue = RC_E_TPM_NOT_SUPPORTED_FEATURE;
            ERROR_STORE(unReturnValue, L"Detected TPM is a TPM2.0.");
            break;
        }
        else if (sTpmState.attribs.tpm12 && sTpmState.attribs.tpmInOperationalMode)
        {
            // Unowned TPM1.2
            unReturnValue = RC_E_TPM12_NO_OWNER;
            ERROR_STORE(unReturnValue, L"Detected TPM1.2 has no owner.");
            break;
        }
        else if (!sTpmState.attribs.infineon)
        {
            // Unsupported vendor
            unReturnValue = RC_E_NO_IFX_TPM;
            ERROR_STORE(unReturnValue, L"Detected TPM is not an Infineon TPM.");
            break;
        }
        else if (sTpmState.attribs.unsupportedChip)
        {
            // Unsupported TPM1.2 chip
            unReturnValue = RC_E_UNSUPPORTED_CHIP;
            ERROR_STORE(unReturnValue, L"Detected TPM1.2 is not supported.");
            break;
        }
        else
        {
            unReturnValue = RC_E_UNSUPPORTED_CHIP;
            ERROR_STORE(unReturnValue, L"Detected TPM is not in the correct mode.");
            break;
        }

        {
            TSS_TPM_AUTHDATA sAuthData;
            Platform_MemorySet(&sAuthData, 0, sizeof(sAuthData));

            if (!PropertyStorage_ExistsElement(PROPERTY_OWNERAUTH_FILE_PATH))
            {
                // By default use the default owner authorization value (Hash of 1..8 in ASCII format)
                unReturnValue = Platform_MemoryCopy(sAuthData.authdata, sizeof(sAuthData.authdata), C_defaultOwnerAuthData.authdata, sizeof(C_defaultOwnerAuthData.authdata));
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"Unexpected return value from function call Platform_MemoryCopy");
                    break;
                }
            }
            else
            {
                // If option <ownerauth> was provided, read owner authorization from file specified in PROPERTY_OWNERAUTH_FILE_PATH
                wchar_t wszOwnerAuthFilePath[MAX_PATH];
                unsigned int unOwnerAuthFilePathSize = RG_LEN(wszOwnerAuthFilePath);
                BYTE* prgbOwnerAuth = NULL;
                unsigned int unOwnerAuthSize = 0;
                IGNORE_RETURN_VALUE(Platform_StringSetZero(wszOwnerAuthFilePath, RG_LEN(wszOwnerAuthFilePath)));

                if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_OWNERAUTH_FILE_PATH, wszOwnerAuthFilePath, &unOwnerAuthFilePathSize))
                {
                    unReturnValue = RC_E_FAIL;
                    ERROR_STORE_FMT(unReturnValue, L"PropertyStorage_GetValueByKey failed to get property '%ls'.", PROPERTY_OWNERAUTH_FILE_PATH);
                    break;
                }

                unReturnValue = FileIO_ReadFileToBuffer(wszOwnerAuthFilePath, &prgbOwnerAuth, &unOwnerAuthSize);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE_FMT(RC_E_INVALID_OWNERAUTH_OPTION, L"Failed to load the owner authorization file (%ls). (0x%.8X)", wszOwnerAuthFilePath, unReturnValue);
                    unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
                    break;
                }

                // It should be 20 bytes, a SHA-1 hash
                if (unOwnerAuthSize != 20)
                {
                    Platform_MemoryFree((void**)&prgbOwnerAuth);
                    unReturnValue = RC_E_INVALID_OWNERAUTH_OPTION;
                    ERROR_STORE_FMT(unReturnValue, L"Owner authorization should be 20 bytes. It is %d bytes.", unOwnerAuthSize);
                    break;
                }

                unReturnValue = Platform_MemoryCopy(sAuthData.authdata, sizeof(sAuthData.authdata), prgbOwnerAuth, unOwnerAuthSize);
                if (RC_SUCCESS != unReturnValue)
                {
                    Platform_MemoryFree((void**)&prgbOwnerAuth);
                    ERROR_STORE(unReturnValue, L"Unexpected return value from function call Platform_MemoryCopy");
                    break;
                }

                Platform_MemoryFree((void**)&prgbOwnerAuth);
            }

            // Check if owner authorization secret is valid
            unReturnValue = FirmwareUpdate_CheckOwnerAuthorization(sAuthData.authdata);
            if (RC_SUCCESS != unReturnValue)
            {
                ERROR_STORE(unReturnValue, L"FirmwareUpdate_CheckOwnerAuthorization returned an unexpected value.");
                if (TSS_TPM_AUTHFAIL == (unReturnValue ^ RC_TPM_MASK))
                {
                    unReturnValue = RC_E_TPM12_INVALID_OWNERAUTH;
                    ERROR_STORE(unReturnValue, L"The owner authorization secret is invalid. Owner authentication check failed.");
                }
                break;
            }

            // Clear ownership
            {
                TSS_TPM_AUTHHANDLE unAuthHandle = 0;
                TSS_TPM_NONCE sNonceEven;
                Platform_MemorySet(&sNonceEven, 0, sizeof(sNonceEven));

                // Create OIAP Session
                unReturnValue = TSS_TPM_OIAP(&unAuthHandle, &sNonceEven);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"TPM_OIAP command returned an unexpected value");
                    break;
                }

                // Clear TPM1.2 Ownership
                unReturnValue = TSS_TPM_OwnerClear(unAuthHandle, &sNonceEven, FALSE, &sAuthData);
                if (RC_SUCCESS != unReturnValue)
                {
                    ERROR_STORE(unReturnValue, L"TPMOwnerClear returned an unexpected value");
                    break;
                }
            }
        }
    }
    WHILE_FALSE_END;

    if (NULL != PpTpmClearOwnership)
    {
        PpTpmClearOwnership->unReturnCode = unReturnValue;
        unReturnValue = RC_SUCCESS;
    }

    return unReturnValue;
}
