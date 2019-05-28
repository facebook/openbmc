# Copyright 2004-present Facebook. All Rights Reserved.

import unittest

from bios_base_tester import captured_output
from bios_plat_info import *


TEST_PLAT_INFO_RESULT = "./test-data/test-plat-info-sku"
TEST_PCIE_CONFIG_RESULT = "./test-data/test-pcie-config-sku"


class BiosUtilPlatInfoUnitTest(unittest.TestCase):
    # Following are plat_info option related tests:

    """
    Test Platform Info response data is match in expectation
    """

    def test_bios_plat_info(self, sku, cmd):
        if os.path.isfile(TEST_PLAT_INFO_RESULT + str(sku)):
            plat_info_result = open(TEST_PLAT_INFO_RESULT + str(sku), "r")
        else:
            plat_info_result = "\n"

        with captured_output() as (out, err):
            self.test_plat_info = do_plat_info_action(cmd)
        output = out.getvalue().strip()
        self.assertEqual(output, plat_info_result.read().strip())

    def test_bios_plat_info_sku1(self):
        # SKU 1
        # [1]   Presense: Present
        # [0]   Non Test Board
        # [010] SKU: Triton-Type 5A (Left sub-system)
        # [001] Slot Index: 1
        self.test_bios_plat_info(1, 0x91)

    def test_bios_plat_info_sku2(self):
        # SKU 2
        # [0]   Presense: Not Present
        # [1]   Test Board
        # [011] SKU: Triton-Type 5B (Right sub-system)
        # [010] Slot Index: 2

        self.test_bios_plat_info(2, 0x5A)

    def test_bios_plat_info_sku3(self):
        # SKU 3
        # [1]   Presense: Present
        # [0]   Non Test Board
        # [100] SKU: Triton-Type 7 SS (IOC based IOM)
        # [011] Slot Index: 3

        self.test_bios_plat_info(3, 0xA3)

    def test_bios_plat_info_sku4(self):
        # SKU 4
        # [0]  Presense: Not Present
        # [1]  Test Board
        # [000] SKU: Yosemite
        # [100] Slot Index: 2

        self.test_bios_plat_info(4, 0x44)


class BiosUtilPcieConfigUnitTest(unittest.TestCase):
    # Following are pcie_config option related tests:

    """
    Test PCIe config response data is match in expectation
    """

    def test_bios_pcie_config(self, sku, result):
        if os.path.isfile(TEST_PCIE_CONFIG_RESULT + str(sku)):
            plat_info_result = open(TEST_PCIE_CONFIG_RESULT + str(sku), "r")
        else:
            plat_info_result = "\n"

        self.test_plat_info = do_pcie_config_action(result)
        self.assertEqual(self.test_plat_info, plat_info_result.read().strip())

    def test_bios_pcie_config_sku1(self):
        # SKU 1: Triton-Type 5
        result = ["06"]
        self.test_bios_pcie_config(1, result)

    def test_bios_pcie_config_sku2(self):
        # SKU 2: Triton-Type 7
        result = ["08"]
        self.test_bios_pcie_config(2, result)

    def test_bios_pcie_config_sku3(self):
        # SKU 3: Yosemite
        result = ["0A"]
        self.test_bios_pcie_config(3, result)

    def test_bios_pcie_config_sku4(self):
        # SKU 4: Unknown
        result = ["00"]
        self.test_bios_pcie_config(4, result)
