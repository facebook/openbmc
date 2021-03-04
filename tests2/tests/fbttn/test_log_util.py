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

from common.base_log_util_test import BaseLogUtilTest

PLATFORM = "fbttn"


class LogUtilTest(BaseLogUtilTest, unittest.TestCase):
    def set_output_regex_mapping(self):
        # TODO: this is initial draft to check any charactors which are not suppose to exist
        # We can work on more details later
        self.output_regex_mapping = {
            "APP_NAME": {"idx": 3, "regex": ":"},
            "FRU#": {"idx": 0, "regex": ":"},
            "FRU_NAME": {"idx": 1, "regex": ":"},
            "TIME_STAMP": {"idx": 2, "regex": "[a-zA-Z]"},
        }

    def set_platform(self):
        self.platform = PLATFORM
