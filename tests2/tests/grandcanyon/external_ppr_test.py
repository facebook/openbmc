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

import unittest

from common.base_ppr_test import BasePPRTest


SLOT_NUM = 1
PPR_FUNC = "Soft"
DIMM_QUANTITY = 4
PARAM1_VAL = ["0x8000", "0x8200", "0x8100", "0x8300"]
PARAM2_VAL = "0xfffffffffffff000"
ERR_TYPE = "0x00000008"
ERR_INJECT = "1"


class PPRTest(BasePPRTest, unittest.TestCase):
    def set_device_config(self):
        self.bmc_username = "root"
        self.bmc_passwd = "0penBmc"
        self.userver_username = "root"
        self.userver_passwd = "000000"

    def set_platform_info(self):
        self.slot_num = SLOT_NUM
        self.ppr_func = PPR_FUNC
        self.dimm_qty = DIMM_QUANTITY

    def set_err_inject_info(self):
        self.param1_val = PARAM1_VAL
        self.param2_val = PARAM2_VAL
        self.err_type = ERR_TYPE
        self.err_inject = ERR_INJECT

    def test_ppr(self):
        super().do_external_ppr_test()
