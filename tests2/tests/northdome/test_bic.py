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

from common.base_bic_test import CommonBicTest
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class BicSlot1Test(CommonBicTest, unittest.TestCase):
    def set_bic_cmd(self):
        self.bic_cmd = "/usr/bin/bic-util slot1"

    @unittest.skip("Test not supported on platform")
    def test_bic_mac(self):
        pass

    @unittest.skip("Test not supported on platform")
    def test_bic_sensor(self):
        pass


class BicSlot2Test(BicSlot1Test):
    def set_bic_cmd(self):
        self.bic_cmd = "/usr/bin/bic-util slot2"


class BicSlot3Test(BicSlot1Test):
    def set_bic_cmd(self):
        self.bic_cmd = "/usr/bin/bic-util slot3"


class BicSlot4Test(BicSlot1Test):
    def set_bic_cmd(self):
        self.bic_cmd = "/usr/bin/bic-util slot4"
