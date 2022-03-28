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
from collections import namedtuple

import common_utils
from aiohttp import web

from board_setup_var import fruid_map, fw_map

FruidUtilInfo = namedtuple(
    "FruidUtilInfo", ["vendor", "model", "serial_number", "part_number"]
)

FWINFO_RESPONSE_CACHE = ""

async def get_fwinfo_response():
    global FWINFO_RESPONSE_CACHE
    if not FWINFO_RESPONSE_CACHE:
        fruid_util_info = {}
        for board in fruid_map:
            fruid_tmp_info = await _get_fruid_util_for_target(board)
            fruid_util_info[board] = fruid_tmp_info._asdict()
        fw_util_info = {}
        for board in fw_map:
            fw_util_info[board] = await _get_fwutil_info_for_target(board)
        response_body = {
            "fruid_info": fruid_util_info,
            "fw_info": fw_util_info
        }
        FWINFO_RESPONSE_CACHE = response_body
    else:
        response_body = FWINFO_RESPONSE_CACHE

    return {
        "data": {"Information": response_body, "Actions": [], "Resources": []},
        "status": 200,
    }


async def rest_fwinfo_handler(request):
    fwinfo_response = await get_fwinfo_response()
    return web.json_response(**fwinfo_response, dumps=common_utils.dumps_bytestr)


async def _get_fruid_util_for_target(target: str) -> FruidUtilInfo:
    fruid_util_response = FruidUtilInfo(
        vendor="", model="", serial_number="", part_number=""
    )
    cmd = "/usr/local/bin/fruid-util"
    retcode, data, _ = await common_utils.async_exec([cmd, target, "--json"])
    if retcode == 0:
        json_data = json.loads(data)
        if type(json_data) is list:
            # yv2 and higher fruid-util returns data in a list.
            fruid_data = json_data[0]
        else:
            # support for yv1
            fruid_data = json_data
        fruid_util_response = fruid_util_response._replace(
            vendor=fruid_data.get("Board Mfg")
        )
        fruid_util_response = fruid_util_response._replace(
            model=fruid_data.get("Board Product")
        )
        fruid_util_response = fruid_util_response._replace(
            serial_number=fruid_data.get("Board Serial")
        )
        fruid_util_response = fruid_util_response._replace(
            part_number=fruid_data.get("Product Part Number")
        )
    return fruid_util_response

def _filter_target(board: str,target: str):
    for item in fw_map[board]:
        if item == target:
            return fw_map[board][item]
    return None

async def _get_fwutil_info_for_target(target: str):
    fwutil_response = {}
    cmd = "/usr/bin/fw-util"
    retcode, process_stdout, _ = await common_utils.async_exec(
        [cmd, target, "--version"]
    )
    for stdout_line in process_stdout.splitlines():
        identifier, version = stdout_line.split(":", 1)
        name = _filter_target(target,identifier)
        if name is not None:
            fwutil_response[name] = version

    return fwutil_response
