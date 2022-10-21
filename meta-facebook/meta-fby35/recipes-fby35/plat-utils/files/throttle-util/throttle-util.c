/*
 * throttle-util
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>
#include <syslog.h>
#include <facebook/fby35_common.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>

static uint8_t bmc_location = 0xff;
#define CPLD_CTRL_ADDR 0x1F
#define SLOT_BUS_BASE  3
#define MAX_READ_RETRY 5

/* Throttle Source control (Enable=1,Disable=0)
Bit 7:IRQ_CPU0_VRHOT_N
Bit 6:IRQ_PVCCD_CPU0_VRHOT_LVC3_N
Bit 5:FM_HSC_TIMER
Bit 4:IRQ_UV_DETECT_N
Bit 3:FAST_PROCHOT_N
Bit 2:FM_PVCCIN_CPU0_PWR_IN_ALERT_N
Bit 1:RSVD
Bit 0:IRQ_SML1_PMBUS_ALERT_N, FM_THROTTLE_N */
static const struct option cl_options[] = {
  {"IRQ_CPU0_VRHOT_N",              no_argument, 0, 7},
  {"IRQ_PVCCD_CPU0_VRHOT_LVC3_N",   no_argument, 0, 6},
  {"FM_HSC_TIMER",                  no_argument, 0, 5},
  {"IRQ_UV_DETECT_N",               no_argument, 0, 4},
  {"FAST_PROCHOT_N",                no_argument, 0, 3},
  {"FM_PVCCIN_CPU0_PWR_IN_ALERT_N", no_argument, 0, 2},
  {"IRQ_SML1_PMBUS_ALERT_N",        no_argument, 0, 0},
  {"FM_THROTTLE_N",                 no_argument, 0, 0},
  {0, 0, 0, 0}
};

/* Throttle Source control (Enable=1,Disable=0)
Bit 7:PVDDCR_CPU0_PMALERT_N
Bit 6:PVDDCR_CPU1_PMALERT_N
Bit 5:FM_HSC_TIMER
Bit 4:IRQ_UV_DETECT_N
Bit 3:FAST_PROCHOT_N
Bit 2:PVDD11_S3_PMALERT_N
Bit 1:IRQ_HSC_ALERT1_N
Bit 0:IRQ_HSC_ALERT2_N */
static const struct option hd_options[] = {
  {"PVDDCR_CPU0_PMALERT_N", no_argument, 0, 7},
  {"PVDDCR_CPU1_PMALERT_N", no_argument, 0, 6},
  {"FM_HSC_TIMER",          no_argument, 0, 5},
  {"IRQ_UV_DETECT_N",       no_argument, 0, 4},
  {"FAST_PROCHOT_N",        no_argument, 0, 3},
  {"PVDD11_S3_PMALERT_N",   no_argument, 0, 2},
  {"IRQ_HSC_ALERT1_N",      no_argument, 0, 1},
  {"IRQ_HSC_ALERT2_N",      no_argument, 0, 0},
  {0, 0, 0, 0}
};

static const struct option *options = cl_options;

static void
print_usage_help(void) {
  int i;

  printf("Usage: throttle-util <%s> --read_status \n", slot_usage);
  printf("Usage: throttle-util <%s> <option> <enable/disable>\n", slot_usage);
  printf("       option:\n");
  for (i = 0; options[i].name != NULL; i++)
    printf("       --%s\n", options[i].name);
}

static int
control_via_cpld(uint8_t slot_id, uint8_t ctrl_bit, uint8_t enable) {
  int i2cfd = 0, ret = -1, retry = 0;
  char path[128];
  uint8_t rbuf[2] = {0};
  uint8_t rlen = 0;
  uint8_t tbuf[2] = {0x03, 0xFF};
  uint8_t tlen = 0;
  uint8_t i;

  snprintf(path, sizeof(path), "/dev/i2c-%d", (slot_id + SLOT_BUS_BASE));

  i2cfd = open(path, O_RDWR);
  if ( i2cfd < 0 ) {
    syslog(LOG_WARNING, "%s() Failed to open %s", __func__, path);
  }

  tlen = 1;
  rlen = 1;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_CTRL_ADDR, tbuf, tlen, rbuf, rlen);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      ret = 0;
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    ret = -1;
    goto stop;
  }

  if (ctrl_bit == 0xFF) {
    printf("Throttle Source control (Enable=1,Disable=0) \n");
    for (i = 0; options[i].name != NULL; i++) {
      printf("%-30s : %s\n", options[i].name, ((rbuf[0] >> options[i].val) & 0x1) ? "Enable":"Disable");
    }
    if ( i2cfd > 0 ) close(i2cfd);
    return 0;
  }

  if (enable) {
    tbuf[1] = (rbuf[0] | (0x1 << ctrl_bit));
  } else {
    tbuf[1] = (rbuf[0] &~(0x1 << ctrl_bit));
  }
  tlen = 2;
  while (retry < MAX_READ_RETRY) {
    ret = i2c_rdwr_msg_transfer(i2cfd, CPLD_CTRL_ADDR, tbuf, tlen, NULL, 0);
    if ( ret < 0 ) {
      retry++;
      msleep(100);
    } else {
      ret = 0;
      break;
    }
  }
  if (retry == MAX_READ_RETRY) {
    syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, tlen);
    ret = -1;
    goto stop;
  }

stop:
  if ( i2cfd > 0 ) close(i2cfd);
  return ret;
}

int
main(int argc, char **argv) {
  int ret = 0;
  int slot_type = 0;
  uint8_t slot_id = 0, prsnt = 0;
  uint8_t enable = 0;

  for (slot_id = FRU_SLOT1; slot_id <= FRU_SLOT4; slot_id++) {
    if (fby35_common_is_fru_prsnt(slot_id, &prsnt)) {
      continue;
    }
    if (prsnt) {
      if ((slot_type = fby35_common_get_slot_type(slot_id)) >= 0)
        break;
    }
  }
  if (slot_type == SERVER_TYPE_HD) {
    options = hd_options;
  }

  if (argc < 3 || argc > 4) {
    goto err_exit;
  }

  ret = fby35_common_get_slot_id(argv[1], &slot_id);
  if ( ret < 0 || slot_id == FRU_ALL ) {
    printf("%s is invalid!\n", argv[1]);
    goto err_exit;
  }

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the location of BMC\n", __func__);
    return -1;
  }
  if ( bmc_location == NIC_BMC && slot_id != FRU_SLOT1 ) {
    printf("slot%d is not supported!\n", slot_id);
    return -1;
  }

  ret = pal_is_fru_prsnt(slot_id, &prsnt);
  if ( ret < 0 || prsnt == 0 ) {
    printf("%s is not present!\n", argv[1]);
    return -1;
  }

  if ( strcmp(argv[2], "--read_status") == 0 ) {
    return control_via_cpld(slot_id, 0xFF, enable);
  }

  if (argc < 4) {
    goto err_exit;
  }

  if (strcmp(argv[3], "enable") == 0) {
    enable=1;
  } else if (strcmp(argv[3], "disable") == 0) {
    enable=0;
  } else {
    printf("Invalid option: %s\n", argv[3]);
    goto err_exit;
  }

  ret = getopt_long(argc, argv, "", options, NULL);
  if ((ret >= 0) && (ret < 8)) {  // bit0 ~ bit7
    return control_via_cpld(slot_id, ret, enable);
  } else {
    printf("Invalid option: %s\n", argv[2]);
  }

err_exit:
  print_usage_help();
  return -1;
}
