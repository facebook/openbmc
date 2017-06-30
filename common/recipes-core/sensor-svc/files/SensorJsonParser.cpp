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
#include "SensorJsonParser.h"
#include "SensorAccessMechanism.h"
#include "SensorAccessViaPath.h"
#include "SensorAccessAVA.h"
#include "SensorAccessNVME.h"
#include "SensorAccessINA230.h"
#include "SensorAccessVR.h"

std::unordered_map<std::string, SensorJsonParser::ObjectParser>
    SensorJsonParser::objectParserMap =
    {
      {"SensorService", SensorJsonParser::parseSensorService},
      {"FRU",  SensorJsonParser::parseFRU},
      {"Sensor",  SensorJsonParser::parseSensor},
    };

static int parseInt(std::string str) {
  if ((str.length() > 2) && (str.at(0) == '0' && str.at(1) == 'x')){
    return std::stoi (str,nullptr,16);
  }
  else {
    return std::stoi (str);
  }
}

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
      std::unique_ptr<SensorAccessMechanism> upSensorAccess(new SensorAccessViaPath(path));
      try {
        //Set unitDiv if set in json file
        const std::string &unitDiv = access.at("unitDiv");
        ((SensorAccessViaPath*)upSensorAccess.get())->setUnitDiv(std::stof(unitDiv));
      }
      catch (const std::out_of_range& oor) {
        //Ignore if unitDiv is not mentioned
      }
      object = sensorTree.addSensor(name, parentPath, id, unit, std::move(upSensorAccess));
    } else if (type.compare("AVA") == 0) {
      std::unique_ptr<SensorAccessMechanism> upSensorAccess(new SensorAccessAVA());
      object = sensorTree.addSensor(name, parentPath, id, unit, std::move(upSensorAccess));
    } else if (type.compare("INA230") == 0) {
      std::unique_ptr<SensorAccessMechanism> upSensorAccess(new SensorAccessINA230());
      object = sensorTree.addSensor(name, parentPath, id, unit, std::move(upSensorAccess));
    } else if (type.compare("NVME") == 0) {
      std::unique_ptr<SensorAccessMechanism> upSensorAccess(new SensorAccessNVME());
      object = sensorTree.addSensor(name, parentPath, id, unit, std::move(upSensorAccess));
    } else if (type.compare("VR") == 0) {
      const std::string &busId = access.at("busId");
      const std::string &loop = access.at("loop");
      const std::string &reg = access.at("reg");
      const std::string &slaveAddr = access.at("slaveAddr");
      std::unique_ptr<SensorAccessMechanism> upSensorAccess(new SensorAccessVR(
                                                                        parseInt(busId),
                                                                        parseInt(loop),
                                                                        parseInt(reg),
                                                                        parseInt(slaveAddr)));
      object = sensorTree.addSensor(name, parentPath, id, unit, std::move(upSensorAccess));
    } else if (type.compare("NONE") == 0) {
      std::unique_ptr<SensorAccessMechanism> upSensorAccess(new SensorAccessMechanism());
      object = sensorTree.addSensor(name, parentPath, id, unit, std::move(upSensorAccess));
    } else {
      LOG(ERROR) << "Specified sensor access mechanism " << type;
      throw std::invalid_argument("Invalid sensor api");
    }
  }

  if (object == nullptr) {
    throwObjectJsonConfliction(name, parentPath);
  }
}
