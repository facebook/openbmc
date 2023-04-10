/*
 *
 * Copyright 2022-present Facebook. All Rights Reserved.
 *
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

#include <exception>
#include <iostream>

#include <openbmc/misc-utils.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>

#include  "SysCpldSettings.hpp"

namespace sysCpld {
class CpldDevice {
public:
  
  CpldDevice(uint8_t fru, uint8_t bus, uint8_t addr);
  ~CpldDevice();

  bool isDevOpen() const;
  bool isAuthComplete();
  bool isBootFail();
  uint8_t readRegister(uint8_t offset);
private:
  bool openDev();
  void closeDev();

  int m_fd = -1;
  uint8_t m_fruid = 0;
  uint8_t m_bus = 0;
  uint8_t m_addr = 0;
 
};
} // end of namespace sysCpld