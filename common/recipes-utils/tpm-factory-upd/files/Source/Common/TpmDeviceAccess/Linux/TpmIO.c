/**
 *  @brief      Implements the TPM I/O interface for Linux
 *  @details    Implements the TPM I/O interface for Linux.
 *  @file       Linux/TpmIO.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "StdInclude.h"
#include "TpmIO.h"
#include "Logging.h"
#include "DeviceAccess.h"
#include "DeviceAccessTpmDriver.h"
#include "Platform.h"
#include "TPM_TIS.h"
#include "PropertyStorage.h"

/// Maximum amount of times to retry the TPM command in case the TPM is not responsive.
#define TPM_FU_MAX_RETRIES 5U
/// Default wait time in milliseconds in between two of the above checks.
#define TPM_FU_RETRY_WAIT_TIME 1000
/// Global flag to signalize if module is connected or disconnected
BOOL g_fConnected = 0;
/// Define for locality configuration setting property
#define PROPERTY_LOCALITY               L"Locality"
/// Define for locality request.
/// If TRUE, locality is requested for use once before first TPM command and released after last TPM response.
/// If FALSE, locality is requested before each TPM command and released after each TPM response.
#define PROPERTY_KEEP_LOCALITY_ACTIVE   L"KeepLocalityActive"
/// Flag indicating locality is set or not
BOOL s_fIsLocalitySet = FALSE;

/**
 *  @brief      TPM connect function
 *  @details    This function handles the connect to the underlying TPM.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_ALREADY_CONNECTED      If TPM I/O is already connected.
 *  @retval     RC_E_COMPONENT_NOT_FOUND    No IFX TPM found.
 *  @retval     RC_E_INTERNAL               Unsupported device access or locality setting.
 *  @retval     ...                         Error codes from DeviceAccess_Initialize and TIS.
 */
_Check_return_
unsigned int
TPMIO_Connect()
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        UINT32 unTpmDeviceAccessModeCfg = 0;
        if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, &unTpmDeviceAccessModeCfg))
        {
            unReturnValue = RC_E_INTERNAL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Retrieving PROPERTY_TPM_DEVICE_ACCESS_MODE failed (%.8x).", unReturnValue);
            break;
        }

        // Check if already connected
        if (FALSE != g_fConnected)
        {
            unReturnValue = RC_E_ALREADY_CONNECTED;
            break;
        }

        // Try to connect to the TPM and check return code
        LOGGING_WRITE_LEVEL4(L"Connecting to TPM...");

        switch (unTpmDeviceAccessModeCfg)
        {
            case TPM_DEVICE_ACCESS_DRIVER:
            {
                unReturnValue = DeviceAccessTpmDriver_Initialize();
                if (RC_SUCCESS != unReturnValue)
                {
                    LOGGING_WRITE_LEVEL1_FMT(L"Error initializing LowLevelIO: 0x%.8X", unReturnValue);
                    break;
                }

                LOGGING_WRITE_LEVEL4(L"Using /dev/tpm0 driver");
                break;
            }
#if !(defined (__aarch64__) || defined (__arm__))
            case TPM_DEVICE_ACCESS_MEMORY_BASED:
            {
                unsigned int unLocality = 0;
                BOOL bFlag = FALSE;
                BOOL fKeepLocalityActive = FALSE;

                // Check if already connected
                if (FALSE != g_fConnected)
                {
                    unReturnValue = RC_E_ALREADY_CONNECTED;
                    break;
                }

                // Get the selected locality for TPM access
                if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOCALITY, &unLocality))
                {
                    unReturnValue = RC_E_INTERNAL;
                    break;
                }

                unReturnValue = DeviceAccess_Initialize((BYTE)unLocality);
                if (RC_SUCCESS != unReturnValue)
                {
                    LOGGING_WRITE_LEVEL1_FMT(L"Error initializing LowLevelIO: 0x%.8X", unReturnValue);
                    break;
                }

                LOGGING_WRITE_LEVEL4(L"Using memory access routines");
                LOGGING_WRITE_LEVEL4_FMT(L"Using Locality: %d", unLocality);

                // Check the presence of a TPM first
                // Check whether TPM.ACCESS.VALID
                unReturnValue = TIS_IsAccessValid((BYTE)unLocality, &bFlag);
                if (RC_SUCCESS != unReturnValue)
                {
                    LOGGING_WRITE_LEVEL1_FMT(L"Error TIS access is not valid: 0x%.8X", unReturnValue);
                    break;
                }

                if (!bFlag)
                {
                    unReturnValue = RC_E_NOT_READY;
                    LOGGING_WRITE_LEVEL1_FMT(L"Error TIS is not ready: 0x%.8X", unReturnValue);
                    break;
                }

                // Remember if locality was active at the time when program started. In this case the locality will be restored
                // to initial value after this program disconnects from the TPM.
                unReturnValue = TIS_IsActiveLocality((BYTE)unLocality, &s_fIsLocalitySet);
                if (RC_SUCCESS != unReturnValue)
                {
                    LOGGING_WRITE_LEVEL1_FMT(L"Error Could not check whether locality is active: 0x%.8X", unReturnValue);
                    break;
                }

                // Shall locality be requested only once before first TPM command and release after last TPM response or shall
                // it be requested and released for each TPM command?
                if (FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_KEEP_LOCALITY_ACTIVE, &fKeepLocalityActive))
                {
                    unReturnValue = RC_E_INTERNAL;
                    break;
                }

                if (fKeepLocalityActive)
                {
                    // Request locality once, keep active as long as this program communicates with the TPM.
                    unReturnValue = TIS_RequestUse((BYTE)unLocality);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        LOGGING_WRITE_LEVEL1_FMT(L"Error Could not request locality: 0x%.8X", unReturnValue);
                        break;
                    }

                    TIS_KeepLocalityActive();
                }
                break;
            }
#endif
            default:
            {
                unReturnValue = RC_E_INTERNAL;
                LOGGING_WRITE_LEVEL1_FMT(L"Error: An Unknown or unsupported device access routine is configured (0x%.8x).", unReturnValue);
                break;
            }
        }

        if (RC_SUCCESS != unReturnValue)
            break;

        // Drop root privileges
        if (0 != seteuid(getuid()))
        {
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Seteuid failed with errno %d (%s).", errno, strerror(errno));
            unReturnValue = RC_E_INTERNAL;
            break;
        }
        if (0 != setegid(getgid()))
        {
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Setegid failed with errno %d (%s).", errno, strerror(errno));
            unReturnValue = RC_E_INTERNAL;
            break;
        }

        LOGGING_WRITE_LEVEL4(L"Connected to TPM");
        g_fConnected = TRUE;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      TPM disconnect function
 *  @details    This function handles the disconnect to the underlying TPM.
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_NOT_CONNECTED      If TPM I/O is not connected to the TPM.
 *  @retval     RC_E_INTERNAL           Unsupported device access or locality setting.
 *  @retval     ...                     Error codes from DeviceAccess_Uninitialize function.
 */
_Check_return_
unsigned int
TPMIO_Disconnect()
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        UINT32 unTpmDeviceAccessModeCfg = 0;

        // Check if connected to the TPM
        if (FALSE == g_fConnected)
        {
            unReturnValue = RC_E_NOT_CONNECTED;
            break;
        }

        if (!(  PropertyStorage_GetUIntegerValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, &unTpmDeviceAccessModeCfg) &&
                (TPM_DEVICE_ACCESS_MEMORY_BASED == unTpmDeviceAccessModeCfg ||
                 TPM_DEVICE_ACCESS_DRIVER == unTpmDeviceAccessModeCfg)))
        {
            unReturnValue = RC_E_INTERNAL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Retrieving device handle failed (%.8x).", unReturnValue);
            break;
        }

        // Try to disconnect the TPM and check return code
        LOGGING_WRITE_LEVEL4(L"Disconnecting from TPM...");

        switch (unTpmDeviceAccessModeCfg)
        {
            case TPM_DEVICE_ACCESS_MEMORY_BASED:
            {
                unsigned int unLocality = 0;
                BOOL fKeepLocalityActive = FALSE;

                // Get the selected locality for TPM access
                if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOCALITY, &unLocality))
                {
                    unReturnValue = RC_E_INTERNAL;
                    break;
                }

                // Do we need to release locality after all
                if (FALSE == PropertyStorage_GetBooleanValueByKey(PROPERTY_KEEP_LOCALITY_ACTIVE, &fKeepLocalityActive))
                {
                    unReturnValue = RC_E_INTERNAL;
                    break;
                }

                if (fKeepLocalityActive)
                {
                    unReturnValue = TIS_ReleaseActiveLocality((BYTE)unLocality);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        LOGGING_WRITE_LEVEL1_FMT(L"Error Could not release locality: 0x%.8X", unReturnValue);
                        break;
                    }
                }

                // Request locality again if it was set at the time when this program started.
                if (s_fIsLocalitySet)
                {
                    unReturnValue = TIS_RequestUse((BYTE)unLocality);
                    if (RC_SUCCESS != unReturnValue)
                    {
                        LOGGING_WRITE_LEVEL1_FMT(L"Error Could not request locality: 0x%.8X", unReturnValue);
                        break;
                    }

                    s_fIsLocalitySet = FALSE;
                }

                unReturnValue = DeviceAccess_Uninitialize((BYTE)unLocality);
                if (RC_SUCCESS != unReturnValue)
                    break;

                break;
            }
            case TPM_DEVICE_ACCESS_DRIVER:
            {
                unReturnValue = DeviceAccessTpmDriver_Uninitialize();
                break;
            }
            default:
            {
                unReturnValue = RC_E_INTERNAL;
                LOGGING_WRITE_LEVEL1_FMT(L"Error: Unknown device access mode configured (0x%.8x).", unReturnValue);
                break;
            }
        }

        LOGGING_WRITE_LEVEL4(L"Disconnected from TPM");
        g_fConnected = FALSE;
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

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
 *  @param      PunMaxDuration          The maximum duration of the command in microseconds (relevant for memory based access / TIS protocol only).
 *
 *  @retval     RC_SUCCESS              The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER      An invalid parameter was passed to the function.
 *  @retval     RC_E_NOT_CONNECTED      If the TPM I/O is not connected to the TPM.
 *  @retval     RC_E_INTERNAL           Unsupported device access or locality setting.
 *  @retval     ...                     Error codes from called functions.
 */
_Check_return_
unsigned int
TPMIO_Transmit(
    _In_bytecount_(PunRequestBufferSize)        const BYTE*     PrgbRequestBuffer,
    _In_                                        unsigned int    PunRequestBufferSize,
    _Out_bytecap_(*PpunResponseBufferSize)      BYTE*           PrgbResponseBuffer,
    _Inout_                                     unsigned int*   PpunResponseBufferSize,
    _In_                                        unsigned int    PunMaxDuration)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        UINT32 unTpmDeviceAccessModeCfg = 0;

        // Check parameters
        if (NULL == PrgbRequestBuffer || NULL == PrgbResponseBuffer || NULL == PpunResponseBufferSize)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }
        // Check if connected to the TPM
        if (FALSE == g_fConnected)
        {
            unReturnValue = RC_E_NOT_CONNECTED;
            break;
        }

        if (!(  PropertyStorage_GetUIntegerValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, &unTpmDeviceAccessModeCfg) &&
                (TPM_DEVICE_ACCESS_MEMORY_BASED == unTpmDeviceAccessModeCfg ||
                 TPM_DEVICE_ACCESS_DRIVER == unTpmDeviceAccessModeCfg)))
        {
            unReturnValue = RC_E_INTERNAL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Retrieving device handle failed (%.8x).", unReturnValue);
            break;
        }

        switch (unTpmDeviceAccessModeCfg)
        {
            case TPM_DEVICE_ACCESS_MEMORY_BASED:
            {
                unsigned int unLocality = 0;
                // Get the selected locality for TPM access
                if (FALSE == PropertyStorage_GetUIntegerValueByKey(PROPERTY_LOCALITY, &unLocality))
                {
                    unReturnValue = RC_E_INTERNAL;
                    break;
                }

                unReturnValue = TIS_TransceiveLPC(
                                    (BYTE)unLocality,
                                    PrgbRequestBuffer,
                                    (UINT16)PunRequestBufferSize,
                                    PrgbResponseBuffer,
                                    (UINT16*)PpunResponseBufferSize,
                                    PunMaxDuration);

                if (RC_SUCCESS != unReturnValue)
                    LOGGING_WRITE_LEVEL1(L"Transmission of data via TIS failed!");

                break;
            }
            case TPM_DEVICE_ACCESS_DRIVER:
            {
                unsigned int unRetryCounter = 0;

                for (unRetryCounter = 0; unRetryCounter < TPM_FU_MAX_RETRIES; unRetryCounter++)
                {
                    unReturnValue = DeviceAccessTpmDriver_Transmit(
                                        PrgbRequestBuffer,
                                        (UINT16)PunRequestBufferSize,
                                        PrgbResponseBuffer,
                                        PpunResponseBufferSize);
                    // Retry after some time in case TPM is not responsive
                    if (RC_SUCCESS != unReturnValue)
                    {
                        LOGGING_WRITE_LEVEL1_FMT(L"Error: TPM communication failed with (0x%.8x).", unReturnValue);
                        LOGGING_WRITE_LEVEL1_FMT(L"TPM might not be ready at the moment (Count:%d)", unRetryCounter);
                        Platform_Sleep(TPM_FU_RETRY_WAIT_TIME);
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }

                if (RC_SUCCESS != unReturnValue)
                    LOGGING_WRITE_LEVEL1(L"Transmission of data via /dev/tpm0 failed!");

                break;
            }
            default:
            {
                unReturnValue = RC_E_INTERNAL;
                LOGGING_WRITE_LEVEL1_FMT(L"Error: Unknown device access mode configured (0x%.8x).", unReturnValue);
                break;
            }
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Read a byte from a specific address (register)
 *  @details    This function reads a byte from the specified address
 *
 *  @param      PunRegisterAddress          Register address.
 *  @param      PpbRegisterValue            Pointer to a byte to store the register value.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function.
 *  @retval     RC_E_INTERNAL               Unsupported device access or locality setting (only on Linux and UEFI).
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
TPMIO_ReadRegister(
    _In_        unsigned int        PunRegisterAddress,
    _Inout_     BYTE*               PpbRegisterValue)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        UINT32 unTpmDeviceAccessModeCfg = 0;

        // Check parameters
        if (NULL == PpbRegisterValue)
        {
            unReturnValue = RC_E_BAD_PARAMETER;
            break;
        }

        if (!(  PropertyStorage_GetUIntegerValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, &unTpmDeviceAccessModeCfg) &&
                (TPM_DEVICE_ACCESS_MEMORY_BASED == unTpmDeviceAccessModeCfg ||
                 TPM_DEVICE_ACCESS_DRIVER == unTpmDeviceAccessModeCfg)))
        {
            unReturnValue = RC_E_INTERNAL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Retrieving device handle failed (%.8x).", unReturnValue);
            break;
        }

        switch (unTpmDeviceAccessModeCfg)
        {
            case TPM_DEVICE_ACCESS_MEMORY_BASED:
            {
                // Read byte from register address
                *PpbRegisterValue = DeviceAccess_ReadByte(PunRegisterAddress);
                unReturnValue = RC_SUCCESS;
                break;
            }
            case TPM_DEVICE_ACCESS_DRIVER:
            {
                *PpbRegisterValue = 0;
                unReturnValue = RC_E_INTERNAL;
                LOGGING_WRITE_LEVEL1_FMT(L"Error: Read/Write register is not supported while using the /dev/tpm0 driver (0x%.8x).", unReturnValue);
                break;
            }
            default:
            {
                unReturnValue = RC_E_INTERNAL;
                LOGGING_WRITE_LEVEL1_FMT(L"Error: Unknown device access routine configured (0x%.8x).", unReturnValue);
                break;
            }
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}

/**
 *  @brief      Write a byte to a specific address (register)
 *  @details    This function writes a byte to the specified address
 *
 *  @param      PunRegisterAddress          Register address.
 *  @param      PbRegisterValue             Byte to write to the register address.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_INTERNAL               Unsupported device access or locality setting (only on Linux and UEFI).
 *  @retval     ...                         Error codes from called functions.
 */
_Check_return_
unsigned int
TPMIO_WriteRegister(
    _In_        unsigned int        PunRegisterAddress,
    _In_        BYTE                PbRegisterValue)
{
    unsigned int unReturnValue = RC_E_FAIL;

    LOGGING_WRITE_LEVEL4(LOGGING_METHOD_ENTRY_STRING);

    do
    {
        UINT32 unTpmDeviceAccessModeCfg = 0;

        if (!(  PropertyStorage_GetUIntegerValueByKey(PROPERTY_TPM_DEVICE_ACCESS_MODE, &unTpmDeviceAccessModeCfg) &&
                (TPM_DEVICE_ACCESS_MEMORY_BASED == unTpmDeviceAccessModeCfg ||
                 TPM_DEVICE_ACCESS_DRIVER == unTpmDeviceAccessModeCfg)))
        {
            unReturnValue = RC_E_INTERNAL;
            LOGGING_WRITE_LEVEL1_FMT(L"Error: Retrieving device handle failed (%.8x).", unReturnValue);
            break;
        }

        switch (unTpmDeviceAccessModeCfg)
        {
            case TPM_DEVICE_ACCESS_MEMORY_BASED:
            {
                // Write byte to register address
                DeviceAccess_WriteByte(PunRegisterAddress, PbRegisterValue);
                unReturnValue = RC_SUCCESS;
                break;
            }
            case TPM_DEVICE_ACCESS_DRIVER:
            {
                unReturnValue = RC_E_INTERNAL;
                LOGGING_WRITE_LEVEL1_FMT(L"Error: Read/Write register feature is not supported while using the /dev/tpm0 driver (0x%.8x).", unReturnValue);
                break;
            }
            default:
            {
                unReturnValue = RC_E_INTERNAL;
                LOGGING_WRITE_LEVEL1_FMT(L"Error: Unknown device access routine configured (0x%.8x).", unReturnValue);
                break;
            }
        }
    }
    WHILE_FALSE_END;

    LOGGING_WRITE_LEVEL4_FMT(LOGGING_METHOD_EXIT_STRING_RET_VAL, unReturnValue);

    return unReturnValue;
}
