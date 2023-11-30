#!/usr/bin/env python3
#
# Copyright 2023-present Facebook. All Rights Reserved.
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

GPIOS = {
    "ABOOT_GRAB": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "BMC_ALIVE": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "BMC_EMMC_RST": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "BMC_LITE_L": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "CPLD_2_BMC_INT": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "CPU_CATERR_L": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "CPU_MSMI_L": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "CPU_RST_L": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "CP_PWRGD": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SCM_OVER_TEMP": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SCM_PWRGD": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SCM_TEMP_ALERT": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SMB_PWRGD": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SW_JTAG_SEL": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SW_CPLD_JTAG_SEL":{
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SW_SPI_SEL": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "USB_DONGLE_PRSNT":{
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "WDT1_RST":{
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    }
}