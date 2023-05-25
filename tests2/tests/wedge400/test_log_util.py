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
from utils.test_utils import qemu_check


class AllLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "all"


class SMBLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "smb"

    def set_fru_id(self):
        self.FRU_ID = pal_get_fru_id(self.FRU)


class SCMLogUtilTest(SMBLogUtilTest):
    FRU = "scm"


class FAN1LogUtilTest(SMBLogUtilTest):
    FRU = "fan1"


class FAN2LogUtilTest(SMBLogUtilTest):
    FRU = "fan2"


class FAN3LogUtilTest(SMBLogUtilTest):
    FRU = "fan3"


class FAN4LogUtilTest(SMBLogUtilTest):
    FRU = "fan4"


class PSU1LogUtilTest(SMBLogUtilTest):
    FRU = "psu1"


class PSU2LogUtilTest(SMBLogUtilTest):
    FRU = "psu2"


class PEM1LogUtilTest(SMBLogUtilTest):
    FRU = "pem1"


class PEM2LogUtilTest(SMBLogUtilTest):
    FRU = "pem2"


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class SysLogUtilTest(BaseLogUtilTest, unittest.TestCase):
    FRU = "sys"
