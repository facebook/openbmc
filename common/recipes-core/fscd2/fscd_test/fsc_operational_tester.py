# Copyright 2004-present Facebook. All Rights Reserved.
from __future__ import unicode_literals
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from fsc_base_tester import BaseFscdUnitTest
import logging

BAD_TEST_CONFIG = "./test-data/config-example-fan-dead-test.json"


class FscdOperationalTest(BaseFscdUnitTest):

    def define_fscd(self):
        super(FscdOperationalTest, self).define_fscd(config=BAD_TEST_CONFIG)

    def test_dead_fan(self):
        fan_data = self.fscd_tester.update_dead_fans(dead_fans=set())
        self.assertEqual(fan_data.pop(), 1, 'Incorrectly identified bad fan')

    def test_chassis_intrusion(self):
        self.assertEqual(self.fscd_tester.chassis_intrusion, True,
                         'Incorrect chassis intrusion state for this config')

        # Intention for the following test is to verify the callout code path
        # for chasiss intrusion handling. If all good console log has
        # "WARNING: Need to perform callout action chassis_intrusion_callout"
        try:
            self.fscd_tester.update_zones(dead_fans=set(), time_difference=2)
        except Exception as ex:
            # Hack to test for exception regex since its supported only > 2.7
            if str(ex) in "Write failed with response=":
                self.assertEqual(1, 1)
            else:
                self.assertEqual(0, 1)

    def test_fan_power(self):
        self.assertEqual(self.fscd_tester.fanpower, True,
                         'Incorrect fanpower state for this config')

        # Intention for the following test is to verify the callout code path
        # for fanpower handling. If all good console log has
        # "WARNING: Need to perform callout action fanpower"
        val = self.fscd_tester.get_fan_power_status()
        self.assertEqual(val, 0, "Incorrect fanpower status return")
