/*
 *
 * Copyright 2016-present Facebook. All Rights Reserved.
 *
 * This file represents platform specific implementation for storing
 * SDR record entries and acts as back-end for IPMI stack
 *
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
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <openbmc/pal.h>
#include <openbmc/ipmi.h>

#define RISER_BUS_ID 0x1

static bool BBV_cycle_ongoing = false;

void *
bbv_power_cycle_handler(void *arg) {
  int rlen = 0;
  int ret = 0;
  int retry = 0;
  int delay_time = (int)arg;
  uint8_t status;
  pthread_detach(pthread_self());


  if(!pal_is_BBV_prsnt())
    goto end;

//power off TP
  pal_set_server_power(FRU_MB,SERVER_POWER_OFF);
  sleep(5);
  ret = pal_get_server_power(FRU_MB, &status);
  if (ret)
    goto end;
  if (status != SERVER_POWER_OFF)
    goto end;

//Power off BBV
  while ( retry < 3 ) {
    rlen = ipmb_send(
      RISER_BUS_ID,
      0x2c,
      NETFN_CHASSIS_REQ << 2,
      0x02,
      0x00);

    if ( rlen < 0 )
    {
      ret = -1;
    }
    else
    {
      if (ipmb_rxb()->cc == 0)
        ret = 0;
      else {
        syslog(LOG_DEBUG, "%s(%d): fail com_code:%x \n", __func__, __LINE__,ipmb_rxb()->cc);
        ret = -1;
      }
    }
    if ( (retry < 3) && (ret == -1) )
      retry++;
    else
      break;
  }
//check BBV is power off
  retry = 0;
  ret = 0;
  while ( retry < 12 ) { // retry 1 minute
    rlen = ipmb_send(
      RISER_BUS_ID,
      0x2c,
      NETFN_CHASSIS_REQ << 2,
      0x01);

    if ( rlen < 0 )
    {
        ret = -1;
    }
    else
    {
      if (ipmb_rxb()->cc == 0) {
        if((ipmb_rxb()->data[0] & 0x01) == 0x00)
          ret = 0;
        else
          ret = -1;
      } else {
        syslog(LOG_DEBUG, "%s(%d): fail com_code:%x \n", __func__, __LINE__,ipmb_rxb()->cc);
        ret = -1;
      }
    }
    sleep(5);
    if ( (retry < 12) && (ret == -1) )
      retry++;
    else
      break;
  }
//Delay
  sleep(delay_time);
//Power on BBV
  retry = 0;
  ret = 0;
  while ( retry < 3 ) {
    rlen = ipmb_send(
      RISER_BUS_ID,
      0x2c,
      NETFN_CHASSIS_REQ << 2,
      0x02,
      0x01);

    if ( rlen < 0 )
    {
      ret = -1;
    }
    else
    {
      if (ipmb_rxb()->cc == 0)
        ret = 0;
      else {
        syslog(LOG_DEBUG, "%s(%d): fail com_code:%x \n", __func__, __LINE__,ipmb_rxb()->cc);
        ret = -1;
      }
    }
    if ( (retry < 3) && (ret == -1) )
      retry++;
    else
      break;
  }
//check BBV is power off
  retry = 0;
  ret = 0;
  while ( retry < 12 ) { // retry 1 minute
    rlen = ipmb_send(
      RISER_BUS_ID,
      0x2c,
      NETFN_CHASSIS_REQ << 2,
      0x01);

    if ( rlen < 0 )
    {
      ret = -1;
    }
    else
    {
      if (ipmb_rxb()->cc == 0) {
        if((ipmb_rxb()->data[0] & 0x01) == 0x01)
          ret = 0;
        else
          ret = -1;
      } else {
        syslog(LOG_DEBUG, "%s(%d): fail com_code:%x \n", __func__, __LINE__,ipmb_rxb()->cc);
        ret = -1;
      }
    }
    sleep(5);
    if ( (retry < 12) && (ret == -1) )
      retry++;
    else
      break;
  }

//power on TP
  pal_set_server_power(FRU_MB,SERVER_POWER_ON);

end:
  BBV_cycle_ongoing = false;
  pthread_exit(0);
}

int
bbv_power_cycle(int delay_time) {
  pthread_t tid_bbv_power_cycle;

  if(!BBV_cycle_ongoing && pal_is_BBV_prsnt()) {
    BBV_cycle_ongoing = true;
    if (pthread_create(&tid_bbv_power_cycle, NULL, bbv_power_cycle_handler, (void*)delay_time) < 0) {
        syslog(LOG_WARNING, "ipmid: bbv_power_cycle_handler pthread_create failed\n");
    }
    return CC_SUCCESS;
  } else {
    return CC_NOT_SUPP_IN_CURR_STATE;
  }

}
