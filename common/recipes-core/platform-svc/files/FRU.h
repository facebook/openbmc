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
#include "HotPlugDetectionMechanism.h"

namespace openbmc {
namespace qin {

class FRU : public Object {
  private:
    bool hotPlugSupport_{false};  // flag whether FRU supports hotplug
    bool isAvailable_{true};      // flag whether FRU is available
    std::string fruJson_;         // string representation of
                                  // FRU json (nlohmann) object
    std::unique_ptr<HotPlugDetectionMechanism> hotPlugDetectionMechanism_;
                                  //hotplug detection mechanism,
                                  //if not set then supports external detection

  public:
    /*
     * Contructor for fru which doesn't support hotplug
     */
    FRU(const std::string &name, Object* parent, const std::string &fruJson)
      : Object(name, parent) , fruJson_(fruJson) {}

    /*
     * Contructor for fru which supports hotplug
     */
    FRU(const std::string &name,
        Object* parent,
        const std::string &fruJson,
        std::unique_ptr<HotPlugDetectionMechanism> hotPlugDetectionMechanism)
      : FRU(name, parent, fruJson) {
      hotPlugSupport_ = true;
      isAvailable_ = false;

      hotPlugDetectionMechanism_ = std::move(hotPlugDetectionMechanism);
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
     * Returns whether FRU supports internal hotplug detection mechanism
     */
    bool isIntHPDetectionSupported() const{
      if (hotPlugDetectionMechanism_) {
        //If hotPlugDetectionMechanism_ is set
        //then supports internal hotplug detection
        return true;
      }
      else {
        return false;
      }
    }

    /*
     * Set whether FRU is available or not
     * Can be set only for FRU which supports external hotplug detection
     * Returns status of operation
     */
    bool setAvailable(bool isAvailable) {
      if (isIntHPDetectionSupported() == false) {
        isAvailable_ = isAvailable;
        return true;
      }
      else {
        return false;
      }
    }

    /*
     * Returns whether FRU supports hotplug or not
     */
    bool isHotPlugSupported() const{
      return hotPlugSupport_;
    }

    /*
     * Detect if fru is available or not and return availability status
     */
    bool detectAvailability() {
      //Detect only if Internal Hotplug mechanism is supported
      if (isIntHPDetectionSupported()) {
        isAvailable_ = hotPlugDetectionMechanism_->detectAvailability();
      }

      return isAvailable_;
    }
};

} // namespace qin
} // namespace openbmc
