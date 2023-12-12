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

# Config for GPIOZ7
devmem_clear_bit "$(scu_addr 438)" 15
devmem_clear_bit "$(scu_addr 510)" 27
devmem_clear_bit "$(scu_addr 4d8)" 15
devmem_set_bit "$(scu_addr 4b8)" 3

# Exporting and setting direction of GPIO7 for MUX access
gpio_export_by_name  "${ASPEED_GPIO}" GPIOZ7 BMC_SPI1_MUX
gpio_set_direction BMC_SPI1_MUX out
