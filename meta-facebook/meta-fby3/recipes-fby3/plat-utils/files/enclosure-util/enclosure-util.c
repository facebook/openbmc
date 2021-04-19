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
#include <facebook/fby3_common.h>

#define CMD_DRIVE_STATUS 0
#define CMD_DRIVE_HEALTH 1
#define NVME_BAD_HEALTH 1
#define NVME_SFLGS_MASK_BIT 0x28          // check bit 5,3
#define NVME_SMART_WARNING_MASK_BIT 0x1F  // check bit 0~4
#define MAX_SERIAL_NUM 20


char path[64] = {0};
int fd;
static uint8_t m_slot_id = 0;
static uint8_t type_2ou = UNKNOWN_BOARD;

static void
print_usage_help(void) {
  printf("Usage: enclosure-util <slot1|slot2|slot3|slot4> --drive-status <all|1U-dev[0..3]|2U-dev[0..13]>\n");
  printf("       enclosure-util <slot1|slot2|slot3|slot4> --drive-health\n");
}

static int
ssd_monitor_enable(uint8_t slot_id, bool enable) {
  int ret = 0;
  ret = bic_enable_ssd_sensor_monitor(slot_id, enable, REXP_BIC_INTF);
  if ( enable == false ) {
    msleep(100);
  }
  return ret;
}

static int
drive_status(ssd_data *ssd) {
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
drive_health(ssd_data *ssd) {
  if ( type_2ou != GPV3_MCHP_BOARD && type_2ou != GPV3_BRCM_BOARD ) { // Not GPv3
    // since accelerator doesn't implement SMART WARNING, do not check it.
    if ((ssd->warning & NVME_SMART_WARNING_MASK_BIT) != NVME_SMART_WARNING_MASK_BIT)
      return NVME_BAD_HEALTH;
  }
  
  if ((ssd->sflgs & NVME_SFLGS_MASK_BIT) != NVME_SFLGS_MASK_BIT)
    return NVME_BAD_HEALTH;

  return 0;
}

static int
get_mapping_parameter(uint8_t device_id, uint8_t type_1ou, uint8_t *bus, uint8_t *intf, uint8_t *mux) {
  
  const uint8_t INTF[] = { 
    0,              // N/A
    FEXP_BIC_INTF,  // DEV_ID0_1OU
    FEXP_BIC_INTF,  // DEV_ID1_1OU
    FEXP_BIC_INTF,  // DEV_ID2_1OU
    FEXP_BIC_INTF,  // DEV_ID3_1OU
    REXP_BIC_INTF,  // DEV_ID0_2OU
    REXP_BIC_INTF,  // DEV_ID1_2OU
    REXP_BIC_INTF,  // DEV_ID2_2OU
    REXP_BIC_INTF,  // DEV_ID3_2OU
    REXP_BIC_INTF,  // DEV_ID4_2OU
    REXP_BIC_INTF   // DEV_ID5_2OU
  };

  // M.2 1OU device ->   3  2  1  0
  // E1S 1OU device ->   0  1  2  3
  const uint8_t BUS_EDSFF1U[] = { 0, 6, 4, 3, 2, 0, 0, 0, 0, 0, 0};
  const uint8_t BUS_1OU_2OU[] = { 0, 2, 2, 4, 4, 2, 2, 4, 4, 6, 6};
  const uint8_t MUX_1OU_2OU[] = { 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

  *intf = INTF[device_id];
  
  if (type_1ou == EDSFF_1U) {
    *bus = BUS_EDSFF1U[device_id];
  } else {
    *bus = BUS_1OU_2OU[device_id];
    *mux = MUX_1OU_2OU[device_id];
  }
  
  return 0;
}

static int
read_nvme_data(uint8_t slot_id, uint8_t device_id, uint8_t cmd) {
  int ret = 0;
  int offset_base = 0;
  char str[32];
  uint8_t type_1ou = 0;
  uint8_t intf = 0;
  uint8_t mux = 0;
  uint8_t bus = 0;
  uint8_t tbuf[8] = {0};
  uint8_t rbuf[64] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ssd_data ssd;
  
  memset(&ssd, 0x00, sizeof(ssd_data));

  //prevent the invalid access
  if ( type_2ou != GPV3_MCHP_BOARD && type_2ou != GPV3_BRCM_BOARD && type_2ou != E1S_BOARD) {
    bic_get_1ou_type(slot_id, &type_1ou);
    get_mapping_parameter(device_id, type_1ou, &bus, &intf, &mux);
  }

  fby3_common_dev_name(device_id, str);

  if (type_2ou == E1S_BOARD) {
    printf("slot%u-%s : %s\n", slot_id, str, "NA");
    return 0;
  } else if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) {
    // mux select
    ret = pal_gpv3_mux_select(slot_id, device_id);
    if (ret) {
      return ret; // fail to select mux
    }
    bus = get_gpv3_bus_number(device_id);
    intf = REXP_BIC_INTF;
  } else if (type_1ou != EDSFF_1U) { // E1S no need but 1/2OU need to stop sensor monitor then switch mux
    tbuf[0] = (bus << 1) + 1;
    tbuf[1] = 0x02; // mux address
    tbuf[2] = 0;    // read back 8 bytes
    tbuf[3] = 0x00; // offset 00
    tbuf[4] = mux;
    tlen = 5;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if (ret != 0) {
      printf("slot%u-%s : %s\n", slot_id, str, "NA");
      syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,tbuf[0],rlen);
      return -1;
    }
  }  

  if (cmd == CMD_DRIVE_STATUS) {
    printf("slot%u-%s : ", slot_id, str);
    do {
      tbuf[0] = (bus << 1) + 1;
      tbuf[1] = 0xD4;
      tbuf[2] = 8; //read back 8 bytes
      tbuf[3] = 0x00;  // offset 00
      tlen = 4;
      ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,tbuf[0],rlen);
        printf("NA");
        break;
      }

      ssd.sflgs = rbuf[offset_base + 1];
      ssd.warning = rbuf[offset_base + 2];
      ssd.temp = rbuf[offset_base + 3];
      ssd.pdlu = rbuf[offset_base + 4];

      tbuf[2] = 24 + offset_base;;  // read back 24 bytes
      tbuf[3] = 0x08;  // offset 08
      ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
      if (ret != 0) {
        syslog(LOG_DEBUG, "%s(): bic_master_write_read offset=%d read length=%d failed", __func__,tbuf[0],rlen);
        printf("NA");
        break;
      }
      ssd.vendor = (rbuf[offset_base + 1] << 8) | rbuf[offset_base + 2];
      memcpy(ssd.serial_num, &rbuf[offset_base + 3], MAX_SERIAL_NUM);

      drive_status(&ssd);
    } while (0);
    printf("\n");

  } else if (cmd == CMD_DRIVE_HEALTH) {
    tbuf[0] = (bus << 1) + 1;
    tbuf[1] = 0xD4;
    tbuf[2] = 0x08; // read back 8 bytes
    tbuf[3] = 0x00; // offset 00
    tlen = 4;
    ret = bic_ipmb_send(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen, intf);
    if (ret != 0) {
      printf("slot%u-%s : %s\n", slot_id, str, "NA");
      return 0;
    }
    
    ssd.sflgs = rbuf[1];
    ssd.warning = rbuf[2];
    ret = drive_health(&ssd);  // ret 1: Abnormal  0:Normal
    printf("slot%u-%s : %s\n", slot_id, str, (ret == 0)?"Normal":"Abnormal");
  } else {
    printf("%s(): unknown cmd \n", __func__);
    return -1;
  }

  return 0;
}

static void
enclosure_sig_handler(int sig) {
  if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) { // Config C or Config D GPv3
    if ( ssd_monitor_enable(m_slot_id, true) < 0 ) {
      printf("err: failed to enable SSD monitoring");
    }
  }
  if (flock(fd, LOCK_UN)) {
    syslog(LOG_WARNING, "%s: failed to unflock on %s", __func__, path);
  }
  close(fd);
  remove(path);
}

int
main(int argc, char **argv) {
  int ret;

  int present = 0;
  uint8_t slot_id = 0;
  uint8_t dev_id = 0;
  uint8_t device_start = 0, device_end = 0;
  uint8_t is_slot_present = 0;
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

  m_slot_id = slot_id;

  // need to check slot present
  ret = pal_is_fru_prsnt(slot_id, &is_slot_present); 
  if (ret < 0 || is_slot_present == 0) {
    printf("%s is not present!\n", argv[1]);
    return -1;
  }

  sa.sa_handler = enclosure_sig_handler;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGSEGV, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  sprintf(path, SLOT_SENSOR_LOCK, slot_id);
  fd = open(path, O_CREAT | O_RDWR, 0666);
  ret = flock(fd, LOCK_EX | LOCK_NB);
  if (ret) {
    if(EWOULDBLOCK == errno) {
      printf("the lock file is already using, please wait \n");
      return -1;
    }
  }

  // check 1/2OU present status
  ret = bic_is_m2_exp_prsnt(slot_id);
  if ( ret < 0 ) {
    printf("%s() Cannot get the m2 prsnt status\n", __func__);
    goto exit;
  }

  present = (uint8_t)ret;

  if ( (present & PRESENT_2OU) == PRESENT_2OU ) {
    if ( fby3_common_get_2ou_board_type(slot_id, &type_2ou) < 0) {
      printf("Failed to get slot%d 2ou board type\n",type_2ou);
      goto exit;
    }
  }

  if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) { // Config C or Config D GPv3
    device_start = DEV_ID0_2OU;
    device_end = DEV_ID13_2OU;
    if ( ssd_monitor_enable(slot_id, false) < 0 ) {
      printf("err: failed to disable SSD monitoring\n");
      goto exit;
    }
  } else if ( type_2ou == E1S_BOARD ) {
    device_start = DEV_ID0_2OU;
    device_end = DEV_ID5_2OU;
  } else if ( present == PRESENT_1OU ) {
    device_start = DEV_ID0_1OU;
    device_end = DEV_ID3_1OU;
  } else if ( present == PRESENT_2OU ) {
    device_start = DEV_ID0_2OU;
    device_end = DEV_ID5_2OU;
  } else if ( present == (PRESENT_1OU + PRESENT_2OU) ) {
    device_start = DEV_ID0_1OU;
    device_end = DEV_ID5_2OU;
  } else {
    printf("Please check the board config \n");
    goto exit;
  }
 
  if ((argc == 4) && !strcmp(argv[2], "--drive-status")) {
    if (!strcmp(argv[3], "all")) {
      for (int i = device_start; i <= device_end; i++) {
        read_nvme_data(slot_id, i, CMD_DRIVE_STATUS);
      }
    } else {
      ret = pal_get_dev_id(argv[3], &dev_id);
      if (ret < 0) {
        printf("pal_get_dev_id failed for %s %s\n", argv[1], argv[3]);
        print_usage_help();
        goto exit;
      }
      if ((dev_id < device_start) || (dev_id > device_end)) {
        printf("Please check the board config \n");
        goto exit;
      }
      read_nvme_data(slot_id, dev_id, CMD_DRIVE_STATUS);
    }
  } else if ((argc == 3) && !strcmp(argv[2], "--drive-health")) {
    for (int i = device_start; i <= device_end; i++) {
      read_nvme_data(slot_id, i, CMD_DRIVE_HEALTH);
    }
  } else {
    print_usage_help();
    goto exit;
  }
  
exit:
  if ( type_2ou == GPV3_MCHP_BOARD || type_2ou == GPV3_BRCM_BOARD ) { // Config C or Config D GPv3
    if ( ssd_monitor_enable(slot_id, true) < 0 ) {
      printf("err: failed to enable SSD monitoring");
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
