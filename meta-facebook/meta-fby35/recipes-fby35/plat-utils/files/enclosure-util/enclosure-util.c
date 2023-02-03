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
#include <facebook/fby35_common.h>

#define CMD_DRIVE_STATUS 0
#define CMD_DRIVE_HEALTH 1
#define NVME_BAD_HEALTH 1
#define NVME_SFLGS_MASK_BIT 0x28          // check bit 5,3
#define NVME_SMART_WARNING_MASK_BIT 0x1F  // check bit 0~4
#define NVME_SSD_I2C_ADDR 0xD4
#define MAX_SERIAL_NUM 20


char path[64] = {0};
int fd;

typedef struct {
  int (*nvme_get_bus_intf)(uint8_t dev_id, uint8_t* bus, uint8_t* intf);
  int (*nvme_set_mux_select)(uint8_t slot_id, uint8_t dev_id);
  int (*board_pre_setup)(uint8_t slot_id);
  int (*board_post_setup)(uint8_t slot_id);
} nvme_ops_t;

typedef struct {
  uint8_t slot_id;
  uint8_t present;
  uint8_t type_1ou;
  uint8_t type_2ou;
  uint8_t dev_start;
  uint8_t dev_end;
  const nvme_ops_t* nvme_ops;
} sys_config_t;

sys_config_t g_sys_conf = {0};

static int
ssd_monitor_enable(uint8_t slot_id, uint8_t intf, bool enable) {
  int ret = 0;
  ret = bic_enable_ssd_sensor_monitor(slot_id, enable, intf);
  if ( enable == false ) {
    msleep(100);
  }
  return ret;
}

//==============================================================================
// VF_1U
//==============================================================================
static int
vf_1u_nvme_get_bus_intf(uint8_t dev_id, uint8_t *bus, uint8_t *intf) {
  const uint8_t bus_map_table[] = {0, 5, 4, 3, 2};

  if (dev_id < DEV_ID0_1OU || dev_id > DEV_ID3_1OU) {
    return -1;
  }

  *intf = FEXP_BIC_INTF;
  *bus = bus_map_table[dev_id];
  return 0;
}

const nvme_ops_t vf_1u_ops = {
  .nvme_get_bus_intf = vf_1u_nvme_get_bus_intf,
  .nvme_set_mux_select = NULL,
  .board_pre_setup = NULL,
  .board_post_setup = NULL,
};

//==============================================================================
// M.2 1OU & 2OU
//==============================================================================
static int
board_1u_2u_nvme_get_bus_intf(uint8_t dev_id, uint8_t *bus, uint8_t *intf) {
  const uint8_t bus_map_table[] = { 0, 2, 2, 4, 4, 2, 2, 4, 4, 6, 6};

  if (dev_id >= DEV_ID0_1OU && dev_id <= DEV_ID3_1OU) {
    *intf = FEXP_BIC_INTF;
  } else if (dev_id >= DEV_ID0_2OU && dev_id <= DEV_ID5_2OU) {
    *intf = REXP_BIC_INTF;
  } else {
    return -1;
  }

  *bus = bus_map_table[dev_id];
  return 0;
}

static int
board_1u_2u_set_mux_select(uint8_t slot_id, uint8_t dev_id) {
  const uint8_t mux_map[] = { 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

  uint8_t bus = 0;
  uint8_t intf = 0;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;

  if (board_1u_2u_nvme_get_bus_intf(dev_id, &bus, &intf) < 0) {
    return -1;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = 0x02; // mux address
  tbuf[2] = 0;    // read back 8 bytes
  tbuf[3] = 0x00; // offset 00
  tbuf[4] = mux_map[dev_id];
  tlen = 5;
  ret = bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if (ret != 0) {
    syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,tbuf[0],rlen);
    return -1;
  }
  return 0;
}

const nvme_ops_t board_1u_2u_ops = {
  .nvme_get_bus_intf = board_1u_2u_nvme_get_bus_intf,
  .nvme_set_mux_select = board_1u_2u_set_mux_select,
  .board_pre_setup = NULL,
  .board_post_setup = NULL,
};

//==============================================================================
// GPV3
//==============================================================================
static int
gpv3_nvme_get_bus_intf(uint8_t dev_id, uint8_t *bus, uint8_t *intf) {
  if (dev_id < DEV_ID0_2OU || dev_id > DEV_ID13_2OU) {
    return -1;
  }
  *bus = get_gpv3_bus_number(dev_id);
  *intf = REXP_BIC_INTF;
  return 0;
}

static int
gpv3_1u_2u_set_mux_select(uint8_t slot_id, uint8_t dev_id) {
  if (pal_gpv3_mux_select(slot_id, dev_id) != BIC_STATUS_SUCCESS) {
    return -1;
  }
  return 0;
}

static int
gpv3_board_pre_setup(uint8_t slot_id) {
  return ssd_monitor_enable(slot_id, false, REXP_BIC_INTF);
}

static int
gpv3_board_post_setup(uint8_t slot_id) {
  return ssd_monitor_enable(slot_id, true, REXP_BIC_INTF);
}

const nvme_ops_t gpv3_ops = {
  .nvme_get_bus_intf = gpv3_nvme_get_bus_intf,
  .nvme_set_mux_select = gpv3_1u_2u_set_mux_select,
  .board_pre_setup = gpv3_board_pre_setup,
  .board_post_setup = gpv3_board_post_setup,
};

static void
print_usage_help(void) {
  printf("Usage: enclosure-util <slot1|slot2|slot3|slot4> --drive-status <all|1U-dev[0..3]|2U-dev[0..13]>\n");
  printf("       enclosure-util <slot1|slot2|slot3|slot4> --drive-health\n");
}

static int
print_drive_status(ssd_data *ssd) {
  t_status_flags status_flag_decoding;
  t_smart_warning smart_warning_decoding;
  t_key_value_pair temp_decoding;
  t_key_value_pair pdlu_decoding;
  t_key_value_pair vendor_decoding;
  t_key_value_pair sn_decoding;

  printf("\n");
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

  nvme_temp_decode(ssd->temp, &temp_decoding);
  printf("%s: %s\n", temp_decoding.key, temp_decoding.value);

  nvme_pdlu_decode(ssd->pdlu, &pdlu_decoding);
  printf("%s: %s\n", pdlu_decoding.key, pdlu_decoding.value);

  nvme_vendor_decode(ssd->vendor, &vendor_decoding);
  printf("%s: %s\n", vendor_decoding.key, vendor_decoding.value);

  nvme_serial_num_decode(ssd->serial_num, &sn_decoding);
  printf("%s: %s\n", sn_decoding.key, sn_decoding.value);

  return 0;
}

static int
drive_health(ssd_data *ssd, bool is_gpv3) {
  if (!ssd)
    return NVME_BAD_HEALTH;

  if (! is_gpv3) { // Not GPv3
    // since accelerator doesn't implement SMART WARNING, do not check it.
    if ((ssd->warning & NVME_SMART_WARNING_MASK_BIT) != NVME_SMART_WARNING_MASK_BIT)
      return NVME_BAD_HEALTH;
  }

  if ((ssd->sflgs & NVME_SFLGS_MASK_BIT) != NVME_SFLGS_MASK_BIT)
    return NVME_BAD_HEALTH;

  return 0;
}

static int
read_nvme_status(uint8_t slot_id, uint8_t bus, uint8_t intf, ssd_data* ssd) {
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;

  if (!ssd) {
    syslog(LOG_ERR, "%s(): invalid ssd pointer", __func__);
    return -1;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = NVME_SSD_I2C_ADDR;
  tbuf[2] = 0x08; //read back 8 bytes
  tbuf[3] = 0x00; // offset 00
  tlen = 4;
  ret = bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if (ret != 0) {
    syslog(LOG_ERR, "%s(): master_write_read failed, bus=%u, offset=%u read length=%u", __func__, bus, tbuf[3], rlen);
    return -1;
  }

  ssd->sflgs = rbuf[1];
  ssd->warning = rbuf[2];
  ssd->temp = rbuf[3];
  ssd->pdlu = rbuf[4];

  tbuf[2] = 24;  // read back 24 bytes
  tbuf[3] = 0x08;  // offset 08
  ret = bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if (ret != 0) {
    syslog(LOG_ERR, "%s(): master_write_read failed, bus=%u, offset=%u read length=%u", __func__, bus, tbuf[3], rlen);
    return -1;
  }
  ssd->vendor = (rbuf[1] << 8) | rbuf[2];
  memcpy(ssd->serial_num, &rbuf[3], MAX_SERIAL_NUM);

  return 0;
}

static int
read_nvme_health(uint8_t slot_id, uint8_t bus, uint8_t intf, ssd_data* ssd) {
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;

  if (!ssd) {
    syslog(LOG_ERR, "%s(): invalid ssd pointer", __func__);
    return -1;
  }

  tbuf[0] = (bus << 1) + 1;
  tbuf[1] = NVME_SSD_I2C_ADDR;
  tbuf[2] = 0x08; // read back 8 bytes
  tbuf[3] = 0x00; // offset 00
  tlen = 4;
  ret = bic_data_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
  if (ret != 0) {
    syslog(LOG_ERR, "%s(): master_write_read failed, bus=%u, offset=%u read length=%u", __func__, bus, tbuf[3], rlen);
    return -1;
  }

  ssd->sflgs = rbuf[1];
  ssd->warning = rbuf[2];
  return 0;
}

static int
read_nvme_data(uint8_t slot_id, uint8_t device_id, uint8_t cmd, sys_config_t* sys_conf) {
  char str[32];
  uint8_t bus = 0;
  uint8_t intf = 0;
  ssd_data ssd;

  memset(&ssd, 0x00, sizeof(ssd_data));

  if (!sys_conf) {
    syslog(LOG_ERR, "%s(): invalid sys_conf pointer", __func__);
    return -1;
  }

  if (sys_conf->nvme_ops->nvme_get_bus_intf(device_id, &bus, &intf) < 0) {
    printf("Error: Failed to get bus and intf\n");
    return -1;
  }

  fby35_common_dev_name(device_id, str);

  if (sys_conf->nvme_ops->nvme_set_mux_select) {
    if (sys_conf->nvme_ops->nvme_set_mux_select(sys_conf->slot_id, device_id) < 0) {
      printf("slot%u-%s : %s\n", slot_id, str, "NA");
      return -1;
    }
  }

  printf("slot%u-%s : ", slot_id, str);
  if (cmd == CMD_DRIVE_STATUS) {
    if (read_nvme_status(slot_id, bus, intf, &ssd) < 0) {
      printf("NA\n");
    } else {
      print_drive_status(&ssd);
      printf("\n");
    }
  } else if (cmd == CMD_DRIVE_HEALTH) {
    if (read_nvme_health(slot_id, bus, intf, &ssd) < 0) {
      printf("NA\n");
    } else {
      printf("%s\n", (drive_health(&ssd, (sys_conf->nvme_ops == &gpv3_ops)) == 0)?"Normal":"Abnormal");
    }
  } else {
    printf("Unknown command\n");
    return -1;
  }

  return 0;
}

static void
enclosure_sig_handler(int sig __attribute__((unused))) {
  if (g_sys_conf.nvme_ops->board_post_setup) {
    if (g_sys_conf.nvme_ops->board_post_setup(g_sys_conf.slot_id) < 0 ) {
      printf("Error: board post setup failed");
    }
  }
  if (flock(fd, LOCK_UN)) {
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
  }
  close(fd);
  remove(path);
}

static int
setup_sys_config(uint8_t slot_id, sys_config_t* sys_conf) {
  uint8_t val = 0;
  int ret;

  if (!sys_conf) {
    syslog(LOG_ERR, "%s(): invalid sys_conf pointer", __func__);
    return -1;
  }

  sys_conf->slot_id = slot_id;

  // need to check slot present
  ret = pal_is_fru_prsnt(slot_id, &val);
  if (ret < 0 || val == 0) {
    printf("slot%u is not present!\n", slot_id);
    return -1;
  }

  // check 1/2OU present status
  ret = bic_is_exp_prsnt(sys_conf->slot_id);
  if ( ret < 0 ) {
    printf("%s() Cannot get the m2 prsnt status\n", __func__);
    return -1;
  } else {
    sys_conf->present = (uint8_t) ret;
  }

  // get 1OU board type
  if ((sys_conf->present & PRESENT_1OU) == PRESENT_1OU) {
    if (bic_get_1ou_type(sys_conf->slot_id, &sys_conf->type_1ou) != 0) {
      printf("Failed to get slot%d 1ou board type\n", sys_conf->slot_id);
      return -1;
    }
  }

  // get 2OU board type
  if ((sys_conf->present & PRESENT_2OU) == PRESENT_2OU) {
    if (fby35_common_get_2ou_board_type(sys_conf->slot_id, &sys_conf->type_2ou) < 0) {
      printf("Failed to get slot%d 2ou board type\n", sys_conf->slot_id);
      return -1;
    }
  }

  // config setup
  if (sys_conf->present == PRESENT_1OU && sys_conf->type_1ou == TYPE_1OU_VERNAL_FALLS_WITH_AST) {
    sys_conf->dev_start = DEV_ID0_1OU;
    sys_conf->dev_end = DEV_ID3_1OU;
    sys_conf->nvme_ops = &vf_1u_ops;
  } else if (sys_conf->present == PRESENT_2OU &&
      (sys_conf->type_2ou == GPV3_MCHP_BOARD ||
      sys_conf->type_2ou == GPV3_BRCM_BOARD)) {
    sys_conf->dev_start = DEV_ID0_2OU;
    sys_conf->dev_end = DEV_ID13_2OU;
    sys_conf->nvme_ops = &gpv3_ops;
  } else if (sys_conf->present == PRESENT_1OU) {
    sys_conf->dev_start = DEV_ID0_1OU;
    sys_conf->dev_end = DEV_ID3_1OU;
    sys_conf->nvme_ops = &board_1u_2u_ops;
  } else if (sys_conf->present == PRESENT_2OU) {
    sys_conf->dev_start = DEV_ID0_2OU;
    sys_conf->dev_end = DEV_ID5_2OU;
    sys_conf->nvme_ops = &board_1u_2u_ops;
  } else if (sys_conf->present == (PRESENT_1OU + PRESENT_2OU)) {
    sys_conf->dev_start = DEV_ID0_1OU;
    sys_conf->dev_end = DEV_ID5_2OU;
    sys_conf->nvme_ops = &board_1u_2u_ops;
  } else {
    printf("Please check the board config \n");
    return -1;
  }

  return 0;
}

int
main(int argc, char **argv) {
  int ret;
  uint8_t slot_id = 0;
  uint8_t dev_id = 0;
  struct sigaction sa;

  if (argc != 3 && argc != 4) {
    print_usage_help();
    return -1;
  }

  ret = pal_get_fru_id(argv[1], &slot_id);
  if (ret < 0) {
    printf("Wrong fru: %s\n", argv[1]);
    print_usage_help();
    return -1;
  }

  if (setup_sys_config(slot_id, &g_sys_conf) < 0) {
    return -1;
  }

  // check file lock
  sprintf(path, SLOT_SENSOR_LOCK, g_sys_conf.slot_id);
  fd = open(path, O_CREAT | O_RDWR, 0666);
  ret = flock(fd, LOCK_EX | LOCK_NB);
  if (ret) {
    if(EWOULDBLOCK == errno) {
      printf("the lock file is already using, please wait \n");
      return -1;
    }
  }

  if (g_sys_conf.nvme_ops->board_pre_setup) {
    if (g_sys_conf.nvme_ops->board_pre_setup(g_sys_conf.slot_id) < 0) {
      printf("Error: board pre setup failed\n");
      goto exit;
    }
  }

  // setup sig
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
      for (int i = g_sys_conf.dev_start; i <= g_sys_conf.dev_end; i++) {
        read_nvme_data(g_sys_conf.slot_id, i, CMD_DRIVE_STATUS, &g_sys_conf);
      }
    } else {
      ret = pal_get_dev_id(argv[3], &dev_id);
      if (ret < 0) {
        printf("pal_get_dev_id failed for %s %s\n", argv[1], argv[3]);
        print_usage_help();
        goto exit;
      }
      if ((dev_id < g_sys_conf.dev_start) || (dev_id > g_sys_conf.dev_end)) {
        printf("Please check the board config \n");
        goto exit;
      }
      read_nvme_data(g_sys_conf.slot_id, dev_id, CMD_DRIVE_STATUS, &g_sys_conf);
    }
  } else if ((argc == 3) && !strcmp(argv[2], "--drive-health")) {
    for (int i = g_sys_conf.dev_start; i <= g_sys_conf.dev_end; i++) {
      read_nvme_data(g_sys_conf.slot_id, i, CMD_DRIVE_HEALTH, &g_sys_conf);
    }
  } else {
    print_usage_help();
    goto exit;
  }

exit:
  if (g_sys_conf.nvme_ops->board_post_setup) {
    if (g_sys_conf.nvme_ops->board_post_setup(g_sys_conf.slot_id) < 0 ) {
      printf("Error: board post setup failed");
    }
  }
  ret = flock(fd, LOCK_UN);
  if (ret != 0) {
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
  }
  close(fd);
  remove(path);
  return 0;
}
