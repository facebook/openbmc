/*
 * Copyright 2019-present Facebook. All Rights Reserved.
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
#ifndef _SENSORLIST_HPP_
#define _SENSORLIST_HPP_
#include "sensorchip.hpp"

// Collection of sensor-chips. Provides efficient look-up of sensor chips.
class SensorList : public std::map<std::string, std::unique_ptr<SensorChip>> {
  protected:
    // Allocates a chip object
    virtual std::unique_ptr<SensorChip> make_chip(const sensors_chip_name *chip, const std::string &name);
  public:
    SensorList(const char *conf_file = nullptr);
    virtual ~SensorList();

    // enumerate all sensor chips.
    void enumerate();
};

#endif
