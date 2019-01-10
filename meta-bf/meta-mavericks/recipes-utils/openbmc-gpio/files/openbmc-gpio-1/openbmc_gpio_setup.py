#!/usr/bin/env python3
#
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

from board_gpio_rev_table import board_gpio_rev_table
from board_gpio_table_v1 import board_gpio_table_v1
from board_gpio_table_v2 import board_gpio_table_v2
from soc_gpio_table import soc_gpio_table
from openbmc_gpio_table import setup_board_gpio
from soc_gpio import soc_get_register

import openbmc_gpio
import sys


def set_register():
    '''
    For DVT/EVT boards the framework is not able to handle for GPIOS0 and
    causes error : 'Failed to configure "GPIOS0" for "BMC_SPI_WP_N":
    Not able to unsatisfy an AND condition'. In order to fix the error
    set the specific bit in the register so the framework can handle it.
    '''
    l_reg = soc_get_register(0x8C)
    l_reg.clear_bit(0, write_through=True)


def wedge_board_rev(soc_gpio_table, board_gpio_rev_table):
    # Setup to read revision
    setup_board_gpio(soc_gpio_table, board_gpio_rev_table)
    # Read the gpio values
    v0 = openbmc_gpio.gpio_get('BOARD_REV_ID0')
    v1 = openbmc_gpio.gpio_get('BOARD_REV_ID1')
    v2 = openbmc_gpio.gpio_get('BOARD_REV_ID2')
    return ((v2 << 2) | (v1 << 1) | v0)


def main():
    print('Setting up GPIOs ... ', end='')
    sys.stdout.flush()
    openbmc_gpio.setup_shadow()
    version = wedge_board_rev(soc_gpio_table,board_gpio_rev_table)
    # In order to satisy/unsatisfy conditions in setup_board_gpio()
    # modify the registers
    set_register()
    if version is 1:
        print('Using GPIO EVT table ', end='')
        setup_board_gpio(soc_gpio_table, board_gpio_table_v1)
    else:
        if version is 2:
            print('Using GPIO DVT table ', end='')
        else:
            print('Unexpected board version %s. Using GPIO DVT table. '
                % version, end='')
        setup_board_gpio(soc_gpio_table, board_gpio_table_v2)
    print('Done')
    sys.stdout.flush()
    return 0


if __name__ == '__main__':
    sys.exit(main())
