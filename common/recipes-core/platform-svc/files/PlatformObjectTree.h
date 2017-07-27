/*
 * PlatformObjectTree.h
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
#include <mutex>
#include <vector>
#include <ipc-interface/Ipc.h>
#include <dbus-utils/DBusInterfaceBase.h>
#include <dbus-utils/DBus.h>
#include <object-tree/ObjectTree.h>
#include <object-tree/Object.h>
#include "FRU.h"
#include "FruService.h"
#include "HotPlugDetectionMechanism.h"
#include "PlatformService.h"
#include "Sensor.h"
#include "SensorService.h"

namespace openbmc {
namespace qin {

/**
 * Tree structured object that manages the Platform Services,
 * and bridges them with the interprocess communicaton (IPC).
 */
class PlatformObjectTree : public ObjectTree {
  private:
    std::unique_ptr<SensorService> sensorService_ = nullptr;
                            // SensorService, to communicate with SensorService
    std::unique_ptr<FruService> fruService_ = nullptr;
                            // FruService, to communicate with SensorService
    std::string platformServiceBasePath_;
                            // Base Path for PlatformService object
    std::unique_lock<std::mutex> sensorServiceLock_
            {std::unique_lock<std::mutex>(*(new std::mutex()), std::defer_lock)};
                            // To serialize push/remove Sensor Tree operations
    std::unique_lock<std::mutex> fruServiceLock_
            {std::unique_lock<std::mutex>(*(new std::mutex()), std::defer_lock)};
                            // To serialize push/remove Fru Tree operations

  public:
    using ObjectTree::ObjectTree; // inherit base constructor
    // prevent compiler from mistaking addObject defined in base and derived
    using ObjectTree::addObject;

    /**
     * Initialize SensorService, called from PlatformJsonParser
     */
    void initSensorService(const std::string & dbusName,
                           const std::string & dbusPath,
                           const std::string & dbusInteface);

    /**
     * Returns pointer to SensorService.
     */
    const SensorService* getSensorService() const;

    /**
     * Set SensorService Availability.
     */
    bool setSensorServiceAvailable(bool isAvaliable);

    /**
     * Initialize FruService, called from PlatformJsonParser
     */
    void initFruService(const std::string & dbusName,
                        const std::string & dbusPath,
                        const std::string & dbusInteface);

    /**
     * Returns pointer to FruService.
     */
    const FruService* getFruService() const;

    /**
     * Set FruService Availability.
     */
    bool setFruServiceAvailable(bool isAvaliable);

    /**
     * Set PlatformService Base Path, set from PlatformJsonParser
     */
    void setPlatformServiceBasePath(const std::string & platformServiceBasePath);

    /**
     * Returns PlatformService Base path.
     */
    std::string const & getPlatformServiceBasePath() const;

    /**
     * Add a PlatformService to the objectMap_.
     */
    PlatformService* addPlatformService(const std::string &name,
                                        const std::string &parentPath);

    /**
     * Add a FRU to the objectMap_.
     */
    FRU* addFRU(const std::string &name,
                const std::string &parentPath,
                const std::string &fruJson);

    /**
     * Add a Hotplug supported FRU to the objectMap_.
     */
    FRU* addFRU(
             const std::string &name,
             const std::string &parentPath,
             const std::string &fruJson,
             std::unique_ptr<HotPlugDetectionMechanism> hotPlugDetectionMechanism);

    /**
     * Add a Sensor to the objectMap_. The Sensor should be only
     * added under the FRU; otherwise, exception is thrown.
     */
    Sensor* addSensor(const std::string &name,
                      const std::string &parentPath,
                      const std::string &sensorJson);

    /**
     * Returns number of FRUs which supports internal hot plug detection
     */
    int getNofHPIntDetectSupportedFrus() {
      LOG(INFO) << "getNofHPIntDetectSupportedFrus";
      return getNofHPIntDetectSupportedFrusRec(*getObject(platformServiceBasePath_));
    }

    /*
     * This method goes through all frus of platform object tree
     * and checks if there is any change in fru availability.
     * If there is change in fru availability,
     * update the same on fru and sensor service
     * todo: This function traverses whole tree
     *       Need changes in data struture to get direct access to frus
     */
    void checkHotPlugSupportedFrus() {
      LOG(INFO) << "checkHotPlugSupportedFrus";
      checkHotPlugSupportedFrusRec(*getObject(platformServiceBasePath_));
    }

    /*
     * Sets availability of fru at fruPath
     * Returns if operation is successful
     */
    bool setFruAvailable(const std::string & fruPath, bool isAvailable);

  private:

    /**
     * Push fru and subtree under fru on Sensor Service
     * sensorServiceLock_ must be acquired before calling this method
     */
    void addFRUtoSensorService(const FRU & fru) throw(const char *);

    /**
     * Delete fru and subtree under fru at Sensor Service
     * sensorServiceLock_ must be acquired before calling this method
     */
    void removeFRUFromSensorService(const FRU & fru) throw(const char *);

    /**
     * Push fru and fru tree under fru on Fru Service
     * fruServiceLock_ must be acquired before calling this method
     */
    void addFRUtoFruService(const FRU & fru) throw(const char *);

    /**
     * Delete fru and fru tree under fru at Fru Service
     * fruServiceLock_ must be acquired before calling this method
     */
    void removeFRUFromFruService(const FRU & fru) throw(const char *);

    /**
     * Returns number of frus which supports internal detection of hotplug
     * under obj subtree
     */
    int getNofHPIntDetectSupportedFrusRec(const Object & obj);

    /**
     * Recursively traverses through tree at obj and checks status of frus
     * which supports internal hotplug detection
     */
    void checkHotPlugSupportedFrusRec(const Object & obj);

    /**
     * Updates fru service and sensor service on change in fru availability
     */
    void changeInFruAvailabilityHandler(const FRU & fru);

    /**
     * Checks if all frus in path from platform service to fru are available
     */
    bool checkIfParentFruAvailable(const FRU & fru);

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
      //Register with platformObjectTree object
      dbus->registerObject(path, interface, this);
      return object;
    }
};

} // namespace qin
} // namespace openbmc
