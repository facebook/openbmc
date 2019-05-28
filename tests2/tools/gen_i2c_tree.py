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

from utils.i2c_utils import I2cSysfsUtils


class Log_Simple:
    def __init__(self, verbose=False):
        self._verbose = verbose

    def verbose(self, message):
        if self._verbose:
            print(message)

    def info(self, message):
        print(message)


def load_i2c_tree(logger):
    i2c_tree = {}
    for filename in os.listdir(I2cSysfsUtils.i2c_device_dir()):
        if not I2cSysfsUtils.is_i2c_device_entry(filename):
            continue

        logger.verbose("parsing i2c device %s" % filename)
        dev_info = {}
        dev_info["name"] = I2cSysfsUtils.i2c_device_get_name(filename)
        dev_info["driver"] = I2cSysfsUtils.i2c_device_get_driver(filename)
        i2c_tree[filename] = dev_info

    return i2c_tree


def dump_i2c_py_format(i2c_tree, filename, logger):
    logger.verbose("writing i2c tree to %s.." % filename)

    with open(filename, "w") as fp:
        fp.write("plat_i2c_tree = {\n")
        for dev_name, info in i2c_tree.items():
            fp.write("    '%s': {\n" % dev_name)
            fp.write("        'name': '%s',\n" % info["name"])
            fp.write("        'driver': '%s',\n" % info["driver"])
            fp.write("    },\n")
        fp.write("}\n")


def dump_i2c_json_format(i2c_tree, filename, logger):
    logger.verbose("writing i2c tree to %s.." % filename)

    with open(filename, "w") as fp:
        fp.write("{\n")
        for dev_name, info in i2c_tree.items():
            fp.write("    '%s': {\n" % dev_name)
            fp.write("        'name': '%s',\n" % info["name"])
            fp.write("        'driver': '%s',\n" % info["driver"])
            fp.write("    },\n")
        fp.write("}\n")


def dump_summary(i2c_tree, logger):
    bindings = 0
    dev_list = []
    for dev_name, info in i2c_tree.items():
        if info["driver"] != "":
            bindings += 1
        else:
            dev_list.append("%s (name=%s)" % (dev_name, info["name"]))

    total = len(i2c_tree.keys())
    total_msg = "Total %d i2c devices" % total
    if total == bindings:
        bind_msg = "all of them are binded with drivers"
    else:
        bind_msg = "%d of them are binded with drivers" % bindings
    logger.info("%s: %s" % (total_msg, bind_msg))

    if dev_list:
        logger.info("List of devices without drivers:")
        for entry in dev_list:
            logger.info("\t%s" % entry)


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
        "-j",
        "--json-format",
        action="store_true",
        default=False,
        help="generate i2c tree in json format",
    )
    parser.add_argument("outfile", action="store")
    args = parser.parse_args()

    logger = Log_Simple(verbose=args.verbose)

    i2c_tree = load_i2c_tree(logger)

    if args.json_format:
        dump_i2c_json_format(i2c_tree, args.outfile, logger)
    else:
        dump_i2c_py_format(i2c_tree, args.outfile, logger)

    dump_summary(i2c_tree, logger)
    logger.info("Commmand completed successfully!")
    sys.exit(0)
