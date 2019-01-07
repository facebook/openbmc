#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All rights reserved.
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
from board_gpio_table_p1 import board_gpio_table_p1
from soc_gpio_table import soc_gpio_table
from openbmc_gpio_table import setup_board_gpio
from soc_gpio import soc_get_register

import openbmc_gpio
import sys


def yamp_board_rev(soc_gpio_table, board_gpio_rev_table):
    return 1

def main():
    print('Setting up GPIOs ... ', end='')
    sys.stdout.flush()
    openbmc_gpio.gpio_shadow_init()
    version = yamp_board_rev(soc_gpio_table,board_gpio_rev_table)
    # In order to satisy/unsatisfy conditions in setup_board_gpio()
    # modify the registers
    if version is 1:
        print('Using GPIO P1 table ', end='')
        setup_board_gpio(soc_gpio_table, board_gpio_table_p1)
    else:
        print('Unexpected board version %s. Using GPIO P1 table. '
              % version, end='')
        setup_board_gpio(soc_gpio_table, board_gpio_table_p2)
    print('Done')
    sys.stdout.flush()
    return 0


if __name__ == '__main__':
    sys.exit(main())
