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

import os
from typing import Dict


#
# File to cache power module types.
# The path should be consistent with the one defined in psumuxmon.py.
#
POWER_TYPE_CACHE = "/var/cache/detect_power_module_type.txt"


def get_presence_info() -> Dict:
    result = []
    devices = ["pem", "psu"]
    for dev in devices:
        result.append(get_presence_info_data(dev))
    return {"Information": result, "Actions": [], "Ressources": devices}


def get_presence_info_data(power_type) -> Dict:
    result = {}
    if not os.path.exists(POWER_TYPE_CACHE):
        raise Exception("Power type cache %s doesn't exist" % POWER_TYPE_CACHE)
    with open(POWER_TYPE_CACHE, "r") as fp:
        lines = fp.readlines()
        if lines:
            for line in lines:
                dev = line.split(": ")[0]
                presence_status = line.split(": ")[1]
                if power_type in dev:
                    result[dev] = presence_status
        else:
            raise Exception("Power module file is empty")
    return result


def get_presence_info_pem() -> Dict:
    return {
        "Information": get_presence_info_data("pem"),
        "Actions": [],
        "Resources": [],
    }


def get_presence_info_psu() -> Dict:
    return {
        "Information": get_presence_info_data("psu"),
        "Actions": [],
        "Resources": [],
    }
