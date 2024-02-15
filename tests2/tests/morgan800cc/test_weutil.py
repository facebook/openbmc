#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import unittest
import json

from utils.test_utils import qemu_check
from utils.shell_util import run_shell_cmd


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Weutil(unittest.TestCase):
    def setUp(self):
        weutil_json_string = run_shell_cmd ("weutil --json")
        self.weutil_json = json.loads(weutil_json_string)
        pass

    def tearDown(self):
        pass

    def weutil_check(self, keyToCheck):
        result = False
        if keyToCheck in self.weutil_json and self.weutil_json[keyToCheck]:
            result = True

        return result

    def test_weutil_crc(self):
        self.assertEqual(
            self.weutil_check("CRC16"),
            True,
            "weutil does not contain a valid CRC",
        )

    def test_weutil_product_serial(self):
        self.assertEqual(
            self.weutil_check("Product Serial Number"),
            True,
            "weutil does not contain a valid Product Serial Number",
        )

    def test_weutil_local_mac(self):
        self.assertEqual(
            self.weutil_check("BMC MAC Base"),
            True,
            "weutil does not contain a valid local mac",
        )

    def test_weutil_product_name(self):
        self.assertEqual(
            self.weutil_check("Product Name"),
            True,
            "weutil does not contain a valid CRC",
        )

    def test_weutil_local_asset_tag(self):
        self.assertEqual(
            self.weutil_check("System Assembly Part Number"),
            True,
            "weutil does not contain a valid Asset Tag",
        )

    def test_weutil_part_number(self):
        self.assertEqual(
            self.weutil_check("Product Part Number"),
            True,
            "weutil does not contain a valid Product Part Number",
        )

    def test_weutil_product_version(self):
        self.assertEqual(
            self.weutil_check("Product Version"),
            True,
            "weutil does not contain a valid Product Version",
        )

    def test_weutil_product_sub_version(self):
        self.assertEqual(
            self.weutil_check("Product Sub-Version"),
            True,
            "weutil does not contain a valid Product Sub-Version",
        )

