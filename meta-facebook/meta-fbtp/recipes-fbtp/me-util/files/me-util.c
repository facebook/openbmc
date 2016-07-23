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

static void
print_usage_help(void) {
  printf("Usage: me-util <[0..n]data_bytes_to_send>\n");
}

int
main(int argc, char **argv) {
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int i;
  int ret;
  int logfd;
  int len;
  char log[128];
  char temp[8];
  ipmb_req_t *req;
  ipmb_res_t *res;

  if (argc < 2) {
    goto err_exit;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = (uint8_t)strtoul(argv[1], NULL, 0);
  req->hdr_cksum = req->res_slave_addr +
                   req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  req->cmd = (uint8_t)strtoul(argv[2], NULL, 0);

  tlen = 6;

  for (i = 3; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, &rbuf, &rlen);

  if (rlen == 0) {
    syslog(LOG_DEBUG, "bic_ipmb_wrapper: Zero bytes received\n");
    return -1;
  }

  memset(log, 0, 128);
  // Remove 7-bytes of IPMB header and last-byte of Checksum and print only data
  for (i = 7; i < rlen-1; i++) {
    printf("%02X ", rbuf[i]);
    memset(temp, 0, 8);
    sprintf(temp, "%02X ", rbuf[i]);
    strcat(log, temp);
  }
  printf("\n");
  sprintf(temp, "\n");
  strcat(log, temp);

  errno = 0;

  logfd = open(LOGFILE, O_CREAT | O_WRONLY);
  if (logfd < 0) {
    syslog(LOG_WARNING, "Opening a tmp file failed. errno: %d", errno);
    return -1;
  }

  len = write(logfd, log, strlen(log));
  if (len != strlen(log)) {
    syslog(LOG_WARNING, "Error writing the log to the file");
    return -1;
  }

  close(logfd);

  return 0;
err_exit:
  print_usage_help();
  return -1;
}
