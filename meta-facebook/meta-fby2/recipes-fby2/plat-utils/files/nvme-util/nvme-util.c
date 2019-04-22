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
#include <openbmc/obmc-i2c.h>

#define MAX_DRIVE_NUM 2
#define MAX_GP_DRIVE_NUM 6
#define MAX_GPV2_DRIVE_NUM 12

#define I2C_DEV_GP_1 "/dev/i2c-1"
#define I2C_DEV_GP_3 "/dev/i2c-5"
#define I2C_GP_MUX_ADDR 0x71

static uint8_t m_slot_id = 0;
static uint8_t m_slot_type = 0xFF;

static void
print_usage_help(void) {
  printf("Usage: nvme-util <slot1|slot2|slot3|slot4> <device[0..n]> <register> <read length> <[0..n]data_bytes_to_send>\n");
}

static int
util_is_numeric(char **argv) {
  int i, j = 0;
  int len = 0;
  for (i = 0; i < 2; i++) { //check register and read length
    len = strlen(argv[i]);
    if (len > 2 && argv[i][0] == '0' && (argv[i][1] == 'x' || argv[i][1] == 'X')) {
      for (j=2; j < len; j++) {
        if (!isxdigit(argv[i][j]))
          return 0;
      }
    } else {
      for (j=0; j < len; j++) {
        if (!isdigit(argv[i][j]))
          return 0;
      }
    }
  }
  return 1;
}

static int
read_bic_nvme_data(uint8_t slot_id, uint8_t drv_num, uint8_t reg, int rlen, int argc, char **argv) {
  int ret = 0;
  uint8_t bus;
  char stype_str[32] = {0};
  uint8_t wbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  int i, retry = 2;
  int wlen = 1;

  if (m_slot_type == SLOT_TYPE_SERVER) {
    bus = 0x3;
    wbuf[0] = 1 << (drv_num - 1);
    sprintf(stype_str, "Server Board");
  } else {  // SLOT_TYPE_GPV2
    bus = (2 + drv_num/2) * 2 + 1;
    wbuf[0] = 1 << (drv_num%2);
    sprintf(stype_str, "Glacier Point V2");
  }

  printf("%s %u Drive%d\n", stype_str, slot_id, drv_num);

  // MUX
  ret = bic_master_write_read(slot_id, bus, 0xe2, wbuf, 1, rbuf, 0);
  if (ret != 0) {
    printf("nvme-util fail to switch mux\n");
    syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
    return ret;
  }

  wbuf[0] = reg;
  if (rlen) {
    printf("NVMe I2C Read:\n");
  } else {
    printf("NVMe I2C Write\n");
    for (i = 0; i < argc; i++) {
      wbuf[wlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
    }
  }

  while (retry >= 0) {
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, wlen, rbuf, rlen);
    if (ret == 0)
      break;

    retry--;
  }

  if (ret != 0) {
    if (rlen) {
      printf("nvme-util fail to read\n");
    } else {
      printf("nvme-util fail to write\n");
    }
    syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
    return ret;
  }

  if (rlen) {
    for (i = 0; i < rlen; i++) {
      if (!(i % 16) && i)
        printf("\n");

      printf("%02X ", rbuf[i]);
    }
    printf("\n");
  }

  return 0;
}

static int
read_gp_nvme_data(uint8_t slot_id, uint8_t drv_num, uint8_t reg, int rlen, int argc, char **argv) {
  int ret = 0, i = 0;
  uint8_t channel = 0;
  char *device;
  int retry = 2;
  uint8_t wbuf[64] = {0x00};
  uint8_t rbuf[64] = {0x00};
  int fd, wlen = 1;

  printf("Glacier Point %u Drive%d\n", slot_id, drv_num);

  // MUX
  switch (drv_num) {
    case 0:
      channel = MUX_CH_1;
      break;
    case 1:
      channel = MUX_CH_0;
      break;
    case 2:
      channel = MUX_CH_4;
      break;
    case 3:
      channel = MUX_CH_3;
      break;
    case 4:
      channel = MUX_CH_2;
      break;
    case 5:
      channel = MUX_CH_5;
      break;
    default:
      return -1;
  }

  device = (slot_id == 1) ? I2C_DEV_GP_1 : I2C_DEV_GP_3;
  ret = fby2_mux_control(device, I2C_GP_MUX_ADDR, channel);
  if (ret != 0) {
     printf("nvme-util fail to switch mux\n");
     syslog(LOG_DEBUG, "%s: fby2_mux_control failed", __func__);
     return ret;
  }

  fd = open(device, O_RDWR);
  if (fd < 0) {
    syslog(LOG_DEBUG, "%s(): open() failed", __func__);
    return -1;
  }

  wbuf[0] = reg;
  if (rlen) {
    printf("NVMe I2C Read:\n");
  } else {
    printf("NVMe I2C Write\n");
    for (i = 0; i < argc; i++) {
      wbuf[wlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
    }
  }

  while (retry >= 0) {
    ret = i2c_rdwr_msg_transfer(fd, 0xd4, wbuf, wlen, rbuf, rlen);
    if (ret == 0)
      break;

    retry--;
  }

  close(fd);

  if (ret != 0) {
    if (rlen) {
      printf("nvme-util fail to read\n");
    } else {
      printf("nvme-util fail to write\n");
    }
    syslog(LOG_DEBUG, "%s(): i2c_rdwr_msg_transfer failed", __func__);
    return ret;
  }

  if (rlen) {
    for (i = 0; i < rlen; i++) {
      if (!(i % 16) && i)
        printf("\n");

      printf("%02X ", rbuf[i]);
    }
    printf("\n");
  } 

  return 0;
}

static void
ssd_monitor_enable(uint8_t slot_id, uint8_t slot_type, uint8_t en) {
  if ((slot_type == SLOT_TYPE_SERVER) || (slot_type == SLOT_TYPE_GPV2)) {
    if (en) {  // enable sensor monitor
      bic_disable_sensor_monitor(slot_id, 0);
    } else {   // disable sensor monitor
      bic_disable_sensor_monitor(slot_id, 1);
      msleep(100);
    }
  }
  else if (slot_type == SLOT_TYPE_GP) {
    if (en) {  // enable sensor monitor
      fby2_disable_gp_m2_monior(slot_id, 0);
    } else {   // disable sensor monitor
      fby2_disable_gp_m2_monior(slot_id, 1);
      msleep(100);
    }
  }
}

static void
nvme_sig_handler(int sig) {
  if (m_slot_id) {
    ssd_monitor_enable(m_slot_id, m_slot_type, 1);
  }
}

int
main(int argc, char **argv) {
  int ret, rlen;
  uint8_t slot_id, slot_type = 0xFF;
  uint8_t drv_num = DEV_NONE;
  uint8_t drv_start = 1, drv_end = MAX_DRIVE_NUM;
  uint8_t reg;
  char *end;
  int (*read_write_nvme_data)(uint8_t,uint8_t,uint8_t,int,int,char **);
  struct sigaction sa;

  if (argc < 5) {
    goto err_exit;
  }

  //slot
  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = 2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = 3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = 4;
  } else {
    goto err_exit;
  }

  //device
  ret = pal_get_dev_id(argv[2], &drv_num);
  if (ret < 0 || drv_num == DEV_ALL || drv_num == DEV_NONE) {
    printf("pal_get_dev_id failed for %s %s\n", argv[1], argv[2]);
    goto err_exit;
  }
  drv_num--;

  if (util_is_numeric(argv + 3)) {
    //register
    errno = 0;
    reg = (uint8_t)strtoul(argv[3], &end, 0);
    if (errno || *end ) {
      goto err_exit;
    }
    //read length
    errno = 0;
    rlen = (int)strtoul(argv[4], &end, 0);
    if (errno || *end ) {
      goto err_exit;
    }
  } else {
    goto err_exit;
  }

  slot_type = fby2_get_slot_type(slot_id);
  if (slot_type == SLOT_TYPE_SERVER) {
    drv_start = 1;  // 1-based
    drv_end = MAX_DRIVE_NUM;
    read_write_nvme_data = read_bic_nvme_data;
  } else if (slot_type == SLOT_TYPE_GP) {
    drv_start = 0;  // 0-based
    drv_end = MAX_GP_DRIVE_NUM - 1;
    read_write_nvme_data = read_gp_nvme_data;
  } else if (slot_type == SLOT_TYPE_GPV2) {
    drv_start = 0;  // 0-based
    drv_end = MAX_GPV2_DRIVE_NUM - 1;
    read_write_nvme_data = read_bic_nvme_data;
  } else {
    goto err_exit;
  }
  m_slot_type = slot_type;
  m_slot_id = slot_id;

  //device
  if ((drv_num < drv_start) || (drv_num > drv_end))
    goto err_exit;

  sa.sa_handler = nvme_sig_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  ssd_monitor_enable(slot_id, slot_type, 0);
  read_write_nvme_data(slot_id, drv_num, reg, rlen, argc-5,argv+5);
  ssd_monitor_enable(slot_id, slot_type, 1);

  return 0;

err_exit:
  print_usage_help();
  return -1;
}
