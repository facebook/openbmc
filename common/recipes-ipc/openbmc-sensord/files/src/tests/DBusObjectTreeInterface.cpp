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
namespace ipc {

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
  "    <method name='getAllAttributes'>"
  "      <arg type='a{sv}' name='attribute get' direction='out'/>"
  "    </method>"
  "    <method name='getAttributes'>"
  "      <arg type='s' name='attribute type' direction='in'/>"
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
  const Object::ChildMap* childMap = obj->getChildMap();
  LOG(INFO) << "geting array of child names of object " << obj->getName();
  for (auto it = childMap->begin(); it != childMap->end(); it++) {
    g_variant_builder_add(builder, "s", it->first.c_str());
  }
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(as)", builder));
  g_variant_builder_unref(builder);
}


void DBusObjectTreeInterface::getAllAttributes(
                                      GDBusMethodInvocation* invocation,
                                      gpointer               arg) {
  if (arg == nullptr) return;

  GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
  Object* obj = static_cast<Object*>(arg);
  const Object::TypeMap* typeMap = obj->getTypeMap();
  LOG(INFO) << "geting array of attribute types and names of object "
    << obj->getName();
  for (auto i = typeMap->begin(); i != typeMap->end(); i++) {
    GVariantBuilder *subBuilder = g_variant_builder_new(G_VARIANT_TYPE("as"));
    const Object::AttrMap* attrMap = i->second.get();
    // for each type, get the attributes
    for (auto j = attrMap->begin(); j != attrMap->end(); j++) {
      g_variant_builder_add(subBuilder, "s",
                            j->second.get()->getName().c_str());
    }
    g_variant_builder_add(builder, "{sv}", i->first.c_str(),
                          g_variant_new("(as)", subBuilder));
  }
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(a{sv})", builder));
  g_variant_builder_unref(builder);
}

void DBusObjectTreeInterface::getAttributes(
                                      GDBusMethodInvocation* invocation,
                                      GVariant*              gvtype,
                                      gpointer               arg) {
  if (arg == nullptr) return;
  const char* type;
  g_variant_get(gvtype, "(&s)", &type);

  GVariantBuilder *builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
  Object* obj = static_cast<Object*>(arg);
  const Object::AttrMap* attrMap = obj->getAttrMap(type);
  if (attrMap != nullptr) {
    LOG(INFO) << "geting array of attribute names with type "
      << type << " in object " << obj->getName();
    for (auto j = attrMap->begin(); j != attrMap->end(); j++) {
      g_variant_builder_add(builder, "s", j->first.c_str());
    }
  } else {
    LOG(WARNING) << "Specified attribute type " << type << " does not "
      << "exist in object " << obj->getName();
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

  if (g_strcmp0(methodName, "Ping") == 0)
    ping(invocation);
  else if (g_strcmp0(methodName, "getObjects") == 0)
    getObjects(invocation, arg);
  else if (g_strcmp0(methodName, "getAllAttributes") == 0)
    getAllAttributes(invocation, arg);
  else if (g_strcmp0(methodName, "getAttributes") == 0)
    getAttributes(invocation, parameters, arg);
}

} // namespace ipc
} // namespace openbmc
