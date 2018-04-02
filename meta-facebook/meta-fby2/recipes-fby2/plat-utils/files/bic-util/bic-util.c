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
#include <facebook/bic.h>
#include <facebook/fby2_gpio.h>
#include <facebook/fby2_sensor.h>
#include <openbmc/ipmi.h>
#include <sys/time.h>
#include <time.h>

#define LAST_RECORD_ID 0xFFFF
#define MAX_SENSOR_NUM 0xFF
#define BYTES_ENTIRE_RECORD 0xFF

#define NUM_SLOTS 4
#define JTAG_TOTAL_API 1

enum cmd_profile_type {
  CMD_AVG_DURATION=0,
  CMD_MIN_DURATION,
  CMD_MAX_DURATION,
  CMD_NUM_SAMPLES,
  CMD_PROFILE_NUM
};

long double cmd_profile[NUM_SLOTS][CMD_PROFILE_NUM]={0};

static const char *option_list[] = {
  "--get_dev_id",
  "--get_gpio",
  "--set_gpio [gpio] [val]",
  "--get_gpio_config",
  "--set_gpio_config [gpio] [config]",
  "--get_config",
  "--get_post_code",
  "--read_fruid",
  "--get_sdr",
  "--read_sensor",
  "--perf_test [loop_count]  (0 to run forever)"
};

static void
print_usage_help(void) {
  int i;

  printf("Usage: bic-util <slot1|slot2|slot3|slot4> <[0..n]data_bytes_to_send>\n");
  printf("Usage: bic-util <slot1|slot2|slot3|slot4> <option>\n");
  printf("       option:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}


// Test to Get device ID
static void
util_get_device_id(uint8_t slot_id) {
  int ret;
  ipmi_dev_id_t id = {0};

  ret = bic_get_dev_id(slot_id, &id);
  if (ret) {
    printf("util_get_device_id: bic_get_dev_id returns %d\n", ret);
    return;
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
}


// Tests for reading GPIO values and configuration
static void
util_get_gpio(uint8_t slot_id) {
  int ret;
  uint8_t i, group, shift, gpio[8] = {0};
#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
  uint8_t server_type = 0xFF, gpio_cnt;
  char **gpio_name;

  ret = fby2_get_server_type(slot_id, &server_type);
  if (ret < 0) {
    printf("Cannot get server type. 0x%x\n", server_type);
    return ;
  }

  // Get GPIO value
  ret = bic_get_gpio_raw(slot_id, gpio);
  if (ret) {
    printf("util_get_gpio: bic_get_gpio returns %d\n", ret);
    return;
  }

  // Choose corresponding GPIO list based on server type
  switch (server_type) {
  case SERVER_TYPE_TL:
    gpio_cnt = gpio_pin_cnt;
    gpio_name = (char **)gpio_pin_name;
    break;
  case SERVER_TYPE_RC:
    gpio_cnt = rc_gpio_pin_cnt;
    gpio_name = (char **)rc_gpio_pin_name;
    break;
  case SERVER_TYPE_EP:
    gpio_cnt = ep_gpio_pin_cnt;
    gpio_name = (char **)ep_gpio_pin_name;
    break;
  default:
    printf("Cannot find corresponding server type. 0x%x\n", server_type);
    return ;
  }

  // Print the gpio index, name and value
  for (i = 0; i < gpio_cnt; i++) {
    group = i/8;
    shift = i%8;
    printf("%d %s: %d\n",i , gpio_name[i], (gpio[group] >> shift) & 0x01);
  }

#else
  ret = bic_get_gpio_raw(slot_id, gpio);
  if (ret) {
    printf("util_get_gpio: bic_get_gpio returns %d\n", ret);
    return;
  }

  // Print the gpio index, name and value
  for (i = 0; i < gpio_pin_cnt; i++) {
    group = i/8;
    shift = i%8;
    printf("%d %s: %d\n",i , gpio_pin_name[i], (gpio[group] >> shift) & 0x01);
  }
#endif
}


// Tests utility for setting GPIO values
static void
util_set_gpio(uint8_t slot_id, uint8_t gpio, uint8_t value) {
  int ret;

  printf("slot %d: setting GPIO %d to %d\n", slot_id, gpio, value);
  ret = bic_set_gpio(slot_id, gpio, value);
  if (ret) {
    printf("ERROR: bic_get_gpio returns %d\n", ret);
    return;
  }
}

static void
util_get_gpio_config(uint8_t slot_id) {
  int ret;
  int i;
  bic_gpio_config_t gpio_config = {0};
  bic_gpio_config_u *t = (bic_gpio_config_u *) &gpio_config;

#if defined(CONFIG_FBY2_RC) || defined(CONFIG_FBY2_EP)
  uint8_t server_type = 0xFF, gpio_cnt = 0xFF;
  char **gpio_name;
  ret = fby2_get_server_type(slot_id, &server_type);
  if (ret < 0) {
    printf("Cannot get server type. 0x%x\n", server_type);
    return ;
  }

  switch(server_type) {
  case SERVER_TYPE_TL:
    gpio_cnt = gpio_pin_cnt;
    gpio_name = (char **)gpio_pin_name;
    break;
  case SERVER_TYPE_RC:
    gpio_cnt = rc_gpio_pin_cnt;
    gpio_name = (char **)rc_gpio_pin_name;
    break;
  case SERVER_TYPE_EP:
    gpio_cnt = ep_gpio_pin_cnt;
    gpio_name = (char **)ep_gpio_pin_name;
    break;
  default:
    printf("Cannot find corresponding server type. 0x%x\n", server_type);
    return ;
  }

  // Read configuration of all bits
  for (i = 0;  i < gpio_cnt; i++) {
    ret = bic_get_gpio_config(slot_id, i, &gpio_config);
    if (ret == -1) {
      continue;
    }

    printf("gpio_config for pin#%d (%s):\n", i, gpio_name[i]);
    printf("Direction: %s", t->bits.dir?"Output,":"Input, ");
    printf(" Interrupt: %s", t->bits.ie?"Enabled, ":"Disabled,");
    printf(" Trigger: %s", t->bits.edge?"Level ":"Edge ");
    if (t->bits.trig == 0x0) {
      printf("Trigger,  Edge: %s\n", "Falling Edge");
    } else if (t->bits.trig == 0x1) {
      printf("Trigger,  Edge: %s\n", "Rising Edge");
    } else if (t->bits.trig == 0x2) {
      printf("Trigger,  Edge: %s\n", "Both Edges");
    } else  {
      printf("Trigger, Edge: %s\n", "Reserved");
    }
  }

#else
  // Read configuration of all bits
  for (i = 0;  i < gpio_pin_cnt; i++) {
    ret = bic_get_gpio_config(slot_id, i, &gpio_config);
    if (ret == -1) {
      continue;
    }

    printf("gpio_config for pin#%d (%s):\n", i, gpio_pin_name[i]);
    printf("Direction: %s", t->bits.dir?"Output,":"Input, ");
    printf(" Interrupt: %s", t->bits.ie?"Enabled, ":"Disabled,");
    printf(" Trigger: %s", t->bits.edge?"Level ":"Edge ");
    if (t->bits.trig == 0x0) {
      printf("Trigger,  Edge: %s\n", "Falling Edge");
    } else if (t->bits.trig == 0x1) {
      printf("Trigger,  Edge: %s\n", "Rising Edge");
    } else if (t->bits.trig == 0x2) {
      printf("Trigger,  Edge: %s\n", "Both Edges");
    } else  {
      printf("Trigger, Edge: %s\n", "Reserved");
    }
  }
#endif
}


// Tests utility for setting GPIO configuration
static void
util_set_gpio_config(uint8_t slot_id, uint8_t gpio, uint8_t config) {
  int ret;

  printf("slot %d: setting GPIO %d config to 0x%02x\n", slot_id, gpio, config);
  ret = bic_set_gpio_config(slot_id, gpio, (bic_gpio_config_t *)&config);
  if (ret) {
    printf("ERROR: bic_set_gpio_config returns %d\n", ret);
    return;
  }
}

static void
util_get_config(uint8_t slot_id) {
  int ret;
  bic_config_t config = {0};
  bic_config_u *t = (bic_config_u *) &config;

  ret = bic_get_config(slot_id, &config);
  if (ret) {
    printf("util_get_config: bic_get_config failed\n");
    return;
  }

  printf("SoL Enabled:  %s\n", t->bits.sol ? "Enabled" : "Disabled");
  printf("POST Enabled: %s\n", t->bits.post ? "Enabled" : "Disabled");
}


// Test to get the POST buffer
static void
util_get_post_buf(uint8_t slot_id) {
  int ret;
  uint8_t buf[MAX_IPMB_RES_LEN] = {0x0};
  uint8_t len;
  int i;

  ret = bic_get_post_buf(slot_id, buf, &len);
  if (ret) {
    printf("util_get_post_buf: bic_get_post_buf returns %d\n", ret);
    return;
  }

  printf("util_get_post_buf: returns %d bytes\n", len);
  for (i = 0; i < len; i++) {
    if (!(i % 16) && i)
      printf("\n");

    printf("%02X ", buf[i]);
  }
  printf("\n");
}


static void
util_read_fruid(uint8_t slot_id) {
  int ret;
  int fru_size = 0;

  char path[64] = {0};
  sprintf(path, "/tmp/fruid_slot%d.bin", slot_id);

  ret = bic_read_fruid(slot_id, 0, path, &fru_size);
  if (ret) {
    printf("util_read_fruid: bic_read_fruid returns %d, fru_size: %d\n", ret, fru_size);
    return;
  }
}



static void
util_get_sdr(uint8_t slot_id) {
  int ret;
  uint8_t rlen;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};

  ipmi_sel_sdr_req_t req;
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;

  while (1) {
    ret = bic_get_sdr(slot_id, &req, res, &rlen);
    if (ret) {
      printf("util_get_sdr:bic_get_sdr returns %d\n", ret);
      continue;
    }

    sdr_full_t *sdr = (sdr_full_t *)res->data;

    printf("type: %d, ", sdr->type);
    printf("sensor_num: %d, ", sdr->sensor_num);
    printf("sensor_type: %d, ", sdr->sensor_type);
    printf("evt_read_type: %d, ", sdr->evt_read_type);
    printf("m_val: %d, ", sdr->m_val);
    printf("m_tolerance: %d, ", sdr->m_tolerance);
    printf("b_val: %d, ", sdr->b_val);
    printf("b_accuracy: %d, ", sdr->b_accuracy);
    printf("accuracy_dir: %d, ", sdr->accuracy_dir);
    printf("rb_exp: %d,\n", sdr->rb_exp);

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      printf("This record is LAST record\n");
      break;
    }
  }
}

// Test to read all Sensors from Monolake Server
static void
util_read_sensor(uint8_t slot_id) {
  int ret;
  int i;
  ipmi_sensor_reading_t sensor;

  for (i = 0; i < MAX_SENSOR_NUM; i++) {
    ret = bic_read_sensor(slot_id, i, &sensor);
    if (ret) {
      continue;
    }

    printf("sensor#%d: value: 0x%X, flags: 0x%X, status: 0x%X, ext_status: 0x%X\n",
            i, sensor.value, sensor.flags, sensor.status, sensor.ext_status);
  }
}


// runs performance test for loopCount loops
static void
util_perf_test(uint8_t slot_id, int loopCount) {
  int ret;
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

  while(1)
  {
    struct timeval  tv1, tv2;
    gettimeofday(&tv1, NULL);

    ret = bic_get_dev_id(slot_id, &id);
    if (ret) {
      printf("util_get_device_id: bic_get_dev_id returns %d, loop=%d\n", ret, i);
      return;
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
  }  // while(1) {}


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

// TODO: Make it as User selectable tests to run
int
main(int argc, char **argv) {
  uint8_t slot_id;

  if (argc < 3) {
    goto err_exit;
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
    goto err_exit;
  }

  if (!strcmp(argv[2], "--get_dev_id")) {
    util_get_device_id(slot_id);
  } else if (!strcmp(argv[2], "--get_gpio")) {
    util_get_gpio(slot_id);
  } else if (!strcmp(argv[2], "--set_gpio")) {
    util_set_gpio(slot_id, atoi(argv[3]), atoi(argv[4]));
  } else if (!strcmp(argv[2], "--get_gpio_config")) {
    util_get_gpio_config(slot_id);
  } else if (!strcmp(argv[2], "--set_gpio_config")) {
    util_set_gpio_config(slot_id, atoi(argv[3]), atoi(argv[4]));
  } else if (!strcmp(argv[2], "--get_config")) {
    util_get_config(slot_id);
  } else if (!strcmp(argv[2], "--get_post_code")) {
    util_get_post_buf(slot_id);
  } else if (!strcmp(argv[2], "--read_fruid")) {
    util_read_fruid(slot_id);
  } else if (!strcmp(argv[2], "--get_sdr")) {
    util_get_sdr(slot_id);
  } else if (!strcmp(argv[2], "--read_sensor")) {
    util_read_sensor(slot_id);
  } else if (!strcmp(argv[2], "--perf_test")) {
    util_perf_test(slot_id, atoi(argv[3]));
  } else if (argc >= 4) {
    return process_command(slot_id, (argc - 2), (argv + 2));
  } else {
    goto err_exit;
  }

  return 0;

err_exit:
  print_usage_help();
  return -1;
}
