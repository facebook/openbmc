/*
Copyright (c) 2022, Intel Corporation

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

#ifndef _ASD_TARGET_API_H_
#define _ASD_TARGET_API_H_

#include "asd_common.h"
#include "logging.h"
#include "config.h"

#define ASD_TARGET_API_VERSION_MAJOR 1
#define ASD_TARGET_API_VERSION_MINOR 0
#define ASD_TARGET_API_VERSION_PATCH 0
#define MAX_TARGET_VERSION_SIZE 12

#define IOCTL_TARGET_GET_API_VERSION           0
#define IOCTL_TARGET_GET_INTERFACE_VERSION     1
#define IOCTL_TARGET_IS_INTERFACE_SUPPORTED    2
#define IOCTL_TARGET_PROCESS_MSG               3
#define IOCTL_TARGET_GET_PIN_FDS               4
#define IOCTL_TARGET_PROCESS_ALL_PIN_EVENTS    5
#define IOCTL_TARGET_PROCESS_PIN_EVENT         6
#define IOCTL_TARGET_SEND_REMOTE_LOG_MSG       7
#define IOCTL_TARGET_GET_I2C_I3C_BUS_CONFIG    8

typedef struct asd_target_events {
    target_fdarr_t fds;
    int num_fds;
} asd_target_events;

typedef struct poll_asd_target_events {
    struct pollfd * poll_fds;
    int num_fds;
} poll_asd_target_events;

typedef struct asd_target_remote_log {
    ASD_LogLevel level;
    ASD_LogStream stream;
    const char* msg;
} asd_target_remote_log;

STATUS asd_target_init(config* asd_cfg);
STATUS asd_target_deinit(void);
size_t asd_target_read(unsigned char* buffer, size_t length, void* opt);  /* unused */
size_t asd_target_write(unsigned char* buffer, size_t length, void* opt); /* unused */
STATUS asd_target_ioctl(void* input, void* output, unsigned int cmd);

void asd_target_log(ASD_LogLevel level, ASD_LogStream stream,
                    ASD_LogOption options, const char* string);
void asd_target_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                           ASD_LogOption options, const unsigned char* ptr,
                           size_t len, const char* prefixPtr);
void asd_target_log_shift(ASD_LogLevel level, ASD_LogStream stream,
                          ASD_LogOption options, unsigned int number_of_bits,
                          unsigned int size_bytes, unsigned char* buffer,
                          const char* prefixPtr);
#endif
