#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

from tests.montblanc.test_data.sensors.sensor import SENSORS_PECI
from utils.test_utils import qemu_check


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    # "/api/sys"
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = [
            "server",
            "bmc",
            "mb",
            "gpios",
        ]

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    # "/api/sys"
    def test_endpoint_api_sys(self):
        self.set_endpoint_sys_attributes()
        self.verify_endpoint_attributes(
            FbossRestEndpointTest.SYS_ENDPOINT, self.endpoint_sys_attrb
        )

    # "/api/sys/sensors"
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = SENSORS_PECI

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["BIC_ok", "status"]

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        endpoint_info = self.get_from_endpoint(self.FRUID_ENDPOINT)
        version = int(
            endpoint_info.split("Version")[1].split(":")[1].split(",")[0].split('"')[1]
        )
        if version == 4:
            self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES_V4
        else:
            self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES

    @unittest.skip("not available")
    # "/api/sys/firmware_info/all"
    def test_endpoint_api_sys_firmware_info_all(self):
        pass
