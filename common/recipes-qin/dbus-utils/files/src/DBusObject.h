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
#include <unordered_map>
#include <memory>
#include <string>
#include <gio/gio.h>
#include "DBusInterfaceBase.h"

namespace openbmc {
namespace qin {

/**
 * DBus object stores object information
 */
class DBusObject {
  public:
    struct InterfaceReg {
      DBusInterfaceBase &interface; // interface object pointer
      void*             userData;  // user data to be an arg in callback
      unsigned int      id;        // reg id of interface
    };

    // The object does not own the interface but keeps a record of what
    // interfaces it is (will be) registered with.
    typedef std::unordered_map<std::string,
                               std::unique_ptr<InterfaceReg>> InterfaceMap;
  private:
    std::string  path_;         // object path
    InterfaceMap interfaceMap_; // hashtable of InterfaceReg_t with interface
                                // name as the key

  public:
    /**
     * Constructor for DBusObject
     *
     * @param objectPath specifies the object path
     */
    DBusObject(const std::string &objectPath) {
      path_ = objectPath;
    }

    const std::string& getPath() const{
      return path_;
    }

    const InterfaceMap& getInterfaceMap() const {
      return interfaceMap_;
    }

    /**
     * Get the interface pointer in the map with specified name
     *
     * @param interface name
     * @return nullptr if not found; interface pointer otherwise
     */
    DBusInterfaceBase* getInterface(
        const std::string &interfaceName) const {
      InterfaceMap::const_iterator it;
      if ((it = interfaceMap_.find(interfaceName)) == interfaceMap_.end()) {
        return nullptr;
      }
      return &(it->second.get()->interface);
    }

    bool containInterface(const std::string &interfaceName) const {
      return getInterface(interfaceName) != nullptr;
    }

    /**
     *  Get the user data associated with the specified interface.
     *
     * @param interface name
     * @return nullptr if not found; userData pointer otherwise
     */
    void* getUserData(const std::string &interfaceName) const {
      InterfaceMap::const_iterator it;
      if ((it = interfaceMap_.find(interfaceName)) == interfaceMap_.end()) {
        return nullptr;
      }
      return it->second.get()->userData;
    }

    /**
     * Add interface to be registered. It will be ref during the
     * registration.
     *
     * @param interface is the object of the class inherited from
     *        DBusInterfaceBase
     * @param userData to be put into the callback function of the
     *        interface
     */
    void addInterface(DBusInterfaceBase &interface, void* userData);

    /**
     * Remove interface from interfaceMap. It is assumed that
     * the corresponding id is zero (unregistered).
     *
     * @param interface name
     */
    void removeInterface(const std::string &interfaceName);

    /**
     * Set the registration id for specified interface
     *
     * @param interfaceName is the name of the interface
     * @return true if success; false if interfaceName is not found
     */
    void setId(const std::string &interfaceName, unsigned int id);

    /**
     * Get the registration id of specified interface
     *
     * @param interfaceName is the name of the interface
     * @param id return
     * @return true if success; false if interfaceName is not found
     */
    bool getId(const std::string &interfaceName, unsigned int &id) const;

};
} // namespace qin
} // namespace openbmc
