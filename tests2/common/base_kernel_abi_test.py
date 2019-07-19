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

#
# This file aims at detecting and reporting kernel ABI changes which
# may be caused by kernel upgrade, defconfig update, and etc.
#

import os

from utils.cit_logger import Logger


class BaseKernelABITest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        # sysfs dependency list
        self.sysfs_dep_list = [
            "/sys/devices",
            "/sys/class/hwmon",
            "/sys/class/gpio",
            "/sys/class/gpio/export",
            "/sys/bus/i2c/devices",
            "/sys/bus/i2c/drivers",
        ]
        # devfs dependency list
        self.devfs_dep_list = [
            "/dev/mem",
            "/dev/watchdog",
            "/dev/mtd0",
            "/dev/mtdblock0",
            "/dev/ttyS0",
        ]
        # procfs dependency list
        self.procfs_dep_list = ["/proc/mtd"]
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def set_platform_specific_paths(self):
        self.platform_dep_list = []
        pass

    def test_kernel_path_exists(self):
        self.set_platform_specific_paths()
        all_files = (
            self.sysfs_dep_list
            + self.devfs_dep_list
            + self.procfs_dep_list
            + self.platform_dep_list
        )
        for pathname in all_files:
            with self.subTest(pathname=pathname):
                self.assertTrue(
                    os.path.exists(pathname), "Filepath {} not present".format(pathname)
                )
