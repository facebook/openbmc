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
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>

#define LAST_RECORD_ID 0xFFFF
#define MAX_SENSOR_NUM 0xFF
#define BYTES_ENTIRE_RECORD 0xFF

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
  printf("PWRGOOD_CPU: %d\n", t->bits.pwrgood_cpu);
  printf("PWRGOOD_PCH_PWROK: %d\n", t->bits.pwrgd_pch_pwrok);
  printf("PVDDR_VRHOT_N: %d\n", t->bits.pvddr_vrhot_n);
  printf("PVCCIN_VRHOT_N: %d\n", t->bits.pvccin_vrhot_n);
  printf("FM_FAST_PROCHOT_N: %d\n", t->bits.fm_fast_prochot_n);
  printf("PCHHOT_CPU_N: %d\n", t->bits.pchhot_cpu_n);
  printf("FM_CPLD_CPU_DIMM_EVENT_C0_N: %d\n", t->bits.fm_cpld_cpu_dimm_event_c0_n);
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
  for (i = 0;  i < MAX_GPIO_PINS; i++) {
    ret = bic_get_gpio_config(slot_id, i, &gpio_config);
    if (ret == -1) {
      continue;
    }

    printf("gpio_config for pin#%d:\n", i);
    printf("Direction: %s", t->bits.dir?"Output":"Input");
    printf("Interrupt Enabled?: %s", t->bits.ie?"Enabled":"Disabled");
    printf("Trigger Type: %s", t->bits.edge?"Level":"Edge");
    if (t->bits.trig == 0x0) {
      printf("Trigger Edge: %s\n", "Falling Edge");
    } else if (t->bits.trig == 0x1) {
      printf("Trigger Edge: %s\n", "Falling Edge");
    } else if (t->bits.trig == 0x2) {
      printf("Trigger Edge: %s\n", "Both Edges");
    } else  {
      printf("Trigger Edge: %s\n", "Reserved");
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

  printf("SoL Enabled?: %s", t->bits.sol? "Enabled" : "Disabled");
  printf("POST Enabled?: %s", t->bits.post? "Enabled" : "Disabled");
  printf("KCS Enabled?: %s", t->bits.kcs? "Enabled" : "Disabled");
  printf("IPMB Enabled?: %s", t->bits.ipmb? "Enabled" : "Disabled");
}

static void
util_set_config(uint8_t slot_id, uint8_t status) {

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
    printf("0x%X:", buf[i]);
  }
  printf("\n");
}

// Tests to read FRUID of Monolake Server
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

static void
util_read_fruid(uint8_t slot_id) {
  int ret;
  int i;

  char path[64] = {0};
  sprintf(path, "/tmp/fruid_slot%d.bin", slot_id);

  ret = bic_read_fruid(slot_id, 0, path);
  if (ret) {
    printf("util_read_fruid: bic_read_fruid returns %d\n", ret);
    return;
  }
}

// Tests to read SEL from Monolake Server
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
  int ret;
  int i;
  uint16_t rsv;
  uint8_t rlen;
  uint8_t rbuf[MAX_IPMB_RES_LEN] = {0};

  ipmi_sel_sdr_req_t req;
  ipmi_sel_sdr_res_t *res = (ipmi_sel_sdr_res_t *) rbuf;

  req.rsv_id = 0;
  req.rec_id = 0;
  req.offset = 0;
  req.nbytes = BYTES_ENTIRE_RECORD;

  while (1) {
    ret = bic_get_sel(slot_id, &req, res, &rlen);
    if (ret) {
      printf("util_get_sel:bic_get_sel returns %d\n", ret);
      continue;
    }

    printf("SEL for rec_id %d\n", req.rec_id);
    printf("Next Record ID is %d\n", res->next_rec_id);
    printf("Record contents are..\n");
    for (i = 0;  i < rlen-2; i++) { // First 2 bytes are next_rec_id
      printf("0x%X:", res->data[i]);
    }
    printf("\n");

    req.rec_id = res->next_rec_id;
    if (req.rec_id == LAST_RECORD_ID) {
      printf("This record is LAST record\n");
      break;
    }
  }
}

// Tests to read SDR records from Monolake Servers
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

// TODO: Make it as User selectable tests to run
int
main(int argc, char **argv) {

  uint8_t slot_id;

  slot_id = atoi(argv[1]);

  util_get_fw_ver(slot_id);

#if 0
  util_get_device_id(slot_id);

  util_get_gpio(slot_id);
  util_get_gpio_config(slot_id);

  util_get_config(slot_id);

  util_get_post_buf(slot_id);

  util_get_fruid_info(slot_id);
  util_read_fruid(slot_id);

  util_get_sel_info(slot_id);
  util_get_sel(slot_id);

  util_get_sdr_info(slot_id);
  util_get_sdr(slot_id);
  util_read_sensor(slot_id);
#endif
}
