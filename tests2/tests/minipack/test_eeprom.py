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


class EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for weutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-SMB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-SMB"]


class SCMEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for seutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/seutil"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-SCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-SCM"]


class PIM1EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for peutil 1
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 1"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-PIM16Q"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-PIM16Q"]


class PIM2EepromTest(CommonEepromTest, unittest.TestCase):
    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 2"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-PIM16Q"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-PIM16Q"]


class PIM3EepromTest(CommonEepromTest, unittest.TestCase):
    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 3"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-PIM16Q"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-PIM16Q"]


class PIM4EepromTest(CommonEepromTest, unittest.TestCase):
    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 4"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-PIM16Q"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-PIM16Q"]


class PIM5EepromTest(CommonEepromTest, unittest.TestCase):
    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 5"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-PIM16Q"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-PIM16Q"]


class PIM6EepromTest(CommonEepromTest, unittest.TestCase):
    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 6"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-PIM16Q"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-PIM16Q"]


class PIM7EepromTest(CommonEepromTest, unittest.TestCase):
    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 7"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-PIM16Q"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-PIM16Q"]


class PIM8EepromTest(CommonEepromTest, unittest.TestCase):
    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 8"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-PIM16Q"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-PIM16Q"]


class FCMTEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fcm-t
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil fcm-t"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FCM"]


class FCMBEepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fcm-b
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil fcm-b"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FCM"]


class FAN1EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 1
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 1"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FB"]


class FAN2EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 2
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 2"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FB"]


class FAN3EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 3
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 3"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FB"]


class FAN4EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 4
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 4"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FB"]


class FAN5EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 5
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 5"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FB"]


class FAN6EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 6
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 6"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FB"]


class FAN7EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 7
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 7"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FB"]


class FAN8EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for feutil fan 8
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 8"]

    def set_product_name(self):
        self.product_name = ["MINIPACK-FB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["MINIPACK-FB"]
