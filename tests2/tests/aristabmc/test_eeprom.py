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

import unittest

from common.base_eeprom_test import EepromV5Test


class AristaEepromTest(EepromV5Test):
    EEPROM_TARGET = None
    PRODUCT_NAMES = None

    def set_eeprom_cmd(self):
        self.assertIsNotNone(self.EEPROM_TARGET, "EEPROM target not set")
        self.eeprom_cmd = ["/usr/bin/weutil -e {}".format(self.EEPROM_TARGET)]

    def set_product_name(self):
        self.assertIsNotNone(self.PRODUCT_NAMES, "Product names not set")
        self.product_name = self.PRODUCT_NAMES

    # These fields are not present in current AristaBMC EEPROM output.
    def test_location_on_fabric(self):
        pass


class BmcEepromTest(AristaEepromTest, unittest.TestCase):
    EEPROM_TARGET = "bmc"
    PRODUCT_NAMES = ["SCM-BMC"]

    def test_bmc_mac(self):
        pass

    def test_x86_mac(self):
        pass

    def test_switch_asic_mac(self):
        pass


class ScmEepromTest(AristaEepromTest, unittest.TestCase):
    EEPROM_TARGET = "scm"
    PRODUCT_NAMES = ["SMB"]


class ChassisEepromTest(AristaEepromTest, unittest.TestCase):
    EEPROM_TARGET = "chassis_eeprom"
    PRODUCT_NAMES = ["M4062ANW"]
