# Copyright 2004-present Facebook. All Rights Reserved.

import os
import shutil
import unittest

from bios_base_tester import Result
from bios_pcie_port_config_tester import BiosUtilPciePortConfigUnitTest
from bios_plat_info_tester import BiosUtilPcieConfigUnitTest, BiosUtilPlatInfoUnitTest


TEST_BIOS_IPMI_UTIL = "./test-data/test-bios_ipmi_util.py"
TEST_BIOS_IPMI_UTIL_TARGET = "../bios-util/bios_ipmi_util.py"
shutil.copyfile(TEST_BIOS_IPMI_UTIL, TEST_BIOS_IPMI_UTIL_TARGET)


def plat_info_suite():
    """
    Gather all the tests from platform info related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(BiosUtilPlatInfoUnitTest("test_bios_plat_info_sku1"))
    test_suite.addTest(BiosUtilPlatInfoUnitTest("test_bios_plat_info_sku2"))
    test_suite.addTest(BiosUtilPlatInfoUnitTest("test_bios_plat_info_sku3"))
    test_suite.addTest(BiosUtilPlatInfoUnitTest("test_bios_plat_info_sku4"))
    test_suite.addTest(BiosUtilPcieConfigUnitTest("test_bios_pcie_config_sku1"))
    test_suite.addTest(BiosUtilPcieConfigUnitTest("test_bios_pcie_config_sku2"))
    test_suite.addTest(BiosUtilPcieConfigUnitTest("test_bios_pcie_config_sku3"))
    test_suite.addTest(BiosUtilPcieConfigUnitTest("test_bios_pcie_config_sku4"))

    return test_suite


def pcie_port_config_suite():
    """
    Gather all the tests from PCIe port config related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_get_bios_pcie_port_config_sku1")
    )
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_get_bios_pcie_port_config_sku2")
    )
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_get_bios_pcie_port_config_sku3")
    )
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_get_bios_pcie_port_config_sku4")
    )
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_get_bios_pcie_port_config_sku5")
    )
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_get_bios_pcie_port_config_sku6")
    )
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_enable_bios_pcie_port_config_sku1")
    )
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_enable_bios_pcie_port_config_sku2")
    )
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_disable_bios_pcie_port_config_sku1")
    )
    test_suite.addTest(
        BiosUtilPciePortConfigUnitTest("test_disable_bios_pcie_port_config_sku2")
    )

    return test_suite


def print_result(testResult):
    print(("\x1b[6;30;32m" + "Total Tests=" + str(testResult.testsRun)))
    print(("\x1b[6;30;31m" + "Total Failed Tests=" + str(len(testResult.failures))))
    print(("\x1b[6;30;31m" + "Total Errored Tests=" + str(len(testResult.errors))))

    if len(testResult.failures) > 0:
        print(("\x1b[6;30;31m" + "\nDetailed Failed Tests="))
        fail_dct = dict(testResult.failures)
        for key, value in list(fail_dct.items()):
            print(("\x1b[6;30;31m" + "\n " + str(key) + str(value)))

    if len(testResult.errors) > 0:
        print(("\x1b[6;30;31m" + "\nDetailed Errored Tests="))
        errors_dct = dict(testResult.errors)
        for key, value in list(errors_dct.items()):
            print(("\x1b[6;30;31m" + "\n " + str(key) + str(value)))

    print("\x1b[0m")


if __name__ == "__main__":

    testResult = Result()

    # platform info tests
    suite1 = plat_info_suite()

    # PCIe port config tests
    suite2 = pcie_port_config_suite()

    alltests = unittest.TestSuite([suite1, suite2])
    alltests.run(testResult)

    print_result(testResult)

    # Remove test python file
    os.remove(TEST_BIOS_IPMI_UTIL_TARGET)
