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
from tests.yamp.test_data.sensors.sensors import (
    FAN_SENSORS,
    PIM_PMBUS_SENSORS,
    PIM_MAX_SENSORS,
    SUP_TEMP_SENSORS,
    PIM_VOLT_SENSORS,
    TH3_SENSORS,
)


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    PIM_PRESENT_ENDPOINT = "/api/sys/pim_present"
    PIM_INFO_ENDPOINT = "/api/sys/piminfo"
    PIM_SERIAL_ENDPOINT = "/api/sys/pimserial"
    PIM_STATUS_ENDPOINT = "/api/sys/pimstatus"

    # "/api/sys"
    def test_endpoint_api_sys(self):
        pass

    # "/api/sys/mb"
    def set_endpoint_mb_attributes(self):
        self.endpoint_mb_attrb = [
            "pim1",
            "pim2",
            "pim3",
            "pim4",
            "pim5",
            "pim6",
            "pim7",
            "pim8",
            "fruid",
            "seutil",
        ]

    # /api/sys/sensors
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = []
        for idx in range(1, 9):
            for pmbus_sensor in PIM_PMBUS_SENSORS:
                self.endpoint_sensors_attrb.append(pmbus_sensor.format(idx))
            for max_sensor in PIM_MAX_SENSORS:
                self.endpoint_sensors_attrb.append(max_sensor.format(idx))
            for volt_sensor in PIM_VOLT_SENSORS:
                self.endpoint_sensors_attrb.append(volt_sensor.format(idx))
        self.endpoint_sensors_attrb.extend(FAN_SENSORS)
        self.endpoint_sensors_attrb.extend(SUP_TEMP_SENSORS)
        self.endpoint_sensors_attrb.extend(TH3_SENSORS)

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

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["BIC_ok", "status"]

    # "/api/sys/slotid"
    def set_endpoint_slotid_attributes(self):
        self.endpoint_slotid_attrb = ["1"]

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

    # "/api/sys/pim_present"
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

    # "/api/sys/piminfo"
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

    # "/api/sys/pimstatus"
    def test_endpoint_api_sys_pimstatus(self):
        self.set_endpoint_pim_presence_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.PIM_STATUS_ENDPOINT,
            self.endpoint_pim_presence,  # same keys as pim_present
        )
