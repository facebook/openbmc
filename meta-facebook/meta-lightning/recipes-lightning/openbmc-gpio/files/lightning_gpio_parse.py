# Copyright 2015-present Facebook. All Rights Reserved.
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

import argparse
import csv
import logging
import re
import sys


GPIO_SYMBOL = "BoardGPIO"
GPIO_TOLERANCE_LIST = [
    "GPIOE0",
    "GPIOG0",
    "GPIOG1",
    "GPIOG2",
    "GPIOG3",
    "GPIOJ0",
    "GPIOJ1",
    "GPIOJ2",
    "GPIOJ3",
    "GPIOP3",
]
GPIO_PASSTHROUGH_LIST = ["SPICS0#", "SPICK", "SPIDO", "SPIDI"]


class CsvReader:
    """
    A class for parsing the CSV files containing the board GPIO config
    """

    def __init__(self, path):
        self.path = path

        fileobj = open(path, "r")
        self.reader = csv.reader(fileobj, delimiter=b",", quotechar=b'"')

    def next(self):
        try:
            line = self.reader.next()
        except StopIteration:
            return None
        return line


def ParsePinFunc(loc):
    switcher = {"": 0, "Default": 0, "Function 1": 1, "Function 2": 2}
    return switcher.get(loc, None)


class LightningGPIO(object):
    def __init__(self, data):
        self.data = data
        self.gpios = {}
        self.tgpios = {}
        self.ptgpios = {}
        self.names = set()

    def parse(self):
        while True:
            line = self.data.next()
            if line is None:
                break

            logging.debug("Parsing line: %s" % line)

            if len(line) < 4:
                logging.error('No enough fields in "%s". Skip!' % line)
                continue

            gpio = None
            pos = ParsePinFunc(line[3])
            if pos is not None:
                print("pos = %s" % pos)
                print("line[4] = %s" % line[4])
                gpio = line[4].split("_")[pos]

            if gpio is None:
                logging.error('Cannot find GPIO file from "%s". Skip!' % line)
                continue

            name = line[5]
            direction = None
            if line[1] == "OUT":
                direction = line[2]
            print("name = %s" % name)
            # assert gpio not in self.gpios and name not in self.names
            assert gpio not in self.gpios
            assert name not in self.names
            if gpio in GPIO_TOLERANCE_LIST:
                self.tgpios[gpio] = name, direction
            elif gpio in GPIO_PASSTHROUGH_LIST:
                self.ptgpios[gpio] = name, direction
            else:
                self.gpios[gpio] = name, direction
            self.names.add(name)

    def print(self, out):
        for gpio, info in sorted(self.gpios.items()):
            if info[1] is None:
                out.write("    %s('%s', '%s'),\n" % (GPIO_SYMBOL, gpio, info[0]))
            else:
                out.write(
                    "    %s('%s', '%s', '%s'),\n"
                    % (GPIO_SYMBOL, gpio, info[0], info[1].lower())
                )

    def print_tolerance_gpio(self, out):
        for tgpio, info in sorted(self.tgpios.items()):
            if info[1] is None:
                out.write("    %s('%s', '%s'),\n" % (GPIO_SYMBOL, tgpio, info[0]))
            else:
                out.write(
                    "    %s('%s', '%s', '%s'),\n"
                    % (GPIO_SYMBOL, tgpio, info[0], info[1].lower())
                )

    def print_passthrough_gpio(self, out):
        for tgpio, info in sorted(self.ptgpios.items()):
            if info[1] is None:
                out.write("    %s('%s', '%s'),\n" % (GPIO_SYMBOL, tgpio, info[0]))
            else:
                out.write(
                    "    %s('%s', '%s', '%s'),\n"
                    % (GPIO_SYMBOL, tgpio, info[0], info[1].lower())
                )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("data", help="The GPIO data file")
    args = ap.parse_args()

    logging.basicConfig(level=logging.INFO, format="%(asctime)s: %(message)s")

    gpio = LightningGPIO(CsvReader(args.data))
    gpio.parse()
    f = open("output.txt", "w")
    sys.stdout = f
    gpio.print(sys.stdout)
    ft = open("t_output.txt", "w")
    sys.stdout = ft
    gpio.print_tolerance_gpio(sys.stdout)
    fpt = open("pt_output.txt", "w")
    sys.stdout = fpt
    gpio.print_passthrough_gpio(sys.stdout)


rc = main()
sys.exit(rc)
