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
#include <stdio.h> 
#include <dirent.h>
#include <regex>
#include <vector>
#include "sensorchip.hpp"

// TODO We are currently in C++14 mode. When distribution is upgraded
// to a compiler which supports C++17, switch to using std::filesystem
// instead of the dirent C style API

using namespace std;


unique_ptr<Sensor> SensorChip::make_sensor(const sensors_chip_name *chip, const sensors_feature *feature, const sensors_subfeature *subfeature)
{
  return unique_ptr<Sensor>(new Sensor(chip, feature, subfeature));
}

void SensorChip::addsensor(std::unique_ptr<Sensor> snr)
{
  snr->initialize();
  string label = snr->get_label();
  if (this->find(label) != this->end()) {
    throw system_error(EEXIST, std::generic_category(), "Label already exists");
  }
  (*this)[label] = move(snr);
}

void SensorChip::enumerate()
{
  const std::map<sensors_feature_type, std::vector<sensors_subfeature_type>>
  interested_subfeatures = {
    {SENSORS_FEATURE_IN, {SENSORS_SUBFEATURE_IN_INPUT,
                          SENSORS_SUBFEATURE_IN_MAX}},
    {SENSORS_FEATURE_FAN, {SENSORS_SUBFEATURE_FAN_INPUT}},
    {SENSORS_FEATURE_TEMP, {SENSORS_SUBFEATURE_TEMP_INPUT,
                            SENSORS_SUBFEATURE_TEMP_MAX}},
    {SENSORS_FEATURE_POWER, {SENSORS_SUBFEATURE_POWER_INPUT,
			     SENSORS_SUBFEATURE_POWER_INPUT_HIGHEST}},
    {SENSORS_FEATURE_ENERGY, {SENSORS_SUBFEATURE_ENERGY_INPUT}},
    {SENSORS_FEATURE_CURR, {SENSORS_SUBFEATURE_CURR_INPUT,
                            SENSORS_SUBFEATURE_CURR_MAX}},
    {SENSORS_FEATURE_HUMIDITY, {SENSORS_SUBFEATURE_HUMIDITY_INPUT}}
  };

  for (int nf = 0; ;) {
    const sensors_feature *feature = sensors_get_features(chip, &nf);
    if (feature == NULL) {
      break;
    }
    if (interested_subfeatures.find(feature->type) == interested_subfeatures.end()) {
      continue;
    }

    for (auto subtype : interested_subfeatures.at(feature->type)) {
      const sensors_subfeature *subfeature = sensors_get_subfeature(chip, feature, subtype);
      if (subfeature != NULL) {
        unique_ptr<Sensor> snr = make_sensor(chip, feature, subfeature);
        addsensor(move(snr));
      }
    }
  }
}

unique_ptr<Sensor> FanSensorChip::make_sensor(const sensors_chip_name *chip, const std::string &name)
{
  return unique_ptr<PWMSensor>(new PWMSensor(chip, name));
}

void FanSensorChip::enumerate()
{
  SensorChip::enumerate();
  string chippath(chip->path);
  std::regex pwm_regex(R"(^pwm\d+$)");
  struct dirent *ent;
  DIR *dir = opendir(chippath.c_str());
  if (!dir) {
    throw system_error(ENOENT, std::generic_category(), "sensor path not present");
  }
  while ((ent = readdir(dir)) != NULL) {
    string name(ent->d_name);
    if (regex_match(name, pwm_regex)) {
      unique_ptr<Sensor> snr = make_sensor(chip, name);
      addsensor(move(snr));
    }
  }
  closedir(dir);
}

unique_ptr<Sensor> LegacyFanSensorChip::make_sensor(const sensors_chip_name *chip, const std::string &name)
{
  return unique_ptr<LegacyPWMSensor>(new LegacyPWMSensor(chip, name));
}

void LegacyFanSensorChip::enumerate()
{
  SensorChip::enumerate();
  string chippath(chip->path);
  std::regex pwm_regex(R"(^(pwm\d+)_en$)");
  struct dirent *ent;
  DIR *dir = opendir(chippath.c_str());
  if (!dir) {
    throw system_error(ENOENT, std::generic_category(), "sensor path not present");
  }
  while ((ent = readdir(dir)) != NULL) {
    smatch sm;
    string name(ent->d_name);
    if (regex_match(name, sm, pwm_regex)) {
      name = sm.str(1);
      unique_ptr<Sensor> snr = make_sensor(chip, name);
      addsensor(move(snr));
    }
  }
  closedir(dir);
}
