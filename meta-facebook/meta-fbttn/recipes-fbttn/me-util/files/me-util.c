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
#include <facebook/bic.h>
#include <openbmc/ipmi.h>

#define LOGFILE "/tmp/me-util.log"

static void
print_usage_help(void) {
  printf("Usage: me-util <slot1> <[0..n]data_bytes_to_send>\n");
}

int
main(int argc, char **argv) {
  uint8_t slot_id;
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

  if (argc < 3) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else {
    goto err_exit;
  }

  for (i = 2; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

#if 1
  ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    return ret;
  }
#endif

  // memcpy(rbuf, tbuf, tlen);
  //rlen = tlen;


  memset(log, 0, 128);
  for (i = 0; i < rlen; i++) {
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
