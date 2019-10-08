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
#ifndef _SENSORCHIP_HPP_
#define _SENSORCHIP_HPP_
#include <memory>
#include <map>
#include <string>
#include "sensor.hpp"

// Collection of sensors grouped in a single "chip". Provides efficient
// lookup of sensors within a chip.
class SensorChip : public std::map<std::string, std::unique_ptr<Sensor>> {
  protected:
  const sensors_chip_name *chip;
  std::string name;

    // Makes a sensor for the given chip.
    virtual std::unique_ptr<Sensor> make_sensor(const sensors_chip_name *chip,
        const sensors_feature *feature);

    // Adds the given sensor to the collection
    virtual void addsensor(std::unique_ptr<Sensor> snr);

  public:
    SensorChip(const sensors_chip_name *_chip, const std::string &n)
      : chip(_chip), name(n) {}
    virtual ~SensorChip() {}

    // Enumerate sensors in this chip
    virtual void enumerate();
};

// Collection of sensors in a Fan chip (Works for 4.18 and above kernels).
class FanSensorChip : public SensorChip {
  protected:
    virtual std::unique_ptr<Sensor> make_sensor(const sensors_chip_name *chip,
        const std::string &name);
  public:
  FanSensorChip(const sensors_chip_name *_chip, const std::string &n) : SensorChip(_chip, n) {}
  virtual ~FanSensorChip() {}
  virtual void enumerate();
};

// Collection of sensors in a legacy-fan chip (Works for 4.1 kernels)
class LegacyFanSensorChip : public SensorChip {
  protected:
    virtual std::unique_ptr<Sensor> make_sensor(const sensors_chip_name *chip,
        const std::string &name);
  public:
  LegacyFanSensorChip(const sensors_chip_name *_chip, const std::string &n) : SensorChip(_chip, n) {}
  virtual ~LegacyFanSensorChip() {}
  virtual void enumerate();
};

#endif
