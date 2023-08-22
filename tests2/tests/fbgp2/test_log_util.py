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


class AllLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "all"

    @unittest.skip("not available")
    def test_log_clear(self):
        pass


class SpbLogUtilTest(AllLogUtilTest):
    FRU = "spb"


class NicLogUtilTest(AllLogUtilTest):
    FRU = "nic"


class Slot1LogUtilTest(AllLogUtilTest):
    FRU = "slot1"


class Slot2LogUtilTest(AllLogUtilTest):
    FRU = "slot2"


class Slot3LogUtilTest(AllLogUtilTest):
    FRU = "slot3"


class Slot4LogUtilTest(AllLogUtilTest):
    FRU = "slot4"


class SysLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "sys"
