#!/usr/bin/env python3
#
#
# Copyright 2018-present Facebook. All Rights Reserved.
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
from utils.test_utils import fscd_config_dir, fscd_test_data_dir


class FscdTest(BaseFscdTest, unittest.TestCase):
    TEST_DATA_PATH = None
    DEFAULT_TEMP = 50000

    def setUp(self, config=None, test_data_path=None):
        self.TEST_DATA_PATH = "{}/yamp/test_data/fscd".format(
            fscd_test_data_dir("yamp")
        )
        super().setUp(config, test_data_path)

    def power_host_on(self):
        retry = 5
        for i in range(retry):
            # If host was already on, script takes care of just returning on
            cmd = "/usr/local/bin/wedge_power.sh on -f"
            data = run_shell_cmd(cmd)
            Logger.info("[FSCD Testing] Try {} Executing cmd= [{}]".format(i, cmd))
            Logger.info("[FSCD Testing] Received data= [{}]".format(data))
            time.sleep(5)
            if self.is_host_on():
                return
        self.assertTrue(
            self.is_host_on(),
            "[FSCD Testing] Retry for {} times"
            " and host failed to power on".format(retry),
        )

    def is_host_on(self):
        """
        Method to test if host power is on
        """
        status = False
        cmd = "/usr/local/bin/wedge_power.sh status"
        data = run_shell_cmd(cmd)
        Logger.info("[FSCD Testing] Executing cmd= [{}]".format(cmd))
        Logger.info("[FSCD Testing] Received data= [{}]".format(data))
        if "on" in data:
            status = True
        Logger.info("[FSCD Testing] userver power status {}".format(status))
        return status

    def get_fan_pwm(self, pwm_val=None, delta_val=None):
        """
        In yamp there is only 1 zone and all fans belong to it.
        PWM effect applies to all fans. Test if all fans report
        the expected PWM
        """
        self.assertNotEqual(pwm_val, None, "Expected PWM value needs to be set")
        self.assertNotEqual(delta_val, None, "Expected delta value needs to be set")

        data = run_shell_cmd("/usr/local/bin/get_fan_speed.sh")
        data = data.split("\n")
        for line in data:
            if len(line) == 0:
                continue
            line = line.split("(")
            line = line[1].split("%")
            print("line[0] = {}".format(line[0]))
            updated_pwm = int(line[0])
            if abs(pwm_val - updated_pwm) <= delta_val:
                continue
            else:
                return [False, data]
        return [True, None]

    def run_pwm_test(
        self, inlet_temp=DEFAULT_TEMP, th3_temp=DEFAULT_TEMP, expected_pwm=28, delta=3
    ):

        PWM_VAL = expected_pwm
        DELTA_VAL = delta
        Logger.info(
            "[FSCD Testing] Setting (inlet={}C, th3={}C ,"
            "expected pwm={})".format(
                int(inlet_temp) / 1000, int(th3_temp) / 1000, int(expected_pwm)
            )
        )

        # Wait for fans to change PWM
        time.sleep(20)

        run_shell_cmd(
            "echo {} > {}/inlet/temp2_input".format(inlet_temp, self.TEST_DATA_PATH)
        )
        run_shell_cmd(
            "echo {} > {}/th3/temp2_input".format(th3_temp, self.TEST_DATA_PATH)
        )

        print(
            "expected = {} inlet = {} th3 = {}".format(
                expected_pwm, inlet_temp, th3_temp
            )
        )

        # Wait for fans to change PWM
        time.sleep(25)

        return self.get_fan_pwm(pwm_val=PWM_VAL, delta_val=DELTA_VAL)


class FscdTestPwm(FscdTest):
    TEST_CONFIG_PATH = "{}/yamp/test_data/fscd".format(fscd_config_dir())

    def setUp(self):
        super().setUp(
            config="fsc-config-test.json", test_data_path=self.TEST_CONFIG_PATH
        )

    def test_fscd_inlet_30_th3_15_duty_cycle_35(self):
        # sub-test1: pwm when all temp <= 30 pwm=89 => duty_cycle=35
        PWM_VAL = 35
        status, pwm_output = self.run_pwm_test(
            inlet_temp=30000, th3_temp=15000, expected_pwm=PWM_VAL, delta=5
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " " received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_24_th3_46_duty_cycle_19(self):
        # sub-test2: pwm when all temp<50C pwm=48 => duty_cycle=19
        PWM_VAL = 19
        status, pwm_output = self.run_pwm_test(
            inlet_temp=24000, th3_temp=46000, expected_pwm=PWM_VAL, delta=5
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_34_th3_62_duty_cycle_56(self):
        # sub-test3: pwm when all temp~[30C,62C] to 33C pwm=144 => duty_cycle=56
        PWM_VAL = 56
        status, pwm_output = self.run_pwm_test(
            inlet_temp=34000, th3_temp=62000, expected_pwm=PWM_VAL, delta=5
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_20_th3_11_duty_cycle_13(self):
        # sub-test5: pwm when all temp~[20C ~ 11C] pwm=32 => duty_cycle=13
        PWM_VAL = 13
        status, pwm_output = self.run_pwm_test(
            inlet_temp=20000, th3_temp=11000, expected_pwm=PWM_VAL, delta=5
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_29_th3_32_duty_cycle_28(self):
        # sub-test4: pwm when all temp~[29C,32C] pwm=72 => duty_cycle=28
        PWM_VAL = 28
        status, pwm_output = self.run_pwm_test(
            inlet_temp=29000, th3_temp=32000, expected_pwm=PWM_VAL, delta=5
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_32_th3_38_duty_cycle_45(self):
        # sub-test5: pwm when all temp~[32C,38C] pwm=114 => duty_cycle=45
        PWM_VAL = 45
        status, pwm_output = self.run_pwm_test(
            inlet_temp=32000, th3_temp=38000, expected_pwm=PWM_VAL, delta=5
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_36_th3_41_duty_cycle_73(self):
        # sub-test6: pwm when all temp~[36C,41C] pwm=185 => duty_cycle=73
        PWM_VAL = 73
        status, pwm_output = self.run_pwm_test(
            inlet_temp=36000, th3_temp=41000, expected_pwm=PWM_VAL, delta=5
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_38_th3_80_duty_cycle_73(self):
        # sub-test6: pwm when all temp~[38C,80C] pwm=185 => duty_cycle=73
        PWM_VAL = 73
        status, pwm_output = self.run_pwm_test(
            inlet_temp=38000, th3_temp=80000, expected_pwm=PWM_VAL, delta=5
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )
