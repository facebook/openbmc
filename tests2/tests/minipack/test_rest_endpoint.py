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
import unittest

from common.base_rest_endpoint_test import FbossRestEndpointTest
from tests.minipack.test_data.sensors.sensors import SENSORS


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    FRUID_SCM_ENDPOINT = "/api/sys/fruid_scm"
    PIM_PRESENT_ENDPOINT = "/api/sys/pim_present"
    MB_SEUTIL_ENDPOINT = "/api/sys/mb/seutil"
    PIM_INFO_ENDPOINT = "/api/sys/piminfo"
    PIM_SERIAL_ENDPOINT = "/api/sys/pimserial"
    PIM_FIRMWARE_INFO = "/api/sys/firmware_info_pim"
    SCM_FIRMWARE_INFO = "/api/sys/firmware_info_scm"
    ALL_FIRMWARE_INFO = (
        "/api/sys/firmware_info_all"  # duplicate endpoint to firmware_info/all
    )
    SYSTEM_LED_INFO = "/api/sys/system_led_info"

    # /api/sys
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = [
            "firmware_info",
            "fc_present",
            "fruid_scm",
            "pim_present",
            "psu_update",
            "bmc",
            "sensors",
            "mb",
            "slotid",
            "mTerm_status",
            "piminfo",
            "usb2i2c_reset",
            "pimserial",
            "server",
            "gpios",
            "system_led_info",
        ]

    # /api/sys/sensors
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = SENSORS

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        platform_specific = [
            "pim1",
            "pim2",
            "pim3",
            "pim4",
            "pim5",
            "pim6",
            "pim7",
            "pim8",
        ]
        self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES + platform_specific

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["BIC_ok", "status"]

    # "/api/sys/slotid"
    def set_endpoint_slotid_attributes(self):
        self.endpoint_slotid_attrb = ["1"]

    # "/api/sys/fruid_scm"
    def set_endpoint_fruid_scm_attributes(self):
        self.endpoint_fruid_scm_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_fruid_scm(self):
        self.set_endpoint_fruid_scm_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_SCM_ENDPOINT, self.endpoint_fruid_scm_attrb
        )

    # "/api/sys/pim_present"
    def set_endpoint_pim_presence_attributes(self):
        self.endpoint_pim_presence = [
            "pim1",
            "pim2",
            "pim3",
            "pim4",
            "pim5",
            "pim6",
            "pim7",
            "pim8",
        ]

    def test_endpoint_api_sys_pim_present(self):
        self.set_endpoint_pim_presence_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.PIM_PRESENT_ENDPOINT, self.endpoint_pim_presence
        )

    # "/api/sys/piminfo"
    def set_endpoint_piminfo_attributes(self):
        self.endpoint_piminfo_attrb = [
            "PIM1",
            "PIM2",
            "PIM3",
            "PIM4",
            "PIM5",
            "PIM6",
            "PIM7",
            "PIM8",
        ]

    def test_endpoint_api_sys_piminfo(self):
        self.set_endpoint_piminfo_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.PIM_INFO_ENDPOINT, self.endpoint_piminfo_attrb
        )

    # "/api/sys/pimserial"
    def test_endpoint_api_sys_pimserial(self):
        self.set_endpoint_piminfo_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.PIM_SERIAL_ENDPOINT,
            self.endpoint_piminfo_attrb,  # same keys as piminfo
        )

    # "/api/sys/mb/seutil"
    def set_endpoint_seutil_fruid_attributes(self):
        self.endpoint_seutil_fruid_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_mb_seutil_fruid(self):
        self.set_endpoint_seutil_fruid_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.MB_SEUTIL_ENDPOINT, self.endpoint_seutil_fruid_attrb
        )

    def set_endpoint_firmware_info_all_attributes(self):
        self.endpoint_firmware_info_all_attrb = [
            "PIM1_TYPE",
            "Right PDBCPLD",
            "PIM2_TYPE",
            "PIM5_TYPE",
            "PIM7",
            "PIM6",
            "PIM5",
            "PIM4",
            "PIM3",
            "PIM2",
            "PIM1",
            "PIM4_TYPE",
            "Bottom FCMCPLD",
            "PIM6_TYPE",
            "PIM8",
            "PIM3_TYPE",
            "SCMCPLD",
            "Left PDBCPLD",
            "PIM7_TYPE",
            "Top FCMCPLD",
            "SMBCPLD",
            "PIM8_TYPE",
        ]

    # "/api/sys/firmware_info_pim"
    def set_endpoint_firmware_info_pim_attributes(self):
        # pim attribute <-> version map.
        self.endpoint_firmware_info_pim_attributes = [
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
        ]

    def test_endpoint_api_sys_firmware_info_pim(self):
        self.set_endpoint_firmware_info_pim_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.PIM_FIRMWARE_INFO,
            self.endpoint_firmware_info_pim_attributes,
        )

    # "/api/sys/firmware_info_scm"
    def set_endpoint_firmware_info_scm_attributes(self):
        # scm attribute <-> version map.
        self.endpoint_firmware_info_scm_attributes = ["1"]

    def test_endpoint_api_sys_firmware_info_scm(self):
        self.set_endpoint_firmware_info_scm_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.SCM_FIRMWARE_INFO,
            self.endpoint_firmware_info_scm_attributes,
        )

    # "/api/sys/firmware_info_all"
    def test_endpoint_api_sys_firmware_all(self):
        self.set_endpoint_firmware_info_all_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.ALL_FIRMWARE_INFO, self.endpoint_firmware_info_all_attrb
        )

    # "/api/sys/system_led_info"
    def set_endpoint_system_led_info_attributes(self):
        self.endpoint_system_led_info_attrb = ["sys", "smb", "psu", "fan"]

    # "/api/sys/system_led_info"
    def test_endpoint_api_sys_system_led_info(self):
        self.set_endpoint_system_led_info_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.SYSTEM_LED_INFO, self.endpoint_system_led_info_attrb
        )
