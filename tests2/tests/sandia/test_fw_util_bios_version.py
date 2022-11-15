#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import unittest

from utils.test_utils import qemu_check
from utils.shell_util import run_shell_cmd


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class fw_util_bios_version(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_fw_util_bios_version(self):

        output = run_shell_cmd("fw_util bios version")
        line = output.split("\n")
        result = False
        for i in range(len(line)):
            if "BIOS:" in line[i]:
                result = line[i].split("BIOS:")[1]
                break
        try:
            float(result)
            result = True
        except ValueError:
            result = False

        self.assertEqual(
            result,
            True,
            "fw_util bios version check is failed",
        )
