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

from common.base_pem_test import PemInfoTest, PemEepromTest, PemBlackboxTest


class Pem1InfoTest(PemInfoTest, unittest.TestCase):
    """
    Test for pem1 info
    """

    def set_pem_cmd(self):
        self.pem_cmd = ["/usr/bin/pem-util pem1 --get_pem_info --print"]
        self.pem_id = 1


class Pem2InfoTest(PemInfoTest, unittest.TestCase):
    """
    Test for pem2 info
    """

    def set_pem_cmd(self):
        self.pem_cmd = ["/usr/bin/pem-util pem2 --get_pem_info --print"]
        self.pem_id = 2


class Pem1EepromTest(PemEepromTest, unittest.TestCase):
    """
    Test for pem1 Eeprom
    """

    def set_pem_cmd(self):
        self.pem_cmd = ["/usr/bin/pem-util pem1 --get_eeprom_info --print"]
        self.pem_id = 1

    def set_product_name(self):
        self.product_name = ["WEDGE400-PEM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PEM"]


class Pem2EepromTest(PemEepromTest, unittest.TestCase):
    """
    Test for pem2 Eeprom
    """

    def set_pem_cmd(self):
        self.pem_cmd = ["/usr/bin/pem-util pem2 --get_eeprom_info --print"]
        self.pem_id = 2

    def set_product_name(self):
        self.product_name = ["WEDGE400-PEM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["PEM"]


class Pem1BlackboxTest(PemBlackboxTest, unittest.TestCase):
    """
    Test for pem1 Blackbox
    """

    def set_pem_cmd(self):
        self.pem_cmd = ["/usr/bin/pem-util pem1 --get_blackbox_info --print"]
        self.pem_id = 1


class Pem2BlackboxTest(PemBlackboxTest, unittest.TestCase):
    """
    Test for pem2 Blackbox
    """

    def set_pem_cmd(self):
        self.pem_cmd = ["/usr/bin/pem-util pem2 --get_blackbox_info --print"]
        self.pem_id = 2
