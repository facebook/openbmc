#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
from tests.fuji.helper.libpal import pal_is_fru_prsnt, pal_get_fru_id
from tests.fuji.test_data.sensors.sensor import (
    PIM1_SENSORS_16Q,
    PIM1_SENSORS_16O,
    PIM2_SENSORS_16Q,
    PIM2_SENSORS_16O,
    PIM3_SENSORS_16Q,
    PIM3_SENSORS_16O,
    PIM4_SENSORS_16Q,
    PIM4_SENSORS_16O,
    PIM5_SENSORS_16Q,
    PIM5_SENSORS_16O,
    PIM6_SENSORS_16Q,
    PIM6_SENSORS_16O,
    PIM7_SENSORS_16Q,
    PIM7_SENSORS_16O,
    PIM8_SENSORS_16Q,
    PIM8_SENSORS_16O,
    PSU1_SENSORS,
    PSU2_SENSORS,
    PSU3_SENSORS,
    PSU4_SENSORS,
    SCM_SENSORS,
    SMB_SENSORS,
)
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    FRUID_SCM_ENDPOINT = "/api/sys/seutil"
    FRUID_SMB_ENDPOINT = "/api/sys/mb/fruid"
    FRUID_FCM_TOP_ENDPOINT = "/api/sys/feutil/fcm_t"
    FRUID_FCM_BOTTOM_ENDPOINT = "/api/sys/feutil/fcm_b"
    FRUID_FAN1_ENDPOINT = "/api/sys/feutil/fan1"
    FRUID_FAN2_ENDPOINT = "/api/sys/feutil/fan2"
    FRUID_FAN3_ENDPOINT = "/api/sys/feutil/fan3"
    FRUID_FAN4_ENDPOINT = "/api/sys/feutil/fan4"
    FRUID_FAN5_ENDPOINT = "/api/sys/feutil/fan5"
    FRUID_FAN6_ENDPOINT = "/api/sys/feutil/fan6"
    FRUID_FAN7_ENDPOINT = "/api/sys/feutil/fan7"
    FRUID_FAN8_ENDPOINT = "/api/sys/feutil/fan8"
    FRUID_PIM1_ENDPOINT = "/api/sys/peutil/pim1"
    FRUID_PIM2_ENDPOINT = "/api/sys/peutil/pim2"
    FRUID_PIM3_ENDPOINT = "/api/sys/peutil/pim3"
    FRUID_PIM4_ENDPOINT = "/api/sys/peutil/pim4"
    FRUID_PIM5_ENDPOINT = "/api/sys/peutil/pim5"
    FRUID_PIM6_ENDPOINT = "/api/sys/peutil/pim6"
    FRUID_PIM7_ENDPOINT = "/api/sys/peutil/pim7"
    FRUID_PIM8_ENDPOINT = "/api/sys/peutil/pim8"
    SCM_PRESENT_ENDPOINT = "/api/sys/presence/scm"
    PSU_PRESENT_ENDPOINT = "/api/sys/presence/psu"
    FAN_PRESENT_ENDPOINT = "/api/sys/presence/fan"
    CPLD_FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/cpld"
    FPGA_FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/fpga"
    SCM_FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/scm"
    ALL_FIRMWARE_INFO_ENDPOINT = "/api/sys/firmware_info/all"
    PIM_INFO_ENDPOINT = "/api/sys/piminfo"
    SMB_INFO_ENDPOINT = "/api/sys/smbinfo"
    PIM_SERIAL_ENDPOINT = "/api/sys/pimserial"

    # "/api/sys"
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = [
            "server",
            "bmc",
            "mb",
            "firmware_info",
            "presence",
            "feutil",
            "seutil",
            "psu_update",
            "gpios",
            "sensors",
        ]

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
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

    def get_pim_name(self, ver):
        """ """
        pim_name = None
        ver = int(ver, 16)
        if ver & 0x80 == 0x0:
            pim_name = "PIM_TYPE_16Q"
        else:
            pim_name = "PIM_TYPE_16O"
        return pim_name

    def get_pim_sensor_type(self, pim_num):
        """
        Get PIM sensors type by read i2c device board version
        """
        pim_sensor_type = None
        bus = (pim_num * 8) + 80
        PATH = "/sys/bus/i2c/devices/%d-0060/board_ver" % (bus)

        if not os.path.exists(PATH):
            raise Exception("Path for PIM board_ver doesn't exist")
        with open(PATH, "r") as fp:
            line = fp.readline()
            if line:
                ver = line.split("0x")[1]
                pim_sensor_type = self.get_pim_name(ver)
            else:
                raise Exception("PIM board_ver is enpty")
        return pim_sensor_type

    # "/api/sys/sensors"
    def set_endpoint_sensors_attributes(self):
        psu_list = [1, 2, 3, 4]
        psu_sensor_list = {
            1: PSU1_SENSORS,
            2: PSU2_SENSORS,
            3: PSU3_SENSORS,
            4: PSU4_SENSORS,
        }
        pim_list = [0, 1, 2, 3, 4, 5, 6, 7]
        pim_16q_sensor_list = {
            0: PIM1_SENSORS_16Q,
            1: PIM2_SENSORS_16Q,
            2: PIM3_SENSORS_16Q,
            3: PIM4_SENSORS_16Q,
            4: PIM5_SENSORS_16Q,
            5: PIM6_SENSORS_16Q,
            6: PIM7_SENSORS_16Q,
            7: PIM8_SENSORS_16Q,
        }
        pim_16o_sensor_list = {
            0: PIM1_SENSORS_16O,
            1: PIM2_SENSORS_16O,
            2: PIM3_SENSORS_16O,
            3: PIM4_SENSORS_16O,
            4: PIM5_SENSORS_16O,
            5: PIM6_SENSORS_16O,
            6: PIM7_SENSORS_16O,
            7: PIM8_SENSORS_16O,
        }
        self.endpoint_sensors_attrb = SCM_SENSORS + SMB_SENSORS
        for psu in psu_list:
            if pal_is_fru_prsnt(pal_get_fru_id("psu{}".format(psu))):
                self.endpoint_sensors_attrb += psu_sensor_list[psu]
            else:
                Logger.info("/api/sys/sensors: get psu{} not present".format(psu))

        for pim in pim_list:
            name = self.get_pim_sensor_type(pim)
            if name == "PIM_TYPE_16Q":
                self.endpoint_sensors_attrb += pim_16q_sensor_list[pim]
            elif name == "PIM_TYPE_16O":
                self.endpoint_sensors_attrb += pim_16o_sensor_list[pim]
            else:
                Logger.info("/api/sys/sensors: get PIM{} sensors None".format(pim + 1))

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
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
        self.endpoint_slotid_attrb = ["0"]

    # "/api/sys/seutil"
    def set_endpoint_fruid_scm_attributes(self):
        self.endpoint_fruid_scm_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_scm(self):
        self.set_endpoint_fruid_scm_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_SCM_ENDPOINT, self.endpoint_fruid_scm_attrb
        )

    # "/api/sys/smbutil"
    def set_endpoint_fruid_smb_attributes(self):
        self.endpoint_fruid_smb_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_smb(self):
        self.set_endpoint_fruid_smb_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.SMB_INFO_ENDPOINT, self.endpoint_fruid_smb_attrb
        )

    # "/api/sys/mb/fruid"
    def set_endpoint_mb_fruid_attributes(self):
        self.endpoint_mb_fruid_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_seutil_fruid(self):
        self.set_endpoint_mb_fruid_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_SMB_ENDPOINT, self.endpoint_mb_fruid_attrb
        )

    # "/api/sys/feutil/fcm_t"
    def set_endpoint_fruid_fcm_top_attributes(self):
        self.endpoint_fruid_fcm_top_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fcm_top(self):
        self.set_endpoint_fruid_fcm_top_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FCM_TOP_ENDPOINT, self.endpoint_fruid_fcm_top_attrb
        )

    # "/api/sys/feutil/fcm_b"
    def set_endpoint_fruid_fcm_bottom_attributes(self):
        self.endpoint_fruid_fcm_bottom_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fcm_bottom(self):
        self.set_endpoint_fruid_fcm_bottom_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FCM_BOTTOM_ENDPOINT,
            self.endpoint_fruid_fcm_bottom_attrb,
        )

    # "/api/sys/feutil/fan1"
    def set_endpoint_fruid_fan1_attributes(self):
        self.endpoint_fruid_fan1_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fan1(self):
        self.set_endpoint_fruid_fan1_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN1_ENDPOINT, self.endpoint_fruid_fan1_attrb
        )

    # "/api/sys/feutil/fan2"
    def set_endpoint_fruid_fan2_attributes(self):
        self.endpoint_fruid_fan2_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fan2(self):
        self.set_endpoint_fruid_fan2_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN2_ENDPOINT, self.endpoint_fruid_fan2_attrb
        )

    # "/api/sys/feutil/fan3"
    def set_endpoint_fruid_fan3_attributes(self):
        self.endpoint_fruid_fan3_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fan3(self):
        self.set_endpoint_fruid_fan3_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN3_ENDPOINT, self.endpoint_fruid_fan3_attrb
        )

    # "/api/sys/feutil/fan4"
    def set_endpoint_fruid_fan4_attributes(self):
        self.endpoint_fruid_fan4_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fan4(self):
        self.set_endpoint_fruid_fan4_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN4_ENDPOINT, self.endpoint_fruid_fan4_attrb
        )

    # "/api/sys/feutil/fan5"
    def set_endpoint_fruid_fan5_attributes(self):
        self.endpoint_fruid_fan5_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fan5(self):
        self.set_endpoint_fruid_fan5_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN5_ENDPOINT, self.endpoint_fruid_fan5_attrb
        )

    # "/api/sys/feutil/fan6"
    def set_endpoint_fruid_fan6_attributes(self):
        self.endpoint_fruid_fan6_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fan6(self):
        self.set_endpoint_fruid_fan6_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN6_ENDPOINT, self.endpoint_fruid_fan6_attrb
        )

    # "/api/sys/feutil/fan7"
    def set_endpoint_fruid_fan7_attributes(self):
        self.endpoint_fruid_fan7_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fan7(self):
        self.set_endpoint_fruid_fan7_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN7_ENDPOINT, self.endpoint_fruid_fan7_attrb
        )

    # "/api/sys/feutil/fan8"
    def set_endpoint_fruid_fan8_attributes(self):
        self.endpoint_fruid_fan8_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_fan8(self):
        self.set_endpoint_fruid_fan8_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_FAN8_ENDPOINT, self.endpoint_fruid_fan8_attrb
        )

    # "/api/sys/peutil/pim1"
    def set_endpoint_fruid_pim1_attributes(self):
        self.endpoint_fruid_pim1_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_pim1(self):
        self.set_endpoint_fruid_pim1_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_PIM1_ENDPOINT, self.endpoint_fruid_pim1_attrb
        )

    # "/api/sys/peutil/pim2"
    def set_endpoint_fruid_pim2_attributes(self):
        self.endpoint_fruid_pim2_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_pim2(self):
        self.set_endpoint_fruid_pim2_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_PIM2_ENDPOINT, self.endpoint_fruid_pim2_attrb
        )

    # "/api/sys/peutil/pim3"
    def set_endpoint_fruid_pim3_attributes(self):
        self.endpoint_fruid_pim3_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_pim3(self):
        self.set_endpoint_fruid_pim3_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_PIM3_ENDPOINT, self.endpoint_fruid_pim3_attrb
        )

    # "/api/sys/peutil/pim4"
    def set_endpoint_fruid_pim4_attributes(self):
        self.endpoint_fruid_pim4_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_pim4(self):
        self.set_endpoint_fruid_pim4_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_PIM4_ENDPOINT, self.endpoint_fruid_pim4_attrb
        )

    # "/api/sys/peutil/pim5"
    def set_endpoint_fruid_pim5_attributes(self):
        self.endpoint_fruid_pim5_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_pim5(self):
        self.set_endpoint_fruid_pim5_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_PIM5_ENDPOINT, self.endpoint_fruid_pim5_attrb
        )

    # "/api/sys/peutil/pim6"
    def set_endpoint_fruid_pim6_attributes(self):
        self.endpoint_fruid_pim6_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_pim6(self):
        self.set_endpoint_fruid_pim6_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_PIM6_ENDPOINT, self.endpoint_fruid_pim6_attrb
        )

    # "/api/sys/peutil/pim7"
    def set_endpoint_fruid_pim7_attributes(self):
        self.endpoint_fruid_pim7_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_pim7(self):
        self.set_endpoint_fruid_pim7_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_PIM7_ENDPOINT, self.endpoint_fruid_pim7_attrb
        )

    # "/api/sys/peutil/pim8"
    def set_endpoint_fruid_pim8_attributes(self):
        self.endpoint_fruid_pim8_attrb = self.FRUID_ATTRIBUTES

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_fruid_pim8(self):
        self.set_endpoint_fruid_pim8_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FRUID_PIM8_ENDPOINT, self.endpoint_fruid_pim8_attrb
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
        self.endpoint_psu_presence = ["psu1", "psu2", "psu3", "psu4"]

    def test_endpoint_api_psu_present(self):
        self.set_endpoint_psu_presence_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.PSU_PRESENT_ENDPOINT, self.endpoint_psu_presence
        )

    # "/api/sys/presence/fan"
    def set_endpoint_fan_presence_attributes(self):
        self.endpoint_fan_presence = [
            "fan1",
            "fan2",
            "fan3",
            "fan4",
            "fan5",
            "fan6",
            "fan7",
            "fan8",
        ]

    def test_endpoint_api_fan_present(self):
        self.set_endpoint_fan_presence_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FAN_PRESENT_ENDPOINT, self.endpoint_fan_presence
        )

    # "/api/sys/firmware_info/cpld"
    def set_endpoint_firmware_info_cpld_attributes(self):
        self.endpoint_firmware_info_cpld_attributes = [
            "FCMCPLD B",
            "FCMCPLD T",
            "PWRCPLD L",
            "PWRCPLD R",
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
        self.endpoint_firmware_info_fpga_attributes = [
            "IOB FPGA",
            "PIM1 DOMFPGA",
            "PIM2 DOMFPGA",
            "PIM3 DOMFPGA",
            "PIM4 DOMFPGA",
            "PIM5 DOMFPGA",
            "PIM6 DOMFPGA",
            "PIM7 DOMFPGA",
            "PIM8 DOMFPGA",
        ]

    def test_endpoint_api_sys_firmware_info_fpga(self):
        self.set_endpoint_firmware_info_fpga_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.FPGA_FIRMWARE_INFO_ENDPOINT,
            self.endpoint_firmware_info_fpga_attributes,
        )

    # "/api/sys/firmware_info/scm"
    def set_endpoint_firmware_info_scm_attributes(self):
        self.endpoint_firmware_info_scm_attributes = [
            "Bridge-IC Version",
            "Bridge-IC Bootloader Version",
            "BIOS Version",
            "CPLD Version",
            "ME Version",
            "PVCCIN VR Version",
            "DDRAB VR Version",
            "P1V05 VR Version",
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
            "BMC Version",
            "Fan Speed Controller Version",
            "TPM Version",
            "FCMCPLD B",
            "FCMCPLD T",
            "PWRCPLD L",
            "PWRCPLD R",
            "SCMCPLD",
            "SMBCPLD",
            "IOB FPGA",
            "PIM1 DOMFPGA",
            "PIM2 DOMFPGA",
            "PIM3 DOMFPGA",
            "PIM4 DOMFPGA",
            "PIM5 DOMFPGA",
            "PIM6 DOMFPGA",
            "PIM7 DOMFPGA",
            "PIM8 DOMFPGA",
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

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
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
