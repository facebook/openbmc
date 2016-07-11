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
#include <stdexcept>
#include <memory>
#include <glog/logging.h>
#include <unordered_map>
#include <object-tree/Object.h>
#include <object-tree/Attribute.h>
#include "SensorApi.h"
#include "SensorAttribute.h"

namespace openbmc {
namespace ipc {

/**
 * SensorObject is a derived class of Object in object-tree.
 * It is a factory for the SensorObject. In addition,
 * it contains the functions for reading and writing the
 * attribute values from sysfs or i2c through SensorApi.
 */
class SensorObject : public Object {
  private:
    std::unique_ptr<SensorApi> sensorApi_;

  public:
    /**
     * Constructor with default parent as nullptr if not specified
     *
     * @param name of the object
     * @param sensorApi provides api to read (write) attribute values.
     *        It should be transferred by ownership here.
     * @param parent of the object; not allowed to be nullptr
     */
    SensorObject(const std::string          &name,
                 std::unique_ptr<SensorApi> sensorApi,
                 Object*                    parent = nullptr)
        : Object(name, parent) {
      LOG(INFO) << "Initializing SensorObject name " << name;
      sensorApi_ = std::move(sensorApi);
    }

    SensorAttribute* getSensorAttribute(const std::string &name,
                                        const std::string &type
                                          = "Generic") const {
      Attribute* attr = getAttribute(name, type);
      if (attr == nullptr) {
        return nullptr;
      }
      return static_cast<SensorAttribute*>(attr);
    }

    SensorApi* getSensorApi() const {
      return sensorApi_.get();
    }

    /**
     * Adds sensor attribute to the sensor object but with type specified.
     * Overrides the addAttribute function in Object.h.
     *
     * @param name of attribute
     * @param type of attribute
     * @return nullptr if name conflict; Attribute pointer otherwise
     */
    SensorAttribute* addAttribute(const std::string &name,
                                  const std::string &type) override;

    /**
     * Read the value of Attribute name with type through sensorApi_.
     *
     * @param name of the attribute to be read
     * @param type of the attribute to be read; "Generic" by default
     * @return value of the attribute
     * @throw std::invalid_argument if name not found
     * @throw std::system_error EPERM if attr has no read modes
     */
    const std::string& readAttrValue(const std::string &name,
                                     const std::string &type
                                       = "Generic") const override;

    /**
     * Write the value of Attribute name with type through sensorApi_.
     *
     * @param value to be written to attribute through sensorApi_
     * @param name of attribute to be written
     * @param type of attribute to be written; "Generic" by default
     * @throw std::invalid_argument if name not found
     * @throw std::system_error EPERM if attr has no write modes
     */
    void writeAttrValue(const std::string &value,
                        const std::string &name,
                        const std::string &type = "Generic") override;
};

} // namespace ipc
} // namespace openbmc
