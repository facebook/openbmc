/**
 *  @brief      Declares the Device access routines via /dev/tpm0
 *  @details
 *  @file       Linux/DeviceAccessTpmDriver.h
 *
 *  Copyright 2016 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/// Define for the PropertyStorage key Device TPM Handle
#define PROPERTY_DEV_TPM_HANDLE L"DevTpmHandle"

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
DeviceAccessTpmDriver_Initialize();

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
DeviceAccessTpmDriver_Uninitialize();

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
    _Inout_                                     unsigned int*   PpunResponseBufferSize);
