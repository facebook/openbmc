/*
 * SensorObjectTree.cpp
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
#include <glog/logging.h>
#include "DBusSensorInterface.h"
#include "DBusSensorServiceInterface.h"
#include "DBusSensorTreeInterface.h"
#include "SensorObjectTree.h"

namespace openbmc {
namespace qin {

static DBusSensorTreeInterface sensorTreeInterface;
static DBusSensorInterface sensorInterface;
static DBusSensorServiceInterface sensorServiceInterface;

Object* SensorObjectTree::addObject(std::unique_ptr<Object> upObj,
                                    const std::string       &parentPath) {
  bool isSensorObject = false;
  try {
    getSensor(upObj.get());
    isSensorObject = true;
    LOG(INFO) << "Object \"" << upObj.get()->getName()
      << "\" is of SensorObject type";
  } catch (...) {
    LOG(INFO) << "Object \"" << upObj.get()->getName()
      << "\" is NOT of SensorObject type.";
  }

  if (isSensorObject) {
    LOG(INFO) << "Checking if parent is of FRU type";
    try {
      getFRU(parentPath); // throw if parent is not FRU
    } catch (...) {
      // rethrow with the suitable message
      throw std::invalid_argument("Invalid parent type");
    }
  }
  return ObjectTree::addObject(std::move(upObj), parentPath);
}

SensorService* SensorObjectTree::addSensorService(
                                             const std::string &name,
                                             const std::string &parentPath) {
  LOG(INFO) << "Adding SensorService \"" << name << "\" under the path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);
  std::unique_ptr<SensorService> upDev(new SensorService(name, parent));
  return static_cast<SensorService*>
            (addObjectByPath(std::move(upDev), path, sensorServiceInterface));
}

FRU* SensorObjectTree::addFRU(const std::string &name,
                              const std::string &parentPath,
                              const std::string &id) {
  LOG(INFO) << "Adding FRU \"" << name << "\" under the path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);

  if (id.empty()){
    std::unique_ptr<FRU> upDev(new FRU(name, parent));
    return static_cast<FRU*>
            (addObjectByPath(std::move(upDev), path, sensorTreeInterface));
  }
  else {
    std::unique_ptr<FRU>
            upDev(new FRU(name, parent, std::stoi (id, nullptr, 0)));
    return static_cast<FRU*>
            (addObjectByPath(std::move(upDev), path, sensorTreeInterface));
  }
}

Sensor* SensorObjectTree::addSensor(const std::string &name,
                                    const std::string &parentPath,
                                    const std::string &id,
                                    const std::string &unit,
                                    std::unique_ptr<SensorAccessMechanism>
                                           upSensorAccess) {
  LOG(INFO) << "Adding new Sensor \"" << name
            << "\" under the path \"" << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  FRU* parent = getFRU(getParent(parentPath, name));

  if (id.empty()){
    std::unique_ptr<Sensor> upObj
            (new Sensor(name, parent, unit, std::move(upSensorAccess)));
    return static_cast<Sensor*>
            (addObjectByPath(std::move(upObj), path, sensorInterface));
  }
  else {
    std::unique_ptr<Sensor> upObj
            (new Sensor(name,
                        parent,
                        std::stoi (id, nullptr, 0),
                        unit,
                        std::move(upSensorAccess)));
    return static_cast<Sensor*>
            (addObjectByPath(std::move(upObj), path, sensorInterface));
  }
}

} // namespace qin
} // namespace openbmc
