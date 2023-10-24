/**
 *  @brief      Declares the TPM access connection
 *  @details    This module provides the connection to the underlying TPM access module (TpmIO interface).
 *  @file       DeviceManagement.h
 *
 *  Copyright 2013 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "StdInclude.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Caches the last TPM command
extern BYTE g_rgbLastRequest[4096];
/// Caches the last TPM command
extern unsigned int g_unSizeLastRequest;
/// Caches the last TPM command
extern BYTE g_rgbLastResponse[4096];
/// Caches the last TPM response
extern unsigned int g_unSizeLastResponse;

/**
 *  @brief      Represents a TPM command
 *  @details    Structure that holds the code and the name of a TPM command.
 */
typedef struct tdIfxTpmCommand
{
    const wchar_t* pwszCommandName;
    unsigned int unCommandCode;
    unsigned int unMaxDuration;
} IfxTpmCommand;

/**
 *  @brief      Device management initialization function
 *  @details    This function initializes the device IO.
 */
void
DeviceManagement_Initialize();

/**
 *  @brief      Device management uninitialization function
 *  @details    This function uninitializes the device IO.
 */
void
DeviceManagement_Uninitialize();

/**
 *  @brief      Device management IsInitialized function
 *  @details    This function returns the flag signalized a module which was initialized or not
 *
 *  @retval     TRUE    If module is initialized.
 *  @retval     FALSE   If module is not initialized.
 */
_Check_return_
BOOL
DeviceManagement_IsInitialized();

/**
 *  @brief      Device management IsConnected function
 *  @details    This function returns the flag signalized a module which was connected to the TPM or not
 *
 *  @retval     TRUE    If module is connected.
 *  @retval     FALSE   If module is not connected.
 */
_Check_return_
BOOL
DeviceManagement_IsConnected();

/**
 *  @brief      Connect to TPM
 *  @details    This function opens a connection to the underlying TPM access module (TpmIO interface).
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_NOT_INITIALIZED    If this module is not initialized.
 *  @retval     ...                     Error codes from TPMIO_Connect function.
 */
_Check_return_
unsigned int
DeviceManagement_Connect();

/**
 *  @brief      Disconnect from TPM
 *  @details    This function closes the connection to the underlying TPM access module (TpmIO interface).
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_NOT_INITIALIZED    If this module is not initialized.
 *  @retval ...                         Error codes from TPMIO_Disconnect function.
 */
_Check_return_
unsigned int
DeviceManagement_Disconnect();

/**
 *  @brief      Device transmit function
 *  @details    This function submits the TPM command to the underlying TPM access module (TpmIO interface).
 *
 *  @param      PrgbRequestBuffer       Pointer to a byte array containing the TPM command request bytes.
 *  @param      PunRequestBufferSize    Size of command request in bytes.
 *  @param      PrgbResponseBuffer      Pointer to a byte array receiving the TPM command response bytes.
 *  @param      PpunResponseBufferSize  Input size of response buffer, output size of TPM command response in bytes.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. Invalid buffer or buffer size.
 *  @retval     RC_E_NOT_INITIALIZED    The module could not be initialized.
 *  @retval     RC_E_NOT_CONNECTED      The connection to the TPM failed.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval ...                         Error codes from s_fpTpmIoTransmit function.
 */
_Check_return_
unsigned int
DeviceManagement_Transmit(
    _In_bytecount_(PunRequestBufferSize)        const BYTE*     PrgbRequestBuffer,
    _In_                                        unsigned int    PunRequestBufferSize,
    _Out_bytecap_(*PpunResponseBufferSize)      BYTE*           PrgbResponseBuffer,
    _Inout_                                     unsigned int*   PpunResponseBufferSize);

/**
 *  @brief      Function to output TPM command name and return the duration.
 *  @details    This function determines the TPM command name from the command ordinal and puts it to the log file.
 *
 *  @param      PunCommandCode          TPM command ordinal.
 *  @param      PpunMaxDuration         Maximum command duration in microseconds (relevant for memory based access / TIS protocol only).
 */
void
DeviceManagement_TpmCommandName(
    _In_    unsigned int    PunCommandCode,
    _Out_   unsigned int*   PpunMaxDuration);

/**
 *  @brief      Register read function
 *  @details    This function reads a byte from a register address.
 *
 *  @param      PunRegisterAddress      Register address to read from.
 *  @param      PpbRegisterValue        Pointer to a byte to store the register value.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function. Invalid buffer or buffer size.
 *  @retval     RC_E_NOT_INITIALIZED    The module could not be initialized.
 *  @retval     RC_E_NOT_CONNECTED      The connection to the TPM failed.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval ...                         Error codes from s_fpTpmIoReadRegister function.
 */
_Check_return_
unsigned int
DeviceManagement_ReadRegister(
    _In_    unsigned int    PunRegisterAddress,
    _Out_   BYTE*           PpbRegisterValue);

/**
 *  @brief      Register write function
 *  @details    This function writes a byte to a register address.
 *              Validity of the address must be checked by the calling function.
 *
 *  @param      PunRegisterAddress      Register address to write to.
 *  @param      PbRegisterValue         Byte value to write to the register.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_NOT_INITIALIZED    The module could not be initialized.
 *  @retval     RC_E_NOT_CONNECTED      The connection to the TPM failed.
 *  @retval     RC_E_FAIL               An unexpected error occurred.
 *  @retval ...                         Error codes from s_fpTpmIoReadRegister function.
 */
_Check_return_
unsigned int
DeviceManagement_WriteRegister(
    _In_    unsigned int    PunRegisterAddress,
    _In_    BYTE            PbRegisterValue);

#ifdef __cplusplus
}
#endif
