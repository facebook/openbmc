/*
 * SensorAccessMechanism.cpp : Platfrom specific implementation
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

#include <openbmc/sensorsvcpal.h>
#include "Sensor.h"

namespace openbmc {
namespace qin {

bool SensorAccessMechanism::checkAccessConditions(Sensor* s) {
  if (accessCondition_ & ACCESS_COND_CHECK_FRUPOWERON) {
    if (s->getFru()->isFruOff()) {
      return false;
    }
  }
  else if (accessCondition_ & ACCESS_COND_CHECK_FRUPOWERON_5SEC){
    if (s->getFru()->getPoweronFlag() < 5) {
      if (s->getId() == MB_SENSOR_HSC_IN_POWER) {
        s->getFru()->incrementPoweronFlag();
      }
      return false;
    }
  }

  return true;
}

void SensorAccessMechanism::rawRead(Sensor* s, float *value){
  readResult_ = READING_NA;
  switch(s->getFru()->getId()){
    case FRU_MB:
    {
      switch (s->getId()) {
        case MB_SENSOR_HSC_IN_VOLT:
          readResult_ = (ReadResult)pal_read_sensor_reading_from_ME(
                                                      MB_SENSOR_HSC_IN_VOLT,
                                                      value);
          break;
        case MB_SENSOR_HSC_OUT_CURR:
          readResult_ = (ReadResult)pal_read_hsc_current_value(value);
          break;
        case MB_SENSOR_HSC_IN_POWER:
          readResult_ = (ReadResult)pal_read_sensor_reading_from_ME(
                                                      MB_SENSOR_HSC_IN_POWER,
                                                      value);
          break;

        //CPU, DIMM, PCH Temp
        case MB_SENSOR_CPU0_TEMP:
        case MB_SENSOR_CPU1_TEMP:
          readResult_ = (ReadResult)pal_read_cpu_temp(s->getId(), value);
          break;
        case MB_SENSOR_CPU0_DIMM_GRPA_TEMP:
        case MB_SENSOR_CPU0_DIMM_GRPB_TEMP:
        case MB_SENSOR_CPU1_DIMM_GRPC_TEMP:
        case MB_SENSOR_CPU1_DIMM_GRPD_TEMP:
          readResult_ = (ReadResult)pal_read_dimm_temp(s->getId(), value);
          break;
        case MB_SENSOR_CPU0_PKG_POWER:
        case MB_SENSOR_CPU1_PKG_POWER:
          readResult_ = (ReadResult)pal_read_cpu_package_power(s->getId(),
                                                               value);
          break;
        case MB_SENSOR_PCH_TEMP:
          readResult_ = (ReadResult)pal_read_sensor_reading_from_ME(
                                                      MB_SENSOR_PCH_TEMP,
                                                      value);
          break;
        //discrete
        case MB_SENSOR_POWER_FAIL:
          readResult_ = (ReadResult)pal_read_CPLD_power_fail_sts (
                                              FRU_MB,
                                              s->getId(),
                                              value,
                                              s->getFru()->getPoweronFlag());
          break;
        case MB_SENSOR_MEMORY_LOOP_FAIL:
          readResult_ = (ReadResult)pal_check_postcodes(FRU_MB,
                                                        s->getId(),
                                                        value);
          break;
        case MB_SENSOR_PROCESSOR_FAIL:
          readResult_ = (ReadResult)pal_check_frb3(FRU_MB, s->getId(), value);
          break;
      }
    }
  }
}

} // namespace qin
} // namespace openbmc
