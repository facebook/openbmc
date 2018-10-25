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
import re


def mac_verify(read_mac):
    if re.match("[0-9a-f]{2}([:]?)[0-9a-f]{2}(\\1[0-9a-f]{2}){4}$",
                read_mac.lower()):
        return True
    return False


def hostmacTest(util):
    utilCMD = util.HostMacCmd
    if utilCMD is None:
        raise Exception("Host MAC command not implemented")

    logger.debug("Executing host MAC command: " + str(utilCMD))
    info = subprocess.check_output(utilCMD).decode()
    logger.debug("Received host MAC={} ".format(info))
    success = mac_verify(info)
    if success:
        print("Host Mac Test [PASSED]")
        sys.exit(0)
    else:
        print("Host Mac Test received MAC={} [FAILED]".format(info))
        sys.exit(1)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python hostMac_test.py type=wedge
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
        hostmacTest(utilType)
    except Exception as e:
        if type(e) == KeyError:
            print("No support for platform type given or none given [FAILED]")
        else:
            print("Host Mac Test [FAILED]")
            print("Error: " + str(e))
        sys.exit(1)
