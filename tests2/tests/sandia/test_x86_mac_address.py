#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import unittest

from utils.test_utils import qemu_check
from utils.shell_util import run_shell_cmd


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class x86_mac_address(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_x86_mac_address(self):

        wedge_us_mac = run_shell_cmd("wedge_us_mac.sh")

        output = run_shell_cmd("weutil")
        line = output.split("\n")
        weutil_x86_mac = 0

        for i in range(len(line)):
            if "Extended MAC Base" in line[i]:
                weutil_x86_mac = line[i].split("Extended MAC Base:")[1]
                break

        self.assertIn(
            wedge_us_mac.strip("\n"),
            weutil_x86_mac.lstrip(),
            "x86 mac address check is failed",
        )
