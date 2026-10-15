#!/usr/bin/env python3
#
# Copyright 2026-present Facebook. All Rights Reserved.
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

from common.base_interface_test import CommonInterfaceTest
from utils.test_utils import qemu_check


class InterfaceTest(CommonInterfaceTest, unittest.TestCase):
    EXPECTED_INTERFACES = [
        "eth0",
        "usb0",
        "eth0.4088",
    ]

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_expected_interfaces_present(self):
        for interface in self.EXPECTED_INTERFACES:
            with self.subTest(interface=interface):
                self.assertTrue(
                    os.path.isdir("/sys/class/net/{}".format(interface)),
                    "Network interface {} is missing".format(interface),
                )
