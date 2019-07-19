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

from common.base_eeprom_test import CommonEepromTest
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


# YAMP is not FB Hardware, thus it uses different EEPROM format from FB HW.
# On this platform, not all field in the EEPROM are populated.
# This base class for YAMP will mask out those fields that does not exist.
# All YAMP Eeprom tests will inherit this class.
# This class is used for testing Switch Card EEPROM, and is also used as
# base class of other EEPROMs with eeprom_cmd overridden
class YAMPEepromTest(CommonEepromTest, unittest.TestCase):
    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil"]

    # On all YAMP EEPROM, product name is always YAMP, even PIMs (LCs)
    def set_product_name(self):
        self.product_name = ["YAMP"]

    # Masking out the fields that is not populated in YAMP
    @unittest.skip("Test not supported on platform")
    def test_odm_pcb(self):
        return

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
    def test_local_mac(self):
        return

    @unittest.skip("Test not supported on platform")
    def test_location_on_fabric(self):
        return


# YAMP SUP has even fewer fields populated. So we will not test against
# those empty fields.
class SupervisorEepromTest(YAMPEepromTest):
    """
    Test for weutil SUP
    """

    def set_eeprom_cmd(self):
        # weutil SUP reads BIOS, thus prints out extra message
        # so we need to filter out irrelevant messages
        self.eeprom_cmd = ["/usr/bin/weutil SUP"]

    def run_eeprom_cmd(self):
        self.assertNotEqual(self.eeprom_cmd, None, "EEPROM command not set")
        Logger.info("Running EEPROM command: " + str(self.eeprom_cmd))
        self.eeprom_info = run_shell_cmd(cmd=self.eeprom_cmd, ignore_err=True)
        Logger.info(self.eeprom_info)
        return self.eeprom_info


class PIM1EepromTest(YAMPEepromTest):
    """
    Test for weutil LC1
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil LC1"]


class PIM2EepromTest(YAMPEepromTest):
    """
    Test for weutil LC2
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil LC2"]


class PIM3EepromTest(YAMPEepromTest):
    """
    Test for weutil LC3
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil LC3"]


class PIM4EepromTest(YAMPEepromTest):
    """
    Test for weutil LC4
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil LC4"]


class PIM5EepromTest(YAMPEepromTest):
    """
    Test for weutil LC5
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil LC5"]


class PIM6EepromTest(YAMPEepromTest):
    """
    Test for weutil LC6
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil LC6"]


class PIM7EepromTest(YAMPEepromTest):
    """
    Test for weutil LC7
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil LC7"]


class PIM8EepromTest(YAMPEepromTest):
    """
    Test for weutil LC8
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil LC8"]
