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

#ifndef _I3C_HANDLER_H_
#define _I3C_HANDLER_H_

#include "config.h"

#ifdef HAVE_DBUS
#include "dbus_helper.h"
#endif

#define UNINITIALIZED_I3C_DRIVER_HANDLE -1
#define UNINITIALIZED_I3C_BUS_TOKEN -1
#define i3C_MAX_DEV_HANDLERS 16

typedef struct I3C_Handler
{
    uint8_t i3c_bus;
    bus_config* config;
#ifdef HAVE_DBUS
    Dbus_Handle* dbus;
#endif
    int bus_token;
    int i3c_driver_handlers[i3C_MAX_DEV_HANDLERS];
} I3C_Handler;

I3C_Handler* I3CHandler(bus_config* config);
STATUS i3c_initialize(I3C_Handler* state);
STATUS i3c_deinitialize(I3C_Handler* state);
STATUS i3c_bus_flock(I3C_Handler* state, uint8_t bus, int op);
STATUS i3c_bus_select(I3C_Handler* state, uint8_t bus);
STATUS i3c_set_sclk(I3C_Handler* state, uint16_t sclk);
STATUS i3c_read_write(I3C_Handler* state, void* msg_set);

#endif
