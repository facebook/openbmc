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


class LibPalTest(common.base_libpal_test.LibPalTest):
    PLATFORM_NAME = "elbert"

    def test_pal_is_fru_prsnt(self):
        super().test_pal_is_fru_prsnt()

    def test_sensor_raw_read(self):
        super().test_sensor_raw_read()

    def test_sensor_read(self):
        super().test_sensor_read()

    def test_sensor_read_fru_not_present(self):
        super().test_sensor_read_fru_not_present()
