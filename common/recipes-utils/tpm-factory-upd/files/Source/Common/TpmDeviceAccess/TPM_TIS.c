/**
 *  @brief      Implements the TIS related functions
 *  @details
 *  @file       TpmDeviceAccess/TPM_TIS.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TPM_TIS.h"
#include "DeviceAccess.h"
#include "Platform.h"
#include "Logging.h"

/**
 *  @brief      Caches the last read value of the access register. The variable is used for logging purposes.
 */
static BYTE s_bCachedAccessRegister = 0xFF;
/**
 *  @brief      Determines whether the value of s_bCachedAccessRegister could be read.
 */
static BOOL s_fCachedAccessRegisterValid = FALSE;
/**
 *  @brief      Caches the last read value of the status register. The variable is used for logging purposes.
 */
static BYTE s_bCachedStatusRegister = 0xFF;
/**
 *  @brief      Determines whether the value of s_bCachedStatusRegister could be read.
 */
static BOOL s_fCachedStatusRegisterValid = FALSE;

/**
 *  @brief      Determines whether TIS protocol shall request the locality at the TPM before each TPM command.
 */
static BOOL s_fKeepLocality = FALSE;

/**
 *  @brief      Represents a TPM register descriptor
 *  @details    Structure that holds the bit value and the name of a TPM register.
 */
typedef struct tdIfxTpmRegister
{
    const BYTE bBitValue;
    const wchar_t* wszBitName;
} IfxTpmRegister;

/// List of TPM status register bits and their names
static IfxTpmRegister s_sStatusRegister[] =
{
    {0x80, L"stsValid"},
    {0x40, L"commandReady"},
    {0x20, L"tpmGo"},
    {0x10, L"dataAvail"},
    {0x08, L"Expect"},
    {0x04, L"selfTestDone"},
    {0x02, L"responseRetry"},
    {0x01, L"reserved"}
};

/// List of TPM access register bits and their names
static IfxTpmRegister s_sAccessRegister[] =
{
    {0x80, L"tpmRegValidSts"},
    {0x40, L"reserved"},
    {0x20, L"activeLocality"},
    {0x10, L"beenSeized"},
    {0x08, L"Seize"},
    {0x04, L"pendingRequest"},
    {0x02, L"requestUse"},
    {0x01, L"tpmEstablishment"}
};

/**
 *  @brief      The Logging macro used within the TIS layer additionally traces actual values of STS and access register.
 */
#define TIS_LOGGING_WRITE_LEVEL1_FMT(LOGMESSAGE, ...) \
{ \
    LOGGING_WRITE_LEVEL1_FMT(LOGMESSAGE, __VA_ARGS__); \
    if (s_fCachedStatusRegisterValid) \
    { \
        unsigned int unTisIterator = 0; \
        LOGGING_WRITE_LEVEL1_FMT(L"Last known value of TIS status register: (0x%.2x)", s_bCachedStatusRegister); \
        for (unTisIterator = 0; unTisIterator < sizeof(s_sStatusRegister) / sizeof(s_sStatusRegister[0]); unTisIterator++) \
        { \
            if (s_bCachedStatusRegister & s_sStatusRegister[unTisIterator].bBitValue) \
            { \
                LOGGING_WRITE_LEVEL1_FMT(L" %ls", s_sStatusRegister[unTisIterator].wszBitName); \
            } \
        } \
    } \
    else \
    { \
        LOGGING_WRITE_LEVEL1(L"Last known value of TIS status register is not available."); \
    } \
    if (s_fCachedAccessRegisterValid) \
    { \
        unsigned int unTisIterator = 0; \
        LOGGING_WRITE_LEVEL1_FMT(L"Last known value of TIS access register: (0x%.2x)", s_bCachedAccessRegister); \
        for (unTisIterator = 0; unTisIterator < sizeof(s_sAccessRegister) / sizeof(s_sAccessRegister[0]); unTisIterator++) \
        { \
            if (s_bCachedAccessRegister & s_sAccessRegister[unTisIterator].bBitValue) \
            { \
                LOGGING_WRITE_LEVEL1_FMT(L" %ls", s_sAccessRegister[unTisIterator].wszBitName); \
            } \
        } \
    } \
    else \
    { \
        LOGGING_WRITE_LEVEL1(L"Last known value of TIS access register is not available."); \
    } \
} \

/**
 *  @brief      Keep the locality active between TPM commands. If not set, the locality would be released after a TPM response
 *              and requested again before the next TPM command.
 *  @details
 */
void
TIS_KeepLocalityActive()
{
    s_fKeepLocality = TRUE;
}

/**
 *  @brief      Read the value of a TIS register
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PusRegOffset    Register offset.
 *  @param      PbRegSize       Register size.
 *  @param      PpValue         Pointer to the value.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_LOCALITY_NOT_SUPPORTED Given locality is not supported.
 *  @retval     RC_E_BAD_PARAMETER          Invalid register size requested.
 */
_Check_return_
UINT32
TIS_ReadRegister(
    _In_    BYTE    PbLocality,
    _In_    UINT16  PusRegOffset,
    _In_    BYTE    PbRegSize,
    _Out_   void*   PpValue)
{
    UINT32 unReturnCode = RC_SUCCESS;
    UINT32 unAddress = 0;

    do
    {
        if (NULL == PpValue)
        {
            unReturnCode = RC_E_BAD_PARAMETER;
            break;
        }

        switch (PbLocality)
        {
            case TIS_LOCALITY_0:
                unAddress = TIS_LOCALITY0OFFSET;
                break;

            case TIS_LOCALITY_1:
                unAddress = TIS_LOCALITY1OFFSET;
                break;

            case TIS_LOCALITY_2:
                unAddress = TIS_LOCALITY2OFFSET;
                break;

            case TIS_LOCALITY_3:
                unAddress = TIS_LOCALITY3OFFSET;
                break;

            case TIS_LOCALITY_4:
                unAddress = TIS_LOCALITY4OFFSET;
                break;

            default:
                unReturnCode = RC_E_LOCALITY_NOT_SUPPORTED;
        }

        if (RC_SUCCESS != unReturnCode)
        {
            PpValue = NULL;
            break;
        }

        // Calculate effective Address
        unAddress = unAddress | PusRegOffset;

        switch (PbRegSize)
        {
            case sizeof(BYTE):
                *(BYTE*)PpValue = DeviceAccess_ReadByte(unAddress);
                break;

            case sizeof(UINT16):
                *(UINT16*)PpValue = DeviceAccess_ReadWord(unAddress);
                break;

            default:
                // Invalid Register Size requested
                unReturnCode = RC_E_BAD_PARAMETER;
                PpValue = NULL;
        }
    }
    WHILE_FALSE_END;

    return unReturnCode;
}

/**
 *  @brief      Write the value into a TIS register
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PusRegOffset    Register offset.
 *  @param      PbRegSize       Register size.
 *  @param      PunValue        Value to write.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_LOCALITY_NOT_SUPPORTED Given locality is not supported.
 *  @retval     RC_E_BAD_PARAMETER          Invalid register size requested.
 */
_Check_return_
UINT32
TIS_WriteRegister(
    _In_    BYTE    PbLocality,
    _In_    UINT16  PusRegOffset,
    _In_    BYTE    PbRegSize,
    _In_    UINT32  PunValue)
{
    UINT32 unReturnCode = RC_SUCCESS;
    UINT32 unAddress = 0;

    do
    {
        switch (PbLocality)
        {
            case TIS_LOCALITY_0:
                unAddress = TIS_LOCALITY0OFFSET;
                break;

            case TIS_LOCALITY_1:
                unAddress = TIS_LOCALITY1OFFSET;
                break;

            case TIS_LOCALITY_2:
                unAddress = TIS_LOCALITY2OFFSET;
                break;

            case TIS_LOCALITY_3:
                unAddress = TIS_LOCALITY3OFFSET;
                break;

            case TIS_LOCALITY_4:
                unAddress = TIS_LOCALITY4OFFSET;
                break;

            default:
                unReturnCode = RC_E_LOCALITY_NOT_SUPPORTED;
        }

        if (RC_SUCCESS != unReturnCode)
            break;

        // Calculate effective Address
        unAddress = unAddress | PusRegOffset;

        switch (PbRegSize)
        {
            case sizeof(BYTE):
                DeviceAccess_WriteByte(unAddress, (BYTE)PunValue);
                break;

            case sizeof(UINT16):
                DeviceAccess_WriteWord(unAddress, (UINT16)PunValue);
                break;

            default:
                // Invalid Register Size requested
                unReturnCode = RC_E_BAD_PARAMETER;
        }
    }
    WHILE_FALSE_END;

    return unReturnCode;
}

/**
 *  @brief      Read the value from the access register
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PpbValue        Pointer to a byte value.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_COMPONENT_NOT_FOUND    Component not found.
 */
_Check_return_
UINT32
TIS_ReadAccessRegister(
    _In_    BYTE    PbLocality,
    _Out_   BYTE*   PpbValue)
{
    UINT32 unReturnCode = RC_SUCCESS;

    s_fCachedAccessRegisterValid = FALSE;
    unReturnCode = TIS_ReadRegister(PbLocality, TIS_TPM_ACCESS, sizeof(BYTE), PpbValue);

    if (unReturnCode == RC_SUCCESS)
    {
        // Cache the last register value for troubleshooting.
        s_bCachedAccessRegister = *PpbValue;
        s_fCachedAccessRegisterValid = TRUE;

        if (*PpbValue == 0xFF)
            // Access register may never be 0xFF if TPM is working.
            unReturnCode = RC_E_COMPONENT_NOT_FOUND;
    }

    return unReturnCode;
}

/**
 *  @brief      Read the value from the status register.
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PpbValue        Pointer to a byte value.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_LOCALITY_NOT_ACTIVE    The locality is not active.
 *  @retval     ...                         Error codes from TIS_ReadRegister function.
 */
_Check_return_
UINT32
TIS_ReadStsRegister(
    _In_    BYTE    PbLocality,
    _Out_   BYTE*   PpbValue)
{
    UINT32 unReturnCode = RC_SUCCESS;
    BOOL bFlag = FALSE;

    do
    {
        if (NULL == PpbValue)
        {
            unReturnCode = RC_E_BAD_PARAMETER;
            break;
        }

        // Initialize return parameter
        *PpbValue = 0;

        // Check whether the requested locality is active.
        unReturnCode = TIS_IsActiveLocality(PbLocality, &bFlag);
        if (RC_SUCCESS != unReturnCode)
            break;

        if (FALSE == bFlag)
        {
            unReturnCode = RC_E_LOCALITY_NOT_ACTIVE;
            break;
        }

        s_fCachedStatusRegisterValid = FALSE;
        unReturnCode = TIS_ReadRegister(PbLocality, TIS_TPM_STS, sizeof(BYTE), PpbValue);
        if (RC_SUCCESS == unReturnCode)
        {
            // Cache the register value for troubleshooting.
            s_bCachedStatusRegister = *PpbValue;
            s_fCachedStatusRegisterValid = TRUE;
        }
    }
    WHILE_FALSE_END;

    return unReturnCode;
}

/**
 *  @brief      Writes the value into the status register
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PbValue         Byte value to write.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_LOCALITY_NOT_ACTIVE    The locality is not active.
 *  @retval     ...                         Error codes from TIS_WriteRegister function.
 */
_Check_return_
UINT32
TIS_WriteStsRegister(
    _In_    BYTE    PbLocality,
    _In_    BYTE    PbValue)
{
    UINT32 unReturnCode = RC_SUCCESS;
    BOOL bFlag = FALSE;

    do
    {
        // Check whether requested Locality is active
        unReturnCode = TIS_IsActiveLocality(PbLocality, &bFlag);
        if (RC_SUCCESS != unReturnCode)
            break;

        if (FALSE == bFlag)
        {
            unReturnCode = RC_E_LOCALITY_NOT_ACTIVE;
            break;
        }

        unReturnCode = TIS_WriteRegister(PbLocality, TIS_TPM_STS, sizeof(BYTE), (UINT32)PbValue);
    }
    WHILE_FALSE_END;

    return unReturnCode;
}

/**
 *  @brief      Returns the value of TPM.ACCESS.VALID
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PpbFlag         Pointer to a BOOL flag.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from TIS_ReadAccessRegister function.
 */
_Check_return_
UINT32
TIS_IsAccessValid(
    _In_    BYTE    PbLocality,
    _Out_   BOOL    *PpbFlag)
{
    UINT32 unReturnCode = RC_E_FAIL;
    BYTE bValue = 0;

    if (PpbFlag != NULL)
    {
        unReturnCode = TIS_ReadAccessRegister(PbLocality, &bValue);
        if (unReturnCode == RC_SUCCESS)
        {
            if (bValue & TIS_TPM_ACCESS_VALID)
                *PpbFlag = TRUE;
            else
                *PpbFlag = FALSE;
        }
        else
            *PpbFlag = FALSE;
    }
    else
        unReturnCode = RC_E_BAD_PARAMETER;

    return unReturnCode;
}

/**
 *  @brief      Returns the value of TPM.ACCESS.ACTIVE.LOCALITY
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PpbFlag         Pointer to a BOOL flag.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from TIS_ReadAccessRegister function.
 */
_Check_return_
UINT32
TIS_IsActiveLocality(
    _In_    BYTE    PbLocality,
    _Out_   BOOL    *PpbFlag)
{
    UINT32 unReturnCode = RC_E_FAIL;
    BYTE bValue = 0;

    if (PpbFlag != NULL)
    {
        unReturnCode = TIS_ReadAccessRegister(PbLocality, &bValue);
        if (unReturnCode == RC_SUCCESS)
        {
            if (bValue & TIS_TPM_ACCESS_ACTIVELOCALITY)
                *PpbFlag = TRUE;
            else
                *PpbFlag = FALSE;
        }
        else
            *PpbFlag = FALSE;
    }
    else
        unReturnCode = RC_E_BAD_PARAMETER;

    return unReturnCode;
}

/**
 *  @brief      Release the currently active locality
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from TIS_WriteRegister function.
 */
_Check_return_
UINT32
TIS_ReleaseActiveLocality(
    _In_    BYTE    PbLocality)
{
    UINT32 unReturnCode = RC_SUCCESS;

    // To clear the active Locality a 1 must be written to ACCESS.activeLocality
    BYTE bValue = TIS_TPM_ACCESS_ACTIVELOCALITY;
    unReturnCode = TIS_WriteRegister(PbLocality, TIS_TPM_ACCESS, sizeof(BYTE), (UINT32)bValue);

    return unReturnCode;
}

/**
 *  @brief      Requests ownership of the TPM
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from TIS_WriteRegister function.
 */
_Check_return_
UINT32
TIS_RequestUse(
    _In_    BYTE    PbLocality)
{
    UINT32 unReturnCode = RC_E_FAIL;
    BYTE bValue = 0;

    // To request the TPM for a given Locality a 1 must be written to ACCESS.requestUse
    bValue = TIS_TPM_ACCESS_REQUESTUSE;
    unReturnCode = TIS_WriteRegister(PbLocality, TIS_TPM_ACCESS, sizeof(BYTE), (UINT32)bValue);

    return unReturnCode;
}

/**
 *  @brief      Returns the value of TPM.STS.BURSTCOUNT
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PpusBurstCount  Pointer to the Burst Count variable.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_LOCALITY_NOT_ACTIVE    Not an active locality.
 *  @retval     ...                         Error codes from:
 *                                              TIS_IsActiveLocality
 *                                              TIS_ReadRegister function
 */
_Check_return_
UINT32
TIS_GetBurstCount(
    _In_    BYTE    PbLocality,
    _Out_   UINT16* PpusBurstCount)
{
    UINT32 unReturnCode = RC_E_FAIL;
    BOOL bFlag = FALSE;

    do
    {
        if (NULL == PpusBurstCount)
        {
            unReturnCode = RC_E_BAD_PARAMETER;
            break;
        }

        // Initialize return parameter
        *PpusBurstCount = 0;

        // Check whether requested Locality is active
        unReturnCode = TIS_IsActiveLocality(PbLocality, &bFlag);
        if (RC_SUCCESS != unReturnCode)
            break;

        if (FALSE == bFlag)
        {
            unReturnCode = RC_E_LOCALITY_NOT_ACTIVE;
            break;
        }

        unReturnCode = TIS_ReadRegister(PbLocality, TIS_TPM_BURSTCOUNT, sizeof(UINT16), PpusBurstCount);
    }
    WHILE_FALSE_END;

    return unReturnCode;
}

/**
 *  @brief      Returns the value of TPM.STS.COMMAND.READY
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PpbFlag         Pointer to a BOOL flag.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from TIS_ReadStsRegister function.
 */
_Check_return_
UINT32
TIS_IsCommandReady(
    _In_    BYTE    PbLocality,
    _Out_   BOOL    *PpbFlag)
{
    UINT32 unReturnCode = RC_E_FAIL;
    BYTE bValue = 0;

    if (PpbFlag != NULL)
    {
        unReturnCode = TIS_ReadStsRegister(PbLocality, &bValue);
        if (unReturnCode == RC_SUCCESS)
        {
            if (bValue & TIS_TPM_STS_CMDRDY)
                *PpbFlag = TRUE;
            else
                *PpbFlag = FALSE;
        }
        else
            *PpbFlag = FALSE;
    }
    else
        unReturnCode = RC_E_BAD_PARAMETER;

    return unReturnCode;
}

/**
 *  @brief      Aborts the currently running action by writing 1 to Command Ready
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from TIS_WriteStsRegister function.
 */
_Check_return_
UINT32
TIS_Abort(
    _In_    BYTE    PbLocality)
{
    return TIS_WriteStsRegister(PbLocality, TIS_TPM_STS_CMDRDY);
}

/**
 *  @brief      Lets the TPM execute a command
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from TIS_WriteStsRegister function.
 */
_Check_return_
UINT32
TIS_Go(
    _In_    BYTE    PbLocality)
{
    return TIS_WriteStsRegister(PbLocality, TIS_TPM_STS_GO);
}

/**
 *  @brief      Returns the value of TPM.STS.DATA.AVAIL
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PpbFlag         Pointer to a BOOL flag.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from TIS_ReadStsRegister function.
 */
_Check_return_
UINT32
TIS_IsDataAvailable(
    _In_    BYTE    PbLocality,
    _Out_   BOOL    *PpbFlag)
{
    UINT32 unReturnCode = RC_E_FAIL;
    BYTE bValue = 0;

    if (PpbFlag != NULL)
    {
        unReturnCode = TIS_ReadStsRegister(PbLocality, &bValue);
        if (unReturnCode == RC_SUCCESS)
        {
            if ((bValue & TIS_TPM_STS_VALID) && (bValue & TIS_TPM_STS_AVAIL))
                *PpbFlag = TRUE;
            else
                *PpbFlag = FALSE;
        }
        else
            *PpbFlag = FALSE;
    }
    else
        unReturnCode = RC_E_BAD_PARAMETER;

    return unReturnCode;
}

/**
 *  @brief      Lets the TPM repeat the last response
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *
 *  @retval     RC_SUCCESS      The operation completed successfully.
 *  @retval     ...             Error codes from TIS_ReadStsRegister function.
 */
_Check_return_
UINT32
TIS_Retry(
    _In_    BYTE    PbLocality)
{
    return TIS_WriteStsRegister(PbLocality, TIS_TPM_STS_RETRY);
}

/**
 *  @brief      Send data block to the TPM
 *  @details    Send a data block to the TPM TIS data FIFO under consideration of the
 *              TIS communication protocol
 *
 *  @param      PbLocality      Locality value.
 *  @param      PrgbByteBuf     Bytes to send.
 *  @param      PusLen          Length of bytes.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. PrgbByteBuf is NULL.
 *  @retval     RC_E_LOCALITY_NOT_ACTIVE    Locality not active.
 *  @retval     RC_E_TPM_NO_DATA_AVAILABLE  TPM no data available.
 *  @retval     RC_E_NOT_READY              Not ready.
 *  @retval     RC_E_TPM_TRANSMIT_DATA      Error during transmit data.
 *  @retval     ...                         Error codes from:
 *                                              TIS_RequestUse,
 *                                              TIS_IsActiveLocality,
 *                                              TIS_IsCommandReady,
 *                                              TIS_Abort,
 *                                              TIS_GetBurstCount,
 *                                              TIS_WriteRegister,
 *                                              TIS_ReadStsRegister function
 */
_Check_return_
UINT32
TIS_SendLPC(
    _In_                    BYTE        PbLocality,
    _In_bytecount_(PusLen)  const BYTE* PrgbByteBuf,
    _In_                    UINT16      PusLen)
{
    UINT32 unReturnCode = RC_SUCCESS;
    BYTE bValue = 0;
    BYTE i = 0;
    BOOL bFlag = FALSE;
    UINT16 usBurstCount = 0;
    UINT16 usTxSize = 0;
    UINT32 unTimeOut = 0;
    UINT32 unPosition = 0;

    do
    {
        // Check input parameter
        if (NULL == PrgbByteBuf)
        {
            unReturnCode = RC_E_BAD_PARAMETER;
            break;
        }

        usTxSize = PusLen;

        if (!s_fKeepLocality)
        {
            // Request the locality
            unReturnCode = TIS_RequestUse(PbLocality);
            if (RC_SUCCESS != unReturnCode)
                break;
        }

        // Check whether requested Locality is active, timeout after TIMEOUT_A
        unTimeOut = (TIMEOUT_A * 1000) / SLEEP_TIME_US_CR;
        do
        {
            unReturnCode = TIS_IsActiveLocality(PbLocality, &bFlag);
            if (RC_SUCCESS != unReturnCode)
            {
                TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: Failed to test the active locality (0x%.8x)", unReturnCode);
                unTimeOut = 0;  // Stop immediately on error
            }
            else if (TRUE == bFlag)
                unTimeOut = 0;  // Stop immediately if flag is set
            else
            {
                Platform_SleepMicroSeconds(SLEEP_TIME_US_CR);
                unTimeOut = unTimeOut - 1;
                if (0 == unTimeOut)
                {
                    unReturnCode = RC_E_LOCALITY_NOT_ACTIVE;
                    TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: Locality 0x%.2X not active after 750ms (0x%.8x)", PbLocality, unReturnCode);
                }
            }
        }
        while (unTimeOut > 0);
        if (RC_SUCCESS != unReturnCode)
            break;

        // Check the commandReady flag first
        unReturnCode = TIS_IsCommandReady(PbLocality, &bFlag);
        if (FALSE == bFlag)
        {
            unReturnCode = TIS_Abort(PbLocality);
            if (RC_SUCCESS != unReturnCode)
                break;

            // Check whether the TPM can receive a command, timeout after TIMEOUT_B
            unTimeOut = (TIMEOUT_B * 1000) / SLEEP_TIME_US;
            do
            {
                unReturnCode = TIS_IsCommandReady(PbLocality, &bFlag);
                if (RC_SUCCESS != unReturnCode)
                {
                    TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: Failed to read the command ready flag (0x%.8x)", unReturnCode);
                    unTimeOut = 0;  // Stop immediately on error
                }
                else if (TRUE == bFlag)
                    unTimeOut = 0;  // Stop immediately if flag is set
                else
                {
                    Platform_SleepMicroSeconds(SLEEP_TIME_US);
                    unTimeOut = unTimeOut - 1;
                    if (0 == unTimeOut)
                    {
                        unReturnCode = RC_E_NOT_READY;
                        TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: Command ready flag not set after 2000ms (0x%.8x)", unReturnCode);
                    }
                }
            }
            while (unTimeOut > 0);
        }
        if (RC_SUCCESS != unReturnCode)
            break;

        if (PusLen > 9)     // 10 bytes should always be writable
        {
            do
            {
                // Read the BurstCount register, timeout after TIMEOUT_C if it remains 0
                unTimeOut = (TIMEOUT_C * 1000) / SLEEP_TIME_US_BURSTCOUNT;
                do
                {
                    unReturnCode = TIS_GetBurstCount(PbLocality, &usBurstCount);
                    if (RC_SUCCESS != unReturnCode)
                    {
                        TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: Failed to read the burst count (0x%.8x)", unReturnCode);
                        unTimeOut = 0;  // Stop immediately on error
                    }
                    else if (0 < usBurstCount)
                        unTimeOut = 0;
                    else
                    {
                        Platform_SleepMicroSeconds(SLEEP_TIME_US_BURSTCOUNT);
                        unTimeOut = unTimeOut - 1;
                        if (0 == unTimeOut)
                        {
                            unReturnCode = RC_E_NOT_READY;
                            TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: Burst count not > 0 after 750ms. Burst count: 0x%.4X (0x%.8x)", usBurstCount, unReturnCode);
                        }
                    }
                }
                while (unTimeOut > 0);
                if (RC_SUCCESS != unReturnCode)
                    break;

                if (usTxSize > usBurstCount)
                {
                    for (i = 0; i < usBurstCount; i++)
                    {
                        // All OK, now write Byte to the TPM FIFO
                        unReturnCode = TIS_WriteRegister(PbLocality, TIS_TPM_DATA_FIFO, sizeof(BYTE), (UINT32)*&PrgbByteBuf[unPosition]);
                        if (RC_SUCCESS != unReturnCode)
                            break;
                        unPosition++;
                    }
                    usTxSize -= usBurstCount;
                }
                else
                {
                    for (i = 0; i < (usTxSize - 1); i++)
                    {
                        // All OK, now write Byte(s) to the TPM FIFO
                        unReturnCode = TIS_WriteRegister(PbLocality, TIS_TPM_DATA_FIFO, sizeof(BYTE), (UINT32)*&PrgbByteBuf[unPosition]);
                        if (RC_SUCCESS != unReturnCode)
                            break;
                        unPosition++;
                    }
                    usTxSize = 1;
                }
                if (RC_SUCCESS != unReturnCode)
                    break;
            }
            while (usTxSize > 1);

            // Last Byte, check stsValid and Expect, timeout after TIMEOUT_C
            unTimeOut = (TIMEOUT_C * 1000) / SLEEP_TIME_US;
            do
            {
                unReturnCode = TIS_ReadStsRegister(PbLocality, &bValue);
                if (RC_SUCCESS != unReturnCode)
                {
                    TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: Failed to read the STS register before last byte (0x%.8x)", unReturnCode);
                    unTimeOut = 0;  // Stop immediately on error
                }
                else if ((bValue & TIS_TPM_STS_VALID) && (bValue & TIS_TPM_STS_EXPECT))
                    unTimeOut = 0;  // Stop immediately if flag is set
                else
                {
                    Platform_SleepMicroSeconds(SLEEP_TIME_US);
                    unTimeOut = unTimeOut - 1;
                    if (0 == unTimeOut)
                    {
                        unReturnCode = RC_E_TPM_TRANSMIT_DATA;
                        TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: STS register: TPM did not set stsValid and Expect bits after timeout of 750 ms. Register value: 0x%.2X (0x%.8x)", bValue, unReturnCode);
                    }
                }
            }
            while (unTimeOut > 0);
            if (RC_SUCCESS != unReturnCode)
            {
                // Warning C6031 can be suppressed here, since in case of failure we can't do anything and we
                // don't want to override the original error; thus, there is no need to check return value.
#if defined(_MSC_VER)
#pragma warning(suppress: 6031)
#endif
                TIS_Abort(PbLocality);  // Abort TPM in case of an Error
                break;
            }
            // Transmit the last Byte now
            unReturnCode = TIS_WriteRegister(PbLocality, TIS_TPM_DATA_FIFO, sizeof(BYTE), (UINT32)*&PrgbByteBuf[unPosition]);
            if (RC_SUCCESS != unReturnCode)
            {
                // Warning C6031 can be suppressed here, since in case of failure we can't do anything and we do not
                // want to override the original error; thus, there is no need to check return value of TIS_Abort().
#if defined(_MSC_VER)
#pragma warning(suppress: 6031)
#endif
                TIS_Abort(PbLocality);  // Abort TPM in case of an Error
                break;
            }
        }
        else // 10 bytes should always be writable.
        {
            for (i = 0; i < PusLen; i++)
            {
                // All OK, now write Byte to the TPM FIFO
                unReturnCode = TIS_WriteRegister(PbLocality, TIS_TPM_DATA_FIFO, sizeof(BYTE), (UINT32)*&PrgbByteBuf[unPosition]);
                if (RC_SUCCESS != unReturnCode)
                    break;
                unPosition++;
            }
        }

        // After the last Byte, check stsValid=TRUE and Expect=FALSE, timeout after TIMEOUT_C
        unTimeOut = (TIMEOUT_C * 1000) / SLEEP_TIME_US;
        do
        {
            unReturnCode = TIS_ReadStsRegister(PbLocality, &bValue);
            if (RC_SUCCESS != unReturnCode)
            {
                TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: Failed to read the STS register after last byte (0x%.8x)", unReturnCode);
                unTimeOut = 0;  // Stop immediately on error
            }
            else if ((bValue & TIS_TPM_STS_VALID) && (!(bValue & TIS_TPM_STS_EXPECT)))
                unTimeOut = 0;  // Stop immediately if condition is met
            else
            {
                Platform_SleepMicroSeconds(SLEEP_TIME_US);
                unTimeOut = unTimeOut - 1;
                if (0 == unTimeOut)
                {
                    unReturnCode = RC_E_TPM_TRANSMIT_DATA;
                    TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_SendLPC: STS register: TPM did not set stsValid and !Expect bit after timeout of 750 ms. Register value: 0x%.2X (0x%.8x)", bValue, unReturnCode);
                }
            }
        }
        while (unTimeOut > 0);
        if (RC_SUCCESS != unReturnCode)
        {
            // Warning C6031 can be suppressed here, since in case of failure we can't do anything and we
            // don't want to override the original error; thus, there is no need to check return value.
#if defined(_MSC_VER)
#pragma warning(suppress: 6031)
#endif
            TIS_Abort(PbLocality);  // Abort TPM in case of an Error
            break;
        }
        // Successful Transmission, now the command can be executed
        unReturnCode = TIS_Go(PbLocality);
    }
    WHILE_FALSE_END;

    return unReturnCode;
}

/**
 *  @brief      Read a data block from the TPM TIS port
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PrgbByteBuf     Pointer to buffer to store bytes.
 *  @param      PpusLen         Pointer to the length.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_BAD_PARAMETER          An invalid parameter was passed to the function. PrgbByteBuf or PpusLen is NULL.
 *  @retval     RC_E_INSUFFICIENT_BUFFER    Insufficient buffer size.
 *  @retval     RC_E_LOCALITY_NOT_ACTIVE    Locality not active.
 *  @retval     RC_E_TPM_NO_DATA_AVAILABLE  TPM no data available.
 *  @retval     RC_E_TPM_RECEIVE_DATA       Error TPM Receive data.
 *  @retval     RC_E_NOT_READY              Not ready.
 *  @retval     RC_E_TPM_TRANSMIT_DATA      Error during transmit data.
 *  @retval     ...                         Error codes from:
 *                                              TIS_IsActiveLocality,
 *                                              TIS_ReadStsRegister,
 *                                              TIS_GetBurstCount,
 *                                              TIS_ReadRegister,
 *                                              TIS_ReadStsRegister,
 *                                              TIS_IsCommandReady,
 *                                              TIS_Abort,
 *                                              TIS_Retry function
 */
_Check_return_
UINT32
TIS_ReadLPC(
    _In_                    BYTE    PbLocality,
    _Out_bytecap_(*PpusLen) BYTE*   PrgbByteBuf,
    _Inout_                 UINT16* PpusLen)
{
    UINT32 unReturnCode = RC_SUCCESS;
    BYTE bValue = 0;
    BYTE bRetryCount = 0;
    BOOL bFlag = FALSE;
    BOOL bUpdateBytes2Read = FALSE;
    BOOL bRxDone = FALSE;
    UINT16 usBurstCount = 0;
    UINT16 usRxSize = 0;
    UINT16 usBytes2Read = 0;
    UINT32 unTimeOut = 0;
    BYTE *pbRxData = NULL;
    BYTE i = 0;

    do
    {
        do
        {
            // Check input parameters
            if (NULL == PrgbByteBuf || NULL == PpusLen)
            {
                unReturnCode = RC_E_BAD_PARAMETER;
                break;
            }

            *PrgbByteBuf = 0;
            bRxDone = FALSE;
            bRetryCount = 0;
            pbRxData = PrgbByteBuf;
            bUpdateBytes2Read = TRUE;

            // Basic Buffer size check, must be at least 10 Bytes (TAG, Size, Command Code)
            if (*PpusLen < 10)
            {
                unReturnCode = RC_E_INSUFFICIENT_BUFFER;
                bRxDone = TRUE; // It makes no sense to retry if the buffer is too small
                break;
            }

            // Check whether requested Locality is active
            unReturnCode = TIS_IsActiveLocality(PbLocality, &bFlag);
            if (RC_SUCCESS != unReturnCode)
            {
                TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_ReadLPC: ACCESS register cannot be read (0x%.8x)", unReturnCode);
                bRxDone = TRUE; // Retry not reasonable if ACCESS register cannot be read
                break;  // Stop immediately on Error
            }
            if (FALSE == bFlag)
            {
                unReturnCode = RC_E_LOCALITY_NOT_ACTIVE;
                TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_ReadLPC: Requested locality is not available (0x%.8x)", unReturnCode);
                bRxDone = TRUE; // Retry not reasonable if the Locality is not available
                break;
            }

            // Check whether there are already data available
            unReturnCode = TIS_ReadStsRegister(PbLocality, &bValue);
            if (RC_SUCCESS != unReturnCode)
            {
                TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_ReadLPC: STS register cannot be read (0x%.8x)", unReturnCode);
                bRxDone = TRUE; // Retry not reasonable if the STS register cannot be read
                break;  // Stop immediately on Error
            }
            if ((!(bValue & TIS_TPM_STS_VALID)) || (!(bValue & TIS_TPM_STS_AVAIL)))
            {
                unReturnCode = RC_E_TPM_NO_DATA_AVAILABLE;
                TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_ReadLPC: STS register contents are not as expected (0x%.2x)", bValue);
                bRxDone = TRUE; // Retry not reasonable when data are not available
                break;
            }

            // Set initial sizes
            usRxSize = 0;
            usBytes2Read = 10;

            while ((usBytes2Read - usRxSize) > 0)
            {
                // Read the BurstCounter whether there are Bytes in the data FIFO
                unTimeOut = (TIMEOUT_D * 1000) / SLEEP_TIME_US_BURSTCOUNT;
                do
                {
                    unReturnCode = TIS_GetBurstCount(PbLocality, &usBurstCount);
                    if (RC_SUCCESS != unReturnCode)
                        unTimeOut = 0;  // Stop immediately on Error
                    else if (usBurstCount > 0)
                        unTimeOut = 0;
                    else
                    {
                        Platform_SleepMicroSeconds(SLEEP_TIME_US_BURSTCOUNT);
                        unTimeOut = unTimeOut - 1;
                        if (0 == unTimeOut)
                            unReturnCode = RC_E_NOT_READY;
                    }
                }
                while (unTimeOut > 0);
                if (RC_SUCCESS != unReturnCode)
                {
                    bRxDone = FALSE;    // It could make sense to retry
                    break;
                }
                // Set BurstCount to the actual number of Bytes to be read
                if (usBurstCount > (usBytes2Read - usRxSize))
                    usBurstCount = usBytes2Read - usRxSize;

                for (i = 0; i < usBurstCount; i++)
                {
                    unReturnCode = TIS_ReadRegister(PbLocality, TIS_TPM_DATA_FIFO, sizeof(BYTE), pbRxData);
                    if (RC_SUCCESS != unReturnCode)
                        break;

                    pbRxData++;
                }

                if (RC_SUCCESS != unReturnCode)
                {
                    bRxDone = FALSE;    // It could make sense to retry
                    break;
                }

                usRxSize += usBurstCount;

                // Correct the number of Bytes to be read according to the real parameter size if available
                if ((usRxSize > 5) && (bUpdateBytes2Read == TRUE))
                {
                    usBytes2Read = (PrgbByteBuf[4] << 8) + PrgbByteBuf[5];
                    bUpdateBytes2Read = FALSE;
                    // Check for sufficient space in the Buffer now
                    if (*PpusLen < usBytes2Read)
                    {
                        TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_ReadLPC: Insufficient space in the receive buffer (%d, %d)", *PpusLen, usBytes2Read);
                        unReturnCode = RC_E_INSUFFICIENT_BUFFER;
                        bRxDone = TRUE; // It makes no sense to retry if the buffer is too small
                        break;
                    }
                }
            };

            if (RC_SUCCESS != unReturnCode)
                break;

            // All Bytes received, check whether this is indicated by the TPM
            unTimeOut = (TIMEOUT_C * 1000) / SLEEP_TIME_US;
            do
            {
                unReturnCode = TIS_ReadStsRegister(PbLocality, &bValue);
                if (RC_SUCCESS != unReturnCode)
                    unTimeOut = 0;  // Stop immediately on Error
                else if ((bValue & TIS_TPM_STS_VALID) && (!(bValue & TIS_TPM_STS_AVAIL)))
                    unTimeOut = 0;  // Stop immediately if condition is met
                else
                {
                    Platform_SleepMicroSeconds(SLEEP_TIME_US);
                    unTimeOut = unTimeOut - 1;
                    if (0 == unTimeOut)
                        unReturnCode = RC_E_TPM_RECEIVE_DATA;
                }
            }
            while (unTimeOut > 0);
            if (RC_SUCCESS != unReturnCode)
            {
                bRxDone = FALSE;
                break;
            }

            // Finally check the correct data size of the received data
            if (usRxSize != usBytes2Read)
            {
                unReturnCode = RC_E_TPM_RECEIVE_DATA;
                bRxDone = FALSE;
                break;
            }

            // All OK, write CmdRdy to prepare the TPM for the next command and to release the retry buffer
            unReturnCode = TIS_Abort(PbLocality);
            if (RC_SUCCESS != unReturnCode)
            {
                bRxDone = FALSE;
                break;
            }

            *PpusLen = usRxSize;

            bRxDone = TRUE; // Reception really successfully done
        }
        WHILE_FALSE_END;

        if ((RC_SUCCESS != unReturnCode) && (bRxDone == FALSE) && (bRetryCount < MAX_TPM_READ_RETRIES))
        {
            unReturnCode = TIS_Retry(PbLocality);
            if (RC_SUCCESS != unReturnCode)
            {
                TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_ReadLPC: TIS_Retry failed (0x%.8x)", unReturnCode);
                bRxDone = TRUE; // It makes no sense to retry if we have an error here
            }

            bRetryCount++;
        }
        else if ((RC_SUCCESS != unReturnCode) && (bRetryCount >= MAX_TPM_READ_RETRIES))
        {
            // Warning C6031 can be suppressed here, since in case of failure we can't do anything and we
            // don't want to override the original error; thus, there is no need to check return value.
#if defined(_MSC_VER)
#pragma warning(suppress: 6031)
#endif
            TIS_Abort(PbLocality);
            TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_ReadLPC: Exceeded the maximum number of read retries (0x%.8x, %d)", unReturnCode, bRetryCount);
            bRxDone = TRUE; // Max number of errors, abort reception
        }
    }
    while (FALSE == bRxDone);

    // Set received size to 0 in case of an error
    if (RC_SUCCESS != unReturnCode && NULL != PpusLen)
        *PpusLen = 0;

    return unReturnCode;
}

/**
 *  @brief      Sends the Transceive Buffer to the TPM and returns the response
 *  @details
 *
 *  @param      PbLocality      Locality value.
 *  @param      PrgbTxBuffer    Pointer to Transceive buffer.
 *  @param      PusTxLen        Length of the Transceive buffer.
 *  @param      PrgbRxBuffer    Pointer to a Receive buffer.
 *  @param      PpusRxLen       Pointer to the length of the Receive buffer.
 *  @param      PunMaxDuration  The maximum duration of the command in microseconds.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_TPM_NO_DATA_AVAILABLE  TPM no data available.
 *  @retval     ...                         Error codes from:
 *                                              TIS_SendLPC,
 *                                              TIS_IsDataAvailable,
 *                                              TIS_ReadLPC,
 *                                              TIS_ReleaseActiveLocality function
 */
_Check_return_
UINT32
TIS_TransceiveLPC(
    _In_                        BYTE        PbLocality,
    _In_bytecount_(PusTxLen)    const BYTE* PrgbTxBuffer,
    _In_                        UINT16      PusTxLen,
    _Out_bytecap_(*PpusRxLen)   BYTE*       PrgbRxBuffer,
    _Inout_                     UINT16*     PpusRxLen,
    _In_                        UINT32      PunMaxDuration)
{
    UINT32 unReturnCode = RC_SUCCESS;
    UINT16 usRxSize = 0;
    UINT32 unTimeOut = 0;
    BOOL bFlag = FALSE;

    do
    {
        unReturnCode = TIS_SendLPC(PbLocality, PrgbTxBuffer, PusTxLen);
        if (RC_SUCCESS != unReturnCode)
        {
            TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_TransceiveLPC: TIS_SendLPC failed with (0x%.8x)", unReturnCode);
            break;
        }

        // The actual timeout will be higher than PunMaxDuration due to additional time consumed by multiple invocations of
        // the TIS_IsDataAvailable function.
        unTimeOut = PunMaxDuration / SLEEP_TIME_US;
        do
        {
            unReturnCode = TIS_IsDataAvailable(PbLocality, &bFlag);
            if (RC_SUCCESS != unReturnCode)
            {
                TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_TransceiveLPC: TIS_IsDataAvailable failed with (0x%.8x)", unReturnCode);
                unTimeOut = 0;  // Stop immediately on Error
            }
            else if (TRUE == bFlag)
            {
                unTimeOut = 0;  // Stop immediately if Flag is set
            }
            else
            {
                Platform_SleepMicroSeconds(SLEEP_TIME_US);
                unTimeOut = unTimeOut - 1;
                if (0 == unTimeOut)
                {
                    unReturnCode = RC_E_TPM_NO_DATA_AVAILABLE;
                    TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_TransceiveLPC: No data available after timeout of %d microseconds (0x%.8x)", PunMaxDuration, unReturnCode);
                }
            }
        }
        while (unTimeOut > 0);
        if (RC_SUCCESS != unReturnCode)
            break;

        usRxSize = *PpusRxLen;
        unReturnCode = TIS_ReadLPC(PbLocality, PrgbRxBuffer, &usRxSize);
        if (RC_SUCCESS != unReturnCode)
        {
            TIS_LOGGING_WRITE_LEVEL1_FMT(L"Error: TIS_TransceiveLPC: TIS_ReadLPC failed with (0x%.8x)", unReturnCode);
            break;
        }

        *PpusRxLen = usRxSize;

        if (!s_fKeepLocality)
        {
            // Release current Locality
            unReturnCode = TIS_ReleaseActiveLocality(PbLocality);
        }
    }
    WHILE_FALSE_END;

    return unReturnCode;
}
