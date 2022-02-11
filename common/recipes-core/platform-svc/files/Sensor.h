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
#include "FRU.h"

namespace openbmc {
namespace qin {

class Sensor : public Object{
  private:
    std::string sensorJson_;   // string representation of Sensor json (nlohmann) object,

  public:
    /*
     * Constructor
     */
    Sensor (const std::string &name, Object* parent, const std::string &sensorJson) : Object(name, parent){
      this->sensorJson_ = sensorJson;
    }

    /*
     * Returns string representation of Sensor Json object.
     */
    std::string const & getSensorJson() const {
      return sensorJson_;
    }
};
} // namespace qin
} // namespace openbmc
