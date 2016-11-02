/*
 * expander-util
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
#include <facebook/exp.h>
#include <openbmc/ipmb.h>

#define LOGFILE "/tmp/expander-util.log"

static void
print_usage_help(void) {
  printf("Usage: expander-util <[0..n]data_bytes_to_send>\n");
}

int
main(int argc, char **argv) {
  ipmb_req_t *req;
  ipmb_res_t *res;
  uint8_t req_len = 0;
  uint8_t res_len = 0;

  uint8_t tbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t netfn;
  uint8_t cmd;
  int i;
  int ret;
  int logfd;
  int len;
  char log[128];
  char temp[8];

  if (argc < 2) {
    goto err_exit;
  }

  netfn = (uint8_t)strtoul(argv[1], NULL, 0);
  cmd = (uint8_t)strtoul(argv[2], NULL, 0);

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = EXPANDER_SLAVE_ADDR << 1;
  req->netfn_lun = netfn << LUN_OFFSET;
  req->hdr_cksum = req->res_slave_addr +
                  req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = BMC_SLAVE_ADDR << 1;
  req->seq_lun = 0x00;
  req->cmd = cmd;

  for (i = 3; i < argc; i++) {
    req->data[req_len++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  tlen = IPMB_HDR_SIZE + IPMI_REQ_HDR_SIZE + req_len;

  lib_ipmb_handle(EXPANDER_IPMB_BUS_NUM, tbuf, tlen, rbuf, &rlen);
  if (rlen == 0)
  {
    printf("IPMB Failed... Zero bytes received.\n");
    return -1;
  }

  res  = (ipmb_res_t*) rbuf;
  res_len = rlen - IPMB_HDR_SIZE - IPMI_RESP_HDR_SIZE;

  memset(log, 0, 128);
  for (i = 0; i < res_len; i++) {
    printf("%02X ", res->data[i]);
    memset(temp, 0, 8);
    sprintf(temp, "%02X ", res->data[i]);
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
    close(logfd);
    return -1;
  }

  close(logfd);

  return 0;

err_exit:
  print_usage_help();
  return -1;
}
