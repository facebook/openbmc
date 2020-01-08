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
import rest_feutil
import rest_sensors
import rest_switch_reset
import rest_vddcore
from aiohttp import web
from rest_utils import dumps_bytestr


class boardApp_Handler:
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

    # Handler for sys/firmware_info/scm resource endpoint
    async def rest_firmware_info_scm_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info_scm(), dumps=dumps_bytestr
        )

    # Handler for sys/firmware_info/all resource endpoint
    async def rest_firmware_info_all_hdl(self, request):
        return web.json_response(
            rest_firmware_info.get_firmware_info_all(), dumps=dumps_bytestr
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

    # Handler for sys/feutil resource endpoint
    async def rest_feutil_hdl(self, request):
        return web.json_response(rest_feutil.get_feutil(), dumps=dumps_bytestr)

    # Handler for sys/feutil/all resource endpoint
    async def rest_feutil_all_hdl(self, request):
        return web.json_response(rest_feutil.get_feutil_all_data(), dumps=dumps_bytestr)

    # Handler for sys/feutil/fcm resource endpoint
    async def rest_feutil_fcm_hdl(self, request):
        return web.json_response(rest_feutil.get_feutil_fcm_data(), dumps=dumps_bytestr)

    # Handler for sys/feutil/fan1 resource endpoint
    async def rest_feutil_fan1_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan1_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan2 resource endpoint
    async def rest_feutil_fan2_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan2_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan3 resource endpoint
    async def rest_feutil_fan3_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan3_data(), dumps=dumps_bytestr
        )

    # Handler for sys/feutil/fan4 resource endpoint
    async def rest_feutil_fan4_hdl(self, request):
        return web.json_response(
            rest_feutil.get_feutil_fan4_data(), dumps=dumps_bytestr
        )

    # Handler for sys/sensors/scm resource endpoint
    async def rest_sensors_scm_hdl(self, request):
        return web.json_response(rest_sensors.get_scm_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/smb resource endpoint
    async def rest_sensors_smb_hdl(self, request):
        return web.json_response(rest_sensors.get_smb_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pem1 resource endpoint
    async def rest_sensors_pem1_hdl(self, request):
        return web.json_response(rest_sensors.get_pem1_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pem2 resource endpoint
    async def rest_sensors_pem2_hdl(self, request):
        return web.json_response(rest_sensors.get_pem2_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu1 resource endpoint
    async def rest_sensors_psu1_hdl(self, request):
        return web.json_response(rest_sensors.get_psu1_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu2 resource endpoint
    async def rest_sensors_psu2_hdl(self, request):
        return web.json_response(rest_sensors.get_psu2_sensors(), dumps=dumps_bytestr)

    # Handler for sys/vddcore resource endpoint
    async def rest_vddcore_get_volt(self, request):
        return web.json_response(rest_vddcore.get_vdd_core(), dumps=dumps_bytestr)

    # Handler for sys/vddcore/{voltage} endpoint
    async def rest_vddcore_set_volt(self, request):
        return web.json_response(
            rest_vddcore.set_vdd_core(request.match_info["volt"]), dumps=dumps_bytestr
        )

    # Handler for sys/switch_reset endpoint
    async def rest_switch_reset_hdl(self, request):
        return web.json_response(rest_switch_reset.reset_switch(), dumps=dumps_bytestr)

    # Handler for sys/switch_reset/cycle_reset endpoint
    async def rest_switch_reset_cycle_reset_hdl(self, request):
        return web.json_response(
            rest_switch_reset.reset_switch_cycle_reset(), dumps=dumps_bytestr
        )

    # Handler for sys/switch_reset/only_reset endpoint
    async def rest_switch_reset_only_reset_hdl(self, request):
        return web.json_response(
            rest_switch_reset.reset_switch_only_reset(), dumps=dumps_bytestr
        )
