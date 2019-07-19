#!/usr/bin/env python
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

from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_cmd


def getChassisFields(num_custom=0):
    chassis_fields = {"Chassis Type", "Chassis Part Number", "Chassis Serial Number"}
    for i in range(1, num_custom + 1):
        chassis_fields.add("Chassis Custom Data {}".format(i))
    return chassis_fields


def getProductFields(num_custom=0):
    product_fields = {
        "Product Manufacturer",
        "Product Name",
        "Product Part Number",
        "Product Version",
        "Product Serial",
        "Product Asset Tag",
        "Product FRU ID",
    }
    for i in range(1, num_custom + 1):
        product_fields.add("Product Custom Data {}".format(i))
    return product_fields


def getBoardFields(num_custom=0):
    board_fields = {
        "Board Mfg Date",
        "Board Mfg",
        "Board Product",
        "Board Serial",
        "Board Part Number",
        "Board FRU ID",
    }
    for i in range(1, num_custom + 1):
        board_fields.add("Board Custom Data {}".format(i))
    return board_fields


class BaseFruTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.fru_cmd = None
        self.fru_fields = None
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def run_fru_cmd(self):
        self.assertNotEqual(self.fru_cmd, None, "FRU command not set")

        Logger.info("Running FRU command: " + str(self.fru_cmd))
        fru_info = run_cmd(cmd=self.fru_cmd)
        Logger.info(fru_info)
        return fru_info


class CommonFruTest(BaseFruTest):
    def getFields(self, field_type, num_custom):
        field_funcs = {
            "chassis": getChassisFields,
            "product": getProductFields,
            "board": getBoardFields,
        }
        self.assertIn(
            field_type, field_funcs, "Unknown field type: {}".format(field_type)
        )
        return field_funcs[field_type](num_custom)

    def test_fru_fields(self):
        fru_info = self.run_fru_cmd()
        for fru_field_type in self.fru_fields:
            num_custom = self.fru_fields[fru_field_type]
            fields = self.getFields(fru_field_type, num_custom)
            with self.subTest(field_type=fru_field_type):
                for fru_field in fields:
                    with self.subTest(field=fru_field):
                        self.assertIn(fru_field, fru_info)
