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
import re
from abc import abstractmethod
import pexpect
import subprocess

from utils.cit_logger import Logger
from utils.ssh_util import OpenBMCSSHSession
from utils.shell_util import run_shell_cmd


class BaseMESetPowerTest(object):
    DEFAULT_COMMAND_EXEC_DELAY = 1
    DEFAULT_COMMAND_PROMTP_TIMEOUT = 10
    DEFAULT_BMC_RECONNECT_TIMEOUT = 300
    DEFAULT_LIMIT_POWER = 90
    DEFAULT_SET_POWER_LIMIT_CMD = (
        "/usr/local/bin/me-util server --set_nm_power_policy {}".format(
            DEFAULT_LIMIT_POWER
        )
    )
    DEFAULT_GET_POWER_CMD = (
        "/usr/local/bin/me-util server --get_nm_power_statistics | grep Current"
    )
    DEFAULT_START_POWER_STRESS_CMD = "/home/ptugen -p 100 -b 1"
    DEFAULT_STOP_POWER_STRESS_CMD = "killall -9 ptugen"

    def setUp(self):
        self.is_log = True
        self.bmc_hostname = None
        self.set_ssh_session_bmc_hostname()
        self.bmc_ssh_session = None
        self.command_exec_delay = self.DEFAULT_COMMAND_EXEC_DELAY
        self.command_promtp_timeout = self.DEFAULT_COMMAND_PROMTP_TIMEOUT
        self.bmc_reconnect_timeout = self.DEFAULT_BMC_RECONNECT_TIMEOUT
        self.set_power_limit_cmd = self.DEFAULT_SET_POWER_LIMIT_CMD
        self.get_power_cmd = self.DEFAULT_GET_POWER_CMD
        self.start_power_stress_cmd = self.DEFAULT_START_POWER_STRESS_CMD
        self.stop_power_stress_cmd = self.DEFAULT_STOP_POWER_STRESS_CMD
        self.power_result = None
        Logger.start(name=self._testMethodName)

    @abstractmethod
    def set_ssh_session_bmc_hostname(self):
        pass

    def connect_to_bmc(self, logging=False):
        """
        method to open ssh session
        """
        if logging:
            print("\nConnecting to BMC ...")
        Logger.info("Connecting to BMC ...")
        
        try:
            self.bmc_ssh_session = OpenBMCSSHSession(self.bmc_hostname)
            self.bmc_ssh_session.connect()
            self.bmc_ssh_session.login()  # connect to BMC
            self.bmc_ssh_session.session.prompt(timeout=20)  # wait for prompt
            
            if logging:
                print("Done")
            return True
        except Exception as e:
            print(e)
            return False

    def is_connection_ready(self):
        """
        Check status of ssh session
        """
        ret = False
        if self.bmc_ssh_session.session.isalive():
            test_cmd = "ifconfig"
            ret = self.send_command_to_bmc(test_cmd)
        return ret

    def send_command_to_bmc(self, command, delay=None, prompt=True):
        if delay is None:
            delay = self.command_exec_delay
        self.bmc_ssh_session.session.sendline(command)  # send command
        time.sleep(delay)  # Delay for execute the command
        if prompt:
            return self.bmc_ssh_session.session.prompt(
                timeout=self.command_promtp_timeout
            )  # wait for prompt

    def wait_until(self, predicate, timeout):
        """
        helper method for waiting some process until it's finished
        or timeout's expired
        """
        end = time.time() + timeout
        while time.time() < end:
            if predicate():
                return True
            else:
                time.sleep(20)
        return False

    def reconnect_to_bmc(self, timeout=30, logging=False):
        """
        reconnect to BMC until the timeout is expired
        """
        if logging:
            print("Reconnecting to BMC ...")
        Logger.info("Reconnecting to BMC ...")
        
        if not self.wait_until(lambda: self.connect_to_bmc(), timeout):
            print("Failed")
            return False
        time.sleep(self.scm_boot_time)  # waiting for SCM to get ready
        if logging:
            print("Done")
        return True

    def set_power_limit(self, logging=False):
        """
        set power limit on BMC
        """
        if not self.bmc_ssh_session.session.isalive():
            if not self.reconnect_to_remote_host(30):
                self.fail("Cannot set power limit due to connection lost!")
        time.sleep(10)  # waiting for session to get ready
        
        if logging:
            print("Set power limit ...")
        Logger.info("Set power limit ...")

        self.send_command_to_bmc(self.set_power_limit_cmd)
        time.sleep(10)  # waiting for command to be successful
        if logging:
            print("Done")

    def get_power(self, logging=False):
        """
        get power on BMC
        """
        if not self.bmc_ssh_session.session.isalive():
            if not self.reconnect_to_remote_host(30):
                self.fail("Cannot get power due to connection lost!")
        time.sleep(10)  # waiting for session to get ready
        
        if logging:
            print("Get power ...")
        Logger.info("Get power ...")

        self.ret = self.send_command_to_bmc(self.get_power_cmd)
        time.sleep(10)  # waiting for command to be successful
        self.power_result = self.bmc_ssh_session.session.before

    def verify_me_set_power(self, logging=False):
        """
        Check power is limited
        """
        if logging:
            print("Check power ...")
        Logger.info("Check power ...")
        
        if re.search("{} Watts".format(self.DEFAULT_LIMIT_POWER), self.power_result):
            return True
        else:
            return False

    def test_me_set_nm_power(self):
        # Connect to BMC
        self.connect_to_bmc(logging=self.is_log)
        
        # Test connection
        if not self.is_connection_ready():
            if not self.reconnect_to_bmc(
                self.bmc_reconnect_timeout, logging=self.is_log
            ):
                self.fail("Cannot establish the connection to BMC!")
        
        # Set power limit on BMC
        self.set_power_limit(logging=self.is_log)
        
        # Run power stress
        if self.is_log:
            print("Run power stress ...")
        Logger.info("Run power stress ...")
        f = subprocess.Popen(
            self.start_power_stress_cmd,
            shell=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        time.sleep(10)
        
        # Get power result from BMC
        if f.poll() is None:
            self.get_power(logging=self.is_log)
        
        # Check power close ssh connection and stop power stress
        run_shell_cmd(self.stop_power_stress_cmd)
        self.bmc_ssh_session.session.close()
        self.assertTrue(self.verify_me_set_power(logging=self.is_log), "ME settting power is invalid!")
