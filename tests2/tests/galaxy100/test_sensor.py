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
import os
import unittest

from common.base_sensor_test import LmSensorsTest
from utils.test_utils import qemu_check


class AstAdcIsa0000SensorTest(LmSensorsTest, unittest.TestCase):
    AST_ADC_ISA_0000_DRIVER = [
        "+5 Bias Voltage",
        "+6.8 Bias Voltage",
        "+3.3 Voltage standby",
        "+2.5 Voltage standby",
    ]

    def set_sensors_cmd(self):
        if os.path.exists("/sys/devices/platform/ast-adc-hwmon/"):
            self.sensors_cmd = ["sensors ast_adc_hwmon-isa-0000"]
        else:
            self.sensors_cmd = ["sensors ast_adc-isa-0000"]

    def test_ast_adc_isa_0000_sensor_keys(self):
        result = self.get_parsed_result()
        for key in AstAdcIsa0000SensorTest.AST_ADC_ISA_0000_DRIVER:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in AST_ADC-ISA-0000_driver data".format(key),
                )


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Galaxy100Ec12c033SensorTest(LmSensorsTest, unittest.TestCase):
    GALAXY100_EC_12C_0_33_DRIVER = [
        "CPU Voltage",
        "+3 Voltage",
        "+5 Voltage",
        "+12 Voltage",
        "DIMM Voltage",
        "CPU Temp",
        "DIMM Temp",
    ]

    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors galaxy100_ec-i2c-0-33"]

    def test_galaxy100_ec_12c_0_33_sensor_keys(self):
        result = self.get_parsed_result()
        for key in Galaxy100Ec12c033SensorTest.GALAXY100_EC_12C_0_33_DRIVER:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in GALAXY100_EC_12C_0_33_driver data".format(key),
                )

    def test_galaxy100_ec_12c_0_33_sensor_data_3V_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["+3 Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            3,
            delta=1,
            msg="+3V Voltage value is {} not within range".format(value),
        )

    def test_galaxy100_ec_12c_0_33_sensor_data_5V_Voltage_range(self):
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

    def test_galaxy100_ec_12c_0_33_sensor_data_12V_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["+12 Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            12,
            delta=1,
            msg="+12V Voltage value is {} not within range".format(value),
        )

    def test_galaxy100_ec_12c_0_33_sensor_data_CPU_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["CPU Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            1,
            delta=1,
            msg="CPU Voltage value is {} not within range".format(value),
        )

    def test_galaxy100_ec_12c_0_33_sensor_data_DIMM_Voltage_range(self):
        result = self.get_parsed_result()
        value = result["DIMM Voltage"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" V")[0]
        self.assertAlmostEqual(
            float(value),
            1,
            delta=1,
            msg="DIMM Voltage value is {} not within range".format(value),
        )

    def test_galaxy100_ec_12c_0_33_sensor_data_CPU_Temp_range(self):
        result = self.get_parsed_result()
        value = result["CPU Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            38,
            delta=22,
            msg="CPU Temp value is {} not within range".format(value),
        )

    def test_galaxy100_ec_12c_0_33_sensor_data_DIMM_Temp_range(self):
        result = self.get_parsed_result()
        value = result["DIMM Temp"]
        # '+12.23 V' extract 12.23 form it
        value = value.split("+")[1].split(" C")[0]
        self.assertAlmostEqual(
            float(value),
            40,
            delta=20,
            msg="DIMM Temp value is {} not within range".format(value),
        )


class Ir3584I2c172SensorTest(LmSensorsTest, unittest.TestCase):
    IR3584_I2C_1_72_DRIVER = ["in0", "curr1"]

    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors IR3584-i2c-1-72"]

    def test_ir3584_i2c_1_72_sensor_keys(self):
        result = self.get_parsed_result()
        for key in Ir3584I2c172SensorTest.IR3584_I2C_1_72_DRIVER:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in IR3584_I2C_1_72_driver data".format(key),
                )


class Ir3581I2c170SensorTest(LmSensorsTest, unittest.TestCase):
    IR3581_I2C_1_70_DRIVER = ["in0", "curr1"]

    def set_sensors_cmd(self):
        self.sensors_cmd = ["sensors IR3581-i2c-1-70"]

    def test_ir3581_i2c_1_70_sensor_keys(self):
        result = self.get_parsed_result()
        for key in Ir3581I2c170SensorTest.IR3581_I2C_1_70_DRIVER:
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in IR3581_I2C_1_70_driver data".format(key),
                )
