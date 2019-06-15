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

from common.base_fru_test import CommonFruTest


class FruMbTest(CommonFruTest):
    def setUp(self):
        self.fru_cmd = ["/usr/local/bin/fruid-util", "mb"]
        self.fru_fields = (
            "Chassis Type",
            "Chassis Part Number",
            "Chassis Serial Number",
            "Chassis Custom Data 1",
            "Chassis Custom Data 2",
            "Board Mfg Date",
            "Board Mfg",
            "Board Product",
            "Board Serial",
            "Board Part Number",
            "Board FRU ID",
            "Board Custom Data 1",
            "Board Custom Data 2",
            "Product Manufacturer",
            "Product Name",
            "Product Part Number",
            "Product Version",
            "Product Serial",
            "Product Asset Tag",
            "Product FRU ID",
            "Product Custom Data 1",
            "Product Custom Data 2",
        )
