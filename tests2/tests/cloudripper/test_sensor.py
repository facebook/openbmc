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
from abc import abstractmethod

from common.base_sensor_test import SensorUtilTest
from tests.cloudripper.test_data.sensors.sensor import (
    PSU1_SENSORS,
    PSU2_SENSORS,
    SCM_SENSORS,
    SMB_SENSORS,
)


class ScmSensorTest(SensorUtilTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util scm"]

    def test_scm_sensor_keys(self):
        result = self.get_parsed_result()
        for key in SCM_SENSORS:
            with self.subTest(key=key):
                self.assertIn(
                    key, result.keys(), "Missing key {} in scm sensor data".format(key)
                )

    def test_scm_temp_sensor_range(self):
        result = self.get_json_threshold_result()
        for key in result.keys():
            with self.subTest(key=key):
                data = result[key]
                if isinstance(data, bool):  # Filter non-sensor item out
                    continue
                ucr = None
                lcr = None
                value = data["value"]

                self.assertNotEqual(  # Assert if value is N/A
                    value,
                    "NA",
                    msg="Sensor={}, reported value={}, is unmeasurable".format(
                        key, value
                    ),
                )

                if data.get("thresholds", None):
                    ucr = data["thresholds"].get("UCR", None)
                    lcr = data["thresholds"].get("LCR", None)

                if ucr:  # Assert if value is above UCR
                    self.assertLess(
                        float(value),
                        float(ucr),
                        msg="Sensor={} reported value={} is over UCR={}".format(
                            key, value, ucr
                        ),
                    )

                if lcr:  # Assert if value is below LCR
                    self.assertGreater(
                        float(value),
                        float(lcr),
                        msg="Sensor={} reported value={} is below LCR={}".format(
                            key, value, lcr
                        ),
                    )


class PsuSensorTest(SensorUtilTest, unittest.TestCase):
    @abstractmethod
    def get_psu_sensors(self):
        self._psu_id = 0
        pass

    def base_test_psu_sensor_keys(self):
        result = self.get_parsed_result()
        for key in self.get_psu_sensors():
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in psu{} sensor data".format(key, self._psu_id),
                )


class Psu1SensorTest(PsuSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util psu1"]
        self._psu_id = 1

    def get_psu_sensors(self):
        return PSU1_SENSORS

    def test_psu1_sensor_keys(self):
        super().base_test_psu_sensor_keys()


class Psu2SensorTest(PsuSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util psu2"]
        self._psu_id = 2

    def get_psu_sensors(self):
        return PSU2_SENSORS

    def test_psu2_sensor_keys(self):
        super().base_test_psu_sensor_keys()


class SmbSensorTest(SensorUtilTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util smb"]

    def test_smb_sensor_keys(self):
        result = self.get_parsed_result()
        for key in SMB_SENSORS:
            with self.subTest(key=key):
                self.assertIn(
                    key, result.keys(), "Missing key {} in SMB sensor data".format(key)
                )
