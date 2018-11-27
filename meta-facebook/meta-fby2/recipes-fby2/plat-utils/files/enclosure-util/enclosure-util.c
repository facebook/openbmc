/*
 * enclosure-util
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
#include <openbmc/nvme-mi.h>
#include <openbmc/pal.h>

#define MAX_DRIVE_NUM 2
#define MAX_GP_DRIVE_NUM 6
#define MAX_GPV2_DRIVE_NUM 12
#define CMD_DRIVE_STATUS 0
#define CMD_DRIVE_HEALTH 1
#define MAX_SERIAL_NUM 20

#define I2C_DEV_GP_1 "/dev/i2c-1"
#define I2C_DEV_GP_3 "/dev/i2c-5"
#define I2C_GP_MUX_ADDR 0x71

/* NVMe-MI SSD Status Flag bit mask */
#define NVME_SFLGS_MASK_BIT 0x28          // check bit 5,3

/* NVMe-MI SSD SMART Critical Warning */
#define NVME_SMART_WARNING_MASK_BIT 0x1F  // check bit 0~4


static uint8_t m_slot_id = 0;
static uint8_t m_slot_type = 0xFF;

static void
print_usage_help(void) {
  printf("Usage: enclosure-util <slot1|slot2|slot3|slot4> --drive-status <number, all>\n");
  printf("       enclosure-util <slot1|slot2|slot3|slot4> --drive-health\n");
}

static int
drive_status(ssd_data *ssd) {
  t_status_flags status_flag_decoding;
  t_smart_warning smart_warning_decoding;
  t_key_value_pair temp_decoding;
  t_key_value_pair pdlu_decoding;
  t_key_value_pair vendor_decoding;
  t_key_value_pair sn_decoding;

  nvme_vendor_decode(ssd->vendor, &vendor_decoding);
  printf("%s: %s\n", vendor_decoding.key, vendor_decoding.value);

  nvme_serial_num_decode(ssd->serial_num, &sn_decoding);
  printf("%s: %s\n", sn_decoding.key, sn_decoding.value);

  nvme_temp_decode(ssd->temp, &temp_decoding);
  printf("%s: %s\n", temp_decoding.key, temp_decoding.value);

  nvme_pdlu_decode(ssd->pdlu, &pdlu_decoding);
  printf("%s: %s\n", pdlu_decoding.key, pdlu_decoding.value);

  nvme_sflgs_decode(ssd->sflgs, &status_flag_decoding);
  printf("%s: %s\n", status_flag_decoding.self.key, status_flag_decoding.self.value);
  printf("    %s: %s\n", status_flag_decoding.read_complete.key, status_flag_decoding.read_complete.value);
  printf("    %s: %s\n", status_flag_decoding.ready.key, status_flag_decoding.ready.value);
  printf("    %s: %s\n", status_flag_decoding.functional.key, status_flag_decoding.functional.value);
  printf("    %s: %s\n", status_flag_decoding.reset_required.key, status_flag_decoding.reset_required.value);
  printf("    %s: %s\n", status_flag_decoding.port0_link.key, status_flag_decoding.port0_link.value);
  printf("    %s: %s\n", status_flag_decoding.port1_link.key, status_flag_decoding.port1_link.value);

  nvme_smart_warning_decode(ssd->warning, &smart_warning_decoding);
  printf("%s: %s\n", smart_warning_decoding.self.key, smart_warning_decoding.self.value);
  printf("    %s: %s\n", smart_warning_decoding.spare_space.key, smart_warning_decoding.spare_space.value);
  printf("    %s: %s\n", smart_warning_decoding.temp_warning.key, smart_warning_decoding.temp_warning.value);
  printf("    %s: %s\n", smart_warning_decoding.reliability.key, smart_warning_decoding.reliability.value);
  printf("    %s: %s\n", smart_warning_decoding.media_status.key, smart_warning_decoding.media_status.value);
  printf("    %s: %s\n", smart_warning_decoding.backup_device.key, smart_warning_decoding.backup_device.value);

  return 0;
}

static int
drive_health(ssd_data *ssd) {
  if ((ssd->warning & NVME_SMART_WARNING_MASK_BIT) != NVME_SMART_WARNING_MASK_BIT)
    return -1;

  if ((ssd->sflgs & NVME_SFLGS_MASK_BIT) != NVME_SFLGS_MASK_BIT)
    return -1;

  return 0;
}

static int
read_bic_nvme_data(uint8_t slot_id, uint8_t drv_num, uint8_t cmd) {
  int ret = 0;
  uint8_t bus, wbuf[8], rbuf[64];
  char stype_str[32] = {0};
  ssd_data ssd;

  if (m_slot_type == SLOT_TYPE_SERVER) {
    bus = 0x3;
    wbuf[0] = 1 << (drv_num - 1);
    sprintf(stype_str, "Server Board");
  } else {  // SLOT_TYPE_GPV2
    bus = (2 + drv_num/2) * 2 + 1;
    wbuf[0] = 1 << (drv_num%2);
    sprintf(stype_str, "Glacier Point V2");
  }

  // MUX
  ret = bic_master_write_read(slot_id, bus, 0xe2, wbuf, 1, rbuf, 0);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
    return ret;
  }

  if (cmd == CMD_DRIVE_STATUS) {
    memset(&ssd, 0x00, sizeof(ssd_data));
    printf("%s %u Drive%d\n", stype_str, slot_id, drv_num);

    do {
      wbuf[0] = 0x00;  // offset 00
      ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, 8);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
        break;
      }
      ssd.sflgs = rbuf[1];
      ssd.warning = rbuf[2];
      ssd.temp = rbuf[3];
      ssd.pdlu = rbuf[4];

      wbuf[0] = 0x08;  // offset 08
      ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, 24);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
        break;
      }
      ssd.vendor = (rbuf[1] << 8) | rbuf[2];
      memcpy(ssd.serial_num, &rbuf[3], MAX_SERIAL_NUM);

      drive_status(&ssd);
    } while (0);

    printf("\n");
    return ret;
  }
  else if (cmd == CMD_DRIVE_HEALTH) {
    memset(&ssd, 0x00, sizeof(ssd_data));

    wbuf[0] = 0x00;  // offset 00
    ret = bic_master_write_read(slot_id, bus, 0xd4, wbuf, 1, rbuf, 8);
    if (ret != 0) {
      syslog(LOG_DEBUG, "%s(): bic_master_write_read failed", __func__);
      return ret;
    }
    ssd.sflgs = rbuf[1];
    ssd.warning = rbuf[2];

    ret = drive_health(&ssd);
    return ret;
  }
  else {
    syslog(LOG_DEBUG, "%s(): unknown cmd", __func__);
    return -1;
  }

  return 0;
}

static int
read_gp_nvme_data(uint8_t slot_id, uint8_t drv_num, uint8_t cmd) {
  int ret = 0;
  uint8_t channel = 0;
  char *device;
  ssd_data ssd;

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
     syslog(LOG_DEBUG, "%s: fby2_mux_control failed", __func__);
     return ret;
  }

  if (cmd == CMD_DRIVE_STATUS) {
    memset(&ssd, 0x00, sizeof(ssd_data));
    printf("Glacier Point %u Drive%d\n", slot_id, drv_num);

    do {
      ret = nvme_sflgs_read(device, &ssd.sflgs);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_sflgs_read failed", __func__);
        break;
      }

      ret = nvme_smart_warning_read(device, &ssd.warning);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_smart_warning_read failed", __func__);
        break;
      }

      ret = nvme_temp_read(device, &ssd.temp);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_temp_read failed", __func__);
        break;
      }

      ret = nvme_pdlu_read(device, &ssd.pdlu);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_pdlu_read failed", __func__);
        break;
      }

      ret = nvme_vendor_read(device, &ssd.vendor);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_vendor_read failed", __func__);
        break;
      }

      ret = nvme_serial_num_read(device, ssd.serial_num, MAX_SERIAL_NUM);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): nvme_serial_num_read failed", __func__);
        break;
      }

      drive_status(&ssd);
    } while (0);

    printf("\n");
    return ret;
  }
  else if (cmd == CMD_DRIVE_HEALTH) {
    memset(&ssd, 0x00, sizeof(ssd_data));

    if (nvme_sflgs_read(device, &ssd.sflgs)) {
      syslog(LOG_DEBUG, "%s(): nvme_sflgs_read failed", __func__);
      return -1;
    }

    if (nvme_smart_warning_read(device, &ssd.warning)) {
      syslog(LOG_DEBUG, "%s(): nvme_smart_warning_read failed", __func__);
      return -1;
    }

    ret = drive_health(&ssd);
    return ret;
  }
  else {
    syslog(LOG_DEBUG, "%s(): unknown cmd", __func__);
    return -1;
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
enclosure_sig_handler(int sig) {
  if (m_slot_id) {
    ssd_monitor_enable(m_slot_id, m_slot_type, 1);
  }
}

int
main(int argc, char **argv) {
  int ret;
  uint8_t slot_id, slot_type = 0xFF;
  uint8_t i, drv_start = 1, drv_end = MAX_DRIVE_NUM;
  char *end = NULL;
  int (*read_nvme_data)(uint8_t,uint8_t,uint8_t);
  struct sigaction sa;

  if (argc < 3) {
    print_usage_help();
    return -1;
  }

  if (!strcmp(argv[1], "slot1")) {
    slot_id = 1;
  } else if (!strcmp(argv[1] , "slot2")) {
    slot_id = 2;
  } else if (!strcmp(argv[1] , "slot3")) {
    slot_id = 3;
  } else if (!strcmp(argv[1] , "slot4")) {
    slot_id = 4;
  } else {
    print_usage_help();
    return -1;
  }

  slot_type = fby2_get_slot_type(slot_id);
  if (slot_type == SLOT_TYPE_SERVER) {
    drv_start = 1;  // 1-based
    drv_end = MAX_DRIVE_NUM;
    read_nvme_data = read_bic_nvme_data;
  } else if (slot_type == SLOT_TYPE_GP) {
    drv_start = 0;  // 0-based
    drv_end = MAX_GP_DRIVE_NUM - 1;
    read_nvme_data = read_gp_nvme_data;
  } else if (slot_type == SLOT_TYPE_GPV2) {
    drv_start = 0;  // 0-based
    drv_end = MAX_GPV2_DRIVE_NUM - 1;
    read_nvme_data = read_bic_nvme_data;
  } else {
    return -1;
  }
  m_slot_type = slot_type;
  m_slot_id = slot_id;

  sa.sa_handler = enclosure_sig_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  if ((argc == 4) && !strcmp(argv[2], "--drive-status")) {
    if (!strcmp(argv[3], "all")) {
      ssd_monitor_enable(slot_id, slot_type, 0);
      for (i = drv_start; i <= drv_end; i++) {
        read_nvme_data(slot_id, i, CMD_DRIVE_STATUS);
      }
      ssd_monitor_enable(slot_id, slot_type, 1);
    }
    else {
      errno = 0;
      ret = strtol(argv[3], &end, 0);
      if (errno || *end || (ret < drv_start) || (ret > drv_end)) {
        print_usage_help();
        return -1;
      }

      ssd_monitor_enable(slot_id, slot_type, 0);
      read_nvme_data(slot_id, (uint8_t)ret, CMD_DRIVE_STATUS);
      ssd_monitor_enable(slot_id, slot_type, 1);
    }
  }
  else if ((argc == 3) && !strcmp(argv[2], "--drive-health")) {
    ssd_monitor_enable(slot_id, slot_type, 0);
    for (i = drv_start; i <= drv_end; i++) {
      ret = read_nvme_data(slot_id, i, CMD_DRIVE_HEALTH);
      printf("M.2-%u: %s\n", i, (ret == 0)?"Normal":"Abnormal");
    }
    ssd_monitor_enable(slot_id, slot_type, 1);
  }
  else {
    print_usage_help();
    return -1;
  }

  return 0;
}
