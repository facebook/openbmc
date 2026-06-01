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

from common.base_kv_test import BaseKvTest
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class KvTest(BaseKvTest, unittest.TestCase):
    def set_kv_keys(self):
        # We should only expect to have kv in python env on BMC, but we can't put
        # these imports at the top level as they'd break test discovery (P2206870769)
        from kv import FPERSIST, kv_get

        # used kv_get in binary mode,
        # because some kv keep the data as binary
        try:
            product_name = kv_get(
                "sys_config/fru1_cpu0_product_name", flags=FPERSIST, binary=True
            )
        except Exception as e:
            self.fail(
                f"Failed to read 'sys_config/fru1_cpu0_product_name' from KV, error: {e}"
            )

        # Common keys for both Intel and AMD
        self.kv_keys = [
            "fru1_restart_cause",
            "server_boot_order",
            "server_cpu_ppin",
            "sysfw_ver_server",
            "sys_config/fru1_cpu0_basic_info",
            "sys_config/fru1_cpu0_product_name",
            "sys_config/fru1_dimm0_location",
            "sys_config/fru1_dimm0_manufacturer_id",
            "sys_config/fru1_dimm0_part_name",
            "sys_config/fru1_dimm0_serial_num",
            "sys_config/fru1_dimm0_speed",
            "sys_config/fru1_dimm0_type",
        ]

        if b"AMD" in product_name:
            self.kv_keys.extend(
                [
                    "sys_config/fru1_dimm1_location",
                    "sys_config/fru1_dimm1_manufacturer_id",
                    "sys_config/fru1_dimm1_part_name",
                    "sys_config/fru1_dimm1_serial_num",
                    "sys_config/fru1_dimm1_speed",
                    "sys_config/fru1_dimm1_type",
                ]
            )
        elif b"Intel(R)" in product_name:
            self.kv_keys.extend(
                [
                    "sys_config/fru1_cpu0_micro_code",
                    "sys_config/fru1_cpu0_turbo_mode",
                    "sys_config/fru1_cpu0_type",
                    "sys_config/fru1_dimm2_location",
                    "sys_config/fru1_dimm2_manufacturer_id",
                    "sys_config/fru1_dimm2_part_name",
                    "sys_config/fru1_dimm2_serial_num",
                    "sys_config/fru1_dimm2_speed",
                    "sys_config/fru1_dimm2_type",
                    "sys_config/fru1_dimm4_location",
                    "sys_config/fru1_dimm4_speed",
                    "sys_config/fru1_dimm4_type",
                    "sys_config/fru1_dimm6_location",
                    "sys_config/fru1_dimm6_speed",
                    "sys_config/fru1_dimm6_type",
                    "sys_config/fru1_dimm8_location",
                    "sys_config/fru1_dimm8_speed",
                    "sys_config/fru1_dimm8_type",
                    "sys_config/fru1_dimm10_location",
                    "sys_config/fru1_dimm10_speed",
                    "sys_config/fru1_dimm10_type",
                ]
            )
        else:
            self.fail(
                f"Unknown CPU type: '{product_name.decode(errors='ignore')}'. Cannot determine which KV keys to test."
            )

    def test_kv_get_key(self):
        # We should only expect to have kv in python env on BMC, but we can't put
        # these imports at the top level as they'd break test discovery (P2206870769)
        from kv import FPERSIST, kv_get

        self.set_kv_keys()
        self.assertNotEqual(self.kv_keys, None, "Expected set of kv keys not set")

        for key in self.kv_keys:
            with self.subTest(key=key):
                # used kv_get in binary mode,
                # because some kv keep the data as binary
                value = kv_get(key, flags=FPERSIST, binary=True)
                self.assertNotEqual(
                    len(value), 0, " kv length = 0 [key={}]".format(key)
                )
