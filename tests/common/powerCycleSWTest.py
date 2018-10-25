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

try:
    from pexpect import pxssh
except:
    import pxssh
import time
import os
import logging

currentPath = os.getcwd()
try:
    testPath = currentPath[0: currentPath.index("common")]
except Exception:
    testPath = "/tmp/tests/"

WAIT_TIME = 200  # seconds

def powerCycleTest(unitTestUtil, utilType, hostnameBMC, hostnameMS):
    if utilType.PowerCmdStatus is None:
        raise Exception("power status command not implemented")
    if utilType.PowerCmdOn is None:
        raise Exception("power on command not implemented")
    if utilType.PowerCmdOff is None:
        raise Exception("power off command not implemented")
    if utilType.PowerCmdReset is None:
        raise Exception("power reset command not implemented")
    pingcmd = ["python",testPath + "common/connectionTest.py", hostnameMS, "6"]
    ssh = pxssh.pxssh()
    unitTestUtil.Login(hostnameBMC, ssh)
    status = ""
    # Power on for test to start if not already
    logger.debug("Checking current power status")
    ssh.sendline(utilType.PowerCmdStatus)
    ssh.prompt()
    if "on" in (ssh.before).decode() or "ON" in (ssh.before).decode():
        status = "on"
    else:
        logger.debug("Executing power ON command")
        ssh.sendline(utilType.PowerCmdOn)
        ssh.prompt()
        status = "off"
    powerCycleOnOffTest(pingcmd, utilType, ssh)
    logger.debug("Waiting before next step ...")
    powerCycleResetTest(pingcmd, utilType, ssh, status)


def powerCycleOnOffTest(pingcmd, utilType, ssh):
    """
    For a given platform test their power on, off, and reset commands
    """
    # Power off
    logger.debug("Executing power OFF command")
    ssh.sendline(utilType.PowerCmdOff)
    ssh.prompt()
    # Check that ping connection is failure
    logger.debug("Checking that ping to Microserver fails since power is off")
    f = subprocess.Popen(pingcmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    if "PASSED" in out.decode():
        print("Power Cycle test off command [FAILED]")
        sys.exit(1)
    print("Power Cycle test OFF command [PASSED]")
    logger.debug("Waiting before next step ...")
    time.sleep(WAIT_TIME)

    # Power on
    logger.debug("Executing power ON command")
    ssh.sendline(utilType.PowerCmdOn)
    ssh.prompt()
    logger.debug("Waiting before next step ...")
    time.sleep(WAIT_TIME)
    # Check that ping connection is good
    logger.debug("Checking that ping to Microserver passes since power is on")
    t_end = time.time() + 60 * 10
    while time.time() < t_end:  # ping until timeout or ping passes
        f = subprocess.Popen(pingcmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = f.communicate()
        if len(err) != 0:
            raise Exception(err)
        if "PASSED" in out.decode():  # break when ping passes
            break
    if "FAILED" in out.decode():
        print("Power Cycle test on command [FAILED]")
        sys.exit(1)
    print("Power Cycle test ON command [PASSED]")


def powerCycleResetTest(pingcmd, utilType, ssh, status):
    # Power reset
    logger.debug("Executing power RESET command")
    ssh.sendline(utilType.PowerCmdReset)
    time.sleep(10)
    ssh.prompt()
    # Check that ping connection is failure then success
    logger.debug(
        "Checking that ping to Microserver fails because Microserver is resetting"
    )
    f = subprocess.Popen(pingcmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = f.communicate()
    if len(err) != 0:
        raise Exception(err)
    if "PASSED" in out.decode():
        print("Power Cycle test RESET command [FAILED]")
        sys.exit(1)
    t_end = time.time() + 60 * 10
    logger.debug(
        "Checking that ping to Microserver passes because Microserver should be done with reset"
    )
    while time.time() < t_end:  # ping until timeout or ping passes
        f = subprocess.Popen(pingcmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        out, err = f.communicate()
        if len(err) != 0:
            raise Exception(err)
        if "PASSED" in out.decode():  # break when ping passes
            break
    print("Power Cycle test RESET command [PASSED]")
    logger.debug("Waiting before next step ...")
    # Set the power to the status when test began
    if status == "off":
        logger.debug("executing power off command")
        ssh.sendline(utilType.PowerCmdOff)
        ssh.prompt()
    if "FAILED" in out.decode():
        print("Power Cycle test RESET command [FAILED]")
        sys.exit(1)
    else:
        print("Power Cycle test [PASSED]")
        sys.exit(0)


if __name__ == "__main__":
    """
    Input to this file should look like the following:
    python powerCycleSWTest.py type hostnameBMC hostnameMS
    """
    util = unitTestUtil.UnitTestUtil()
    logger = util.logger(logging.WARN)
    try:
        args = util.Argparser(
            ["type", "hostnameBMC", "hostnameMS", "--verbose"],
            [str, str, str, None],
            [
                "a platform type",
                "a hostname for the BMC",
                "a hostname for the Microserver",
                "output all steps from test with mode options: DEBUG, INFO, WARNING, ERROR",
            ],
        )
        if args.verbose is not None:
            logger = util.logger(args.verbose)
        platformType = args.type
        hostnameBMC = args.hostnameBMC
        hostnameMS = args.hostnameMS
        utilType = util.importUtil(platformType)
        powerCycleTest(util, utilType, hostnameBMC, hostnameMS)
    except Exception as e:
        print("Power Test [FAILED]")
        print("Error: " + str(e))
    sys.exit(1)
