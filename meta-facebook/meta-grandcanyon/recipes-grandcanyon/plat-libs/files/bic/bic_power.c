/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specification available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openbmc/libgpio.h>
#include <facebook/fbgc_gpio.h>
#include <facebook/fbgc_common.h>
#include "bic_power.h"

#define BS_FPGA_PWR_BTN_BIT 0
#define BS_FPGA_RST_BTN_BIT 1

//The value of power button to FPGA
#define POWER_BTN_HIGH ((1 << BS_FPGA_PWR_BTN_BIT) | (1 << BS_FPGA_RST_BTN_BIT))
#define POWER_BTN_LOW  ((0 << BS_FPGA_PWR_BTN_BIT) | (1 << BS_FPGA_RST_BTN_BIT))

//The delay of power control
#define DELAY_DC_POWER_CYCLE 5
#define DELAY_DC_POWER_OFF 6
#define DELAY_GRACEFUL_SHUTDOWN 1
#define DELAY_DC_POWER_ON 1
#define DELAY_RESET 1

#define PWR_CTRL_ACT_CNT 3


static int
bic_set_pwr_btn(uint8_t val) {
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0x00};
  uint8_t rlen = 0;
  int ret = 0;
  bic_pwr_command command;
  
  memset(&command, 0, sizeof(command));
  command.bus_id = (BIC_I2C_FPGA_BUS << 1) | 1;
  command.slave_addr = BIC_I2C_FPGA_ADDR;
  command.rw_count = 0x01; //read 1 byte 
  command.offset = BIC_FPGA_SEVER_PWR_REG;
  command.val = val;

  ret = bic_ipmb_wrapper(NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, (uint8_t *)&command, sizeof(command), rbuf, &rlen);

  return ret;
}

int
bic_server_power_ctrl(int action) {
  uint8_t pwr_seq[PWR_CTRL_ACT_CNT] = {POWER_BTN_HIGH, POWER_BTN_LOW, POWER_BTN_HIGH};
  gpio_value_t rst_seq[PWR_CTRL_ACT_CNT] = {GPIO_VALUE_HIGH, GPIO_VALUE_LOW, GPIO_VALUE_HIGH};
  uint8_t status = 0;
  int ret = 0;
  int i = 0;
  int times = 0;
  
  if ( (action != SET_DC_POWER_OFF) && (action != SET_DC_POWER_ON) 
    && (action != SET_GRACEFUL_POWER_OFF) && (action != SET_HOST_RESET)) {
    syslog(LOG_ERR, "%s() invalid action\n", __func__);
    return -1;
  }
  
  //DC on/off: send comand to BIC
  //Reset:     trigger GPIO to UIC FPGA
  for (i = 0; i < PWR_CTRL_ACT_CNT; i++) {
    if (action == SET_HOST_RESET) {
      ret = gpio_set_value_by_shadow(fbgc_get_gpio_name(GPIO_BMC_COMP_SYS_RST_N_R), rst_seq[i]);
    } else {
      ret = bic_set_pwr_btn(pwr_seq[i]);
    }
    
    if (ret < 0) {
      syslog(LOG_ERR, "%s() Cannot set power state to %02X\n", __func__, (action == SET_HOST_RESET) ? rst_seq[i] : pwr_seq[i]);
      return ret;
    }

    if ((pwr_seq[i] == POWER_BTN_LOW) || (rst_seq[i] == GPIO_VALUE_LOW)) {
      switch (action) {
        case SET_DC_POWER_OFF:
          sleep(DELAY_DC_POWER_OFF);
          break;
        case SET_DC_POWER_ON:
          sleep(DELAY_DC_POWER_ON);
          break;
        case SET_GRACEFUL_POWER_OFF:
          sleep(DELAY_GRACEFUL_SHUTDOWN);
          break;
        case SET_HOST_RESET:
          sleep(DELAY_RESET);
          break;
        default:
          return -1;
      }
    }     
  }
  
  if (action == SET_HOST_RESET) {
    return ret;
  } else {
    while (times < WAIT_POWER_STATUS_CHANGE_TIME) {
      ret = bic_get_server_power_status(&status);
      if ((ret == 0) && (action == SET_DC_POWER_ON) && (status == STAT_DC_ON)) {
        return ret;
      } else if ((ret == 0) && (action == SET_DC_POWER_OFF) && (status == STAT_DC_OFF)) {
        return ret;
      } else if ((ret == 0) && (action == SET_GRACEFUL_POWER_OFF) && (status == STAT_DC_OFF)) {
        return ret;
      } else {
        sleep(1);
        times++;
      }
    }
    syslog(LOG_ERR, "%s() Cannot set power state\n", __func__);
    return -1;
  }
}

int
bic_server_power_cycle() {
  int ret = 0;

  ret = bic_server_power_ctrl(SET_DC_POWER_OFF);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot set power state to off\n", __func__);
    return ret;
  }

  sleep(DELAY_DC_POWER_CYCLE);

  ret = bic_server_power_ctrl(SET_DC_POWER_ON);
  if (ret < 0) {
    syslog(LOG_ERR, "%s() Cannot set power state to on\n", __func__);
    return ret;
  }

  return ret; 
}

int
bic_get_server_power_status(uint8_t *power_status) {
  uint8_t rbuf[MAX_IPMB_BUFFER] = {0};
  uint8_t ianalen = 3;
  uint8_t rlen = 0;
  int ret = 0;
  bic_pwr_status_command command;
  bic_pwr_status_result result;
  
  if (power_status == NULL) {
    syslog(LOG_WARNING, "%s() NULL pointer: *power_status", __func__);
    return -1;
  }
  
  memset(&command, 0, sizeof(command));
  memcpy(&command, (uint8_t *)&IANA_ID, ianalen);
  command.gpio_action = BIC_GPIO_GET_OUTPUT_STATUS;
  command.gpio_number = BIC_PWR_STATUS_GPIO_NUM;
  
  ret = bic_ipmb_wrapper(NETFN_OEM_1S_REQ, BIC_CMD_OEM_GET_SET_GPIO, (uint8_t *)&command, sizeof(command), rbuf, &rlen);
  if ((ret == 0) && (sizeof(result) == rlen)) {
    memset(&result, 0, sizeof(result));
    memcpy(&result, rbuf, rlen);
    *power_status = result.gpio_status & 1;    
  }
  
  return ret;
}
