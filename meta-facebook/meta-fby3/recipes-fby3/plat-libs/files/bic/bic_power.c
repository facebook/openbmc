/*
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
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
#include <openbmc/kv.h>
#include <facebook/fby3_common.h>
#include "bic_power.h"

//The value of power button to CPLD
#define POWER_BTN_HIGH 0x3
#define POWER_BTN_LOW  0x2

//The value of reset button to CPLD
#define RESET_BTN_HIGH 0x3
#define RESET_BTN_LOW  0x1

//The delay of the value in S5 state
#define DELAY_POWER_CYCLE 5
#define DELAY_POWER_OFF 6
#define DELAY_GRACEFUL_SHUTDOWN 1

//The type of power off
enum {
  NORMAL_POWER_OFF = 0x0,
  GRACEFUL_POWER_OFF
};

static int
bic_server_power_control(uint8_t slot_id, uint8_t val) {
  uint8_t tbuf[5] = {0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 5;
  uint8_t rlen = 0;
  int ret;

  tbuf[0] = 0x05; //bus id
  tbuf[1] = 0x42; //slave addr
  tbuf[2] = 0x01; //read 1 byte 
  tbuf[3] = 0x00; //register offset
  tbuf[4] = val;

  ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  return ret;
}

int
bic_server_power_on(uint8_t slot_id) {
  uint8_t pwr_seq[3] = {POWER_BTN_HIGH, POWER_BTN_LOW, POWER_BTN_HIGH};
  int sts_cnt = sizeof(pwr_seq);
  int ret;
  int i;
  char sys_conf[16] = {0};
  
  for (i = 0; i < sts_cnt; i++) {
    ret = bic_server_power_control(slot_id, pwr_seq[i]);
    if ( ret < 0 ) {
      printf("%s() Cannot set power state to %02X\n", __func__, pwr_seq[i]);
      return ret;
    }

    if ( POWER_BTN_LOW == pwr_seq[i] ) sleep(1);    
  }
  
  if ( kv_get("sled_system_conf", sys_conf, NULL, KV_FPERSIST) == 0 ) {
    if ( strcmp(sys_conf, "Type_DPB") == 0 ) {
      ret = fby3_common_fscd_ctrl(FAN_MANUAL_MODE);
    }
  }

  return ret;
}

static int
bic_server_power_off_with_param(uint8_t slot_id, uint8_t gs_flag) {
  uint8_t pwr_seq[3] = {POWER_BTN_HIGH, POWER_BTN_LOW, POWER_BTN_HIGH};
  int sts_cnt = sizeof(pwr_seq);
  int ret;
  int i;
  int delay = 0;

  for (i = 0; i < sts_cnt; i++) {
    ret = bic_server_power_control(slot_id, pwr_seq[i]);
    if ( ret < 0 ) {
      printf("%s() Cannot set power state to %02X\n", __func__, pwr_seq[i]);
      return ret;
    }
 
    if ( POWER_BTN_LOW == pwr_seq[i] ) {
      delay = (gs_flag == GRACEFUL_POWER_OFF)?DELAY_GRACEFUL_SHUTDOWN:DELAY_POWER_OFF;
      sleep(delay);
    } 
  }

  return ret;
}

int
bic_server_power_off(uint8_t slot_id) {
  return bic_server_power_off_with_param(slot_id, NORMAL_POWER_OFF);
}

int
bic_server_graceful_power_off(uint8_t slot_id) {
  return bic_server_power_off_with_param(slot_id, GRACEFUL_POWER_OFF);
}

int
bic_server_power_reset(uint8_t slot_id) {
  uint8_t pwr_seq[3] = {RESET_BTN_HIGH, RESET_BTN_LOW, RESET_BTN_HIGH};
  int sts_cnt = sizeof(pwr_seq);
  int ret;
  int i;

  for (i = 0; i < sts_cnt; i++) {
    ret = bic_server_power_control(slot_id, pwr_seq[i]);
    if ( ret < 0 ) {
      printf("%s() Cannot set power state to %02X\n", __func__, pwr_seq[i]);
      return ret;
    }

    if ( RESET_BTN_LOW == pwr_seq[i] ) sleep(1);
  }

  return ret;
}

int
bic_server_power_cycle(uint8_t slot_id) {
  struct pwr_instance{
    uint8_t pwr_sts;
    int (*change_power)(uint8_t);
  } pwr_seq[] = {
    { POWER_BTN_LOW , bic_server_power_off},
    { POWER_BTN_HIGH, bic_server_power_on },
  };

  int sts_cnt = (sizeof(pwr_seq)/sizeof(struct pwr_instance));
  int ret;
  int i;
  int retry = 10;
  uint8_t status;

  for (i = 0; i < sts_cnt; i++) {
    ret = pwr_seq[i].change_power(slot_id);
    if ( ret < 0 ) {
      printf("%s() Cannot set power state to %02X\n", __func__, pwr_seq[i].pwr_sts);
      return ret;
    }

    if ( POWER_BTN_LOW == pwr_seq[i].pwr_sts ) {
      sleep(DELAY_POWER_CYCLE);
      do {
        if (bic_get_server_power_status(slot_id, &status) < 0) {
            return -1;
        }
        if (status != 0) {
          ret = pwr_seq[i].change_power(slot_id);
          if ( ret < 0 ) {
            printf("%s() Cannot set power state to %02X\n", __func__, pwr_seq[i].pwr_sts);
            return ret;
          }
          sleep(1);
        } else {
          break;
        }
        retry--;
        if (retry == 0) break;
      } while (status != 0);
    }
  }

  return ret; 
}

int
bic_get_server_power_status(uint8_t slot_id, uint8_t *power_status)
{
  uint8_t tbuf[3]  = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 3;
  uint8_t rlen = 0;
  int ret;

  memcpy(tbuf, (uint8_t *)&IANA_ID, tlen);

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_GET_GPIO, tbuf, tlen, rbuf, &rlen);

  *power_status = (rbuf[8] & 0x40) >> 6;

  return ret;
}

//Only for class 2
int
bic_do_12V_cycle(uint8_t slot_id) {
  uint8_t tbuf[3] = {0x9c, 0x9c, 0x00};
  uint8_t rbuf[1] = {0};
  uint8_t tlen = 3;
  uint8_t rlen = 0;

  return bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, BIC_CMD_OEM_SET_12V_CYCLE, tbuf, tlen, rbuf, &rlen, BB_BIC_INTF);
}
