/*
Copyright (c) 2019, Intel Corporation

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

#ifndef __CONFIG_H_
#define __CONFIG_H_

#include "asd_common.h"

typedef enum
{
    JTAG_DRIVER_MODE_SOFTWARE = 0,
    JTAG_DRIVER_MODE_HARDWARE = 1
} JTAG_DRIVER_MODE;

typedef struct jtag_config
{
    // use HW or SW jtag driver.
    JTAG_DRIVER_MODE mode;
    JTAG_CHAIN_SELECT_MODE chain_mode;
} jtag_config;

#define MAX_I2C_BUSES 20
typedef struct i2c_config
{
    bool enable_i2c;
    uint8_t default_bus;
    // if 0th item is false, bus 0 is not allowed
    bool allowed_buses[MAX_I2C_BUSES];
} i2c_config;

typedef struct config
{
    jtag_config jtag;
    remote_logging_config remote_logging;
    i2c_config i2c;
} config;

STATUS set_config_defaults(config* config, const i2c_options* i2c);

#endif // __CONFIG_H_
