# Copyright 2004-present Facebook. All Rights Reserved.

import unittest

from bios_base_tester import Result
from bios_boot_order_tester import BiosUtilBootOrderUnitTest
from bios_config_tester import BiosUtilConfigUnitTest
from bios_pcie_port_config_tester import BiosUtilPciePortConfigUnitTest
from bios_plat_info_tester import BiosUtilPlatInfoUnitTest
from bios_postcode_tester import BiosUtilPostCodeUnitTest
from bios_usage_tester import BiosUtilUsageUnitTest


def usage_suite():
    """
    Gather all the tests from utility usage related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(BiosUtilUsageUnitTest("test_biosutil_usage_all"))
    test_suite.addTest(BiosUtilUsageUnitTest("test_biosutil_usage_boot_order_set"))
    test_suite.addTest(BiosUtilUsageUnitTest("test_biosutil_usage_boot_order_get"))
    test_suite.addTest(BiosUtilUsageUnitTest("test_biosutil_usage_boot_order_enable"))
    test_suite.addTest(BiosUtilUsageUnitTest("test_biosutil_usage_boot_order_disable"))
    return test_suite


def config_suite():
    """
    Gather all the tests from configuration related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(BiosUtilConfigUnitTest("test_bios_support_config"))
    return test_suite


def boot_order_suite():
    """
    Gather all the tests from Boot Order related tests in a test suite.
    """
    test_suite = unittest.TestSuite()

    test_suite.addTest(BiosUtilBootOrderUnitTest("test_get_bios_boot_mode"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_get_bios_clear_cmos"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_get_bios_force_boot_BIOS_setup"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_get_bios_boot_order"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_enable_bios_clear_cmos"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_enable_force_boot_BIOS_setup"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_disable_bios_clear_cmos"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_disable_force_boot_BIOS_setup"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_disable_boot_order"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_set_bios_boot_mode"))
    test_suite.addTest(BiosUtilBootOrderUnitTest("test_set_bios_boot_order"))

    return test_suite


def postcode_suite():
    """
    Gather all the tests from BIOS POST code related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(BiosUtilPostCodeUnitTest("test_bios_postcode"))

    return test_suite


def plat_info_suite():
    """
    Gather all the tests from platform info related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(BiosUtilPlatInfoUnitTest("test_bios_plat_info"))

    return test_suite


def pcie_port_config_suite():
    """
    Gather all the tests from PCIe port config related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(BiosUtilPciePortConfigUnitTest("test_bios_pcie_port_config"))

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
    # usage tests
    suite1 = usage_suite()

    # config tests
    suite2 = config_suite()

    # boot_order tests
    suite3 = boot_order_suite()

    # postcode tests
    suite4 = postcode_suite()

    # platform info tests
    suite5 = plat_info_suite()

    # PCIe port config tests
    suite6 = pcie_port_config_suite()

    alltests = unittest.TestSuite([suite1, suite2, suite3, suite4, suite5, suite6])
    alltests.run(testResult)

    print_result(testResult)
