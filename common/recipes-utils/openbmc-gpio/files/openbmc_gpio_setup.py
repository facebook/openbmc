#!/usr/bin/python -tt
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
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from board_gpio_table import board_gpio_table
from soc_gpio_table import soc_gpio_table

import openbmc_gpio
import openbmc_gpio_table

import logging
import sys


def setup_board_gpio(soc_gpio_table, board_gpio_table, validate=True):
    soc = openbmc_gpio_table.SocGPIOTable(soc_gpio_table)
    gpio_configured = []
    for gpio in board_gpio_table:
        try:
            soc.config_function(gpio.gpio, write_through=False)
            gpio_configured.append(gpio.gpio)
        except openbmc_gpio_table.ConfigUnknownFunction as e:
            # not multiple-function GPIO pin
            pass
        except openbmc_gpio_table.NotSmartEnoughException as e:
            logging.error('Failed to configure "%s" for "%s": %s'
                          % (gpio.gpio, gpio.shadow, str(e)))

    soc.write_to_hw()

    if validate:
        all_functions = set(soc.get_active_functions(refresh=True))
        for gpio in gpio_configured:
            if gpio not in all_functions:
                raise Exception('Failed to configure function "%s"' % gpio)

    for gpio in board_gpio_table:
        openbmc_gpio.gpio_export(gpio.gpio, gpio.shadow)
        if gpio.value == openbmc_gpio_table.GPIO_INPUT:
            continue
        elif gpio.value == openbmc_gpio_table.GPIO_OUT_HIGH:
            openbmc_gpio.gpio_set(gpio.gpio, 1)
        elif gpio.value == openbmc_gpio_table.GPIO_OUT_LOW:
            openbmc_gpio.gpio_set(gpio.gpio, 0)
        else:
            raise Exception('Invalid value "%s"' % gpio.value)

def main():
    print('Setting up GPIOs ... ', end='')
    sys.stdout.flush()
    openbmc_gpio.setup_shadow()
    setup_board_gpio(soc_gpio_table, board_gpio_table)
    print('Done')
    sys.stdout.flush()
    return 0

if __name__ == '__main__':
    sys.exit(main())
