#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.

# This software may be used and distributed according to the terms of the
# GNU General Public License version 2.

import unittest

from utils.test_utils import qemu_check
from utils.shell_util import run_shell_cmd


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class Weutil(unittest.TestCase):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    # flake8: noqa: C901
    def test_weutil(self):

        output = run_shell_cmd("weutil")
        line = output.split("\n")
        result = True

        for i in range(len(line)):
            if "CRC8:" in line[i]:
                field = line[i].split("CRC8:")[1]
                if field is None:
                    result = False
                    break
            if "Local MAC:" in line[i]:
                field = line[i].split("Local MAC:")[1]
                if field is None:
                    result = False
                    break
            if "Product Name:" in line[i]:
                field = line[i].split("Product Name:")[1]
                if field is None:
                    result = False
                    break
            if "Product Part Number:" in line[i]:
                field = line[i].split("Product Part Number:")[1]
                if field is None:
                    result = False
                    break
            if "Product Asset Tag:" in line[i]:
                field = line[i].split("Product Asset Tag:")[1]
                if field is None:
                    result = False
                    break
            if "Product Serial Number:" in line[i]:
                field = line[i].split("Product Serial Number:")[1]
                if field is None:
                    result = False
                    break
            if "Product Sub-Version:" in line[i]:
                field = line[i].split("Product Sub-Version:")[1]
                if field is None:
                    result = False
                    break
            if "Product Version:" in line[i]:
                field = line[i].split("Product Version:")[1]
                if field is None:
                    result = False
                    break

        if result is False:
            print("The follwing field is empty")
            print(line[i])

        self.assertEqual(
            result,
            True,
            "weutil check is failed ",
        )
