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
#include <map>
#include <cmath>
#include <syslog.h>
#include "sensor.hpp"

using namespace std;

void Sensor::initialize()
{
  const std::map<sensors_feature_type, sensors_subfeature_type> interested_subfeatures = {
    {SENSORS_FEATURE_IN, SENSORS_SUBFEATURE_IN_INPUT},
    {SENSORS_FEATURE_FAN, SENSORS_SUBFEATURE_FAN_INPUT},
    {SENSORS_FEATURE_TEMP, SENSORS_SUBFEATURE_TEMP_INPUT},
    {SENSORS_FEATURE_POWER, SENSORS_SUBFEATURE_POWER_INPUT}, // SENSORS_SUBFEATURE_POWER_AVERAGE
    {SENSORS_FEATURE_ENERGY, SENSORS_SUBFEATURE_ENERGY_INPUT},
    {SENSORS_FEATURE_CURR, SENSORS_SUBFEATURE_CURR_INPUT},
    {SENSORS_FEATURE_HUMIDITY, SENSORS_SUBFEATURE_HUMIDITY_INPUT}
  };

  if (!feature) {
  }

  if (feature == nullptr || chip == nullptr) {
    throw system_error(EINVAL, std::generic_category(), "Initialize without chip|featire");
  }

  if (interested_subfeatures.find(feature->type) == interested_subfeatures.end()) {
    return;
  }
  name.assign(feature->name);
  const char *clabel = sensors_get_label(chip, feature);
  if (clabel)
    label.assign(clabel);
  else
    label.assign(name);
  for (int nsf = 0; ;) {
    const sensors_subfeature *sub = sensors_get_all_subfeatures(chip, feature, &nsf);
    if (!sub) {
      break;
    }
    if (sub->type != interested_subfeatures.at(feature->type)) {
      continue;
    }
    if (subfeature) {
			syslog(LOG_ERR, "%s:%s Duplicate subfeature %s found while %s exists\n",
				chip->path, feature->name, subfeature->name, sub->name);
    } else {
      subfeature = sub;
    }
  }
	if (subfeature == nullptr) {
    throw system_error(ENOENT, std::generic_category(), "Sensor Read Failure");
	}
}


float Sensor::read()
{
  double value;
  if (subfeature == nullptr) {
    throw system_error(ENOTSUP, std::generic_category(), "Sensor feature not supported");
  }
  if (sensors_get_value(chip, subfeature->number, &value)) {
    throw system_error(EIO, std::generic_category(), "Sensor Read Failure");
  }
  return float(value);
}


void Sensor::write(float value)
{
  if (subfeature == nullptr) {
    throw system_error(ENOTSUP, std::generic_category(), "Sensor feature not supported");
  }
  if (sensors_set_value(chip, subfeature->number, (double)value)) {
    throw system_error(EIO, std::generic_category(), "Sensor Write Failure");
  }
}

void PWMSensor::initialize()
{
  path = string(chip->path) + "/" + name;
}

float PWMSensor::read()
{
  InFile file;
  file.open(path);
  int val;
  file >> val;
  file.close();
  return ceil(float(val) * 100.0 / 255.0);
}

void PWMSensor::write(float value)
{
  OutFile file;
  int val = int(value * 255.0 / 100.0);
  file.open(path);
  file << val << endl;
  file.close();
}

int LegacyPWMSensor::unit_max()
{
  InFile file;
  file.open(unit_path);
  int unit;
  file >> unit;
  file.close();
  return unit + 1;
}

void LegacyPWMSensor::initialize()
{
  // convert pwm0 to pwm1
  int index = stoi(name.substr(3)) + 1;
  label = "pwm" + to_string(index);

  string base(chip->path);
  en_path = base + "/" + name + "_en";
  type_path = base + "/" + name + "_type";
  falling_path = base + "/" + name + "_falling";
  rising_path = base + "/" + name + "_rising";
  unit_path = base + "/pwm_type_m_unit";
}

float LegacyPWMSensor::read()
{
  InFile file;
  file.open(en_path);
  int en;
  file >> en;
  file.close();
  if (!en) {
    throw system_error(EIO, std::generic_category(), "PWM Not enabled");
  }
  int val;
  file.open(falling_path);
  file >> std::hex >> val;
  file.close();
  if (val == 0)
    return 100.0;
  int max = unit_max();
  return float((100 * val + (max - 1)) / (max));
}

void LegacyPWMSensor::write(float val)
{
  OutFile file;
  int max = unit_max();
  int value = (int(val) * max) / 100;
  if (value == 0) {
    file.open(en_path);
    file << 0 << endl;
    file.close();
  } else if (value == max) {
    value = 0;
  }

  file.open(type_path);
  file << 0 << endl;
  file.close();

  file.open(rising_path);
  file << 0 << endl;
  file.close();

  file.open(falling_path);
  file << value << endl;
  file.close();

  file.open(en_path);
  file << 1 << endl;
  file.close();
}
