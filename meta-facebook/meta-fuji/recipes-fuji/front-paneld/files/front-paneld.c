/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <openbmc/kv.h>
#include <openbmc/ipmi.h>
#include <openbmc/ipmb.h>
#include <openbmc/pal.h>
#include <openbmc/pal_sensors.h>
#include <openbmc/misc-utils.h>
#include <openbmc/sdr.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/log.h>

#define FRONTPANELD_NAME                    "front-paneld"
//Debug card presence check interval.
#define DBG_CARD_CHECK_INTERVAL             500
#define BTN_POWER_OFF                       40

#define ADM1278_NAME  "adm1278"
#define ADM1278_ADDR 0x10
#define SCM_ADM1278_BUS 16

#define INTERVAL_MAX  5
#define PIM_RETRY     10

#define LED_CHECK_INTERVAL_S 5
#define LED_SETLED_INTERVAL 100

/* Dynamic change lmsensors config for different PIM card type */
static void
add_pim_lmsensor_conf(uint8_t num, uint8_t bus, uint8_t pim_type) {
  char cmd[1024];

  snprintf(cmd, sizeof(cmd),
           "cat /etc/sensors.d/custom/%s.conf"
           " | sed 's/{pimid}/%d/g'"
           " | sed 's/{i2cbus0}/%d/g'"
           " | sed 's/{i2cbus1}/%d/g'"
           " | sed 's/{i2cbus2}/%d/g'"
           " | sed 's/{i2cbus3}/%d/g'"
           " | sed 's/{i2cbus4}/%d/g'"
           " | sed 's/{i2cbus5}/%d/g'"
           " | sed 's/{i2cbus6}/%d/g'"
           " | sed 's/{i2cbus7}/%d/g'"
           " > /etc/sensors.d/pim%d.conf",
           pim_type == PIM_TYPE_16O ? "PIM16O" : "PIM16Q", num,
           bus, bus + 1, bus + 2, bus + 3,
           bus + 4, bus + 5, bus + 6, bus + 7, num);

  run_command(cmd);
}

static void
remove_pim_lmsensor_conf(uint8_t num) {
  char path[PATH_MAX];

  snprintf(path, sizeof(path), "/etc/sensors.d/pim%d.conf", num);
  if (unlink(path))
    syslog(LOG_ERR, "%s: Failed to remove %s", __func__, path);
}

static int
write_adm1278_conf(uint8_t bus, uint8_t addr, uint8_t cmd, uint16_t data) {
  int dev = -1, read_back = -1;

  dev = i2c_cdev_slave_open(bus, addr, I2C_SLAVE_FORCE_CLAIM);
  if (dev < 0) {
    syslog(LOG_ERR, "%s: i2c_cdev_slave_open bus %d addr 0x%x failed", __func__,
           bus, addr);
    return dev;
  }

  i2c_smbus_write_word_data(dev, cmd, data);
  read_back = i2c_smbus_read_word_data(dev, cmd);
  if (read_back < 0) {
    close(dev);
    syslog(LOG_ERR, "%s: i2c_smbus_read_word_data failed", __func__);
    return read_back;
  }

  if (((uint16_t) read_back) != data) {
    syslog(LOG_WARNING, "%s: data isn't expectd value (%04X, %04X)",
                        __func__, data, read_back);
    close(dev);
    return -1;
  }

  close(dev);
  return 0;
}

static void
pim_device_detect_log(uint8_t num, uint8_t bus, uint8_t addr,
                      char* device, int state) {
  switch (state) {
    case I2C_BUS_ERROR:
    case I2C_FUNC_ERROR:
      syslog(LOG_CRIT, "PIM %d Bus:%d get fail", num, bus);
      break;
    case I2C_DEVICE_ERROR:
      syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s get fail",
             num, bus, addr, device);
      break;
    case I2c_DRIVER_EXIST:
      syslog(LOG_WARNING, "PIM %d Bus:%d Addr:0x%x Device:%s driver exist",
             num, bus, addr, device);
      break;
  }
}

static void
pim_driver_add(uint8_t num, uint8_t pim_type) {
/*         PIM16Q      PIM16O
 * CH1     DOMFPGA     DOMFPGA
 * CH2     EEPROM      EEPROM
 * CH3     LM75        LM75
 * CH4     LM75        LM75
 * CH5     ADM1278     ADM1278
           LM75         -----
 * CH6     DOMFPGA     DOMFPGA
 * CH7     UCD90160    UCD90160
 * CH8      -----      SI5391B
*/
  int state;
  uint8_t bus = ((num - 1) * 8) + 80;
  add_pim_lmsensor_conf(num, bus, pim_type);

  state = i2c_detect_device(bus + 4, ADM1278_ADDR);
  if (state) {
    pim_device_detect_log(num, bus + 4, ADM1278_ADDR, ADM1278_NAME, state);
  }

  /* Config ADM1278 power monitor averaging */
  if (write_adm1278_conf(bus + 4, ADM1278_ADDR, 0xD4, 0x3F1E)) {
    syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s "
                     "can't config register",
                      num, bus + 4, ADM1278_ADDR, ADM1278_NAME);
  }

  sleep(1);
}

static void
pim_driver_del(uint8_t num, uint8_t pim_type) {
  remove_pim_lmsensor_conf(num);
}

static int
pim_thresh_init_file_check(uint8_t fru) {
  char fru_name[32];
  char fpath[64];

  pal_get_fru_name(fru, fru_name);
  snprintf(fpath, sizeof(fpath), INIT_THRESHOLD_BIN, fru_name);

  return access(fpath, F_OK);
}

int
pim_thresh_init(uint8_t fru) {
  int i, sensor_cnt;
  uint8_t snr_num;
  uint8_t *sensor_list;
  thresh_sensor_t snr[MAX_NUM_FRUS][MAX_SENSOR_NUM] = {0};

  pal_get_fru_sensor_list(fru, &sensor_list, &sensor_cnt);
  for (i = 0; i < sensor_cnt; i++) {
    snr_num = sensor_list[i];
    if (sdr_get_snr_thresh(fru, snr_num, &snr[fru - 1][snr_num]) < 0) {
#ifdef DEBUG
      syslog(LOG_WARNING, "pim_thresh_init: sdr_get_snr_thresh for FRU: %d",
             fru);
#endif
      continue;
    }
  }

  if (access(THRESHOLD_PATH, F_OK) == -1) {
    mkdir(THRESHOLD_PATH, 0777);
  }

  if (pal_copy_all_thresh_to_file(fru, snr[fru - 1]) < 0) {
    syslog(LOG_WARNING, "%s: Fail to copy thresh to file for FRU: %d",
           __func__, fru);
    return -1;
  }

  return 0;
}

//clear the pimserial cache in /tmp
static void
clear_pimserial_cache(int pim){
  char pimserial_file[PATH_MAX];
  int ret;

  snprintf(pimserial_file, sizeof(pimserial_file),"/tmp/pim%d_serial.txt", pim);
  if (!access(pimserial_file, F_OK)) {
    syslog(LOG_INFO, "found %s", pimserial_file);
    ret = unlink(pimserial_file);
    if(ret != 0){
      syslog(LOG_CRIT, "deleting  %s failed with errno %s", pimserial_file, strerror(errno));
    }else{
      syslog(LOG_INFO, "%s has been deleted", pimserial_file);
    }
  }
}

// Thread for monitoring pim plug
static void *
pim_monitor_handler(void *unused) {
  uint8_t fru;
  uint8_t num;
  uint8_t ret;
  uint8_t prsnt = 0;
  uint8_t prsnt_ori = 0;
  uint8_t curr_state = 0x00;
  uint8_t pim_type;
  uint8_t pim_pedigree;
  uint8_t pim_phy_type;
  char *pim_phy_type_str = NULL;
  uint8_t pim_type_old[10] = {PIM_TYPE_UNPLUG};
  uint8_t interval[10];
  bool thresh_first[10];

  memset(interval, INTERVAL_MAX, sizeof(interval));
  memset(thresh_first, true, sizeof(thresh_first));
  while (1) {
    for (fru = FRU_PIM1; fru <= FRU_PIM8; fru++) {
      ret = pal_is_fru_prsnt(fru, &prsnt);
      if (ret) {
        goto pim_mon_out;
      }
      /* Get original prsnt state PIM1 @bit0, PIM2 @bit1, ..., PIM8 @bit7 */
      num = fru - FRU_PIM1 + 1;
      prsnt_ori = GETBIT(curr_state, (num - 1));
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt != prsnt_ori) {
        if (prsnt) {
          syslog(LOG_WARNING, "PIM %d is plugged in.", num);
          pim_type = pal_get_pim_type(fru, PIM_RETRY);
          pim_driver_add(num, pim_type);

          if (pim_type != pim_type_old[num]) {
            if (pim_type == PIM_TYPE_16Q) {
              if (!pal_set_pim_type_to_file(fru, "16q")) {
                syslog(LOG_INFO, "PIM %d type is 16Q", num);
                pim_type_old[num] = PIM_TYPE_16Q;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16Q failed", num);
              }
              if (!pal_set_pim_pedigree_to_file(fru, "none")) {
                syslog(LOG_WARNING,
                      "pal_set_pim_pedigree_to_file: PIM %d For PIM16Q set pedigree to none", num);
              } else {
                syslog(LOG_WARNING,
                      "pal_set_pim_pedigree_to_file: PIM %d set none failed", num);
              }

              pim_phy_type = pal_get_pim_phy_type(fru, PIM_RETRY);
              if (pim_phy_type == PIM_16Q_PHY_7NM) {
                pim_phy_type_str = "7nm";
              } else if (pim_phy_type == PIM_16Q_PHY_16NM) {
                pim_phy_type_str = "16nm";
              } else {
                pim_phy_type_str = "unknown";
              }
              if (!pal_set_pim_phy_type_to_file(fru, pim_phy_type_str)) {
                syslog(LOG_INFO,
                      "pal_set_pim_phy_type_to_file: PIM %d set phy type %s", num, pim_phy_type_str);
              } else {
                syslog(LOG_WARNING,
                      "pal_set_pim_phy_type_to_file: PIM %d fail to set phy type", num);
              }
            } else if (pim_type == PIM_TYPE_16O) {
              if (!pal_set_pim_type_to_file(fru, "16o")) {
                syslog(LOG_INFO, "PIM %d type is 16O", num);
                pim_type_old[num] = PIM_TYPE_16O;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16O failed", num);
              }
              pim_pedigree = pal_get_pim_pedigree(fru, PIM_RETRY);
              if(pim_pedigree == PIM_16O_SIMULATE){
                if (!pal_set_pim_pedigree_to_file(fru, "simulate")) {
                  syslog(LOG_INFO, "PIM %d pedigree is simulate version", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set simulate version failed", num);
                }
              }else if(pim_pedigree == PIM_16O_ALPHA1){
                if (!pal_set_pim_pedigree_to_file(fru, "alpha1")) {
                  syslog(LOG_INFO, "PIM %d pedigree is alpha1 version", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set alpha1 version failed", num);
                }
              }else if(pim_pedigree == PIM_16O_ALPHA2){
                if (!pal_set_pim_pedigree_to_file(fru, "alpha2")) {
                  syslog(LOG_INFO, "PIM %d pedigree is alpha2 version", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set alpha2 version failed", num);
                }
              }else{
                if (!pal_set_pim_pedigree_to_file(fru, "none")) {
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set pedigree none", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d fail to set pedigree", num);
                }
              }
            } else {
              if (!pal_set_pim_type_to_file(fru, "none")) {
                syslog(LOG_CRIT,
                        "PIM %d type cannot detect, DOMFPGA get fail", num);
                pim_type_old[num] = PIM_TYPE_NONE;
              } else {
                syslog(LOG_WARNING,
                      "pal_set_pim_type_to_file: PIM %d set none failed", num);
              }
              if (!pal_set_pim_pedigree_to_file(fru, "none")) {
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set pedigree none", num);
                }else{
                  syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set none failed", num);
                }
            }
            pal_set_sdr_update_flag(fru,1);
            if (thresh_first[num] == true) {
              thresh_first[num] = false;
            } else {
              if (pim_thresh_init_file_check(fru)) {
                pim_thresh_init(fru);
              } else {
                pal_set_pim_thresh(fru);
              }
            }
          }
        } else {
          syslog(LOG_WARNING, "PIM %d is unplugged.", num);
          clear_pimserial_cache(num);
          pal_clear_thresh_value(fru);
          pal_set_sdr_update_flag(fru,1);
          pim_driver_del(num, pim_type_old[num]);
          pim_type_old[num] = PIM_TYPE_UNPLUG;
          if (pal_set_pim_type_to_file(fru, "unplug")) {
            syslog(LOG_WARNING,
                  "pal_set_pim_type_to_file: PIM %d set unplug failed", num);
          }
          if (pal_set_pim_pedigree_to_file(fru, "none")) {
            syslog(LOG_WARNING,
                  "pal_get_pim_pedigree: PIM %d set pedigree failed", num);
          }
        }
        /* Set PIM1 prsnt state @bit0, PIM2 @bit1, ..., PIM8 @bit7 */
        curr_state = prsnt ? SETBIT(curr_state, (num - 1))
                           : CLEARBIT(curr_state, (num - 1));
      }
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt) {
        if (interval[num] == 0) {
          interval[num] = INTERVAL_MAX;
          pal_set_pim_sts_led(fru);
          msleep(50);
        } else {
          interval[num]--;
        }
      }
    }
pim_mon_out:
    sleep(1);
  }
  return NULL;
}

// Thread for monitoring scm plug
static void *
scm_monitor_handler(void *unused) {
  int curr = -1;
  int prev = -1;
  int ret;
  uint8_t prsnt = 0;
  uint8_t power;

  while (1) {
    ret = pal_is_fru_prsnt(FRU_SCM, &prsnt);
    if (ret) {
      goto scm_mon_out;
    }
    curr = prsnt;
    if (curr != prev) {
      if (curr) {
        // SCM was inserted
        syslog(LOG_WARNING, "SCM Insertion\n");

        ret = pal_get_server_power(FRU_SCM, &power);
        if (ret) {
          goto scm_mon_out;
        }
        if (power == SERVER_POWER_OFF) {
          sleep(3);
          syslog(LOG_WARNING, "SCM power on\n");
          pal_set_server_power(FRU_SCM, SERVER_POWER_ON);
          /* Setup management port LED */
          run_command("/usr/local/bin/setup_mgmt.sh led");
          /* Config ADM1278 power monitor averaging */
          if (write_adm1278_conf(SCM_ADM1278_BUS, ADM1278_ADDR, 0xD4, 0x3F1E)) {
            syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x Device:%s "
                             "can't config register",
                             SCM_ADM1278_BUS, ADM1278_ADDR, ADM1278_NAME);
          }
          goto scm_mon_out;
        }
      } else {
        // SCM was removed
        syslog(LOG_WARNING, "SCM Extraction\n");
      }
    }
scm_mon_out:
    prev = curr;
    sleep(1);
  }
  return NULL;
}

/*
 * @brief  checking SCM and PIM presence
 * @note   will be use for sys_led controling
 * @param  brd_rev:
 * @retval 0 is all present , -1 is one or more absent
 */
static int sys_present_check(int brd_rev) {
  int ret = 0;
  uint8_t prsnt = 0;

  char fru_name[8];
  uint8_t fru_list[] = { FRU_SCM  ,FRU_PIM1, FRU_PIM2,
                         FRU_PIM3 ,FRU_PIM4, FRU_PIM5,
                         FRU_PIM6 ,FRU_PIM7, FRU_PIM8 };

  for (int fru = 0; fru < sizeof(fru_list) / sizeof(fru_list[0]); fru++) {
    pal_get_fru_name(fru_list[fru], fru_name);
    if (pal_is_fru_prsnt(fru_list[fru], &prsnt)) {
      syslog(LOG_CRIT, "cannot get fru %s presence status", fru_name);
      continue;
    }
    if (prsnt == 0) {
      syslog(LOG_WARNING, "fru %s is not presence", fru_name);
      ret = -1;
    }
  }
  return ret;
}

/*
 * @brief  checking PSU_AC_OK,PSU_PWR_OK and PSU presence
 * @note   will be use for psu_led controling
 * @param  brd_rev:
 * @retval 0 is normal , -1 is fail
 */
static int psu_check(int brd_rev) {
  uint8_t status = 0;
  int ret = 0;
  char path[LARGEST_DEVICE_NAME + 1];
  char *sysfs[] = { "psu_L_pwr_ok",   "psu_R_pwr_ok",
                    "psu1_ac_ok",     "psu2_ac_ok",
                    "psu3_ac_ok",     "psu4_ac_ok" };
  for (int psu = 1; psu <= 4; psu++) {
    if(pal_is_fru_prsnt(FRU_PSU1+psu-1, &status)) {
      syslog(LOG_CRIT, "cannot get FRU_PSU%d presence status ", psu);
      ret = -1;
      continue;
    }
    if (status == 0) { // status 0 is absent
      syslog(LOG_WARNING, "FRU_PSU%d is not presence", psu);
      ret = -1;
    }
  }

  for (int i = 0; i < sizeof(sysfs) / sizeof(sysfs[0]); i++) {
    snprintf(path, LARGEST_DEVICE_NAME, SMBCPLD_PATH_FMT, sysfs[i]);
    if (device_read(path, (int*)&status)) {
      syslog(LOG_CRIT, "cannot access %s", sysfs[i]);
      ret = -1;
      continue;
    }
    if (status == 0) { // status 0 is absent
      syslog(LOG_WARNING, "%s is not ok", sysfs[i]);
      ret = -1;
    }
  }

  return ret;
}

/*
 * @brief  checking fan presence and fan speed
 * @note   will be use for fan_led controling
 * @param  brd_rev:
 * @retval 0 is normal , -1 is fail
 */
static int fan_check(int brd_rev) {
  uint8_t status = 0;
  int ret = 0;
  float value = 0, ucr = 0, lcr = 0;
  char sensor_name[32];
  int sensor_num[] = {
      SMB_SENSOR_FAN1_FRONT_TACH,       SMB_SENSOR_FAN1_REAR_TACH,
      SMB_SENSOR_FAN2_FRONT_TACH,       SMB_SENSOR_FAN2_REAR_TACH,
      SMB_SENSOR_FAN3_FRONT_TACH,       SMB_SENSOR_FAN3_REAR_TACH,
      SMB_SENSOR_FAN4_FRONT_TACH,       SMB_SENSOR_FAN4_REAR_TACH,
      SMB_SENSOR_FAN5_FRONT_TACH,       SMB_SENSOR_FAN5_REAR_TACH,
      SMB_SENSOR_FAN6_FRONT_TACH,       SMB_SENSOR_FAN6_REAR_TACH,
      SMB_SENSOR_FAN7_FRONT_TACH,       SMB_SENSOR_FAN7_REAR_TACH,
      SMB_SENSOR_FAN8_FRONT_TACH,       SMB_SENSOR_FAN8_REAR_TACH
  };

  for (int fan = 1; fan <= 8; fan++) {
    if (pal_is_fru_prsnt(FRU_FAN1+fan-1, &status)) {
      syslog(LOG_CRIT, "cannot get FRU presence status FRU_FAN%d", fan);
      ret = -1;
      continue;
    }
    if (status == 0) { // status 0 is absent
      syslog(LOG_WARNING, "FRU_FAN%d is not presence", fan);
      ret = -1;
    }
  }

  for (int i = 0; i < sizeof(sensor_num) / sizeof(sensor_num[0]); i++) {
    pal_get_sensor_name(FRU_SMB,sensor_num[i], sensor_name);
    if (sensor_cache_read(FRU_SMB, sensor_num[i], &value)) {
      syslog(LOG_INFO, "can't read %s from cache read raw instead", sensor_name);
      if (pal_sensor_read_raw(FRU_SMB, sensor_num[i], &value)) {
        syslog(LOG_CRIT, "can't read sensor %s", sensor_name);
        ret = -1;
        continue;
      } else {
        // write cahed after read success
        if (value == 0) {
          sensor_cache_write(FRU_SMB, sensor_num[i], false, 0.0);
        } else {
          sensor_cache_write(FRU_SMB, sensor_num[i], true, value);
        }
      }
    }
    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], UCR_THRESH, &ucr);
    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], LCR_THRESH, &lcr);
    if(value == 0) {
      syslog(LOG_WARNING, "sensor %s value is 0", sensor_name);
      ret = -1;
    }else if(value > ucr) {
      syslog(LOG_WARNING, "sensor %s value is %.2f over than UCR %.2f ", sensor_name, value, ucr);
      ret = -1;
    }else if(value < lcr) {
      syslog(LOG_WARNING, "sensor %s value is %.2f less than LCR %.2f ", sensor_name, value, lcr);
      ret = -1;
    }
  }

  return ret;
}

/*
 * @brief  checking sensor threshold on SMB
 * @note   will be used for smb_led
 * @param  brd_rev:
 * @retval 0 is normal , -1 is fail
 */
static int smb_check(int brd_rev) {
  int ret = 0;
  float value = 0, ucr = 0, lcr = 0;
  char sensor_name[32];
  int sensor_num[] = {
      /* Sensors on SMB */
      SMB_XP3R3V_BMC,                   SMB_XP2R5V_BMC,
      SMB_XP1R8V_BMC,                   SMB_XP1R2V_BMC,
      SMB_XP1R0V_FPGA,                  SMB_XP3R3V_USB,
      SMB_XP5R0V,                       SMB_XP3R3V_EARLY,
      SMB_LM57_VTEMP,                   SMB_XP1R8,
      SMB_XP1R2,                        SMB_VDDC_SW,
      SMB_XP3R3V,                       SMB_XP1R8V_AVDD,
      SMB_XP1R2V_TVDD,                  SMB_XP0R75V_1_PVDD,
      SMB_XP0R75V_2_PVDD,               SMB_XP0R75V_3_PVDD,
      SMB_VDD_PCIE,                     SMB_XP0R84V_DCSU,
      SMB_XP0R84V_CSU,                  SMB_XP1R84V_CSU,
      SMB_XP3R3V_TCXO,                  SMB_OUTPUT_VOLTAGE_XP0R75V_1,
      SMB_OUTPUT_CURRENT_XP0R75V_1,     SMB_INPUT_VOLTAGE_1,
      SMB_OUTPUT_VOLTAGE_XP1R2V,        SMB_OUTPUT_CURRENT_XP1R2V,
      SMB_OUTPUT_VOLTAGE_XP0R75V_2,     SMB_OUTPUT_CURRENT_XP0R75V_2,
      SMB_INPUT_VOLTAGE_2,              SMB_TMP422_U20_1_TEMP,
      SMB_TMP422_U20_2_TEMP,            SMB_TMP422_U20_3_TEMP,
      SIM_LM75_U1_TEMP,                 SMB_SENSOR_TEMP1,
      SMB_SENSOR_TEMP2,                 SMB_SENSOR_TEMP3,
      SMB_VDDC_SW_TEMP,                 SMB_XP12R0V_VDDC_SW_IN,
      SMB_VDDC_SW_IN_SENS,
      SMB_VDDC_SW_POWER_IN,             SMB_VDDC_SW_POWER_OUT,
      SMB_VDDC_SW_CURR_IN,              SMB_VDDC_SW_CURR_OUT,
      /* Sensors on PDB */
      SMB_SENSOR_PDB_L_TEMP1,           SMB_SENSOR_PDB_L_TEMP2,
      SMB_SENSOR_PDB_R_TEMP1,           SMB_SENSOR_PDB_R_TEMP2,
      /* Sensors on FCM */
      SMB_SENSOR_FCM_T_TEMP1,           SMB_SENSOR_FCM_T_TEMP2,
      SMB_SENSOR_FCM_B_TEMP1,           SMB_SENSOR_FCM_B_TEMP2,
      SMB_SENSOR_FCM_T_HSC_VOLT,        SMB_SENSOR_FCM_T_HSC_CURR,
      SMB_SENSOR_FCM_T_HSC_POWER_VOLT,  SMB_SENSOR_FCM_B_HSC_VOLT,
      SMB_SENSOR_FCM_B_HSC_CURR,        SMB_SENSOR_FCM_B_HSC_POWER_VOLT
  };
  for (int i = 0; i < sizeof(sensor_num) / sizeof(sensor_num[0]); i++) {
    pal_get_sensor_name(FRU_SMB, sensor_num[i], sensor_name);

    if (sensor_cache_read(FRU_SMB, sensor_num[i], &value)) {
      syslog(LOG_INFO, "can't read %s from cache read raw instead", sensor_name);
      if (pal_sensor_read_raw(FRU_SMB, sensor_num[i], &value)) {
        syslog(LOG_CRIT, "can't read sensor %s", sensor_name);
        ret = -1;
        continue;
      } else {
        // write cahed after read success
        sensor_cache_write(FRU_SMB, sensor_num[i], true, value);
      }
    }
    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], UCR_THRESH, &ucr);
    pal_get_sensor_threshold(FRU_SMB, sensor_num[i], LCR_THRESH, &lcr);

    if (ucr != 0 && value > ucr) {
      syslog(LOG_WARNING, "sensor %s value is %.2f over than UCR %.2f ", sensor_name, value, ucr);
      ret = -1;
    } else if (lcr != 0 && value < lcr) {
      syslog(LOG_WARNING, "sensor %s value is %.2f less than LCR %.2f ", sensor_name, value, lcr);
      ret = -1;
    }
  }
  return ret;
}

static uint8_t leds[4] = {
  SIM_LED_OFF,// SLED_NAME_SYS
  SIM_LED_OFF,// SLED_NAME_FAN
  SIM_LED_OFF,// SLED_NAME_PSU
  SIM_LED_OFF // SLED_NAME_SMB
};

/*
 * @brief  hanlder event and set led event to "leds"
 * @note   leds is global variable sync with simLED_setled_handler()
 * @param  *unused:
 * @retval None
 */
static void *
simLED_monitor_handler(void *unused) {
  int brd_rev;
  struct timeval timeval;
  long curr_time;
  long last_time = 0;
  bool status_ug = false;
  pal_get_board_rev(&brd_rev);
  while (1) {
    gettimeofday(&timeval, NULL);
    curr_time = timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
    if ( last_time == 0 || curr_time < last_time ||
         curr_time - last_time > LED_CHECK_INTERVAL_S * 1000 ) {
      last_time = curr_time;

      //SYS LED
      // BLUE            all FRU(SCM and PIM) present,no level alarms
      // AMBER           one or more FRU not present and have alarm
      // ALTERNATE       firmware upgrade in process
      // AMBER FLASHING  need technicial required,
      status_ug = pal_is_fw_update_ongoing(FRU_ALL);
      if (sys_present_check(brd_rev)) {
        leds[SLED_NAME_SYS] = SIM_LED_AMBER;
      } else {
        if(status_ug) {
          syslog(LOG_WARNING, "firmware is upgrading");
          leds[SLED_NAME_SYS] = SIM_LED_ALT_BLINK;
        }else{
          leds[SLED_NAME_SYS] = SIM_LED_BLUE;
        }
      }

      //FAN LED
      // BLUE   all presence and sensor normal
      // AMBER  one or more absence or sensor out-of-range RPM
      if (!status_ug) { // skip sensor check when firmware is upgrading
        if (fan_check(brd_rev) == 0) {
          leds[SLED_NAME_FAN] = SIM_LED_BLUE;
        } else {
          leds[SLED_NAME_FAN] = SIM_LED_AMBER;
        }
      }

      //PSU LED
      // BLUE     all PSUs present and INPUT OK,PWR OK
      // AMBER    one or more not present or INPUT OK or PWR OK de-asserted
      if (!status_ug) { // skip sensor check when firmware is upgrading
        if (psu_check(brd_rev) == 0) {
          leds[SLED_NAME_PSU] = SIM_LED_BLUE;
        } else {
          leds[SLED_NAME_PSU] = SIM_LED_AMBER;
        }
      }

      //SMB LED
      // BLUE   sensor ok
      // AMBER  fail
      if (!status_ug)  { // skip sensor check when firmware is upgrading
        if (smb_check(brd_rev) == 0) {
          leds[SLED_NAME_SMB] = SIM_LED_BLUE;
        } else {
          leds[SLED_NAME_SMB] = SIM_LED_AMBER;
        }
      }
    }
    sleep(1);
  }
  return NULL;
}

/*
 * @brief  controling led in SIM Board
 * @note   leds is global variable sync with simLED_monitor_handler()
 * @param  *unused:
 * @retval None
 */
static void *
simLED_setled_handler(void *unused) {
  int brd_rev;
  static uint8_t led_alt = 0;
  struct timeval timeval;
  long curr_time;
  long last_time = 0;
  pal_get_board_rev(&brd_rev);
  while(1) {
    gettimeofday(&timeval, NULL);
    curr_time = timeval.tv_sec * 1000 + timeval.tv_usec / 1000;
    if ( last_time == 0 || curr_time < last_time ||
         curr_time - last_time > 500 ){  // blinking every 500ms
      led_alt = 1-led_alt;
      last_time = curr_time;
    }

    for (uint8_t led = 0; led < sizeof(leds) / sizeof(leds[0]); led++) {
      uint8_t state = leds[led];
      if (state == SIM_LED_OFF) {
        set_sled(brd_rev, SLED_COLOR_OFF, led);
      } else if (state == SIM_LED_BLUE) {
        set_sled(brd_rev, SLED_COLOR_BLUE, led);
      } else if (state == SIM_LED_AMBER) {
        set_sled(brd_rev, SLED_COLOR_AMBER, led);
      } else if (state == SIM_LED_ALT_BLINK) {
        if (led_alt) {
          set_sled(brd_rev, SLED_COLOR_BLUE, led);
        } else {
          set_sled(brd_rev, SLED_COLOR_AMBER, led);
        }
      } else if (state == SIM_LED_AMBER_BLINK) {
        if (led_alt) {
          set_sled(brd_rev, SLED_COLOR_AMBER, led);
        } else {
          set_sled(brd_rev, SLED_COLOR_OFF, led);
        }
      }
    }
    msleep(LED_SETLED_INTERVAL);
  }
  return NULL;
}

// Thread for monitoring debug card hotswap
static void *
debug_card_handler(void *unused) {
  int curr = -1;
  int prev = -1;
  int ret = -1;
  uint8_t prsnt = 0;

  syslog(LOG_INFO, "%s: %s started", FRONTPANELD_NAME, __FUNCTION__);
  while (1) {
    // Check if debug card present or not
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret) {
      goto debug_card_out;
    }
    curr = prsnt;

    // Check if Debug Card was either inserted or removed
    if (curr != prev) {
      if (!curr) {
        // Debug Card was removed
        syslog(LOG_INFO, "Debug Card Extraction\n");

      } else {
        // Debug Card was inserted
        syslog(LOG_INFO, "Debug Card Insertion\n");
      }
      prev = curr;
    }
debug_card_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }

  return NULL;
}

/* Thread to handle Uart Selection Button */
static void *
uart_sel_btn_handler(void *unused) {
  uint8_t btn = 0;
  uint8_t prsnt = 0;
  int ret = -1;

  syslog(LOG_INFO, "%s: %s started", FRONTPANELD_NAME, __FUNCTION__);

  while (1) {
    /* Check the position of hand switch
     * and if reset button is pressed */
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret || !prsnt) {
      goto uart_sel_btn_out;
    }
    ret = pal_get_dbg_uart_btn(&btn);
    if (ret || !btn) {
      goto uart_sel_btn_out;
    }
    if (btn) {
      syslog(LOG_INFO, "Uart button pressed\n");
      pal_switch_uart_mux();
      pal_clr_dbg_btn();
    }
uart_sel_btn_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }

  return NULL;
}

/* Thread to handle Power Button to power on/off userver */
static void *
pwr_btn_handler(void *unused) {
  int ret = -1;
  uint8_t btn = 0;
  uint8_t cmd = 0;
  uint8_t power = 0;
  uint8_t prsnt = 0;

  syslog(LOG_INFO, "%s: %s started", FRONTPANELD_NAME, __FUNCTION__);
  while (1) {
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret || !prsnt) {
      goto pwr_btn_out;
    }

    ret = pal_get_dbg_pwr_btn(&btn);
    if (ret || !btn) {
        goto pwr_btn_out;
    }

    if (btn) {
      syslog(LOG_INFO, "Power button pressed\n");

      // Get the current power state (on or off)
      ret = pal_get_server_power(FRU_SCM, &power);
      if (ret) {
        pal_clr_dbg_btn();
        goto pwr_btn_out;
      }

      // Set power command should reverse of current power state
      cmd = !power;
      pal_update_ts_sled();

      // Reverse the power state of the given server.
      ret = pal_set_server_power(FRU_SCM, cmd);
      pal_clr_dbg_btn();
    }
pwr_btn_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }

  return NULL;
}

/* Thread to handle Reset Button to reset userver */
static void *
rst_btn_handler(void *unused) {
  uint8_t btn = 0;
  int ret = -1;
  uint8_t prsnt = 0;

  syslog(LOG_INFO, "%s: %s started", FRONTPANELD_NAME, __FUNCTION__);
  while (1) {
    ret = pal_is_debug_card_prsnt(&prsnt);
    if (ret || !prsnt) {
      goto rst_btn_out;
    }
    /* Check the position of hand switch
     * and if reset button is pressed */
    ret = pal_get_dbg_rst_btn(&btn);
    if (ret || !btn) {
      goto rst_btn_out;
    }

    if (btn) {
      syslog(LOG_INFO, "Rst button pressed\n");
      /* Reset userver */
      pal_set_server_power(FRU_SCM, SERVER_POWER_RESET);
      pal_clr_dbg_btn();
    }
rst_btn_out:
    msleep(DBG_CARD_CHECK_INTERVAL);
  }

  return NULL;
}

void
exithandler(int signum) {
  int brd_rev;
  pal_get_board_rev(&brd_rev);
  set_sled(brd_rev, SLED_COLOR_AMBER, SLED_NAME_SYS);
  exit(0);
}

int
main (int argc, char * const argv[]) {
  pthread_t tid_pim_monitor;
  pthread_t tid_scm_monitor;
  pthread_t tid_sim_monitor;
  pthread_t tid_sim_setled;
  pthread_t tid_debug_card;
  pthread_t tid_uart_sel_btn;
  pthread_t tid_pwr_btn;
  pthread_t tid_rst_btn;
  int rc;
  int pid_file;

  signal(SIGTERM, exithandler);
  pid_file = open("/var/run/front-paneld.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      printf("Another front-paneld instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("front-paneld", LOG_CONS, LOG_DAEMON);
  }

  if ((rc = pthread_create(&tid_pim_monitor, NULL,
                           pim_monitor_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create pim monitor thread: %s",
           strerror(rc));
    exit(1);
  }
  if ((rc = pthread_create(&tid_sim_monitor, NULL,
                           simLED_monitor_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create sim monitor thread: %s",
           strerror(rc));
    exit(1);
  }
  if ((rc = pthread_create(&tid_sim_setled, NULL,
                           simLED_setled_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create sim setled thread: %s",
           strerror(rc));
    exit(1);
  }

  if ((rc = pthread_create(&tid_scm_monitor, NULL,
                           scm_monitor_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create scm monitor thread: %s",
           strerror(rc));
    exit(1);
  }

  if ((rc = pthread_create(&tid_debug_card, NULL,
                           debug_card_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create debug_card thread: %s",
           strerror(rc));
    exit(1);
  }

  if ((rc = pthread_create(&tid_uart_sel_btn, NULL,
                           uart_sel_btn_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create uart_sel_btn thread: %s",
           strerror(rc));
    exit(1);
  }

  if ((rc = pthread_create(&tid_pwr_btn, NULL,
                           pwr_btn_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create pwr_btn thread: %s",
           strerror(rc));
    exit(1);
  }

  if ((rc = pthread_create(&tid_rst_btn, NULL,
                           rst_btn_handler, NULL))) {
    syslog(LOG_WARNING, "failed to create rst_btn thread: %s",
           strerror(rc));
    exit(1);
  }


  pthread_join(tid_pim_monitor, NULL);
  pthread_join(tid_scm_monitor, NULL);
  pthread_join(tid_sim_monitor, NULL);
  pthread_join(tid_sim_setled, NULL);
  pthread_join(tid_debug_card, NULL);
  pthread_join(tid_uart_sel_btn, NULL);
  pthread_join(tid_pwr_btn, NULL);
  pthread_join(tid_rst_btn, NULL);

  return 0;
}
