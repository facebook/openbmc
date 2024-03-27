#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms,Inc. and affiliates. (http://www.meta.com)
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not,write to the
# Free Software Foundation,Inc.,
# 51 Franklin Street,Fifth Floor,
# Boston,MA 02110-1301 USA
#

GPIOS = {
    "FM_BMC_READY_R_L": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 0,
    },
    "BMC_I2C1_EN": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 0,
    },
    "BMC_I2C2_EN": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 0,
    },
    "MUX_JTAG_SEL_0": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 0,
    },
    "MUX_JTAG_SEL_1": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 0,
    },
    "IOB_FLASH_SEL": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 0,
    },
    "SPI_MUX_SEL": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 0,
    },
}
