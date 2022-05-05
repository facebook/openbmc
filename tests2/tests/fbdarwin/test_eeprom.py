#!/usr/bin/env python3
#
# Copyright 2022-present Facebook. All Rights Reserved.
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

import re
import unittest

from common.base_eeprom_test import CommonEepromTest
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class FBDARWINEepromTest(CommonEepromTest):
    """
    Base Class for FBDARWIN EEPROM fields.
    """

    # On all FBDARWIN EEPROM, product name is always FBDARWIN, even PIMs
    def set_product_name(self):
        self.product_name = ["FBDARWIN"]

    # Masking out the fields that is not populated in FBDARWIN
    def test_odm_pcb(self):
        self.get_eeprom_info()
        self.assertIn("ODM PCBA Part Number", self.eeprom_info)

    @unittest.skip("Test not supported on platform")
    def test_asset_tag(self):
        return

    @unittest.skip("Test not supported on platform")
    def test_system_manufacturer(self):
        return

    def test_system_manufacturer_date(self):
        self.get_eeprom_info()
        # Only Manufacturing date is populated
        self.assertIn("System Manufacturing Date", self.eeprom_info)

    @unittest.skip("Test not supported on platform")
    def test_location_on_fabric(self):
        return

    def test_system_assembly_part_number(self):
        self.get_eeprom_info()
        self.assertIn("System Assembly Part Number", self.eeprom_info)


class BmcEepromTest(FBDARWINEepromTest, unittest.TestCase):
    """
    Test for weutil BMC
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil BMC"]
