/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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

#include <syslog.h>
#include "sensorlist.hpp"

#ifndef SENSOR_CONF
#define SENSOR_CONF nullptr
#endif
SensorList sensors(SENSOR_CONF);

extern "C" int sensors_read(const char *chip, const char *label, float *value)
{
  int ret = -1;
  if (!chip || !label || !value) {
    errno = EINVAL;
    return -1;
  }

  try {
    *value = sensors.at(chip)->at(label)->read();
    ret = 0;
  } catch (std::out_of_range &e) {
    syslog(LOG_ERR, "Read(%s:%s): Out of range exception: %s\n", chip, label, e.what());
  } catch (std::system_error &e) {
    syslog(LOG_ERR, "Read(%s:%s): System error: %s - %s\n", chip, label, e.code().message().c_str(), e.what());
  } catch (...) {
    syslog(LOG_CRIT, "Read(%s:%s) Unknown error", chip, label);
  }
  return ret;
}

extern "C" int sensors_write(const char *chip, const char *label, float value)
{
  int ret = -1;
  if (!chip || !label) {
    errno = EINVAL;
    return -1;
  }
  try {
    sensors.at(chip)->at(label)->write(value);
    ret = 0;
  } catch (std::out_of_range &e) {
    syslog(LOG_ERR, "Write(%s:%s): Out of range exception: %s\n", chip, label, e.what());
  } catch (std::system_error &e) {
    syslog(LOG_ERR, "Write(%s:%s): System error: %s - %s\n", chip, label, e.code().message().c_str(), e.what());
  } catch (...) {
    syslog(LOG_CRIT, "Write(%s:%s) Unknown error", chip, label);
  }
  return ret;
}

extern "C" int sensors_read_fan(const char *label, float *value)
{
  if (sensors.find("aspeed_pwm_tacho-isa-0000") != sensors.end()) {
    return sensors_read("aspeed_pwm_tacho-isa-0000", label, value);
  }
  return sensors_read("ast_pwm-isa-0000", label, value);
}

extern "C" int sensors_write_fan(const char *label, float value)
{
  if (sensors.find("aspeed_pwm_tacho-isa-0000") != sensors.end()) {
    return sensors_write("aspeed_pwm_tacho-isa-0000", label, value);
  }
  return sensors_write("ast_pwm-isa-0000", label, value);
}

extern "C" int sensors_read_adc(const char *label, float *value)
{
  if (sensors.find("iio_hwmon-isa-0000") != sensors.end()) {
    return sensors_read("iio_hwmon-isa-0000", label, value);
  }
  return sensors_read("ast_adc-isa-0000", label, value);
}

