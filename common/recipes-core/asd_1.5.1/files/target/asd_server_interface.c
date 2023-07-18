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

#include "asd_server_interface.h"
#include "asd_server_api.h"
#include <safe_str_lib.h>

static STATUS asd_server_interface_version(char* version)
{
    STATUS status = ST_ERR;
    if (version != NULL) {
        if (sprintf_s(version, MAX_SERVER_INTERFACE_VERSION_SIZE, "%d.%d.%d",
                      ASD_SERVER_INTERFACE_API_VERSION_MAJOR,
                      ASD_SERVER_INTERFACE_API_VERSION_MINOR,
                      ASD_SERVER_INTERFACE_API_VERSION_PATCH) > 0) {
            status = ST_OK;
        }
    }
    return status;
}

static bool asd_is_api_server_version_supported(char* version)
{
    bool supported = false;
    int cmp = -1;
    char server_api_version[MAX_SERVER_INTERFACE_VERSION_SIZE];
    STATUS status = ST_ERR;

    if (version != NULL) {
        status = asd_server_ioctl(NULL, server_api_version, IOCTL_SERVER_GET_API_VERSION);
    }
    if (status == ST_OK) {
        strcmp_s(version, MAX_SERVER_INTERFACE_VERSION_SIZE, 
                 server_api_version, &cmp);
        if (cmp == 0) {
            supported = true;
        } else {
              ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                      ASD_LogOption_None,
                      "Incompatible ASD Server API versions");
        }
        ASD_log(ASD_LogLevel_Info, ASD_LogStream_Daemon, ASD_LogOption_None,
                "server_interface_version = %s, server_api_version = %s",
                version, server_api_version);
    }
    return supported;
}

size_t asd_api_server_read(unsigned char* buffer, size_t length, void* opt)
{
    return asd_server_read(buffer, length, opt);
}

size_t asd_api_server_write(void* buffer, size_t length, void* opt)
{
    return asd_server_write(buffer, length, opt);
}

STATUS asd_api_server_ioctl(void* input, void* output, unsigned int cmd)
{
    STATUS status = ST_ERR;
    char * server_interface_version = NULL;
    switch(cmd)
    {
        case IOCTL_SERVER_GET_INTERFACE_VERSION:
            if (output == NULL)
                break;
            server_interface_version = (char *) output;
            status = asd_server_interface_version(server_interface_version);
            break;
        case IOCTL_SERVER_IS_INTERFACE_SUPPORTED:
            if (input == NULL || output == NULL)
                break;
            server_interface_version = (char *) input;
            bool * supported = (bool *) output;
            *supported = asd_is_api_server_version_supported(server_interface_version);
            status = ST_OK;
            break;
        default:
            status = asd_server_ioctl(input, output, cmd);
    }
    return status;
}

void asd_api_server_log(ASD_LogLevel level, ASD_LogStream stream,
                        ASD_LogOption options, const char* format, ...)
{
    char string[MAX_LOG_SIZE];
    va_list args;
    va_start(args, format);
    vsprintf_s(string, sizeof(string), format, args);
    asd_server_log(level, stream, options, string);
    va_end( args );
}

void asd_api_server_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                               ASD_LogOption options, const unsigned char* ptr,
                               size_t len, const char* prefixPtr)
{
    asd_server_log_buffer(level, stream, options, ptr, len, prefixPtr);
}

void asd_api_server_log_shift(ASD_LogLevel level, ASD_LogStream stream,
                              ASD_LogOption options,
                              unsigned int number_of_bits,
                              unsigned int size_bytes, unsigned char* buffer,
                              const char* prefixPtr)
{
    asd_server_log_shift(level, stream, options, number_of_bits, size_bytes,
                         buffer, prefixPtr);
}
