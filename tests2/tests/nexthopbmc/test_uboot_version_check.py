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

from common.base_uboot_version_test import BaseUbootVersionCheck


class UbootVersionCheck(BaseUbootVersionCheck, unittest.TestCase):
    def set_uboot_version_check_params(self):
        self.uboot_ver_regex = r"\"uboot_ver\":\s+\"(?P<uboot_ver>20\d{2}\.\d{2})\""
