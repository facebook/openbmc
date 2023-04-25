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

import common.base_libpal_test
from utils.test_utils import qemu_check


class LibPalTest(common.base_libpal_test.LibPalTest):
    PLATFORM_NAME = "yosemite35"

    @unittest.skip("disable due to T92189295")
    def test_sensor_raw_read(self):
        pass

    @unittest.skip("disable due to T92189295")
    def test_sensor_read(self):
        pass

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_pal_get_sensor_name(self):
        super().test_pal_get_sensor_name()
