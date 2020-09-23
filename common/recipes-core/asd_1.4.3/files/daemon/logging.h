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

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdbool.h>
#include <stdlib.h>

#define ENABLE_DEBUG_LOGGING
#define CALLBACK_LOG_MESSAGE_LENGTH 256

typedef enum
{
    ASD_LogLevel_Trace = 0,
    ASD_LogLevel_Debug,
    ASD_LogLevel_Info,
    ASD_LogLevel_Warning,
    ASD_LogLevel_Error,
    ASD_LogLevel_Off
} ASD_LogLevel;

static const char* ASD_LogLevelString[] = {"Trace",   "Debug", "Info",
                                           "Warning", "Error", "Off"};

typedef enum
{
    ASD_LogStream_None = 0,
    ASD_LogStream_Network = 1,
    ASD_LogStream_JTAG = 2,
    ASD_LogStream_Pins = 4,
    ASD_LogStream_I2C = 8,
    ASD_LogStream_Test = 16,
    ASD_LogStream_Daemon = 32,
    ASD_LogStream_SDK = 64,
    ASD_LogStream_All = 0xFFFF,
} ASD_LogStream;

typedef enum
{
    ASD_LogOption_None = 0,
    ASD_LogOption_No_Remote = 1
} ASD_LogOption;

extern ASD_LogLevel asd_log_level;
extern ASD_LogStream asd_log_streams;

typedef bool (*ShouldLogFunctionPtr)(ASD_LogLevel, ASD_LogStream);
typedef void (*LogFunctionPtr)(ASD_LogLevel, ASD_LogStream, const char*);

void ASD_log(ASD_LogLevel level, ASD_LogStream stream, ASD_LogOption options,
             const char* format, ...);

void ASD_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                    ASD_LogOption options, const unsigned char* ptr, size_t len,
                    const char* prefixPtr);

void ASD_log_shift(ASD_LogLevel level, ASD_LogStream stream,
                   ASD_LogOption options, unsigned int number_of_bits,
                   unsigned int size_bytes, unsigned char* buffer,
                   const char* prefixPtr);

void ASD_initialize_log_settings(ASD_LogLevel level, ASD_LogStream stream,
                                 bool write_to_syslog,
                                 ShouldLogFunctionPtr should_log_ptr,
                                 LogFunctionPtr log_ptr);

bool strtolevel(char* input, ASD_LogLevel* output);

bool strtostreams(char* input, ASD_LogStream* output);

// maps individual ASD_LogStream flag values to string values.
static inline char* streamtostring(ASD_LogStream stream)
{
    if (stream == ASD_LogStream_None)
        return "None";
    if (stream == ASD_LogStream_Network)
        return "Network";
    if (stream == ASD_LogStream_JTAG)
        return "JTAG";
    if (stream == ASD_LogStream_Pins)
        return "Pins";
    if (stream == ASD_LogStream_I2C)
        return "I2C";
    if (stream == ASD_LogStream_Test)
        return "Test";
    if (stream == ASD_LogStream_Daemon)
        return "Daemon";
    if (stream == ASD_LogStream_SDK)
        return "SDK";
    if (stream == ASD_LogStream_All)
        return "All";
    return "Unknown-Stream";
}

#endif // _LOGGING_H_
