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
import unittest
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_cmd


class BaseFruTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.fru_cmd = None
        self.fru_info = None
        self.fru_fields = None
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def set_fru_cmd(self):
        pass

    @abstractmethod
    def set_fru_fields(self):
        pass

    def run_fru_cmd(self):
        self.assertNotEqual(self.fru_cmd, None, "FRU command not set")

        Logger.info("Running FRU command: " + str(self.fru_cmd))
        self.fru_info = run_cmd(cmd=self.fru_cmd)
        Logger.info(self.fru_info)
        return self.fru_info


class CommonFruTest(BaseFruTest):
    def log_check(self, name):
        Logger.log_testname(name)
        self.set_fru_cmd()
        self.set_fru_fields()
        if not self.fru_info:
            self.fru_info = self.run_fru_cmd()

    def test_fru_fields(self):
        self.log_check(name=self._testMethodName)
        self.set_fru_fields()
        for fru_field in self.fru_fields:
            self.assertIn(fru_field, self.fru_info)
