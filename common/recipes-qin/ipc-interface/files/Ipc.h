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
#include <functional>

namespace openbmc {
namespace qin {

/**
 * Ipc class provides a general interface for polymorphism over
 * interprocess communication techniques such as file systems
 * and DBus.
 */
class Ipc {
  public:
    typedef void onConnFuncType(void);

    virtual ~Ipc() {}

    /**
     * Register for the Ipc connection. Throw errors if the connection
     * failed.
     */
    virtual void registerConnection() = 0;

    /**
     * Unregister for the Ipc connection. Throw errors properly.
     */
    virtual void unregisterConnection() = 0;

    /**
     * Register the object path on the connection. Throw errors if
     * the object cannot be registered.
     *
     * @param path of the object
     * @param userData to be associated with the object
     * @throw std::invalid_argument if path is illegal
     */
    virtual void registerObject(const std::string &path,
                                void*             userData) = 0;

    /**
     * Unregister the dbus object with all the associated object args.
     *
     * @param path of the object
     */
    virtual void unregisterObject(const std::string &path) = 0;

    /**
     * Check if the object path matches the rule of the Ipc
     */
    virtual bool isPathAllowed(const std::string &path) const = 0;

    /**
     * The get function that forms a path from the given parentPath
     * and name.
     *
     * @param parentPath is path to parent of object to create path with
     * @param name is the name of the object to create path with
     * @return new path
     */
    virtual const std::string getPath(const std::string &parentPath,
                                      const std::string &name) const = 0;

    /**
     * Callback on connection acquired for the user application to set
     */
    std::function<onConnFuncType> onConnAcquired;

    /**
     * Callback on connection lost for the user application to set
     */
    std::function<onConnFuncType> onConnLost;
};
} // namesapce ipc
} // namespace openbmc
