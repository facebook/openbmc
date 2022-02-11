/*
 * FruObjectTree.h
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
#include <dbus-utils/DBusInterfaceBase.h>
#include <dbus-utils/DBus.h>
#include <ipc-interface/Ipc.h>
#include <object-tree/ObjectTree.h>
#include <object-tree/Object.h>
#include "FRU.h"
#include "FruService.h"

namespace openbmc {
namespace qin {

/**
 * Tree structured object superclass that manages the fru objects,
 * and bridges them with the interprocess communicaton (IPC).
 */
class FruObjectTree : public ObjectTree {
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
     * Add a FruService to the objectMap_.
     */
     FruService* addFruService(const std::string &name,
                               const std::string &parentPath);

     /**
      * Add a FRU to the objectMap_.
      */
     FRU* addFRU(const std::string                     &name,
                 const std::string                     &parentPath,
                 std::unique_ptr<FruIdAccessMechanism> fruIdAccess);

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
      FruService* fruService = nullptr;

      // FruService registers fruTree object on dbus
      // fruTree access is required to perform add and delete operation at FruService object path
      if ((fruService = dynamic_cast<FruService*>(object)) != nullptr) {
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
