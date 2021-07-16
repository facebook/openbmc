# Copyright 2019-present Facebook. All rights reserved.
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

from openbmc_gpio_table import BoardGPIO


# The following table is generated using:
# python openbmc_gpio_parser.py data/agc032a-bmc-gpio-evta.csv
# DO NOT MODIFY THE TABLE!!!
# Manual modification will be overridden!!!

board_gpio_rev_table = [
    BoardGPIO("GPIOG4", "BMC_CPLD_BOARD_REV_ID0"),
    BoardGPIO("GPIOG5", "BMC_CPLD_BOARD_REV_ID1"),
    BoardGPIO("GPIOG6", "BMC_CPLD_BOARD_TYPE"),
    BoardGPIO("GPIOG7", "BMC_CPLD_BOARD_REV_ID2"),
]
