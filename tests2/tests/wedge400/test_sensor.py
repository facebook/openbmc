#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
from utils.shell_util import run_shell_cmd

from common.base_sensor_test import SensorUtilTest
from tests.wedge400.test_data.sensors.sensors import (
    PEM1_SENSORS,
    PEM2_SENSORS,
    PSU1_SENSORS,
    PSU2_SENSORS,
    SCM_SENSORS,
    SMB_SENSORS_W400,
    SMB_SENSORS_W400CEVT,
    SMB_SENSORS_W400CEVT2,
)


class ScmSensorTest(SensorUtilTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util scm"]

    def test_scm_sensor_keys(self):
        result = self.get_parsed_result()
        if not result["present"]:
            self.skipTest("scm is not present")

        for key in SCM_SENSORS:
            with self.subTest(key=key):
                self.assertIn(
                    key, result.keys(), "Missing key {} in scm sensor data".format(key)
                )

    def test_scm_temp_sensor_range(self):
        result = self.get_parsed_result()
        TEMP_KEYS = [
            "SCM_OUTLET_TEMP",
            "SCM_INLET_TEMP",
            "MB_OUTLET_TEMP",
            "MB_INLET_TEMP",
            "SOC_TEMP",
            "SOC_DIMMA_TEMP",
            "SOC_DIMMB_TEMP",
        ]
        for key in TEMP_KEYS:
            with self.subTest(key=key):
                value = result[key]
                self.assertAlmostEqual(
                    float(value),
                    50,
                    delta=30,
                    msg="Sensor={} reported value={} not within range".format(
                        key, value
                    ),
                )


class SmbSensorTest(SensorUtilTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util smb"]

    def test_smb_sensor_keys(self):
        data = run_shell_cmd(". openbmc-utils.sh && wedge_board_type_rev")
        if data == "WEDGE400_EVT/EVT3\n":
            SMB_SENSORS = SMB_SENSORS_W400
        elif data == "WEDGE400_DVT\n":
            SMB_SENSORS = SMB_SENSORS_W400
        elif data == "WEDGE400_DVT2\n":
            SMB_SENSORS = SMB_SENSORS_W400
        elif data == "WEDGE400-C_EVT\n":
            SMB_SENSORS = SMB_SENSORS_W400CEVT
        elif data == "WEDGE400-C_EVT2\n":
            SMB_SENSORS = SMB_SENSORS_W400CEVT2
        else:
            self.skipTest("Skip test for {}".format(data))
        result = self.get_parsed_result()
        for key in SMB_SENSORS:
            with self.subTest(key=key):
                self.assertIn(
                    key, result.keys(), "Missing key {} in SMB sensor data".format(key)
                )


class PemSensorTest(SensorUtilTest, unittest.TestCase):
    @abstractmethod
    def get_pem_sensors(self):
        self._pem_id = 0
        pass

    def base_test_pem_sensor_keys(self):
        result = self.get_parsed_result()
        if not result["present"]:
            self.skipTest("pem{} is not present".format(self._pem_id))

        for key in self.get_pem_sensors():
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in pem{} sensor data".format(key, self._pem_id),
                )


class Pem1SensorTest(PemSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pem1"]
        self._pem_id = 1

    def get_pem_sensors(self):
        return PEM1_SENSORS

    def test_pem1_sensor_keys(self):
        super().base_test_pem_sensor_keys()


class Pem2SensorTest(PemSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util pem2"]
        self._pem_id = 2

    def get_pem_sensors(self):
        return PEM2_SENSORS

    def test_pem2_sensor_keys(self):
        super().base_test_pem_sensor_keys()


class PsuSensorTest(SensorUtilTest, unittest.TestCase):
    @abstractmethod
    def get_psu_sensors(self):
        self._psu_id = 0
        pass

    def base_test_psu_sensor_keys(self):
        result = self.get_parsed_result()
        if not result["present"]:
            self.skipTest("psu{} is not present".format(self._psu_id))

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
