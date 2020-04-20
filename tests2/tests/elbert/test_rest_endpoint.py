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
import unittest

from common.base_rest_endpoint_test import FbossRestEndpointTest


# ELBERTTODO REST API Support, CHASSIS/LC/SC/AST2620
class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    FRUID_SCM_ENDPOINT = "/api/sys/seutil"

    # "/api/sys"
    def test_endpoint_api_sys(self):
        pass

    # "/api/sys/mb"
    def test_endpoint_api_sys_mb(self):
        pass

    @unittest.skip("ELBERTTODO: Test not supported yet")
    def test_endpoint_api_sys_mb_fruid(self):
        pass

    @unittest.skip("ELBERTTODO: Test not supported yet")
    def test_endpoint_api_sys_firmware_info_all(self):
        pass

    # "/api/sys/sensors"
    def test_endpoint_api_sys_sensors(self):
        pass

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["BIC_ok", "status"]

    # "/api/sys/slotid"
    def set_endpoint_slotid_attributes(self):
        self.endpoint_slotid_attrb = ["1"]
