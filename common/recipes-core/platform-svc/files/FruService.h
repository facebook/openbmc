/*
 * FruService.h
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
#include <string>
#include <gio/gio.h>
#include <vector>

/*
 * This class is implemented to communicate with FruService via DBus
 */
class FruService {
  private:
    std::string dbusName_;             //well known dbus name for FruService
    std::string dbusPath_;             //path for FruService object to push FruService
    std::string dbusInteface_;         //Interface implemented by FruService to push FruService
    GDBusProxy* proxy_;                // proxy to FruService dbus object
    bool isAvailable_;                 //Flag whether FruService is available on dbus

  public:
    /*
     * Constructor
     */
    FruService (std::string dbusName, std::string dbusPath, std::string dbusInteface);

    /*
     * Returns dbusName of FruService
     */
    std::string const& getDBusName() const;

    /*
     * Returns dbusPath of FruService
     */
    std::string const& getDBusPath() const;

    /*
     * Returns whether FruService is available
     */
    bool isAvailable() const;

    /*
     * Sets availability of FruService
     * Returns whether operation is successful or not
     */
    bool setIsAvailable(bool isAvailable);

    /*
     * Reset fruTree at FruService
     * Returns whether operation is successful or not
     */
    bool reset() const;

    /*
     * Add FRU at FruService
     * Returns whether operation is successful or not
     */
    bool addFRU(std::string fruParentPath, std::string fruJson) const;


    /*
     * Remove FRU and its subtree from FruService
     * Returns whether operation is successful or not
     */
    bool removeFRU(std::string fruPath) const;
};
