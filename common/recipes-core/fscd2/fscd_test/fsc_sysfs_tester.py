# Copyright 2004-present Facebook. All Rights Reserved.
from __future__ import unicode_literals
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from fsc_base_tester import BaseFscdUnitTest
import logging

TEST_CONFIG = "./test-data/config-example-test-sysfs.json"


class FscdSysfsTester(BaseFscdUnitTest):

    def define_fscd(self):
        super(FscdSysfsTester, self).define_fscd(config=TEST_CONFIG)

    def test_sensor_read(self):
        '''
        Test if sensor tuples are getting built.
        '''
        tuples = self.fscd_tester.machine.read_sensors(
            self.fscd_tester.sensors)
        count = len(tuples['wedge100'])
        self.assertEqual(count, 3, 'Incorrect sensor tupple count')

    def test_fan_read(self):
        '''
        Test if fan tuples are getting built.
        'fan 2' has a source that reads a file and dumps data like from
        platform.
        '''
        tuples = self.fscd_tester.machine.read_fans(
            fans=self.fscd_tester.fans)
        count = len(tuples)
        self.assertEqual(count, 10, 'Incorrect sensor tupple count')

    def test_fsc_safe_guards(self):
        '''
        Test fsc safe guards
        Examples: Triggers board action when sensor temp read reaches limits
        configured in json
        '''
        self.fscd_tester.update_zones(dead_fans=set(), time_difference=2)
        # Observe on console : "WARNING: Host needs action shutdown
        # and cause chip_temp(v=20) limit(t=10) reached
