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

from rest_utils import DEFAULT_TIMEOUT_SEC

MAX_PIM_NUM = 8
BASE = 1
pim_number_re = re.compile("\s*PIM (\S.*):")
fpga_type_re = re.compile("\s*(\S.*) DOMFPGA: (\S.*)")


def prepare_piminfo():
    pim_type = {str(i): "NA" for i in range(1, 9)}
    pim_fpga_ver = {str(i): "NA" for i in range(1, 9)}
    current_pim = 0
    try:
        stdout = subprocess.check_output(
            ["/usr/local/bin/fpga_ver.sh", "-u"], timeout=DEFAULT_TIMEOUT_SEC
        )
        for text_line in stdout.decode().splitlines():
            # Check if this line shows PIM slot number
            m = pim_number_re.match(text_line, 0)
            if m:
                current_pim = m.group(1)
            m = fpga_type_re.match(text_line, 0)
            if m:
                pim_type[current_pim] = m.group(1)
                pim_fpga_ver[current_pim] = m.group(2)
    except Exception:
        pass

    return pim_type, pim_fpga_ver


# Use DOM FPGA Version to detect pim presence
def get_pim_present():
    state = {}
    pim_type, pim_fpga_ver = prepare_piminfo()
    for i in range(1, MAX_PIM_NUM + 1):
        pim_slot = "pim{:d}".format(i + BASE - 1)
        if pim_type[str(i)] == "NA" or pim_fpga_ver[str(i)] == "NA":
            state[pim_slot] = "Removed"
        else:
            state[pim_slot] = "Present"

    result = {"Information": state, "Actions": [], "Resources": []}
    return result
