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

#include <errno.h>
#include <string>
#include <memory>
#include <fstream>
#include <system_error>
#include <stdexcept>
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include <object-tree/Object.h>
#include "SensorObjectTree.h"
#include "SensorObject.h"
#include "SensorDevice.h"
#include "SensorTemp.h"
#include "SensorPower.h"
#include "SensorPwm.h"
#include "SensorFan.h"
#include "SensorCurrent.h"
#include "SensorVoltage.h"
#include "SensorAttribute.h"
#include "SensorSysfsApi.h"
#include "SensorJsonParser.h"

namespace openbmc {
namespace qin {

std::unordered_map<std::string, SensorJsonParser::ObjectParser>
    SensorJsonParser::objectParserMap =
    {
      {"Generic",       SensorJsonParser::parseGenericObject},
      {"SensorDevice",  SensorJsonParser::parseSensorDevice},
      {"SensorObject",  SensorJsonParser::parseSensorObject},
      {"SensorTemp",    SensorJsonParser::parseSensorTemp},
      {"SensorPower",   SensorJsonParser::parseSensorPower},
      {"SensorPwm",     SensorJsonParser::parseSensorPwm},
      {"SensorFan",     SensorJsonParser::parseSensorFan},
      {"SensorCurrent", SensorJsonParser::parseSensorCurrent},
      {"SensorVoltage", SensorJsonParser::parseSensorVoltage}
    };

void SensorJsonParser::parse(const std::string &jsonFile,
                             SensorObjectTree  &sensorTree,
                             const std::string &parentPath) {
  LOG(INFO) << "Parsing the json file \"" << jsonFile << "\" and build "
    "the sensor object tree at path \"" << parentPath << "\"";

  std::ifstream ifs(jsonFile);
  if (!ifs.good()) {
    LOG(ERROR) << "Json file \"" << jsonFile << "\" cannot be opened";
    throw std::system_error(errno, std::system_category(), strerror(errno));
  }
  if (!sensorTree.containObject(parentPath)) {
    LOG(ERROR) << "Parent object at \"" << parentPath << "\" does not exist";
    throw std::invalid_argument("Path not found");
  }
  nlohmann::json jObject(ifs);
  parseObject(jObject, sensorTree, parentPath);
}

void SensorJsonParser::parseObject(const nlohmann::json &jObject,
                                   SensorObjectTree     &sensorTree,
                                   const std::string    &parentPath) {
  const std::string &type = jObject.at("objectType");
  LOG(INFO) << "Parsing the object with type \"" << type
    << "\" under the parent path \"" << parentPath << "\"";

  auto it = objectParserMap.find(type);
  if (it == objectParserMap.end()) {
    LOG(ERROR) << "Wrong object type \""<< type << "\" specified";
    throw std::invalid_argument("Wrong object type");
  }
  // call the corresponding parser to the object type
  it->second(jObject, sensorTree, parentPath);

  if (jObject.find("childObjects") != jObject.end()) {
    const std::string newPath =
        sensorTree.getIpc()->getPath(parentPath, jObject.at("objectName"));
    LOG(INFO) << "Adding childObjects under the parent path " << newPath;
    for (auto& childObject : jObject.at("childObjects")) {
      parseObject(childObject, sensorTree, newPath);
    }
  }
}

void SensorJsonParser::parseGenericObject(const nlohmann::json &jObject,
                                          SensorObjectTree     &sensorTree,
                                          const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the generic Object \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";

  Object* object = sensorTree.addObject(name, parentPath);
  if (object == nullptr) {
    throwObjectJsonConfliction(name, parentPath);
  }
  if (jObject.find("attributes") != jObject.end()) {
    parseGenericAttribute(jObject.at("attributes"), *object);
  }
}

void SensorJsonParser::parseSensorDevice(const nlohmann::json &jObject,
                                         SensorObjectTree     &sensorTree,
                                         const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  const nlohmann::json &access = jObject.at("access");
  const std::string &api = access.at("api");
  LOG(INFO) << "Parsing the SensorDevice \""<< name << "\" with access api \""
    << api << "\" under the parent path \"" << parentPath << "\"";

  Object* object = nullptr;
  if (api.compare("sysfs") == 0) {
    const std::string &fsPath = access.at("path");
    std::unique_ptr<SensorApi> uSensorApi(new SensorSysfsApi(fsPath));
    object = sensorTree.addSensorDevice(name,
                                        parentPath,
                                        std::move(uSensorApi));
  } else if (api.compare("i2c") == 0) {
    // TODO: add sensor object with i2c sensorApi here
    LOG(WARNING) << "The i2c SensorApi has not yet been implemented.";
    throw std::runtime_error("i2c access api not supported yet");
  } else {
    LOG(ERROR) << "Specified sensor api " << access.at("api")
      << " not exist. Should not happen if the JSON file validates schema.";
    throw std::invalid_argument("Invalid sensor api");
  }

  if (object == nullptr) {
    throwObjectJsonConfliction(name, parentPath);
  }
  if (jObject.find("attributes") != jObject.end()) {
    parseSensorAttribute(jObject.at("attributes"), *object);
  }
}

void SensorJsonParser::parseSensorObject(const nlohmann::json &jObject,
                                         SensorObjectTree     &sensorTree,
                                         const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the SensorObject \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";
  parseSensorTypeObject(std::unique_ptr<Object>(new SensorObject(name)),
                        jObject,
                        sensorTree,
                        parentPath);
}

void SensorJsonParser::parseSensorTemp(const nlohmann::json &jObject,
                                       SensorObjectTree     &sensorTree,
                                       const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the SensorTemp \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";
  parseSensorTypeObject(std::unique_ptr<Object>(new SensorTemp(name)),
                        jObject,
                        sensorTree,
                        parentPath);
}

void SensorJsonParser::parseSensorPower(const nlohmann::json &jObject,
                                        SensorObjectTree     &sensorTree,
                                        const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the SensorPower \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";
  parseSensorTypeObject(std::unique_ptr<Object>(new SensorPower(name)),
                        jObject,
                        sensorTree,
                        parentPath);
}

void SensorJsonParser::parseSensorPwm(const nlohmann::json &jObject,
                                      SensorObjectTree     &sensorTree,
                                      const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the SensorPwm \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";
  parseSensorTypeObject(std::unique_ptr<Object>(new SensorPwm(name)),
                        jObject,
                        sensorTree,
                        parentPath);
}

void SensorJsonParser::parseSensorFan(const nlohmann::json &jObject,
                                      SensorObjectTree     &sensorTree,
                                      const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the SensorFan \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";
  parseSensorTypeObject(std::unique_ptr<Object>(new SensorFan(name)),
                        jObject,
                        sensorTree,
                        parentPath);
}

void SensorJsonParser::parseSensorCurrent(const nlohmann::json &jObject,
                                          SensorObjectTree     &sensorTree,
                                          const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the SensorCurrent \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";
  parseSensorTypeObject(std::unique_ptr<Object>(new SensorCurrent(name)),
                        jObject,
                        sensorTree,
                        parentPath);
}

void SensorJsonParser::parseSensorVoltage(const nlohmann::json &jObject,
                                          SensorObjectTree     &sensorTree,
                                          const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the SensorVoltage \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";
  parseSensorTypeObject(std::unique_ptr<Object>(new SensorVoltage(name)),
                        jObject,
                        sensorTree,
                        parentPath);
}

void SensorJsonParser::parseGenericAttribute(const nlohmann::json &attributes,
                                             Object               &object) {
  LOG(INFO) << "Adding attributes in object \"" << object.getName() << "\"";

  for (auto &attribute: attributes) {
    // if attribute type is not specified, put type as Generic
    const std::string &name = attribute.at("name");
    Attribute* attr = object.addAttribute(name);
    LOG(INFO) << "Adding generic Attribute with name \"" << name;

    if (attr == nullptr) {
      throwAttrJsonConfliction(name, object.getName());
    }
    if (attribute.find("modes") != attribute.end()) {
      const std::string &modes = attribute.at("modes");
      setAttrModes(*attr, modes);
    }
    if (attribute.find("value") != attribute.end()) {
      const std::string &value = attribute.at("value");
      writeAttrValue(*attr, object, value);
    }
  }
}

void SensorJsonParser::parseSensorAttribute(const nlohmann::json &attributes,
                                            Object               &object) {
  LOG(INFO) << "Adding sensor attributes in object \"" << object.getName()
    << "\"";

  for (auto &attribute: attributes) {
    // if attribute type is not specified, put type as Generic
    const std::string &name = attribute.at("name");
    Attribute* attr = object.addAttribute(name);
    LOG(INFO) << "Adding sensor Attribute with name \"" << name
      << "\" in object \"" << object.getName() << "\"";

    if (attr == nullptr) {
      throwAttrJsonConfliction(name, object.getName());
    }
    if (attribute.find("modes") != attribute.end()) {
      const std::string &modes = attribute.at("modes");
      setAttrModes(*attr, modes);
    }
    // has to set addr before writing value for sensor attribute
    if (attribute.find("addr") != attribute.end()) {
      const std::string &addr = attribute.at("addr");
      LOG(INFO) << "Setting Attribute \"" << name << "\" with addr \""
        << addr << "\"";
      (static_cast<SensorAttribute*>(attr))->setAddr(addr);
    }
    if (attribute.find("value") != attribute.end()) {
      const std::string &value = attribute.at("value");
      writeAttrValue(*attr, object, value);
    }
  }
}

} // namespace openbmc
} // namespace qin
