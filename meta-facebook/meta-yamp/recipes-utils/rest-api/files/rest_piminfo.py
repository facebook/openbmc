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

from rest_pim_present import check_pim_presence
from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for getting PIM info
# So far, YAMP has only 16Qs.
pim_number_re = re.compile("\s*PIM (\S.*) : (\S.*)")


def prepare_piminfo():
    pim_type = {
        "1": "NA",
        "2": "NA",
        "3": "NA",
        "4": "NA",
        "5": "NA",
        "6": "NA",
        "7": "NA",
        "8": "NA",
    }
    pim_fpga_ver = {
        "1": "NA",
        "2": "NA",
        "3": "NA",
        "4": "NA",
        "5": "NA",
        "6": "NA",
        "7": "NA",
        "8": "NA",
    }
    current_pim = 0
    proc = subprocess.Popen(
        ["/usr/local/bin/fpga_ver.sh", "-u"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        data, err = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
        for text_line_bytes in data.split(b"\n"):
            text_line = text_line_bytes.decode()
            # Check if this line shows PIM slot number
            m = pim_number_re.match(text_line, 0)
            if m:
                current_pim = m.group(1)
                pim_ver = m.group(2)
                # FPGA returns 0x0 if pim is not present
                # Old YAMP FPGA has a bug that returns
                # wrong pim_version. So, use pim_presence information
                # to make sure we are returning the correct value
                if pim_ver != "0x0" and check_pim_presence(int(current_pim)):
                    pim_type[str(current_pim)] = "16Q"
                    pim_fpga_ver[str(current_pim)] = pim_ver
    except Exception:
        pass
    return pim_type, pim_fpga_ver


def prepare_pimver():
    pim_ver = {str(i): "NA" for i in range(1, 9)}
    for pim in pim_ver:
        proc = subprocess.Popen(
            ["/usr/local/bin/peutil", pim],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        pim_version_str = None
        counter = 0
        try:
            data, err = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
            # pim_ver[pim] will be represented as:
            # <Product Production State>.<Product Version>.<Product sub_version>
            for text_line_bytes in data.split(b"\n"):
                text_line = text_line_bytes.decode()
                if "Production" in text_line:
                    _, version = text_line.split(":")
                    pim_version_str = version.strip()
                    pim_version_str += "."
                elif "Version" in text_line:
                    _, version = text_line.split(":")
                    if counter == 1:
                        pim_version_str += version.strip()
                        pim_version_str += "."
                    elif counter == 2:
                        pim_version_str += version.strip()
                    counter = counter + 1
            pim_ver[pim] = pim_version_str
        except Exception:
            pass
    return pim_ver


def get_piminfo():
    result = {}
    pim_type, pim_fpga_ver = prepare_piminfo()
    pim_ver = prepare_pimver()
    for pim_number in range(1, 9):
        result["PIM" + str(pim_number)] = {
            "type": pim_type[str(pim_number)],
            "fpga_ver": pim_fpga_ver[str(pim_number)],
            "ver": pim_ver[str(pim_number)],
        }
    fresult = {"Information": result, "Actions": [], "Resources": []}
    return fresult
