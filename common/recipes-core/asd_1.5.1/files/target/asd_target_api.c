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
#include "asd_target_api.h"
#include "asd_server_interface.h"
#include "asd_msg.h"
#include <safe_str_lib.h>

static STATUS asd_target_process_all_pin_events(const struct pollfd* poll_fds, size_t num_fds);
static STATUS asd_target_process_pin_event(struct pollfd poll_fd);

static STATUS asd_target_version(char* version)
{
    STATUS status = ST_ERR;
    if (version != NULL) {
        if (sprintf_s(version, MAX_TARGET_VERSION_SIZE, "%d.%d.%d",
                      ASD_TARGET_API_VERSION_MAJOR,
                      ASD_TARGET_API_VERSION_MINOR,
                      ASD_TARGET_API_VERSION_PATCH) > 0) {
            status = ST_OK;
        }
    }
    return status;
}

STATUS asd_target_init(config* asd_cfg)
{
    // Check API version compatibility
    char server_interface_version[MAX_TARGET_VERSION_SIZE];
    bool supported = false;
    STATUS status = asd_api_server_ioctl(NULL, server_interface_version,
                                         IOCTL_SERVER_GET_INTERFACE_VERSION);
    if(status == ST_OK) {
        status = asd_api_server_ioctl(server_interface_version, &supported,
                                      IOCTL_SERVER_IS_INTERFACE_SUPPORTED);
    }
    if (status == ST_OK) {
        if(supported) {
            status = asd_msg_init(asd_cfg);
        } else {
            status = ST_ERR;
        }
    }
    return status;
}

STATUS asd_target_deinit(void)
{
    return asd_msg_free();
}

size_t asd_target_read(unsigned char* buffer, size_t length, void* opt)
{
    return 0;
}

size_t asd_target_write(unsigned char* buffer, size_t length, void* opt)
{
    return 0;
}

STATUS asd_target_ioctl(void* input, void* output, unsigned int cmd)
{
    STATUS status = ST_ERR;
    switch(cmd)
    {
        case IOCTL_TARGET_GET_API_VERSION:
            if (output == NULL)
                break;
            char * target_api_version = (char *) output;
            status = asd_target_version(target_api_version);
            break;
        case IOCTL_TARGET_PROCESS_MSG:
            status = asd_msg_read();
            break;
        case IOCTL_TARGET_GET_PIN_FDS:
            if (output == NULL)
                break;
            asd_target_events * pin_events = (asd_target_events *) output;
            status = asd_msg_get_fds(&pin_events->fds, &pin_events->num_fds);
            break;
        case IOCTL_TARGET_PROCESS_ALL_PIN_EVENTS:
            if (input == NULL)
                break;
            poll_asd_target_events * poll_events = (poll_asd_target_events *)input;
            status = asd_target_process_all_pin_events(poll_events->poll_fds, poll_events->num_fds);
            break;
        case IOCTL_TARGET_PROCESS_PIN_EVENT:
            if (input == NULL)
                break;
            struct pollfd * poll_fd = (struct pollfd *)input;
            status = asd_target_process_pin_event(*poll_fd);
            break;
        case IOCTL_TARGET_SEND_REMOTE_LOG_MSG:
            if (input == NULL)
                break;
            asd_target_remote_log * remote_log = (struct pollfd *)input;
            send_remote_log_message(remote_log->level, remote_log->stream, remote_log->msg);
            status = ST_OK;
            break;
        case IOCTL_TARGET_GET_I2C_I3C_BUS_CONFIG:
            if (output == NULL)
                break;
            bus_options * target_bus_options = (bus_options *)output;
            status = target_get_i2c_i3c_config(target_bus_options);
            break;
    }
    return status;
}

static STATUS asd_target_process_all_pin_events(const struct pollfd* poll_fds, size_t num_fds)
{
    STATUS result = ST_OK;

    for (int i = 0; i < num_fds; i++)
    {
        result = asd_target_process_pin_event(poll_fds[i]);
        if (result != ST_OK)
            break;
    }
    return result;
}

static STATUS asd_target_process_pin_event(struct pollfd poll_fd)
{
    return asd_msg_event(poll_fd);
}

void asd_target_log(ASD_LogLevel level, ASD_LogStream stream,
                    ASD_LogOption options, const char* string)
{
    ASD_log(level, stream, options, string);
}

void asd_target_log_buffer(ASD_LogLevel level, ASD_LogStream stream,
                           ASD_LogOption options, const unsigned char* ptr,
                           size_t len, const char* prefixPtr)
{
    ASD_log_buffer(level, stream, options, ptr, len, prefixPtr);
}

void asd_target_log_shift(ASD_LogLevel level, ASD_LogStream stream,
                          ASD_LogOption options, unsigned int number_of_bits,
                          unsigned int size_bytes, unsigned char* buffer,
                          const char* prefixPtr)
{
    ASD_log_shift(level, stream, options, number_of_bits, size_bytes, buffer,
                  prefixPtr);
}
