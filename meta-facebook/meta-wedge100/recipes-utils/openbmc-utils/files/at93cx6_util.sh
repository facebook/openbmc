#!/bin/bash
#
# Copyright 2020-present Facebook. All rights reserved.
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

#
# This is the wrapper of "at93cx6_util_py3.py": people should call this
# wrapper instead of running "at93cx6_util_py3.py" directly, because:
#   - the wrapper ensures correct SS/SCK/MISO/MOSI pins are used.
#   - the wrapper ensures single instance of eeprom access, which avoids
#     potential eeprom corruption caused by concurrent eeprom access.
#

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

LOCK_FILE="/var/run/at93cx6_access.lock"

at93cx6_cleanup() {
    gpio_set_value SWITCH_EEPROM1_WRT 0
}

#
# Ensure a single instance of at93cx6_util; otherwise, the GPIO pins
# could be toggled by another instance unexpectedly.
#
exec 99>"$LOCK_FILE"
if ! flock -n 99; then
    echo "Another instance is running. Exiting!!"
    exit 1
fi

trap at93cx6_cleanup INT TERM QUIT EXIT

if [ ! -L "${SHADOW_GPIO}/BMC_EEPROM1_SPI_SS" ]; then
    gpio_export_by_name "$ASPEED_GPIO" GPIOI4 BMC_EEPROM1_SPI_SS
    gpio_export_by_name "$ASPEED_GPIO" GPIOI5 BMC_EEPROM1_SPI_SCK
    gpio_export_by_name "$ASPEED_GPIO" GPIOI6 BMC_EEPROM1_SPI_MOSI
    gpio_export_by_name "$ASPEED_GPIO" GPIOI7 BMC_EEPROM1_SPI_MISO
fi

#
# Below "devmem_clear_bit" operations ensure GPIOE2 (SWITCH_EEPROM1_WRT)
# is configured as GPIO correctly. The pinctrl should be handled when the
# GPIO pin was exported, but let's just keep it as is for now.
#
devmem_clear_bit "$(scu_addr 80)" 18
devmem_clear_bit "$(scu_addr 8C)" 13
devmem_clear_bit "$(scu_addr 70)" 22
gpio_set_value SWITCH_EEPROM1_WRT 1

/usr/local/bin/at93cx6_util_py3.py --cs BMC_EEPROM1_SPI_SS \
                                   --clk BMC_EEPROM1_SPI_SCK \
                                   --mosi BMC_EEPROM1_SPI_MOSI \
                                   --miso BMC_EEPROM1_SPI_MISO $@
