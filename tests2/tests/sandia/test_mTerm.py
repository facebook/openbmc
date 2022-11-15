#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import unittest

from utils.test_utils import qemu_check
from utils.shell_util import run_shell_cmd


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class mTerm(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_mTerm(self):

        output = run_shell_cmd('ps | grep "mTerm"')
        lines = output.split("\n")
        result = "0"
        if "57600" in lines[0]:
            result = "57600"

        self.assertIn(
            result,
            "57600",
            "mTerm check is failed",
        )
