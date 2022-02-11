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
namespace qin {

/**
 * SensorDevice is a derived class of Object in object-tree. It is a factory
 * for the SensorObject. In addition, it contains the functions for reading
 * and writing the attribute values from sysfs or i2c through SensorApi. The
 * SensorApi should be passed by ownership to it.
 */
class SensorDevice : public Object {
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
    SensorDevice(const std::string          &name,
                 std::unique_ptr<SensorApi> sensorApi,
                 Object*                    parent = nullptr)
        : Object(name, parent) {
      LOG(INFO) << "Initializing SensorObject \"" << name << "\"";
      sensorApi_ = std::move(sensorApi);
    }

    SensorAttribute* getAttribute(const std::string &name) const override {
      return static_cast<SensorAttribute*>(Object::getAttribute(name));
    }

    SensorApi* getSensorApi() const {
      return sensorApi_.get();
    }

    /**
     * Adds sensor attribute to the sensor object.
     * Overrides the addAttribute function in Object.h.
     *
     * @param name of attribute
     * @throw std::invalid_argument if name conflicts
     * @return nullptr if name conflict; Attribute pointer otherwise
     */
    SensorAttribute* addAttribute(const std::string &name) override;

    /**
     * Read the value of specified SensorAttribute through sensorApi_.
     * It is assumed that the attr can be accessed through sensorApi_.
     *
     * @param the object to be associated with the reading
     * @param the attribute to be read
     * @return value of the attribute
     * @throw std::system_error EPERM if attr has no read modes
     */
    const std::string& readAttrValue(const Object    &object,
                                     SensorAttribute &attr) const;

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
     * Write the value of specified SensorAttribute through sensorApi_.
     * It is assumed that the attr can be accessed through sensorApi_.
     *
     * @param the object to be associated with the writing
     * @param attribute to be written
     * @param value to be written to attribute through sensorApi_
     * @throw std::system_error EPERM if attr has no write modes
     */
    void writeAttrValue(const Object      &object,
                        SensorAttribute   &attr,
                        const std::string &value);

    /**
     * Write the value of Attribute with specified name through sensorApi_.
     *
     * @param name of attribute to be written
     * @param value to be written to attribute through sensorApi_
     * @throw std::invalid_argument if name not found
     * @throw std::system_error EPERM if attr has no write modes
     */
    void writeAttrValue(const std::string &name,
                        const std::string &value) override;

    /**
     * Dump the sensor object info into json format. It calls the
     * Object::dumpToJson() but adds the access method and changes
     * the objectType to "SensorApi".
     *
     * @return nlohmann::json object with entries specified in
     *         Object::dumpToJson() plus the "access" entry for SensorApi.
     */
    nlohmann::json dumpToJson() const override {
      LOG(INFO) << "Dumping SensorDevice into json";
      nlohmann::json dump = Object::dumpToJson();
      addDumpInfo(dump);
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
      LOG(INFO) << "Dumping SensorDevice recursively into json";
      nlohmann::json dump = Object::dumpToJsonRecursive();
      addDumpInfo(dump);
      return dump;
    }

  protected:

    /**
     * A helper function to add the object type and access entries to dump.
     *
     * @param dump to be added with type and access entries
     */
    void addDumpInfo(nlohmann::json &dump) const {
      dump["objectType"] = "SensorDevice";
      dump["access"] = sensorApi_.get()->dumpToJson();
    }
};

} // namespace openbmc
} // namespace qin
