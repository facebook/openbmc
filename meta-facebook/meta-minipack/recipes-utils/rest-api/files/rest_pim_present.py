#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
import re
import subprocess


MAX_PIM_NUM = 8


# Use PIM FPGA to detect pim present
def is_pim_present(pim_no):
    smbcpld = "/sys/bus/i2c/devices/12-003e"
    pim_prsnt = "{:s}/pim_fpga_cpld_{:d}_prsnt_n_status".format(smbcpld, pim_no)
    try:
        with open(pim_prsnt, "r") as f:
            val = f.read()
            val = val.split("\n", 1)
            if val[0] == "0x1":
                return 0
            if val[0] == "0x0":
                return 1
    except:
        return None


def get_pim_present():
    state = {}

    for i in range(1, MAX_PIM_NUM + 1):
        pim_slot = "pim{:d}".format(i)
        if is_pim_present(i):
            state[pim_slot] = "Present"
        else:
            state[pim_slot] = "Removed"

    result = {"Information": state, "Actions": [], "Resources": []}
    return result
