/*
 * DBusSensorTreeInterface.cpp
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

#include <string>
#include <vector>
#include <glog/logging.h>
#include <gio/gio.h>
#include "DBusSensorTreeInterface.h"
#include "FRU.h"
#include "Sensor.h"

namespace openbmc {
namespace qin {

static const char* xml =
  "<!DOCTYPE node PUBLIC"
  " \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
  " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
  "<node>"
  "  <interface name='org.openbmc.SensorTree'>"
  "    <method name='getSensorPathByName'>"
  "      <arg type='s' name='name' direction='in'/>"
  "      <arg type='s' name='path' direction='out'/>"
  "    </method>"
  "    <method name='getSensorPathById'>"
  "      <arg type='y' name='id' direction='in'/>"
  "      <arg type='s' name='path' direction='out'/>"
  "    </method>"
  "    <method name='getFRUList'>"
  "      <arg type='as' name='path' direction='out'/>"
  "    </method>"
  "    <method name='getFruPathByName'>"
  "      <arg type='s' name='name' direction='in'/>"
  "      <arg type='s' name='path' direction='out'/>"
  "    </method>"
  "    <method name='getFruPathById'>"
  "      <arg type='y' name='id' direction='in'/>"
  "      <arg type='s' name='path' direction='out'/>"
  "    </method>"
  "    <method name='getSensorObjects'>"
  "      <arg type='a(syids)' name='sensorlist' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

DBusSensorTreeInterface::DBusSensorTreeInterface() {
  info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
  if (info_ == nullptr) {
    LOG(ERROR) << "xml parsing for dbus interface failed";
    throw std::invalid_argument("XML parsing failed");
  }
  no_ = 0;
  name_ = info_->interfaces[no_]->name;
  vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
}

DBusSensorTreeInterface::~DBusSensorTreeInterface() {
  g_dbus_node_info_unref(info_);
}

/*
* Helper function, returns path of Object obj
*/
static std::string getPathToCurrentObject(Object* obj){
  if (obj->getParent() == nullptr){
    return "/" + obj->getName();
  }
  else {
    return getPathToCurrentObject(obj->getParent()) + "/" + obj->getName();
  }
}

/*
* Helper function, returns vector of FRU names of
* all FRUs under Object obj subtree
*/
static std::vector<std::string> getFRUListRec(Object* obj) {
  std::vector<std::string> sensorObjectPaths;

  for (auto &it : obj->getChildMap()) {
    if (dynamic_cast<FRU*>(it.second) != nullptr){
      sensorObjectPaths.push_back(it.second->getName());
      std::vector<std::string> childSubtree = getFRUListRec(it.second);
      if (!childSubtree.empty()){
        for (auto &itVec : childSubtree) {
          sensorObjectPaths.push_back(itVec);
        }
      }
    }
  }

  return sensorObjectPaths;
}

void DBusSensorTreeInterface::getFRUList(GDBusMethodInvocation* invocation,
                                         gpointer               arg) {
  GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
  Object* obj = static_cast<Object*>(arg);
  LOG(INFO) << "getFRUList " << obj->getName();

  std::vector<std::string> fruList = getFRUListRec(obj);
  std::string objectPath = getPathToCurrentObject(obj);

  for (auto &it : fruList) {
    g_variant_builder_add(builder, "s", it.c_str());
  }

  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(as)", builder));
  g_variant_builder_unref(builder);
}


/*
* Helper function, locates FRU with fruName under subtree and returns its path
*/
static std::string getFruPathByNameRec(Object* obj, std::string fruName) {
  std::string path;

  for (auto &it : obj->getChildMap()) {
    if (dynamic_cast<FRU*>(it.second) != nullptr) {
      //Child is FRU
      if (fruName.compare(it.first) == 0){
        // FRU found
        path = it.first;
        break;
      }
      else {
        path = getFruPathByNameRec(it.second, fruName);
        if (!path.empty()){
          // If sensor is located no need to search more
          path = it.first + "/" + path;
          break;
        }
      }
    }
  }

  return path;
}

void DBusSensorTreeInterface::getFruPathByName(
                                          GDBusMethodInvocation* invocation,
                                          GVariant*              parameters,
                                          gpointer               arg){
  Object* obj = static_cast<Object*>(arg);
  const gchar *fruName;
  g_variant_get(parameters, "(&s)", &fruName);

  LOG(INFO) << "getFruPathByName of " << fruName << " from " << obj->getName();
  std::string path = getFruPathByNameRec(obj, std::string(fruName));

  if (!path.empty()) {
    path = getPathToCurrentObject(obj) + "/" + path;
  }

  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", path.c_str()));
}

/*
* Helper function, locates FRU with fruId under subtree and returns its path
*/
static std::string getFruPathByIdRec(Object* obj, uint8_t fruId) {
  std::string path;

  for (auto &it : obj->getChildMap()) {
    if (dynamic_cast<FRU*>(it.second) != nullptr) {
      //Child is sensor device
      if (fruId == ((FRU*)it.second)->getId()){
        // FRU found
        path = it.first;
        break;
      }
      else {
        path = getFruPathByIdRec(it.second, fruId);
        if (!path.empty()){
          // If sensor is located no need to search more
          path = it.first + "/" + path;
          break;
        }
      }
    }
  }
  return path;
}

void DBusSensorTreeInterface::getFruPathById(GDBusMethodInvocation* invocation,
                                             GVariant*              parameters,
                                             gpointer               arg){
  Object* obj = static_cast<Object*>(arg);
  gchar fruId;
  g_variant_get(parameters, "(y)", &fruId);

  LOG(INFO) << "getFruPathById of " << fruId << " from " << obj->getName();
  std::string path = getFruPathByIdRec(obj, fruId);

  if (!path.empty()) {
    path = getPathToCurrentObject(obj) + "/" + path;
  }

  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", path.c_str()));
}

/*
* Helper function, locates Sensor with sensorName under subtree and
* returns its path
*/
static std::string getSensorPathByNameRec(Object* obj,
                                          std::string sensorName) {
  std::string path;

  for (auto &it : obj->getChildMap()) {
    if (dynamic_cast<FRU*>(it.second) != nullptr) {
      //Child is FRU
      path = getSensorPathByNameRec(it.second, sensorName);
      if (!path.empty()){
        // If sensor is located no need to search more
        path = it.first + "/" + path;
        break;
      }
    }
    else if (dynamic_cast<Sensor*>(it.second) != nullptr) {
      //Child is sensor object
      if (sensorName.compare(it.second->getName()) == 0) {
        path = it.second->getName();
        break;
      }
    }
  }

  return path;
}

void DBusSensorTreeInterface::getSensorPathByName(
                                             GDBusMethodInvocation* invocation,
                                             GVariant*              parameters,
                                             gpointer               arg) {
  Object* obj = static_cast<Object*>(arg);
  const gchar *sensorName;
  g_variant_get(parameters, "(&s)", &sensorName);

  LOG(INFO) << "getSensorPath of " << sensorName << " from " << obj->getName();
  std::string path = getSensorPathByNameRec(obj, std::string(sensorName));

  if (!path.empty()) {
    path = getPathToCurrentObject(obj) + "/" + path;
  }

  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", path.c_str()));
}

/*
* Helper function, locates Sensor with id under subtree and returns its path
*/
static std::string getSensorPathByIdRec(Object* obj, uint16_t id) {
  std::string path;

  for (auto &it : obj->getChildMap()) {
    if (dynamic_cast<FRU*>(it.second) != nullptr) {
      //Child is FRU
      path = getSensorPathByIdRec(it.second, id);
      if (!path.empty()){
        // If sensor is located no need to search more
        path = it.first + "/" + path;
        break;
      }
    }
    else if (dynamic_cast<Sensor*>(it.second) != nullptr) {
      //Child is sensor object
      if (((Sensor*) it.second)->getId() == id) {
        path = it.second->getName();
        break;
      }
    }
  }

  return path;
}

void DBusSensorTreeInterface::getSensorPathById(
                                             GDBusMethodInvocation* invocation,
                                             GVariant*              parameters,
                                             gpointer               arg) {
  Object* obj = static_cast<Object*>(arg);

  uint8_t id;
  g_variant_get(parameters, "(y)", &id);

  LOG(INFO) << "getSensorPathById of " << (int)id
            << " from " << obj->getName();

  std::string path = getSensorPathByIdRec(obj, id);

  if (!path.empty()) {
    path = getPathToCurrentObject(obj) + "/" + path;
  }

  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", path.c_str()));
}

/**
 * Recursively traverses through subtree under Object obj and
 * adds all sensor objects to GVariantBuilder* builder
 */
static void addSensorObjects(GVariantBuilder* builder, Object* obj) {
  for (auto &it : obj->getChildMap()) {
    Sensor* sensor;
    if ((sensor = dynamic_cast<Sensor*>(it.second)) != nullptr) {
      //child is Sensor, add it to builder
      g_variant_builder_add(builder,
                            "(syids)",
                            sensor->getName().c_str(),
                            sensor->getId(),
                            sensor->getLastReadStatus(),
                            sensor->getValue(),
                            sensor->getUnit().c_str());
    }
  }

  for (auto &it : obj->getChildMap()) {
    if (dynamic_cast<FRU*>(it.second) != nullptr) {
      //Recursively call on child FRU
      addSensorObjects(builder, it.second);
    }
  }
}

void DBusSensorTreeInterface::getSensorObjects(
                                           GDBusMethodInvocation* invocation,
                                           gpointer               arg) {
  Object* obj = static_cast<Object*>(arg);
  LOG(INFO) << "getSensorObjects of " << obj->getName();

  GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("a(syids)"));

  addSensorObjects(builder, obj);

  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(a(syids))", builder));
  g_variant_builder_unref(builder);
}

void DBusSensorTreeInterface::methodCallBack(
                          GDBusConnection*       connection,
                          const char*            sender,
                          const char*            objectPath,
                          const char*            interfaceName,
                          const char*            methodName,
                          GVariant*              parameters,
                          GDBusMethodInvocation* invocation,
                          gpointer               arg) {
  // arg should be a pointer to Object in object-tree
  DCHECK(arg != nullptr) << "Empty object passed to callback";

  if (g_strcmp0(methodName, "getSensorPathByName") == 0) {
    getSensorPathByName(invocation, parameters, arg);
  }
  else if (g_strcmp0(methodName, "getSensorPathById") == 0) {
    getSensorPathById(invocation, parameters, arg);
  }
  else if (g_strcmp0(methodName, "getFRUList") == 0) {
    getFRUList(invocation, arg);
  }
  else if (g_strcmp0(methodName, "getFruPathByName") == 0) {
    getFruPathByName(invocation, parameters, arg);
  }
  else if (g_strcmp0(methodName, "getFruPathById") == 0) {
    getFruPathById(invocation, parameters, arg);
  }
  else if (g_strcmp0(methodName, "getSensorObjects") == 0) {
    getSensorObjects(invocation, arg);
  }
}

} // namespace qin
} // namespace openbmc
