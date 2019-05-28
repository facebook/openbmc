# Copyright 2004-present Facebook. All Rights Reserved.

import unittest

from bios_base_tester import captured_output
from bios_board import bios_main_fru
from bios_boot_order import *


class BiosUtilBootOrderUnitTest(unittest.TestCase):
    # Following are boot_order option related tests:

    """
    Test Boot order response data is match in expectation
    """
    # ===== Get===== #
    # boot_mode
    def test_get_bios_boot_mode(self):

        get_boot_order_result = [0x0, 0x0, 0x9, 0x2, 0x3, 0x4]
        with captured_output() as (out, err):
            do_boot_order_action("get", "--boot_mode", "", get_boot_order_result)
        output = out.getvalue().strip()
        self.assertEqual(output, "Boot Mode: Legacy")

        get_boot_order_result = [0x1, 0x0, 0x9, 0x2, 0x3, 0x4]
        with captured_output() as (out, err):
            do_boot_order_action("get", "--boot_mode", "", get_boot_order_result)
        output = out.getvalue().strip()
        self.assertEqual(output, "Boot Mode: UEFI")

    # clear_CMOS
    def test_get_bios_clear_cmos(self):

        get_boot_order_result = [0x1, 0x0, 0x9, 0x2, 0x3, 0x4]
        with captured_output() as (out, err):
            do_boot_order_action("get", "--clear_CMOS", "", get_boot_order_result)
        output = out.getvalue().strip()
        self.assertEqual(output, "Clear CMOS Function: Disabled")

        get_boot_order_result = [0x83, 0x0, 0x9, 0x2, 0x3, 0x4]
        with captured_output() as (out, err):
            do_boot_order_action("get", "--clear_CMOS", "", get_boot_order_result)
        output = out.getvalue().strip()
        self.assertEqual(output, "Clear CMOS Function: Enabled")

    # force_boot_BIOS_setup
    def test_get_bios_force_boot_BIOS_setup(self):
        get_boot_order_result = [0x1, 0x0, 0x9, 0x2, 0x3, 0x4]
        with captured_output() as (out, err):
            do_boot_order_action(
                "get", "--force_boot_BIOS_setup", "", get_boot_order_result
            )
        output = out.getvalue().strip()
        self.assertEqual(output, "Force Boot to BIOS Setup Function: Disabled")

        get_boot_order_result = [0x85, 0x0, 0x9, 0x2, 0x3, 0x4]
        with captured_output() as (out, err):
            do_boot_order_action(
                "get", "--force_boot_BIOS_setup", "", get_boot_order_result
            )
        output = out.getvalue().strip()
        self.assertEqual(output, "Force Boot to BIOS Setup Function: Enabled")

    # boot_order
    def test_get_bios_boot_order(self):
        get_boot_order_result = [0x1, 0x0, 0x9, 0x2, 0x3, 0x4]
        with captured_output() as (out, err):
            do_boot_order_action("get", "--boot_order", "", get_boot_order_result)
        output = out.getvalue().strip()
        self.assertEqual(
            output,
            "Boot Order: USB Device, IPv6 Network, SATA HDD, SATA-CDROM, Other Removable Device",
        )

    # ===== Enable ===== #
    # clear_CMOS
    def test_enable_bios_clear_cmos(self):
        actual_result = [""]
        data = [0x3, 0x0, 0x9, 0x2, 0x3, 0x4]
        expect_result = [0x83, 0x0, 0x9, 0x2, 0x3, 0x4]
        boot_flags_valid = 1
        actual_result = get_boot_order_req_data(data, boot_flags_valid)

        self.assertEqual(actual_result, expect_result)

    # force_boot_BIOS_setup
    def test_enable_force_boot_BIOS_setup(self):
        actual_result = [""]
        data = [0x5, 0x0, 0x9, 0x2, 0x3, 0x4]
        expect_result = [0x85, 0x0, 0x9, 0x2, 0x3, 0x4]
        boot_flags_valid = 1
        actual_result = get_boot_order_req_data(data, boot_flags_valid)

        self.assertEqual(actual_result, expect_result)

    # ===== Disable ===== #
    # clear_CMOS
    def test_disable_bios_clear_cmos(self):
        actual_result = [""]
        data = [0x83, 0x0, 0x9, 0x2, 0x3, 0x4]
        expect_result = [0x3, 0x0, 0x9, 0x2, 0x3, 0x4]
        boot_flags_valid = 0
        actual_result = get_boot_order_req_data(data, boot_flags_valid)

        self.assertEqual(actual_result, expect_result)

    # force_boot_BIOS_setup
    def test_disable_force_boot_BIOS_setup(self):
        actual_result = [""]
        data = [0x85, 0x0, 0x9, 0x2, 0x3, 0x4]
        expect_result = [0x5, 0x0, 0x9, 0x2, 0x3, 0x4]
        boot_flags_valid = 0
        actual_result = get_boot_order_req_data(data, boot_flags_valid)

        self.assertEqual(actual_result, expect_result)

    # boot_order
    def test_disable_boot_order(self):
        actual_result = [""]
        data = [0x81, 0x0, 0x9, 0x2, 0x3, 0x4]
        expect_result = [0x1, 0x0, 0x9, 0x2, 0x3, 0x4]
        boot_flags_valid = 0
        actual_result = get_boot_order_req_data(data, boot_flags_valid)

        self.assertEqual(actual_result, expect_result)

    # ===== Set ===== #
    # boot_mode
    def test_set_bios_boot_mode(self):
        # test to set Legacy
        actual_result = [""]
        data = [0x0, 0x0, 0x1, 0x2, 0x3, 0x4]
        expect_result = [0x80, 0x0, 0x1, 0x2, 0x3, 0x4]
        boot_flags_valid = 1
        actual_result = get_boot_order_req_data(data, boot_flags_valid)
        self.assertEqual(actual_result, expect_result)

        # Check the bios boot_order value
        with captured_output() as (out, err):
            do_boot_order_action("get", "--boot_mode", "", data)
        output = out.getvalue().strip()
        self.assertEqual(output, "Boot Mode: Legacy")

        # test to set UEFI
        actual_result = [""]
        data = [0x1, 0x0, 0x1, 0x2, 0x3, 0x4]
        expect_result = [0x81, 0x0, 0x1, 0x2, 0x3, 0x4]
        boot_flags_valid = 1
        actual_result = get_boot_order_req_data(data, boot_flags_valid)
        self.assertEqual(actual_result, expect_result)

        # Check the bios boot_order value
        with captured_output() as (out, err):
            do_boot_order_action("get", "--boot_mode", "", data)
        output = out.getvalue().strip()
        self.assertEqual(output, "Boot Mode: UEFI")

    # boot_order
    def test_set_bios_boot_order(self):
        actual_result = [""]
        data = [0x81, 0x0, 0x1, 0x2, 0x3, 0x4]
        expect_result = [0x81, 0x0, 0x1, 0x2, 0x3, 0x4]
        boot_flags_valid = 1
        actual_result = get_boot_order_req_data(data, boot_flags_valid)
        self.assertEqual(actual_result, expect_result)

        # Check the bios boot_order value
        with captured_output() as (out, err):
            do_boot_order_action("get", "--boot_order", "", data)
        output = out.getvalue().strip()
        self.assertEqual(
            output,
            "Boot Order: USB Device, IPv4 Network, SATA HDD, SATA-CDROM, Other Removable Device",
        )
