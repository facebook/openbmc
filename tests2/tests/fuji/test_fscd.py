#!/usr/bin/env python3
#
# Copyright 2021-present Facebook. All Rights Reserved.
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
import re

from common.base_fscd_test import BaseFscdTest
from tests.fuji.helper.libpal import pal_get_board_rev, BoardRevision
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd
from utils.test_utils import qemu_check


@unittest.skipIf(qemu_check(), "test env is QEMU, skipped")
class FscdTest(BaseFscdTest, unittest.TestCase):

    TEST_DATA_PATH = "/usr/local/bin/tests2/tests/fuji/test_data/fscd"
    DEFAULT_TEMP = 26000

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
        In fuji there is only 1 zone and all fans belong to it.
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
        expected_pwm=40,
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
        # Initialize PWM 40 for testing normal up curve in the next following steps
        run_shell_cmd(
            "echo {} > {}/inlet/temp1_input".format(20000, self.TEST_DATA_PATH)
        )
        # Wait for fans to change PWM
        time.sleep(10)

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
        time.sleep(20)
        return self.get_fan_pwm(pwm_val=PWM_VAL)


class FscdTestPwmFuji(FscdTest):
    def setUp(self):
        self.brd_rev = pal_get_board_rev()
        if self.brd_rev is None:
            self.brd_rev = "Undefined" # This problem should be fixed in libpal after figuring out all the board revisions and their corresponding platforms.
            Logger.info(
                "[FSCD Testing] got undefined platform. Running test anyway."
            )
        if re.search(r'EVT[1-3]', self.brd_rev):
            Logger.info(
                "[FSCD Testing] skip fscd test on {} platform".format(self.brd_rev)
            )
            self.skipTest("skip fscd test on {} platform".format(self.brd_rev))
        else:
            config_file = "fsc-config.json"
            Logger.info(
                "[FSCD Testing] test fscd on {} platform with config file {}".format(
                    self.brd_rev, config_file
                )
            )


        # Backup original config
        run_shell_cmd("cp /etc/fsc/zone.fsc /etc/fsc/zone.fsc.orig")
        # Overwrite fscd config
        run_shell_cmd("cp {}/zone.fsc /etc/fsc/zone.fsc".format(super().TEST_DATA_PATH))
        super().setUp(config=config_file, test_data_path=super().TEST_DATA_PATH)

    def tearDown(self):
        # Recover original config
        run_shell_cmd("mv /etc/fsc/zone.fsc.orig /etc/fsc/zone.fsc")
        super().tearDown()

    def test_fscd_inlet_26_duty_cycle_40(self):
        # sub-test1: pwm when all temp=26C pwm=25 => duty_cycle=40
        PWM_VAL = 40
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=26000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_27_duty_cycle_40(self):
        # sub-test2: pwm when all temp~[27C,28C) pwm=25 => duty_cycle=40
        PWM_VAL = 40
        status, pwm_output = self.run_pwm_test(
            userver_temp=-68000,
            switch_temp=28000,
            inlet_temp=27000,
            expected_pwm=PWM_VAL,
        )
        self.assertTrue(
            status,
            "Expected {} for all fans but " "received {}".format(PWM_VAL, pwm_output),
        )

    def test_fscd_inlet_28_duty_cycle_45(self):
        # sub-test3: pwm when all temp~[28C,29C) pwm=28 => duty_cycle=45
        PWM_VAL = 45
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

    def test_fscd_inlet_29_duty_cycle_50(self):
        # sub-test4: pwm when all temp~[29C,30C) pwm=32 => duty_cycle=50
        PWM_VAL = 50
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

    def test_fscd_inlet_30_duty_cycle_55(self):
        # sub-test5: pwm when all temp>=30C pwm=35 => duty_cycle=55
        PWM_VAL = 55
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
