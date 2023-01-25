#!/usr/bin/env python3
#
# Copyright 2022-present Facebook. All Rights Reserved.
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
from tests.elbert.helper.libpal import (
    pal_get_fru_id,
    pal_get_pim_type,
    pal_is_fru_prsnt,
)
from tests.elbert.test_data.sensors.sensors import (
    FAN_SENSORS,
    PIM_16Q2_SENSORS,
    PIM_16Q_SENSORS,
    PIM_8DDM_SENSORS,
    PSU_SENSORS,
    SCM_SENSORS,
    SMB_SENSORS,
)
from utils.test_utils import qemu_check


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


class FanSensorTest(SensorUtilTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util fan"]

    def test_fan_sensor_keys(self):
        result = self.get_parsed_result()
        for key in FAN_SENSORS:
            with self.subTest(key=key):
                self.assertIn(
                    key, result.keys(), "Missing key {} in FAN sensor data".format(key)
                )


class PimSensorTest(SensorUtilTest, unittest.TestCase):
    def set_pim_id(self):
        self._pim_id = 2

    def set_sensors_cmd(self):
        self.set_pim_id()
        self._fru_id = pal_get_fru_id("pim{}".format(self._pim_id))
        if not pal_is_fru_prsnt(self._fru_id):
            self.skipTest("pim{} is not present".format(self._pim_id))
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim{}".format(self._pim_id)]

    def get_pim_sensors(self):
        fru_id = pal_get_fru_id("pim{}".format(self._pim_id))
        if not pal_is_fru_prsnt(fru_id):
            self.skipTest("pim{} is not present".format(self._pim_id))
        pim_type = pal_get_pim_type(fru_id)
        if pim_type in ["UNPLUG", "NONE"]:
            self.skipTest("pim{} type unknown".format(self._pim_id))
        sensor_list = []
        if pim_type == "16Q":
            sensor_list = PIM_16Q_SENSORS
        elif pim_type == "16Q2":
            sensor_list = PIM_16Q2_SENSORS
        elif pim_type == "8DDM":
            sensor_list = PIM_8DDM_SENSORS
        return [sensor.format(self._pim_id) for sensor in sensor_list]

    def base_test_pim_sensor_keys(self):
        self.set_sensors_cmd()
        result = self.get_parsed_result()
        for key in self.get_pim_sensors():
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in pim{} sensor data".format(key, self._pim_id),
                )


class Pim2SensorTest(PimSensorTest, unittest.TestCase):
    def set_pim_id(self):
        self._pim_id = 2

    def test_pim2_sensor_keys(self):
        super().base_test_pim_sensor_keys()


class Pim3SensorTest(PimSensorTest, unittest.TestCase):
    def set_pim_id(self):
        self._pim_id = 3

    def test_pim3_sensor_keys(self):
        super().base_test_pim_sensor_keys()


class Pim4SensorTest(PimSensorTest, unittest.TestCase):
    def set_pim_id(self):
        self._pim_id = 4

    def test_pim4_sensor_keys(self):
        super().base_test_pim_sensor_keys()


class Pim5SensorTest(PimSensorTest, unittest.TestCase):
    def set_pim_id(self):
        self._pim_id = 5

    def test_pim5_sensor_keys(self):
        super().base_test_pim_sensor_keys()


class Pim6SensorTest(PimSensorTest, unittest.TestCase):
    def set_pim_id(self):
        self._pim_id = 6

    def test_pim6_sensor_keys(self):
        super().base_test_pim_sensor_keys()


class Pim7SensorTest(PimSensorTest, unittest.TestCase):
    def set_pim_id(self):
        self._pim_id = 7

    def test_pim7_sensor_keys(self):
        super().base_test_pim_sensor_keys()


class Pim8SensorTest(PimSensorTest, unittest.TestCase):
    def set_pim_id(self):
        self._pim_id = 8

    def test_pim8_sensor_keys(self):
        super().base_test_pim_sensor_keys()


class PsuSensorTest(SensorUtilTest, unittest.TestCase):
    def set_psu_id(self):
        self._psu_id = 1

    def set_sensors_cmd(self):
        self.set_psu_id()
        if not pal_is_fru_prsnt(pal_get_fru_id("psu{}".format(self._psu_id))):
            self.skipTest("psu{} is not present".format(self._psu_id))
        self.sensors_cmd = ["/usr/local/bin/sensor-util psu{}".format(self._psu_id)]

    def get_psu_sensors(self):
        return [sensor.format(self._psu_id) for sensor in PSU_SENSORS]

    def base_test_psu_sensor_keys(self):
        self.set_sensors_cmd()
        result = self.get_parsed_result()
        for key in self.get_psu_sensors():
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in psu{} sensor data".format(key, self._psu_id),
                )


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Psu1SensorTest(PsuSensorTest, unittest.TestCase):
    def set_psu_id(self):
        self._psu_id = 1

    def test_psu1_sensor_keys(self):
        super().base_test_psu_sensor_keys()


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Psu2SensorTest(PsuSensorTest, unittest.TestCase):
    def set_psu_id(self):
        self._psu_id = 2

    def test_psu2_sensor_keys(self):
        super().base_test_psu_sensor_keys()


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Psu3SensorTest(PsuSensorTest, unittest.TestCase):
    def set_psu_id(self):
        self._psu_id = 3

    def test_psu3_sensor_keys(self):
        super().base_test_psu_sensor_keys()


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Psu4SensorTest(PsuSensorTest, unittest.TestCase):
    def set_psu_id(self):
        self._psu_id = 4

    def test_psu4_sensor_keys(self):
        super().base_test_psu_sensor_keys()


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class AllSensorTest(SensorUtilTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util all"]

    def get_all_sensors(self):
        sensor_list = []
        sensor_list.extend(SCM_SENSORS)
        sensor_list.extend(SMB_SENSORS)

        for pim_id in range(2, 10):
            pim_fru_id = pal_get_fru_id("pim{}".format(pim_id))
            if pal_is_fru_prsnt(pim_fru_id):
                pim_type = pal_get_pim_type(pim_fru_id)
                if pim_type == "16Q":
                    sensor_list.extend(
                        [sensor.format(pim_id) for sensor in PIM_16Q_SENSORS]
                    )
                elif pim_type == "16Q2":
                    sensor_list.extend(
                        [sensor.format(pim_id) for sensor in PIM_16Q2_SENSORS]
                    )
                elif pim_type == "8DDM":
                    sensor_list.extend(
                        [sensor.format(pim_id) for sensor in PIM_8DDM_SENSORS]
                    )

        for psu_id in range(1, 5):
            if pal_is_fru_prsnt(pal_get_fru_id("psu{}".format(psu_id))):
                sensor_list.extend([sensor.format(psu_id) for sensor in PSU_SENSORS])

        return sensor_list

    def test_all_sensor_keys(self):
        result = self.get_parsed_result()
        for key in self.get_all_sensors():
            with self.subTest(key=key):
                self.assertIn(
                    key, result.keys(), "Missing key {} in ALL sensor data".format(key)
                )
