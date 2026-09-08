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

import glob
import re
import subprocess
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


# set_vdd.sh evaluates its argument with shell arithmetic expansion ($(($1))),
# so only hand it an unsigned decimal integer.
VDD_CORE_VALUE_RE = re.compile(r"[0-9]+")


# Handler for vdd_core resource endpoint
def get_vdd_core() -> Dict:
    return {"Information": get_vdd_core_data(), "Actions": [], "Resources": []}


def get_vdd_core_data() -> Dict:
    path = glob.glob("/sys/bus/i2c/devices/17-0040/hwmon/hwmon*/in3_input")
    if len(path) == 0:
        return {"result": "failure", "reason": "can't access device"}
    path = path[0]
    cmd = ["/usr/bin/head", path]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    return {"VDD_CORE": data.decode("utf-8").split("\n")[0]}


def set_vdd_core(value: str) -> Dict:
    if VDD_CORE_VALUE_RE.fullmatch(value) is None or int(value) <= 0:
        return {"result": "failure", "reason": "invalid value"}
    # Pass the canonical form: shell arithmetic reads a leading zero as octal,
    # so "0750" would otherwise set 488 mV instead of 750 mV.
    cmd = ["/usr/local/bin/set_vdd.sh", str(int(value))]
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
