#!/usr/bin/env python
#
# Copyright 2019-present Facebook. All Rights Reserved.
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
import unittest
from abc import abstractmethod

from utils.cit_logger import Logger


class BaseCpuUtilizationTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.cpu_utilization_cmd = None
        self.expected_cpu_utilization = None
        self.skip_processes = None
        self.number_of_retry = None
        self.result_threshold = None
        self.wait_time = None
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    def set_cpu_utilization_cmd(self):
        """
            we want to get two iteration of the top command with a 1
            sec delay between them.We are skipping the very first top
            iteration because it could the cpu consumption from
            the time the system last booted. Then we are using awk
            to make sure that we get the second iteration which
            starts after the line with Mem.
        """
        self.cpu_utilization_cmd = "top -bn 2 -d 1 | awk 'p>1; /^Mem/{++p}'"

    @abstractmethod
    def init_cpu_variables(self):
        pass

    def get_cpu_utilization(self):
        """
            Getting the data containing CPU consumption
        """
        self.set_cpu_utilization_cmd()
        self.assertNotEqual(
            self.cpu_utilization_cmd, None, "Command to get cpu utilization is not set"
        )
        Logger.info("Executing cmd={}".format(self.cpu_utilization_cmd))
        info = os.popen(self.cpu_utilization_cmd)
        return info

    def dumpAllCpuData(self, save_data_dbg, cpu_percentage_used):
        """
            Used to dump all the data in formatted user
            friendly way in case we need to debug this.
        """
        cpu_data = "\n"
        cpu_data += "\nCPU percentage after skipping the processes: {}".format(
            cpu_percentage_used
        )
        cpu_data += "\nExpected CPU consumption: {}\n".format(
            self.expected_cpu_utilization
        )
        for line in save_data_dbg:
            cpu_data += line
        return cpu_data

    def test_cpu_utilization(self):
        """
            This test is ran a specific number of time
            to make sure that we isolate for the posibility
            of the test being flaky. result_threshold is used
            to make sure that the test pass or fail
        """
        self.init_cpu_variables()
        self.skip_cpu_utilization_processes()
        retry = 0
        result = 0
        cpu_data = "\n"
        while retry < self.number_of_retry:
            cpu_value = self.compute_cpu_utilization()
            result += cpu_value[0]
            cpu_data += cpu_value[1]
            retry += 1
            # use a delay to allow processes to run on cpu
            time.sleep(self.wait_time)

        self.assertLess(result, self.result_threshold, cpu_data)

    def compute_cpu_utilization(self):
        """
            Used to test the cpu utilization and make
            sure it's not above the threshold
        """
        stream = self.get_cpu_utilization()
        self.assertNotEqual(
            self.expected_cpu_utilization, None, "Expected set of cpu data not set"
        )
        self.assertNotEqual(
            self.skip_processes, None, "processes to be skipped not set"
        )

        count = 0
        cpu_percentage_used = 0
        save_data_dbg = []  # Saving data for debuging purpose

        # Get the total cpu consumption and them subtract
        # it from the one of the processes cpu
        # consumption that is very unpredicatable
        for line in stream.readlines():
            save_data_dbg.append(line)
            data = line.split()
            if count == 0:
                # Add the usr, sys, and nic processes total consumption
                usr = int(re.sub("[^0-9]", "", data[1]))
                sys = int(re.sub("[^0-9]", "", data[3]))
                nic = int(re.sub("[^0-9]", "", data[5]))
                cpu_percentage_used = usr + sys + nic
                count += 1
            else:
                for proc in self.skip_processes:
                    if re.search(proc, line):
                        proc_cpu_consumption = int(re.sub("[^0-9]", "", data[6]))
                        cpu_percentage_used -= proc_cpu_consumption

        # Update the cpu data for debugging purpose
        cpu_data = self.dumpAllCpuData(save_data_dbg, cpu_percentage_used)

        cpu_consumption = 1
        if cpu_percentage_used < self.expected_cpu_utilization:
            cpu_consumption = 0
        return [cpu_consumption, cpu_data]
