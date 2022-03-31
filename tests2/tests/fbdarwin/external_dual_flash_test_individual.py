#!/usr/bin/env python3
#
# Copyright 2022-present Facebook. All Rights Reserved.
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
from common.base_dual_flash_test import BaseBmcDualFlashTest


class BMCDualFlashTest(BaseBmcDualFlashTest, unittest.TestCase):
    """
    Individual test for BMC dual flash
    """

    DEFAULT_GET_BOOT_INFO_CMD = "boot_info.sh bmc"
    DEFAULT_SWITCH_SLAVE_CMD = "boot_info.sh bmc reset slave"
    DEFAULT_POWER_RESET_CMD = "wedge_power.sh reset -s"
    DEFAULT_SCM_BOOT_TIME = 30  # SCM startup time
    DEFAULT_BMC_RECONNECT_TIMEOUT = 300  # BMC Booting timeout
    DEFAULT_COMMAND_EXEC_DELAY = 1  # Delay for UUT command handler
    DEFAULT_COMMAND_PROMTP_TIME_OUT = 10  # Waiting prompt timeout
    BOOT_SLAVE_KEYWORD = [
        "WDT1 Timeout Count:  0",
        "WDT2 Timeout Count:  1",
        "Slave",
    ]
    BOOT_MASTER_KEYWORD = [
        "WDT1 Timeout Count:  0",
        "WDT2 Timeout Count:  0",
        "Master",
    ]

    def test_bmc_dual_flash(self):
        super().do_external_BMC_dual_flash()
