#!/usr/bin/env python3
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
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


class SMBLogUtilTest(AllLogUtilTest):
    FRU = "smb"


class SCMLogUtilTest(AllLogUtilTest):
    FRU = "scm"


class PIM1LogUtilTest(AllLogUtilTest):
    FRU = "pim1"


class PIM2LogUtilTest(AllLogUtilTest):
    FRU = "pim2"


class PIM3LogUtilTest(AllLogUtilTest):
    FRU = "pim3"


class PIM4LogUtilTest(AllLogUtilTest):
    FRU = "pim4"


class PIM5LogUtilTest(AllLogUtilTest):
    FRU = "pim5"


class PIM6LogUtilTest(AllLogUtilTest):
    FRU = "pim6"


class PIM7LogUtilTest(AllLogUtilTest):
    FRU = "pim7"


class PIM8LogUtilTest(AllLogUtilTest):
    FRU = "pim8"


class PSU1LogUtilTest(AllLogUtilTest):
    FRU = "psu1"


class PSU2LogUtilTest(AllLogUtilTest):
    FRU = "psu2"


class PSU3LogUtilTest(AllLogUtilTest):
    FRU = "psu3"


class PSU4LogUtilTest(AllLogUtilTest):
    FRU = "psu4"


class SysLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "sys"
