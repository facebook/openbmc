/*
 * DBusFruInterface.h: FRUs in FruTree registers on dbus using this interface
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

#pragma once
#include <dbus-utils/DBusInterfaceBase.h>

namespace openbmc {
namespace qin {

class DBusFruInterface: public DBusInterfaceBase {
  public:
    /**
     * Constructor to initialize the member variables
     * The initialization depends on the definition of the static variables
     * and functions such as xml and methodCallBack.
     */
    DBusFruInterface();

    /**
     * Destructor to deallocate memory info_
     */
    ~DBusFruInterface();

    /**
     * Handles the callback by matching the method names in the DBus message
     * to the functions. The above callbacks should be invoked here with
     * method name specified. Checkout g_dbus_connection_register_object in
     * gio library for details.
     */
    static void methodCallBack(GDBusConnection*       connection,
                               const char*            sender,
                               const char*            objectPath,
                               const char*            name,
                               const char*            methodName,
                               GVariant*              parameters,
                               GDBusMethodInvocation* invocation,
                               gpointer               arg);

  private:
    /**
     * Callback for getFruIdInfo method, returns FruIdInfo over dbus
     */
    static void getFruIdInfo(GDBusMethodInvocation* invocation,
                             gpointer               arg);

    /**
     * Callback for fruIdDumpBinaryData method,
     * dumps fruID binary data at destFilePath
     */
    static void fruIdDumpBinaryData(GVariant*              parameters,
                                    GDBusMethodInvocation* invocation,
                                    gpointer               arg);

    /**
     * Callback for fruIdWriteBinaryData method,
     * reads fruID binary data from binFilePath and updates fruID information
     */
    static void fruIdWriteBinaryData(GVariant*              parameters,
                                     GDBusMethodInvocation* invocation,
                                     gpointer               arg);
};
} // namespace qin
} // namespace openbmc
