/*
 * SensorAccessINA230.cpp : Platfrom specific implementation
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

#include <openbmc/sensorsvcpal.h>
#include "SensorAccessINA230.h"
#include "Sensor.h"

namespace openbmc {
namespace qin {

void SensorAccessINA230::rawRead(Sensor* s, float *value) {
  readResult_ =  (ReadResult)pal_read_INA230(s->getId(),
                                             value,
                                             s->getFru()->getPoweronFlag());
}

} // namespace qin
} // namespace openbmc
