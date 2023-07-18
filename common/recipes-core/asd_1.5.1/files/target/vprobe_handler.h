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

#ifndef _VPROBE_HANDLER_H_
#define _VPROBE_HANDLER_H_

#include <stdbool.h>

#include "asd_common.h"
#ifdef HAVE_DBUS
#include "dbus_helper.h"
#endif
#include "target_handler.h"

#define MAX_REMOTE_PROBES 8
#define BUFFER_LINE_SIZE 150

typedef struct vProbe_Handler
{
    uint8_t remoteProbes;
    uint8_t remoteConfigs;
    char probesConfig[MAX_DATA_SIZE];
    bool initialized;
#ifdef HAVE_DBUS
    Dbus_Handle* dbus;
#endif
} vProbe_Handler;

vProbe_Handler* vProbeHandler();
STATUS vProbe_initialize(vProbe_Handler* state);
STATUS vProbe_deinitialize(vProbe_Handler* state);
#endif // _VPROBE_HANDLER_H_
