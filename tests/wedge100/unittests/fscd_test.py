#!/usr/bin/env python
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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import subprocess
import logging
import time

DEFAULT_TEMP = 23000


class fscdTest():

    TEST_DATA_PATH = "/tmp/tests/wedge100/unittests/fscd/test_data"

    def run_shell_cmd(self, cmd=None):
        if not cmd:
            return
        logging.debug("Executing {}".format(cmd))
        f = subprocess.Popen(cmd,
                             shell=True,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        info, err = f.communicate()
        # err = err.decode('utf-8')
        # if len(err) > 0:
        #    raise Exception(err + " [FAILED]")
        info = info.decode('utf-8')
        return info

    def restart_fscd(self):
        self.run_shell_cmd("sv restart fscd")

    def setup_for_test(self, config=None):
        '''
        Series of tests that are driven by changing the temperature sensors
        '''
        if config is None:
            exit(1)
        # Backup original config
        self.run_shell_cmd("cp /etc/fsc-config.json /etc/fsc-config.json.orig")
        # Copy test config to fsc-config and restart fsc

        self.run_shell_cmd("cp {}/{} /etc/fsc-config.json".
                           format(self.TEST_DATA_PATH, str(config)))
        self.restart_fscd()

    def setup_for_test2(self):
        '''
        Series of tests that are driven by changing the fan RPM reads
        '''
        pass

    def tear_down_tests(self):
        self.run_shell_cmd("/usr/local/bin/wedge_power.sh on")
        self.run_shell_cmd("cp /etc/fsc-config.json.orig /etc/fsc-config.json")
        self.restart_fscd()

    def get_fan_pwm(self, pwm_val=None):
        if pwm_val is None:
            logging.warning("[FSCD Testing] PWM not provided. Returning...")
            return [False, "No PWM"]

        data = self.run_shell_cmd("/usr/local/bin/get_fan_speed.sh")
        data = data.split('\n')
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

    def run_pwm_test(self,
                     userver_temp=DEFAULT_TEMP,
                     switch_temp=DEFAULT_TEMP,
                     intake_temp=DEFAULT_TEMP,
                     outlet_temp=DEFAULT_TEMP,
                     expected_pwm=28):

        PWM_VAL = expected_pwm
        print("[FSCD Testing] Setting (userver={}C, switch={}C ,"
              "intake={}C, outlet={}C)".format(
                    int(userver_temp)/1000,
                    int(switch_temp)/1000,
                    int(intake_temp)/1000,
                    int(outlet_temp)/1000))
        self.run_shell_cmd("echo {} > {}/userver/temp1_input".format(
            userver_temp, self.TEST_DATA_PATH))
        self.run_shell_cmd("echo {} > {}/switch/temp1_input".format(
            switch_temp, self.TEST_DATA_PATH))
        self.run_shell_cmd("echo {} > {}/intake/temp1_input".format(
            intake_temp, self.TEST_DATA_PATH))
        self.run_shell_cmd("echo {} > {}/outlet/temp1_input".format(
            outlet_temp, self.TEST_DATA_PATH))

        # Wait for fans to change PWM
        time.sleep(20)
        rc, output = self.get_fan_pwm(pwm_val=PWM_VAL)
        if rc:
            print("[FSCD Testing] PWM test,expected={}% [PASSED]".
                  format(PWM_VAL))
        else:
            print("[FSCD Testing] PWM test,expected={}% received={} [FAILED]".
                  format(PWM_VAL, output))
            return False
        return True

    def is_userver_on(self):
        data = self.run_shell_cmd("/usr/local/bin/wedge_power.sh status")
        if "on" in data:
            return True
        return False

    def run_test1(self):
        '''
        test1:
        Setup: fsc-config-test1.json - reads sensors temperature from test_data
        but reads RPMs and PWMs from HW.
        1) Series of tests to verify PWM setting based on the fsc-config
        2) host action test for shutdown when a high temp event can occur
        3) host action test for shutdown when read failure can occur
        '''
        self.setup_for_test(config="fsc-config-test1.json")
        rc = True

        # sub-test1: pwm when all temp=23C pwm=12 => duty_cycle=38
        if not self.run_pwm_test(userver_temp=23000,
                                 switch_temp=23000,
                                 intake_temp=23000,
                                 outlet_temp=23000,
                                 expected_pwm=38):
            rc = False

        # sub-test2: pwm when all temp<30C pwm=12 => duty_cycle=38
        if not self.run_pwm_test(userver_temp=23000,
                                 switch_temp=28000,
                                 intake_temp=28000,
                                 outlet_temp=23000,
                                 expected_pwm=38):
            rc = False

        # sub-test3: pwm when all temp~[20C,33C] to 33C pwm=12 => duty_cycle=38
        if not self.run_pwm_test(userver_temp=30000,
                                 switch_temp=32000,
                                 intake_temp=33000,
                                 outlet_temp=30000,
                                 expected_pwm=38):
            rc = False

        # sub-test4: pwm when all temp~[30C,35C] pwm=16 => duty_cycle=51
        if not self.run_pwm_test(userver_temp=32000,
                                 switch_temp=32000,
                                 intake_temp=41000,
                                 outlet_temp=32000,
                                 expected_pwm=51):
            rc = False

        # sub-test4: pwm when all temp~[38C] pwm=16 => duty_cycle=51
        if not self.run_pwm_test(userver_temp=32000,
                                 switch_temp=32000,
                                 intake_temp=39000,
                                 outlet_temp=32000,
                                 expected_pwm=51):
            rc = False

        # sub-test5: pwm when all temp~[32C,42C] pwm=16 => duty_cycle=51
        if not self.run_pwm_test(userver_temp=32000,
                                 switch_temp=38000,
                                 intake_temp=42000,
                                 outlet_temp=32000,
                                 expected_pwm=51):
            rc = False

        # sub-test6: pwm when all temp~[40C,50C] pwm=16 => duty_cycle=51
        if not self.run_pwm_test(userver_temp=40000,
                                 switch_temp=41000,
                                 intake_temp=50000,
                                 outlet_temp=40000,
                                 expected_pwm=51):
            rc = False

        # sub-test7: pwm when all temp~[46C,50C] pwm=23 => duty_cycle=74
        if not self.run_pwm_test(userver_temp=46000,
                                 switch_temp=47000,
                                 intake_temp=85000,
                                 outlet_temp=46000,
                                 expected_pwm=74):
            rc = False

        # sub-test8: At this point the system should shutdown because
        # temperature limit reached
        # Wait for threshold cycle = ~20sec
        print("[FSCD Testing] System should shutdown because temperature "
              " limit reached. Verifying...")
        time.sleep(70)
        if not self.is_userver_on():
            print("[FSCD Testing] Host action test: Expected userver to "
                  "power off due to high temperature event [PASSED]")
        else:
            print("[FSCD Testing] Host action test: Expected userver to "
                  "power off due to high temperature event [FAILED]")
            rc = False

        # sub-test9: Remove sensor read path which is equivalent to read
        # failure. Wait for threshold cycle = ~20sec.
        # Firstly recover to good state
        print("[FSCD Testing] Recovering system for next test...")
        self.run_pwm_test(userver_temp=40000,
                          switch_temp=41000,
                          intake_temp=44000,
                          outlet_temp=40000,
                          expected_pwm=51)
        self.run_shell_cmd("/usr/local/bin/wedge_power.sh on")
        time.sleep(10)
        if self.is_userver_on():
            print("[FSCD Testing] Ready for next test...")
        else:
            print("[FSCD Testing] Unable to recover to good state")
            return False

        self.run_shell_cmd("rm {}/intake/temp1_input".format(
                            self.TEST_DATA_PATH))
        time.sleep(70)
        if not self.is_userver_on():
            print("[FSCD Testing] Host action test: Expected userver to "
                  "power off due to read failure event [PASSED]")
        else:
            print("[FSCD Testing] Host action test: Expected userver to "
                  "power off due to read failure event [FAILED]")
            rc = False

        return rc

    def run_test2(self):
        '''
        test2:
        Setup: fsc-config-test2.json - reads sensors temperature from HW
        but reads RPMs and PWMs from test-data.
        1) fan_dead testing
        '''
        rc = True
        self.setup_for_test(config="fsc-config-test2.json")
        # Fan PWM boost to 100%
        time.sleep(30)
        pwmrc, output = self.get_fan_pwm(pwm_val=100)
        if pwmrc:
            print("[FSCD Testing] PWM test,expected=100% [PASSED]")
        else:
            print("[FSCD Testing] PWM test,expected=100% [FAILED]")
            rc = False
        # Wait for a few cycles for host action to be performed
        time.sleep(90)

        cmd = 'grep -c \'fscd: All fans report failed RPM not consistent with Fantray status 0x1f\' /var/log/messages'
        data = self.run_shell_cmd(cmd)
        if int(data) > 0:
            if self.is_userver_on():
                print("[FSCD Testing] Host action test: Expected userver to "
                      "power on due to fan RPM read failure event but"
                      " fantray status is GOOD [PASSED]")
            else:
                print("[FSCD Testing] Host action test: Expected userver to "
                      "power on due to fan RPM read failure event but"
                      " fantray status is GOOD [FAILED]")
                rc = False
        else:
            print("[FSCD Testing] Host action test: Expected userver to "
                  "power on due to fan RPM read failure event but"
                  " fantray status is GOOD [FAILED]")
            rc = False
        return rc


if __name__ == '__main__':
        '''
        test is run on target python <testfile>
        '''
        test = fscdTest()
        try:
            result1 = test.run_test1()
            test.tear_down_tests()
            # Diabling this test because with the new apprach we do
            # not shutdown userver for fan read failuers
            #result2 = test.run_test2()
            if result1:
                print("FSCD Testing [PASSED]")
                test.tear_down_tests()
                exit(0)
            print("FSCD Testing [FAILED]")
            test.tear_down_tests()
            exit(1)
        except Exception as e:
            print("FSCD Testing raised exception: {} [FAILED]".format(e))
            test.tear_down_tests()
