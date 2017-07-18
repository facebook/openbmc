/*
 * PlatformObjectTree.cpp
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

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <memory>
#include <glog/logging.h>
#include "PlatformObjectTree.h"
#include "DBusPlatformSvcInterface.h"

static DBusPlatformSvcInterface platformSvcInterface;

PlatformService* PlatformObjectTree::addPlatformService(const std::string &name,
                                                        const std::string &parentPath) {
  LOG(INFO) << "Adding PlatformService \"" << name << "\" under the path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);
  std::unique_ptr<PlatformService> upDev(new PlatformService(name, parent));
  return static_cast<PlatformService*>(addObjectByPath(std::move(upDev), path, platformSvcInterface));
}

FRU* PlatformObjectTree::addFRU(const std::string &name,
                                const std::string &parentPath,
                                bool              hotPlugSupport,
                                bool              isAvailable,
                                const std::string &fruJson) {
  LOG(INFO) << "Adding FRU \"" << name << "\" under the path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);

  std::unique_ptr<FRU> upDev(new FRU(name, parent, hotPlugSupport, isAvailable, fruJson));
  return static_cast<FRU*>(addObjectByPath(std::move(upDev), path, platformSvcInterface));
}

Sensor* PlatformObjectTree::addSensor(const std::string &name,
                                      const std::string &parentPath,
                                      const std::string &sensorJson) {
  LOG(INFO) << "Adding new Sensor \"" << name << "\" under the path \"" << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  FRU* parent = getFRU(getParent(parentPath, name));

  std::unique_ptr<Sensor> upObj(new Sensor(name, parent, sensorJson));
  return static_cast<Sensor*>(addObjectByPath(std::move(upObj), path, platformSvcInterface));
}

void PlatformObjectTree::initSensorService(std::string dbusName, std::string dbusPath, std::string dbusInteface) {
  sensorService_ = new SensorService(dbusName, dbusPath, dbusInteface);
}

const SensorService* PlatformObjectTree::getSensorService() const {
  return sensorService_;
}

bool PlatformObjectTree::setSensorServiceAvailable(bool isAvaliable) {
  return sensorService_->setIsAvailable(isAvaliable);
}

void PlatformObjectTree::initFruService(std::string dbusName, std::string dbusPath, std::string dbusInteface) {
  fruService_ = new FruService(dbusName, dbusPath, dbusInteface);
}

const FruService* PlatformObjectTree::getFruService() const {
  return fruService_;
}

bool PlatformObjectTree::setFruServiceAvailable(bool isAvaliable) {
  return fruService_->setIsAvailable(isAvaliable);
}

void PlatformObjectTree::setPlatformServiceBasePath(std::string platformServiceBasePath) {
  platformServiceBasePath_ = platformServiceBasePath;
}

std::string const & PlatformObjectTree::getPlatformServiceBasePath() const{
  return platformServiceBasePath_;
}
