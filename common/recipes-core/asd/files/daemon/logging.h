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

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    LogType_MIN = -1,
    LogType_None,
    LogType_IRDR,
    LogType_NETWORK,
    LogType_JTAG,
    LogType_All,
    LogType_Debug,
    LogType_Error,
    LogType_Syslog,
    LogType_MAX,
} ASD_LogType;

#ifdef __cplusplus
extern "C" {
#endif

void ASD_log(ASD_LogType log_type, const char* format, ...);

void ASD_log_buffer(ASD_LogType log_type, const unsigned char* ptr, size_t len,
                    const char* prefixPtr);

void ASD_initialize_log_settings(ASD_LogType type);

#ifdef __cplusplus
}
#endif

#endif  // _LOGGING_H_
