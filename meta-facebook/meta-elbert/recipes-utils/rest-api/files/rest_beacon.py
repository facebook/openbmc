#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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


# Handler for beacon/mode resource endpoint
def beacon_on(mode: str) -> Dict:
    beacon_status = _set_beacon(on=True, mode=mode)
    return {"Information": beacon_status, "Actions": [], "Resources": []}


# Handler for beacon/off resource endpoint
def beacon_off() -> Dict:
    beacon_status = _set_beacon(on=False)
    return {"Information": beacon_status, "Actions": [], "Resources": []}


def _parse_beacon_status(data: str) -> Dict:
    result = {"state": "OFF", "mode": "NA"}
    beacon_re = r"Beacon LED:\s*(now|already)\s*(ON|OFF)\s*(in\s*([a-z]+)\s*mode)?"
    m = re.search(beacon_re, data)
    if m:
        result["state"] = m.group(2)
        if m.group(4):
            result["mode"] = m.group(4)
    return result


def _set_beacon(on: bool = True, mode: str = "") -> Dict:
    cmd = ["/usr/local/bin/beacon_led.sh", "on" if on else "off"]
    if on and mode:
        cmd.append( mode )
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return _parse_beacon_status(data)
