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


def powerCycleTest(util):
    """
    For a given platform test that power on and off change hardware correctly
    """
    if utilType.PowerCmdStatus is None:
        raise Exception("power status command not implemented")
    if utilType.PowerCmdOn is None:
        raise Exception("power on command not implemented")
    if utilType.PowerCmdOff is None:
        raise Exception("power off command not implemented")
    if utilType.PowerHW is None:
        raise Exception("power hardware command not implemented")
    status = ''
    # Power on for test to start if not already
    logger.debug("executing: " + str(util.PowerCmdStatus))
    f = subprocess.Popen(util.PowerCmdStatus,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    out = out.decode('utf-8')
    if 'on' or 'ON' in out:
        status = 'on'
    else:
        logger.debug("executing power on command")
        f = subprocess.Popen(util.PowerCmdOn,
                             shell=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = f.communicate()
        if len(err) != 0:
            raise Exception(err)
        status = 'off'
    # Power off
    logger.debug("executing power off command")
    f = subprocess.Popen(util.PowerCmdOff,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    # Check power hardware reads off
    logger.debug("checking that hardware reads off")
    f = subprocess.Popen(util.PowerHW,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    out = out.decode('utf-8')
    if len(err) != 0:
        raise Exception(err)
    if ('off' not in out.lower()):
        print("Power Test bad read of hardware. Expected OFF [FAILED]")
        sys.exit(1)
    # Power on
    logger.debug("executing power on command")
    f = subprocess.Popen(util.PowerCmdOn,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    # Check power hardware reads on
    logger.debug("checking that hardware reads on")
    f = subprocess.Popen(util.PowerHW,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    out, err = f.communicate()
    out = out.decode('utf-8')
    if len(err) != 0:
        raise Exception(err)
    if ('on' not in out.lower()):
        print("Power Test bad read of hardware. Expected ON [FAILED]")
        sys.exit(1)
    # Set the power to the status when test began
    if status == 'off':
        logger.debug("executing power off command")
        f = subprocess.Popen(util.PowerCmdOff,
                             shell=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        out, err = f.communicate()
        if len(err) != 0:
            raise Exception(err)
        out = out.decode('utf-8')
        if ('off' not in out.lower()):
            print("Power Test bad read of hardware [FAILED]")
            sys.exit(1)

    print("Power Test correct reading of hardware [PASSED]")
    sys.exit(0)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python powerCycleHWTest.py type
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
        powerCycleTest(utilType)
    except Exception as e:
        print("Power Test [FAILED]")
        print("Error: " + str(e))
    sys.exit(1)
