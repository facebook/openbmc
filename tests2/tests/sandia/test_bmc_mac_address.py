#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import unittest

from utils.test_utils import qemu_check
from utils.shell_util import run_shell_cmd


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class bmc_mac_address(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_bmc_mac_address(self):

        eth0_mac_address = run_shell_cmd("cat /sys/class/net/eth0/address")

        output = run_shell_cmd("weutil")
        line = output.split("\n")
        weutil_bmc_mac = 0

        for i in range(len(line)):
            if "Local MAC" in line[i]:
                weutil_bmc_mac = line[i].split("Local MAC:")[1]
                break

        self.assertIn(
            eth0_mac_address.strip("\n"),
            weutil_bmc_mac.lstrip(),
            "Bmc mac address check is failed",
        )
