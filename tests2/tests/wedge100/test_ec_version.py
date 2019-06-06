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
import unittest

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class EcVersionTest(unittest.TestCase):
    def setUp(self):
        self.ec_version = "/usr/local/bin/ec_version.sh"
        self.ec_version_paths = {
            "version": "/sys/bus/i2c/devices/i2c-4/4-0033/version",
            "date": "/sys/bus/i2c/devices/i2c-4/4-0033/date",
        }
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def test_ec_revision_format(self):
        """
        ec version returns version and date
        """
        data = run_shell_cmd(self.ec_version).split("\n")
        self.assertIn(
            "EC Released Version",
            data[0],
            "EC released version missing received={}".format(data),
        )
        self.assertIn(
            "EC Released Date",
            data[1],
            "EC released date missing received={}".format(data),
        )

    def test_ec_version_sysfs_path_exists(self):
        for key, path in self.ec_version_paths.items():
            with self.subTest(path=path):
                self.assertTrue(
                    os.path.exists(path),
                    "EC Version i2c sysfs path doesnt exist. Key = {}".format(key),
                )
