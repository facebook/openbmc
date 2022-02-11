#!/usr/bin/env python3
#
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

import argparse
import csv
import logging
import re
import sys


GPIO_SYMBOL = 'BoardGPIO'


class CsvReader:
    '''
    A class for parsing the CSV files containing the board GPIO config
    '''
    def __init__(self, path):
        self.path = path

        fileobj = open(path, 'r')
        self.reader = csv.reader(fileobj, delimiter=b',', quotechar=b'"')

    def __next__(self):
        try:
            line = next(self.reader)
        except StopIteration:
            return None
        return line


class WedgeGPIO(object):
    def __init__(self, data):
        self.data = data
        self.gpios = {}
        self.names = set()

    def parse(self):
        while True:
            line = next(self.data)
            if line is None:
                break

            logging.debug('Parsing line: %s' % line)

            if len(line) < 4:
                logging.error('No enough fields in "%s". Skip!' % line)
                continue

            gpio = None
            for part in line[1].split('_'):
                if part.startswith('GPIO'):
                    gpio = part
                    break
            if gpio is None:
                logging.error('Cannot find GPIO file from "%s". Skip!' % line)
                continue

            name = line[3]
            assert gpio not in self.gpios and name not in self.names
            self.gpios[gpio] = name
            self.names.add(name)

    def print(self, out):
        for gpio in sorted(self.gpios):
            out.write('    %s(\'%s\', \'%s\'),\n'
                      % (GPIO_SYMBOL, gpio, self.gpios[gpio]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('data', help='The GPIO data file')
    args = ap.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s: %(message)s')

    gpio = WedgeGPIO(CsvReader(args.data))
    gpio.parse()
    gpio.print(sys.stdout)


rc = main()
sys.exit(rc)
