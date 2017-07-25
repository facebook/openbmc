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

#include <unordered_map>
#include <glog/logging.h>
#include "PlatformObjectTree.h"
#include "DBusPlatformSvcInterface.h"

namespace openbmc {
namespace qin {

static DBusPlatformSvcInterface platformSvcInterface;

PlatformService* PlatformObjectTree::addPlatformService(
                                          const std::string &name,
                                          const std::string &parentPath) {
  LOG(INFO) << "Adding PlatformService \"" << name << "\" under the path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);
  std::unique_ptr<PlatformService> upDev(new PlatformService(name, parent));
  return static_cast<PlatformService*>(addObjectByPath(std::move(upDev),
                                                       path,
                                                       platformSvcInterface));
}

FRU* PlatformObjectTree::addFRU(const std::string &name,
                                const std::string &parentPath,
                                const std::string &fruJson) {
  LOG(INFO) << "Adding FRU \"" << name << "\" under the path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);

  std::unique_ptr<FRU> upDev(new FRU(name, parent, fruJson));
  return static_cast<FRU*>(addObjectByPath(std::move(upDev),
                                           path,
                                           platformSvcInterface));
}

FRU* PlatformObjectTree::addFRU(
                         const std::string &name,
                         const std::string &parentPath,
                         const std::string &fruJson,
                         std::unique_ptr<HotPlugDetectionMechanism> hotPlugDetectionMechanism) {
  LOG(INFO) << "Adding FRU \"" << name << "\" under the path \""
    << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  Object* parent = getParent(parentPath, name);

  std::unique_ptr<FRU> upDev(new FRU(name,
                                     parent,
                                     fruJson,
                                     std::move(hotPlugDetectionMechanism)));
  return static_cast<FRU*>(addObjectByPath(std::move(upDev),
                                           path,
                                           platformSvcInterface));
}

Sensor* PlatformObjectTree::addSensor(const std::string &name,
                                      const std::string &parentPath,
                                      const std::string &sensorJson) {
  LOG(INFO) << "Adding new Sensor \"" << name
            << "\" under the path \"" << parentPath << "\"";
  const std::string path = getPath(parentPath, name);
  FRU* parent = getFRU(getParent(parentPath, name));

  std::unique_ptr<Sensor> upObj(new Sensor(name, parent, sensorJson));
  return static_cast<Sensor*>(addObjectByPath(std::move(upObj),
                                              path,
                                              platformSvcInterface));
}

void PlatformObjectTree::initSensorService(const std::string & dbusName,
                                           const std::string & dbusPath,
                                           const std::string & dbusInteface) {
  sensorService_ = std::unique_ptr<SensorService>(
                          new SensorService(dbusName, dbusPath, dbusInteface));
}

const SensorService* PlatformObjectTree::getSensorService() const {
  return sensorService_.get();
}

bool PlatformObjectTree::setSensorServiceAvailable(bool isAvaliable) {
  //Acquire Sensor Service Lock
  sensorServiceLock_.lock();

  bool oldAvailability = sensorService_->isAvailable();
  bool status = sensorService_->setIsAvailable(isAvaliable);

  if (status == true) {
    //Sensor Service availability updated susccessfully
    if (oldAvailability == false && isAvaliable == true) {
      //Transition from unavailable to available
      //Reset tree at sensorService_
      sensorService_->reset();
      //Push FRUs on Sensor Service
      Object* obj = getObject(platformServiceBasePath_);
      for (auto &it : obj->getChildMap()) {
        FRU* fru;
        if ((fru = dynamic_cast<FRU*>(it.second)) != nullptr) {
          // And fru and tree under fru to Sensor Service
          addFRUtoSensorService(*fru);
        }
      }
    }
  }

  //Release Sensor Service Lock
  sensorServiceLock_.unlock();
  return status;
}

void PlatformObjectTree::initFruService(const std::string & dbusName,
                                        const std::string & dbusPath,
                                        const std::string & dbusInteface) {
  fruService_ = std::unique_ptr<FruService>(
                             new FruService(dbusName, dbusPath, dbusInteface));
}

const FruService* PlatformObjectTree::getFruService() const {
  return fruService_.get();
}

bool PlatformObjectTree::setFruServiceAvailable(bool isAvaliable) {
  //Acquire Fru Service Lock
  fruServiceLock_.lock();

  bool oldAvailability = fruService_->isAvailable();
  bool status = fruService_->setIsAvailable(isAvaliable);

  if (status == true) {
    //Fru Service availability updated susccessfully
    if (oldAvailability == false && isAvaliable == true) {
      //Transition from unavailable to available
      //Reset tree at fruService_
      fruService_->reset();
      //Push FRUs on Fru Service
      Object* obj = getObject(platformServiceBasePath_);
      for (auto &it : obj->getChildMap()) {
        FRU* fru;
        if ((fru = dynamic_cast<FRU*>(it.second)) != nullptr) {
          // And fru and child frus to Fru Service
          addFRUtoFruService(*fru);
        }
      }
    }
  }

  //Release fru service lock
  fruServiceLock_.unlock();
  return status;
}

void PlatformObjectTree::setPlatformServiceBasePath(
                                 const std::string & platformServiceBasePath) {
  platformServiceBasePath_ = platformServiceBasePath;
}

const std::string & PlatformObjectTree::getPlatformServiceBasePath() const{
  return platformServiceBasePath_;
}

void PlatformObjectTree::addFRUtoSensorService(const FRU & fru) throw(const char *){
  LOG(INFO) << "addFRUtoSensorService " << fru.getName();

  //sensorServiceLock_ must be acquired before calling this method
  if (sensorServiceLock_.owns_lock() == false) {
    throw "addFRUtoSensorService called without acquiring sensor service lock";
  }

  if (fru.isAvailable() && sensorService_->isAvailable()) {
    // Get Parent of FRU Object
    Object* parent = fru.getParent();

    //Remove PlatformService Base path from FRU parent path
    std::string fruParentPath = parent->getObjectPath().erase(0,
                                             platformServiceBasePath_.length());

    //Add FRU to SensorService
    sensorService_->addFRU(fruParentPath, fru.getFruJson());

    //Vector to store json string represenation of all child Sensors
    std::vector<std::string> sensorJsonList;

    //Add all child Sensor's json string
    for (auto &it : fru.getChildMap()) {
      Sensor* sensorChild;
      if ((sensorChild = dynamic_cast<Sensor*>(it.second)) != nullptr) {
        sensorJsonList.push_back(sensorChild->getSensorJson());
      }
    }
    //Add Sensors to platform Service
    sensorService_->addSensors(fruParentPath + "/" +
                               fru.getName(), sensorJsonList);

    //Add recursively all child FRUs
    for (auto &it : fru.getChildMap()) {
      FRU* fruChild;
      if ((fruChild = dynamic_cast<FRU*>(it.second)) != nullptr) {
        addFRUtoSensorService(*fruChild);
      }
    }
  }
}

void PlatformObjectTree::removeFRUFromSensorService(const FRU & fru) throw(const char *){
  LOG(INFO) << "removeFRUFromSensorService " << fru.getName();

  //sensorServiceLock_ must be acquired before calling this method
  if (sensorServiceLock_.owns_lock() == false) {
    throw "addFRUtoSensorService called without acquiring sensor service lock";
  }

  if (sensorService_->isAvailable()) {
    sensorService_->removeFRU(fru.getObjectPath().erase(0,
                                           platformServiceBasePath_.length()));
  }
}

void PlatformObjectTree::addFRUtoFruService(const FRU & fru) throw(const char *){
  LOG(INFO) << "addFRUtoFruService " << fru.getName();

  //fruServiceLock_ must be acquired before calling this method
  if (fruServiceLock_.owns_lock() == false) {
    throw "addFRUtoFruService called without acquiring fru service lock";
  }

  if (fru.isAvailable() && fruService_->isAvailable()) {
    // Get Parent of FRU Object
    Object* parent = fru.getParent();

    //Remove PlatformService Base path from FRU parent path
    std::string fruParentPath = parent->getObjectPath().erase(0,
                                            platformServiceBasePath_.length());

    //Add FRU to FruService
    fruService_->addFRU(fruParentPath, fru.getFruJson());

    //Add recursively all child FRUs
    for (auto &it : fru.getChildMap()) {
      FRU* fruChild;
      if ((fruChild = dynamic_cast<FRU*>(it.second)) != nullptr) {
        addFRUtoFruService(*fruChild);
      }
    }
  }
}

void PlatformObjectTree::removeFRUFromFruService(const FRU & fru) throw(const char *){
  LOG(INFO) << "removeFRUFromFruService " << fru.getName();

  //fruServiceLock_ must be acquired before calling this method
  if (fruServiceLock_.owns_lock() == false) {
    throw "addFRUtoFruService called without acquiring fru service lock";
  }

  if (fruService_->isAvailable()) {
    fruService_->removeFRU(fru.getObjectPath().erase(0,
                                           platformServiceBasePath_.length()));
  }
}

} // namespace qin
} // namespace openbmc
