#!/usr/bin/env python3
#
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

from board_gpio_rev_table import board_gpio_rev_table
from board_gpio_table_v1 import (
    board_gpio_table_v1_th3,
    board_gpio_table_v1_gb,
    board_gpio_table_v1_gb_respin
)
from soc_gpio_table import soc_gpio_table
from openbmc_gpio_table import setup_board_gpio
from soc_gpio import soc_get_register

import openbmc_gpio
import sys
import os


def set_register():
    """
    For DVT/EVT boards the framework is not able to handle for GPIOI4 and
    causes error : 'Failed to configure "GPIOI4" for "BMC_SPI1_CS0":
    Not able to unsatisfy an AND condition'. In order to fix the error
    set the specific bit in the register so the framework can handle it.
    """
    """
    The write operation to SCU70 only can set to '1',
    to clear to '0', it must write '1' to SCU7C (write 1 clear).
    """
    l_reg = soc_get_register(0x7C)
    l_reg.set_bit(5, write_through=True)
    l_reg = soc_get_register(0x7C)
    l_reg.set_bit(12, write_through=True)


def wedge400_board_rev(soc_gpio_table, board_gpio_rev_table):
    # Setup to read revision
    setup_board_gpio(soc_gpio_table, board_gpio_rev_table)
    # Read the gpio values
    v0 = openbmc_gpio.gpio_get_value("BMC_CPLD_BOARD_REV_ID0")
    v1 = openbmc_gpio.gpio_get_value("BMC_CPLD_BOARD_REV_ID1")
    v2 = openbmc_gpio.gpio_get_value("BMC_CPLD_BOARD_REV_ID2")
    return (v2 << 2) | (v1 << 1) | v0


def wedge400_board_type():
    val = openbmc_gpio.gpio_get_value("BMC_CPLD_BOARD_TYPE_0")

    if val == 1:
        return 0
    elif val == 0:
        return 1
    else:
        return None


def main():
    print("Setting up GPIOs ... ", end="")
    sys.stdout.flush()
    openbmc_gpio.gpio_shadow_init()
    version = wedge400_board_rev(soc_gpio_table, board_gpio_rev_table)
    brd_type = wedge400_board_type()
    # In order to satisy/unsatisfy conditions in setup_board_gpio()
    # modify the registers
    set_register()

    if brd_type == 0:
        print("Using GPIO Wedge400 table ", end="")
        setup_board_gpio(soc_gpio_table, board_gpio_table_v1_th3)
    elif brd_type == 1:
        print("Using GPIO Wedge400-C table ", end="")
        if version >= 4:
            setup_board_gpio(soc_gpio_table, board_gpio_table_v1_gb_respin)
        else:
            setup_board_gpio(soc_gpio_table, board_gpio_table_v1_gb)
    else:
        print("Unexpected board version %s. Using GPIO DVT table. " % version, end="")
        setup_board_gpio(soc_gpio_table, board_gpio_table_v1_gb)
    print("Done")
    sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
