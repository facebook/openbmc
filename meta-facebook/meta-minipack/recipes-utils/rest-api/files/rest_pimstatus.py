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

# Handler for getting PIM status.
# fpga_ver is the source of truth for a healthy and available PIM.
# A PIM can be present (seated) however still be faulty/offline.
# This handler is the ultimate signal for PIM status.

FPGA_VER_PATH = "/sys/bus/i2c/devices/{}-0060/fpga_ver"

PIM_MAP = {
    "pim1": "80",
    "pim2": "88",
    "pim3": "96",
    "pim4": "104",
    "pim5": "112",
    "pim6": "120",
    "pim7": "128",
    "pim8": "136",
}


def prepare_pimstatus():
    status = {}

    for k, v in PIM_MAP.items():
        pim_path = FPGA_VER_PATH.format(v)
        try:
            with open(pim_path, "r") as fh:
                ver = fh.readline()
                status[k] = "down" if ver == "0xff" else "up"
        except Exception:
            status[k] = "unknown"

    return status


def get_pimstatus():
    status = prepare_pimstatus()
    fresult = {"Information": status, "Actions": [], "Resources": []}
    return fresult
