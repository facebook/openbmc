#!/usr/bin/env python3
#
# Copyright 2026-present Facebook. All Rights Reserved.
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
# GPIO names configured by the Aristabmc setup-gpio.sh script.  The base test
# checks that gpio-setup exported each name into /tmp/gpionames.

INPUT_GPIOS = (
    "VRHOT_L",
    "CPU_MSMI_L",
    "CPU_RESET_L",
    "CP_PWR_ON",
    "DPM_CP_PGOOD",
    "CPLD_2_BMC_INTR",
    "CPU_CATERR_L",
    "BMC_MODE",
    "EPHY_INT_L",
    "SICSID_ALERT_L",
    "OVERTEMP_L",
    "USB_DONGLE_PRSNT",
    "SYS_RESET",
    "BMC_WDTRST1",
    "CPU_OT_L",
    "DPM_APU_SYS_PWRGOOD",
    "CPU_OVER_TEMP",
    "MSW_INTR_L",
    "SWC_CP_PWR_OK",
    "SPI_ROM_REQ",
)

OUTPUT_GPIOS = (
    "SW_CPLD_JTAG_SEL",
    "AST_IDPROM_WP",
    "BUF_BFLSH_WP_L",
    "BMC_ALIVE",
    "CPU_JTAG_SEL",
    "ABOOT_GRAB",
    "SW_SPI_WP_L",
    "SW_SPI_HOLD_L",
    "BMC_LITE_L",
    "BMC_SPI_2_CPLD",
)

# Input levels and output values can change with the attached CPU/switch and
# runtime services.  Direction is the stable setup-gpio contract.
GPIOS = {name: {"direction": "in"} for name in INPUT_GPIOS}
GPIOS.update({name: {"direction": "out"} for name in OUTPUT_GPIOS})
