#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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

from tests.fbgc.test_data.firmware_upgrade.firmware_upgrade_config import FwUpgradeTest


class ServerFPGAFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for Server FPGA
    """

    DEFAULT_POWER_RESET_CMD = "/usr/local/bin/power-util server 12V-cycle"

    def test_server_fpga_fw_upgrade(self):
        super().do_external_firmware_upgrade("server_fpga")


class UicFPGAFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for UIC FPGA
    """

    DEFAULT_POWER_RESET_CMD = "/usr/local/bin/power-util sled-cycle"

    def test_uic_fpga_fw_upgrade(self):
        super().do_external_firmware_upgrade("uic_fpga")


class BicFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for Bridge-IC
    """

    DEFAULT_POWER_RESET_CMD = "/usr/local/bin/power-util server 12V-cycle"

    def test_bic_fw_upgrade(self):
        super().do_external_firmware_upgrade("bic")


class BiosFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for Master BIOS
    """

    DEFAULT_POWER_RESET_CMD = "/sbin/reboot"

    def test_bios_fw_upgrade(self):
        super().do_external_firmware_upgrade("bios")


class BmcFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for BMC
    """

    DEFAULT_POWER_RESET_CMD = "/sbin/reboot"

    def test_bmc_fw_upgrade(self):
        super().do_external_firmware_upgrade("bmc")


class RomFwUpgradeTest(FwUpgradeTest, unittest.TestCase):
    """
    Individual test for ROM
    """

    DEFAULT_POWER_RESET_CMD = "/sbin/reboot"

    def test_rom_fw_upgrade(self):
        super().do_external_firmware_upgrade("rom")
