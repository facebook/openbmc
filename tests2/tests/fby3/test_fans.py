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
import unittest

from common.base_fans_test import CommonFanUtilBasedFansTest
from utils.cit_logger import Logger
from utils.test_utils import qemu_check


@unittest.skip("FIXME for HW CIT T163968017")
@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class FansTest(CommonFanUtilBasedFansTest, unittest.TestCase):
    def setUp(self):
        from pal import pal_get_tach_cnt, pal_get_pwm_cnt

        Logger.start(name=self._testMethodName)

        tach_cnt = pal_get_tach_cnt()
        pwm_cnt = pal_get_pwm_cnt()
        is_dual_fan = (tach_cnt // pwm_cnt) == 2

        self.fans = list(range(tach_cnt))
        self.names = {"Fan {}".format(fan): fan for fan in range(tach_cnt)}
        if is_dual_fan:
            self.pwms = {0: [0, 1], 1: [2, 3], 2: [4, 5], 3: [6, 7]}
        else:
            self.pwms = {0: [0], 1: [1], 2: [2], 3: [3]}

        self.kill_fan_ctrl_cmd = ["/usr/bin/sv stop fscd"]
        self.start_fan_ctrl_cmd = ["/usr/bin/sv start fscd"]

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        self.kill_fan_controller()
        self.start_fan_controller()
