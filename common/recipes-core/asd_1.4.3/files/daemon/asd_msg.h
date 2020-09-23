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

#ifndef ASD_ASD_MSG_H
#define ASD_ASD_MSG_H

#include "config.h"

#include <poll.h>

#include "asd_common.h"
#include "i2c_handler.h"
#include "i2c_msg_builder.h"
#include "jtag_handler.h"
#include "logging.h"
#include "target_handler.h"

// Two simple rules for the version string are:
// 1. less than 265 in length (or it will be truncated in the plugin)
// 2. no dashes, as they are used later up the sw stack between components.
static char asd_version[] = "ASD_BMC_v1.4.3";

#define NUM_IN_FLIGHT_BUFFERS_TO_USE 20
#define MAX_MULTICHAINS 16
#define CHARS_PER_CHAIN 5
#define OPENBMC_V "OPENBMC_VERSION"
#define ASCII_DOUBLE_QUOTES 34

typedef STATUS (*SendFunctionPtr)(void* state, unsigned char* buffer,
                                  size_t length);
typedef STATUS (*ReadFunctionPtr)(void* state, void* connection, void* buffer,
                                  size_t* num_to_read, bool* data_pending);

typedef enum
{
    READ_STATE_INITIAL = 0,
    READ_STATE_HEADER,
    READ_STATE_BUFFER,
} ReadState;

typedef struct incoming_msg
{
    ssize_t read_index;
    ssize_t data_size;
    ReadState read_state;
    struct asd_message msg;
} incoming_msg;

typedef struct ASD_MSG
{
    SendFunctionPtr send_function;
    ReadFunctionPtr read_function;
    config* asd_cfg;
    struct incoming_msg in_msg;
    struct asd_message out_msg;
    JTAG_Handler* jtag_handler;
    Target_Control_Handle* target_handler;
    I2C_Handler* i2c_handler;
    bool handlers_initialized;
    ShouldLogFunctionPtr should_remote_log;
    LogFunctionPtr send_remote_logging_message;
    IPC_LogType ipc_asd_log_map[6];
    void* callback_state;
    unsigned char prdy_timeout;
    JTAG_CHAIN_SELECT_MODE jtag_chain_mode;
    char bmc_version[120];
    int bmc_version_size;
    uint8_t msg_flow;
} ASD_MSG;

struct packet_data
{
    unsigned char* next_data;
    unsigned int used, total;
};

typedef enum
{
    ScanType_Read,
    ScanType_Write,
    ScanType_ReadWrite
} ScanType;

ASD_MSG* asd_msg_init(SendFunctionPtr, ReadFunctionPtr, void* callback_state,
                      config* asd_cfg);
STATUS asd_msg_free(ASD_MSG* state);
STATUS asd_msg_on_msg_recv(ASD_MSG* state);
int get_message_size(struct asd_message* s_message);
uint8_t lsb_from_msg_size(u_int32_t response_cnt);
uint8_t msb_from_msg_size(u_int32_t response_cnt);
STATUS determine_shift_end_state(ASD_MSG* state, ScanType scan_type,
                                 struct packet_data* packet,
                                 enum jtag_states* end_state);
STATUS asd_msg_read(ASD_MSG* state, void* conn, bool* data_pending);
void send_error_message(ASD_MSG* state, struct asd_message* input_message,
                        ASDError cmd_stat);
STATUS send_response(ASD_MSG* state, struct asd_message* message);
STATUS asd_msg_get_fds(ASD_MSG* state, target_fdarr_t* fds, int* num_fds);
STATUS asd_msg_event(ASD_MSG* state, struct pollfd poll_fd);
STATUS process_i2c_messages(ASD_MSG* state, struct asd_message* in_msg);
STATUS do_read_command(uint8_t cmd, I2C_Msg_Builder* builder,
                       struct packet_data* packet, bool* force_stop);
STATUS do_write_command(uint8_t cmd, I2C_Msg_Builder* builder,
                        struct packet_data* packet, bool* force_stop);
STATUS build_responses(ASD_MSG* state, int* response_cnt,
                       I2C_Msg_Builder* builder, bool ack);
void* get_packet_data(struct packet_data* packet, int bytes_wanted);
void process_message(ASD_MSG* state);
STATUS read_openbmc_version(ASD_MSG* state);

#endif // ASD_ASD_MSG_H
