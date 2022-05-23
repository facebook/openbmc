#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import unittest

import pexpect
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class SolTest(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_sol_execution(self):

        result = False
        child = pexpect.spawn("sol.sh")
        child.expect(b"")
        child.sendline(b"\r\n")

        try:
            index = child.expect([b"Username:", b"RP/0/RP0/CPU0:ios#"])
            if index == 0 or index == 1:
                result = True
        except Exception:
            print("Timed out")

        self.assertTrue(
            result,
            "sol.sh is failed. Received={}".format(child.before),
        )
