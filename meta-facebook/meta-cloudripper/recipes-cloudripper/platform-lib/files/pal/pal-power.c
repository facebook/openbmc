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

 /*
  * This file contains functions and logics that depends on Cloudripper specific
  * hardware and kernel drivers. Here, some of the empty "default" functions
  * are overridden with simple functions that returns non-zero error code.
  * This is for preventing any potential escape of failures through the
  * default functions that will return 0 no matter what.
  */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <openbmc/log.h>
#include <openbmc/libgpio.h>
#include <openbmc/kv.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/sensor-correction.h>
#include <openbmc/misc-utils.h>
#include <facebook/bic.h>
#include <facebook/wedge_eeprom.h>
#include "pal-power.h"
#include "pal.h"

int pal_set_last_pwr_state(uint8_t fru, char *state) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_set_key_value(key, state);
  if (ret < 0) {
    PAL_DEBUG("pal_set_last_pwr_state: pal_set_key_value failed for fru %u", fru);
  }
  return ret;
}

int pal_get_last_pwr_state(uint8_t fru, char *state) {
  int ret;
  char key[MAX_KEY_LEN];

  sprintf(key, "%s", "pwr_server_last_state");

  ret = pal_get_key_value(key, state);
  if (ret < 0) {
    PAL_DEBUG("pal_get_last_pwr_state: pal_get_key_value failed for fru %u", fru);
  }
  return ret;
}

int pal_set_com_pwr_btn_n(char *status) {
  char path[64];
  int ret;
  sprintf(path, SCM_SYSFS, COM_PWR_BTN_N);

  ret = device_write_buff(path, status);
  if (ret) {
    PAL_DEBUG("device_write_buff failed for %s\n", path);
    return -1;
  }

  return 0;
}

// Power On the server
static int server_power_on(void) {
  int ret, val;

  ret = device_read(SCM_COM_PWR_ENBLE, &val);
  if (ret || val) {
    if (pal_set_com_pwr_btn_n("1")) {
      return -1;
    }

    if (pal_set_com_pwr_btn_n("0")) {
      return -1;
    }
    sleep(1);

    if (pal_set_com_pwr_btn_n("1")) {
      return -1;
    }
    /* Wait for server power good ready */
    sleep(1);

    if (!is_server_on()) {
      return -1;
    }
  } else {
    ret = device_write_buff(SCM_COM_PWR_ENBLE, "1");
    if (ret) {
      syslog(LOG_WARNING, "%s: Power on is failed", __func__);
      return -1;
    }
  }

  return 0;
}

// Power Off the server in given slot
static int server_power_off(bool gs_flag) {
  int ret;

  if (gs_flag) {
    ret = pal_set_com_pwr_btn_n("0");
    if (ret) {
      return -1;
    }
    sleep(DELAY_GRACEFUL_SHUTDOWN);

    ret = pal_set_com_pwr_btn_n("1");
    if (ret) {
      return -1;
    }
  } else {
    ret = device_write_buff(SCM_COM_PWR_ENBLE, "0");
    if (ret) {
      syslog(LOG_WARNING, "%s: Power off is failed",__func__);
      return -1;
    }
  }

  return 0;
}

int pal_get_server_power(uint8_t slot_id, uint8_t *status) {
  int ret;
  char value[MAX_VALUE_LEN];
  bic_gpio_t gpio;
  uint8_t retry = MAX_READ_RETRY;

  /* check if the CPU is turned on or not */
  while (retry) {
    ret = bic_get_gpio(IPMB_BUS, &gpio);
    if (!ret)
      break;
    msleep(50);
    retry--;
  }
  if (ret) {
    // Check for if the BIC is irresponsive due to 12V_OFF or 12V_CYCLE
    OBMC_INFO("pal_get_server_power: bic_get_gpio returned error hence"
        " reading the kv_store for last power state  for slot %d", slot_id);
    pal_get_last_pwr_state(slot_id, value);
    if (!(strncmp(value, "off", strlen("off")))) {
      *status = SERVER_POWER_OFF;
    } else if (!(strncmp(value, "on", strlen("on")))) {
      *status = SERVER_POWER_ON;
    } else {
      return ret;
    }
    return 0;
  }

  if (gpio.pwrgood_cpu) {
    *status = SERVER_POWER_ON;
  } else {
    *status = SERVER_POWER_OFF;
  }

  return 0;
}

// Power Off, Power On, or Power Reset the server in given slot
int pal_set_server_power(uint8_t slot_id, uint8_t cmd) {
  int ret = 0;
  uint8_t status;
  bool gs_flag = false;

  if (pal_get_server_power(slot_id, &status) < 0) {
    return -1;
  }

  switch(cmd) {
  case SERVER_POWER_ON:
    if (status == SERVER_POWER_ON)
      return 1;
    else
        return server_power_on();
    break;

  case SERVER_POWER_OFF:
    if (status == SERVER_POWER_OFF)
      return 1;
    else
      return server_power_off(gs_flag);
    break;

  case SERVER_POWER_CYCLE:
    if (status == SERVER_POWER_ON) {
      if (server_power_off(gs_flag))
        return -1;
      sleep(DELAY_POWER_CYCLE);
      return server_power_on();
    } else if (status == SERVER_POWER_OFF) {
      return server_power_on();
    }
    break;

  case SERVER_POWER_RESET:
    if (status == SERVER_POWER_ON) {
      ret = pal_set_rst_btn(slot_id, 0);
      if (ret < 0) {
        OBMC_CRIT("Micro-server can't power reset");
        return ret;
      }
      msleep(100); //some server miss to detect a quick pulse, so delay 100ms between low high
      ret = pal_set_rst_btn(slot_id, 1);
      if (ret < 0) {
        OBMC_CRIT("Micro-server in reset state, can't go back to normal state");
        return ret;
      }
    } else if (status == SERVER_POWER_OFF) {
        OBMC_CRIT("Micro-server power status is off, ignore power reset action");
      return -2;
    }
    break;

  case SERVER_GRACEFUL_SHUTDOWN:
    if (status == SERVER_POWER_OFF) {
      return 1;
    } else {
      gs_flag = true;
      return server_power_off(gs_flag);
    }
    break;

  default:
    ret = -1;
    break;
  }

  return ret;
}

bool is_server_on(void) {
  int ret;
  uint8_t status;
  ret = pal_get_server_power(FRU_SCM, &status);
  if (ret) {
    return false;
  }

  if (status == SERVER_POWER_ON) {
    return true;
  } else {
    return false;
  }
}

int pal_set_gb_power(int option) {
  char path[256];
  int ret;
  char sysfs[128];

  sprintf(sysfs, GB_POWER);

  switch(option) {
  case GB_POWER_ON:
    sprintf(path, SMB_SYSFS, sysfs);
    ret = device_write_buff(path, "1");
    break;
  case GB_POWER_OFF:
    sprintf(path, SMB_SYSFS, sysfs);
    ret = device_write_buff(path, "0");
    break;
  case GB_RESET:
    sprintf(path, SMB_SYSFS, sysfs);
    ret = device_write_buff(path, "0");
    sleep(1);
    ret = device_write_buff(path, "1");
    break;
  default:
    ret = -1;
  }
  if (ret)
    return -1;
  return 0;
}
