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

#ifndef _ASD_SERVER_INTERFACE_H_
#define _ASD_SERVER_INTERFACE_H_

#include "asd_common.h"
#include "logging.h"

#define ASD_SERVER_INTERFACE_API_VERSION_MAJOR 1
#define ASD_SERVER_INTERFACE_API_VERSION_MINOR 0
#define ASD_SERVER_INTERFACE_API_VERSION_PATCH 0
#define MAX_SERVER_INTERFACE_VERSION_SIZE 12

#define IOCTL_SERVER_GET_API_VERSION           0
#define IOCTL_SERVER_GET_INTERFACE_VERSION     1
#define IOCTL_SERVER_IS_INTERFACE_SUPPORTED    2
#define IOCTL_SERVER_IS_DATA_PENDING           3


#define MAX_LOG_SIZE                           120

size_t asd_api_server_read(unsigned char* buffer, size_t length, void* opt);
size_t asd_api_server_write(void* buffer, size_t length, void* opt);
STATUS asd_api_server_ioctl(void* input, void* output, unsigned int cmd);

void asd_api_server_log(ASD_LogLevel level, ASD_LogStream stream,
                        ASD_LogOption options, const char* format, ...);
void asd_api_server_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                               ASD_LogOption options, const unsigned char* ptr,
                               size_t len, const char* prefixPtr);
void asd_api_server_log_shift(ASD_LogLevel level, ASD_LogStream stream,
                              ASD_LogOption options,
                              unsigned int number_of_bits,
                              unsigned int size_bytes, unsigned char* buffer,
                              const char* prefixPtr);
#endif
