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

#ifndef _ASD_MAIN_H_
#define _ASD_MAIN_H_

#include <poll.h>
#include <stdbool.h>
#include <stddef.h>

#include "asd_common.h"
#include "asd_msg.h"
#include "authenticate.h"
#include "ext_network.h"
#include "logging.h"
#include "session.h"

// DEFAULTS
#define DEFAULT_JTAG_FLOW JFLOW_BMC
#define DEFAULT_FRU 0x1
#define DEFAULT_I2C_ENABLE false
#define DEFAULT_I2C_BUS 0x04
#define DEFAULT_PORT 5123
#define DEFAULT_CERT_FILE "/etc/ssl/certs/https/server.pem"
#define DEFAULT_LOG_TO_SYSLOG false
#define DEFAULT_FORCE_JTAG_HW false
#define DEFAULT_LOG_LEVEL ASD_LogLevel_Warning
#define DEFAULT_LOG_STREAMS ASD_LogStream_All

typedef struct session_options
{
    uint16_t n_port_number;
    char* cp_certkeyfile;
    char* cp_net_bind_device;
    extnet_hdlr_type_t e_extnet_type;
    auth_hdlr_type_t e_auth_type;
} session_options;

typedef struct asd_args
{
    session_options session;
    i2c_options i2c;
    uint8_t fru;
    uint8_t msg_flow;
    bool use_syslog;
    bool force_jtag_hw;
    ASD_LogLevel log_level;
    ASD_LogStream log_streams;
} asd_args;

typedef struct asd_state
{
    asd_args args;
    ASD_MSG* asd_msg;
    int host_fd;
    int event_fd;
    config config;
    Session* session;
    ExtNet* extnet;
} asd_state;

#define HOST_FD_INDEX 0
#define GPIO_FD_INDEX 1
#define MAX_FDS (GPIO_FD_INDEX + MAX_SESSIONS + NUM_GPIOS + NUM_DBUS_FDS)

typedef enum
{
    CLOSE_CLIENT_EVENT = 1
} InternalEventTypes;

#ifndef UNIT_TEST_MAIN
int main(int argc, char** argv);
#endif

int asd_main(int argc, char** argv);

bool process_command_line(int argc, char** argv, asd_args* args);
void showUsage(char** argv);
STATUS init_asd_state(asd_state* state);
STATUS send_out_msg_on_socket(void* state, unsigned char* buffer,
                              size_t length);
void deinit_asd_state(asd_state* state);
STATUS on_client_disconnect(asd_state* state);
STATUS on_client_connect(asd_state* state, extnet_conn_t* p_extcon);
void on_connection_aborted(void);
STATUS request_processing_loop(asd_state* state);
STATUS process_new_client(asd_state* state, struct pollfd* poll_fds,
                          size_t num_fds, int* num_clients, int client_index);
STATUS process_all_client_messages(asd_state* state,
                                   const struct pollfd* poll_fds,
                                   size_t num_fds);
STATUS process_all_gpio_events(asd_state* state, const struct pollfd* poll_fds,
                               size_t num_fds);
STATUS process_client_message(asd_state* state, struct pollfd poll_fd);
STATUS ensure_client_authenticated(asd_state* state, extnet_conn_t* p_extconn);

STATUS read_data(asd_state* state, extnet_conn_t* p_extconn, void* buffer,
                 size_t* num_to_read, bool* b_data_pending);

STATUS close_connection(asd_state* state);
void log_client_address(const extnet_conn_t* p_extcon);

#endif // _ASD_MAIN_H_
