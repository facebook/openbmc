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

from common.base_eeprom_test import CommonEepromTest
from utils.cit_logger import Logger
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class EepromTest(CommonEepromTest, unittest.TestCase):
    _FABRIC_CARD_NAMES = ["GALAXY-FC", "GALAXY-FAB"]
    _LC_CARD_NAMES = ["GALAXY-LC"]

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil"]

    def set_product_name(self):
        self.product_name = self._FABRIC_CARD_NAMES + self._LC_CARD_NAMES

    def set_location_on_fabric(self):
        self.location_on_fabric = ["RIGHT", "LEFT"]

    def test_location_on_fabric(self):
        found, name = self.is_found_product_name()
        for item in self._FABRIC_CARD_NAMES:
            if item.lower() in name:
                # FABRIC card have no location set in the chassis
                # proabably because they are all at the back
                Logger.info("On Fabric card skipping test")
                return unittest.skip("Skipping test on Fabric card")
        super().test_location_on_fabric()
