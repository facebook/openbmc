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
import os.path

from common.base_i2c_test import BaseI2cTest
from tests.fuji.test_data.i2c.i2c import (
    plat_i2c_tree,
    device_lm75,
    minipack2_get_pim_i2c_tree,
    minipack2_get_pim_alternate_i2c,
    minipack2_smb_alternate_i2c,
    minipack2_fcmt_alternate_i2c,
    minipack2_fcmb_alternate_i2c,
    minipack2_smbpowcpld_pdbl_alternate_i2c,
    minipack2_smbpowcpld_pdbr_alternate_i2c,
)
from utils.test_utils import qemu_check
from utils.i2c_utils import I2cSysfsUtils
from tests.fuji.helper.libpal import pal_is_fru_prsnt, pal_get_fru_id


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class I2cTest(BaseI2cTest, unittest.TestCase):
    def i2c_tree_update_lm75(self, i2c_path):
        name = I2cSysfsUtils.i2c_device_get_name(i2c_path)
        if name == "tmp75":
            self.i2c_tree.update({i2c_path: device_lm75["tmp75"]})
        elif name == "lm75":
            self.i2c_tree.update({i2c_path: device_lm75["lm75"]})

    def i2c_tree_update(self):
        for pim in range(1, 9):  # pim1 - pim8
            if pal_is_fru_prsnt(pal_get_fru_id("pim{}".format(pim))):
                self.i2c_tree.update(minipack2_get_pim_i2c_tree(pim))

        for i2c_path, i2c_info in plat_i2c_tree.items():
            if i2c_info["driver"] == "lm75":
                self.i2c_tree_update_lm75(i2c_path)

    def i2c_tree_pim_update_alternate_list(self, pim):
        self.assertTrue(pim >= 1 and pim <= 8, "Invalid PIM number [1-8]")
        pim_alternate = minipack2_get_pim_alternate_i2c(pim)
        if pal_is_fru_prsnt(pal_get_fru_id("pim{}".format(pim))):
            self.i2c_tree_update_alternate_list(pim_alternate)

    def i2c_tree_update_alternate_list(self, alternate_list):
        for device_set in alternate_list:
            found_num = 0
            for alternate in device_set:
                path = "/sys/bus/i2c/devices/{}".format(list(alternate)[0])
                if os.path.isdir(path):
                    self.i2c_tree.update(alternate)
                    found_num = found_num + 1
            self.assertEqual(
                1,
                found_num,
                "should found only 1 device in alternate list {}".format(
                    [list(alternate)[0] for alternate in device_set]
                ),
            )

    def load_golden_i2c_tree(self):
        self.i2c_tree = plat_i2c_tree
        self.i2c_tree_update()
        self.i2c_tree_update_alternate_list(minipack2_smb_alternate_i2c)
        for pim in range(1, 9):  # PIM1-PIM8
            self.i2c_tree_pim_update_alternate_list(pim)
        self.i2c_tree_update_alternate_list(minipack2_fcmt_alternate_i2c)
        self.i2c_tree_update_alternate_list(minipack2_fcmb_alternate_i2c)
        self.i2c_tree_update_alternate_list(minipack2_smbpowcpld_pdbl_alternate_i2c)
        self.i2c_tree_update_alternate_list(minipack2_smbpowcpld_pdbr_alternate_i2c)
