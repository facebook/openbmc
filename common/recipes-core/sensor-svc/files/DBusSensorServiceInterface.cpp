/*
 * DBusSensorServiceInterface.cpp
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
#include <glog/logging.h>
#include <gio/gio.h>
#include <nlohmann/json.hpp>
#include "DBusSensorServiceInterface.h"
#include "SensorJsonParser.h"

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
  "    <method name='addFRU'>"
  "      <arg type='s' name='fruParentPath' direction='in'/>"
  "      <arg type='s' name='fruJsonString' direction='in'/>"
  "      <arg type='b' name='status' direction='out'/>"
  "    </method>"
  "    <method name='addSensors'>"
  "      <arg type='s' name='fruPath' direction='in'/>"
  "      <arg type='as' name='sensorJsonString' direction='in'/>"
  "      <arg type='b' name='status' direction='out'/>"
  "    </method>"
  "    <method name='resetTree'>"
  "    </method>"
  "    <method name='removeFRU'>"
  "      <arg type='s' name='fruPath' direction='in'/>"
  "      <arg type='b' name='status' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

DBusSensorServiceInterface::DBusSensorServiceInterface() {
  info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
  if (info_ == nullptr) {
    LOG(ERROR) << "xml parsing for dbus interface failed";
    throw std::invalid_argument("XML parsing failed");
  }
  no_ = 0;
  name_ = info_->interfaces[no_]->name;
  vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
}

DBusSensorServiceInterface::~DBusSensorServiceInterface() {
  g_dbus_node_info_unref(info_);
}

void DBusSensorServiceInterface::addFRU(GDBusMethodInvocation* invocation,
                                        GVariant*              parameters,
                                        SensorObjectTree*      sensorTree,
                                        const char*            objectPath) {
  gchar* fruJsonString = NULL;
  gchar* fruParentPath = NULL;
  gboolean status = TRUE;

  g_variant_get(parameters, "(ss)", &fruParentPath, &fruJsonString);

  LOG(INFO) << "addFRU at " << fruParentPath << " FRU: " << fruJsonString;

  if (sensorTree->getObject(fruParentPath) == nullptr) {
    LOG(ERROR) << "Object " << fruParentPath << " does not exists";
    status = FALSE;
  }
  else {
    //Covert fruJsonString to nlohmann::json jObject and add fru to SensorTree
    nlohmann::json jObject = nlohmann::json::parse(fruJsonString);
    SensorJsonParser::parseFRU(jObject, *sensorTree, fruParentPath);
  }

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", status));
}

void DBusSensorServiceInterface::addSensors(GDBusMethodInvocation* invocation,
                                            GVariant*              parameters,
                                            SensorObjectTree*      sensorTree,
                                            const char*            objectPath) {
  GVariantIter* iter = NULL;
  gchar* sensorJsonString = NULL;
  gchar* frupath = NULL;
  gboolean status = TRUE;

  g_variant_get(parameters, "(sas)", &frupath, &iter);

  LOG(INFO) << "addSensors at " << frupath;

  if (dynamic_cast<FRU*>(sensorTree->getObject(frupath)) == nullptr) {
    LOG(ERROR) << "Fru " << frupath << " does not exists";
    status = FALSE;
  }
  else if (iter != NULL) {
    while (g_variant_iter_loop (iter, "&s", &sensorJsonString)) {
      LOG(INFO) << "Sensor :" << sensorJsonString;

      //Covert sensorJsonString to nlohmann::json jObject
      //and add sensor to SensorTree
      nlohmann::json jObject = nlohmann::json::parse(sensorJsonString);
      SensorJsonParser::parseSensor(jObject, *sensorTree, frupath);
    }
  }

  g_variant_iter_free (iter);
  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", status));
}

/**
 * Helper function to remove subtree at Object obj from sensorTree
 */
static void deleteSubtree(SensorObjectTree* sensorTree , Object* obj) {
  Object::ChildMap childMap = obj->getChildMap();
  for (auto it = childMap.cbegin(); it != childMap.cend();) {
    deleteSubtree(sensorTree, (it++)->second);
  }

  sensorTree->deleteObjectByPath(obj->getObjectPath());
}

void DBusSensorServiceInterface::resetTree(GDBusMethodInvocation* invocation,
                                           SensorObjectTree*      sensorTree,
                                           const char*            objectPath) {
  LOG(INFO) << "resetTree at " << objectPath;

  Object* obj = sensorTree->getObject(objectPath);
  Object::ChildMap childMap = obj->getChildMap();
  for (auto it = childMap.cbegin(); it != childMap.cend();) {
    deleteSubtree(sensorTree, (it++)->second);
  }

  g_dbus_method_invocation_return_value (invocation, NULL);
}

void DBusSensorServiceInterface::removeFRU(GDBusMethodInvocation* invocation,
                                           GVariant*              parameters,
                                           SensorObjectTree*      sensorTree,
                                           const char*            objectPath) {
  gchar* fruPath = NULL;
  gboolean status = TRUE;
  g_variant_get(parameters, "(&s)", &fruPath);

  LOG(INFO) << "removeFRU " << fruPath;

  Object* obj = sensorTree->getObject(fruPath);
  if ((dynamic_cast<FRU*>(obj)) != nullptr) {
    deleteSubtree(sensorTree, obj);
  }
  else {
    LOG(ERROR) << "FRU " << fruPath << " does not exists";
    status = FALSE;
  }

  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", status));
}

void DBusSensorServiceInterface::methodCallBack(
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

  SensorObjectTree* sensorTree = static_cast<SensorObjectTree*>(arg);

  if (g_strcmp0(methodName, "addFRU") == 0) {
    addFRU(invocation, parameters, sensorTree, objectPath);
  }
  else if (g_strcmp0(methodName, "addSensors") == 0) {
    addSensors(invocation, parameters, sensorTree, objectPath);
  }
  else if (g_strcmp0(methodName, "resetTree") == 0) {
    resetTree(invocation, sensorTree, objectPath);
  }
  else if (g_strcmp0(methodName, "removeFRU") == 0) {
    removeFRU(invocation, parameters, sensorTree, objectPath);
  }
  else {
    // For remaining methods call DBusSensorTreeInterface::methodCallBack
    // Pass SensorService object (sensorTree->getObject(objectPath)) as arg
    DBusSensorTreeInterface::methodCallBack(connection,
                                            sender,
                                            objectPath,
                                            interfaceName,
                                            methodName,
                                            parameters,
                                            invocation,
                                            sensorTree->getObject(objectPath));
  }
}

} // namespace qin
} // namespace openbmc
