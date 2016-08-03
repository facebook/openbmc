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
#include <gio/gio.h>
#include <string>

namespace openbmc {
namespace qin {

/**
 * This is a DBus method call interface base class to be inherited by
 * interface implementing classes.
 */
class DBusInterfaceBase {
  protected:
    std::string           name_;   // interface name specified in the xml
    GDBusNodeInfo*        info_;   // introspection data from xml
    unsigned int          no_{0};  // interface no. in xml (0 by default)
    GDBusInterfaceVTable  vtable_; // contains callbacks: method,
                                   // get_property, and set_property

  public:
    /**
     * Virtual destructor holds the inheriting implementations
     * reponsible for specifying their deconstructors.
     */
    virtual ~DBusInterfaceBase() {};

    const std::string& getName() const {
      return name_;
    }

    GDBusNodeInfo* getInfo() {
      return info_;
    }

    unsigned int getNo() {
      return no_;
    }

    GDBusInterfaceVTable* getVtable() {
      return &vtable_;
    }
};

} // namespace qin
} // namespace openbmc
