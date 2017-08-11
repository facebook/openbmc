/*
 * SensorObjectTree.h
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

#pragma once
#include <string>
#include <memory>
#include <ipc-interface/Ipc.h>
#include <object-tree/ObjectTree.h>
#include <object-tree/Object.h>
#include "FRU.h"
#include "Sensor.h"
#include "SensorService.h"
#include <dbus-utils/DBusInterfaceBase.h>
#include <dbus-utils/DBus.h>
#include <cstdint>

namespace openbmc {
namespace qin {

/**
 * Tree structured object superclass that manages the sensor devices and objects,
 * and bridges them with the interprocess communicaton (IPC).
 */
class SensorObjectTree : public ObjectTree {
  public:
    using ObjectTree::ObjectTree; // inherit base constructor
    // prevent compiler from mistaking addObject defined in base and derived
    using ObjectTree::addObject;

    /**
     * Get the FRU with specified path.
     */
    FRU* getFRU(const std::string &path) const {
      Object* object = getObject(path);
      if (object == nullptr) {
        return nullptr;
      }
      return getFRU(object);
    }

    /**
     * Get the sensor with specified path.
     */
    Sensor* getSensor(const std::string &path) const {
      Object* object = getObject(path);
      if (object == nullptr) {
        return nullptr;
      }
      return getSensor(object);
    }

    /**
     * Add the specified Object to objectMap_ under the specified parent
     * path. It is assumed that the object should not have any parent nor
     * children objects. It's also assumed that the Sensor can only be
     * added to FRU as the parent. Otherwise, this function will
     * throw an exception.
     */
    Object* addObject(std::unique_ptr<Object> upObj,
                      const std::string       &parentPath) override;


    /**
     * Add a SensorService to the objectMap_.
     */
     SensorService* addSensorService(const std::string &name,
                                     const std::string &parentPath);

     /**
      * Add a FRU to the objectMap_.
      */
     FRU* addFRU(const std::string &name,
                 const std::string &parentPath,
                 const std::string &id);

     /**
      * Add a Sensor to the objectMap_. The Sensor should be only
      * added under the FRU; otherwise, exception is thrown.
      */
     Sensor* addSensor(const std::string &name,
                       const std::string &parentPath,
                       const std::string &id,
                       const std::string &unit,
                       std::unique_ptr<SensorAccessMechanism> upSensorAccess);

  private:

    /**
     * Get the FRU from object.
     */
    FRU* getFRU(Object* object) const {
      FRU* fru = nullptr;
      if ((fru = dynamic_cast<FRU*>(object)) == nullptr) {
        LOG(ERROR) << "Object is not of FRU type";
        throw std::invalid_argument("Invalid cast to FRU");
      }
      return fru;
    }

    /**
     * Get the Sensor from object.
     */
    Sensor* getSensor(Object* object) const {
      Sensor* sensor = nullptr;
      if ((sensor = dynamic_cast<Sensor*>(object)) == nullptr) {
        LOG(ERROR) << "Object is not of Sensor type";
        throw std::invalid_argument("Invalid cast to Sensor");
      }
      return sensor;
    }

    /**
     * Get the DBus from object.
     */
    DBus* getDBusObject(Ipc* object) const {
      DBus* dbus = nullptr;
      if ((dbus = dynamic_cast<DBus*>(object)) == nullptr) {
        LOG(ERROR) << "Object is not of DBus type";
        throw std::invalid_argument("Invalid cast to DBus");
      }
      return dbus;
    }

    /**
     * addObject with specified interface
     */
    Object* addObject(const std::string &name,
                      const std::string &parentPath,
                      DBusInterfaceBase &interface) {
      LOG(INFO) << "Adding object \"" << name << "\" under path \""
        << parentPath << "\"";
      Object* parent = getParent(parentPath, name);
      const std::string path = getPath(parentPath, name);
      std::unique_ptr<Object> upObj(new Object(name, parent));
      return addObjectByPath(std::move(upObj), path, interface);
    }

    /**
     * addObjectByPath with specified interface
     */
    Object* addObjectByPath(std::unique_ptr<Object> upObj,
                            const std::string       &path,
                            DBusInterfaceBase       &interface) {
      Object* object = upObj.get();
      objectMap_.insert(std::make_pair(path, std::move(upObj)));

      DBus* dbus = getDBusObject(ipc_.get());
      SensorService* sensorService = nullptr;
      FRU* fru = nullptr;

      // SensorService registers sensorTree object on dbus
      // sensorTree access is required to perform add
      // and delete opeartion at SensorService object path
      if ((sensorService = dynamic_cast<SensorService*>(object)) != nullptr) {
        dbus->registerObject(path, interface, this);
      }
      else {
        dbus->registerObject(path, interface, object);
      }
      return object;
    }
};

} // namespace qin
} // namespace openbmc
