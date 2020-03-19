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

FPGA_VER_PATH = "/sys/bus/i2c/drivers/scdcpld/4-0023/{}_fpga_rev_major"

PIM_MAP = {
    "pim1": "lc1",
    "pim2": "lc2",
    "pim3": "lc3",
    "pim4": "lc4",
    "pim5": "lc5",
    "pim6": "lc6",
    "pim7": "lc7",
    "pim8": "lc8",
}


def prepare_pimstatus():
    status = {}

    for k, v in PIM_MAP.items():
        pim_path = FPGA_VER_PATH.format(v)
        try:
            with open(pim_path, "r", encoding="utf-8") as fh:
                ver = fh.readline().strip()
                status[k] = "down" if ver == "0xff" else "up"
        except Exception:
            status[k] = "unknown"

    return status


def get_pimstatus():
    status = prepare_pimstatus()
    fresult = {"Information": status, "Actions": [], "Resources": []}
    return fresult
