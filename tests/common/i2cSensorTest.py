#!/usr/bin/env python
#
# Copyright 2018-present Facebook. All Rights Reserved.
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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import sys
import subprocess
import unitTestUtil
import logging


def i2c_check(name, bus, address):
    """
    For a given name and address, checks i2cdetect and i2cdump for any failures
    """
    failed = []
    cmd = 'i2cdetect -y ' + bus
    logger.debug("executing: " + str(cmd))
    f = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    info = info.decode('utf-8')
    info = info.split('\n')
    row = address[2]
    col = address[3]
    line = info[int(row) + 1]
    index = int('0x' + col, 16)
    line = line.split(' ')
    reading = line[index + 1]
    if reading != 'UU' and reading != address[2] + address[3]:
        failed += [str(name) + ' detect']
    cmd = 'i2cdump -f -y ' + bus + ' ' + address + ' b'
    logger.debug("executing: " + str(cmd))
    f = subprocess.Popen(cmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    info = info.decode('utf-8')
    info = info.split('\n')
    badread = True
    for i in range(1, len(info) - 1):
        line = info[i].split(':')[1].split('  ')[0]
        if line.count('X') != 32:
            badread = False
            break
    if badread:
        failed += [str(name) + ' dump']
    return failed


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python i2cSensorTest.py wedgeSensor.json
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        data = {}
        args = util.Argparser(['json', '--verbose'], [str, None],
                              ['json file',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        data = util.JSONparser(args.json)
        failed = []
        for name in data:
            failed += i2c_check(name, data[name]['bus'], data[name]['address'])
    except Exception as e:
        print("sensor i2c test [FAILED]")
        print("Error code returned: " + str(e))
        sys.exit(1)
    if len(failed) == 0:
        print("i2c test [PASSED]")
        sys.exit(0)
    else:
        print('i2c test for names:' + str(failed) + " [FAILED]")
        sys.exit(1)
