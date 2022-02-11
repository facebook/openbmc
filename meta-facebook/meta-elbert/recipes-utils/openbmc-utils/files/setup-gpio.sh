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

setup_gpio_elbert_evt() {
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB1 BMC_ALIVE
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB3 CPLD_JTAG_SEL_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB4 CPU_TEMP_ALERT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB6 SCM_PWRGD
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOB7 SMB_PWRGD
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOF1 I2C_TPM_PIRQ_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOH0 BMC_BOARD_EEPROM_WP_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOH1 BMC_TEMP_ALERT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOI0 JTAG_TRST_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOS7 SMB_INTERRUPT
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV0 BMC_SPI1_CS0_MUX_SEL
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOV2 BMC_CHASSIS_EEPROM_WP_L
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX1 SPI2CS1#_GPIO86
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX2 SPI2CS2#_GPIO84
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOX7 SPI_TPM_PIRQ_N
    gpio_export_by_name  "${ASPEED_GPIO}" GPIOY2 CPU_OVERTEMP
}

setup_gpio_elbert_evt

gpio_set_direction BMC_ALIVE in
gpio_set_direction CPLD_JTAG_SEL_L out
gpio_set_direction CPU_TEMP_ALERT in
gpio_set_direction SCM_PWRGD in
gpio_set_direction SMB_PWRGD in
gpio_set_direction I2C_TPM_PIRQ_N in
gpio_set_direction BMC_BOARD_EEPROM_WP_L out
gpio_set_direction BMC_TEMP_ALERT  in
gpio_set_direction SMB_INTERRUPT in
gpio_set_direction BMC_SPI1_CS0_MUX_SEL out
gpio_set_direction BMC_CHASSIS_EEPROM_WP_L out
gpio_set_direction SPI_TPM_PIRQ_N in
gpio_set_direction CPU_OVERTEMP in
gpio_set_direction JTAG_TRST_L out

# Once we set "out", output values will be random unless we set them
# Set Default GPO values
gpio_set_value CPLD_JTAG_SEL_L 1 # Disabled
gpio_set_value BMC_SPI1_CS0_MUX_SEL 0 # BIOS
gpio_set_value BMC_ALIVE 0 # Default
gpio_set_value BMC_BOARD_EEPROM_WP_L 1 # Disabled
gpio_set_value BMC_CHASSIS_EEPROM_WP_L 1 # Disabled
gpio_set_value JTAG_TRST_L 1 # Disabled
