/*
Copyright (c) 2020, Intel Corporation

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

#include "vprobe_handler.h"

#include <fcntl.h>
#define HAVE_C99
#include <safe_str_lib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "logging.h"

static const ASD_LogStream stream = ASD_LogStream_JTAG;
static const ASD_LogOption option = ASD_LogOption_None;

vProbe_Handler* vProbeHandler()
{
    vProbe_Handler* state = (vProbe_Handler*)malloc(sizeof(vProbe_Handler));

    if (state == NULL)
    {
        return NULL;
    }

#ifdef HAVE_DBUS
    state->dbus = dbus_helper();
    if (state->dbus == NULL)
    {
        free(state);
        return NULL;
    }
#endif

    state->initialized = false;
    state->remoteProbes = 0;
    state->remoteConfigs = 0;

    return state;
}

STATUS vProbeConfigAppend(char** buff, size_t* destsize, const char* str)
{
    if (destsize == NULL || str == NULL || buff == NULL)
        return ST_ERR;

    uint32_t count = strnlen_s(str, MAX_DATA_SIZE);
    if (strcpy_s(*buff, *destsize, str))
    {
        return ST_ERR;
    }

    *buff += count;
    *destsize = *destsize - count;
    return ST_OK;
}

STATUS vProbe_initialize(vProbe_Handler* state)
{
    int ia[1] = {0};
    uint8_t line[BUFFER_LINE_SIZE];
    size_t cfg_remain_size = 0;
    char* buffer = NULL;
    STATUS result;

    if (state == NULL)
        return ST_ERR;

    cfg_remain_size = sizeof(state->probesConfig);
    buffer = state->probesConfig;

    // TODO: read configuration from dbus

    // Build XML config for remote probes
    result = vProbeConfigAppend(&buffer, &cfg_remain_size,
                                "<ASD_JTAG Enable=\"true\">\r\n");
    if (result != ST_OK)
        return result;

    for (unsigned int i = 0; i < state->remoteConfigs; i++)
    {
        ia[0] = MAX_SCAN_CHAINS + i; // UniqueId
        if (sprintf_s(line, sizeof(line), "<Probe UniqueId=\"%d\">\r\n", ia, 1))
            return ST_ERR;

        result = vProbeConfigAppend(&buffer, &cfg_remain_size, line);
        if (result != ST_OK)
            return result;

        ia[0] = MAX_SCAN_CHAINS + i; // UniqueId
        if (sprintf_s(line, sizeof(line),
                      "<Interface.Jtag Protocol_Value=\"%d\" "
                      "JTAG_Speed=\"1000000\" Divisor=\"1\"/>\r\n",
                      ia, 1))
        {
            return ST_ERR;
        }

        result = vProbeConfigAppend(&buffer, &cfg_remain_size, line);
        if (result != ST_OK)
            return result;

        result = vProbeConfigAppend(&buffer, &cfg_remain_size, "</Probe>\r\n");
        if (result != ST_OK)
            return result;
    }

    result = vProbeConfigAppend(&buffer, &cfg_remain_size, "</ASD_JTAG>\r\n");
    if (result != ST_OK)
        return result;

    *buffer = 0x0; // Null termination caracter
    ASD_log(ASD_LogLevel_Debug, stream, option, "vProbe config:\r\n%s",
            state->probesConfig);

    state->initialized = true;
    return ST_OK;
}

STATUS vProbe_deinitialize(vProbe_Handler* state)
{
    if (state == NULL || !state->initialized)
        return ST_ERR;

#ifdef HAVE_DBUS
    if (state->dbus != NULL)
    {
        dbus_deinitialize(state->dbus);
        free(state->dbus);
        state->dbus = NULL;
    }
#endif

    return ST_OK;
}
