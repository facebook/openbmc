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


# ast_pwm-isa-0000 : covered as part of fans test
class PantherPlus(LmSensorsTest, unittest.TestCase):
    PANTHER_PLUS = ["CPU Temp", "DIMM0 Temp"]

    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors fb_panther_plus-i2c-4-40"]

    def test_panther_plus_sensor_keys(self):
        result = self.get_parsed_result()
        for key in PantherPlus.PANTHER_PLUS:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in panther plus data".format(key),
                )

    def test_panther_plus_Memory_Temp_range(self):
        result = self.get_parsed_result()
        value = result["DIMM0 Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            30,
            delta=20,
            msg="Memory Temp value is {} not within range".format(value),
        )

    def test_panther_plus_CPU_Temp_range(self):
        result = self.get_parsed_result()
        value = result["CPU Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            20,
            delta=10,
            msg="CPU Temp value is {} not within range".format(value),
        )


class Tmp75SensorTest(LmSensorsTest, unittest.TestCase):
    TMP75_SENSORS = ["Switch Temp", "Inlet Temp", "Outlet Temp"]

    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors tmp75-*"]

    def test_tmp75_sensor_keys(self):
        result = self.get_parsed_result()
        for key in Tmp75SensorTest.TMP75_SENSORS:
            with self.subTest(key=key):
                self.assertIn(
                    key, result.keys(), "Missing key {} in tmp75 data".format(key)
                )

    def test_tmp75_sensor_Outlet_Middle_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Outlet Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            25,
            delta=10,
            msg="Outlet Temp value is {} not within range".format(value),
        )

    def test_tmp75_sensor_Inlet_Middle_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Inlet Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            25,
            delta=10,
            msg="Inlet Temp value is {} not within range".format(value),
        )

    def test_tmp75_sensor_Switch_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Switch Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            35,
            delta=15,
            msg="Switch Temp value is {} not within range".format(value),
        )


class Max127Sensor(LmSensorsTest, unittest.TestCase):
    MAX127_SENSOR = ["+1 Voltage", "+2.5 Voltage", "+3.3 Voltage", "+5 Voltage"]

    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors max127-i2c-6-28"]

    def test_max127_sensor_keys(self):
        result = self.get_parsed_result()
        for key in Max127Sensor.MAX127_SENSOR:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in max127-i2c-6-28 data".format(key),
                )

    def test_max127_sensor_data_1V_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["+1 Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            1,
            delta=1,
            msg="+1V Voltage value is {} not within range".format(value),
        )

    def test_max127_sensor_data_2_5V_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["+2.5 Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            2.5,
            delta=1,
            msg="+2.5V Voltage value is {} not within range".format(value),
        )

    def test_max127_sensor_data_3_3V_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["+3.3 Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            3.3,
            delta=1,
            msg="+3.3V Voltage value is {} not within range".format(value),
        )

    def test_max127_sensor_data_5V_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["+5 Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            5,
            delta=1,
            msg="+5V Voltage value is {} not within range".format(value),
        )


class Adm1278Sensor(LmSensorsTest, unittest.TestCase):
    ADM1278_SENSOR = ["vin", "pin", "iout1"]

    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors adm1278-i2c-12-10"]

    def test_adm1278_sensor_keys(self):
        result = self.get_parsed_result()
        for key in Adm1278Sensor.ADM1278_SENSOR:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in adm1278-i2c-12-10 data".format(key),
                )


class NcpSensor(LmSensorsTest, unittest.TestCase):
    NCP_ADAPTOR = ["ncp4200-i2c-1-60", "ncp4200-i2c-2-60", "ncp4200-i2c-8-60"]

    NCP_SENSOR_KEYS = ["vin", "vout1", "temp1", "pout1", "iout1"]

    def set_sensors_cmd(self):
        sensor_string = "sensors " + self.ncp_driver
        self.sensors_cmd = [sensor_string]

    def base_test_ncp_sensor_keys(self):
        result = self.get_parsed_result()
        for key in NcpSensor.NCP_SENSOR_KEYS:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in {} data".format(key, self.ncp_driver),
                )

    def test_ncp_sensor_keys(self):
        for item in self.NCP_ADAPTOR:
            self.ncp_driver = item
            self.base_test_ncp_sensor_keys()
