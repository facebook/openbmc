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
#include <unordered_map>
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include "SensorDevice.h"
#include "SensorObject.h"

namespace openbmc {
namespace qin {

/**
 * SensorCurrent is a derived class of SensorObject. It is a factory
 * for the SensorAttribute. It should be always the child of a SensorDevice.
 * It contains the functions for reading and writing the attribute values
 * from sysfs or i2c through SensorApi which is provided by the parent
 * SensorDevice instance.
 */
class SensorCurrent : public SensorObject {
  public:
    using SensorObject::SensorObject; // inherit constructor

  private:
    /**
     * A helper function to add the object type entry to dump.
     *
     * @param dump to be added with type entry
     */
    void addDumpInfo(nlohmann::json &dump) const override {
      dump["objectType"] = "SensorCurrent";
    }
};

} // namespace openbmc
} // namespace qin
