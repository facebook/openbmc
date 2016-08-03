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
#include <dbus-utils/DBusInterfaceBase.h>

namespace openbmc {
namespace qin {

/**
 * DBus Interface that manages the object-tree.
 */
class DBusObjectTreeInterface: public DBusInterfaceBase {
  public:
    /**
     * Constructor to initialize the member variables
     * The initialization depends on the definition of the static variables
     * and functions such as xml and methodCallBack.
     */
    DBusObjectTreeInterface();

    ~DBusObjectTreeInterface();

    /**
     * The dbus interface org.openbmc.Object for object management
     * All the subfunctions in the callback handler should comply
     * with what is specified in the xml.
     */
    static const char* xml;

    /**
     * Send a timestamp message as a respond to ping.
     * Used as a test function
     *
     * @param invocation stands for the identity of the message
     */
    static void ping(GDBusMethodInvocation* invocation);

    /**
     * List the child objects of the specified object by responding
     * an glib array of strings of the object names.
     *
     * @param invocation stands for the identity of the message
     * @param arg is the pointer to the specified object
     */
    static void getObjects(GDBusMethodInvocation* invocation,
                            gpointer               arg);

    /**
     * List all the attributes of the specified object.
     *
     * @param invocation stands for the identity of the message
     * @param arg is the pointer to the specified object
     */
    static void getAttrByObject(GDBusMethodInvocation* invocation,
                                 gpointer               arg);

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
                                gpointer               arg);
};

} // namespace qin
} // namespace openbmc
