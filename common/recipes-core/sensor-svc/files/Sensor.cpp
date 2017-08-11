/*
 * Sensor.cpp
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

#include "Sensor.h"
#include "SensorAccessMechanism.h"

namespace openbmc {
namespace qin {

Sensor::Sensor (const std::string &name,
                Object* parent,
                uint8_t id,
                const std::string &unit,
                std::unique_ptr<SensorAccessMechanism> sensorAccess)
             : Object(name, parent) {
  this->id_ = id;
  this->unit_ = unit;
  this->sensorAccess_ = std::move(sensorAccess);
}

Sensor::Sensor (const std::string &name,
                Object* parent,
                const std::string &unit,
                std::unique_ptr<SensorAccessMechanism> sensorAccess)
             : Object(name, parent) {
  this->unit_ = unit;
  this->sensorAccess_ = std::move(sensorAccess);
}

FRU* Sensor::getFru() {
  return dynamic_cast<FRU*>(this->getParent());
}

uint8_t Sensor::getId(){
  return id_;
}

float Sensor::getValue() {
  return value_;
}

std::string Sensor::getUnit() {
  return unit_;
}

ReadResult Sensor::getLastReadStatus() {
  return sensorAccess_->getLastReadResult();
}

ReadResult Sensor::sensorRawRead(){
  float val;
  ReadResult readResult = sensorAccess_->sensorRawRead(this, &val);
  if (readResult == READING_SUCCESS){
    value_ = val;
  }

  return readResult;
}

} // namespace qin
} // namespace openbmc
