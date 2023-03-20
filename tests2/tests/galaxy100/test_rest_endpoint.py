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
import os
import unittest

from common.base_rest_endpoint_test import FbossRestEndpointTest
from utils.test_utils import qemu_check


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):

    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    FRUID_SCM_ENDPOINT = "/api/sys/mb/fruid_scm"
    CHASSIS_EEPROM_ENDPOINT = "/api/sys/mb/chassis_eeprom"
    SEUTIL_ENDPOINT = "/api/sys/mb/seutil"

    # /api
    def set_endpoint_api_attributes(self):
        self.endpoint_api_attrb = ["sys"]

    # /api/sys
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = [
            "mb",
            "fc_present",
            "gpios",
            "server",
            "psu_update",
            "sensors",
            "bmc",
            "i2cflush",
            "usb2i2c_reset",
            "firmware_info",
        ]

    # /api/sys/fc_present
    def set_endpoint_fc_present_attributes(self):
        self.endpoint_fc_present_attrb = ["Not Applicable"]

    # /api/sys/sensors
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = [
            "galaxy100_ec-i2c-0-33",
            "IR3581-i2c-1-70",
            "IR3584-i2c-1-72",
            "pwr1014a-i2c-2-40",
        ]
        if os.path.exists("/sys/devices/platform/ast-adc-hwmon/"):
            self.endpoint_sensors_attrb.append("ast_adc_hwmon-isa-0000")
        else:
            self.endpoint_sensors_attrb.append("ast_adc-isa-0000")

    # "/api/sys/mb"
    def set_endpoint_mb_attributes(self):
        self.endpoint_mb_attrb = ["fruid_scm", "fruid", "chassis_eeprom", "seutil"]

    # "/api/sys/mb/fruid_scm"
    def set_endpoint_fruid_scm_attributes(self):
        self.endpoint_fruid_scm_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_mb_fruid_scm(self):
        self.set_endpoint_fruid_scm_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_SCM_ENDPOINT, self.endpoint_fruid_scm_attrb
        )

    # "/api/sys/mb/chassis_eeprom"
    def set_endpoint_chassis_eeprom_attributes(self):
        self.endpoint_chassis_eeprom_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_mb_chassis_eeprom(self):
        self.set_endpoint_chassis_eeprom_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.CHASSIS_EEPROM_ENDPOINT, self.endpoint_chassis_eeprom_attrb
        )

    # "/api/sys/mb/seutil"
    def set_endpoint_seutil_attributes(self):
        self.endpoint_seutil_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_mb_seutil(self):
        self.set_endpoint_seutil_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.SEUTIL_ENDPOINT, self.endpoint_seutil_attrb
        )

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["status"]

    # "/api/sys/firmware_info"
    def set_endpoint_firmware_info_attributes(self):
        self.endpoint_firmware_info_attrb = ["QSFP_CPLD", "SCM_CPLD", "SYS_CPLD"]

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES

    # "/api/sys/bmc"
    def set_endpoint_bmc_attributes(self):
        self.endpoint_bmc_attrb = [
            "CPU Usage",
            "Description",
            "Memory Usage",
            "OpenBMC Version",
            "Reset Reason",
            "Uptime",
        ]

    @unittest.skip("Test not supported on platform")
    def set_endpoint_firmware_info_all_attributes(self):
        pass
