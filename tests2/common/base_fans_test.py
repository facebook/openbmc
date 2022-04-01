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
import os
import re
import time
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseFansTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.read_fans_cmd = None
        self.write_fans_cmd = None
        self.kill_fan_ctrl_cmd = None
        self.start_fan_ctrl_cmd = None
        self.fans = None
        self.names = None
        self.pwms = None

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def get_fan_speed(self):
        pass

    @abstractmethod
    def set_fan_speed(self):
        pass

    def kill_fan_controller(self):
        """
        Helper method to kill fan controller: FSCD, fand
        """
        self.assertNotEqual(
            self.kill_fan_ctrl_cmd, None, "Kill Fan Controller cmd not set"
        )
        for cmd in self.kill_fan_ctrl_cmd:
            run_shell_cmd(cmd, ignore_err=True)

    def start_fan_controller(self):
        """
        Helper method to start fan controller: FSCD, fand
        """
        self.assertNotEqual(
            self.start_fan_ctrl_cmd, None, "Kill Fan Controller cmd not set"
        )
        for cmd in self.start_fan_ctrl_cmd:
            run_shell_cmd(cmd, ignore_err=True)
        time.sleep(10)  # allow time for fans to be properly set

    @abstractmethod
    def fan_read(self):
        """
        Test if fan read is returning sane data
        """
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        pass

    @abstractmethod
    def fan_set_and_read(self):
        """
        Test if fan set is setting pwm as expected
        """
        pass


class CommonShellBasedFansTest(BaseFansTest):
    def get_fan_speed(self, fan_id=None):
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        cmd = self.read_fans_cmd
        if fan_id:
            cmd = self.read_fans_cmd + " " + str(fan_id)
        Logger.debug("Executing: {}".format(cmd))
        return run_shell_cmd(cmd)

    def set_fan_speed(self, fan_id, pwm):
        """
        Helper function to set fan speed
        """
        self.assertNotEqual(self.write_fans_cmd, None, "Set Fan cmd not set")
        cmd = self.write_fans_cmd + " " + str(pwm) + " " + str(fan_id)
        Logger.debug("Executing: {}".format(cmd))
        run_shell_cmd(cmd)
        time.sleep(10)  # allow time for fans to be properly set

    def verify_get_fan_speed_output(self, info):
        """
        Parses get_fan_speed.sh output
        and test it is in the expected format
        Arguments: info = dump of get_fan_speed.sh
        """
        Logger.info("Parsing Fans dump: {}".format(info))
        info = info.split("\n")
        goodRead = ["Fan", "RPMs:", "%"]
        for line in info:
            if len(line) == 0:
                continue
            for word in goodRead:
                if word not in line:
                    return False
            val = line.split(":")[1].split(",")
            if len(val) != 3:
                return False
        return True

    def get_speed(self, info):
        """
        Given a dump of 1 fan rpm read extract speed
        """
        info = info.split(":")[1].split(",")
        pwm_str = re.sub("[^0-9]", "", info[2])
        return int(pwm_str)

    def test_all_fans_read(self):
        """
        Test if all fan dump is returning sane data
        """
        # Sleep needed to test and make sure that we won't run
        # into accessing sysfs path issue causing test to fail
        time.sleep(2)
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        data = self.get_fan_speed()
        Logger.info("Fans dump:\n" + data)
        self.assertTrue(self.verify_get_fan_speed_output(data), "Get fan speeds failed")

    def fan_read(self, fan=None):
        """
        Test if fan read for a specific fan is returning sane data
        """
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        data = self.get_fan_speed(fan_id=fan)
        Logger.info("Fan dump:\n" + data)
        self.assertTrue(self.verify_get_fan_speed_output(data), "Get fan speed failed")

    def fan_set_and_read(self, fan_id=None, pwm=None):
        """
        Test if fan set is setting pwm as expected to 0
        """
        self.assertNotEqual(pwm, None, "PWM needs to be set for testing")
        self.assertNotEqual(fan_id, None, "fan_id needs to be set for testing")

        # stop fan controller
        self.assertNotEqual(
            self.kill_fan_ctrl_cmd, None, "Kill Fan Controller cmd not set"
        )
        self.kill_fan_controller()

        # set fan speed
        self.assertNotEqual(self.write_fans_cmd, None, "Set Fan cmd not set")
        self.set_fan_speed(fan_id=fan_id, pwm=pwm)

        # read fan speed
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        data = self.get_fan_speed(fan_id=fan_id)
        Logger.info("Fans dump:\n" + data)

        # parse fan data and get pwm only
        speed = self.get_speed(data)
        self.assertAlmostEqual(
            speed, pwm, delta=5, msg="Speed was not set to {} pwm [FAILED]".format(pwm)
        )


class CommonFanUtilBasedFansTest(BaseFansTest):
    def get_fan_speed(self, fan_id=None):
        """
        Helper function to get fan speed
        """
        cmd = "/usr/local/bin/fan-util --get"
        if fan_id is not None:
            cmd = cmd + " " + str(fan_id)
        Logger.debug("Executing: {}".format(cmd))
        ret = run_shell_cmd(cmd)
        return ret

    def set_fan_speed(self, pwm_id, pwm):
        """
        Helper function to set fan speed
        """
        cmd = "/usr/local/bin/fan-util --set " + str(pwm) + " " + str(pwm_id)
        Logger.debug("Executing: {}".format(cmd))
        run_shell_cmd(cmd)
        time.sleep(10)  # allow time for fans to be properly set

    def set_all_fans_speed(self, pwm):
        """
        Helper function to set fan speed
        """
        cmd = "/usr/local/bin/fan-util --set " + str(pwm)
        Logger.debug("Executing: {}".format(cmd))
        run_shell_cmd(cmd)
        time.sleep(10)  # allow time for fans to be properly set

    def verify_get_fan_speed_output(self, info, fan=None):
        """
        Parses fan-util output
        and test it is in the expected format
        Arguments: info = dump of get_fan_speed.sh
        """
        self.assertNotEqual(self.fans, None, "Number of fans undefined")
        self.assertNotEqual(self.pwms, None, "Number of PWM undefined")
        self.assertNotEqual(self.names, None, "Names of fans not set")
        Logger.info("Parsing Fans dump: {}".format(info))
        info = info.split("\n")
        num = 0
        for line in info:
            m = re.match(r"(.+) Speed: (\d+) RPM \((\d+)%\)", line)
            if not m:
                if re.match(r"^\s*$", line):
                    continue
                attrs = line.split(": ")
                self.assertIn(
                    attrs[0],
                    [
                        "Fan Mode",
                        "Sensor Fail",
                        "Fan Fail",
                        "Sled Fan Latch Open",
                        "FSCD Driver",
                    ],
                    "Accepted printed attributes",
                )
                if attrs[0] == "Sensor Fail" and attrs[1] != "None":
                    self.assertEqual(
                        attrs[1],
                        "None",
                        "Sensor failure, fan-util output: {}".format(info),
                    )
                continue
            fan_name = m.group(1)
            self.assertIn(fan_name, self.names, "Unknown Fan")
            read_fan = self.names[fan_name]
            if fan is not None:
                self.assertEqual(read_fan, fan, "Incorrect fan read")
            else:
                self.assertEqual(read_fan, num, "Incorrect index of fan")
            num += 1
        self.assertLessEqual(num, len(self.fans), "Number of fans")
        self.assertGreater(num, 0, "At least one fan")
        if fan is None:
            self.assertEqual(num, len(self.fans), "Exact number of fans")
        return True

    def get_speed(self, info, fan_id=None):
        """
        Given a dump of 1 fan rpm read extract speed
        """
        out = info.splitlines()
        ret = []
        num = 0
        for line in out:
            m = re.match(r"(.+) Speed: (\d+) RPM \((\d+)%\)", line)
            if m is not None:
                self.assertIn(m.group(1), self.names, "Unknown fan name")
                if fan_id is not None:
                    self.assertEquals(fan_id, self.names[m.group(1)], "Bad fan index")
                else:
                    self.assertEquals(num, self.names[m.group(1)], "Bad fan index")
                ret.append(int(m.group(3)))
                num += 1
        return ret

    def test_all_fans_read(self):
        """
        Test if all fan dump is returning sane data
        """
        Logger.log_testname(self._testMethodName)
        data = self.get_fan_speed()
        Logger.info("Fans dump:\n" + data)
        self.assertTrue(self.verify_get_fan_speed_output(data), "Get fan speeds failed")

    def fan_read(self, fan=None):
        """
        Test if fan read for a specific fan is returning sane data
        """
        data = self.get_fan_speed(fan_id=fan)
        Logger.info("Fan dump:\n" + data)
        self.assertTrue(
            self.verify_get_fan_speed_output(data, fan), "Get fan speed failed"
        )

    def fan_set_and_read(self, pwm_id=None, fan_id=None, pwm=None):
        """
        Test if fan set is setting pwm as expected to 0
        """
        self.assertNotEqual(pwm, None, "PWM needs to be set for testing")
        self.assertNotEqual(fan_id, None, "fan_id needs to be set for testing")

        # stop fan controller
        self.assertNotEqual(
            self.kill_fan_ctrl_cmd, None, "Kill Fan Controller cmd not set"
        )
        self.kill_fan_controller()

        # set fan speed
        self.set_fan_speed(pwm_id=pwm_id, pwm=pwm)

        for fan in fan_id:
            with self.subTest(pwm_id=pwm_id, fan_id=fan):
                # read fan speed
                data = self.get_fan_speed(fan_id=fan)
                Logger.info("Fans dump:\n" + data)

                # parse fan data and get pwm only
                speed = self.get_speed(data, fan_id=fan)
                self.assertEqual(
                    len(speed), 1, msg="More than one fan speed was returned"
                )
                self.assertAlmostEqual(
                    speed[0],
                    pwm,
                    delta=5,
                    msg="Speed was not set to {} pwm [FAILED]".format(pwm),
                )

    def test_set_all_fans_and_read(self):
        """
        Test setting all fans speed and read to ensure it is the same
        """
        # stop fan controller
        self.assertNotEqual(
            self.kill_fan_ctrl_cmd, None, "Kill Fan Controller cmd not set"
        )
        self.kill_fan_controller()
        # set fan speed
        pwm = 75
        self.set_all_fans_speed(pwm=pwm)

        # read fan speed
        data = self.get_fan_speed()
        Logger.info("Fans dump:\n" + data)
        # parse fan data and get pwm only
        speed = self.get_speed(data)
        self.assertEqual(len(speed), len(self.fans), msg="Expected number of Fans")
        for s in speed:
            self.assertAlmostEqual(
                s, pwm, delta=5, msg="Speed was not set to {} pwm [FAILED]".format(pwm)
            )

    def test_fan_read(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.fans, None, "Fans must be defined")
        for fan in self.fans:
            with self.subTest(fan=fan):
                self.fan_read(fan=fan)

    def test_fan_pwm_set(self):
        """
        For each fan read and test speed
        """
        Logger.log_testname(self._testMethodName)
        self.assertNotEqual(self.fans, None, "Fans must be defined")
        for pwm_id in self.pwms:
            with self.subTest(pwm_id=pwm_id):
                self.fan_set_and_read(pwm_id=pwm_id, fan_id=self.pwms[pwm_id], pwm=40)
