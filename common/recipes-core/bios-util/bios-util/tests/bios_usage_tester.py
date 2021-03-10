# Copyright 2004-present Facebook. All Rights Reserved.

import unittest

from bios_base_tester import (
    TEST_BIOS_UTIL_CONFIG,
    TEST_BIOS_UTIL_DEFAULT_CONFIG,
    captured_output,
)
from bios_board import *
from bios_boot_order import *


TEST_BIOS_UTIL_USAGE_ALL = "./test-data/test-usage-all"
TEST_BIOS_UTIL_USAGE_BOOT_ORDER_ALL = "./test-data/test-usage-boot-order-all"
TEST_BIOS_UTIL_USAGE_BOOT_ORDER_SET = "./test-data/test-usage-boot-order-set"
TEST_BIOS_UTIL_USAGE_BOOT_ORDER_GET = "./test-data/test-usage-boot-order-get"
TEST_BIOS_UTIL_USAGE_BOOT_ORDER_ENABLE = "./test-data/test-usage-boot-order-enable"
TEST_BIOS_UTIL_USAGE_BOOT_ORDER_DIABLE = "./test-data/test-usage-boot-order-disable"


def test_check_biosutil_usage(self, argv, test_result_file):
    with captured_output() as (out, err):
        check_bios_util(TEST_BIOS_UTIL_CONFIG, TEST_BIOS_UTIL_DEFAULT_CONFIG, argv)
    output = out.getvalue().strip()

    usage_expected_result = open(test_result_file, "r").read().strip()
    # print(usage_expected_result)  # Reserved for debugging

    self.assertEqual(output, usage_expected_result)


class BiosUtilUsageUnitTest(unittest.TestCase):
    def test_biosutil_usage_all(self):
        # All
        argv = ["", "FRU"]
        test_check_biosutil_usage(self, argv, TEST_BIOS_UTIL_USAGE_ALL)

    def test_biosutil_usage_boot_order_set(self):
        # boot_order "set" action
        argv = ["", "FRU", "--boot_order", "set"]
        test_check_biosutil_usage(self, argv, TEST_BIOS_UTIL_USAGE_BOOT_ORDER_SET)

    def test_biosutil_usage_boot_order_get(self):
        # boot_order "get" action
        argv = ["", "FRU", "--boot_order", "get"]
        test_check_biosutil_usage(self, argv, TEST_BIOS_UTIL_USAGE_BOOT_ORDER_GET)

    def test_biosutil_usage_boot_order_enable(self):
        # boot_order "set" action
        argv = ["", "FRU", "--boot_order", "enable"]
        test_check_biosutil_usage(self, argv, TEST_BIOS_UTIL_USAGE_BOOT_ORDER_ENABLE)

    def test_biosutil_usage_boot_order_disable(self):
        # boot_order "set" action
        argv = ["", "FRU", "--boot_order", "disable"]
        test_check_biosutil_usage(self, argv, TEST_BIOS_UTIL_USAGE_BOOT_ORDER_DIABLE)

    def test_biosutil_usage_posecode(self):
        # boot_order "set" action
        argv = ["", "FRU", "--boot_order", "disable"]
        test_check_biosutil_usage(self, argv, TEST_BIOS_UTIL_USAGE_BOOT_ORDER_DIABLE)
