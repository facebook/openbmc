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
import os

POWER_TYPE_CACHE = "/var/cache/detect_power_module_type.txt"


def match_power_type(dev, presence):
    """
    check to see which power device is being used and
    if it is being detected. Then we return the list
    of the power device kernel driver modules that must
    present in the image.
    pem1 = 1 --> means pem with ltc4151 inserted
    pem2 = 1 --> means pem with ltc4281 inserted
    psu1 and psu2 = 1 -> means system with PSU inserted
    """
    power_type = None
    if dev == "pem1" and presence == 1:
        power_type = "pem1"
    elif dev == "pem2" and presence == 1:
        power_type = "pem2"
    elif dev == "psu1" or dev == "psu2":
        if presence == 1:
            power_type = "psu"
    else:
        raise Exception("file contains unknown module")

    return power_type


def get_power_type():
    """
    Read appropriate path that contains the presence
    status of various power module option.
    """
    power_type = None

    if not os.path.exists(POWER_TYPE_CACHE):
        raise Exception("Path for power type doesn't exist")

    with open(POWER_TYPE_CACHE, "r") as fp:
        lines = fp.readlines()
        if lines:
            for line in lines:
                dev = line.split(": ")[0]
                presence = int(line.split(": ")[1])
                if presence == 1:
                    power_type = match_power_type(dev, presence)
                    break
        else:
            raise Exception("Power module file is empty")

    return power_type
