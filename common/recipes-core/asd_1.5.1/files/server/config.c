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

#include "config.h"

#include <stdint.h>
#include <stdlib.h>

#include "logging.h"

STATUS set_config_defaults(config* config, const bus_options* opt)
{
    if (config == NULL || opt == NULL)
    {
        return ST_ERR;
    }
    config->jtag.mode = JTAG_DRIVER_MODE_SOFTWARE;
    config->jtag.chain_mode = JTAG_CHAIN_SELECT_MODE_SINGLE;
    config->remote_logging.logging_level = IPC_LogType_Off;
    config->remote_logging.logging_stream = 0;
    config->buscfg.enable_i2c = opt->enable_i2c;
    config->buscfg.enable_i3c = opt->enable_i3c;
    config->buscfg.default_bus = opt->bus;

    for (int i = 0; i < MAX_IxC_BUSES; i++)
    {
        if (opt->enable_i2c || opt->enable_i3c)
        {
            config->buscfg.bus_config_type[i] = opt->bus_config_type[i];
            config->buscfg.bus_config_map[i] = opt->bus_config_map[i];
        }
        else
        {
            config->buscfg.bus_config_type[i] = BUS_CONFIG_NOT_ALLOWED;
            config->buscfg.bus_config_map[i] = 0;
        }
    }

    return ST_OK;
}
