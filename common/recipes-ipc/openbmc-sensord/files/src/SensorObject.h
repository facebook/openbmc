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
#include <unordered_map>
#include <glog/logging.h>
#include <nlohmann/json.hpp>
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
      LOG(INFO) << "Initializing SensorObject \"" << name << "\"";
      sensorApi_ = std::move(sensorApi);
    }

    SensorAttribute* getAttribute(const std::string &name) const override {
      Attribute* attr = Object::getAttribute(name);
      if (attr == nullptr) {
        return nullptr;
      }
      return static_cast<SensorAttribute*>(attr);
    }

    SensorApi* getSensorApi() const {
      return sensorApi_.get();
    }

    /**
     * Adds sensor attribute to the sensor object.
     * Overrides the addAttribute function in Object.h.
     *
     * @param name of attribute
     * @return nullptr if name conflict; Attribute pointer otherwise
     */
    SensorAttribute* addAttribute(const std::string &name) override;

    /**
     * Read the value of Attribute name with type through sensorApi_.
     *
     * @param name of the attribute to be read
     * @return value of the attribute
     * @throw std::invalid_argument if name not found
     * @throw std::system_error EPERM if attr has no read modes
     */
    const std::string& readAttrValue(const std::string &name) const override;

    /**
     * Write the value of Attribute name with type through sensorApi_.
     *
     * @param value to be written to attribute through sensorApi_
     * @param name of attribute to be written
     * @throw std::invalid_argument if name not found
     * @throw std::system_error EPERM if attr has no write modes
     */
    void writeAttrValue(const std::string &value,
                        const std::string &name) override;

    /**
     * Dump the sensor object info into json format. It calls the
     * Object::dumpToJson() but adds the access method and changes
     * the objectType to "SensorApi".
     *
     * @return nlohmann::json object with entries specified in
     *         Object::dumpToJson() plus the "access" entry for SensorApi.
     */
    nlohmann::json dumpToJson() const override {
      LOG(INFO) << "Dumping SensorObject into json";
      nlohmann::json dump = Object::dumpToJson();
      dump["objectType"] = "Sensor";
      dump["access"] = sensorApi_.get()->dumpToJson();
      return dump;
    }

    /**
     * Dump the sensor object info iteratively into json format. It calls the
     * Object::dumpToJsonIterative() but adds the access method and changes
     * the objectType to "SensorApi".
     *
     * @return nlohmann::json object with entries specified in
     *         Object::dumpToJson() plus the "access" entry for SensorApi.
     */
    nlohmann::json dumpToJsonRecursive() const override {
      LOG(INFO) << "Dumping SensorObject into json";
      nlohmann::json dump = Object::dumpToJsonRecursive();
      dump["objectType"] = "Sensor";
      dump["access"] = sensorApi_.get()->dumpToJson();
      return dump;
    }
};

} // namespace ipc
} // namespace openbmc
