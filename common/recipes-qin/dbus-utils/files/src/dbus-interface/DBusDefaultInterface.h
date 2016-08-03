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
#include <string>
#include <stdexcept>
#include <gio/gio.h>
#include "../DBusInterfaceBase.h"

namespace openbmc {
namespace qin {

/**
 * Default interface for DBus class. Will be used in DBus
 * class if no interface is specified. This interface only
 * provides the callback for ping. The xml string specifies
 * the method callbacks that are implemented in this interface class.
 * The specification in xml should meet the implementation.
 * All the method calls will be added into methodCallBack
 * function that can be registered with a dbus object.
 * Note that this is not the same as the interface in java.
 */
class DBusDefaultInterface: public DBusInterfaceBase {
  public:
    /**
     * All the subfunctions in the callback handler should comply
     * with what is specified in the xml.
     */
    static const char* xml;

    /**
     * Constructor to initialize the member variables
     * The initialization depends on the definition of the static variables
     * and functions such as xml and methodCallBack.
     *
     * @throw std::invalid_argument if xml cannot be parsed successfully.
     */
    DBusDefaultInterface();

    ~DBusDefaultInterface(){
      g_dbus_node_info_unref(info_);
    }

    /**
     * Send a timestamp message as a respond to ping.
     * Used as a test function
     *
     * @param invocation stands for the identity of the message
     */
    static void ping(GDBusMethodInvocation* invocation);

    /**
     * Handles the callback for managing dbus objects
     * The above functions should be invoked here with methodName specified.
     * Checkout g_dbus_connection_register_object in gio library for details.
     */
    static void methodCallBack (GDBusConnection*       connection,
                                const char*            sender,
                                const char*            objectPath,
                                const char*            name,
                                const char*            methodName,
                                GVariant*              parameters,
                                GDBusMethodInvocation* invocation,
                                gpointer               arg) {
      if (g_strcmp0(methodName, "Ping") == 0) {
        ping(invocation);
      }
    }
};

} // namespace qin
} // namespace openbmc
