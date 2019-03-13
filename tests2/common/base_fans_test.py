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
import unittest
import time
import re
from abc import abstractmethod
from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd

class BaseFansTest(unittest.TestCase):

    def setUp(self):
        self.start_logging(__name__)
        self.read_fans_cmd = None
        self.write_fans_cmd = None
        self.kill_fan_ctrl_cmd = None
        self.start_fan_ctrl_cmd = None

    def tearDown(self):
        pass

    def start_logging(self, name):
        Logger.start(name)

    @abstractmethod
    def get_fan_speed(self):
        pass

    @abstractmethod
    def set_fan_speed(self):
        pass

    def kill_fan_controller(self):
        '''
        Helper method to kill fan controller: FSCD, fand
        '''
        self.assertNotEqual(self.kill_fan_ctrl_cmd, None, "Kill Fan Controller cmd not set")
        for cmd in self.kill_fan_ctrl_cmd:
            run_shell_cmd(cmd)

    def start_fan_controller(self):
        '''
        Helper method to start fan controller: FSCD, fand
        '''
        self.assertNotEqual(self.start_fan_ctrl_cmd, None, "Kill Fan Controller cmd not set")
        for cmd in self.start_fan_ctrl_cmd:
            run_shell_cmd(cmd)
        time.sleep(10)  # allow time for fans to be properly set

    @abstractmethod
    def fan_read(self):
        '''
        Test if fan read is returning sane data
        '''
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        pass

    @abstractmethod
    def fan_set_and_read(self):
        '''
        Test if fan set is setting pwm as expected
        '''
        pass


class CommonShellBasedFansTest(BaseFansTest):

    def get_fan_speed(self, fan_id=None):
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        cmd = self.read_fans_cmd
        if fan_id:
            cmd = self.read_fans_cmd + " " + str(fan_id)
        Logger.debug("Executing: {}".format(cmd))
        return(run_shell_cmd(cmd))

    def set_fan_speed(self, fan_id, pwm):
        '''
        Helper function to set fan speed
        '''
        self.assertNotEqual(self.write_fans_cmd, None, "Set Fan cmd not set")
        cmd = self.write_fans_cmd + " " + str(pwm) + " " + str(fan_id)
        Logger.debug("Executing: {}".format(cmd))
        run_shell_cmd(cmd)
        time.sleep(10)  # allow time for fans to be properly set

    def verify_get_fan_speed_output(self, info):
        '''
        Parses get_fan_speed.sh output
        and test it is in the expected format
        Arguments: info = dump of get_fan_speed.sh
        '''
        Logger.info("Parsing Fans dump: {}".format(info))
        info = info.split('\n')
        goodRead = ['Fan', 'RPMs:', '%']
        for line in info:
            if len(line) == 0:
                continue
            for word in goodRead:
                if word not in line:
                    return False
            val = line.split(':')[1].split(',')
            if len(val) != 3:
                return False
        return True

    def get_speed(self, info):
        """
        Given a dump of 1 fan rpm read extract speed
        """
        info = info.split(':')[1].split(',')
        pwm_str = re.sub("[^0-9]", "", info[2])
        return int(pwm_str)

    def test_all_fans_read(self):
        '''
        Test if all fan dump is returning sane data
        '''
        Logger.log_testname(name=__name__)
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        data = self.get_fan_speed()
        Logger.info("Fans dump:\n" + data)
        self.assertTrue(self.verify_get_fan_speed_output(data),
            "Get fan speeds failed")

    def fan_read(self, fan=None):
        '''
        Test if fan read for a specific fan is returning sane data
        '''
        self.assertNotEqual(self.read_fans_cmd, None, "Get Fan cmd not set")
        data = self.get_fan_speed(fan_id=fan)
        Logger.info("Fan dump:\n" + data)
        self.assertTrue(self.verify_get_fan_speed_output(data),
            "Get fan speed failed")

    def fan_set_and_read(self, fan_id=None, pwm=None):
        '''
        Test if fan set is setting pwm as expected to 0
        '''
        self.assertNotEqual(pwm, None, "PWM needs to be set for testing")
        self.assertNotEqual(fan_id, None, "fan_id needs to be set for testing")

        # stop fan controller
        self.assertNotEqual(self.kill_fan_ctrl_cmd, None,
            "Kill Fan Controller cmd not set")
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
        self.assertAlmostEqual(speed, pwm, delta=5,
            msg="Speed was not set to {} pwm [FAILED]".format(pwm))
