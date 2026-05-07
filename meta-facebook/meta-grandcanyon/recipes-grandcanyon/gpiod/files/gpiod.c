/*
 * gpiod
 *
 * Copyright 2021-present Facebook. All Rights Reserved.
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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/file.h>
#include <openbmc/libgpio.h>
#include <openbmc/pal.h>
#include <openbmc/pal_sensors.h>
#include <openbmc/kv.h>
#include <facebook/fbgc_gpio.h>
#include <facebook/exp.h>
#include <facebook/fbgc_common.h>

#define MONITOR_FRUS_PRESENT_STATUS_INTERVAL    1  // seconds
#define MONITOR_SERVER_POWER_STATUS_INTERVAL    1  // seconds
#define MONITOR_SCC_STBY_POWER_INTERVAL         1  // seconds
#define MONITOR_SCC_STARTUP_UV_FAULT_INTERVAL   3  // seconds
#define SCC_STARTUP_DELAY_S                     5  // seconds
#define IPMI_RETRY_DELAY_MS                     100  // ms

static void
e1s_iocm_remove_event(int e1s_iocm_slot_id, uint8_t *present_status) {
  char cmd[MAX_PATH_LEN] = {0};
  uint8_t chassis_type = 0;

  if (present_status == NULL) {
    syslog(LOG_ERR, "%s() Failed to disable E1.S %d/IOCM I2C because the parameter is NULL\n", __func__, e1s_iocm_slot_id);
    return;
  }

  if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
    syslog(LOG_WARNING, "%s() Failed to get chassis type.\n", __func__);
    return;
  }

  if (present_status[e1s_iocm_slot_id] == FRU_ABSENT) {
    if ((chassis_type == CHASSIS_TYPE7) && (e1s_iocm_slot_id == T5_E1S0_T7_IOC_AVENGER)) {
      memset(cmd, 0, sizeof(cmd));
      snprintf(cmd, sizeof(cmd), "sv stop iocd_%d > /dev/null 2>&1", I2C_T5E1S0_T7IOC_BUS);
      if (system(cmd) != 0) {
        syslog(LOG_WARNING, "%s() Fail to stop IOC Daemon:%d\n", __func__, I2C_T5E1S0_T7IOC_BUS);
        return;
      }
    }
  }
}

static void
e1s_iocm_insert_event(int e1s_iocm_slot_id, uint8_t *present_status) {
  char cmd[MAX_PATH_LEN] = {0};
  uint8_t chassis_type = 0;
  uint8_t server_power_status = SERVER_POWER_ON;

  if (present_status == NULL) {
    syslog(LOG_ERR, "%s() Failed to enable E1.S %d/IOCM I2C because the parameter is NULL\n", __func__, e1s_iocm_slot_id);
    return;
  }

  if (pal_get_server_power(FRU_SERVER, &server_power_status) < 0) {
    syslog(LOG_ERR, "%s() Failed to enable E1.S %d/IOCM I2C because failed to get server power status\n", __func__, e1s_iocm_slot_id);
    return;
  }

  if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
    syslog(LOG_WARNING, "%s() Failed to get chassis type.\n", __func__);
    return;
  }

  if ((present_status[e1s_iocm_slot_id] == FRU_PRESENT) && (server_power_status == SERVER_POWER_ON)) {
    if ((chassis_type == CHASSIS_TYPE7) && (e1s_iocm_slot_id == T5_E1S0_T7_IOC_AVENGER)) {
      memset(cmd, 0, sizeof(cmd));
      snprintf(cmd, sizeof(cmd), "sv start iocd_%d > /dev/null 2>&1", I2C_T5E1S0_T7IOC_BUS);
      if (system(cmd) != 0) {
        syslog(LOG_WARNING, "%s() Fail to start IOC Daemon:%d\n", __func__, I2C_T5E1S0_T7IOC_BUS);
        return;
      }
    }
  }
}

static void
fru_remove_event(int fru_id, uint8_t *e1s_iocm_present_status) {
  int ret = 0;
  uint8_t chassis_type = 0;
  char cmd[MAX_FILE_PATH] = {0};

  if (fru_id == FRU_SERVER) {
    // AC off server
    ret = pal_set_server_power(FRU_SERVER, SERVER_12V_OFF);
    if (ret < 0) {
      syslog(LOG_ERR, "%s(): Failed to AC off server\n", __func__);
    }
    
    pal_set_error_code(ERR_CODE_SERVER_MISSING, ERR_CODE_ENABLE);
    
  } else if (fru_id == FRU_SCC) {
    // Stop SCC IOCD
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "sv stop iocd_%d > /dev/null 2>&1", I2C_T5IOC_BUS);
    if (system(cmd) != 0) {
      syslog(LOG_WARNING, "%s() Fail to stop IOC Daemon:%d\n", __func__, I2C_T5IOC_BUS);
      return;
    }
    // AC off SCC
    ret = gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_SCC_STBY_PWR_EN), GPIO_VALUE_LOW);
    if (ret < 0) {
      syslog(LOG_ERR, "%s(): Failed to AC off SCC\n", __func__);
    }
    
    pal_set_error_code(ERR_CODE_SCC_MISSING, ERR_CODE_ENABLE);
    
  } else if (fru_id == FRU_E1S_IOCM) {
    if (e1s_iocm_present_status == NULL) {
      syslog(LOG_ERR, "%s(): Failed to deal with remove event because the parameter: *e1s_iocm_present_status is NULL\n", __func__);
      return;
    }
    e1s_iocm_remove_event(T5_E1S0_T7_IOC_AVENGER, e1s_iocm_present_status);
    e1s_iocm_remove_event(T5_E1S1_T7_IOCM_VOLT, e1s_iocm_present_status);
    
    if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
      pal_set_error_code(ERR_CODE_E1S_MISSING, ERR_CODE_ENABLE);
      pal_set_error_code(ERR_CODE_IOCM_MISSING, ERR_CODE_ENABLE);
    } else {
      if (chassis_type == CHASSIS_TYPE7) {
        pal_set_error_code(ERR_CODE_IOCM_MISSING, ERR_CODE_ENABLE);
      } else if (chassis_type == CHASSIS_TYPE5) {
        pal_set_error_code(ERR_CODE_E1S_MISSING, ERR_CODE_ENABLE);
      } else {
        pal_set_error_code(ERR_CODE_E1S_MISSING, ERR_CODE_ENABLE);
        pal_set_error_code(ERR_CODE_IOCM_MISSING, ERR_CODE_ENABLE);
      }
    }
  }
}

static void
fru_insert_event(int fru_id, uint8_t *e1s_iocm_present_status) {
  int ret = 0;
  uint8_t chassis_type = 0;
  char power_policy_cfg[MAX_VALUE_LEN] = {0};
  
  memset(power_policy_cfg, 0, sizeof(power_policy_cfg));

  if (fru_id == FRU_SERVER) {
    // AC on server
    ret = pal_set_server_power(FRU_SERVER, SERVER_12V_ON);
    if (ret < 0) {
      syslog(LOG_ERR, "%s(): Failed to AC on server\n", __func__);
    }
    
    //power policy
    ret = pal_get_key_value("server_por_cfg", power_policy_cfg);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s(): Failed to get power policy config\n", __func__);
      return;
    }
    
    if (strcmp(power_policy_cfg, "on") == 0) {
      sleep(3);
      ret = pal_set_server_power(FRU_SERVER, SERVER_POWER_ON);
      if (ret < 0) {
        syslog(LOG_ERR, "%s(): Failed to DC on server\n", __func__);
        return;
      }
    }
    
    pal_set_error_code(ERR_CODE_SERVER_MISSING, ERR_CODE_DISABLE);
    
  } else if (fru_id == FRU_SCC) {
    // AC on SCC
    ret = gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_SCC_STBY_PWR_EN), GPIO_VALUE_HIGH);
    if (ret < 0) {
      syslog(LOG_ERR, "%s(): Failed to AC on SCC\n", __func__);
    }
    
    pal_set_error_code(ERR_CODE_SCC_MISSING, ERR_CODE_DISABLE);
    
  } else if (fru_id == FRU_E1S_IOCM) {
    if (e1s_iocm_present_status == NULL) {
      syslog(LOG_ERR, "%s(): Failed to deal with insert event because the parameter: *e1s_iocm_present_status is NULL\n", __func__);
      return;
    }
    e1s_iocm_insert_event(T5_E1S0_T7_IOC_AVENGER, e1s_iocm_present_status);
    e1s_iocm_insert_event(T5_E1S1_T7_IOCM_VOLT, e1s_iocm_present_status);
    
    if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
      pal_set_error_code(ERR_CODE_E1S_MISSING, ERR_CODE_DISABLE);
      pal_set_error_code(ERR_CODE_IOCM_MISSING, ERR_CODE_DISABLE);
    } else {
      if (chassis_type == CHASSIS_TYPE7) {
        pal_set_error_code(ERR_CODE_IOCM_MISSING, ERR_CODE_DISABLE);
      } else if (chassis_type == CHASSIS_TYPE5) {
        pal_set_error_code(ERR_CODE_E1S_MISSING, ERR_CODE_DISABLE);
      }
    }
  }
}


static void *
fru_missing_monitor() {
  uint8_t fru_present_flag = 0, chassis_type = 0, uic_location_id = 0;
  uint8_t fru_present_status[FRU_CNT] = {FRU_PRESENT};
  uint8_t e1s_iocm_present_status[E1S_IOCM_SLOT_NUM] = {FRU_PRESENT};
  char fru_name[MAX_FRU_NAME_STR] = {0};
  char uic_location = '?';
  int fru_id = 0, e1s_iocm_slot_id = 0;
  
  memset(&fru_present_status, FRU_PRESENT, sizeof(fru_present_status));
  memset(&e1s_iocm_present_status, FRU_PRESENT, sizeof(e1s_iocm_present_status));
  memset(&fru_name, 0, sizeof(fru_name));
  
  // set flag to notice BMC gpiod fru_missing_monitor is ready
  kv_set("flag_gpiod_fru_miss", STR_VALUE_1, 0, 0);

  while(1) {
    for (fru_id = FRU_SERVER; fru_id < FRU_CNT; fru_id++) {
      if ((fru_id == FRU_SERVER) || (fru_id == FRU_SCC)) {
        if (pal_is_fru_prsnt(fru_id, &fru_present_flag) < 0) {
          syslog(LOG_WARNING, "%s(): fail to get fru: %d present status\n", __func__, fru_id);
        } else {
          if (pal_get_fru_name(fru_id, fru_name) < 0) {
            syslog(LOG_WARNING, "%s(): fail to get fru: %d name\n", __func__, fru_id);
          } else {
            // fru insert
            if ((fru_present_flag == FRU_PRESENT) && (fru_present_status[fru_id] == FRU_ABSENT)) {
              syslog(LOG_CRIT, "DEASSERT: %s missing\n", fru_name);
              fru_present_status[fru_id] = FRU_PRESENT;
              fru_insert_event(fru_id, NULL);
          
            // fru remove
            } else if ((fru_present_flag == FRU_ABSENT) && (fru_present_status[fru_id] == FRU_PRESENT)) {
              syslog(LOG_CRIT, "ASSERT: %s missing\n", fru_name);
              fru_present_status[fru_id] = FRU_ABSENT;
              fru_remove_event(fru_id, NULL);
            }
          } 
        }
      } 
      
      if (fru_id == FRU_E1S_IOCM) {
        if (fbgc_common_get_chassis_type(&chassis_type) < 0) {
          chassis_type = -1;
        }
        
        //Type 7
        if (chassis_type == CHASSIS_TYPE7) {
          if ((is_e1s_iocm_present(T5_E1S0_T7_IOC_AVENGER) == true) && (is_e1s_iocm_present(T5_E1S1_T7_IOCM_VOLT) == true)) {
            if (fru_present_status[fru_id] == FRU_ABSENT) {
              syslog(LOG_CRIT, "DEASSERT: iocm missing\n");
              fru_present_status[fru_id] = FRU_PRESENT;
              e1s_iocm_present_status[T5_E1S0_T7_IOC_AVENGER] = FRU_PRESENT;
              e1s_iocm_present_status[T5_E1S1_T7_IOCM_VOLT] = FRU_PRESENT;
              fru_insert_event(fru_id, e1s_iocm_present_status);
            }
          
          } else {
            if (fru_present_status[fru_id] == FRU_PRESENT) {
              syslog(LOG_CRIT, "ASSERT: iocm missing\n");
              fru_present_status[fru_id] = FRU_ABSENT;

              if (is_e1s_iocm_present(T5_E1S0_T7_IOC_AVENGER) == false) {
                e1s_iocm_present_status[T5_E1S0_T7_IOC_AVENGER] = FRU_ABSENT;
              }
              
              if (is_e1s_iocm_present(T5_E1S1_T7_IOCM_VOLT) == false) {
                e1s_iocm_present_status[T5_E1S1_T7_IOCM_VOLT] = FRU_ABSENT;
              }

              fru_remove_event(fru_id, e1s_iocm_present_status);
            }
          }
        
        // Type 5 and Type unknown
        } else {
          if (pal_get_uic_location(&uic_location_id) < 0) {
            syslog(LOG_WARNING, "%s(): fail to get uic location\n", __func__);
            uic_location = '?';
          } else {
            if(uic_location_id == UIC_SIDEA) {
              uic_location = 'a';
            } else if(uic_location_id == UIC_SIDEB) {
              uic_location = 'b';
            } else {
              uic_location = '?';
            }
          }
            
          for (e1s_iocm_slot_id = T5_E1S0_T7_IOC_AVENGER; e1s_iocm_slot_id < E1S_IOCM_SLOT_NUM; e1s_iocm_slot_id++) {
            if ((is_e1s_iocm_present(e1s_iocm_slot_id) == true) && (e1s_iocm_present_status[e1s_iocm_slot_id] == FRU_ABSENT)) {
              if (chassis_type == CHASSIS_TYPE5) {
                syslog(LOG_CRIT, "DEASSERT: e1.s %c%d missing\n", uic_location, e1s_iocm_slot_id);
              } else {
                syslog(LOG_CRIT, "DEASSERT: chassis type unknown, e1.s %d or iocm missing\n", e1s_iocm_slot_id);
              }
              e1s_iocm_present_status[e1s_iocm_slot_id] = FRU_PRESENT;
              fru_insert_event(fru_id, e1s_iocm_present_status);
            }
          
            if ((is_e1s_iocm_present(e1s_iocm_slot_id) == false) && (e1s_iocm_present_status[e1s_iocm_slot_id] == FRU_PRESENT)) {
              if (chassis_type == CHASSIS_TYPE5) {
                syslog(LOG_CRIT, "ASSERT: e1.s %c%d missing\n", uic_location, e1s_iocm_slot_id);
              } else {
                syslog(LOG_CRIT, "ASSERT: chassis type unknown, e1.s %d or iocm missing\n", e1s_iocm_slot_id);
              }
              e1s_iocm_present_status[e1s_iocm_slot_id] = FRU_ABSENT;
              fru_present_status[fru_id] = FRU_ABSENT;
              fru_remove_event(fru_id, e1s_iocm_present_status);
            }
          }
            
          if ((is_e1s_iocm_present(T5_E1S0_T7_IOC_AVENGER) == true) && (is_e1s_iocm_present(T5_E1S1_T7_IOCM_VOLT) == true)) {
            fru_present_status[fru_id] = FRU_PRESENT;
          }
        }
      }
      
    } // for loop end
    
    sleep(MONITOR_FRUS_PRESENT_STATUS_INTERVAL);
  } // while loop end
  
  pthread_exit(NULL);
}

static void *
server_power_monitor() {
  uint8_t server_present = FRU_PRESENT;
  uint8_t server_pre_pwr_status = -1, server_cur_pwr_status = -1;
  int ret = 0;
  
  // set flag to notice BMC gpiod server_power_monitor is ready
  kv_set("flag_gpiod_server_pwr", STR_VALUE_1, 0, 0);

  while(1) {
    if (pal_is_fru_prsnt(FRU_SERVER, &server_present) < 0) {
      syslog(LOG_WARNING, "%s(): fail to get fru: %d present status\n", __func__, FRU_SERVER);
    } else {
      // if server is present, monitor server power status
      if (server_present == FRU_PRESENT) {
        ret = pal_get_server_power(FRU_SERVER, &server_cur_pwr_status);
        if (ret == 0) {

          //*****Server power from on change to off
          if ((server_pre_pwr_status == SERVER_POWER_ON)
           && ((server_cur_pwr_status == SERVER_POWER_OFF) || (server_cur_pwr_status == SERVER_12V_OFF))) {
            syslog(LOG_CRIT, "FRU: %d, Server is powered off", FRU_SERVER);
        
          //*****Server power from off change to on
          } else if (((server_pre_pwr_status == SERVER_POWER_OFF) || (server_pre_pwr_status == SERVER_12V_OFF))
                   && (server_cur_pwr_status == SERVER_POWER_ON)) {
            syslog(LOG_CRIT, "FRU: %d, Server is powered on", FRU_SERVER);
          }

          server_pre_pwr_status = server_cur_pwr_status;
        }
      } // server present end
    }
    
    sleep(MONITOR_SERVER_POWER_STATUS_INTERVAL);
  } // while loop end

  pthread_exit(NULL);
}

static pthread_mutex_t scc_startup_flag_mutex = PTHREAD_MUTEX_INITIALIZER;
static volatile bool scc_startup_clear_done = false;

static int
scc_flag_lock(const char *caller)
{
  int ret = pthread_mutex_lock(&scc_startup_flag_mutex);
  if (ret != 0) {
    syslog(LOG_ERR, "%s: pthread_mutex_lock failed: %s", caller, strerror(ret));
  }
  return ret;
}

static int
scc_flag_unlock(const char *caller)
{
  int ret = pthread_mutex_unlock(&scc_startup_flag_mutex);
  if (ret != 0) {
    syslog(LOG_ERR, "%s: pthread_mutex_unlock failed: %s", caller, strerror(ret));
  }
  return ret;
}

static void*
scc_stby_power_monitor() {
  gpio_value_t scc_stby_pg_value = GPIO_VALUE_INVALID;
  gpio_value_t scc_i2c_en_value = GPIO_VALUE_INVALID;
  const char * STR_SCC_STBY_PGOOD = fbgc_get_gpio_name(GPIO_SCC_STBY_PGOOD);
  const char * STR_SCC_I2C_EN_R = fbgc_get_gpio_name(GPIO_SCC_I2C_EN_R);

  if (STR_SCC_STBY_PGOOD == NULL || STR_SCC_I2C_EN_R == NULL) {
    syslog(LOG_ERR, "Failed to start SCC stby power monitor, GPIO name mapping error");
    pthread_exit(NULL);
  }

  // set flag to notice BMC gpiod scc_stby_power_monitor is ready
  kv_set("flag_gpiod_scc_pwr", STR_VALUE_1, 0, 0);

  while (1) {
    scc_stby_pg_value = gpio_get_value_by_shadow(STR_SCC_STBY_PGOOD);
    scc_i2c_en_value = gpio_get_value_by_shadow(STR_SCC_I2C_EN_R);
    // sync GPIO_SCC_I2C_EN_R with GPIO_SCC_STBY_PGOOD for leakage prevention
    if ((scc_stby_pg_value != GPIO_VALUE_INVALID) && (scc_stby_pg_value != scc_i2c_en_value)) {
      if (gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_SCC_I2C_EN_R), scc_stby_pg_value) < 0) {
        syslog(LOG_WARNING, "%s(): Failed to set GPIO_SCC_I2C_EN_R.\n", __func__);
      }
    }

    if (scc_stby_pg_value == GPIO_VALUE_LOW) {
      if (scc_flag_lock(__func__) == 0) {
        scc_startup_clear_done = false;
        scc_flag_unlock(__func__);
      }
    }

    sleep(MONITOR_SCC_STBY_POWER_INTERVAL);
  }

  pthread_exit(NULL);
  return NULL;
}

const efuse_profile_t efuse_mfr_fault_configs[] = {
  {
    .mfr = MFR_MPS,
    .name = "MPS",
    .uv_mask = UV_MASK,
    .other_fault_mask = MPS_OTHER_FAULT_MASK,
  },
  {
    .mfr = MFR_TI,
    .name = "TI",
    .uv_mask = UV_MASK,
    .other_fault_mask = TI_OTHER_FAULT_MASK,
  },
};

const efuse_profile_t *
get_efuse_mfr_fault_config(mfr_id_t mfr)
{
  size_t i;

  for (i = 0; i < (sizeof(efuse_mfr_fault_configs) / sizeof(efuse_mfr_fault_configs[0])); i++) {
    if (efuse_mfr_fault_configs[i].mfr == mfr) {
      return &efuse_mfr_fault_configs[i];
    }
  }

  return NULL;
}

static int
read_status_word(uint8_t bus, uint8_t addr, uint16_t *status_word)
{
  uint8_t txbuf[4] = {0};
  uint8_t rxbuf[4] = {0};
  uint8_t rxlen = 0;
  uint8_t bus_sel = bus * 2 + 1;
  int ret;
  int retry;

  if (status_word == NULL) {
    return -1;
  }

  txbuf[0] = bus_sel;
  txbuf[1] = addr;
  txbuf[2] = BLOCK_READ_2BYTE;
  txbuf[3] = PMBUS_STATUS_WORD;

  for (retry = 0; retry < MAX_RETRY; retry++) {

    rxlen = 0;
    ret = expander_ipmb_wrapper(EXPANDER_NETFN, EXPANDER_CMD,
                                txbuf, sizeof(txbuf),
                                rxbuf, &rxlen);

    if (ret == 0 && rxlen >= 2) {
      *status_word = (uint16_t)rxbuf[0] | ((uint16_t)rxbuf[1] << 8);
      return 0;
    }

    syslog(LOG_WARNING, "%s: read STATUS_WORD retry %d failed (ret=%d len=%u), bus=[%u] addr=0x%02X", __func__, retry + 1, ret, rxlen, bus, addr);

    msleep(IPMI_RETRY_DELAY_MS);
  }

  syslog(LOG_WARNING, "%s: read STATUS_WORD failed after %d retries, bus=exp[%u] addr=0x%02X", __func__, MAX_RETRY, bus, addr);

  return -1;
}

static int
clear_faults(uint8_t bus, uint8_t addr)
{
  uint8_t txbuf[4] = {0};
  uint8_t rxbuf[4] = {0};
  uint8_t rxlen = 0;
  uint8_t bus_sel = bus * 2 + 1;
  int ret;
  int retry;

  txbuf[0] = bus_sel;
  txbuf[1] = addr;
  txbuf[2] = WRITE_BYTE;
  txbuf[3] = PMBUS_CLEAR_FAULTS;

  for (retry = 0; retry < MAX_RETRY; retry++) {

    rxlen = 0;
    ret = expander_ipmb_wrapper(EXPANDER_NETFN, EXPANDER_CMD,
                                txbuf, sizeof(txbuf),
                                rxbuf, &rxlen);

    if (ret == 0) {
      return 0;
    }

    syslog(LOG_WARNING, "%s: CLEAR_FAULTS retry %d failed (ret=%d), bus=exp[%u] addr=0x%02X", __func__, retry + 1, ret, bus, addr);

    msleep(IPMI_RETRY_DELAY_MS);
  }

  syslog(LOG_WARNING, "%s: CLEAR_FAULTS failed after %d retries, bus=exp[%u] addr=0x%02X", __func__, MAX_RETRY, bus, addr);

  return -1;
}

static inline fault_type_t
classify_fault(uint16_t status_word, int status_out, const efuse_profile_t *fault_config)
{
  bool has_uv, has_other, has_input;

  if (fault_config == NULL) {
    return FAULT_INVALID;
  }

  has_uv = !!(status_word & fault_config->uv_mask);
  has_other = !!(status_word & fault_config->other_fault_mask);
  has_input = !!(status_word & fault_config->other_fault_mask & INPUT_FAULT_BIT);

  /*
   * TI: STATUS_WORD bit15 only means STATUS_VOUT(7Ah) has active bits.
   * If 7Ah bit5 VOUT_UV_WARN is set during startup, treat it as UV-only.
   */
  if (fault_config->mfr == MFR_TI && (status_word & OUT_STATUS_BIT)) {
    if (status_out & STATUS_OUT_VOUT_UV_WARN) {
      has_uv = true;
      has_other = !!(status_word & (fault_config->other_fault_mask & ~OUT_STATUS_BIT));
    }
  }

  if (!has_uv && !has_other)
    return FAULT_NONE;

  if (has_uv && !has_other)
    return FAULT_UV_ONLY;

  if (!has_uv && has_other)
    return FAULT_OTHER_ONLY;

  if (has_uv && has_other){
    if(!has_input) {
      return FAULT_UV_AND_OTHER;
    }
    return FAULT_UV_ONLY;
  }

  return FAULT_INVALID;
}

static void
scc_stby_uv_fault_check(void)
{
  mfr_id_t mfr;
  const efuse_profile_t *fault_config;
  uint16_t status_word = 0;
  uint16_t status_after = 0;
  uint8_t status_out = 0;
  const char *comp = "P12V_STBY_SCC";
  fault_type_t fault_type;
  int ret;

  sleep(SCC_STARTUP_DELAY_S);

  mfr = pal_detect_efuse_mfr_id(SCC_STBY_BUS, SCC_STBY_ADDR);
  fault_config = get_efuse_mfr_fault_config(mfr);
  if (fault_config == NULL) {
    syslog(LOG_WARNING, "%s: %s unknown MFR_ID (%s), bus=%u addr=0x%02X",
           __func__, comp, pal_get_mfr_name(mfr), SCC_STBY_BUS, SCC_STBY_ADDR);
    return;
  }

  ret = read_status_word(SCC_STBY_BUS, SCC_STBY_ADDR, &status_word);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: %s read STATUS_WORD(79h) failed", __func__, comp);
    return;
  }

  if (mfr == MFR_TI && (status_word & OUT_STATUS_BIT)) {
    ret = pal_read_pmbus_byte_from_exp(SCC_STBY_BUS, SCC_STBY_ADDR,
                          PMBUS_STATUS_VOUT, BLOCK_READ_1BYTE, &status_out);
    if (ret < 0) {
      syslog(LOG_WARNING, "%s: %s read STATUS_OUT(7Ah) failed", __func__, comp);
      return;
    }
  }

  fault_type = classify_fault(status_word, status_out, fault_config);

  switch (fault_type) {
    case FAULT_NONE:
      syslog(LOG_INFO, "%s: %s: no UV fault detected, STATUS_WORD(79h): 0x%04X, skip clear fault on startup", __func__, comp, status_word);
      return;
    case FAULT_OTHER_ONLY:
    case FAULT_UV_ONLY:
    case FAULT_UV_AND_OTHER:
      //Go ahead and clear the fault.
      break;
    default:
      syslog(LOG_WARNING, "%s: %s invalid fault classification, STATUS_WORD(79h): 0x%04X", __func__, comp, status_word);
      return;
  }

  /* Step 4: clear UV fault */
  ret = clear_faults(SCC_STBY_BUS, SCC_STBY_ADDR);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: %s CLEAR_FAULTS failed, STATUS_WORD(79h): 0x%04X", __func__, comp, status_word);
    return;
  }

  ret = read_status_word(SCC_STBY_BUS, SCC_STBY_ADDR, &status_after);
  if (ret < 0) {
    syslog(LOG_WARNING, "%s: %s CLEAR_FAULTS done but readback failed, STATUS_WORD(79h): 0x%04X", __func__, comp, status_word);
    return;
  }

  switch (fault_type) {
    case FAULT_UV_AND_OTHER:
      syslog(LOG_CRIT, "%s: %s: UV fault and other fault detected, cleared STATUS_WORD(79h): 0x%04X -> 0x%04X",
             __func__, comp, status_word, status_after);
      break;
    case FAULT_OTHER_ONLY:
      syslog(LOG_CRIT, "%s: %s: Other fault detected, cleared STATUS_WORD(79h): 0x%04X -> 0x%04X",
             __func__, comp, status_word, status_after);
      break;
    case FAULT_UV_ONLY:
    default:
      syslog(LOG_INFO, "%s: %s: UV fault detected, cleared STATUS_WORD(79h): 0x%04X -> 0x%04X",
             __func__, comp, status_word, status_after);
      break;
  }
}

static void *
scc_stby_uv_fault_monitor(void *arg)
{
  gpio_value_t scc_stby_pg_value = GPIO_VALUE_INVALID;
  bool local_clear_done = false;
  const char *str_scc_stby_pgood = fbgc_get_gpio_name(GPIO_SCC_STBY_PGOOD);

  if (str_scc_stby_pgood == NULL) {
    syslog(LOG_ERR, "%s: failed to start, GPIO name mapping error", __func__);
    pthread_exit(NULL);
  }

  /* initial state */
  kv_set("flag_gpiod_scc_fault", STR_VALUE_1, 0, 0);

  while (1) {
    scc_stby_pg_value = gpio_get_value_by_shadow(str_scc_stby_pgood);
    if (scc_stby_pg_value == GPIO_VALUE_INVALID) {
      syslog(LOG_WARNING, "%s: failed to read %s", __func__, str_scc_stby_pgood);
      sleep(MONITOR_SCC_STARTUP_UV_FAULT_INTERVAL);
      continue;
    }

    /* LOW -> HIGH : real SCC startup */
   if (scc_stby_pg_value == GPIO_VALUE_HIGH) {
      if (scc_flag_lock(__func__) == 0) {
        local_clear_done = scc_startup_clear_done;
        scc_flag_unlock(__func__);
      } else {
        sleep(MONITOR_SCC_STARTUP_UV_FAULT_INTERVAL);
        continue;
      }

      if (!local_clear_done) {
        scc_stby_uv_fault_check();

        if (scc_flag_lock(__func__) == 0) {
          scc_startup_clear_done = true;
          scc_flag_unlock(__func__);
        }
      }
    }

    if (scc_stby_pg_value == GPIO_VALUE_LOW) {
      if (scc_flag_lock(__func__) == 0) {
        scc_startup_clear_done = false;
        scc_flag_unlock(__func__);
      }
    }
    sleep(MONITOR_SCC_STARTUP_UV_FAULT_INTERVAL);
  }

  return NULL;
}

static void
print_usage() {
  printf("Usage: gpiod [ %s ]\n", pal_server_list);
}

static void
run_gpiod(int argc, char **argv) {
  pthread_t tid_fru_missing_monitor;
  pthread_t tid_server_power_monitor;
  pthread_t tid_scc_stby_power_monitor;
  pthread_t tid_scc_stby_uv_fault_monitor;
  uint8_t board_rev_id = 0xff;
  
  int ret_fru_missing = 0, ret_server_power = 0, ret_scc_stby_power = 0,  ret_scc_stby_uv_fault = 0;

  if (argv == NULL) {
    syslog(LOG_ERR, "fail to execute gpiod because NULL parameter: **argv\n");
    exit(EXIT_FAILURE);
  }
  
  // Monitor fru missing by polling (server, SCC, E1.S, IOCM)
  if (pthread_create(&tid_fru_missing_monitor, NULL, fru_missing_monitor, NULL) < 0) {
    syslog(LOG_ERR, "fail to creat thread for monitor fru missing\n");
    ret_fru_missing = -1;
  }
  
  // Monitor server power by polling
  if (pthread_create(&tid_server_power_monitor, NULL, server_power_monitor, NULL) < 0) {
    syslog(LOG_ERR, "fail to creat thread for monitor server host\n");
    ret_server_power = -1;
  }

  // Monitor scc standby power
  if (pthread_create(&tid_scc_stby_power_monitor, NULL, scc_stby_power_monitor, NULL) < 0) {
    syslog(LOG_ERR, "fail to creat thread for scc standby power monitor\n");
    ret_scc_stby_power = -1;
  }

  if (fbgc_common_get_system_stage(&board_rev_id) < 0) {
    syslog(LOG_WARNING, "%s: get stage failed", __func__);
    return;
  } else {
    if (board_rev_id == UIC_STAGE_MP) {
      syslog(LOG_INFO, "%s: Hack stage, skip", __func__);
      ret_scc_stby_uv_fault = -1;
    } else {
      // Monitor scc standby start up uv fault
      if (pthread_create(&tid_scc_stby_uv_fault_monitor, NULL, scc_stby_uv_fault_monitor, NULL) < 0) {
        syslog(LOG_ERR, "fail to creat thread for scc standby start up uv fault monitor\n");
        ret_scc_stby_uv_fault = -1;
      }
    }
  }

  if (ret_fru_missing == 0) {
    pthread_join(tid_fru_missing_monitor, NULL);
  }

  if (ret_server_power == 0) {
    pthread_join(tid_server_power_monitor, NULL);
  }

  if (ret_scc_stby_power == 0) {
    pthread_join(tid_scc_stby_power_monitor, NULL);
  }

  if (ret_scc_stby_uv_fault == 0) {
    pthread_join(tid_scc_stby_uv_fault_monitor, NULL);
  }
}
 
int
main(int argc, char **argv) {
  int rc = 0, pid_file = 0;
  
  if (argv == NULL) {
    syslog(LOG_ERR, "fail to execute gpiod because NULL parameter: **argv\n");
    exit(EXIT_FAILURE);
  }

  if (argc < 2) {
    print_usage();
    exit(EXIT_FAILURE);
  }

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = pal_flock_retry(pid_file);
  if (rc < 0) {
    if (EWOULDBLOCK == errno) {
      printf("another gpiod instance is running...\n");
    } else {
      syslog(LOG_ERR, "fail to execute gpiod because %s\n", strerror(errno));
    }
    
    pal_unflock_retry(pid_file);
    close(pid_file);
    exit(EXIT_FAILURE);
    
  } else {
    syslog(LOG_INFO, "daemon started");
    run_gpiod(argc, argv);
  }
  
  pal_unflock_retry(pid_file);
  close(pid_file);
  return 0;
}
