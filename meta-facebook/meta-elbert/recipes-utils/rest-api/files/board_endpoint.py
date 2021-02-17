#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

import rest_fruid_scm
import rest_fw_ver
import rest_peutil
import rest_pim_present
import rest_piminfo
import rest_pimserial
import rest_pimstatus
import rest_sensors
import rest_seutil
import rest_smbinfo
from aiohttp import web
from rest_utils import dumps_bytestr


class boardApp_Handler:
    # Handler for sys/mb/fruid/scm resource endpoint
    async def rest_fruid_scm_hdl(self, request):
        return web.json_response(rest_fruid_scm.get_fruid_scm(), dumps=dumps_bytestr)

    # Handler for sys/mb/seutil resource endpoint
    async def rest_seutil_hdl(self, request):
        return web.json_response(rest_seutil.get_seutil(), dumps=dumps_bytestr)

    # Handler for sys/mb/pim$/peutil resource endpoint
    async def rest_peutil_hdl(self, request):
        path = request.rel_url.path
        pim = re.search(r"pim(\d)", path).group(1)
        return web.json_response(rest_peutil.get_peutil(pim), dumps=dumps_bytestr)

    # Handler for sys/pim_info resource endpoint (version)
    async def rest_piminfo_hdl(self, request):
        return web.json_response(rest_piminfo.get_piminfo())

    # Handler for sys/pim_serial resource endpoint
    async def rest_pimserial_hdl(self, request):
        return web.json_response(rest_pimserial.get_pimserial())

    # Handler for sys/pimstatus resource endpoint
    async def rest_pimstatus_hdl(self, request):
        return web.json_response(rest_pimstatus.get_pimstatus())

    # Handler for sys/pim_present resource endpoint
    async def rest_pim_present_hdl(self, request):
        return web.json_response(
            rest_pim_present.get_pim_present(), dumps=dumps_bytestr
        )

    # Handler for sys/smb_info resource endpoint (version)
    async def rest_smbinfo_hdl(self, request):
        return web.json_response(rest_smbinfo.get_smbinfo())

    async def rest_firmware_info_all_hdl(self, request):
        fws = await rest_fw_ver.get_all_fw_ver()
        return web.json_response(fws, dumps=dumps_bytestr)

    # Handler for sys/sensors/scm resource endpoint
    async def rest_sensors_scm_hdl(self, request):
        return web.json_response(rest_sensors.get_scm_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/smb resource endpoint
    async def rest_sensors_smb_hdl(self, request):
        return web.json_response(rest_sensors.get_smb_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu1 resource endpoint
    async def rest_sensors_psu1_hdl(self, request):
        return web.json_response(rest_sensors.get_psu1_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu2 resource endpoint
    async def rest_sensors_psu2_hdl(self, request):
        return web.json_response(rest_sensors.get_psu2_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu3 resource endpoint
    async def rest_sensors_psu3_hdl(self, request):
        return web.json_response(rest_sensors.get_psu3_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/psu4 resource endpoint
    async def rest_sensors_psu4_hdl(self, request):
        return web.json_response(rest_sensors.get_psu4_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim2 resource endpoint
    async def rest_sensors_pim2_hdl(self, request):
        return web.json_response(rest_sensors.get_pim2_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim3 resource endpoint
    async def rest_sensors_pim3_hdl(self, request):
        return web.json_response(rest_sensors.get_pim3_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim4 resource endpoint
    async def rest_sensors_pim4_hdl(self, request):
        return web.json_response(rest_sensors.get_pim4_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim5 resource endpoint
    async def rest_sensors_pim5_hdl(self, request):
        return web.json_response(rest_sensors.get_pim5_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim6 resource endpoint
    async def rest_sensors_pim6_hdl(self, request):
        return web.json_response(rest_sensors.get_pim6_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim7 resource endpoint
    async def rest_sensors_pim7_hdl(self, request):
        return web.json_response(rest_sensors.get_pim7_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim8 resource endpoint
    async def rest_sensors_pim8_hdl(self, request):
        return web.json_response(rest_sensors.get_pim8_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/pim9 resource endpoint
    async def rest_sensors_pim9_hdl(self, request):
        return web.json_response(rest_sensors.get_pim9_sensors(), dumps=dumps_bytestr)

    # Handler for sys/sensors/fan resource endpoint
    async def rest_sensors_fan_hdl(self, request):
        return web.json_response(rest_sensors.get_fan_sensors(), dumps=dumps_bytestr)
