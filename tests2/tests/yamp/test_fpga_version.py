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

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class FpgaVersionTest(unittest.TestCase):
    def setUp(self):
        self.fpga_version = "/usr/local/bin/fpga_ver.sh"
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_sup_fpga_revision_format(self):
        data = run_shell_cmd(self.fpga_version).split("\n")
        self.assertIn("SUP_FPGA", data[1], "SUP_FPGA missing received={}".format(data))

        value = data[1].split(":")[1]
        self.assertRegex(
            value,
            r"\b[0-9]+\.[0-9]+\b",
            "SUP_FPGA version {} isn't numeric".format(value),
        )

    def test_sup_fpga_sysfs_path_exists(self):
        self.sup_version_paths = {
            "major": "/sys/bus/i2c/devices/12-0043/cpld_ver_major",
            "minor": "/sys/bus/i2c/devices/12-0043/cpld_ver_minor",
        }
        for key, path in self.sup_version_paths.items():
            with self.subTest(path=path):
                self.assertTrue(
                    os.path.exists(path),
                    "SUP Version i2c sysfs path doesnt exist. Key = {}".format(key),
                )

    def test_scd_fpga_revision_format(self):
        data = run_shell_cmd(self.fpga_version).split("\n")
        self.assertIn("SCD_FPGA", data[3], "SCD_FPGA missing received={}".format(data))

        value = data[3].split(":")[1]
        self.assertRegex(
            value,
            r"\b[0-9]+\.[0-9]+\b",
            "SCD_FPGA version {} isn't numeric".format(value),
        )

    def test_scd_fpga_sysfs_path_exists(self):
        self.scd_version_paths = {
            "major": "/sys/bus/i2c/devices/4-0023/cpld_ver_major",
            "minor": "/sys/bus/i2c/devices/4-0023/cpld_ver_minor",
        }
        for key, path in self.scd_version_paths.items():
            with self.subTest(path=path):
                self.assertTrue(
                    os.path.exists(path),
                    "SCD FPGA Version i2c sysfs path doesnt exist. Key = {}".format(
                        key
                    ),
                )

    def test_pims_fpga_revision_format(self):
        data = run_shell_cmd(self.fpga_version).split("\n")

        for pim_count in range(1, 8):
            i = pim_count + 4  # data index
            with self.subTest(pim=pim_count):
                self.assertIn(
                    "PIM {}".format(pim_count),
                    data[i],
                    "PIM{} FPGA missing received={}".format(pim_count, data),
                )

                self.assertRegex(
                    data[i],
                    r"\b[0-9]+\.[0-9]+\b",
                    "PIM{} version {} isn't numeric".format(pim_count, data[i]),
                )

    def test_pims_fpga_sysfs_path_exists(self):
        self.pim_version_paths = {
            "major": "/sys/bus/i2c/devices/4-0023/lc{}_fpga_rev_major",
            "minor": "/sys/bus/i2c/devices/4-0023/lc{}_fpga_rev_minor",
        }
        for pim_count in range(1, 8):
            for key, path in self.pim_version_paths.items():
                path = path.format(pim_count)
                with self.subTest(path=path):
                    self.assertTrue(
                        os.path.exists(path),
                        "PIM{} FPGA Version i2c sysfs path doesnt exist.\
                        Key = {}".format(
                            pim_count, key
                        ),
                    )
