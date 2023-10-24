/**
 *  @brief      Implements the TPM_FlushSpecific command.
 *  @details    The module receives the input parameters, marshals these parameters
 *              to a byte array, sends the command to the TPM and unmarshals the response
 *              back to the out parameters.
 *  @file       TPM_FlushSpecific.c
 *
 *  Copyright 2014 - 2022 Infineon Technologies AG ( www.infineon.com )
 *
 *  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "TPM_FlushSpecific.h"
#include "TPM_Marshal.h"
#include "TPM2_Marshal.h"
#include "DeviceManagement.h"
#include "Platform.h"

/**
 *  @brief      This function handles the TPM_FlushSpecific command.
 *  @details    The function receives the input parameters, marshals these parameters
 *              to a byte array, sends the command to the TPM and unmarshals the response
 *              back to the out parameters.
 *
 *  @param      PunHandle           The handle of the item to flush.
 *  @param      PunResourceType     The type of resource that is being flushed.
 *
 *  @retval     RC_SUCCESS          The operation completed successfully.
 *  @retval     ...                 Error codes from called functions.
 */
_Check_return_
unsigned int
TSS_TPM_FlushSpecific(
    _In_ TSS_TPM_AUTHHANDLE     PunHandle,
    _In_ TSS_TPM_RESOURCE_TYPE  PunResourceType)
{
    unsigned int unReturnValue = RC_SUCCESS;
    do
    {
        TSS_BYTE rgbRequest[TSS_MAX_COMMAND_SIZE];
        TSS_BYTE rgbResponse[TSS_MAX_RESPONSE_SIZE];
        TSS_BYTE* pbBuffer = NULL;
        TSS_INT32 nSizeRemaining = sizeof(rgbRequest);
        TSS_INT32 nSizeResponse = sizeof(rgbResponse);
        // Request parameters
        TSS_TPM_TAG tag = TSS_TPM_TAG_RQU_COMMAND;
        TSS_UINT32 unCommandSize = 0;
        TSS_TPM_COMMAND_CODE commandCode = TSS_TPM_ORD_FlushSpecific;
        // Response parameters
        TSS_UINT32 unResponseSize = 0;
        TSS_TPM_RESULT responseCode = TSS_TPM_RC_SUCCESS;

        Platform_MemorySet(rgbRequest, 0, sizeof(rgbRequest));
        Platform_MemorySet(rgbResponse, 0, sizeof(rgbResponse));
        // Marshal the request
        pbBuffer = rgbRequest;
        unReturnValue = TSS_TPM_TAG_Marshal(&tag, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT32_Marshal(&unCommandSize, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_TPM_COMMAND_CODE_Marshal(&commandCode, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT32_Marshal(&PunHandle, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT32_Marshal(&PunResourceType, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Overwrite unCommandSize
        unCommandSize = sizeof(rgbRequest) - nSizeRemaining;
        pbBuffer = rgbRequest + 2;
        nSizeRemaining = 4;
        unReturnValue = TSS_UINT32_Marshal(&unCommandSize, &pbBuffer, &nSizeRemaining);
        if (RC_SUCCESS != unReturnValue)
            break;

        // Transmit the command over TDDL
        unReturnValue = DeviceManagement_Transmit(rgbRequest, unCommandSize, rgbResponse, (unsigned int*)&nSizeResponse);
        if (TSS_TPM_RC_SUCCESS != unReturnValue)
            break;

        // Unmarshal the response
        pbBuffer = rgbResponse;
        nSizeRemaining = nSizeResponse;
        unReturnValue = TSS_TPM_TAG_Unmarshal(&tag, &pbBuffer, &nSizeRemaining);
        if (TSS_TPM_RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_UINT32_Unmarshal(&unResponseSize, &pbBuffer, &nSizeRemaining);
        if (TSS_TPM_RC_SUCCESS != unReturnValue)
            break;
        unReturnValue = TSS_TPM_RESULT_Unmarshal(&responseCode, &pbBuffer, &nSizeRemaining);
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
