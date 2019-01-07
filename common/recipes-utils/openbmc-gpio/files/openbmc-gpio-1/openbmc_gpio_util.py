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





from soc_gpio_table import soc_gpio_table

import openbmc_gpio
import openbmc_gpio_table

import argparse
import logging
import sys


def _get_gpio_table():
    gpio = openbmc_gpio_table.SocGPIOTable(soc_gpio_table)
    return gpio


def dump_func(args):
    gpio = _get_gpio_table()
    gpio.dump_functions()
    return 0


def config_func(args):
    gpio = _get_gpio_table()
    try:
        gpio.config_function(args.function)
    except openbmc_gpio_table.NotSmartEnoughException as e:
        print('The code is not smart enough to set function "%s": %s\n'
              'Please set the function manually.'
              % (args.function, str(e)))
        print('The current HW setting for this function is:')
        gpio.dump_function(args.function)
        return -1
    except Exception as e:
        print('Failed to set function "%s": %s\n'
              'Please set the function manually.'
              % (args.function, str(e)))
        print('The current HW setting for this function is:')
        gpio.dump_function(args.function)
        logging.exception('Exception:')
        return -2

    print('Function "%s" is set' % args.function)
    return 0


def export_func(args):
    openbmc_gpio.gpio_export(args.gpio, args.shadow)


def set_func(args):
    openbmc_gpio.gpio_set(args.gpio, args.value,
                          change_direction=False if args.keep else True)


def get_func(args):
    val = openbmc_gpio.gpio_get(
        args.gpio, change_direction=False if args.keep else True)
    print('%d' % val)


def info_func(args):
    res = openbmc_gpio.gpio_info(args.gpio)
    print('GPIO info for %s:' % args.gpio)
    print('Shadow: %s\nDirection: %s\nValue: %s'
          % (res['shadow'], res['direction'], res['value']))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--debug', action='store_true',
                    help='Enable debug messages')

    subparser = ap.add_subparsers()

    dump_parser = subparser.add_parser(
        'dump', help='Dump the current HW GPIO settings')
    dump_parser.set_defaults(func=dump_func)

    config_parser = subparser.add_parser(
        'config', help='Configure one HW pin to a function')
    config_parser.add_argument(
        'function', help='The function name to set')
    config_parser.set_defaults(func=config_func)

    export_parser = subparser.add_parser(
        'export', help='Export a GPIO directory')
    export_parser.add_argument(
        'gpio', help='The GPIO name, i.e. "A4", or "GPIOD2"')
    export_parser.add_argument(
        'shadow', default=None,
        help='The shadow name given to this GPIO')
    export_parser.set_defaults(func=export_func)


    set_parser = subparser.add_parser(
        'set', help='Set a value for a GPIO')
    set_parser.add_argument(
        'gpio', help='The GPIO name or number')
    set_parser.add_argument(
        'value', type=int, choices=[0, 1],
        help='The value to set')
    set_parser.add_argument(
        '-k', '--keep', action='store_true',
        help='Keep the GPIO direction')
    set_parser.set_defaults(func=set_func)

    get_parser = subparser.add_parser(
        'get', help='Get a GPIO\'s value')
    get_parser.add_argument(
        'gpio', help='The GPIO name or number')
    get_parser.add_argument(
        '-k', '--keep', action='store_true',
        help='Keep the GPIO direction')
    get_parser.set_defaults(func=get_func)

    info_parser = subparser.add_parser(
        'info', help='Get a GPIO\'s info')
    info_parser.add_argument(
        'gpio', help='The GPIO name or number')
    info_parser.set_defaults(func=info_func)

    args = ap.parse_args()

    logging.basicConfig(level=logging.DEBUG if args.debug else logging.INFO,
                        format='%(asctime)s: %(message)s')

    return args.func(args)


rc = main()
sys.exit(rc)
