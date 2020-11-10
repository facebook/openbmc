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
import subprocess
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for system led info resource endpoint
def get_system_led_info() -> Dict:
    return {"Information": _get_system_led_info(), "Actions": [], "Resources": []}


def _parse_system_led_info(data: str) -> Dict:
    """ Parse led info

    Data format: <led_type>:<color>
    """
    result = {}
    for line in data.splitlines():
        if re.match(r".*:.*", line):
            split = line.split(":")
            led_type = split[0].strip()
            color = split[1].strip()
            result[led_type] = color

    return result


def _get_system_led_info() -> Dict:
    # Get system leds info
    cmd = ["/usr/local/bin/read_sled.sh"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return _parse_system_led_info(data)
