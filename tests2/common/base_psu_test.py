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


class BasePsuTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.psu_cmd = None
        self.psu_info = None
        self.psu_id = None
        self.regex = None

    @abstractmethod
    def set_psu_cmd(self):
        pass


class CommonPsuTest(BasePsuTest):
    def get_psu_info(self):
        if self.psu_cmd is not None:
            self.psu_info = run_shell_cmd(self.psu_cmd)
        return self.psu_info

    def verify_psu_data(self, skip=0, ignores=None):
        """
        Method to verify psu data based on regular expression
        for each test case.
        """
        data_regex = re.compile(self.regex)
        ignore_regex = None
        if ignores:
            ignore_regex = re.compile(
                "|".join([r"%s" % key for key in ignores]), flags=re.I
            )
        info = self.psu_info.split("\n")
        num = 1
        for line in info:
            if not (skip % num):
                m = data_regex.match(line)
                if not m and bool(line.strip()):
                    if ignore_regex:
                        if not ignore_regex.findall(line):
                            print(line)
                            return False
                    else:
                        print(line)
                        return False
            num += 1
        return True

    def test_psu_info(self):
        """
        Test psu information data.
        <NAME>    <PMbus Command> : <TEXT DATA>
        Example:
        PSU Information           : PSU1 (Bus:22 Addr:0x58)
        ---------------           : -----------------------
        MFR_ID             (0x99) : Delta
        MFR_MODEL          (0x9A) : ECD55020006
        MFR_REVISION       (0x9B) : PR
        MFR_DATE           (0x9D) : 20190223
        MFR_SERIAL         (0x9E) : THAF11908002T
        PRI_FW_VER         (0xDD) : 3.0
        SEC_FW_VER         (0xD7) : 3.0
        STATUS_WORD        (0x79) : 0x0
        STATUS_VOUT        (0x7A) : 0x0
        STATUS_IOUT        (0x7B) : 0x0
        STATUS_INPUT       (0x7C) : 0x0
        STATUS_TEMP        (0x7D) : 0x0
        STATUS_CML         (0x7E) : 0x0
        STATUS_FAN         (0x81) : 0x0
        STATUS_STBY_WORD   (0xD3) : 0x0
        STATUS_VSTBY       (0xD4) : 0x0
        STATUS_ISTBY       (0xD5) : 0x0
        OPTN_TIME_TOTAL    (0xD8) : 173D:12H:0M:57S
        OPTN_TIME_PRESENT  (0xD9) : 2D:0H:51M:53S
        """
        self.regex = r"^.+\s+\((0x|0X)[a-fA-F0-9]+\)\s+:\s+.+"
        ignores_case = ["------", "information"]
        self.set_psu_cmd()
        option = " psu{} --get_psu_info".format(self.psu_id)
        self.psu_cmd += option
        self.get_psu_info()
        self.assertTrue(
            self.verify_psu_data(ignores=ignores_case), "psu data is invalid!"
        )

    def test_eeprom_info(self):
        """
        Test psu FRU eeprom information data.
        <NAME>                    : <TEXT DATA>
        Example:
        FRU Information           : PSU1 (Bus:22 Addr:0x50)
        ---------------           : -----------------------
        Product Manufacturer      : DELTA
        Product Name              : DDM1500BH12A3F
        Product Part Number       : ECD55020006
        Product Version           : PR
        Product Serial            : THAF11908002T
        Product Asset Tag         : N/A
        Product FRU ID            : P3C300A00
        """
        self.regex = r"^.+\s+:\s+.+"
        ignores_case = ["------", "information"]
        self.set_psu_cmd()
        option = " psu{} --get_eeprom_info".format(self.psu_id)
        self.psu_cmd += option
        self.get_psu_info()
        self.assertTrue(
            self.verify_psu_data(ignores=ignores_case), "psu data is invalid!"
        )

    def test_blackbox_info(self):
        """
        Test psu blackbox information data.
        <NAME>    <PMbus Command> : <DATA>
        Example:
        Blackbox Information      : PSU1 (Bus:22 Addr:0x58)
        --------------------      : -----------------------
        PAGE                      : 0
        PRI_FW_VER         (0xDD) : 3.0
        SEC_FW_VER         (0xD7) : 3.0
        STATUS_WORD        (0x79) : 0x284a
        STATUS_VOUT        (0x7A) : 0x0
        STATUS_IOUT        (0x7B) : 0x0
        STATUS_INPUT       (0x7C) : 0x8
        STATUS_TEMP        (0x7D) : 0x0
        STATUS_CML         (0x7E) : 0x80
        STATUS_FAN         (0x81) : 0x0
        STATUS_STBY_WORD   (0xD3) : 0x800
        STATUS_VSTBY       (0xD4) : 0x0
        STATUS_ISTBY       (0xD5) : 0x0
        OPTN_TIME_TOTAL    (0xD8) : 63D:18H:57M:34S
        OPTN_TIME_PRESENT  (0xD9) : 1D:15H:22M:53S
        IN_VOLT            (0x88) : 121.50 V
        12V_VOLT           (0x89) : 12.00 V
        IN_CURR            (0x8B) : 0.45 A
        12V_CURR           (0x8C) : 6.78 A
        TEMP1              (0x8D) : 30.00 C
        TEMP2              (0x8E) : 29.00 C
        TEMP3              (0x8F) : 36.00 C
        FAN_SPEED          (0x90) : 6688.00 RPM
        """
        self.regex = r"^.+\s+\((0x|0X)[a-fA-F0-9]+\)\s+:\s+.+"
        ignores_case = ["------", "information", "page"]
        self.set_psu_cmd()
        option = " psu{} --get_blackbox_info --print".format(self.psu_id)
        self.psu_cmd += option
        self.get_psu_info()
        self.assertTrue(
            self.verify_psu_data(ignores=ignores_case), "psu data is invalid!"
        )
