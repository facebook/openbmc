#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
from boardroutes import board_routes


DEFAULT_TIMEOUT_SEC = 20

common_routes = [
    "/api",
    "/api/sys",
    "/api/sys/mb",
    "/api/sys/mb/fruid",
    "/api/sys/bmc",
    "/api/sys/server",
    "/api/sys/sensors",
    "/api/sys/gpios",
    "/api/sys/fc_present",
    "/api/sys/psu_update",
    "/api/sys/fscd_sensor_data",
    "/api/sys/modbus",
    "/api/sys/modbus/cmd",
    "/api/sys/modbus_registers",
    "/api/sys/modbus/registers",
    "/api/sys/modbus/devices",
    "/api/sys/modbus/read",
]


all_routes = common_routes + board_routes
