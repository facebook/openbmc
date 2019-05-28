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
import subprocess
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for sys/firmware resource endpoint
def get_firmware_info() -> Dict:
    result = []
    devices = ["cpld", "fpga"]
    for dev in devices:
        result.append(get_dev_ver_data(dev))
    return {"Information": result, "Actions": [], "Resources": devices}


# Handler for sys/firmware/cpld resource endpoint
def get_firmware_info_cpld() -> Dict:
    return {"Information": get_dev_ver_data("cpld"), "Actions": [], "Resources": []}


# Handler for sys/firmware/fpga resource endpoint
def get_firmware_info_fpga() -> Dict:
    return {"Information": get_dev_ver_data("fpga"), "Actions": [], "Resources": []}


def _parse_firmware_info_data(data) -> Dict:
    result = {}
    for sdata in data.splitlines():
        dev = sdata.split(": ")[0]
        firmware_version = sdata.split(": ")[1]
        result[dev] = firmware_version
    return result


def get_dev_ver_data(dev) -> Dict:
    cmd = ["/usr/local/bin/{}_ver.sh".format(dev)]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return _parse_firmware_info_data(data)
