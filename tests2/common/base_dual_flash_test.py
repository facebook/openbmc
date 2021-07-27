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

import os
import time
from common.base_fw_upgrade_test import BaseFwUpgradeTest

# Global variables
G_VERBOSE = False


class BaseBmcDualFlashTest(BaseFwUpgradeTest):
    # class BaseBmcDualFlashTest():
    """
    On dev server
    1. copy directory tests2 to dev server

    On BMC
    1. static ip to interface to /mnt/data/etc/rc.local
        - add this line to file: ifconfig eth0 <ip>

    Test plan:
    Running cit external test command as below:
    1. normal upgrade:
        python3 cit_runner.py
            --run-test "tests.cloudripper.external_dual_flash_test_individual"
            --external --bmc-host <bmc host ip address>
    """

    def setUp(self):
        self.bmc_reconnect_timeout = self.DEFAULT_BMC_RECONNECT_TIMEOUT
        self.command_exec_delay = self.DEFAULT_COMMAND_EXEC_DELAY
        self.command_promtp_timeout = self.DEFAULT_COMMAND_PROMTP_TIME_OUT
        self.power_reset_cmd = self.DEFAULT_POWER_RESET_CMD
        self.command_get_boot_info = self.DEFAULT_GET_BOOT_INFO_CMD
        self.command_switch_slave_boot_info = self.DEFAULT_SWITCH_SLAVE_CMD
        self.scm_boot_time = self.DEFAULT_SCM_BOOT_TIME
        self.boot_slave_keyword = self.BOOT_SLAVE_KEYWORD
        self.boot_master_keyword = self.BOOT_MASTER_KEYWORD
        self.set_ssh_session_bmc_hostname()

    def delete_local_ssh_key(self):
        os.system(
            "ssh-keygen -f ~/.ssh/known_hosts -R {} 2>/dev/null 1>/dev/null".format(
                self.bmc_hostname
            )
        )

    def do_external_BMC_dual_flash(self):
        """
        Main function to perform collective and individual firmware upgrade
        for components according to entity in json file
        To perform individual firmware upgrading, please pass the "component"
        parameter is the name of component. It's supposed to match the entity
        name in json file. e.g. bios, scm, ... etc.
        """

        print("Start dual flash test!")
        self.delete_local_ssh_key()
        self.connect_to_remote_host(logging=G_VERBOSE)
        # Test connection
        if not self.is_connection_ready():
            print("ok")
            if not self.reconnect_to_remote_host(
                self.bmc_reconnect_timeout, logging=G_VERBOSE
            ):
                self.fail("Cannot establish the connection to UUT!")

        # Reset BMC
        self.send_command_to_UUT(self.power_reset_cmd)
        time.sleep(10)  # delay for the current process to be done
        self.delete_local_ssh_key()
        self.reconnect_to_remote_host(self.bmc_reconnect_timeout, logging=G_VERBOSE)
        print("Reset BMC ... ok")

        # Check boot info  master
        self.send_command_to_UUT(self.command_get_boot_info)
        current_ver = self.receive_command_output_from_UUT(only_last=False)
        for boot_key_string in self.boot_master_keyword:
            if boot_key_string not in current_ver:
                self.fail(
                    "BMC boot is not master or WDT Count is fail {}".format(current_ver)
                )
                return -1
        print("Check master boot info ... ok")

        # Switch slave
        self.send_command_to_UUT(self.command_switch_slave_boot_info)
        time.sleep(10)  # delay for the current process to be done
        self.delete_local_ssh_key()
        self.reconnect_to_remote_host(self.bmc_reconnect_timeout, logging=G_VERBOSE)
        print("Switch slave ... ok")

        # Check boot info  slave
        self.send_command_to_UUT(self.command_get_boot_info)
        current_ver = self.receive_command_output_from_UUT(only_last=False)
        for boot_key_string in self.boot_slave_keyword:
            if boot_key_string not in current_ver:
                self.fail(
                    "BMC boot is not slave or WDT Count is fail {}".format(current_ver)
                )
                return -1
        print("Check slave boot info ... ok")

        # Reset BMC
        self.send_command_to_UUT(self.power_reset_cmd)
        time.sleep(10)  # delay for the current process to be done
        self.delete_local_ssh_key()
        self.reconnect_to_remote_host(self.bmc_reconnect_timeout, logging=G_VERBOSE)
        print("Reset BMC ... ok")

        # Check boot info  master
        self.send_command_to_UUT(self.command_get_boot_info)
        current_ver = self.receive_command_output_from_UUT(only_last=False)
        for boot_key_string in self.boot_master_keyword:
            if boot_key_string not in current_ver:
                self.fail(
                    "BMC boot is not master or WDT Count is fail {}".format(current_ver)
                )
                return -1
        print("Check master boot info ... ok")
