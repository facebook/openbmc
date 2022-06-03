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
import unittest

from common.base_usb_host_test import BaseUSBHostTest
from tests.wedge400.helper.libpal import (
    pal_get_board_type_rev,
    BoardRevision,
)
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class USBHostDeviceTest(BaseUSBHostTest, unittest.TestCase):
    def set_usb_devices(self):
        platform_type_rev = pal_get_board_type_rev()
        if (
            platform_type_rev == "Wedge400-MP-Respin"
            or platform_type_rev == "Wedge400C-MP-Respin"
        ):
            self.usb_devices = [
                "1d6b:0001",  # USB 1.1 Root Hub
                "1d6b:0002",  # USB 2.0 Root Hub
                "0403:6011",  # FT4232 Serial (UART) IC
            ]
        else:
            self.usb_devices = [
                "1d6b:0001",  # USB 1.1 Root Hub
                "1d6b:0002",  # USB 2.0 Root Hub
                "0403:6001",  # FT232 Serial (UART) IC
            ]
