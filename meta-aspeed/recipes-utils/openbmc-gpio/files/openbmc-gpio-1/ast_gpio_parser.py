#!/usr/bin/python -tt
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
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import argparse
import csv
import logging
import re
import sys

FUNC_SYMBOL = 'Function'
AND_SYMBOL = 'And'
OR_SYMBOL = 'Or'
BE_SYMBOL = 'BitsEqual'
BNE_SYMBOL = 'BitsNotEqual'


class CsvReader:
    '''
    A class for parsing the CSV files containing the pin mapping data.
    '''
    def __init__(self, path):
        self.path = path

        fileobj = open(path, 'r')
        self.reader = csv.reader(fileobj, delimiter=b',', quotechar=b'"')

    def next(self):
        try:
            line = self.reader.next()
        except StopIteration:
            return None
        return line


class Expression(object):
    def __init__(self, exp):
        self.exp = exp

    def parse(self):
        # Strap[4,1:0]=100
        m = re.match('([^\[]+)\[(.+)\](!*=)([01]+)', self.exp)
        # ('Strap', '4,1:0', '=', '100')
        try:
            if len(m.groups()) != 4:
                raise
            scuname  = m.group(1)
            if scuname.lower() == 'strap':
                scu = 0x70
            elif scuname.startswith('SCU'):
                scu = int(scuname[3:], 16)

            bits = []
            for bit in m.group(2).split(','):
                if ':' in bit:
                    # it is a range
                    s, _, e = bit.partition(':')
                    ss = int(s)
                    ee = int(e)
                    if ss < ee:
                        assert 0 <= ss and ee <= 31
                        bits += range(ss, ee + 1)
                    else:
                        assert 0 <= ee and ss <= 31
                        bits += range(ee, ss + 1)
                else:
                    bits += [int(bit)]
            if m.group(3) == '!=':
                cond = BNE_SYMBOL
            else:
                cond = BE_SYMBOL
            value = int(m.group(4), 2)
            return '%s(0x%x, %s, 0x%x)' \
                % (cond, scu, sorted(bits, reverse=True), value)
        except Exception as e:
            logging.exception('Failed to parse expression "%s"', self.exp)


class Conditions(object):
    def __init__(self, cond):
        self.cond = re.sub('\s+', '', cond)

    def _next_condition_pos(self, cur):
        for idx in range(cur, len(self.cond)):
            c = self.cond[idx]
            if c == '&' or c == '|':
                return idx
        return -1

    def _get_expression(self, exp):
        if exp == '':
            return None
        return Expression(exp).parse()

    def _parse(self, idx):
        next_idx = self._next_condition_pos(idx + 1)
        if next_idx == -1:
            # end of string
            return self._get_expression(self.cond[idx:])

        prev = self._get_expression(self.cond[idx:next_idx])
        if self.cond[next_idx] == '&':
            cond = AND_SYMBOL
        else:
            cond = OR_SYMBOL
        return '%s(%s, %s)' % (cond, prev, self._parse(next_idx + 1))

    def parse(self):
        return self._parse(0)


class AstGPIO(object):
    def __init__(self, data):
        self.data = data
        self.undefined_func = 1
        self.pins = {}
        self.functions = set()

    def _parse_conditions(self, cond):
        return Conditions(cond).parse()

    def _parse_funcs(self, parts):
        func_fmt = FUNC_SYMBOL + '(\'{func}\', {cond})'
        if len(parts) < 1:
            return []
        # Unable to process SIORD
        if 'SIORD' in ' '.join(parts):
            logging.warning('Unable to process SIORD. Ignore')
            return []
        if 'GFX064' in ' '.join(parts):
            logging.warning('Unable to process GFX064. Ignore')
            return []
        if 'LHCR0' in ' '.join(parts):
            logging.warning('Unable to process LHCR0. Ignore')
            return []
        func = parts[0]
        if func == '':
            # nothing after
            return []
        if func == '-':
            func = 'UNDEFINED%d' % self.undefined_func
            self.undefined_func += 1
        assert func not in self.functions
        self.functions.add(func)
        if len(parts) == 1:
            # just has the function name, the last function
            return [func_fmt.format(func=func, cond=None)]
        cond = self._parse_conditions(parts[1])
        return [func_fmt.format(func=func, cond=cond)] \
            + self._parse_funcs(parts[2:])

    def parse(self):
        while True:
            line = self.data.next()
            if line is None:
                break

            logging.debug('Parsing line: %s' % line)

            # V21,ROMA24,SCU88[28]=1 & SCU94[1]=0,VPOR6,SCU88[28]=1 & SCU94[1]=1,GPIOR4
            pin = line[0]
            if pin == "":
                # empty line
                continue
            funcs = self._parse_funcs(line[1:])
            logging.debug('%s: %s' % (pin, funcs))
            assert pin not in self.pins
            self.pins[pin] = funcs

    def print(self, out):
        for pin in sorted(self.pins):
            if len(self.pins[pin]) == 0:
                logging.warning('Pin "%s" has no function defined. Skip' % pin)
                continue
            out.write('    \'%s\': [\n        %s\n    ],\n'
                      % (pin, ',\n        '.join(self.pins[pin])))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('data', help='The GPIO data file')
    args = ap.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s: %(message)s')

    gpio = AstGPIO(CsvReader(args.data))
    gpio.parse()
    gpio.print(sys.stdout)


rc = main()
sys.exit(rc)
