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
import time
import unitTestUtil
import logging


def fansTest(platformType, util):
    """
    Manually sets a fan to a percentage and then verifies change. This test has
    coverage for wedge, wedge100, bryce canyon, cmm, and tioga pass.
    """
    if util.GetFanCmd is None:
        raise Exception("fan get command not implemented")
    if util.SetFanCmd is None:
        raise Exception("fan set command not implemented")
    if util.KillControlCmd is None:
        raise Exception("fan kill commands not implemented")
    if util.StartControlCmd is None:
        raise Exception("fan start command not implemented")
    # get fan test
    logger.debug("Executing: {}".format(util.GetFanCmd))
    f = subprocess.Popen(util.GetFanCmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    fans, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    getFanTest = util.get_fan_test(fans)
    if getFanTest:
        print("Read fan for " + platformType + " platform [PASSED]")
    else:
        print("Read fan for " + platformType + " platform [FAILED]")
        print("Fan info returned: " + fans)
        sys.exit(1)  # failed get test so no need to proceed

    # kill or stop controller
    logger.debug("Stopping fan controller")
    for cmd in util.KillControlCmd:
        err = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE).communicate()[1]
        if len(err) != 0:
            raise Exception(err)

    # set fan speed
    logger.debug("Executing: ".format(util.SetFanCmd))
    time.sleep(5)   # allow time for fans to be properly set
    err = subprocess.Popen(util.SetFanCmd,
                           shell=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE).communicate()[1]
    if len(err) != 0:
        raise Exception(err)
    time.sleep(10)  # allow time for fans to be properly set

    # get speed
    logger.debug("Checking that fans were properly set")
    f = subprocess.Popen(util.GetFanCmd,
                         shell=True,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
    fans, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)

    speed = util.get_speed(fans)

    # start controller
    logger.debug("Starting fan controller")
    err = subprocess.Popen(util.StartControlCmd,
                           shell=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE).communicate()[1]
    if len(err) != 0:
        raise Exception(err)
    # fan control test
    if speed == 0:
        print("Fan control for " + platformType + " platform [PASSED]")
        sys.exit(0)
    else:
        print("Fan control for " + platformType + " platform [FAILED]")
        sys.exit(1)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python fansTest.py wedge
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
        fansTest(platformType, utilType)
    except Exception as e:
        if type(e) == KeyError:
            print("No support for platform type given or none given [FAILED]")
        else:
            print("Get and Set fan test [FAILED]")
            print("Error code returned: " + str(e, 'utf-8'))
        sys.exit(1)
