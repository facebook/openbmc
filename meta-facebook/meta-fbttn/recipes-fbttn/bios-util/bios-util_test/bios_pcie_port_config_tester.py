# Copyright 2004-present Facebook. All Rights Reserved.

import unittest

from bios_base_tester import captured_output
from bios_pcie_port_config import *


TEST_PCIE_PORT_CONFIG_RESULT = "./test-data/test-pcie-port-config-sku"
TEST_PLATFORM_FILE_T5 = "./test-data/test-system-t5.bin"
TEST_PLATFORM_FILE_T7 = "./test-data/test-system-t7.bin"


class BiosUtilPciePortConfigUnitTest(unittest.TestCase):
    # Following are pcie_port_config option related tests:

    """
    Tests that work for supporting to get/set PCIe port config
    """
    # ===== Get ===== #
    def test_bios_pcie_port_config(
        self, sku, plat_sku_file, function, device, res_data
    ):
        if os.path.isfile(TEST_PCIE_PORT_CONFIG_RESULT + str(sku)):
            pcie_port_config_result = open(TEST_PCIE_PORT_CONFIG_RESULT + str(sku), "r")
        else:
            pcie_port_config_result = "\n"

        with captured_output() as (out, err):
            self.test_pcie_port_config = do_pcie_port_config_action(
                plat_sku_file, function, device, res_data
            )
        output = out.getvalue().strip()
        self.assertEqual(output, pcie_port_config_result.read().strip())

    def test_get_bios_pcie_port_config_sku1(self):
        # SKU 1: type 5
        # SCC IOC: Enabled
        # flash1: Enabled
        # flash2: Enabled
        # NIC: Enabled
        res_data = [0x80, 0x80]

        self.test_bios_pcie_port_config(1, TEST_PLATFORM_FILE_T5, "get", "", res_data)

    def test_get_bios_pcie_port_config_sku2(self):
        # SKU 2: type 5
        # SCC IOC: Disabled
        # flash1: Disabled
        # flash2: Disabled
        # NIC: Disabled
        res_data = [0x83, 0x8F]

        self.test_bios_pcie_port_config(2, TEST_PLATFORM_FILE_T5, "get", "", res_data)

    def test_get_bios_pcie_port_config_sku3(self):
        # SKU 3: type 5
        # * PCIe Port #3 is invalid and will be ignored.
        #   SCC IOC: Unknown
        #   flash1: Unknown
        #   flash2: Unknown
        # * PCIe Port #1 is invalid and will be ignored.
        #   NIC: Unknown
        res_data = [0x03, 0x0F]

        self.test_bios_pcie_port_config(3, TEST_PLATFORM_FILE_T5, "get", "", res_data)

    def test_get_bios_pcie_port_config_sku4(self):
        # SKU 4: type 7
        # SCC IOC: Enabled
        # IOM IOC: Enabled
        # NIC: Enabled
        res_data = [0x80, 0x80]

        self.test_bios_pcie_port_config(4, TEST_PLATFORM_FILE_T7, "get", "", res_data)

    def test_get_bios_pcie_port_config_sku5(self):
        # SKU 5: type 7
        # SCC IOC: Disabled
        # IOM IOC: Disabled
        # NIC: Disabled
        res_data = [0x83, 0x8F]

        self.test_bios_pcie_port_config(5, TEST_PLATFORM_FILE_T7, "get", "", res_data)

    def test_get_bios_pcie_port_config_sku6(self):
        # SKU 6: type 7
        # * PCIe Port #3 is invalid and will be ignored.
        #   SCC IOC: Unknown
        #   IOM IOC: Unknown
        # * PCIe Port #1 is invalid and will be ignored.
        #   NIC: Unknown
        res_data = [0x03, 0x0F]

        self.test_bios_pcie_port_config(6, TEST_PLATFORM_FILE_T7, "get", "", res_data)

    # ===== Enable ===== #
    def test_enable_bios_pcie_port_config_sku1(self):
        # SKU 1: type 5
        # SCC IOC: Enabled
        # flash1: Enabled
        # flash2: Enabled
        # NIC: Enabled
        res_data = [0x00, 0x00]

        plat_sku_file = TEST_PLATFORM_FILE_T5
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "enable", "--scc_ioc", res_data
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "enable", "--flash1", self.test_pcie_port_config
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "enable", "--flash2", self.test_pcie_port_config
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "enable", "--nic", self.test_pcie_port_config
        )

        self.test_bios_pcie_port_config(
            1, plat_sku_file, "get", "", self.test_pcie_port_config
        )

    def test_enable_bios_pcie_port_config_sku2(self):
        # SKU 2: type 7
        # SCC IOC: Enabled
        # IOM IOC: Enabled
        # NIC: Enabled
        res_data = [0x00, 0x00]

        plat_sku_file = TEST_PLATFORM_FILE_T7
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "enable", "--scc_ioc", res_data
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "enable", "--iom_ioc", self.test_pcie_port_config
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "enable", "--nic", self.test_pcie_port_config
        )

        self.test_bios_pcie_port_config(
            4, plat_sku_file, "get", "", self.test_pcie_port_config
        )

    # ===== Disable ===== #
    def test_disable_bios_pcie_port_config_sku1(self):
        # SKU 1: type 5
        # SCC IOC: Disabled
        # flash1: Disabled
        # flash2: Disabled
        # NIC: Disabled
        res_data = [0x00, 0x00]

        plat_sku_file = TEST_PLATFORM_FILE_T5
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "disable", "--scc_ioc", res_data
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "disable", "--flash1", self.test_pcie_port_config
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "disable", "--flash2", self.test_pcie_port_config
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "disable", "--nic", self.test_pcie_port_config
        )

        self.test_bios_pcie_port_config(
            2, plat_sku_file, "get", "", self.test_pcie_port_config
        )

    def test_disable_bios_pcie_port_config_sku2(self):
        # SKU 2: type 7
        # SCC IOC: Disabled
        # IOM IOC: Disabled
        # NIC: Disabled
        res_data = [0x00, 0x00]

        plat_sku_file = TEST_PLATFORM_FILE_T7
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "disable", "--scc_ioc", res_data
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "disable", "--iom_ioc", self.test_pcie_port_config
        )
        self.test_pcie_port_config = do_pcie_port_config_action(
            plat_sku_file, "disable", "--nic", self.test_pcie_port_config
        )

        self.test_bios_pcie_port_config(
            5, plat_sku_file, "get", "", self.test_pcie_port_config
        )
