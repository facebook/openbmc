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

#include "asd_common.h"
#include "asd_server_api.h"
#include "asd_main.h"
#include <safe_str_lib.h>

static STATUS asd_server_version(char* version)
{
    STATUS status = ST_ERR;
    if (version != NULL) {
        if (sprintf_s(version, MAX_SERVER_VERSION_SIZE, "%d.%d.%d",
                      ASD_SERVER_API_VERSION_MAJOR,
                      ASD_SERVER_API_VERSION_MINOR,
                      ASD_SERVER_API_VERSION_PATCH) > 0) {
            status = ST_OK;
        }
    }
    return status;
}

size_t asd_server_read(unsigned char* buffer, size_t length, void* opt)
{
    return read_data(buffer, length);
}

size_t asd_server_write(void* buffer, size_t length, void* opt)
{
    if(send_out_msg_on_socket(buffer, length) == ST_OK)
        return length;

    return 0;
}

STATUS asd_server_ioctl(void* input, void* output, unsigned int cmd)
{
    STATUS status = ST_ERR;
    switch(cmd)
    {
        case IOCTL_SERVER_GET_API_VERSION:
            if (output == NULL)
                break;
            char * server_api_version = (char *) output;
            status = asd_server_version(server_api_version);
            break;
        case IOCTL_SERVER_IS_DATA_PENDING:
            if (output == NULL)
                break;
            bool * data_pending = (bool *) output;
            *data_pending = is_data_pending();
            status = ST_OK;
            break;
    }
    return status;
}

void asd_server_log(ASD_LogLevel level, ASD_LogStream stream,
                     ASD_LogOption options, const char* string)
{
    ASD_log(level, stream, options, string);
}

void asd_server_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                            ASD_LogOption options, const unsigned char* ptr,
                            size_t len, const char* prefixPtr)
{
    ASD_log_buffer(level, stream, options, ptr, len, prefixPtr);
}

void asd_server_log_shift(ASD_LogLevel level, ASD_LogStream stream,
                          ASD_LogOption options, unsigned int number_of_bits,
                          unsigned int size_bytes, unsigned char* buffer,
                          const char* prefixPtr)
{
    ASD_log_shift(level, stream, options, number_of_bits, size_bytes, buffer,
                  prefixPtr);
}
