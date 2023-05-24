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

#######################################################################
# This test will check the Device Config vs Meta Recommended Config   #
#######################################################################


import unittest

from common.base_deployment_config_test import DeploymentConfig
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import fscd_config_dir, qemu_check

CONFIG_DIR = "{}/wedge400/test_data/config/".format(fscd_config_dir())


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class DeploymentConfigTest(DeploymentConfig, unittest.TestCase):
    def get_device_config(self):
        return run_shell_cmd("presence_util.sh").strip()

        """
        Currently, Wedge400/Wedge400C supports two configurations.
        Based on the PSU type we are differentiating the configs
        """

    def get_desired_device_configs(self):
        with open("/tmp/cache_store/power_type", "r") as psutype:
            ptype = psutype.read().strip()

        if ptype == "PEM":
            platform_config_files = [
                "wedge400_dc_pem_slot1",
                "wedge400_dc_pem_slot2",
            ]
        elif ptype == "PSU48":
            platform_config_files = [
                "wedge400_psu48_slot1",
                "wedge400_psu48_slot2",
            ]
        elif ptype == "PSU":
            platform_config_files = ["wedge400_psu"]
        else:
            Logger.error("PSU/PEM Eeprom is not programmed !!! ")

        desired_configs = []
        for filename in platform_config_files:
            full_path = CONFIG_DIR + filename
            with open(full_path, "r") as f:
                desired_configs.append(f.read().strip())
        return desired_configs
