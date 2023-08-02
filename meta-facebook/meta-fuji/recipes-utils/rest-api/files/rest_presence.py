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

RE_PRESENCE_UTIL_OUTPUT = re.compile(
    # e.g. "psu1 : 1" -> name = "psu1", value = "1"
    r"^(?P<name>[^\s]+) \s* : \s* (?P<value>\d+)$",
    flags=re.MULTILINE | re.VERBOSE,
)


# Handler for sys/presence resource endpoint
def get_presence_info_scm() -> Dict:
    return {
        "Information": get_presence_info_data("scm"),
        "Actions": [],
        "Resources": [],
    }


def get_presence_info_fan() -> Dict:
    return {
        "Information": get_presence_info_data("fan"),
        "Actions": [],
        "Resources": [],
    }


def get_presence_info_psu() -> Dict:
    return {
        "Information": get_presence_info_data("psu"),
        "Actions": [],
        "Resources": [],
    }


def get_presence_info_pim() -> Dict:
    return {
        "Information": get_presence_info_data("pim"),
        "Actions": [],
        "Resources": [],
    }


def get_presence_info() -> Dict:
    result = []
    devices = ["scm", "fan", "psu", "pim"]
    for dev in devices:
        result.append(get_presence_info_data(dev))
    return {"Information": result, "Actions": [], "Resources": devices}


def _parse_presence_info_data(data) -> Dict[str, str]:
    matches = RE_PRESENCE_UTIL_OUTPUT.findall(data)
    result = {name: value for (name, value) in matches}
    return result


def get_presence_info_data(dev) -> Dict:
    cmd = ["/usr/local/bin/presence_util.sh", dev]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return _parse_presence_info_data(data)
