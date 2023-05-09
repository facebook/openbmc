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

from common.base_fru_test import CommonFruTest
from utils.test_utils import check_fru_availability, qemu_check


class FruSlot1Test(CommonFruTest, unittest.TestCase):
    def setUp(self):
        self.fru = "slot1"
        self.fru_cmd = ["/usr/local/bin/fruid-util", "slot1"]
        self.fru_fields = {"chassis": 1, "product": 2, "board": 2}

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_fru_fields(self):
        if not check_fru_availability(self.fru):
            self.skipTest("skip test due to {} not available".format(self.fru))
        super().test_fru_fields()


class FruSlot2Test(FruSlot1Test, unittest.TestCase):
    def setUp(self):
        self.fru = "slot2"
        self.fru_cmd = ["/usr/local/bin/fruid-util", "slot2"]
        self.fru_fields = {"chassis": 1, "product": 2, "board": 2}


class FruSlot3Test(FruSlot1Test, unittest.TestCase):
    def setUp(self):
        self.fru = "slot3"
        self.fru_cmd = ["/usr/local/bin/fruid-util", "slot3"]
        self.fru_fields = {"chassis": 1, "product": 2, "board": 2}


class FruSlot4Test(FruSlot1Test, unittest.TestCase):
    def setUp(self):
        self.fru = "slot4"
        self.fru_cmd = ["/usr/local/bin/fruid-util", "slot4"]
        self.fru_fields = {"chassis": 1, "product": 2, "board": 2}


class FruBmcTest(FruSlot1Test, unittest.TestCase):
    def setUp(self):
        self.fru = "bmc"
        self.fru_cmd = ["/usr/local/bin/fruid-util", "bmc"]
        self.fru_fields = {"product": 2, "board": 2}


class FruBbTest(FruSlot1Test, unittest.TestCase):
    def setUp(self):
        self.fru = "bb"
        self.fru_cmd = ["/usr/local/bin/fruid-util", "bb"]
        self.fru_fields = {"chassis": 0, "product": 2, "board": 2}


class FruNicTest(FruSlot1Test, unittest.TestCase):
    def setUp(self):
        self.fru = "nic"
        self.fru_cmd = ["/usr/local/bin/fruid-util", "nic"]
        self.fru_fields = {"product": 0, "board": 0}

    def getProductFields(self, num_custom=0):
        product_fields = {
            "Product Manufacturer",
            "Product Name",
            "Product Part Number",
            "Product Version",
            "Product Serial",
        }
        for i in range(1, num_custom + 1):
            product_fields.add("Product Custom Data {}".format(i))
        return product_fields
