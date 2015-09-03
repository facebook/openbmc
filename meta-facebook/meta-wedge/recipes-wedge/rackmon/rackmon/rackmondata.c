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
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "modbus.h"
#include "rackmond.h"

int main(int argc, char **argv) {
    int error = 0;
    rackmond_command cmd;
    int clisock;
    uint16_t wire_cmd_len = sizeof(cmd);
    struct sockaddr_un rackmond_addr;
    cmd.type = COMMAND_TYPE_DUMP_DATA_JSON;
    clisock = socket(AF_UNIX, SOCK_STREAM, 0);
    CHECKP(socket, clisock);
    rackmond_addr.sun_family = AF_UNIX;
    strcpy(rackmond_addr.sun_path, "/var/run/rackmond.sock");
    int addr_len = strlen(rackmond_addr.sun_path) + sizeof(rackmond_addr.sun_family);
    CHECKP(connect, connect(clisock, (struct sockaddr*) &rackmond_addr, addr_len));
    CHECKP(send, send(clisock, &wire_cmd_len, sizeof(wire_cmd_len), 0));
    CHECKP(send, send(clisock, &cmd, wire_cmd_len, 0));
    char readbuf[256];
    ssize_t n_read;
    while((n_read = read(clisock, readbuf, sizeof(readbuf))) > 0) {
      write(1, readbuf, n_read);
    }
cleanup:
    if(error != 0) {
      if(errno != 0) {
        fprintf(stderr, "%s\n", strerror(errno));
      }
      error = 1;
    }
    return error;
}
