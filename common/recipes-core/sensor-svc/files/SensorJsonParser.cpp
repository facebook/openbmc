/*
 * SensorJsonParser.cpp
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

#include <string>
#include <fstream>
#include <glog/logging.h>
#include <nlohmann/json.hpp>
#include "SensorAccessMechanism.h"
#include "SensorAccessViaPath.h"
#include "SensorAccessAVA.h"
#include "SensorAccessNVME.h"
#include "SensorAccessINA230.h"
#include "SensorAccessVR.h"
#include "SensorJsonParser.h"

namespace openbmc {
namespace qin {

std::unordered_map<std::string, SensorJsonParser::ObjectParser>
    SensorJsonParser::objectParserMap =
    {
      {"SensorService", SensorJsonParser::parseSensorService},
      {"FRU",  SensorJsonParser::parseFRU},
      {"Sensor",  SensorJsonParser::parseSensor},
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

void SensorJsonParser::parseSensorService(const nlohmann::json &jObject,
                                          SensorObjectTree     &sensorTree,
                                          const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the Sensor Service \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";

  Object* object = sensorTree.addSensorService(name, parentPath);

  if (object == nullptr) {
    throwObjectJsonConfliction(name, parentPath);
  }
}

void SensorJsonParser::parseFRU(const nlohmann::json &jObject,
                                SensorObjectTree     &sensorTree,
                                const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the FRU \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";

  std::string id;

  try {
    id = jObject.at("id");
  }
  catch (const std::out_of_range& oor) {
    //Ignore if id is not mentioned
  }

  Object* object = sensorTree.addFRU(name, parentPath, id);

  if (object == nullptr) {
    throwObjectJsonConfliction(name, parentPath);
  }
}

void SensorJsonParser::parseSensor(const nlohmann::json &jObject,
                                   SensorObjectTree     &sensorTree,
                                   const std::string    &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the Sensor \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";

  std::string id;
  try {
    id = jObject.at("id");
  }
  catch (const std::out_of_range& oor) {
    //Ignore if id is not mentioned
  }
  const std::string &unit = jObject.at("unit");

  Object* object = nullptr;
  std::unique_ptr<SensorAccessMechanism> upSensorAccess;

  if (jObject.find("access") == jObject.end()) {
    LOG(WARNING) << "Access Mechanism for " << name << " not defined";
    throw std::runtime_error("Access Mechanism for " + name + " not defined");
  }
  else {
    // Decode access mechanism
    const nlohmann::json &access = jObject.at("access");
    const std::string &type = access.at("type");

    if (type.compare("path") == 0) {
      const std::string &path = access.at("path");
      try {
        //Set unitDiv if set in json file
        const std::string &unitDiv = access.at("unitDiv");
        upSensorAccess = std::unique_ptr<SensorAccessMechanism>
                           (new SensorAccessViaPath(path, std::stof(unitDiv)));
      }
      catch (const std::out_of_range& oor) {
        //if unitDiv is not mentioned
        upSensorAccess = std::unique_ptr<SensorAccessMechanism>
                           (new SensorAccessViaPath(path));
      }
    } else if (type.compare("AVA") == 0) {
      upSensorAccess =  std::unique_ptr<SensorAccessMechanism>
                           (new SensorAccessAVA());
    } else if (type.compare("INA230") == 0) {
      upSensorAccess = std::unique_ptr<SensorAccessMechanism>
                           (new SensorAccessINA230());
    } else if (type.compare("NVME") == 0) {
      upSensorAccess = std::unique_ptr<SensorAccessMechanism>
                           (new SensorAccessNVME());
    } else if (type.compare("VR") == 0) {
      const std::string &busId = access.at("busId");
      const std::string &loop = access.at("loop");
      const std::string &reg = access.at("reg");
      const std::string &slaveAddr = access.at("slaveAddr");
      upSensorAccess =  std::unique_ptr<SensorAccessMechanism>
                           (new SensorAccessVR(
                                       std::stoi(busId, nullptr, 0),
                                       std::stoi(loop, nullptr, 0),
                                       std::stoi(reg, nullptr, 0),
                                       std::stoi(slaveAddr, nullptr, 0)));
    } else if (type.compare("NONE") == 0) {
      upSensorAccess = std::unique_ptr<SensorAccessMechanism>
                           (new SensorAccessMechanism());
    } else {
      LOG(ERROR) << "Specified sensor access mechanism " << type;
      throw std::invalid_argument("Invalid sensor api");
    }

    object = sensorTree.addSensor(name,
                                  parentPath,
                                  id,
                                  unit,
                                  std::move(upSensorAccess));
  }

  if (object == nullptr) {
    throwObjectJsonConfliction(name, parentPath);
  }
}

} // namespace qin
} // namespace openbmc
