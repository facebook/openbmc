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


class FruMbTest(CommonFruTest, unittest.TestCase):
    def setUp(self):
        self.fru_cmd = ["/usr/local/bin/fruid-util", "mb"]
        self.fru_fields = {"product": 2, "board": 3, "chassis": 2}

    def getBoardFields(self, num_custom=0):
        board_fields = {
            "Board Mfg Date",
            "Board Mfg",
            "Board Product",
            "Board Serial",
            "Board Part Number",
        }
        for i in range(1, num_custom + 1):
            board_fields.add("Board Custom Data {}".format(i))
        return board_fields

    def getProductFields(self, num_custom=0):
        product_fields = {
            "Product Manufacturer",
            "Product Name",
            "Product Part Number",
            "Product Version",
            "Product Serial",
            "Product Asset Tag",
        }
        for i in range(1, num_custom + 1):
            product_fields.add("Product Custom Data {}".format(i))
        return product_fields


class FruRiserSlot2Test(CommonFruTest, unittest.TestCase):
    def setUp(self):
        self.fru_cmd = ["/usr/local/bin/fruid-util", "riser_slot2"]
        self.fru_fields = {"product": 1, "board": 4}

    def getBoardFields(self, num_custom=0):
        board_fields = {
            "Board Mfg Date",
            "Board Mfg",
            "Board Product",
            "Board Serial",
            "Board Part Number",
        }
        for i in range(1, num_custom + 1):
            board_fields.add("Board Custom Data {}".format(i))
        return board_fields

    def getProductFields(self, num_custom=0):
        product_fields = {
            "Product Part Number",
            "Product Serial",
        }
        for i in range(1, num_custom + 1):
            product_fields.add("Product Custom Data {}".format(i))
        return product_fields

    def getFields(self, field_type, num_custom):
        field_funcs = {
            "product": self.getProductFields,
            "board": self.getBoardFields,
        }
        self.assertIn(
            field_type, field_funcs, "Unknown field type: {}".format(field_type)
        )
        return field_funcs[field_type](num_custom)


class FruRiserSlot3Test(FruRiserSlot2Test):
    pass


@unittest.skip("not available")
class FruRiserSlot4Test(FruRiserSlot2Test):
    pass
