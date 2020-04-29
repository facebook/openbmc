/*
 * bic-util
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include <facebook/minipack_gpio.h>
#include <openbmc/ipmi.h>
#include <openbmc/fruid.h>
#include <fcntl.h>

#define LAST_RECORD_ID 0xFFFF
#define MAX_SENSOR_NUM 0xFF
#define BYTES_ENTIRE_RECORD 0xFF
#define EEPROM_SIZE 256
#define FRU_NAME "MINILAKE"
#define FRU_PATH "/tmp/fruid_minilake.bin"

#define LOGFILE "/tmp/bic-util.log"

#define MAX_ARG_NUM 64
#define MAX_CMD_RETRY 2
#define MAX_TOTAL_RETRY 30

static int total_retry = 0;

static const char *option_list[] = {
  "--get_dev_id",
  "--get_gpio",
  "--get_gpio_config",
  "--get_config",
  "--get_post_code",
  "--get_sdr",
  "--read_sensor",
  "--read_fruid",
  "--read_mac"
};

static void
print_usage_help(void) {
  int i;

  printf("Usage: bic-util <scm> <[0..n]data_bytes_to_send>\n");
  printf("Usage: bic-util <scm> <--me> <[0..n]data_bytes_to_send>\n");
  printf("Usage: bic-util <scm> <--me_file> <path>\n");
  printf("Usage: bic-util <scm> <option>\n");
  printf("       option:\n");
  for (i = 0; i < sizeof(option_list)/sizeof(option_list[0]); i++)
    printf("       %s\n", option_list[i]);
}

/* Print the FRUID in detail */
static void
print_fruid_info(fruid_info_t *fruid, const char *name)
{
  /* Print format */
  printf("%-27s: %s", "\nFRU Information",
      name /* Name of the FRU device */ );
  printf("%-27s: %s", "\n---------------", "------------------");

  if (fruid->chassis.flag) {
    printf("%-27s: %s", "\nChassis Type",fruid->chassis.type_str);
    printf("%-27s: %s", "\nChassis Part Number",fruid->chassis.part);
    printf("%-27s: %s", "\nChassis Serial Number",fruid->chassis.serial);
    if (fruid->chassis.custom1 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 1",fruid->chassis.custom1);
    if (fruid->chassis.custom2 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 2",fruid->chassis.custom2);
    if (fruid->chassis.custom3 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 3",fruid->chassis.custom3);
    if (fruid->chassis.custom4 != NULL)
      printf("%-27s: %s", "\nChassis Custom Data 4",fruid->chassis.custom4);
  }

  if (fruid->board.flag) {
    printf("%-27s: %s", "\nBoard Mfg Date",fruid->board.mfg_time_str);
    printf("%-27s: %s", "\nBoard Mfg",fruid->board.mfg);
    printf("%-27s: %s", "\nBoard Product",fruid->board.name);
    printf("%-27s: %s", "\nBoard Serial",fruid->board.serial);
    printf("%-27s: %s", "\nBoard Part Number",fruid->board.part);
    printf("%-27s: %s", "\nBoard FRU ID",fruid->board.fruid);
    if (fruid->board.custom1 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 1",fruid->board.custom1);
    if (fruid->board.custom2 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 2",fruid->board.custom2);
    if (fruid->board.custom3 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 3",fruid->board.custom3);
    if (fruid->board.custom4 != NULL)
      printf("%-27s: %s", "\nBoard Custom Data 4",fruid->board.custom4);
  }

  if (fruid->product.flag) {
    printf("%-27s: %s", "\nProduct Manufacturer",fruid->product.mfg);
    printf("%-27s: %s", "\nProduct Name",fruid->product.name);
    printf("%-27s: %s", "\nProduct Part Number",fruid->product.part);
    printf("%-27s: %s", "\nProduct Version",fruid->product.version);
    printf("%-27s: %s", "\nProduct Serial",fruid->product.serial);
    printf("%-27s: %s", "\nProduct Asset Tag",fruid->product.asset_tag);
    printf("%-27s: %s", "\nProduct FRU ID",fruid->product.fruid);
    if (fruid->product.custom1 != NULL)
      printf("%-27s: %s", "\nProduct Custom Data 1",fruid->product.custom1);
    if (fruid->product.custom2 != NULL){
      printf("%-27s: %s", "\nProduct Custom Data 2",fruid->product.custom2);
    }
    if (fruid->product.custom3 != NULL){
      printf("%-27s: %s", "\nProduct Custom Data 3",fruid->product.custom3);
    }
    if (fruid->product.custom4 != NULL){
      printf("%-27s: %s", "\nProduct Custom Data 4",fruid->product.custom4);
    }
  }

  printf("\n");
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

static void
util_get_fw_ver(uint8_t slot_id) {
  int i, j, ret;
  uint8_t buf[16] = {0};
  for (i = 1; i <= 8; i++) {
    ret = bic_get_fw_ver(slot_id, i, buf);
    printf("version of comp: %d is", i);
    for (j = 0; j < 10; j++)
      printf("%02X:", buf[j]);
    printf("\n");
  }
}

// Tests for reading GPIO values and configuration
static void
util_get_gpio(uint8_t slot_id) {
  int ret;
  bic_gpio_t gpio = {0};

  ret = bic_get_gpio(slot_id, &gpio);
  if (ret) {
    printf("util_get_gpio: bic_get_gpio returns %d\n", ret);
    return;
  }

  bic_gpio_u *t = (bic_gpio_u*) &gpio;

  // Print response
  printf("XDP_CPU_SYSPWROK: %d\n", t->bits.pwrgood_cpu);
  printf("PWRGD_PCH_PWROK: %d\n", t->bits.pwrgd_pch_pwrok);
  printf("PVDDR_VRHOT_N: %d\n", t->bits.pvddr_vrhot_n);
  printf("PVCCIN_VRHOT_N: %d\n", t->bits.pvccin_vrhot_n);
  printf("FM_FAST_PROCHOT_N: %d\n", t->bits.fm_fast_prochot_n);
  printf("PCHHOT_CPU_N: %d\n", t->bits.pchhot_cpu_n);
  printf("FM_CPLD_CPU_DIMM_EVENT_CO_N: %d\n", t->bits.fm_cpld_cpu_dimm_event_c0_n);
  printf("FM_CPLD_BDXDE_THERMTRIP_N: %d\n", t->bits.fm_cpld_bdxde_thermtrip_n);
  printf("THERMTRIP_PCH_N: %d\n", t->bits.thermtrip_pch_n);
  printf("FM_CPLD_FIVR_FAULT: %d\n", t->bits.fm_cpld_fivr_fault);
  printf("FM_BDXDE_CATERR_LVT3_N: %d\n", t->bits.fm_bdxde_caterr_lvt3_n);
  printf("FM_BDXDE_ERR_LVT3_N: %d\n", t->bits.fm_bdxde_err_lvt3_n);
  printf("SLP_S4_N: %d\n", t->bits.slp_s4_n);
  printf("FM_NMI_EVENT_BMC_N: %d\n", t->bits.fm_nmi_event_bmc_n);
  printf("FM_SMI_BMC_N: %d\n", t->bits.fm_smi_bmc_n);
  printf("RST_PLTRST_BMC_N: %d\n", t->bits.rst_pltrst_bmc_n);
  printf("FP_RST_BTN_BUF_N: %d\n", t->bits.fp_rst_btn_buf_n);
  printf("BMC_RST_BTN_OUT_N: %d\n", t->bits.bmc_rst_btn_out_n);
  printf("FM_BDE_POST_CMPLT_N: %d\n", t->bits.fm_bde_post_cmplt_n);
  printf("FM_BDXDE_SLP3_N: %d\n", t->bits.fm_bdxde_slp3_n);
  printf("FM_PWR_LED_N: %d\n", t->bits.fm_pwr_led_n);
  printf("PWRGD_PVCCIN: %d\n", t->bits.pwrgd_pvccin);
  printf("SVR_ID: %d\n", t->bits.svr_id);
  printf("BMC_READY_N: %d\n", t->bits.bmc_ready_n);
  printf("BMC_COM_SW_N: %d\n", t->bits.bmc_com_sw_n);
  printf("rsvd: %d\n", t->bits.rsvd);
}

static void
util_get_gpio_config(uint8_t slot_id) {
  int ret;
  int i;
  bic_gpio_config_t gpio_config = {0};
  bic_gpio_config_u *t = (bic_gpio_config_u *) &gpio_config;


  // Read configuration of all bits
  for (i = 0;  i < gpio_pin_cnt; i++) {
    ret = bic_get_gpio_config(slot_id, i, &gpio_config);
    if (ret == -1) {
      continue;
    }

    printf("gpio_config for pin#%d (%s):\n",
           i, minipack_gpio_type_to_name(i));
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
}

static void
util_get_config(uint8_t slot_id) {
  int ret;
  int i;
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

// Tests to read FRUID of Minilake
static void
util_get_fruid_info(uint8_t slot_id) {
  int ret;
  int i;

  ipmi_fruid_info_t info = {0};

  ret = bic_get_fruid_info(slot_id, 0, &info);
  if (ret) {
    printf("util_get_fruid_info: bic_get_fruid_info returns %d\n", ret);
    return;
  }

  printf("FRUID info for 1S Slot..\n");

  printf("FRUID Size: %d\n", (info.size_msb << 8) + (info.size_lsb));
  printf("Accessed as : %s\n", (info.bytes_words)?"Words":"Bytes");
}

static int
get_fruid_info(uint8_t slot_id, fruid_info_t *fruid) {
  int ret;
  ret = fruid_parse(FRU_PATH, &fruid);
  if(ret){
    printf("Unable to parse eeprom data.");
    return -1;
  } 
  
  print_fruid_info(&fruid, FRU_NAME);
  free_fruid_info(&fruid);
  
  return 0;
}

static void
util_read_fruid(uint8_t slot_id) {
  int ret;
  int fru_size = 0;
  fruid_info_t fruid;

  ret = bic_read_fruid(slot_id, 0, FRU_PATH, &fru_size);
  if (ret) {
    printf("util_read_fruid: bic_read_fruid returns %d, fru_size: %d\n", ret, fru_size);
    return;
  }
  
  ret = get_fruid_info(slot_id, &fruid);
  
  if (ret) {
    printf("util_read_fruid: print_fruid returns %d, fru_size: %d\n", ret, fru_size);
    return;
  }
}

// Tests to read SEL from Minilake
static void
util_get_sel_info(uint8_t slot_id) {
  int ret;

  ipmi_sel_sdr_info_t info;

  ret = bic_get_sel_info(slot_id, &info);
  if (ret) {
    printf("util_get_sel_info:bic_get_sel_info returns %d\n", ret);
    return;
  }

  printf("SEL info for 1S Slot is..\n");

  printf("version: 0x%X\n", info.ver);
  printf("Record Count: 0x%X\n", info.rec_count);
  printf("Free Space: 0x%X\n", info.free_space);
  printf("Recent Add TS: 0x%X:0x%X:0x%X:0x%X\n", info.add_ts[3], info.add_ts[2], info.add_ts[1], info.add_ts[0]);
  printf("Recent Erase TS: 0x%X:0x%X:0x%X:0x%X\n", info.erase_ts[3], info.erase_ts[2], info.erase_ts[1], info.erase_ts[0]);
  printf("Operation Support: 0x%X\n", info.oper);
}

static void
util_get_sel(uint8_t slot_id) {
  int i, ret;
  uint16_t rsv;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};

  ipmi_sel_sdr_req_t req;
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;

  while (1) {
    size_t rlen = sizeof(rbuf);

    ret = bic_get_sel(slot_id, &req, res, &rlen);
    if (ret) {
      printf("util_get_sel:bic_get_sel returns %d\n", ret);
      continue;
    }

    printf("SEL for rec_id %d\n", req.rec_id);
    printf("Next Record ID is %d\n", res->next_rec_id);
    if (rlen >= 2) {
      printf("Record contents are..\n");
      for (i = 0;  i < rlen-2; i++) { // First 2 bytes are next_rec_id
        printf("0x%X:", res->data[i]);
      }
    }
    printf("\n");

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      printf("This record is LAST record\n");
      break;
    }
  }
}

// Tests to read SDR records from Minilake
static void
util_get_sdr_info(uint8_t slot_id) {
  int ret;

  ipmi_sel_sdr_info_t info;

  ret = bic_get_sdr_info(slot_id, &info);
  if (ret) {
    printf("util_get_sdr_info:bic_get_sdr_info returns %d\n", ret);
    return;
  }

  printf("SDR info for 1S Slot is..\n");

  printf("version: 0x%X\n", info.ver);
  printf("Record Count: 0x%X\n", info.rec_count);
  printf("Free Space: 0x%X\n", info.free_space);
  printf("Recent Add TS: 0x%X:0x%X:0x%X:0x%X\n", info.add_ts[3], info.add_ts[2], info.add_ts[1], info.add_ts[0]);
  printf("Recent Erase TS: 0x%X:0x%X:0x%X:0x%X\n", info.erase_ts[3], info.erase_ts[2], info.erase_ts[1], info.erase_ts[0]);
  printf("Operation Support: 0x%X\n", info.oper);
}

static void
util_get_sdr(uint8_t slot_id) {
  int ret;
  int i;
  uint16_t rsv;
  size_t rlen;
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

    sdr_full_t *sdr = res->data;

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

// Test to read all Sensors from Minilake
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

// Test to read MAC address from Minilake
static void
util_read_mac(uint8_t slot_id) {
  int ret;
  int i;
  uint8_t mbuf[8] = {0};


  ret = bic_read_mac(slot_id, mbuf, sizeof(mbuf));
  if (ret) {
    printf("Cannot get MAC address from Minilake.\n");
    return;
  }

  printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
          mbuf[2], mbuf[3], mbuf[4], mbuf[5], mbuf[6], mbuf[7]);
}

static int
process_command(uint8_t slot_id, int argc, char **argv) {
  int i, ret, retry = 2;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  size_t rlen;

  for (i = 0; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  while (retry >= 0) {
    rlen = sizeof(rbuf);
    ret = bic_ipmb_wrapper(slot_id, tbuf[0]>>2, tbuf[1],
                           &tbuf[2], tlen-2, rbuf, &rlen);
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
process_command_me(uint8_t slot_id, int argc, char **argv) {
  int i, ret, retry = MAX_CMD_RETRY;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  //int logfd, len;
  //char log[256];

  for (i = 0; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

  while (retry >= 0) {
    ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
    if (ret == 0)
      break;

    total_retry++;
    retry--;
  }
  if (ret) {
    printf("ME no response!\n");
    return ret;
  }

  //log[0] = 0;
  if (rbuf[0] != 0x00) {
    printf("Completion Code: %02X, ", rbuf[0]);
  }

  for (i = 1; i < rlen; i++) {
    printf("%02X ", rbuf[i]);
    //sprintf(log, "%s%02X ", log, rbuf[i]);
  }
  printf("\n");

#if 0
  sprintf(log, "%s\n", log);

  logfd = open(LOGFILE, O_CREAT | O_WRONLY);
  if (logfd < 0) {
    syslog(LOG_WARNING, "Opening a tmp file failed. errno: %d", errno);
    return -1;
  }

  len = write(logfd, log, strlen(log));
  if (len != strlen(log)) {
    syslog(LOG_WARNING, "Error writing the log to the file");
    close(logfd);
    return -1;
  }
  close(logfd);
#endif

  return 0;
}

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
    for (argc = 0; argc < MAX_ARG_NUM && str; argc++, 
	     str = strtok_r(NULL, del, &next)) {
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

    process_command_me(slot_id, argc, argv);
    if (total_retry > MAX_TOTAL_RETRY) {
      printf("Maximum retry count exceeded\n");
      fclose(fp);
      return -1;
    }
  }
  fclose(fp);

  return 0;
}

int
main(int argc, char **argv) {
  uint8_t slot_id;

  if (argc < 3) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "scm")) {
    slot_id = 0;
  }
  else {
    goto err_exit;
  }

  if (!strcmp(argv[2], "--get_dev_id")) {
    util_get_device_id(slot_id);
  } else if (!strcmp(argv[2], "--get_gpio")) {
    util_get_gpio(slot_id);
  } else if (!strcmp(argv[2], "--get_gpio_config")) {
    util_get_gpio_config(slot_id);
  } else if (!strcmp(argv[2], "--get_config")) {
    util_get_config(slot_id);
  } else if (!strcmp(argv[2], "--get_post_code")) {
    util_get_post_buf(slot_id);
  } else if (!strcmp(argv[2], "--get_sdr")) {
    util_get_sdr(slot_id);
  } else if (!strcmp(argv[2], "--read_sensor")) {
    util_read_sensor(slot_id);
  } else if (!strcmp(argv[2], "--read_fruid")) {
    util_read_fruid(slot_id);
  } else if (!strcmp(argv[2], "--read_mac")) {
    util_read_mac(slot_id);
  } else if (!strcmp(argv[2], "--me_file")) {
    if (argc < 4) {
      goto err_exit;
    }
    process_file(slot_id, argv[3]);
    return 0;
  } else if (argc >= 4) {
    if(!strcmp(argv[2], "--me")) {
      return process_command_me(slot_id, (argc - 3), (argv + 3));
    }
    else
      return process_command(slot_id, (argc - 2), (argv + 2));
  } 
 else {
    goto err_exit;
  }

  return 0;

err_exit:
  print_usage_help();
  return -1;

}
