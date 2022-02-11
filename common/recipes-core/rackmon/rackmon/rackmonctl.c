/*
 * Copyright 2014-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "modbus.h"
#include "rackmond.h"

static struct {
    const char *name;
    uint16_t type;
} cmd_map[] = {
    {
        .name = "data",
        .type = COMMAND_TYPE_DUMP_DATA_JSON,
    },
    {
        .name = "info",
        .type = COMMAND_TYPE_DUMP_DATA_INFO,
    },
    {
        .name = "status",
        .type = COMMAND_TYPE_DUMP_STATUS,
    },
    {
        .name = "force_scan",
        .type = COMMAND_TYPE_FORCE_SCAN,
    },
    {
        .name = "pause",
        .type = COMMAND_TYPE_PAUSE_MONITORING,
    },
    {
        .name = "resume",
        .type = COMMAND_TYPE_START_MONITORING,
    },

    /*
     * Make sure this is the last entry.
     */
    {
        .name = NULL,
        .type = COMMAND_TYPE_NONE,
    },
};

static void usage(const char *prog_name)
{
    int i;
    fprintf(stderr, "Usage: %s <command>\n", prog_name);
    fprintf(stderr, "Available commands are:\n");
    for (i = 0; cmd_map[i].name != NULL; i++) {
        fprintf(stderr, " - %s\n", cmd_map[i].name);
    }
}

int main(int argc, char **argv) {
    int error = 0;
    int i, clisock, addr_len;
    rackmond_command cmd;
    uint16_t wire_cmd_len = sizeof(cmd);
    struct sockaddr_un rackmond_addr;
    char *callname;
    const char *cmd_name;
    char readbuf[1024];
    ssize_t n_read;

    callname = basename(argv[0]);
    if (strcmp("rackmondata", callname) == 0) {
        cmd_name = "data";
    } else if (strcmp("rackmoninfo", callname) == 0) {
        cmd_name = "info";
    } else if (strcmp("rackmonstatus", callname) == 0) {
        cmd_name = "status";
    } else if (strcmp("rackmonscan", callname) == 0) {
        cmd_name = "force_scan";
    } else {
        if (argc < 2) {
            usage(argv[0]);
            return -1;
        }
        cmd_name = argv[1];
    }

    /* Match command name with type. */
    cmd.type = 0;
    for (i = 0; cmd_map[i].name != NULL; i++) {
        if (strcmp(cmd_name, cmd_map[i].name) == 0) {
            cmd.type = cmd_map[i].type;
            break;
        }
    }
    if (cmd.type == 0) {
        usage(argv[0]);
        return -1;
    }

    clisock = socket(AF_UNIX, SOCK_STREAM, 0);
    ERR_LOG_EXIT(clisock, "failed to create socket");

    rackmond_addr.sun_family = AF_UNIX;
    strcpy(rackmond_addr.sun_path, RACKMON_IPC_SOCKET);
    addr_len = strlen(rackmond_addr.sun_path) +
               sizeof(rackmond_addr.sun_family);
    ERR_LOG_EXIT(connect(clisock, (struct sockaddr*)&rackmond_addr, addr_len),
                 "failed to connect to socket");

    ERR_LOG_EXIT(send(clisock, &wire_cmd_len, sizeof(wire_cmd_len), 0),
                 "failed to send command-length to socket");
    ERR_LOG_EXIT(send(clisock, &cmd, wire_cmd_len, 0),
                 "failed to send command body to socket");

    /* Read and dump response from server. */
    while((n_read = read(clisock, readbuf, sizeof(readbuf))) > 0) {
        if (write(STDOUT_FILENO, readbuf, n_read) < 0) {
            fprintf(stderr, "ERROR: could not write to STDOUT: %d %s\n",
                    error, strerror(errno));
        }
    }

cleanup:
    if (clisock >= 0) {
        close(clisock);
    }
    return error;
}
