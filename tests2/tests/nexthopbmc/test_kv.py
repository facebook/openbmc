#!/usr/bin/env python3
#
# Copyright (c) 2026 Nexthop Systems Inc.
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

import unittest

from common.base_kv_test import BaseKvTest
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class KvTest(BaseKvTest, unittest.TestCase):
    def set_kv_keys(self):
        # nexthopbmc runs ipmi-lite, which only records the host restart cause.
        # The SMBIOS sys_config/* keys other platforms check are never populated.
        self.kv_keys = ["fru1_restart_cause"]
