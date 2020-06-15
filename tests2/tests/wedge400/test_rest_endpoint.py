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
import json
import time
import unittest

from common.base_rest_endpoint_test import FbossRestEndpointTest
from tests.wedge400.helper.libpal import pal_get_board_type, pal_get_board_type_rev
from tests.wedge400.test_data.sensors.sensors import (
    PEM1_SENSORS,
    PEM2_SENSORS,
    PSU1_SENSORS,
    PSU2_SENSORS,
    SCM_SENSORS,
    SMB_SENSORS_W400,
    SMB_SENSORS_W400CEVT,
    SMB_SENSORS_W400CEVT2,
)
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    VDDCORE_ENDPOINT = "/api/sys/vddcore"
    SWITCHRESET_ENDPOINT = "/api/sys/switch_reset"
    FRUID_SCM_ENDPOINT = "/api/sys/seutil"
    FRUID_SMB_ENDPOINT = "/api/sys/mb/fruid"
    FRUID_FCM_ENDPOINT = "/api/sys/feutil/fcm"
    FRUID_FAN1_ENDPOINT = "/api/sys/feutil/fan1"
    FRUID_FAN2_ENDPOINT = "/api/sys/feutil/fan2"
    FRUID_FAN3_ENDPOINT = "/api/sys/feutil/fan3"
    FRUID_FAN4_ENDPOINT = "/api/sys/feutil/fan4"
    SCM_PRESENT_ENDPOINT = "/api/sys/presence/scm"
    PSU_PRESENT_ENDPOINT = "/api/sys/presence/psu"
    FAN_PRESENT_ENDPOINT = "/api/sys/presence/fan"
    CPLD_FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/cpld"
    FPGA_FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/fpga"
    SCM_FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/scm"
    ALL_FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/all"

    # "/api/sys"
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = [
            "server",
            "bmc",
            "mb",
            "slotid",
            "firmware_info",
            "presence",
            "feutil",
            "seutil",
            "psu_update",
            "gpios",
            "sensors",
            "mTerm_status",
        ]
        platform_type = pal_get_board_type()
        # Wedge400C need to add vddcore and switch_reset end point
        if platform_type == "Wedge400C":
            self.endpoint_sys_attrb.append("vddcore")
            self.endpoint_sys_attrb.append("switch_reset")

    def test_endpoint_api_sys(self):
        self.set_endpoint_sys_attributes()
        self.verify_endpoint_attributes(
            FbossRestEndpointTest.SYS_ENDPOINT, self.endpoint_sys_attrb
        )

    def check_fru_sensors_present(self, fru):
        sensors = run_shell_cmd("sensor-util {}".format(fru))
        if "not present" in sensors:
            return False
        else:
            return True

    # "/api/sys/sensors"
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = []
        platform_type_rev = pal_get_board_type_rev()
        if (
            platform_type_rev == "Wedge400-EVT/EVT3"
            or platform_type_rev == "Wedge400-DVT"
            or platform_type_rev == "Wedge400-DVT2/PVT/PVT2"
            or platform_type_rev == "Wedge400-PVT3"
            or platform_type_rev == "Wedge400-MP"
        ):
            self.endpoint_sensors_attrb += SMB_SENSORS_W400
        elif platform_type_rev == "Wedge400C-EVT":
            self.endpoint_sensors_attrb += SMB_SENSORS_W400CEVT
        elif (
            platform_type_rev == "Wedge400C-EVT2"
            or platform_type_rev == "Wedge400C-DVT"
        ):
            self.endpoint_sensors_attrb += SMB_SENSORS_W400CEVT2
        else:
            self.skipTest("Skip test on {} board".format(platform_type_rev))

        self.endpoint_sensors_attrb += SCM_SENSORS
        if self.check_fru_sensors_present("pem1"):
            self.endpoint_sensors_attrb += PEM1_SENSORS
        if self.check_fru_sensors_present("pem2"):
            self.endpoint_sensors_attrb += PEM2_SENSORS
        if self.check_fru_sensors_present("psu1"):
            self.endpoint_sensors_attrb += PSU1_SENSORS
        if self.check_fru_sensors_present("psu2"):
            self.endpoint_sensors_attrb += PSU2_SENSORS

    def test_endpoint_api_sys_sensors(self):
        self.set_endpoint_sensors_attributes()
        self.verify_endpoint_attributes(
            self.SENSORS_ENDPOINT, self.endpoint_sensors_attrb
        )

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        self.endpoint_fruid_attrb = self.FRUID_ATTRIBUTES

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["BIC_ok", "status"]

    # "/api/sys/slotid"
    def set_endpoint_slotid_attributes(self):
        self.endpoint_slotid_attrb = ["1"]

    # "/api/sys/seutil"
    def set_endpoint_fruid_scm_attributes(self):
        self.endpoint_fruid_scm_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_fruid_scm(self):
        self.set_endpoint_fruid_scm_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_SCM_ENDPOINT, self.endpoint_fruid_scm_attrb
        )

    # "/api/sys/mb/fruid"
    def set_endpoint_mb_fruid_attributes(self):
        self.endpoint_mb_fruid_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_seutil_fruid(self):
        self.set_endpoint_mb_fruid_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_SMB_ENDPOINT, self.endpoint_mb_fruid_attrb
        )

    # "/api/sys/feutil/fcm"
    def set_endpoint_fruid_fcm_attributes(self):
        self.endpoint_fruid_fcm_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_fruid_fcm(self):
        self.set_endpoint_fruid_fcm_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FCM_ENDPOINT, self.endpoint_fruid_fcm_attrb
        )

    # "/api/sys/feutil/fan1"
    def set_endpoint_fruid_fan1_attributes(self):
        self.endpoint_fruid_fan1_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_fruid_fan1(self):
        self.set_endpoint_fruid_fan1_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN1_ENDPOINT, self.endpoint_fruid_fan1_attrb
        )

    # "/api/sys/feutil/fan2"
    def set_endpoint_fruid_fan2_attributes(self):
        self.endpoint_fruid_fan2_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_fruid_fan2(self):
        self.set_endpoint_fruid_fan2_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN2_ENDPOINT, self.endpoint_fruid_fan2_attrb
        )

    # "/api/sys/feutil/fan3"
    def set_endpoint_fruid_fan3_attributes(self):
        self.endpoint_fruid_fan3_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_fruid_fan3(self):
        self.set_endpoint_fruid_fan3_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN3_ENDPOINT, self.endpoint_fruid_fan3_attrb
        )

    # "/api/sys/feutil/fan4"
    def set_endpoint_fruid_fan4_attributes(self):
        self.endpoint_fruid_fan4_attrb = self.FRUID_ATTRIBUTES

    def test_endpoint_api_sys_fruid_fan4(self):
        self.set_endpoint_fruid_fan4_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN4_ENDPOINT, self.endpoint_fruid_fan4_attrb
        )

    # "/api/sys/presence/scm"
    def set_endpoint_scm_presence_attributes(self):
        self.endpoint_scm_presence = ["scm"]

    def test_endpoint_api_scm_present(self):
        self.set_endpoint_scm_presence_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.SCM_PRESENT_ENDPOINT, self.endpoint_scm_presence
        )

    # "/api/sys/presence/psu"
    def set_endpoint_psu_presence_attributes(self):
        self.endpoint_psu_presence = ["psu1", "psu2"]

    def test_endpoint_api_psu_present(self):
        self.set_endpoint_psu_presence_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.PSU_PRESENT_ENDPOINT, self.endpoint_psu_presence
        )

    # "/api/sys/presence/fan"
    def set_endpoint_fan_presence_attributes(self):
        self.endpoint_fan_presence = ["fan1", "fan2", "fan3", "fan4"]

    def test_endpoint_api_fan_present(self):
        self.set_endpoint_fan_presence_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FAN_PRESENT_ENDPOINT, self.endpoint_fan_presence
        )

    # "/api/sys/firmware_info/cpld"
    def set_endpoint_firmware_info_cpld_attributes(self):
        self.endpoint_firmware_info_cpld_attributes = [
            "FCMCPLD",
            "PWRCPLD",
            "SCMCPLD",
            "SMBCPLD",
        ]

    def test_endpoint_api_sys_firmware_info_cpld(self):
        self.set_endpoint_firmware_info_cpld_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.CPLD_FIRMWARE_INFO_ENDPOINT,
            self.endpoint_firmware_info_cpld_attributes,
        )

    # "/api/sys/firmware_info/fpga"
    def set_endpoint_firmware_info_fpga_attributes(self):
        self.endpoint_firmware_info_fpga_attributes = ["DOMFPGA1", "DOMFPGA2"]

    def test_endpoint_api_sys_firmware_info_fpga(self):
        self.set_endpoint_firmware_info_fpga_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FPGA_FIRMWARE_INFO_ENDPOINT,
            self.endpoint_firmware_info_fpga_attributes,
        )

    # "/api/sys/firmware_info/scm"
    def set_endpoint_firmware_info_scm_attributes(self):
        self.endpoint_firmware_info_scm_attributes = [
            "CPLD Version",
            "Bridge-IC Version",
            "ME Version",
            "Bridge-IC Bootloader Version",
            "PVCCIN VR Version",
            "DDRAB VR Version",
            "P1V05 VR Version",
            "BIOS Version",
        ]

    def test_endpoint_api_sys_firmware_info_scm(self):
        self.set_endpoint_firmware_info_scm_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.SCM_FIRMWARE_INFO_ENDPOINT,
            self.endpoint_firmware_info_scm_attributes,
        )

    # "/api/sys/firmware_info_all"
    def set_endpoint_firmware_info_all_attributes(self):
        self.endpoint_firmware_info_all_attributes = [
            "ALTBMC Version",
            "BMC Version",
            "Fan Speed Controller Version",
            "TPM Version",
            "FCMCPLD",
            "PWRCPLD",
            "SCMCPLD",
            "SMBCPLD",
            "DOMFPGA1",
            "DOMFPGA2",
            "Bridge-IC Version",
            "Bridge-IC Bootloader Version",
            "BIOS Version",
            "CPLD Version",
            "ME Version",
            "PVCCIN VR Version",
            "DDRAB VR Version",
            "P1V05 VR Version",
        ]

    def test_endpoint_api_sys_firmware_info_all(self):
        self.set_endpoint_firmware_info_all_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.ALL_FIRMWARE_INFO_ENDPOINT,
            self.endpoint_firmware_info_all_attributes,
        )

    def get_vddcore(self):
        # test accessing get vdd core
        info = json.loads(self.get_from_endpoint(RestEndpointTest.VDDCORE_ENDPOINT))
        if "Information" not in info:
            Logger.info("can't get information in {}".format(info))
            return None
        if "VDD_CORE" not in info["Information"]:
            Logger.info("not have VDD_CORE in {}".format(info))
            return None
        return int(info["Information"]["VDD_CORE"])

    def set_vddcore(self, target):
        info = json.loads(
            self.get_from_endpoint(
                RestEndpointTest.VDDCORE_ENDPOINT + "/{}".format(target)
            )
        )
        if "result" not in info:
            Logger.info("can't detect result on vddcore command {}".format(info))
            return False
        if info["result"] != "success":
            Logger.info("set vddcore not success {}".format(info))
            return False
        return True

    # "/api/sys/vddcore"
    def test_endpoint_api_sys_vddcore_get(self):
        platform_type = pal_get_board_type()
        # Wedge400 need to skip vddcore
        if platform_type != "Wedge400C":
            self.skipTest("Skip vddcore test on {} platform".format(platform_type))

        # test accessing get vdd core
        volt = self.get_vddcore()
        if volt is None:
            self.fail("get vddcore failed")

    # "/api/sys/vddcore/{#vddcore_value}"
    def test_endpoint_api_sys_vddcore_post(self):
        platform_type = pal_get_board_type()
        # Wedge400 need to skip vddcore
        if platform_type != "Wedge400C":
            self.skipTest("Skip vddcore test on {} platform".format(platform_type))

        # save default value
        default_volt = self.get_vddcore()
        if default_volt is None:
            self.fail("can't get default value")

        # test setting vdd core to target value
        target_volt = 750
        if not self.set_vddcore(target_volt):
            self.fail("failed to set vddcore to target value {}".format(target_volt))

        # test get back vddcore to check value is changed to target value
        time.sleep(3)
        current_volt = self.get_vddcore()
        if abs(current_volt - target_volt) > 10:
            self.fail(
                "VDD_CORE not changed target:{}, current:{}".format(
                    target_volt, current_volt
                )
            )

        # set vddcore back to default value
        if not self.set_vddcore(default_volt):
            self.fail(
                "failed to set vddcore back to default value {}".format(default_volt)
            )

    # "/api/sys/switch_reset"
    def set_endpoint_api_sys_switch_reset_attributes(self):
        self.endpoint_switch_reset_attributes = ["cycle_reset", "only_reset"]

    def test_endpoint_api_sys_switch_reset(self):
        platform_type = pal_get_board_type()
        # Wedge400 need to skip vddcore
        if platform_type != "Wedge400C":
            self.skipTest("Skip vddcore test on Wedge400 platform")

        self.set_endpoint_api_sys_switch_reset_attributes()

        self.verify_endpoint_attributes(
            RestEndpointTest.SWITCHRESET_ENDPOINT,
            self.endpoint_switch_reset_attributes,
        )
