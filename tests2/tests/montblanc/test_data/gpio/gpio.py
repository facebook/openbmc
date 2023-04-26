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
    "BMC_DEBUG_JUMPER1": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "BMC_DEBUG_JUMPER2": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "BMC_DEBUG_JUMPER3": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "DEBUG_PRSNT": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "PWRGD_PCH_PWROK": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "PWR_BTN_L": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "RST_BTN_L": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "BMC_PWRGD_R": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "FM_BMC_READY_R_L": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 0,
    },
    "BMC_PTH_PWR_BTN_L": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "BMC_PTH_RST_BTN_L": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 1,
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
    "BMC_CPU_RST": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "BMC_GPIO126_USB2APWREN": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "COME_ISO_PWRGD_CPU_LVC3": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "FM_CPU_MSMI_CATERR_LVT3_BUF_N": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "ISO_FM_BMC_NMI_L": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "ISO_FM_BMC_ONCTL_L": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": 0,
    },
    "ISO_FPGA_CRC_ERROR": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
    "ISO_SMB_FPGA_VPP_ALERT_L": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": 1,
    },
}
