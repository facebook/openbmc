/*
 * Sensor.h
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

#pragma once
#include <string>
#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
#include <object-tree/Object.h>
#include "SensorAccessMechanism.h"
#include "FRU.h"

namespace openbmc {
namespace qin {

class Sensor : public Object{
  private:
    uint8_t id_ = 0xFF;                           // Sensor Id
    float value_;                                 // Last Read Sensor Value
    std::string unit_;                            // Unit of Sensor
    std::unique_ptr<SensorAccessMechanism> sensorAccess_;
                                                  // sensorAccess mechanism

  public:
    /*
     * Constructor
     */
    Sensor (const std::string &name,
            Object* parent,
            uint8_t id,
            const std::string &unit,
            std::unique_ptr<SensorAccessMechanism> sensorAccess);
    /*
     * Constructor without ID
     */
    Sensor (const std::string &name,
            Object* parent,
            const std::string &unit,
            std::unique_ptr<SensorAccessMechanism> sensorAccess);

    /*
     * Returns parent FRU
     */
    FRU* getFru();

    /*
     * Return Sensor Id
     */
    uint8_t getId();

    /*
     * Returns Sensor Value
     */
    float getValue();

    /*
     * Returns Sensor value
     */
    std::string getUnit();

    /*
     * Returns last raw read status
     */
    ReadResult getLastReadStatus();

    /*
     * sensorRaw
     */
    ReadResult sensorRawRead();
};

} // namespace qin
} // namespace openbmc
