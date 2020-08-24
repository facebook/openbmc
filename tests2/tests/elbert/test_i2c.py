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

from common.base_i2c_test import BaseI2cTest
from tests.elbert.test_data.i2c.i2c import pim_devices, plat_i2c_tree, psu_devices
from utils.shell_util import run_shell_cmd


# Map between PIM and SMBus channel
pim_bus = [16, 17, 18, 23, 20, 21, 22, 19]


class ElbertI2cTest(BaseI2cTest, unittest.TestCase):
    def load_golden_i2c_tree(self):
        self.i2c_tree = plat_i2c_tree

        # Add PIM2-9 devices for available pims
        for pim in range(2, 9 + 1):
            cmd = "head -n 1 /sys/bus/i2c/devices/4-0023/pim{}_present"
            if "0x1" in run_shell_cmd(cmd.format(pim)):
                for address, name, driver in pim_devices:
                    self.i2c_tree["{:02d}-{}".format(pim_bus[pim - 2], address)] = {
                        "name": name,
                        "driver": driver,
                    }

        # Add PSU1-4 devices for available psus
        for psu in range(1, 4 + 1):
            cmd = "head -n 1 /sys/bus/i2c/devices/4-0023/psu{}_present"
            if "0x1" in run_shell_cmd(cmd.format(psu)):
                for address, name, driver in psu_devices:
                    self.i2c_tree["{:02d}-{}".format(23 + psu, address)] = {
                        "name": name,
                        "driver": driver,
                    }
