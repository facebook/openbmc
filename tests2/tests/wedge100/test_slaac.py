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
import ipaddress
import unittest

from common.base_slaac_test import BaseSlaacTest
from utils.cit_logger import Logger
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class InterfaceTest(BaseSlaacTest, unittest.TestCase):
    def test_eth0_v6_slaac_interface(self):
        """
        Tests eth0 SLAAC IP
        """
        self.set_ifname("eth0")
        Logger.log_testname(name=self._testMethodName)
        addr = ipaddress.ip_address(self.get_ipv6_address())
        haystack = addr.exploded.replace(":", "")
        self.assertIn(
            self.generate_modified_eui_64_mac_address(),
            haystack,
            "SLAAC IP doesn't found",
        )
