# Copyright 2004-present Facebook. All Rights Reserved.
from __future__ import unicode_literals
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from fsc_base_tester import BaseFscdUnitTest
import logging

class FscdBmcMachineUnitTest(BaseFscdUnitTest):
    # Tests data parsing from util works as expected for sensors and fans
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

        #TODO : Handle sysfs test

#TODO : Handle write tests
