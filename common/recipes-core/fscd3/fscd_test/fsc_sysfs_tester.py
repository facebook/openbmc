# Copyright 2004-present Facebook. All Rights Reserved.

import logging
from subprocess import PIPE, Popen

from fsc_base_tester import BaseFscdUnitTest


TEST_CONFIG = "./test-data/config-example-test-sysfs.json"


class FscdSysfsTester(BaseFscdUnitTest):
    def define_fscd(self):
        super(FscdSysfsTester, self).define_fscd(config=TEST_CONFIG)

    def test_sensor_read(self):
        """
        Test if sensor tuples are getting built.
        """
        tuples = self.fscd_tester.machine.read_sensors(self.fscd_tester.sensors)
        count = len(tuples["wedge100"])
        self.assertEqual(count, 3, "Incorrect sensor tupple count")

    def test_fan_read(self):
        """
        Test if fan tuples are getting built.
        'fan 2' has a source that reads a file and dumps data like from
        platform.
        """
        tuples = self.fscd_tester.machine.read_fans(fans=self.fscd_tester.fans)
        count = len(tuples)
        self.assertEqual(count, 10, "Incorrect sensor tupple count")

    def test_fsc_safe_guards(self):
        """
        Test fsc safe guards
        Examples: Triggers board action when sensor temp read reaches limits
        configured in json
        """
        self.fscd_tester.update_zones(dead_fans=set(), time_difference=2)
        # Observe on console : "WARNING: Host needs action shutdown
        # and cause chip_temp(v=20) limit(t=10) reached


class FscdSysfsOperationalTester(BaseFscdUnitTest):
    def define_fscd(self):
        super(FscdSysfsOperationalTester, self).define_fscd(config=TEST_CONFIG)

    def test_fscd_operation(self):
        # Replicating the way fscd will read and update zones
        # to check in each iteration if fan speeds
        # are being set the way we expect
        self.fscd_tester.builder()

        # Test number of tuples built
        tuples = self.fscd_tester.machine.read_sensors(self.fscd_tester.sensors)
        count = len(tuples["wedge100"])
        self.assertEqual(count, 3, "Incorrect sensor tupple count")

        dead_fans = set()
        dead_fans = self.fscd_tester.update_dead_fans(dead_fans)

        # Test that number of dead fans in 0
        self.assertEqual(len(dead_fans), 0, "Incorrect number of dead fans")

        self.fscd_tester.update_zones(dead_fans, 2)

        # In the following cases we set the temperatures to different ranges
        # and check fscd is setting the pwm to the values we expected

        # case 1
        # Lets change the temp that fscd will read:
        cmd1 = "echo 20000 > ./test-data/test-sysfs-data/4-0033/temp1_input"
        cmd2 = "echo 20000 > ./test-data/test-sysfs-data/3-004b/temp1_input"
        cmd3 = "echo 30000 > ./test-data/test-sysfs-data/3-0048/temp1_input"
        cmd = cmd1 + ";" + cmd2 + ";" + cmd3
        Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        self.fscd_tester.update_zones(dead_fans, 2)

        # Check if fscd set the pwm based on above inputs
        pwm_cmd = "cat ./test-data/test-sysfs-data/fantray1_pwm"
        pwm_value = Popen(pwm_cmd, shell=True, stdout=PIPE).stdout.read()
        self.assertEqual(
            int(pwm_value),
            12,
            "Incorrect pwm value was set expected=12 computed=%d" % int(pwm_value),
        )

        # case 2
        # Lets change the temp that fscd will read now to mid range
        cmd1 = "echo 33000 > ./test-data/test-sysfs-data/4-0033/temp1_input"
        cmd2 = "echo 35000 > ./test-data/test-sysfs-data/3-004b/temp1_input"
        cmd3 = "echo 40000 > ./test-data/test-sysfs-data/3-0048/temp1_input"
        cmd = cmd1 + ";" + cmd2 + ";" + cmd3
        Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        self.fscd_tester.update_zones(dead_fans, 2)

        # Check if fscd set the pwm based on above inputs
        pwm_cmd = "cat ./test-data/test-sysfs-data/fantray1_pwm"
        pwm_value = Popen(pwm_cmd, shell=True, stdout=PIPE).stdout.read()
        self.assertEqual(
            int(pwm_value),
            20,
            "Incorrect pwm value was set expected=20 computed=%d" % int(pwm_value),
        )

        # case 3
        # Lets change the temp that fscd will read now to high range
        cmd1 = "echo 50000 > ./test-data/test-sysfs-data/4-0033/temp1_input"
        cmd2 = "echo 20000 > ./test-data/test-sysfs-data/3-004b/temp1_input"
        cmd3 = "echo 30000 > ./test-data/test-sysfs-data/3-0048/temp1_input"
        cmd = cmd1 + ";" + cmd2 + ";" + cmd3
        Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        self.fscd_tester.update_zones(dead_fans, 2)

        # Check if fscd set the pwm based on above inputs
        pwm_cmd = "cat ./test-data/test-sysfs-data/fantray1_pwm"
        pwm_value = Popen(pwm_cmd, shell=True, stdout=PIPE).stdout.read()
        self.assertEqual(
            int(pwm_value),
            28,
            "Incorrect pwm value was set expected=28 computed=%d" % int(pwm_value),
        )

        # case 4
        # Lets change the temp that fscd will read now to higher range
        cmd1 = "echo 50000 > ./test-data/test-sysfs-data/4-0033/temp1_input"
        cmd2 = "echo 90000 > ./test-data/test-sysfs-data/3-004b/temp1_input"
        cmd3 = "echo 33000 > ./test-data/test-sysfs-data/3-0048/temp1_input"
        cmd = cmd1 + ";" + cmd2 + ";" + cmd3
        Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        self.fscd_tester.update_zones(dead_fans, 2)

        # Check if fscd set the pwm based on above inputs
        pwm_cmd = "cat ./test-data/test-sysfs-data/fantray1_pwm"
        pwm_value = Popen(pwm_cmd, shell=True, stdout=PIPE).stdout.read()
        self.assertEqual(
            int(pwm_value),
            28,
            "Incorrect pwm value was set expected=28 computed=%d" % int(pwm_value),
        )

        # reset to original values
        cmd1 = "echo 15000 > ./test-data/test-sysfs-data/4-0033/temp1_input"
        cmd2 = "echo 15000 > ./test-data/test-sysfs-data/3-004b/temp1_input"
        cmd3 = "echo 15000 > ./test-data/test-sysfs-data/3-0048/temp1_input"
        cmd = cmd1 + ";" + cmd2 + ";" + cmd3
        Popen(cmd, shell=True, stdout=PIPE).stdout.read()
