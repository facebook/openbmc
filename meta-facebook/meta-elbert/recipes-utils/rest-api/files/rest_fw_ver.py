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
from asyncio.subprocess import PIPE, create_subprocess_exec
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


def _parse_all_fpga_ver(data) -> Dict:
    """Example output:
    ------SCM-FPGA------
    SCM_FPGA: 1.11
    ------FAN-FPGA------
    FAN_FPGA: 1.1
    ------SMB-FPGA------
    SMB_FPGA: 1.15
    ------SMB-CPLD------
    SMB_CPLD: 4.1
    ------PIM-FPGA------
    PIM 2: 4.2
    PIM 3: 4.2
    PIM 4: 4.2
    PIM 5: 4.2
    PIM 6: 4.2
    PIM 7: 4.2
    PIM 8: 4.2
    PIM 9: 4.2

    Will return a dict of the form:
    {"PIM 2": "4.2",
    "PIM 3": "4.2",
    "PIM 4": "4.2",
    ...
    "SCM": "1.11",
    "SMB": "14.4"}
    """
    rs = {}
    pim = 0
    pim_rx = re.compile("^PIM ([2-9])\s*:\s*(\S+)")
    fpga_rx = re.compile("([a-zA-Z]+)[_ ]FPGA\s*:\s*(\S+)")
    cpld_rx = re.compile("([a-zA-Z]+_CPLD)\s*:\s*(\S+)")
    for l in data.splitlines():
        m = pim_rx.match(l)
        if m:
            pim = m.group(1)
            rs["PIM " + pim] = m.group(2)
            continue
        m = fpga_rx.match(l)
        if m:
            rs[m.group(1)] = m.group(2)
            continue
        m = cpld_rx.match(l)
        if m:
            rs[m.group(1)] = m.group(2)

    return rs


async def get_all_fw_ver() -> Dict:
    cmd = "/usr/local/bin/fpga_ver.sh"
    proc = await create_subprocess_exec(cmd, stdout=PIPE)
    data, _ = await proc.communicate()
    await proc.wait()
    return _parse_all_fpga_ver(data.decode(errors="ignore"))
