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

from common.base_sensor_test import SensorUtilTest
from tests.fbttn.test_data.sensors.sensors import SENSORS


class MBSensorTest(SensorUtilTest, unittest.TestCase):
    FRU_NAME = "server"

    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util {}".format(self.FRU_NAME)]

    def test_sensor_keys(self):
        result = self.get_parsed_result()
        for key in SENSORS[self.FRU_NAME]:
            with self.subTest(sensor=key):
                self.assertIn(key, result.keys(), "Missing sensor {}".format(key))


class NicSensorTest(MBSensorTest):
    FRU_NAME = "nic"


class IOMSensorTest(MBSensorTest):
    FRU_NAME = "iom"


class DPBSensorTest(MBSensorTest):
    FRU_NAME = "dpb"


class SCCSensorTest(MBSensorTest):
    FRU_NAME = "scc"
