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
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class SensorCalibrationTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_max6581_tuned(self):
        # Test the IMP port state register
        data = run_shell_cmd("i2cget -f -y 4 0x4d 0x4a").split("\n")
        self.assertIn("0x03", data[0], "unexpected value in MAX6581 0x4A")
        data = run_shell_cmd("i2cget -f -y 4 0x4d 0x4b").split("\n")
        self.assertIn("0x1f", data[0], "unexpected value in MAX6581 0x4B")
        data = run_shell_cmd("i2cget -f -y 4 0x4d 0x4c").split("\n")
        self.assertIn("0x03", data[0], "unexpected value in MAX6581 0x4C")
