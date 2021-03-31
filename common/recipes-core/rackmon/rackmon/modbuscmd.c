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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include "modbus.h"
#include "rackmond.h"

void usage() {
  fprintf(stderr,
      "modbuscmd [-v] [-t <timeout in ms>] [-x <expected response length>] modbus_command\n"
      "\tmodbus command should be specified in hex\n"
      "\teg:\ta40300000008\n"
      "\tif an expected response length is provided, modbuscmd will stop receving and check crc immediately "
      "after receiving that many bytes\n");
  exit(1);
}

int send_command(rackmond_command* cmd, uint16_t wire_cmd_len, char *response)
{
    int clisock;
    int error = 0;
    uint16_t response_len_actual = 0;
    struct sockaddr_un rackmond_addr;

    clisock = socket(AF_UNIX, SOCK_STREAM, 0);
    ERR_LOG_EXIT(clisock, "failed to create socket");

    rackmond_addr.sun_family = AF_UNIX;
    strcpy(rackmond_addr.sun_path, "/var/run/rackmond.sock");
    int addr_len = strlen(rackmond_addr.sun_path) + sizeof(rackmond_addr.sun_family);
    ERR_LOG_EXIT(connect(clisock, (struct sockaddr*)&rackmond_addr, addr_len),
                 "connect to socket failed");

    ERR_LOG_EXIT(send(clisock, &wire_cmd_len, sizeof(wire_cmd_len), 0),
                 "send to socket failed");
    ERR_LOG_EXIT(send(clisock, cmd, wire_cmd_len, 0), "send to socket failed");
    ERR_LOG_EXIT(recv(clisock, &response_len_actual,
                      sizeof(response_len_actual), 0),
                 "recv from socket failed");
    if(response_len_actual == 0) {
      uint16_t errcode = 0;
      ERR_LOG_EXIT(recv(clisock, &errcode, sizeof(errcode), 0),
                   "recv from socket failed");
      fprintf(stderr, "modbus error: %d (%s)\n", errcode, modbus_strerror(errcode));
      error = 1;
      goto cleanup;
    }
    ERR_LOG_EXIT(recv(clisock, response, response_len_actual, 0),
                 "recv from socket failed");
    error = (int)response_len_actual;
cleanup:
    close(clisock);
    return error;
}

int profile_overhead()
{
#ifndef RACKMON_PROFILING
  printf("Profiling is not enabled!\n");
  return -1;
#else
  CHECK_LATENCY_START();
  CHECK_LATENCY_END("profile_overhead");
  return 0;
#endif
}

int main(int argc, char **argv) {
    int error = 0;
    char *modbus_cmd = NULL;
    size_t cmd_len = 0;
    int expected = 0;
    uint32_t timeout = 0;
    verbose = 0;
    rackmond_command *cmd = NULL;
    char *response = NULL;
    int response_len_actual;
    int measure_profile_overhead = 0;
#ifdef RACKMON_PROFILING
    obmc_log_init("modbuscmd", LOG_INFO, 0);
    obmc_log_set_syslog(LOG_CONS, LOG_DAEMON);
    obmc_log_unset_std_stream();
#endif

    int opt;
    while((opt = getopt(argc, argv, "w:x:t:g:vp")) != -1) {
      switch (opt) {
        case 'p':
          measure_profile_overhead = 1;
          break;
      case 'x':
        expected = atoi(optarg);
        break;
      case 't':
        timeout = atol(optarg);
        break;
      case 'v':
        verbose = 1;
        break;
      default:
        usage();
        break;
      }
    }
    if (measure_profile_overhead)
      return profile_overhead();

    if(optind < argc) {
      modbus_cmd = argv[optind];
    }
    if(modbus_cmd == NULL) {
      usage();
    }

    //convert hex to bytes
    cmd_len = strlen(modbus_cmd);
    if(cmd_len < 4) {
      fprintf(stderr, "Modbus command too short!\n");
      exit(1);
    }
    
    cmd = malloc(sizeof(rackmond_command) + cmd_len);
    cmd->type = COMMAND_TYPE_RAW_MODBUS;
    cmd->raw_modbus.custom_timeout = timeout;
    memcpy(cmd->raw_modbus.data, modbus_cmd, cmd_len);
    decode_hex_in_place(cmd->raw_modbus.data, &cmd_len);
    cmd->raw_modbus.length = cmd_len;
    cmd->raw_modbus.expected_response_length = expected;
    response = malloc(expected ? expected : 1024);
    uint16_t wire_cmd_len = sizeof(rackmond_command) + cmd_len;
    CHECK_LATENCY_START();
    response_len_actual = send_command(cmd, wire_cmd_len, response);
    CHECK_LATENCY_END("modbuscmd::send_command cmd_len=%lu, resp_len=%lu status=%d",
       (unsigned long)wire_cmd_len, (unsigned long)expected, response_len_actual);

    if(response_len_actual >= 0) {
      printf("Response: ");
      print_hex(stdout, response, response_len_actual);
      printf("\n");
    } else {
      error = 1;
    }
    free(cmd);
    free(response);
    if(error != 0) {
      if(errno != 0) {
        fprintf(stderr, "errno err: %s\n", strerror(errno));
      }
      error = 1;
    }
    return error;
}
