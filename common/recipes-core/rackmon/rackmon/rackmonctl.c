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
    cmd.type = 0;
    int clisock;
    uint16_t wire_cmd_len = sizeof(cmd);
    struct sockaddr_un rackmond_addr;
    char *callname = argv[0];
    char *s;

    for (s = callname; *s != '\0';) {
      if (*s++ == '/')
        callname = s;
    }
    if (strcmp("rackmondata", callname) == 0) {
      cmd.type = COMMAND_TYPE_DUMP_DATA_JSON;
    }
    if (strcmp("rackmonstatus", callname) == 0) {
      cmd.type = COMMAND_TYPE_DUMP_STATUS;
    }
    if (strcmp("rackmonscan", callname) == 0) {
      cmd.type = COMMAND_TYPE_FORCE_SCAN;
    }
    if (argc > 1 && (strcmp("data", argv[1]) == 0)) {
      cmd.type = COMMAND_TYPE_DUMP_DATA_JSON;
    }
    if (argc > 1 && (strcmp("status", argv[1]) == 0)) {
      cmd.type = COMMAND_TYPE_DUMP_STATUS;
    }
    if (argc > 1 && (strcmp("force_scan", argv[1]) == 0)) {
      cmd.type = COMMAND_TYPE_FORCE_SCAN;
    }
    if (argc > 1 && (strcmp("pause", argv[1]) == 0)) {
      cmd.type = COMMAND_TYPE_PAUSE_MONITORING;
    }
    if (argc > 1 && (strcmp("resume", argv[1]) == 0)) {
      cmd.type = COMMAND_TYPE_START_MONITORING;
    }
    if(cmd.type == 0) {
      fprintf(stderr, "Usage: %s { status | data | force_scan | pause | resume }\n", callname);
      exit(1);
    }
    clisock = socket(AF_UNIX, SOCK_STREAM, 0);
    CHECKP(socket, clisock);
    rackmond_addr.sun_family = AF_UNIX;
    strcpy(rackmond_addr.sun_path, "/var/run/rackmond.sock");
    int addr_len = strlen(rackmond_addr.sun_path) + sizeof(rackmond_addr.sun_family);
    CHECKP(connect, connect(clisock, (struct sockaddr*) &rackmond_addr, addr_len));
    CHECKP(send, send(clisock, &wire_cmd_len, sizeof(wire_cmd_len), 0));
    CHECKP(send, send(clisock, &cmd, wire_cmd_len, 0));
    char readbuf[1024];
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
