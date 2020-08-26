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
#ifndef _SENSOR_HPP_
#define _SENSOR_HPP_
#include <string>
#include <fstream>
#include <system_error>
#include <sensors/sensors.h>

// ofstream which throws by default
class OutFile : public std::ofstream {
  public:
    OutFile() : std::ofstream() {
      exceptions(~std::ofstream::goodbit);
    }
};

// ifstream which throws by default
class InFile : public std::ifstream {
  public:
    InFile() : std::ifstream() {
      exceptions(~std::ifstream::goodbit);
    }
};

// Sensor capable of reading/writing sensor values.
// Not all sensors might support writing.
class Sensor {
  protected:
  const sensors_chip_name *chip;
  const sensors_feature *feature;
  const sensors_subfeature *subfeature;
  std::string name;
  std::string label;
  public:
    Sensor(const sensors_chip_name *_c, const sensors_feature *_feature, const sensors_subfeature *_subfeature)
      : chip(_c), feature(_feature), subfeature(_subfeature), name(), label("") {}
    virtual ~Sensor() {}

    // Initialize a sensor.
    virtual void initialize();

    // Get the label
    std::string get_label() {return label;}

    // Reads the current value of the sensor
    virtual float read();

    // Writes a value to the sensor
    virtual void write(float val);
};

// Sensor capable of reading/writing PWM from fanchips on
// 4.18 kernels and above
class PWMSensor : public Sensor {
  protected:
  std::string path;
  public:
    PWMSensor(const sensors_chip_name *fanchip, const std::string &_name)
      : Sensor(fanchip, nullptr, nullptr), path() {name = _name; label = _name;}
    virtual ~PWMSensor() {}

    // Initialize a sensor.
    virtual void initialize();
    
    // Read the current PWM
    virtual float read();

    // Write a PWM value
    virtual void write(float val);
};

// Sensor capable of reading/writing PWM from fanchips on
// 4.1 kernels and below
class LegacyPWMSensor : public Sensor {
  std::string en_path;
  std::string rising_path;
  std::string falling_path;
  std::string type_path;
  std::string unit_path;
  int unit_max();
  public:
    LegacyPWMSensor(const sensors_chip_name *fanchip, const std::string &_name)
      : Sensor(fanchip, nullptr, nullptr), en_path(),
      rising_path(), falling_path(), type_path(), unit_path() {name = _name;}
    virtual ~LegacyPWMSensor() {}

    // Initialize a sensor.
    virtual void initialize();

    // Read the current PWM
    virtual float read();

    // Write a PWM value
    virtual void write(float val);
};

#endif
