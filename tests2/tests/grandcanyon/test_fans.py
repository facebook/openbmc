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
import ctypes
import unittest

from common.base_fans_test import CommonFanUtilBasedFansTest
from utils.cit_logger import Logger
from utils.test_utils import qemu_check


UNKNOWN_FAN_CNT = 0
SINGLE_FAN_CNT = 4
DUAL_FAN_CNT = 8


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class FansTest(CommonFanUtilBasedFansTest, unittest.TestCase):
    def pal_get_tach_cnt(self) -> int:
        libpal = ctypes.CDLL("libpal.so.0")
        fan_cnt = libpal.pal_get_tach_cnt()
        if fan_cnt == UNKNOWN_FAN_CNT:
            raise ValueError("Unknown fan count")

        return fan_cnt

    def setUp(self):
        Logger.start(name=self._testMethodName)

        fan_cnt = self.pal_get_tach_cnt()
        self.fans = [i for i in range(fan_cnt)]
        self.pwms = {0: self.fans}

        if fan_cnt == DUAL_FAN_CNT:
            self.names = {
                "Fan 0 Front": 0,
                "Fan 0 Rear": 1,
                "Fan 1 Front": 2,
                "Fan 1 Rear": 3,
                "Fan 2 Front": 4,
                "Fan 2 Rear": 5,
                "Fan 3 Front": 6,
                "Fan 3 Rear": 7,
            }
        elif fan_cnt == SINGLE_FAN_CNT:
            self.names = {
                "Fan 0 Front": 0,
                "Fan 1 Front": 1,
                "Fan 2 Front": 2,
                "Fan 3 Front": 3,
            }

        self.kill_fan_ctrl_cmd = ["/usr/bin/sv stop fscd"]
        self.start_fan_ctrl_cmd = ["/usr/bin/sv start fscd"]

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        self.kill_fan_controller()
        self.start_fan_controller()
