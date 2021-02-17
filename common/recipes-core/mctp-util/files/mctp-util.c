/*
 * mctp-util
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <linux/netlink.h>
#include <openbmc/pal.h>
#include <openbmc/obmc-mctp.h>
#include "mctp-util.h"

#define DEFAULT_EID 0x8
#define MLX_MFG_ID 0x19810000
#define BCM_MFG_ID 0x3d110000
#define MAX_PAYLOAD_SIZE 1024  // including Message Header and body

#define noDEBUG


static void default_mctp_util_usage(void) {
  printf("Usage: mctp-util <bus#> <dst_eid> <type> <cmd payload> \n\n");
  printf("Sends MCTP data over SMbus \n\n");
  printf("       -bus#          I2C Bus number\n");
  printf("       -dst_eid       destination EID\n");
  printf("       -type          MCTP message type\n");
  printf("           0            - MCTP Control Message\n");
  printf("           1            - PLDM\n");
  printf("           2            - NC-SI\n");
  printf("           3            - Ethernet\n");
  printf("           4            - NVME\n");
  printf("           5            - SPDM\n");
  printf("           6            - Secured Messages\n");
}



int main(int argc, char **argv)
{
  uint8_t bus = 0;
  uint8_t eid = 0;
  uint8_t mctp_type = 0;
  uint8_t tbuf[MAX_PAYLOAD_SIZE] = {0};
  uint8_t rbuf[MAX_PAYLOAD_SIZE] = {0};
  uint16_t addr = 0;
  int tlen = 0;
  int rlen = 0;
  int ret = -1;
  int i, arg_num = 1;

  if (argc < 5) { // min params: mctp-util <bus> <eid> <type> <data>
    goto bail;
  }

  if (argc > (MAX_PAYLOAD_SIZE+3)) {  // 3 = mctp-util <bus> <eid>
    printf("max size (%d) exceeded\n", MAX_PAYLOAD_SIZE);
    goto bail;
  }

  bus = (uint8_t)strtoul(argv[arg_num++], NULL, 0);
  eid = (uint8_t)strtoul(argv[arg_num++], NULL, 0);
  mctp_type = (uint8_t)strtoul(argv[arg_num++], NULL, 0);
  tbuf[tlen++] = mctp_type;
  for (i = arg_num; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

#ifdef DEBUG
  printf("argc=%d bus=%d eid=%d type=%d\n", argc, bus, eid, mctp_type);
  printf("data: ");
  for (i = 1; i<tlen; ++i)
    printf("0x%x ", tbuf[i]);
  printf("\n");
#endif
  pal_get_bmc_ipmb_slave_addr(&addr, bus);
  ret = send_mctp_cmd(bus, addr, DEFAULT_EID, eid, tbuf, tlen, rbuf, &rlen);
  if (ret < 0) {
    printf("Error sending MCTP cmd, ret = %d\n", ret);
  } else {
    for (int i = 0; i<rlen; ++i)
      printf("0x%x ", rbuf[i]);
    printf("\n");
  }
  return ret;

bail:
  default_mctp_util_usage();
  return ret;
}
