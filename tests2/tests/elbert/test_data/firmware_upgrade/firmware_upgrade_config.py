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

from common.base_fw_upgrade_test import BaseFwUpgradeTest, DEV_SERVER_RESOURCE_PATH


# Global variables
# Upgrader and binary path on dev server and bmc target BMC unit.
UUT_RESOURCE_PATH = "/mnt/data1/fw_upgrade"

# You have to add sys path of upgrader before importing upgrader class here.
sys.path.insert(0, DEV_SERVER_RESOURCE_PATH)
try:
    from entity_upgrader import FwUpgrader
except Exception:
    pass

# Override default settings
UPGRADING_TIMEOUT = {
    "bios": 1200,
    "scm": 120,
    "smb": 120,
    "smb_cpld": 60,
    "fan": 60,
    "pim_base": 30,
    "pim8ddm": 30,
    "pim16q": 30,
    "th4_qspi": 30,
}
EXPECTED_KEYWORD = [
    "closed by remote host",
    "not supported",
    "No EEPROM/flash",
    "FAIL",
    "returned error",  # end of failed expected keyword
    "PASS",
    "VERIFIED.",
    "content is identical",
    "bios succeeded",
    "Exit code = 0... Success",
    "Erase/write done",
]
NUM_LAST_FAILED_EXPECTED_KEY = 4  # zero-based number 0...N

# bios runs
#   wedge_power.sh off && sleep 60 && bios_util.sh write <rom> --init-aconf; \
#     RC=$?; wedge_power.sh on; exit $RC
# --init-aconf performs a SECOND flash write (the aboot_conf partition) after
# the main image. Every success keyword -- "Erase/write done", then "VERIFIED."
# -- is printed by the FIRST write, so matching any of them ends the wait while
# aboot_conf is still unwritten. The test then power cycles the box, leaving the
# host with no valid Aboot config and unable to boot (`aconf_util.sh program`
# is what recovers it).
#
# The command ends in `exit`, so the session closing is its real completion
# signal. Wait for that instead; failure keywords still short-circuit.
WAIT_FOR_EOF_ENTITIES = frozenset({"bios"})
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
        self.wait_for_eof_entities = WAIT_FOR_EOF_ENTITIES
        self.upgrader_path = DEV_SERVER_RESOURCE_PATH
        self.remote_bin_path = UUT_RESOURCE_PATH
        self.upgrading_timeout = UPGRADING_TIMEOUT
        self.bmc_reconnect_timeout = BMC_RECONNECT_TIMEOUT
        self.scm_boot_time = SCM_BOOT_TIME
        self.power_reset_cmd = POWER_RESET_CMD
        self.override_common()
        pass
