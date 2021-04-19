#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
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

from common.base_kv_test import BaseKvTest


class KvTest(BaseKvTest, unittest.TestCase):
    def set_kv_keys(self):
        self.kv_keys = [
            "fan1_sensor_health",
            "fan2_sensor_health",
            "fan3_sensor_health",
            "fan4_sensor_health",
            "fcm_sensor_health",
            "fru1_restart_cause",
            "psu1_sensor_health",
            "psu2_sensor_health",
            "pwr_server_last_state",
            "scm_sensor_health",
            "server_boot_order",
            "server_por_cfg",
            "server_sel_error",
            "smb_sensor_health",
            "sysfw_ver_server",
            "timestamp_sled",
        ]
