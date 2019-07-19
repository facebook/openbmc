#!/usr/bin/env python
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
from tests.cmm.test_data.sensors.sensors import SENSORS


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    # /api/sys
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = [
            "psu_update",
            "gpios",
            "component_presence",
            "mb",
            "fc_present",
            "firmware_info",
            "slotid",
            "mTerm_status",
            "sensors",
            "bmc",
        ]

    # /api/sys/sensors
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = SENSORS

    # "/api/sys/mb"
    def set_endpoint_mb_attributes(self):
        self.endpoint_mb_attrb = [
            "fruid",
            "chassis_eeprom",
            "chassis_all_serial_and_location",
        ]

    # "/api/sys/server"
    @unittest.skip("Test not supported on platform")
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = None
        pass

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES

    # "/api/sys/bmc"
    def set_endpoint_bmc_attributes(self):
        self.endpoint_bmc_attrb = self.BMC_ATTRIBUTES

    # "/api/sys/firmware_info_all
    @unittest.skip("Test not supported on platform")
    def set_endpoint_firmware_info_all_attributes(self):
        self.endpoint_firmware_info_all_attrb = None
        pass
