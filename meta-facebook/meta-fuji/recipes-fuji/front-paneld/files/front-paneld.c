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
#define BICMOND_NAME                        "bicmond"
#define BICMOND_DEAMON                      "/usr/local/bin/bicmond -D"
//Debug card presence check interval.
#define DBG_CARD_CHECK_INTERVAL             500
#define BTN_POWER_OFF                       40

#define INTERVAL_MAX  5
#define PIM_RETRY     10

#define LED_CHECK_INTERVAL_S 5
#define LED_SETLED_INTERVAL 100

struct dev_bus_addr PIM_I2C_DEVICE_CHANGE_LIST[MAX_PIM][PIM_I2C_DEVICE_NUMBER];
struct dev_bus_addr SCM_I2C_DEVICE_CHANGE_LIST[SCM_I2C_DEVICE_NUMBER];

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

/* checking process is running by name */
static bool
is_process_run(const char* pname) {
  FILE *fp;
  bool ret=false;
  char *buf_ptr, *buf_ptr_new;
  #define INIT_BUFFER_SIZE 1000
  int buf_size = INIT_BUFFER_SIZE;
  int str_size = 200;
  int tmp_size;
  char str[200];

  fp = popen("ps w", "r");
  if(NULL == fp)
    return false;

  buf_ptr = (char *)malloc(INIT_BUFFER_SIZE * sizeof(char) + sizeof(char));
  if (buf_ptr == NULL)
    goto close_fp;
  memset(buf_ptr, 0, sizeof(char));
  tmp_size = str_size;
  while(fgets(str, str_size, fp) != NULL) {
    tmp_size = tmp_size + str_size;
    if(tmp_size + str_size >= buf_size) {
      buf_ptr_new = realloc(buf_ptr, sizeof(char) * buf_size * 2 + sizeof(char));
      buf_size *= 2;
      if(buf_ptr_new == NULL) {
        syslog(LOG_ERR,
              "%s realloc() fail, please check memory remaining", __func__);
        goto free_buf;
      } else {
        buf_ptr = buf_ptr_new;
      }
    }
    strncat(buf_ptr, str, str_size);
  }

  if (strstr(buf_ptr, pname) != NULL) {
    ret = true;
  } else {
    ret = false;
  }

free_buf:
  if(buf_ptr)
    free(buf_ptr);
close_fp:
  if(pclose(fp) == -1)
    syslog(LOG_ERR, "%s pclose() fail ", __func__);

  return ret;
}

/* load the driver of respin chip on pim board */
static void
pim_driver_add(uint8_t num) {
/*         PIM16Q      PIM16O
 * CH0     DOMFPGA     DOMFPGA
 * CH1     EEPROM      EEPROM
 * CH2     LM75        LM75
 * CH3     LM75        LM75
 * CH4   [ ADM1278 or  ADM1278
           LM25066 ]
           LM75
 * CH5     DOMFPGA     DOMFPGA
 * CH7                 SI5391B
*/
  uint8_t bus = ((num - 1) * 8) + 80;
  uint8_t bus_id;
  uint8_t fru = num - 1 + FRU_PIM1;
  uint8_t i = 0;
  struct dev_bus_addr *pim_device = PIM_I2C_DEVICE_CHANGE_LIST[num - 1];
  uint8_t pim_type;
  pim_i2c_mux_bind(num);
  sleep(2);

  struct dev_addr_driver *pim_hsc = pal_get_pim_hsc(fru);
  /* 0 channel of paca9548 */
  /* need to add DOM FPGA before getting PIM Type*/
  bus_id = bus + PIM_DOM_DEVICE_CH  ;
  if (pal_add_i2c_device(bus_id, PIM_DOM_FPGA_ADDR, DOM_FPGA_DRIVER)) {
      syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                       num, bus_id, PIM_DOM_FPGA_ADDR, DOM_FPGA_DRIVER);
  } else {
    pim_device[i].bus_id = bus_id;
    pim_device[i++].device_address = PIM_DOM_FPGA_ADDR;
  }

  pim_type = pal_get_pim_type(fru, PIM_RETRY);
  add_pim_lmsensor_conf(num, bus, pim_type);
  /* 1 channel of paca9548 */
  bus_id = bus + PIM_EEPROM_DEVICE_CH  ;
  if (pal_add_i2c_device(bus_id, PIM_EEPROM_ADDR, EEPROM_24C64_DRIVER)) {
      syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                       num, bus_id, PIM_EEPROM_ADDR, EEPROM_24C64_DRIVER);
  } else {
    pim_device[i].bus_id = bus_id;
    pim_device[i++].device_address = PIM_EEPROM_ADDR;
  }

  /* channel 2 of paca9548 */
  bus_id = bus + PIM_LM75_TEMP_48_DEVICE_CH  ;
  if (pal_add_i2c_device(bus_id, PIM_LM75_TEMP_48_ADDR, LM75_DRIVER)) {
      syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                       num, bus_id, PIM_LM75_TEMP_48_ADDR, LM75_DRIVER);
  } else {
    pim_device[i].bus_id = bus_id;
    pim_device[i++].device_address = PIM_LM75_TEMP_48_ADDR;
  }

  /* channel 3 of paca9548 */
  bus_id = bus + PIM_LM75_TEMP_4B_DEVICE_CH  ;
  if (pal_add_i2c_device(bus_id, PIM_LM75_TEMP_4B_ADDR, LM75_DRIVER)) {
      syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                       num, bus_id, PIM_LM75_TEMP_4B_ADDR, LM75_DRIVER);
  } else {
    pim_device[i].bus_id = bus_id;
    pim_device[i++].device_address = PIM_LM75_TEMP_4B_ADDR;
  }

  /* channel 4 of paca9548 */
  bus_id = bus + PIM_LM75_TEMP_4A_DEVICE_CH  ;
  if (pal_add_i2c_device(bus_id, PIM_LM75_TEMP_4A_ADDR, LM75_DRIVER)) {
      syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                       num, bus_id, PIM_LM75_TEMP_4A_ADDR, LM75_DRIVER);
  } else {
    pim_device[i].bus_id = bus_id;
    pim_device[i++].device_address = PIM_LM75_TEMP_4A_ADDR;
  }

  /* detect channel 4 of paca9548
   *   Hot-swap chip designed for dual-footprint
   *    ADM1278  0x10
   *    LM25066  0x44
   */
  bus_id = bus + PIM_HSC_DEVICE_CH  ;
  if ( pim_hsc == NULL ){
    syslog(LOG_CRIT, "Cannot find device:   %s:0x%02x %s:0x%02x on PIM %d Bus:%d",
                    ADM1278_DRIVER, PIM_HSC_ADM1278_ADDR,
                    LM25066_DRIVER, PIM_HSC_LM25066_ADDR,
                    num, bus_id);
  } else {
    /* Config ADM1278 power monitor averaging */
    if (pim_hsc->addr == PIM_HSC_ADM1278_ADDR){
      if (write_adm1278_conf(bus_id, PIM_HSC_ADM1278_ADDR, 0xD4, 0x3F1E)) {
        syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s "
                          "can't config register",
                          num, bus_id, PIM_HSC_ADM1278_ADDR, ADM1278_DRIVER);
      }
    }

    if (pal_add_i2c_device(bus_id, pim_hsc->addr, (char *)pim_hsc->chip_name)) {
      syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x Device:%s "
                      "driver cannot add.",
                      num, bus_id,  pim_hsc->addr, pim_hsc->chip_name);
    } else {
      set_dev_addr_to_file(fru, KEY_HSC, pim_hsc->addr);
      syslog(LOG_CRIT, "Load driver: PIM %d Bus:%d Addr:0x%x Chip:%s ",
                      num, bus_id, pim_hsc->addr, pim_hsc->chip_name);
      pim_device[i].bus_id = bus_id;
      pim_device[i++].device_address = pim_hsc->addr;
    }
  }
  run_command(". openbmc-utils.sh && i2c_check_driver_binding fix-binding");
}

static void
pim_driver_del(uint8_t num, uint8_t pim_type) {
  int i;
  int state = -1;
  struct dev_bus_addr *pim_device = PIM_I2C_DEVICE_CHANGE_LIST[num - 1];

  for (i = 0; i < PIM_I2C_DEVICE_NUMBER; i++) {
    /* i2c slave address should not be  0 */
    if (pim_device[i].device_address != 0){
      state = pal_del_i2c_device(pim_device[i].bus_id, pim_device[i].device_address);
      if ( state != 0) {
        syslog(LOG_CRIT, "PIM %d Bus:%d Addr:0x%x "
                "driver cannot delete.",
                num, pim_device[i].bus_id, pim_device[i].device_address);
        continue;
      }
      pim_device[i].bus_id = 0;
      pim_device[i].device_address = 0;
    }
  }
  pim_i2c_mux_unbind(num);
  remove_pim_lmsensor_conf(num);
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

static void
scm_driver_add() {
/*         SCM
 * CH1     ADM1278
 * CH2     EEPROM
 * CH3     LM75
 * CH4     LM75
 * CH5     ADM1278
           LM75
           LM25066
 * CH6     DOMFPGA
 * CH7     UCD90160
 * CH8     SI5391B
*/
  uint8_t i = 0;
  uint8_t bus_id;
  scm_i2c_mux_bind();
  struct dev_addr_driver* scm_hsc = pal_get_scm_hsc();
  memset(SCM_I2C_DEVICE_CHANGE_LIST, 0, sizeof(SCM_I2C_DEVICE_CHANGE_LIST));
  syslog(LOG_INFO, "scm_driver_add");
  /* detect 0 channel of paca9548 */
  bus_id = 16;
  if ( scm_hsc == NULL ) {
    syslog(LOG_CRIT, "Cannot find device:   %s:0x%02x %s:0x%02x on SCM Bus:%d",
                      ADM1278_DRIVER, SCM_HSC_ADM1278_ADDR,
                      LM25066_DRIVER, SCM_HSC_LM25066_ADDR,
                      bus_id);
  } else {
    /* Config ADM1278 power monitor averaging */
    if ( scm_hsc->addr == SCM_HSC_ADM1278_ADDR ){
      if (write_adm1278_conf(bus_id, scm_hsc->addr, 0xD4, 0x3F1E)) {
        syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x Device:%s "
                          "can't config register",
                          bus_id, SCM_HSC_ADM1278_ADDR, ADM1278_DRIVER);
      }
    }

    if (pal_add_i2c_device(bus_id, scm_hsc->addr, (char*)scm_hsc->chip_name)) {
      syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x Device:%s "
                        "driver cannot add.",
                        bus_id, scm_hsc->addr, scm_hsc->chip_name);
    } else {
      SCM_I2C_DEVICE_CHANGE_LIST[i].bus_id = bus_id;
      SCM_I2C_DEVICE_CHANGE_LIST[i++].device_address = scm_hsc->addr;
    }
    set_dev_addr_to_file(FRU_SCM, KEY_HSC, scm_hsc->addr);
  }

  /* channel 2 of paca9548 */
  bus_id = 17 ;
  if (pal_add_i2c_device(bus_id, SCM_LM75_4C_ADDR, LM75_DRIVER)) {
      syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                        bus_id, SCM_LM75_4C_ADDR, LM75_DRIVER);
  } else {
      SCM_I2C_DEVICE_CHANGE_LIST[i].bus_id = bus_id;
      SCM_I2C_DEVICE_CHANGE_LIST[i++].device_address = SCM_LM75_4C_ADDR;
  }

  if (pal_add_i2c_device(bus_id, SCM_LM75_4D_ADDR, LM75_DRIVER)) {
      syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                        bus_id, SCM_LM75_4D_ADDR, LM75_DRIVER);
  } else {
      SCM_I2C_DEVICE_CHANGE_LIST[i].bus_id = bus_id;
      SCM_I2C_DEVICE_CHANGE_LIST[i++].device_address = SCM_LM75_4D_ADDR;
  }

  /* channel 3 of paca9548 */
  bus_id = 19 ;
  if (pal_add_i2c_device(bus_id, SCM_EEPROM_52_ADDR, EEPROM_24C64_DRIVER)) {
      syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                        bus_id, SCM_EEPROM_52_ADDR, EEPROM_24C64_DRIVER);
  } else {
      SCM_I2C_DEVICE_CHANGE_LIST[i].bus_id = bus_id;
      SCM_I2C_DEVICE_CHANGE_LIST[i++].device_address = SCM_EEPROM_52_ADDR;
  }

  /* channel 4 of paca9548 */
  bus_id = 20 ;
  if (pal_add_i2c_device(bus_id, SCM_EEPROM_50_ADDR, EEPROM_24C02_DRIVER)) {
      syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                        bus_id, SCM_EEPROM_50_ADDR, EEPROM_24C02_DRIVER);
  } else {
      SCM_I2C_DEVICE_CHANGE_LIST[i].bus_id = bus_id;
      SCM_I2C_DEVICE_CHANGE_LIST[i++].device_address = SCM_EEPROM_50_ADDR;
  }

  /* channel 8 of paca9548 */
  bus_id = 22 ;
  if (pal_add_i2c_device(bus_id, SCM_EEPROM_52_ADDR, EEPROM_24C02_DRIVER)) {
      syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x Device:%s "
                       "driver cannot add.",
                        bus_id, SCM_EEPROM_52_ADDR, EEPROM_24C02_DRIVER);
  } else {
      SCM_I2C_DEVICE_CHANGE_LIST[i].bus_id = bus_id;
      SCM_I2C_DEVICE_CHANGE_LIST[i++].device_address = SCM_EEPROM_52_ADDR;
  }
  sleep(1);
  run_command(". openbmc-utils.sh && i2c_check_driver_binding fix-binding");
}

static void
scm_driver_del() {
  int i;
  int state = -1;

  for (i = 0; i < SCM_I2C_DEVICE_NUMBER; i++) {
    /* i2c slave address should not be  0 */
    if (SCM_I2C_DEVICE_CHANGE_LIST[i].device_address != 0){
      state = pal_del_i2c_device(
                SCM_I2C_DEVICE_CHANGE_LIST[i].bus_id,
                SCM_I2C_DEVICE_CHANGE_LIST[i].device_address);
      if (state != 0) {
        syslog(LOG_CRIT, "SCM Bus:%d Addr:0x%x driver cannot delete.",
                  SCM_I2C_DEVICE_CHANGE_LIST[i].bus_id,
                  SCM_I2C_DEVICE_CHANGE_LIST[i].device_address);
        continue;
      }
      SCM_I2C_DEVICE_CHANGE_LIST[i].bus_id = 0;
      SCM_I2C_DEVICE_CHANGE_LIST[i].device_address = 0;
    }
  }
  scm_i2c_mux_unbind();
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
  uint8_t pim_type_old[10] = {PIM_TYPE_UNPLUG};
  uint8_t interval[10];

  memset(interval, INTERVAL_MAX, sizeof(interval));
  memset(PIM_I2C_DEVICE_CHANGE_LIST, 0, sizeof(PIM_I2C_DEVICE_CHANGE_LIST));
  while (1) {
    for (fru = FRU_PIM1; fru <= FRU_PIM8; fru++) {
      ret = pal_is_fru_prsnt(fru, &prsnt);
      if (ret) {
        continue;
      }
      /* Get original prsnt state PIM1 @bit0, PIM2 @bit1, ..., PIM8 @bit7 */
      num = fru - FRU_PIM1 + 1;
      prsnt_ori = GETBIT(curr_state, (num - 1));
      /* 1 is prsnt, 0 is not prsnt. */
      if (prsnt != prsnt_ori) {
        if (prsnt) {
          syslog(LOG_WARNING, "PIM %d is plugged in.", num);
          pim_driver_add(num);
          pim_type = pal_get_pim_type(fru, PIM_RETRY);

          if (pim_type != pim_type_old[num]) {
            if (pim_type == PIM_TYPE_16Q) {
              if (!pal_set_pim_type_to_file(fru, PIM_TYPE_16Q)) {
                syslog(LOG_INFO, "PIM %d type is 16Q", num);
                pim_type_old[num] = PIM_TYPE_16Q;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16Q failed", num);
              }
              if (!pal_set_pim_pedigree_to_file(fru, PIM_16O_NONE_VERSION)) {
                syslog(LOG_WARNING,
                      "pal_set_pim_pedigree_to_file: PIM %d For PIM16Q set pedigree to none", num);
              } else {
                syslog(LOG_WARNING,
                      "pal_set_pim_pedigree_to_file: PIM %d set none failed", num);
              }

              pim_phy_type = pal_get_pim_phy_type(fru, PIM_RETRY);
              if (pal_set_pim_phy_type_to_file(fru, pim_phy_type)) {
                syslog(LOG_WARNING,
                      "pal_set_pim_phy_type_to_file: PIM %d fail to set phy type", num);
              }
            } else if (pim_type == PIM_TYPE_16O) {
              if (!pal_set_pim_type_to_file(fru, PIM_TYPE_16O)) {
                syslog(LOG_INFO, "PIM %d type is 16O", num);
                pim_type_old[num] = PIM_TYPE_16O;
              } else {
                syslog(LOG_WARNING,
                       "pal_set_pim_type_to_file: PIM %d set 16O failed", num);
              }
              pim_pedigree = pal_get_pim_pedigree(fru, PIM_RETRY);
              if (pal_set_pim_pedigree_to_file(fru, pim_pedigree)) {
                syslog(LOG_WARNING,
                      "pal_set_pim_pedigree_to_file: PIM %d set version failed",
                       num);
              }
            } else {
              if (!pal_set_pim_type_to_file(fru, PIM_TYPE_NONE)) {
                syslog(LOG_CRIT,
                        "PIM %d type cannot detect, DOMFPGA get fail", num);
                pim_type_old[num] = PIM_TYPE_NONE;
              } else {
                syslog(LOG_WARNING,
                      "pal_set_pim_type_to_file: PIM %d set none failed", num);
              }
              if (pal_set_pim_pedigree_to_file(fru, PIM_16O_NONE_VERSION)) {
                syslog(LOG_WARNING,
                       "pal_set_pim_pedigree_to_file: PIM %d set none failed",
                       num);
              }
            }
            pal_set_sdr_update_flag(fru,1);
          }
        } else {
          syslog(LOG_WARNING, "PIM %d is unplugged.", num);
          clear_pimserial_cache(num);
          pal_clear_sensor_cache_value(fru);
          pal_clear_thresh_value(fru);
          pal_set_sdr_update_flag(fru,1);
          pim_driver_del(num, pim_type_old[num]);
          pim_type_old[num] = PIM_TYPE_UNPLUG;
          if (pal_set_pim_type_to_file(fru, PIM_TYPE_UNPLUG)) {
            syslog(LOG_WARNING,
                  "pal_set_pim_type_to_file: PIM %d set unplug failed", num);
          }
          if (pal_set_pim_pedigree_to_file(fru, PIM_16O_NONE_VERSION)) {
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

  memset(SCM_I2C_DEVICE_CHANGE_LIST, 0, sizeof(SCM_I2C_DEVICE_CHANGE_LIST));
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
        sleep(1);
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
          sleep(5);
        }
        // add scm driver after SCM power on
        scm_driver_add();

        if ( ! is_process_run(BICMOND_NAME) ) {
          syslog(LOG_WARNING, "BICMOND didn't run yet, Start BICMOND\n");
          if ( system(BICMOND_DEAMON) < 0 ) {
            syslog(LOG_CRIT, "failed to Start BICMOND\n");
          }
          if ( system("systemctl restart setup_bic_cache.service") < 0) {
            syslog(LOG_CRIT, "failed to restart setup_bic_cache\n");
          }
        }
      } else {
        // SCM was removed
        syslog(LOG_WARNING, "SCM Extraction\n");
        scm_driver_del();
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
      SMB_XP0R84V_CSU,                  SMB_XP1R8V_CSU,
      SMB_XP3R3V_TCXO,                  SMB_OUTPUT_VOLTAGE_XP0R75V_1,
      SMB_OUTPUT_CURRENT_XP0R75V_1,     SMB_INPUT_VOLTAGE_1,
      SMB_OUTPUT_VOLTAGE_XP1R2V,        SMB_OUTPUT_CURRENT_XP1R2V,
      SMB_OUTPUT_VOLTAGE_XP0R75V_2,     SMB_OUTPUT_CURRENT_XP0R75V_2,
      SMB_INPUT_VOLTAGE_2,              
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
      //PSU LED
      // BLUE     all PSUs present and INPUT OK,PWR OK
      // AMBER    one or more not present or INPUT OK or PWR OK de-asserted
        if (psu_check(brd_rev) == 0) {
          leds[SLED_NAME_PSU] = SIM_LED_BLUE;
        } else {
          leds[SLED_NAME_PSU] = SIM_LED_AMBER;
        }
      //SMB LED
      // BLUE   sensor ok
      // AMBER  fail
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
