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

from common.base_sensor_test import LmSensorsTest
from tests.yamp.test_data.sensors.sensors import (
    FAN_SENSORS,
    PIM_MAX_SENSORS,
    PIM_PMBUS_SENSORS,
    PIM_VOLT_SENSORS,
    SUP_TEMP_SENSORS,
    TH3_SENSORS,
)


class BasePimSensorTest(LmSensorsTest, unittest.TestCase):
    def base_test_pim_sensor_keys(self, keyset):
        result = self.get_parsed_result()
        for key in keyset:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in pim{} sensor data. Expected set ={}".format(
                        key, self._pim_id, keyset
                    ),
                )


class BasePimMaxSensorTest(BasePimSensorTest):
    def get_pim_max_keys(self, pim):
        PIM_MAX_KEYS = []
        for item in PIM_MAX_SENSORS:
            PIM_MAX_KEYS.append(item.format(pim))
        return PIM_MAX_KEYS

    def base_test_pim_max_sensor_keys(self, pim):
        super().base_test_pim_sensor_keys(self.get_pim_max_keys(pim))

    def base_pim_temp_range_check(self, pim):
        result = self.get_parsed_result()
        for key in self.get_pim_max_keys(pim):
            with self.subTest(key=key):
                value = result[key]
                value = value.split("+")[1].split(" C")[0]
                self.assertAlmostEqual(
                    float(value),
                    40,
                    delta=20,
                    msg="{} value is {} not within range".format(key, value),
                )


class BasePimVoltSensorTest(BasePimSensorTest):
    def get_pim_volt_keys(self, pim):
        PIM_VOLT_KEYS = []
        for item in PIM_VOLT_SENSORS:
            PIM_VOLT_KEYS.append(item.format(pim))
        return PIM_VOLT_KEYS

    def base_test_pim_volt_sensor_keys(self, pim):
        super().base_test_pim_sensor_keys(self.get_pim_volt_keys(pim))


class BasePimPmbusSensorTest(BasePimSensorTest):
    def get_pim_pmbus_keys(self, pim):
        PIM_PMBUS_KEYS = []
        for item in PIM_PMBUS_SENSORS:
            PIM_PMBUS_KEYS.append(item.format(pim))
        return PIM_PMBUS_KEYS

    def base_test_pim_pmbus_sensor_keys(self, pim):
        super().base_test_pim_sensor_keys(self.get_pim_pmbus_keys(pim))


class Pim1SensorTest(BasePimPmbusSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors pmbus-i2c-14-42"]
        self._pim_id = 1

    def test_pim1_pmbus_sensor_keys(self):
        super().base_test_pim_pmbus_sensor_keys(1)


class Pim2SensorTest(BasePimPmbusSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors pmbus-i2c-15-42"]
        self._pim_id = 2

    def test_pim2_pmbus_sensor_keys(self):
        super().base_test_pim_pmbus_sensor_keys(2)


class Pim3SensorTest(BasePimPmbusSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors pmbus-i2c-16-42"]
        self._pim_id = 3

    def test_pim3_pmbus_sensor_keys(self):
        super().base_test_pim_pmbus_sensor_keys(3)


class Pim4SensorTest(BasePimPmbusSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors pmbus-i2c-17-42"]
        self._pim_id = 4

    def test_pim4_pmbus_sensor_keys(self):
        super().base_test_pim_pmbus_sensor_keys(4)


class Pim5SensorTest(BasePimPmbusSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors pmbus-i2c-18-42"]
        self._pim_id = 5

    def test_pim5_pmbus_sensor_keys(self):
        super().base_test_pim_pmbus_sensor_keys(5)


class Pim6SensorTest(BasePimPmbusSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors pmbus-i2c-19-42"]
        self._pim_id = 6

    def test_pim6_pmbus_sensor_keys(self):
        super().base_test_pim_pmbus_sensor_keys(6)


class Pim7SensorTest(BasePimPmbusSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors pmbus-i2c-20-42"]
        self._pim_id = 7

    def test_pim7_pmbus_sensor_keys(self):
        super().base_test_pim_pmbus_sensor_keys(7)


class Pim8SensorTest(BasePimPmbusSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors pmbus-i2c-21-42"]
        self._pim_id = 8

    def test_pim8_pmbus_sensor_keys(self):
        super().base_test_pim_pmbus_sensor_keys(8)


class Pim1MaxSensorTest(BasePimMaxSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max6658-i2c-14-4c"]
        self._pim_id = 1

    def test_pim1_max_sensor_keys(self):
        super().base_test_pim_max_sensor_keys(1)

    def test_pim1_max_sensor_temp_range(self):
        super().base_pim_temp_range_check(1)


class Pim2MaxSensorTest(BasePimMaxSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max6658-i2c-15-4c"]
        self._pim_id = 2

    def test_pim2_max_sensor_keys(self):
        super().base_test_pim_max_sensor_keys(2)

    def test_pim2_max_sensor_temp_range(self):
        super().base_pim_temp_range_check(2)


class Pim3MaxSensorTest(BasePimMaxSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max6658-i2c-16-4c"]
        self._pim_id = 3

    def test_pim3_max_sensor_keys(self):
        super().base_test_pim_max_sensor_keys(3)

    def test_pim3_max_sensor_temp_range(self):
        super().base_pim_temp_range_check(3)


class Pim4MaxSensorTest(BasePimMaxSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max6658-i2c-17-4c"]
        self._pim_id = 4

    def test_pim4_max_sensor_keys(self):
        super().base_test_pim_max_sensor_keys(4)

    def test_pim4_max_sensor_temp_range(self):
        super().base_pim_temp_range_check(4)


class Pim5MaxSensorTest(BasePimMaxSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max6658-i2c-18-4c"]
        self._pim_id = 5

    def test_pim5_max_sensor_keys(self):
        super().base_test_pim_max_sensor_keys(5)

    def test_pim5_max_sensor_temp_range(self):
        super().base_pim_temp_range_check(5)


class Pim6MaxSensorTest(BasePimMaxSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max6658-i2c-19-4c"]
        self._pim_id = 6

    def test_pim6_max_sensor_keys(self):
        super().base_test_pim_max_sensor_keys(6)

    def test_pim6_max_sensor_temp_range(self):
        super().base_pim_temp_range_check(6)


class Pim7MaxSensorTest(BasePimMaxSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max6658-i2c-20-4c"]
        self._pim_id = 7

    def test_pim7_max_sensor_keys(self):
        super().base_test_pim_max_sensor_keys(7)

    def test_pim7_max_sensor_temp_range(self):
        super().base_pim_temp_range_check(7)


class Pim8MaxSensorTest(BasePimMaxSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max6658-i2c-21-4c"]
        self._pim_id = 8

    def test_pim8_max_sensor_keys(self):
        super().base_test_pim_max_sensor_keys(8)

    def test_pim8_max_sensor_temp_range(self):
        super().base_pim_temp_range_check(8)


class Pim1VoltSensorTest(BasePimVoltSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors ucd90120-i2c-14-4e"]
        self._pim_id = 1

    def test_pim1_max_sensor_keys(self):
        super().base_test_pim_volt_sensor_keys(1)


class Pim2VoltSensorTest(BasePimVoltSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors ucd90120-i2c-15-4e"]
        self._pim_id = 2

    def test_pim2_max_sensor_keys(self):
        super().base_test_pim_volt_sensor_keys(2)


class Pim3VoltSensorTest(BasePimVoltSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors ucd90120-i2c-16-4e"]
        self._pim_id = 3

    def test_pim3_max_sensor_keys(self):
        super().base_test_pim_volt_sensor_keys(3)


class Pim4VoltSensorTest(BasePimVoltSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors ucd90120-i2c-17-4e"]
        self._pim_id = 4

    def test_pim4_max_sensor_keys(self):
        super().base_test_pim_volt_sensor_keys(4)


class Pim5VoltSensorTest(BasePimVoltSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors ucd90120-i2c-18-4e"]
        self._pim_id = 5

    def test_pim5_max_sensor_keys(self):
        super().base_test_pim_volt_sensor_keys(5)


class Pim6VoltSensorTest(BasePimVoltSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors ucd90120-i2c-19-4e"]
        self._pim_id = 6

    def test_pim6_max_sensor_keys(self):
        super().base_test_pim_volt_sensor_keys(6)


class Pim7VoltSensorTest(BasePimVoltSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors ucd90120-i2c-20-4e"]
        self._pim_id = 7

    def test_pim7_max_sensor_keys(self):
        super().base_test_pim_volt_sensor_keys(7)


class Pim8VoltSensorTest(BasePimVoltSensorTest):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors ucd90120-i2c-21-4e"]
        self._pim_id = 8

    def test_pim8_max_sensor_keys(self):
        super().base_test_pim_volt_sensor_keys(8)


class SupSensorTest(LmSensorsTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max6658-i2c-11-4c"]

    def test_sup_sensor_keys(self):
        result = self.get_parsed_result()
        for key in SUP_TEMP_SENSORS:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in sup: max6658-i2c-11-4c data".format(key),
                )

    def test_sup_temp_range(self):
        result = self.get_parsed_result()
        for key in SUP_TEMP_SENSORS:
            with self.subTest(key=key):
                value = result[key]
                value = value.split("+")[1].split(" C")[0]
                self.assertAlmostEqual(
                    float(value),
                    40,
                    delta=20,
                    msg="{} value is {} not within range".format(key, value),
                )


class Th3SensorTest(LmSensorsTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors pmbus-i2c-3-44"]

    def test_th3_sensor_keys(self):
        result = self.get_parsed_result()
        for key in TH3_SENSORS:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in th3: pmbus-i2c-3-44, expected={}".format(
                        key, TH3_SENSORS
                    ),
                )


class FanSensorTest(LmSensorsTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors fancpld-*"]

    def test_fan_sensor_keys(self):
        result = self.get_parsed_result()
        for key in FAN_SENSORS:
            with self.subTest(key=key):
                self.assertIn(
                    key, result.keys(), "Missing key {} in fancpld data".format(key)
                )
