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
from tests.wedge400.helper.libpal import pal_get_board_type


class FscdTest(BaseFscdTest, unittest.TestCase):

    TEST_DATA_PATH = "/usr/local/bin/tests2/tests/wedge400/test_data/fscd"
    DEFAULT_TEMP = 28000

    def power_host_on(self):
        retry = 5
        for num_retry in range(retry):
            # If host was already on, script takes care of just returning on
            cmd = "/usr/local/bin/wedge_power.sh on"
            data = run_shell_cmd(cmd)
            Logger.info(
                "[FSCD Testing] Try {} Executing cmd= [{}]".format(num_retry, cmd)
            )
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
            if abs(int(line[0]) - int(pwm_val)) < 2:
                continue
            else:
                return [False, data]
        return [True, None]

    def run_pwm_test(
        self,
        userver_temp=-DEFAULT_TEMP,
        switch_temp=DEFAULT_TEMP,
        inlet_temp=DEFAULT_TEMP,
        expected_pwm=30,
    ):

        PWM_VAL = expected_pwm
        Logger.info(
            "[FSCD Testing] Setting (userver={}C, switch={}C ,"
            "inlet={}C, expected pwm={})".format(
                int(userver_temp) / 1000,
                int(switch_temp) / 1000,
                int(inlet_temp) / 1000,
                int(expected_pwm),
            )
        )
        # Initialize PWM 30 for testing normal up curve in the next following steps
        run_shell_cmd(
            "echo {} > {}/inlet/temp1_input".format(20000, self.TEST_DATA_PATH)
        )
        # Wait for fans to change PWM
        time.sleep(20)

        run_shell_cmd(
            "echo {} > {}/userver/temp1_input".format(userver_temp, self.TEST_DATA_PATH)
        )
        run_shell_cmd(
            "echo {} > {}/switch/temp1_input".format(switch_temp, self.TEST_DATA_PATH)
        )
        run_shell_cmd(
            "echo {} > {}/inlet/temp1_input".format(inlet_temp, self.TEST_DATA_PATH)
        )

        # Wait for fans to change PWM
        time.sleep(60)
        return self.get_fan_pwm(pwm_val=PWM_VAL)


class FscdTestPwmWedge400(FscdTest):
    brd_type = None

    def setUp(self):
        self.brd_type = pal_get_board_type()
        if self.brd_type is None:
            self.fail("Get board type failed!")
        elif self.brd_type == "Wedge400":
            config_file = "fsc-config-wedge400.json"
            Logger.info(
                "[FSCD Testing] test fscd on {} platform with config file {}".format(
                    self.brd_type, config_file
                )
            )
        elif self.brd_type == "Wedge400C":
            Logger.info(
                "[FSCD Testing] skip Wedge400 fscd test on {} platform".format(
                    self.brd_type
                )
            )
            self.skipTest(
                "skip Wedge400 fscd test on {} platform".format(self.brd_type)
            )
        else:
            self.fail("Invalid platform!")

        # Backup original config
        run_shell_cmd("cp /etc/fsc/zone-w400.fsc /etc/fsc/zone-w400.fsc.orig")
        # Overwrite fscd config
        run_shell_cmd(
            "cp {}/zone-w400.fsc /etc/fsc/zone-w400.fsc".format(super().TEST_DATA_PATH)
        )
        super().setUp(config=config_file, test_data_path=super().TEST_DATA_PATH)

    def tearDown(self):
        # Recover original config
        run_shell_cmd("mv /etc/fsc/zone-w400.fsc.orig /etc/fsc/zone-w400.fsc")
        super().tearDown()

    def test_fscd_inlet_28_duty_cycle_30(self):
        # sub-test1: pwm when all temp=28C pwm=19 => duty_cycle=30
        PWM_VAL = 30
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=28000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_28dot99_duty_cycle_30(self):
        # sub-test2: pwm when all temp<29C pwm=19 => duty_cycle=30
        PWM_VAL = 30
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=28990,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_29_duty_cycle_35(self):
        # sub-test3: pwm when all temp~[29C,30C) to 33C pwm=22 => duty_cycle=35

        PWM_VAL = 35
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=29000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_30_duty_cycle_40(self):
        # sub-test3: pwm when all temp~[30C,31C) to 33C pwm=25 => duty_cycle=40
        PWM_VAL = 40
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=30000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_31_duty_cycle_45(self):
        # sub-test3: pwm when all temp~[31C,33C) to 33C pwm=28 => duty_cycle=45
        PWM_VAL = 45
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=31000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_33_duty_cycle_50(self):
        # sub-test3: pwm when all temp~[33C,34C) to 33C pwm=32 => duty_cycle=50
        PWM_VAL = 50
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=33000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_34_duty_cycle_55(self):
        # sub-test3: pwm when all temp~[34C,35C) pwm=35 => duty_cycle=55
        PWM_VAL = 55
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=34000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_35_duty_cycle_60(self):
        # sub-test3: pwm when all temp>=35C pwm=38 => duty_cycle=60
        PWM_VAL = 60
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=35000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )


class FscdTestPwmWedge400C(FscdTest):
    brd_type = None

    def setUp(self):
        self.brd_type = pal_get_board_type()
        if self.brd_type is None:
            self.fail("Get board type failed!")
        elif self.brd_type == "Wedge400":
            Logger.info(
                "[FSCD Testing] skip Wedge400C fscd test on {} platform".format(
                    self.brd_type
                )
            )
            self.skipTest(
                "skip Wedge400C fscd test on {} platform".format(self.brd_type)
            )
        elif self.brd_type == "Wedge400C":
            config_file = "fsc-config-wedge400c.json"
            Logger.info(
                "[FSCD Testing] test fscd on {} platform with config file {}".format(
                    self.brd_type, config_file
                )
            )
        else:
            self.fail("Invalid platform!")

        # Backup original config
        run_shell_cmd("cp /etc/fsc/zone-w400c.fsc /etc/fsc/zone-w400c.fsc.orig")
        # Overwrite fscd config
        run_shell_cmd(
            "cp {}/zone-w400c.fsc /etc/fsc/zone-w400c.fsc".format(
                super().TEST_DATA_PATH
            )
        )
        super().setUp(config=config_file, test_data_path=super().TEST_DATA_PATH)

    def tearDown(self):
        # Recover original config
        run_shell_cmd("mv /etc/fsc/zone-w400c.fsc.orig /etc/fsc/zone-w400c.fsc")
        super().tearDown()

    def test_fscd_inlet_29_duty_cycle_30(self):
        # sub-test1: pwm when all temp=29C pwm=19 => duty_cycle=30
        PWM_VAL = 30
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=29000,
            inlet_temp=29000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_29dot99_duty_cycle_30(self):
        # sub-test2: pwm when all temp<30C pwm=19 => duty_cycle=30
        PWM_VAL = 30
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=29000,
            inlet_temp=29990,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_30_duty_cycle_35(self):
        # sub-test3: pwm when all temp~[30C,31C) to 33C pwm=22 => duty_cycle=35
        PWM_VAL = 35
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=29000,
            inlet_temp=30000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_31_duty_cycle_40(self):
        # sub-test3: pwm when all temp~[31C,32C) to 33C pwm=25 => duty_cycle=40
        PWM_VAL = 40
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=29000,
            inlet_temp=31000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_32_duty_cycle_45(self):
        # sub-test3: pwm when all temp~[32C,33C) to 33C pwm=28 => duty_cycle=45
        PWM_VAL = 45
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=29000,
            inlet_temp=32000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_33_duty_cycle_50(self):
        # sub-test3: pwm when all temp~[33C,34C) to 33C pwm=32 => duty_cycle=50
        PWM_VAL = 50
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=29000,
            inlet_temp=33000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_34_duty_cycle_55(self):
        # sub-test3: pwm when all temp~[34C,35C) pwm=35 => duty_cycle=55
        PWM_VAL = 55
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=29000,
            inlet_temp=34000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_35_duty_cycle_60(self):
        # sub-test3: pwm when all temp>35C pwm=38 => duty_cycle=60
        PWM_VAL = 60
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=29000,
            inlet_temp=35000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )
