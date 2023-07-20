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
import re
import unittest

from tests.wedge100.power_supply import get_power_type
from tests.wedge100.test_data.sysfs_nodes.sysfs_nodes import LTC4281_SYSFS_NODES
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check, running_systemd


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class PsumuxmonTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def get_ltc_hwmon_path(self, path):
        result = re.split("hwmon", path)
        if os.path.isdir(result[0]):
            construct_hwmon_path = result[0] + "hwmon"
            x = None
            for x in os.listdir(construct_hwmon_path):
                if x.startswith("hwmon"):
                    construct_hwmon_path = (
                        construct_hwmon_path + "/" + x + "/" + result[2].split("/")[1]
                    )
                    return construct_hwmon_path
        return None

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

    def check_ltc4281_sysfs_nodes(self, path_dir):
        """
        check all the sysfs nodes for ltc4281 and
        make sure that the following conditions are met:
        1- the path exits
        2- the sysfs node is reable
        It returns a list that contains
        [a boolean value, pathname, failure mode, data read]
        """
        if os.path.isdir(path_dir):
            for filename in LTC4281_SYSFS_NODES:
                pathname = os.path.join(path_dir, filename)
                if not os.path.isfile(pathname):
                    return [False, pathname, "access", None]
                node_data = self.sysfs_read(pathname)
                if not isinstance(node_data, int):
                    return [False, pathname, "read", node_data]
        else:
            raise Exception("{} directory is not found".format(path_dir))
        return [True, None, None, None]

    def is_ltc4151_i2c_dev_addr_present(self):
        return get_power_type() == "pem1"

    def is_ltc4281_i2c_dev_addr_present(self):
        return get_power_type() == "pem2"

    def test_psumuxmon_runit_sv_status(self):
        power_type = get_power_type()
        if power_type.startswith("pem"):
            if running_systemd():
                cmd = ["/bin/systemctl status psumuxmon"]
                data = run_shell_cmd(cmd, ignore_err=True)
                self.assertIn("running", data, "psumuxmon unit not running")
            else:
                cmd = ["/usr/bin/sv status psumuxmon"]
                data = run_shell_cmd(cmd)
                self.assertIn("run", data, "psumuxmon process not running")
        else:
            self.skipTest("pem not detected. skipping this test...")

    def test_psumuxmon_ltc4151_sensor_path_exists(self):
        # Based on lab device deployment, sensor data might not be accessible.
        # Verify that path exists. Sensors ltc4251 is on both old PEM and new PEM
        if (
            self.is_ltc4151_i2c_dev_addr_present()
            or self.is_ltc4281_i2c_dev_addr_present()
        ):
            if os.path.exists("/sys/bus/i2c/devices/14-006f/"):
                path = "/sys/bus/i2c/devices/14-006f/hwmon/hwmon*/in1_input"
            else:
                path = "/sys/bus/i2c/devices/15-006f/hwmon/hwmon*/in1_input"
            self.assertTrue(
                os.path.exists(self.get_ltc_hwmon_path(path)),
                "psumuxmon LTC sensor path accessible",
            )
        else:
            self.skipTest("ltc4151 eeprom i2c addr not detected. skipping this test...")

    def test_psumuxmon_ltc4281_sensor_sysfs_nodes_exists(self):
        # check to see if the sysfs path for ltc4281 has
        # been created
        if self.is_ltc4281_i2c_dev_addr_present():
            if os.path.exists("/sys/bus/i2c/devices/14-004a"):
                path = "/sys/bus/i2c/devices/14-004a"
            else:
                path = "/sys/bus/i2c/devices/15-004a"
            sysfs_nodes_results = self.check_ltc4281_sysfs_nodes(path)
            self.assertTrue(
                sysfs_nodes_results[0],
                "{} path failed {} with value {}".format(
                    sysfs_nodes_results[1],
                    sysfs_nodes_results[2],
                    sysfs_nodes_results[3],
                ),
            )
        else:
            self.skipTest("ltc4281 eeprom i2c addr not detected. skipping this test...")
