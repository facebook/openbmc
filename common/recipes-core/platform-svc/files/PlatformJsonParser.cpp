/*
 * PlatformJsonParser.cpp
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
#include "PlatformObjectTree.h"
#include "PlatformJsonParser.h"
#include <gio/gio.h>

std::unordered_map<std::string, PlatformJsonParser::ObjectParser>
    PlatformJsonParser::objectParserMap =
    {
      {"PlatformService", PlatformJsonParser::parsePlatformService},
      {"FRU",  PlatformJsonParser::parseFRU},
      {"Sensor",  PlatformJsonParser::parseSensor},
    };

void PlatformJsonParser::parse(const std::string   &jsonFile,
                               PlatformObjectTree  &platformTree,
                               const std::string   &parentPath) {
  LOG(INFO) << "Parsing the json file \"" << jsonFile << "\" and build "
    "the sensor object tree at path \"" << parentPath << "\"";

  std::ifstream ifs(jsonFile);
  if (!ifs.good()) {
    LOG(ERROR) << "Json file \"" << jsonFile << "\" cannot be opened";
    throw std::system_error(errno, std::system_category(), strerror(errno));
  }
  if (!platformTree.containObject(parentPath)) {
    LOG(ERROR) << "Parent object at \"" << parentPath << "\" does not exist";
    throw std::invalid_argument("Path not found");
  }
  nlohmann::json jObject(ifs);
  parseObject(jObject, platformTree, parentPath);
}

void PlatformJsonParser::parseObject(const nlohmann::json   &jObject,
                                     PlatformObjectTree     &platformTree,
                                     const std::string      &parentPath) {
  const std::string &type = jObject.at("objectType");
  LOG(INFO) << "Parsing the object with type \"" << type
    << "\" under the parent path \"" << parentPath << "\"";

  auto it = objectParserMap.find(type);
  if (it == objectParserMap.end()) {
    LOG(ERROR) << "Wrong object type \""<< type << "\" specified";
    throw std::invalid_argument("Wrong object type");
  }
  // call the corresponding parser to the object type
  it->second(jObject, platformTree, parentPath);

  if (jObject.find("childObjects") != jObject.end()) {
    const std::string newPath =
        platformTree.getIpc()->getPath(parentPath, jObject.at("objectName"));
    LOG(INFO) << "Adding childObjects under the parent path " << newPath;
    for (auto& childObject : jObject.at("childObjects")) {
      parseObject(childObject, platformTree, newPath);
    }
  }
}

void PlatformJsonParser::parsePlatformService(const nlohmann::json   &jObject,
                                              PlatformObjectTree     &platformTree,
                                              const std::string      &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the Sensor Service \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";

  const std::string &sensorServiceDBusName = jObject.at("sensorServiceDBusName");
  const std::string &sensorServiceDBusPath = jObject.at("sensorServiceDBusPath");
  const std::string &sensorServiceDBusInterface = jObject.at("sensorServiceDBusInterface");

  platformTree.initSensorService(sensorServiceDBusName, sensorServiceDBusPath, sensorServiceDBusInterface);
  Object* object = platformTree.addPlatformService(name, parentPath);

  if (object == nullptr) {
    throwObjectJsonConfliction(name, parentPath);
  }
  else {
    //Set Base path for platform Service
    platformTree.setPlatformServiceBasePath(parentPath + "/" + name);
  }
}

void PlatformJsonParser::parseFRU(const nlohmann::json   &jObject,
                                  PlatformObjectTree     &platformTree,
                                  const std::string      &parentPath) {

  const std::string &name = jObject.at("objectName");
  const std::string &hotplugSupport = jObject.at("hotPlugSupport");

  LOG(INFO) << "Parsing the FRU \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";

  // remove childObjects from fruJson and store it in FRU
  nlohmann::json fruJson = jObject;
  fruJson.erase("childObjects");

  FRU* fru = nullptr;

  if (hotplugSupport.compare("1") == 0) {
    fru = platformTree.addFRU(name, parentPath, true, false, fruJson.dump());
  }
  else {
    fru = platformTree.addFRU(name, parentPath, false, true, fruJson.dump());
  }

  if (fru == nullptr) {
    throwObjectJsonConfliction(name, parentPath);
  }
}

void PlatformJsonParser::parseSensor(const nlohmann::json &jObject,
                                     PlatformObjectTree     &platformTree,
                                     const std::string      &parentPath) {
  const std::string &name = jObject.at("objectName");
  LOG(INFO) << "Parsing the Sensor \"" << name <<
    "\" under the parent path \"" << parentPath << "\"";

  Sensor* object = platformTree.addSensor(name, parentPath, jObject.dump());

  if (object == nullptr) {
    throwObjectJsonConfliction(name, parentPath);
  }
}
