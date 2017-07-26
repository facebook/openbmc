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
from aiohttp import web
import rest_component_presence
import rest_firmware
import rest_chassis_eeprom
import rest_chassis_all_serial_and_location


class boardApp_Handler:

    # Handler to fetch component presence
    async def rest_comp_presence(self,request):
        return web.json_response(rest_component_presence.get_presence())

    # Handler to fetch firmware_info
    async def rest_firmware_info(self,request):
        return web.json_response(rest_firmware.get_firmware_info())

    # Handler to fetch chassis eeprom
    async def rest_chassis_eeprom_hdl(self,request):
        return web.json_response(rest_chassis_eeprom.get_chassis_eeprom())

    # Handler to fetch SN and location for each card on chassis
    async def rest_all_serial_and_location_hdl(self,request):
        return web.json_response(rest_chassis_all_serial_and_location.get_all_serials_and_locations())
