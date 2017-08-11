/*
 * SensorAccessViaPath.cpp : Platfrom specific implementation
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

#include <openbmc/gpio.h>
#include "SensorAccessViaPath.h"
#include <cstdint>
#include <chrono>
#include <thread>
#include <openbmc/sensorsvcpal.h>
#include "Sensor.h"

namespace openbmc {
namespace qin {

bool SensorAccessViaPath::preRawRead(Sensor* s, float* value) {
  if (s->getId() == MB_SENSOR_P3V_BAT) {
    gpio_set(GPIO_BAT_SENSE_EN_N, GPIO_VALUE_HIGH);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return true;
}

void SensorAccessViaPath::postRawRead(Sensor* s, float* value) {
  switch (s->getId()) {
    case MB_SENSOR_INLET_REMOTE_TEMP:
      pal_apply_inlet_correction(value);
      break;

    case MB_SENSOR_P3V_BAT:
      gpio_set(GPIO_BAT_SENSE_EN_N, GPIO_VALUE_HIGH);
      break;

    case MEZZ_SENSOR_TEMP:
      if (*value > NIC_MAX_TEMP) {
        readResult_ = READING_NA;
        *value = 0;
      }
      break;
  }
}

} // namespace qin
} // namespace openbmc
