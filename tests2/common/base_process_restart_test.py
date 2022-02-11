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

import re
import time
from abc import abstractmethod
from typing import Dict

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseProcessRestartTest(object):
    def setUp(self):
        Logger.start(name=self._testMethodName)
        self.expected_process = None
        # Number of seconds till PIDs are polled again
        self.num_sec = 10
        # Number of times PIDs are polled
        self.num_iter = 10
        pass

    def tearDown(self):
        Logger.info("Finished logging for {}".format(self._testMethodName))
        pass

    @abstractmethod
    def set_expected_process(self):
        pass

    @abstractmethod
    def set_num_sec(self):
        pass

    @abstractmethod
    def set_num_iter(self):
        pass

    def get_processes(self) -> Dict[str, int]:
        processes = {}
        stream = run_shell_cmd("ps")
        for line in stream.splitlines():
            for process_name in self.expected_process:
                process_regex = r".*" + re.escape(process_name) + r".*"
                if re.search(process_regex, line):
                    process_details = line.split()
                    process_pid = process_details[0]
                    processes[process_name] = process_pid
        self.assertGreater(len(processes), 0, "No expected processes running")
        return processes

    def compare_pids(self, process_name: str, original_pid: int, new_pid: int) -> int:
        if new_pid != original_pid:
            Logger.info(
                "PID changed in {}: original pid={}; new pid = {}".format(
                    process_name, original_pid, new_pid
                )
            )
            return 1
        else:
            return 0

    def test_process_restarts(self):
        self.set_expected_process()
        self.assertNotEqual(self.expected_process, None, "Expected process not set")
        original_processes = self.get_processes()
        restart_count = {}
        for key in original_processes:
            restart_count[key] = 0
        now = time.time()
        for _i in range(self.num_iter):
            time.sleep(self.num_sec)
            new_processes = self.get_processes()
            for key in new_processes:
                with self.subTest(key=key):
                    restart_count[key] += self.compare_pids(
                        key, int(original_processes[key]), int(new_processes[key])
                    )
                    self.assertEqual(
                        new_processes[key],
                        original_processes[key],
                        "Process {} restarted {} times in {} seconds".format(
                            key, restart_count[key], (time.time() - now)
                        ),
                    )
            original_processes = new_processes
