#!/usr/bin/env python
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
        self.kill_fan_ctrl_cmd = ["/usr/bin/killall -USR1 fand"]
        self.start_fan_ctrl_cmd = ["/bin/sh /etc/init.d/setup-fan.sh"]

    def tearDown(self):
        self.kill_fan_controller()
        self.start_fan_controller()

    def test_all_fans_read(self):
        """
        Test if all fan dump is returning sane data
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        data = self.get_fan_speed()
        Logger.info("Fans dump:\n" + data)
        self.assertTrue(self.verify_get_fan_speed_output(data), "Get fan speeds failed")

    def test_fan1_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=1)

    def test_fan2_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=2)

    def test_fan3_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=3)

    def test_fan4_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=4)

    def test_fan5_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=5)

    def test_fan6_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=6)

    def test_fan7_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=7)

    def test_fan8_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=8)

    def test_fan9_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=9)

    def test_fan10_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=10)

    def test_fan11_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=11)

    def test_fan12_read(self):
        """
         For each fan read and test speed
        """
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        super().fan_read(fan=12)

    def verify_get_fan_speed_output(self, info):
        """
        Parses get_fan_speed.sh output
        and test it is in the expected format
        Arguments: info = dump of get_fan_speed.sh
        """
        Logger.info("Parsing Fans dump: {}".format(info))
        info = info.split("\n")
        goodRead = ["Fan", "RPMs:"]
        for line in info:
            if len(line) == 0:
                continue
            for word in goodRead:
                if word not in line:
                    return False
            val = line.split(":")[1].split(",")
            if len(val) != 2:
                return False
        return True
