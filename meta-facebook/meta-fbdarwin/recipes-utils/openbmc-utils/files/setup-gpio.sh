#!/bin/bash
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

setup_gpio_fbdarwin_evt() {
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB1 BMC_ALIVE
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOC3 TPM_BFLSH_WP_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOC4 BMC_MODE
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOG6 BMC_BOARD_EEPROM_WP_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOO0 BMC_SYS_PWR_CYC0
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOO2 BMC_SYS_PWR_CYC1
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOO4 OOB_EEPROM_SPI_SEL
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOO6 OOB_RST
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX7 SPI_TPM_PIRQ_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY3 BMC_EMMC_RST
}

setup_gpio_fbdarwin_evt

gpio_set_direction BMC_ALIVE out
gpio_set_direction TPM_BFLSH_WP_L in
gpio_set_direction BMC_MODE in
gpio_set_direction BMC_BOARD_EEPROM_WP_L out
gpio_set_direction BMC_SYS_PWR_CYC0 out
gpio_set_direction BMC_SYS_PWR_CYC1 out
gpio_set_direction OOB_EEPROM_SPI_SEL out
gpio_set_direction OOB_RST out
gpio_set_direction BMC_EMMC_RST out
gpio_set_direction SPI_TPM_PIRQ_N in

# Once we set "out", output values will be random unless we set them
# Set Default GPIO values
# Set BMC_EMMC_RST High to prevent EMMC disable
gpio_set BMC_EMMC_RST 1

gpio_set_value BMC_ALIVE 1
gpio_set_value BMC_BOARD_EEPROM_WP_L 1 # Disabled
gpio_set_value BMC_SYS_PWR_CYC0 0
gpio_set_value BMC_SYS_PWR_CYC1 0
gpio_set_value OOB_EEPROM_SPI_SEL 0
gpio_set_value OOB_RST 0
