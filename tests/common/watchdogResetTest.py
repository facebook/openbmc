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
import subprocess
import os
import logging
from time import sleep
from watchdogUtils import WatchdogUtils

currentPath = os.getcwd()
try:
    testPath = currentPath[0:currentPath.index('tests')]
except Exception:
    testPath = '/tmp/'

def watchdogTest(unitTestUtil, platType, logger):
    """Test if watchdog is configured properly on bmc.
    """
    if not platType.watchdogDaemonKill:
        raise Exception("no daemon process kill commands provided")
    if not platType.watchdogDaemonRestore:
        raise Exception("no daemon process restore commands provided")

    wdtUtils = WatchdogUtils(unitTestUtil)

    #
    # Step 1: make sure watchdog is enabled after bmc bootup.
    #
    if not wdtUtils.watchdog_is_running():
        raise Exception("watchdog is not enabled after bmc bootup")

    #
    # Step 2: kill the program which is responsible for kicking watchdog,
    # and make sure watchdog is not stopped when the program exit.
    #
    logger.debug("killing watchdog-kicking daemon processes")
    for cmd in platType.watchdogDaemonKill:
        unitTestUtil.run_shell_cmd(cmd)
    if not wdtUtils.watchdog_is_running(check_counter=True):
        raise Exception("watchdog is stopped when daemon process exit")

    #
    # Step 3: check if watchdog can be stopped.
    #
    logger.debug("testing if watchdog can be stopped")
    wdtUtils.stop_watchdog()
    if wdtUtils.watchdog_is_running():
        raise Exception("watchdog cannot be stopped")

    #
    # Step 4: check if watchdog can be started.
    #
    logger.debug("testing if watchdog can be started")
    wdtUtils.start_watchdog()
    if not wdtUtils.watchdog_is_running(check_counter=True):
        raise Exception("watchdog cannot be started")

    #
    # Step 5: check if watchdog can be kicked properly. The bmc will
    # reboot if watchdog cannot be kicked properly.
    #
    interval = 6
    iterations = 6
    logger.debug("testing if watchdog can be kicked properly")
    for i in range(iterations):
        wdtUtils.kick_watchdog()
        logger.debug('[%d/%d] watchdog kicked. Sleeping for %d seconds' %
                     (i, iterations, interval))
        sleep(interval)

    #
    # Step 6: restore watchdog-kicking daemon process.
    #
    logger.debug("restoreing watchdog-kicking process")
    for cmd in platType.watchdogDaemonRestore:
        unitTestUtil.run_shell_cmd(cmd)

if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python watchdogResetTest.py type
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(['type', '--verbose'], [str, None],
                              ['a platform type',
                              'output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR'])
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        platType = util.importUtil(args.type)
        watchdogTest(util, platType, logger)
    except Exception as e:
        print("watchdog Test [FAILED]")
        print("Error: " + str(e))
        sys.exit(1)

    print("Watchdog Test [PASSED]")
    sys.exit(0)
