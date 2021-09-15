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

from tests.yamp.test_data.firmware_upgrade.firmware_upgrade_config import (
    FwUpgradeTest,
)


class ScmFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for System Control Management CPLD
    """

    def test_scm_fw_upgrade(self):
        super().do_external_firmware_upgrade("dawson")


class SmbFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for switch card cpld which is
    also called livingston
    """

    def test_smb_fw_upgrade(self):
        super().do_external_firmware_upgrade("livingston")


class FpgaFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for PIM FPGA which is also
    called sperry
    """

    def test_fpga_fw_upgrade(self):
        super().do_external_firmware_upgrade("sperry")


class BiosFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for Master BIOS
    """

    def test_bios_fw_upgrade(self):
        super().do_external_firmware_upgrade("bios")
