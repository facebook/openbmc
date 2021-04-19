/*
 * DBusFruServiceInterface.h: FruService in FruObjectTree registers on dbus using this interface
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
#include "FruObjectTree.h"

namespace openbmc {
namespace qin {

class DBusFruServiceInterface: public DBusInterfaceBase {
  public:
    /**
     * Constructor to initialize the member variables
     * The initialization depends on the definition of the static variables
     * and functions such as xml and methodCallBack.
     */
    DBusFruServiceInterface();

    /**
     * Destructor to deallocate memory info_
     */
    ~DBusFruServiceInterface();

    /**
     * Handles the callback by matching the method names in the DBus message
     * to the functions. The above callbacks should be invoked here with
     * method name specified. Checkout g_dbus_connection_register_object in
     * gio library for details.
     */
    static void methodCallBack(GDBusConnection*       connection,
                               const char*            sender,
                               const char*            objectPath,
                               const char*            interfaceName,
                               const char*            methodName,
                               GVariant*              parameters,
                               GDBusMethodInvocation* invocation,
                               gpointer               arg);

  private:
    /**
     * Callback for addFRU method, adds fru under specified path
     */
    static void addFRU(GDBusMethodInvocation* invocation,
                       GVariant*              parameters,
                       FruObjectTree*         fruTree,
                       const char*            objectPath);

    /**
     * Callback for resetTree method, deletes fruTree under FruService
     */
    static void resetTree(GDBusMethodInvocation* invocation,
                          FruObjectTree*         fruTree,
                          const char*            objectPath);

    /**
     * Callback for removeFRU method, removes specified FRU and its children
     */
    static void removeFRU(GDBusMethodInvocation* invocation,
                          GVariant*              parameters,
                          FruObjectTree*         fruTree,
                          const char*            objectPath);
};

} // namespace qin
} // namespace openbmc
