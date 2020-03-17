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
import re
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseBicTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.bic_cmd = None
        self.bic_info = None
        self.regex = None

    @abstractmethod
    def set_bic_cmd(self):
        pass


class CommonBicTest(BaseBicTest):
    def get_bic_info(self):
        if self.bic_cmd is not None:
            self.bic_info = run_shell_cmd(self.bic_cmd)
        return self.bic_info

    def verify_bic_data(self, skip=0, ignore="", re_flags=0):
        """
        Verify the data based on regex for each of test cases.
        """
        data_regex = re.compile(self.regex, flags=re_flags)
        info = self.bic_info.split("\n")
        num = 1
        for line in info:
            if not (skip % num):
                m = data_regex.match(line)
                if not m and bool(line.strip()):
                    if ignore not in line.lower():
                        return False
            num += 1
        return True

    def test_bic_dev_id(self):
        """
        Method to verify BIC device id data.
        <NAME>: <HEX VALUE>...<HEX VALUE>
        Example:
        Device ID: 0x25
        Device Revision: 0x80
        Firmware Revision: 0x1:0x11
        IPMI Version: 0x2
        Device Support: 0xBF
        Manufacturer ID: 0x0:0x1C:0x4C
        Product ID: 0x46:0x20
        Aux. FW Rev: 0x0:0x0:0x0:0x0
        """
        option = " --get_dev_id"
        self.regex = r"^.+:\s(0x|0X)[a-fA-F0-9]+\s*"
        self.set_bic_cmd()
        self.bic_cmd += option
        self.get_bic_info()
        self.assertTrue(self.verify_bic_data(), "bic data is invalid!")

    def test_bic_gpio(self):
        """
        Method to verify BIC GPIO data.
        <NAME>: <DEC VALUE>...<DEC VALUE>
        Example:
        XDP_CPU_SYSPWROK: 1
        PWRGD_PCH_PWROK: 1
        PVDDR_VRHOT_N: 1
        PVCCIN_VRHOT_N: 1
        FM_FAST_PROCHOT_N: 1
        PCHHOT_CPU_N: 1
        FM_CPLD_CPU_DIMM_EVENT_CO_N: 1
        FM_CPLD_BDXDE_THERMTRIP_N: 1
        ...
        ...
        """
        option = " --get_gpio"
        self.regex = r"^.+:\s[0-9]+\s*"
        self.set_bic_cmd()
        self.bic_cmd += option
        self.get_bic_info()
        self.assertTrue(self.verify_bic_data(), "bic data is invalid!")

    def test_bic_gpio_config(self):
        """
        Method to verify BIC GPIO Config data.
        pin#<NUMBER of PIN>
        Example:
        gpio_config for pin#0 (XDP_CPU_SYSPWROK):
        Direction: Input,  Interrupt: Enabled,  Trigger: Edge Trigger,  Edge: Both Edges
        gpio_config for pin#1 (PWRGD_PCH_PWROK):
        Direction: Input,  Interrupt: Enabled,  Trigger: Edge Trigger,  Edge: Falling Edge
        gpio_config for pin#2 (PVDDR_VRHOT_N):
        Direction: Input,  Interrupt: Disabled, Trigger: Edge Trigger,  Edge: Falling Edge
        ...
        ...
        """
        option = " --get_gpio_config"
        self.regex = r"^.+pin#[0-9]+\s*"
        self.set_bic_cmd()
        self.bic_cmd += option
        self.get_bic_info()
        self.assertTrue(self.verify_bic_data(skip=1), "bic data is invalid!")

    def test_bic_config(self):
        """
        Method to verify BIC config data.
        <NAME>: <STATUS TEXT>
        Example:
        SoL Enabled:  Enabled
        POST Enabled: Enabled
        """
        option = " --get_config"
        self.regex = r"^.+:\s+(enabled|disabled).*$"
        self.set_bic_cmd()
        self.bic_cmd += option
        self.get_bic_info()
        self.assertTrue(self.verify_bic_data(re_flags=re.I), "bic data is invalid!")

    def test_bic_post_code(self):
        """
        Method to verify POST data.
        <HEX VALUE>...<HEX VALUE>
        Example:
        AA BB 00 01 EF 0A 0E EA
        ...
        ...
        """
        option = " --get_post_code"
        self.regex = r"^.+[a-fA-F0-9]+.+\s*"
        self.set_bic_cmd()
        self.bic_cmd += option
        self.get_bic_info()
        self.assertTrue(self.verify_bic_data(), "bic data is invalid!")

    def test_bic_sdr(self):
        """
        Method to verify BIC GPIO data.
        <NAME>: <DEC VALUE>
        Example:
        type: 1, sensor_num: 1, sensor_type: 1, evt_read_type: 1, m_val: 1, m_tolerance: 0, b_val: 0, b_accuracy: 0, accuracy_dir: 0, rb_exp: 0,
        type: 1, sensor_num: 7, sensor_type: 1, evt_read_type: 1, m_val: 1, m_tolerance: 0, b_val: 0, b_accuracy: 0, accuracy_dir: 0, rb_exp: 0,
        type: 1, sensor_num: 8, sensor_type: 1, evt_read_type: 1, m_val: 1, m_tolerance: 0, b_val: 0, b_accuracy: 0, accuracy_dir: 0, rb_exp: 0,
        type: 1, sensor_num: 5, sensor_type: 1, evt_read_type: 1, m_val: 1, m_tolerance: 0, b_val: 0, b_accuracy: 0, accuracy_dir: 0, rb_exp: 0,
        type: 1, sensor_num: 9, sensor_type: 1, evt_read_type: 1, m_val: 1, m_tolerance: 0, b_val: 0, b_accuracy: 0, accuracy_dir: 0, rb_exp: 0,
        ...
        ...
        """
        option = " --get_sdr"
        ignore_str = "last record"
        self.regex = r"^(.+:\s[0-9]+,)\s*"
        self.set_bic_cmd()
        self.bic_cmd += option
        self.get_bic_info()
        self.assertTrue(self.verify_bic_data(ignore=ignore_str), "bic data is invalid!")

    def test_bic_sensor(self):
        """
        Method to verify BIC sensor data.
        <SENSOR#>: <NAME: HEX VALUE>,...,<NAME: HEX VALUE>
        Example:
        sensor#1: value: 0x1A, flags: 0xC0, status: 0xC0, ext_status: 0x0
        sensor#5: value: 0x21, flags: 0xC0, status: 0xC0, ext_status: 0x0
        sensor#7: value: 0x17, flags: 0xC0, status: 0xC0, ext_status: 0x0
        sensor#8: value: 0x1C, flags: 0xC0, status: 0xC0, ext_status: 0x0
        sensor#9: value: 0xB9, flags: 0xC0, status: 0xC0, ext_status: 0x0
        ...
        ...
        """
        option = " --read_sensor"
        self.regex = r"^(.+:\s(0x|0X)[a-fA-F0-9]+,)\s*"
        self.set_bic_cmd()
        self.bic_cmd += option
        self.get_bic_info()
        self.assertTrue(self.verify_bic_data(), "bic data is invalid!")

    def test_bic_fruid(self):
        """
        Method to verify BIC FRUID data.
        <NAME>                    : <TEXT INFORMATION>
        Example:
        FRU Information           : MINILAKE
        ---------------           : ------------------
        Chassis Type              : Rack Mount Chassis
        Chassis Part Number       :
        Chassis Serial Number     :
        Board Mfg Date            : Thu Jul  4 17:14:00 2019
        Board Mfg                 : Quanta
        Board Product             : Minilake
        ...
        ...
        """
        option = " --read_fruid"
        self.regex = r"^.+:\s(.+)?\s*"
        self.set_bic_cmd()
        self.bic_cmd += option
        self.get_bic_info()
        self.assertTrue(self.verify_bic_data(), "bic data is invalid!")

    def test_bic_mac(self):
        """
        Method to verify BIC MAC data.
        <NAME>: <FF:FF:FF:FF:FF:FF>
        Example:
        MAC address: b4:a9:fc:1e:0b:65
        """
        option = " --read_mac"
        self.regex = r"^.+:\s([a-fA-F0-9]{2}:?){6}$"
        self.set_bic_cmd()
        self.bic_cmd += option
        self.get_bic_info()
        self.assertTrue(self.verify_bic_data(), "bic data is invalid!")
