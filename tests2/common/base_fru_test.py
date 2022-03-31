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
import json

from utils.cit_logger import Logger
from utils.shell_util import run_cmd


class BaseFruTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.fru_cmd = None
        self.fru_fields = None
        self.fru_data_in_json = None

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    def run_fru_cmd(self):
        self.assertNotEqual(self.fru_cmd, None, "FRU command not set")

        Logger.info("Running FRU command: " + str(self.fru_cmd))
        fru_info = run_cmd(cmd=self.fru_cmd)
        Logger.info(fru_info)
        return fru_info

    def getChassisFields(self, num_custom=0):
        chassis_fields = {
            "Chassis Type",
            "Chassis Part Number",
            "Chassis Serial Number",
        }
        for i in range(1, num_custom + 1):
            chassis_fields.add("Chassis Custom Data {}".format(i))
        return chassis_fields

    def getProductFields(self, num_custom=0):
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

    def getBoardFields(self, num_custom=0):
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

    def checkFruFieldInJson(self, field: str) -> bool:
        """[summary] to check if given fru field is empty
            fruid-util tend to skip fru fileds if that given fields is empty
            if output in stdout. It will cause CIT to fail since the field is
            missing from the output. In this case we should double check the
            output in json format to determine if the tested field are really
            mssing or not before determined pass/fail of the test cases
            We'll give a free-pass to "Custom Data" since every platform tend
            to use this filed differently and its not a fixed field/value for
            testing.
        Args:
            field (str): Board FRU ID, Product Custom Data 1..etc

        Returns:
            bool: return True if field present in json else False
        """
        if "Custom Data" in field:
            return True
        if field in self.fru_data_in_json.keys() and not self.fru_data_in_json[field]:
            return True
        if (
            field in self.fru_data_in_json.keys()
            and self.fru_data_in_json[field] == "N/A"
        ):
            return True
        return False


class CommonFruTest(BaseFruTest):
    def getFields(self, field_type, num_custom):
        field_funcs = {
            "chassis": self.getChassisFields,
            "product": self.getProductFields,
            "board": self.getBoardFields,
        }
        self.assertIn(
            field_type, field_funcs, "Unknown field type: {}".format(field_type)
        )
        return field_funcs[field_type](num_custom)

    def test_fru_fields(self):
        # get fru info in json for reference
        cmd = self.fru_cmd + ["--json"]
        self.fru_data_in_json = json.loads(run_cmd(cmd))[0]

        # continue to fru test
        fru_info = self.run_fru_cmd()
        for fru_field_type in self.fru_fields:
            num_custom = self.fru_fields[fru_field_type]
            fields = self.getFields(fru_field_type, num_custom)
            with self.subTest(field_type=fru_field_type):
                for fru_field in fields:
                    with self.subTest(field=fru_field):
                        try:
                            self.assertIn(fru_field, fru_info)
                        except AssertionError:
                            Logger.info(
                                "{} not found in stdout, checking json".format(
                                    fru_field
                                )
                            )
                            if self.checkFruFieldInJson(fru_field):
                                self.assertTrue(True, "{} is empty or N/A, test passed")
                            else:
                                self.assertIn(fru_field, fru_info)
