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
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <object-tree/Object.h>
#include <object-tree/Attribute.h>
#include "SensorDevice.h"
#include "SensorAttribute.h"

namespace openbmc {
namespace qin {

/**
 * SensorObject is a derived class of Object in object-tree. It is a factory
 * for the SensorAttribute. It should be always the child of a SensorDevice.
 * It contains the functions for reading and writing the attribute values
 * from sysfs or i2c through SensorApi which is provided by the parent
 * SensorDevice instance.
 */
class SensorObject : public Object {
  public:
    using Object::Object; // inherit constructor

    SensorAttribute* getAttribute(const std::string &name) const override {
      return static_cast<SensorAttribute*>(Object::getAttribute(name));
    }

    /**
     * Adds sensor attribute to the sensor object.
     *
     * @param name of attribute
     * @throw std::invalid_argument if name conflicts
     * @return Attribute pointer
     */
    SensorAttribute* addAttribute(const std::string &name) override;

    /**
     * Read the value of Attribute name with type through sensorApi_.
     * Will call the readAttrValue function from SensorDevice.
     *
     * @param name of the attribute to be read
     * @return value of the attribute
     * @throw std::invalid_argument if name not found
     * @throw std::system_error EPERM if attr has no read modes
     */
    virtual const std::string& readAttrValue(const std::string &name)
        const override;

    /**
     * Write the value of Attribute name with type through sensorApi_.
     * Will call the writeAttrValue function from SensorDevice.
     *
     * @param name of attribute to be written
     * @param value to be written to attribute through sensorApi_
     * @throw std::invalid_argument if name not found
     * @throw std::system_error EPERM if attr has no write modes
     */
    virtual void writeAttrValue(const std::string &name,
                                const std::string &value) override;

    /**
     * Dump the sensor object info into json format. It calls the
     * Object::dumpToJson() but adds the access method and changes
     * the objectType to "SensorApi".
     *
     * @return nlohmann::json object with entries specified in
     *         Object::dumpToJson() plus the "access" entry for SensorApi.
     */
    virtual nlohmann::json dumpToJson() const override {
      LOG(INFO) << "Dumping SensorObject into json";
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
    virtual nlohmann::json dumpToJsonRecursive() const override {
      LOG(INFO) << "Dumping SensorObject recursively into json";
      nlohmann::json dump = Object::dumpToJsonRecursive();
      addDumpInfo(dump);
      return dump;
    }

  protected:

    /**
     * A helper function to add the object type entry to dump.
     *
     * @param dump to be added with type enty
     */
    virtual void addDumpInfo(nlohmann::json &dump) const {
      dump["objectType"] = "SensorObject";
    }
};

} // namespace qin
} // namespace openbmc
