/*
 * Copyright 2014-present Facebook. All Rights Reserved.
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
#include <nlohmann/json.hpp>
#include <object-tree/Object.h>
#include "SensorAttribute.h"

namespace openbmc {
namespace qin {

/**
 * Sensor API for reading and writing value of the attribute from the
 * SensorObject path. Dummy implementation here. Virtual functions
 * remain to be implemented by the derived classes.
 */
class SensorApi {
  public:
    /**
     * Reads value from the path specified by object and attr.
     * It's dummy here. The derived class should implement this function.
     *
     * @param object of Attribute to be read
     * @param attr of the value to be read
     * @return value read
     */
    virtual const std::string readValue(const Object          &object,
                                        const SensorAttribute &attr) const = 0;

    /**
     * Writes value to the path specified by object and attr.
     * It's dummy here. The derived class should implement this function.
     *
     * @param object of Attribute to be written
     * @param attr of the value to be written
     * @param value to be written
     */
    virtual void writeValue(const Object          &object,
                            const SensorAttribute &attr,
                            const std::string     &value) = 0;

    /**
     * Dump the sensor api info into json format.
     */
    virtual nlohmann::json dumpToJson() const = 0;
};
} // namespace qin
} // namespace openbmc
