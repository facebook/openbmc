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

from common.base_sensor_test import SensorUtilTest
from tests.fby35.test_data.sensors.sensors import SENSORS
from utils.test_utils import check_fru_availability, qemu_check


class Slot1SensorTest(SensorUtilTest, unittest.TestCase):
    FRU_NAME = "slot1"

    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util {}".format(self.FRU_NAME)]

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_sensor_keys(self):
        if not check_fru_availability(self.FRU_NAME):
            self.skipTest("skip test due to {} not available".format(self.FRU_NAME))
        result = self.get_parsed_result()
        for key in SENSORS[self.FRU_NAME]:
            with self.subTest(sensor=key):
                self.assertIn(key, result.keys(), "Missing sensor {}".format(key))


class Slot2SensorTest(Slot1SensorTest):
    FRU_NAME = "slot2"


class Slot3SensorTest(Slot1SensorTest):
    FRU_NAME = "slot3"


class Slot4SensorTest(Slot1SensorTest):
    FRU_NAME = "slot4"


class NicSensorTest(Slot1SensorTest):
    FRU_NAME = "nic"


class BmcSensorTest(Slot1SensorTest):
    FRU_NAME = "bmc"
