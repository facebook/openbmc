/**
 *  @brief      Implements the TPM_FieldUpgrade command.
 *  @details    The module receives the input parameters, marshals these parameters
 *              to a byte array and sends the command to the TPM.
 *  @file       TPM_FieldUpgradeUpdate.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TPM_FieldUpgradeUpdate.h"
#include "TPM_Types.h"
#include "Platform.h"

/**
 *  @brief      Calls TPM_Fieldupgrade.
 *  @details    Transmits the TPM1.2 command TPM_Fieldupgrade with the given data block.
 *
 *  @param      PpbFieldUpgradeBlock        Pointer on data block to be sent.
 *  @param      PunFieldUpgradeBlockSize    Size of data block to be sent in bytes.
 *
 *  @retval     RC_SUCCESS                  The operation completed successfully.
 *  @retval     RC_E_FAIL                   An unexpected error occurred.
 *  @retval     ...                         Error codes from Micro TSS functions.
 */
_Check_return_
unsigned int
TSS_TPM_FieldUpgradeUpdate(
    _In_bytecount_(PunFieldUpgradeBlockSize)    const TSS_BYTE* PpbFieldUpgradeBlock,
    _In_                                        TSS_UINT16      PunFieldUpgradeBlockSize)
{
    unsigned int unReturnValue = RC_E_FAIL;

    do
    {
        TSS_BYTE rgbRequest[TSS_MAX_COMMAND_SIZE];
        TSS_BYTE rgbResponse[TSS_MAX_RESPONSE_SIZE];
        TSS_BYTE* pbBuffer = NULL;
        TSS_INT32 nSizeRemaining = sizeof(rgbRequest);
        TSS_UINT32 unSizeResponse = sizeof(rgbResponse);
        // Request parameters
        TSS_TPM_ST tag = TSS_TPM_TAG_RQU_COMMAND;
        TSS_UINT32 unCommandSize = 0;
        TSS_TPM_CC commandCode = TPM_CC_FieldUpgradeCommand;
        TSS_BYTE* pbLRCStart = NULL;
        SubCmd_d subCommandCode = TPM_FieldUpgradeUpdate;
        TSS_BYTE bLRC = 0;
        // Response parameters
        TSS_UINT32 unResponseSize = 0;
        TSS_TPM_RC responseCode = TSS_TPM_RC_SUCCESS;

        Platform_MemorySet(rgbRequest, 0, sizeof(rgbRequest));
        Platform_MemorySet(rgbResponse, 0, sizeof(rgbResponse));
        // Marshal the request
        pbBuffer = rgbRequest;
        unReturnValue = TSS_TPMI_ST_COMMAND_TAG_Marshal(&tag, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT32_Marshal(&unCommandSize, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_TPM_CC_Marshal(&commandCode, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;
        pbLRCStart = pbBuffer; // Store current position for later LRC calculation
        unReturnValue = TSS_SubCmd_d_Marshal(&subCommandCode, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT16_Marshal(&PunFieldUpgradeBlockSize, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_BYTE_Array_Marshal(PpbFieldUpgradeBlock, &pbBuffer, &nSizeRemaining, PunFieldUpgradeBlockSize);
        if (RC_SUCCESS != unReturnValue)
            break;
        bLRC = TSS_CalcLRC(pbLRCStart, (TSS_UINT32)(pbBuffer - pbLRCStart)); // Calculate LRC beginning from stored location to current pointer position
        unReturnValue = TSS_UINT8_Marshal(&bLRC, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Update command size
        unCommandSize = sizeof(rgbRequest) - nSizeRemaining;
        pbBuffer = rgbRequest + sizeof(tag);
        unReturnValue = TSS_UINT32_Marshal(&unCommandSize, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Transmit the command over TDDL
        unReturnValue = DeviceManagement_Transmit(rgbRequest, unCommandSize, rgbResponse, &unSizeResponse);
        if (TSS_TPM_RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal the response
        pbBuffer = rgbResponse;
        nSizeRemaining = unSizeResponse;
        unReturnValue = TSS_TPM_ST_Unmarshal(&tag, &pbBuffer, &nSizeRemaining);
        if (TSS_TPM_RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT32_Unmarshal(&unResponseSize, &pbBuffer, &nSizeRemaining);
        if (TSS_TPM_RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_TPM_RC_Unmarshal(&responseCode, &pbBuffer, &nSizeRemaining);
        if (TSS_TPM_RC_SUCCESS != unReturnValue)
            break;
        if (responseCode != TSS_TPM_RC_SUCCESS)
        {
            unReturnValue = RC_TPM_MASK | responseCode;
            break;
        }
    }
    WHILE_FALSE_END;

    return unReturnValue;
}
