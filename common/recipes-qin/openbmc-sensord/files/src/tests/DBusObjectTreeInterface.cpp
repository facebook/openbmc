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

#include <ctime>
#include <string>
#include <string.h>
#include <gio/gio.h>
#include <object-tree/Object.h>
#include "DBusObjectTreeInterface.h"

namespace openbmc {
namespace qin {

const char* DBusObjectTreeInterface::xml =
  "<!DOCTYPE node PUBLIC"
  " \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
  " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
  "<node>"
  "  <interface name='org.openbmc.ObjectTree'>"
  "    <method name='Ping'>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "    <method name='getObjects'>"
  "      <arg type='as' name='object get' direction='out'/>"
  "    </method>"
  "    <method name='getAttrByObject'>"
  "      <arg type='as' name='attribute get' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";


DBusObjectTreeInterface::DBusObjectTreeInterface() {
  info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
  if (info_ == nullptr) {
    LOG(ERROR) << "xml parsing for dbus interface failed";
    throw std::invalid_argument("Error parsing XML");
  }
  no_ = 0;
  name_ = info_->interfaces[no_]->name;
  vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
}

DBusObjectTreeInterface::~DBusObjectTreeInterface() {
  g_dbus_node_info_unref(info_);
}

void DBusObjectTreeInterface::ping(GDBusMethodInvocation* invocation) {
  char* response;
  std::time_t t = std::time(nullptr);
  char* tchars = std::asctime(std::localtime(&t));
  tchars[strlen(tchars) - 1] = '\0';
  response = g_strdup_printf("Ping received at '%s'", tchars);
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new ("(s)", response));
  g_free(response);
}


void DBusObjectTreeInterface::getObjects(GDBusMethodInvocation* invocation,
                                         gpointer               arg) {
  if (arg == nullptr) return;

  GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
  Object* obj = static_cast<Object*>(arg);
  const Object::ChildMap* childMap = &(obj->getChildMap());
  LOG(INFO) << "Listing array of child names of object " << obj->getName();
  for (auto it = childMap->begin(); it != childMap->end(); it++) {
    g_variant_builder_add(builder, "s", it->first.c_str());
  }
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(as)", builder));
  g_variant_builder_unref(builder);
}

void DBusObjectTreeInterface::getAttrByObject(
                                      GDBusMethodInvocation* invocation,
                                      gpointer               arg) {
  if (arg == nullptr) return;

  GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
  Object* obj = static_cast<Object*>(arg);
  LOG(INFO) << "geting array of attribute types and names of object "
    << obj->getName();
  for (auto &it : obj->getAttrMap()) {
    g_variant_builder_add(builder, "s", it.first.c_str());
  }
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(as)", builder));
  g_variant_builder_unref(builder);
}

void DBusObjectTreeInterface::methodCallBack(
                                       GDBusConnection*       connection,
                                       const char*            sender,
                                       const char*            objectPath,
                                       const char*            interfaceName,
                                       const char*            methodName,
                                       GVariant*              parameters,
                                       GDBusMethodInvocation* invocation,
                                       gpointer               arg) {

  if (g_strcmp0(methodName, "Ping") == 0) {
    ping(invocation);
  } else if (g_strcmp0(methodName, "getObjects") == 0) {
    getObjects(invocation, arg);
  } else if (g_strcmp0(methodName, "getAttrByObject") == 0) {
    getAttrByObject(invocation, arg);
  }
}

} // namespace qin
} // namespace openbmc
