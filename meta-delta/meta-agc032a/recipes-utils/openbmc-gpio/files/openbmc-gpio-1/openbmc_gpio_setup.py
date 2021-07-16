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

from board_gpio_table_v1 import board_gpio_table_v1_delta
from soc_gpio_table import soc_gpio_table
from openbmc_gpio_table import setup_board_gpio
from soc_gpio import soc_get_register

import openbmc_gpio
import sys
import os


def set_register():

    """
    # Error message when booting.

    # ERROR:root:Failed to configure "GPIOM6" for "GPIO_M6": Not able to unsatisfy an AND condition
    # ERROR:root:Failed to configure "GPIOM7" for "GPIO_M7": Not able to unsatisfy an AND condition
    # SET SCU84[31:30]=0

    """
    """
    In order to fix the error
    set the specific bit in the register so the framework can handle it.
    """
    l_reg = soc_get_register(0x84)
    l_reg.clear_bit(30, write_through=True)
    l_reg = soc_get_register(0x84)
    l_reg.clear_bit(31, write_through=True)

    """
    # ERROR:root:Pin "A12" has no function set up. All possibile functions are SD1CMD:SCU90[[0]]==0x1, SDA10:SCU90[[23]]==0x1.
    # ERROR:root:Pin "B11" has no function set up. All possibile functions are SD1WP#:SCU90[[0]]==0x1, SDA13:SCU90[[26]]==0x1.
    # ERROR:root:Pin "B12" has no function set up. All possibile functions are SD1DAT0:SCU90[[0]]==0x1, SCL11:SCU90[[24]]==0x1.
    # ERROR:root:Pin "C11" has no function set up. All possibile functions are SD1CD#:SCU90[[0]]==0x1, SCL13:SCU90[[26]]==0x1.
    # ERROR:root:Pin "C12" has no function set up. All possibile functions are SD1CLK:SCU90[[0]]==0x1, SCL10:SCU90[[23]]==0x1.
    # ERROR:root:Pin "D10" has no function set up. All possibile functions are SD1DAT2:SCU90[[0]]==0x1, SCL12:SCU90[[25]]==0x1.
    # ERROR:root:Pin "D9" has no function set up. All possibile functions are SD1DAT1:SCU90[[0]]==0x1, SDA11:SCU90[[24]]==0x1.
    # ERROR:root:Pin "E12" has no function set up. All possibile functions are SD1DAT3:SCU90[[0]]==0x1, SDA12:SCU90[[25]]==0x1.

    # SET SCU90[23:26]=1
    """
    # l_reg = soc_get_register(0x90)
    # l_reg.set_bit(23, write_through=True)
    # l_reg = soc_get_register(0x90)
    # l_reg.set_bit(24, write_through=True)
    # l_reg = soc_get_register(0x90)
    # l_reg.set_bit(25, write_through=True)
    # l_reg = soc_get_register(0x90)
    # l_reg.set_bit(26, write_through=True)


def agc032a_board_rev():
    stream = os.popen("head -n 1 /sys/bus/i2c/devices/7-0032/board_id")
    board_id = stream.read()
    if stream.close():
        return None
    else:
        return board_id


def agc032a_board_type():
    stream = os.popen("head -n 1 /sys/bus/i2c/devices/7-0032/board_ver")
    board_ver = stream.read()
    if stream.close():
        return None
    else:
        return board_ver


def main():
    print("Setting up GPIOs ... \n", end="")
    sys.stdout.flush()
    openbmc_gpio.gpio_shadow_init()
    version = agc032a_board_rev()
    brd_type = agc032a_board_type()
    # In order to satisy/unsatisfy conditions in setup_board_gpio()
    # modify the registers
    set_register()

    print("Board version %s \nBoard type %s. \nUsing Delta GPIO table.\n" % (version, brd_type), end="")
    setup_board_gpio(soc_gpio_table, board_gpio_table_v1_delta)
    print("Done")
    sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
