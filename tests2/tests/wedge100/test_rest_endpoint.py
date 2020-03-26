#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
import json
import unittest

from common.base_rest_endpoint_test import FbossRestEndpointTest


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    SYS_FW_INFO_ENDPOINT = "/api/sys/firmware_info/sys"
    FAN_FW_INFO_ENDPOINT = "/api/sys/firmware_info/fan"
    # /api
    def set_endpoint_api_attributes(self):
        self.endpoint_api_attrb = ["sys"]

    # /api/sys
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = [
            "psu_update",
            "gpios",
            "i2cflush",
            "mb",
            "fc_present",
            "modbus_registers",
            "server",
            "slotid",
            "mTerm_status",
            "sensors",
            "bmc",
            "firmware_info",
        ]

    # /api/sys/sensors
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = [
            "tmp75-i2c-3-48",
            "tmp75-i2c-3-49",
            "tmp75-i2c-3-4a",
            "tmp75-i2c-3-4b",
            "tmp75-i2c-3-4c",
            "fancpld-i2c-8-33",
            "com_e_driver-i2c-4-33",
            "ltc4151-i2c-7-6f",
            "tmp75-i2c-8-48",
            "tmp75-i2c-8-49",
        ]

    # "/api/sys/mb"
    def set_endpoint_mb_attributes(self):
        self.endpoint_mb_attrb = ["fruid"]

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["BIC_ok", "status"]

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        self.endpoint_fruid_attrb = [
            "Assembled At",
            "Local MAC",
            "Product Asset Tag",
            "Product Name",
        ]

    # "/api/sys/mTerm_status"
    def set_endpoint_mterm_attributes(self):
        self.endpoint_mterm_attrb = ["Running"]

    # "/api/sys/bmc"
    def set_endpoint_bmc_attributes(self):
        self.endpoint_bmc_attrb = [
            "At-Scale-Debug Running",
            "CPU Usage",
            "Description",
            "MAC Addr",
            "Memory Usage",
            "OpenBMC Version",
            "SPI0 Vendor",
            "SPI1 Vendor",
            "TPM FW version",
            "TPM TCG version",
            "Reset Reason",
            "Uptime",
            "kernel version",
            "load-1",
            "load-15",
            "load-5",
            "open-fds",
            "u-boot version",
            "vboot",
        ]

    def set_endpoint_fc_present_attributes(self):
        self.endpoint_fc_present_attrb = ["Not Applicable"]

    # "/api/sys/slotid"
    def set_endpoint_slotid_attributes(self):
        self.endpoint_slotid_attrb = ["0"]

    # "/api/sys/firmware_info"
    def set_endpoint_firmware_info_attributes(self):
        self.endpoint_firmware_info_attrb = ["all", "sys", "fan"]

    def set_endpoint_firmware_info_all_attributes(self):
        # TODO: what if this changes?
        self.endpoint_firmware_info_all_attrb = ["6.101", "1.11"]

    def set_endpoint_firmware_info_sys_attributes(self):
        # TODO: what if this changes?
        self.endpoint_firmware_info_sys_attrb = ["6.101"]

    def set_endpoint_firmware_info_fan_attributes(self):
        # TODO: what if this changes?
        self.endpoint_firmware_info_fan_attrb = ["1.11"]

    def test_endpoint_api_sys_firmware_info_fan(self):
        self.set_endpoint_firmware_info_fan_attributes()
        self.assertNotEqual(self.endpoint_firmware_info_fan_attrb, None)
        info = self.get_from_endpoint(RestEndpointTest.SYS_FW_INFO_ENDPOINT)
        # If we get any JSON object I'm happy for the time being
        self.assertTrue(json.loads(info))

    def test_endpoint_api_sys_firmware_info_sys(self):
        self.set_endpoint_firmware_info_sys_attributes()
        self.assertNotEqual(self.endpoint_firmware_info_sys_attrb, None)
        info = self.get_from_endpoint(RestEndpointTest.FAN_FW_INFO_ENDPOINT)
        # If we get any JSON object I'm happy for the time being
        self.assertTrue(json.loads(info))
