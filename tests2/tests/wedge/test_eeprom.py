#!/usr/bin/env python
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

from common.base_eeprom_test import CommonEepromTest


class EepromTest(CommonEepromTest, unittest.TestCase):
    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil"]

    def set_product_name(self):
        self.product_name = ["Wedge"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["WEDGE"]
