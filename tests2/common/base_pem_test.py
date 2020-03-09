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
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from common.base_eeprom_test import CommonEepromTest


class BasePemTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.pem_cmd = None
        self.pem_id = None
        self.pem_info = None
        self.eeprom_info = None
        self.pem_hsc_sensors = [
            "PEM{}_IN_VOLT",
            "PEM{}_OUT_VOLT",
            "PEM{}_CURR",
            "PEM{}_POWER",
            "PEM{}_FAN1_SPEED",
            "PEM{}_FAN2_SPEED",
            "PEM{}_HOT_SWAP_TEMP",
            "PEM{}_AIR_INLET_TEMP",
            "PEM{}_AIR_OUTLET_TEMP",
        ]
        self.pem_hsc_discrete_sensors = [
            "ON_STATUS",
            "FET_BAD_COOLDOWN_STATUS",
            "FET_SHORT_PRESENT",
            "ON_PIN_STATUS",
            "POWER_GOOD_STATUS",
            "OC_COOLDOWN_STATUS",
            "UV_STATUS",
            "OV_STATUS",
            "GPIO3_STATUS",
            "GPIO2_STATUS",
            "GPIO1_STATUS",
            "LERT_STATUS",
            "EEPROM_BUSY",
            "ADC_IDLE",
            "TICKER_OVERFLOW_PRESENT",
            "METER_OVERFLOW_PRESENT",
            "EEPROM_Done",
            "FET_Bad_Fault",
            "FET_Short_Fault",
            "On_Fault",
            "Power_Bad_Fault",
            "OC_Fault",
            "UV_Fault",
            "OV_Fault",
            "Power_Alarm_High",
            "Power_Alarm_Low",
            "Vsense_Alarm_High",
            "Vsense_Alarm_Low",
            "VSourve_Alarm_High",
            "VSourve_Alarm_Low",
            "VGPIO_Alarm_High",
            "VGPIO_Alarm_Low",
        ]
        self.pem_hsc_eeprom_sensors = [
            "On Fault Mask",
            "On Delay",
            "On/Enb",
            "Mass Write Enable",
            "Fet on",
            "OC Autoretry",
            "UV Autoretry",
            "OV Autoretry",
            "On FB Mode",
            "On UV Mode",
            "On OV Mode",
            "On Vin Mode",
            "EEPROM Done Alert",
            "FET Bad Fault Alert",
            "FET Short Alert",
            "On Alert",
            "PB Alert",
            "OC Alert",
            "UV Alert",
            "OV Alert",
            "Power Alarm High",
            "Power Alarm Low",
            "Vsense Alarm High",
            "Vsense Alarm Low",
            "VSourve Alarm High",
            "VSourve Alarm Low",
            "VGPIO Alarm High",
            "VGPIO Alarm Low",
            "EEPROM_Done",
            "FET_Bad_Fault",
            "FET_Short_Fault",
            "On_Fault",
            "Power_Bad_Fault",
            "OC_Fault",
            "UV_Fault",
            "OV_Fault",
            "Power_Alarm_High",
            "Power_Alarm_Low",
            "Vsense_Alarm_High",
            "Vsense_Alarm_Low",
            "VSourve_Alarm_High",
            "VSourve_Alarm_Low",
            "VGPIO_Alarm_High",
            "VGPIO_Alarm_Low",
            "GPIO3 PD",
            "GPIO2 PD",
            "GPIO1 Config",
            "GPIO1 Output",
            "ADC Conv Alert",
            "Stress to GPIO2",
            "Meter Overflow Alert",
            "Coulomb Meter",
            "Tick Out",
            "Int Clock Out",
            "Clock Divider",
            "ILIM Adjust",
            "Foldback Mode",
            "Vsource/VDD",
            "GPIO Mode",
            "ADC 16-BIT/12-BIT",
        ]
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def set_pem_cmd(self):
        pass

    def run_pem_cmd(self):
        self.set_pem_cmd()
        self.assertNotEqual(self.pem_cmd, None, "PEM command not set")

        Logger.info("Running PEM command: " + str(self.pem_cmd))
        self.pem_info = run_shell_cmd(cmd=self.pem_cmd)
        Logger.info(self.pem_info)
        return self.pem_info

    @abstractmethod
    def get_pem_info(self):
        self.pem_info = self.run_pem_cmd()
        if "not present" in self.pem_info:
            self.skipTest("pem{} is not present".format(self.pem_id))
        return self.pem_info


class PemInfoTest(BasePemTest):
    @abstractmethod
    def set_pem_sensors(self):
        pass

    @abstractmethod
    def set_pem_discrete_sensors(self):
        pass

    def test_pem_threshold_sensors(self):
        self.get_pem_info()
        self.set_pem_sensors()
        for sensor in self.pem_hsc_sensors:
            with self.subTest(sensor=sensor):
                self.assertIn(
                    sensor.format(self.pem_id),
                    self.pem_info,
                    "Cannot find {} in PEM{}".format(
                        sensor.format(self.pem_id), self.pem_id
                    ),
                )

    def test_pem_discrete_sensors(self):
        self.get_pem_info()
        self.set_pem_discrete_sensors()
        for sensor in self.pem_hsc_discrete_sensors:
            with self.subTest(sensor=sensor):
                self.assertIn(
                    sensor,
                    self.pem_info,
                    "Cannot find {} in PEM{}".format(sensor, self.pem_id),
                )


class PemEepromTest(BasePemTest, CommonEepromTest):
    def get_eeprom_info(self):
        if not self.eeprom_info:
            self.eeprom_info = self.get_pem_info()
        return self.eeprom_info


class PemBlackboxTest(BasePemTest):
    @abstractmethod
    def set_pem_hsc_eeprom_sensors(self):
        pass

    def test_pem_hsc_eeprom_sensors(self):
        self.get_pem_info()
        self.set_pem_hsc_eeprom_sensors()
        for sensor in self.pem_hsc_eeprom_sensors:
            with self.subTest(sensor=sensor):
                self.assertIn(
                    sensor,
                    self.pem_info,
                    "Cannot find {} in PEM{}".format(sensor, self.pem_id),
                )
