/*
 * me-util
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>

#define LOGFILE "/tmp/me-util.log"

#define MAX_ARG_NUM 64

static void
print_usage_help(void) {
  printf("Usage: me-util <[0..n]data_bytes_to_send>\n");
  printf("Usage: me-util <--file> <path>\n");
}

static int
process_command(int argc, char **argv) {
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int i;
  ipmb_req_t *req;
  ipmb_res_t *res;

  if (argc < 2) {
    goto err_exit;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = (uint8_t)strtoul(argv[0], NULL, 0);
  req->hdr_cksum = req->res_slave_addr +
                   req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  req->cmd = (uint8_t)strtoul(argv[1], NULL, 0);

  tlen = 6;

  for (i = 2; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, &rbuf, &rlen);

  if (rlen == 0) {
    syslog(LOG_DEBUG, "me-util process_command: Zero bytes received\n");
    // Add ME no response error message
    printf("ME no response!\n");
    return -1;
  }

  // Remove 7-bytes of IPMB header and last-byte of Checksum and print only data
  // Print completion code if it is not 0x00
  if(rbuf[6] != 0){
    printf("Com_code:%02X, ", rbuf[6]);
  }

  for (i = 7; i < rlen-1; i++) {
    printf("%02X ", rbuf[i]);
  }
  printf("\n");

  return 0;
err_exit:
  print_usage_help();
  return -1;
}

static int
process_file(char *path) {
  FILE *fp;
  int argc, final_ret=0, ret;
  char buf[1024];
  char *str, *next, *del=" \n";
  char *argv[MAX_ARG_NUM];

  if (!(fp = fopen(path, "r"))) {
    syslog(LOG_WARNING, "Failed to open %s", path);
    return -1;
  }

  while (fgets(buf, sizeof(buf), fp) != NULL) {
    str = strtok_r(buf, del, &next);
    for (argc = 0; argc < MAX_ARG_NUM && str; argc++, str = strtok_r(NULL, del, &next)) {
      if (str[0] == '#')
        break;

      if (!argc && !strcmp(str, "echo")) {
        printf("%s", (*next) ? next : "\n");
        break;
      }
      argv[argc] = str;
    }
    if (argc < 1)
      continue;

    ret = process_command(argc, argv);
    // return failure if any command failed
    if (ret)
      final_ret = ret;
  }
  fclose(fp);

  return final_ret;
}

int
main(int argc, char **argv) {
  if (argc < 2) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "--file")) {
    if (argc < 3) {
      goto err_exit;
    }

    return process_file(argv[2]);;
  }

  return process_command((argc - 1), (argv + 1));;

err_exit:
  print_usage_help();
  return -1;
}
