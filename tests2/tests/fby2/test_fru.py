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


class FruSpbTest(CommonFruTest):
    def set_fru_cmd(self):
        self.fru_cmd = ["/usr/local/bin/fruid-util", "spb"]

    def set_fru_fields(self):
        self.fru_fields = (
            "Chassis Type",
            "Chassis Part Number",
            "Chassis Serial Number",
            "Board Mfg Date",
            "Board Mfg",
            "Board Product",
            "Board Serial",
            "Board Part Number",
            "Board FRU ID",
            "Board Custom Data 1",
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
