#!/usr/bin/env python3
#
# Copyright 2020-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
import time
import unittest

from common.base_fscd_test import BaseFscdTest
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class FscdTest(BaseFscdTest, unittest.TestCase):

    TEST_DATA_PATH = "/usr/local/bin/tests2/tests/agc032a/test_data/fscd"
    DEFAULT_TEMP = 28000

    def power_host_on(self):
        pass

    def is_host_on(self):
        """
        Method to test if host power is on
        """
        pass

    def get_fan_pwm(self, pwm_val=None):
        """
        In wedge100 there is only 1 zone and all fans belong to it.
        PWM effect applies to all fans. Test if all fans report
        the expected PWM
        """
        self.assertNotEqual(pwm_val, None, "Expected PWM value needs to be set")

        data = run_shell_cmd("/usr/local/bin/get_fan_speed.sh")
        data = data.split("\n")
        for line in data:
            if len(line) == 0:
                continue
            line = line.split("(")
            line = line[1].split("%, ")
            front = line[0]
            line = line[1].split("%")
            rear = line[0]
            # print("front:{}, rear:{}", front, rear)
            if (abs(int(front) - int(pwm_val)) < 2) and (abs(int(rear) - int(pwm_val)) < 2):
                continue
            else:
                return [False, data]
        return [True, None]

    def run_pwm_test(
        self,
        lf_temp=DEFAULT_TEMP,
        rf_temp=DEFAULT_TEMP,
        upper_mac_temp=DEFAULT_TEMP,
        lower_mac_temp=DEFAULT_TEMP,
        expected_pwm=40,
    ):

        PWM_VAL = expected_pwm
        Logger.info(
            "[FSCD Testing] Setting (LF={}C, RF={}C ,"
            "UPPER MAC={}C, LOWER MAC={}C, expected pwm={})".format(
                int(lf_temp) / 1000,
                int(rf_temp) / 1000,
                int(upper_mac_temp) / 1000,
                int(lower_mac_temp) / 1000,
                int(expected_pwm),
            )
        )
        run_shell_cmd(
            "echo {} > {}/lf/temp1_input".format(lf_temp, self.TEST_DATA_PATH)
        )
        run_shell_cmd(
            "echo {} > {}/rf/temp1_input".format(rf_temp, self.TEST_DATA_PATH)
        )
        run_shell_cmd(
            "echo {} > {}/upper_mac/temp1_input".format(upper_mac_temp, self.TEST_DATA_PATH)
        )
        run_shell_cmd(
            "echo {} > {}/lower_mac/temp1_input".format(lower_mac_temp, self.TEST_DATA_PATH)
        )

        # Wait for fans to change PWM
        time.sleep(10)
        return self.get_fan_pwm(pwm_val=PWM_VAL)


class FscdTestPwmAGC032a(FscdTest):
    brd_type = None

    def setUp(self):
        config_file = "fsc-config-agc032a.json"
        # Backup original config
        run_shell_cmd("cp /etc/fsc/zone-agc032a.fsc /etc/fsc/zone-agc032a.fsc.orig")
        # Overwrite fscd config
        run_shell_cmd(
            "cp {}/zone-agc032a.fsc /etc/fsc/zone-agc032a.fsc".format(super().TEST_DATA_PATH)
        )
        super().setUp(config=config_file, test_data_path=super().TEST_DATA_PATH)

    def tearDown(self):
        # Recover original config
        run_shell_cmd("mv /etc/fsc/zone-agc032a.fsc.orig /etc/fsc/zone-agc032a.fsc")
        super().tearDown()

    def test_fscd_inlet_28_duty_cycle_40(self):
        # sub-test1: pwm when all temp=28C pwm=102 => duty_cycle=40
        PWM_VAL = 40
        status, pwm_output = self.run_pwm_test(
            lf_temp=28000,
            rf_temp=28000,
            upper_mac_temp=28000,
            lower_mac_temp=28000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_39dot99_duty_cycle_40(self):
        # sub-test2: pwm when all temp<40C pwm=102 => duty_cycle=40
        PWM_VAL = 40
        status, pwm_output = self.run_pwm_test(
            lf_temp=39990,
            rf_temp=39990,
            upper_mac_temp=39990,
            lower_mac_temp=39990,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_41_duty_cycle_60(self):
        # sub-test3: pwm when all temp=41C pwm=153 => duty_cycle=60
        PWM_VAL = 60
        status, pwm_output = self.run_pwm_test(
            lf_temp=41000,
            rf_temp=41000,
            upper_mac_temp=41000,
            lower_mac_temp=41000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_49dot99_duty_cycle_60(self):
        # sub-test4: pwm when all temp<50C pwm=153 => duty_cycle=60
        PWM_VAL = 60
        status, pwm_output = self.run_pwm_test(
            lf_temp=49990,
            rf_temp=49990,
            upper_mac_temp=49990,
            lower_mac_temp=49990,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_51_duty_cycle_80(self):
        # sub-test5: pwm when all temp=51C pwm=204 => duty_cycle=80
        PWM_VAL = 80
        status, pwm_output = self.run_pwm_test(
            lf_temp=51000,
            rf_temp=51000,
            upper_mac_temp=51000,
            lower_mac_temp=51000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_59dot99_duty_cycle_80(self):
        # sub-test6: pwm when all temp<60C pwm=204 => duty_cycle=80
        PWM_VAL = 80
        status, pwm_output = self.run_pwm_test(
            lf_temp=59990,
            rf_temp=59990,
            upper_mac_temp=59990,
            lower_mac_temp=59990,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    #def test_fscd_inlet_61_duty_cycle_100(self):
        # sub-test7: pwm when all temp>=61C pwm=255 => duty_cycle=100
       # PWM_VAL = 100
      #  status, pwm_output = self.run_pwm_test(
        #   lf_temp=61000,
         #   rf_temp=61000,
        #    upper_mac_temp=61000,
        #    lower_mac_temp=61000,
        #    expected_pwm=PWM_VAL,
      #  )
      #  self.assertTrue(
        #    status,
       #     "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
       # )


