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
#include <functional>
#include <nlohmann/json.hpp>
#include <object-tree/Object.h>
#include "SensorObjectTree.h"

namespace openbmc {
namespace qin {

/**
 * Json parser to parse the json file containing the information for
 * building the sensor object tree. It is based on nlohmann json.
 * The instace of SensorObjectTree should be passed in for the building
 * process.
 */
class SensorJsonParser {
  public:
    typedef std::function<void(const nlohmann::json&,
                               SensorObjectTree&,
                               const std::string&)> ObjectParser;

    // a mapping from the strings of object types defined in JSON schema
    // to the corresponding object parser
    static std::unordered_map<std::string, ObjectParser> objectParserMap;

    /**
     * Parse the json file and build up a branch of SensorObjectTree
     * into the specified tree under the specified parent object at the
     * parentPath. Notice that the SensorObject will be treated as the
     * leaf and that it should not have children. It is assumed that the
     * json file VALIDATES the sensor-info.schema.json.
     *
     * @param jsonFile specifies the information to build the sensorTree.
     *        It should validate the sensor-info.schema.json.
     * @param sensorTree is a reference to the SensorObjectTree to be
     *        constructed.
     * @param parentPath parent object path to start building sensor object
     *        tree from. Parent object cannot be a SensorObject.
     * @throw std::system_error errno if jsonFile cannot be opened.
     * @throw std::out_of_rage if some entries are missing in json file. This
     *        will not happen if json file is validated.
     * @throw std::invalid_argument if parentPath does not exist in sensorTree,
     *        if parent object is a SensorObject, or if the objects specified
     *        in the jsonFile conflict the existing structure in sensorTree.
     */
    static void parse(const std::string &jsonFile,
                      SensorObjectTree  &sensorTree,
                      const std::string &parentPath);

    /**
     * Parse the specified json object into the sensorTree under the object at
     * parentPath. The json object is assumed to contain the declaration of
     * ONE object to be added under parentPath and should validate the schema
     * sensor-info.schema.json.
     *
     * @param jObject json object to be build in the sensor tree
     * @param sensorTree to be built with the object in
     * @param parentPath of the parent object in the sensor tree to build
     *        the object from
     * @throw std::out_of_rage if some entries are missing in jObject. This
     *        will not happen if jObject is validated.
     * @throw std::invalid_argument if jObject has invalid object type and
     *        sensor api declared or if there has been already the object
     *        with the same name existing in the sensor tree
     * @throw std::runtime_error "not supported" if i2c is specified as the
     *        access method for SensorDevice
     */
    static void parseObject(const nlohmann::json &jObject,
                            SensorObjectTree     &sensorTree,
                            const std::string    &parentPath);

    /**
     * Parse the specified json array of generic attributes into the sensorTree
     * under the specified object. If the value entry is not empty, the value
     * will be written into the created Attribute instance if the modes is
     * writtable (RW or WO). Otherwise, the value will be set into the
     * attribute's cache. The attributes array should validate the
     * corresponding definition in the schema sensor-json.schema.json.
     *
     * @param attributes json array of groups of Attribute declaration
     * @param object in the sensor tree where the attributes should be
     *        added in
     * @throw std::out_of_rage if some entries are missing in jObject.
     *        This will not happen if attributes are validated.
     * @throw std::invalid_argument if there has already been an
     *        attribute with the same name exisiting or if the specified
     *        modes do not exist
     */
    static void parseGenericAttribute(const nlohmann::json &attributes,
                                      Object               &object);

    /**
     * Similar to the function parseGenericAttribute. This function parses
     * the sensor attributes into the specified object.
     *
     * @param attributes json array of groups of Attribute declaration
     * @param object in the sensor tree where the attributes should be
     *        added in
     * @throw std::out_of_rage if some entries are missing in jObject.
     *        This will not happen if attributes are validated.
     * @throw std::invalid_argument if there has already been an
     *        attribute with the same name exisiting or if the specified
     *        modes do not exist
     */
    static void parseSensorAttribute(const nlohmann::json &attributes,
                                     Object               &object);

  private:

    static void throwObjectJsonConfliction(const std::string &name,
                                           const std::string &parentPath) {
      // there must be duplicate object
      LOG(ERROR) << "Object \"" << name << "\" cannot be added "
        "under parent object at \"" << parentPath << "\"";
      throw std::invalid_argument("JSON conflicts with the sensor tree");
    }

    static void throwAttrJsonConfliction(const std::string &attrName,
                                         const std::string &objectName) {
      // there must be duplicate attribute
      LOG(ERROR) << "Attribute \"" << attrName
        << "\" cannot be added into Object \"" << objectName << "\"";
      throw std::invalid_argument("JSON conflicts with the sensor tree");
    }

    /**
     * A helper function that adds the specified generic object into the
     * sensor tree and that proceeds to add the attributes under it.
     *
     * @param nlohmann::json format of the object to be added
     * @param sensorTree to add the object in
     * @param parent object path to add the object at
     */
    static void parseGenericObject(const nlohmann::json &jObject,
                                   SensorObjectTree     &sensorTree,
                                   const std::string    &parentPath);

    /**
     * A helper function that adds the specified sensor device into the
     * sensor tree and that proceeds to add the attributes under it. Note
     * that the SensorApi will be created and passed into the sensor device.
     *
     * @param nlohmann::json format of the sensor device to be added
     * @param sensorTree to add the sensor device in
     * @param parent object path to add the sensor device at
     */
    static void parseSensorDevice(const nlohmann::json &jObject,
                                  SensorObjectTree     &sensorTree,
                                  const std::string    &parentPath);

    /**
     * A helper function that adds the specified sensor object into the
     * sensor tree and that proceeds to add the attributes under it.
     *
     * @param nlohmann::json format of the sensor object to be added
     * @param sensorTree to add the sensor object in
     * @param parent object path to add the sensor object at
     */
    static void parseSensorObject(const nlohmann::json &jObject,
                                  SensorObjectTree     &sensorTree,
                                  const std::string    &parentPath);

    // function similar to parseSensorObject but adds sensor temp
    static void parseSensorTemp(const nlohmann::json &jObject,
                                SensorObjectTree     &sensorTree,
                                const std::string    &parentPath);

    // function similar to parseSensorObject but adds sensor power
    static void parseSensorPower(const nlohmann::json &jObject,
                                 SensorObjectTree     &sensorTree,
                                 const std::string    &parentPath);

    // function similar to parseSensorObject but adds sensor pwm
    static void parseSensorPwm(const nlohmann::json &jObject,
                               SensorObjectTree     &sensorTree,
                               const std::string    &parentPath);

    // function similar to parseSensorObject but adds sensor fan
    static void parseSensorFan(const nlohmann::json &jObject,
                               SensorObjectTree     &sensorTree,
                               const std::string    &parentPath);

    // function similar to parseSensorObject but adds sensor current
    static void parseSensorCurrent(const nlohmann::json &jObject,
                                   SensorObjectTree     &sensorTree,
                                   const std::string    &parentPath);

    // function similar to parseSensorObject but adds sensor voltage
    static void parseSensorVoltage(const nlohmann::json &jObject,
                                   SensorObjectTree     &sensorTree,
                                   const std::string    &parentPath);

    /**
     * A helper function that adds the specified sensor object into the
     * sensor tree and that proceeds to add the attributes under it.
     *
     * @param unique pointer to the object to be added
     * @param nlohmann::json format of the object to be added
     * @param sensorTree to add the object in
     * @param parent object path to add the object at
     */
    static void parseSensorTypeObject(std::unique_ptr<Object> upObj,
                                      const nlohmann::json    &jObject,
                                      SensorObjectTree        &sensorTree,
                                      const std::string       &parentPath) {
      const std::string name = upObj.get()->getName();
      Object* object = sensorTree.addObject(std::move(upObj), parentPath);
      if (object == nullptr) {
        throwObjectJsonConfliction(name, parentPath);
      }
      if (jObject.find("attributes") != jObject.end()) {
        parseSensorAttribute(jObject.at("attributes"), *object);
      }
    }

    /**
     * Set the Attribute modes to the specified modes in string.
     *
     * @param attribute to be set modes with
     * @param modes in string expression
     * @throw std::invalid_argument if the modes in string does not match
     *        any of the modes in
     */
    static void setAttrModes(Attribute &attr, const std::string &modes) {
      LOG(INFO) << "Setting modes \"" << modes << "\" Attribute \""
        << attr.getName() << "\"";
      auto it = Attribute::stringModesMap.find(modes);
      if (it == Attribute::stringModesMap.end()) {
        // will not happen if json file validates schema
        LOG(ERROR) << "Specified modes \"" << modes << "\" not exist";
        throw std::invalid_argument("Modes not exist");
      }
      attr.setModes(static_cast<Attribute::Modes>(it->second));
    }

    /**
     * Write the attribute value if it is writable or set the value at
     * attribute's cache if it is not.
     *
     * @param attribute to be written or set value with
     * @param object that contains the attribute
     * @param value to be written or set
     */
    static void writeAttrValue(Attribute         &attr,
                               Object            &object,
                               const std::string &value) {
      if (attr.isWritable()) {
        LOG(INFO) << "Writing value \"" << value << "\" to Attribute \""
          << attr.getName() << "\"";
        object.writeAttrValue(attr.getName(), value);
      } else {
        LOG(INFO) << "Setting value \"" << value << "\" to Attribute \""
          << attr.getName() << "\" cache";
        attr.setValue(value);
      }
    }
};
} // namespace openbmc
} // namespace qin
