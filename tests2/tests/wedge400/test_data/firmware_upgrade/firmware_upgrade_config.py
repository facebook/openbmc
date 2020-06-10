#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.,
#
# This program file is free software; you can redistribute it and/or modify it,
# under the terms of the GNU General Public License as published by the,
# Free Software Foundation; version 2 of the License.,
#
# This program is distributed in the hope that it will be useful, but WITHOUT,
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or,
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License,
# for more details.,
#
# You should have received a copy of the GNU General Public License,
# along with this program in a file named COPYING; if not, write to the,
# Free Software Foundation, Inc.,,
# 51 Franklin Street, Fifth Floor,,
# Boston, MA 02110-1301 USA,
#

import sys
from abc import abstractmethod

from common.base_fw_upgrade_test import BaseFwUpgradeTest


# Global variables
# Upgrader and binary path on dev server and bmc target BMC unit.
DEV_SERVER_RESOURCE_PATH = "/tmp/fw_upgrade"
UUT_RESOURCE_PATH = "/mnt/data1/fw_upgrade"


# You have to add sys path of upgrader before importing upgrader class here.
sys.path.append(DEV_SERVER_RESOURCE_PATH)
try:
    from entity_upgrader import FwUpgrader
except Exception:
    pass

# Override default settings
UPGRADING_TIMEOUT = {
    "bic": 300,
    "bios": 1200,
    "fpga": 900,
    "scm": 300,
    "fcm": 300,
    "pwr": 300,
    "smb": 300,
}
EXPECTED_KEYWORD = [
    "closed by remote host",
    "not supported",
    "No EEPROM/flash",
    "FAIL!",
    "returned error",  # end of failed expected keyword
    "PASS!",
    "VERIFIED.",
    "content is identical",
    "bios succeeded",
    "bic succeeded",
]
NUM_LAST_FAILED_EXPECTED_KEY = 4  # zero-based number 0...N
BMC_RECONNECT_TIMEOUT = 300
SCM_BOOT_TIME = 30
try:
    POWER_RESET_CMD = FwUpgrader._POWER_RESET_HARD
except Exception:
    pass


class FwUpgradeTest(BaseFwUpgradeTest):
    """
        Class to initialize common settings for the external firmware testing
    """

    @abstractmethod
    def override_common(self):
        pass

    def set_common_settings(self):
        self.hostname = None
        self.num_last_failed_expected_key = NUM_LAST_FAILED_EXPECTED_KEY
        self.expected_keyword = EXPECTED_KEYWORD
        self.upgrader_path = DEV_SERVER_RESOURCE_PATH
        self.remote_bin_path = UUT_RESOURCE_PATH
        self.upgrading_timeout = UPGRADING_TIMEOUT
        self.bmc_reconnect_timeout = BMC_RECONNECT_TIMEOUT
        self.scm_boot_time = SCM_BOOT_TIME
        self.power_reset_cmd = POWER_RESET_CMD
        self.override_common()
        pass
