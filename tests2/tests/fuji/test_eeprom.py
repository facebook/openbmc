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
import unittest

from common.base_eeprom_test import CommonEepromTest


class EepromTest(CommonEepromTest, unittest.TestCase):
    """
    Test for weutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/smbutil"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_SMB"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SMB"]


class BMCEepromTest(EepromTest, unittest.TestCase):
    """
    Test for beutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/beutil"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_BMC"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["BMC"]


class SIMEepromTest(EepromTest, unittest.TestCase):
    """
    Test for simutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/bin/weutil"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SIM"]


class SCMEepromTest(EepromTest, unittest.TestCase):
    """
    Test for seutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/seutil"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_SCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SCM"]


class PIM1EepromTest(EepromTest, unittest.TestCase):
    """
    Test for peutil 1
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 1"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_PIM_16QDD"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PIM16Q"]


class PIM2EepromTest(EepromTest, unittest.TestCase):
    """
    Test for peutil 2
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 2"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_PIM_16QDD"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PIM16Q"]


class PIM3EepromTest(EepromTest, unittest.TestCase):
    """
    Test for peutil 3
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 3"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_PIM_16QDD"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PIM16Q"]


class PIM4EepromTest(EepromTest, unittest.TestCase):
    """
    Test for peutil 4
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 4"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_PIM_16QDD"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PIM16Q"]


class PIM5EepromTest(EepromTest, unittest.TestCase):
    """
    Test for peutil 5
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 5"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_PIM_16QDD"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PIM16Q"]


class PIM6EepromTest(EepromTest, unittest.TestCase):
    """
    Test for peutil 6
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 6"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_PIM_16QDD"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PIM16Q"]


class PIM7EepromTest(EepromTest, unittest.TestCase):
    """
    Test for peutil 7
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 7"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_PIM_16QDD"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PIM16Q"]


class PIM8EepromTest(EepromTest, unittest.TestCase):
    """
    Test for peutil 8
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/peutil 8"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_PIM_16QDD"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PIM16Q"]


class FCMTEepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fcm-t
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil fcm-t"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_FCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FCM"]


class FCMBEepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fcm-b
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil fcm-b"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2_FCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FCM"]


class FAN1EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 1
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 1"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2-FAN", "WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN2EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 2
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 2"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2-FAN", "WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN3EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 3
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 3"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2-FAN", "WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN4EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 4
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 4"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2-FAN", "WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN5EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 5
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 5"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2-FAN", "WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN6EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 6
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 6"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2-FAN", "WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN7EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 7
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 7"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2-FAN", "WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN8EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 8
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 8"]

    def set_product_name(self):
        self.product_name = ["MINIPACK2-FAN", "WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]
