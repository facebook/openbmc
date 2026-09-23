#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

from rest_helper import read_gpio_by_shadow


def get_leakage_status():
    """
    Returns the leakage sensor status by reading GPIOs.
    """
    value = read_gpio_by_shadow("LEAK_BMC_L")
    if value == 0:
        detected = True
    elif value == 1:
        detected = False
    else:
        detected = None

    leakage_info = {
        "LEAKAGE_DETECTED": detected,
    }
    return {"Information": leakage_info, "Actions": [], "Resources": []}
