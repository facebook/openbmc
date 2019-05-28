#!/usr/bin/env python
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
#

import argparse
import os
import sys


_GPIO_SHADOW_DIR = "/tmp/gpionames"


class Log_Simple:
    def __init__(self, verbose=False):
        self._verbose = verbose

    def verbose(self, message):
        if self._verbose:
            print(message)

    def info(self, message):
        print(message)


def read_pin_info(pin_dir):
    pin_info = {}
    for filename in os.listdir(pin_dir):
        pathname = os.path.join(pin_dir, filename)
        if not os.path.isfile(pathname):
            continue

        with open(pathname) as fhandle:
            value = fhandle.read()
        pin_info[filename] = str(value).strip()

    return pin_info


def load_gpio_list(logger, pin_details=False):
    gpio_info = {}
    for filename in os.listdir(_GPIO_SHADOW_DIR):
        pathname = os.path.join(_GPIO_SHADOW_DIR, filename)
        if not os.path.islink(pathname):
            continue

        logger.verbose("parsing gpio pin %s" % filename)
        if pin_details:
            pin_info = read_pin_info(pathname)
        else:
            pin_info = {}
        gpio_info[filename] = pin_info

    return gpio_info


def dump_gpio_py_format(gpio_info, filename, logger):
    logger.verbose("writing gpio info to %s.." % filename)

    with open(filename, "w") as fp:
        fp.write("plat_gpio_list = {\n")
        for pin_name, pin_info in gpio_info.items():
            fp.write("    '%s': {\n" % pin_name)
            for key, value in pin_info.items():
                fp.write("        '%s': '%s',\n" % (key, value))
            fp.write("    },\n")
        fp.write("}\n")


def dump_gpio_json_format(gpio_info, filename, logger):
    logger.verbose("writing gpio info to %s.." % filename)

    with open(filename, "w") as fp:
        fp.write("{\n")
        for pin_name, pin_info in gpio_info.items():
            fp.write("    '%s': {\n" % pin_name)
            for key, value in pin_info.items():
                fp.write("        '%s': '%s',\n" % (key, value))
            fp.write("    },\n")
        fp.write("}\n")


def dump_summary(gpio_info, logger):
    total = len(gpio_info.keys())
    logger.info("Total %d gpio pins exported" % total)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="increase output verbosity",
    )
    parser.add_argument(
        "-d",
        "--pin-details",
        action="store_true",
        default=False,
        help="read details of each gpio pin",
    )
    parser.add_argument(
        "-j",
        "--json-format",
        action="store_true",
        default=False,
        help="generate gpio info in json format",
    )
    parser.add_argument("outfile", action="store")
    args = parser.parse_args()

    logger = Log_Simple(verbose=args.verbose)

    gpio_info = load_gpio_list(logger, pin_details=args.pin_details)

    if args.json_format:
        dump_gpio_json_format(gpio_info, args.outfile, logger)
    else:
        dump_gpio_py_format(gpio_info, args.outfile, logger)

    dump_summary(gpio_info, logger)
    logger.info("Commmand completed successfully!")
    sys.exit(0)
