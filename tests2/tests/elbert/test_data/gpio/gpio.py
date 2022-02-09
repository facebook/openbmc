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
    "BMC_ALIVE": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "BMC_BOARD_EEPROM_WP_L": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "BMC_CHASSIS_EEPROM_WP_L": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "BMC_SPI1_CS0_MUX_SEL": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "BMC_TEMP_ALERT": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "CPLD_JTAG_SEL_L": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "CPU_OVERTEMP": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "CPU_TEMP_ALERT": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "I2C_TPM_PIRQ_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "JTAG_TRST_L": {
        "active_low": "0",
        "direction": "out",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SCM_FPGA_LATCH_L": {
        "active_low": "0",
        "direction": "out",
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
    "SMB_INTERRUPT": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SMB_PWRGD": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
    "SPI2CS1#_GPIO86": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SPI2CS2#_GPIO84": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "0",
    },
    "SPI_TPM_PIRQ_N": {
        "active_low": "0",
        "direction": "in",
        "edge": "none",
        "uevent": "",
        "value": "1",
    },
}


def pim_gpios(pim):
    return {
        "PIM{}_FPGA_DONE".format(pim): {
            "active_low": "0",
            "direction": "in",
            "uevent": "",
            "value": "1",
        },
        "PIM{}_FPGA_INIT".format(pim): {
            "active_low": "0",
            "direction": "in",
            "uevent": "",
            "value": "1",
        },
        "PIM{}_FPGA_RESET_L".format(pim): {
            "active_low": "0",
            "direction": "out",
            "uevent": "",
            "value": "1",
        },
        "PIM{}_FULL_POWER_EN".format(pim): {
            "active_low": "0",
            "direction": "in",
            "uevent": "",
            "value": "1",
        },
    }
