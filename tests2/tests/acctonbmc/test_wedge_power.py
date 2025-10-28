#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

from utils.cit_logger import Logger
from utils.test_utils import qemu_check

"""
Tests Wedge Power
"""


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class BaseWedgePowerTest(unittest.TestCase):
    """
    Base class for testing the power control functionalities of a system.

    This class provides common setup, teardown, and utility methods for testing
    various power control commands. It is designed to be extended by specific
    test classes that implement actual test cases.
    """

    CMD_EXECUTION_TIME_LIMIT = 15  # seconds
    EBUSY_ERR = 16
    NUM_INSTANCES = 3  # Number of instances for concurrent testing

    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    def run_power_cmd(self, command):
        """
        Executes a wedge power command and returns its output and status.

        Constructs and runs a command using 'wedge_power.sh',
        handling its output and standard error.
        Manages execution timeouts to ensure it doesn't exceed a set limit.

        Args:
            command (str): The specific wedge power command
                           (e.g., 'on', 'off', 'reset').

        Returns:
            tuple: A tuple containing the command's return code (int)
                   and output (str).

        Raises:
            Exception: If there's an error in command execution.
        """
        full_command = f"wedge_power.sh {command}"
        Logger.debug("Executing: " + str(full_command))
        process = subprocess.Popen(
            full_command,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        try:
            stdout, stderr = \
                process.communicate(timeout=self.CMD_EXECUTION_TIME_LIMIT)
            if stderr:
                raise Exception(stderr.decode())
            return process.returncode, stdout.decode()
        except subprocess.TimeoutExpired:
            Logger.error(
                "{0!r}".format(full_command) +
                 " timed out after {self.CMD_EXECUTION_TIME_LIMIT} seconds."
            )
            process.kill()
            process.communicate()  # Clean up the process
            return -1, ""

    def power_status(self):
        """
        Checks the current power status of the system.

        Returns:
            str: The current power status ("on" or "off").
        """
        return_code, output = self.run_power_cmd("status")
        if return_code != 0:
            Logger.error(
                f"Failed to get power status. Return code: {return_code}"
            )
            return "error"
        return output.strip()

    def run_power_cmd_test(self, cmd):
        """
        Executes a power control command within a time limit and verifies its
        outcome as part of a test case.

        Args:
            cmd (str): The power control command to execute ("off", "on",
            "reset", "on -f").

        Returns:
            int: 0 if the command executed successfully within the time limit
            and the power status matches the command, 1 otherwise.
        """
        Logger.info(f"Executing cmd={cmd}")

        return_code, _ = self.run_power_cmd(cmd)
        if return_code != 0:
            Logger.error(
                "{0!r}".format(cmd) + " failed with return code {return_code}."
            )
            return 1

        status_code, output = self.run_power_cmd("status")
        expected_status = "on" if cmd in ["on", "reset", "on -f"] else "off"

        if status_code == 0 and expected_status in output:
            return 0
        return 1


class WedgePowerTest(BaseWedgePowerTest):
    def test_wedge_power_status(self):
        """
        Tests wedge power status
        """
        Logger.log_testname(name=self._testMethodName)
        status = self.power_status()
        self.assertIn(
            status,
            ["Microserver power is on", "Microserver power is off"],
            "Invalid power status returned",
        )

    def test_wedge_power_off(self):
        """
        Tests wedge power off
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(
            self.run_power_cmd_test("off"), 0, "Power off test failed"
        )

    def test_wedge_power_on(self):
        """
        Tests wedge power on
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(
            self.run_power_cmd_test("on"), 0, "Power on test failed"
        )

    def test_wedge_power_on_force(self):
        """
        Tests wedge power on with force
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(
            self.run_power_cmd_test("on -f"), 0, "Power on with force test failed"
        )

    def test_wedge_power_reset(self):
        """
        Tests wedge power reset
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(
            self.run_power_cmd_test("reset"), 0, "Power reset test failed"
        )
