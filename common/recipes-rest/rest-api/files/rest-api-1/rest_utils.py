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
import json
from boardroutes import board_routes
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

# aiohttp allows users to pass a "dumps" function, which will convert
# different data types to JSON. This new dumps function will simply call
# the original dumps function, along with the new type handler that can
# process byte strings.
def dumps_bytestr(obj):
    # aiohttp's json_response function uses py3 JSON encoder, which
    # doesn't know how to handle a byte string. So we extend this function
    # to handle the case. This is a standard way to add a new type,
    # as stated in JSON encoder source code.
    def default_bytestr(o):
        # If the object is a byte string, it will be converted
        # to a regular string. Otherwise we move on (pass) to
        # the usual error generation routine
        try:
           o = o.decode('utf-8')
           return o
        except AttributeError:
           pass
        raise TypeError(repr(o) + " is not JSON serializable")
    # Just call default dumps function, but pass the new default function
    # that is capable of process byte strings.
    return json.dumps(obj, default=default_bytestr)


