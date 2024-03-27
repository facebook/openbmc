#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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


class ChassisEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for CHASSIS EEPROM
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["weutil"]

    def set_product_name(self):
        self.product_name = ["TAHAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SMB"]

    # overide for FBOSS EEPROMv4 format, display different
    def set_eeprom_location(self):
        self.test_eeprom_location = "EEPROM location on Fabric"

    # overide for FBOSS EEPROMv4 format, display different
    def set_odm_pcba_number(self):
        self.odm_pcba_part_number = "ODM/JDM PCBA Part Number"
        self.odm_pcba_serial_number = "ODM/JDM PCBA Serial Number"

    # not asset tag field for FBOSS EEPROMv4 format
    def test_asset_tag(self):
        pass


class BMCEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for BMC EEPROM
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["weutil -e bmc_eeprom"]

    def set_product_name(self):
        self.product_name = ["MINERVA_BMC"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["BMC"]

    # overide for FBOSS EEPROMv4 format, display different
    def set_eeprom_location(self):
        self.test_eeprom_location = "EEPROM location on Fabric"

    # overide for FBOSS EEPROMv4 format, display different
    def set_odm_pcba_number(self):
        self.odm_pcba_part_number = "ODM/JDM PCBA Part Number"
        self.odm_pcba_serial_number = "ODM/JDM PCBA Serial Number"

    # not asset tag field for FBOSS EEPROMv4 format
    def test_asset_tag(self):
        pass

    # no mac field for BMC EEPROM
    def test_extended_mac_base(self):
        pass

    # no mac field for BMC EEPROM
    def test_local_mac(self):
        pass
