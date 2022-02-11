/*
 * DBusPlatformSvcInterface.cpp
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
#include <object-tree/Object.h>
#include "DBusPlatformSvcInterface.h"
#include "PlatformObjectTree.h"

namespace openbmc {
namespace qin {

static const char* xml =
"<!DOCTYPE node PUBLIC"
" \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
" \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
"<node>"
"  <interface name='org.openbmc.PlatformService'>"
"    <method name='getAccessInformation'>"
"      <arg type='s' name='type' direction='out'/>"
"      <arg type='s' name='dbusName' direction='out'/>"
"      <arg type='s' name='dbusPath' direction='out'/>"
"    </method>"
"  </interface>"
"</node>";

DBusPlatformSvcInterface::DBusPlatformSvcInterface() {
  info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
  if (info_ == nullptr) {
    LOG(ERROR) << "xml parsing for dbus interface failed";
    throw std::invalid_argument("XML parsing failed");
  }
  no_ = 0;
  name_ = info_->interfaces[no_]->name;
  vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
}

DBusPlatformSvcInterface::~DBusPlatformSvcInterface() {
  g_dbus_node_info_unref(info_);
}

void DBusPlatformSvcInterface::getAccessInformation(
                                         GDBusConnection*       connection,
                                         const char*            objectPath,
                                         GDBusMethodInvocation* invocation,
                                         gpointer               arg) {
  LOG(INFO) << "getAccessInformation : " << objectPath;
  PlatformObjectTree* platformObjectTree =
              static_cast<PlatformObjectTree*>(arg);

  Object* obj = platformObjectTree->getObject(std::string(objectPath));

  if (dynamic_cast<Sensor*>(obj) != nullptr) {
    //obj of type Sensor
    //dbusPath of obj is Base dbus path for Sensor Service
    //                   + (objectpath - platform service base path)
    std::string dbusPath =
              platformObjectTree->getSensorService()->getDBusPath() +
              std::string(objectPath).erase(0,
                    platformObjectTree->getPlatformServiceBasePath().length());

    g_dbus_method_invocation_return_value(
              invocation,
              g_variant_new("(sss)",
              "Sensor",
              platformObjectTree->getSensorService()->getDBusName().c_str(),
              dbusPath.c_str()));
  }
  else if (dynamic_cast<FRU*>(obj) != nullptr) {
    //obj of type FRU
    //dbusPath of obj is Base dbus path for FRU Service
    //                   + (objectpath - platform service base path)
    std::string dbusPath =
              platformObjectTree->getFruService()->getDBusPath() +
              std::string(objectPath).erase(0,
                    platformObjectTree->getPlatformServiceBasePath().length());

    g_dbus_method_invocation_return_value(
              invocation,
              g_variant_new("(sss)",
              "FRU",
              platformObjectTree->getFruService()->getDBusName().c_str(),
              dbusPath.c_str()));
  }
  else {
    g_dbus_method_invocation_return_value(invocation,
              g_variant_new("(sss)",
              "PlatformService",
              ((DBus*)platformObjectTree->getIpc())->getName().c_str(),
              objectPath));
  }
}

void DBusPlatformSvcInterface::methodCallBack(
                               GDBusConnection*       connection,
                               const char*            sender,
                               const char*            objectPath,
                               const char*            interfaceName,
                               const char*            methodName,
                               GVariant*              parameters,
                               GDBusMethodInvocation* invocation,
                               gpointer               arg) {
  // arg should be a pointer to PlatformObjectTree
  DCHECK(arg != nullptr) << "Empty object passed to callback";

  if (g_strcmp0(methodName, "getAccessInformation") == 0) {
    getAccessInformation(connection, objectPath, invocation, arg);
  }
}

} // namespace qin
} // namespace openbmc
