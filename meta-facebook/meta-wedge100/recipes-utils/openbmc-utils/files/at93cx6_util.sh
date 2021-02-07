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

# shellcheck disable=SC1091
. /usr/local/bin/openbmc-utils.sh

devmem_clear_bit "$(scu_addr 80)" 18
devmem_clear_bit "$(scu_addr 8C)" 13
devmem_clear_bit "$(scu_addr 70)" 22

if [ ! -L "${SHADOW_GPIO}/BMC_EEPROM1_SPI_SS" ]; then
    gpio_export_by_name "$ASPEED_GPIO" GPIOI4 BMC_EEPROM1_SPI_SS
    gpio_export_by_name "$ASPEED_GPIO" GPIOI5 BMC_EEPROM1_SPI_SCK
    gpio_export_by_name "$ASPEED_GPIO" GPIOI6 BMC_EEPROM1_SPI_MOSI
    gpio_export_by_name "$ASPEED_GPIO" GPIOI7 BMC_EEPROM1_SPI_MISO
fi

gpio_set_value SWITCH_EEPROM1_WRT 1

/usr/local/bin/at93cx6_util_py3.py --cs BMC_EEPROM1_SPI_SS \
                                   --clk BMC_EEPROM1_SPI_SCK \
                                   --mosi BMC_EEPROM1_SPI_MOSI \
                                   --miso BMC_EEPROM1_SPI_MISO $@
