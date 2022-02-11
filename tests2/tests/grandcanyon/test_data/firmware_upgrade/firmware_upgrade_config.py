#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.,
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
import os
from abc import abstractmethod
from utils.shell_util import run_shell_cmd
from utils.cit_logger import Logger

from common.base_fw_upgrade_test import DEV_SERVER_RESOURCE_PATH, BaseFwUpgradeTest


# Global variables
# Upgrader and binary path on dev server and bmc target BMC unit.
UUT_RESOURCE_PATH = "/tmp/fw_upgrade"

# You have to add sys path of upgrader before importing upgrader class here.
sys.path.append(DEV_SERVER_RESOURCE_PATH)
try:
    from entity_upgrader import FwUpgrader
except Exception:
    pass

try:
    # The upgrader need to be inside e.g. /tmp/fw_upgrade on dev server.
    sys.path.insert(0, DEV_SERVER_RESOURCE_PATH)
    import fw_json as fw_up
    from constants import (
        UFW_NAME,
        UFW_VERSION,
        UFW_HASH,
        UFW_HASH_VALUE,
        HashType,
        UFW_GET_VERSION,
        UFW_CMD,
        UFW_CONDITION,
        UFW_ENTITY_INSTANCE,
        UFW_PRIORITY,
    )
    from entity_upgrader import FwEntityUpgrader, FwUpgrader
except Exception:
    pass

# Override default settings
UPGRADING_TIMEOUT = {
    "bic": 900,
    "bios": 1200,
    "server_fpga": 900,
    "uic_fpga": 900,
    "bmc": 360,
    "rom": 360,
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
    "bmc succeeded",
    "rom succeeded",
    "fpga succeeded",
]
NUM_LAST_FAILED_EXPECTED_KEY = 4  # zero-based number 0...N
BMC_RECONNECT_TIMEOUT = 300
SCM_BOOT_TIME = 30


class FwUpgradeTest(BaseFwUpgradeTest):
    """
    Class to initialize common settings for the external firmware testing
    """

    @abstractmethod
    def override_common(self):
        pass

    def verify_binary_checksum_on_remote_target(self, logging=False):
        """
        verify that the firmware binary hash on UUT matches
        the ones in the json
        """
        if logging:
            self.print_line(
                "Checking binary on UUT ",
                "left",
                ".",
                self.MAX_LINE_LEN - 10,
                endline=" ",
            )

        sha1sum_cmd = "sha1sum {} | cut -d ' ' -f 1"
        md5sum_cmd = "md5sum {} | cut -d ' ' -f 1"

        for fw_entity in self.json:
            filename = os.path.join(
                self.remote_bin_path, self.json[fw_entity][UFW_NAME]
            )

            # copy f/w image file to UUT
            test_cmd = "mkdir {}".format(self.remote_bin_path)
            self.send_command_to_UUT(test_cmd)

            run_shell_cmd(
                "sshpass -p 0penBmc scp {} {}:{}".format(
                    filename, self.bmc_hostname, filename
                )
            )

            matchingHash = False

            if self.json[fw_entity][UFW_HASH] == HashType.SHA1SUM.value:
                test_cmd = sha1sum_cmd.format(filename)
            elif self.json[fw_entity][UFW_HASH] == HashType.MD5SUM.value:
                test_cmd = md5sum_cmd.format(filename)
            else:
                self.fail("unknow hash type on component {}".format(fw_entity))

            self.send_command_to_UUT(test_cmd)
            binary_hash = self.receive_command_output_from_UUT(only_last=True)
            # verify hash string from UUT
            matchingHash = binary_hash == self.json[fw_entity][UFW_HASH_VALUE]
            self.assertTrue(
                matchingHash,
                "firmware component {} missmatch for file {}".format(
                    fw_entity, filename
                ),
            )
        if logging:
            print("Done")

    def set_common_settings(self):
        self.hostname = None
        self.num_last_failed_expected_key = NUM_LAST_FAILED_EXPECTED_KEY
        self.expected_keyword = EXPECTED_KEYWORD
        self.upgrader_path = DEV_SERVER_RESOURCE_PATH
        self.remote_bin_path = UUT_RESOURCE_PATH
        self.upgrading_timeout = UPGRADING_TIMEOUT
        self.bmc_reconnect_timeout = BMC_RECONNECT_TIMEOUT
        self.scm_boot_time = SCM_BOOT_TIME
        self.override_common()
        pass
