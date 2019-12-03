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
#include <facebook/fby3_common.h>

#define LOGFILE "/tmp/me-util.log"

#define MAX_ARG_NUM 64
#define MAX_CMD_RETRY 2
#define MAX_TOTAL_RETRY 30

enum {
  UTIL_EXECUTION_OK = 0,
  UTIL_EXECUTION_FAIL = -1,
};

static int total_retry = 0;

static void
print_usage_help(void) {
  printf("\nUsage: me-util <slot0|slot1|slot2|slot3> <[0..n]data_bytes_to_send>\n");
  printf("                                           <cmd>\n");
  printf("\ncmd list:\n");
  printf("   --get_dev_id   - get device ID\n");
}

static int
me_get_dev_id(uint8_t slot_id) {
  int ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  tbuf[0] = NETFN_APP_REQ << 2;
  tbuf[1] = CMD_APP_GET_DEVICE_ID;
  tlen = 2;

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == UTIL_EXECUTION_OK)
      break;

    total_retry++;
    retry--;
  }

  if (ret < 0 ) {
    printf("ME no response!\n");
    return UTIL_EXECUTION_FAIL;
  }

  if (rbuf[0] != CC_SUCCESS) {
    printf("Completion Code: %02X, ", rbuf[0]);
  }

  if (rlen < 16) {
    printf("return incomplete len=%d\n", rlen);
  }

  printf("Device ID:                 %02x\n", rbuf[1]);
  printf("Device Revision:           %02x\n", rbuf[2]);
  printf("Firmware Revision:         %02x %02x\n", rbuf[3], rbuf[4]);
  printf("IPMI Version:              %02x\n", rbuf[5]);
  printf("Additional Device Support: %02x\n", rbuf[6]);
  printf("Manufacturer ID:           %02x %02x %02x\n", rbuf[7], rbuf[8], rbuf[9]);
  printf("Product ID:                %02x %02x\n", rbuf[10], rbuf[11]);
  printf("Aux Firmware Revision:     %02x %02x %02x\n", rbuf[12], rbuf[13], rbuf[14]);

  return ret;
}

static int
process_command(uint8_t slot_id, int argc, char **argv) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  for (i = 0; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == UTIL_EXECUTION_OK)
      break;

    total_retry++;
    retry--;
  }

  if (ret < 0) {
    printf("ME no response!\n");
    return ret;
  }

  if (rbuf[0] != CC_SUCCESS) {
    printf("Completion Code: %02X, ", rbuf[0]);
  }

  for (i = 1; i < rlen; i++) {
    printf("%02X ", rbuf[i]);
  }
  printf("\n");

  return ret;
}

int
do_cmds(uint8_t fru, char *cmd) {
  int ret = UTIL_EXECUTION_FAIL;

  if ( (strcmp(cmd, "--get_dev_id") == 0) ) {
    return me_get_dev_id(fru);  
  } else {
    print_usage_help();
  } 

  return ret;
}

int
main(int argc, char **argv) {
  uint8_t slot_id;
  int ret;

  if (argc < 3) {
    goto err_exit;
  }

  ret = is_valid_slot_str(argv[1], &slot_id);
  if ( ret < 0 ) {
    goto err_exit;
  }

  if ( (strncmp(argv[2], "--", 2) == 0) ) {
    return do_cmds(slot_id, argv[2]);
  } else {
    if ( argc < 4) {
      goto err_exit;
    }
    return process_command(slot_id, (argc - 2), (argv + 2));
  }

err_exit:
  print_usage_help();
  return UTIL_EXECUTION_FAIL;
}
