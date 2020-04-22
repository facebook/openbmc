#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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


class ELBERTEepromTest(CommonEepromTest):
    """
    Base Class for ELBERT EEPROM fields.
    """

    # On all ELBERT EEPROM, product name is always ELBERT, even PIMs (LCs)
    def set_product_name(self):
        self.product_name = ["ELBERT"]

    # Masking out the fields that is not populated in ELBERT
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


# ELBERT SUP has even fewer fields populated. So we will not test against
# those empty fields.
class SupervisorEepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil SUP
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil SUP"]

    def run_eeprom_cmd(self):
        self.assertNotEqual(self.eeprom_cmd, None, "EEPROM command not set")
        Logger.info("Running EEPROM command: " + str(self.eeprom_cmd))
        self.eeprom_info = run_shell_cmd(cmd=self.eeprom_cmd, ignore_err=True)
        Logger.info(self.eeprom_info)
        return self.eeprom_info

    def test_mac_offset(self):
        # Make sure LocalMac is the same as Base + 1
        #
        self.get_eeprom_info()
        macRe = r"[0-9a-fA-F:]{17}"
        localMacRe = r"Local MAC: ({})".format(macRe)
        baseMacRe = r"Extended MAC Base: ({})".format(macRe)
        localMac = re.search(localMacRe, self.eeprom_info)
        self.assertNotEqual(localMac, None, "Local MAC address not found")
        baseMac = re.search(baseMacRe, self.eeprom_info)
        self.assertNotEqual(baseMac, None, "Extended MAC base address not found")

        # Convert MAC to hex
        localMac = hex(int(localMac.group(1).replace(":", ""), 16))
        baseMac = hex(int(baseMac.group(1).replace(":", ""), 16))

        # We expect Local mac to be BASE + 1 for ELBERT
        expectedBaseMac = hex((int(baseMac, 16) + 1))
        self.assertEqual(
            localMac, expectedBaseMac, "Local Mac is does not match expected value"
        )


# ELBERTTODO Chassis/LC/SCD/AST2620 Support
