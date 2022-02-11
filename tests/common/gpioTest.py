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
import unitTestUtil
import logging
import os
import time

_GPIO_SHADOW_ROOT = '/tmp/gpionames'

def read_data(path):
    handle = open(path, "r")
    data = handle.readline().rstrip()
    handle.close()
    logger.debug("reading path={} data={}".format(path, data))
    return data


def gpio_test(data):
    """
    given a json of gpios and expected values, check that all of the gpios
    exist and its values
    """
    errors = 0
    for gpioname, v in data['gpios'].items():
        gpio_path = os.path.join(_GPIO_SHADOW_ROOT, str(gpioname))
        if not os.path.exists(gpio_path):
            print("GPIO test : Missing GPIO={} [FAILED]".format(gpioname))
            errors += 1
            continue
        
        logger.debug("GPIO name={} present".format(gpioname))
        for item, jdata in v.items():
            rdata = read_data(os.path.join(gpio_path, str(item)))
            if rdata not in jdata:
                print("GPIO test : Incorrect {} for GPIO={} [FAILED]".format(item, gpioname))
                errors += 1

    if errors != 0:
        raise Exception('total %d errors found' % errors)

if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python gpioTest.py gpiolist.json
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
        gpio_test(data)
    except Exception as e:
        print("GPIO test [FAILED]")
        print("Error code returned: " + str(e))
        sys.exit(1)

    print("GPIO test [PASSED]")
    sys.exit(0)
