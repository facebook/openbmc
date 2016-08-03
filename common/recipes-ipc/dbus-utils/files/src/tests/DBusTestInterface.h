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

#pragma once
#include <glog/logging.h>
#include <gio/gio.h>
#include "../DBusInterfaceBase.h"

namespace openbmc {
namespace qin {

/**
 * Test interface for DBus. The xml string specifies the method
 * callbacks that are implemented in this interface class.
 * The specification in xml should meet the implementation.
 * All the method calls will be added into methodCallBack
 * function that can be registered with a dbus object.
 * Note that this is not the same as the interface in java.
 */
class DBusTestInterface: public DBusInterfaceBase {
  public:
    const char* xml =
      "<!DOCTYPE node PUBLIC"
      " \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" "
      " \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">"
      "<node>"
      "  <interface name='org.openbmc.TestInterface'>"
      "  </interface>"
      "</node>";

    DBusTestInterface() {
      info_ = g_dbus_node_info_new_for_xml(xml, nullptr);
      if (info_ == nullptr)
        LOG(FATAL) << "xml parsing for dbus interface failed";
      no_ = 0;
      name_ = info_->interfaces[no_]->name;
      vtable_ = {methodCallBack, nullptr, nullptr, nullptr};
    }

    ~DBusTestInterface() {
      g_dbus_node_info_unref(info_);
    }

    static void methodCallBack (GDBusConnection*       connection,
                                const char*            sender,
                                const char*            objectPath,
                                const char*            name,
                                const char*            methodName,
                                GVariant*              parameters,
                                GDBusMethodInvocation* invocation,
                                gpointer               arg) {}
};

} // namespace qin
} // namespace openbmc
