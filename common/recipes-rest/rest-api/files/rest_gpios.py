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

import re
from rest_fruid import get_fruid
from rest_helper import read_gpio_by_shadow


def read_wedge_back_ports():
    bhinfo = {
        "port_1": {
            "pin_1": read_gpio_by_shadow("RMON1_PF"),
            "pin_2": read_gpio_by_shadow("RMON1_RF"),
            "pin_3": read_gpio_by_shadow("RMON2_PF"),
            "pin_4": read_gpio_by_shadow("RMON2_RF"),
        },
        "port_2": {
            "pin_1": read_gpio_by_shadow("RMON3_PF"),
            "pin_2": read_gpio_by_shadow("RMON3_RF"),
        },
    }
    return bhinfo


def get_gpios():
    fruinfo = get_fruid()
    gpioinfo = {}
    if re.match("WEDGE.*", fruinfo["Information"]["Product Name"], re.IGNORECASE):
        gpioinfo["back_ports"] = read_wedge_back_ports()
    return gpioinfo
