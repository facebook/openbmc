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
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class CHASSISEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for CHASSIS EEPROM
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil -e chassis_eeprom"]

    def set_product_name(self):
        self.product_name = ["MINIPACK3"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FCM"]

    def set_eeprom_location(self):
        self.test_eeprom_location = "EEPROM location on Fabric"

    def set_odm_pcba_number(self):
        self.odm_pcba_part_number = "ODM/JDM PCBA Part Number"
        self.odm_pcba_serial_number = "ODM/JDM PCBA Serial Number"

    def test_asset_tag(self):
        self.asset_tag = []


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class BMCEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for BMC EEPROM
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil -e bmc_eeprom"]

    def set_product_name(self):
        self.product_name = ["MINIPACK3"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["BMC"]

    def set_eeprom_location(self):
        self.test_eeprom_location = "EEPROM location on Fabric"

    def set_odm_pcba_number(self):
        self.odm_pcba_part_number = "ODM/JDM PCBA Part Number"
        self.odm_pcba_serial_number = "ODM/JDM PCBA Serial Number"

    def test_asset_tag(self):
        self.asset_tag = []

    def test_extended_mac_base(self):
        self.extended_mac = []

    def test_local_mac(self):
        self.local_mac = []


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class SCMEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for SCM EEPROM
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil -e scm_eeprom"]

    def set_product_name(self):
        self.product_name = ["MINIPACK3"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SCM"]

    def set_eeprom_location(self):
        self.test_eeprom_location = "EEPROM location on Fabric"

    def set_odm_pcba_number(self):
        self.odm_pcba_part_number = "ODM/JDM PCBA Part Number"
        self.odm_pcba_serial_number = "ODM/JDM PCBA Serial Number"

    def test_asset_tag(self):
        self.asset_tag = []

    def test_extended_mac_base(self):
        self.extended_mac = []
