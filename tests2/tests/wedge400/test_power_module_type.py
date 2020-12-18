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

from tests.wedge400.helper.libpal import pal_detect_power_supply_present, BoardRevision
from tests.wedge400.test_data.sysfs_nodes.sysfs_nodes import LTC4282_SYSFS_NODES
from tests.wedge400.test_data.sysfs_nodes.sysfs_nodes import MAX6615_SYSFS_NODES
from utils.cit_logger import Logger


class PowerModuleDetectionTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def sysfs_read(self, inp):
        """
        Read value from sysfs path,
        if value is valid return integer value, else return none.
        """
        try:
            with open(inp, "r") as f:
                str_val = f.readline().rstrip("\n")
                if str_val.find("0x") is -1:
                    val = int(str_val, 10)
                else:
                    val = int(str_val, 16)
                return val
        except Exception:
            return None

    def check_ltc4282_sysfs_nodes(self, path_dir):
        """
        check all the sysfs nodes for ltc4282 and
        make sure that the following conditions are met:
        1- the path exits
        2- the sysfs node is reable
        It returns a list that contains
        [a boolean value, pathname, failure mode, data read]
        """
        if os.path.isdir(path_dir):
            for filename in LTC4282_SYSFS_NODES:
                pathname = os.path.join(path_dir, filename)
                if not os.path.isfile(pathname):
                    return [False, pathname, "access", None]
                node_data = self.sysfs_read(pathname)
                if not isinstance(node_data, int):
                    return [False, pathname, "read", node_data]
        else:
            raise Exception("{} directory is not found".format(path_dir))
        return [True, None, None, None]

    def check_max6615_sysfs_nodes(self, path_dir):
        """
        check all the sysfs nodes for max6615 and
        make sure that the following conditions are met:
        1- the path exits
        2- the sysfs node is reable
        It returns a list that contains
        [a boolean value, pathname, failure mode, data read]
        """
        if os.path.isdir(path_dir):
            for filename in MAX6615_SYSFS_NODES:
                pathname = os.path.join(path_dir, filename)
                if not os.path.isfile(pathname):
                    return [False, pathname, "access", None]
                node_data = self.sysfs_read(pathname)
                if not isinstance(node_data, int):
                    return [False, pathname, "read", node_data]
        else:
            raise Exception("{} directory is not found".format(path_dir))
        return [True, None, None, None]

    def test_power_module_ltc4282_sensor_sysfs_nodes_exists(self):
        # via fru id in pal_detect_power_supply_present() to
        # check weather the pems are present
        # if not ,do not need to run this test

        if pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PEM1) == "pem1":
            cmd = "/sys/bus/i2c/devices/24-0058/hwmon/hwmon21/"
            sysfs_nodes_results = self.check_ltc4282_sysfs_nodes(cmd)
            self.assertTrue(
                sysfs_nodes_results[0],
                "{} path failed {} with value {}".format(
                    sysfs_nodes_results[1],
                    sysfs_nodes_results[2],
                    sysfs_nodes_results[3],
                ),
            )

        if pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PEM2) == "pem2":
            cmd = "/sys/bus/i2c/devices/25-0058/hwmon/hwmon21/"
            sysfs_nodes_results = self.check_ltc4282_sysfs_nodes(cmd)
            self.assertTrue(
                sysfs_nodes_results[0],
                "{} path failed {} with value {}".format(
                    sysfs_nodes_results[1],
                    sysfs_nodes_results[2],
                    sysfs_nodes_results[3],
                ),
            )

        if (
            pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PSU1) == "psu1"
            or pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PSU2)
            == "psu2"
        ):
            self.skipTest("ltc4282 eeprom i2c addr not detected. skipping this test...")

    def test_fanctlr_max6615_fan_sysfs_nodes_exists(self):
        # via fru id in pal_detect_power_supply_present() to
        # check weather the pems are present
        # if not ,do not need to run this test

        if pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PEM1) == "pem1":
            path = "/sys/bus/i2c/devices/24-0018/hwmon/hwmon22/"
            sysfs_nodes_results = self.check_max6615_sysfs_nodes(path)
            self.assertTrue(
                sysfs_nodes_results[0],
                "{} path failed {} with value {}".format(
                    sysfs_nodes_results[1],
                    sysfs_nodes_results[2],
                    sysfs_nodes_results[3],
                ),
            )

        if pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PEM2) == "pem2":
            path = "/sys/bus/i2c/devices/25-0018/hwmon/hwmon22/"
            sysfs_nodes_results = self.check_max6615_sysfs_nodes(path)
            self.assertTrue(
                sysfs_nodes_results[0],
                "{} path failed {} with value {}".format(
                    sysfs_nodes_results[1],
                    sysfs_nodes_results[2],
                    sysfs_nodes_results[3],
                ),
            )

        if (
            pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PSU1) == "psu1"
            or pal_detect_power_supply_present(BoardRevision.POWER_MODULE_PSU2)
            == "psu2"
        ):
            self.skipTest("max6615 eeprom i2c addr not detected. skipping this test...")
