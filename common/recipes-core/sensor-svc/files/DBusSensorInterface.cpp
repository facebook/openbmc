/*
 * DBusSensorInterface.cpp
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
#include "DBusSensorInterface.h"
#include "Sensor.h"

namespace openbmc {
namespace qin {

static const char* xml =
"<!DOCTYPE node PUBLIC"
" \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
" \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
"<node>"
"  <interface name='org.openbmc.SensorObject'>"
"    <method name='getSensorId'>"
"      <arg type='y' name='value' direction='out'/>"
"    </method>"
"    <method name='sensorRead'>"
"      <arg type='i' name='readStatus' direction='out'/>"
"      <arg type='d' name='value' direction='out'/>"
"    </method>"
"    <method name='sensorRawRead'>"
"      <arg type='i' name='readStatus' direction='out'/>"
"      <arg type='d' name='value' direction='out'/>"
"    </method>"
"    <method name='getSensorObject'>"
"      <arg type='s' name='name' direction='out'/>"
"      <arg type='y' name='id' direction='out'/>"
"      <arg type='i' name='readStatus' direction='out'/>"
"      <arg type='d' name='value' direction='out'/>"
"      <arg type='s' name='unit' direction='out'/>"
"    </method>"
"  </interface>"
"</node>";

DBusSensorInterface::DBusSensorInterface() {
  info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
  if (info_ == nullptr) {
    LOG(ERROR) << "xml parsing for dbus interface failed";
    throw std::invalid_argument("XML parsing failed");
  }
  no_ = 0;
  name_ = info_->interfaces[no_]->name;
  vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
}

DBusSensorInterface::~DBusSensorInterface() {
  g_dbus_node_info_unref(info_);
}

void DBusSensorInterface::sensorRead(GDBusMethodInvocation* invocation,
                                     gpointer               arg) {
  Sensor* obj = static_cast<Sensor*>(arg);
  LOG(INFO) << "sensorRead of " << obj->getName();
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(id)",
                                        obj->getLastReadStatus(),
                                        obj->getValue()));
}

void DBusSensorInterface::sensorRawRead(GDBusMethodInvocation* invocation,
                                        gpointer               arg) {
  Sensor* obj = static_cast<Sensor*>(arg);
  LOG(INFO) << "sensorRawRead of " << obj->getName();
  obj->sensorRawRead();
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(id)",
                                        obj->getLastReadStatus(),
                                        obj->getValue()));
}

void DBusSensorInterface::getSensorObject(GDBusMethodInvocation* invocation,
                                          gpointer               arg) {
  Sensor* obj = static_cast<Sensor*>(arg);
  LOG(INFO) << "getSensorObject of " << obj->getName();
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(syids)",
                                        obj->getName().c_str(),
                                        obj->getId(),
                                        obj->getLastReadStatus(),
                                        obj->getValue(),
                                        obj->getUnit().c_str()));
}

void DBusSensorInterface::getSensorId(GDBusMethodInvocation* invocation,
                                      gpointer               arg) {
  Sensor* obj = static_cast<Sensor*>(arg);
  LOG(INFO) << "sensorRead of " << obj->getName();
  g_dbus_method_invocation_return_value(invocation,
                                        g_variant_new("(y)",
                                        obj->getId(),
                                        obj->getValue()));
}

void DBusSensorInterface::methodCallBack(
                          GDBusConnection*       connection,
                          const char*            sender,
                          const char*            objectPath,
                          const char*            interfaceName,
                          const char*            methodName,
                          GVariant*              parameters,
                          GDBusMethodInvocation* invocation,
                          gpointer               arg) {
  // arg should be a pointer to Sensor
  DCHECK(arg != nullptr) << "Empty object passed to callback";

  if (g_strcmp0(methodName, "sensorRead") == 0) {
    sensorRead(invocation, arg);
  }
  else if (g_strcmp0(methodName, "sensorRawRead") == 0) {
    sensorRawRead(invocation, arg);
  }
  else if (g_strcmp0(methodName, "getSensorObject") == 0) {
    getSensorObject(invocation, arg);
  }
  else if (g_strcmp0(methodName, "getSensorId") == 0) {
    getSensorId(invocation, arg);
  }
}

} // namespace qin
} // namespace openbmc
