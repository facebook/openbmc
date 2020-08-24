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

    # On all ELBERT EEPROM, product name is always ELBERT, even PIMs
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


# ELBERT SCM has even fewer fields populated. So we will not test against
# those empty fields.
class SupervisorEepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil SCM
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil SCM"]

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


class ChassisEepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil CHASSIS
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil CHASSIS"]


class SmbExtraEepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil SMB_EXTRA
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil SMB_EXTRA"]


class SmbEepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil SMB
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil SMB"]


class BmcEepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil BMC
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil BMC"]


def pimMissing(pimNum):
    cmd = "head -n 1 /sys/bus/i2c/devices/4-0023/pim{}_present"
    return "0x0" in run_shell_cmd(cmd.format(pimNum))


@unittest.skipIf(pimMissing(2), "Skipping because PIM2 is not inserted.")
class PIM2EepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil PIM2
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil PIM2"]


@unittest.skipIf(pimMissing(3), "Skipping because PIM3 is not inserted.")
class PIM3EepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil PIM3
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil PIM3"]


@unittest.skipIf(pimMissing(4), "Skipping because PIM4 is not inserted.")
class PIM4EepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil PIM4
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil PIM4"]


@unittest.skipIf(pimMissing(5), "Skipping because PIM5 is not inserted.")
class PIM5EepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil PIM5
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil PIM5"]


@unittest.skipIf(pimMissing(6), "Skipping because PIM6 is not inserted.")
class PIM6EepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil PIM6
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil PIM6"]


@unittest.skipIf(pimMissing(7), "Skipping because PIM7 is not inserted.")
class PIM7EepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil PIM7
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil PIM7"]


@unittest.skipIf(pimMissing(8), "Skipping because PIM8 is not inserted.")
class PIM8EepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil PIM8
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil PIM8"]


@unittest.skipIf(pimMissing(9), "Skipping because PIM9 is not inserted.")
class PIM9EepromTest(ELBERTEepromTest, unittest.TestCase):
    """
    Test for weutil PIM9
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil PIM9"]
