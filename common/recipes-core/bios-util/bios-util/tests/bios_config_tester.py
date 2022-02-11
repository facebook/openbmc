# Copyright 2004-present Facebook. All Rights Reserved.

from bios_base_tester import BaseBiosUtilUnitTest


class BiosUtilConfigUnitTest(BaseBiosUtilUnitTest):
    # Following are config related tests:

    def test_bios_support_config(self):
        """
        Test bios supports object values against config values
        """
        self.assertTrue(
            self.bios_tester.boot_mode,
            "Incorrect boot_mode config: (expected, actual) = (true, false)",
        )
        self.assertTrue(
            self.bios_tester.clear_cmos,
            "Incorrect clear_cmos config: (expected, actual) = (true, false)",
        )
        self.assertTrue(
            self.bios_tester.force_boot_bios_setup,
            "Incorrect force_boot_bios_setup config: (expected, actual) = (true, false)",
        )
        self.assertTrue(
            self.bios_tester.boot_order,
            "Incorrect boot_order config: (expected, actual) = (true, false)",
        )
        self.assertTrue(
            self.bios_tester.plat_info,
            "Incorrect plat_info config: (expected, actual) = (true, false)",
        )
        self.assertTrue(
            self.bios_tester.pcie_port_config,
            "Incorrect pcie_port_config config: (expected, actual) = (true, false)",
        )
        self.assertTrue(
            self.bios_tester.postcode,
            "Incorrect postcode config: (expected, actual) = (true, false)",
        )

        expected_result = ["scc_ioc", "flash1", "flash2", "nic"]
        self.assertEqual(self.bios_tester.pcie_ports, expected_result)
