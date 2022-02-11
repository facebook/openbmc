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
from abc import abstractmethod

from utils.cit_logger import Logger


#
# "BaseI2cDriverPresenceTest" aims at testing if corresponding i2c drivers
# are loaded properly. These drivers could be built-in or loadable modules,
# but there must be an entry in "/sys/bus/i2c/drivers" for each driver.
#
class BaseI2cDriverPresenceTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.i2c_dirver_sysfs_dir = "/sys/bus/i2c/drivers"
        self.i2c_driver_list = None

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    #
    # Each platform needs to initialize "self.i2c_driver_list" in below
    # method.
    #
    @abstractmethod
    def set_i2c_driver_list(self):
        pass

    def check_i2c_driver_presence(self, driver_name):
        pathname = os.path.join(self.i2c_dirver_sysfs_dir, driver_name)
        self.assertTrue(
            os.path.isdir(pathname), "i2c driver %s is missing" % driver_name
        )

    def test_i2c_driver_presence(self):
        self.set_i2c_driver_list()
        self.assertNotEqual(
            self.i2c_driver_list, None, "Golden spi driver list not specified"
        )

        for i2c_driver in self.i2c_driver_list:
            with self.subTest(i2c_driver=i2c_driver):
                self.check_i2c_driver_presence(i2c_driver)
