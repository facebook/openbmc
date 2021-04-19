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


import re
from abc import abstractmethod
from unittest import TestCase

from utils.cit_logger import Logger
from utils.shell_util import run_cmd, run_shell_cmd


class BaseSlotUtilTest(TestCase):
    """[summary] Base class for fw_util test
    To provide common function
    """

    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.set_slot_nums()

    @abstractmethod
    def set_slot_nums(self):
        pass

    def convert_output_to_dict(self, output):
        ret = {}
        for _output in output:
            lst = _output.split(":", 1)
            if len(lst) > 1:
                ret[lst[0].strip()] = lst[1].strip()
        return ret

    def test_show_slot_by_name(self):
        """
        check if [slot_name] exist in the output
        """
        for num in self.slot_nums:
            with self.subTest(slot="slot{}".format(num)):
                cmd = ["slot-util", "slot{}".format(num)]
                shell_output = run_cmd(cmd).split("\n")
                output_dict = self.convert_output_to_dict(shell_output[1:])
                self.assertTrue(
                    "slot{}".format(num) in output_dict,
                    "missing header slot{} in output".format(num),
                )

    def test_show_slot_by_number(self):
        """
        check if [slot_name] exist in the output
        """
        for num in self.slot_nums:
            with self.subTest(slot=num):
                cmd = ["slot-util", str(num)]
                shell_output = run_cmd(cmd).split("\n")
                output_dict = self.convert_output_to_dict(shell_output[1:])
                self.assertTrue(
                    "slot{}".format(num) in output_dict,
                    "missing header slot{} in output".format(num),
                )

    def test_show_slot_mac(self):
        """
        test slot-util --show-mac
        """
        cmd = "slot-util --show-mac"
        cmd_output = run_shell_cmd(cmd, exp_return_code=1).split("\n")
        output_dict = self.convert_output_to_dict(cmd_output[1:])
        mac_pattern = (
            r"^[0-9a-z]{2}:[0-9a-z]{2}:[0-9a-z]{2}:[0-9a-z]{2}:[0-9a-z]{2}:[0-9a-z]{2}$"
        )
        r = re.compile(mac_pattern)
        for num in self.slot_nums:
            with self.subTest(slot=num):
                key = "slot{} MAC".format(num)
                self.assertTrue(key in output_dict)
                self.assertIsNotNone(r.match(output_dict[key]))
