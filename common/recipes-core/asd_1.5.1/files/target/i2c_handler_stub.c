/*
Copyright (c) 2021, Intel Corporation

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

// This file implements all i2c_handler functions required by ASD in stub
// mode. A template for all interface functions is included but read / write
// operation will always return an error.

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "i2c_handler.h"
#include "logging.h"

#define I2C_DEV_FILE_NAME "/dev/i2c"
#define MAX_I2C_DEV_FILENAME 256

static const ASD_LogStream stream = ASD_LogStream_I2C;
static const ASD_LogOption option = ASD_LogOption_None;

static bool i2c_enabled(I2C_Handler* state);
static STATUS i2c_open_driver(I2C_Handler* state, uint8_t bus);
static void i2c_close_driver(I2C_Handler* state);
static bool i2c_bus_allowed(I2C_Handler* state, uint8_t bus);

I2C_Handler* I2CHandler(bus_config* config)
{
    if (config == NULL)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "I2C_Handler stub invalid config parameter.");
        return NULL;
    }

    I2C_Handler* state = (I2C_Handler*)malloc(sizeof(I2C_Handler));
    if (state == NULL)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to malloc I2C_Handler stub.");
    }
    else
    {
        state->i2c_driver_handle = UNINITIALIZED_I2C_DRIVER_HANDLE;
        state->config = config;
    }

    return state;
}

STATUS i2c_initialize(I2C_Handler* state)
{
    STATUS status = ST_ERR;
    if (state != NULL && i2c_enabled(state))
    {
        status = i2c_bus_select(state, state->config->default_bus);
    }
    return status;
}

STATUS i2c_deinitialize(I2C_Handler* state)
{
    if (state == NULL)
        return ST_ERR;
    i2c_close_driver(state);
    state = NULL;
    return ST_OK;
}

STATUS i2c_bus_flock(I2C_Handler* state, uint8_t bus, int op)
{
    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
            "i2c stub - bus %d %s", bus, op == LOCK_EX ? "LOCK" : "UNLOCK");
    return ST_OK;
}

STATUS i2c_bus_select(I2C_Handler* state, uint8_t bus)
{
    STATUS status = ST_ERR;
    if (state != NULL && i2c_enabled(state))
    {
        if (bus == state->i2c_bus)
        {
            status = ST_OK;
        }
        else if (i2c_bus_allowed(state, bus))
        {
            i2c_close_driver(state);
            ASD_log(ASD_LogLevel_Error, stream, option, "Selecting Stub Bus %d",
                    bus);
            status = i2c_open_driver(state, bus);
            if (status == ST_OK)
            {
                state->config->default_bus = bus;
            }
        }
        else
        {
            ASD_log(ASD_LogLevel_Error, stream, option, "Bus %d not allowed",
                    bus);
        }
    }
    return status;
}

STATUS i2c_set_sclk(I2C_Handler* state, uint16_t sclk)
{
    if (state == NULL || !i2c_enabled(state))
        return ST_ERR;
    return ST_OK;
}

STATUS i2c_read_write(I2C_Handler* state, void* msg_set)
{
    ASD_log(ASD_LogLevel_Error, stream, option,
            "i2c_read_write stub function, R/W not implemented");
    return ST_ERR;
}

static bool i2c_enabled(I2C_Handler* state)
{
    return state->config->enable_i2c;
}

STATUS i2c_open_driver(I2C_Handler* state, uint8_t bus)
{
    char i2c_dev[MAX_I2C_DEV_FILENAME];
    state->i2c_bus = bus;
    snprintf(i2c_dev, sizeof(i2c_dev), "%s-%d", I2C_DEV_FILE_NAME, bus);
    state->i2c_driver_handle = UNINITIALIZED_I2C_DRIVER_HANDLE;
    ASD_log(ASD_LogLevel_Debug, stream, option,
            "Can't open %s, ASD is running i2c stub mode", i2c_dev);
    return ST_OK;
}

void i2c_close_driver(I2C_Handler* state)
{
    if (state->i2c_driver_handle != UNINITIALIZED_I2C_DRIVER_HANDLE)
    {
        close(state->i2c_driver_handle);
        state->i2c_driver_handle = UNINITIALIZED_I2C_DRIVER_HANDLE;
    }
}

static bool i2c_bus_allowed(I2C_Handler* state, uint8_t bus)
{
    for (int i = 0; i < MAX_IxC_BUSES; i++)
    {
        if (state->config->bus_config_map[i] == bus &&
            state->config->bus_config_type[i] == BUS_CONFIG_I2C)
            return true;
    }
    return false;
}
