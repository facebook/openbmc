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
    devices = ["cpld", "fpga", "scm", "all"]
    return {"Information": result, "Actions": [], "Resources": devices}


# Handler for sys/firmware/cpld resource endpoint
def get_firmware_info_cpld() -> Dict:
    cmd = ["/usr/bin/fw-util", "cpld", "--version"]
    return {"Information": get_dev_ver_data(cmd), "Actions": [], "Resources": []}


# Handler for sys/firmware/fpga resource endpoint
def get_firmware_info_fpga() -> Dict:
    cmd = ["/usr/bin/fw-util", "fpga", "--version"]
    return {"Information": get_dev_ver_data(cmd), "Actions": [], "Resources": []}


# Handler for sys/firmware/scm resource endpoint
def get_firmware_info_scm() -> Dict:
    cmd = ["/usr/bin/fw-util", "scm", "--version"]
    return {"Information": get_dev_ver_data(cmd), "Actions": [], "Resources": []}


# Handler for sys/firmware/all resource endpoint
def get_firmware_info_all() -> Dict:
    cmd = ["/usr/bin/fw-util", "all", "--version"]
    return {"Information": get_dev_ver_data(cmd), "Actions": [], "Resources": []}


def _parse_firmware_info_data(data) -> Dict:
    result = {}
    for sdata in data.splitlines():
        items = sdata.split(": ")
        if len(items) < 2:
            continue
        dev = items[0]
        firmware_version = items[1]
        result[dev] = firmware_version
    return result


def get_dev_ver_data(cmd) -> Dict:
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return _parse_firmware_info_data(data)
