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

from tests.wedge400.test_data.firmware_upgrade.firmware_upgrade_config import (
    FwUpgradeTest,
)


class ScmFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for System Control Management CPLD
    """

    def test_scm_fw_upgrade(self):
        super().do_external_firmware_upgrade("scm")


class FcmFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for Fan Control Management CPLD
    """

    def test_fcm_fw_upgrade(self):
        super().do_external_firmware_upgrade("fcm")


class SmbFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for System Management Board CPLD
    """

    def test_smb_fw_upgrade(self):
        super().do_external_firmware_upgrade("smb")


class PwrFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for Power CPLD
    """

    def test_pwr_fw_upgrade(self):
        super().do_external_firmware_upgrade("pwr")


class FpgaFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for DOM FPGA
    """

    def test_fpga_fw_upgrade(self):
        super().do_external_firmware_upgrade("fpga")


class BicFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for Bridge-IC
    """

    def test_bic_fw_upgrade(self):
        super().do_external_firmware_upgrade("bic")


class BiosFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
        Individual test for Master BIOS
    """

    def test_bios_fw_upgrade(self):
        super().do_external_firmware_upgrade("bios")
