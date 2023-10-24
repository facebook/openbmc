/**
 *  @brief      Implements the Device access routines via configuration setting DEVICE_PATH
 *  @details    Implements the Device access routines using configuration setting DEVICE_PATH. Default value is /dev/tpm0.
 *  @file       Linux/DeviceAccessTpmDriver.c
 *
 *  Copyright 2016 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "StdInclude.h"
#include "DeviceAccessTpmDriver.h"
#include "Logging.h"
#include "PropertyStorage.h"

#define DEV_TPM "/dev/tpm0"

/**
 *  @brief      Initialize the device access via configuration setting DEVICE_PATH
 *  @details    Default value is /dev/tpm0. If an invalid device path is configured
 *              the tool will return "No connection to the TPM or TPM not found (0xE0295200)".
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_INTERNAL   The operation failed.
 *  @retval     RC_E_NO_TPM     In case of an open call to /dev/tpm0 failed.
 */
_Check_return_
unsigned int
DeviceAccessTpmDriver_Initialize()
{
    unsigned int unReturnValue = RC_E_FAIL;
    LOGGING_WRITE_LEVEL4(L"Using Kernel-Driver");

    do
    {
        UINT32 unFileHandle = 0;
        char szDevicePath[PROPERTY_STORAGE_MAX_VALUE] = {0};
        wchar_t wszDevicePath[PROPERTY_STORAGE_MAX_VALUE] = {0};
        UINT32 unDevicePathSize = RG_LEN(wszDevicePath);

        if (FALSE == PropertyStorage_GetValueByKey(PROPERTY_TPM_DEVICE_ACCESS_PATH, wszDevicePath, &unDevicePathSize))
        {
            unReturnValue = RC_E_INTERNAL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Retrieving PROPERTY_TPM_DEVICE_ACCESS_PATH failed (%.8x).", unReturnValue);
            break;
        }

        unDevicePathSize = wcstombs(szDevicePath, wszDevicePath, unDevicePathSize + 1 );

        unFileHandle = open(szDevicePath, O_RDWR);
        if (unFileHandle == (UINT32) - 1)
        {
            int nErrorNumber = errno;
            if (EBUSY == nErrorNumber)
                unReturnValue = RC_E_DEVICE_ALREADY_IN_USE;
            else if (EACCES == nErrorNumber)
                unReturnValue = RC_E_TPM_ACCESS_DENIED;
            else
                unReturnValue = RC_E_NO_TPM;

            LOGGING_WRITE_LEVEL1_FMT(L"Error: Open device pseudo file %ls failed with errno %d (%s).", wszDevicePath, errno, strerror(errno));
            break;
        }

        if (!PropertyStorage_AddKeyUIntegerValuePair(PROPERTY_DEV_TPM_HANDLE, unFileHandle))
        {
            unReturnValue = RC_E_INTERNAL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Storing device handle failed (%.8x).", unReturnValue);
            break;
        }

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      UnInitialize the device access
 *  @details
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     RC_E_INTERNAL   If the DEV_TPM_HANDLE is not present in the property PropertyStorage.
 *  @retval     RC_E_FAIL       An unexpected error occurred.
 */
_Check_return_
unsigned int
DeviceAccessTpmDriver_Uninitialize()
{
    unsigned int unReturnValue = RC_E_FAIL;
    do
    {
        UINT32 unFileHandle = 0;

        if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_DEV_TPM_HANDLE, &unFileHandle) ||
                0 == unFileHandle)
        {
            unReturnValue = RC_E_INTERNAL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Retrieving device handle failed (%.8x).", unReturnValue);
            break;
        }

        if (close(unFileHandle) == -1)
        {
            unReturnValue = RC_E_FAIL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Close device pseudo file failed with errno %d (%s).", errno, strerror(errno));
            break;
        }

        if (!PropertyStorage_RemoveElement(PROPERTY_DEV_TPM_HANDLE))
            LOGGING_WRITE_LEVEL3_FMT(L"Error: Removing device handle (%.8x).", unReturnValue);

        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}

/**
 *  @brief      TPM transmit function
 *  @details    This function submits the TPM command to the underlying TPM.
 *
 *  @param      PrgbRequestBuffer       Pointer to a byte array containing the TPM command request bytes.
 *  @param      PunRequestBufferSize    Size of command request in bytes.
 *  @param      PrgbResponseBuffer      Pointer to a byte array receiving the TPM command response bytes.
 *  @param      PpunResponseBufferSize  Input size of response buffer, output size of TPM command response in bytes.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_INTERNAL           If the DEV_TPM_HANDLE is not present in the property PropertyStorage.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 */
_Check_return_
unsigned int
DeviceAccessTpmDriver_Transmit(
    _In_bytecount_(PunRequestBufferSize)        const BYTE*     PrgbRequestBuffer,
    _In_                                        unsigned int    PunRequestBufferSize,
    _Out_bytecap_(*PpunResponseBufferSize)      BYTE*           PrgbResponseBuffer,
    _Inout_                                     unsigned int*   PpunResponseBufferSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        UINT32 unFileHandle = 0;
        int nBytes = 0;

        // Check parameters
        if (NULL == PrgbRequestBuffer || NULL == PrgbResponseBuffer || NULL == PpunResponseBufferSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        if (!PropertyStorage_GetUIntegerValueByKey(PROPERTY_DEV_TPM_HANDLE, &unFileHandle) ||
                0 == unFileHandle)
        {
            unReturnValue = RC_E_INTERNAL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Retrieving device handle failed (%.8x).", unReturnValue);
            break;
        }

        nBytes = write(unFileHandle, PrgbRequestBuffer, PunRequestBufferSize);
        if (nBytes == -1 || nBytes != (int)PunRequestBufferSize)
        {
            LOGGING_WRITE_LEVEL1_FMT(L"Error: DeviceAccess_Transmit: Write failed with errno %d (%s).", errno, strerror(errno));
            unReturnValue = RC_E_FAIL;
            break;
        }
        nBytes = read(unFileHandle, PrgbResponseBuffer, *PpunResponseBufferSize);
        if (nBytes == -1)
        {
            LOGGING_WRITE_LEVEL1_FMT(L"Error: DeviceAccess_Transmit: Read failed with errno %d (%s).", errno, strerror(errno));
            unReturnValue = RC_E_FAIL;
            break;
        }
        *PpunResponseBufferSize = nBytes;
        unReturnValue = RC_SUCCESS;
    }
    WHILE_FALSE_END;

    return unReturnValue;
}
