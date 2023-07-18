/*
Copyright (c) 2021, Intel Corporation

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

#include "asd_msg.h"

#include <safe_mem_lib.h>
#include <safe_str_lib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

#include "asd_server_interface.h"

ASD_MSG msg_state;

static void get_scan_length(unsigned char cmd, uint8_t* num_of_bits,
                            uint8_t* num_of_bytes);
STATUS process_jtag_message(struct asd_message* s_message);
void process_message(void);
void send_remote_log_message(ASD_LogLevel, ASD_LogStream, const char* message);
bool should_remote_log(ASD_LogLevel, ASD_LogStream);
STATUS write_cfg(writeCfg cmd, struct packet_data* packet);
STATUS asd_write_set_active_chain_event(uint8_t scan_chain);
bus_config_type bus_type(uint8_t bus);
STATUS do_bus_select_command(struct packet_data* packet);
STATUS do_set_sclk_command(struct packet_data* packet);
STATUS do_read_write(void* msg_set);
static ASD_MSG* instance = NULL;

STATUS read_openbmc_version()
{
    STATUS result = ST_ERR;
    FILE* fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    explicit_bzero(&msg_state.bmc_version, sizeof(msg_state.bmc_version));
    msg_state.bmc_version_size = 0;
    fp = fopen("/etc/os-release", "r");
    if (fp == NULL)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                "Failed to open /etc/os-release");
    }
    else
    {
        while ((read = getline(&line, &len, fp)) != -1)
        {
            char* ptr = strstr(line, OPENBMC_V);
            if (ptr != NULL)
            {
                line = line + sizeof(OPENBMC_V);
                msg_state.bmc_version_size = read - sizeof(OPENBMC_V) - 1;
                if (memcpy_s(msg_state.bmc_version, sizeof(msg_state.bmc_version),
                             line, msg_state.bmc_version_size) == 1)
                {
                    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK,
                            ASD_LogOption_None, "memcpy_s failed");
                    break;
                }
                else
                {
                    char* p = msg_state.bmc_version;
                    bool first_occurrance = true;
                    for (int n = 0; n < msg_state.bmc_version_size; n++)
                    {
                        if (p[n] == ASCII_DOUBLE_QUOTES &&
                            first_occurrance == true)
                        {
                            p[n] = '<';
                            first_occurrance = false;
                        }
                        else if (p[n] == ASCII_DOUBLE_QUOTES &&
                                 first_occurrance == false)
                        {
                            p[n] = '>';
                        }
                    }
                    result = ST_OK;
                    break;
                }
            }
        }
        fclose(fp);
    }
    return result;
}

STATUS dev_flock(uint8_t bus, int op)
{
    STATUS status = ST_OK;

    if (msg_state.buscfg == NULL)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Failed to read data for flock");
        return ST_ERR;
    }

    switch (bus_type(bus))
    {
        case BUS_CONFIG_I2C:
            if (msg_state.i2c_handler == NULL)
            {
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                        ASD_LogOption_None, "Null i2c handler in flock");
                status = ST_ERR;
                break;
            }
            status = i2c_bus_flock(msg_state.i2c_handler, bus, op);
            break;
        case BUS_CONFIG_I3C:
            if (msg_state.i3c_handler == NULL)
            {
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                        ASD_LogOption_None, "Null i3c handler in flock");
                status = ST_ERR;
                break;
            }
            status = i3c_bus_flock(msg_state.i3c_handler, bus, op);
            break;
        case BUS_CONFIG_NOT_ALLOWED:
        default:
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
                    "flock not allowed");
            status = ST_ERR;
    }
    if (status != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "bus %d device failed to %s", bus,
                op == LOCK_EX ? "Lock" : "Unlock");
    }
    return status;
}

STATUS asd_msg_init(config* asd_cfg)
{
    if (asd_cfg == NULL)
    {
        return ST_ERR;
    }
    else if (instance)
    {
        // only one instance allowed
        return ST_ERR;
    }
    else
    {
        msg_state.jtag_handler = JTAGHandler();
        msg_state.target_handler = TargetHandler();
        // User setting
        msg_state.jtag_handler->fru = asd_cfg->usrcfg.fru;
        msg_state.jtag_handler->msg_flow = asd_cfg->usrcfg.msg_flow;
        msg_state.jtag_handler->force_jtag_hw = asd_cfg->usrcfg.force_jtag_hw;
        msg_state.target_handler->fru = asd_cfg->usrcfg.fru;

        msg_state.buscfg = &asd_cfg->buscfg;
        msg_state.i2c_handler = I2CHandler(msg_state.buscfg);
        msg_state.i3c_handler = I3CHandler(msg_state.buscfg);
        msg_state.vprobe_handler = vProbeHandler();
        if (!msg_state.jtag_handler || !msg_state.target_handler ||
            !msg_state.i2c_handler || !msg_state.i3c_handler ||
            !msg_state.vprobe_handler)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                    ASD_LogOption_None, "Failed to create handlers");
            asd_msg_free();
        }
        else
        {
            msg_state.asd_cfg = asd_cfg;
            msg_state.handlers_initialized = false;
            msg_state.send_remote_logging_message = &send_remote_log_message;
            msg_state.in_msg.read_state = READ_STATE_INITIAL;
            msg_state.in_msg.read_index = 0;
            msg_state.prdy_timeout = 0;
            msg_state.jtag_chain_mode = JTAG_CHAIN_SELECT_MODE_SINGLE;
            instance = &msg_state;
            read_openbmc_version();
        }
    }
    return ST_OK;
}

STATUS asd_msg_free(void)
{
    STATUS jtag_result = ST_OK;
    STATUS i2c_result = ST_OK;
    STATUS i3c_result = ST_OK;
    STATUS target_result = ST_OK;
    STATUS vprobe_result = ST_OK;

    if (instance)
    {
        if (msg_state.jtag_handler)
        {
            jtag_result = JTAG_deinitialize(msg_state.jtag_handler);
            if (jtag_result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to de-initialize the JTAG handler");
            }
            free(msg_state.jtag_handler);
            msg_state.jtag_handler = NULL;
        }

        if (msg_state.i2c_handler)
        {
            i2c_result = i2c_deinitialize(msg_state.i2c_handler);
            if (i2c_result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to de-initialize the i2c handler");
            }
            free(msg_state.i2c_handler);
            msg_state.i2c_handler = NULL;
        }

        if (msg_state.i3c_handler)
        {
            i3c_result = i3c_deinitialize(msg_state.i3c_handler);
            if (i3c_result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to de-initialize the i3c handler");
            }
            free(msg_state.i3c_handler);
            msg_state.i3c_handler = NULL;
        }

        if (msg_state.target_handler)
        {
            target_result = target_deinitialize(msg_state.target_handler);
            if (target_result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to de-initialize the Target handler");
            }
            free(msg_state.target_handler);
            msg_state.target_handler = NULL;
        }

        if (msg_state.vprobe_handler)
        {
            vprobe_result = vProbe_deinitialize(msg_state.vprobe_handler);
            if (vprobe_result != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to de-initialize the vProbe handler");
            }
            free(msg_state.vprobe_handler);
            msg_state.vprobe_handler = NULL;
        }
        instance = NULL;
    }
    else
    {
        return ST_ERR;
    }

    if ((jtag_result != ST_OK) || (i2c_result != ST_OK) ||
        (target_result != ST_OK) || (vprobe_result != ST_OK))
    {
        return ST_ERR;
    }

    return ST_OK;
}

STATUS asd_msg_on_msg_recv(void)
{
    STATUS result = ST_OK;

    struct asd_message* msg = &msg_state.in_msg.msg;
    struct asd_message* msg_out = &msg_state.out_msg;

    int data_size = get_message_size(msg);
    if (msg->header.enc_bit)
    {
        result = ST_ERR;
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                "ASD message encryption not supported.");
        send_error_message(msg, ASD_MSG_CRYPY_NOT_SUPPORTED);
        return result;
    }
    if (msg->header.type == AGENT_CONTROL_TYPE)
    {
        if (memcpy_s(&msg_state.out_msg.header, sizeof(struct message_header),
                     &msg->header, sizeof(struct message_header)))
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                    "memcpy_s: msg header to out msg header copy buffer "
                    "failed.");
            result = ST_ERR;
        }
        // The plugin stores the command in the cmd_stat parameter.
        // For the response, the plugin expects the same value in the
        // first byte of the buffer.
        msg_state.out_msg.buffer[0] = (unsigned char)msg->header.cmd_stat;
        msg_state.out_msg.header.size_lsb = 1;
        msg_state.out_msg.header.size_msb = 0;
        msg_state.out_msg.header.cmd_stat = ASD_SUCCESS;

        switch (msg->header.cmd_stat)
        {
            case NUM_IN_FLIGHT_MESSAGES_SUPPORTED_CMD:
                msg_state.out_msg.buffer[1] = NUM_IN_FLIGHT_BUFFERS_TO_USE;
                msg_state.out_msg.header.size_lsb += 1;
                break;
            case MAX_DATA_SIZE_CMD:
                msg_state.out_msg.buffer[1] = (MAX_DATA_SIZE)&0xFF;
                msg_state.out_msg.buffer[2] = (MAX_DATA_SIZE >> 8) & 0xFF;
                msg_state.out_msg.header.size_lsb = 3;
                msg_state.out_msg.header.size_msb = 0;
                msg_state.out_msg.header.cmd_stat = ASD_SUCCESS;
                break;
            case SUPPORTED_JTAG_CHAINS_CMD:
                msg_state.out_msg.buffer[1] = SUPPORTED_JTAG_CHAINS;
                msg_state.out_msg.header.size_lsb = 2;
                msg_state.out_msg.header.size_msb = 0;
                break;
            case SUPPORTED_I2C_BUSES_CMD:
            {
                unsigned char supported_buses = 0;
                if (msg_state.buscfg->enable_i2c || msg_state.buscfg->enable_i3c)
                {
                    for (int i = 0; i < MAX_IxC_BUSES; i++)
                    {
                        switch (msg_state.buscfg->bus_config_type[i])
                        {
                            case BUS_CONFIG_I2C:
                            case BUS_CONFIG_I3C:
                                msg_state.out_msg.buffer[2 + supported_buses++] =
                                    msg_state.buscfg->bus_config_map[i];
                                break;
                            case BUS_CONFIG_NOT_ALLOWED:
                            default:
                                break;
                        }
                    }
                    msg_state.out_msg.buffer[1] = supported_buses;
                    msg_state.out_msg.header.size_lsb = supported_buses + 2;
                    msg_state.out_msg.header.size_msb = 0;
                }
                else
                {
                    msg_state.out_msg.buffer[1] = 0;
                    msg_state.out_msg.header.size_lsb = 2;
                    msg_state.out_msg.header.size_msb = 0;
                }
                break;
            }
            case SUPPORTED_REMOTE_PROBES_CMD:
                if (!msg_state.vprobe_handler->initialized)
                {
                    if (vProbe_initialize(msg_state.vprobe_handler) != ST_OK)
                    {
                        result = ST_ERR;
                        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                                ASD_LogOption_None,
                                "Failed to initialize the vprobe handler");
                        send_error_message(msg,
                                           ASD_FAILURE_INIT_VPROBE_HANDLER);
                        return result;
                    }
                }
                msg_state.out_msg.buffer[1] = msg_state.vprobe_handler->remoteProbes;
                msg_state.out_msg.header.size_lsb = 2;
                msg_state.out_msg.header.size_msb = 0;
                break;
            case REMOTE_PROBES_CONFIG_CMD:
                if (msg_state.vprobe_handler->remoteConfigs > 0)
                {
                    uint16_t config_size = 0;

                    if (strcpy_s(&msg_state.out_msg.buffer[1],
                                 sizeof(msg_state.out_msg.buffer) - 1,
                                 msg_state.vprobe_handler->probesConfig))
                    {
                        ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG,
                                ASD_LogOption_None,
                                "strcpy_s: remote probe config to out msg "
                                "buffer copy failed.");
                        result = ST_ERR;
                    }
                    config_size = (uint16_t)(
                        strnlen(msg_state.vprobe_handler->probesConfig,
                                sizeof(msg_state.vprobe_handler->probesConfig)) +
                        1);
                    msg_state.out_msg.header.size_lsb =
                        (uint8_t)(config_size & 0xFF);
                    msg_state.out_msg.header.size_msb =
                        (uint8_t)(config_size >> 8);
                }
                break;
            case OBTAIN_DOWNSTREAM_VERSION_CMD:
                // The +/-1 references below are to account for the
                // write of cmd_stat to buffer position 0 above
                if (memcpy_s(&msg_state.out_msg.buffer[1], sizeof(asd_version),
                             asd_version, sizeof(asd_version)))
                {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG,
                            ASD_LogOption_None,
                            "memcpy_s: asd_version to out msg buffer copy "
                            "failed.");
                    result = ST_ERR;
                }
                msg_state.out_msg.header.size_lsb += sizeof(asd_version);
                if (memcpy_s(&msg_state.out_msg.buffer[sizeof(asd_version)], 1,
                             "_", 1))
                {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG,
                            ASD_LogOption_None,
                            "memcpy_s: asd_version to out msg buffer copy "
                            "failed.");
                    result = ST_ERR;
                }
                if (memcpy_s(&msg_state.out_msg.buffer[sizeof(asd_version) + 1],
                             msg_state.bmc_version_size, msg_state.bmc_version,
                             msg_state.bmc_version_size))
                {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG,
                            ASD_LogOption_None,
                            "memcpy_s: asd_version to out msg buffer copy "
                            "failed.");
                    result = ST_ERR;
                }
                msg_state.out_msg.header.size_lsb += msg_state.bmc_version_size;
                if (memcpy_s(
                        &msg_state.out_msg.buffer[sizeof(asd_version) +
                                               msg_state.bmc_version_size + 1],
                        1, "\0", 1))
                {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG,
                            ASD_LogOption_None,
                            "memcpy_s: asd_version to out msg buffer copy "
                            "failed.");
                    result = ST_ERR;
                }
                break;
            case AGENT_CONFIGURATION_CMD:
            {
                // An agent configuration command was sent.
                // The next byte is the Agent Config type.
                struct packet_data packet;
                packet.next_data = (unsigned char*)&msg->buffer;
                packet.used = 0;
                packet.total = (unsigned int)data_size;
                u_int8_t* config_type = get_packet_data(&packet, 1);
                if (!config_type)
                    break;
                if (*config_type == AGENT_CONFIG_TYPE_LOGGING)
                {
                    // We will store the logging stream only
                    // for the sake of sending it back to
                    // the IPC plugin. Technically the
                    // protocol is defined in such a way as
                    // where the BMC could emit log messages
                    // for several streams, but since we
                    // only have it implemented with one
                    // stream, we wont do much with this
                    // stored stream.
                    u_int8_t* logging = get_packet_data(&packet, 1);
                    if (!logging)
                        break;
                    msg_state.asd_cfg->remote_logging.value = *logging;
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(
                        ASD_LogLevel_Debug, ASD_LogStream_SDK,
                        ASD_LogOption_No_Remote,
                        "Remote logging command received. Stream: %d Level: %d",
                        msg_state.asd_cfg->remote_logging.logging_stream,
                        msg_state.asd_cfg->remote_logging.logging_level);
#endif
                }
                else if (*config_type == AGENT_CONFIG_TYPE_GPIO)
                {
                    u_int8_t* gpio_config = get_packet_data(&packet, 1);
                    if (!gpio_config)
                        break;
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(
                        ASD_LogLevel_Debug, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Remote config command received. GPIO config: 0x%02x",
                        *gpio_config);
#endif
                }
                else if (*config_type == AGENT_CONFIG_TYPE_JTAG_SETTINGS)
                {
                    u_int8_t* mode = get_packet_data(&packet, 1);
                    if (!mode)
                        break;
                    if ((*mode & JTAG_DRIVER_MODE_MASK) ==
                        JTAG_DRIVER_MODE_HARDWARE)
                        msg_state.asd_cfg->jtag.mode = JTAG_DRIVER_MODE_HARDWARE;
                    else
                        msg_state.asd_cfg->jtag.mode = JTAG_DRIVER_MODE_SOFTWARE;
                    if ((*mode & JTAG_CHAIN_SELECT_MODE_MASK) ==
                        JTAG_CHAIN_SELECT_MODE_MULTI)
                        msg_state.jtag_chain_mode = JTAG_CHAIN_SELECT_MODE_MULTI;
                    else
                        msg_state.jtag_chain_mode = JTAG_CHAIN_SELECT_MODE_SINGLE;
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(
                        ASD_LogLevel_Debug, ASD_LogStream_SDK,
                        ASD_LogOption_No_Remote,
                        "Remote config command received. JTAG Driver Mode: %s"
                        "and Jtag Chain Select Mode: %s",
                        msg_state.asd_cfg->jtag.mode == JTAG_DRIVER_MODE_SOFTWARE
                            ? "Software"
                            : "Hardware",
                        msg_state.jtag_chain_mode == JTAG_CHAIN_SELECT_MODE_SINGLE
                            ? "Single"
                            : "Multi");
#endif
                }
                break;
            }
            case LOOPBACK_CMD:
                ASD_log(ASD_LogLevel_Trace, ASD_LogStream_Network,
                        ASD_LogOption_None, "LOOPBACK MODE.");
                uint16_t delay = (msg->buffer[0]);
                uint16_t size =
                    (msg->header.size_lsb) + (msg->header.size_msb << 8);
                uint32_t crc_data_calc = 0;
                uint32_t read_crc = 0;
                if (size > MAX_DATA_SIZE)
                {
                    size = MAX_DATA_SIZE;
                }
                ASD_log(ASD_LogLevel_Trace, ASD_LogStream_Network,
                        ASD_LogOption_None, "size: %d, delay: %d.",
                        size, delay);
                msg_out->header = msg->header;
                for (uint16_t index = 0; index < size; index++)
                {
                    msg_out->buffer[index] = msg->buffer[index];
                    if (index > 4)
                    {
                        crc_data_calc ^= (msg_out->buffer[index] & 0xff);
                    }
                }
                read_crc = ((msg_out->buffer[1] << 24) & 0xff) |
                    ((msg_out->buffer[2] << 16) & 0xff) |
                    ((msg_out->buffer[3] << 8) & 0xff) |
                    ((msg_out->buffer[4]) & 0xff);
                ASD_log(ASD_LogLevel_Trace, ASD_LogStream_Network,
                        ASD_LogOption_None,
                        "CRC in payload: 0x%x, CRC calulated: 0x%x.",
                        crc_data_calc, read_crc);
                //suspend execution for microsecond intervales
                // valid delays will be from 1 to 255. (1ms to 255ms)
                usleep(delay*1000);
                break;
            default:
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Unsupported Agent Control command received %d",
                        msg->header.cmd_stat);
#endif
            }
        }
        if (send_response(&msg_state.out_msg) != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                    ASD_LogOption_No_Remote,
                    "Failed to send agent control message response.");
        }
    }
    else
    {
        if (!msg_state.handlers_initialized)
        {
            if (JTAG_initialize(msg_state.jtag_handler,
                                msg_state.asd_cfg->jtag.mode ==
                                    JTAG_DRIVER_MODE_SOFTWARE) != ST_OK)
            {
                result = ST_ERR;
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to initialize the jtag handler");
                send_error_message(msg, ASD_FAILURE_INIT_JTAG_HANDLER);
                return result;
            }

            if (msg_state.buscfg->enable_i2c)
            {
                if (i2c_initialize(msg_state.i2c_handler) != ST_OK)
                {
                    result = ST_ERR;
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                            ASD_LogOption_None,
                            "Failed to initialize the i2c handler");
                    send_error_message(msg,
                                       ASD_FAILURE_INIT_I2C_HANDLER);
                    return result;
                }
            }

            if (msg_state.buscfg->enable_i3c)
            {
                if (i3c_initialize(msg_state.i3c_handler) != ST_OK)
                {
                    result = ST_ERR;
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                            ASD_LogOption_None,
                            "Failed to initialize the i3c handler");
                    send_error_message(msg,
                                       ASD_FAILURE_INIT_I2C_HANDLER);
                    return result;
                }
            }

            if (target_initialize(msg_state.target_handler,
                                  msg_state.asd_cfg->jtag.xdp_fail_enable) == ST_OK)
            {
                msg_state.handlers_initialized = true;
                if (msg_state.target_handler->xdp_present == true)
                {
                    send_error_message(msg, ASD_FAILURE_XDP_PRESENT);
                }
            }
            else
            {
                result = ST_ERR;
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to initialize the target handler");
                if (JTAG_deinitialize(msg_state.jtag_handler) != ST_OK)
                {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                            ASD_LogOption_None,
                            "Failed to de-initialize the JTAG handler");
                }
                send_error_message( msg, ASD_FAILURE_XDP_PRESENT);
                return result;
            }
        }
        process_message();
    }
    return result;
}

void send_error_message(struct asd_message* input_message,
                        ASDError cmd_stat)
{
    if (!input_message)
        return;
    struct asd_message error_message = {{0}}; // initialize the struct
    if (memcpy_s(&error_message.header, sizeof(struct message_header),
                 &(input_message->header), sizeof(struct message_header)))
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                "memcpy_s: input header to error header copy  failed.");
    }
    error_message.header.type = 1;
    error_message.header.size_lsb = 0;
    error_message.header.size_msb = 0;
    error_message.header.cmd_stat = cmd_stat;
    if (send_response(&error_message) != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_No_Remote,
                "Failed to send an error message to client");
    }
}

bool should_remote_log(ASD_LogLevel asd_level, ASD_LogStream asd_stream)
{
    (void)asd_stream;
    bool result = false;
    if (instance)
    {
        config* asd_cfg = instance->asd_cfg;
        if (asd_cfg->remote_logging.logging_level != IPC_LogType_Off)
        {
            if (asd_cfg->remote_logging.logging_level <=
                asd_cfg->ipc_asd_log_map[asd_level])
                result = true;
        }
    }
    return result;
}

void send_remote_log_message(ASD_LogLevel asd_level, ASD_LogStream asd_stream,
                             const char* message)
{
    if (!instance || !message)
        return;

    if (should_remote_log(asd_level, asd_stream))
    {
        remote_logging_config config_byte = {{0}};
        config_byte.logging_level = instance->asd_cfg->ipc_asd_log_map[asd_level];
        config_byte.logging_stream =
            instance->asd_cfg->remote_logging.logging_stream;
        struct asd_message msg = {{0}};
        size_t message_length = strnlen_s(message, MAX_DATA_SIZE);

        // minus 2 for the 2 prefixing bytes.
        if (message_length > (MAX_DATA_SIZE - 2))
            message_length = (MAX_DATA_SIZE - 2);
        u_int32_t buffer_length = (u_int32_t)message_length + 2;

        msg.header.type = HARDWARE_LOG_EVENT;
        msg.header.tag = 0;
        msg.header.origin_id = 0;
        msg.buffer[0] = AGENT_CONFIGURATION_CMD;
        msg.buffer[1] = config_byte.value;
        // Copy message into remaining buffer space.
        if (memcpy_s(&msg.buffer[2], sizeof(msg.buffer), message,
                     message_length))
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                    "memcpy_s: message to msg buffer[2] copy failed.");
        }
        // Store the message size into the msb and lsb fields.
        // The size is the length of the message string, plus 2
        // for the 2 prefix bytes containing the stream and
        // level bits and the cmd type.
        msg.header.size_lsb = lsb_from_msg_size(buffer_length);
        msg.header.size_msb = msb_from_msg_size(buffer_length);
        msg.header.cmd_stat = ASD_SUCCESS;
        if (send_response(&msg) != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_No_Remote,
                "Failed to send remote message to client");
        }
    }
}

STATUS __attribute__((weak))
passthrough_jtag_message(ASD_MSG* state, struct asd_message* s_message)
{
    return ST_ERR;
}

void process_message()
{
    struct asd_message* msg = &msg_state.in_msg.msg;
    if (((msg->header.type != JTAG_TYPE) && (msg->header.type != I2C_TYPE)) ||
        (msg->header.cmd_stat != 0 && msg->header.cmd_stat != 0x80 &&
         msg->header.cmd_stat != 1))
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                "Received the message that is not supported by this host\n"
                "Message must be in the format:\n"
                "Type = 1 or 6 & cmd_stat=0 or 1 or 0x80\n"
                "I got Type=%x, cmd_stat=%x",
                msg->header.type, msg->header.cmd_stat);
        send_error_message(msg, ASD_MSG_NOT_SUPPORTED);
        return;
    }

    if (msg->header.type == JTAG_TYPE)
    {
        if (msg_state.jtag_handler->msg_flow == JFLOW_BIC) {
            if (passthrough_jtag_message(&msg_state, msg) != ST_OK) {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                        "Failed to passthrough JTAG message");
                send_error_message( msg, ASD_FAILURE_PROCESS_JTAG_MSG);
            }
            return;
        } else if (process_jtag_message(msg) != ST_OK) {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Failed to process JTAG message");
            send_error_message( msg, ASD_FAILURE_PROCESS_JTAG_MSG);
            return;
        }
    }
    else if (msg->header.type == I2C_TYPE)
    {
        if (msg_state.buscfg->enable_i2c || msg_state.buscfg->enable_i3c)
        {
            if (dev_flock(msg_state.buscfg->default_bus, LOCK_EX) != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "Failed to lock device");
                send_error_message( msg, ASD_FAILURE_PROCESS_I2C_LOCK);
                return;
            }
            if (process_i2c_messages(msg) != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "Failed to process I2C message");
                send_error_message( msg, ASD_FAILURE_PROCESS_I2C_MSG);
            }
            if (dev_flock(msg_state.buscfg->default_bus, LOCK_UN) != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "Failed to lock device");
                send_error_message( msg, ASD_FAILURE_REMOVE_I2C_LOCK);
            }
            return;
        }
        else
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Received I2C message that is not supported "
                    "by this platform");
            send_error_message( msg, ASD_I2C_MSG_NOT_SUPPORTED);
            return;
        }
    }
}

STATUS write_event_config(const uint8_t data_byte)
{
    STATUS status = ST_OK;
    // If bit 7 is set, then we will be enabling an event, otherwise
    // disabling
    bool enable = (data_byte >> 7) == 1;
    // Bits 0 through 6 are an unsigned int indicating the event config
    // register
    uint8_t event_cfg_raw = (uint8_t)(data_byte & WRITE_CFG_MASK);
    if (event_cfg_raw < WRITE_CONFIG_MAX)
    {
        WriteConfig event_cfg = (WriteConfig)event_cfg_raw;
        status =
            target_write_event_config(msg_state.target_handler, event_cfg, enable);
    }
    else
    {
        // We will not return an error for this
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                "Invalid or unsupported write event config register: "
                "0x%02x",
                event_cfg_raw);
    }
    return status;
}

STATUS read_status(const ReadType index, uint8_t pin,
                   unsigned char* return_buffer, int* bytes_written)
{
    *bytes_written = 0;
    bool pinAsserted = false;

    STATUS status = target_read(msg_state.target_handler, (Pin)pin, &pinAsserted);

    if (status == ST_OK)
    {
        return_buffer[(*bytes_written)++] =
            (unsigned char)(READ_STATUS_MIN + index);
        return_buffer[*bytes_written] = pin;
        if (pinAsserted)
            return_buffer[*bytes_written] |= (1 << 7); // set 7th bit
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK, ASD_LogOption_None,
                "read_status %d: %s", pin,
                pinAsserted ? "asserted" : "deasserted");
#endif
        (*bytes_written)++;
    }
    else
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                "read_status failed.");
    }
    return status;
}

STATUS process_jtag_message(struct asd_message* s_message)
{
    u_int32_t response_cnt = 0;
    enum jtag_states end_state;
    STATUS status = ST_OK;
    int size = get_message_size(s_message);
    struct packet_data packet;
    unsigned char* data_ptr;
    uint8_t cmd = 0;

    if (size == -1)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                "Failed to process jtag message because "
                "get message size failed.");
        return ST_ERR;
    }

    explicit_bzero(&msg_state.out_msg.header, sizeof(struct message_header));
    explicit_bzero(&msg_state.out_msg.buffer, MAX_DATA_SIZE);

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network, ASD_LogOption_No_Remote,
            "NetReq tag: %d size: %d", s_message->header.tag, size);
    ASD_log_buffer(ASD_LogLevel_Debug, ASD_LogStream_Network,
                   ASD_LogOption_No_Remote, s_message->buffer, (size_t)size,
                   "NetReq");
#endif

    packet.next_data = s_message->buffer;
    packet.used = 0;
    packet.total = (unsigned int)size;

    while (packet.used < packet.total)
    {
        data_ptr = get_packet_data(&packet, 1);
        if (data_ptr == NULL)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Failed to read data for cmd, short packet");
            status = ST_ERR;
            break;
        }
        else
        {
            cmd = *data_ptr;
        }

        if (cmd == WRITE_EVENT_CONFIG)
        {
            data_ptr = get_packet_data(&packet, 1);
            if (data_ptr == NULL)
            {
                ASD_log(
                    ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Failed to read data for WRITE_EVENT_CONFIG, short packet");
                status = ST_ERR;
                break;
            }

            status = write_event_config(*data_ptr);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "write_event_config failed, %d",
                        status);
                break;
            }
        }
        else if (cmd >= WRITE_CFG_MIN && cmd <= WRITE_CFG_MAX)
        {
            status = write_cfg((writeCfg)cmd, &packet);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "write_cfg failed, %d", status);
                break;
            }
        }
        else if (cmd == WRITE_PINS)
        {
            data_ptr = get_packet_data(&packet, 1);
            if (data_ptr == NULL)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to read data for WRITE_PINS, short packet");
                status = ST_ERR;
                break;
            }

            uint8_t data = *data_ptr;
            bool assert = (data >> 7) == 1;
            uint8_t index = data & WRITE_PIN_MASK;

            if (index < PIN_MAX)
            {
                Pin pin = (Pin)index;
                status = target_write(msg_state.target_handler, pin, assert);
                if (status != ST_OK)
                {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                            ASD_LogOption_None, "target_write failed, %d",
                            status);
                    break;
                }
#ifdef SKIP_WAIT_CYCLES_DURING_RESET
                if (pin == PIN_RESET_BUTTON && assert == true)
                {
                    while (packet.used < packet.total)
                    {
                        data_ptr = get_packet_data(&packet, 2);
                        if (data_ptr == NULL)
                        {
                            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                                    ASD_LogOption_None,
                                    "Failed to read WAIT_CYCLES data");
                            status = ST_ERR;
                            break;
                        }
                        data = *data_ptr;
                        if (data != WAIT_CYCLES_TCK_DISABLE)
                        {
                            packet.next_data -= 2;
                            packet.used -= 2;
                            break;
                        }
                    }
                    if (status == ST_ERR)
                        break;
                }
#endif
            }
            else if ((index & SCAN_CHAIN_SELECT) == SCAN_CHAIN_SELECT)
            {
                if (msg_state.jtag_chain_mode == JTAG_CHAIN_SELECT_MODE_SINGLE)
                {
                    uint8_t scan_chain = (index & SCAN_CHAIN_SELECT_MASK);
                    if (scan_chain >= MAX_SCAN_CHAINS)
                    {
                        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                                ASD_LogOption_None,
                                "Unexpected scan chain: 0x%02x", scan_chain);
                        status = ST_ERR;
                        break;
                    }
                    status =
                        asd_write_set_active_chain_event(scan_chain);
                    if (status != ST_OK)
                    {
                        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                                ASD_LogOption_None,
                                "Target set active chain failed, %d", status);
                        break;
                    }
                }
                else
                {
                    uint8_t chain_bytes_length =
                        (index & SCAN_CHAIN_SELECT_MASK) + 1;
                    uint8_t chain_bytes[MAX_MULTICHAINS];
                    explicit_bzero(chain_bytes, sizeof(chain_bytes));
                    // e.g.: chain_bytes_length = 1
                    // chain_bytes[0] = b'11111111 = Chain
                    // 0-7 are selected chain_bytes[1] =
                    // b'00000011 = Chain 8 (bit 0) and 9
                    // (bit 1) are selected Notes: support
                    // up to 16 chain_bytes = 128 jtag
                    // chains
                    for (int i = 0; i < chain_bytes_length; i++)
                    {
                        data_ptr = get_packet_data(&packet, 1);
                        if (data_ptr == NULL)
                        {
                            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                                    ASD_LogOption_None,
                                    "Failed to read data for chain_bytes");
                            status = ST_ERR;
                            break;
                        }
                        chain_bytes[i] = *data_ptr;
                    }
                    // Implementing specific code for
                    // multi-jtag chain select add here:
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(
                        ASD_LogLevel_Debug, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Multi-jtag chain select command not yet implemented");
#endif
                    // e.g. < Chain 21 > Received chain
                    // bytes_length: Chain: 0x20 0000
                    char line[MAX_MULTICHAINS * CHARS_PER_CHAIN];
                    uint8_t pos = 0;
                    explicit_bzero(line, sizeof(line));
                    for (int i = chain_bytes_length - 1; i >= 0; i--)
                    {
                        snprintf(&line[pos], sizeof(line), "%02X",
                                 chain_bytes[i]);
                        pos = pos + 2;
                        if (i % 2 == 0 && i != 0)
                        {
                            snprintf(&line[pos], sizeof(line), " ");
                            pos++;
                        }
                    }
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK,
                            ASD_LogOption_None, "Chain : 0x%s", line);
#endif
                }
            }
            else
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Unexpected WRITE_PINS index: 0x%02x", index);
                status = ST_ERR;
                break;
            }
        }
        else if (cmd >= READ_STATUS_MIN && cmd <= READ_STATUS_MAX)
        {
            int bytes_written = 0;
            uint8_t index = (cmd & READ_STATUS_MASK);
            ReadType readStatusTypeIndex = (ReadType)index;

            data_ptr = get_packet_data(&packet, 1);
            if (data_ptr == NULL)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to read data for Read Status, short packet");
                status = ST_ERR;
                break;
            }

            if (response_cnt + 2 > MAX_DATA_SIZE)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to process READ_STATUS. "
                        "Response buffer already full");
                status = ST_ERR;
                break;
            }

            uint8_t pin = (*data_ptr & READ_STATUS_PIN_MASK);
            status = read_status(readStatusTypeIndex, pin,
                                 &(msg_state.out_msg.buffer[response_cnt]),
                                 &bytes_written);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "read_status failed, %d", status);
                break;
            }
            response_cnt += bytes_written;
        }
        else if (cmd == WAIT_CYCLES_TCK_DISABLE ||
                 cmd == WAIT_CYCLES_TCK_ENABLE)
        {

            data_ptr = get_packet_data(&packet, 1);
            if (data_ptr == NULL)
            {
                ASD_log(
                    ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Failed to read data for WAIT_CYCLES_TCK, short packet");
                status = ST_ERR;
                break;
            }

            unsigned int number_of_cycles = *data_ptr;
            if (number_of_cycles == 0)
                number_of_cycles = 256;
            status = JTAG_wait_cycles(msg_state.jtag_handler, number_of_cycles);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "JTAG_wait_cycles failed, %d",
                        status);
                break;
            }
        }
        else if (cmd == WAIT_PRDY)
        {
            status =
                target_wait_PRDY(msg_state.target_handler, msg_state.prdy_timeout);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, " wait for PRDY failed, %d",
                        status);
                break;
            }
        }
        else if (cmd == CLEAR_TIMEOUT)
        {
            // This command does not apply to JTAG
            // so we will likely not implement it.
            ASD_log(ASD_LogLevel_Info, ASD_LogStream_SDK, ASD_LogOption_None,
                    "CLEAR_TIMEOUT not yet implemented.");
        }
        else if (cmd == TAP_RESET)
        {
            status = JTAG_tap_reset(msg_state.jtag_handler);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "JTAG_tap_reset failed, %d",
                        status);
                break;
            }
        }
        else if (cmd == WAIT_SYNC)
        {
            uint16_t timeout = 0;
            uint16_t delay = 0;
            data_ptr = get_packet_data(&packet, WAIT_SYNC_CMD_LENGTH);
            if (data_ptr == NULL)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to read data for the WaitSync command, short "
                        "packet");
                status = ST_ERR;
                break;
            }
            // read timout and delay. 2 bytes each, LSB first
            timeout = data_ptr[0] + (data_ptr[1] << 8);
            delay = data_ptr[2] + (data_ptr[3] << 8);
            status = target_wait_sync(msg_state.target_handler, timeout, delay);
            if (status == ST_TIMEOUT)
            {
                ASD_log(ASD_LogLevel_Warning, ASD_LogStream_SDK,
                        ASD_LogOption_None, "target_wait_sync timed out");
                status = ST_OK;
            }
            else if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "target_wait_sync failed: %d",
                        status);
                break;
            }
        }
        else if (cmd >= TAP_STATE_MIN && cmd <= TAP_STATE_MAX)
        {
            status = JTAG_set_tap_state(
                msg_state.jtag_handler,
                (enum jtag_states)(cmd & (uint8_t)TAP_STATE_MASK));
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "JTAG_set_tap_state failed, %d",
                        status);
                break;
            }
        }
        else if (cmd >= WRITE_SCAN_MIN && cmd <= WRITE_SCAN_MAX)
        {
            uint8_t num_of_bits = 0;
            uint8_t num_of_bytes = 0;

            get_scan_length(cmd, &num_of_bits, &num_of_bytes);
            data_ptr = get_packet_data(&packet, num_of_bytes);
            if (data_ptr == NULL)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to read data from buffer: %d", num_of_bytes);
                status = ST_ERR;
                break;
            }

            status = determine_shift_end_state(ScanType_Write, &packet,
                                               &end_state);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "determine_shift_end_state failed, %d", status);
                break;
            }
            status = JTAG_shift(msg_state.jtag_handler, num_of_bits, num_of_bytes,
                                data_ptr, 0, NULL, end_state);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "JTAG_shift failed, %d", status);
                break;
            }
        }
        else if (cmd >= READ_SCAN_MIN && cmd <= READ_SCAN_MAX)
        {
            uint8_t num_of_bits = 0;
            uint8_t num_of_bytes = 0;

            get_scan_length(cmd, &num_of_bits, &num_of_bytes);
            if (response_cnt + sizeof(char) + num_of_bytes > MAX_DATA_SIZE)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to process READ_SCAN. "
                        "Response buffer already full");
                status = ST_ERR;
                break;
            }
            msg_state.out_msg.buffer[response_cnt++] = cmd;
            status = determine_shift_end_state(ScanType_Read, &packet,
                                               &end_state);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "determine_shift_end_state failed, %d", status);
                break;
            }
            status = JTAG_shift(
                msg_state.jtag_handler, num_of_bits, 0, NULL, num_of_bytes,
                &(msg_state.out_msg.buffer[response_cnt]), end_state);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "JTAG_shift failed, %d", status);
                break;
            }
            response_cnt += num_of_bytes;
        }
        else if (cmd >= READ_WRITE_SCAN_MIN)
        {
            uint8_t num_of_bits = 0;
            uint8_t num_of_bytes = 0;
            get_scan_length(cmd, &num_of_bits, &num_of_bytes);
            if (response_cnt + sizeof(char) + num_of_bytes > MAX_DATA_SIZE)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to process READ_WRITE_SCAN. "
                        "Response buffer already full");
                status = ST_ERR;
                break;
            }
            msg_state.out_msg.buffer[response_cnt++] = cmd;
            data_ptr = get_packet_data(&packet, num_of_bytes);
            if (data_ptr == NULL)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Failed to read data from buffer: %d", num_of_bytes);
                status = ST_ERR;
                break;
            }
            status = determine_shift_end_state(ScanType_ReadWrite,
                                               &packet, &end_state);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "determine_shift_end_state failed, %d", status);
                break;
            }
            status =
                JTAG_shift(msg_state.jtag_handler, num_of_bits, num_of_bytes,
                           data_ptr, num_of_bytes,
                           &(msg_state.out_msg.buffer[response_cnt]), end_state);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "JTAG_shift failed, %d", status);
                break;
            }
            response_cnt += num_of_bytes;
        }
        else
        {
            // Unknown Command
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Encountered unknown command 0x%02x", (int)cmd);
            status = ST_ERR;
            break;
        }
    }

    if (status == ST_OK)
    {
        if (memcpy_s(&msg_state.out_msg.header, sizeof(struct message_header),
                     &s_message->header, sizeof(struct message_header)))
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                    "memcpy_s: message header to out msg header copy failed.");
            return ST_ERR;
        }

        msg_state.out_msg.header.size_lsb = lsb_from_msg_size(response_cnt);
        msg_state.out_msg.header.size_msb = msb_from_msg_size(response_cnt);
        msg_state.out_msg.header.cmd_stat = ASD_SUCCESS;

        status = send_response(&msg_state.out_msg);
        if (status != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                    ASD_LogOption_No_Remote,
                    "Failed to send message back on the socket");
        }
    }
    return status;
}

STATUS write_cfg(const writeCfg cmd, struct packet_data* packet)
{
    STATUS status = ST_ERR;
    unsigned char* data;

    if (cmd == JTAG_FREQ)
    {
        // JTAG_FREQ (Index 1, Size: 1 Byte)
        //  Bit 7:6 � Prescale Value (b'00 � 1, b'01 � 2, b'10 � 4, b'11
        //  � 8) Bit 5:0 � Divisor (1-64, 64 is expressed as value 0)
        //  The TAP clock frequency is determined by dividing the system
        //  clock of the TAP Master first through the prescale value
        //  (1,2,4,8) and then through the divisor (1-64).
        // e.g. system clock/(prescale * divisor)
        uint8_t prescaleVal = 0, divisorVal = 0, tCLK = 0;
        data = get_packet_data(packet, 1);
        if (data == NULL)
        {
            status = ST_ERR;
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Unable to read JTAG_FREQ data byte");
        }
        else
        {
            prescaleVal = data[0] >> (uint8_t)5;
            divisorVal = data[0] & (uint8_t)0x1f;

            if (prescaleVal == 0)
            {
                prescaleVal = 1;
            }
            else if (prescaleVal == 1)
            {
                prescaleVal = 2;
            }
            else if (prescaleVal == 2)
            {
                prescaleVal = 4;
            }
            else if (prescaleVal == 3)
            {
                prescaleVal = 8;
            }
            else
            {
                prescaleVal = 1;
            }

            if (divisorVal == 0)
            {
                divisorVal = 64;
            }

            tCLK = (prescaleVal * divisorVal);
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Set JTAG TAP Pre: %d  Div: %d  TCK: %d", prescaleVal,
                    divisorVal, tCLK);
#endif

            status = JTAG_set_jtag_tck(msg_state.jtag_handler, tCLK);
            if (status != ST_OK)
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "Unable to set the JTAG TAP TCK!");
        }
    }
    else if (cmd == DR_PREFIX)
    {
        // DR Postfix (A.K.A. DR Prefix in At-Scale Debug Arch. Spec.)
        // set drPaddingNearTDI 1 byte of data

        data = get_packet_data(packet, 1);
        if (data == NULL)
        {
            status = ST_ERR;
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Unable to read DRPost data byte");
        }
        else
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Setting DRPost padding to %d", data[0]);
#endif
            status = JTAG_set_padding(msg_state.jtag_handler,
                                      JTAGPaddingTypes_DRPost, data[0]);
            if (status != ST_OK)
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "failed to set DRPost padding");
        }
    }
    else if (cmd == DR_POSTFIX)
    {
        // DR preFix (A.K.A. DR Postfix in At-Scale Debug Arch. Spec.)
        // drPaddingNearTDO 1 byte of data

        data = get_packet_data(packet, 1);
        if (data == NULL)
        {
            status = ST_ERR;
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Unable to read DRPre data byte");
        }
        else
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Setting DRPre padding to %d", data[0]);
#endif
            status = JTAG_set_padding(msg_state.jtag_handler,
                                      JTAGPaddingTypes_DRPre, data[0]);
            if (status != ST_OK)
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "failed to set DRPre padding");
        }
    }
    else if (cmd == IR_PREFIX)
    {
        // IR Postfix (A.K.A. IR Prefix in At-Scale Debug Arch. Spec.)
        // irPaddingNearTDI 2 bytes of data

        data = get_packet_data(packet, 2);
        if (data == NULL)
        {
            status = ST_ERR;
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Unable to read IRPost data byte");
        }
        else
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Setting IRPost padding to %d", (data[1] << 8) | data[0]);
#endif
            status =
                JTAG_set_padding(msg_state.jtag_handler, JTAGPaddingTypes_IRPost,
                                 (data[1] << 8) | data[0]);
            if (status != ST_OK)
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "failed to set IRPost padding");
        }
    }
    else if (cmd == IR_POSTFIX)
    {
        // IR Prefix (A.K.A. IR Postfix in At-Scale Debug Arch. Spec.)
        // irPaddingNearTDO 2 bytes of data

        data = get_packet_data(packet, 2);
        if (data == NULL)
        {
            status = ST_ERR;
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Unable to read IRPre data byte");
        }
        else
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Setting IRPre padding to %d", (data[1] << 8) | data[0]);
#endif
            status =
                JTAG_set_padding(msg_state.jtag_handler, JTAGPaddingTypes_IRPre,
                                 (data[1] << 8) | data[0]);
            if (status != ST_OK)
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None, "failed to set IRPre padding");
        }
    }
    else if (cmd == PRDY_TIMEOUT)
    {
        // PRDY timeout
        // 1 bytes of data

        data = get_packet_data(packet, 1);
        if (data == NULL)
        {
            status = ST_ERR;
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Unable to read PRDY_TIMEOUT data byte");
        }
        else
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK, ASD_LogOption_None,
                    "PRDY Timeout config set to %d", data[0]);
#endif
            msg_state.prdy_timeout = data[0];
            status = ST_OK;
        }
    }

    return status;
}

static void get_scan_length(const unsigned char cmd, uint8_t* num_of_bits,
                            uint8_t* num_of_bytes)
{
    *num_of_bits = (uint8_t)(cmd & SCAN_LENGTH_MASK);
    *num_of_bytes = (uint8_t)((*num_of_bits + 7) / 8);

    if (*num_of_bits == 0)
    {
        // this is 64bits
        *num_of_bytes = 8;
        *num_of_bits = 64;
    }
}

STATUS determine_shift_end_state(ScanType scan_type,
                                 struct packet_data* packet,
                                 enum jtag_states* end_state)
{
    unsigned char* next_cmd_ptr = NULL;
    unsigned char* next_cmd2_ptr = NULL;
    unsigned int next_cmd2_counter = 0;
    STATUS status;

    if (!packet || !end_state)
        return ST_ERR;

    // First we will get the default end_state, to use if there are
    // no more bytes to read from the packet.
    status = JTAG_get_tap_state(msg_state.jtag_handler, end_state);

    if (status == ST_OK)
    {
        // Peek ahead to get next command byte
        next_cmd_ptr = get_packet_data(packet, 1);
        if (next_cmd_ptr != NULL)
        {
            if (*next_cmd_ptr >= TAP_STATE_MIN &&
                *next_cmd_ptr <= TAP_STATE_MAX)
            {
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Next is TAP_STATE(0x%x)", *next_cmd_ptr);
                // Next command is a tap state command
                *end_state = (enum jtag_states)(*next_cmd_ptr & TAP_STATE_MASK);

                // In hardware mode, we must peek ahead for the next tap state
                // to determine the end state
                if (msg_state.asd_cfg->jtag.mode == JTAG_DRIVER_MODE_HARDWARE)
                {
                    while (packet->used < packet->total)
                    {
                        next_cmd2_ptr = get_packet_data(packet, 1);
                        next_cmd2_counter++;
                        if (next_cmd2_ptr == NULL)
                            break;
                        if (*next_cmd2_ptr >= TAP_STATE_MIN &&
                            *next_cmd2_ptr <= TAP_STATE_MAX)
                        {
                            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK,
                                    ASD_LogOption_None,
                                    "Next2 is TAP_STATE(0x%x)", *next_cmd2_ptr);
                            break;
                        }
                        else if (*next_cmd2_ptr == WRITE_EVENT_CONFIG ||
                                 *next_cmd2_ptr == DR_PREFIX ||
                                 *next_cmd2_ptr == DR_POSTFIX ||
                                 *next_cmd2_ptr == WAIT_CYCLES_TCK_DISABLE ||
                                 *next_cmd2_ptr == WAIT_CYCLES_TCK_ENABLE ||
                                 *next_cmd2_ptr == JTAG_FREQ)
                        {
                            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK,
                                    ASD_LogOption_None,
                                    "Next2 is 1 byte length: 0x%x",
                                    *next_cmd2_ptr);
                            next_cmd2_ptr = get_packet_data(packet, 1);
                            next_cmd2_counter++;
                            continue;
                        }
                        else if (*next_cmd2_ptr == IR_PREFIX ||
                                 *next_cmd2_ptr == IR_POSTFIX)
                        {
                            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK,
                                    ASD_LogOption_None,
                                    "Next2 is 2 byte length: 0x%x",
                                    *next_cmd2_ptr);
                            next_cmd2_ptr = get_packet_data(packet, 2);
                            next_cmd2_counter = next_cmd2_counter + 2;
                            continue;
                        }
                        else
                        {
                            ASD_log(ASD_LogLevel_Warning, ASD_LogStream_SDK,
                                    ASD_LogOption_None,
                                    "Not expected Next2 command 0x%x",
                                    *next_cmd2_ptr);
                            break;
                        }
                    }
                }

                if (next_cmd2_ptr)
                {
                    if (*next_cmd2_ptr >= TAP_STATE_MIN &&
                        *next_cmd2_ptr <= TAP_STATE_MAX)
                    {
                        if ((*next_cmd2_ptr & TAP_STATE_MASK) == jtag_pau_dr ||
                            (*next_cmd2_ptr & TAP_STATE_MASK) == jtag_pau_ir)
                        {
                            *end_state = (enum jtag_states)(*next_cmd2_ptr &
                                                            TAP_STATE_MASK);
#ifdef ENABLE_DEBUG_LOGGING
                            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_SDK,
                                    ASD_LogOption_None,
                                    "Staying in state: 0x%02x", *end_state);
#endif
                        }
                        else if ((*next_cmd2_ptr & TAP_STATE_MASK) == jtag_rti)
                        {
                            *end_state = jtag_rti;
                        }
                    }
                }
            }
            else if (scan_type == ScanType_Read &&
                     (*next_cmd_ptr < READ_SCAN_MIN ||
                      *next_cmd_ptr > READ_SCAN_MAX))
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Unexpected sequence during read scan: 0x%02x",
                        *next_cmd_ptr);
                status = ST_ERR;
            }
            else if (scan_type == ScanType_Write &&
                     (*next_cmd_ptr < WRITE_SCAN_MIN ||
                      *next_cmd_ptr > WRITE_SCAN_MAX))
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Unexpected sequence during write scan: 0x%02x",
                        *next_cmd_ptr);
                status = ST_ERR;
            }
            else if (scan_type == ScanType_ReadWrite &&
                     (*next_cmd_ptr < READ_WRITE_SCAN_MIN ||
                      *next_cmd_ptr > READ_WRITE_SCAN_MAX))
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK,
                        ASD_LogOption_None,
                        "Unexpected sequence during read write scan: 0x%02x",
                        *next_cmd_ptr);
                status = ST_ERR;
            }

            packet->next_data--; // Un-peek next_cmd_ptr
            packet->used--;
            if (next_cmd2_ptr)
            {
                packet->next_data -= next_cmd2_counter; // Un-peek next_cmd2_ptr
                packet->used -= next_cmd2_counter;
            }
        }
    }
    return status;
}

STATUS asd_msg_read(void)
{
    STATUS result = ST_ERR;
    STATUS status = ST_ERR;
    bool b_data_pending = false;

    struct asd_message* msg = &msg_state.in_msg.msg;
    switch (msg_state.in_msg.read_state)
    {
        case READ_STATE_INITIAL:
        {
            explicit_bzero(&msg->header, sizeof(struct message_header));
            explicit_bzero(&msg->buffer, MAX_DATA_SIZE);
            msg_state.in_msg.read_state = READ_STATE_HEADER;
            msg_state.in_msg.read_index = 0;
            msg_state.in_msg.data_size = 0;
            // do not 'break' here, continue on and read the header
        }
        case READ_STATE_HEADER:
        {
            void* buffer =
                (void*)(&(msg->header) + msg_state.in_msg.read_index);
            size_t num_to_read =
                sizeof(msg->header) - msg_state.in_msg.read_index;

            size_t size = asd_api_server_read(buffer, num_to_read, NULL);

            if (size > 0)
            {
                result = ST_OK;
                num_to_read -= size;
                if (num_to_read == 0)
                {
                    int data_size = get_message_size(msg);
                    if (data_size == -1)
                    {
                        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                                ASD_LogOption_None,
                                "Failed to read header size.");
                        send_error_message( msg,
                                           ASD_FAILURE_HEADER_SIZE);
                        msg_state.in_msg.read_state = READ_STATE_INITIAL;
                        msg_state.in_msg.read_index = 0;
                        result = ST_ERR;
                    }
                    else if (data_size > 0)
                    {
                        msg_state.in_msg.read_state = READ_STATE_BUFFER;
                        msg_state.in_msg.read_index = 0;
                        msg_state.in_msg.data_size = data_size;
                    }
                    else
                    {
                        // we have finished
                        // reading a message and
                        // there is no buffer to
                        // read. Set back to
                        // initial state for
                        // next packet and
                        // process message.
                        msg_state.in_msg.read_state = READ_STATE_INITIAL;
                        result = asd_msg_on_msg_recv();
                        if (result == ST_ERR)
                            break;
                    }
                }
                else
                {
                    msg_state.in_msg.read_index =
                        sizeof(msg->header) - num_to_read;
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Daemon,
                            ASD_LogOption_None,
                            "Socket buffer read not complete (%d of %d)",
                            msg_state.in_msg.read_index, sizeof(msg->header));
#endif
                }
            }

            status = asd_api_server_ioctl(NULL, &b_data_pending,
                                          IOCTL_SERVER_IS_DATA_PENDING);
            if (status == ST_ERR)
                break;
            // we can skip the break if the buffer is ready to read
            if (msg_state.in_msg.read_state != READ_STATE_BUFFER ||
                b_data_pending == false)
                break;
        }
        case READ_STATE_BUFFER:
        {
            void* buffer = (void*)(msg->buffer + msg_state.in_msg.read_index);
            size_t num_to_read = (size_t)(msg_state.in_msg.data_size -
                                     msg_state.in_msg.read_index);

            size_t size = asd_api_server_read(buffer, num_to_read, NULL);

            if (size > 0)
            {
                result = ST_OK;
                num_to_read -= size;
                if (num_to_read == 0)
                {
                    // we have finished reading a
                    // packet. Set back to initial
                    // state for next packet.
                    msg_state.in_msg.read_state = READ_STATE_INITIAL;
                    result = asd_msg_on_msg_recv();
                    if (result == ST_ERR)
                        break;
                }
                else
                {
                    msg_state.in_msg.read_index =
                        msg_state.in_msg.data_size - num_to_read;
#ifdef ENABLE_DEBUG_LOGGING
                    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Daemon,
                            ASD_LogOption_None,
                            "Socket buffer read not complete (%d of %d)",
                            msg_state.in_msg.read_index,
                            msg_state.in_msg.data_size);
#endif
                }
            }
            break;
        }
        default:
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                    ASD_LogOption_None, "Invalid socket read state: %d",
                    msg_state.in_msg.read_state);
        }
    }
    return result;
}

void* get_packet_data(struct packet_data* packet, int bytes_wanted)
{
    void* p;

    if (packet == NULL || packet->used + bytes_wanted > packet->total)
        return NULL;

    p = packet->next_data;

    packet->next_data += bytes_wanted;
    packet->used += bytes_wanted;

    return p;
}

uint8_t lsb_from_msg_size(u_int32_t response_cnt)
{
    return (uint8_t)(response_cnt & (uint8_t)0xFF);
}

uint8_t msb_from_msg_size(u_int32_t response_cnt)
{
    return (uint8_t)((response_cnt >> (uint8_t)8) & (uint8_t)0x1F);
}

int get_message_size(struct asd_message* s_message)
{
    int result = (s_message->header.size_lsb & 0xFF) |
                 ((s_message->header.size_msb & 0x1F) << 8);
    if (result > MAX_DATA_SIZE)
        result = -1;
    return result;
}

STATUS send_response(struct asd_message* message)
{
    unsigned char send_buffer[MAX_PACKET_SIZE];
    size_t send_buffer_size = 0;

    if (!message)
        return ST_ERR;

    int size = get_message_size(message);
    if (size == -1)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                ASD_LogOption_No_Remote,
                "Failed to send message because get message size failed.");
        return ST_ERR;
    }

#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network, ASD_LogOption_No_Remote,
            "Response header:  origin_id: 0x%02x\n"
            "    reserved: 0x%02x    enc_bit: 0x%02x\n"
            "    type: 0x%02x        size_lsb: 0x%02x\n"
            "    size_msb: 0x%02x    tag: 0x%02x\n"
            "    cmd_stat: 0x%02x",
            message->header.origin_id, message->header.reserved,
            message->header.enc_bit, message->header.type,
            message->header.size_lsb, message->header.size_msb,
            message->header.tag, message->header.cmd_stat);
    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network, ASD_LogOption_No_Remote,
            "Response Buffer size: %d", size);
    ASD_log_buffer(ASD_LogLevel_Debug, ASD_LogStream_Network,
                   ASD_LogOption_No_Remote, message->buffer, (size_t)size,
                   "NetRsp");
#endif

    send_buffer_size = sizeof(struct message_header) + size;

    if (memcpy_s(&send_buffer, MAX_PACKET_SIZE,
                 (unsigned char*)&message->header, sizeof(message->header)))
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                "memcpy_s: message header to send_buffer copy failed.");
    }
    if (memcpy_s(&send_buffer[sizeof(message->header)],
                 MAX_PACKET_SIZE - sizeof(message->header), message->buffer,
                 (size_t)size))
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG, ASD_LogOption_None,
                "memcpy_s: message buffer to send buffer offset copy failed.");
    }

    size = asd_api_server_write(send_buffer, send_buffer_size, NULL);

    if (size != send_buffer_size)
        return ST_ERR;

    return ST_OK;
}

STATUS asd_msg_get_fds(target_fdarr_t* fds, int* num_fds)
{
    STATUS result = ST_ERR;

    if (fds == NULL || num_fds == NULL)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon, ASD_LogOption_None,
                "asd_msg_get_fds, invalid param(s)");
    }
    else
    {
        struct asd_message* msg = &msg_state.in_msg.msg;
        // skip target_get_fds call for AGENT_CONTROL_TYPE commands
        if (msg->header.type != AGENT_CONTROL_TYPE)
        {
            result = target_get_fds(msg_state.target_handler, fds, num_fds);
        }
    }
    return result;
}

static STATUS send_pin_event(ASD_EVENT value)
{
    STATUS result;
    struct asd_message message = {{0}};

    message.header.size_lsb = 1;
    message.header.size_msb = 0;
    message.header.type = JTAG_TYPE;
    message.header.tag = BROADCAST_MESSAGE_ORIGIN_ID;
    message.header.origin_id = BROADCAST_MESSAGE_ORIGIN_ID;
    message.buffer[0] = (value & 0xFF);

    result = send_response(&message);
    if (result != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_No_Remote,
                "Failed to send an event message to client");
    }
    return result;
}

STATUS asd_msg_event(struct pollfd poll_fd)
{
    STATUS result;
    ASD_EVENT event;
    struct asd_message* msg = &msg_state.in_msg.msg;

    result = target_event(msg_state.target_handler, poll_fd, &event);

    if (result == ST_OK)
    {
        if (event == ASD_EVENT_XDP_PRESENT)
        {
            // return an error so that everything
            // can be shut down if condition to ignore
            // XDP is not set.
            send_error_message( msg, ASD_FAILURE_XDP_PRESENT);
            if (msg_state.asd_cfg->jtag.xdp_fail_enable == true)
            {
                result = ST_ERR;
            }
        }
        else if (event != ASD_EVENT_NONE)
        {
            result = send_pin_event(event);
        }
    }
    return result;
}

STATUS asd_write_set_active_chain_event(uint8_t scan_chain)
{
    STATUS result;
    result = target_write(msg_state.target_handler, PIN_TCK_MUX_SELECT,
                          scan_chain == SCAN_CHAIN_1);
    if (result != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                "target_jtag_chain_select failed, %d", result);
        return result;
    }
    result = JTAG_set_active_chain(msg_state.jtag_handler, (scanChain)scan_chain);
    if (result != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                "JTAG_set_active_chain failed, %d", result);
    }
    return result;
}

bus_config_type bus_type(uint8_t bus)
{
    for (int i = 0; i < MAX_IxC_BUSES; i++)
    {
        if (msg_state.buscfg->bus_config_map[i] == bus)
            return msg_state.buscfg->bus_config_type[i];
    }
    return BUS_CONFIG_NOT_ALLOWED;
}

STATUS do_bus_select_command(struct packet_data* packet)
{
    STATUS status = ST_OK;
    uint8_t* data_ptr = get_packet_data(packet, 1);

    if (data_ptr == NULL)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Failed to read data for I2C_WRITE_CFG_BUS_SELECT, "
                "short packet");
        return ST_ERR;
    }

    uint8_t bus = (uint8_t)*data_ptr;
    if (msg_state.buscfg == NULL)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "msg_state.buscfg == NULL");
        return ST_ERR;
    }

    // Release lock on previous bus and lock selected bus if required.
    if (msg_state.buscfg->default_bus != bus)
    {
        status = dev_flock(msg_state.buscfg->default_bus, LOCK_UN);
        if (status != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Failed to remove dev lock ");
            send_error_message( (struct asd_message *)&packet, ASD_FAILURE_REMOVE_I2C_LOCK);
        }
        status = dev_flock(bus, LOCK_EX);
        if (status != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                    "Failed to remove dev lock");
            send_error_message( (struct asd_message *)&packet, ASD_FAILURE_PROCESS_I2C_LOCK);
            return ST_ERR;
        }
    }

    switch (bus_type(bus))
    {
        case BUS_CONFIG_I2C:
            if (msg_state.i2c_handler == NULL)
            {
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                        ASD_LogOption_None, "Null i2c handler on bus select");
                status = ST_ERR;
                break;
            }
            status = i2c_bus_select(msg_state.i2c_handler, bus);
            break;
        case BUS_CONFIG_I3C:
            if (msg_state.i3c_handler == NULL)
            {
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                        ASD_LogOption_None, "Null i3c handler on bus select");
                status = ST_ERR;
                break;
            }
            status = i3c_bus_select(msg_state.i3c_handler, bus);
            break;
        case BUS_CONFIG_NOT_ALLOWED:
        default:
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
                    "Bus selection not allowed");
            status = ST_ERR;
    }

    if (status != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "bus select command failed for bus %d", bus);
    }

    return status;
}

STATUS do_set_sclk_command(struct packet_data* packet)
{
    STATUS status = ST_OK;
    uint8_t* data_ptr = get_packet_data(packet, 2);

    if (data_ptr == NULL)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Failed to read data for I2C_WRITE_CFG_SCLK, short packet");
        return ST_ERR;
    }

    uint8_t bus = msg_state.buscfg->default_bus;
    uint8_t lsb = *data_ptr;
    uint8_t msb = *(data_ptr + 1);
    switch (bus_type(bus))
    {
        case BUS_CONFIG_I2C:
            if (msg_state.i2c_handler == NULL)
            {
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                        ASD_LogOption_None, "Null i2c handler on set sclk");
                status = ST_ERR;
                break;
            }
            status =
                i2c_set_sclk(msg_state.i2c_handler, (uint16_t)((msb << 8) + lsb));
            break;
        case BUS_CONFIG_I3C:
            if (msg_state.i3c_handler == NULL)
            {
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                        ASD_LogOption_None, "Null i3c handler on set sclk");
                status = ST_ERR;
                break;
            }
            status =
                i3c_set_sclk(msg_state.i3c_handler, (uint16_t)((msb << 8) + lsb));
            break;
        case BUS_CONFIG_NOT_ALLOWED:
        default:
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
                    "Set sclk not allowed");
            status = ST_ERR;
    }
    if (status != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Set sclk command failed for bus %d", bus);
    }

    return status;
}

STATUS do_read_command(uint8_t cmd, I2C_Msg_Builder* builder,
                       struct packet_data* packet, bool* force_stop)
{
    STATUS status;
    asd_i2c_msg msg;
    uint8_t* data_ptr;
    msg.length = (uint8_t)(cmd & I2C_LENGTH_MASK);
    msg.read = true;

    explicit_bzero(msg.buffer, ASD_I2C_BUFFER_LEN);
    if (force_stop == NULL)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_No_Remote,
                "Invalid force stop");
#endif
        status = ST_ERR;
    }
    else
    {
        *force_stop = false;

        data_ptr = get_packet_data(packet, 1);
        if (data_ptr == NULL)
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                    ASD_LogOption_No_Remote,
                    "Failed to read data for I2C_READ, short packet");
#endif
            status = ST_ERR;
        }
        else
        {
            msg.address = (uint8_t)(*data_ptr & I2C_ADDRESS_MASK) >> 1;
            *force_stop =
                ((*data_ptr & I2C_FORCE_STOP_MASK) == I2C_FORCE_STOP_MASK);

            status = i2c_msg_add(builder, &msg);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C,
                        ASD_LogOption_No_Remote, "i2c_msg_add failed, %d",
                        status);
            }
        }
    }

    return status;
}

STATUS do_write_command(uint8_t cmd, I2C_Msg_Builder* builder,
                        struct packet_data* packet, bool* force_stop)
{
    STATUS status;
    asd_i2c_msg msg;
    uint8_t* data_ptr;

    if (force_stop == NULL)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_No_Remote,
                "Invalid force stop");
#endif
        status = ST_ERR;
    }
    else
    {
        *force_stop = false;
        msg.length = (uint8_t)(cmd & I2C_LENGTH_MASK);
        msg.read = false;

        data_ptr = get_packet_data(packet, 1);
        if (data_ptr == NULL)
        {
#ifdef ENABLE_DEBUG_LOGGING
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                    ASD_LogOption_No_Remote,
                    "Failed to read data for I2C_WRITE address, short packet");
#endif
            status = ST_ERR;
        }
        else
        {
            msg.address = (uint8_t)(*data_ptr & I2C_ADDRESS_MASK) >> 1;
            *force_stop =
                ((*data_ptr & I2C_FORCE_STOP_MASK) == I2C_FORCE_STOP_MASK);
            data_ptr = get_packet_data(packet, msg.length);
            if (data_ptr == NULL)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(
                    ASD_LogLevel_Debug, ASD_LogStream_I2C,
                    ASD_LogOption_No_Remote,
                    "Failed to read data for I2C_WRITE buffer, short packet");
#endif
                status = ST_ERR;
            }
            else
            {
                for (int i = 0; i < msg.length; i++)
                {
                    msg.buffer[i] = data_ptr[i];
                }
                status = i2c_msg_add(builder, &msg);
#ifdef ENABLE_DEBUG_LOGGING
                if (status != ST_OK)
                {
                    ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Network,
                            ASD_LogOption_No_Remote, "i2c_msg_add failed, %d",
                            status);
                }
#endif
            }
        }
    }

    return status;
}

STATUS do_read_write(void* msg_set)
{
    STATUS status;
    uint8_t bus;

    if (msg_set == NULL)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Failed to read data for do_read_write operation");
        return ST_ERR;
    }

    bus = msg_state.buscfg->default_bus;
    switch (bus_type(bus))
    {
        case BUS_CONFIG_I2C:
            if (msg_state.i2c_handler == NULL)
            {
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                        ASD_LogOption_None, "Null i2c handler on read/write");
                status = ST_ERR;
                break;
            }
            status = i2c_read_write(msg_state.i2c_handler, msg_set);
            break;
        case BUS_CONFIG_I3C:
            if (msg_state.i3c_handler == NULL)
            {
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                        ASD_LogOption_None, "Null i3c handler on read/write");
                status = ST_ERR;
                break;
            }
            status = i3c_read_write(msg_state.i3c_handler, msg_set);
            break;
        case BUS_CONFIG_NOT_ALLOWED:
        default:
            ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C, ASD_LogOption_None,
                    "Read/Write not allowed");
            status = ST_ERR;
    }
    if (status != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Read/Write failed on bus %d", bus);
    }

    return status;
}

STATUS build_responses(int* response_cnt,
                       I2C_Msg_Builder* builder, bool ack)
{
    STATUS status;
    asd_i2c_msg msg;
    u_int32_t num_i2c_messages = 0;

    if (!response_cnt || !builder)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Invalid build_respones");
        status = ST_ERR;
    }
    else
    {
        status = i2c_msg_get_count(builder, &num_i2c_messages);

        if (status == ST_OK)
        {
            for (int i = 0; i < num_i2c_messages; i++)
            {
                status = i2c_msg_get_asd_i2c_msg(builder, i, &msg);
                if (status != ST_OK)
                    break;

                if ((msg.length + (*response_cnt) + 2) > MAX_DATA_SIZE)
                { // 2 for command header
                    // and ack
                    // bytes
                    // buffer is too full for this next
                    // packet send response on socket with
                    // packet continuation bit set.
                    msg_state.out_msg.header.size_lsb =
                        (uint32_t)((*response_cnt) & 0xFF);
                    msg_state.out_msg.header.size_msb =
                        (uint32_t)(((*response_cnt) >> 8) & 0x1F);
                    msg_state.out_msg.header.cmd_stat =
                        ASD_SUCCESS | ASD_PACKET_CONTINUATION;
                    status = send_response(&msg_state.out_msg);
                    if (status != ST_OK)
                    {
                        ASD_log(ASD_LogLevel_Error | ASD_LogOption_No_Remote,
                                ASD_LogStream_I2C, ASD_LogOption_None,
                                "Failed to send message back on the socket");
                        break;
                    }
                    explicit_bzero(msg_state.out_msg.buffer, MAX_DATA_SIZE);
                    (*response_cnt) = 0;
                }

                uint8_t cmd_byte =
                    (uint8_t)(msg.read ? I2C_READ_MIN : I2C_WRITE_MIN) +
                    msg.length;
                msg_state.out_msg.buffer[(*response_cnt)++] = cmd_byte;
                // Linux driver does not return the address or
                // read ACKs, but Open IPC and the Aurora SDK
                // do. So we will just fake it here.
                if (ack)
                    msg_state.out_msg.buffer[(*response_cnt)++] =
                        (uint8_t)(I2C_ADDRESS_ACK + msg.length);
                else
                    msg_state.out_msg.buffer[(*response_cnt)++] =
                        (uint8_t)(msg.length);

                if (msg.read)
                {
                    for (int j = 0; j < msg.length; j++)
                    {
                        msg_state.out_msg.buffer[(*response_cnt)++] =
                            msg.buffer[j]; // data
                    }
                }
            }
        }
    }

    return status;
}

STATUS process_i2c_messages(struct asd_message* in_msg)
{
    int response_cnt = 0;
    STATUS status;
    int size;
    struct packet_data packet;
    uint8_t* data_ptr;
    uint8_t cmd;
    bool i2c_command_pending = false;
    bool force_stop = false;

    if (!in_msg)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Invalid process_i2c_messages input.");
        return ST_ERR;
    }

    I2C_Msg_Builder* builder = I2CMsgBuilder();

    if (!builder)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                "Invalid process_i2c_messages input.");
        status = ST_ERR;
    }
    else
    {
        size = get_message_size(in_msg);
        if (size == -1)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C, ASD_LogOption_None,
                    "Failed to process i2c message because "
                    "get message size failed.");
            status = ST_ERR;
        }
        else
        {

            explicit_bzero(&msg_state.out_msg.header,
                           sizeof(struct message_header));
            explicit_bzero(&msg_state.out_msg.buffer, MAX_DATA_SIZE);
            if (memcpy_s(&msg_state.out_msg.header, sizeof(struct message_header),
                         &in_msg->header, sizeof(struct message_header)))
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_JTAG,
                        ASD_LogOption_None, "memcpy_s: message header to \
							out msg header copy failed.");
            }
            status = i2c_msg_initialize(builder);
            if (status != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C,
                        ASD_LogOption_None,
                        "Failed to initialize i2c msg builder.");
            }
            else
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                        ASD_LogOption_None, "NetReq tag: %d size: %d",
                        in_msg->header.tag, size);
                ASD_log_buffer(ASD_LogLevel_Debug, ASD_LogStream_I2C,
                               ASD_LogOption_None, in_msg->buffer, (size_t)size,
                               "NetReq");
#endif
                packet.next_data = in_msg->buffer;
                packet.used = 0;
                packet.total = size;

                while (packet.used < packet.total)
                {
                    data_ptr = get_packet_data(&packet, 1);
                    if (data_ptr == NULL)
                    {
                        ASD_log(ASD_LogLevel_Error, ASD_LogStream_SDK, ASD_LogOption_None,
                                "Failed to read data for cmd, short packet");
                        status = ST_ERR;
                        break;
                    }
                    else
                    {
                        cmd = *data_ptr;
                    }
                    if (cmd == I2C_WRITE_CFG_BUS_SELECT)
                    {
                        status = do_bus_select_command(&packet);
                        if (status != ST_OK)
                        {
                            msg_state.out_msg.header.size_lsb =
                                (uint32_t)(response_cnt & 0xFF);
                            msg_state.out_msg.header.size_msb =
                                (uint32_t)((response_cnt >> 8) & 0x1F);
                            msg_state.out_msg.header.cmd_stat =
                                ASD_FAILURE_PROCESS_I2C_MSG;
                            status = send_response(&msg_state.out_msg);
                            if (status != ST_OK)
                            {
                                ASD_log(ASD_LogLevel_Error |
                                            ASD_LogOption_No_Remote,
                                        ASD_LogStream_I2C, ASD_LogOption_None,
                                        "Failed to send error message back on "
                                        "the socket - process_i2c_message");
                            }
                            continue;
                        }
                    }
                    else if (cmd == I2C_WRITE_CFG_SCLK)
                    {
                        status = do_set_sclk_command(&packet);
                    }
                    else if (cmd >= I2C_READ_MIN && cmd <= I2C_READ_MAX)
                    {
                        status =
                            do_read_command(cmd, builder, &packet, &force_stop);
                        i2c_command_pending = true;
                    }
                    else if (cmd >= I2C_WRITE_MIN && cmd <= I2C_WRITE_MAX)
                    {
                        status = do_write_command(cmd, builder, &packet,
                                                  &force_stop);
                        i2c_command_pending = true;
                    }
                    else
                    {
                        // Unknown Command
                        ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C,
                                ASD_LogOption_None,
                                "Encountered unknown i2c command 0x%02x",
                                (int)cmd);
                        status = ST_ERR;
                        break;
                    }

                    if (status != ST_OK)
                        break;

                    // if a i2c command is pending and
                    // either we are done looping through
                    // all commands, or the force stop bit
                    // was set, then we need to also flush
                    // the commands
                    if (i2c_command_pending &&
                        ((packet.used == packet.total) || force_stop))
                    {
                        i2c_command_pending = false;
                        force_stop = false;
                        status = do_read_write(builder->msg_set);
                        if (status != ST_OK)
                        {
                            ASD_log(ASD_LogLevel_Error, ASD_LogStream_I2C,
                                    ASD_LogOption_None,
                                    "i2c_read_write failed, %d, assuming NAK",
                                    status);
                        }
                        status = build_responses(&response_cnt, builder,
                                                 (status == ST_OK));
                        if (status != ST_OK)
                        {
                            ASD_log(
                                ASD_LogLevel_Error, ASD_LogStream_I2C,
                                ASD_LogOption_None,
                                "i2c_read_write failed to parse i2c responses"
                                ", %d",
                                status);
                            break;
                        }
                        status = i2c_msg_reset(builder);
                        if (status != ST_OK)
                        {
                            ASD_log(
                                ASD_LogLevel_Error, ASD_LogStream_I2C,
                                ASD_LogOption_None,
                                "i2c_msg_reset failed to reset response builder"
                                ", %d",
                                status);
                            break;
                        }
                    }
                }
            }
        }
    }

    if (status == ST_OK)
    {
        msg_state.out_msg.header.size_lsb = (uint32_t)(response_cnt & 0xFF);
        msg_state.out_msg.header.size_msb = (uint32_t)((response_cnt >> 8) & 0x1F);
        msg_state.out_msg.header.cmd_stat = ASD_SUCCESS;
        status = send_response(&msg_state.out_msg);
        if (status != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error | ASD_LogOption_No_Remote,
                    ASD_LogStream_I2C, ASD_LogOption_None,
                    "Failed to send message back on the socket - "
                    "process_i2c_message");
        }
    }
    if (builder)
    {
        i2c_msg_deinitialize(builder);
        free(builder);
    }
    return status;
}
