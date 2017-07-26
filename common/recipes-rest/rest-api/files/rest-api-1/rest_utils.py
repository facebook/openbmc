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
from boardroutes import *
common_routes = [
    '/api',
    '/api/sys',
    '/api/sys/mb',
    '/api/sys/mb/fruid',
    '/api/sys/bmc',
    '/api/sys/server',
    '/api/sys/sensors',
    '/api/sys/gpios',
    '/api/sys/fc_present',
    '/api/sys/modbus_registers',
    '/api/sys/psu_update',
    '/api/sys/slotid',
    '/api/sys/mTerm_status'
]


all_routes = common_routes + board_routes

def get_endpoints(path):
    endpoints = set()
    splitpaths = {}
    splitpaths = path.split('/')
    position = len(splitpaths)
    for route in all_routes:
        rest_route_path = route.split('/')
        if len(rest_route_path) > position and path in route:
            endpoints.add(rest_route_path[position])
    return list(endpoints)
