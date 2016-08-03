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
#include <glog/logging.h>
#include <gio/gio.h>
#include "DBusDefaultInterface.h"

namespace openbmc {
namespace qin {

const char* DBusDefaultInterface::xml =
  "<!DOCTYPE node PUBLIC"
  " \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
  " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
  "<node>"
  "  <interface name='org.openbmc.DefaultInterface'>"
  "    <method name='Ping'>"
  "      <arg type='s' name='response' direction='out'/>"
  "    </method>"
  "  </interface>"
  "</node>";

DBusDefaultInterface::DBusDefaultInterface() {
  info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
  if (info_ == nullptr) {
    LOG(ERROR) << "xml parsing for dbus interface failed";
    throw std::invalid_argument("XML parsing failed");
  }
  no_ = 0;
  name_ = info_->interfaces[no_]->name;
  vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
}

void DBusDefaultInterface::ping(GDBusMethodInvocation* invocation) {
  std::time_t t = std::time(nullptr);
  char* tchars = std::asctime(std::localtime(&t));
  tchars[strlen(tchars) - 1] = '\0';
  std::string response = "Ping received at " + std::string(tchars);
  g_dbus_method_invocation_return_value(
    invocation,
    g_variant_new ("(s)", response.c_str()));
}

} // namespace qin
} // namespace openbmc
