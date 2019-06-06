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
import os
import re
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class PsumuxmonTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_psumuxmon_runit_sv_status(self):
        cmd = ["/usr/bin/sv status psumuxmon"]
        data = run_shell_cmd(cmd)
        self.assertIn("run", data, "psumuxmon process not running")

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

    def test_psumuxmon_ltc_sensor_path_exists(self):
        # Based on lab device deployment, sensor data might not be accessible.
        # Verify that path exists
        cmd = "/sys/bus/i2c/devices/7-006f/hwmon/hwmon*/in1_input"
        self.assertTrue(
            os.path.exists(self.get_ltc_hwmon_path(cmd)),
            "psumuxmon LTC sensor path accessible",
        )
