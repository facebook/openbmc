#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

import subprocess
import unittest
import time

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd

"""
Tests Wedge Power
"""


class BaseWedgePowerTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def run_power_cmd(self, cmd):
        self.assertNotEqual(cmd, None, "run_power cmd not set")
        if cmd != "":
            Logger.debug("Executing: " + str(cmd))
            f = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
            )
            err = f.communicate()[1]
            if len(err) > 0:
                raise Exception(err)
            return f.returncode
        return -1

    def power_status(self):
        output = run_shell_cmd("wedge_power.sh status")
        if "on" or "off" in output:
            return 0
        return 1

    def power_off(self):
        cmd = "wedge_power.sh off"
        Logger.info("Executing cmd={}".format(cmd))
        self.run_power_cmd(cmd)
        time.sleep(10)
        output = run_shell_cmd("wedge_power.sh status")
        if "off" in output:
            return 0
        return 1

    def power_on(self):
        cmd = "wedge_power.sh on"
        Logger.info("Executing cmd={}".format(cmd))
        self.run_power_cmd(cmd)
        time.sleep(10)
        output = run_shell_cmd("wedge_power.sh status")
        if "on" in output:
            return 0
        return 1

    def power_on_force(self):
        cmd = "wedge_power.sh on -f"
        Logger.info("Executing cmd={}".format(cmd))
        self.run_power_cmd(cmd)
        time.sleep(10)
        output = run_shell_cmd("wedge_power.sh status")
        if "on" in output:
            return 0
        return 1

    def power_reset(self):
        cmd = "wedge_power.sh reset"
        Logger.info("Executing cmd={}".format(cmd))
        self.run_power_cmd(cmd)
        time.sleep(10)
        output = run_shell_cmd("wedge_power.sh status")
        if "on" in output:
            return 0
        return 1


class WedgePowerTest(BaseWedgePowerTest):
    def test_wedge_power_status(self):
        """
        Tests wedge power status
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.power_status(), 0, "Power status test for failed")

    def test_wedge_power_off(self):
        """
        Tests wedge power off
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.power_off(), 0, "Power off test for failed")

    def test_wedge_power_on(self):
        """
        Tests wedge power on
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.power_on(), 0, "Power on test for failed")

    def test_wedge_power_on_force(self):
        """
        Tests wedge power on
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.power_on_force(), 0, "Power on test for failed")

    def test_wedge_power_reset(self):
        """
        Tests wedge power reset
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.power_reset(), 0, "Power reset test for failed")
