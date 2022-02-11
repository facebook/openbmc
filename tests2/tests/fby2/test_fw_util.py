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
import unittest

from common.base_fw_util_test import CommonFwUtilTest

PLATFORM = "fby2"


class FwUtilVersionTest(CommonFwUtilTest, unittest.TestCase):
    def set_platform(self):
        self.platform = PLATFORM

    def test_version_header(self):
        for fru, comps in self.fw_util_info.items():
            for comp_name in comps.keys():
                cmd = "/usr/bin/fw-util {} --version {}".format(fru, comp_name)
                with self.subTest(comp_name):
                    if "version header" in comps[comp_name]:
                        self.verify_output(
                            cmd,
                            comps[comp_name]["version header"],
                            msg="fw-util version not match"
                            " expect '{}' but not found".format(
                                comps[comp_name]["version header"]
                            ),
                        )
