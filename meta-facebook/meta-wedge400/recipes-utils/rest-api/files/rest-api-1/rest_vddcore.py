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

import subprocess
import glob
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for vdd_core resource endpoint
def get_vdd_core() -> Dict:
    return {"Information": get_vdd_core_data(), "Actions": [], "Resources": []}


def get_vdd_core_data() -> Dict:
    path = glob.glob("/sys/bus/i2c/devices/1-0040/hwmon/hwmon*/in3_input")
    if len(path) == 0:
        return {"result": "failure", "reason": "can't access device"}
    path = path[0]
    cmd = ["/usr/bin/head", path]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    return {"VDD_CORE": data.decode("utf-8").split("\n")[0]}


def set_vdd_core(value) -> Dict:
    cmd = ["/usr/local/bin/set_vdd.sh", value]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    rc = proc.returncode
    if rc == 0:
        return {"result": "success"}
    elif rc == 254:
        return {"result": "failure", "reason": "can't access device"}
    elif rc == 255:
        return {"result": "failure", "reason": "invalid value"}
    else:
        return {"result": "failure", "reason": "unknown reason"}
