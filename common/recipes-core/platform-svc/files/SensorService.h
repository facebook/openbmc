/*
 * SensorService.h
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

namespace openbmc {
namespace qin {

/*
 * This class is implemented to communicate with SensorService via DBus
 */
class SensorService {
  private:
    std::string dbusName_;     //well known dbus name for SensorService
    std::string dbusPath_;     //path for SensorService object
    std::string dbusInteface_; //Interface implemented by SensorService
    GDBusProxy* proxy_;        // proxy to SensorService dbus object
    bool isAvailable_;         //Flag whether SensorService is available on dbus

  public:
    /*
     * Constructor
     */
    SensorService (const std::string & dbusName,
                   const std::string & dbusPath,
                   const std::string & dbusInteface);

    /*
     * Returns dbusName of SensorService
     */
    const std::string & getDBusName() const;

    /*
     * Returns dbusPath of SensorService
     */
    const std::string & getDBusPath() const;

    /*
     * Returns whether SensorService is available
     */
    bool isAvailable() const;

    /*
     * Sets availability of SensorService
     * Returns whether operation is successful or not
     */
    bool setIsAvailable(bool isAvailable);

    /*
     * Reset sensorTree at SensorService
     * Returns whether operation is successful or not
     */
    bool reset();

    /*
     * Add FRU at SensorService
     * Returns whether operation is successful or not
     */
    bool addFRU(const std::string & fruParentPath,
                const std::string & fruJson);

    /*
     * Add Sensors under FRU at SensorService
     * Returns whether operation is successful or not
     */
    bool addSensors(const std::string & fruPath,
                    const std::vector<std::string> & sensorJsonList);

    /*
     * Remove FRU and its subtree from SensorService
     * Returns whether operation is successful or not
     */
    bool removeFRU(std::string fruPath);
};

} // namespace qin
} // namespace openbmc
