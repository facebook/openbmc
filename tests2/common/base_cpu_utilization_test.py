#!/usr/bin/env python3
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
import pathlib
import re
import time
import unittest
from abc import abstractmethod
from typing import List, Tuple

from utils.cit_logger import Logger


def get_system_cpu_usage() -> Tuple[int, int]:
    """Returns a snapshot of the system CPU usage at the time of call
    (ticks since boot). Returns a tuple (used, total), where used is the
    ticks used directly or indirectly by the user (user time + user time of
    low priority processes (nice) + system (kernel) time caused by the above)
    """
    with open("/proc/stat") as f:
        for line in f:
            if line.startswith("cpu "):
                values = [
                    int(x) for x in line.split()[1:9]
                ]  # (usr, nic, sys, idle, iowait, irq, softirq, steal)
                return (
                    values[0] + values[1] + values[2],
                    sum(values),
                )  # (usr+nic+sys, total)


def get_proc_cpu_usage(pid: int) -> int:
    """Returns a stapshot of the CPU usage (user+system) for the specified pid.
    If the pid doesn't exist, returns 0.
    """
    try:
        with open("/proc/{}/stat".format(pid)) as f:
            line = f.read()
            fields = line.split()
            (usr, sys) = (int(fields[14]), int(fields[15]))
            return usr + sys
    except OSError:
        return 0


def get_pids_for_cmdlines(cmdline_regexes: List[str]) -> List[int]:
    """Returns a list of pids corresponding to processes whose command line
    matches at least one of the items in the `cmdlines` list.
    """
    regex = "|".join(cmdline_regexes)
    pids = []
    for subdir in pathlib.Path("/proc").iterdir():
        if not subdir.is_dir() or not re.fullmatch("[0-9]+", subdir.name):
            continue
        try:
            cmdline = (
                subdir.joinpath("cmdline").read_text().rstrip("\0").replace("\0", " ")
            )
        except OSError:
            cmdline = ""
        if re.search(regex, cmdline):
            pids.append(int(subdir.name))
    return pids


class BaseCpuUtilizationTest(unittest.TestCase):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.cpu_utilization_cmd = None
        self.expected_cpu_utilization = None
        self.skip_processes = None
        self.number_of_retries = 10
        self.result_threshold = 3
        self.wait_time = 5

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))

    @abstractmethod
    def init_cpu_variables(self):
        pass

    @abstractmethod
    def skip_cpu_utilization_processes(self):
        pass

    def test_cpu_utilization(self):
        """Test that CPU utilization is below a threshold.

        This test is run a specific number of times
        to make sure that we isolate for the posibility
        of the test being flaky. At least result_threshold runs
        (out of number_of_retries) should show a utilization below
        the threshold.
        """

        self.init_cpu_variables()
        self.assertNotEqual(
            self.expected_cpu_utilization, None, "CPU utilization threshold not set"
        )
        self.skip_cpu_utilization_processes()

        # We run compute_cpu_utilization number_of_retries times and
        # the test passes if in at least result_threshold runs the
        # CPU utilization is below the threshold (expected_cpu_utilization)
        good_runs = 0
        cpu_data = []
        for _ in range(self.number_of_retries):
            (percentage_used_minus_procs, percentage_used) = (
                self.compute_cpu_utilization()
            )
            if percentage_used_minus_procs < self.expected_cpu_utilization:
                good_runs += 1
            if good_runs >= self.result_threshold:
                return
            cpu_data += [(percentage_used_minus_procs, percentage_used)]

        debug_info = (
            "Expected CPU utilization <= {}%\n"
            "Actual utilizations for each run (without excluded processes "
            "(used in test), with excluded processes (for info)):"
        ).format(self.expected_cpu_utilization)

        debug_info += "\n".join(
            "{}%   {}%".format(p, p_with_excluded) for (p, p_with_excluded) in cpu_data
        )
        self.assertGreaterEqual(good_runs, self.result_threshold, debug_info)

    def compute_cpu_utilization(self):
        """Computes the CPU utilization in percents. Returns a tuple
        (actual_percentage_minus_excluded processes, actual_percentage).
        """

        # Get list of excluded PIDs
        pids = get_pids_for_cmdlines(self.skip_processes) + [os.getpid()]
        # Capture usage of these PIDs (from their start to now)
        old_proc_usages = [get_proc_cpu_usage(pid) for pid in pids]
        # Capture system usage (from boot to now)
        (old_used, old_total) = get_system_cpu_usage()

        # Sleep for a duration during which we'll calculate CPU usage
        time.sleep(self.wait_time)

        # Capture system usage after sleep
        (used, total) = get_system_cpu_usage()
        # Capture process usage after sleep
        proc_usages = [get_proc_cpu_usage(pid) for pid in pids]

        # Calculate used percentage
        diff_used = used - old_used  # Ticks used by all processes, including in kernel
        diff_total = total - old_total  # Total ticks, including idle time
        # Ticks used by excluded processes (if >= condition is to protect from
        # processes that didn't exist at either the first or second measurement
        diff_procs = sum(
            proc_usages[i] - old_proc_usages[i]
            for i in range(len(pids))
            if proc_usages[i] >= old_proc_usages[i]
        )
        # CPU usage percentage for all processes
        percentage_used = 100 * diff_used // diff_total
        # CPU usage percentage for non-excluded processes <- the interesting value
        percentage_used_minus_procs = 100 * (diff_used - diff_procs) // diff_total

        return (percentage_used_minus_procs, percentage_used)
