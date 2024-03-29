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

GPIOS = {
    "BMC_JTAG_MUX_IN": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "BMC_UART_SEL_0": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "BMC_FCM_1_SEL": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "FCM_2_CARD_PRESENT": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "FCM_1_CARD_PRESENT": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "BMC_SCM_CPLD_JTAG_EN_N": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "FCM_2_CPLD_JTAG_EN_N": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "BMC_FCM_2_SEL": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "FCM_1_CPLD_JTAG_EN_N": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "BMC_GPIO63": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "UCD90160A_CNTRL": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "BMC_UART_SEL_3": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "SYS_CPLD_JTAG_EN_N": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "BMC_I2C_SEL": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "BMC_GPIO53": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "FPGA_BMC_CONFIG_DONE": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "FPGA_NSTATUS": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "BMC_UART_SEL_1": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "LM75_ALERT": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "CPU_RST#": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "GPIO123_USB2BVBUSSNS": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "GPIO126_USB2APWREN": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "GPIO125_USB2AVBUSSNS": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "BMC_UART_SEL_2": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "BMC_GPIO61": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "UCD90160A_2_GPI1": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "UCD90160A_1_GPI1": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "PWRMONITOR_BMC": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "BMC_PWRGD": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "FPGA_DEV_CLR_N": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "FPGA_DEV_OE": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "FPGA_CONFIG_SEL": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "LPCD0": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "LPCD1": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "LPCD2": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "LPCD3": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "LPCCLK": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "LPCFRAME#": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "LPCIRQ#": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "LPCRST#": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "BMC_FPGA_JTAG_EN": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "BMC_TPM_SPI_PIRQ_N": {
        "active_low": "0",
        "direction": "in",
        "uevent": "",
        "edge": "none",
        "value": "1",
    },
    "BMC_GPIO57": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
    "BMC_EMMC_RST_N": {
        "active_low": "0",
        "direction": "out",
        "uevent": "",
        "edge": "none",
        "value": "0",
    },
}
