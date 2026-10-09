#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates. (http://www.meta.com)
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

from common.base_i2c_test import BaseI2cTest
from tests.nexthopbmc.test_data.i2c.i2c import plat_i2c_tree, plat_optional_i2c_tree
from utils.i2c_utils import I2cSysfsUtils


class I2cTest(BaseI2cTest, unittest.TestCase):
    def load_golden_i2c_tree(self):
        self.i2c_tree = dict(plat_i2c_tree)

        # Only check the optional EEPROMs on the hardware revisions which
        # actually have them.
        for i2c_path, i2c_info in plat_optional_i2c_tree.items():
            if os.path.isdir(I2cSysfsUtils.i2c_device_abspath(i2c_path)):
                self.i2c_tree[i2c_path] = i2c_info
