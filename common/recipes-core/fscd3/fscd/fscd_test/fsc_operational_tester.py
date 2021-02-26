# Copyright 2004-present Facebook. All Rights Reserved.

import logging

from fsc_base_tester import BaseFscdUnitTest


BAD_TEST_CONFIG = "./test-data/config-example-fan-dead-test.json"


class FscdOperationalTest(BaseFscdUnitTest):
    def define_fscd(self):
        super(FscdOperationalTest, self).define_fscd(config=BAD_TEST_CONFIG)

    def test_dead_fan(self):
        # Intention for the following test is to verify the callout code path
        # for dead fan handling is called.. If all good console log has
        # "WARNING: Fan 1 needs action dead
        # WARNING: Fan 1 needs action led_red"
        fan_data = self.fscd_tester.update_dead_fans(dead_fans=set())
        dead_fan = fan_data.pop()
        self.assertEqual(dead_fan.fan_num, 0, "Incorrectly identified bad fan")

    def test_chassis_intrusion(self):
        self.assertEqual(
            self.fscd_tester.chassis_intrusion,
            True,
            "Incorrect chassis intrusion state for this config",
        )

        # Intention for the following test is to verify the callout code path
        # for chasiss intrusion handling. If all good console log has
        # "WARNING: Need to perform callout action chassis_intrusion_callout"
        self.fscd_tester.update_zones(dead_fans=set(), time_difference=2)
        try:
            self.fscd_tester.update_zones(dead_fans=set(), time_difference=2)
        except Exception as ex:
            # Hack to test for exception regex since its supported only > 2.7
            if "Write failed with response=" in str(ex):
                self.assertEqual(1, 1)
            else:
                self.assertEqual(0, 1)

    def test_fan_led(self):
        # Intention for the following test is to verify the callout code path
        # for dead fan handling is called.. If all good console log has
        # "WARNING: Fan 1 needs action led_blue"
        self.fscd_tester.fsc_set_all_fan_led(color="led_blue")
