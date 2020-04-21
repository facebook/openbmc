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

//long double cmd_profile[NUM_SLOTS][CMD_PROFILE_NUM]={0};

static const char *option_list[] = {
  "--get_gpio",
  "--set_gpio $gpio_num $value",
  "--get_gpio_config",
  "--set_gpio_config $gpio_num $value",
  "--check_status",
  "--get_dev_id",
  "--reset",
};

static void
print_usage_help(void) {
  int i;

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
util_get_device_id(uint8_t slot_id) {
  int ret = 0;
  ipmi_dev_id_t id = {0};

  ret = bic_get_dev_id(slot_id, &id, NONE_INTF);
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
util_bic_reset(uint8_t slot_id) {
  int ret = 0;
  ret = bic_reset(slot_id);
  printf("Performing BIC reset, status %d\n", ret);
  return ret;
}

static int
process_command(uint8_t slot_id, int argc, char **argv) {
  int i, ret, retry = 2;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;

  for (i = 0; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  while (retry >= 0) {
    ret = bic_ipmb_wrapper(slot_id, tbuf[0]>>2, tbuf[1], &tbuf[2], tlen-2, rbuf, &rlen);
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
#if 0
static int
process_file(uint8_t slot_id, char *path) {
  FILE *fp;
  int argc;
  char buf[1024];
  char *str, *next, *del=" \n";
  char *argv[MAX_ARG_NUM];

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
    if (argc < 1)
      continue;

    process_command(slot_id, argc, argv);
  }
  fclose(fp);

  return 0;
}
#endif

#define BIT_VALUE(list, index) \
           ((((uint8_t*)&list)[index/8]) >> (index % 8)) & 0x1\

static int
util_get_gpio(uint8_t slot_id) {
  int ret = 0;
  uint8_t i;
  uint8_t gpio_pin_cnt = fby3_get_gpio_list_size();
  char gpio_pin_name[32] = "\0";
  bic_gpio_t gpio = {0};

  ret = bic_get_gpio(slot_id, &gpio);
  if ( ret < 0 ) {
    printf("%s() bic_get_gpio returns %d\n", __func__, ret);
    return ret;
  }

  // Print the gpio index, name and value
  for (i = 0; i < gpio_pin_cnt; i++) {
    fby3_get_gpio_name(slot_id, i, gpio_pin_name);
    printf("%d %s: %d\n",i , gpio_pin_name, BIT_VALUE(gpio, i));
  }

  return ret;
}

static int
util_set_gpio(uint8_t slot_id, uint8_t gpio_num, uint8_t gpio_val) {
  uint8_t gpio_pin_cnt = fby3_get_gpio_list_size();
  char gpio_pin_name[32] = "\0";
  int ret = -1;

  if ( gpio_num > gpio_pin_cnt ) {
    printf("slot %d: Invalid GPIO pin number %d\n", slot_id, gpio_num);
    return ret;
  }

  fby3_get_gpio_name(slot_id, gpio_num, gpio_pin_name);
  printf("slot %d: setting [%d]%s to %d\n", slot_id, gpio_num, gpio_pin_name, gpio_val);

  ret = bic_set_gpio(slot_id, gpio_num, gpio_val);
  if (ret < 0) {
    printf("%s() bic_set_gpio returns %d\n", __func__, ret);
  }

  return ret;
}

static int
util_get_gpio_config(uint8_t slot_id) {
  int ret = 0;
  uint8_t i;
  uint8_t gpio_pin_cnt = fby3_get_gpio_list_size();
  char gpio_pin_name[32] = "\0";
  bic_gpio_config_t gpio_config = {0}; 

  // Print the gpio index, name and value
  for (i = 0; i < gpio_pin_cnt; i++) {
    fby3_get_gpio_name(slot_id, i, gpio_pin_name);
    //printf("%d %s: %d\n",i , gpio_pin_name, BIT_VALUE(gpio, i));
    ret = bic_get_gpio_config(slot_id, i, (uint8_t *)&gpio_config);
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
util_set_gpio_config(uint8_t slot_id, uint8_t gpio_num, uint8_t config_val) {
  int ret = 0;
  uint8_t gpio_pin_cnt = fby3_get_gpio_list_size();
  char gpio_pin_name[32] = "\0";

  if ( gpio_num > gpio_pin_cnt ) {
    printf("slot %d: Invalid GPIO pin number %d\n", slot_id, gpio_num);
    return ret;
  }

  fby3_get_gpio_name(slot_id, gpio_num, gpio_pin_name);
  printf("slot %d: setting GPIO [%d]%s config to 0x%02x\n", slot_id, gpio_num, gpio_pin_name, config_val);
  ret = bic_set_gpio_config(slot_id, gpio_num, config_val);
  if (ret < 0) {
    printf("%s() bic_set_gpio_config returns %d\n", __func__, ret);
  }

  return 0;
}

int
main(int argc, char **argv) {
  uint8_t slot_id = 0;
  uint8_t is_fru_present = 0;
  int ret = 0;
  uint8_t gpio_num = 0;
  uint8_t gpio_val = 0;

  if (argc < 3) {
    goto err_exit;
  }

  ret = fby3_common_get_slot_id(argv[1], &slot_id);
  if ( ret < 0 ) {
    goto err_exit;
  }

  ret = fby3_common_is_fru_prsnt(slot_id, &is_fru_present);
  if ( ret < 0 || is_fru_present == 0 ) {
    printf("%s is not present!\n", argv[1]);
    return -1;
  }

  if ( strncmp(argv[2], "--", 2) == 0 ) {
    if ( strcmp(argv[2], "--get_gpio") == 0 ) {
      return util_get_gpio(slot_id);
    } else if ( strcmp(argv[2], "--set_gpio") == 0 ) {
      gpio_num = atoi(argv[3]);
      gpio_val = atoi(argv[4]);
      if ( argc != 5 || gpio_num > 0xff || gpio_val > 1 ) {
        goto err_exit;
      }
      return util_set_gpio(slot_id, gpio_num, gpio_val);
    } else if ( strcmp(argv[2], "--get_gpio_config") == 0 ) {
      return util_get_gpio_config(slot_id);
    } else if ( strcmp(argv[2], "--set_gpio_config") == 0 ) {
      gpio_num = atoi(argv[3]);
      gpio_val = (uint8_t)strtol(argv[4], NULL, 0);
      if ( argc != 5 || gpio_num > 0xff ) {
        goto err_exit;
      }
      return util_set_gpio_config(slot_id, gpio_num, gpio_val);
    } else if ( strcmp(argv[2], "--check_status") == 0 ) {
      return util_check_status(slot_id);
    } else if ( strcmp(argv[2], "--get_dev_id") == 0 ) {
      return util_get_device_id(slot_id);
    } else if ( strcmp(argv[2], "--reset") == 0 ) {
      return util_bic_reset(slot_id);
    } else {
      printf("Invalid option: %s\n", argv[2]);
      goto err_exit;
    }
  } else {
    return process_command(slot_id, (argc - 2), (argv + 2));
  }

err_exit:
  print_usage_help();
  return -1;
}
