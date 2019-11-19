#!/usr/bin/env python3
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
    """
    Test for weutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil"]

    def set_product_name(self):
        self.product_name = ["WEDGE400"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SMB"]


class SCMEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for seutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/seutil"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-SCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SCM"]


class FCMEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fcm
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil fcm"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FCM"]


class FAN1EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 1
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 1"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN2EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 2
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 2"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN3EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 3
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 3"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN4EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 4
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 4"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]
