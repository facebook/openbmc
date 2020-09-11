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


WEDGE40 = ["Wedge-DC-F", "Wedge-AC-F"]


def read_wedge_back_ports(legacy=False):
    if legacy:
        bhinfo = {
            "port_1": {
                "pin_1": read_gpio_by_shadow("BLOODHOUND_GPIOP0"),
                "pin_2": read_gpio_by_shadow("BLOODHOUND_GPIOP1"),
                "pin_3": read_gpio_by_shadow("BLOODHOUND_GPIOP2"),
                "pin_4": read_gpio_by_shadow("BLOODHOUND_GPIOP3"),
            },
            "port_2": {
                "pin_1": read_gpio_by_shadow("BLOODHOUND_GPIOP4"),
                "pin_2": read_gpio_by_shadow("BLOODHOUND_GPIOP5"),
            },
        }
    else:
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
        is_wedge40 = fruinfo["Information"]["Product Name"] in WEDGE40
        gpioinfo["back_ports"] = read_wedge_back_ports(is_wedge40)
    return gpioinfo
