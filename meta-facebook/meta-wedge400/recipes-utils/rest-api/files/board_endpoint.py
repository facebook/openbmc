#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

import re

import rest_firmware_info
import rest_presence
import rest_seutil
import rest_usb2i2c_reset
from aiohttp import web
from rest_utils import dumps_bytestr


class boardApp_Handler:
    # Handler to reset usb-to-i2c
    async def rest_usb2i2c_reset_hdl(self, request):
        return web.json_response(rest_usb2i2c_reset.set_usb2i2c(), dumps=dumps_bytestr)

    # Handler for sys/seutil resource endpoint
    async def rest_seutil_hdl(self, request):
        return web.json_response(rest_seutil.get_seutil(), dumps=dumps_bytestr)

    # Handler for sys/firmware_info resource endpoint
    async def rest_firmware_info_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info(), dumps=dumps_bytestr
        )

    # Handler for sys/firmware_info/cpld resource endpoint
    async def rest_firmware_info_cpld_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info_cpld(), dumps=dumps_bytestr
        )

    # Handler for sys/firmware_info/fpga resource endpoint
    async def rest_firmware_info_fpga_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info_fpga(), dumps=dumps_bytestr
        )

    # Handler for sys/presence resource endpoint
    async def rest_presence_hdl(self, request):
        return web.json_response(rest_presence.get_presence_info(), dumps=dumps_bytestr)

    # Handler for sys/presence/scm resource endpoint
    async def rest_presence_scm_hdl(self, request):
        return web.json_response(
            rest_presence.get_presence_info_scm(), dumps=dumps_bytestr
        )

    # Handler for sys/presence/fan resource endpoint
    async def rest_presence_fan_hdl(self, request):
        return web.json_response(
            rest_presence.get_presence_info_fan(), dumps=dumps_bytestr
        )

    # Handler for sys/presence/psu resource endpoint
    async def rest_presence_psu_hdl(self, request):
        return web.json_response(
            rest_presence.get_presence_info_psu(), dumps=dumps_bytestr
        )
