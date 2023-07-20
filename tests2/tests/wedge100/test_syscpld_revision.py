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
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class SysCpldRevisionTest(unittest.TestCase):
    def setUp(self):
        self.cpld_rev = "/usr/local/bin/cpld_rev.sh"
        self.cpld_paths = [
            "/sys/bus/i2c/devices/12-0031/cpld_rev",
            "/sys/bus/i2c/devices/12-0031/cpld_sub_rev",
            "/sys/bus/i2c/devices/8-0033/cpld_rev",
            "/sys/bus/i2c/devices/8-0033/cpld_sub_rev",
        ]
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_cpld_revision_format(self):
        """
        cpld_rev returns X.X
        """
        version = run_shell_cmd(self.cpld_rev).rstrip("\n").split(".")
        self.assertTrue(
            version[0].isdigit(),
            "CPLD major version is not digit, received={}".format(version),
        )
        self.assertTrue(
            version[1].isdigit(),
            "CPLD minor version is not digit, received={}".format(version),
        )

    def test_cpld_fan_revision_format(self):
        """
        cpld_rev --fan returns X.X
        """
        version = run_shell_cmd([self.cpld_rev, "--fan"]).rstrip("\n").split(".")
        self.assertTrue(
            version[0].isdigit(),
            "Fan major version is not digit, received={}".format(version),
        )
        self.assertTrue(
            version[1].isdigit(),
            "Fan minor version is not digit, received={}".format(version),
        )

    def test_cpld_version_sysfs_path_exists(self):
        for path in self.cpld_paths:
            with self.subTest(path=path):
                self.assertTrue(
                    os.path.exists(path), "CPLD version i2c sysfs path doesnt exist"
                )

    def test_cpld_version_sysfs_path_access(self):
        for path in self.cpld_paths:
            with self.subTest(path=path):
                cmd = "head -n 1 " + path
                data = run_shell_cmd(cmd).rstrip("\n")
                self.assertTrue(int(data, 16), "CPLD major version is not digit")
