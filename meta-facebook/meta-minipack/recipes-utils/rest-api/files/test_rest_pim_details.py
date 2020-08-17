#!/usr/bin/env python3
#
# Copyright 2014-present Facebook. All Rights Reserved.
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

import board_setup_routes
from aiohttp import test_utils, web
from common_middlewares import jsonerrorhandler


PIM_PRESENT = """Wedge EEPROM /sys/bus/i2c/devices/81-0056/eeprom:
Version: 3
Product Name: MINIPACK-PIM16Q
Product Part Number: 123123
System Assembly Part Number: 123123
Facebook PCBA Part Number: 123123
Facebook PCB Part Number: 123123
ODM PCBA Part Number: 123123
ODM PCBA Serial Number: 123123
Product Production State: 4
Product Version: 7
Product Sub-Version: 4
Product Serial Number: 123123
Product Asset Tag: 123123
System Manufacturer: test
System Manufacturing Date: 11-05-19
PCB Manufacturer: test
Assembled At: TEST
Local MAC: 00:00:00:00:00:00
Extended MAC Base: 00:00:00:00:00:00
Extended MAC Address Size: 0
Location on Fabric: MINIPACK-PIM16Q
CRC8: 0x80
"""

PIM_PRESENT_RESPONSE = {
    "status": "present",
    "version": "3",
    "product_name": "MINIPACK-PIM16Q",
    "product_part_number": "123123",
    "system_assembly_part_number": "123123",
    "facebook_pcba_part_number": "123123",
    "facebook_pcb_part_number": "123123",
    "odm_pcba_part_number": "123123",
    "odm_pcba_serial_number": "123123",
    "product_production_state": "4",
    "product_version": "7",
    "product_sub_version": "4",
    "product_serial_number": "123123",
    "product_asset_tag": "123123",
    "system_manufacturer": "test",
    "system_manufacturing_date": "11-05-19",
    "pcb_manufacturer": "test",
    "assembled_at": "TEST",
    "local_mac": "00",
    "extended_mac_base": "00",
    "extended_mac_address_size": "0",
    "location_on_fabric": "MINIPACK-PIM16Q",
    "crc8": "0x80",
}

PIM_MISSING = """PIM3 not inserted or could not detect eeprom
"""

PIM_MISSING_RESPONSE = {
    "status": "missing",
    "reason": "PIM3 not inserted or could not detect eeprom",
}


class TestPimDetails(test_utils.AioHTTPTestCase):
    def setUp(self):
        super().setUp()
        self.subprocess_mock = unittest.mock.Mock()
        patcher = unittest.mock.patch("subprocess.Popen", self.subprocess_mock)
        patcher.start()
        self.addCleanup(patcher.stop)

    async def get_application(self):
        """
        Override the get_app method to return your application.
        """
        self.route_to_test = "/api/sys/pimdetails"

        webapp = web.Application(middlewares=[jsonerrorhandler])
        board_setup_routes.setup_board_routes(webapp, False)
        return webapp

    @test_utils.unittest_run_loop
    async def test_rest_pimdetail_on_all_pim_present(self):
        self.maxDiff = None
        self.subprocess_mock.return_value.communicate.return_value = (
            str.encode(PIM_PRESENT),
            "",
        )
        request = await self.client.request("GET", self.route_to_test)
        assert request.status == 200
        resp = await request.json()
        expected_response = {}
        for idx in range(1, 9):
            expected_response[str(idx)] = PIM_PRESENT_RESPONSE
        self.assertEqual(expected_response, resp)

    @test_utils.unittest_run_loop
    async def test_rest_pimdetail_on_pims_missing(self):
        self.maxDiff = None
        self.subprocess_mock.return_value.communicate.return_value = (
            str.encode(PIM_MISSING),
            "",
        )
        request = await self.client.request("GET", self.route_to_test)
        assert request.status == 200
        resp = await request.json()
        expected_response = {}
        for idx in range(1, 9):
            expected_response[str(idx)] = PIM_MISSING_RESPONSE
        self.assertEqual(expected_response, resp)
