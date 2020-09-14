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

from tests.wedge100.test_data.firmware_upgrade.firmware_upgrade_config import (
    FwUpgradeTest,
)


class bcm5387EepromFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for bcm5387 Eeprom
    """

    def test_bcm5387_eeprom_fw_upgrade(self):
        super().do_external_firmware_upgrade("bcm5387_eeprom")


class FanRackmonCpldFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for Fan Rackmon  CPLD
    """

    def test_fan_rackmon_cpld_fw_upgrade(self):
        super().do_external_firmware_upgrade("fan_rackmon_cpld")


class SysCpldFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for Sys CPLD
    """

    def test_sys_cpld_fw_upgrade(self):
        super().do_external_firmware_upgrade("sys_cpld")
