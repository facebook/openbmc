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
import unittest

from common.base_interface_test import CommonInterfaceTest
from utils.cit_logger import Logger


"""
Tests eth0 v4 interface
"""


class InterfaceTest(CommonInterfaceTest, unittest.TestCase):
    def test_eth0_v4_interface(self):
        """
        Tests eth0 v4 interface
        """
        self.set_ifname("eth0")
        Logger.log_testname(self._testMethodName)
        self.assertEqual(self.ping_v4(), 0, "Ping test for eth0 v4 failed")
