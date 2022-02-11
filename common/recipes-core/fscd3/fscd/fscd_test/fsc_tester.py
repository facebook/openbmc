# Copyright 2004-present Facebook. All Rights Reserved.

import unittest

from fsc_base_tester import Result
from fsc_bmc_machine_tester import FscdBmcMachineUnitTest, FscdBmcMachineUnitTest2
from fsc_config_tester import FscdConfigUnitTest
from fsc_operational_tester import FscdOperationalTest
from fsc_sysfs_tester import FscdSysfsOperationalTester, FscdSysfsTester


def config_suite():
    """
    Gather all the tests from configuration related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(FscdConfigUnitTest("test_config_params"))
    test_suite.addTest(FscdConfigUnitTest("test_zone_config"))
    test_suite.addTest(FscdConfigUnitTest("test_fan_config"))
    test_suite.addTest(FscdConfigUnitTest("test_fan_zone_config"))
    test_suite.addTest(FscdConfigUnitTest("test_profile_config"))
    return test_suite


def bmc_machine_util_suite():
    """
    Gather all the tests from BMC Machine related tests in a test suite.(util)
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(FscdBmcMachineUnitTest("test_sensor_read"))
    test_suite.addTest(FscdBmcMachineUnitTest("test_fan_read"))
    test_suite.addTest(FscdBmcMachineUnitTest2("test_sensor_read"))
    test_suite.addTest(FscdBmcMachineUnitTest2("test_fan_read"))
    test_suite.addTest(FscdBmcMachineUnitTest2("test_fan_write_all"))
    return test_suite


def bmc_machine_sysfs_suite():
    """
    Gather all the tests from BMC Machine related tests in a test suite.(sysfs)
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(FscdSysfsTester("test_sensor_read"))
    test_suite.addTest(FscdSysfsTester("test_fan_read"))
    test_suite.addTest(FscdSysfsTester("test_fsc_safe_guards"))
    return test_suite


def operational_sysfs_suite():
    """
    Gather all the tests from BMC Machine related tests in a test suite.(sysfs)
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(FscdSysfsOperationalTester("test_fscd_operation"))
    return test_suite


def operational_suite():
    """
    Gather all the tests needed for callouts from fsc
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(FscdOperationalTest("test_dead_fan"))
    test_suite.addTest(FscdOperationalTest("test_chassis_intrusion"))
    test_suite.addTest(FscdOperationalTest("test_fan_led"))
    # TODO
    # test_suite.addTest(FscdOperationalTest('test_fan_power'))
    # host shutdown action
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
    # config tests
    testResult = Result()
    suite1 = config_suite()

    # bmc_machine reads/writes tests
    suite2 = bmc_machine_util_suite()

    # operational tests
    suite3 = operational_suite()

    # sysfs tests
    suite4 = bmc_machine_sysfs_suite()

    # operational sysfs tests
    suite5 = operational_sysfs_suite()

    alltests = unittest.TestSuite([suite1, suite2, suite3, suite4, suite5])
    alltests.run(testResult)

    print_result(testResult)
