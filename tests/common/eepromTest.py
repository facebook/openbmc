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
import subprocess
import sys
import unitTestUtil
import logging


def eepromTest(util):
    """
    Uses util to read off of the eeprom then checks that the product name
    matches the platform. This test has coverage for wedge, wedge100,
    galaxy100, bryce canyon, cmm, and tioga pass.
    """
    utilCMD = util.EEPROMCmd
    if utilCMD is None:
        raise Exception("Eeprom command not implemented")
    prodnames = util.ProductName
    if prodnames is None:
        raise Exception("Eeprom product name not implemented")
    logger.debug("Executing eeprom util dump with command: " + str(utilCMD))
    f = subprocess.Popen(utilCMD,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    info, err = f.communicate()
    if len(err) > 0:
        raise Exception(err + " [FAILED]")
    info = info.decode('utf-8')
    name = info.split('Product Name')[1].split(': ')[1].split('\n')[0]
    # check all possible product name types
    success = False
    index = 0
    for i in range(len(prodnames)):
        if prodnames[i].lower() in name.lower():
            success = True
            index = i
    if success:
        print("EEPROM read for " + prodnames[index] + " platform [PASSED]")
        sys.exit(0)
    else:
        print("EEPROM read for " + prodnames[index] + " platform [FAILED]")
        sys.exit(1)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python eepromTest.py type=wedge
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(['type', '--verbose'], [str, None],
                              ['a platform type',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        platformType = args.type
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        utilType = util.importUtil(platformType)
        eepromTest(utilType)
    except Exception as e:
        if type(e) == KeyError:
            print("No support for platform type given or none given [FAILED]")
        else:
            print("EEPROM Test [FAILED]")
            print("Error: " + str(e))
        sys.exit(1)
