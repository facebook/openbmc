#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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
import re
import unittest

from common.base_interface_test import CommonInterfaceTest
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check

"""
Tests eth0 & usb0 v6 interface
"""


class InterfaceTest(CommonInterfaceTest, unittest.TestCase):
    def get_eeprom_value(self, field: str) -> str:
        """Get a field value from the EEPROM."""
        eeprom_output = run_shell_cmd(cmd="weutil")
        field_match = re.search(rf"{field}:\s*(\S+)", eeprom_output)
        self.assertTrue(
            field_match,
            f"Could not read {field} from EEPROM",
        )
        return field_match.group(1)

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_usb0_v6_interface(self):
        """
        Tests usb0 v6 interface
        """
        self.set_ifname("usb0")
        Logger.log_testname(self._testMethodName)
        self.assertEqual(self.ping_v6(), 0, "Ping test for usb0 v6 failed")

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

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_eth0_consistent_mac_address(self):
        """
        Tests eth0 MAC address matches the BMC MAC stored in the chassis EEPROM
        """
        self.set_ifname("eth0")
        Logger.log_testname(self._testMethodName)
        eth0_mac = self.get_mac_address().lower()
        eeprom_mac = self.get_eeprom_value("BMC MAC Base").lower()
        self.assertEqual(
            eth0_mac,
            eeprom_mac,
            f"eth0 MAC {eth0_mac} does not match EEPROM BMC MAC Base {eeprom_mac}",
        )
