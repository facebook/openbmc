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

from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseEepromTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.eeprom_cmd = None
        self.eeprom_info = None
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def set_eeprom_cmd(self):
        pass

    @abstractmethod
    def set_product_name(self):
        pass

    @abstractmethod
    def set_location_on_fabric(self):
        pass

    def run_eeprom_cmd(self):
        self.assertNotEqual(self.eeprom_cmd, None, "EEPROM command not set")

        Logger.info("Running EEPROM command: " + str(self.eeprom_cmd))
        self.eeprom_info = run_shell_cmd(cmd=self.eeprom_cmd)
        Logger.info(self.eeprom_info)
        return self.eeprom_info


class CommonEepromTest(BaseEepromTest):
    def get_eeprom_info(self):
        self.set_eeprom_cmd()
        if not self.eeprom_info:
            self.eeprom_info = self.run_eeprom_cmd()
        return self.eeprom_info

    def test_version(self):
        self.get_eeprom_info()
        self.assertIn("Version", self.eeprom_info)

    def get_product_name(self):
        self.get_eeprom_info()
        self.set_product_name()
        name = self.eeprom_info.split("Product Name")[1].split(": ")[1].split("\n")[0]
        return name.lower()

    def is_found_product_name(self):
        name = self.get_product_name()
        found = False
        # check all possible product name types
        for item in self.product_name:
            if item.lower() in name:
                found = True
                break
        return found, name

    def test_product_name(self):
        """
        Tests if the platform defined product name is present
        in eeprom dump
        """
        found, name = self.is_found_product_name()
        self.assertTrue(
            found, "Product name %s not in set %s".format(name, self.product_name)
        )

    def test_product_part_number(self):
        self.get_eeprom_info()
        self.assertIn("Product Part Number", self.eeprom_info)

    def test_odm_pcb(self):
        self.get_eeprom_info()
        self.assertIn("ODM PCBA Part Number", self.eeprom_info)
        self.assertIn("ODM PCBA Serial Number", self.eeprom_info)

    def test_asset_tag(self):
        self.get_eeprom_info()
        self.assertIn("Product Asset Tag", self.eeprom_info)

    def test_product_serial_number(self):
        self.get_eeprom_info()
        self.assertIn("Product Production State", self.eeprom_info)
        self.assertIn("Product Version", self.eeprom_info)
        self.assertIn("Product Sub-Version", self.eeprom_info)
        self.assertIn("Product Serial Number", self.eeprom_info)

    def test_system_manufacturer(self):
        self.get_eeprom_info()
        self.assertIn("System Manufacturer", self.eeprom_info)
        self.assertIn("System Manufacturing Date", self.eeprom_info)

    def test_local_mac(self):
        self.get_eeprom_info()
        self.assertIn("Local MAC", self.eeprom_info)

    def test_extended_mac_base(self):
        self.get_eeprom_info()
        self.assertIn("Extended MAC Base", self.eeprom_info)

    def test_location_on_fabric(self):
        self.set_location_on_fabric()
        self.get_eeprom_info()
        found = False
        name = (
            self.eeprom_info.split("Location on Fabric")[1]
            .split(": ")[1]
            .split("\n")[0]
        )
        # check all possible product name types
        for item in self.location_on_fabric:
            if item.lower() in name.lower():
                found = True
        self.assertTrue(
            found,
            "Location on Fabric %s not in set %s".format(name, self.location_on_fabric),
        )
