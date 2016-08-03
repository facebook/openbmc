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
#include <glog/logging.h>
#include <gio/gio.h>
#include "../DBusInterfaceBase.h"

namespace openbmc {
namespace qin {

/**
 * DBus Object Interface that manages the generic Objects.
 */
class DBusObjectInterface: public DBusInterfaceBase {
  public:
    /**
     * Constructor to initialize the member variables
     * The initialization depends on the definition of the static variables
     * and functions such as xml and methodCallBack.
     */
    DBusObjectInterface();

    ~DBusObjectInterface();

    /**
     * All the subfunctions in the callback handler should comply
     * with what is specified in the xml.
     */
    static const char* xml;

    /**
     * Send a timestamp message as a respond to ping.
     * Used as a test function.
     *
     * @param invocation stands for the identity of the message
     */
    static void ping(GDBusMethodInvocation* invocation);

    /**
     * Get the names of the child objects of the specified object by
     * responding a glib array of strings.
     *
     * @param invocation stands for the identity of the message
     * @param arg is the pointer to the specified object
     */
    static void getObjectsByParent(GDBusMethodInvocation* invocation,
                                   gpointer               arg);

    /**
     * Get all the names of the attributes of the specified object in
     * a blig array of string.
     *
     * @param invocation stands for the identity of the message
     * @param arg is the pointer to the specified object
     */
    static void getAttrsByObject(GDBusMethodInvocation* invocation,
                                 gpointer               arg);

    /**
     * Get the Attribute value from the cache of Attribute instance.
     * Will not read from the file system or any other APIs.
     * Send GIO_ERROR_NOT_FOUND to DBus if the specified attribute
     * does not exist.
     *
     * @param invocation stands for the identity of the message
     * @param gvparam should be a GVariant containing a string
     *        of attr name
     * @param arg is the pointer to the specified object
     */
    static void getAttrValue(GDBusMethodInvocation* invocation,
                             GVariant*              gvparam,
                             gpointer               arg);

    /**
     * Read the Attribute value through the file system or other APIs if
     * available. Send GIO_ERROR_NOT_FOUND to DBus if the specified attribute
     * does not exist. Send GIO_ERROR_FAILED if the reading failed.
     *
     * @param invocation stands for the identity of the message
     * @param gvparam should be a GVariant containing a string of
     *        attr name
     * @param arg is the pointer to the specified object
     */
    static void readAttrValue(GDBusMethodInvocation* invocation,
                              GVariant*              gvparam,
                              gpointer               arg);

    /**
     * Set the Attribute value to the cache of Attribute instance.
     * Will not write the file system or any other APIs.
     * Send GIO_ERROR_NOT_FOUND to DBus if the specified attribute
     * does not exist.
     *
     * @param invocation stands for the identity of the message
     * @param gvparam should be a GVariant containing in order a string of
     *        attr name and a string of value to be set
     * @param arg is the pointer to the specified object
     */
    static void setAttrValue(GDBusMethodInvocation* invocation,
                             GVariant*              gvparam,
                             gpointer               arg);

    /**
     * Write the Attribute value through the file system or other APIs if
     * available. Send GIO_ERROR_NOT_FOUND to DBus if the specified attribute
     * does not exist. Send GIO_ERROR_FAILED if the writing failed.
     *
     * @param invocation stands for the identity of the message
     * @param gvparam should be a GVariant containing in order a string of
     *        attr name and a string of value to be set
     * @param arg is the pointer to the specified object
     */
    static void writeAttrValue(GDBusMethodInvocation* invocation,
                               GVariant*              gvparam,
                               gpointer               arg);

    /**
     * Dump the object into a string of JSON.
     *
     * @param invocation stands for the identity of the message
     * @param arg is the pointer to the specified object
     */
    static void dumpByObject(GDBusMethodInvocation* invocation,
                             gpointer               arg);

    /**
     * Dump the object recursively with the child objects into a
     * string of JSON.
     *
     * @param invocation stands for the identity of the message
     * @param arg is the pointer to the specified object
     */
    static void dumpRecursiveByObject(GDBusMethodInvocation* invocation,
                                      gpointer               arg);

    /**
     * Dump the whole object tree iteratively into a string of JSON.
     *
     * @param invocation stands for the identity of the message
     * @param arg is the pointer to the specified object
     */
    static void dumpTree(GDBusMethodInvocation* invocation,
                         gpointer               arg);

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
     * Helper function for sending an error to the DBus when the
     * attribute is not found with the specified name.
     *
     * @param invocation to pass the dbus error message
     * @param name of the attribute
     */
    static void errorAttrNotFound(GDBusMethodInvocation* invocation,
                                  const std::string      &name) {
      LOG(ERROR) << "Attribute name " << name;
      g_dbus_method_invocation_return_error(invocation,
                                            G_IO_ERROR,
                                            G_IO_ERROR_NOT_FOUND,
                                            "Attribute not found");
    }
};

} // namespace qin
} // namespace openbmc
