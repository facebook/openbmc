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
import json
import os
import unittest

from common.base_rest_endpoint_test import FbossRestEndpointTest
from utils.test_utils import qemu_check


POWER_TYPE_CACHE = "/var/cache/detect_power_module_type.txt"


class RestEndpointTest(FbossRestEndpointTest, unittest.TestCase):
    """
    Input data to the test needs to be a list like below.
    User can choose to sends these lists from jsons too.
    """

    SYS_FW_INFO_ENDPOINT = "/api/sys/firmware_info/sys"
    FAN_FW_INFO_ENDPOINT = "/api/sys/firmware_info/fan"
    EEPROM_FW_INFO_ENDPOINT = "/api/sys/firmware_info/internal_switch_config"
    PEM_PRESENT_ENDPOINT = "/api/sys/presence/pem"
    PSU_PRESENT_ENDPOINT = "/api/sys/presence/psu"

    def get_sensors_list(self, dev, presence_value):
        """
        check to see which power device is being used and
        if it is being detected. Then we return the list
        of the power device kernel driver modules that must
        present in the image.
        pem1 = 1 --> means pem with ltc4151 inserted
        pem2 = 1 --> means pem with ltc4281 inserted
        psu1 and psu2 = 1 -> means system with PSU inserted
        """
        kmods_list = []
        if dev == "pem1" and presence_value == 1:
            if os.path.exists("/sys/bus/i2c/devices/14-006f/"):
                kmods_list.append("ltc4151-i2c-14-6f")
            else:
                kmods_list.append("ltc4151-i2c-15-6f")
        elif dev == "pem2" and presence_value == 1:
            if os.path.exists("/sys/bus/i2c/devices/14-006f/"):
                kmods_list.extend(["ltc4151-i2c-14-6f", "ltc4281-i2c-14-4a"])
            else:
                kmods_list.extend(["ltc4151-i2c-15-6f", "ltc4281-i2c-15-4a"])
        elif dev == "psu1" or dev == "psu2":
            if presence_value == 1:
                # TODO: PFE driver unstable now. That section
                # will be filled out after kernel upgrade which
                # will hopefully lead to better drivcer stability
                kmods_list = []
        else:
            raise Exception("file contains unknown module")

        return kmods_list

    def get_power_module(self):
        """
        Read appropriate path that contains the presence
        status of various power module option.
        """
        power_module_list = []
        if not os.path.exists(POWER_TYPE_CACHE):
            raise Exception("Path for power type doesn't exist")
        with open(POWER_TYPE_CACHE, "r") as fp:
            lines = fp.readlines()
            if lines:
                for line in lines:
                    dev = line.split(": ")[0]
                    presence_status = line.split(": ")[1]
                    if int(presence_status) == 1:
                        power_module_list = self.get_sensors_list(
                            dev, int(presence_status)
                        )
            else:
                raise Exception("Power module file is empty")
        return power_module_list

    # /api
    def set_endpoint_api_attributes(self):
        self.endpoint_api_attrb = ["sys"]

    # /api/sys
    def set_endpoint_sys_attributes(self):
        self.endpoint_sys_attrb = [
            "psu_update",
            "gpios",
            "i2cflush",
            "mb",
            "fc_present",
            "modbus_registers",
            "server",
            "sensors",
            "bmc",
            "firmware_info",
        ]

    # /api/sys/sensors
    def set_endpoint_sensors_attributes(self):
        self.endpoint_sensors_attrb = [
            "tmp75-i2c-3-48",
            "tmp75-i2c-3-49",
            "tmp75-i2c-3-4a",
            "tmp75-i2c-3-4b",
            "tmp75-i2c-3-4c",
            "fancpld-i2c-8-33",
            "com_e_driver-i2c-4-33",
            "tmp75-i2c-8-48",
            "tmp75-i2c-8-49",
        ]
        power_sensors_list = self.get_power_module()
        self.endpoint_sensors_attrb.extend(power_sensors_list)

    # "/api/sys/mb"
    def set_endpoint_mb_attributes(self):
        self.endpoint_mb_attrb = ["fruid"]

    # "/api/sys/server"
    def set_endpoint_server_attributes(self):
        self.endpoint_server_attrb = ["BIC_ok", "status"]

    # "/api/sys/mb/fruid"
    def set_endpoint_fruid_attributes(self):
        self.endpoint_fruid_attrb = [
            "Assembled At",
            "Local MAC",
            "Product Asset Tag",
            "Product Name",
        ]

    # "/api/sys/bmc"
    def set_endpoint_bmc_attributes(self):
        self.endpoint_bmc_attrb = [
            "At-Scale-Debug Running",
            "CPU Usage",
            "Description",
            "MAC Addr",
            "Memory Usage",
            "OpenBMC Version",
            "SPI0 Vendor",
            "SPI1 Vendor",
            "TPM FW version",
            "TPM TCG version",
            "Reset Reason",
            "Uptime",
            "kernel version",
            "load-1",
            "load-15",
            "load-5",
            "open-fds",
            "u-boot version",
            "vboot",
        ]

    def set_endpoint_fc_present_attributes(self):
        self.endpoint_fc_present_attrb = ["Not Applicable"]

    # "/api/sys/firmware_info"
    def set_endpoint_firmware_info_attributes(self):
        self.endpoint_firmware_info_attrb = [
            "all",
            "sys",
            "fan",
            "internal_switch_config",
        ]

    def set_endpoint_firmware_info_all_attributes(self):
        # TODO: what if this changes?
        self.endpoint_firmware_info_all_attrb = [
            "6.101",
            "1.11",
            "36e0d729d079561511c18db645f3f7617fd7c14fcb9be9712",
        ]

    def set_endpoint_firmware_info_sys_attributes(self):
        # TODO: what if this changes?
        self.endpoint_firmware_info_sys_attrb = ["6.101"]

    def set_endpoint_firmware_info_fan_attributes(self):
        # TODO: what if this changes?
        self.endpoint_firmware_info_fan_attrb = ["1.11"]

    def set_endpoint_pem_presence_attributes(self):
        self.endpoint_pem_presence = ["pem1", "pem2"]

    def set_endpoint_psu_presence_attributes(self):
        self.endpoint_psu_presence = ["psu1", "psu2"]

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_pem_present(self):
        self.set_endpoint_pem_presence_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.PEM_PRESENT_ENDPOINT, self.endpoint_pem_presence
        )

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_psu_present(self):
        self.set_endpoint_psu_presence_attributes()
        self.verify_endpoint_attributes(
            RestEndpointTest.PSU_PRESENT_ENDPOINT, self.endpoint_psu_presence
        )

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_firmware_info_fan(self):
        self.set_endpoint_firmware_info_fan_attributes()
        self.assertNotEqual(self.endpoint_firmware_info_fan_attrb, None)
        info = self.get_from_endpoint(RestEndpointTest.SYS_FW_INFO_ENDPOINT)
        # If we get any JSON object I'm happy for the time being
        self.assertTrue(json.loads(info))

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_sys_firmware_info_sys(self):
        self.set_endpoint_firmware_info_sys_attributes()
        self.assertNotEqual(self.endpoint_firmware_info_sys_attrb, None)
        info = self.get_from_endpoint(RestEndpointTest.FAN_FW_INFO_ENDPOINT)
        self.assertTrue(json.loads(info))

    def set_endpoint_firmware_info_eeprom_attributes(self):
        # TODO: what if this changes?
        self.endpoint_firmware_info_eeprom_attrb = [
            "36e0d729d079561511c18db645f3f7617fd7c14fcb9be9712"
        ]

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_endpoint_api_eeprom_firmware_info_sys(self):
        self.set_endpoint_firmware_info_eeprom_attributes()
        self.assertNotEqual(self.endpoint_firmware_info_eeprom_attrb, None)
        info = self.get_from_endpoint(RestEndpointTest.EEPROM_FW_INFO_ENDPOINT)
        # If we get any JSON object I'm happy for the time being
        self.assertTrue(json.loads(info))
