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
#include <syslog.h>
#include <stdint.h>
#include <ctype.h>
#include <getopt.h>
#include <openbmc/ipmi.h>
#include <openbmc/pal.h>
#include <facebook/bic.h>
#include <facebook/bic_bios_fwupdate.h>
#include <facebook/fby35_common.h>
#include <facebook/fby35_gpio.h>
#include <sys/time.h>
#include <time.h>

enum {
  UTIL_GET_GPIO = 0x0,
  UTIL_SET_GPIO,
  UTIL_GET_VGPIO,
  UTIL_SET_VGPIO,
  UTIL_GET_DEV_ID,
  UTIL_RESET,
  UTIL_GET_POSTCODE,
  UTIL_GET_SDR,
  UTIL_READ_SENSOR,
  UTIL_CHECK_STS,
  UTIL_CLEAR_CMOS,
  UTIL_PERF_TEST,
  UTIL_CHECK_USB,
  UTIL_FILE,
};

static uint8_t bmc_location = 0xff;
static const char *intf_name[4] = {"Server Board", "Front Expansion Board", "Riser Expansion Board", "Baseboard"};
const uint8_t intf_size = 4;

static const char *option_list[] = {
  "--get_gpio",
  "--set_gpio [gpio_num] [value]",
  "--get_virtual_gpio",
  "--set_virtual_gpio [gpio_num] [value]",
  "--get_dev_id",
  "--reset",
  "--get_post_code",
  "--get_sdr",
  "--read_sensor",
  "--check_status",
  "--clear_cmos",
  "--perf_test [loop_count] (0 to run forever)",
  "--check_usb_port",
  "--file [path]",
};

static const struct option options[] = {
  {"get_gpio",         no_argument,       0, UTIL_GET_GPIO},
  {"set_gpio",         required_argument, 0, UTIL_SET_GPIO},
  {"get_virtual_gpio", no_argument,       0, UTIL_GET_VGPIO},
  {"set_virtual_gpio", required_argument, 0, UTIL_SET_VGPIO},
  {"get_dev_id",       no_argument,       0, UTIL_GET_DEV_ID},
  {"reset",            no_argument,       0, UTIL_RESET},
  {"get_post_code",    no_argument,       0, UTIL_GET_POSTCODE},
  {"get_sdr",          no_argument,       0, UTIL_GET_SDR},
  {"read_sensor",      no_argument,       0, UTIL_READ_SENSOR},
  {"check_status",     no_argument,       0, UTIL_CHECK_STS},
  {"clear_cmos",       no_argument,       0, UTIL_CLEAR_CMOS},
  {"perf_test",        required_argument, 0, UTIL_PERF_TEST},
  {"check_usb_port",   no_argument,       0, UTIL_CHECK_USB},
  {"file",             required_argument, 0, UTIL_FILE},
  {0, 0, 0, 0}
};

static void
print_usage_help(void) {
  size_t i;

  printf("Usage: bic-util <%s> <[0..n]data_bytes_to_send>\n", slot_usage);
  printf("Usage: bic-util <%s> <option>\n", slot_usage);
  printf("       option:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}

// Check BIC status
static int
util_check_status(uint8_t slot_id) {
  int ret = 0;
  uint8_t status;

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
    ret = bic_data_send(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen, intf);
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
process_file(uint8_t slot_id, char *path, uint8_t intf) {
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

    process_command(slot_id, argc, argv, intf);
  }
  fclose(fp);

  return 0;
}

static int
util_get_gpio(uint8_t slot_id, uint8_t intf) {
  int ret = 0;
  uint8_t i;
  uint8_t gpio_pin_cnt = y35_get_gpio_list_size(slot_id, false, intf);
  char gpio_pin_name[64] = {0};
  bic_gpio_t gpio = {0};

  if (gpio_pin_cnt == 0) {
    printf("%s() unsupported board type\n", __func__);
    return -1;
  }

  ret = bic_get_gpio(slot_id, &gpio, intf);
  if (ret < 0) {
    printf("%s() bic_get_gpio returns %d\n", __func__, ret);
    return ret;
  }

  // Print the gpio index, name and value
  for (i = 0; i < gpio_pin_cnt; i++) {
    y35_get_gpio_name(slot_id, i, gpio_pin_name, false, intf);
    printf("%d %s: %d\n",i , gpio_pin_name, BIT_VALUE(gpio, i));
  }

  return 0;
}

static int
util_set_gpio(uint8_t slot_id, uint8_t gpio_num, uint8_t gpio_val, uint8_t intf) {
  uint8_t gpio_pin_cnt = y35_get_gpio_list_size(slot_id, false, intf);
  char gpio_pin_name[64] = {0};
  int ret = 0;

  if (gpio_pin_cnt == 0) {
    printf("%s() unsupported board type\n", __func__);
    return -1;
  }

  if (gpio_num >= gpio_pin_cnt) {
    printf("slot %d: Invalid GPIO pin number %d\n", slot_id, gpio_num);
    return -1;
  }

  y35_get_gpio_name(slot_id, gpio_num, gpio_pin_name, false, intf);
  printf("slot %d: setting [%d]%s to %d\n", slot_id, gpio_num, gpio_pin_name, gpio_val);

  ret = remote_bic_set_gpio(slot_id, gpio_num, gpio_val, intf);
  if (ret < 0) {
    printf("%s() remote_bic_set_gpio returns %d\n", __func__, ret);
  }

  return ret;
}

static int
util_get_virtual_gpio(uint8_t slot_id) {
  int ret = 0, i = 0;
  uint8_t value = 0;
  uint8_t direction = 0;
  uint8_t gpio_pin_cnt = y35_get_gpio_list_size(slot_id, true, NONE_INTF);
  char gpio_pin_name[32] = "\0";

  if (gpio_pin_cnt == 0) {
    printf("%s() virtual GPIO count: %d\n", __func__, gpio_pin_cnt);
    return ret;
  }

  for (i = 0; i < gpio_pin_cnt; i++) {
    ret = bic_get_virtual_gpio(slot_id, i, &value, &direction);
    if (ret < 0) {
      printf("%s() bic_get_virtual_gpio returns %d\n", __func__, ret);
      return ret;
    }
    y35_get_gpio_name(slot_id, i, gpio_pin_name, true, NONE_INTF);
    printf("%d %s: %d\n",i , gpio_pin_name, value);
  }

  return ret;
}

static int
util_set_virtual_gpio(uint8_t slot_id, uint8_t gpio_num, uint8_t gpio_val) {
  int ret = -1;
  uint8_t gpio_pin_cnt = y35_get_gpio_list_size(slot_id, true, NONE_INTF);
  char gpio_pin_name[32] = "\0";

  if ((gpio_num > gpio_pin_cnt) || (gpio_pin_cnt == 0)) {
    printf("slot %d: Invalid virtual GPIO pin number %d, pin count %d\n", slot_id, gpio_num, gpio_pin_cnt);
    return ret;
  }

  y35_get_gpio_name(slot_id, gpio_num, gpio_pin_name, true, NONE_INTF);
  printf("slot %d: setting [%d]%s to %d\n", slot_id, gpio_num, gpio_pin_name, gpio_val);

  ret = bic_set_virtual_gpio(slot_id, gpio_num, gpio_val);
  if (ret < 0) {
    printf("%s() bic_set_virtual_gpio returns %d\n", __func__, ret);
  }

  return ret;
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
  int sensor_cnt = 0;
  ipmi_extend_sensor_reading_t sensor = {0};
  uint8_t config_status = 0xff;
  uint8_t *sensor_list;

  ret = bic_is_exp_prsnt(slot_id);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the status of 1OU/2OU\n", __func__);
    return -1;
  }
  config_status = (uint8_t) ret;

  ret = pal_get_fru_sensor_list(slot_id, &sensor_list, &sensor_cnt);
  if (ret < 0) {
    printf("fru%d get sensor list failed!\n", slot_id);
    return ret;
  }

  for (i = 0; i < sensor_cnt; i++) {
    ret = pal_read_bic_sensor(slot_id, sensor_list[i], &sensor, bmc_location, config_status);
    if (ret < 0 ) {
      continue;
    }
    if (sensor.read_type == STANDARD_CMD) {
      printf("sensor num: 0x%02X: value: 0x%02X, flags: 0x%02X, status: 0x%02X, ext_status: 0x%02X\n",
              sensor_list[i], sensor.value, sensor.flags, sensor.status, sensor.ext_status);
    } else {
      printf("sensor num: 0x%02X: value: 0x%04X, flags: 0x%02X\n", sensor_list[i], sensor.value, sensor.flags);
    }
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

  ret = bic_is_exp_prsnt(slot_id);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the status of 1OU/2OU\n", __func__);
    return -1;
  }
  config_status = (uint8_t) ret;

  if ( (config_status & PRESENT_1OU) == PRESENT_1OU && (bmc_location != NIC_BMC) ) {
    intf_list[1] = FEXP_BIC_INTF;
  } else if ( (config_status & PRESENT_2OU) == PRESENT_2OU ) {
    if ( fby35_common_get_2ou_board_type(slot_id, &type_2ou) == 0 ) {
      if ( ((type_2ou & DPV2_X8_BOARD) != DPV2_X8_BOARD) &&
           ((type_2ou & DPV2_X16_BOARD) != DPV2_X16_BOARD) ) {
        intf_list[2] = REXP_BIC_INTF;
      }
    } else {
      printf("%s() Failed to get 2OU board type\n", __func__);
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
util_print_dword_postcode_buf(uint8_t slot_id) {
  int ret = 0;
  size_t len;
  size_t intput_len = 0;
  size_t i;
  uint32_t * dw_postcode_buf = malloc( MAX_POSTCODE_NUM * sizeof(uint32_t));
  if (dw_postcode_buf) {
    intput_len = MAX_POSTCODE_NUM;
  } else {
    syslog(LOG_ERR, "%s Error, failed to allocate dw_postcode buffer", __func__);
    intput_len = 0;
    return -1;
  }

  ret = bic_request_post_buffer_dword_data(slot_id, dw_postcode_buf, intput_len, &len);
  if (ret) {
    printf("bic_request_post_buffer_dword_data: returns %d\n", ret);
    free(dw_postcode_buf);
    return ret;
  }
  printf("util_get_post_buf: returns %zu dword\n", len);
  for (i = 0; i < len; i++) {
    if (!(i % 4) && i)
      printf("\n");

    printf("[%08X] ", dw_postcode_buf[i]);
  }
  printf("\n");
  if(dw_postcode_buf)
    free(dw_postcode_buf);

  return ret;

}

static int
util_bic_clear_cmos(uint8_t slot_id) {
  return pal_clear_cmos(slot_id);
}

int
util_check_usb_port(uint8_t slot_id, uint8_t intf) {
  uint8_t tbuf[512], rbuf[512] = {0};
  uint8_t fw_comp;
  int i, ret = -1;
  int transferlen = sizeof(tbuf);
  int transferred = 0, received = 0;
  int retries = 3;
  usb_dev bic_udev;
  usb_dev *udev = &bic_udev;

  for (i = 0; i < transferlen; ++i) {
    tbuf[i] = i + 1;
  }

  udev->handle = NULL;
  udev->ci = 1;
  udev->epaddr = USB_INPUT_PORT;

  if (intf == NONE_INTF) {
    fw_comp = FW_BIOS;
  } else if (intf == FEXP_BIC_INTF) {
    fw_comp = FW_1OU_CXL;
  } else {
    fw_comp = 0xFF;
  }

  // init usb device
  ret = bic_init_usb_dev(slot_id, fw_comp, udev, SB_USB_PRODUCT_ID, SB_USB_VENDOR_ID);
  if (ret < 0) {
    goto exit;
  }

  printf("Input test data, USB timeout: 3000ms\n");
  while (true) {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, tbuf, transferlen, &transferred, 3000);
    if ((ret != 0) || (transferlen != transferred)) {
      printf("Error in transferring data! err = %d (%s) and transferred = %d (expected length %d)\n",
             ret, libusb_error_name(ret), transferred, transferlen);
      if (!(--retries)) {
        ret = -1;
        break;
      }
      msleep(100);
    } else {
      break;
    }
  }
  if (ret != 0) {
    goto exit;
  }

  udev->epaddr = USB_OUTPUT_PORT;
  retries = 3;
  while (true) {
    ret = libusb_bulk_transfer(udev->handle, udev->epaddr, rbuf, sizeof(rbuf), &received, 3000);
    if (ret != 0) {
      printf("Error in receiving data! err = %d (%s)\n", ret, libusb_error_name(ret));
      if (!(--retries)) {
        ret = -1;
        break;
      }
    } else {
      //printf("Received length: %d\n", received);
      printf("Received data: ");
      for (i = 0; i < received; ++i) {
        printf("%02x ", rbuf[i]);
      }
      printf("\n");
      break;
    }
  }

exit:
  if (ret != 0) {
    printf("Check USB port Failed\n");
  } else {
    printf("Check USB port Successful\n");
  }
  printf("\n");

  // close usb device
  bic_close_usb_dev(udev);

  return ret;
}

int
main(int argc, char **argv) {
  int ret = 0, i = 0;
  int opt_idx = 2;  // option: argv[opt_idx]
  uint8_t slot_id = 0;
  uint8_t is_fru_present = 0;
  uint8_t gpio_num = 0;
  uint8_t gpio_val = 0;
  uint8_t intf = NONE_INTF;

  if (argc < 3) {
    goto err_exit;
  }

  ret = fby35_common_get_slot_id(argv[1], &slot_id);
  if ( ret < 0 ) {
    printf("%s is invalid!\n", argv[1]);
    goto err_exit;
  }

  ret = fby35_common_get_bmc_location(&bmc_location);
  if ( ret < 0 ) {
    printf("%s() Couldn't get the location of BMC\n", __func__);
    return -1;
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

  if (strcmp(argv[opt_idx], "sb") == 0) {
    intf = NONE_INTF;
    ++opt_idx;
  } else if (strcmp(argv[opt_idx], "1ou") == 0) {
    if (slot_id != FRU_ALL) {
      ret = bic_is_exp_prsnt(slot_id);
      if (!(ret > 0 && (ret & PRESENT_1OU) == PRESENT_1OU)) {
        printf("%s %s is not present!\n", argv[1], argv[2]);
        return -1;
      }
    }
    intf = FEXP_BIC_INTF;
    ++opt_idx;
  } else {
    intf = NONE_INTF;
  }

  if (strncmp(argv[opt_idx], "--", 2) == 0) {
    ret = getopt_long(argc, argv, "", options, NULL);
    switch (ret) {
      case UTIL_GET_GPIO:
      case UTIL_GET_VGPIO:
        if (argc != (opt_idx+1)) {
          goto err_exit;
        }
        if (ret == UTIL_GET_VGPIO) {
          if (intf != NONE_INTF) {
            goto err_exit;
          }
          return util_get_virtual_gpio(slot_id);
        }
        return util_get_gpio(slot_id, intf);
      case UTIL_SET_GPIO:
      case UTIL_SET_VGPIO:
        if (argc != (opt_idx+3)) {
          goto err_exit;
        }
        gpio_num = atoi(argv[opt_idx+1]);
        gpio_val = atoi(argv[opt_idx+2]);
        if (gpio_val > 1) {
          goto err_exit;
        }
        if (ret == UTIL_SET_VGPIO) {
          if (intf != NONE_INTF) {
            goto err_exit;
          }
          return util_set_virtual_gpio(slot_id, gpio_num, gpio_val);
        }
        return util_set_gpio(slot_id, gpio_num, gpio_val, intf);
      case UTIL_GET_DEV_ID:
        if (argc != (opt_idx+1)) {
          goto err_exit;
        }
        return util_get_device_id(slot_id, intf);
      case UTIL_RESET:
        if (argc != (opt_idx+1)) {
          goto err_exit;
        }
        return util_bic_reset(slot_id, intf);
      case UTIL_GET_POSTCODE:
        if (argc != (opt_idx+1) || intf != NONE_INTF) {
          goto err_exit;
        }
        if (fby35_common_get_slot_type(slot_id) == SERVER_TYPE_HD) {
          return util_print_dword_postcode_buf(slot_id);
        }
        return util_get_postcode(slot_id);
      case UTIL_GET_SDR:
        if (argc != (opt_idx+1) || intf != NONE_INTF) {
          goto err_exit;
        }
        return util_get_sdr(slot_id);
      case UTIL_READ_SENSOR:
        if (argc != (opt_idx+1) || intf != NONE_INTF) {
          goto err_exit;
        }
        return util_read_sensor(slot_id);
      case UTIL_CHECK_STS:
        if (argc != (opt_idx+1) || intf != NONE_INTF) {
          goto err_exit;
        }
        return util_check_status(slot_id);
      case UTIL_CLEAR_CMOS:
        if (argc != (opt_idx+1) || intf != NONE_INTF) {
          goto err_exit;
        }
        return util_bic_clear_cmos(slot_id);
      case UTIL_PERF_TEST:
        if (argc != (opt_idx+2)) {
          goto err_exit;
        }
        return util_perf_test(slot_id, atoi(optarg), intf);
      case UTIL_CHECK_USB:
        if (argc != (opt_idx+1)) {
          goto err_exit;
        }
        return util_check_usb_port(slot_id, intf);
      case UTIL_FILE:
        if (argc != (opt_idx+2)) {
          goto err_exit;
        }
        if (slot_id == FRU_ALL) {
          if (bmc_location == NIC_BMC) {
            return process_file(FRU_SLOT1, optarg, intf);
          }
          for (i = FRU_SLOT1; i <= FRU_SLOT4; i++) {
            if (pal_is_fru_prsnt(i, &is_fru_present) || !is_fru_present) {
              printf("slot%d is not present!\n", i);
              continue;
            }
            if (intf == FEXP_BIC_INTF) {
              ret = bic_is_exp_prsnt(i);
              if (!(ret > 0 && (ret & PRESENT_1OU) == PRESENT_1OU)) {
                printf("slot%d 1ou is not present!\n", i);
                continue;
              }
            }
            process_file(i, optarg, intf);
          }
          return 0;
        }
        return process_file(slot_id, optarg, intf);
      default:
        printf("Invalid option: %s\n", argv[opt_idx]);
        goto err_exit;
    }
  } else if (argc >= (opt_idx+2)) {
    if (util_is_numeric(argv + opt_idx)) {
      return process_command(slot_id, (argc - opt_idx), (argv + opt_idx), intf);
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
