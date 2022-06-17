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

#include "asd_main.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <pthread.h>
#include <safe_str_lib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#ifdef HAVE_DBUS
#include <systemd/sd-journal.h>
#endif
#include <unistd.h>
#include <syslog.h>

#include "target_handler.h"

#ifndef UNIT_TEST_MAIN
int main(int argc, char** argv)
{
    return asd_main(argc, argv);
}
#endif

int asd_main(int argc, char** argv)
{
    STATUS result = ST_ERR;
    asd_state state = {};
    uint8_t jumper_status = 0;

    ASD_initialize_log_settings(DEFAULT_LOG_LEVEL, DEFAULT_LOG_STREAMS, false,
                                NULL, NULL);

    if (process_command_line(argc, argv, &state.args))
    {
        ASD_log(ASD_LogLevel_Warning, ASD_LogStream_Daemon,
                ASD_LogOption_None, "ASD is initializing...");

        result = pal_is_jumper_enable(state.args.fru, &jumper_status);
        if (result == ST_OK) {
           if (jumper_status == ASD_HW_JUMPER_POP) {
              ASD_log(ASD_LogLevel_Warning, ASD_LogStream_Daemon,
                ASD_LogOption_None, "Jumper popped, cannot do ASD.");

              syslog(LOG_CRIT, "Jumper popped, cannot do ASD.");
              return ST_ERR;
           }
        }

        result = init_asd_state(&state);
        if (result == ST_OK)
        {
            ASD_log(ASD_LogLevel_Warning, ASD_LogStream_Daemon,
                    ASD_LogOption_None, "ASD is initialized.");
            result = request_processing_loop(&state);
            ASD_log(ASD_LogLevel_Warning, ASD_LogStream_Daemon,
                    ASD_LogOption_None, "ASD server closing.");
        }
        deinit_asd_state(&state);
        free(state.extnet);
        free(state.session);
    }
    return result == ST_OK ? 0 : 1;
}

bool process_command_line(int argc, char** argv, asd_args* args)
{
    int c = 0;
    opterr = 0;              // prevent getopt_long from printing shell messages
    uint8_t bus_counter = 0; // Up to 4 buses

    // Set Default argument values.

    args->busopt.enable_i2c = DEFAULT_I2C_ENABLE;
    args->busopt.enable_i3c = DEFAULT_I3C_ENABLE;
    args->busopt.bus = DEFAULT_I2C_BUS;
    args->use_syslog = DEFAULT_LOG_TO_SYSLOG;
    args->log_level = DEFAULT_LOG_LEVEL;
    args->log_streams = DEFAULT_LOG_STREAMS;
    args->session.n_port_number = DEFAULT_PORT;
    args->session.cp_certkeyfile = DEFAULT_CERT_FILE;
    args->session.cp_net_bind_device = NULL;
    args->session.e_extnet_type = EXTNET_HDLR_NON_ENCRYPT;
    args->session.e_auth_type = AUTH_HDLR_NONE;
    args->xdp_fail_enable = DEFAULT_XDP_FAIL_ENABLE;
    args->force_jtag_hw = DEFAULT_FORCE_JTAG_HW;
    args->msg_flow = DEFAULT_JTAG_FLOW;
    args->fru = DEFAULT_FRU;

    enum
    {
        ARG_LOG_LEVEL = 256,
        ARG_LOG_STREAMS,
        ARG_HELP,
        ARG_XDP
    };

    struct option opts[] = {
        {"xdp-ignore", 0, NULL, ARG_XDP},
        {"log-level", 1, NULL, ARG_LOG_LEVEL},
        {"log-streams", 1, NULL, ARG_LOG_STREAMS},
        {"help", 0, NULL, ARG_HELP},
        {NULL, 0, NULL, 0},
    };

    while ((c = getopt_long(argc, argv, "p:uk:n:si:c:f:j:m", opts, NULL)) != -1)
    {
        switch (c)
        {
            case 'f':
            {
                uint8_t fru = (uint8_t)strtol(optarg, NULL, 10);
                args->fru = fru;
                break;
            }
            case 'j':
            {
                args->msg_flow = (uint8_t)strtol(optarg, NULL, 10);
                break;
            }
            case 'm':
            {
                args->force_jtag_hw = true;
                break;
            }
            case 'p':
            {
                uint16_t port = (uint16_t)strtol(optarg, NULL, 10);
                fprintf(stderr, "Setting Port: %d\n", port);
                args->session.n_port_number = port;
                break;
            }
            case 's':
            {
                args->use_syslog = true;
                break;
            }
            case 'u':
            {
                args->session.e_extnet_type = EXTNET_HDLR_NON_ENCRYPT;
                args->session.e_auth_type = AUTH_HDLR_NONE;
                break;
            }
            case 'k':
            {
                args->session.cp_certkeyfile = optarg;
                break;
            }
            case 'n':
            {
                args->session.cp_net_bind_device = optarg;
                break;
            }
            case 'i':
            {
                char* pch;
                uint8_t bus;
                bool first_i2c = true;
                char* endptr;
                args->busopt.enable_i2c = true;
                pch = strtok(optarg, ",");
                while (pch != NULL)
                {
                    errno = 0;
                    bus = (uint8_t)strtol(pch, &endptr, 10);
                    if ((errno == ERANGE) || (endptr == pch))
                    {
                        fprintf(stderr, "Wrong I2C bus list arguments(-i)\n");
                        break;
                    }
                    if (bus_counter >= MAX_IxC_BUSES)
                    {
                        fprintf(stderr, "Discard I2C bus: %d\n", bus);
                    }
                    else
                    {
                        if (first_i2c)
                        {
                            args->busopt.bus = bus;
                            first_i2c = false;
                        }
                        fprintf(stderr, "Enabling I2C bus: %d\n", bus);
                        args->busopt.bus_config_type[bus_counter] =
                            BUS_CONFIG_I2C;
                        args->busopt.bus_config_map[bus_counter] = bus;
                    }
                    pch = strtok(NULL, ",");
                    bus_counter++;
                }
                break;
            }
            case 'c':
            {
                char* pch;
                uint8_t bus;
                bool first_i3c = true;
                char* endptr;
                args->busopt.enable_i3c = true;
                pch = strtok(optarg, ",");
                while (pch != NULL)
                {
                    errno = 0;
                    bus = (uint8_t)strtol(pch, &endptr, 10);
                    if ((errno == ERANGE) || (endptr == pch))
                    {
                        fprintf(stderr, "Wrong I3C bus list arguments(-c)\n");
                        break;
                    }
                    if (bus_counter >= MAX_IxC_BUSES)
                    {
                        fprintf(stderr, "Discard I3C bus: %d\n", bus);
                    }
                    else
                    {
                        if (first_i3c)
                        {
                            args->busopt.bus = bus;
                            first_i3c = false;
                        }
                        fprintf(stderr, "Enabling I3C bus: %d\n", bus);
                        args->busopt.bus_config_type[bus_counter] =
                            BUS_CONFIG_I3C;
                        args->busopt.bus_config_map[bus_counter] = bus;
                    }
                    pch = strtok(NULL, ",");
                    bus_counter++;
                }
                break;
            }
            case ARG_XDP:
            {
                args->xdp_fail_enable = false;
                fprintf(stderr, "Ignore XDP presence\n");
                break;
            }
            case ARG_LOG_LEVEL:
            {
                if (!strtolevel(optarg, &args->log_level))
                {
                    showUsage(argv);
                    return false;
                }
                break;
            }
            case ARG_LOG_STREAMS:
            {
                if (!strtostreams(optarg, &args->log_streams))
                {
                    showUsage(argv);
                    return false;
                }
                break;
            }
            case ARG_HELP:
            default:
            {
                showUsage(argv);
                return false;
            }
        }
    }

    fprintf(stderr, "ASD ver: %s\n", asd_version);
    fprintf(stderr, "Setting Port: %d, FRU: %d\n", args->session.n_port_number, args->fru);
    fprintf(stderr, "Msg Flow: %d\n", args->msg_flow);
    fprintf(stderr, "Log level: %s\n", ASD_LogLevelString[args->log_level]);
    if (args->force_jtag_hw)
        fprintf(stderr, "Force JTAG driver in HW mode\n");
    return true;
}

void showUsage(char** argv)
{
    ASD_log(
        ASD_LogLevel_Error, ASD_LogStream_Daemon, ASD_LogOption_No_Remote,
        "\nUsage: %s [option]\n\n"
        "  -p <number> Port number (default=%d)\n\n"
        "  -f <number> FRU number (default=%d)\n"
        "  -j <number> JTAG flow (default=%d, BMC = 1, BIC = 2)\n"
        "  -m          Force JTAG driver in HW mode\n"
        "  -s          Route log messages to the system log\n"
        //"  -u          Run in plain TCP, no SSL (default: SSL/Auth Mode)\n"
        //"  -k <file>   Specify SSL Certificate/Key file (default: %s)\n"
        "  -n <device> Bind only to specific network device (eth0, etc)\n"
        "  -i <buses>  Decimal i2c allowed bus list(default: none)\n"
        "              Use comma to enable multiple i2c buses: -i 2,9\n"
        "              The first bus will be used as default bus.\n"
        "              The total number of i2c/i3c bus assignments cannot\n"
        "              exceed 4 buses.\n"
        "  -c <buses>  Decimal i3c allowed bus list(default: none)\n"
        "              Use comma to enable multiple i3c buses: -c 0,1,2,3\n"
        "              The first bus will be used as default bus.\n"
        "              The total number of i2c/i3c bus assignments cannot\n"
        "              exceed 4 buses.\n"
        "  --xdp-ignore               Connect ASD even with XDP connected\n"
        "                             Warning: Driving signals from both\n"
        "                             ASD and XDP may cause electrical issues\n"
        "                             or lead into a HW damage.\n"
        "  --log-level=<level>        Specify Logging Level (default: %s)\n"
        "                             Levels:\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "  --log-streams=<streams>    Specify Logging Streams (default: %s)\n"
        "                             Multiple streams can be comma "
        "separated.\n"
        "                             Streams:\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "                               %s\n"
        "  --help                     Show this list\n"
        "\n"
        "Examples:\n"
        "\n"
        "Log from the daemon and jtag at trace level.\n"
        "     asd --log-level=trace --log-streams=daemon,jtag\n"
        "Enable i2c bus 2 and bus 9.\n"
        "     asd -i 2,9\n"
        "\n"
        "Default logging, only listen on eth0.\n"
        "     asd -n eth0\n"
        "\n",
        argv[0], DEFAULT_PORT, DEFAULT_FRU, DEFAULT_JTAG_FLOW, //DEFAULT_CERT_FILE,
        ASD_LogLevelString[DEFAULT_LOG_LEVEL],
        ASD_LogLevelString[ASD_LogLevel_Off],
        ASD_LogLevelString[ASD_LogLevel_Error],
        ASD_LogLevelString[ASD_LogLevel_Warning],
        ASD_LogLevelString[ASD_LogLevel_Info],
        ASD_LogLevelString[ASD_LogLevel_Debug],
        ASD_LogLevelString[ASD_LogLevel_Trace],
        streamtostring(DEFAULT_LOG_STREAMS), streamtostring(ASD_LogStream_All),
        streamtostring(ASD_LogStream_Test), streamtostring(ASD_LogStream_I2C),
        streamtostring(ASD_LogStream_Pins), streamtostring(ASD_LogStream_JTAG),
        streamtostring(ASD_LogStream_Network),
        streamtostring(ASD_LogStream_Daemon),
        streamtostring(ASD_LogStream_SDK));
}

STATUS init_asd_state(asd_state* state)
{

    STATUS result = set_config_defaults(&state->config, &state->args.busopt);

    if (result == ST_OK)
    {
        ASD_initialize_log_settings(state->args.log_level,
                                    state->args.log_streams,
                                    state->args.use_syslog, NULL, NULL);

        state->extnet =
            extnet_init(state->args.session.e_extnet_type,
                        state->args.session.cp_certkeyfile, MAX_SESSIONS);
        if (!state->extnet)
        {
            result = ST_ERR;
        }
    }

    if (result == ST_OK)
    {
        result = auth_init(state->args.session.e_auth_type, NULL);
    }

    if (result == ST_OK)
    {
        state->session = session_init(state->extnet);
        if (!state->session)
            result = ST_ERR;
        else
        {
            state->event_fd = eventfd(0, O_NONBLOCK);
            if (state->event_fd == -1)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                        ASD_LogOption_None,
                        "Could not setup event file descriptor.");
                result = ST_ERR;
            }
            else
            {
                result = extnet_open_external_socket(
                    state->extnet, state->args.session.cp_net_bind_device,
                    state->args.session.n_port_number, &state->host_fd);
                if (result != ST_OK)
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                            ASD_LogOption_None,
                            "Could not open the external socket");
            }
        }
    }

    return result;
}

void deinit_asd_state(asd_state* state)
{
    session_close_all(state->session);
    if (state->host_fd != 0)
        close(state->host_fd);
    if (state->asd_msg)
    {
        if (asd_msg_free(state->asd_msg) != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                    ASD_LogOption_None, "Failed to de-initialize the asd_msg");
        }
        free(state->asd_msg);
        state->asd_msg = NULL;
    }
}

STATUS send_out_msg_on_socket(void* state, unsigned char* buffer, size_t length)
{
    extnet_conn_t authd_conn;

    int cnt = 0;
    STATUS result = ST_ERR;

    if (state && buffer)
    {
        result = ST_OK;
        if (session_get_authenticated_conn(((asd_state*)state)->session,
                                           &authd_conn) != ST_OK)
        {
            result = ST_ERR;
        }

        if (result == ST_OK)
        {
            cnt = extnet_send(((asd_state*)state)->extnet, &authd_conn, buffer,
                              length);
            if (cnt != length)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                        ASD_LogOption_No_Remote,
                        "Failed to write to the socket: %d", cnt);
                result = ST_ERR;
            }
        }
    }

    return result;
}

STATUS request_processing_loop(asd_state* state)
{
    STATUS result = ST_OK;
    struct pollfd poll_fds[MAX_FDS] = {{0}};

    poll_fds[HOST_FD_INDEX].fd = state->host_fd;
    poll_fds[HOST_FD_INDEX].events = POLLIN;
    while (1)
    {
        session_fdarr_t session_fds = {-1};
        target_fdarr_t target_fds = {-1};
        int n_clients = 0, i;
        int n_gpios = 0;
        int n_timeout = -1;
        int client_fd_index = 0;
        if (state->asd_msg)
        {
            if (asd_msg_get_fds(state->asd_msg, &target_fds, &n_gpios) == ST_OK)
            {
                for (i = 0; i < n_gpios; i++)
                {
                    poll_fds[GPIO_FD_INDEX + i] = target_fds[i];
                }
            }
        }
        if (result == ST_OK)
        {
            client_fd_index = GPIO_FD_INDEX + n_gpios;
            if (session_getfds(state->session, &session_fds, &n_clients,
                               &n_timeout) != ST_OK)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Daemon,
                        ASD_LogOption_None, "Cannot get client session fds!");
#endif
                result = ST_ERR;
            }
            else
            {
                for (i = 0; i < n_clients; i++)
                {
                    poll_fds[client_fd_index + i].fd = session_fds[i];
                    poll_fds[client_fd_index + i].events = POLLIN;
                }
            }
        }
        if (result == ST_OK &&
            poll(poll_fds, (nfds_t)(client_fd_index + n_clients + n_gpios),
                 n_timeout) == -1)
        {
            result = ST_ERR;
        }
        if (result == ST_OK)
        {
            if (poll_fds[HOST_FD_INDEX].revents & POLLIN)
                process_new_client(state, poll_fds, MAX_FDS, &n_clients,
                                   client_fd_index);
            if (state->asd_msg && n_gpios > 0)
            {
                if (process_all_gpio_events(
                        state, (const struct pollfd*)(&poll_fds[GPIO_FD_INDEX]),
                        (size_t)n_gpios) != ST_OK)
                {
                    close_connection(state);
                    continue;
                }
            }
            process_all_client_messages(
                state, (const struct pollfd*)(&poll_fds[client_fd_index]),
                (size_t)n_clients);
        }
        if (result != ST_OK)
            break;
    }
    return result;
}

STATUS close_connection(asd_state* state)
{
    STATUS result = ST_OK;
    extnet_conn_t authd_conn;

    if (session_get_authenticated_conn(state->session, &authd_conn) != ST_OK)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon, ASD_LogOption_None,
                "Authorized client already disconnected.");
    }
    else
    {
        ASD_log(ASD_LogLevel_Warning, ASD_LogStream_Daemon, ASD_LogOption_None,
                "Disconnecting client.");
        result = on_client_disconnect(state);
        if (result == ST_OK)
            session_close(state->session, &authd_conn);
    }
    return result;
}

STATUS process_new_client(asd_state* state, struct pollfd* poll_fds,
                          size_t num_fds, int* num_clients, int client_index)
{
    ASD_log(ASD_LogLevel_Warning, ASD_LogStream_Daemon, ASD_LogOption_None,
            "Client Connecting.");
    extnet_conn_t new_extconn;
    STATUS result = ST_OK;

    if (!state || !poll_fds || !num_clients)
        result = ST_ERR;

    if (result == ST_OK)
    {
        syslog(LOG_CRIT, "ASD service connecting, FRU: %u", state->args.fru);
        result = extnet_accept_connection(state->extnet, state->host_fd,
                                          &new_extconn);
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                    ASD_LogOption_None,
                    "Failed to accept incoming connection.");
            on_connection_aborted();
        }
    }

    if (result == ST_OK)
    {
        result = session_open(state->session, &new_extconn);
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                    ASD_LogOption_None,
                    "Unable to add session for new connection fd %d",
                    new_extconn.sockfd);
            extnet_close_client(state->extnet, &new_extconn);
        }
    }

    if (result == ST_OK && state->args.session.e_auth_type == AUTH_HDLR_NONE)
    {
        /* Special case where auth is not required.
         * Stuff fd to the poll_fds array to immediately
         * process the connection. Otherwise it may be
         * timed out as unauthenticated. */
        if (client_index + (*num_clients) < num_fds)
        {
            poll_fds[client_index + *num_clients].fd = new_extconn.sockfd;
            poll_fds[client_index + *num_clients].revents |= POLLIN;
            (*num_clients)++;
        }
    }
    return result;
}

STATUS process_all_gpio_events(asd_state* state, const struct pollfd* poll_fds,
                               size_t num_fds)
{
    STATUS result = ST_OK;

    for (int i = 0; i < num_fds; i++)
    {
        result = asd_msg_event(state->asd_msg, poll_fds[i]);
        if (result != ST_OK)
            break;
    }
    return result;
}

STATUS process_all_client_messages(asd_state* state,
                                   const struct pollfd* poll_fds,
                                   size_t num_fds)
{
    STATUS result = ST_OK;
    if (!state || !poll_fds)
    {
        result = ST_ERR;
    }
    else
    {
        session_close_expired_unauth(state->session);

        for (int i = 0; i < num_fds; i++)
        {
            struct pollfd poll_fd = poll_fds[i];
            if ((poll_fd.revents & POLLIN) == POLLIN)
            {
                STATUS client_result = process_client_message(state, poll_fd);

                // If we error processing 1 client, we will
                // still process the others
                if (client_result != ST_OK)
                    result = client_result;
            }
        }
    }
    return result;
}

STATUS process_client_message(asd_state* state, const struct pollfd poll_fd)
{
    STATUS result = ST_OK;
    bool b_data_pending = false;
    extnet_conn_t* p_extconn = NULL;

    if (!state)
        result = ST_ERR;

    if (result == ST_OK)
    {
        p_extconn = session_lookup_conn(state->session, poll_fd.fd);
        if (!p_extconn)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                    ASD_LogOption_None, "Session for fd %d vanished!",
                    poll_fd.fd);
            result = ST_ERR;
        }
    }

    if (result == ST_OK)
    {
        result = session_get_data_pending(state->session, p_extconn,
                                          &b_data_pending);
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                    ASD_LogOption_None,
                    "Cannot get session data pending for fd %d!", poll_fd.fd);
        }
    }

    if (result == ST_OK && (b_data_pending || poll_fd.revents & POLLIN))
    {
        result = ensure_client_authenticated(state, p_extconn);

        if (result == ST_OK)
        {
            state->asd_msg->xdp_fail_enable = state->args.xdp_fail_enable;
            result = asd_msg_read(state->asd_msg, p_extconn, &b_data_pending);
            if (result != ST_OK)
            {
                on_client_disconnect(state);
                session_close(state->session, p_extconn);
            }
            else
            {
                result = session_set_data_pending(state->session, p_extconn,
                                                  b_data_pending);
            }
        }
    }
    return result;
}

STATUS read_data(asd_state* state, extnet_conn_t* p_extconn, void* buffer,
                 size_t* num_to_read, bool* b_data_pending)
{
    STATUS result = ST_ERR;
    if (state && p_extconn && buffer && num_to_read && b_data_pending)
    {

        int cnt = extnet_recv(state->extnet, p_extconn, buffer, *num_to_read,
                              b_data_pending);

        if (cnt < 1)
        {
            if (cnt == 0) {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                        ASD_LogOption_None, "Client disconnected");
                syslog(LOG_CRIT, "ASD service disconnected, FRU: %u",
                       state->args.fru);
            }
            else
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                        ASD_LogOption_None, "Socket buffer receive failed: %d",
                        cnt);
        }
        else
        {
            (*num_to_read) -= cnt;
            result = ST_OK;
        }
    }
    return result;
}

STATUS ensure_client_authenticated(asd_state* state, extnet_conn_t* p_extconn)
{
    STATUS result = ST_ERR;
    if (state && p_extconn)
    {
        result = session_already_authenticated(state->session, p_extconn);
        if (result != ST_OK)
        {
            // Authenticate the client
            result =
                auth_client_handshake(state->session, state->extnet, p_extconn);
            if (result == ST_OK)
            {
                result = session_auth_complete(state->session, p_extconn);
            }
            if (result == ST_OK)
            {
#ifdef ENABLE_DEBUG_LOGGING
                ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Daemon,
                        ASD_LogOption_None,
                        "Session on fd %d now authenticated",
                        p_extconn->sockfd);
#endif

                result = on_client_connect(state, p_extconn);
                if (result != ST_OK)
                {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                            ASD_LogOption_None, "Connection attempt failed.");
                    on_client_disconnect(state);
                }
            }

            if (result != ST_OK)
            {
                on_connection_aborted();
                session_close(state->session, p_extconn);
            }
        }
    }
    return result;
}

STATUS __attribute__((weak))
init_passthrough_path(void *state, uint8_t fru,
                      STATUS (*send_back_to_client)(void* state, unsigned char* buffer, size_t length))
{
    return ST_ERR;
}

STATUS on_client_connect(asd_state* state, extnet_conn_t* p_extcon)
{
    STATUS result = ST_OK;
    static bus_options target_bus_options;

    if (!state || !p_extcon)
    {
        result = ST_ERR;
    }

    if (result == ST_OK)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Daemon, ASD_LogOption_None,
                "Preparing for client connection");
#endif

        log_client_address(p_extcon);

        result = target_get_i2c_i3c_config(&target_bus_options);

#ifdef ENABLE_DEBUG_LOGGING
        if (result != ST_OK)
        {
            ASD_log(ASD_LogLevel_Warning, ASD_LogStream_Daemon,
                    ASD_LogOption_No_Remote,
                    "Failed to read i2c/i3c platform config");
        }
#endif
        if (state->args.busopt.enable_i2c || state->args.busopt.enable_i3c)
        {
            result = set_config_defaults(&state->config, &state->args.busopt);
        }
        else
        {
            for (int i = 0; i < MAX_IxC_BUSES; i++)
            {
                if (target_bus_options.bus_config_type[i] == BUS_CONFIG_I2C)
                {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                            ASD_LogOption_No_Remote, "Enabling I2C bus: %d",
                            target_bus_options.bus_config_map[i]);
                }
                if (target_bus_options.bus_config_type[i] == BUS_CONFIG_I3C)
                {
                    ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                            ASD_LogOption_No_Remote, "Enabling I3C bus: %d",
                            target_bus_options.bus_config_map[i]);
                }
            }
            result = set_config_defaults(&state->config, &target_bus_options);
        }
    }

    if (result == ST_OK)
    {
        state->asd_msg =
            asd_msg_init(&send_out_msg_on_socket, (ReadFunctionPtr)&read_data,
                         (void*)state, &state->config);
        if (!state->asd_msg)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                    ASD_LogOption_None, "Failed to create asd_msg.");
            result = ST_ERR;
        }
    }

    if (result == ST_OK)
    {
        // Provide params to each handler
        state->asd_msg->jtag_handler->fru = state->args.fru;
        state->asd_msg->jtag_handler->msg_flow = state->args.msg_flow;
        state->asd_msg->jtag_handler->force_jtag_hw = state->args.force_jtag_hw;
        state->asd_msg->target_handler->fru = state->args.fru;
        // init passthrough
        if (state->asd_msg->jtag_handler->msg_flow == JFLOW_BIC) {
            result = init_passthrough_path(state, state->args.fru, state->asd_msg->send_function);
            if (result == ST_ERR) {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                        ASD_LogOption_None, "Failed to init passthrought path.");
            }
        }

        ASD_initialize_log_settings(
            state->args.log_level, state->args.log_streams,
            state->args.use_syslog, state->asd_msg->should_remote_log,
            state->asd_msg->send_remote_logging_message);
    }

    return result;
}

void log_client_address(const extnet_conn_t* p_extcon)
{
    struct sockaddr_in6 addr;
    uint8_t client_addr[INET6_ADDRSTRLEN];
    socklen_t addr_sz = sizeof(addr);
#ifdef HAVE_DBUS
    int retcode = 0;
#endif

    if (!getpeername(p_extcon->sockfd, (struct sockaddr*)&addr, &addr_sz))
    {
        if (inet_ntop(AF_INET6, &addr.sin6_addr, client_addr,
                      sizeof(client_addr)))
        {
            ASD_log(ASD_LogLevel_Info, ASD_LogStream_Daemon, ASD_LogOption_None,
                    "client %s connected", client_addr);
        }
        else
        {
            if (strcpy_s(client_addr, INET6_ADDRSTRLEN, "address unknown"))
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                        ASD_LogOption_None, "strcpy_safe: address unknown");
            }
        }
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Info, ASD_LogStream_Daemon, ASD_LogOption_None,
                "ASD is now connected %s", client_addr);
#endif
#ifdef HAVE_DBUS
        // Log ASD connection event into the systems logs
        retcode = sd_journal_send(
            "MESSAGE=At-Scale-Debug is now connected", "PRIORITY=%i", LOG_INFO,
            "REDFISH_MESSAGE_ID=%s", "OpenBMC.0.1.AtScaleDebugConnected",
            "REDFISH_MESSAGE_ARGS=%s", client_addr, NULL);
        if (retcode < 0)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                    ASD_LogOption_None, "sd_journal_send failed %d", retcode);
        }
#endif
    }
}

STATUS on_client_disconnect(asd_state* state)
{
    STATUS result = ST_OK;
#ifdef HAVE_DBUS
    int retcode;
#endif

    if (!state)
    {
        result = ST_ERR;
    }

    if (result == ST_OK)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Debug, ASD_LogStream_Daemon, ASD_LogOption_None,
                "Cleaning up after client connection");
#endif

        result = set_config_defaults(&state->config, &state->args.busopt);
    }

    if (result == ST_OK)
    {
        ASD_initialize_log_settings(state->args.log_level,
                                    state->args.log_streams,
                                    state->args.use_syslog, NULL, NULL);

        if (state->asd_msg)
        {
            if (asd_msg_free(state->asd_msg) != ST_OK)
            {
                ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                        ASD_LogOption_None,
                        "Failed to de-initialize the asd_msg");
                result = ST_ERR;
            }
            free(state->asd_msg);
            state->asd_msg = NULL;
        }
    }

    if (result == ST_OK)
    {
#ifdef ENABLE_DEBUG_LOGGING
        ASD_log(ASD_LogLevel_Info, ASD_LogStream_Daemon, ASD_LogOption_None,
                "ASD is now disconnected");
#endif
#ifdef HAVE_DBUS
        // Log ASD connection event into the systems logs
        retcode =
            sd_journal_send("MESSAGE=At-Scale-Debug is now disconnected",
                            "PRIORITY=%i", LOG_INFO, "REDFISH_MESSAGE_ID=%s",
                            "OpenBMC.0.1.AtScaleDebugDisconnected", NULL);

        if (retcode < 0)
        {
            ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon,
                    ASD_LogOption_None, "sd_journal_send failed %d", retcode);
        }
#endif
    }
    return result;
}

void on_connection_aborted(void)
{
#ifdef HAVE_DBUS
    int retcode = 0;
#endif
    // log connection aborted
#ifdef ENABLE_DEBUG_LOGGING
    ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon, ASD_LogOption_None,
            "ASD connection aborted");
#endif
#ifdef HAVE_DBUS
    retcode = sd_journal_send("MESSAGE=At-Scale-Debug connection failed",
                              "PRIORITY=%i", LOG_INFO, "REDFISH_MESSAGE_ID=%s",
                              "OpenBMC.0.1.AtScaleDebugConnectionFailed", NULL);

    if (retcode < 0)
    {
        ASD_log(ASD_LogLevel_Error, ASD_LogStream_Daemon, ASD_LogOption_None,
                "sd_journal_send failed %d", retcode);
    }
#endif
}
