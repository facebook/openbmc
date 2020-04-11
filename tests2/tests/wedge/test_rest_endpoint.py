#!/usr/bin/env python3
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

from common.base_rest_endpoint_test import FbossRestEndpointTest
from utils.shell_util import run_shell_cmd


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    PSU1_I2C_DEVICE = "pfe1100-i2c-7-59"
    PSU2_I2C_DEVICE = "pfe1100-i2c-7-5a"
    PSU_I2C_CMD = ["sensors | grep pfe"]

    # /api/sys
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = [
            "psu_update",
            "gpios",
            "mb",
            "fc_present",
            "modbus_registers",
            "server",
            "slotid",
            "mTerm_status",
            "sensors",
            "bmc",
        ]

    # /api/sys/sensors
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = [
            "ast_pwm-isa-0000",
            "ncp4200-i2c-1-60",
            "ncp4200-i2c-2-60",
            "tmp75-i2c-3-48",
            "tmp75-i2c-3-49",
            "tmp75-i2c-3-4a",
            "fb_panther_plus-i2c-4-40",
            "max127-i2c-6-28",
            "ncp4200-i2c-8-60",
            "ast_adc-isa-0000",
            "adm1278-i2c-12-10",
        ]

        psu_i2c_devices = run_shell_cmd(cmd=RestEndpointTest.PSU_I2C_CMD)
        if RestEndpointTest.PSU1_I2C_DEVICE in psu_i2c_devices:
            self.endpoint_sensors_attrb.append(RestEndpointTest.PSU1_I2C_DEVICE)
        if RestEndpointTest.PSU2_I2C_DEVICE in psu_i2c_devices:
            self.endpoint_sensors_attrb.append(RestEndpointTest.PSU2_I2C_DEVICE)

    # "/api/sys/mb"
    def set_endpoint_mb_attributes(self):
        self.endpoint_mb_attrb = self.MB_ATTRIBUTES

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["BIC_ok", "status"]

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES

    # "/api/sys/bmc"
    def set_endpoint_bmc_attributes(self):
        self.endpoint_bmc_attrb = self.BMC_ATTRIBUTES

    # "/api/sys/slotid"
    def set_endpoint_slotid_attributes(self):
        self.endpoint_slotid_attrb = ["16"]

    # "/api/sys/firmware_info_all
    @unittest.skip("Test not supported on platform")
    def set_endpoint_firmware_info_all_attributes(self):
        self.endpoint_firmware_info_all_attrb = None
        pass
