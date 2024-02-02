#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import subprocess
import threading
import unittest
import time

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd

"""
Tests Wedge Power
"""


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

        Constructs and runs a command using 'wedge_power.sh', handling its output and
        standard error. Manages execution timeouts to ensure it doesn't exceed a set limit.

        Args:
            command (str): The specific wedge power command (e.g., 'on', 'off', 'reset').

        Returns:
            tuple: A tuple containing the command's return code (int) and output (str).

        Raises:
            Exception: If there's an error in command execution.
        """
        full_command = f"wedge_power.sh {command}"
        Logger.debug("Executing: " + str(full_command))
        process = subprocess.Popen(
            full_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
        try:
            stdout, stderr = process.communicate(timeout=self.CMD_EXECUTION_TIME_LIMIT)
            if stderr:
                raise Exception(stderr.decode())
            return process.returncode, stdout.decode()
        except subprocess.TimeoutExpired:
            Logger.error(f"Command '{full_command}' timed out after {self.CMD_EXECUTION_TIME_LIMIT} seconds.")
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
            Logger.error(f"Failed to get power status. Return code: {return_code}")
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
            Logger.error(f"Command '{cmd}' failed with return code {return_code}.")
            return 1

        status_code, output = self.run_power_cmd("status")
        expected_status = "on" if cmd in ["on", "reset", "on -f"] else "off"

        if status_code == 0 and expected_status in output:
            return 0
        return 1

    def execute_power_cmd_thread(self, cmd, results, index):
        """
        Executes a power control command in a threaded context and stores its exit code.

        Designed to be used in a multithreaded test setup, this method runs the specified
        command in a thread and records its exit code in a shared results list. It's part of
        testing how power control commands perform when executed concurrently.

        Args:
            cmd (str): Command to execute.
            results (list): List to store exit codes from each thread.
            index (int): Position in the results list for this thread's exit code.
        """
        try:
            exit_code, _ = self.run_power_cmd(cmd)
            results[index] = exit_code
        except Exception as e:
            results[index] = str(e)

    def run_concurrent_power_command_test(self, cmd):
        """
        Test multiple instances running a specified power command concurrently.
        """
        threads = []
        results = [None] * self.NUM_INSTANCES

        for i in range(self.NUM_INSTANCES):
            thread = threading.Thread(target=self.execute_power_cmd_thread, args=(cmd, results, i))
            threads.append(thread)
            thread.start()

        for thread in threads:
            thread.join()

        success_count = results.count(0)
        busy_count = results.count(self.EBUSY_ERR)

        # Assert that only one instance succeeded and others received the EBUSY_ERR
        self.assertEqual(success_count, 1, "Only one instance should succeed")
        self.assertEqual(busy_count, self.NUM_INSTANCES - 1, "Other instances should receive busy error")


class WedgePowerTest(BaseWedgePowerTest):
    def test_wedge_power_status(self):
        """
        Tests wedge power status
        """
        Logger.log_testname(name=self._testMethodName)
        status = self.power_status()
        self.assertIn(status, ["Microserver power is on", "Microserver power is off"], "Invalid power status returned")

    def test_wedge_power_off(self):
        """
        Tests wedge power off
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.run_power_cmd_test("off"), 0, "Power off test failed")

    def test_wedge_power_on(self):
        """
        Tests wedge power on
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.run_power_cmd_test("on"), 0, "Power on test failed")

    def test_wedge_power_on_force(self):
        """
        Tests wedge power on with force
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.run_power_cmd_test("on -f"), 0, "Power on with force test failed")

    def test_wedge_power_reset(self):
        """
        Tests wedge power reset
        """
        Logger.log_testname(name=self._testMethodName)
        self.assertEqual(self.run_power_cmd_test("reset"), 0, "Power reset test failed")

    def test_wedge_power_off_concurrent(self):
        """
        Test multiple instances running 'off' command concurrently.
        """
        Logger.log_testname(name=self._testMethodName)
        status = self.power_status()
        if "off" in status:
            self.run_power_cmd("on")
        self.run_concurrent_power_command_test("off")

    def test_wedge_power_reset_concurrent(self):
        """
        Test multiple instances running 'reset' command concurrently.
        """
        Logger.log_testname(name=self._testMethodName)
        status = self.power_status()
        if "off" in status:
            self.run_power_cmd("on")
        self.run_concurrent_power_command_test("reset")

