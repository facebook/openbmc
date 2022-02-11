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

from tests.elbert.test_data.firmware_upgrade.firmware_upgrade_config import (
    FwUpgradeTest,
)


class BiosFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for Master BIOS
    """

    def test_bios_fw_upgrade(self):
        super().do_external_firmware_upgrade("bios")


class ScmFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for System Control Management CPLD
    """

    def test_scm_fw_upgrade(self):
        super().do_external_firmware_upgrade("scm")


class SmbFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for System Management Board CPLD
    """

    def test_smb_fw_upgrade(self):
        super().do_external_firmware_upgrade("smb")


class SmbCpldFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for System Management Board Extra CPLD
    """

    def test_smb_cpld_fw_upgrade(self):
        super().do_external_firmware_upgrade("smb_cpld")


class FanFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for Fan CPLD
    """

    def test_fan_fw_upgrade(self):
        super().do_external_firmware_upgrade("fan")


class PimBaseUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for Port Interface Module CPLD Base Partition
    """

    def test_pim_base_fw_upgrade(self):
        super().do_external_firmware_upgrade("pim_base")


class Pim16qUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for Port Interface Module CPLD PIM16Q Partition
    """

    def test_pim16q_fw_upgrade(self):
        super().do_external_firmware_upgrade("pim16q")


class Pim8ddmUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for Port Interface Module CPLD PIM8DDM Partition
    """

    def test_pim8ddm_fw_upgrade(self):
        super().do_external_firmware_upgrade("pim8ddm")
