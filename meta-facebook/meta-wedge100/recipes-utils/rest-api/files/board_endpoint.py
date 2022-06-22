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

import rest_fw_ver
import rest_i2cflush
import rest_presence
import rest_usb2i2c_reset
from aiohttp import web
from common_utils import dumps_bytestr


class boardApp_Handler:

    # Disable the endpoint in BMC until we root cause cp2112 issues.
    # Handler to reset usb-to-i2c
    # async def rest_usb2i2c_reset_hdl(self,request):
    # return rest_usb2i2c_reset.set_usb2i2c()

    async def rest_i2cflush_hdl(self, request):
        return web.json_response(rest_i2cflush.i2cflush(), dumps=dumps_bytestr)

    async def rest_firmware_info_all_hdl(self, request):
        versions = await asyncio.gather(
            rest_fw_ver.get_sys_cpld_ver(),
            rest_fw_ver.get_fan_cpld_ver(),
            rest_fw_ver.get_internal_switch_config(),
        )
        all_versions = {
            "SYS_CPLD": versions[0],
            "FAN_CPLD": versions[1],
            "INTERNAL_SWITCH_CONFIG": versions[2],
        }
        return web.json_response(all_versions, dumps=dumps_bytestr)

    async def rest_firmware_info_sys_hdl(self, request):
        cpld_version = await rest_fw_ver.get_sys_cpld_ver()
        response = {"SYS_CPLD": cpld_version}
        return web.json_response(response, dumps=dumps_bytestr)

    async def rest_firmware_info_fan_hdl(self, request):
        fan_version = await rest_fw_ver.get_fan_cpld_ver()
        response = {"FAN_CPLD": fan_version}
        return web.json_response(response, dumps=dumps_bytestr)

    async def rest_firmware_info_internal_switch_config_hdl(self, request):
        internal_switch_config = await rest_fw_ver.get_internal_switch_config()
        response = {"INTERNAL_SWITCH_CONFIG": internal_switch_config}
        return web.json_response(response, dumps=dumps_bytestr)

    async def rest_firmware_info_hdl(self, request):
        details = {
            "Information": {"Description": "Firmware versions"},
            "Actions": [],
            "Resources": ["all", "fan", "sys", "internal_switch_config"],
        }
        return web.json_response(details, dumps=dumps_bytestr)

    async def rest_presence_hdl(self, request):
        return web.json_response(rest_presence.get_presence_info(), dumps=dumps_bytestr)

    async def rest_presence_pem_hdl(self, request):
        return web.json_response(
            rest_presence.get_presence_info_pem(), dumps=dumps_bytestr
        )

    async def rest_presence_psu_hdl(self, request):
        return web.json_response(
            rest_presence.get_presence_info_psu(), dumps=dumps_bytestr
        )
