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
//#include <stdio.h>
//#include <iostream>
#include <syslog.h>
#include <string.h>
#include <mutex>
#include "sensorlist.hpp"

using namespace std;

SensorList::SensorList(const char *conf_file)
{
  _sensor_list_build(conf_file);
}


SensorList::~SensorList()
{
  sensors_cleanup();
}

unique_ptr<SensorChip> SensorList::make_chip(const sensors_chip_name *chip, const string &name)
{
  if (name == "ast_pwm-isa-0000") {
    return unique_ptr<LegacyFanSensorChip>(new LegacyFanSensorChip(chip, name));
  }
  if (name == "aspeed_pwm_tacho-isa-0000" || name == "aspeed_pwm_tachometer-isa-0000" || name == "aspeed_tach-isa-0000"
      || strstr(name.c_str(), "pwmfan-isa-") || strstr(name.c_str(), "max31790-i2c-")) {
    return unique_ptr<FanSensorChip>(new FanSensorChip(chip, name));
  }
  return unique_ptr<SensorChip>(new SensorChip(chip, name));
}


void SensorList::enumerate()
{
  for (int nr = 0; ;) {
    char cname[128];
    const sensors_chip_name *chip = sensors_get_detected_chips(NULL, &nr);
    if (chip == NULL) {
      break;
    }
    sensors_snprintf_chip_name(cname, sizeof(cname), chip);
    string name(cname);
    if (this->find(name) != this->end()) {
      syslog(LOG_ERR, "Ignoring duplicated chip: %s\n", name.c_str());
      continue;
    }
    (*this)[name] = make_chip(chip, name);
    (*this)[name]->enumerate();
    for (auto& it : *((*this)[name])) {
      const string& label = it.first;
      if (labelToChip.find(label) == labelToChip.end()) {
        labelToChip[label] = name;
      } else {
        syslog(LOG_INFO, "Warning: duplicated label: %s", label.c_str());
      }
    }
  }
}

void SensorList::re_enumerate(const char *conf_file)
{
  std::unique_lock exclock(listSharedMutex);
  syslog(LOG_INFO, "sensor list renumerate start");

  sensors_cleanup();
  this->clear();
  labelToChip.clear();

  _sensor_list_build(conf_file);
  syslog(LOG_INFO, "sensor list renumerate end");
}

std::unique_ptr<SensorChip>& SensorList::find_chip_by_label(const std::string& label)
{
  return at(labelToChip.at(label));
}

void SensorList::_sensor_list_build(const char* conf_file)
{
  FILE *f = NULL;

  if (conf_file != NULL) {
    f = fopen(conf_file, "r");
  }
  sensors_init(f);
  if (f != NULL) {
    fclose(f);
  }
  try {
    enumerate();
  } catch (std::out_of_range &e) {
    syslog(LOG_ERR, "Initialization: Out of range exception: %s\n", e.what());
  } catch (std::system_error &e) {
    syslog(LOG_ERR, "Initialization: System error: %s - %s\n", e.code().message().c_str(), e.what());
  } catch (...) {
    syslog(LOG_CRIT, "Initialization: Unknown error");
  }
}
