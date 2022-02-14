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

FruidUtilInfo = namedtuple(
    "FruidUtilInfo", ["vendor", "model", "serial_number", "part_number"]
)
FwUtilInfo = namedtuple("FwUtilInfo", ["bmc_ver", "bmc_cpld_ver", "bic_version"])

FWINFO_RESPONSE_CACHE = ""


async def get_fwinfo_response():
    global FWINFO_RESPONSE_CACHE
    if not FWINFO_RESPONSE_CACHE:
        fw_info_slot1 = await _get_fruid_util_for_target("slot1")
        fw_info_nic = await _get_fruid_util_for_target("nic")
        fw_info_nicexp = await _get_fruid_util_for_target("nicexp")
        fw_util_info = await _get_fwutil_info_all()
        response_body = {
            "fruid_info": {
                "slot1": fw_info_slot1._asdict(),
                "nic": fw_info_nic._asdict(),
                "nicexp": fw_info_nicexp._asdict(),
            },
            "fw_info": fw_util_info._asdict(),
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


async def _get_fwutil_info_all() -> FwUtilInfo:
    fwutil_response = FwUtilInfo(bmc_ver="", bmc_cpld_ver="", bic_version="")
    cmd = "/usr/bin/fw-util"
    retcode, process_stdout, _ = await common_utils.async_exec(
        [cmd, "all", "--version"]
    )
    for stdout_line in process_stdout.splitlines():
        identifier, version = stdout_line.split(":", 1)
        if identifier == "BMC Version":
            fwutil_response = fwutil_response._replace(bmc_ver=version.strip())
        if identifier == "BMC CPLD Version":
            fwutil_response = fwutil_response._replace(bmc_cpld_ver=version.strip())
        if identifier == "2OU Bridge-IC Version":
            fwutil_response = fwutil_response._replace(bic_version=version.strip())
    return fwutil_response
