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
import unittest

from common.base_log_util_test import BaseLogUtilTest
from tests.montblanc.helper.libpal import pal_get_fru_id


class AllLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "all"


class SCMLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "scm"

    def set_fru_id(self):
        self.FRU_ID = pal_get_fru_id(self.FRU)


class SysLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "sys"
