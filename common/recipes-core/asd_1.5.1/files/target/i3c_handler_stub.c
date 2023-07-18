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

#include "i3c_handler.h"

// This file implements all i3c_handler functions required by ASD in stub
// mode. A template for all interface functions is included but read / write
// operation will always return an error.

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "logging.h"

#define I3C_BUS_ADDRESS_RESERVED 127

static const ASD_LogStream stream = ASD_LogStream_I2C;
static const ASD_LogOption option = ASD_LogOption_None;

static bool i3c_enabled(I3C_Handler* state);
static bool i3c_bus_allowed(I3C_Handler* state, uint8_t bus);
static STATUS i3c_get_dev_name(I3C_Handler* state, uint8_t bus, uint8_t* dev);

I3C_Handler* I3CHandler(bus_config* config)
{
    if (config == NULL)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Invalid config parameter.");
        return NULL;
    }

    I3C_Handler* state = (I3C_Handler*)malloc(sizeof(I3C_Handler));
    if (state == NULL)
    {
        ASD_log(ASD_LogLevel_Error, stream, option,
                "Failed to malloc I3C_Handler.");
    }
    else
    {
        for (int i = 0; i < i3C_MAX_DEV_HANDLERS; i++)
            state->i3c_driver_handlers[i] = UNINITIALIZED_I3C_DRIVER_HANDLE;
        state->config = config;
        state->bus_token = UNINITIALIZED_I3C_BUS_TOKEN;
        state->dbus = NULL;
    }

    return state;
}

STATUS i3c_initialize(I3C_Handler* state)
{
    STATUS status = ST_ERR;
    if (state != NULL && i3c_enabled(state))
    {
        status = ST_OK;
        state->i3c_bus = I3C_BUS_ADDRESS_RESERVED;
    }
    return status;
}

STATUS i3c_deinitialize(I3C_Handler* state)
{
    STATUS status = ST_OK;

    if (state == NULL)
        return ST_ERR;

    return status;
}

STATUS i3c_bus_flock(I3C_Handler* state, uint8_t bus, int op)
{
    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
            "i3c stub - bus %d %s", bus, op == LOCK_EX ? "LOCK" : "UNLOCK");

    if (state == NULL)
        return ST_ERR;

    return ST_OK;
}

STATUS i3c_bus_select(I3C_Handler* state, uint8_t bus)
{
    STATUS status = ST_ERR;
    if (state != NULL && i3c_enabled(state))
    {
        if (bus == state->i3c_bus)
        {
            ASD_log(ASD_LogLevel_Error, stream, option, "Selecting Stub Bus %d",
                    bus);
            status = ST_OK;
        }
        else if (i3c_bus_allowed(state, bus))
        {
            ASD_log(ASD_LogLevel_Error, stream, option, "Selecting Stub Bus %d",
                    bus);
            state->i3c_bus = bus;
            state->config->default_bus = bus;
            status = ST_OK;
        }
        else
        {
            ASD_log(ASD_LogLevel_Error, stream, option, "Bus %d not allowed",
                    bus);
        }
    }
    return status;
}

STATUS i3c_set_sclk(I3C_Handler* state, uint16_t sclk)
{
    if (state == NULL || !i3c_enabled(state))
        return ST_ERR;
    return ST_OK;
}

STATUS i3c_read_write(I3C_Handler* state, void* msg_set)
{
    ASD_log(ASD_LogLevel_Error, stream, option,
            "i3c_read_write stub function, R/W not implemented");

    return ST_ERR;
}

static bool i3c_enabled(I3C_Handler* state)
{
    return state->config->enable_i3c;
}

static bool i3c_bus_allowed(I3C_Handler* state, uint8_t bus)
{
    if (state == NULL)
        return false;
    for (int i = 0; i < MAX_IxC_BUSES; i++)
    {
        if (state->config->bus_config_map[i] == bus &&
            state->config->bus_config_type[i] == BUS_CONFIG_I3C)
            return true;
    }
    return false;
}
