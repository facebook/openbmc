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

import unittest

from common.base_i2c_test import BaseI2cTest
from tests.fuji.test_data.i2c.i2c import plat_i2c_tree, device_lm75
from utils.test_utils import qemu_check
from utils.i2c_utils import I2cSysfsUtils


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class I2cTest(BaseI2cTest, unittest.TestCase):
    def i2c_tree_update_lm75(self,i2c_path):
        name = I2cSysfsUtils.i2c_device_get_name(i2c_path)
        if name=="tmp75":
            self.i2c_tree.update({i2c_path:device_lm75["tmp75"]})
        elif name=="lm75":
            self.i2c_tree.update({i2c_path:device_lm75["lm75"]})

    def i2c_tree_update(self):
        for i2c_path, i2c_info in plat_i2c_tree.items():
            if i2c_info["driver"]=="lm75":
                self.i2c_tree_update_lm75(i2c_path)

    def load_golden_i2c_tree(self):
        self.i2c_tree = plat_i2c_tree
        self.i2c_tree_update()
