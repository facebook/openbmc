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
#include "SensorObject.h"
#include "SensorApi.h"

namespace openbmc {
namespace ipc {

/**
 * Tree structured object superclass that manages the sensor objects and
 * bridges them with the interprocess communicaton (IPC).
 */
class SensorObjectTree : public ObjectTree {
  public:
    using ObjectTree::ObjectTree; // inherit base constructor

    /**
     * Add a SensorObject to the objectMap_. Need to specify the SensorApi.
     *
     * @param name of the object to be added
     * @param parentPath is the parent object path for the object
     *        to be added at
     * @param uSensorApi unique_ptr of sensorApi to be transferred to the
     *        sensor object to read or write attribute values
     * @return nullptr if parentPath not found or the object with name
     *         has already existed under the parent; otherwise,
     *         object is created and added
     */
    SensorObject* addSensorObject(const std::string           &name,
                                  const std::string           &parentPath,
                                  std::unique_ptr<SensorApi>  uSensorApi);

};

} // namespace ipc
} // namespace openbmc
