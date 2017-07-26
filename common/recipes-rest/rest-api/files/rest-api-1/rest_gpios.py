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

from rest_fruid import get_fruid
from rest_helper import read_gpio_sysfs

WEDGES = ["Wedge-AC-F", "Wedge-DC-F"]

def read_wedge_back_ports():
    bhinfo = { "port_1": { "pin_1": read_gpio_sysfs(120),
                           "pin_2": read_gpio_sysfs(121),
                           "pin_3": read_gpio_sysfs(122),
                           "pin_4": read_gpio_sysfs(123)
                           },
               "port_2": { "pin_1": read_gpio_sysfs(124),
                           "pin_2": read_gpio_sysfs(125),
                           "pin_3": read_gpio_sysfs(126),
                           "pin_4": read_gpio_sysfs(52)
                           }
               }
    return bhinfo

def get_gpios():
    fruinfo = get_fruid()
    gpioinfo = {}
    if fruinfo["Information"]["Product Name"] in WEDGES:
        gpioinfo["back_ports"] = read_wedge_back_ports()
    return gpioinfo
