/*
 * nvme-util
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
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <ctype.h>
#include <openbmc/pal.h>

#define MAX_RETRY 3
static uint8_t m_slot_id = 0;
static uint8_t m_intf = 0;

static void
print_usage_help(void) {
  printf("Usage: nvme-util <slot1|slot3> <2U-dev[0..n]> <register> <read length> <[0..n]data_bytes_to_send>\n");
}

static int
read_bic_nvme_data(uint8_t slot_id, uint8_t dev_id, int argc, char **argv) {
  int ret = 0;
  char stype_str[32] = {0};
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int i = 0, retry = MAX_RETRY;

  sprintf(stype_str, "Glacier Point V3");

  printf("%s %u Drive%d\n", stype_str, slot_id, dev_id - DEV_ID0_2OU); // 0-based number

  // mux select
  ret = pal_gpv3_mux_select(slot_id, dev_id);
  if (ret) {
    return ret; // fail to select mux
  }

  tlen = 0;
  tbuf[tlen++] = (get_gpv3_bus_number(dev_id) << 1) + 1;
  tbuf[tlen++] = 0xD4;
  tbuf[tlen++] = (uint8_t)strtoul(argv[1], NULL, 0); //read cnt
  tbuf[tlen++] = (uint8_t)strtoul(argv[0], NULL, 0); //register
  for (i = 2; i < argc; i++) { // data
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  printf("Device register: 0x%02X\n", tbuf[3]);
  if ( argc - 2  > 0 ) printf("To write NVMe %d bytes.\n", argc - 2);
  else printf("To read NVMe %d bytes:\n", tbuf[2]);

  do {
    ret = bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, m_intf);
    if (ret < 0 ) msleep(100);
    else break;
  } while ( retry-- >= 0 );

  if ( ret < 0 ) {
    if ( rlen > 0 ) printf("nvme-util fail to read\n");
    else printf("nvme-util fail to write\n");
  } else {
    for (i = 0; i < rlen; i++) {
      if (!(i % 16) && i) printf("\n");
      printf("%02X ", rbuf[i]);
    }
    printf("\n");
  }

  return ret;
}

static int
ssd_monitor_enable(uint8_t slot_id, bool enable) {
  int ret = 0;
  ret = bic_enable_ssd_sensor_monitor(slot_id, enable, m_intf);
  if ( enable == false ) {
    msleep(100);
  }
  return ret;
}

static void
nvme_sig_handler(int sig __attribute__((unused))) {
  if (m_slot_id) {
    ssd_monitor_enable(m_slot_id, true);
  }
}

int
main(int argc, char **argv) {
#define DEV_PREFIX "2U-dev"

  int ret = -1;
  uint8_t slot_id = 0xff;
  uint8_t dev_id = DEV_NONE;
  uint8_t bmc_location = 0;
  int (*read_write_nvme_data)(uint8_t, uint8_t, int, char **);
  struct sigaction sa;

  do {
    if ( argc < 5 ) {
      print_usage_help();
      break;
    }

    // check slot info
    if ( strcmp(argv[1], "slot1") == 0 ) slot_id = FRU_SLOT1;
    else if ( strcmp(argv[1], "slot3") == 0 ) slot_id = FRU_SLOT3;
    else break;

    if ( fby35_common_get_bmc_location(&bmc_location) < 0 ) {
      printf("Failed to get bmc location\n");
      break;
    } else if ( NIC_BMC == bmc_location && slot_id == FRU_SLOT3 ) {
      printf("%s is not supported\n", argv[1]);
      break;
    }

    if ( strncmp(argv[2], DEV_PREFIX, strlen(DEV_PREFIX)) != 0 ) {
      printf("%s is not supported\n", argv[2]);
      break;
    } else m_intf = REXP_BIC_INTF;

    //device
    if ( pal_get_dev_id(argv[2], &dev_id) < 0 ) {
      printf("pal_get_dev_id failed for %s %s\n", argv[1], argv[2]);
      break;
    }

    read_write_nvme_data = read_bic_nvme_data;
    m_slot_id = slot_id;

    sa.sa_handler = nvme_sig_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if ( ssd_monitor_enable(slot_id, false) < 0 ) {
      printf("err: failed to disable SSD monitoring\n");
      break;
    }

    if ( read_write_nvme_data(slot_id, dev_id, argc-3,argv+3) < 0 ) {
      printf("err: failed to read_write_nvme_data\n");
      printf("info: try to enable SSD monitoring\n");
      if ( ssd_monitor_enable(slot_id, true) < 0 ) {
        printf("err: failed to enable SSD monitoring");
      }
      break;
    }

    if ( ssd_monitor_enable(slot_id, true) < 0 ) {
      printf("err: failed to enable SSD monitoring");
      break;
    }
    ret = 0;
  } while(0);

  return ret;
}
