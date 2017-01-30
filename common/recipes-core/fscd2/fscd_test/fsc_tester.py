# Copyright 2004-present Facebook. All Rights Reserved.
from __future__ import unicode_literals
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import unittest
from fsc_bmc_machine_tester import FscdBmcMachineUnitTest
from fsc_config_tester import FscdConfigUnitTest
from fsc_base_tester import Result
from fsc_operational_tester import FscdOperationalTest


def config_suite():
    """
    Gather all the tests from configuration related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(FscdConfigUnitTest('test_config_params'))
    test_suite.addTest(FscdConfigUnitTest('test_zone_config'))
    test_suite.addTest(FscdConfigUnitTest('test_fan_config'))
    test_suite.addTest(FscdConfigUnitTest('test_fan_zone_config'))
    test_suite.addTest(FscdConfigUnitTest('test_profile_config'))
    return test_suite


def bmc_machine_suite():
    """
    Gather all the tests from BMC Machine related tests in a test suite.
    """
    test_suite = unittest.TestSuite()
    test_suite.addTest(FscdBmcMachineUnitTest('test_sensor_read'))
    test_suite.addTest(FscdBmcMachineUnitTest('test_fan_read'))
    return test_suite


def operational_suite():
    test_suite = unittest.TestSuite()
    test_suite.addTest(FscdOperationalTest('test_dead_fan'))
    test_suite.addTest(FscdOperationalTest('test_chassis_intrusion'))
    test_suite.addTest(FscdOperationalTest('test_fan_power'))
    return test_suite


def print_result(testResult):
    print('\x1b[6;30;32m' + 'Total Tests=' + str(testResult.testsRun) +
          '\x1b[6;30;31m' + ' Failed Tests=' + str(testResult.failures) +
          '\x1b[0m')


if __name__ == '__main__':
    # config tests
    testResult = Result()
    suite1 = config_suite()
    suite1.run(testResult)

    # bmc_machine reads/writes tests
    suite2 = bmc_machine_suite()
    suite2.run(testResult)

    # operational tests
    suite3 = operational_suite()
    suite3.run(testResult)

    print_result(testResult)
