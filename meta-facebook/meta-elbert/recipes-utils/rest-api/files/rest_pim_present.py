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
import re
import subprocess


MAX_PIM_NUM = 8


# Use PIM FPGA to detect pim present
def check_pim_presence(pim_no):
    scdbase = "/sys/bus/i2c/drivers/smbcpld/4-0023"
    try:
        pim_prsnt = "{:s}/pim{:d}_present".format(scdbase, pim_no)
        with open(pim_prsnt, "r") as f:
            val = f.read()
            val = val.split("\n", 1)
            if val[0] == "0x0":
                return 0
            if val[0] == "0x1":
                return 1
    except:
        return None


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
