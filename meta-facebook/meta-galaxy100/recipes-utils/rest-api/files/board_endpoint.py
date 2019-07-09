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
import rest_chassis_eeprom
import rest_firmware
import rest_fruid_scm
import rest_i2cflush
import rest_seutil
import rest_usb2i2c_reset
from aiohttp import web
from rest_utils import dumps_bytestr, get_endpoints


class boardApp_Handler:

    # Handler for sys/mb/fruid_scm resource endpoint
    async def rest_fruid_scm_hdl(self, request):
        return web.json_response(rest_fruid_scm.get_fruid_scm(), dumps=dumps_bytestr)

    # Handler for sys/mb/seutil resource endpoint
    async def rest_chassis_eeprom_hdl(self, request):
        return web.json_response(
            rest_chassis_eeprom.get_chassis_eeprom(), dumps=dumps_bytestr
        )

    # Handler for sys/mb/seutil resource endpoint
    async def rest_seutil_hdl(self, request):
        return web.json_response(rest_seutil.get_seutil(), dumps=dumps_bytestr)

    # Handler to reset usb-to-i2c
    async def rest_usb2i2c_reset_hdl(self, request):
        return web.json_response(rest_usb2i2c_reset.set_usb2i2c(), dumps=dumps_bytestr)

    # Handler to get firmware info
    async def rest_firmware_info(self, request):
        return web.json_response(rest_firmware.get_firmware_info(), dumps=dumps_bytestr)

    # Handler to unfreeze I2C bus through I2C clock flush
    async def rest_i2cflush_hdl(self, request):
        return web.json_response(rest_i2cflush.i2cflush(), dumps=dumps_bytestr)
