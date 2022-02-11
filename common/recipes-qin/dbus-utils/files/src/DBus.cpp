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

#include <functional>
#include <unordered_map>
#include <string>
#include <regex>
#include <memory>
#include <system_error>
#include <stdexcept>
#include <mutex>
#include <glog/logging.h>
#include <gio/gio.h>
#include "DBus.h"
#include "DBusInterfaceBase.h"
#include "DBusObject.h"

namespace openbmc {
namespace qin {

std::regex DBus::dbusNameRegex =
    std::regex("^([A-Za-z0-9_\\-]+\\.)+[A-Za-z0-9_\\-]+$");

std::regex DBus::pathRegex = std::regex("^(\/[A-Za-z0-9_\\-]+)+$");

DBus::DBus(const std::string &name,
           DBusInterfaceBase* interface) {
  if (interface != nullptr) {
    interface_ = interface;
  }
  if (!std::regex_match(name, dbusNameRegex)) {
    LOG(ERROR) << "DBus name " << name << " does not match the "
      << "dbus naming convention";
    throw std::invalid_argument("Invalid DBus name");
  }
  name_ = name;
}

void DBus::registerConnection() {
  std::lock_guard<std::mutex> lock(m_);
  LOG(INFO) << "Acquiring DBus name " << name_;
  if (id_ > 0) {
    LOG(WARNING) << "DBus name " << name_ << " has already been acquired";
    return;
  }
  // Reserve bus name from system dbus
  // A daemon should not reserve from session bus
  id_ = g_bus_own_name(G_BUS_TYPE_SYSTEM,
                       name_.c_str(),
                       G_BUS_NAME_OWNER_FLAGS_NONE,
                       onBusAcquired,
                       onNameAcquired,
                       onNameLost,
                       this,  // arg; throw dbus instance
                       nullptr); // g_free_func for arg
  if (id_ == 0) {
    LOG(ERROR) << "DBus name " << name_ << " cannot be acquired";
    throw std::system_error(ECONNREFUSED,
                            std::system_category(),
                            "DBus name acquisition failed");
  }
  LOG(INFO) << "DBus name " << name_ << " acquired";
}

void DBus::unregisterConnection() {
  std::lock_guard<std::mutex> lock(m_);
  LOG(INFO) << "Releasing DBus name " << name_.c_str();
  if (id_ == 0) {
    LOG(WARNING) << "DBus name " << name_ << " has already been released";
    return;
  }
  g_bus_unown_name(id_);
  id_ = 0;
  connection_ = nullptr;
  LOG(INFO) << "DBus name " << name_ << " released";
}

bool DBus::isObjectRegistered(const std::string &path,
                              DBusInterfaceBase &interface) const {
  DBusObject* object;
  {
    std::lock_guard<std::mutex> lock(m_);
    object = getMutableDBusObject(path);
  }
  if (object == nullptr) {
    return false;
  }
  return object->containInterface(interface.getName());
}

void DBus::registerObject(const std::string &path,
                          DBusInterfaceBase &interface,
                          void* userData) {
  std::lock_guard<std::mutex> lock(m_);
  DBusObject* object = getMutableDBusObject(path);
  LOG(INFO) << "Registering object at path " << path << " with interface "
    << interface.getName();
  if (object == nullptr) {
    if (!isPathAllowed(path)) {
      throw std::invalid_argument("Path does not match regex");
    }
    LOG(INFO) << "Creating new object at path " << path;
    std::unique_ptr<DBusObject> upObj(new DBusObject(path));
    object = upObj.get();
    objectMap_.insert(std::make_pair(path, std::move(upObj)));
  } else {
    unsigned int id = 0;
    if (object->getId(interface.getName(), id) && id > 0) {
      LOG(WARNING) << "Object at path " << path << " has already been"
        << " registered with interface " << interface.getName();
      return;
    }
  }

  if (connection_ != nullptr) {
    LOG(INFO) << "DBus connection good. Registering object at path " << path
      << " with interface " << interface.getName();
    registerObjectInterface(*object, interface, userData);
    return;
  }
  LOG(WARNING) << "DBus connection bad. Adding interface "
    << interface.getName() << " to the list of object at path "
    << path << " for future registration";
  object->addInterface(interface, userData);
}

void DBus::unregisterObject(const std::string &path,
                            DBusInterfaceBase &interface) {
  std::lock_guard<std::mutex> lock(m_);
  DBusObject* object = getMutableDBusObject(path);
  LOG(INFO) << "Unregistering object at path " << path << " with interface "
    << interface.getName();
  if (object == nullptr) {
    LOG(WARNING) << "Object to be unregistered does not exit at path "
      << path;
    throw std::system_error(ENOENT, std::system_category());
  }

  unregisterObjectInterface(*object, interface);
  if (object->getInterfaceMap().size() == 0) {
    LOG(INFO) << "Deleting object at path " << path;
    objectMap_.erase(path);
  }
}

void DBus::unregisterObject(const std::string &path) {
  std::lock_guard<std::mutex> lock(m_);
  DBusObject* object = getMutableDBusObject(path);
  LOG(INFO) << "Unregistering object at path " << path;
  if (object == nullptr) {
    LOG(WARNING) << "Object to be unregistered does not exit at path "
      << path;
    throw std::system_error(ENOENT, std::system_category());
  }

  const DBusObject::InterfaceMap &map = object->getInterfaceMap();
  while (map.size() > 0) {
    unregisterObjectInterface(*object, map.begin()->second.get()->interface);
  }
  DCHECK(object->getInterfaceMap().size() == 0) << "Interfaces are not "
    << "completely removed from object's interfaceMap";
  LOG(INFO) << "Deleting object at path " << path;
  objectMap_.erase(path);
}

void DBus::registerObjectInterface(DBusObject              &object,
                                   DBusInterfaceBase &interface,
                                   void*                   userData) {
  DCHECK(connection_ != nullptr) << "No Connection when registering object "
    << object.getPath() << " with interface " << interface.getName();
  DCHECK(objectMap_.find(object.getPath()) != objectMap_.end())
    << "Object " << object.getPath() << " not found in objectMap";

  unsigned int id = 0;
  DCHECK(!object.getId(interface.getName(), id) || id == 0)
    << "Object " << object.getPath() << " has been registered with interface "
    << interface.getName();

  object.addInterface(interface, userData);
  GError* error = nullptr;
  id = g_dbus_connection_register_object (
              connection_,
              object.getPath().c_str(),
              interface.getInfo()->interfaces[interface.getNo()],
              interface.getVtable(),
              userData,  // throw as a parameter to callback
              nullptr,   // arg_free_func
              &error);  // GError**
  if (id == 0) {
    LOG(ERROR) << "Object registration failed at path "<< object.getPath()
      << " with interface " << interface.getName();
    throw std::runtime_error("Object registration failed: " +
                             std::string(error->message));
  }

  object.setId(interface.getName(), id);
  LOG(INFO) << "Object at path " << object.getPath() << " registered"
    << " with interface " << interface.getName() << " with id " << id;
}

void DBus::unregisterObjectInterface(DBusObject              &object,
                                     DBusInterfaceBase &interface) {
  DCHECK(objectMap_.find(object.getPath()) != objectMap_.end())
    << "Object " << object.getPath() << " not found in objectMap";

  unsigned int id = 0;
  DCHECK(object.getId(interface.getName(), id))
    << "Object to be unregistered does not have the interface "
    << interface.getName() << " at path " << object.getPath();

  if (id > 0 && connection_ != nullptr &&
      !g_dbus_connection_unregister_object(connection_, id)) {
    LOG(ERROR) << "Object cannot be unregistered with the interface "
       << interface.getName() << " at path " << object.getPath();
    throw std::runtime_error("Object unregistration failed");
  }

  std::string path = object.getPath();
  if (id > 0) {
    // set id to 0 needed before removing the interface
    object.setId(interface.getName(), 0);
  }
  object.removeInterface(interface.getName());
  LOG(INFO) << "Object at path " << path << " unregistered"
    << " with interface " << interface.getName();
}

void DBus::onNameAcquired(GDBusConnection *connection,
                         const gchar     *name,
                         gpointer         arg) {
  LOG(INFO) << "DBus connection to be registered with name "
    << name << " acquired";

  DBus* dbus = static_cast<DBus*>(arg);
  {
    std::lock_guard<std::mutex> lock(dbus->m_);
    DBusObjectMap &oMap = dbus->objectMap_;
    if (dbus->onConnAcquired) {
      LOG(INFO) << "Executing user defined function";
      dbus->onConnAcquired();
    }

    LOG(INFO) << "Registering objects with interfaces";
    dbus->connection_ = connection;
    for (auto i = oMap.begin(); i != oMap.end(); i++) {
      DBusObject* object = i->second.get();
      const DBusObject::InterfaceMap &iMap = object->getInterfaceMap();
      for (auto j = iMap.begin(); j != iMap.end(); j++) {
        try {
          // need to catch exception here since throw can't get throw c library
          DBusInterfaceBase* interface = object->getInterface(j->first);
          void* userData = object->getUserData(j->first);
          dbus->registerObjectInterface(*object, *interface, userData);
        } catch (std::runtime_error &e) {
          // currently don't do anything but log
          LOG(ERROR) << e.what();
        }
      }
    }
  }
  dbus->cvConn_.notify_all(); // notify connection has been setup
}

void DBus::onNameLost(GDBusConnection *connection,
                      const char      *name,
                      gpointer         arg) {
  LOG(WARNING) << "DBus name " << name << " lost.";

  DBus* dbus = static_cast<DBus*>(arg);
  std::lock_guard<std::mutex> lock(dbus->m_);
  DBusObjectMap &oMap = dbus->objectMap_;
  if (dbus->onConnLost) {
    LOG(INFO) << "Executing user defined function";
    dbus->onConnLost();
  }

  LOG(INFO) << "Resetting id for each object";
  dbus->connection_ = nullptr;
  for (auto i = oMap.begin(); i != oMap.end(); i++) {
    DBusObject* object = i->second.get();
    const DBusObject::InterfaceMap &iMap = object->getInterfaceMap();
    for (auto j = iMap.begin(); j != iMap.end(); j++) {
      // unregister all the objects
      object->setId(j->second.get()->interface.getName(), 0);
    }
  }

  if (connection == nullptr) {
    LOG(WARNING) << "DBus connection failed";
  } else {
    LOG(WARNING) << "DBus daemon refused the DBus name " << name;
    if (!g_dbus_connection_close_sync(connection, nullptr, nullptr)) {
      LOG(ERROR) << "DBus connection cannot be closed";
    }
  }
}

} // namespace qin
} // namespace openbmc
