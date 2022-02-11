#!/usr/bin/env python3
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

import sys

import openbmc_gpio
from board_gpio_table import board_gpio_table
from board_passthrough_gpio_table import board_passthrough_gpio_table
from board_tolerance_gpio_table import board_tolerance_gpio_table
from openbmc_gpio_table import (
    BitsEqual,
    Function,
    setup_board_gpio,
    setup_board_tolerance_gpio,
)
from soc_gpio import soc_get_register
from soc_gpio_table import soc_gpio_table


GPIO_Tolerant_AtoD = 0x1E78001C
GPIO_Tolerant_EtoH = 0x1E78003C
GPIO_Tolerant_ItoL = 0x1E7800AC
GPIO_Tolerant_MtoP = 0x1E7800FC
GPIO_Tolerant_QtoT = 0x1E78012C
GPIO_Tolerant_UtoX = 0x1E78015C
GPIO_Tolerant_YtoAB = 0x1E78018C

board_gpioOffsetDic = {
    "0": GPIO_Tolerant_AtoD,
    "1": GPIO_Tolerant_EtoH,
    "2": GPIO_Tolerant_ItoL,
    "3": GPIO_Tolerant_MtoP,
    "4": GPIO_Tolerant_QtoT,
    "5": GPIO_Tolerant_UtoX,
    "6": GPIO_Tolerant_YtoAB,
}


soc_passthrough_gpio_table = {
    "B22": [
        Function("SPICS0#", BitsEqual(0x70, [12], 0x1)),
        Function("VBCS#", BitsEqual(0x70, [5], 0x1)),
        Function("GPIOI4", None),
    ],
    "G19": [
        Function("SPICK", BitsEqual(0x70, [12], 0x1)),
        Function("VBCK", BitsEqual(0x70, [5], 0x1)),
        Function("GPIOI5", None),
    ],
    "C18": [
        Function("SPIDO", BitsEqual(0x70, [12], 0x1)),
        Function("VBDO", BitsEqual(0x70, [5], 0x1)),
        Function("GPIOI6", None),
    ],
    "E20": [
        Function("SPIDI", BitsEqual(0x70, [12], 0x1)),
        Function("VBDI", BitsEqual(0x70, [5], 0x1)),
        Function("GPIOI7", None),
    ],
}


def set_register():
    """
    For DVT/EVT boards the framework is not able to handle for GPIOS0 and
    causes error : 'Failed to configure "GPIOS0" for "BMC_SPI_WP_N":
    Not able to unsatisfy an AND condition'. In order to fix the error
    set the specific bit in the register so the framework can handle it.
    """
    l_reg = soc_get_register(0x8C)
    l_reg.clear_bit(0, write_through=True)


def main():
    print("Setting up GPIOs ... ", end="")
    sys.stdout.flush()
    openbmc_gpio.gpio_shadow_init()

    setup_board_gpio(soc_gpio_table, board_gpio_table)
    setup_board_gpio(soc_passthrough_gpio_table, board_passthrough_gpio_table)
    setup_board_tolerance_gpio(board_tolerance_gpio_table, board_gpioOffsetDic)
    print("Done")
    sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
