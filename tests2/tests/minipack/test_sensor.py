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
from abc import abstractmethod

from common.base_sensor_test import SensorUtilTest
from tests.minipack.test_data.sensors.sensors import (
    PIM1_SENSORS,
    PIM2_SENSORS,
    PIM3_SENSORS,
    PIM4_SENSORS,
    PIM5_SENSORS,
    PIM6_SENSORS,
    PIM7_SENSORS,
    PIM8_SENSORS,
    PSU1_SENSORS,
    PSU2_SENSORS,
    PSU3_SENSORS,
    PSU4_SENSORS,
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
        result = self.get_parsed_result()
        OUTLET_TEMP_KEYS = [
            "SCM_OUTLET_LOCAL_TEMP",
            "SCM_OUTLET_REMOTE_TEMP",
            "SCM_INLET_LOCAL_TEMP",
            "SCM_INLET_REMOTE_TEMP",
            "MB_OUTLET_TEMP",
            "MB_INLET_TEMP",
            "SOC_TEMP",
            "SOC_DIMMA0_TEMP",
            "SOC_DIMMB0_TEMP",
        ]
        for key in OUTLET_TEMP_KEYS:
            with self.subTest(key=key):
                value = result[key]
                self.assertAlmostEqual(
                    float(value),
                    30,
                    delta=20,
                    msg="Sensor={} reported value={} not within range".format(
                        key, value
                    ),
                )


class PimSensorTest(SensorUtilTest, unittest.TestCase):
    @abstractmethod
    def get_pim_sensors(self):
        self._pim_id = 0
        pass

    def get_pim_temp_keys(self):
        PIM_TEMP_KEYS = []
        PIM_TEMP_KEYS.append("PIM{}_TEMP1".format(self._pim_id))
        PIM_TEMP_KEYS.append("PIM{}_TEMP1".format(self._pim_id))
        return PIM_TEMP_KEYS

    def base_test_pim_sensor_keys(self):
        result = self.get_parsed_result()
        for key in self.get_pim_sensors():
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in pim{} sensor data".format(key, self._pim_id),
                )

    def base_test_pim_temp_sensor_range(self):
        result = self.get_parsed_result()
        PIM_TEMP_KEYS = self.get_pim_temp_keys()
        for key in PIM_TEMP_KEYS:
            with self.subTest(key=key):
                value = result[key]
                self.assertAlmostEqual(
                    float(value),
                    30,
                    delta=20,
                    msg="Sensor={} reported value={} not within range".format(
                        key, value
                    ),
                )


class Pim1SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim1"]
        self._pim_id = 1

    def get_pim_sensors(self):
        return PIM1_SENSORS

    def test_pim1_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim1_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim2SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim2"]
        self._pim_id = 2

    def get_pim_sensors(self):
        return PIM2_SENSORS

    def test_pim2_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim2_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim3SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim3"]
        self._pim_id = 3

    def get_pim_sensors(self):
        return PIM3_SENSORS

    def test_pim3_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim3_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim4SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim4"]
        self._pim_id = 4

    def get_pim_sensors(self):
        return PIM4_SENSORS

    def test_pim4_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim4_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim5SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim5"]
        self._pim_id = 5

    def get_pim_sensors(self):
        return PIM5_SENSORS

    def test_pim5_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim5_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim6SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim6"]
        self._pim_id = 6

    def get_pim_sensors(self):
        return PIM6_SENSORS

    def test_pim6_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim6_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim7SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim7"]
        self._pim_id = 7

    def get_pim_sensors(self):
        return PIM7_SENSORS

    def test_pim7_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim7_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim8SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim8"]
        self._pim_id = 8

    def get_pim_sensors(self):
        return PIM8_SENSORS

    def test_pim8_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim8_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


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
                    "Missing key {} in pim{} sensor data".format(key, self._psu_id),
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


class Psu3SensorTest(PsuSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util psu3"]
        self._psu_id = 3

    def get_psu_sensors(self):
        return PSU3_SENSORS

    def test_psu3_sensor_keys(self):
        super().base_test_psu_sensor_keys()


class Psu4SensorTest(PsuSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util psu4"]
        self._psu_id = 4

    def get_psu_sensors(self):
        return PSU4_SENSORS

    def test_psu4_sensor_keys(self):
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
