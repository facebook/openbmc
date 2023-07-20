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
import unittest

from common.base_interface_test import CommonInterfaceTest
from utils.cit_logger import Logger
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class InterfaceTest(CommonInterfaceTest, unittest.TestCase):
    def test_usb0_v6_interface(self):
        """
        Tests eth0 v6 interface
        """
        self.set_ifname("usb0")
        Logger.log_testname(self._testMethodName)
        self.assertEqual(self.ping_v6(), 0, "Ping test for usb0 v6 failed")

    def test_usb0_v6_interface_link_local(self):
        """
        Tests eth0 v6 interface
        """
        self.set_ifname("usb0")
        Logger.log_testname(self._testMethodName)
        self.assertEqual(
            self.ping_v6_link_local(), 0, "Ping test for usb0 v6 over link local failed"
        )

    def test_eth0_4088_v6_interface(self):
        """
        Tests eth0 v6 interface
        """
        self.set_ifname("eth0.4088")
        Logger.log_testname(self._testMethodName)
        self.assertEqual(self.ping_v6(), 0, "Ping test for eth0.4088 v6 failed")

    def test_eth0_4088_v6_interface_link_local(self):
        """
        Tests eth0 v6 interface
        """
        self.set_ifname("eth0.4088")
        Logger.log_testname(self._testMethodName)
        self.assertEqual(
            self.ping_v6_link_local(),
            0,
            "Ping test for eth0.4088 v6 over link local failed",
        )
