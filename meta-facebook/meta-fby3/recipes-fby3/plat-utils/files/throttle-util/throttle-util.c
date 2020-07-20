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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <ctype.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <facebook/fby3_common.h>
#include <facebook/fby3_gpio.h>
#include <openbmc/obmc-i2c.h>
#include <sys/time.h>
#include <time.h>

static uint8_t bmc_location = 0xff;
#define CPLD_CTRL_ADDR 0x1F
#define SLOT_BUS_BASE  3
#define MAX_READ_RETRY 5

/* Throttle Source control (Enable=1,Disable=0)
Bit 7:IRQ_PVDDQ_ABC_VRHOT_LVT3_N
Bit 6:IRQ_PVDDQ_DEF_VRHOT_LVT3_N
Bit 5:FM_HSC_TIMER
Bit 4:IRQ_UV_DETECT_N
Bit 3:FAST_PROCHOT_N
Bit 2:IRQ_PVCCIN_CPU1_VRHOT_LVC3_N
Bit 1:IRQ_PVCCIO_CPU_VRHOT_LVC3_N
Bit 0:IRQ_SML1_PMBUS_ALERT_N, FM_THROTTLE_N */

#define IRQ_PVDDQ_ABC_VRHOT_LVT3_N   7
#define IRQ_PVDDQ_DEF_VRHOT_LVT3_N   6
#define FM_HSC_TIMER                 5
#define IRQ_UV_DETECT_N              4
#define FAST_PROCHOT_N               3
#define IRQ_PVCCIN_CPU1_VRHOT_LVC3_N 2
#define IRQ_PVCCIO_CPU_VRHOT_LVC3_N  1
#define FM_THROTTLE_N                0

static const char *option_list[] = {
  "--IRQ_PVDDQ_ABC_VRHOT_LVT3_N",
  "--IRQ_PVDDQ_DEF_VRHOT_LVT3_N",
  "--FM_HSC_TIMER",
  "--IRQ_UV_DETECT_N",
  "--FAST_PROCHOT_N",
  "--IRQ_PVCCIN_CPU1_VRHOT_LVC3_N",
  "--IRQ_PVCCIO_CPU_VRHOT_LVC3_N",
  "--FM_Throttle",
  "--OC_Warning",
};

static void
print_usage_help(void) {
  int i;

  printf("Usage: throttle-util <%s> --read_status \n", slot_usage);
  printf("Usage: throttle-util <%s> <option> <enable/disable>\n", slot_usage);
  printf("       option:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}

static int
control_via_cpld(uint8_t slot_id, uint8_t ctrl_bit, uint8_t enable) {
  int i2cfd = 0, ret = -1, retry = 0;
  char path[128];
  uint8_t rbuf[2] = {0};
  uint8_t rlen = 0;
  uint8_t tbuf[2] = {0x03, 0xFF};
  uint8_t tlen = 0;

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
    printf("IRQ_PVDDQ_ABC_VRHOT_LVT3_N             : %s\n", (rbuf[0] >> IRQ_PVDDQ_ABC_VRHOT_LVT3_N) & 0x1 ? "Enable":"Disable");
    printf("IRQ_PVDDQ_DEF_VRHOT_LVT3_N             : %s\n", (rbuf[0] >> IRQ_PVDDQ_DEF_VRHOT_LVT3_N) & 0x1 ? "Enable":"Disable");
    printf("FM_HSC_TIMER                           : %s\n", (rbuf[0] >> FM_HSC_TIMER) & 0x1 ? "Enable":"Disable");
    printf("IRQ_UV_DETECT_N                        : %s\n", (rbuf[0] >> IRQ_UV_DETECT_N) & 0x1 ? "Enable":"Disable");
    printf("FAST_PROCHOT_N                         : %s\n", (rbuf[0] >> FAST_PROCHOT_N) & 0x1 ? "Enable":"Disable");
    printf("IRQ_PVCCIN_CPU1_VRHOT_LVC3_N           : %s\n", (rbuf[0] >> IRQ_PVCCIN_CPU1_VRHOT_LVC3_N) & 0x1 ? "Enable":"Disable");
    printf("IRQ_PVCCIO_CPU_VRHOT_LVC3_N            : %s\n", (rbuf[0] >> IRQ_PVCCIO_CPU_VRHOT_LVC3_N) & 0x1 ? "Enable":"Disable");
    printf("IRQ_SML1_PMBUS_ALERT_N / FM_THROTTLE_N : %s\n", (rbuf[0] >> FM_THROTTLE_N) & 0x1 ? "Enable":"Disable");
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
  uint8_t slot_id = 0;
  uint8_t is_fru_present = 0;
  int ret = 0;
  uint8_t enable = 0;

  if (argc < 3) {
    goto err_exit;
  }

  ret = fby3_common_get_slot_id(argv[1], &slot_id);
  if ( ret < 0 ) {
    printf("%s is invalid!\n", argv[1]);
    goto err_exit;
  }

  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the location of BMC\n", __func__);
    return -1;
  }

  if ( slot_id != FRU_ALL ) {
    if ( bmc_location == NIC_BMC && slot_id != FRU_SLOT1 ) {
      printf("slot%d is not supported!\n", slot_id);
      return -1;
    }

    ret = fby3_common_is_fru_prsnt(slot_id, &is_fru_present);
    if ( ret < 0 || is_fru_present == 0 ) {
      printf("%s is not present!\n", argv[1]);
      return -1;
    }
  }

  if ( strcmp(argv[2], "--read_status") == 0 ) {
    return control_via_cpld(slot_id, 0xFF, enable);
  }

  if (strcmp(argv[3], "enable") == 0) {
    enable=1;
  } else if (strcmp(argv[3], "disable") == 0) {
    enable=0;
  } else {
    printf("Invalid option: %s\n", argv[3]);
    goto err_exit;
  }

  if ( strcmp(argv[2], "--OC_Warning") == 0 ) {
    return control_via_cpld(slot_id, FM_THROTTLE_N, enable);

  } else if ( strcmp(argv[2], "--FM_Throttle") == 0 ) {
    return control_via_cpld(slot_id, FM_THROTTLE_N, enable);

  } else if ( strcmp(argv[2], "--IRQ_PVCCIO_CPU_VRHOT_LVC3_N") == 0 ) {
    return control_via_cpld(slot_id, IRQ_PVCCIO_CPU_VRHOT_LVC3_N, enable);

  } else if ( strcmp(argv[2], "--IRQ_PVCCIN_CPU1_VRHOT_LVC3_N") == 0 ) {
    return control_via_cpld(slot_id, IRQ_PVCCIN_CPU1_VRHOT_LVC3_N, enable);

  } else if ( strcmp(argv[2], "--FAST_PROCHOT_N") == 0 ) {
    return control_via_cpld(slot_id, FAST_PROCHOT_N, enable);

  } else if ( strcmp(argv[2], "--IRQ_UV_DETECT_N") == 0 ) {
    return control_via_cpld(slot_id, IRQ_UV_DETECT_N, enable);

  } else if ( strcmp(argv[2], "--FM_HSC_TIMER") == 0 ) {
    return control_via_cpld(slot_id, FM_HSC_TIMER, enable);

  } else if ( strcmp(argv[2], "--IRQ_PVDDQ_DEF_VRHOT_LVT3_N") == 0 ) {
    return control_via_cpld(slot_id, IRQ_PVDDQ_DEF_VRHOT_LVT3_N, enable);

  } else if ( strcmp(argv[2], "--IRQ_PVDDQ_ABC_VRHOT_LVT3_N") == 0 ) {
    return control_via_cpld(slot_id, IRQ_PVDDQ_ABC_VRHOT_LVT3_N, enable);

  } else {
    printf("Invalid option: %s\n", argv[2]);
    goto err_exit;
  }

err_exit:
  print_usage_help();
  return -1;
}
