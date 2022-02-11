/*
 * FRU.cpp : Platfrom specific implementation
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

#include "FRU.h"
#include <openbmc/sensorsvcpal.h>

namespace openbmc {
namespace qin {

bool FRU::isFruOff() {
  int ret;
  uint8_t status;
  ret = pal_get_server_power(fruId_, &status);
  if (ret) {
    return false;
  }

  if (status == SERVER_POWER_OFF) {
    poweronFlag_ = 0;
    return true;
  } else {
    return false;
  }
}

} // namespace qin
} // namespace openbmc
