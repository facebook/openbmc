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

import os
import unittest

import common.base_libpal_test
from tests.wedge400.helper.libpal import pal_get_board_type
from utils.test_utils import qemu_check


class LibPalTest(common.base_libpal_test.LibPalTest):
    PLATFORM_NAME = "wedge400"

    def get_skip_sensors(self):
        # we need to skip Wedge400C GB sensors if SDK is not running in
        # host side.
        board_type = pal_get_board_type()
        if board_type == "Wedge400":
            return []

        # GB sensors are located on SMB (FRU ID 2), and the sensor names
        # are SMB_SENSOR_SW_DIE_TEMP[1-2] and SMB_SENSOR_GB_TEMP[1-10].
        # NOTE: we will need to update fru_id and sensor_num if the macros
        # were updated in pal.h. Is there a way to reference the definitions
        # in pal.h?
        gb_sensors = [
            [2, 56],
            [2, 57],
            [2, 100],
            [2, 101],
            [2, 102],
            [2, 103],
            [2, 104],
            [2, 105],
            [2, 106],
            [2, 107],
            [2, 108],
            [2, 109],
        ]

        try:
            gb_hwmon = os.path.realpath("/sys/bus/i2c/devices/3-002a/hwmon/hwmon*")
            if not os.path.exists(gb_hwmon):
                return gb_sensors

            with open(os.path.join(gb_hwmon, "sdk_status"), "r") as f:
                output = f.readline().rstrip("\n")
                sdk_status = int(output, 10)
        except Exception:
            sdk_status = 0

        # "sdk_status == 0" means SDK is not running.
        if sdk_status == 0:
            return gb_sensors

        return []

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_pal_is_fru_prsnt(self):
        super().test_pal_is_fru_prsnt()

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_sensor_raw_read(self):
        skipped = self.get_skip_sensors()
        super().test_sensor_raw_read(skip_sensors=skipped)

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_sensor_read(self):
        skipped = self.get_skip_sensors()
        super().test_sensor_read(skip_sensors=skipped)

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_sensor_read_fru_not_present(self):
        super().test_sensor_read_fru_not_present()

    @unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
    def test_pal_get_sensor_name(self):
        super().test_pal_get_sensor_name()
