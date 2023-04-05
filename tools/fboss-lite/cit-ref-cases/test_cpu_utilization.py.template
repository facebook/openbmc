#!/usr/bin/env python3
#
# Copyright 2022-present Facebook. All Rights Reserved.
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

from common.base_cpu_utilization_test import BaseCpuUtilizationTest


class CpuUtilizationTest(BaseCpuUtilizationTest):
    def init_cpu_variables(self):
        """
        It is noticeable that the sys cpu consumption goes up
        quite high where it varies a lot ,so we are setting
        the expected cpu consumption to 60 (several processes
        that tend to consume lots of cpu will be skipped). A
        retry of 10 times  will be used to isolate flaky test.
        result_threshold is set to 8 because we will target 80% of failure.
        """
        self.expected_cpu_utilization = 75
        self.number_of_retry = 10
        self.result_threshold = 8
        self.wait_time = 5

    def skip_cpu_utilization_processes(self):
        """
        we will define a list of processes here that we
        want to skip. Those tend to be proccesses that we need
        which consume a large amount of cpu consumption, or where
        the cpu consumption varies a lot.
        """
        self.skip_processes = [
            "u-fb",
            "top",
            "awk",
            "rotor_client",
            "rsyslogd",
        ]
