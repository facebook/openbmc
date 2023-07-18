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

#include "i2c_msg_builder.h"

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <stdlib.h>

#include "logging.h"

static STATUS copy_i2c_to_asd(asd_i2c_msg* asd, struct i2c_msg* i2c);
static STATUS copy_asd_to_i2c(const asd_i2c_msg* asd, struct i2c_msg* i2c);

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
        state->msg_set = malloc(sizeof(struct i2c_rdwr_ioctl_data));
        if (state->msg_set != NULL)
        {
            struct i2c_rdwr_ioctl_data* ioctl_data = state->msg_set;
            ioctl_data->nmsgs = 0;
            ioctl_data->msgs = NULL;
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
        struct i2c_rdwr_ioctl_data* ioctl_data = state->msg_set;
        if (ioctl_data->nmsgs == 0)
        {
            ioctl_data->msgs = (struct i2c_msg*)malloc(sizeof(struct i2c_msg));
        }
        else
        {
            ioctl_data->msgs = (struct i2c_msg*)realloc(
                ioctl_data->msgs,
                (ioctl_data->nmsgs + 1) * sizeof(struct i2c_msg));
        }
        if (ioctl_data->msgs)
        {
            struct i2c_msg* i2c_msg1 = ioctl_data->msgs + ioctl_data->nmsgs;
            status = copy_asd_to_i2c(msg, i2c_msg1);
            if (status == ST_OK)
                ioctl_data->nmsgs++;
        }
    }
    return status;
}

STATUS i2c_msg_get_count(I2C_Msg_Builder* state, uint32_t* count)
{
    STATUS status = ST_ERR;
    if (state != NULL && state->msg_set != NULL && count != NULL)
    {
        struct i2c_rdwr_ioctl_data* ioctl_data = state->msg_set;
        *count = ioctl_data->nmsgs;
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
        struct i2c_rdwr_ioctl_data* ioctl_data = state->msg_set;
        if (ioctl_data->nmsgs > index)
        {
            struct i2c_msg* i2c_msg1 = ioctl_data->msgs + index;
            status = copy_i2c_to_asd(msg, i2c_msg1);
        }
    }
    return status;
}

STATUS i2c_msg_reset(I2C_Msg_Builder* state)
{
    STATUS status = ST_ERR;
    if (state != NULL)
    {
        struct i2c_rdwr_ioctl_data* ioctl_data = state->msg_set;
        for (int i = 0; i < ioctl_data->nmsgs; i++)
        {
            struct i2c_msg* i2c_msg1 = ioctl_data->msgs + i;
            if (i2c_msg1 != NULL && i2c_msg1->buf != NULL)
            {
                free(i2c_msg1->buf);
                i2c_msg1->buf = NULL;
            }
        }
        ioctl_data->nmsgs = 0;
        if (ioctl_data->msgs)
        {
            free(ioctl_data->msgs);
            ioctl_data->msgs = NULL;
        }
        status = ST_OK;
    }
    return status;
}

static STATUS copy_asd_to_i2c(const asd_i2c_msg* asd, struct i2c_msg* i2c)
{
    STATUS status = ST_ERR;
    if (i2c != NULL && asd != NULL)
    {
        i2c->addr = asd->address;
        i2c->len = asd->length;
        i2c->flags = 0;
        if (asd->read)
            i2c->flags |= I2C_M_RD;
        i2c->buf = (__u8*)malloc(i2c->len);
        if (i2c->buf != NULL)
        {
            for (int i = 0; i < i2c->len; i++)
            {
                i2c->buf[i] = asd->buffer[i];
            }
            status = ST_OK;
        }
    }
    return status;
}

static STATUS copy_i2c_to_asd(asd_i2c_msg* asd, struct i2c_msg* i2c)
{
    STATUS status = ST_ERR;

    if (asd != NULL && i2c != NULL)
    {
        asd->address = (uint8_t)i2c->addr;
        asd->length = (uint8_t)i2c->len;
        asd->read = ((i2c->flags & I2C_M_RD) == I2C_M_RD);
        for (int i = 0; i < i2c->len; i++)
        {
            asd->buffer[i] = i2c->buf[i];
        }
        status = ST_OK;
    }
    return status;
}
