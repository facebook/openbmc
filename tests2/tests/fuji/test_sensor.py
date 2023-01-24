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
import os
import unittest
from abc import abstractmethod

from common.base_sensor_test import SensorUtilTest
from tests.fuji.helper.libpal import pal_is_fru_prsnt, pal_get_fru_id
from tests.fuji.test_data.sensors.sensor import (
    PIM1_SENSORS_16Q,
    PIM1_SENSORS_16O,
    PIM2_SENSORS_16Q,
    PIM2_SENSORS_16O,
    PIM3_SENSORS_16Q,
    PIM3_SENSORS_16O,
    PIM4_SENSORS_16Q,
    PIM4_SENSORS_16O,
    PIM5_SENSORS_16Q,
    PIM5_SENSORS_16O,
    PIM6_SENSORS_16Q,
    PIM6_SENSORS_16O,
    PIM7_SENSORS_16Q,
    PIM7_SENSORS_16O,
    PIM8_SENSORS_16Q,
    PIM8_SENSORS_16O,
    PSU1_SENSORS,
    PSU2_SENSORS,
    PSU3_SENSORS,
    PSU4_SENSORS,
    SCM_SENSORS,
    SMB_SENSORS,
)
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
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
        SCM_TEMP_KEYS = [
            "SCM_OUTLET_U7_TEMP",
            "SCM_INLET_U8_TEMP",
            "BMC_LM75_U9_TEMP",
            "MB_OUTLET_TEMP",
            "MB_INLET_TEMP",
            "PCH_TEMP",
            "VCCIN_VR_TEMP",
            "1V05MIX_VR_TEMP",
            "SOC_TEMP",
            "VDDR_VR_TEMP",
            "SOC_DIMMA0_TEMP",
            "SOC_DIMMB0_TEMP",
        ]
        for key in SCM_TEMP_KEYS:
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


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class PimSensorTest(SensorUtilTest, unittest.TestCase):
    @abstractmethod
    def get_pim_sensors(self):
        self._pim_id = 0
        pass

    def get_pim_temp_keys(self):
        PIM_TEMP_KEYS = []
        PIM_TEMP_KEYS.append("PIM{}_LM75_U37_TEMP_BASE".format(self._pim_id))
        PIM_TEMP_KEYS.append("PIM{}_LM75_U26_TEMP".format(self._pim_id))
        PIM_TEMP_KEYS.append("PIM{}_LM75_U37_TEMP_MEZZ".format(self._pim_id))
        # PIM_TEMP_KEYS.append("PIM{}_QSFP_TEMP".format(self._pim_id))
        return PIM_TEMP_KEYS

    def base_test_pim_sensor_keys(self):
        self.set_sensors_cmd()
        if not pal_is_fru_prsnt(pal_get_fru_id("pim{}".format(self._pim_id))):
            self.skipTest("pim{} is not present".format(self._pim_id))
        result = self.get_parsed_result()
        skip_XP3R3V_EARLY = self.check_pim_powerseq_UCD90124A()
        for key in self.get_pim_sensors():
            # PowerSequence UCD90124A does not have XP3R3V_EARLY sensor, so skipping it
            if key.find("XP3R3V_EARLY") and skip_XP3R3V_EARLY:
                continue
            with self.subTest(key=key):
                self.assertIn(
                    key,
                    result.keys(),
                    "Missing key {} in pim{} sensor data".format(key, self._pim_id),
                )

    def base_test_pim_temp_sensor_range(self):
        self.set_sensors_cmd()
        if not pal_is_fru_prsnt(pal_get_fru_id("pim{}".format(self._pim_id))):
            self.skipTest("pim{} is not present".format(self._pim_id))
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

    def get_pim_name(self, ver):
        """ """
        pim_name = None
        ver = int(ver, 16)
        if ver & 0x80 == 0x0:
            pim_name = "PIM_TYPE_16Q"
        elif ver & 0x80 == 0x80:
            pim_name = "PIM_TYPE_16O"
        else:
            pim_name = None
        return pim_name

    def get_pim_sensor_type(self, pim_num):
        """
        Get PIM sensors type by read i2c device board version
        """
        pim_sensor_type = None
        bus = (pim_num * 8) + 80
        PATH = "/sys/bus/i2c/devices/%d-0060/board_ver" % (bus)

        if not os.path.exists(PATH):
            raise Exception("Path for PIM board_ver doesn't exist")
        with open(PATH, "r") as fp:
            line = fp.readline()
            if line:
                ver = line.split("0x")[1]
                pim_sensor_type = self.get_pim_name(ver)
            else:
                raise Exception("PIM board_ver is empty")
        return pim_sensor_type

    def check_pim_powerseq_UCD90124A(self):
        """
        Check for PowerSequence UCD90124A CH7 0x40
        """
        PATH = "/tmp/cache_store/pim%d_pwrseq_addr" % (self._pim_id)
        if not os.path.exists(PATH):
            raise Exception("Path for PIM pwrseq_addr doesn't exist")
        with open(PATH, "r") as fp:
            line = fp.readline()
            if line:
                if line == "0x40":
                    return True
                else:
                    return False
            else:
                raise Exception("PIM pwrseq_addr is empty")


class Pim1SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("pim1")):
            self.skipTest("pim1 is not present")
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim1"]
        self._pim_id = 1

    def get_pim_sensors(self):
        name = self.get_pim_sensor_type(0)
        if name == "PIM_TYPE_16Q":
            return PIM1_SENSORS_16Q
        elif name == "PIM_TYPE_16O":
            return PIM1_SENSORS_16O
        else:
            return PIM1_SENSORS_16Q

    def test_pim1_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim1_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim2SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("pim2")):
            self.skipTest("pim2 is not present")
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim2"]
        self._pim_id = 2

    def get_pim_sensors(self):
        name = self.get_pim_sensor_type(1)
        if name == "PIM_TYPE_16Q":
            return PIM2_SENSORS_16Q
        elif name == "PIM_TYPE_16O":
            return PIM2_SENSORS_16O
        else:
            return PIM2_SENSORS_16Q

    def test_pim2_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim2_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim3SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("pim3")):
            self.skipTest("pim3 is not present")
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim3"]
        self._pim_id = 3

    def get_pim_sensors(self):
        name = self.get_pim_sensor_type(2)
        if name == "PIM_TYPE_16Q":
            return PIM3_SENSORS_16Q
        elif name == "PIM_TYPE_16O":
            return PIM3_SENSORS_16O
        else:
            return PIM3_SENSORS_16Q

    def test_pim3_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim3_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim4SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("pim4")):
            self.skipTest("pim4 is not present")
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim4"]
        self._pim_id = 4

    def get_pim_sensors(self):
        name = self.get_pim_sensor_type(3)
        if name == "PIM_TYPE_16Q":
            return PIM4_SENSORS_16Q
        elif name == "PIM_TYPE_16O":
            return PIM4_SENSORS_16O
        else:
            return PIM4_SENSORS_16Q

    def test_pim4_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim4_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim5SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("pim5")):
            self.skipTest("pim5 is not present")
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim5"]
        self._pim_id = 5

    def get_pim_sensors(self):
        name = self.get_pim_sensor_type(4)
        if name == "PIM_TYPE_16Q":
            return PIM5_SENSORS_16Q
        elif name == "PIM_TYPE_16O":
            return PIM5_SENSORS_16O
        else:
            return PIM5_SENSORS_16Q

    def test_pim5_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim5_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim6SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("pim6")):
            self.skipTest("pim6 is not present")
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim6"]
        self._pim_id = 6

    def get_pim_sensors(self):
        name = self.get_pim_sensor_type(5)
        if name == "PIM_TYPE_16Q":
            return PIM6_SENSORS_16Q
        elif name == "PIM_TYPE_16O":
            return PIM6_SENSORS_16O
        else:
            return PIM6_SENSORS_16Q

    def test_pim6_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim6_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim7SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("pim7")):
            self.skipTest("pim7 is not present")
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim7"]
        self._pim_id = 7

    def get_pim_sensors(self):
        name = self.get_pim_sensor_type(6)
        if name == "PIM_TYPE_16Q":
            return PIM7_SENSORS_16Q
        elif name == "PIM_TYPE_16O":
            return PIM7_SENSORS_16O
        else:
            return PIM7_SENSORS_16Q

    def test_pim7_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim7_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


class Pim8SensorTest(PimSensorTest, unittest.TestCase):
    def set_sensors_cmd(self):
        if not pal_is_fru_prsnt(pal_get_fru_id("pim8")):
            self.skipTest("pim8 is not present")
        self.sensors_cmd = ["/usr/local/bin/sensor-util pim8"]
        self._pim_id = 8

    def get_pim_sensors(self):
        name = self.get_pim_sensor_type(7)
        if name == "PIM_TYPE_16Q":
            return PIM8_SENSORS_16Q
        elif name == "PIM_TYPE_16O":
            return PIM8_SENSORS_16O
        else:
            return PIM8_SENSORS_16Q

    def test_pim8_sensor_keys(self):
        super().base_test_pim_sensor_keys()

    def test_pim8_temp_sensor_range(self):
        super().base_test_pim_temp_sensor_range()


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class PsuSensorTest(SensorUtilTest, unittest.TestCase):
    @abstractmethod
    def get_psu_sensors(self):
        self._psu_id = 0
        pass

    def base_test_psu_sensor_keys(self):
        self.set_sensors_cmd()
        if not pal_is_fru_prsnt(pal_get_fru_id("psu{}".format(self._psu_id))):
            self.skipTest("psu{} is not present".format(self._psu_id))
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
