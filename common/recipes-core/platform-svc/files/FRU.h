/*
 * FRU.h
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
#include <object-tree/Object.h>
#include <vector>
#include <nlohmann/json.hpp>
using namespace openbmc::qin;

class FRU : public Object {
  private:
    bool hotPlugSupport_;   // flag whether FRU supports hotplug
    bool isAvailable_;      // flag whether FRU is available
    std::string fruJson_;   // string representation of FRU json (nlohmann) object,

  public:
    /*
     * Contructor
     */
    FRU(const std::string &name, Object* parent, bool hotPlugSupport, bool isAvailable, const std::string &fruJson)
      : Object(name, parent) {
      this->hotPlugSupport_ = hotPlugSupport;
      this->isAvailable_ = isAvailable;
      this->fruJson_ = fruJson;
    }

    /*
     * Returns string representation of FRU Json object.
     */
    std::string const & getFruJson() const{
      return fruJson_;
    }

    /*
     * Returns whether FRU is available or not
     */
    bool isAvailable() const{
      return isAvailable_;
    }

    /*
     * Set whether FRU is available or not
     */
    void setAvailable(bool isAvailable) {
      isAvailable_ = isAvailable;
    }

    /*
     * Returns whether FRU supports hotplug or not
     */
    bool isHotPlugSupported() const{
      return hotPlugSupport_;
    }
};
