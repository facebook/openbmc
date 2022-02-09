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

    TEST_DATA_PATH = "/tmp/tests/minipack/unittests/fscd/test_data"

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

    def start_fscd(self):
        self.run_shell_cmd("sv start fscd")

    def stop_fscd(self):
        self.run_shell_cmd("sv stop fscd")
        self.run_shell_cmd("/usr/local/bin/wdtcli stop")

    def start_sensord(self):
        self.run_shell_cmd("sv start sensord")

    def stop_sensord(self):
        self.run_shell_cmd("sv stop sensord")

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
        time.sleep(10)

    def setup_for_test2(self):
        '''
        Series of tests that are driven by changing the fan RPM reads
        '''
        pass

    def tear_down_tests(self):
        MAIN_POWER = "/sys/bus/i2c/drivers/smbcpld/12-003e/cpld_in_p1220"
        self.stop_fscd()
        self.run_shell_cmd("/usr/local/bin/wedge_power.sh on")
        self.run_shell_cmd("cp /etc/fsc-config.json.orig /etc/fsc-config.json")
        self.run_shell_cmd("echo 0x1 > %s" % MAIN_POWER)
        self.start_sensord()
        time.sleep(30)
        self.start_fscd()

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

    def run_pwm_test1(self,
                     inlet_temp=DEFAULT_TEMP,
                     switch1_temp=DEFAULT_TEMP,
                     switch2_temp=DEFAULT_TEMP,
                     expected_pwm=16):

        PWM_VAL = expected_pwm
        print("[FSCD Testing] Setting (inlet={}C, switch1={}C ,"
              "switch2={}C)".format(
                    inlet_temp,
                    switch1_temp,
                    switch2_temp))
        self.run_shell_cmd("echo {} > /tmp/cache_store/scm_sensor6".format(
            inlet_temp))
        self.run_shell_cmd("echo {} > /tmp/cache_store/smb_sensor26".format(
            switch1_temp))
        self.run_shell_cmd("echo {} > /tmp/cache_store/smb_sensor27".format(
            switch2_temp))

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

    def run_pwm_test2(self, name, PWM_VAL):
        print("[FSCD Testing] %s testing..." % name)
        rc, output = self.get_fan_pwm(pwm_val=PWM_VAL)
        if rc:
            print("[FSCD Testing] {} testing,expected={}% [PASSED]".
                  format(name, PWM_VAL))
        else:
            print("[FSCD Testing] {} testing,"
                  "expected={}% received={}"
                  " [FAILED]".format(name, PWM_VAL, output))
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
        Setup: fsc-32/64-config-test1.json - reads data like original one.
        Just change the values where sensor-util reads from.
        1) Series of tests to verify PWM setting based on the fsc-config
        2) host action test for shutdown when a high temp event can occur
        3) host action test for shutdown when read failure can occur
        '''
        fcm_b = self.run_shell_cmd(
                "head -n1 /sys/class/i2c-adapter/i2c-72/72-0033/cpld_ver")
        fcm_t = self.run_shell_cmd(
                "head -n1 /sys/class/i2c-adapter/i2c-64/64-0033/cpld_ver")
        if "0x0" in fcm_b or "0x0" in fcm_t:
            self.setup_for_test(config="fsc-32-config-test1.json")
        else:
            self.setup_for_test(config="fsc-64-config-test1.json")
        rc = True
        time.sleep(20)
        # sub-test1: pwm when all temp=5C duty_cycle=25
        if not self.run_pwm_test1(inlet_temp=5,
                                 switch1_temp=5,
                                 switch2_temp=5,
                                 expected_pwm=25):
            rc = False

        # sub-test2: pwm when all temp=20C duty_cycle=25
        if not self.run_pwm_test1(inlet_temp=20,
                                 switch1_temp=20,
                                 switch2_temp=20,
                                 expected_pwm=25):
            rc = False

        # sub-test3: pwm when all temp=23C duty_cycle=28
        if not self.run_pwm_test1(inlet_temp=23,
                                 switch1_temp=23,
                                 switch2_temp=23,
                                 expected_pwm=28):
            rc = False

        # sub-test4: pwm when all temp=26C duty_cycle=31
        if not self.run_pwm_test1(inlet_temp=26,
                                 switch1_temp=26,
                                 switch2_temp=26,
                                 expected_pwm=31):
            rc = False

        # sub-test5: pwm when all temp=29C duty_cycle=34
        if not self.run_pwm_test1(inlet_temp=29,
                                 switch1_temp=29,
                                 switch2_temp=29,
                                 expected_pwm=34):
            rc = False

        # sub-test6: pwm when all temp=31C duty_cycle=38
        if not self.run_pwm_test1(inlet_temp=31,
                                 switch1_temp=31,
                                 switch2_temp=31,
                                 expected_pwm=38):
            rc = False

        # sub-test7: pwm when all temp=33C duty_cycle=41
        if not self.run_pwm_test1(inlet_temp=33,
                                 switch1_temp=33,
                                 switch2_temp=33,
                                 expected_pwm=41):
            rc = False

        # sub-test8: pwm when all temp=35C duty_cycle=44
        if not self.run_pwm_test1(inlet_temp=35,
                                 switch1_temp=35,
                                 switch2_temp=35,
                                 expected_pwm=44):
            rc = False

        # sub-test9: pwm when all temp=37C duty_cycle=47
        if not self.run_pwm_test1(inlet_temp=37,
                                 switch1_temp=37,
                                 switch2_temp=37,
                                 expected_pwm=47):
            rc = False

        # sub-test10: pwm when all temp=39C duty_cycle=50
        if not self.run_pwm_test1(inlet_temp=39,
                                 switch1_temp=39,
                                 switch2_temp=39,
                                 expected_pwm=50):
            rc = False

        # sub-test11: pwm when switch1 temp=100C duty_cycle=41
        if not self.run_pwm_test1(inlet_temp=20,
                                 switch1_temp=100,
                                 switch2_temp=20,
                                 expected_pwm=41):
            rc = False

        # sub-test12: pwm when switch2 temp=100C duty_cycle=41
        if not self.run_pwm_test1(inlet_temp=20,
                                 switch1_temp=20,
                                 switch2_temp=100,
                                 expected_pwm=41):
            rc = False

        # sub-test13: pwm when switch1 temp=29C duty_cycle=50
        if not self.run_pwm_test1(inlet_temp=20,
                                 switch1_temp=105,
                                 switch2_temp=20,
                                 expected_pwm=50):
            rc = False

        # sub-test14: pwm when switch2 temp=29C duty_cycle=50
        if not self.run_pwm_test1(inlet_temp=20,
                                 switch1_temp=20,
                                 switch2_temp=105,
                                 expected_pwm=50):
            rc = False

        # sub-test15: At this point the system should shutdown because
        # temperature limit reached
        # Wait for threshold cycle = ~20sec
        if not self.run_pwm_test1(inlet_temp=20,
                                 switch1_temp=115,
                                 switch2_temp=20,
                                 expected_pwm=50):
            rc = False

        print("[FSCD Testing] System should shutdown because temperature "
              "limit reached. Verifying...")
        time.sleep(70)
        if not self.is_userver_on():
            print("[FSCD Testing] Host action test: Expected userver to "
                  "power off due to high temperature event [PASSED]")
        else:
            print("[FSCD Testing] Host action test: Expected userver to "
                  "power off due to high temperature event [FAILED]")
            rc = False

        # sub-test16: Remove sensor read path which is equivalent to read
        # failure. Wait for threshold cycle = ~20sec.
        # Firstly recover to good state
        print("[FSCD Testing] Recovering system for next test...")
        self.run_pwm_test1(inlet_temp=39,
                          switch1_temp=39,
                          switch2_temp=39,
                          expected_pwm=50)
        self.run_shell_cmd("/usr/local/bin/wedge_power.sh on")
        time.sleep(10)
        if self.is_userver_on():
            print("[FSCD Testing] Ready for next test...")
        else:
            print("[FSCD Testing] Unable to recover to good state")
            return False

        self.run_shell_cmd("rm /tmp/cache_store/scm_sensor6")
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
        fan dead test
        Setup: fsc-32/64-config-test2.json - reads sensors temperature from
        sensor-util but reads RPMs and PWMs from test-data.
        1) fan_dead_boost test for one or more fans are broken
        '''
        fcm_b = self.run_shell_cmd(
                "head -n1 /sys/class/i2c-adapter/i2c-72/72-0033/cpld_ver")
        fcm_t = self.run_shell_cmd(
                "head -n1 /sys/class/i2c-adapter/i2c-64/64-0033/cpld_ver")
        if "0x0" in fcm_b or "0x0" in fcm_t:
            self.setup_for_test(config="fsc-32-config-test2.json")
        else:
            self.setup_for_test(config="fsc-64-config-test2.json")
        rc = True
        time.sleep(30)

        # sub-test1: setup temp environment to make sure fan speed raises
        #identical value every time.
        print("[FSCD Testing] Setup temp environment to set pwm as 25%")
        if not self.run_pwm_test1(inlet_temp=20,
                                 switch1_temp=20,
                                 switch2_temp=20,
                                 expected_pwm=25):
            rc = False

        # sub-test1: One fan dead testing
        PWM_VAL = 38
        path = "/tmp/tests/minipack/unittests/fscd/test_data/fan1_input"
        self.run_shell_cmd("echo 0 > {}".format(path))
        path = "/tmp/tests/minipack/unittests/fscd/test_data/fan2_input"
        self.run_shell_cmd("echo 0 > {}".format(path))
        time.sleep(30)
        if not self.run_pwm_test2("One fan dead", PWM_VAL):
            rc = False

        # sub-test2: Two fans dead testing
        PWM_VAL = 63
        path = "/tmp/tests/minipack/unittests/fscd/test_data/fan3_input"
        self.run_shell_cmd("echo 0 > {}".format(path))
        path = "/tmp/tests/minipack/unittests/fscd/test_data/fan4_input"
        self.run_shell_cmd("echo 0 > {}".format(path))
        time.sleep(30)

        if not self.run_pwm_test2("Two fans dead", PWM_VAL):
            rc = False

        return rc


if __name__ == '__main__':
        '''
        test is run on target python <testfile>
        '''
        test = fscdTest()
        try:
            test.stop_sensord()
            time.sleep(20)
            result1 = test.run_test1()
            test.tear_down_tests()
            test.stop_sensord()
            result2 = test.run_test2()
            if result2:
                print("FSCD Testing [PASSED]")
                test.tear_down_tests()
                exit(0)
            print("FSCD Testing [FAILED]")
            test.tear_down_tests()
            exit(1)
        except Exception as e:
            print("FSCD Testing raised exception: {} [FAILED]".format(e))
            test.tear_down_tests()
