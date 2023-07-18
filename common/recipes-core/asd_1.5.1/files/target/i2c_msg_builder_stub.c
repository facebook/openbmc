/*
Copyright (c) 2018, Intel Corporation

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Intel Corporation nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// This file implements all i2c_msg_builder functions required by ASD in stub
// mode. A template for all interface functions is included but read / write
// operation will always return an error.

#include <stdint.h>
#include <stdlib.h>

#include "i2c_msg_builder.h"
#include "logging.h"
#define I2C_RDW_IOCTL_DATA_SIZE 8

I2C_Msg_Builder* I2CMsgBuilder()
{
    I2C_Msg_Builder* state = (I2C_Msg_Builder*)malloc(sizeof(I2C_Msg_Builder));
    if (state == NULL)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Network, ASD_LogOption_None,
                "Failed to malloc I2C_Msg_Builder.");
    }
    else
    {
        state->msg_set = NULL;
    }

    return state;
}

STATUS i2c_msg_initialize(I2C_Msg_Builder* state)
{
    STATUS status = ST_ERR;
    if (state != NULL)
    {
        state->msg_set = malloc(I2C_RDW_IOCTL_DATA_SIZE);
        if (state->msg_set != NULL)
        {
            status = ST_OK;
        }
    }
    return status;
}

STATUS i2c_msg_deinitialize(I2C_Msg_Builder* state)
{
    STATUS status = ST_ERR;
    if (state != NULL)
    {
        if (state->msg_set != NULL)
        {
            status = i2c_msg_reset(state);
            if (status == ST_OK)
            {
                free(state->msg_set);
                state->msg_set = NULL;
            }
        }
        status = ST_OK;
    }
    return status;
}

STATUS i2c_msg_add(I2C_Msg_Builder* state, asd_i2c_msg* msg)
{
    STATUS status = ST_ERR;
    if (state != NULL && msg != NULL)
    {
        status = ST_OK;
    }
    return status;
}

STATUS i2c_msg_get_count(I2C_Msg_Builder* state, uint32_t* count)
{
    STATUS status = ST_ERR;
    if (state != NULL && state->msg_set != NULL && count != NULL)
    {
        *count = 0;
        status = ST_OK;
    }
    return status;
}

STATUS i2c_msg_get_asd_i2c_msg(I2C_Msg_Builder* state, uint32_t index,
                               asd_i2c_msg* msg)
{
    STATUS status = ST_ERR;
    if (state != NULL && state->msg_set != NULL && msg != NULL)
    {
        status = ST_OK;
    }
    return status;
}

STATUS i2c_msg_reset(I2C_Msg_Builder* state)
{
    STATUS status = ST_ERR;
    if (state != NULL)
    {
        status = ST_OK;
    }
    return status;
}
