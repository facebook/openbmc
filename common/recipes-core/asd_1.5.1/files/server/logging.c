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

#include "../include/logging.h"

#include <ctype.h>
#include <safe_str_lib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

static bool WriteToSyslog = false;
static ShouldLogFunctionPtr shouldLogCallback = NULL;
static LogFunctionPtr loggingCallback = NULL;
ASD_LogLevel asd_log_level = ASD_LogLevel_Error;
ASD_LogStream asd_log_streams = ASD_LogStream_All;

bool ShouldLog(ASD_LogLevel level, ASD_LogStream stream)
{
    bool log = false;
    if (level >= asd_log_level && (asd_log_streams & stream) != 0)
        log = true;
    return log;
}

void ASD_log(ASD_LogLevel level, ASD_LogStream stream, ASD_LogOption options,
             const char* format, ...)
{
    bool no_remote =
        (options & (uint8_t)ASD_LogOption_No_Remote) == ASD_LogOption_No_Remote;
    bool local_log = ShouldLog(level, stream);
    bool remoteLog = (!no_remote && shouldLogCallback && loggingCallback &&
                      shouldLogCallback(level, stream));
    if (!local_log && !remoteLog)
        return;

    va_list args;
    va_start(args, format);
    if (local_log)
    {
        if (WriteToSyslog)
        {
            vsyslog(LOG_USER, format, args);
        }
        else
        {
            vfprintf(stderr, format, args);
            fprintf(stderr, "\n");
        }
    }
    if (remoteLog)
    {
        char buffer[CALLBACK_LOG_MESSAGE_LENGTH];
        explicit_bzero(buffer, CALLBACK_LOG_MESSAGE_LENGTH);
        snprintf(buffer, CALLBACK_LOG_MESSAGE_LENGTH, format, args);
        loggingCallback(level, stream, buffer);
    }
    va_end(args);
}

void ASD_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                    ASD_LogOption options, const unsigned char* ptr, size_t len,
                    const char* prefixPtr)
{
    const unsigned char* ubuf = ptr;
    unsigned int i = 0, l = 0;
    unsigned char* h;
    char line[256];
    static const unsigned char itoh[] = "0123456789abcdef";

    bool no_remote =
        (options & ASD_LogOption_No_Remote) == ASD_LogOption_No_Remote;
    bool local_log = ShouldLog(level, stream);
    bool remoteLog = (!no_remote && shouldLogCallback && loggingCallback &&
                      shouldLogCallback(level, stream));
    if (!local_log && !remoteLog)
        return;

    /*  0         1         2         3         4         5         6
     *  0123456789012345678901234567890123456789012345678901234567890123456789
     *  PREFIX: 0000000: 0000 0000 0000 0000 0000 0000 0000 0000
     */
    while (i < len)
    {
        explicit_bzero(line, sizeof(line));
        snprintf(line, sizeof(line), "%-6.6s: %07x: ", prefixPtr, i);
        h = (unsigned char*)&line[17];
        for (l = 0; l < 16 && (l + i) < len; l++)
        {
            *h++ = itoh[(*ubuf) >> 4];
            *h++ = itoh[(*ubuf) & 0xf];
            if (l & 1)
                *h++ = ' ';
            ubuf++;
        }
        if (remoteLog)
        {
            loggingCallback(level, stream, line);
        }
        *h = '\n';
        i += l;
        if (local_log)
        {
            if (WriteToSyslog)
                syslog(LOG_USER, "%s", line);
            else
                fprintf(stderr, "%s", line);
        }
    }
}

void buffer_to_hex(unsigned int number_of_bits, unsigned int number_of_bytes,
                   const unsigned char* buffer, unsigned char* result)
{
    static const unsigned char itoh[] = "0123456789abcdef";
    int result_index = (number_of_bytes * 2) - 1;
    unsigned int i = 0;

    int last_bit_mask = (0xff >> (8 - (number_of_bits % 8)));
    if (last_bit_mask != 0 &&
        (buffer[number_of_bytes] & last_bit_mask) >> 4 == 0)
        result_index--;

    for (i = 0; i < number_of_bytes; ++i)
    {
        int bit_mask = 0xff;
        if ((i + 1) == number_of_bytes && number_of_bits % 8 != 0)
        {
            // last byte zero out excess bits
            bit_mask = last_bit_mask;
        }
        result[result_index--] = itoh[(buffer[i] & bit_mask) & 0xf];
        if (result_index >= 0)
            result[result_index--] = itoh[(buffer[i] & bit_mask) >> 4];
    }
}

void ASD_log_shift(ASD_LogLevel level, ASD_LogStream stream,
                   ASD_LogOption options, unsigned int number_of_bits,
                   unsigned int size_bytes, unsigned char* buffer,
                   const char* prefixPtr)
{
    unsigned char* result;
    size_t result_size =
        size_bytes * 2; // each byte will print as two characters
    bool no_remote =
        (options & ASD_LogOption_No_Remote) == ASD_LogOption_No_Remote;
    bool local_log = ShouldLog(level, stream);
    bool remoteLog = (!no_remote && shouldLogCallback && loggingCallback &&
                      shouldLogCallback(level, stream));
    if (!local_log && !remoteLog)
        return;
    if (!buffer || size_bytes == 0 || number_of_bits == 0)
        return;

    unsigned int number_of_bytes = (number_of_bits + 7) / 8;
    if (number_of_bytes > size_bytes)
    {
        number_of_bytes = size_bytes;
        number_of_bits = (number_of_bytes * 8);
    }

    result = (unsigned char*)malloc(result_size + 1);
    if (!result)
    {
        return;
    }
    explicit_bzero(result, result_size + 1);

    buffer_to_hex(number_of_bits, number_of_bytes, buffer, result);
    ASD_log(level, stream, options, "%s: [%db] 0x%s", prefixPtr, number_of_bits,
            result);
    free(result);
}

void ASD_initialize_log_settings(ASD_LogLevel level, ASD_LogStream stream,
                                 bool write_to_syslog,
                                 ShouldLogFunctionPtr should_log_ptr,
                                 LogFunctionPtr log_ptr)
{
    WriteToSyslog = write_to_syslog;
    shouldLogCallback = should_log_ptr;
    loggingCallback = log_ptr;

    asd_log_level = level;
    asd_log_streams = stream;
}

#define STRTOLEVELMAX 10
bool strtolevel(char* input, ASD_LogLevel* output)
{
    char temp[STRTOLEVELMAX];
    int cmp = 0;

    if (input != NULL && output != NULL)
    {
        explicit_bzero(temp, sizeof(char) * STRTOLEVELMAX);
        for (int i = 0; i < strnlen_s(input, STRTOLEVELMAX); i++)
            temp[i] = (char)tolower(input[i]);

        strcmp_s(temp, STRTOLEVELMAX, "off", &cmp);
        if (cmp == 0)
        {
            *output = ASD_LogLevel_Off;
            return true;
        }
        strcmp_s(temp, STRTOLEVELMAX, "trace", &cmp);
        if (cmp == 0)
        {
            *output = ASD_LogLevel_Trace;
            return true;
        }
        strcmp_s(temp, STRTOLEVELMAX, "debug", &cmp);
        if (cmp == 0)
        {
            *output = ASD_LogLevel_Debug;
            return true;
        }
        strcmp_s(temp, STRTOLEVELMAX, "info", &cmp);
        if (cmp == 0)
        {
            *output = ASD_LogLevel_Info;
            return true;
        }
        strcmp_s(temp, STRTOLEVELMAX, "warning", &cmp);
        if (cmp == 0)
        {
            *output = ASD_LogLevel_Warning;
            return true;
        }
        strcmp_s(temp, STRTOLEVELMAX, "error", &cmp);
        if (cmp == 0)
        {
            *output = ASD_LogLevel_Error;
            return true;
        }
    }
    return false;
}

#define STRTOSTREAMMAX 10
// supports multiple comma delimited streams
bool strtostreams(char* input, ASD_LogStream* output)
{
    bool result = false;
    char *string, *token, *original;
    char temp[STRTOSTREAMMAX];
    int cmp = 0;

    if (input != NULL && output != NULL)
    {
        *output = ASD_LogStream_None;
        original = string = strdup(input);
        if (string != NULL)
        {
            while ((token = strsep(&string, ",")) != NULL)
            {
                explicit_bzero(temp, sizeof(char) * STRTOSTREAMMAX);
                for (int i = 0; i < strnlen_s(token, STRTOSTREAMMAX); i++)
                    temp[i] = (char)tolower(token[i]);
                do
                {
                    strcmp_s(temp, STRTOSTREAMMAX, "none", &cmp);
                    if (cmp == 0)
                    {
                        *output |= ASD_LogStream_None;
                        result = true;
                        break;
                    }
                    strcmp_s(temp, STRTOSTREAMMAX, "network", &cmp);
                    if (cmp == 0)
                    {
                        *output |= ASD_LogStream_Network;
                        result = true;
                        break;
                    }
                    strcmp_s(temp, STRTOSTREAMMAX, "jtag", &cmp);
                    if (cmp == 0)
                    {
                        *output |= ASD_LogStream_JTAG;
                        result = true;
                        break;
                    }
                    strcmp_s(temp, STRTOSTREAMMAX, "pins", &cmp);
                    if (cmp == 0)
                    {
                        *output |= ASD_LogStream_Pins;
                        result = true;
                        break;
                    }
                    strcmp_s(temp, STRTOSTREAMMAX, "i2c", &cmp);
                    if (cmp == 0)
                    {
                        *output |= ASD_LogStream_I2C;
                        result = true;
                        break;
                    }
                    strcmp_s(temp, STRTOSTREAMMAX, "test", &cmp);
                    if (cmp == 0)
                    {
                        *output |= ASD_LogStream_Test;
                        result = true;
                        break;
                    }
                    strcmp_s(temp, STRTOSTREAMMAX, "daemon", &cmp);
                    if (cmp == 0)
                    {
                        *output |= ASD_LogStream_Daemon;
                        result = true;
                        break;
                    }
                    strcmp_s(temp, STRTOSTREAMMAX, "sdk", &cmp);
                    if (cmp == 0)
                    {
                        *output |= ASD_LogStream_SDK;
                        result = true;
                        break;
                    }
                    strcmp_s(temp, STRTOSTREAMMAX, "all", &cmp);
                    if (cmp == 0)
                    {
                        *output |= ASD_LogStream_All;
                        result = true;
                        break;
                    }
                    else
                    {
                        result = false;
                    }
                } while (0);
            }
            free(original);
        }
    }
    return result;
}
