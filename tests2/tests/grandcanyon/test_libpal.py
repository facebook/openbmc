#
# Copyright 2021-present Facebook. All Rights Reserved.
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

try:
    import pal
except ModuleNotFoundError:
    # Suppress if module not found to support test case discovery without
    # BMC dependencies (will test if pal is importable at LibPalTest.setUp())
    pass


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class LibPalTest(common.base_libpal_test.LibPalTest):
    PLATFORM_NAME = "grandcanyon"

    @unittest.skip("disable due to T92189295")
    def test_sensor_raw_read(self):
        pass

    @unittest.skip("disable due to T92189295")
    def test_sensor_read(self):
        pass

    @unittest.skip(
        "disable due to the platform name 'Grand Canyon' didn't match Regex."
    )
    def test_pal_get_platform_name(self):
        pass

    def test_pal_get_sensor_name(self):
        fru_ids = [pal.pal_get_fru_id(fru_name) for fru_name in pal.pal_get_fru_list()]

        for fru_id in fru_ids:
            # Not support to get server and BMC sensor name
            if fru_id == 1 or fru_id == 2:
                continue
            for snr_num in pal.pal_get_fru_sensor_list(fru_id):
                sensor_name = pal.pal_get_sensor_name(fru_id, snr_num)

                self.assertRegex(sensor_name, r"^[^ ]+$")
