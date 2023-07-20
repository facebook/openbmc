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

from common.base_sensor_test import LmSensorsTest
from utils.test_utils import qemu_check


# Note: Fancpld covered under fans_test ; LTC driver covered under psumuxmon


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class ComESensorTest(LmSensorsTest, unittest.TestCase):
    COM_E_DRIVER = [
        "CPU Vcore",
        "+3V Voltage",
        "+5V Voltage",
        "+12V Voltage",
        "VDIMM Voltage",
        "Memory Temp",
        "CPU Temp",
    ]

    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors com_e_driver-*"]

    def test_com_e_sensor_keys(self):
        result = self.get_parsed_result()
        for key in ComESensorTest.COM_E_DRIVER:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in com_e_driver data".format(key),
                )

    def test_com_e_sensor_data_3V_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["+3V Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            3,
            delta=1,
            msg="+3V Voltage value is {} not within range".format(value),
        )

    def test_com_e_sensor_data_5V_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["+5V Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            5,
            delta=1,
            msg="+5V Voltage value is {} not within range".format(value),
        )

    def test_com_e_sensor_data_12V_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["+12V Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            12,
            delta=1,
            msg="+12V Voltage value is {} not within range".format(value),
        )

    def test_com_e_sensor_data_Memory_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Memory Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            30,
            delta=20,
            msg="Memory Temp value is {} not within range".format(value),
        )

    def test_com_e_sensor_data_CPU_Temp_range(self):
        result = self.get_parsed_result()
        value = result["CPU Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            40,
            delta=20,
            msg="CPU Temp value is {} not within range".format(value),
        )


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Tmp75SensorTest(LmSensorsTest, unittest.TestCase):
    TMP75_SENSORS = [
        "Outlet Middle Temp",
        "Inlet Middle Temp",
        "Switch Temp",
        "Inlet Left Temp",
        "Inlet Right Temp",
        "Outlet Right Temp",
        "Outlet Left Temp",
    ]

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
        value = result["Outlet Middle Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            25,
            delta=10,
            msg="Outlet Middle Temp value is {} not within range".format(value),
        )

    def test_tmp75_sensor_Inlet_Middle_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Inlet Middle Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            25,
            delta=10,
            msg="Inlet Middle Temp value is {} not within range".format(value),
        )

    def test_tmp75_sensor_Switch_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Switch Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            35,
            delta=10,
            msg="Switch Temp value is {} not within range".format(value),
        )

    def test_tmp75_sensor_Inlet_Left_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Inlet Left Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            25,
            delta=10,
            msg="Inlet Left Temp value is {} not within range".format(value),
        )

    def test_tmp75_sensor_Inlet_Right_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Inlet Right Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            25,
            delta=10,
            msg="Inlet Right Temp value is {} not within range".format(value),
        )

    def test_tmp75_sensor_Outlet_Right_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Outlet Right Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            25,
            delta=10,
            msg="Outlet Right Temp value is {} not within range".format(value),
        )

    def test_tmp75_sensor_Outlet_Left_Temp_range(self):
        result = self.get_parsed_result()
        value = result["Outlet Left Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            25,
            delta=10,
            msg="Outlet Left Temp value is {} not within range".format(value),
        )
