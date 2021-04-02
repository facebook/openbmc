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

import json
import os.path
import re
import subprocess

from rest_utils import DEFAULT_TIMEOUT_SEC


MAX_PIM_NUM = 8
SMB_P1_DETECT_PATH = "/tmp/.smb_p1_board"

# Use PIM EEPROM to detect pim present
def check_pim_presence(pim_no):
    # P1 has different PIM bus mapping
    if os.path.isfile(SMB_P1_DETECT_PATH):
        pim_i2c_buses = [16, 17, 18, 23, 20, 21, 22, 19]
    else:
        pim_i2c_buses = [16, 17, 18, 19, 20, 21, 22, 23]

    pim_bus = pim_i2c_buses[pim_no - 2]
    try:
        # Try to access the EEPROM and check the return code
        proc = subprocess.run(
            ["/usr/sbin/i2cget", "-f", "-y", str(pim_bus), "0x50"],
            capture_output=True,
            timeout=DEFAULT_TIMEOUT_SEC,
            check=True
        )
        return 1
    except subprocess.CalledProcessError:
        return 0


def get_pim_present():
    state = {}

    for i in range(2, MAX_PIM_NUM + 2):
        pim_slot = "pim{:d}".format(i)
        if check_pim_presence(i):
            state[pim_slot] = "Present"
        else:
            state[pim_slot] = "Removed"

    result = {"Information": state, "Actions": [], "Resources": []}
    return result
