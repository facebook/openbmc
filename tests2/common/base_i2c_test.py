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

import fnmatch
import os
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.i2c_utils import I2cSysfsUtils


class BaseI2cTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.i2c_tree = None

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    @abstractmethod
    def load_golden_i2c_tree(self):
        pass

    def check_i2c_device(self, i2c_path, i2c_info):

        Logger.debug("checking i2c device %s" % i2c_path)

        dev_dir = I2cSysfsUtils.i2c_device_abspath(i2c_path)
        self.assertTrue(
            os.path.isdir(dev_dir),
            "%s (name=%s, driver=%s) is missing"
            % (i2c_path, i2c_info["name"], i2c_info["driver"]),
        )

        err_str = ""

        # Verify device name/type is correct.
        name = I2cSysfsUtils.i2c_device_get_name(i2c_path)
        if not fnmatch.fnmatch(name, i2c_info["name"]):
            err_str += "name mismatch: expect %s, actual %s; " % (
                i2c_info["name"],
                name,
            )

        # Make sure correct driver is binded to the device.
        driver = I2cSysfsUtils.i2c_device_get_driver(i2c_path)
        if driver != i2c_info["driver"]:
            err_str += "driver mismatch: expect %s, actual %s;" % (
                i2c_info["driver"],
                driver,
            )

        self.assertEqual(err_str, "", "%s: %s" % (i2c_path, err_str))

    def test_i2c_tree(self):
        """check if i2c devices are created and drivers are binded properly."""
        self.load_golden_i2c_tree()
        self.assertIsNot(self.i2c_tree, None, "golden I2C tree not set")

        for i2c_path, i2c_info in self.i2c_tree.items():
            with self.subTest(i2c_path=i2c_path):
                self.check_i2c_device(i2c_path, i2c_info)
