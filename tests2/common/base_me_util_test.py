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


import re
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseMETest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.me_cmd = None

    @abstractmethod
    def set_me_cmd(self):
        pass


class CommonMETest(BaseMETest):
    def get_me_info(self):
        if self.me_cmd is not None:
            Logger.info("running cmd: {}".format(self.me_cmd))
            self.me_info = run_shell_cmd(self.me_cmd)
        return self.me_info

    def verify_me_dev_id_data(self, skip=0, re_flags=0):
        """
        Verify the data based on regex for each of test cases.
        """
        info = self.me_info.split("\n")
        regex = r"^.+:\s+[a-fA-F0-9]+\s*"
        data_regex = re.compile(regex, flags=re_flags)
        num = 1
        for line in info:
            if not (skip % num):
                m = data_regex.match(line)
                if not m and bool(line.strip()):
                    return False
            num += 1
        return True

    def verify_me_nm_power_data(self, skip=0, re_flags=0):
        """
        Verify the data based on regex for each of test cases.
        """
        info = self.me_info.split("\n")
        num = 1
        for line in info:
            if not (skip % num):
                if re.search("Power data", line):
                    continue
                elif re.search("Value", line):
                    regex = r"^.+:\s+\d{1,5} Watts"
                elif re.search("Timestamp", line):
                    regex = r"^.+:\s+\d{1,4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\s*"
                else:
                    regex = r"^.+:\s+[a-fA-F0-9]+\s*"

                data_regex = re.compile(regex, flags=re_flags)
                m = data_regex.match(line)
                if not m and bool(line.strip()):
                    return False
            num += 1
        return True

    def test_me_dev_id(self):
        """
        Method to verify ME device data.
        <NAME>: <HEX VALUE>...<HEX VALUE>
        Example:
        Device ID:                 50
        Device Revision:           01
        Firmware Revision:         04 44
        IPMI Version:              02
        Additional Device Support: 21
        Manufacturer ID:           57 01 00
        Product ID:                0f 0b
        Aux Firmware Revision:     04 02 10 01
        """
        option = " --get_dev_id"
        self.set_me_cmd()
        self.me_cmd += option
        self.get_me_info()
        self.assertTrue(self.verify_me_dev_id_data(), "ME data is invalid!")

    def test_me_nm_power_statistic(self):
        """
        Method to verify ME power data.
        <NAME>: <HEX VALUE>...<HEX VALUE>
        <POWER VALUE>: <DEC VALUE> Watts
        <TIMESTAMP>: <YEAR>-<MONTH>-<DAY> <HOUR>:<MIN>:<SEC>
        Example:
        Manufacturer ID:                  57 01 00
        Statistics Power data:
                Current Value:            108 Watts
                Minimum Value:             10 Watts
                Maximum Value:            411 Watts
                Average Value:            117 Watts
        Timestamp:                        2021-07-07 01:26:06
        Statistics Reporting Period:      34 11 00 00
        Domain ID | Policy State:         50
        """
        option = " --get_nm_power_statistics"
        self.set_me_cmd()
        self.me_cmd += option
        self.get_me_info()
        self.assertTrue(self.verify_me_nm_power_data(), "ME data is invalid!")
