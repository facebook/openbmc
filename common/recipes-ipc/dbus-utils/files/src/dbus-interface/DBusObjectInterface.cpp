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
#include <stdexcept>
#include <system_error>
#include <glog/logging.h>
#include <gio/gio.h>
#include <object-tree/Object.h>
#include "DBusObjectInterface.h"

namespace openbmc {
namespace qin {

const char* DBusObjectInterface::xml =
  "<!DOCTYPE node PUBLIC"
  " \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
  " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
  "<node>"
  "  <interface name='org.openbmc.Object'>"
  "    <method name='ping'>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "    <method name='getObjectsByParent'>"
  "      <arg type='as' name='object get' direction='out'/>"
  "    </method>"
  "    <method name='getAttrsByObject'>"
  "      <arg type='as' name='attribute get' direction='out'/>"
  "    </method>"
  "    <method name='getAttrValue'>"
  "      <arg type='s' name='attribute name' direction='in'/>"
  "      <arg type='s' name='attribute value' direction='out'/>"
  "    </method>"
  "    <method name='readAttrValue'>"
  "      <arg type='s' name='attribute name' direction='in'/>"
  "      <arg type='s' name='attribute value' direction='out'/>"
  "    </method>"
  "    <method name='setAttrValue'>"
  "      <arg type='s' name='attribute name' direction='in'/>"
  "      <arg type='s' name='attribute value' direction='in'/>"
  "    </method>"
  "    <method name='writeAttrValue'>"
  "      <arg type='s' name='attribute name' direction='in'/>"
  "      <arg type='s' name='attribute value' direction='in'/>"
  "    </method>"
  "    <method name='dumpByObject'>"
  "      <arg type='s' name='json string' direction='out'/>"
  "    </method>"
  "    <method name='dumpRecursiveByObject'>"
  "      <arg type='s' name='json string' direction='out'/>"
  "    </method>"
  "    <method name='dumpTree'>"
  "      <arg type='s' name='json string' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

DBusObjectInterface::DBusObjectInterface() {
  info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
  if (info_ == nullptr) {
    LOG(ERROR) << "xml parsing for dbus interface failed";
    throw std::invalid_argument("XML parsing failed");
  }
  no_ = 0;
  name_ = info_->interfaces[no_]->name;
  vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
}

DBusObjectInterface::~DBusObjectInterface() {
  g_dbus_node_info_unref(info_);
}

void DBusObjectInterface::ping(GDBusMethodInvocation* invocation) {
  std::time_t t = std::time(nullptr);
  char* tchars = std::asctime(std::localtime(&t));
  tchars[strlen(tchars) - 1] = '\0';
  std::string response = tchars;
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new ("(s)", response.c_str()));
}


void DBusObjectInterface::getObjectsByParent(GDBusMethodInvocation* invocation,
                                             gpointer               arg) {
  GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
  Object* obj = static_cast<Object*>(arg);
  LOG(INFO) << "Listing array of child names of object \"" << obj->getName()
    << "\"";
  for (auto &it : obj->getChildMap()) {
    g_variant_builder_add(builder, "s", it.first.c_str());
  }
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(as)", builder));
  g_variant_builder_unref(builder);
}


void DBusObjectInterface::getAttrsByObject(
                                      GDBusMethodInvocation* invocation,
                                      gpointer               arg) {
  GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("as"));
  Object* obj = static_cast<Object*>(arg);
  LOG(INFO) << "Listing an array of attribute names for object \""
    << obj->getName() << "\"";
  for (auto &it : obj->getAttrMap()) {
    g_variant_builder_add(builder, "s", it.first.c_str());
  }
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(as)", builder));
  g_variant_builder_unref(builder);
}

void DBusObjectInterface::getAttrValue(GDBusMethodInvocation* invocation,
                                       GVariant*              gvparam,
                                       gpointer               arg) {
  const char* name;
  g_variant_get(gvparam, "(&s)", &name);
  LOG(INFO) << "Getting value of Attribute \"" << name << "\"";

  Object* obj = static_cast<Object*>(arg);
  Attribute* attr = obj->getAttribute(name);
  if (attr == nullptr) {
    errorAttrNotFound(invocation, name);
    return;
  }
  g_dbus_method_invocation_return_value(
      invocation, g_variant_new("(s)", attr->getValue().c_str()));
}

void DBusObjectInterface::readAttrValue(GDBusMethodInvocation* invocation,
                                        GVariant*              gvparam,
                                        gpointer               arg) {
  const char* name;
  g_variant_get(gvparam, "(&s)", &name);
  LOG(INFO) << "Reading value of Attribute \"" << name << "\"";

  Object* obj = static_cast<Object*>(arg);
  Attribute* attr = obj->getAttribute(name);
  if (attr == nullptr) {
    errorAttrNotFound(invocation, name);
    return;
  }

  std::string value;
  try {
    value = obj->readAttrValue(name);
  } catch (const std::system_error &e) {
    g_dbus_method_invocation_return_error(invocation,
                                          G_IO_ERROR,
                                          G_IO_ERROR_FAILED,
                                          e.what());
  }

  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", value.c_str()));
}

void DBusObjectInterface::setAttrValue(GDBusMethodInvocation* invocation,
                                       GVariant*              gvparam,
                                       gpointer               arg) {
  const char* name;
  const char* value;
  g_variant_get(gvparam, "(&s&s)", &name, &value);
  LOG(INFO) << "Setting value \"" << value << "\" to Attribute \"" << name
    << "\"";

  Object* obj = static_cast<Object*>(arg);
  Attribute* attr = obj->getAttribute(name);
  if (attr == nullptr) {
    errorAttrNotFound(invocation, name);
    return;
  }
  attr->setValue(value);

  LOG(INFO) << "Setting Attribute value successful";
  g_dbus_method_invocation_return_value(invocation, nullptr);
}

void DBusObjectInterface::writeAttrValue(GDBusMethodInvocation* invocation,
                                         GVariant*              gvparam,
                                         gpointer               arg) {
  const char* value;
  const char* name;
  g_variant_get(gvparam, "(&s&s)", &name, &value);
  LOG(INFO) << "Writing value \"" << value << "\" to Attribute \"" << name
    << "\"";

  Object* obj = static_cast<Object*>(arg);
  Attribute* attr = obj->getAttribute(name);
  if (attr == nullptr) {
    errorAttrNotFound(invocation, name);
    return;
  }

  try {
    obj->writeAttrValue(name, value);
  } catch (const std::system_error &e) {
    g_dbus_method_invocation_return_error(invocation,
                                          G_IO_ERROR,
                                          G_IO_ERROR_FAILED,
                                          e.what());
    return;
  }

  LOG(INFO) << "Writing Attribute value successful";
  g_dbus_method_invocation_return_value(invocation, nullptr);
}

void DBusObjectInterface::dumpByObject(GDBusMethodInvocation* invocation,
                                       gpointer               arg) {
  Object* obj = static_cast<Object*>(arg);
  LOG(INFO) << "Dumpping the object \"" << obj->getName()
    << "\" into json string";
  const std::string objDump = obj->dumpToJson().dump();
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", objDump.c_str()));
}

void DBusObjectInterface::dumpRecursiveByObject(
                                       GDBusMethodInvocation* invocation,
                                       gpointer               arg) {
  Object* obj = static_cast<Object*>(arg);
  LOG(INFO) << "Dumpping the object \"" << obj->getName()
    << "\" recursively into json string";
  const std::string objDump = obj->dumpToJsonRecursive().dump();
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", objDump.c_str()));
}

void DBusObjectInterface::dumpTree(GDBusMethodInvocation* invocation,
                                   gpointer               arg) {
  Object* root = static_cast<Object*>(arg);
  while (root->getParent() != nullptr) {
    root = root->getParent();
  }
  LOG(INFO) << "Dumpping the object tree starting at root " << root->getName()
    << " into json string";
  const std::string treeDump = root->dumpToJsonRecursive().dump();
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(s)", treeDump.c_str()));
}

void DBusObjectInterface::methodCallBack(
                          GDBusConnection*       connection,
                          const char*            sender,
                          const char*            objectPath,
                          const char*            interfaceName,
                          const char*            methodName,
                          GVariant*              parameters,
                          GDBusMethodInvocation* invocation,
                          gpointer               arg) {
  // arg should be a pointer to Object in object-tree
  // Cannot check the type here because it is a callback function
  // It's the user's responsibility to make sure arg is Object*.
  DCHECK(arg != nullptr) << "Empty object passed to callback";

  if (g_strcmp0(methodName, "ping") == 0) {
    ping(invocation);
  } else if (g_strcmp0(methodName, "getObjectsByParent") == 0) {
    getObjectsByParent(invocation, arg);
  } else if (g_strcmp0(methodName, "getAttrsByObject") == 0) {
    getAttrsByObject(invocation, arg);
  } else if (g_strcmp0(methodName, "getAttrValue") == 0) {
    getAttrValue(invocation, parameters, arg);
  } else if (g_strcmp0(methodName, "readAttrValue") == 0) {
    readAttrValue(invocation, parameters, arg);
  } else if (g_strcmp0(methodName, "setAttrValue") == 0) {
    setAttrValue(invocation, parameters, arg);
  } else if (g_strcmp0(methodName, "writeAttrValue") == 0) {
    writeAttrValue(invocation, parameters, arg);
  } else if (g_strcmp0(methodName, "dumpByObject") == 0) {
    dumpByObject(invocation, arg);
  } else if (g_strcmp0(methodName, "dumpRecursiveByObject") == 0) {
    dumpRecursiveByObject(invocation, arg);
  } else if (g_strcmp0(methodName, "dumpTree") == 0) {
    dumpTree(invocation, arg);
  }
}

} // namespace qin
} // namespace openbmc
