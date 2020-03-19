#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
import rest_scdinfo
import rest_seutil
from aiohttp import web
from rest_utils import dumps_bytestr, get_endpoints


class boardApp_Handler:
    # Handler for sys/mb/fruid/scm resource endpoint
    async def rest_fruid_scm_hdl(self, request):
        return web.json_response(rest_fruid_scm.get_fruid_scm(), dumps=dumps_bytestr)

    # Handler for sys/mb/scdinfo resource endpoint
    async def rest_scdinfo_hdl(self, request):
        return web.json_response(rest_scdinfo.get_scdinfo(), dumps=dumps_bytestr)

    # Handler for sys/pim_present resource endpoint
    async def rest_pim_present_hdl(self, request):
        return web.json_response(
            rest_pim_present.get_pim_present(), dumps=dumps_bytestr
        )

    # Handler for sys/pim_info resource endpoint (version)
    async def rest_piminfo_hdl(self, request):
        return web.json_response(rest_piminfo.get_piminfo())

    # Handler for sys/pim_serial resource endpoint
    async def rest_pimserial_hdl(self, request):
        return web.json_response(rest_pimserial.get_pimserial())

    # Handler for sys/pimstatus resource endpoint
    async def rest_pimstatus_hdl(self, request):
        return web.json_response(rest_pimstatus.get_pimstatus())

    # Handler for sys/mb/seutil resource endpoint
    async def rest_seutil_hdl(self, request):
        return web.json_response(rest_seutil.get_seutil(), dumps=dumps_bytestr)

    # Handler for sys/mb/pim$/peutil resource endpoint
    async def rest_peutil_hdl(self, request):
        path = request.rel_url.path
        pim = re.search(r"pim(\d)", path).group(1)
        return web.json_response(rest_peutil.get_peutil(pim), dumps=dumps_bytestr)

    async def rest_firmware_info_all_hdl(self, request):
        fws = await rest_fw_ver.get_all_fw_ver()
        return web.json_response(fws, dumps=dumps_bytestr)
