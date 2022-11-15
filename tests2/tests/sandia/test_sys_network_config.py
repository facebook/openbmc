#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import unittest

from utils.test_utils import qemu_check
from utils.shell_util import run_shell_cmd


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class sys_networkd_config(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_pid(self):
        output = run_shell_cmd("weutil")
        line = output.split("\n")

        for i in range(len(line)):
            if "Product Name:" in line[i]:
                field = line[i].split("Product Name:")[1]

        output = run_shell_cmd("grep -w model /etc/systemd/network/10-eth0.network")
        line = output.split(":")
        for i in range(len(line)):
            if "model" in line[i]:
                pid = line[i].split("=")[1]

        self.assertEqual(
            field.lstrip(),
            pid,
            "pid config in sys-networkd is failed ",
        )

    def test_serial_num(self):
        output = run_shell_cmd("weutil")
        line = output.split("\n")

        for i in range(len(line)):
            if "Product Serial Number:" in line[i]:
                field = line[i].split("Product Serial Number:")[1]

        output = run_shell_cmd("grep -w model /etc/systemd/network/10-eth0.network")
        line = output.split(":")
        for i in range(len(line)):
            if "serial" in line[i]:
                serial_num = line[i].split("=")[1]

        self.assertEqual(
            field.lstrip(),
            serial_num.strip("\n"),
            "serial number config in sys-networkdis failed ",
        )
