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
import re
import unittest

from common.base_sensor_test import SensorUtilTest
from tests.fbtp.test_data.sensors.sensors import SENSORS
from utils.cit_logger import Logger
from utils.shell_util import run_cmd
from utils.test_utils import check_fru_availability, qemu_check


class MBSensorTest(SensorUtilTest, unittest.TestCase):
    FRU_NAME = "mb"

    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util {}".format(self.FRU_NAME)]

    def test_sensor_keys(self):
        result = self.get_parsed_result()
        for key in SENSORS[self.FRU_NAME]:
            with self.subTest(sensor=key):
                self.assertIn(key, result.keys(), "Missing sensor {}".format(key))


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class NicSensorTest(MBSensorTest):
    FRU_NAME = "nic"


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Riser2SensorTest(MBSensorTest):
    FRU_NAME = "riser_slot2"

    def test_sensor_keys(self):
        if not check_fru_availability(self.FRU_NAME):
            self.skipTest("skip test due to {} not available".format(self.FRU_NAME))
        # for fbtp, T6/8 won't have riser sensor, so we'll skip the test
        cmd = ["kv", "get", "mb_system_conf_desc", "persistent"]
        m = re.search("Type 6/8 compute", run_cmd(cmd))
        if m:
            Logger.info("{} sensor not present, skip the test".format(self.FRU_NAME))
            self.assertTrue(True)
            return
        result = self.get_parsed_result()
        for key in SENSORS[self.FRU_NAME]:
            with self.subTest(sensor=key):
                self.assertIn(key, result.keys(), "Missing sensor {}".format(key))


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Riser3SensorTest(Riser2SensorTest):
    FRU_NAME = "riser_slot3"


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Riser4SensorTest(Riser2SensorTest):
    FRU_NAME = "riser_slot4"
