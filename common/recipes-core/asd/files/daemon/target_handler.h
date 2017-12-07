/*
Copyright (c) 2017, Intel Corporation
 
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

#ifndef _TARGET_CONTROL_HANDLER_H_
#define _TARGET_CONTROL_HANDLER_H_
#include "asd/SoftwareJTAGHandler.h"
#include <pthread.h>
#include <stdbool.h>

// ASD Protocol Event IDs
typedef enum {
    ASD_EVENT_PLRSTDEASSRT = 1,
    ASD_EVENT_PLRSTASSERT,
    ASD_EVENT_PRDY_EVENT,
    ASD_EVENT_PWRRESTORE,
    ASD_EVENT_PWRFAIL
} ASD_EVENT;

typedef enum {
    READ_TYPE_MIN = -1,
    READ_TYPE_PROBE = 0,
    READ_TYPE_PIN,
    READ_TYPE_MAX  // Insert new read cfg indices before READ_STATUS_INDEX_MAX
} ReadType;

typedef enum {
    WRITE_CONFIG_MIN = -1,
    WRITE_CONFIG_BREAK_ALL = 0,
    WRITE_CONFIG_RESET_BREAK,
    WRITE_CONFIG_REPORT_PRDY,
    WRITE_CONFIG_REPORT_PLTRST,
    WRITE_CONFIG_REPORT_MBP,
    WRITE_CONFIG_MAX  // Insert before WRITE_EVENT_CONFIG_MAX
} WriteConfig;

typedef enum {
    PRDY_EVENT_DETECTED = 0
} probeStatus;

// These values are based on the Interface.Pins definition in the OpenIPC config xml and the 5.1.4 of the At-Scale spec.
typedef enum {
    PIN_MIN = -1,
    PIN_PWRGOOD = 0,
    PIN_PREQ,
    PIN_RESET_BUTTON,
    PIN_POWER_BUTTON,
    PIN_EARLY_BOOT_STALL,
    PIN_MICROBREAK,
    PIN_PRDY,
    PIN_MAX  // Insert before WRITE_PIN_MAX
} Pin;

typedef enum {
    PIN_EVENT,
    XDP_PRESENT_EVENT
} eventTypes;

typedef struct event_configuration {
    bool break_all;
    bool reset_break;
    bool report_PRDY;
    bool report_PLTRST;
    bool report_MBP;
} event_configuration;

typedef STATUS (*TargetHandlerEventFunctionPtr)(eventTypes, ASD_EVENT);

typedef struct Target_Control_Handle {
    /* FRU being monitored */
    uint8_t fru;

    /* Worker thread monitoring various Pins */
    pthread_t worker_thread;
    bool thread_started;
    bool exit_thread;

    /* Event handling and configuration */
    event_configuration event_cfg;
    pthread_mutex_t write_config_mutex;
    TargetHandlerEventFunctionPtr event_cb;
} Target_Control_Handle;

#ifdef __cplusplus
extern "C" {
#endif

Target_Control_Handle* TargetHandler(uint8_t fru, TargetHandlerEventFunctionPtr event_cb);
STATUS target_initialize(Target_Control_Handle* state);
STATUS target_deinitialize(Target_Control_Handle* state);
STATUS target_write(Target_Control_Handle* state, const Pin pin, const bool assert);
STATUS target_read(Target_Control_Handle* state, const ReadType statusRegister,
                   const uint8_t pin, bool* asserted);
STATUS target_write_event_config(Target_Control_Handle* state, const WriteConfig event_cfg,
                                 const bool enable);
STATUS target_wait_PRDY(Target_Control_Handle* state, const uint8_t log2time);
STATUS target_jtag_chain_select(Target_Control_Handle* state, const uint8_t scan_chain);

#ifdef __cplusplus
}
#endif

#endif  // _TARGET_CONTROL_HANDLER_H_
