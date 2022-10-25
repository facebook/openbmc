/*
 * bic-util
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
#include <sys/time.h>
#include <time.h>

static uint8_t bmc_location = 0xff;
static uint8_t riser_board = UNKNOWN_BOARD;
static uint8_t expFru = 0xff;
const static char *intf_name[4] = {"Server Board", "Front Expansion Board", "Riser Expansion Board", "Baseboard"};
const uint8_t intf_size = 4;
static const uint8_t gpio_sb_usbhub[] = {GPIO_RST_USB_HUB};
static const uint8_t gpio_cwc_usbhub[] = {CWC_GPIO_RST_USB_HUB};
static const uint8_t gpio_gpv3_usbhub[] = {GPV3_GPIO_RST_USB_HUB1, GPV3_GPIO_RST_USB_HUB2, GPV3_GPIO_RST_USB_HUB3};
static const uint8_t gpio_cwc_usbmux[] = {CWC_GPIO_USB_MUX};

static const char *option_list[] = {
  "--get_gpio",
  "--set_gpio [gpio_num] [value]",
  "--get_gpio_config",
  "--set_gpio_config $gpio_num $value",
  "--check_status",
  "--get_dev_id",
  "--reset",
  "--get_post_code",
  "--get_sdr",
  "--read_sensor",
  "--perf_test [loop_count] (0 to run forever)",
  "--clear_cmos",
  "--file [path]",
  "--check_usb_port [sb|1ou|2ou]",
  "--get_board_revision 1ou",
};

static const char *class2_options[] = {
  "--get_slot_id",
  "--set_usb_hub [0|1|2|all] [on|off|reset|to-bmc|to-usbc]",
  "--get_usb_hub [0|1|2|all]",
  "--get_pci_link",
};

static void
print_usage_help(void) {
  int i;

  if (riser_board == UNKNOWN_BOARD) {
    pal_get_2ou_board_type(FRU_SLOT1, &riser_board);
  }

  printf("Usage: bic-util <%s> <[0..n]data_bytes_to_send>\n", slot_usage);
  if ( riser_board  == CWC_MCHP_BOARD ) {
    printf("Usage: bic-util <slot1-2U-exp|slot1-2U-top|slot1-2U-bot> <[0..n]data_bytes_to_send>\n");
  }
  printf("Usage: bic-util <%s> <option>\n", slot_usage);
  if ( riser_board == CWC_MCHP_BOARD ) {
    printf("Usage: bic-util <slot1-2U-exp|slot1-2U-top|slot1-2U-bot> <--reset|--get_gpio|--set_gpio|--get_dev_id|--set_usb_hub|--get_usb_hub>\n");
  } else {
    printf("Usage: bic-util <slot1-2U|slot3-2U> <--reset|--get_gpio|--set_gpio|--get_gpio_config|--set_gpio_config|--get_dev_id>\n");
  }
  printf("       option:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);

  if ( riser_board == CWC_MCHP_BOARD || riser_board == GPV3_MCHP_BOARD ) {
    for (i = 0; i < sizeof(class2_options)/sizeof(class2_options[0]); i++) {
      printf("       %s\n", class2_options[i]);
    }
  }
}

static int
util_check_exp_status(uint8_t exp) {
  uint8_t status = 0, intf = 0;
  int ret = 0;

  switch (exp) {
    case FRU_CWC:
      intf = REXP_BIC_INTF;
      break;
    case FRU_2U_TOP:
      intf = RREXP_BIC_INTF1;
      break;
    case FRU_2U_BOT:
      intf = RREXP_BIC_INTF2;
      break;
    default:
      printf("Unknown exp fru : %d\n", exp);
      return -1;
  }

  ret = pal_is_fru_prsnt(exp, &status);

  if (ret < 0) {
    printf("unable to check fru presence\n");
  } else {
    if (status == 1) {
      ret = pal_get_exp_power(exp, &status);
    } else {
      printf("Fru is empty, unable to check BIC status\n");
      return -1;
    }
  }

  if (ret < 0) {
    printf("unable to check fru power\n");
  } else {
    if (status == SERVER_12V_ON) {
      if ( is_bic_ready(exp, intf) == BIC_STATUS_SUCCESS ) {
        printf("BIC status ok\n");
        ret = 0;
      } else {
        printf("Error: BIC not ready\n");
        ret = -1;
      }
    } else {
      printf("Fru is 12V-off, unable to check BIC status\n");
      ret = -1;
    }
  }

  return ret;
}

// Check BIC status
static int
util_check_status(uint8_t slot_id) {
  int ret = 0;
  uint8_t status;

  if (slot_id == FRU_SLOT1 && 
      (expFru == FRU_CWC || expFru == FRU_2U_TOP || expFru == FRU_2U_BOT)) {
    return util_check_exp_status(expFru);
  }

  // BIC status is only valid if 12V-on. check this first
  ret = pal_get_server_12v_power(slot_id, &status);
  if ( (ret < 0) || (SERVER_12V_OFF == status) ) {
    ret = pal_is_fru_prsnt(slot_id, &status);

    if (ret < 0) {
       printf("unable to check BIC status\n");
       return ret;
    }

    if (status == 0) {
      printf("Slot is empty, unable to check BIC status\n");
    } else {
      printf("Slot is 12V-off, unable to check BIC status\n");
    }
    ret = 0;
  } else {
    if ( is_bic_ready(slot_id, NONE_INTF) == BIC_STATUS_SUCCESS ) {
      printf("BIC status ok\n");
      ret = 0;
    } else {
      printf("Error: BIC not ready\n");
      ret = -1;
    }
  }
  return ret;
}

// Test to Get device ID
static int
util_get_device_id(uint8_t slot_id, uint8_t intf) {
  int ret = 0;
  ipmi_dev_id_t id = {0};

  ret = bic_get_dev_id(slot_id, &id, intf);
  if (ret) {
    printf("util_get_device_id: bic_get_dev_id returns %d\n", ret);
    return ret;
  }

  // Print response
  printf("Device ID: 0x%X\n", id.dev_id);
  printf("Device Revision: 0x%X\n", id.dev_rev);
  printf("Firmware Revision: 0x%X:0x%X\n", id.fw_rev1, id.fw_rev2);
  printf("IPMI Version: 0x%X\n", id.ipmi_ver);
  printf("Device Support: 0x%X\n", id.dev_support);
  printf("Manufacturer ID: 0x%X:0x%X:0x%X\n", id.mfg_id[2], id.mfg_id[1], id.mfg_id[0]);
  printf("Product ID: 0x%X:0x%X\n", id.prod_id[1], id.prod_id[0]);
  printf("Aux. FW Rev: 0x%X:0x%X:0x%X:0x%X\n", id.aux_fw_rev[0], id.aux_fw_rev[1],id.aux_fw_rev[2],id.aux_fw_rev[3]);

  return ret;
}

// reset BIC
static int
util_bic_reset(uint8_t slot_id, uint8_t intf) {
  int ret = 0;
  ret = bic_reset(slot_id, intf);
  printf("Performing BIC reset, status %d\n", ret);
  return ret;
}

static int
util_is_numeric(char **argv) {
  int j = 0;
  int len = 0;
  for (int i = 0; i < 2; i++) { //check netFn cmd
    len = strlen(argv[i]);
    if (len > 2 && argv[i][0] == '0' && (argv[i][1] == 'x' || argv[i][1] == 'X')) {
      j=2;
      for (; j < len; j++) {
        if (!isxdigit(argv[i][j]))
          return 0;
      }
    } else {
      j=0;
      for (; j < len; j++) {
        if (!isdigit(argv[i][j]))
          return 0;
      }
    }
  }
  return 1;
}

static int
process_command(uint8_t slot_id, int argc, char **argv, uint8_t intf) {
  int i, ret, retry = 2;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  for (i = 0; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  while (retry >= 0) {
    ret = bic_ipmb_send(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen, intf);
    if (ret == 0)
      break;

    retry--;
  }

  if (ret) {
    printf("BIC no response!\n");
    return ret;
  }

  for (i = 0; i < rlen; i++) {
    if (!(i % 16) && i)
      printf("\n");

    printf("%02X ", rbuf[i]);
  }
  printf("\n");

  return 0;
}

static int
process_file(uint8_t slot_id, char *path) {
#define MAX_ARG_NUM 64
  FILE *fp;
  int argc;
  char buf[1024];
  char *str, *next, *del=" \n";
  char *argv[MAX_ARG_NUM] = {NULL};

  if (!(fp = fopen(path, "r"))) {
    syslog(LOG_WARNING, "Failed to open %s", path);
    return -1;
  }

  while (fgets(buf, sizeof(buf), fp) != NULL) {
    str = strtok_r(buf, del, &next);
    for (argc = 0; argc < MAX_ARG_NUM && str; argc++, str = strtok_r(NULL, del, &next)) {
      if (str[0] == '#')
        break;

      if (!argc && !strcmp(str, "echo")) {
        printf("%s", (*next) ? next : "\n");
        break;
      }
      argv[argc] = str;
    }
    if (argc <= 1) {
      continue;
    }

    process_command(slot_id, argc, argv, NONE_INTF);
  }
  fclose(fp);

  return 0;
}

#define BIT_VALUE(list, index) \
           ((((uint8_t*)&list)[index/8]) >> (index % 8)) & 0x1\

static int
util_get_gpio(uint8_t slot_id, uint8_t intf) {
  int ret = 0;
  uint8_t i;
  uint8_t gpio_pin_cnt = fby3_get_gpio_list_size(intf);
  char gpio_pin_name[32] = "\0";
  bic_gpio_t gpio = {0};

  ret = bic_get_gpio(slot_id, &gpio, intf);
  if ( ret < 0 ) {
    printf("%s() bic_get_gpio returns %d\n", __func__, ret);
    return ret;
  }

  if (expFru == FRU_CWC || expFru == FRU_2U_TOP || expFru == FRU_2U_BOT) {
    gpio_pin_cnt = fby3_get_exp_gpio_list_size(expFru);
  }

  // Print the gpio index, name and value
  for (i = 0; i < gpio_pin_cnt; i++) {
    if (expFru == FRU_CWC || expFru == FRU_2U_TOP || expFru == FRU_2U_BOT) {
      fby3_get_exp_gpio_name(expFru, i, gpio_pin_name);
    } else {
      fby3_get_gpio_name(slot_id, i, gpio_pin_name, intf);
    }
    printf("%d %s: %d\n",i , gpio_pin_name, BIT_VALUE(gpio, i));
  }

  return ret;
}

static int
util_set_gpio(uint8_t slot_id, uint8_t gpio_num, uint8_t gpio_val, uint8_t intf) {
  uint8_t gpio_pin_cnt = fby3_get_gpio_list_size(intf);
  char gpio_pin_name[32] = "\0";
  int ret = -1;

  if (expFru == FRU_CWC || expFru == FRU_2U_TOP || expFru == FRU_2U_BOT) {
    gpio_pin_cnt = fby3_get_exp_gpio_list_size(expFru);
  }

  if ( gpio_num >= gpio_pin_cnt ) {
    printf("slot %d: Invalid GPIO pin number %d\n", slot_id, gpio_num);
    return ret;
  }

  if (expFru == FRU_CWC || expFru == FRU_2U_TOP || expFru == FRU_2U_BOT) {
    fby3_get_exp_gpio_name(expFru, gpio_num, gpio_pin_name);
  } else {
    fby3_get_gpio_name(slot_id, gpio_num, gpio_pin_name, intf);
  }
  printf("slot %d: setting [%d]%s to %d\n", slot_id, gpio_num, gpio_pin_name, gpio_val);

  ret = remote_bic_set_gpio(slot_id, gpio_num, gpio_val, intf);
  if (ret < 0) {
    printf("%s() bic_set_gpio returns %d\n", __func__, ret);
  }

  return ret;
}

static int
util_get_gpio_config(uint8_t slot_id, uint8_t intf) {
  int ret = 0;
  uint8_t i;
  uint8_t gpio_pin_cnt = fby3_get_gpio_list_size(intf);
  char gpio_pin_name[32] = "\0";
  bic_gpio_config_t gpio_config = {0}; 

  // Print the gpio index, name and value
  for (i = 0; i < gpio_pin_cnt; i++) {
    fby3_get_gpio_name(slot_id, i, gpio_pin_name, intf);
    //printf("%d %s: %d\n",i , gpio_pin_name, BIT_VALUE(gpio, i));
    if ( intf != NONE_INTF ) {
      ret = remote_bic_get_gpio_config(slot_id, i, (uint8_t *)&gpio_config,intf);
    } else {
      ret = bic_get_gpio_config(slot_id, i, (uint8_t *)&gpio_config);
    }
    if ( ret < 0 ) {
      printf("Failed to get %s config\n\n", gpio_pin_name);
      continue;
    }

    printf("gpio_config for pin#%d (%s):\n", i, gpio_pin_name);
    printf("Direction: %s", gpio_config.dir > 0?"Output,":"Input, ");
    printf(" Interrupt: %s", gpio_config.ie > 0?"Enabled, ":"Disabled,");
    printf(" Trigger: %s", gpio_config.edge?"Level ":"Edge ");
    if (gpio_config.trig == 0x0) {
      printf("Trigger,  Edge: %s\n", "Falling Edge");
    } else if (gpio_config.trig == 0x1) {
      printf("Trigger,  Edge: %s\n", "Rising Edge");
    } else if (gpio_config.trig == 0x2) {
      printf("Trigger,  Edge: %s\n", "Both Edges");
    } else  {
      printf("Trigger, Edge: %s\n", "Reserved");
    }
  }

  return 0;
}

static int
util_set_gpio_config(uint8_t slot_id, uint8_t gpio_num, uint8_t config_val, uint8_t intf) {
  int ret = 0;
  uint8_t gpio_pin_cnt = fby3_get_gpio_list_size(intf);
  char gpio_pin_name[32] = "\0";

  if ( gpio_num >= gpio_pin_cnt ) {
    printf("slot %d: Invalid GPIO pin number %d\n", slot_id, gpio_num);
    return ret;
  }

  fby3_get_gpio_name(slot_id, gpio_num, gpio_pin_name, intf);
  printf("slot %d: setting GPIO [%d]%s config to 0x%02x\n", slot_id, gpio_num, gpio_pin_name, config_val);
  if ( intf != NONE_INTF ) {
    ret = remote_bic_set_gpio_config(slot_id, gpio_num, config_val,intf);
  } else {
    ret = bic_set_gpio_config(slot_id, gpio_num, config_val);
  }
  if (ret < 0) {
    printf("%s() bic_set_gpio_config returns %d\n", __func__, ret);
  }

  return 0;
}

static int
util_perf_test(uint8_t slot_id, int loopCount, uint8_t intf) {
#define NUM_SLOTS FRU_SLOT4
  enum cmd_profile_type {
    CMD_AVG_DURATION = 0x0,
    CMD_MIN_DURATION,
    CMD_MAX_DURATION,
    CMD_NUM_SAMPLES,
    CMD_PROFILE_NUM
  };

  long double cmd_profile[NUM_SLOTS][CMD_PROFILE_NUM] = {0};
  int ret = 0;
  ipmi_dev_id_t id = {0};
  int i = 0;
  int index = slot_id -1;
  long double elapsedTime = 0;

  cmd_profile[index][CMD_MIN_DURATION] = 3000000;
  cmd_profile[index][CMD_MAX_DURATION] = 0;
  cmd_profile[index][CMD_NUM_SAMPLES]  = 0;
  cmd_profile[index][CMD_AVG_DURATION] = 0;

  printf("bic-util: perf test on slot%d for ", slot_id);
  if (loopCount)
    printf("%d cycles\n", loopCount);
  else
    printf("infinite cycles\n");

  struct timeval  tv1, tv2;
  while(1) {
    gettimeofday(&tv1, NULL);

    ret = bic_get_dev_id(slot_id, &id, intf);
    if (ret) {
      printf("util_perf_test: bic_get_dev_id returns %d, loop=%d\n", ret, i);
      return ret;
    }

    gettimeofday(&tv2, NULL);

    elapsedTime = (((long double)(tv2.tv_usec - tv1.tv_usec) / 1000000 +
                   (long double)(tv2.tv_sec - tv1.tv_sec)) * 1000000);

    cmd_profile[index][CMD_AVG_DURATION] += elapsedTime;
    cmd_profile[index][CMD_NUM_SAMPLES] += 1;
    if (cmd_profile[index][CMD_MAX_DURATION] < elapsedTime)
      cmd_profile[index][CMD_MAX_DURATION] = elapsedTime;

    if (cmd_profile[index][CMD_MIN_DURATION] > elapsedTime && elapsedTime > 0)
      cmd_profile[index][CMD_MIN_DURATION] = elapsedTime;

    ++i;

    if ((i & 0xfff) == 0) {
      printf("slot%d stats: loop %d num cmds=%d, avg duration(us)=%d, min duration(us)=%d, max duration(us)=%d\n",
            slot_id, i,
            (int)cmd_profile[index][CMD_NUM_SAMPLES],
            (int)(cmd_profile[index][CMD_AVG_DURATION]/cmd_profile[index][CMD_NUM_SAMPLES]),
            (int)cmd_profile[index][CMD_MIN_DURATION],
            (int)cmd_profile[index][CMD_MAX_DURATION]
          );
    }

    if (loopCount != 0  && i==loopCount) {
      printf("slot%d stats after loop %d\n num cmds=%d, avg duration(us)=%d, min duration(us)=%d, max duration(us)=%d\n",
            slot_id, i,
            (int)cmd_profile[index][CMD_NUM_SAMPLES],
            (int)(cmd_profile[index][CMD_AVG_DURATION]/cmd_profile[index][CMD_NUM_SAMPLES]),
            (int)cmd_profile[index][CMD_MIN_DURATION],
            (int)cmd_profile[index][CMD_MAX_DURATION]
          );
      break;
    }
  }

  return ret;
}

static int
util_read_sensor(uint8_t slot_id) {
  int ret = 0;
  int i = 0;
  ipmi_sensor_reading_t sensor = {0};
  uint8_t intf_list[4] = {NONE_INTF};
  uint8_t intf_index = 0;
  uint8_t config_status = 0xff;
  uint8_t type_2ou = UNKNOWN_BOARD;

  ret = bic_is_m2_exp_prsnt(slot_id);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the status of 1OU/2OU\n", __func__);
    return -1;
  }
  config_status = (uint8_t) ret;

  if ( (config_status & PRESENT_1OU) == PRESENT_1OU && (bmc_location != NIC_BMC) ) {
    intf_list[1] = FEXP_BIC_INTF;
  } else if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
    if ( fby3_common_get_2ou_board_type(slot_id, &type_2ou) < 0) {
      printf("%s() Couldn't get 2OU board type\n", __func__);
      return -1;
    }

    switch (type_2ou) {
    case DP_RISER_BOARD:
      break;
    default:
      intf_list[2] = REXP_BIC_INTF;
      break;
    }
  }

  if ( bmc_location == NIC_BMC ) {
    intf_list[3] = BB_BIC_INTF;
  }

  for ( intf_index = 0; intf_index < intf_size; intf_index++ ) {
    if ( intf_list[intf_index] == 0 ) {
      continue;
    }

    printf("%s:\n", intf_name[intf_index]);

    //TODO: If a sensor is not existed, a warning message is logged in /var/log/messages
    for (i = 0; i < MAX_SENSOR_NUM; i++) {
      ret = bic_get_sensor_reading(slot_id, i, &sensor, intf_list[intf_index]);
      if (ret < 0 ) {
        continue;
      }

      printf("sensor num: 0x%02X: value: 0x%02X, flags: 0x%02X, status: 0x%02X, ext_status: 0x%02X\n",
              i, sensor.value, sensor.flags, sensor.status, sensor.ext_status);
    }
    printf("\n");
  }
  return ret;
}

static int
util_get_sdr(uint8_t slot_id) {
#define LAST_RECORD_ID 0xFFFF
#define BYTES_ENTIRE_RECORD 0xFF
  int ret = 0;
  uint8_t rlen = 0;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};
  uint8_t intf_list[4] = {NONE_INTF};
  uint8_t intf_index = 0;
  uint8_t config_status = 0xff;
  uint8_t type_2ou = UNKNOWN_BOARD;

  ipmi_sel_sdr_req_t req;
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  ret = bic_is_m2_exp_prsnt(slot_id);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the status of 1OU/2OU\n", __func__);
    return -1;
  }
  config_status = (uint8_t) ret;

  if ( (config_status & PRESENT_1OU) == PRESENT_1OU && (bmc_location != NIC_BMC) ) {
    intf_list[1] = FEXP_BIC_INTF;
  } else if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
    if ( fby3_common_get_2ou_board_type(slot_id, &type_2ou) < 0) {
      printf("%s() Couldn't get 2OU board type\n", __func__);
      return -1;
    }

    switch (type_2ou) {
    case DP_RISER_BOARD:
      break;
    default:
      intf_list[2] = REXP_BIC_INTF;
      break;
    }
  }

  if ( bmc_location == NIC_BMC ) {
    intf_list[3] = BB_BIC_INTF;
  }

  for( intf_index = 0; intf_index < intf_size; intf_index++ ) {
    if ( intf_list[intf_index] == 0 ) {
      continue;
    }

    printf("%s:\n", intf_name[intf_index]);
    req.rsv_id = 0;
    req.rec_id = 0;
    req.offset = 0;
    req.nbytes = BYTES_ENTIRE_RECORD;
    for ( ;req.rec_id != LAST_RECORD_ID; ) {
      ret = bic_get_sdr(slot_id, &req, res, &rlen, intf_list[intf_index]);
      if (ret) {
        printf("util_get_sdr:bic_get_sdr returns %d\n", ret);
        continue;
      }

      sdr_full_t *sdr = (sdr_full_t *)res->data;

      printf("type: %d, ", sdr->type);
      printf("sensor num: 0x%02X, ", sdr->sensor_num);
      printf("type: 0x%02X, ", sdr->sensor_type);
      printf("evt_read_type: 0x%02X, ", sdr->evt_read_type);
      printf("m_val: 0x%02X, ", sdr->m_val);
      printf("m_tol: 0x%02X, ", sdr->m_tolerance);
      printf("b_val: 0x%02X, ", sdr->b_val);
      printf("b_acc: 0x%02X, ", sdr->b_accuracy);
      printf("acc_dir: 0x%02X, ", sdr->accuracy_dir);
      printf("rb_exp: 0x%02X,\n", sdr->rb_exp);

      req.rec_id = res->next_rec_id;
    }
    printf("This record is LAST record\n");
    printf("\n");
  }
  return ret;
}

static int
util_get_postcode(uint8_t slot_id) {
  int ret = 0;
  uint8_t buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t len = 0;
  int i = 0;

  ret = bic_get_80port_record(slot_id, buf, &len, NONE_INTF);
  if ( ret < 0 ) {
    printf("%s: Failed to get POST code of slot%d\n", __func__, slot_id);
    return ret;
  }

  printf("%s: returns %d bytes\n", __func__, len);
  for (i = 0; i < len; i++) {
    if (!(i % 16) && i)
      printf("\n");

    printf("%02X ", buf[i]);
  }
  printf("\n");
  return ret;
}

static int
util_bic_clear_cmos(uint8_t slot_id) {
  int ret = 0;
  uint8_t power_status = 0;

  ret = pal_get_server_power(slot_id, &power_status);
  if (ret < 0) {
    printf("Failed to get server power status\n");
    goto out;
  } else if (power_status == SERVER_POWER_ON) {
    printf("Can't performing CMOS clear while server is powered ON\n");
    goto out;
  }

  ret = bic_clear_cmos(slot_id);
  printf("Performing CMOS clear, status %d\n", ret);
  ret = pal_set_nic_perst(slot_id, NIC_PE_RST_HIGH);
  if (ret < 0) {
    printf("Failed to set nic card PERST high\n");
  }
out:
  return ret;
}

int
bic_comp_init_usb_dev(uint8_t slot_id, usb_dev* udev, char *comp)
{
#define TI_VENDOR_ID_SB  0x1CBE
#define TI_VENDOR_ID_1U  0x1CBF
#define TI_VENDOR_ID_2U  0x1CC0
#define TI_PRODUCT_ID 0x0007
  int ret;
  int index = 0;
  char found = 0;
  ssize_t cnt;
  uint8_t bmc_location = 0;
  uint16_t vendor_id = TI_VENDOR_ID_SB;
  int recheck = MAX_CHECK_DEVICE_TIME;

  ret = libusb_init(NULL);
  if (ret < 0) {
    printf("Failed to initialise libusb\n");
    goto error_exit;
  } else {
    printf("Init libusb Successful!\n");
  }

  if (strcmp("sb", comp) == 0) {
    vendor_id = TI_VENDOR_ID_SB;
  } else if (strcmp("1ou", comp)== 0) {
    vendor_id = TI_VENDOR_ID_1U;
  } else if (strcmp("2ou", comp)== 0) {
    vendor_id = TI_VENDOR_ID_2U;
  }

  do {
    cnt = libusb_get_device_list(NULL, &udev->devs);
    if (cnt < 0) {
      printf("There are no USB devices on bus\n");
      goto error_exit;
    }
    index = 0;
    while ((udev->dev = udev->devs[index++]) != NULL) {
      ret = libusb_get_device_descriptor(udev->dev, &udev->desc);
      if ( ret < 0 ) {
        printf("Failed to get device descriptor -- exit\n");
        libusb_free_device_list(udev->devs,1);
        goto error_exit;
      }

      ret = libusb_open(udev->dev,&udev->handle);
      if ( ret < 0 ) {
        printf("Error opening device -- exit\n");
        libusb_free_device_list(udev->devs,1);
        goto error_exit;
      }

      if( (vendor_id == udev->desc.idVendor) && (TI_PRODUCT_ID == udev->desc.idProduct) ) {
        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iManufacturer, (unsigned char*) udev->manufacturer, sizeof(udev->manufacturer));
        if ( ret < 0 ) {
          printf("Error get Manufacturer string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        ret = libusb_get_port_numbers(udev->dev, udev->path, sizeof(udev->path));
        if (ret < 0) {
          printf("Error get port number\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        if ( (bmc_location == BB_BMC) || (bmc_location == DVT_BB_BMC) ) {
          if ( udev->path[1] != slot_id) {
            continue;
          }
        }
        printf("%04x:%04x (bus %d, device %d)",udev->desc.idVendor, udev->desc.idProduct, libusb_get_bus_number(udev->dev), libusb_get_device_address(udev->dev));
        printf(" path: %d", udev->path[0]);
        for (index = 1; index < ret; index++) {
          printf(".%d", udev->path[index]);
        }
        printf("\n");

        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iProduct, (unsigned char*) udev->product, sizeof(udev->product));
        if ( ret < 0 ) {
          printf("Error get Product string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        printf("Manufactured : %s\n",udev->manufacturer);
        printf("Product : %s\n",udev->product);
        printf("----------------------------------------\n");
        printf("Device Descriptors:\n");
        printf("Vendor ID : %x\n",udev->desc.idVendor);
        printf("Product ID : %x\n",udev->desc.idProduct);
        printf("Serial Number : %x\n",udev->desc.iSerialNumber);
        printf("Size of Device Descriptor : %d\n",udev->desc.bLength);
        printf("Type of Descriptor : %d\n",udev->desc.bDescriptorType);
        printf("USB Specification Release Number : %d\n",udev->desc.bcdUSB);
        printf("Device Release Number : %d\n",udev->desc.bcdDevice);
        printf("Device Class : %d\n",udev->desc.bDeviceClass);
        printf("Device Sub-Class : %d\n",udev->desc.bDeviceSubClass);
        printf("Device Protocol : %d\n",udev->desc.bDeviceProtocol);
        printf("Max. Packet Size : %d\n",udev->desc.bMaxPacketSize0);
        printf("No. of Configuraions : %d\n",udev->desc.bNumConfigurations);

        found = 1;
        break;
      }
    }

    if ( found != 1) {
      sleep(3);
    } else {
      break;
    }
  } while ((--recheck) > 0);


  if ( found == 0 ) {
    printf("Device NOT found -- exit\n");
    libusb_free_device_list(udev->devs,1);
    ret = -1;
    goto error_exit;
  }

  ret = libusb_get_configuration(udev->handle, &udev->config);
  if ( ret != 0 ) {
    printf("Error in libusb_get_configuration -- exit\n");
    libusb_free_device_list(udev->devs,1);
    goto error_exit;
  }

  printf("Configured value : %d\n", udev->config);
  if ( udev->config != 1 ) {
    libusb_set_configuration(udev->handle, 1);
    if ( ret != 0 ) {
      printf("Error in libusb_set_configuration -- exit\n");
      libusb_free_device_list(udev->devs,1);
      goto error_exit;
    }
    printf("Device is in configured state!\n");
  }

  libusb_free_device_list(udev->devs, 1);

  if(libusb_kernel_driver_active(udev->handle, udev->ci) == 1) {
    printf("Kernel Driver Active\n");
    if(libusb_detach_kernel_driver(udev->handle, udev->ci) == 0) {
      printf("Kernel Driver Detached!");
    } else {
      printf("Couldn't detach kernel driver -- exit\n");
      libusb_free_device_list(udev->devs,1);
      goto error_exit;
    }
  }

  ret = libusb_claim_interface(udev->handle, udev->ci);
  if ( ret < 0 ) {
    printf("Couldn't claim interface -- exit. err:%s\n", libusb_error_name(ret));
    libusb_free_device_list(udev->devs,1);
    goto error_exit;
  }
  printf("Claimed Interface: %d, EP addr: 0x%02X\n", udev->ci, udev->epaddr);

  active_config(udev->dev, udev->handle);
  return 0;
error_exit:
  return -1;
}



static int
util_get_board_revision(uint8_t slot_id, char *comp) {
  int ret = 0;
  int retry = 0;
  uint8_t type_1ou = 0;
  uint8_t tbuf[5] = {0x9C, 0x9C, 0x00, 0x00, 0x00};
  uint8_t rbuf[256] = {0};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  uint8_t board_rev_id0 = 0, board_rev_id1 = 0, board_rev_id2 = 0;

  if (strcmp("1ou", comp) == 0) {
    ret = bic_is_m2_exp_prsnt_cache(slot_id);
    if ( ret < 0 ) {
      printf("Couldn't read bic_is_m2_exp_prsnt_cache\n");
      return -1;
    }

    if ( (ret & PRESENT_1OU) != PRESENT_1OU ) {
      printf("1OU board is not present\n");
      return -1;
    }

    if ( bic_get_1ou_type(slot_id, &type_1ou) < 0 || type_1ou != EDSFF_1U ) {
      printf("get_board_revision only support in E1S 1OU board, board type %02X (Expected: %02X)\n", type_1ou, EDSFF_1U);
      return -1;
    }

    tbuf[4] = 50; //FW_BOARD_REV_ID0
    do {
      ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SINGLE_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen, FEXP_BIC_INTF);
    } while (ret && (retry++ <= 3));
    if (ret) {
      printf("get BOARD_REV_ID0 fail, retry 3 times\n");
      return -1;
    }
    board_rev_id0 = rbuf[3] ? 1 : 0;
    printf("BOARD_REV_ID0: %d\n", board_rev_id0);

    tbuf[4] = 51; //FW_BOARD_REV_ID1
    retry = 0;
    do {
      ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SINGLE_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen, FEXP_BIC_INTF);
    } while (ret && (retry++ <= 3));
    if (ret) {
      printf("get BOARD_REV_ID1 fail, retry 3 times\n");
      return -1;
    }
    board_rev_id1 = rbuf[3] ? 1 : 0;
    printf("BOARD_REV_ID1: %d\n", board_rev_id1);

    tbuf[4] = 73; //FW_BOARD_REV_ID2
    retry = 0;
    do {
      ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_SINGLE_GPIO_CONFIG, tbuf, tlen, rbuf, &rlen, FEXP_BIC_INTF);
    } while (ret && (retry++ <= 3));
    if (ret) {
      printf("get BOARD_REV_ID2 fail, retry 3 times\n");
      return -1;
    }
    board_rev_id2 = rbuf[3] ? 1 : 0;
    printf("BOARD_REV_ID2: %d\n", board_rev_id2);

    if ( board_rev_id0 && board_rev_id1 ) {
      printf("Efuse: Maxim, ");
    } else {
      printf("Efuse: TI, ");
    }

    if ( board_rev_id2 ) {
      printf("Clock buffer: IDT1 \n");
    } else {
      printf("Clock buffer: IDT2 \n");
    }
  }
  return 0;
}

static int
util_check_usb_port(uint8_t slot_id, char *comp) {
  int ret = -1;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;
  int transferlen = 64;
  int transferred = 0;
  uint8_t tbuf[64] = {0};
  int retries = 3;

  bic_set_gpio(slot_id, RST_USB_HUB_N, GPIO_HIGH);

  udev->ci = 1;
  udev->epaddr = 0x1;

  // init usb device
  ret = bic_comp_init_usb_dev(slot_id, udev, comp);
  if (ret < 0) {
    return ret;
  }

  printf("Input test data, USB timeout: 3000ms\n");
  while(true)
  {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, tbuf, transferlen, &transferred, 3000);
    if(((ret != 0) || (transferlen != transferred))) {
      printf("Error in transferring data! err = %d and transferred = %d(expected data length 64)\n",ret ,transferred);
      printf("Retry since  %s\n", libusb_error_name(ret));
      retries--;
      if (!retries) {
        ret = -1;
        break;
      }
      msleep(100);
    } else
      break;
  }

  if (ret != 0) {
    printf("Check USB port Failed\n");
  } else {
    printf("Check USB port Successful\n");
  }

  bic_set_gpio(slot_id, RST_USB_HUB_N, GPIO_LOW);

  printf("\n");
  return ret;
}

static int
util_get_slot_id() {
  uint8_t index = 0;

  if (bic_get_mb_index(&index) != 0) {
    return -1;
  }

  printf("Slot ID = %d\n", index);

  return 0;
}

static int
set_single_usb_hub(uint8_t slot, uint8_t nb, uint8_t ctl, uint8_t intf) {
  int ret = 0;
  if (ctl == 2) { //reset
    ret = remote_bic_set_gpio(slot, nb, 0, intf);
    if (ret == 0) {
      sleep(3); //for usb devices to be released
      ret = remote_bic_set_gpio(slot, nb, 1, intf);
    } 
  } else if (ctl == 0 || ctl == 1) {
    ret = remote_bic_set_gpio(slot, nb, ctl, intf);
  } else {
    return -1;
  }
  return ret;
}

static int
set_usb_hub_mux(uint8_t slot, uint8_t nb, uint8_t ctl, uint8_t intf) {
  uint8_t mux = 0;
  if (ctl == 10) {  //to bmc
    mux = 0;
  } else if (ctl == 11) { //to usb c
    mux = 1;
  } else {
    return -1;
  }
  return remote_bic_set_gpio(slot, nb, mux, intf);
}

static int
util_set_usb_hub(uint8_t hub, uint8_t on_off, uint8_t fru, uint8_t intf) {
  int ret = 0;
  switch (fru) {
    case FRU_CWC:
      if (hub == 0xff || hub == 0) {
        if (on_off == 10 || on_off == 11) {
          ret = set_usb_hub_mux(FRU_SLOT1, gpio_cwc_usbmux[0], on_off, intf);
        } else {
          ret = set_single_usb_hub(FRU_SLOT1, gpio_cwc_usbhub[0], on_off, intf);
        }
      } else {
        printf("Wrong usb hub number!\n");
        return -1;
      }
      break;
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      if (hub == 0xff) {  //all
        ret = set_single_usb_hub(FRU_SLOT1, gpio_gpv3_usbhub[1], on_off, intf);
        ret = set_single_usb_hub(FRU_SLOT1, gpio_gpv3_usbhub[0], on_off, intf);
        ret = set_single_usb_hub(FRU_SLOT1, gpio_gpv3_usbhub[2], on_off, intf);
      } else if (hub <= 2) {
        ret = set_single_usb_hub(FRU_SLOT1, gpio_gpv3_usbhub[hub], on_off, intf);
      } else {
        printf("Wrong usb hub number!\n");
        return -1;
      }
      break;
    case FRU_SLOT1:
      if (hub == 0xff || hub == 0) {
        ret = set_single_usb_hub(FRU_SLOT1, gpio_sb_usbhub[0], on_off, intf);
      } else {
        printf("Wrong usb hub number!\n");
        return -1;
      }
      break;
    default:
      printf("Unknown fru!\n");
      ret = -1;
  }

  if (ret) {
    printf("Fail to set usb hub status\n");
  }

  return ret;
}

static int
print_usb_hub_status(bic_gpio_t gpio, const uint8_t *gpio_nb, uint8_t gpio_len, uint8_t mux) {
  uint8_t i = 0, nb = 0;
  uint32_t offset = 0, mask = 0;
  for (i = 0; i < gpio_len; ++i) {
    nb = gpio_nb[i];
    offset = 0;
    mask = 0;

    while (nb >= 32) {
      nb -= 32;
      offset++;
    }
    mask = 1 << nb;

    if (gpio.gpio[offset] & mask) {
      if (gpio_nb[i] == mux) {
        printf("USB HUB is switched to usb c\n");
      } else if (gpio_len == 1) {
        printf("USB HUB is on\n");
      } else {
        printf("USB HUB%d is on\n", i);
      }
    } else {
      if (gpio_nb[i] == mux) {
        printf("USB HUB is switched to bmc\n");
      } else if (gpio_len == 1) {
        printf("USB HUB is off\n");
      } else {
        printf("USB HUB%d is off\n", i);
      }
    }
  }

  return 0;
}

static int
util_get_usb_hub(uint8_t hub, uint8_t fru, uint8_t intf) {
  int ret = 0;
  bic_gpio_t gpio = {0};
  uint8_t len = 0, mux = 0xff;
  uint8_t cwc_gpio[2] = {0};
  const uint8_t *start = NULL;
  switch (fru) {
    case FRU_CWC:
      if (hub > 0 && hub != 0xff) {
        printf("Wrong usb hub number!\n");
        return -1;
      }
      len = 2;
      cwc_gpio[0] = gpio_cwc_usbhub[0];
      cwc_gpio[1] = gpio_cwc_usbmux[0];
      mux = gpio_cwc_usbmux[0];
      start = cwc_gpio;
      ret = bic_get_gpio(FRU_SLOT1, &gpio, intf);
      break;
    case FRU_2U_TOP:
    case FRU_2U_BOT:
      if (hub == 0xff) {
        len = 3;
        start = &gpio_gpv3_usbhub[0];
      } else if (hub <= 2) {
        len = 1;
        start = &gpio_gpv3_usbhub[hub];
      } else {
        printf("Wrong usb hub number!\n");
        return -1;
      }
      ret = bic_get_gpio(FRU_SLOT1, &gpio, intf);
      break;
    case FRU_SLOT1:
      if (hub > 0 && hub != 0xff) {
        printf("Wrong usb hub number!\n");
        return -1;
      }
      len = 1;
      start = &gpio_sb_usbhub[0];
      ret = bic_get_gpio(FRU_SLOT1, &gpio, intf);
      break;
    default:
      printf("Unknown fru!\n");
      ret = -1;
  }

  if (ret == 0) {
    print_usb_hub_status(gpio, start, len, mux);
  } else {
    printf("Fail to get usb hub status\n");
  }

  return ret;
}

static int
util_get_pci_link(uint8_t fru) {
  // the bit mapping table for singel/dual M2
  uint8_t sgl_m2_tbl[]  = {5, 4, 15, 14, 13, 12, 8, 9, 11, 10, 2, 3, 1, 0};
  uint8_t dual_m2_tbl[] = {4, 14, 12, 8, 10, 2, 1, 0};
  uint8_t intf_list[3] = {REXP_BIC_INTF, RREXP_BIC_INTF1, RREXP_BIC_INTF2};
  size_t intf_end = 3;
  size_t intf_st = 0;
  int ret = -1;
  enum {
    GPV3_84CH_DUAL  = 0x4,
    GPV3_100CH_DUAL = 0x6,
  };

  if ( riser_board == GPV3_MCHP_BOARD ) {
    intf_st = 0;
    intf_end = 1;
  } else if ( riser_board == CWC_MCHP_BOARD ) {
    intf_st = 1;
    intf_end = 3;
  } else {
    printf("The system may not support the feature\n");
    return -1;
  }

  for ( size_t i = intf_st; i < intf_end; i++ ) {
    // start getting data
    uint8_t rbuf[8] = {0};
    uint8_t rlen = 0;
    ret = bic_get_gpv3_pci_link(fru, rbuf, &rlen, intf_list[i]);
    if ( ret < 0 ) {
      printf("Failed to get PCI link status\n");
      break;
    }

    uint16_t result = (rbuf[3] << 8 | rbuf[4]);
    uint8_t m2_cfg = pal_get_gpv3_cfg();
    uint8_t *tbl_p = sgl_m2_tbl;
    size_t tbl_size = sizeof(sgl_m2_tbl);
    bool is_dual = false;
    // print data
    if ( m2_cfg == GPV3_84CH_DUAL || m2_cfg == GPV3_100CH_DUAL ) {
      tbl_size = sizeof(dual_m2_tbl);
      is_dual = true;
      tbl_p = dual_m2_tbl;
    }

    uint8_t e1s_offset = tbl_size - 2;
    if ( intf_list[i] == REXP_BIC_INTF ) printf("2U GPv3:\n");
    else if ( intf_list[i] == RREXP_BIC_INTF1 ) printf("4U TOP GPv3:\n");
    else printf("4U BOT GPv3:\n");

    for ( size_t j = 0 ; j < tbl_size; j++ ) {
      char *link_status = ((result >> tbl_p[j]) & 0x1)?"Up":"Down";
      if ( j >= e1s_offset ) printf("E1S_%d Link status: %s\n", (j - e1s_offset), link_status);
      else {
        char *m2_type = (is_dual == true)?"Dual":"Single";
        uint8_t idx = (is_dual == true)?((j*2)+1):j;
        printf("%s M2_%d Link status: %s\n", m2_type, idx, link_status);
      }
    }
    printf("\n");
  }

  return ret;
}

int
main(int argc, char **argv) {
  uint8_t slot_id = 0;
  uint8_t is_fru_present = 0;
  int ret = 0;
  uint8_t gpio_num = 0;
  uint8_t gpio_val = 0;
  uint8_t intf = NONE_INTF;
  int i = 0;
  uint8_t hub = 0, ctl = 0;

  if (argc < 3) {
    goto err_exit;
  }

  // because the usage of bic-util is displayed based on the platform
  // we should get the information first
  ret = fby3_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the location of BMC\n", __func__);
    return -1;
  }

  // check the expansion
  if ( bmc_location == NIC_BMC ) {
    ret = bic_is_m2_exp_prsnt(FRU_SLOT1);
    if ( ret < 0 ) {
      printf("Failed to run bic_is_m2_exp_prsnt()\n");
      return -1;
    }

    // check if the exp presence
    if ( (ret & PRESENT_2OU) == PRESENT_2OU ) {
      ret = fby3_common_get_2ou_board_type(FRU_SLOT1, &riser_board);
      if ( ret < 0 ) {
        printf("Failed to run fby3_common_get_2ou_board_type()\n");
        return -1;
      }
    }

    ret = fby3_common_get_slot_id(argv[1], &slot_id);
    if ( ret < 0 ) {
      if ( (riser_board == CWC_MCHP_BOARD || riser_board == GPV3_MCHP_BOARD || riser_board == GPV3_BRCM_BOARD) && 
          pal_get_fru_id(argv[1], &expFru) ) {
        printf("%s is invalid!\n", argv[1]);
        goto err_exit;
      }
    }

  } else {
    ret = fby3_common_get_slot_id(argv[1], &slot_id);
    if ( ret < 0 ) {
      char exp_slot[16] = {0};
      char *str;
      strncpy(exp_slot, argv[1], sizeof(exp_slot)-1);
      str = strtok(exp_slot,"-");
      ret = fby3_common_get_slot_id(str, &slot_id);
      if ( ret < 0 ) {
        printf("%s is invalid!\n", str);
        goto err_exit;
      }

      // Check Config D GPv3
      ret = bic_is_m2_exp_prsnt(slot_id);
      if ( ret < 0 ) {
        printf("Failed to run bic_is_m2_exp_prsnt()\n");
        return -1;
      }

      // check if the exp presence
      if ( (ret & PRESENT_2OU) == PRESENT_2OU ) {
        ret = fby3_common_get_2ou_board_type(slot_id, &riser_board);
        if ( ret < 0 ) {
          printf("Failed to run fby3_common_get_2ou_board_type()\n");
          return -1;
        }
      }

      if ( (riser_board == GPV3_MCHP_BOARD || riser_board == GPV3_BRCM_BOARD) &&
          pal_get_fru_id(argv[1], &expFru) ) {
        printf("%s is invalid!\n", argv[1]);
        goto err_exit;
      }
    }
  }

  if (expFru != 0xff && pal_get_fru_slot(expFru, &slot_id)) {
    printf("Fail to get fru:%d slot id!\n", expFru);
    goto err_exit;
  }

  if ( slot_id != FRU_ALL ) {
    if ( bmc_location == NIC_BMC && slot_id != FRU_SLOT1 ) {
      printf("slot%d is not supported!\n", slot_id);
      return -1;
    }

    ret = pal_is_fru_prsnt(slot_id, &is_fru_present);
    if ( ret < 0 || is_fru_present == 0 ) {
      printf("%s is not present!\n", argv[1]);
      return -1;
    }
  }

  if ( riser_board == CWC_MCHP_BOARD ) {
    switch (expFru) {
      case FRU_CWC:
        intf = REXP_BIC_INTF;
        break;
      case FRU_2U_TOP:
        intf = RREXP_BIC_INTF1;
        break;
      case FRU_2U_BOT:
        intf = RREXP_BIC_INTF2;
        break;
    }

    if ( intf != NONE_INTF ) {
      ret = pal_is_fru_prsnt(expFru, &is_fru_present);
      if ( ret < 0 || is_fru_present == 0 ) {
        printf("exp:%d is not present!\n", expFru);
        return -1;
      }
    }
  } else {
    // get the argv_idx and intf
    if (expFru == FRU_2U || expFru == FRU_2U_SLOT3) {
      intf = REXP_BIC_INTF;
    }

    // check if 1/2OU present
    if ( intf != NONE_INTF ) {
      ret = bic_is_m2_exp_prsnt_cache(slot_id);
      if ( ret < 0 ) {
        printf("Couldn't read bic_is_m2_exp_prsnt_cache\n");
        return BIC_STATUS_FAILURE;
      }

      //return if the 1ou/2ou is not present
      if ( intf == FEXP_BIC_INTF ) {
        if ( (bmc_location == NIC_BMC) || (ret & PRESENT_1OU) != PRESENT_1OU ) {
          printf("1OU is not present\n");
          return BIC_STATUS_FAILURE;
        }
      } else {
        if ( (ret & PRESENT_2OU) != PRESENT_2OU ) {
          printf("2OU is not present\n");
          return BIC_STATUS_FAILURE;
        }
      }
    }
  }

  if ( strncmp(argv[2], "--", 2) == 0 ) {
    if ( strcmp(argv[2], "--get_gpio") == 0 ) {
      if ( argc != 3 ) goto err_exit;
      return util_get_gpio(slot_id, intf);
    } else if ( strcmp(argv[2], "--set_gpio") == 0 ) {
      if ( argc != 5 ) goto err_exit;

      gpio_num = atoi(argv[3]);
      gpio_val = atoi(argv[4]);
      if ( gpio_num > 0xff || gpio_val > 1 ) goto err_exit;
      return util_set_gpio(slot_id, gpio_num, gpio_val, intf);
    } else if ( strcmp(argv[2], "--get_gpio_config") == 0 ) {
      if ( argc != 3 ) goto err_exit;
      return util_get_gpio_config(slot_id, intf);
    } else if ( strcmp(argv[2], "--set_gpio_config") == 0 ) {
      if ( argc != 5 ) goto err_exit;

      gpio_num = atoi(argv[3]);
      gpio_val = (uint8_t)strtol(argv[4], NULL, 0);
      if ( gpio_num > 0xff ) goto err_exit;
      else return util_set_gpio_config(slot_id, gpio_num, gpio_val, intf);
    } else if ( strcmp(argv[2], "--check_status") == 0 ) {
      if ( argc != 3 ) goto err_exit;
      return util_check_status(slot_id);
    } else if ( strcmp(argv[2], "--get_dev_id") == 0 ) {
      if ( argc != 3 ) goto err_exit;
      return util_get_device_id(slot_id, intf);
    } else if ( strcmp(argv[2], "--get_sdr") == 0 ) {
      if ( argc != 3 ) goto err_exit;
      return util_get_sdr(slot_id);
    } else if ( strcmp(argv[2], "--read_sensor") == 0 ) {
      if ( argc != 3 ) goto err_exit;
      return util_read_sensor(slot_id);
    } else if ( strcmp(argv[2], "--reset") == 0 ) {
      if ( argc != 3 ) goto err_exit;
      return util_bic_reset(slot_id, intf);
    } else if ( strcmp(argv[2], "--get_post_code") == 0 ) {
      if ( argc != 3 ) goto err_exit;
      return util_get_postcode(slot_id);
    } else if ( strcmp(argv[2], "--perf_test") == 0 ) {
      if ( argc != 4 ) goto err_exit;
      else return util_perf_test(slot_id, atoi(argv[3]), intf);
    } else if (!strcmp(argv[2], "--clear_cmos")) {
      if (argc != 3) goto err_exit;
      return util_bic_clear_cmos(slot_id);
    } else if ( strcmp(argv[2], "--file") == 0 ) {
      if ( argc != 4 ) goto err_exit;
      if ( slot_id == FRU_ALL ) {
        if ( bmc_location != NIC_BMC ) {
          for ( i = FRU_SLOT1; i <= FRU_SLOT4; i++ ) {
            ret = pal_is_fru_prsnt(i, &is_fru_present);
            if ( ret < 0 || is_fru_present == 0 ) {
              printf("slot%d is not present!\n", i);
            } else process_file(i, argv[3]);
          }
        } else process_file(FRU_SLOT1, argv[3]);
        return 0;
      } else return process_file(slot_id, argv[3]);
    } else if ( strcmp(argv[2], "--check_usb_port") == 0 ) {
      if ( argc != 4 ) goto err_exit;
      if ( (strcmp("sb", argv[3]) != 0) && (strcmp("1ou", argv[3]) != 0) && (strcmp("2ou", argv[3]) != 0) ){
        printf("Invalid component: %s\n", argv[3]);
        goto err_exit;
      }
      return util_check_usb_port(slot_id, argv[3]);
    } else if ( strcmp(argv[2], "--get_slot_id") == 0 ) {
      if (bmc_location != NIC_BMC) {
        printf("Option not supported for this sku!\n");
        return -1;
      }

      if (argc != 3) {
        goto err_exit;
      }

      return util_get_slot_id();
    } else if ( strcmp(argv[2], "--get_board_revision") == 0 ) {
      if ( argc != 4 ) goto err_exit;
      if ( (strcmp("1ou", argv[3]) != 0) ) {
        printf("Invalid component: %s\n", argv[3]);
        goto err_exit;
      }
      return util_get_board_revision(slot_id, argv[3]);
    } else if ( strcmp(argv[2], "--get_pci_link") == 0 ) {
      return util_get_pci_link(slot_id);
    } else if ( strcmp(argv[2], "--set_usb_hub") == 0 ) {
      if ( riser_board == CWC_MCHP_BOARD ) {
        if ( argc == 5 ) {
          if ( strcmp(argv[3], "all") == 0 ) {
            hub = 0xff;
          } else {
            hub = atoi(argv[3]);
          }
          if ( strcmp(argv[4], "on") == 0 ) {
            ctl = 1;
          } else if ( strcmp(argv[4], "off") == 0 ) {
            ctl = 0;
          } else if ( strcmp(argv[4], "reset") == 0 ) {
            ctl = 2;
          } else if ( strcmp(argv[4], "to-bmc") == 0 ) {
            ctl = 10;
          } else if ( strcmp(argv[4], "to-usbc") == 0 ) {
            ctl = 11;
          } else {
            goto err_exit;
          }

          if (expFru == FRU_ALL) {
            util_set_usb_hub(hub, ctl, FRU_SLOT1, NONE_INTF);
            util_set_usb_hub(hub, ctl, FRU_CWC, REXP_BIC_INTF);
            if (pal_is_fru_prsnt(FRU_2U_TOP, &is_fru_present) == 0 && is_fru_present > 0) {
              util_set_usb_hub(hub, ctl, FRU_2U_TOP, RREXP_BIC_INTF1);
            } else {
              printf("Top gpv3 is not present!\n");
            }
            if (pal_is_fru_prsnt(FRU_2U_BOT, &is_fru_present) == 0 && is_fru_present > 0) {
              util_set_usb_hub(hub, ctl, FRU_2U_BOT, RREXP_BIC_INTF2);
            } else {
              printf("Bot gpv3 is not present!\n");
            }
            return 0;
          } else {
            return util_set_usb_hub(hub, ctl, expFru == 0xff ? slot_id : expFru, intf);
          }
        } else {
          goto err_exit;
        }
      } else {
        printf("Command not support!\n");
        goto err_exit;
      }
    } else if ( strcmp(argv[2], "--get_usb_hub") == 0 ) {
      if ( riser_board == CWC_MCHP_BOARD ) {
        if ( argc == 4 ) {
          if ( strcmp(argv[3], "all") == 0 ) {
            hub = 0xff;
          } else {
            hub = atoi(argv[3]);
          }

          if (expFru == FRU_ALL) {
            printf("Top GPV3:\n");
            if (pal_is_fru_prsnt(FRU_2U_TOP, &is_fru_present) == 0 && is_fru_present > 0) {
              util_get_usb_hub(hub, FRU_2U_TOP, RREXP_BIC_INTF1);
            } else {
              printf("Top gpv3 is not present!\n");
            }
            printf("Bot GPV3:\n");
            if (pal_is_fru_prsnt(FRU_2U_BOT, &is_fru_present) == 0 && is_fru_present > 0) {
              util_get_usb_hub(hub, FRU_2U_BOT, RREXP_BIC_INTF2);
            } else {
              printf("Bot gpv3 is not present!\n");
            }
            printf("CWC:\n");
            util_get_usb_hub(hub, FRU_CWC, REXP_BIC_INTF);
            printf("Slot1:\n");
            util_get_usb_hub(hub, FRU_SLOT1, NONE_INTF);
            return 0;
          } else {
            return util_get_usb_hub(hub, expFru == 0xff ? slot_id : expFru, intf);
          }
        } else {
          goto err_exit;
        }
      } else {
        printf("Command not support!\n");
        goto err_exit;
      }
    } else {
      printf("Invalid option: %s\n", argv[2]);
      goto err_exit;
    }
  } else if (argc >= 4) {
    if (util_is_numeric(argv + 2)) {
      return process_command(slot_id, (argc - 2), (argv + 2), intf);
    } else {
      goto err_exit;
    }
  } else {
    goto err_exit;
  }

err_exit:
  print_usage_help();
  return -1;
}
