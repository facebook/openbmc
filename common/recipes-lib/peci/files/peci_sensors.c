#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <peci.h>
#include <linux/peci-ioctl.h>
#include "peci_sensors.h"

#define PECI_READING_SKIP    (1)
#define PECI_READING_NA      (-2)


static uint8_t m_TjMax[CPU_ID_NUM] = {0};
static float m_Dts[CPU_ID_NUM] = {0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF};


int
cmd_peci_rdpkgconfig(PECI_RD_PKG_CONFIG_INFO* info, uint8_t* rx_buf, uint8_t rx_len) {
  int ret;
  uint8_t tbuf[PECI_BUFFER_SIZE];

  tbuf[0] = PECI_CMD_RD_PKG_CONFIG;
  tbuf[1] = info->dev_info;
  tbuf[2] = info->index;
  tbuf[3] = info->para_l;
  tbuf[4] = info->para_h;
  ret = peci_raw(info->cpu_addr, rx_len, tbuf, 5, rx_buf, rx_len);
  if (ret != PECI_CC_SUCCESS) {
#ifdef DEBUG
    syslog(LOG_WARNING, "%s err addr=0x%x index=%x, cc=0x%x",
                        __func__, info->cpu_addr, info->index, ret);
#endif
    return -1;
  }

  return 0;
}

int
cmd_peci_get_thermal_margin(uint8_t cpu_addr, float* value) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];
  int16_t tmp;

  info.cpu_addr = cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_PKG_TEMP;
  info.para_l = 0x00;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  tmp = (rx_buf[2] << 8) | rx_buf[1];
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s rxbuf[1]=%x, rxbuf[2]=%x, tmp=%x", __func__, rx_buf[1], rx_buf[2], tmp);
#endif
  if (tmp <= (int16_t)0x81ff) {  // 0x8000 ~ 0x81ff for error code
    return -1;
  }

  *value = (float)(tmp / 64);
  return 0;
}

int
cmd_peci_get_tjmax(uint8_t cpu_addr, int* tjmax) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];

  info.cpu_addr = cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_TEMP_TARGET;
  info.para_l = 0x00;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  *tjmax = rx_buf[3];
  return 0;
}

int
cmd_peci_dimm_thermal_reading(uint8_t cpu_addr, uint8_t channel, uint8_t* temp) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];

  info.cpu_addr = cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_DIMM_THERMAL_RW;
  info.para_l = channel;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  if(rx_buf[PECI_THERMAL_DIMM0_BYTE] > rx_buf[PECI_THERMAL_DIMM1_BYTE]) {
    *temp = rx_buf[PECI_THERMAL_DIMM0_BYTE];
  } else {
    *temp = rx_buf[PECI_THERMAL_DIMM1_BYTE];
  }
  return 0;
}

int
cmd_peci_get_cpu_err_num(uint8_t cpu_addr, int* num, uint8_t is_caterr) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret=0;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];
  int cpu_num=-1;

  info.cpu_addr= cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_PKG_IDENTIFIER;
  info.para_l = 0x05;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

//CATERR ERROR
  if (is_caterr) {
    if ((rx_buf[3] & BOTH_CPU_ERROR_MASK) == BOTH_CPU_ERROR_MASK)
      cpu_num = 2; //Both
    else if ((rx_buf[3] & EXTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 1; //CPU1
    else if ((rx_buf[3] & INTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 0; //CPU0
  } else {
//MSMI
    if (((rx_buf[2] & BOTH_CPU_ERROR_MASK) == BOTH_CPU_ERROR_MASK))
      cpu_num = 2; //Both
    else if ((rx_buf[2] & EXTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 1; //CPU1
    else if ((rx_buf[2] & INTERNAL_CPU_ERROR_MASK) > 0)
      cpu_num = 0; //CPU0
  }
  *num = cpu_num;
#ifdef DEBUG
  syslog(LOG_DEBUG,"%s cpu error number=%x\n",__func__, *num);
#endif
  return 0;
}

int
cmd_peci_accumulated_energy(uint8_t cpu_addr, uint32_t* value) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret=0;
  int i=0;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];

  info.cpu_addr= cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_ACCUMULATED_ENERGY_STATUS;
  info.para_l = 0xFF;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  for (i=0; i<4; i++) {
    *value |= rx_buf[i] << (8*i);
  }
  return 0;
}

int
cmd_peci_total_time(uint8_t cpu_addr, uint32_t* value) {
  PECI_RD_PKG_CONFIG_INFO info;
  int ret=0;
  int i=0;
  uint8_t rx_len=5;
  uint8_t rx_buf[rx_len];

  info.cpu_addr= cpu_addr;
  info.dev_info = 0x00;
  info.index = PECI_INDEX_TOTAL_TIME;
  info.para_l = 0x00;
  info.para_h = 0x00;

  ret = cmd_peci_rdpkgconfig(&info, rx_buf, rx_len);
  if (ret != 0) {
    return -1;
  }

  for (i=0; i<4; i++) {
    *value |= rx_buf[i] << (8*i);
  }
  return 0;
}

int
lib_get_cpu_tjmax(uint8_t cpu_id, float *value) {
  if (!m_TjMax[cpu_id]) {
    return PECI_READING_NA;
  }

  *value = (float)m_TjMax[cpu_id];
  return 0;
}

int
lib_get_cpu_thermal_margin(uint8_t cpu_id, float *value) {

  if (m_Dts[cpu_id] > 0) {
    return PECI_READING_NA;
  }

  *value = m_Dts[cpu_id];
  return 0;
}

int
lib_get_cpu_pkg_pwr(uint8_t cpu_id, uint8_t cpu_addr, float *value) {
// Energy units: Intel Doc#554767, p37, 2^(-ENERGY UNIT) J, ENERGY UNIT defalut is 14
  // Run Time units: Intel Doc#554767, p33, msec
  float unit = 0.06103515625f; // 2^(-14)*1000 = 0.06103515625
  uint32_t pkg_energy=0, run_time=0, diff_energy=0, diff_time=0;
  static uint32_t last_pkg_energy[4] = {0}, last_run_time[4] = {0};
  static uint8_t retry[CPU_ID_NUM] = {0}; // CPU0 and CPU1
  int ret = PECI_READING_NA;

  ret = cmd_peci_accumulated_energy(cpu_addr, &pkg_energy);
  if (ret != 0) {
    goto error_exit;
  }

  ret =  cmd_peci_total_time(cpu_addr, &run_time);
  if (ret != 0) {
    goto error_exit;
  }

  // need at least 2 entries to calculate
  if (last_pkg_energy[cpu_id] == 0 && last_run_time[cpu_id] == 0) {
    if (ret == 0) {
      last_pkg_energy[cpu_id] = pkg_energy;
      last_run_time[cpu_id] = run_time;
    }
    ret = PECI_READING_NA;
  }

  if (!ret) {
    if (pkg_energy >= last_pkg_energy[cpu_id]) {
      diff_energy = pkg_energy - last_pkg_energy[cpu_id];
    } else {
      diff_energy = pkg_energy + (0xffffffff - last_pkg_energy[cpu_id] + 1);
    }
    last_pkg_energy[cpu_id] = pkg_energy;

    if (run_time >= last_run_time[cpu_id]) {
      diff_time = run_time - last_run_time[cpu_id];
    } else {
      diff_time = run_time + (0xffffffff - last_run_time[cpu_id] + 1);
    }
    last_run_time[cpu_id] = run_time;

    if (diff_time == 0) {
      ret = PECI_READING_NA;
    } else {
      *value = ((float)diff_energy / (float)diff_time * unit);
    }
  }

error_exit:
  if (ret != 0) {
    retry[cpu_id]++;
    if (retry[cpu_id] <= 3) {
      return PECI_READING_SKIP;
    }
  } else {
    retry[cpu_id] = 0;
  }
  return ret;
}

int
lib_get_cpu_temp(uint8_t cpu_id, uint8_t cpu_addr, float *value) {
  int ret, tjmax;
  float dts;
  static uint8_t retry[CPU_ID_NUM] = {0};

  if (!m_TjMax[cpu_id]) {
    ret = cmd_peci_get_tjmax(cpu_addr, &tjmax);
    if (!ret) {
      m_TjMax[cpu_id] = tjmax;
    }
  }

  ret = cmd_peci_get_thermal_margin(cpu_addr, &dts);
  if (ret != 0) {
    retry[cpu_id]++;
    if (retry[cpu_id] <= 3) {
      return PECI_READING_SKIP;
    }
    m_Dts[cpu_id] = 0xFF;
    return PECI_READING_NA;
  } else {
    retry[cpu_id] = 0;
    m_Dts[cpu_id] = dts;
  }

  if (!m_TjMax[cpu_id]) {
    return PECI_READING_NA;
  }

  *value = (float)m_TjMax[cpu_id] + dts;
#ifdef DEBUG
  syslog(LOG_DEBUG, "%s cpu_id=%d value=%f", __func__, cpu_id, *value);
#endif
  return 0;
}

int
lib_get_cpu_dimm_temp(uint8_t cpu_addr, uint8_t dimm_id, float *value) {
  int ret;
  uint8_t temp=0;

  ret = cmd_peci_dimm_thermal_reading(cpu_addr, dimm_id, &temp);
  if(ret == 0)
    *value = (float)temp;

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s DIMM Temp=%f id=%d\n", __func__, *value, dimm_id);
#endif
  return ret;
}

