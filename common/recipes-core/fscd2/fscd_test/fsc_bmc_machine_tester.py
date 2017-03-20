# Copyright 2004-present Facebook. All Rights Reserved.
from __future__ import unicode_literals
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from fsc_base_tester import BaseFscdUnitTest
import logging
from subprocess import Popen, PIPE


class FscdBmcMachineUnitTest(BaseFscdUnitTest):
    # Tests data parsing from util works as expected for sensors and fans
    # Current class uses the same config as config_suite

    def test_sensor_read(self):
        pass

    def test_fan_read(self):
        pass


class FscdBmcMachineUnitTest(FscdBmcMachineUnitTest):

    def test_sensor_read(self):
        '''
        Test if sensor tuples are getting built.
        'linear_dimm' has a source that reads a file and dumps data like from
        platform.
        '''
        tuples = self.fscd_tester.machine.read_sensors(
            self.fscd_tester.sensors)
        count = len(tuples['slot1'])
        self.assertEqual(count, 41, 'Incorrect sensor tupple count')
        # logging.info(tuples)

    def test_fan_read(self):
        '''
        Test if fan tuples are getting built.
        'fan 2' has a source that reads a file and dumps data like from
        platform.
        '''
        tuples = self.fscd_tester.machine.read_fans(
            self.fscd_tester.fans)
        count = len(tuples)
        self.assertEqual(count, 2, 'Incorrect fan tupple count')


class FscdBmcMachineUnitTest2(FscdBmcMachineUnitTest):
    # This class picks a new config and tests sensor and fan read
    # to incorporate different types of fan data outputs while using util
    # across different platforms.
    # Example: Lightning fan-util retunrs front and rear fans while other
    # platforms return just fan data
    REAR_FAN_TEST_CONFIG = "./test-data/config-example-test-util-with-rear-fan1.json"

    def define_fscd(self):
        super(FscdBmcMachineUnitTest2, self).define_fscd(config=self.REAR_FAN_TEST_CONFIG)

    def test_sensor_read(self):
        '''
        Test if sensor tuples are getting built.
        'linear_dimm' has a source that reads a file and dumps data like from
        platform.
        '''
        tuples = self.fscd_tester.machine.read_sensors(
            self.fscd_tester.sensors)
        count = len(tuples['slot1'])
        self.assertEqual(count, 41, 'Incorrect sensor tupple count')
        # logging.info(tuples)

    def test_fan_read(self):
        '''
        Test if fan tuples are getting built.
        'fan 2' has a source that reads a file and dumps data like from
        platform.
        '''
        tuples = self.fscd_tester.machine.read_fans(
            self.fscd_tester.fans)
        count = len(tuples)
        logging.info(tuples)
        self.assertEqual(count, 8, 'Incorrect fan tupple count')

    def test_fan_write_all(self):
        '''
        Test pwn set is working by writing to a file.
        '''
        self.fscd_tester.machine.set_all_pwm(self.fscd_tester.fans, 20)
        cmd = 'cat ./test-data/test-util-data/fan0-set-data'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        self.assertEqual(int(data), 20, 'Fan0 PWM (expected value=20)\
                         (returned=%d)' % int(data))

        cmd = 'cat ./test-data/test-util-data/fan1-set-data'
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        self.assertEqual(int(data), 20, 'Fan2 PWM (expected value=20)\
                         (returned=%d)' % int(data))
