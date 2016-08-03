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
#include <memory>
#include <ipc-interface/Ipc.h>
#include <object-tree/ObjectTree.h>
#include <object-tree/Object.h>
#include "SensorDevice.h"
#include "SensorObject.h"
#include "SensorApi.h"

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
     * Get the sensor device with specified path.
     *
     * @param path to the object
     * @throw std::invalid_argument if the object at path is not of
     *        SensorDevice type
     * @return nullptr if not found; SensorDevice pointer otherwise
     */
    SensorDevice* getSensorDevice(const std::string &path) const {
      Object* object = getObject(path);
      if (object == nullptr) {
        return nullptr;
      }
      return getSensorDevice(object);
    }

    /**
     * Get the sensor object with specified path.
     *
     * @param path to the object
     * @throw std::invalid_argument if the object at path is not of
     *        SensorObject type
     * @return nullptr if not found; SensorObject pointer otherwise
     */
    SensorObject* getSensorObject(const std::string &path) const {
      Object* object = getObject(path);
      if (object == nullptr) {
        return nullptr;
      }
      return getSensorObject(object);
    }

    /**
     * Add the specified Object to objectMap_ under the specified parent
     * path. It is assumed that the object should not have any parent nor
     * children objects. It's also assumed that the SensorObject can only be
     * added to SensorDevice as the parent. Otherwise, this function will
     * throw an exception. Note that this provides the interface for adding any
     * derived class instances of SensorObject such as SensorTemp, SensorPower,
     * and so on.
     *
     * @param unique_ptr to Object to be added
     * @param parentPath is the parent object path for the object
     *        to be added at
     * @throw std::invalid_argument if
     *        * the unique_ptr to object is empty
     *        * object has non-empty children or non-null parent
     *        * parentPath not found
     *        * object is SensorObject but parent is not SensorDevice
     *        * name does not meet the ipc's rule
     *        * the object with the same name has already existed under
     *          the parent
     * @return pointer to the created Object
     */
    Object* addObject(std::unique_ptr<Object> upObj,
                      const std::string       &parentPath) override;

    /**
     * Add a SensorDevice to the objectMap_. Need to specify the SensorApi.
     *
     * @param name of the object to be added
     * @param parentPath is the parent object path for the object
     *        to be added at
     * @param uSensorApi unique_ptr of sensorApi to be transferred to the
     *        sensor device to read or write attribute values
     * @throw std::invalid_argument if parentPath not found or the object with
     *        the name has already existed under the parent.
     * @return pointer to the created SensorDevice
     */
    SensorDevice* addSensorDevice(const std::string           &name,
                                  const std::string           &parentPath,
                                  std::unique_ptr<SensorApi>  uSensorApi);

    /**
     * Add a SensorObject to the objectMap_. The SensorObject should be only
     * added under the SensorDevice; otherwise, exception is thrown.
     *
     * @param name of the object to be added
     * @param parentPath is the parent object path for the object
     *        to be added at
     * @throw std::invalid_argument if parentPath not found, the parent is not
     *        SensorDevice, the object with the name has already existed
     *        under the parent, or the sensorType specified is not valid
     * @return pointer to the created SensorObject
     */
    SensorObject* addSensorObject(const std::string &name,
                                  const std::string &parentPath);

  private:

    /**
     * Get the SensorDevice from object.
     *
     * @param object to be transformed
     * @throw std::invalid_argument if the object is not of SensorDevice type
     * @return pointer to SensorDevice
     */
    SensorDevice* getSensorDevice(Object* object) const {
      SensorDevice* sDevice = nullptr;
      if ((sDevice = dynamic_cast<SensorDevice*>(object)) == nullptr) {
        LOG(ERROR) << "Object is not of SensorDevice type";
        throw std::invalid_argument("Invalid cast to SensorDevice");
      }
      return sDevice;
    }

    /**
     * Get the SensorObject from object.
     *
     * @param object to be transformed
     * @throw std::invalid_argument if the object is not of SensorObject type
     * @return pointer to SensorObject
     */
    SensorObject* getSensorObject(Object* object) const {
      SensorObject* sObject = nullptr;
      if ((sObject = dynamic_cast<SensorObject*>(object)) == nullptr) {
        LOG(ERROR) << "Object is not of SensorObject type";
        throw std::invalid_argument("Invalid cast to SensorObject");
      }
      return sObject;
    }
};

} // namespace qin
} // namespace openbmc
