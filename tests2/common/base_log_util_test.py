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


import json
import re
import subprocess
from abc import abstractmethod

from utils.cit_logger import Logger
from utils.shell_util import run_shell_cmd


class BaseLogUtilTest(object):
    """[summary] Base class for fw_util test
    To provide common function
    """

    def verify_output(self, cmd, str_wanted, msg=None):
        Logger.info("executing cmd = {}".format(cmd))
        out = run_shell_cmd(cmd)
        self.assertIn(str_wanted, out, msg=msg)

    @abstractmethod
    def set_platform(self):
        pass

    @abstractmethod
    def set_output_regex_mapping(self):
        pass

    def setUp(self):
        self.set_platform()
        self.set_output_regex_mapping()
        with open(
            "/usr/local/bin/tests2/tests/{}/test_data/logutil/info.json".format(
                self.platform
            )
        ) as f:
            self.info = json.load(f)
        Logger.start(name=self._testMethodName)

    def test_show_log(self):
        for fru in self.info.keys():
            cmd = ["log-util", fru, "--print"]
            with self.subTest("test show log {}".format(fru)):
                ret = subprocess.run(
                    cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
                )
                self.assertEqual(ret.returncode, 0)

    def test_show_log_json(self):
        for fru in self.info.keys():
            cmd = ["log-util", fru, "--json", "--print"]
            with self.subTest("test show log json {}".format(fru)):
                out = subprocess.run(
                    cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
                )
                Logger.info("CLI cmd: {}".format(cmd))
                Logger.info("CLI output: {}".format(out))
                # test if return == 0
                self.assertEqual(out.returncode, 0)
                if out.stdout:
                    out = json.loads(out.stdout.decode("utf-8"))
                    # if Logs is not empty, then we proceed to verify FRU name
                    if out["Logs"]:
                        self.assertEqual(out["Logs"][0]["FRU_NAME"], fru)

    def test_validate_log_format(self):
        for fru in self.info.keys():
            cmd = "log-util {} --print | head -n 1".format(fru)
            console_out = subprocess.run(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
            ).stdout.decode("utf-8")
            Logger.info("CLI cmd: {}".format(cmd))
            Logger.info("CLI output: {}".format(console_out))
            # only test if FRU name exist in output.
            # If log being cleared before, then nothing can be test
            if fru in console_out:
                out = [_out for _out in console_out.split("  ") if _out]
                for key, value in self.output_regex_mapping.items():
                    with self.subTest("validate log format {}".format(key)):
                        regex = value["regex"]
                        idx = value["idx"]
                        Logger.info("regex: {}".format(regex))
                        Logger.info("content: {}".format(out[idx]))
                        rx = re.compile(regex)
                        self.assertIsNone(rx.search(out[idx]))

    def test_validate_log_format_json(self):
        for fru in self.info.keys():
            cmd = "log-util {} --print --json".format(fru)
            console_out = json.loads(
                subprocess.run(
                    cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
                ).stdout.decode("utf-8")
            )
            Logger.info("CLI cmd: {}".format(" ".join(cmd)))
            Logger.info("CLI output: {}".format(console_out))
            # only test if Logs existed
            if console_out["Logs"]:
                # check the first logs
                _out = console_out["Logs"][0]
                for log_header, log_format in self.output_regex_mapping.items():
                    with self.subTest("validate log format json {}".format(log_header)):
                        regex = log_format["regex"]
                        Logger.info("regex: {}".format(regex))
                        Logger.info("content: {}".format(_out[log_header]))
                        rx = re.compile(regex)
                        self.assertIsNone(rx.search(_out[log_header]))
