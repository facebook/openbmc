#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
import re
import unittest

from common.base_enclosure_util_test import BaseEnclosureUtilTest
from utils.shell_util import run_cmd
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class EnclosureUtilTest(BaseEnclosureUtilTest):
    def testHddStatus(self):
        """
        test enclosure-util --hdd-status and verify its output
        """
        cmd = ["enclosure-util", "--hdd-status"]
        out = run_cmd(cmd)
        pattern = (
            r"Normal HDD ID:(\s\d+)* \n"
            + r"Abnormal HDD ID:(\s\d+)* \n"
            + r"Missing HDD ID:(\s\d+)* \n$"
        )
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))

    def testHdd1Status(self):
        """
        test enclosure-util --hdd-status 1 and verify its output
        """
        cmd = ["enclosure-util", "--hdd-status", "1"]
        out = run_cmd(cmd)
        pattern = r"HDD_1 Status:(\s\d+)+\n$"
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))

    def testHddError(self):
        """
        test enclosure-util --error and verify its output
        only check for invalid characters
        """
        cmd = ["enclosure-util", "--error"]
        out = run_cmd(cmd)

        if re.search("No Error", out):
            pattern = r"Error Counter: 0 [(]No Error[)]"
        else:
            pattern = r"Error Counter: ([a-zA-Z0-9\n\s:]*)$"

        self.assertRegex(out, pattern, "unexpected output: {}".format(out))

    def testFlashHealth(self):
        """
        test enclosure-util --e1s-health and verify its output
        """
        cmd = ["enclosure-util", "--e1s-health"]
        out = run_cmd(cmd)
        pattern = r"e1.s 0:(\s[a-zA-Z]+)\ne1.s 1:(\s[a-zA-Z]+)\n$"
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))

    def testFlashStatus(self):
        """
        test enclosure-util --e1s-status and verify its output
        only check for invalid characters
        """
        cmd = ["enclosure-util", "--e1s-status"]
        out = run_cmd(cmd)
        pattern = r"e1.s 0:\s*\n((.+:.+)\n)+\ne1.s 1:\s*\n((.+:.+)\n)+\n$"
        self.assertRegex(out, pattern, "unexpected output: {}".format(out))
