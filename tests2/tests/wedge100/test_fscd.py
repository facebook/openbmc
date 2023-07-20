#!/usr/bin/env python3
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
from utils.test_utils import qemu_check, tests_dir


class FscdTest(BaseFscdTest, unittest.TestCase):
    TEST_DATA_PATH = None
    DEFAULT_TEMP = 23000

    def setUp(self, config=None, test_data_path=None):
        self.TEST_DATA_PATH = "{}/wedge100/test_data/fscd".format(tests_dir())
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
            line = line[1].split("%")
            if int(line[0]) == int(pwm_val):
                continue
            else:
                return [False, data]
        return [True, None]

    def run_pwm_test(
        self,
        userver_temp=DEFAULT_TEMP,
        switch_temp=DEFAULT_TEMP,
        intake_temp=DEFAULT_TEMP,
        outlet_temp=DEFAULT_TEMP,
        expected_pwm=28,
    ):

        PWM_VAL = expected_pwm
        Logger.info(
            "[FSCD Testing] Setting (userver={}C, switch={}C ,"
            "intake={}C, outlet={}C, expected pwm={})".format(
                int(userver_temp) / 1000,
                int(switch_temp) / 1000,
                int(intake_temp) / 1000,
                int(outlet_temp) / 1000,
                int(expected_pwm),
            )
        )
        run_shell_cmd(
            "echo {} > {}/userver/temp1_input".format(userver_temp, self.TEST_DATA_PATH)
        )
        run_shell_cmd(
            "echo {} > {}/switch/temp1_input".format(switch_temp, self.TEST_DATA_PATH)
        )
        run_shell_cmd(
            "echo {} > {}/intake/temp1_input".format(intake_temp, self.TEST_DATA_PATH)
        )
        run_shell_cmd(
            "echo {} > {}/outlet/temp1_input".format(outlet_temp, self.TEST_DATA_PATH)
        )

        # Wait for fans to change PWM
        time.sleep(10)
        return self.get_fan_pwm(pwm_val=PWM_VAL)


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class FscdTestPwm(FscdTest):
    TEST_CONFIG_PATH = "{}/wedge100/test_data/fscd".format(tests_dir())

    def setUp(self):
        super().setUp(
            config="fsc-config-test1.json", test_data_path=self.TEST_CONFIG_PATH
        )

    def test_fscd_intake_23_duty_cycle_38(self):
        # sub-test1: pwm when all temp=23C pwm=12 => duty_cycle=38
        PWM_VAL = 38
        status, pwm_output = self.run_pwm_test(
            userver_temp=23000,
            switch_temp=23000,
            intake_temp=23000,
            outlet_temp=23000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_intake_28_duty_cycle_38(self):
        # sub-test2: pwm when all temp<30C pwm=12 => duty_cycle=38
        PWM_VAL = 38
        status, pwm_output = self.run_pwm_test(
            userver_temp=23000,
            switch_temp=23000,
            intake_temp=28000,
            outlet_temp=23000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_intake_33_duty_cycle_38(self):
        # sub-test3: pwm when all temp~[20C,33C] to 33C pwm=12 => duty_cycle=38
        PWM_VAL = 38
        status, pwm_output = self.run_pwm_test(
            userver_temp=30000,
            switch_temp=32000,
            intake_temp=33000,
            outlet_temp=30000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_intake_39_duty_cycle_51(self):
        # sub-test5: pwm when all temp~[38C] pwm=16 => duty_cycle=51
        PWM_VAL = 38
        status, pwm_output = self.run_pwm_test(
            userver_temp=32000,
            switch_temp=32000,
            intake_temp=39000,
            outlet_temp=32000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_intake_41_duty_cycle_51(self):
        # sub-test4: pwm when all temp~[30C,35C] pwm=16 => duty_cycle=51
        PWM_VAL = 51
        status, pwm_output = self.run_pwm_test(
            userver_temp=32000,
            switch_temp=32000,
            intake_temp=41000,
            outlet_temp=32000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_intake_42_duty_cycle_51(self):
        # sub-test5: pwm when all temp~[32C,42C] pwm=16 => duty_cycle=51
        PWM_VAL = 51
        status, pwm_output = self.run_pwm_test(
            userver_temp=32000,
            switch_temp=38000,
            intake_temp=42000,
            outlet_temp=32000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_intake_50_duty_cycle_51(self):
        # sub-test6: pwm when all temp~[40C,50C] pwm=16 => duty_cycle=51
        PWM_VAL = 51
        status, pwm_output = self.run_pwm_test(
            userver_temp=40000,
            switch_temp=41000,
            intake_temp=50000,
            outlet_temp=40000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but" " received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_intake_85_duty_cycle_74(self):
        # sub-test6: pwm when all temp~[40C,50C] pwm=16 => duty_cycle=51
        PWM_VAL = 74
        status, pwm_output = self.run_pwm_test(
            userver_temp=46000,
            switch_temp=47000,
            intake_temp=85000,
            outlet_temp=46000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " " received {}".format(PWM_VAL, pwm_output),
        )
