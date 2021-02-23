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
from abc import abstractmethod

from utils.cit_logger import Logger


class BaseSwitchTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.path = None

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def set_swtich_node_cmd(self):
        pass

    def sysfs_read(self, inp):
        """
        Read value from syscpld path,
        if value is valid return integer value, else return none.
        """
        try:
            with open(inp, "r") as f:
                str_val = f.readline().rstrip("\n")
                if str_val.find("0x") is -1:
                    val = int(str_val, 10)
                else:
                    val = int(str_val, 16)
                return val
        except Exception:
            return None

    def check_smb_syscpld_nodes_value(self, filename):
        Logger.info("filename is {}".format(filename))
        if not os.path.isfile(filename):
            return [False, filename, "access", None]
        node_data = self.sysfs_read(filename)
        if not isinstance(node_data, int):
            return [False, filename, "read", node_data]
        if node_data == 0x1:
            return [True, None, None, None]
        else:
            return [False, filename, "read", node_data]

    def test_syscpld_nodes_GB_turn_on(self):
        self.set_swtich_node_cmd()
        syscpld_nodes_value = self.check_smb_syscpld_nodes_value(self.path)
        self.assertTrue(
            syscpld_nodes_value[0],
            "{} path failed {} with value {}".format(
                syscpld_nodes_value[1],
                syscpld_nodes_value[2],
                syscpld_nodes_value[3],
            ),
        )
