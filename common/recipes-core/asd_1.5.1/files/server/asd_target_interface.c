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

#include "asd_target_interface.h"
#include "asd_target_api.h"
#include <safe_str_lib.h>

static STATUS asd_target_interface_version(char* version)
{
    STATUS status = ST_ERR;
    if (version != NULL) {
        if (sprintf_s(version, MAX_TARGET_INTERFACE_VERSION_SIZE, "%d.%d.%d",
                      ASD_TARGET_INTERFACE_API_VERSION_MAJOR,
                      ASD_TARGET_INTERFACE_API_VERSION_MINOR,
                      ASD_TARGET_INTERFACE_API_VERSION_PATCH) > 0) {
            status = ST_OK;
        }
    }
    return status;
}

static bool asd_is_api_target_version_supported(char* version)
{
    bool supported = false;
    int cmp = -1;
    char target_api_version[MAX_TARGET_INTERFACE_VERSION_SIZE];
    STATUS status = ST_ERR;

    if (version != NULL) {
        status = asd_target_ioctl(NULL, target_api_version, IOCTL_TARGET_GET_API_VERSION);
    }
    if (status == ST_OK) {
        strcmp_s(version, MAX_TARGET_INTERFACE_VERSION_SIZE, 
                 target_api_version, &cmp);
        if (cmp == 0) {
            supported = true;
        } else {
              ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                      ASD_LogOption_None,
                      "Incompatible ASD Target API versions");
        }
        ASD_log(ASD_LogLevel_Info, ASD_LogStream_SDK, ASD_LogOption_None,
                "target_interface_version = %s, target_api_version = %s",
                version, target_api_version);
    }
    return supported;
}

STATUS asd_api_target_init(config* asd_cfg)
{
    // Check API version compatibility
    char target_interface_version[MAX_TARGET_INTERFACE_VERSION_SIZE];
    STATUS status = asd_target_interface_version(target_interface_version);
    if(status == ST_OK) {
        if(asd_is_api_target_version_supported(target_interface_version)) {
            status = asd_target_init(asd_cfg);
        } else {
            status = ST_ERR;
        }
    }
    return status;
}

STATUS asd_api_target_deinit(void)
{
    return asd_target_deinit();
}

size_t asd_api_target_read(unsigned char* buffer, size_t length, void* opt)
{
    return asd_target_read(buffer, length, opt);
}

size_t asd_api_target_write(unsigned char* buffer, size_t length, void* opt)
{
    return asd_target_write(buffer, length, opt);
}

STATUS asd_api_target_ioctl(void* input, void* output, unsigned int cmd)
{
    STATUS status = ST_ERR;
    char * target_interface_version = NULL;
    switch(cmd)
    {
        case IOCTL_TARGET_GET_INTERFACE_VERSION:
            if (output == NULL)
                break;
            target_interface_version = (char *) output;
            status = asd_target_interface_version(target_interface_version);
        break;
        case IOCTL_TARGET_IS_INTERFACE_SUPPORTED:
            if (input == NULL || output == NULL)
                break;
            target_interface_version = (char *) input;
            bool * supported = (bool *) output;
            *supported = asd_is_api_target_version_supported(target_interface_version);
            status = ST_OK;
        default:
            status = asd_target_ioctl(input, output, cmd);
    }
    return status;
}

void asd_api_target_log(ASD_LogLevel level, ASD_LogStream stream,
                     ASD_LogOption options, const char* format, ...)
{
    char string[MAX_LOG_SIZE];
    va_list args;
    va_start(args, format);
    vsprintf_s(string, sizeof(string), format, args);
    asd_target_log(level, stream, options, string);
    va_end( args );
}

void asd_api_target_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                            ASD_LogOption options, const unsigned char* ptr,
                            size_t len, const char* prefixPtr)
{
    asd_target_log_buffer(level, stream, options, ptr, len, prefixPtr);
}

void asd_api_target_log_shift(ASD_LogLevel level, ASD_LogStream stream,
                           ASD_LogOption options, unsigned int number_of_bits,
                           unsigned int size_bytes, unsigned char* buffer,
                           const char* prefixPtr)
{
    asd_target_log_shift(level, stream, options, number_of_bits, size_bytes,
                         buffer, prefixPtr);
}
