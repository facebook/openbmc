#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
from utils.sysfs_utils import SysfsUtils


class BaseSysfsTest(object):
    def setUp(self):
        self.path = None
        self.files_to_test = None
        Logger.start(name=self._testMethodName)

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def setPath(self):
        pass

    @abstractmethod
    def setFilesToTest(self):
        pass

    def test_valid_files(self):
        self.setPath()
        self.setFilesToTest()
        self.assertTrue(os.path.exists(self.path))
        for filename in self.files_to_test:
            filepath = os.path.join(self.path, filename)
            self.assertIsNotNone(SysfsUtils.read_int(filepath))
