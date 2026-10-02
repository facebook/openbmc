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

from common.base_flash_data0_test import BaseFlashData0Test


class FlashData0Test(BaseFlashData0Test, unittest.TestCase):
    def set_data0_info(self):
        self.data0_dev = "/dev/ubi0_0"
        self.data0_mountpoint = "/mnt/data"
        self.data0_fs_type = "ubifs"
        self.data0_size_mb = 64.0
