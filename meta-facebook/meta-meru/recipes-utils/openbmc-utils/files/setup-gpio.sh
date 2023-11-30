#!/bin/bash
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

### BEGIN INIT INFO
# Provides:          gpio-setup
# Required-Start:
# Required-Stop:
# Default-Start:     S
# Default-Stop:
# Short-Description:  Set up GPIO pins as appropriate
### END INIT INFO

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

setup_gpio_meru_evt() {
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB0 ABOOT_GRAB
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB1 BMC_ALIVE
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB2 CPU_MSMI_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB3 CP_PWRGD
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB4 SCM_OVER_TEMP
    # gpio_export_by_name  "${ASPEED_GPIO}" GPIOB5 GBESW_INT_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB6 SCM_PWRGD
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB7 SMB_PWRGD
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOD2 SW_JTAG_SEL
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOD3 SW_CPLD_JTAG_SEL
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI0 JTAG_TRST_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP1 CPU_CATERR_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOP2 USB_DONGLE_PRSNT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOS7 CPLD_2_BMC_INT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV0 SW_SPI_SEL
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV1 CPU_RST_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY0 WDT1_RST
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY2 SCM_TEMP_ALERT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY3 BMC_EMMC_RST
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ2 BMC_LITE_L
}

setup_gpio_meru_evt

gpio_set_direction ABOOT_GRAB out
gpio_set_direction BMC_ALIVE out
gpio_set_direction CPU_MSMI_L in
gpio_set_direction CP_PWRGD in
gpio_set_direction SCM_OVER_TEMP in
# gpio_set_direction GBESW_INT_L in
gpio_set_direction SCM_PWRGD in
gpio_set_direction SMB_PWRGD in
gpio_set_direction SW_JTAG_SEL out
gpio_set_direction SW_CPLD_JTAG_SEL out
gpio_set_direction JTAG_TRST_L out
gpio_set_direction CPU_CATERR_L in
gpio_set_direction USB_DONGLE_PRSNT in
gpio_set_direction CPLD_2_BMC_INT in
gpio_set_direction SW_SPI_SEL out
gpio_set_direction CPU_RST_L in
gpio_set_direction WDT1_RST out
gpio_set_direction SCM_TEMP_ALERT in
gpio_set_direction BMC_EMMC_RST out
gpio_set_direction BMC_LITE_L out

# Once we set "out", output values will be random unless we set them
# Set Default GPIO values
# Set BMC_EMMC_RST High to prevent EMMC disable
gpio_set BMC_EMMC_RST 1

gpio_set_value ABOOT_GRAB 0
gpio_set_value BMC_ALIVE 1
gpio_set_value SW_JTAG_SEL 0
gpio_set_value SW_CPLD_JTAG_SEL 0
gpio_set_value JTAG_TRST_L 0
gpio_set_value SW_SPI_SEL 0
gpio_set_value WDT1_RST 0
gpio_set_value BMC_LITE_L 0
