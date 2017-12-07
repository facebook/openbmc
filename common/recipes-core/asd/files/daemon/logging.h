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
    LogType_MIN = 0,
    LogType_None,
    LogType_IRDR,
    LogType_NETWORK,
    LogType_JTAG,
    // no log message should be logged as All.
    LogType_All,
    LogType_Debug,
    LogType_Error,
    LogType_MAX,
    LogType_NoRemote = 0x80  // Special flag to indicate that the message should not be forwarded on to a remote logger.
} ASD_LogType;

typedef bool (*ShouldLogFunctionPtr)(ASD_LogType);
typedef void (*LogFunctionPtr)(ASD_LogType, const char*);

void ASD_log(ASD_LogType log_type, const char* format, ...);

void ASD_log_buffer(ASD_LogType log_type, const unsigned char* ptr, size_t len,
                    const char* prefixPtr);

void ASD_log_shift(const bool is_dr, const bool is_input, const unsigned int number_of_bits,
                   unsigned int size_bytes, unsigned char* buffer);

void ASD_initialize_log_settings(ASD_LogType type, bool b_usesyslog,
                                 ShouldLogFunctionPtr should_log_ptr, LogFunctionPtr log_ptr);

#endif  // _LOGGING_H_
