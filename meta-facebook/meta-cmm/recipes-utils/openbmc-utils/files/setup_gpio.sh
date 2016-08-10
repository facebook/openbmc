#!/bin/sh
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

source /usr/local/bin/openbmc-utils.sh

# GPIOM0: BMC_CPLD_TMS
# GPIOM1: BMC_CPLD_TDI
# GPIOM2: BMC_CPLD_TCK
# GPIOM3: BMC_CPLD_TDO
# SCU84[24|25|26|27] must be 0
devmem_clear_bit $(scu_addr 84) 24
devmem_clear_bit $(scu_addr 84) 25
devmem_clear_bit $(scu_addr 84) 26
devmem_clear_bit $(scu_addr 84) 27
gpio_export M0 BMC_CPLD_TMS
gpio_export M1 BMC_CPLD_TDI
gpio_export M2 BMC_CPLD_TCK
gpio_export M3 BMC_CPLD_TDO


# GPIOF0: CPLD_JTAG_SEL (needs to be low)
# SCU90[30] must 0 adn SCU80[24] must be 0
devmem_clear_bit $(scu_addr 90) 30
devmem_clear_bit $(scu_addr 80) 24
gpio_export F0 CPLD_JTAG_SEL
