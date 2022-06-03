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
from tests.wedge400.helper.libpal import pal_get_board_type_rev
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
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


class SCMEepromTest(EepromTest, unittest.TestCase):
    """
    Test for seutil
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/seutil"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-SCM", "WEDGE400C-SCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["SCM"]


class FCMEepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fcm
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil fcm"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FCM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FCM"]


class FAN1EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 1
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 1"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN2EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 2
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 2"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN3EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 3
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 3"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]


class FAN4EepromTest(EepromTest, unittest.TestCase):
    """
    Test for feutil fan 4
    """

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/feutil 4"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-FAN"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["FAN"]

class BSMEepromTest(EepromTest, unittest.TestCase):
    """
    Test for bsm-eutil BSM Board
    """

    def setUp(self):
        platform_type_rev = pal_get_board_type_rev()
        if (
            platform_type_rev == "Wedge400-MP-Respin"
            or platform_type_rev == "Wedge400C-DVT"
            or platform_type_rev == "Wedge400C-DVT2"
            or platform_type_rev == "Wedge400C-MP-Respin"
        ):
            pass
        else:
            self.skipTest("Skipping because BSM EEPROM not available on {} ".format(platform_type_rev))
        super().setUp()

    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/bsm-eutil"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-BSM","WEDGE400C-BSM"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["BSM"]

class RACKMONEepromTest(EepromTest, unittest.TestCase):
    """
    Test for reutil Rackmon
    """

    def setUp(self):
        platform_type_rev = pal_get_board_type_rev()
        if (
           platform_type_rev == "Wedge400-MP-Respin"
            or platform_type_rev == "Wedge400C-MP-Respin"
        ):
            pass
        else:
            self.skipTest("Skipping because Rackmon EEPROM not available on {} ".format(platform_type_rev))
        super().setUp()


    def set_eeprom_cmd(self):
        self.eeprom_cmd = ["/usr/local/bin/reutil"]

    def set_product_name(self):
        self.product_name = ["WEDGE400-RACKMON"]

    def set_location_on_fabric(self):
        self.location_on_fabric = ["RACKMON"]
