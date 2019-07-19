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
import unittest

from common.base_fans_test import CommonShellBasedFansTest
from utils.cit_logger import Logger


class FansTest(CommonShellBasedFansTest, unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.read_fans_cmd = "/usr/local/bin/get_fan_speed.sh"
        self.write_fans_cmd = "/usr/local/bin/set_fan_speed.sh"
        self.kill_fan_ctrl_cmd = ["/usr/local/bin/wdtcli stop", "/usr/bin/sv stop fscd"]
        self.start_fan_ctrl_cmd = ["/usr/bin/sv start fscd"]

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        self.kill_fan_controller()
        self.start_fan_controller()

    def test_fan1_read(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=1)

    def test_fan2_read(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=2)

    def test_fan3_read(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=3)

    def test_fan4_read(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=4)

    def test_fan1_pwm_set(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.write_fans_cmd, None, "Set Fan cmd not set")
        super().fan_set_and_read(fan_id=1, pwm=0)

    def test_fan2_pwm_set(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.write_fans_cmd, None, "Set Fan cmd not set")
        super().fan_set_and_read(fan_id=2, pwm=30)

    def test_fan3_pwm_set(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.write_fans_cmd, None, "Set Fan cmd not set")
        super().fan_set_and_read(fan_id=3, pwm=50)

    def test_fan4_pwm_set(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.write_fans_cmd, None, "Set Fan cmd not set")
        super().fan_set_and_read(fan_id=4, pwm=70)
