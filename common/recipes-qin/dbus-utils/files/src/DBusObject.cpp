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

#include <unordered_map>
#include <memory>
#include <string>
#include <glog/logging.h>
#include <gio/gio.h>
#include "DBusObject.h"
#include "DBusInterfaceBase.h"

namespace openbmc {
namespace qin {

void DBusObject::addInterface(DBusInterfaceBase &interface, void* userData) {
  if (!containInterface(interface.getName())) {
    // the registration id is initialized as 0 meaning not registered
    std::unique_ptr<InterfaceReg> uptr(new InterfaceReg{interface, userData, 0});
    interfaceMap_.insert(std::make_pair(interface.getName(), std::move(uptr)));
  } else {
    LOG(INFO) << "Interface " << interface.getName() << " has existed.";
  }
}

void DBusObject::removeInterface(const std::string &interfaceName) {
  InterfaceMap::iterator it;
  if ((it = interfaceMap_.find(interfaceName)) == interfaceMap_.end()) {
    LOG(ERROR) << "Interface " << interfaceName << " not found.";
    return;
  }
  if (it->second.get()->id > 0) {
    LOG(ERROR) << "Interface " << interfaceName
               << " is still registered.";
    return;
  }
  interfaceMap_.erase(it);
  DCHECK(getInterface(interfaceName) == nullptr) << "Error removing interface";
}

void DBusObject::setId(const std::string &interfaceName, unsigned int id) {
  InterfaceMap::const_iterator it;
  if ((it = interfaceMap_.find(interfaceName)) == interfaceMap_.end()) {
    LOG(ERROR) << "Interface " << interfaceName << " not found.";
    return;
  }
  it->second.get()->id = id;
}

bool DBusObject::getId(const std::string &interfaceName,
                       unsigned int &id) const {
  InterfaceMap::const_iterator it;
  if ((it = interfaceMap_.find(interfaceName)) == interfaceMap_.end()) {
    LOG(WARNING) << "Interface " << interfaceName << " not found.";
    return false;
  }
  id = it->second.get()->id;
  return true;
}

} // namespace qin
} // namespace openbmc
