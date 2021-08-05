#!/usr/bin/env python3
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
from unittest import TestCase

from utils.cit_logger import Logger
from utils.shell_util import run_cmd


class BaseEnclosureUtilTest(TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def testHddStatus(self):
        """
        test enclosure-util --hdd-status and verify its output
        """
        cmd = ["enclosure-util", "--hdd-status"]
        out = run_cmd(cmd)
        pattern = r"Normal Drives:(\s\d+)* \nAbnormal Drives:(\s\d+)* \nMissing Drives:(\s\d+)* \n$"
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))

    def testHdd1Status(self):
        """
        test enclosure-util --hdd-status 1 and verify its output
        """
        cmd = ["enclosure-util", "--hdd-status", "1"]
        out = run_cmd(cmd)
        pattern = r"Drive_1 Status:(\s\d+)+\n$"
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))

    def testHddError(self):
        """
        test enclosure-util --error and verify its output
        only check for invalid characters
        """
        cmd = ["enclosure-util", "--error"]
        out = run_cmd(cmd)
        pattern = r"Error Counter: ([a-zA-Z0-9\n\s:]*)$"
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))

    def testFlashHealth(self):
        """
        test enclosure-util --flash-health and verify its output
        """
        cmd = ["enclosure-util", "--flash-health"]
        out = run_cmd(cmd)
        pattern = r"flash-1:(\s[a-zA-Z]+)\nflash-2:(\s[a-zA-Z]+)\n$"
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))

    def testFlashStatus(self):
        """
        test enclosure-util --flash-status and verify its output
        only check for invalid characters
        """
        cmd = ["enclosure-util", "--flash-status"]
        out = run_cmd(cmd)
        pattern = r"^[a-zA-Z0-9\n\s:\-()]*$"
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))
