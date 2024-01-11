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
from utils.test_utils import qemu_check


class AllLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "all"

    @unittest.skip("not available")
    def test_log_clear(self):
        pass

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_log_format(self):
        super().test_log_format()


class MbLogUtilTest(AllLogUtilTest):
    FRU = "tray0_mb"


class NicLogUtilTest(AllLogUtilTest):
    FRU = "tray0_nic0"


class SysLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "sys"
