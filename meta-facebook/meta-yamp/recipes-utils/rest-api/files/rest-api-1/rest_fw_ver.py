#!/usr/bin/env python3
import re
import subprocess
from asyncio.subprocess import PIPE, create_subprocess_exec
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


def _parse_all_fpga_ver(data) -> Dict:
    """Example output:
    ------SUP-FPGA------
    SUP_FPGA: 3.8
    ------SCD-FPGA------
    SCD_FPGA: 14.4
    ------PIM-FPGA------
    PIM 1 : 8.2
    PIM 2 : 8.2
    PIM 3 : 8.2
    PIM 4 : 8.2
    PIM 5 : 8.2
    PIM 6 : 8.2
    PIM 7 : 8.2
    PIM 8 : 8.2

    Will return a dict of the form:
    {"PIM 1": "8.2",
    "PIM 2": "8.2",
    "PIM 3": "8.2",
    ...
    "SUP": "3.8",
    "SCD": "14.4"}
    """
    rs = {}
    pim = 0
    pim_rx = re.compile("^PIM ([1-8])\s*:\s*(\S+)")
    fpga_rx = re.compile("([a-zA-Z]+)[_ ]FPGA\s*:\s*(\S+)")
    for l in data.splitlines():
        m = pim_rx.match(l)
        if m:
            pim = m.group(1)
            rs["PIM " + pim] = m.group(2)
            continue
        m = fpga_rx.match(l)
        if m:
            rs[m.group(1)] = m.group(2)

    return rs


async def get_all_fw_ver() -> Dict:
    cmd = "/usr/local/bin/fpga_ver.sh"
    proc = await create_subprocess_exec(cmd, stdout=PIPE)
    data, _ = await proc.communicate()
    await proc.wait()
    return _parse_all_fpga_ver(data.decode(errors="ignore"))
