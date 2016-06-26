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

namespace openbmc {
namespace ipc {

/**
 * Attribute for object
 */
class Attribute {
  public:
    // RO: read only; WO: write only; RW: read write
    enum Modes {RO, WO, RW};

  protected:
    std::string name_;
    std::string type_{"Generic"};
    std::string value_{""};
    Modes       modes_{RO};

  public:
    /**
     * Constructor to set the name and type of the attribute
     */
    Attribute(const std::string &name) {
      name_  = name;
    }

    Attribute(const std::string &name, const std::string &type) {
      name_  = name;
      type_  = type;
    }

    const std::string& getName() const {
      return name_;
    }

    const std::string& getType() const {
      return type_;
    }

    const std::string& getValue() const {
      return value_;
    }

    Modes getModes() const {
      return modes_;
    }

    void setValue(const std::string &value) {
      value_ = value;
    }

    /**
     *  Set the modes of the attribute. glog error if
     *  the input is NULL or is not "RO" || "WO" || "RW"
     *
     *  @param modes is either RO, WO, or RW
     */
    void setModes(Modes modes) {
      modes_ = modes;
    }

    /**
     * Return if the attribute is readable.
     * If it is readable, the value can be read through
     * sysfs or I2c.
     */
    bool isReadable() const {
      return modes_ == RO || modes_ == RW;
    }

    /**
     * Return if the attribute is writable.
     * If it is writable, the value can be written through
     * sysfs or I2c.
     */
    bool isWritable() const {
      return modes_ == WO || modes_ == RW;
    }
};

} // namespace openbmc
} // namespace ipc
