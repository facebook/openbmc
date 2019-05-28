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
import asyncio
from concurrent.futures import ThreadPoolExecutor

import rest_chassis_all_serial_and_location
import rest_chassis_eeprom
import rest_component_presence
import rest_firmware
from aiohttp import web
from rest_utils import dumps_bytestr, get_endpoints


# A separate decorator for board specific API function
# Note that we use different executor stored in different
# class variable name (also a different class)
def board_force_async(func):
    async def func_wrapper(self, *args, **kwargs):
        loop = asyncio.get_event_loop()
        # As we separated *arts into (self, *args) pair, we need to
        # write "self" again, as the first argument before *args
        result = await loop.run_in_executor(
            self.board_executor, func, self, *args, **kwargs
        )
        return result

    return func_wrapper


# Method implementation using the decorator
class boardApp_Handler:
    # Just as in common REST API handler, we use a separate
    # executor, in order to prevent any issue in common REST handlers
    # from using up all threads for board specific handlers.
    def __init__(self):
        self.board_executor = ThreadPoolExecutor(5)

    # Handler to fetch component presence
    def helper_rest_comp_presence(self, request):
        return web.json_response(
            rest_component_presence.get_presence(), dumps=dumps_bytestr
        )

    @board_force_async
    def rest_comp_presence(self, request):
        return self.helper_rest_comp_presence(request)

    # Handler to fetch firmware_info
    def helper_rest_firmware_info(self, request):
        return web.json_response(rest_firmware.get_firmware_info(), dumps=dumps_bytestr)

    @board_force_async
    def rest_firmware_info(self, request):
        return self.helper_rest_firmware_info(request)

    # Handler to fetch chassis eeprom
    def helper_rest_chassis_eeprom_hdl(self, request):
        return web.json_response(
            rest_chassis_eeprom.get_chassis_eeprom(), dumps=dumps_bytestr
        )

    @board_force_async
    def rest_chassis_eeprom_hdl(self, request):
        return self.helper_rest_chassis_eeprom_hdl(request)

    # Handler to fetch SN and location for each card on chassis
    def helper_rest_all_serial_and_location_hdl(self, request):
        return web.json_response(
            rest_chassis_all_serial_and_location.get_all_serials_and_locations(),
            dumps=dumps_bytestr,
        )

    @board_force_async
    def rest_all_serial_and_location_hdl(self, request):
        return self.helper_rest_all_serial_and_location_hdl(request)
