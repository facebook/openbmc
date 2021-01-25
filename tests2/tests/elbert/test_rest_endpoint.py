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
from tests.elbert.test_data.sensors.sensors import (
    SCM_SENSORS,
    SMB_SENSORS,
    PIM_16Q_SENSORS,
    PIM_8DDM_SENSORS,
    FAN_SENSORS,
    PSU_SENSORS,
)
from utils.shell_util import run_shell_cmd


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    FRUID_SCM_ENDPOINT = "/api/sys/fruid_scm"
    FRUID_SCM_ENDPOINT = "/api/sys/mb/seutil"
    PIM_PRESENT_ENDPOINT = "/api/sys/pim_present"
    PIM_INFO_ENDPOINT = "/api/sys/piminfo"
    PIM_SERIAL_ENDPOINT = "/api/sys/pimserial"
    PIM_STATUS_ENDPOINT = "/api/sys/pimstatus"
    SMB_INFO_ENDPOINT = "/api/sys/smbinfo"


    # "/api/sys"
    def test_endpoint_api_sys(self):
        self.endpoint_sys_attrb = [
            "fruid_scm",
            "pim_present",
            "mb",
            "piminfo",
            "pimserial",
        ]

    # "/api/sys/mb"
    def test_endpoint_api_sys_mb(self):
        pass

    def set_endpoint_firmware_info_all_attributes(self):
        self.endpoint_firmware_info_all_attrb = [
            "FAN",
            "PIM2",
            "PIM3",
            "PIM4",
            "PIM5",
            "PIM6",
            "PIM7",
            "PIM8",
            "PIM9",
            "SCM",
            "SMB",
            "SMB_CPLD",
        ]

    # "/api/sys/sensors"
    def test_endpoint_api_sys_sensors(self):
        cmd = "/usr/local/bin/pim_types.sh"
        pim_types = run_shell_cmd(cmd)
        self.endpoint_sensors_attrb = []
        for idx in range(2, 10):
            if "PIM {}: PIM16Q".format(idx) in pim_types:
                for sensor in PIM_16Q_SENSORS:
                    self.endpoint_sensors_attrb.append(sensor.format(idx))
            elif "PIM {}: PIM8DDM".format(idx) in pim_types:
                for sensor in PIM_8DDM_SENSORS:
                    self.endpoint_sensors_attrb.append(sensor.format(idx))

        for idx in range(1, 5):
            cmd = "head -n 1 /sys/bus/i2c/devices/4-0023/psu{}_present"
            if "0x1" in run_shell_cmd(cmd.format(idx)):
                for sensor in PSU_SENSORS:
                    self.endpoint_sensors_attrb.append(sensor.format(idx))
        self.endpoint_sensors_attrb.extend(SCM_SENSORS)
        self.endpoint_sensors_attrb.extend(SMB_SENSORS)
        self.endpoint_sensors_attrb.extend(FAN_SENSORS)

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        platform_specific = [
            "pim2",
            "pim3",
            "pim4",
            "pim5",
            "pim6",
            "pim7",
            "pim8",
            "pim9",
        ]
        self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES + platform_specific

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["BIC_ok", "status"]

    # "/api/sys/slotid"
    def set_endpoint_slotid_attributes(self):
        self.endpoint_slotid_attrb = ["1"]

    # "/api/sys/pim_present"
    def set_endpoint_pim_presence_attributes(self):
        self.endpoint_pim_presence = [
            "pim2",
            "pim3",
            "pim4",
            "pim5",
            "pim6",
            "pim7",
            "pim8",
            "pim9",
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
            "PIM2",
            "PIM3",
            "PIM4",
            "PIM5",
            "PIM6",
            "PIM7",
            "PIM8",
            "PIM9",
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
