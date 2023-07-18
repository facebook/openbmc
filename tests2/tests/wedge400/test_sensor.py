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

from common.base_sensor_test import SensorUtilTest
from tests.wedge400.helper.libpal import (
    pal_get_board_type_rev,
    pal_detect_power_supply_present,
    BoardRevision,
)
from tests.wedge400.test_data.sensors.sensors import (
    PEM1_SENSORS,
    PEM2_SENSORS,
    PSU1_SENSORS,
    PSU2_SENSORS,
    SCM_SENSORS_ORIGINAL,
    SCM_SENSORS_RESPIN,
    SMB_SENSORS_W400,
    SMB_SENSORS_W400RESPIN,
    SMB_SENSORS_W400CEVT,
    SMB_SENSORS_W400CEVT2,
    SMB_SENSORS_W400CRESPIN,
    FAN1_SENSORS,
    FAN2_SENSORS,
    FAN3_SENSORS,
    FAN4_SENSORS,
)
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class ScmSensorTest(SensorUtilTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util scm"]

    def test_scm_sensor_keys(self):
        SCM_SENSORS = []
        platform_type_rev = pal_get_board_type_rev()
        if (
            platform_type_rev == "Wedge400-MP-Respin"
            or platform_type_rev == "Wedge400C-MP-Respin"
        ):
            SCM_SENSORS = SCM_SENSORS_RESPIN
        else:
            SCM_SENSORS = SCM_SENSORS_ORIGINAL

        result = self.get_parsed_result()
        if not result["present"]:
            self.skipTest("scm is not present")

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


class SmbSensorTest(SensorUtilTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util smb"]

    def test_smb_sensor_keys(self):
        SMB_SENSORS = []
        platform_type_rev = pal_get_board_type_rev()
        if (
            platform_type_rev == "Wedge400-EVT/EVT3"
            or platform_type_rev == "Wedge400-DVT"
            or platform_type_rev == "Wedge400-DVT2/PVT/PVT2"
            or platform_type_rev == "Wedge400-PVT3"
            or platform_type_rev == "Wedge400-MP"
        ):
            SMB_SENSORS = SMB_SENSORS_W400
        elif platform_type_rev == "Wedge400-MP-Respin":
            SMB_SENSORS = SMB_SENSORS_W400RESPIN
        elif platform_type_rev == "Wedge400C-EVT":
            SMB_SENSORS = SMB_SENSORS_W400CEVT
        elif (
            platform_type_rev == "Wedge400C-EVT2"
            or platform_type_rev == "Wedge400C-DVT"
            or platform_type_rev == "Wedge400C-DVT2"
        ):
            SMB_SENSORS = SMB_SENSORS_W400CEVT2
        elif platform_type_rev == "Wedge400C-MP-Respin":
            SMB_SENSORS = SMB_SENSORS_W400CRESPIN
        else:
            self.skipTest("Skip test on {} board".format(platform_type_rev))
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
        self.set_sensors_cmd()
        if (
            self._pem_id == 1
            and pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PEM1)
            != "pem1"
        ) or (
            self._pem_id == 2
            and pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PEM2)
            != "pem2"
        ):
            self.skipTest("pem{} is not present".format(self._pem_id))
        result = self.get_parsed_result()

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
        self.set_sensors_cmd()
        if (
            self._psu_id == 1
            and pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PSU1)
            != "psu1"
        ) or (
            self._psu_id == 2
            and pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PSU2)
            != "psu2"
        ):
            self.skipTest("psu{} is not present".format(self._psu_id))
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


class FanSensorTest(SensorUtilTest, unittest.TestCase):
    @abstractmethod
    def get_fan_sensors(self):
        self._fan_id = 0
        pass

    def base_test_fan_sensor_keys(self):
        self.set_sensors_cmd()
        result = self.get_parsed_result()
        for key in self.get_fan_sensors():
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in fan{} sensor data".format(key, self._fan_id),
                )


class Fan1SensorTest(FanSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util fan1"]
        self._fan_id = 1

    def get_fan_sensors(self):
        return FAN1_SENSORS

    def test_fan1_sensor_keys(self):
        super().base_test_fan_sensor_keys()


class Fan2SensorTest(FanSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util fan2"]
        self._fan_id = 2

    def get_fan_sensors(self):
        return FAN2_SENSORS

    def test_fan2_sensor_keys(self):
        super().base_test_fan_sensor_keys()


class Fan3SensorTest(FanSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util fan3"]
        self._fan_id = 3

    def get_fan_sensors(self):
        return FAN3_SENSORS

    def test_fan3_sensor_keys(self):
        super().base_test_fan_sensor_keys()


class Fan4SensorTest(FanSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        self.sensors_cmd = ["/usr/local/bin/sensor-util fan4"]
        self._fan_id = 4

    def get_fan_sensors(self):
        return FAN4_SENSORS

    def test_fan4_sensor_keys(self):
        super().base_test_fan_sensor_keys()
