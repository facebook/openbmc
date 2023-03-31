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
import time
import unittest

from common.base_fans_test import CommonShellBasedFansTest
from tests.fuji.helper.libpal import pal_get_fru_id, pal_is_fru_prsnt
from utils.cit_logger import Logger
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class FansTest(CommonShellBasedFansTest, unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.read_fans_cmd = "/usr/local/bin/get_fan_speed.sh"
        self.write_fans_cmd = "/usr/local/bin/set_fan_speed.sh"
        self.kill_fan_ctrl_cmd = [
            "systemctl stop fscd.service",
            "/usr/local/bin/wdtcli stop",
        ]
        self.start_fan_ctrl_cmd = ["systemctl start fscd.service"]

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        self.kill_fan_controller()
        self.start_fan_controller()

    def fan_read_test_impl(self, fan_id):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        if not pal_is_fru_prsnt(pal_get_fru_id(f"fan{fan_id}")):
            self.skipTest(f"fan{fan_id} is not present")
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        # Sleep needed to test and make sure that we won't run
        # into accessing sysfs path issue causing test to fail
        time.sleep(2)
        super().fan_read(fan=fan_id)

    def fan_set_pwm_test_impl(self, fan_id, pwm):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        if not pal_is_fru_prsnt(pal_get_fru_id(f"fan{fan_id}")):
            self.skipTest(f"fan{fan_id} is not present")
        self.assertNotEqual(self.write_fans_cmd, None, "Set Fan cmd not set")
        # Sleep needed to test and make sure that we won't run
        # into accessing sysfs path issue causing test to fail
        time.sleep(2)
        super().fan_set_and_read(fan_id=fan_id, pwm=pwm)

    def test_fans_read(self):
        for i in range(1, 9):
            with self.subTest(fan_id=i):
                self.fan_read_test_impl(i)

    def test_fans_pwm_set(self):
        for i in range(1, 9):
            with self.subTest(fan_id=i):
                pwm = i * 10
                self.fan_set_pwm_test_impl(i, pwm)
