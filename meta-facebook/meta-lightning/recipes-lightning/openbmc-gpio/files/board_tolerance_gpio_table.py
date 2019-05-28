# Copyright 2015-present Facebook. All rights reserved.
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
from __future__ import absolute_import, division, print_function, unicode_literals

from openbmc_gpio_table import BoardGPIO


# The fallowing table is generated using:
# python lightning_gpio_parse.py data/lightning-BMC-GPIO-DVT.csv
# DO NOT MODIFY THE TABLE!!!
# Manual modification will be overridden!!!

board_tolerance_gpio_table = [
    BoardGPIO("GPIOE0", "FLA1_WPN", "high"),
    BoardGPIO("GPIOG0", "HIGH_HEX_0", "low"),
    BoardGPIO("GPIOG1", "HIGH_HEX_1", "high"),
    BoardGPIO("GPIOG2", "HIGH_HEX_2", "low"),
    BoardGPIO("GPIOG3", "HIGH_HEX_3", "high"),
    BoardGPIO("GPIOJ0", "LOW_HEX_0", "high"),
    BoardGPIO("GPIOJ1", "LOW_HEX_1", "low"),
    BoardGPIO("GPIOJ2", "LOW_HEX_2", "high"),
    BoardGPIO("GPIOJ3", "LOW_HEX_3", "low"),
    BoardGPIO("GPIOP3", "BMC_UART_SWITCH_1", "low"),
]
