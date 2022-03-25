/*
 * sensorsvcpal.c
 *
 * Copyright 2017-present Facebook. All Rights Reserved.
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
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/mman.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "sensorsvcpal.h"
#include <openbmc/obmc-i2c.h>
#include <sys/stat.h>
#include <openbmc/gpio.h>
#include <openbmc/kv.h>

#define BIT(value, index) ((value >> index) & 1)

#define LAST_KEY "last_key"
#define GPIO_VAL "/sys/class/gpio/gpio%d/value"
#define GPIO_DIR "/sys/class/gpio/gpio%d/direction"

#define GPIO_POWER_GOOD 14


#define GPIO_BOARD_SKU_ID0 120
#define GPIO_BOARD_SKU_ID1 121
#define GPIO_BOARD_SKU_ID2 122
#define GPIO_BOARD_SKU_ID3 123
#define GPIO_BOARD_SKU_ID4 124


#define GPIO_SLT_CFG0 142
#define GPIO_SLT_CFG1 143
#define GPIO_FM_CPU0_SKTOCC_LVT3_N 51
#define GPIO_FM_CPU1_SKTOCC_LVT3_N 208
#define GPIO_FM_BIOS_POST_CMPLT_N 215
#define GPIO_FM_SLPS4_N 193
#define GPIO_FM_FORCE_ADR_N 66

#define LARGEST_DEVICE_NAME 120
#define PWM_DIR "/sys/devices/platform/ast_pwm_tacho.0"
#define PWM_UNIT_MAX 96

#define HSC_IN_VOLT "in1_input"
#define HSC_OUT_CURR "curr1_input"

#define RISER_BUS_ID 0x1

#define READING_NA -2
#define READING_SKIP 1

#define NIC_MAX_TEMP 125

#define MAX_READ_RETRY 10
#define POST_CODE_FILE       "/sys/devices/platform/ast-snoop-dma.0/data_history"

#define CPLD_BUS_ID 0x6
#define CPLD_ADDR 0xA0

static size_t pal_pwm_cnt = 2;

static int key_func_por_policy (int event, void *arg);
static int key_func_lps (int event, void *arg);
static int key_func_tz (int event, void *arg);

static uint8_t power_fail_log = 0;

enum key_event {
  KEY_BEFORE_SET,
  KEY_AFTER_INI,
};

struct pal_key_cfg {
  char *name;
  char *def_val;
  int (*function)(int, void*);
} key_cfg[] = {
  /* name, default value, function */
  {"pwr_server_last_state", "on", key_func_lps},
  {"sysfw_ver_server", "0", NULL},
  {"identify_sled", "off", NULL},
  {"timestamp_sled", "0", NULL},
  {"server_por_cfg", "lps", key_func_por_policy},
  {"server_sensor_health", "1", NULL},
  {"nic_sensor_health", "1", NULL},
  {"server_sel_error", "1", NULL},
  {"server_boot_order", "0000000", NULL},
  {"ntp_server", "", NULL},
  {"time_zone", "UTC", key_func_tz},
  /* Add more Keys here */
  {LAST_KEY, LAST_KEY, NULL} /* This is the last key of the list */
};

typedef struct _inlet_corr_t {
  uint8_t duty;
  int8_t delta_t;
} inlet_corr_t;

static inlet_corr_t g_ict[] = {
  { 10, 7 },
  { 12, 6 },
  { 14, 5 },
  { 18, 4 },
  { 20, 3 },
  { 24, 2 },
  { 32, 1 },
  { 41, 0 },
};

static uint8_t g_ict_count = sizeof(g_ict)/sizeof(inlet_corr_t);

static void _print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t val, char *event);

void pal_apply_inlet_correction(float *value) {
  static int8_t dt = 0;
  int i;
  uint8_t pwm[2] = {0};

  // Get PWM value
  if (pal_get_pwm_value(0, &pwm[0]) || pal_get_pwm_value(1, &pwm[1])) {
    // If error reading PWM value, use the previous deltaT
    *value -= dt;
    return;
  }
  pwm[0] = (pwm[0] + pwm[1]) /2;

  // Scan through the correction table to get correction value for given PWM
  dt=g_ict[0].delta_t;
  for (i=0; i< g_ict_count; i++) {
    if (pwm[0] >= g_ict[i].duty)
      dt = g_ict[i].delta_t;
    else
      break;
  }

  // Apply correction for the sensor
  *(float*)value -= dt;
}

static int
pal_control_mux(int fd, uint8_t addr, uint8_t channel) {
  uint8_t tcount = 1, rcount = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // PCA9544A
  if (channel < 4)
    tbuf[0] = 0x04 + channel;
  else
    tbuf[0] = 0x00; // close all channels

  return i2c_rdwr_msg_transfer(fd, addr, tbuf, tcount, rbuf, rcount);
}

static int
pal_control_switch(int fd, uint8_t addr, uint8_t channel) {
  uint8_t tcount = 1, rcount = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  // PCA9846
  if (channel < 4)
    tbuf[0] = 0x01 << channel;
  else
    tbuf[0] = 0x00; // close all channels

  return i2c_rdwr_msg_transfer(fd, addr, tbuf, tcount, rbuf, rcount);
}

static int
read_device(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
    int err = errno;
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return err;
  }

  rc = fscanf(fp, "%d", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

static int
read_device_hex(const char *device, int *value) {
  FILE *fp;
  int rc;

  fp = fopen(device, "r");
  if (!fp) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to open device %s", device);
#endif
    return errno;
  }

  rc = fscanf(fp, "%x", value);
  fclose(fp);
  if (rc != 1) {
#ifdef DEBUG
    syslog(LOG_INFO, "failed to read device %s", device);
#endif
    return ENOENT;
  } else {
    return 0;
  }
}

int
pal_read_hsc_current_value(float *value) {
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  float hsc_b = 20475;
  float Rsence;
  ipmb_req_t *req;
  char path[64] = {0};
  int val=0;
  int ret = 0;
  static int retry = 0;

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  req->cmd = CMD_NM_SEND_RAW_PMBUS;
  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = 0x86;
  //HSC slave addr check for SS and DS
  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID4);
  read_device(path, &val);
  if (val){ //DS
    req->data[4] = 0x8A;
    Rsence = 0.265;
  }else{    //SS
    req->data[4] = 0x22;
    Rsence = 0.505;
  }
  req->data[5] = 0x00;
  req->data[6] = 0x00;
  req->data[7] = 0x01;
  req->data[8] = 0x02;
  req->data[9] = 0x8C;
  tlen = 16;

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf, &rlen);

  if (rlen == 0) {
#ifdef DEBUG
    syslog(LOG_DEBUG, "read_hsc_current_value: Zero bytes received\n");
#endif
    ret = READING_NA;
  }
  if (rbuf[6] == 0)
  {
    *value = ((float) (rbuf[11] << 8 | rbuf[10])*10-hsc_b )/(800*Rsence);
    retry = 0;
  } else {
    ret = READING_NA;
  }

  if (ret == READING_NA) {
    retry++;
    if (retry <= 3 )
      ret = READING_SKIP;
  }

  return ret;
}

int
pal_read_sensor_reading_from_ME(uint8_t snr_num, float *value) {
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  int ret = 0;
  enum {
    e_HSC_PIN,
    e_HSC_VIN,
    e_PCH_TEMP,
    e_MAX,
  };
  static uint8_t retry[e_MAX] = {0};

  req = (ipmb_req_t*)tbuf;
  req->res_slave_addr = 0x2C; //ME's Slave Address
  req->netfn_lun = NETFN_SENSOR_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;
  req->cmd = CMD_SENSOR_GET_SENSOR_READING;
  req->data[0] = snr_num;
  tlen = 7;

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf, &rlen);

  if (rlen == 0) {
  //ME no response
#ifdef DEBUG
    syslog(LOG_DEBUG, "read HSC %x from_ME: Zero bytes received\n", snr_num);
#endif
   ret = READING_NA;
  } else {
    if (rbuf[6] == 0)
    {
        if (rbuf[8] & 0x20) {
          //not available
          ret = READING_NA;
        }
    } else {
      ret = READING_NA;
    }
  }

  if(snr_num == MB_SENSOR_HSC_IN_POWER) {
    if (!ret) {
      *value = (((float) rbuf[7])*0x28 + 0 )/10 ;
      retry[e_HSC_PIN] = 0;
    } else {
      retry[e_HSC_PIN]++;
      if (retry[e_HSC_PIN] <= 3)
        ret = READING_SKIP;
    }
  } else if(snr_num == MB_SENSOR_HSC_IN_VOLT) {
    if (!ret) {
      *value = (((float) rbuf[7])*0x02 + (0x5e*10) )/100 ;
      retry[e_HSC_VIN] = 0;
    } else {
      retry[e_HSC_VIN]++;
      if (retry[e_HSC_VIN] <= 3)
        ret = READING_SKIP;
    }
  } else if(snr_num == MB_SENSOR_PCH_TEMP) {
    if (!ret) {
      *value = (float) rbuf[7];
      retry[e_PCH_TEMP] = 0;
    } else {
      retry[e_PCH_TEMP]++;
      if (retry[e_PCH_TEMP] <= 3)
        ret = READING_SKIP;
    }
  }
  return ret;
}

int
pal_read_cpu_temp(uint8_t snr_num, float *value) {
  int ret = 0;
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf1[256] = {0x00};
  static uint8_t tjmax[2] = {0x00};
  static uint8_t tjmax_flag[2] = {0};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  int cpu_index;
  int16_t dts;
  static uint8_t retry[2] = {0x00}; // CPU0 and CPU1

  switch (snr_num) {
    case MB_SENSOR_CPU0_TEMP:
      cpu_index = 0;
      break;
    case MB_SENSOR_CPU1_TEMP:
      cpu_index = 1;
      break;
    default:
      return -1;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address

  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  if( tjmax_flag[cpu_index] == 0 ) { // First time to get CPU0/CPU1 Tjmax reading
    //Get CPU0/CPU1 Tjmax
    req->cmd = CMD_NM_SEND_RAW_PECI;
    req->data[0] = 0x57;
    req->data[1] = 0x01;
    req->data[2] = 0x00;
    req->data[3] = 0x30 + cpu_index;
    req->data[4] = 0x05;
    req->data[5] = 0x05;
    req->data[6] = 0xa1;
    req->data[7] = 0x00;
    req->data[8] = 0x10;
    req->data[9] = 0x00;
    req->data[10] = 0x00;
    tlen = 17;
    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf1, &rlen);
    if (rlen == 0) {
    //ME no response
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    } else {
      if (rbuf1[6] == 0)
      {
        // If PECI command successes and got a reasonable value
        if ( (rbuf1[10] == 0x40) && rbuf1[13] > 50) {
          tjmax[cpu_index] = rbuf1[13];
          tjmax_flag[cpu_index] = 1;
        }
      }
    }
  }

  //Updated CPU Tjmax cache
  sprintf(key, "mb_sensor%d", (cpu_index?MB_SENSOR_CPU1_TJMAX:MB_SENSOR_CPU0_TJMAX));
  if (tjmax_flag[cpu_index] != 0) {
    sprintf(str, "%.2f",(float) tjmax[cpu_index]);
  } else {
    //ME no response or PECI command completion code error. Set "NA" in sensor cache.
    strcpy(str, "NA");
  }
  kv_set(key, str, 0, 0);

  // Get CPU temp if BMC got TjMax
  ret = READING_NA;
  if (tjmax_flag[cpu_index] != 0) {
    rlen = 0;
    memset( rbuf1,0x00,sizeof(rbuf1) );
    //Get CPU Temp
    req->cmd = CMD_NM_SEND_RAW_PECI;
    req->data[0] = 0x57;
    req->data[1] = 0x01;
    req->data[2] = 0x00;
    req->data[3] = 0x30 + cpu_index;
    req->data[4] = 0x05;
    req->data[5] = 0x05;
    req->data[6] = 0xa1;
    req->data[7] = 0x00;
    req->data[8] = 0x02;
    req->data[9] = 0xff;
    req->data[10] = 0x00;
    tlen = 17;

    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf1, &rlen);

    if (rlen == 0) {
      //ME no response
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    } else {
      if (rbuf1[6] == 0) { // ME Completion Code
        if ( (rbuf1[10] == 0x40) ) { // PECI Completion Code
          dts = (rbuf1[11] | rbuf1[12] << 8);
          // Intel Doc#554767 p.58: Reserved Values 0x8000~0x81ff
          if (dts <= -32257) {
            ret = READING_NA;
          } else {
            // 16-bit, 2s complement [15]Sign Bit;[14:6]Integer Value;[5:0]Fractional Value
            *value = (float) (tjmax[0] + (dts >> 6));
            ret = 0;
          }
        }
      }
    }
  }

  if (ret != 0) {
    retry[cpu_index]++;
    if (retry[cpu_index] <= 3) {
      ret = READING_SKIP;
      return ret;
    }
  } else
    retry[cpu_index] = 0;

  return ret;
}

static int
pal_get_platform_id(uint8_t *id) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID0);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = val&0x01;

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID1);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<1);

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID2);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<2);

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID3);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<3);

  sprintf(path, GPIO_VAL, GPIO_BOARD_SKU_ID4);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<4);

  return 0;
}

int
pal_read_dimm_temp(uint8_t snr_num, float *value) {
  int ret = READING_NA;
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf1[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  int dimm_index, i;
  int max = 0;
  static uint8_t retry[4] = {0x00};
  int val;
  char path[64] = {0};
  static int odm_id = -1;
  uint8_t BoardInfo;

  //Use FM_BOARD_SKU_ID0 to identify ODM to apply filter
  if (odm_id == -1) {
    ret = pal_get_platform_id(&BoardInfo);
    if (ret == 0) {
      odm_id = (int) (BoardInfo & 0x1);
    }
  }

  // show NA if BIOS has not completed POST.
  sprintf(path, GPIO_VAL, GPIO_FM_BIOS_POST_CMPLT_N);
  if (read_device(path, &val) || val) {
    return ret;
  }

  switch (snr_num) {
    case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
      dimm_index = 0;
      break;
    case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
      dimm_index = 1;
      break;
    case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
      dimm_index = 2;
      break;
    case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
      dimm_index = 3;
      break;
    default:
      return -1;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address

  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  for (i=0; i<3; i++) { // Get 3 channel for each DIMM group
    //Get DIMM Temp per channel
    req->cmd = CMD_NM_SEND_RAW_PECI;
    req->data[0] = 0x57;
    req->data[1] = 0x01;
    req->data[2] = 0x00;
    req->data[3] = 0x30 + (dimm_index / 2);
    req->data[4] = 0x05;
    req->data[5] = 0x05;
    req->data[6] = 0xa1;
    req->data[7] = 0x00;
    req->data[8] = 0x0e;
    req->data[9] = 0x00 + (dimm_index % 2 * 3) + i;
    req->data[10] = 0x00;
    tlen = 17;

    // Invoke IPMB library handler
    lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf1, &rlen);
    if (rlen == 0) {
    //ME no response
#ifdef DEBUG
      syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    } else {
      if (rbuf1[6] == 0)
      {
        // If PECI command successes
        if ( (rbuf1[10] == 0x40)) {
          if (rbuf1[11] > max)
            max = rbuf1[11];
          if (rbuf1[12] > max)
            max = rbuf1[11];
        }
      }
    }
  }

  if (odm_id == 1) {
    // Filter abnormal values: 0x0 and 0xFF
    if (max != 0 && max != 0xFF)
      ret = 0;
  } else {
    // Filter abnormal values: 0x0
    if (max != 0)
      ret = 0;
  }

  if (ret != 0) {
    retry[dimm_index]++;
    if (retry[dimm_index] <= 3) {
      ret = READING_SKIP;
      return ret;
    }
  } else
    retry[dimm_index] = 0;

  if (ret == 0) {
    *value = (float)max;
  }

  return ret;
}

int
pal_read_cpu_package_power(uint8_t snr_num, float *value) {
  int ret = READING_NA;
  uint8_t bus_id = 0x4; //TODO: ME's address 0x2c in FBTP
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf1[256] = {0x00};
  // Energy units: Intel Doc#554767, p37, 2^(-ENERGY UNIT) J, ENERGY UNIT defalut is 14
  // Run Time units: Intel Doc#554767, p33, msec
  // 2^(-14)*1000 = 0.06103515625
  float unit = 0.06103515625f;
  static uint32_t last_pkg_energy[2] = {0}, last_run_time[2] = {0};
  uint32_t pkg_energy, run_time, diff_energy, diff_time;
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  ipmb_req_t *req;
  int cpu_index;
  static uint8_t retry[2] = {0x00}; // CPU0 and CPU1

  switch (snr_num) {
    case MB_SENSOR_CPU0_PKG_POWER:
      cpu_index = 0;
      break;
    case MB_SENSOR_CPU1_PKG_POWER:
      cpu_index = 1;
      break;
    default:
      return -1;
  }

  req = (ipmb_req_t*)tbuf;

  req->res_slave_addr = 0x2C; //ME's Slave Address

  req->netfn_lun = NETFN_NM_REQ<<2;
  req->hdr_cksum = req->res_slave_addr + req->netfn_lun;
  req->hdr_cksum = ZERO_CKSUM_CONST - req->hdr_cksum;

  req->req_slave_addr = 0x20;
  req->seq_lun = 0x00;

  // Get CPU package power and run time
  rlen = 0;
  memset( rbuf1,0x00,sizeof(rbuf1) );
  //Read Accumulated Energy Pkg and Accumulated Run Time
  req->cmd = CMD_NM_AGGREGATED_SEND_RAW_PECI;
  req->data[0] = 0x57;
  req->data[1] = 0x01;
  req->data[2] = 0x00;
  req->data[3] = 0x30 + cpu_index;
  req->data[4] = 0x05;
  req->data[5] = 0x05;
  req->data[6] = 0xa1;
  req->data[7] = 0x00;
  req->data[8] = 0x03;
  req->data[9] = 0xff;
  req->data[10] = 0x00;
  req->data[11] = 0x30 + cpu_index;
  req->data[12] = 0x05;
  req->data[13] = 0x05;
  req->data[14] = 0xa1;
  req->data[15] = 0x00;
  req->data[16] = 0x1F;
  req->data[17] = 0x00;
  req->data[18] = 0x00;
  tlen = 25;

  // Invoke IPMB library handler
  lib_ipmb_handle(bus_id, tbuf, tlen+1, rbuf1, &rlen);

  if (rlen == 0) {
    //ME no response
#ifdef DEBUG
    syslog(LOG_DEBUG, "%s(%d): Zero bytes received\n", __func__, __LINE__);
#endif
    goto error_exit;
  } else {
    if (rbuf1[6] == 0) { // ME Completion Code
      if ( (rbuf1[10] == 0x00) && (rbuf1[11] == 0x40) && // 1st ME CC & PECI CC
           (rbuf1[16] == 0x00) && (rbuf1[17] == 0x40) ){ // 2nd ME CC & PECI CC
        pkg_energy = rbuf1[15];
        pkg_energy = (pkg_energy << 8) | rbuf1[14];
        pkg_energy = (pkg_energy << 8) | rbuf1[13];
        pkg_energy = (pkg_energy << 8) | rbuf1[12];

        run_time = rbuf1[21];
        run_time = (run_time << 8) | rbuf1[20];
        run_time = (run_time << 8) | rbuf1[19];
        run_time = (run_time << 8) | rbuf1[18];

        ret = 0;
      }
    }
  }

  // need at least 2 entries to calculate
  if (last_pkg_energy[cpu_index] == 0 && last_run_time[cpu_index] == 0) {
    if (ret == 0) {
      last_pkg_energy[cpu_index] = pkg_energy;
      last_run_time[cpu_index] = run_time;
    }
    ret = READING_NA;
  }

  if(!ret) {
    if(pkg_energy >= last_pkg_energy[cpu_index])
      diff_energy = pkg_energy - last_pkg_energy[cpu_index];
    else
      diff_energy = pkg_energy + (0xffffffff - last_pkg_energy[cpu_index] + 1);
    last_pkg_energy[cpu_index] = pkg_energy;

    if(run_time >= last_run_time[cpu_index])
      diff_time = run_time - last_run_time[cpu_index];
    else
      diff_time = run_time + (0xffffffff - last_run_time[cpu_index] + 1);
    last_run_time[cpu_index] = run_time;

    if(diff_time == 0)
      ret = READING_NA;
    else
      *value = ((float)diff_energy / (float)diff_time * unit);
  }

error_exit:
  if (ret != 0) {
    retry[cpu_index]++;
    if (retry[cpu_index] <= 3) {
      ret = READING_SKIP;
      return ret;
    }
  } else
    retry[cpu_index] = 0;

  return ret;
}

static int
pal_get_slot_cfg_id(uint8_t *id) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_SLT_CFG0);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = val&0x01;

  sprintf(path, GPIO_VAL, GPIO_SLT_CFG1);
  if (read_device(path, &val)) {
    return -1;
  }
  *id = *id | (val<<1);

   return 0;
}


int
pal_read_ava_temp(uint8_t sensor_num, float *value) {
  int fd = 0;
  char fn[32];
  int ret = READING_NA;;
  static unsigned int retry[6] = {0};
  uint8_t i_retry;
  uint8_t tcount, rcount, slot_cfg, addr, mux_chan, mux_addr = 0xe2;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  if (pal_get_slot_cfg_id(&slot_cfg) < 0)
    slot_cfg = SLOT_CFG_EMPTY;

  switch(sensor_num) {
    case MB_SENSOR_C2_AVA_FTEMP:
      i_retry = 0; break;
    case MB_SENSOR_C2_AVA_RTEMP:
      i_retry = 1; break;
    case MB_SENSOR_C3_AVA_FTEMP:
      i_retry = 2; break;
    case MB_SENSOR_C3_AVA_RTEMP:
      i_retry = 3; break;
    case MB_SENSOR_C4_AVA_FTEMP:
      i_retry = 4; break;
    case MB_SENSOR_C4_AVA_RTEMP:
      i_retry = 5; break;
    default:
      return READING_NA;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_AVA_FTEMP:
    case MB_SENSOR_C2_AVA_RTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 0;
      break;
    case MB_SENSOR_C3_AVA_FTEMP:
    case MB_SENSOR_C3_AVA_RTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 1;
      break;
    case MB_SENSOR_C4_AVA_FTEMP:
    case MB_SENSOR_C4_AVA_RTEMP:
      if(slot_cfg != SLOT_CFG_SS_3x8)
        return READING_NA;
      mux_chan = 2;
      break;
    default:
      return READING_NA;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_AVA_FTEMP:
    case MB_SENSOR_C3_AVA_FTEMP:
    case MB_SENSOR_C4_AVA_FTEMP:
      addr = 0x92;
      break;
    case MB_SENSOR_C2_AVA_RTEMP:
    case MB_SENSOR_C3_AVA_RTEMP:
    case MB_SENSOR_C4_AVA_RTEMP:
      addr = 0x90;
      break;
    default:
      return READING_NA;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // control multiplexer to target channel.
  ret = pal_control_mux(fd, mux_addr, mux_chan);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // Read 2 bytes from TMP75
  tbuf[0] = 0x00;
  tcount = 1;
  rcount = 2;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tcount, rbuf, rcount);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }
  ret = 0;
  retry[i_retry] = 0;

  // rbuf:MSB, LSB; 12-bit value on Bit[15:4], unit: 0.0625
  *value = (float)(signed char)rbuf[0];

error_exit:
  if (fd > 0) {
    pal_control_mux(fd, mux_addr, 0xff); // close
    close(fd);
  }

  if (ret == READING_NA && ++retry[i_retry] <= 3)
    ret = READING_SKIP;

  return ret;
}

int
pal_read_INA230 (uint8_t sensor_num, float *value, int pot) {
  int fd = 0;
  char fn[32];
  int ret = READING_NA;;
  static unsigned int retry[12] = {0};
  uint8_t i_retry = 0;
  uint8_t slot_cfg, addr, mux_chan, mux_addr = 0xe2;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  /*previous_addr - it is used for identifying the slave address.
   *                If the slave address is changed, bus_voltage and shunt_voltage are updated.
   *Rshunt - it is defined by the schematic. It will be 2m ohm in the fbtp
   *
  */
  static uint8_t previous_addr = 0;
  const float Rshunt = 0.002;    // 2m ohm
  static float bus_voltage = 0x0;
  static float shunt_voltage = 0x0;
  static float current = 0x0;
  static float power = 0x0;

  if (pal_get_slot_cfg_id(&slot_cfg) < 0)
    slot_cfg = SLOT_CFG_EMPTY;

  switch(sensor_num) {
    case MB_SENSOR_C2_P12V_INA230_VOL:
      i_retry = 0; break;
    case MB_SENSOR_C2_P12V_INA230_CURR:
      i_retry = 1; break;
    case MB_SENSOR_C2_P12V_INA230_PWR:
      i_retry = 2; break;
    case MB_SENSOR_C3_P12V_INA230_VOL:
      i_retry = 3; break;
    case MB_SENSOR_C3_P12V_INA230_CURR:
      i_retry = 4; break;
    case MB_SENSOR_C3_P12V_INA230_PWR:
      i_retry = 5; break;
    case MB_SENSOR_C4_P12V_INA230_VOL:
      i_retry = 6; break;
    case MB_SENSOR_C4_P12V_INA230_CURR:
      i_retry = 7; break;
    case MB_SENSOR_C4_P12V_INA230_PWR:
      i_retry = 8; break;
    case MB_SENSOR_CONN_P12V_INA230_VOL:
      i_retry = 9; break;
    case MB_SENSOR_CONN_P12V_INA230_CURR:
      i_retry = 10; break;
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      i_retry = 11; break;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_P12V_INA230_VOL:
    case MB_SENSOR_C2_P12V_INA230_CURR:
    case MB_SENSOR_C2_P12V_INA230_PWR:
    case MB_SENSOR_C3_P12V_INA230_VOL:
    case MB_SENSOR_C3_P12V_INA230_CURR:
    case MB_SENSOR_C3_P12V_INA230_PWR:
    case MB_SENSOR_CONN_P12V_INA230_VOL:
    case MB_SENSOR_CONN_P12V_INA230_CURR:
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      break;
    case MB_SENSOR_C4_P12V_INA230_VOL:
    case MB_SENSOR_C4_P12V_INA230_CURR:
    case MB_SENSOR_C4_P12V_INA230_PWR:
      if(slot_cfg != SLOT_CFG_SS_3x8)
        return READING_NA;
      break;
    default:
      return READING_NA;
  }

  //use channel 4
  mux_chan = 0x3;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  //control multiplexer to target channel.
  ret = pal_control_mux(fd, mux_addr, mux_chan);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_P12V_INA230_VOL:
    case MB_SENSOR_C2_P12V_INA230_CURR:
    case MB_SENSOR_C2_P12V_INA230_PWR:
      addr = 0x80;
      break;
    case MB_SENSOR_C3_P12V_INA230_VOL:
    case MB_SENSOR_C3_P12V_INA230_CURR:
    case MB_SENSOR_C3_P12V_INA230_PWR:
      addr = 0x82;
      break;
    case MB_SENSOR_C4_P12V_INA230_VOL:
    case MB_SENSOR_C4_P12V_INA230_CURR:
    case MB_SENSOR_C4_P12V_INA230_PWR:
      addr = 0x88;
      break;
    case MB_SENSOR_CONN_P12V_INA230_VOL:
    case MB_SENSOR_CONN_P12V_INA230_CURR:
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      addr = 0x8A;
      break;
    default:
        syslog(LOG_WARNING, "read_INA230: undefined sensor number") ;
      break;
  }

  //check the previous all INA230 sensors are read and change the addr to the next
  if ( previous_addr != addr )
  {
    //record the current addr
    previous_addr = addr;

    //get the bus voltage
    tbuf[0] = 0x02;
    memset(rbuf, 0, sizeof(rbuf));
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 2);
    if (ret < 0) {
      ret = READING_NA;
      goto error_exit;
    }

    //transform the value from raw data to the reading
    bus_voltage = ((rbuf[1] + rbuf[0]*256) * 0.00125);

    //get the shunt voltage
    tbuf[0] = 0x01;
    memset(rbuf, 0, sizeof(rbuf));
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, 1, rbuf, 2);
    if (ret < 0) {
      ret = READING_NA;
      goto error_exit;
    }

    //If shunt voltage is a negative value, set the shunt_voltage to 0
    if ( BIT(rbuf[0], 7) )
    {
      shunt_voltage = 0;
    }
    else
    {
      shunt_voltage = ((rbuf[1] + rbuf[0]*256) * 0.0000025);
    }

    //get the current according to the shunt_voltage and Rshunt
    current = (shunt_voltage / Rshunt);

    //get the power according to the bus_voltage and current
    power = current * bus_voltage;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_P12V_INA230_VOL:
    case MB_SENSOR_C3_P12V_INA230_VOL:
    case MB_SENSOR_C4_P12V_INA230_VOL:
    case MB_SENSOR_CONN_P12V_INA230_VOL:
      *value = bus_voltage;
      break;
    case MB_SENSOR_C2_P12V_INA230_CURR:
    case MB_SENSOR_C3_P12V_INA230_CURR:
    case MB_SENSOR_C4_P12V_INA230_CURR:
    case MB_SENSOR_CONN_P12V_INA230_CURR:
      *value = current;
      break;
    case MB_SENSOR_C2_P12V_INA230_PWR:
    case MB_SENSOR_C3_P12V_INA230_PWR:
    case MB_SENSOR_C4_P12V_INA230_PWR:
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      *value = power;
      break;
    default:
        syslog(LOG_WARNING, "read_INA230: undefined sensor number") ;
      break;
  }

    ret = 0;
    retry[i_retry] = 0;

error_exit:
  if (fd > 0) {
    pal_control_mux(fd, mux_addr, 0xff); // close
    close(fd);
  }

  if (ret == READING_NA && ++retry[i_retry] <= 3)
    ret = READING_SKIP;

  return ret;
}

int
pal_read_nvme_temp(uint8_t sensor_num, float *value) {
  int fd = 0;
  char fn[32];
  int ret = READING_NA;
  static unsigned int retry[12] = {0};
  uint8_t i_retry;
  uint8_t tcount, rcount, slot_cfg, addr = 0xd4, mux_chan, mux_addr = 0xe2;
  uint8_t switch_chan, switch_addr=0xe6;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  if (pal_get_slot_cfg_id(&slot_cfg) < 0)
    slot_cfg = SLOT_CFG_EMPTY;

  switch(sensor_num) {
    case MB_SENSOR_C2_1_NVME_CTEMP:
      i_retry = 0; break;
    case MB_SENSOR_C2_2_NVME_CTEMP:
      i_retry = 1; break;
    case MB_SENSOR_C2_3_NVME_CTEMP:
      i_retry = 2; break;
    case MB_SENSOR_C2_4_NVME_CTEMP:
      i_retry = 3; break;
    case MB_SENSOR_C3_1_NVME_CTEMP:
      i_retry = 4; break;
    case MB_SENSOR_C3_2_NVME_CTEMP:
      i_retry = 5; break;
    case MB_SENSOR_C3_3_NVME_CTEMP:
      i_retry = 6; break;
    case MB_SENSOR_C3_4_NVME_CTEMP:
      i_retry = 7; break;
    case MB_SENSOR_C4_1_NVME_CTEMP:
      i_retry = 8; break;
    case MB_SENSOR_C4_2_NVME_CTEMP:
      i_retry = 9; break;
    case MB_SENSOR_C4_3_NVME_CTEMP:
      i_retry = 10; break;
    case MB_SENSOR_C4_4_NVME_CTEMP:
      i_retry = 11; break;
    default:
      return READING_NA;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_1_NVME_CTEMP:
    case MB_SENSOR_C2_2_NVME_CTEMP:
    case MB_SENSOR_C2_3_NVME_CTEMP:
    case MB_SENSOR_C2_4_NVME_CTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 0;
      break;
    case MB_SENSOR_C3_1_NVME_CTEMP:
    case MB_SENSOR_C3_2_NVME_CTEMP:
    case MB_SENSOR_C3_3_NVME_CTEMP:
    case MB_SENSOR_C3_4_NVME_CTEMP:
      if(slot_cfg == SLOT_CFG_EMPTY)
        return READING_NA;
      mux_chan = 1;
      break;
    case MB_SENSOR_C4_1_NVME_CTEMP:
    case MB_SENSOR_C4_2_NVME_CTEMP:
    case MB_SENSOR_C4_3_NVME_CTEMP:
    case MB_SENSOR_C4_4_NVME_CTEMP:
      if(slot_cfg != SLOT_CFG_SS_3x8)
        return READING_NA;
      mux_chan = 2;
      break;
    default:
      return READING_NA;
  }

  switch(sensor_num) {
    case MB_SENSOR_C2_1_NVME_CTEMP:
    case MB_SENSOR_C3_1_NVME_CTEMP:
    case MB_SENSOR_C4_1_NVME_CTEMP:
      switch_chan = 0;
      break;
    case MB_SENSOR_C2_2_NVME_CTEMP:
    case MB_SENSOR_C3_2_NVME_CTEMP:
    case MB_SENSOR_C4_2_NVME_CTEMP:
      switch_chan = 1;
      break;
    case MB_SENSOR_C2_3_NVME_CTEMP:
    case MB_SENSOR_C3_3_NVME_CTEMP:
    case MB_SENSOR_C4_3_NVME_CTEMP:
      switch_chan = 2;
      break;
    case MB_SENSOR_C2_4_NVME_CTEMP:
    case MB_SENSOR_C3_4_NVME_CTEMP:
    case MB_SENSOR_C4_4_NVME_CTEMP:
      switch_chan = 3;
      break;
    default:
      return READING_NA;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", RISER_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // control I2C multiplexer to target channel.
  ret = pal_control_mux(fd, mux_addr, mux_chan);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // control I2C switch to target channel.
  ret = pal_control_switch(fd, switch_addr, switch_chan);
  // Report temp of PCIe card on MB_SENSOR_CX_1_NVME_CTEMP senosrs,
  // no I2C Switch on PCIe Card
  if (ret < 0 && switch_chan != 0) {
    ret = READING_NA;
    goto error_exit;
  }

  // Read 8 bytes from NVMe
  tbuf[0] = 0x00;
  tcount = 1;
  rcount = 8;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tcount, rbuf, rcount);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }
  ret = 0;
  retry[i_retry] = 0;

  // Cmd 0: length, SFLGS, SMART Warnings, CTemp, PDLU, Reserved, Reserved, PEC
  *value = (float)(signed char)rbuf[3];

error_exit:
  if (fd > 0) {
    pal_control_switch(fd, switch_addr, 0xff); // close
    pal_control_mux(fd, mux_addr, 0xff); // close
    close(fd);
  }

  if (ret == READING_NA && ++retry[i_retry] <= 3)
    ret = READING_SKIP;

  return ret;
}

static void
pal_add_cri_sel(char *str)
{
  char cmd[128];
  snprintf(cmd, 128, "logger -p local0.err \"%s\"",str);
  system(cmd);
}

static void
add_CPLD_event (uint8_t fru, uint8_t snr_num, uint8_t reg, uint8_t snr_val, uint8_t value) {
  char sensor_name[32] = {0}, event_str[30] = {0};
  pal_get_sensor_name(fru, snr_num, sensor_name);

  strcpy(event_str, "");
  switch(reg) {
      case PWRDATA1_REG :
	 if (value == 0x40)
	   strcat(event_str, "FM_CTNR_PS_ON power rail fails");
	 else if (value == 0x00)
	   strcat(event_str, "PWRGD_P12V_MAIN power rail fails");
	 else if (value == 0xc0)
	   strcat(event_str, "PWRGD_P5V power rail fails");
	 else if (value == 0xe0)
	   strcat(event_str, "PWRGD_P3V3 power rail fails");
	 else if (value == 0xf7)
	   strcat(event_str, "PWRGD_PVPP_ABC power rail fails");
	 else if (value == 0xfb)
	   strcat(event_str, "PWRGD_PVPP_DEF power rail fails");
	 else if (value == 0xfd)
	   strcat(event_str, "PWRGD_PVPP_GHJ power rail fails");
	 else if (value == 0xfe)
	   strcat(event_str, "PWRGD_PVPP_KLM power rail fails");
	 else
	   strcat(event_str, "Unknown power rail fails(PWRDATA1)");
        break;
      case PWRDATA2_REG :
	 if (value == 0x55)
	   strcat(event_str, "PWRGD_PVTT_CPU0 power rail fails");
	 else if (value == 0xaa)
	   strcat(event_str, "PWRGD_PVTT_CPU1 power rail fails");
	 else if (value == 0xd5)
	   strcat(event_str, "PWRGD_PVCCIO_CPU0 power rail fails");
	 else if (value == 0xea)
	   strcat(event_str, "PWRGD_PVCCIO_CPU1 power rail fails");
	 else if (value == 0xf7)
	   strcat(event_str, "PWRGD_PVCCIN_CPU0 power rail fails");
	 else if (value == 0xfb)
	   strcat(event_str, "PWRGD_PVCCIN_CPU1 power rail fails");
	 else if (value == 0xfd)
	   strcat(event_str, "PWRGD_PVSA_CPU0 power rail fails");
	 else if (value == 0xfe)
	   strcat(event_str, "PWRGD_PVSA_CPU1 power rail fails");
	 else
	   strcat(event_str, "Unknown power rail fails(PWRDATA2)");
        break;
      case PWRDATA3_REG :
	 if (value == 0x40)
	   strcat(event_str, "PWRGD_CPUPWRGD power rail fails");
	 else if (value == 0x80)
	   strcat(event_str, "RST_PLTRST_N power rail fails");
	 else
	   strcat(event_str, "Unknown power rail fails(PWRDATA3)");
        break;
    }
    if(power_fail_log == 0){
      _print_sensor_discrete_log(fru, snr_num, sensor_name, reg, event_str);
      pal_add_cri_sel(event_str);
      power_fail_log = 1;
    }
}

int
pal_read_CPLD_power_fail_sts (uint8_t fru, uint8_t sensor_num, float *value, int pot) {
  int fd = 0;
  char fn[32];
  int ret = READING_NA, i;
  static unsigned int retry=0;
  static uint8_t power_fail = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0}, data_chk;
  uint8_t sensor_value;
  int val;
  char path[64] = {0};

  //Check SLPS4 is high before start monitor CPLD power fail
  sprintf(path, GPIO_VAL, GPIO_FM_SLPS4_N);
  if (read_device(path, &val)) {
    goto error_exit;
  }
  if (val == 0x0) {
    power_fail = 0;
    goto error_exit;
  }

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", CPLD_BUS_ID);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto error_exit;
  }

  for(i=0;i<3;i++) {
    switch(i) {
      case MAIN_PWR_STS_REG :
        data_chk = MAIN_PWR_STS_VAL;
        break;
      case CPU0_PWR_STS_REG :
        data_chk = CPU0_PWR_STS_VAL;
        break;
      case CPU1_PWR_STS_REG :
        data_chk = CPU1_PWR_STS_VAL;
        break;
    }

    tbuf[0] = i;
    ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDR, tbuf, 1, rbuf, 1);
    if (ret < 0) {
      ret = READING_NA;
      goto error_exit;
    }
    if ( rbuf[0] != data_chk ) {
      power_fail++;
      break;
    }
  }

  if(power_fail <= 3) {
    ret = 0;
    *value = 0;
    power_fail_log = 0;
  } else {
    for(i=3;i<6;i++) {
      switch(i) {
        case PWRDATA1_REG :
          data_chk = PWRDATA1_VAL;
          break;
        case PWRDATA2_REG :
          data_chk = PWRDATA2_VAL;
          break;
        case PWRDATA3_REG :
          data_chk = PWRDATA3_VAL;
          break;
      }
      // Read 1 byte in offset 00h
      tbuf[0] = i;
      ret = i2c_rdwr_msg_transfer(fd, CPLD_ADDR, tbuf, 1, rbuf, 1);
      if (ret < 0) {
        ret = READING_NA;
        goto error_exit;
      }
      if ( (rbuf[0] != data_chk) && (power_fail > 3)) {
        sensor_value = 0x01<<i;
        *value = sensor_value;
        add_CPLD_event(fru, sensor_num, i , sensor_value, rbuf[0]);
        break;
      }
    }
  }

error_exit:
  if (fd > 0) {
    close(fd);
  }
  if ((ret == READING_NA) && (retry < MAX_READ_RETRY)){
    ret = READING_SKIP;
    retry++;
  } else {
    retry = 0;
  }

  return ret;
}

static int
pal_key_index(char *key) {

  int i;

  i = 0;
  while(strcmp(key_cfg[i].name, LAST_KEY)) {

    // If Key is valid, return success
    if (!strcmp(key, key_cfg[i].name))
      return i;

    i++;
  }

#ifdef DEBUG
  syslog(LOG_WARNING, "pal_key_index: invalid key - %s", key);
#endif
  return -1;
}

int
pal_set_key_value(char *key, char *value) {
  int index, ret;
  // Check is key is defined and valid
  if ((index = pal_key_index(key)) < 0)
    return -1;

  if (key_cfg[index].function) {
    ret = key_cfg[index].function(KEY_BEFORE_SET, value);
    if (ret < 0)
      return ret;
  }

  return kv_set(key, value, 0, KV_FPERSIST);
}

static int
key_func_por_policy (int event, void *arg)
{
  char cmd[MAX_VALUE_LEN];
  char value[MAX_VALUE_LEN];

  switch (event) {
    case KEY_BEFORE_SET:
      if (pal_is_fw_update_ongoing(FRU_MB))
        return -1;
      // sync to env
      snprintf(cmd, MAX_VALUE_LEN, "/sbin/fw_setenv por_policy %s", (char *)arg);
      system(cmd);
      break;
    case KEY_AFTER_INI:
      // sync to env
      kv_get("server_por_cfg", value, NULL, KV_FPERSIST);
      snprintf(cmd, MAX_VALUE_LEN, "/sbin/fw_setenv por_policy %s", value);
      system(cmd);
      break;
  }

  return 0;
}

static int
key_func_lps (int event, void *arg)
{
  char cmd[MAX_VALUE_LEN];
  char value[MAX_VALUE_LEN];

  switch (event) {
    case KEY_BEFORE_SET:
      if (pal_is_fw_update_ongoing(FRU_MB))
        return -1;
      snprintf(cmd, MAX_VALUE_LEN, "/sbin/fw_setenv por_ls %s", (char *)arg);
      system(cmd);
      break;
    case KEY_AFTER_INI:
      kv_get("pwr_server_last_state", value, NULL, KV_FPERSIST);
      snprintf(cmd, MAX_VALUE_LEN, "/sbin/fw_setenv por_ls %s", value);
      system(cmd);
      break;
  }

  return 0;
}

static int
key_func_tz (int event, void *arg)
{
  char cmd[MAX_VALUE_LEN];
  char timezone[MAX_VALUE_LEN];
  char path[MAX_VALUE_LEN];

  switch (event) {
    case KEY_BEFORE_SET:
      snprintf(timezone, MAX_VALUE_LEN, "%s", (char *)arg);
      snprintf(path, MAX_VALUE_LEN, "/usr/share/zoneinfo/%s", (char *)arg);
      if( access(path, F_OK) != -1 ) {
        snprintf(cmd, MAX_VALUE_LEN, "echo %s > /etc/timezone", timezone);
        system(cmd);
        snprintf(cmd, MAX_VALUE_LEN, "ln -fs %s /etc/localtime", path);
        system(cmd);
      } else {
        return -1;
      }
      break;
    case KEY_AFTER_INI:
      break;
  }

  return 0;
}

int
pal_get_server_power(uint8_t fru, uint8_t *status) {
  int val;
  char path[64] = {0};

  sprintf(path, GPIO_VAL, GPIO_POWER_GOOD);

  if (read_device(path, &val)) {
    return -1;
  }

  if (val == 0x0) {
    *status = 0;
  } else {
    *status = 1;
  }

  return 0;
}

static bool
is_server_off(void) {
  int ret;
  uint8_t status;
  ret = pal_get_server_power(FRU_MB, &status);
  if (ret) {
    return false;
  }

  if (status == SERVER_POWER_OFF) {
    return true;
  } else {
    return false;
  }
}

int
pal_check_postcodes(uint8_t fru_id, uint8_t sensor_num, float *value) {
  static int log_asserted = 0;
  const int loop_threshold = 3;
  const int longest_loop_code = 4;
  int i, nearest_00, loop_count, check_until;
  size_t len;
  uint8_t buff[256];
  uint8_t location, maj_err, min_err, mem_train_fail;
  int ret = READING_NA, rc;
  static unsigned int retry=0;
  char sensor_name[32] = {0};
  char str[32] = {0};

  if (fru_id != 1) {
    syslog(LOG_ERR, "Not Supported Operation for fru %d", fru_id);
    goto error_exit;
  }

  if (is_server_off()) {
    log_asserted = 0;
    goto error_exit;
  }

  len = 0; // clear higher bits
  rc = pal_get_80port_record(FRU_MB, buff, sizeof(buff), &len);
  if (rc != PAL_EOK)
    goto error_exit;

  mem_train_fail = 0;
  loop_count = 0;
  check_until = len - (longest_loop_code * (loop_threshold+1) );
  // Check post code from tail
  for(i = len - 1; i >= 0 && i >= check_until; i--) {
    if (buff[i] == 0x00) {
      if (loop_count < loop_threshold) {
        // found 00
        loop_count++;
        nearest_00 = i;
        continue;
      } else {
        // found (loop_threshold+1)-th 00 from tail
        if (!memcmp(&buff[i], &buff[nearest_00], len - nearest_00)) {
          // PostCode looping over loop_threshold times
          if ((nearest_00 - i) == 4) {
            // Loop Convention1
            mem_train_fail = 1;
            location = buff[i+1];
            maj_err = buff[i+2];
            min_err = buff[i+3];
          }
          if ((nearest_00 - i) == 3) {
            // Loop Convention2
            mem_train_fail = 1;
            location = 0x00;
            maj_err = buff[i+1];
            min_err = buff[i+2];
          }
        }
        // break after 2nd 00
        break;
      }
    }
  }

  if (mem_train_fail) {
    if (!log_asserted) {
      pal_get_sensor_name(fru_id, sensor_num, sensor_name);
      if (location) {
        snprintf(str, sizeof(str), "Location:%02X Err:%02X %02X",location, maj_err, min_err);
        _print_sensor_discrete_log(fru_id, sensor_num, sensor_name, 0x01, str);
        snprintf(str, sizeof(str), "DIMM %02X initial fails",location);
        pal_add_cri_sel(str);
        //syslog(LOG_CRIT, "Memory training failure at %02X MajErr:%02X, MinErr:%02X", location, maj_err, min_err);
      } else {
        snprintf(str, sizeof(str), "Location Unknown Err:%02X %02X", maj_err, min_err);
        _print_sensor_discrete_log(fru_id, sensor_num, sensor_name, 0x01, str);
        //syslog(LOG_CRIT, "Memory training failure MajErr:%02X, MinErr:%02X", maj_err, min_err);
        snprintf(str, sizeof(str), "DIMM XX initial fails");
        pal_add_cri_sel(str);
      }
    }
    log_asserted = 1;
  }
  else
  {
    log_asserted = 0;
  }
  *value = (float)log_asserted;
  ret = 0;

error_exit:
  if ((ret == READING_NA) && (retry < MAX_READ_RETRY)){
    ret = READING_SKIP;
    retry++;
  } else {
    retry = 0;
  }

  return ret;
}

int
pal_check_frb3(uint8_t fru_id, uint8_t sensor_num, float *value) {
  static unsigned int retry = 0;
  static uint8_t frb3_fail = 0x10; // bit 4: FRB3 failure
  static time_t rst_time = 0;
  static uint8_t postcodes_last[256] = {0};
  uint8_t postcodes[256] = {0};
  struct stat file_stat;
  int ret = READING_NA, rc, len;
  char sensor_name[32] = {0};
  char error[32] = {0};

  if (fru_id != 1) {
    syslog(LOG_ERR, "Not Supported Operation for fru %d", fru_id);
    return READING_NA;
  }

  if (stat("/tmp/rst_touch", &file_stat) == 0 && file_stat.st_mtime > rst_time) {
    rst_time = file_stat.st_mtime;
    // assume fail till we know it is not
    frb3_fail = 0x10; // bit 4: FRB3 failure
    retry = 0;
    // cache current postcode buffer
    memset(postcodes_last, 0, sizeof(postcodes_last));
    pal_get_80port_record(FRU_MB, postcodes_last, sizeof(postcodes_last), &len);
  }

  if (frb3_fail) {
    // KCS transaction
    if (stat("/tmp/kcs_touch", &file_stat) == 0 && file_stat.st_mtime > rst_time)
      frb3_fail = 0;

    // Port 80 updated
    memset(postcodes, 0, sizeof(postcodes_last));
    rc = pal_get_80port_record(FRU_MB, postcodes, sizeof(postcodes), &len);
    if (rc == PAL_EOK && memcmp(postcodes_last, postcodes, 256) != 0) {
      frb3_fail = 0;
    }

    // BIOS POST COMPLT, in case BMC reboot when system idle in OS
    if (gpio_get(GPIO_FM_BIOS_POST_CMPLT_N) == GPIO_VALUE_LOW)
      frb3_fail = 0;
  }

  if (frb3_fail)
    retry++;
  else
    retry = 0;

  if (retry == MAX_READ_RETRY) {
    pal_get_sensor_name(fru_id, sensor_num, sensor_name);
    snprintf(error, sizeof(error), "FRB3 failure");
    _print_sensor_discrete_log(fru_id, sensor_num, sensor_name, frb3_fail, error);
  }

  *value = (float)frb3_fail;
  ret = 0;

  return ret;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
  case FRU_MB:
    switch(sensor_num) {
    case MB_SENSOR_INLET_TEMP:
      sprintf(name, "MB_INLET_TEMP");
      break;
    case MB_SENSOR_OUTLET_TEMP:
      sprintf(name, "MB_OUTLET_TEMP");
      break;
    case MB_SENSOR_INLET_REMOTE_TEMP:
      sprintf(name, "MB_INLET_REMOTE_TEMP");
      break;
    case MB_SENSOR_OUTLET_REMOTE_TEMP:
      sprintf(name, "MB_OUTLET_REMOTE_TEMP");
      break;
    case MB_SENSOR_FAN0_TACH:
      sprintf(name, "MB_FAN0_TACH");
      break;
    case MB_SENSOR_FAN1_TACH:
      sprintf(name, "MB_FAN1_TACH");
      break;
    case MB_SENSOR_P3V3:
      sprintf(name, "MB_P3V3");
      break;
    case MB_SENSOR_P5V:
      sprintf(name, "MB_P5V");
      break;
    case MB_SENSOR_P12V:
      sprintf(name, "MB_P12V");
      break;
    case MB_SENSOR_P1V05:
      sprintf(name, "MB_P1V05");
      break;
    case MB_SENSOR_PVNN_PCH_STBY:
      sprintf(name, "MB_PVNN_PCH_STBY");
      break;
    case MB_SENSOR_P3V3_STBY:
      sprintf(name, "MB_P3V3_STBY");
      break;
    case MB_SENSOR_P5V_STBY:
      sprintf(name, "MB_P5V_STBY");
      break;
    case MB_SENSOR_P3V_BAT:
      sprintf(name, "MB_P3V_BAT");
      break;
    case MB_SENSOR_HSC_IN_VOLT:
      sprintf(name, "MB_HSC_IN_VOLT");
      break;
    case MB_SENSOR_HSC_OUT_CURR:
      sprintf(name, "MB_HSC_OUT_CURR");
      break;
    case MB_SENSOR_HSC_IN_POWER:
      sprintf(name, "MB_HSC_IN_POWER");
      break;
    case MB_SENSOR_CPU0_TEMP:
      sprintf(name, "MB_CPU0_TEMP");
      break;
    case MB_SENSOR_CPU0_TJMAX:
      sprintf(name, "MB_CPU0_TJMAX");
      break;
    case MB_SENSOR_CPU0_PKG_POWER:
      sprintf(name, "MB_CPU0_PKG_POWER");
      break;
    case MB_SENSOR_CPU1_TEMP:
      sprintf(name, "MB_CPU1_TEMP");
      break;
    case MB_SENSOR_CPU1_TJMAX:
      sprintf(name, "MB_CPU1_TJMAX");
      break;
    case MB_SENSOR_CPU1_PKG_POWER:
      sprintf(name, "MB_CPU1_PKG_POWER");
      break;
    case MB_SENSOR_PCH_TEMP:
      sprintf(name, "MB_PCH_TEMP");
      break;
    case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
      sprintf(name, "MB_CPU0_DIMM_GRPA_TEMP");
      break;
    case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
      sprintf(name, "MB_CPU0_DIMM_GRPB_TEMP");
      break;
    case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
      sprintf(name, "MB_CPU1_DIMM_GRPC_TEMP");
      break;
    case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
      sprintf(name, "MB_CPU1_DIMM_GRPD_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_TEMP:
      sprintf(name, "MB_VR_CPU0_VCCIN_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_CURR:
      sprintf(name, "MB_VR_CPU0_VCCIN_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_VOLT:
      sprintf(name, "MB_VR_CPU0_VCCIN_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VCCIN_POWER:
      sprintf(name, "MB_VR_CPU0_VCCIN_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VSA_TEMP:
      sprintf(name, "MB_VR_CPU0_VSA_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VSA_CURR:
      sprintf(name, "MB_VR_CPU0_VSA_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VSA_VOLT:
      sprintf(name, "MB_VR_CPU0_VSA_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VSA_POWER:
      sprintf(name, "MB_VR_CPU0_VSA_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_TEMP:
      sprintf(name, "MB_VR_CPU0_VCCIO_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_CURR:
      sprintf(name, "MB_VR_CPU0_VCCIO_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_VOLT:
      sprintf(name, "MB_VR_CPU0_VCCIO_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VCCIO_POWER:
      sprintf(name, "MB_VR_CPU0_VCCIO_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_TEMP:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_CURR:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_VOLT:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPA_POWER:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPA_POWER");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_TEMP:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_TEMP");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_CURR:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_CURR");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_VOLT:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_VOLT");
      break;
    case MB_SENSOR_VR_CPU0_VDDQ_GRPB_POWER:
      sprintf(name, "MB_VR_CPU0_VDDQ_GRPB_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_TEMP:
      sprintf(name, "MB_VR_CPU1_VCCIN_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_CURR:
      sprintf(name, "MB_VR_CPU1_VCCIN_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_VOLT:
      sprintf(name, "MB_VR_CPU1_VCCIN_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VCCIN_POWER:
      sprintf(name, "MB_VR_CPU1_VCCIN_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VSA_TEMP:
      sprintf(name, "MB_VR_CPU1_VSA_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VSA_CURR:
      sprintf(name, "MB_VR_CPU1_VSA_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VSA_VOLT:
      sprintf(name, "MB_VR_CPU1_VSA_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VSA_POWER:
      sprintf(name, "MB_VR_CPU1_VSA_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_TEMP:
      sprintf(name, "MB_VR_CPU1_VCCIO_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_CURR:
      sprintf(name, "MB_VR_CPU1_VCCIO_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_VOLT:
      sprintf(name, "MB_VR_CPU1_VCCIO_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VCCIO_POWER:
      sprintf(name, "MB_VR_CPU1_VCCIO_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_TEMP:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_CURR:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_VOLT:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPC_POWER:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPC_POWER");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_TEMP:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_TEMP");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_CURR:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_CURR");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_VOLT:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_VOLT");
      break;
    case MB_SENSOR_VR_CPU1_VDDQ_GRPD_POWER:
      sprintf(name, "MB_VR_CPU1_VDDQ_GRPD_POWER");
      break;
    case MB_SENSOR_VR_PCH_PVNN_TEMP:
      sprintf(name, "MB_VR_PCH_PVNN_TEMP");
      break;
    case MB_SENSOR_VR_PCH_PVNN_CURR:
      sprintf(name, "MB_VR_PCH_PVNN_CURR");
      break;
    case MB_SENSOR_VR_PCH_PVNN_VOLT:
      sprintf(name, "MB_VR_PCH_PVNN_VOLT");
      break;
    case MB_SENSOR_VR_PCH_PVNN_POWER:
      sprintf(name, "MB_VR_PCH_PVNN_POWER");
      break;
    case MB_SENSOR_VR_PCH_P1V05_TEMP:
      sprintf(name, "MB_VR_PCH_P1V05_TEMP");
      break;
    case MB_SENSOR_VR_PCH_P1V05_CURR:
      sprintf(name, "MB_VR_PCH_P1V05_CURR");
      break;
    case MB_SENSOR_VR_PCH_P1V05_VOLT:
      sprintf(name, "MB_VR_PCH_P1V05_VOLT");
      break;
    case MB_SENSOR_VR_PCH_P1V05_POWER:
      sprintf(name, "MB_VR_PCH_P1V05_POWER");
      break;
    case MB_SENSOR_C2_AVA_FTEMP:
      sprintf(name, "MB_C2_AVA_FTEMP");
      break;
    case MB_SENSOR_C2_AVA_RTEMP:
      sprintf(name, "MB_C2_AVA_RTEMP");
      break;
    case MB_SENSOR_C2_1_NVME_CTEMP:
      sprintf(name, "MB_C2_0_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_2_NVME_CTEMP:
      sprintf(name, "MB_C2_1_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_3_NVME_CTEMP:
      sprintf(name, "MB_C2_2_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_4_NVME_CTEMP:
      sprintf(name, "MB_C2_3_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_AVA_FTEMP:
      sprintf(name, "MB_C3_AVA_FTEMP");
      break;
    case MB_SENSOR_C3_AVA_RTEMP:
      sprintf(name, "MB_C3_AVA_RTEMP");
      break;
    case MB_SENSOR_C3_1_NVME_CTEMP:
      sprintf(name, "MB_C3_0_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_2_NVME_CTEMP:
      sprintf(name, "MB_C3_1_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_3_NVME_CTEMP:
      sprintf(name, "MB_C3_2_NVME_CTEMP");
      break;
    case MB_SENSOR_C3_4_NVME_CTEMP:
      sprintf(name, "MB_C3_3_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_AVA_FTEMP:
      sprintf(name, "MB_C4_AVA_FTEMP");
      break;
    case MB_SENSOR_C4_AVA_RTEMP:
      sprintf(name, "MB_C4_AVA_RTEMP");
      break;
    case MB_SENSOR_C4_1_NVME_CTEMP:
      sprintf(name, "MB_C4_0_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_2_NVME_CTEMP:
      sprintf(name, "MB_C4_1_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_3_NVME_CTEMP:
      sprintf(name, "MB_C4_2_NVME_CTEMP");
      break;
    case MB_SENSOR_C4_4_NVME_CTEMP:
      sprintf(name, "MB_C4_3_NVME_CTEMP");
      break;
    case MB_SENSOR_C2_P12V_INA230_VOL:
      sprintf(name, "MB_C2_P12V_INA230_VOL");
      break;
    case MB_SENSOR_C2_P12V_INA230_CURR:
      sprintf(name, "MB_C2_P12V_INA230_CURR");
      break;
    case MB_SENSOR_C2_P12V_INA230_PWR:
      sprintf(name, "MB_C2_P12V_INA230_PWR");
      break;
    case MB_SENSOR_C3_P12V_INA230_VOL:
      sprintf(name, "MB_C3_P12V_INA230_VOL");
      break;
    case MB_SENSOR_C3_P12V_INA230_CURR:
      sprintf(name, "MB_C3_P12V_INA230_CURR");
      break;
    case MB_SENSOR_C3_P12V_INA230_PWR:
      sprintf(name, "MB_C3_P12V_INA230_PWR");
      break;
    case MB_SENSOR_C4_P12V_INA230_VOL:
      sprintf(name, "MB_C4_P12V_INA230_VOL");
      break;
    case MB_SENSOR_C4_P12V_INA230_CURR:
      sprintf(name, "MB_C4_P12V_INA230_CURR");
      break;
    case MB_SENSOR_C4_P12V_INA230_PWR:
      sprintf(name, "MB_C4_P12V_INA230_PWR");
      break;
    case MB_SENSOR_CONN_P12V_INA230_VOL:
      sprintf(name, "MB_CONN_P12V_INA230_VOL");
      break;
    case MB_SENSOR_CONN_P12V_INA230_CURR:
      sprintf(name, "MB_CONN_P12V_INA230_CURR");
      break;
    case MB_SENSOR_CONN_P12V_INA230_PWR:
      sprintf(name, "MB_CONN_P12V_INA230_PWR");
      break;
    case MB_SENSOR_POWER_FAIL:
      sprintf(name, "MB_POWER_FAIL");
      break;
    case MB_SENSOR_MEMORY_LOOP_FAIL:
      sprintf(name, "MB_MEMORY_LOOP_FAIL");
      break;
    case MB_SENSOR_PROCESSOR_FAIL:
      sprintf(name, "MB_PROCESSOR_FAIL");
      break;

    default:
      return -1;
    }
    break;
  case FRU_NIC:
    switch(sensor_num) {
    case MEZZ_SENSOR_TEMP:
      sprintf(name, "MEZZ_SENSOR_TEMP");
      break;
    default:
      return -1;
    }
    break;
  default:
    return -1;
  }
  return 0;
}

static void
_print_sensor_discrete_log(uint8_t fru, uint8_t snr_num, char *snr_name,
    uint8_t val, char *event) {
  if (val) {
    syslog(LOG_CRIT, "ASSERT: %s discrete - raised - FRU: %d, num: 0x%X,"
        " snr: %-16s val: 0x%X", event, fru, snr_num, snr_name, val);
  } else {
    syslog(LOG_CRIT, "DEASSERT: %s discrete - settled - FRU: %d, num: 0x%X,"
        " snr: %-16s val: 0x%X", event, fru, snr_num, snr_name, val);
  }
  pal_update_ts_sled();
}


// Helper function for msleep
void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

void
pal_update_ts_sled()
{
  char key[MAX_KEY_LEN] = {0};
  char tstr[MAX_VALUE_LEN] = {0};
  struct timespec ts;

  clock_gettime(CLOCK_REALTIME, &ts);
  sprintf(tstr, "%ld", ts.tv_sec);

  sprintf(key, "timestamp_sled");

  pal_set_key_value(key, tstr);
}

int
pal_get_pwm_value(uint8_t fan_num, uint8_t *value) {
  char path[LARGEST_DEVICE_NAME] = {0};
  char device_name[LARGEST_DEVICE_NAME] = {0};
  int val = 0;
  int pwm_enable = 0;

  if (fan_num < 0 || fan_num >= pal_pwm_cnt) {
    syslog(LOG_INFO, "pal_get_pwm_value: fan number is invalid - %d", fan_num);
    return -1;
  }

  // Need check pwmX_en to determine the PWM is 0 or 100.
  snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_en", fan_num);
  snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
  if (read_device(path, &pwm_enable)) {
    syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
    return -1;
  }

  if(pwm_enable) {
    snprintf(device_name, LARGEST_DEVICE_NAME, "pwm%d_falling", fan_num);
    snprintf(path, LARGEST_DEVICE_NAME, "%s/%s", PWM_DIR, device_name);
    if (read_device_hex(path, &val)) {
      syslog(LOG_INFO, "pal_get_pwm_value: read %s failed", path);
      return -1;
    }

    if(val == 0)
      *value = 100;
    else
      *value = (100 * val + (PWM_UNIT_MAX-1)) / PWM_UNIT_MAX;
  } else {
    *value = 0;
  }

  return 0;
}

bool
pal_is_cpu1_socket_occupy(void) {
  char path[64] = {0};
  int val;

  sprintf(path, GPIO_VAL, GPIO_FM_CPU1_SKTOCC_LVT3_N);
  if (read_device(path, &val)) {
    return false;
  }

  if (val) {
    return false;
  } else {
    return true;
  }

}
